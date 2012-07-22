/* Written by Dmitri Soloviev <dmi3sol@gmail.com>
  
  http://opensigtran.org
  http://telestax.com
  
  GPL version 3 or later
*/
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>

#include "../defs.h"
#include "ss7linkvalues.h"

#define maxSUlen 	300
#define maxSUnumber 	32
#define u16 unsigned short



#define suRECsize (maxSUlen>>1)	// ..len>>2 (due to WORD size)

#define RXmask 0x1F
#define TXmask 0x1F

#define RXsu (rxSu+rxpos*suRECsize)
#define TXsu (txSu+txpos*suRECsize)

#define nextRXsu rxpos=((rxpos+1)&RXmask)
#define nextTXsu txpos=((txpos+1)&TXmask)

#define set_adr(abc) ((unsigned short *)abc)



//unsigned char xx[300];

int SS7LINK::init(int key){
    u16 *shm;
    u16 r,t;
    int shm_perm = 0100666;
    int shm_id = shmget((key_t)key,(2*maxSUnumber*maxSUlen)+3*sizeof(u16),shm_perm);

    if(shm_id == -1){
        return 1;
    }
    shm = (u16 *) shmat(shm_id,0,0);
        
    rxSu = shm;
    txSu = shm + maxSUnumber*(maxSUlen>>1);
           
    u16* w;
        
    r = shm[(maxSUlen>>1)*maxSUnumber*2];
    t = shm[(maxSUlen>>1)*maxSUnumber*2 +1];

    log("r is",1,(char *)&r);
    log("t is",1,(char *)&t);
    
    rxpos = (r - 1)&RXmask;
    log("RXfrom",1,(char*)&rxpos);
    while((rxSu[((rxpos)&RXmask)*(maxSUlen>>1)]&0xF000)==0){
	log("Rxfilled",1,(char*)&rxpos);
	rxpos += (maxSUnumber - 1);
	rxpos &= RXmask;
    }
    rxpos ++; rxpos &= RXmask;
    
    txpos = (t + 1)&TXmask;
    log("RX pos",1,(char *)&rxpos);
    log("TX pos",1,(char *)&txpos);
    return 0;
}

int SS7LINK::rx_msg(void *buf)
  {
   u16 len;
   SS7MESSAGE *ss7;
   
   ss7 = (SS7MESSAGE *)buf;
   len = rxSu[rxpos*(maxSUlen>>1)];
   
   errorlevel = 0;

   //printf("\n %d bytes\n",len);
   if(( len == 0 )||(len&0xF000)) 
     { // get error level etc
       return 0;
     }

   ss7 = (SS7MESSAGE *)buf;
   ss7->len = len;

   memcpy(ss7->body.board.byte,rxSu + rxpos*(maxSUlen>>1) +1 ,len);
   RXfsnbsn = ss7->body.board.word[0];
//   log("RX ",ss7->len,&(ss7->body.board.byte[0]));
   rf5su(log_rx_link,ss7->len,&(ss7->body.board.byte[0]));

   rxSu[rxpos*(maxSUlen>>1)] = 0xC000;
   nextRXsu;

   return ss7->len;
  }

int SS7LINK::tx_msg(void *buf)
  {
   u16 *len;
   SS7MESSAGE *ss7;
   void *p;
    
   ss7 = (SS7MESSAGE *)buf;
   len = set_adr(TXsu);
   p = len+1;

   if (((*len)&0xF000)==0)// not TX'd already
     { log("failed TX pos",1,(char *)&txpos);
       return 0;
     }
	
   ss7 = (SS7MESSAGE *)buf;
 
   if( (ss7->len) >= 280 )
   { return 0;
   }

   memcpy(p,ss7->body.board.byte,ss7->len);
   *len = ss7->len;
   nextTXsu;

//   log("TX ",ss7->len,&(ss7->body.board.byte[0]));
   rf5su(log_tx_link,ss7->len,&(ss7->body.board.byte[0]));
 
   return ss7->len;
  }
