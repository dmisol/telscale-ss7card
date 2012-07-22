/* Written by Dmitri Soloviev <dmi3sol@gmail.com>
  
  http://opensigtran.org
  http://telestax.com
  
  GPL version 3 or later
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct RF5REC
{ unsigned sec,usec;
  unsigned link;
  unsigned textdata;
  unsigned taillen;
  char data[280];
} *rf5data;

#define uint32 unsigned

#define MAXLINKS 16
#define LINKMASK 0x0F

struct pcap_hdr_s {
        uint32 magic_number;   /* magic number */
        uint32 version;
        uint32  thiszone;       /* GMT to local correction */
        uint32 sigfigs;        /* accuracy of timestamps */
        uint32 snaplen;        /* max length of captured packets, in octets */
        uint32 network;        /* data link type */
        } pcap_hdr_t;

struct pcaprec_hdr_s {
        uint32 ts_sec;         /* timestamp seconds */
        uint32 ts_usec;        /* timestamp microseconds */
        uint32 incl_len;       /* number of octets of packet saved in file */
        uint32 orig_len;       /* actual length of packet */
} pcaprec_hdr_t;

char *rf5buffer;
unsigned *rf5pos;

FILE *f[MAXLINKS];
FILE *global;
FILE *txt;

int isOpened[MAXLINKS];

char buf[1000];
char str[3000];
char nameCommon[100];

void linkOpen(int i){
    char name[100];

    sprintf(name,"%d-%s",i,nameCommon);
    f[i] = fopen(name,"w");
    fwrite(&(pcap_hdr_t),sizeof(pcap_hdr_t),1,f[i]);
    
    isOpened[i] = 1;
}


int main(int argc, char* argv[])
{
   unsigned i,lnk;
   struct tm *tmp;
   time_t t;
   int sock;
   
   for(i=0;i<MAXLINKS;i++) 
       isOpened[i] = 0;
   
      
   t = time(0);
   tmp = localtime(&t);
   sprintf(nameCommon,"tdm%04d%02d%02d.txt",tmp->tm_year+1900,tmp->tm_mon+1,tmp->tm_mday);
   txt = fopen(nameCommon,"w");
   global = fopen(nameCommon,"w");
   if(!global){
       printf("file \"%s\" error\n",nameCommon);
       return 1;
   }
   
   sprintf(nameCommon,"tdm%04d%02d%02d.pcap",tmp->tm_year+1900,tmp->tm_mon+1,tmp->tm_mday);
   
   
   global = fopen(nameCommon,"w");
   if(!global){
       printf("file \"%s\" error\n",nameCommon);
       return 1;
   }
   pcap_hdr_t.magic_number = 0xa1b2c3d4;
   pcap_hdr_t.version = 0x040002;
   pcap_hdr_t.thiszone = 0;
   pcap_hdr_t.sigfigs = 0;
   pcap_hdr_t.snaplen = 280;
   pcap_hdr_t.network = 140;
   fwrite(&(pcap_hdr_t),sizeof(pcap_hdr_t),1,global);
   
   
   if( (sock = socket(AF_INET,SOCK_DGRAM,0)) < 0){
       printf("can't open socket\n");
       return 1;
   }  
   i = 1;
   if( setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&i,sizeof(i)) != 0){
      printf("setsockopt failed");
      return 2; 
   }  
   struct sockaddr_in sa;
   bzero(&sa,sizeof(sa));
   sa.sin_addr.s_addr = INADDR_ANY;
   sa.sin_port   = htons(2000);
   sa.sin_family = AF_INET;
  
   if( bind(sock,(struct sockaddr*)&sa,sizeof(sa)) != 0 ){
      printf("can't bind");
      perror("bind");
      return 3;
   }
   
   rf5data = (struct RF5REC *)buf;
   
   while(1){
       if((i = recv(sock,buf,sizeof(buf),0))>0){
           pcaprec_hdr_t.ts_sec = rf5data->sec;
           pcaprec_hdr_t.ts_usec = rf5data->usec;
           pcaprec_hdr_t.incl_len = pcaprec_hdr_t.orig_len = rf5data->taillen;
           fwrite(&(pcaprec_hdr_t),sizeof(pcaprec_hdr_t),1,global);
           fwrite(rf5data->data,rf5data->taillen,1,global);
           
           
           lnk = rf5data->link & LINKMASK;           
           
           if(lnk == rf5data->link)     sprintf(str,"%d RX%d ",rf5data->sec,lnk,rf5data->taillen);
           else                         sprintf(str,"%d\t\tTX%d ",rf5data->sec,lnk,rf5data->taillen);
           
           for(i=0;i<(rf5data->taillen);i++)
               if(i%4)  sprintf(str + strlen(str),"%02X",rf5data->data[i]&0xFF);
               else     sprintf(str + strlen(str)," %02X",rf5data->data[i]&0xFF);
           fprintf(txt,"%s\n",str);
           
           if(! isOpened[lnk]) linkOpen(lnk);
           
           fwrite(&(pcaprec_hdr_t),sizeof(pcaprec_hdr_t),1,f[lnk]);
           fwrite(rf5data->data,rf5data->taillen,1,f[lnk]);
           
           fflush(0);
       }
   }
}
