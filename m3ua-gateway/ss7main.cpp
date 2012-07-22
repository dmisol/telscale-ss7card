/* Written by Dmitri Soloviev <dmi3sol@gmail.com>
  
  http://opensigtran.org
  http://telestax.com
  
  GPL version 3 or later
*/

// argv[1]:
// ISDN: netw=xx
// SS7: links=x,opc=dddd,dpc=dddd,ni=dd(bb),sls=d
// argv[2] - (*) ip address for logging


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/io.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>



#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <bits/signum.h>
#include <syslog.h>

#include "defs.h"

#include "../shm_keys.h"

#include "ss7-mtp2/ss7link.h"
#include "ss7-mtp3-m3ua/mtp_m3ua.h"
#include "tcp_serv/tcpserver.h"
#include "sctp_serv/sctpserver.h"


#define VERSION	"===== JUST A TEST"

int execmsg=0;
int lastmsg=0;

int execproc = MAXPROCQTY;
int lastproc = 0;

PROCDATA **procqueue;
MSGDATA **msgqueue;

SS7LINK *ss7link;
SS7MTPM3UA *ss7mtp;
TCPSERVER *tcpserver;
SCTPSERVER *sctpserver;

TIMER GlobalTimer = 0;

/*SS7MESSAGE*/char *buffer;
SS7MESSAGE *freebuffer;
int memalloc;



