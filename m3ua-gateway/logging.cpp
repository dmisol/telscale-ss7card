/* Written by Dmitri Soloviev <dmi3sol@gmail.com>
  
  http://opensigtran.org
  http://telestax.com
  
  GPL version 3 or later
*/
#include <sys/shm.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>

#include "defs.h"


char *Bytes[] = {	"00","01","02","03","04","05","06","07","08","09","0A","0B","0C","0D","0E","0F",
			"10","11","12","13","14","15","16","17","18","19","1A","1B","1C","1D","1E","1F",
			"20","21","22","23","24","25","26","27","28","29","2A","2B","2C","2D","2E","2F",
			"30","31","32","33","34","35","36","37","38","39","3A","3B","3C","3D","3E","3F",
			"40","41","42","43","44","45","46","47","48","49","4A","4B","4C","4D","4E","4F",
			"50","51","52","53","54","55","56","57","58","59","5A","5B","5C","5D","5E","5F",
			"60","61","62","63","64","65","66","67","68","69","6A","6B","6C","6D","6E","6F",
			"70","71","72","73","74","75","76","77","78","79","7A","7B","7C","7D","7E","7F",
			"80","81","82","83","84","85","86","87","88","89","8A","8B","8C","8D","8E","8F",
			"90","91","92","93","94","95","96","97","98","99","9A","9B","9C","9D","9E","9F",
			"A0","A1","A2","A3","A4","A5","A6","A7","A8","A9","AA","AB","AC","AD","AE","AF",
			"B0","B1","B2","B3","B4","B5","B6","B7","B8","B9","BA","BB","BC","BD","BE","BF",
			"C0","C1","C2","C3","C4","C5","C6","C7","C8","C9","CA","CB","CC","CD","CE","CF",
			"D0","D1","D2","D3","D4","D5","D6","D7","D8","D9","DA","DB","DC","DD","DE","DF",
			"E0","E1","E2","E3","E4","E5","E6","E7","E8","E9","EA","EB","EC","ED","EE","EF",
			"F0","F1","F2","F3","F4","F5","F6","F7","F8","F9","FA","FB","FC","FD","FE","FF"};

#define BLOCK_SUP
static char logstring[500];

static char *logdata;
static int *logpos;
/*
void decode(char *text, void *ptr, int netw)
{
  int pos;
  ISDNMESSAGE *msg;
  char *tail;

  msg = (ISDNMESSAGE*) ptr;
  
  pos = sprintf(logstring,text);


  if( isINFO(msg->) )
  { pos+= sprintf(logstring+pos,"INFO[%02X/%02X]",
				msg->frame.lapd.ns,
				msg->frame.lapd.nr);

    if(((msg->frame.dss1.PD) != DSS1_PROTOCOL) || ((msg->frame.dss1.CRLen) != DSS1_CRLEN)    )
		 pos+= sprintf(logstring+pos,"?");
	else pos+= sprintf(logstring+pos," ");

	pos+= sprintf(logstring+pos,"%02X(%04X) ",msg->frame.dss1.MT,msg->frame.dss1.CR);

	tail = &(msg->frame.dss1.tail[0]);
	while(tail<&(msg->board.byte[msg->len]))
	{  if((*tail)&0x80)
	     { 	pos+= sprintf(logstring+pos,"<%02X> ",((*tail)&0xFF));
	        tail++;}
	   else
	     { 	pos+= sprintf(logstring+pos,"<%02X:",((*tail)&0xFF));

	        for(int q=0;q<tail[1];q++)
	          pos+= sprintf(logstring+pos,"%02X",((tail[2+q])&0xFF));
	        pos+= sprintf(logstring+pos,"> ");
	        tail+=(tail[1]+2);}
	}
	pos+= sprintf(logstring+pos,";");
	
	log(logstring,0,0);	// !!
  }

  else if( isSUP(msg->) )
  { if(isRR(msg->))
		pos+= sprintf(logstring+pos,"RR  [%02X]",
					msg->frame.lapd.nr);
	else if(isRNR(msg->))
		pos+= sprintf(logstring+pos,"RNR [%02X]",
					msg->frame.lapd.nr);
	else if(isREJ(msg->))
		pos+= sprintf(logstring+pos,"REJ [%02X]",
					msg->frame.lapd.nr);
	else pos+= sprintf(logstring+pos,"SUP(?):%02X",msg->frame.lapd.ns);
#ifndef BLOCK_SUP
	log(logstring,0,0);	// !!
#endif	
  }
  else
  { if(isSABME(msg->))
		pos+= sprintf(logstring+pos,"SABME");
    else if(isUA(msg->))
		pos+= sprintf(logstring+pos,"UA");
         else pos+= sprintf(logstring+pos," ? :%02X",msg->frame.lapd.ns);

    log(logstring,0,0);	// !!
  }

//  log(logstring,0,0);
}


*/
void log_shmem(key_t logmemid)
{
   int shm_id,shm_perm,i;
   shm_perm = 0100666;
   shm_id = shmget(logmemid,(sizeof(int)+64*300),IPC_CREAT|shm_perm);
   if(shm_id == -1)
     { printf("logging impossible\n");
       exit(1); }
   logdata = (char*) shmat(shm_id,0,0);
   logpos = (int *) &logdata[64*300];

   for(i=0;i<(64*300);i++) logdata[i]=0;
   *logpos = ((*logpos)-25)&0x3F;

   printf("logging started\n");
   log("*******************",0,0);
}

