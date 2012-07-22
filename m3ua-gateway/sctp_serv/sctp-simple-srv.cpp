/* Written by Dmitri Soloviev <dmi3sol@gmail.com>
  
  http://opensigtran.org
  http://telestax.com
  
  GPL version 3 or later
*/

#define VERSION "simple sctp server v 2.00"
#define CLQTY 5

#include "../defs.h"
#include "sctpserver.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <netinet/sctp.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

void SCTPSERVER::count_connections(char *str)
{ int i,j;
  j = 0;
  for(i=0;i<clqty;i++)
    if(cl[i]!=0xFFFF) j++;
  if(debug&&ALL_EVENTS) log(str,1,(char *)&j);

  state = j;
}

int SCTPSERVER::event(void *param)
{
  MSGDATA *p;
  int i,j;
  SS7MESSAGE *ptr;

  p=(MSGDATA *)param;

  switch(p->msg){
	case SS7_TIMER:
		while((ptr=(SS7MESSAGE *)serv_recv())!=0){
		    post(m3ua,   SS7_M3UATRANSFER,(void *)ptr);
		}
		return 0;

	case SS7_M3UATRANSFER:
		serv_send((unsigned *)p->param, p->param2);
		return 0;

	default: return 1;
  }
}

int SCTPSERVER::start(void *param)
{
	SCTPSERVERSTARTUP *ptr;
	
	ptr=(SCTPSERVERSTARTUP *)param;
	
	procdata=ptr->procdata;			/* store configuration */

	m3ua = ptr-> m3ua;
	port = ptr-> port;
	clqty = CLQTY;

	cl = (int *)malloc(clqty*sizeof(int));
	for(int i=0;i<clqty;i++) cl[i] = 0xFFFF;
	ip = (long *)malloc(clqty*sizeof(long));
        
        memset(&sinfo, 0, sizeof(sinfo));

	if(init_sctp_serv())
	{	log(VERSION,0,0);
		return 0;
	}
	else 
	{ 	free(cl);
		return 1;
	}
}


int SCTPSERVER::stop(void *param)
{ 
    free(cl);
    free(ip);
    close(req_sock);
	return 0;
}

void* SCTPSERVER::serv_recv()
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
     if(setsockopt(cl[j],SOL_SCTP,SCTP_NODELAY,&value,sizeof(value))!=0)
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
              if(buf[0]>292) {
                  log("length is",4,(char*)&(buf[0]));
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
	  else {  log("expected 8, read:",4,(char*)&j);
		  log("errno",4,(char*)&errno);
	          if( (errno == EINTR)&&(j<0) ) { return 0; }
		  close(cl[i]);cl[i]=0xffff;
		  count_connections("read failed, active");
    		}
          return 0;

	    }
     }

  return 0;
}

int  SCTPSERVER::serv_send(unsigned *buf, int stream)
{
 unsigned res;
 int j,sock,tr;

 sinfo.sinfo_stream = stream;
 
// log("TX",buf[0],(char *)&(buf[1]));

 for(res=j=0;j<clqty;j++)
 { sock=cl[j];
   if(sock!=0xFFFF)
   {    
       // to update when blackfin tools ready:
       // in RFC6458: SCTP_DEFAULT_SNDINFO
       // now it's too fresh
       tr = setsockopt(sock, IPPROTO_SCTP, SCTP_DEFAULT_SEND_PARAM, &sinfo, sizeof(sinfo));
       if(tr<0) log("stream setting failed",2,(char *)&tr);

       tr = send(sock,(&(buf[1])),buf[0],0);      // should be replaced with sctp-specific
         if( tr<0 )
            { if( (errno == EINTR)&&(tr<0) ) {log(" tx intr",0,0); continue;}
	      close(cl[j]);
	      cl[j] = 0xFFFF;
	      count_connections("write failed, active");
              goto next_socket_tx;
            }
	 else if(tr != buf[0]) log("sent only",4,(char *)buf);
//	 else log("TX",tr,(char *)(&buf[1]));
         res++;
   }
   next_socket_tx: ;
 }
 return res;
}


int  SCTPSERVER::init_sctp_serv()
{
  int ret,value;
  struct sockaddr_in servaddr;
  struct sctp_initmsg initmsg;
//  char buffer[MAX_BUFFER+1];
  time_t currentTime;

  for(sock=0;sock<clqty;sock++)
     cl[sock]=0xFFFF;

  if( (req_sock= socket( AF_INET, SOCK_STREAM, IPPROTO_SCTP )) < 0)
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
//    printf("errno:%d\n",errno); 
//    char *c; perror(c);
//    printf("err: %s\n",c);
     close(req_sock); return 0;}

  memset( &initmsg, 0, sizeof(initmsg) );
  initmsg.sinit_num_ostreams = SCTP_STREAMS_QTY;
  initmsg.sinit_max_instreams = SCTP_STREAMS_QTY;
  initmsg.sinit_max_attempts = 4;
  ret = setsockopt( req_sock, IPPROTO_SCTP, SCTP_INITMSG, 
                     &initmsg, sizeof(initmsg) );

  if(listen(req_sock,3) < 0 )
  { log("server listen failed",0,0);close(req_sock); return 0;}

  return 1;
}

