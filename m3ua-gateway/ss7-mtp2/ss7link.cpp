/**@file ITU-T SS7 MTP2
*
* Copyright 2012, Dmitri Soloviev <dmi3sol@gmail.com> for 
* Telestax http://opensigtran.org
*
* This software is distributed under the terms of the GNU Affero Public License.
* See the COPYING file in the main directory for details.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../defs.h"
#include "../../shm_keys.h"
#include "../../portnumbers.h"
#include "ss7linkvalues.h"
#include "string.h"
#include <syslog.h>

#define VERSION "ss7 link ver 1.6"
#define check_link

#define TX_QUEUE_GAP	30

/* 
 * negative ack is now checked and processed;
 * ack timer is still ignored 
 *                                             */
 
int SS7LINK::start(void *param)
{
	SS7LINKSTARTUP *ptr;
	
	ptr=(SS7LINKSTARTUP *)param;
	
	procdata=ptr->procdata;			/* store configuration */

	if(init(ptr->key)){
		log("shared memory HDLC interface failed",0,0);
		return 1;
	}
	
	link = ptr->link;
	mtp = ptr->mtp;
	pcap_ip = ptr->pcap_ip;
	pcap_port = ptr->pcap_port;
	
	if(	  ((sock_rx = socket(AF_INET,SOCK_DGRAM,0)) < 0)
		||((sock_tx = socket(AF_INET,SOCK_DGRAM,0)) < 0)) {
		log("can't open socket for logging",0,0);
		return 2;
	}
	
	static struct sockaddr_in sa;
	sa.sin_addr.s_addr = 0;
	sa.sin_port   = htons(FIRTS_OWN_PCAP_PORT + 2*(ptr->key - KEY_LINK0));
	sa.sin_family = AF_INET;
	if(bind(sock_rx, (struct sockaddr *) &sa, sizeof(struct sockaddr_in)) < 0){
		log("can't bind socket for logging",0,0);
		return 3;
	}
	sa.sin_addr.s_addr = 0;
	sa.sin_port   = htons(FIRTS_OWN_PCAP_PORT + 2*(ptr->key - KEY_LINK0) + 1);
	sa.sin_family = AF_INET;
	if(bind(sock_tx, (struct sockaddr *) &sa, sizeof(struct sockaddr_in)) < 0){
		log("can't bind socket for logging",0,0);
		return 4;
	}
	
	txbuffer=(SS7MESSAGE **) malloc(128*sizeof(void *) );
	if(txbuffer==NULL) {
		log("tx buffer allocation failed",0,0);
		return 5;
	}

	lssu.len=4;
	lssu.body.LSSU[0]=0xffff;			/* initialize variables */
	lssu.body.LSSU[1]=_SIOS_;

	tx_last_ackd=tx_last_placed=0xff;

	rx_ack_request=0;

	state = _SIO_;	// to check E1 state	//_SIOS_;
	T1 = T1value - 2;			/* wait a little	*/
	tx_msg(&lssu);				/* .. and controller    */

	log(VERSION,0,0);
	return 0;
}


int SS7LINK::stop(void *param)
{   
//	free(txtimer);
	free(txbuffer);

	lssu.len=4;
	lssu.body.LSSU[0]=0xffff;
	lssu.body.LSSU[1]=_SIPO_;
	
	tx_msg(&lssu);

	return 0;
}

void SS7LINK::check_e1()
{
    /*
   char ch;

        ch = inb(0x31a);
	ch &= 0x0F;
	if(ch!=hw_byte)
	{  if((ch&8)==(char)8)
	   { syslog(LOG_USER|LOG_ERR,"*** los ***\n");
	     log("*** LOS ***",0,0);
	     if(state != _SIOS_)
		post(mtp,SS7_LINKPAUSE,(void *)procdata);
	      state = _SIOS_;
	    }
	   else if((ch&4)==(char)4)
	   { syslog(LOG_USER|LOG_ERR,"*** ais ***\n");
	     log("*** RJA ***",0,0);
	     if(state != _SIOS_)
		post(mtp,SS7_LINKPAUSE,(void *)procdata);
	      state = _SIOS_;
	    }
	   else if((ch&2)==(char)2)
	   { syslog(LOG_USER|LOG_ERR,"*** rja ***\n");
	     log("*** RJA ***",0,0);
	     if(state != _SIOS_)
		post(mtp,SS7_LINKPAUSE,(void *)procdata);
	      state = _SIOS_;
	    }
	   else if((ch&1)==(char)1)
	    { syslog(LOG_USER|LOG_ERR,"*** sync **\n");
	      log("*** SYNC **",0,0);
	      if(state != _SIOS_)
		post(mtp,SS7_LINKPAUSE,(void *)procdata);
	      state = _SIOS_;
	    }
	   else
	   { syslog(LOG_USER|LOG_ERR,"e1 ok (physical link)\n");
	     log("physical link established",0,0);
	     
	     state = _SIO_;
	     lssu.len = 4;
	     lssu.body.LSSU[0] = 0xFFFF;
	     lssu.body.LSSU[1] = state;
	     (void)tx_msg(&lssu);}

	   hw_byte = ch;
	}
     */
}