void log(char *id, int len, char *data)
{
  int pos,i;
  char *logstr;
  int index;
  
  index = *logpos;
  logstr = logdata + 300*((index)&0x3F);
  
  if(execproc<lastproc)
      sprintf(logstr,"%6s > %s ",procqueue[execproc]->objectname,id);
  else  sprintf(logstr,"%s ",id);

  //while( (pos%9)!= 7 )pos+=sprintf(logstr+pos," ");

  //if(len) pos+=sprintf(logstr+pos,"[%04d] ",len);
  
  for(i=0;i<len;i++){
      if((i&3)==0) strcat(logstr," ");
      strcat(logstr,Bytes[data[i]&0xFF]);
  }

  index++;
  *logpos = index&0x3F;
}
/*
void log(char *id, int len, char *data)
{
  struct tm *tmp;
  int pos,i;
  static char logstr[800];
  char *contlog;
  int index;
  struct timeval tv;

  gettimeofday(&tv,0);
  tmp = localtime(&(tv.tv_sec));


  pos = 0;

  if(execproc<lastproc)
      pos+=sprintf(logstr,"%02i:%02i:%02i.%03i %6s > %s",tmp->tm_hour,tmp->tm_min,tmp->tm_sec,tv.tv_usec/1000,procqueue[execproc]->objectname,id);
  else  pos+=sprintf(logstr,"%02i:%02i:%02i.%03i %s",tmp->tm_hour,tmp->tm_min,tmp->tm_sec,tv.tv_usec/1000,id);

  while( (pos%9)!= 7 )pos+=sprintf(logstr+pos," ");

  if(len) pos+=sprintf(logstr+pos,"[%04d] ",len);
  
  for(i=0;i<len;i++)
		if(i&3) pos+=sprintf(logstr+pos,"%02X",data[i]&0xFF);
		else	pos+=sprintf(logstr+pos," %02X",data[i]&0xFF);

  contlog = logstr;

  index = *logpos;
  while(pos > 300)
    { if(*contlog==' ') { contlog++; pos--;}

      memcpy((logdata+ 300*((index)&0x3F)),contlog,300);
      index++;			

      pos-=300;
      contlog += 300;
    }

  if(*contlog==' '){ contlog++; pos--;}
    	  
  memcpy((logdata+ 300*((index)&0x3F))     ,contlog ,pos);
  memset((logdata+(300*((index)&0x3F))+pos),(char)0,300-pos);
  index++;

  *logpos = index&0x3F;
}
*/

struct RF5REC
{ unsigned long sec,usec;
  unsigned link;
  unsigned textdata;
  unsigned taillen;
  char data[280];
} *rf5data;

#ifdef RF5TOSHM
char *rf5buffer;
int *rf5pos;

void rf5shmem(key_t memid)
{
   int shm_id,shm_perm;
   shm_perm = 0100666;
   shm_id = shmget(memid,(sizeof(int)+64*sizeof(struct RF5REC)),IPC_CREAT|shm_perm);
   if(shm_id == -1)
     { log("rf5 logging impossible",0,0);
       exit(1); }
   rf5buffer = (char *) shmat(shm_id,0,0);
   rf5pos = (int *) &rf5buffer[64*sizeof(struct RF5REC)];

   for(unsigned i=0;i<(64*sizeof(struct RF5REC));i++)
       rf5buffer[i]=0;
   *rf5pos = ((*rf5pos)-25)&0x3F;

   log("rf5 logging started",0,0);
}

void rf5su(int link, int len, char *data)
{ struct timeval tv;
  struct timezone tz;
  int index;


  tz. tz_minuteswest = tz. tz_dsttime = 0;
  gettimeofday(&tv,&tz);

  index = *rf5pos;
  rf5data = (struct RF5REC*) (rf5buffer + sizeof(struct RF5REC)*((index)&0x3F));

  rf5data->sec = tv.tv_sec; rf5data->usec = tv.tv_usec;
  rf5data->link = link;
  rf5data->textdata = 0;

  if(len >=280) len = 278;
  rf5data->taillen = len;
  memcpy(&(rf5data->data[0]),data,len);

  index++;
  *rf5pos = index&0x3F;
}
#else
static int rf5sock;
static struct sockaddr_in rf5sa;
   
void rf5shmem(unsigned ip, unsigned short port)
{
    if(ip == 0) {
        rf5sock = 0;
        return;
    }
    
   if( (rf5sock = socket(AF_INET,SOCK_DGRAM,0)) < 0){
       log("can't open socket for logging",0,0);
       return;
   }
   rf5sa.sin_addr.s_addr = ip;
   rf5sa.sin_port   = htons(port);
   rf5sa.sin_family = AF_INET;
}

void rf5su(int link, int len, char *data)
{ struct timeval tv;
  struct timezone tz;
  int index;
  char buffer[350];

  if(rf5sock == 0) return;

  tz. tz_minuteswest = tz. tz_dsttime = 0;
  gettimeofday(&tv,&tz);

  rf5data = (struct RF5REC*) buffer;

  rf5data->sec = tv.tv_sec; rf5data->usec = tv.tv_usec;
  rf5data->link = link;
  rf5data->textdata = 0;

  if(len >=280) len = 278;
  rf5data->taillen = len;
  memcpy(&(rf5data->data[0]),data,len);
  
  sendto(rf5sock,rf5data,rf5data->taillen + 3*sizeof(unsigned) + 2*sizeof(long),0,(sockaddr*)&rf5sa,sizeof(rf5sa));
}
#endif