int main(int argc,char *argv[])
{
  PROCDATA *proc;
  MSGDATA  *msg;
  SS7LINKSTARTUP *ss7linkstartup;
  SS7MTPM3UASTARTUP *ss7mtpstartup;
  TCPSERVERSTARTUP *tcpserverstartup;
  SCTPSERVERSTARTUP *sctpserverstartup;
  void *generalbuf;
  char *pos;
  
  SS7PROC *ss7proc;
  
  int opc=0,dpc=0,ni=0,links=1;
  int linkid = 0, appid = 0;
  int tcp = 0, sctp = 0;

  if(argc>1){
      if((pos=strstr(argv[1],"linkid="))) { sscanf(pos+7,"%d",&linkid); }       // shm key for the first link
      if((pos=strstr(argv[1],"appid="))) { sscanf(pos+6,"%d",&appid); }         // shm key for logging

      if((pos=strstr(argv[1],"links="))) { sscanf(pos+6,"%d",&links); }         // no of SS7 links
      
      if((pos=strstr(argv[1],"tcp="))) { sscanf(pos+4,"%d",&tcp); }             // tcp server port
      if((pos=strstr(argv[1],"sctp="))) { sscanf(pos+5,"%d",&sctp); }           // sctp server port
      
      if((pos=strstr(argv[1],"opc="))) { sscanf(pos+4,"%d",&opc); }             // own SPC
      if((pos=strstr(argv[1],"dpc="))) { sscanf(pos+4,"%d",&dpc); }             // another end of the link
      if((pos=strstr(argv[1],"ni=")))  {                                        // ni value
  	sscanf(pos+3,"%d",&ni); 
	switch(ni){ 	case 10: ni = 2; break;
			case 11: ni = 3; break;}}      
      }
  else exit(0);
  
  if((!tcp) && (!sctp)) sctp = DEFAULT_SCTP_PORT;
  
  log_shmem(KEY_LOG_APP0 + appid);
  //rf5shmem(KEY_TRACE);

  
  if(argc>3)    rf5shmem(inet_addr(argv[2]),atoi(argv[3]));
  else          rf5shmem(0,0);
  

  buffer = (char *)malloc(SS7BufSize * SS7MsgSize);
/* buffer for internal messages/events */
  msgqueue = (MSGDATA **) malloc ( ( MSGQUEUESIZE * sizeof(void *) ) );
  for(int i=0;i<MSGQUEUESIZE;i++)
	msgqueue[i] = (MSGDATA *) malloc( sizeof(MSGDATA) );
/* memory for process queue */
  procqueue = (PROCDATA **) malloc ( MAXPROCQTY * sizeof(void *) );

  memalloc = 0;
  freebuffer = (SS7MESSAGE *)&buffer[memalloc*SS7MsgSize];

  generalbuf = malloc(1024);



  
  
  
  lastproc = 0; 
  
  for(execproc = 0;execproc<links;execproc++){
	lastproc++;
	procqueue[execproc] = (PROCDATA *) malloc( sizeof(PROCDATA) );
	proc = procqueue[execproc];
	proc->classname = PROC_ITULINK;
	sprintf(proc->objectname,"link%d",execproc);
	
	
//	proc->classptr=malloc(sizeof(SS7LINK));
//	ss7link = (SS7LINK *) proc->classptr;
	ss7link = new SS7LINK();
	proc->classptr = ss7link;
	
	
	ss7linkstartup = (SS7LINKSTARTUP *) generalbuf;
	/* link initialization */
	ss7linkstartup->procdata = proc;
	ss7linkstartup->key = KEY_LINK0 + linkid + execproc;
	ss7linkstartup->linkset = 0;
	ss7linkstartup->link = execproc;//sls;
	ss7linkstartup->mtp = links;	// e.a. next one
	ss7link->debug = MANAGEMENT_TRAFFIC|ESSENTIAL_EVENTS;//ALL_EVENTS;//ESSENTIAL_EVENTS;//ESSENTIAL_TRAFFIC;//ALL_TRAFFIC
	if(ss7link->start( (void *) generalbuf )){
		log("link launching failed",0,0); exit(1);}
	else log("link launched",0,0);
  }
  
		lastproc++;
	procqueue[execproc] = (PROCDATA *) malloc( sizeof(PROCDATA) );
	proc = procqueue[execproc];
	proc->classname = ITUMTP;
	strcpy(proc->objectname,"mtp3ua");
	
//	proc->classptr=malloc(sizeof(SS7MTPM3UA));
//	ss7mtp = (SS7MTPM3UA *) proc->classptr;
	ss7mtp = new SS7MTPM3UA();
	proc->classptr = ss7mtp;
	
	ss7mtpstartup = (SS7MTPM3UASTARTUP *) generalbuf;
	/* mtp initialization */
	ss7mtpstartup->procdata = proc;
	ss7mtpstartup->l2 = ( ((( dpc )&0x3FFF)+(( opc )<<14)) + (( ni )<<30)   );			/* ni is essential */
	ss7mtpstartup->link_first = 0;
	ss7mtpstartup->link_last = (links-1);
	ss7mtpstartup->sctp = lastproc;	// next one
	ss7mtp->debug = ALL_TRAFFIC|ABNORMAL_TRAFFIC|ESSENTIAL_EVENTS|MANAGEMENT_TRAFFIC;
	if(ss7mtp->start( (void *) generalbuf ) ){ 
		log("mtp launching failed",0,0); exit(1);}
	else log("mtp launched",0,0);

  if(tcp){
	execproc++; lastproc++; 
	procqueue[execproc] = (PROCDATA *) malloc( sizeof(PROCDATA) );
	proc = procqueue[execproc];
	proc->classname = PROC_TCPSERVER;
	strcpy(proc->objectname,"tcp(s)");
	
//	proc->classptr=malloc(sizeof(TCPSERVER));
//	tcpserver = (TCPSERVER *) proc->classptr;
	tcpserver = new TCPSERVER();
	proc->classptr = tcpserver;
	
	tcpserverstartup = (TCPSERVERSTARTUP *) generalbuf;
	tcpserverstartup->procdata = proc;
	tcpserverstartup->m3ua = execproc-1;       // prev one
	tcpserverstartup->port = tcp;
	tcpserver->debug = ALL_TRAFFIC|ALL_EVENTS;//ABNORMAL_TRAFFIC|ESSENTIAL_EVENTS;//|MANAGEMENT_TRAFFIC;
	if(tcpserver->start( (void *) generalbuf ) ){ 
		log("failed",0,0); exit(1);}
	else log("started",0,0);
  } else {
	execproc++; lastproc++; 
	procqueue[execproc] = (PROCDATA *) malloc( sizeof(PROCDATA) );
	proc = procqueue[execproc];
	proc->classname = PROC_SCTPSERVER;
	strcpy(proc->objectname,"sctp(s)");
	
//	proc->classptr=malloc(sizeof(SCTPSERVER));
//	sctpserver = (SCTPSERVER *) proc->classptr;
	sctpserver = new SCTPSERVER();
	proc->classptr = sctpserver;
	
	sctpserverstartup = (SCTPSERVERSTARTUP *) generalbuf;
	sctpserverstartup->procdata = proc;
	sctpserverstartup->m3ua = execproc-1;       // prev one
	sctpserverstartup->port = sctp;
	sctpserver->debug = ABNORMAL_TRAFFIC|ESSENTIAL_EVENTS;//|MANAGEMENT_TRAFFIC;
	if(sctpserver->start( (void *) generalbuf ) ){
		log("failed",0,0); exit(1);}
	else log("started",0,0);
  }

  

//  execproc = -1;  log(VERSION,0,0);
  while(1)
  {

    
       usleep(50000L);

	 GlobalTimer++;
	 execproc = -1;
	 for(int i=0;i<lastproc;i++)
	   post(i,SS7_TIMER,NULL);

	 while( (execmsg)!=(lastmsg) ) {
	 	msg = msgqueue[execmsg];
		execproc = msg->dest;
		if(execproc < lastproc ) {
			proc = procqueue[execproc];
			ss7proc = (SS7PROC *) proc->classptr;
			ss7proc->event(msg);
		/*    
                  proc = procqueue[execproc];
		  switch(proc->classname){
			case PROC_TCPSERVER:	tcpserver = (TCPSERVER *) proc->classptr;
							tcpserver->event(msg);
							break;
			case PROC_SCTPSERVER:	sctpserver = (SCTPSERVER *) proc->classptr;
							sctpserver->event(msg);
							break;
                        case PROC_ITULINK:	ss7link = (SS7LINK *) proc->classptr;
							ss7link->event(msg);
							break;
			case PROC_ITUMTP:	ss7mtp = (SS7MTPM3UA *) proc->classptr;
							ss7mtp->event(msg);
							break;
			
			default: log("message to unknown process type",2,(char *)&(msg->msg));}
		*/
		}
		else log("process unknown",0,0);
		
		execmsg++;
		if(execmsg>=MSGQUEUESIZE) execmsg = 0;}
	  
  }
}

void post(int dest, int msg, void *param)
{
	MSGDATA *ptr;

	ptr = (MSGDATA *)msgqueue[lastmsg];
	ptr->msg = msg;
	ptr->dest = dest;
	ptr->src = execproc;
	ptr->param = param;
	lastmsg++;
	if(lastmsg>=MSGQUEUESIZE) lastmsg = 0;
}

// for SCTP
void post(int dest, int msg, void *param, unsigned stream, unsigned rc)
{
	MSGDATA *ptr;

	ptr = (MSGDATA *)msgqueue[lastmsg];
	ptr->msg = msg;
	ptr->dest = dest;
	ptr->src = execproc;
	ptr->param = param;
	ptr->param2 = stream;
	ptr->param3 = rc;
	lastmsg++;
	if(lastmsg>=MSGQUEUESIZE) lastmsg = 0;
}

void mem(void)
{
	memalloc++;
	if( memalloc >= SS7BufSize ) memalloc=0;
	freebuffer = (SS7MESSAGE *)&buffer[memalloc*SS7MsgSize];
}