void SS7LINK::timer(){
    SS7MESSAGE   *ss7msg;
    int len, index;
    
	check_e1();
	while( (len = rx_msg(freebuffer)) )
	   { ss7msg = freebuffer;
		mem();

		if( (ss7msg->body.su.mtp2.bitfield.bib) !=  (lssu.body.bitfield.fib) ){
				/* negative ack received */
			last_txd = ss7msg->body.su.mtp2.bitfield.bsn;
			lssu.body.bitfield.fib = ~(lssu.body.bitfield.fib);
		}
				
		if(len>5){				/* MSU */
		    if(state&OVERFLOW_MASK){
				/* link alignment is in progress */
				/* nothing is currently expected */
		    }
		    else {				/* link is working */
			  if( ((lssu.body.bitfield.last_rxd+1) - (ss7msg->body.su.mtp2.bitfield.fsn))&0x7F ){	
				/* sequence failed */
				lssu.body.bitfield.bib = ~(ss7msg->body.su.mtp2.bitfield.fib);
			  }
			  else 	{ 
				/* sequence OK */
				post(mtp,SS7_LINKTRANSFER,(void *)ss7msg);	
				lssu.body.bitfield.last_rxd = ss7msg->body.su.mtp2.bitfield.fsn;
			  }
			  rx_ack_request=1;
			}
		}

		else if(len==3) {		/* FISU */
		     if(state&OVERFLOW_MASK) {
				/* link alignment in progress */
			 if( state==_SIE_ ) {
			     state = _FISU_;
			     lssu.body.LSSU[0] = 0xFFFF;
			     lssu.body.LSSU[1] = _FISU_;
			     lssu.len = 3;
			     (void)tx_msg(&lssu);
			     tx_last_placed = last_txd = 0xff;

			     post(mtp,SS7_LINKRESUME,(void *)procdata);

			     if(ss7msg->body.board.word[0]!=0xFFFF)
					rx_ack_request=1;
			 }

		     }
		    else {	/* link is working  */
			;	/* just ignore it ! */
		    }				
		 }
		
		else {
		     alignment_lssu(len, ss7msg);/* LSSU */
		     break;
		 }
	}	// while( rx_msg() )
	
	
#ifdef check_link
				/* if controller collects dust.. */
        if(errorlevel)	{	/* wait until it is clear */
	    if( state!=_SIOS_ ) {	/* if marked is OK */
		T1 = T1value - 2;
		state = _SIOS_;			/* inform once */

		//if(debug&ESSENTIAL_EVENTS) 
			log("link is faulty! error level is",2,(char *)&errorlevel);
		
		lssu.len = 4;
		lssu.body.LSSU[0] = 0xFFFF;
		lssu.body.LSSU[1] = state;
		
		(void)tx_msg(&lssu);		/* periodically send _SIOS_ */

		post(mtp,SS7_LINKPAUSE,(void *)procdata);
	    }
	    
	    errorlevel = 0;
	    return;
	}
#endif	
	
	
	if( (((state&0x0FFF) == state) || (state==0xFFFF)) && state ) 
	{ alignment_timer();		/* if link is down		*/
	  return;	}				/* timer for alignment	*/
		 
	if(tx_last_ackd != tx_last_placed)	/* else		*/
	{					/* check ack timers	if any */ 
	}

	if((tx_last_placed-last_txd)&0x7F )	/*if anything to tx */
	{ index = (last_txd+1)&0x7F;
	  ss7msg = txbuffer[index];
	  ss7msg->body.su.mtp2.word.fsnbsn = lssu.body.LSSU[0];
	  ss7msg->body.su.mtp2.bitfield.fsn = (last_txd+1)&0x7F;
	  while(tx_msg(ss7msg)) {
	      last_txd = (last_txd+1)&0x7F;
	      lssu.body.bitfield.fsn = last_txd;
//		 txtimer[last_txd] = GlobalTimer;
	      rx_ack_request=0; 
	     if(!((tx_last_placed-last_txd)&0x7F)) break;
	     else{
		index = (last_txd+1)&0x7F;
		ss7msg = txbuffer[index];
		ss7msg->body.su.mtp2.word.fsnbsn = lssu.body.LSSU[0];
		ss7msg->body.su.mtp2.bitfield.fsn = (last_txd+1)&0x7F;
	     }
	  }
	}
	else{
	    if(state == BUF_OVERFLOW) {
		if(debug&ESSENTIAL_EVENTS) log("link buffer is empty now",0,0);
		state = 0;
		post(mtp,SS7_LINKRESUME,(void *)procdata);
	    }
	    
	    if(rx_ack_request) {		/* if nothing, but ack	*/
		lssu.len=3;
		lssu.body.LSSU[1]=_FISU_;
		if(tx_msg(&lssu))
		    rx_ack_request=0;
	    }
	}
}

