#define VERSION "simple sctp server v 3.00"
#define CLQTY 2
#define MAX_MSG_PER_TICK	70

#include "../defs.h"
#include "sctpserver.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
//#include <netinet/sctp.h>
#include <unistd.h>
#include <fcntl.h>
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

void SCTPSERVER::timer(){
  SS7MESSAGE *ptr;

  while(do_send()) ;
  int cntr = 0;

  while((ptr=(SS7MESSAGE *)serv_recv())!=0){
    post(m3ua,   SS7_M3UATRANSFER,(void *)ptr);
    if(cntr++ > MAX_MSG_PER_TICK) return;
  }
}


int SCTPSERVER::event(void *param){
  MSGDATA *p;
  p=(MSGDATA *)param;

  serv_send(p->param, p->stream);
  while(do_send()) ;
  return 0;
}


int SCTPSERVER::start(void *param){

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

	txbuffer = (void **) malloc( SCTP_FIFO_SIZE *sizeof(void *) );
	streamid = (int*) malloc( SCTP_FIFO_SIZE *sizeof(int) );
	put_ptr = get_ptr = 0;
	client_id_to_start_with = 0;
//	top_fifo_load = 0;

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

     
      value = 1; 
      // Makes the socket non-blocking
      ioctl(cl[j], FIONBIO , &value );

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
                
		  if(j!=(buf[0]-8)) log("RX incomplete packet",0,0);
                  //if(debug&&ALL_TRAFFIC)log("RX",buf[0],(char*)&(buf[1]));
                  return buf;}
              else {  //log("returned:",4,(char*)&j);
		  //log("errno",4,(char*)&errno);
	          if( (errno == EINTR)&&(j<0) ) { return 0; }
		  close(cl[i]);cl[i]=0xffff;
		  count_connections("read failed, active");
    		} 
          }
	  else {  log("SCTP recv() returned:",4,(char*)&j);
		  log("SCTP errno",4,(char*)&errno);
	          if( (errno == EINTR)&&(j<0) ) { return 0; }
		  close(cl[i]);cl[i]=0xffff;
		  count_connections("read failed, active");
    		} return 0;
	}
     }

  return 0;
}

int  SCTPSERVER::serv_send(void *buf, int stream){
	int i;
	
//	log("placing to queue",2,(char*)&put_ptr);
//	log("pos to read is",2,(char*)&get_ptr);

	streamid[put_ptr] = stream;
	txbuffer[put_ptr] = buf;
	put_ptr = (put_ptr+1)%SCTP_FIFO_SIZE;

//	log("placing for next is",2,(char*)&put_ptr);

	if (put_ptr == get_ptr) {
	    log("FIFO is full",0,0);
	    for(i=0;i<CLQTY;i++)
		if(cl[i]!=0xFFFF){
		    close(cl[i]);
		    cl[i] = 0xFFFF;
		    count_connections("TX buffer underrun");
		}
//	    log("+++",0,0);
	    return 0;
	}
//	log("+++",0,0);
	return 1;
}

int  SCTPSERVER::do_send()
{

// log("trying to tx",2,(char*)&get_ptr);
// log("tx till",2,(char*)&put_ptr);

 if(put_ptr == get_ptr) {
//    log("nothing 2 tx",0,0);
    return 0;
 }
    
 unsigned res;
 int j,sock,tr;


 sinfo.sinfo_stream = streamid[get_ptr];

 unsigned *buf = (unsigned *)txbuffer[get_ptr];
 if(client_id_to_start_with>=CLQTY)  client_id_to_start_with = 0;
 
 for(res=0,j=client_id_to_start_with;j<clqty;j++)
 { sock=cl[j];
   if(sock!=0xFFFF)
   {    
	// to update when blackfin tools ready:
	// in RFC6458: SCTP_DEFAULT_SNDINFO
	// now it's too fresh
	tr = setsockopt(sock, IPPROTO_SCTP, SCTP_DEFAULT_SEND_PARAM, &sinfo, sizeof(sinfo));

	int remained = buf[0];
	char *data; data = (char*)&(buf[1]);
	
	// should be replaced with sctp-specific
	    tr = send(sock,data,remained,0);
	    if(tr<0){
		if(errno == EINTR) { 
			log("TX intr",0,0); 
			client_id_to_start_with = j; return 0; }
		else if(errno == EAGAIN) { 
			//log("TX again",2,(char*)&remained); 
/*
			int fifo;
			    if(put_ptr > get_ptr) fifo = put_ptr-get_ptr;
			    else fifo = put_ptr+(SCTP_FIFO_SIZE)-get_ptr;
			if(fifo > top_fifo_load) {
			    top_fifo_load = fifo;
			    log("top fifo size is",2,(char*)&fifo);
			}
*/
			client_id_to_start_with = j; return 0;}
		else {
			log("TX error ",1,(char *)&errno); 
			close(cl[j]);
			cl[j] = 0xFFFF;
			count_connections("write failed, active");
			goto next_socket_tx; 
		}
	    }
        res++;
   }
   next_socket_tx: ;
 }
 client_id_to_start_with = 0; get_ptr = (get_ptr+1)%SCTP_FIFO_SIZE;
// log("next tx is",2,(char*)&get_ptr);
// log("---",0,0);
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

