#define VERSION "simple tcp server v 2.00"
#define CLQTY 3
#define MAX_MSG_PER_TICK	60
#include "../defs.h"
#include "tcpserver.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <netinet/sctp.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

void TCPSERVER::count_connections(char *str)
{ int i,j;
  j = 0;
  for(i=0;i<clqty;i++)
    if(cl[i]!=0xFFFF) j++;
  if(debug&&ALL_EVENTS) log(str,1,(char *)&j);
/*
  if(state){	 if(j==0) post(m3ua, SS7_SCTPPAUSE,0);}
  else		 if(j) post(m3ua, SS7_SCTPRESUME,0);
*/
  state = j;
}

void TCPSERVER::timer(){
  unsigned cntr = 0;
  SS7MESSAGE *ptr;
  
  while((ptr=(SS7MESSAGE *)serv_recv())!=0){
    post(m3ua,   SS7_M3UATRANSFER,(void *)ptr);

    if(cntr++ > MAX_MSG_PER_TICK) return;
  }
}


int TCPSERVER::event(void *param)
{
  MSGDATA *p;

  p=(MSGDATA *)param;

  serv_send((unsigned *)p->param);
  return 0;
}


int TCPSERVER::start(void *param)
{
	TCPSERVERSTARTUP *ptr;
	
	ptr=(TCPSERVERSTARTUP *)param;
	
	procdata=ptr->procdata;			/* store configuration */

	m3ua = ptr-> m3ua;
	port = ptr-> port;
	clqty = CLQTY;

	cl = (int *)malloc(clqty*sizeof(int));
	for(int i=0;i<clqty;i++) cl[i] = 0xFFFF;
	ip = (long *)malloc(clqty*sizeof(long));

	if(init_sctp_serv())
	{	log(VERSION,0,0);
		return 0;
	}
	else 
	{ 	free(cl);
		return 1;
	}
}


int TCPSERVER::stop(void *param)
{ 
    free(cl);
    free(ip);
    close(req_sock);
	return 0;
}


void* TCPSERVER::serv_recv()
{
  int i,j;
  fd_set mask;
  int maxsock;
  int vacant;
  unsigned int addrlen;
  struct sockaddr_in *dummy;
  struct sockaddr from;
  struct timeval timeout;
  unsigned *buf;

  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  dummy = (struct sockaddr_in *)&from;

  FD_ZERO(&mask);
  FD_SET(req_sock,&mask);
  maxsock=req_sock;
  for(i=vacant=0;i<clqty;i++)
    if(cl[i]!=0xFFFF)
     { FD_SET(cl[i],&mask);
       if(cl[i]>maxsock) maxsock=cl[i]; }
    else {vacant++;}

  if( select(maxsock+1,&mask,(fd_set *)0,(fd_set *)0,&timeout) < 0 )
    {  if( errno == EINTR ) { return 0;}
         else{ log(" select failed",0,0); exit(1);}
    }
  
  if(FD_ISSET(req_sock,&mask)&&(vacant!=0))
  {  for(i=j=0;i<clqty;i++) if(cl[i]==0xFFFF) {j=i;break;}
     if( (cl[j] = accept(req_sock,(struct sockaddr *)dummy,&addrlen)) < 0 )
     { cl[j] = 0xFFFF;
       count_connections("accept failed, active");
       return 0;}
       
     int value = 1;
     if(setsockopt(cl[j],SOL_TCP,TCP_NODELAY,&value,sizeof(value))!=0)
     { close(cl[j]);
       cl[j] = 0xFFFF;
       count_connections("setting SCTP_NODELAY failed, active");
       return 0;}
     
     count_connections("accepted, active");
     }


  for(i=0;i<clqty;i++)
    if(cl[i]!=0xFFFF)
     { if(FD_ISSET(cl[i],&mask))
        { 
          buf = (unsigned *)freebuffer;
          mem();
          
          j = recv(cl[i],&(buf[1]),8,0);
          if(j==8){
              buf[0] = ntohl(buf[2]);
              if(buf[0]>(SS7MsgSize-10)) {
                  log("first u32 is",4,(char*)&buf[2]);
                  close(cl[i]);cl[i]=0xffff;
		  count_connections("msg is too long, active");
              }
              if( (j=recv(cl[i],&(buf[3]),(buf[0]-8),0)) > 0 ){ 
                  
                  //if(debug&&ALL_TRAFFIC)log("RX",buf[0],(char*)&(buf[1]));
                  return buf;}
              else {  //log("returned:",4,(char*)&j);
		  //log("errno",4,(char*)&errno);
	          if( (errno == EINTR)&&(j<0) ) { return 0; }
		  close(cl[i]);cl[i]=0xffff;
		  count_connections("read failed, active");
    		} 
          }
          else {  //log("returned:",4,(char*)&j);
		  //log("errno",4,(char*)&errno);
	          if( (errno == EINTR)&&(j<0) ) { return 0; }
		  close(cl[i]);cl[i]=0xffff;
		  count_connections("read failed, active");
    		}
          return 0;

	    }
     }

  return 0;
}

int  TCPSERVER::serv_send(unsigned *buf)
{
 unsigned res;
 int j,sock,tr;
 
 for(res=j=0;j<clqty;j++)
 { sock=cl[j];
   if(sock!=0xFFFF)
   {    

	int attempt = 0;
	int remained = buf[0];
	char *data; data = (char*)&(buf[1]);
	do{
		// should be replaced with sctp-specific
	    tr = send(sock,data,remained,0);
	    if(tr<0){
		if(errno == EINTR) { log("TX intr",0,0); attempt++; }
		else if(errno == EAGAIN) { log("TX again",2,(char*)&remained); attempt++; }
		else attempt = 100;

		if(attempt>5){
			log("TX error ",1,(char *)&errno); 
			close(cl[j]);
			cl[j] = 0xFFFF;
			count_connections("write failed, active");
			goto next_socket_tx; 
		}

		SS7MESSAGE *ptr;
		unsigned rxcounter = 0;
		while(((ptr=(SS7MESSAGE *)serv_recv())!=0)&&((rxcounter++)<MAX_MSG_PER_TICK/4))
		    post(m3ua,   SS7_M3UATRANSFER,(void *)ptr);

		usleep(TimerTick*1000L);
	    }
	    else if(tr!=remained) {
		log("TX incomplete",0,0);
		data += tr;
		remained -= tr;
		tr = 0;
	    }
	}while(tr != remained);
         res++;
   }
   next_socket_tx: ;
 }
// log("sock tx done",0,0);
 return res;
}







int  TCPSERVER::init_sctp_serv()
{
  int ret,value;
  struct sockaddr_in servaddr;
//  struct sctp_initmsg initmsg;
  time_t currentTime;

  for(sock=0;sock<clqty;sock++)
     cl[sock]=0xFFFF;

  if( (req_sock= socket( AF_INET, SOCK_STREAM, IPPROTO_TCP )) < 0)
  { log("server socket failed",0,0);close(req_sock); return 0;}

  value = 1;
  if( setsockopt(req_sock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)) != 0)
  { log("setsockopt(SO_REUSEADDR) failed",4,(char *)&errno); return 0; }

  bzero( (void *)&servaddr, sizeof(servaddr) );
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl( INADDR_ANY );
  servaddr.sin_port = htons(port);

  if( bind( req_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) 
  { log("server bind failed",0,0); 

     close(req_sock); return 0;}

  if(listen(req_sock,3) < 0 )
  { log("server listen failed",0,0);close(req_sock); return 0;}

  return 1;
}