int SS7LINK::event(void *param)
{
  MSGDATA *p;
  SS7MESSAGE   *ss7msg;
  int len,index;

  p=(MSGDATA *)param;
  switch(p->msg)
     {
   	case SS7_LINKTRANSFER: 
		ss7msg = (SS7MESSAGE *)p->param;

		ss7msg->body.su.mtp2.bitfield.li = ((ss7msg->len<(63+3))?((ss7msg->len)-3):63);
		
		tx_last_placed = (tx_last_placed+1)&0x7F;
		index = tx_last_placed&0x7F;
		txbuffer[index] = ss7msg;
		
		if(state){
			post(mtp,SS7_LINKPAUSE,(void *)procdata);
			break;
		}

		len=(tx_last_ackd-tx_last_placed)&0x7F;	/* positions remainded */
		if( ((len!=0) && (len<TX_QUEUE_GAP)) ){ 
		    state = BUF_OVERFLOW;			
			log("link buffer is full: remained",1,(char *)&len);
		   post(mtp,SS7_LINKPAUSE,(void *)procdata);
		}

		break;
	case SS7_LINKSTOP: 
		lssu.len = 4;
		lssu.body.LSSU[0] = 0xFFFF;
		lssu.body.LSSU[1] = _SIPO_;
		state = _SIPO_;
		while(!tx_msg(&lssu));
		break;
	case SS7_LINKRESET: 
		lssu.len = 4;
		lssu.body.LSSU[0] = 0xFFFF;
		lssu.body.LSSU[1] = _SIOS_;
		state = _SIOS_;
		(void)tx_msg(&lssu);
		break;
	default:			return 1;
     }
  return 0;
}
  
void SS7LINK::alignment_timer()
{
   T1++; Tsig++; T4++;

   long tmp;
   tmp = T1value;

   //log("alignment timer",2,(char *)&state);

   if(T1>T1value)
   { state = _SIOS_;
     T1 = 0;
	Tsig = 0;
	lssu.len = 4;
	lssu.body.LSSU[0] = 0xFFFF;
	lssu.body.LSSU[1] = _SIOS_;

	state = _SIOS_;

	(void)tx_msg(&lssu);
	return;
   }

   lssu.len = 4;
   switch(state)
     {		case _SIOS_:
				return;
		case _SIO_: 
				if( T2 >= T2value ) {
				    T1 = 0;
				    Tsig = 0;
				    state=_SIOS_;
				    lssu.len = 4;
				} 
				break;
		case _SIE_: 
				if( T3 >= T3value ){
				    Tsig = 0;
				    lssu.len = 4;
				    state = _SIO_ ;
				}
				else if( T4 >= T4value ){ 
				    if(lssu.len !=3 )	{ 
					Tsig = 0;
					lssu.len = 3;
					lssu.body.LSSU[0] = 0xFFFF;
					lssu.body.LSSU[1] = _FISU_ ;
					while(!tx_msg(&lssu));
				    }
				    return;
				}
				break;
		case 0xFFFF: return;
		default:
				lssu.len = 4;	/* something wrong ! */
				state = _SIPO_;
				break;
     }  
   lssu.body.LSSU[1] = state;
   (void)tx_msg(&lssu);
}
//void SS7LINK::alignment_lssu(WORD word)
void SS7LINK::alignment_lssu(int len, void *buf)
{
  SS7MESSAGE *ss7msg;
  ss7msg = (SS7MESSAGE *)buf;
  unsigned word = ss7msg->body.board.word[1];
  lssu.len = 4;
  switch(word){
	case _SIO_: 
		if(!(state&0x0FFF))	{	/* IS -> IDLE*/
			state = _SIOS_;
			lssu.len = 4;
			T1 = 0;
			Tsig = 0;
			break;
		}
		if( state==_SIE_ )	{	/* PROVING -> N.A. ???? */
			lssu.len = 4;
			T4 = 0;
			break;
		}
		if( (state==_SIO_) )	{	/* N.A. -> A.*/
			state = _SIE_;
			lssu.len = 4;
			T3 = 0;
			T4 = 0;
			break;
		}
		if( state==_SIOS_)	{	/* IDLE -> N.A.*/
			state = _SIO_;
			lssu.len = 4;
			T1 = 0;
			Tsig = 0;
			break;
		}
		break ;				/* nothing changed */
	case _SIN_: 
	case _SIE_: 
		if((state == _SIE_) && (T4 >= T4value) ) return;
		T3 = 0;
		lssu.len = 4;
		state = _SIE_;
		break;
	case _SIOS_:
		state=_SIO_;
		T1 = 0;
		T2 = 0;
		lssu.len = 4;
		lssu.body.LSSU[0]=0xFFFF;
		break;
	case _SIPO_:
	case _SIB_:
	default:
		state = _SIOS_;
		lssu.len = 4;
		lssu.body.LSSU[0]=0xFFFF;
  }
  lssu.body.LSSU[1] = state;
  (void)tx_msg(&lssu);
}
