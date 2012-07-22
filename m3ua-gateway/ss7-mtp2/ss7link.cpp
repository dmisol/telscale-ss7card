/* Written by Dmitri Soloviev <dmi3sol@gmail.com>
  
  http://opensigtran.org
  http://telestax.com
  
  GPL version 3 or later
*/
#include "../defs.h"
#include "ss7linkvalues.h"
#include "string.h"
#include <syslog.h>
#include <stdlib.h>
#define VERSION "ss7 link ver 1.4"
#define check_link

/* 
 * negative ack is now checked and processed;
 * ack timer is still ignored 
 *                                             */
 
int SS7LINK::start(void *param)
{
	SS7LINKSTARTUP *ptr;
	
	ptr=(SS7LINKSTARTUP *)param;
	
	procdata=ptr->procdata;			/* store cnfiguration */

	if(init(ptr->key)){
		log("shared memory HDLC interface failed",0,0);
		return 1;
	}
	
	linkset = ptr->linkset;
	link = ptr->link;
	mtp = ptr->mtp;
	
	log_rx_link = link;
	log_tx_link = link|0x80;
	
	
	txbuffer=(SS7MESSAGE **) malloc(128*sizeof(void *) );
	if(txbuffer==NULL) 
	  { log("tx buffer allocation failed",0,0);
	    return 1; }
    
	lssu.len=4;
	lssu.body.LSSU[0]=0xffff;			/* initialize variables */
	lssu.body.LSSU[1]=_SIOS_;

	tx_last_ackd=tx_last_placed=0xff;

	rx_ack_request=0;

	state = _SIO_;	// to check E1 state	//_SIOS_;
	T1 = T1value - 2;			/* wait a little		*/
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

int SS7LINK::event(void *param)
{
  MSGDATA *p;
  SS7MESSAGE   *ss7msg;
  int len,index;

  p=(MSGDATA *)param;
//  log("link event",2,(char *)&(p->msg));
  switch(p->msg)
     {
 	case SS7_TIMER:

		check_e1();
		while( (len = rx_msg(freebuffer)) )
		   { ss7msg = freebuffer;
			 mem();

			 if( (ss7msg->body.su.mtp2.bitfield.bib) !=  (lssu.body.bitfield.fib) )
			 {											/* negative ack received */
			 	if(debug&ALL_EVENTS) log("got negative ack",0,0);
			 	last_txd = ss7msg->body.su.mtp2.bitfield.bsn;
				log("last valid is",1,(char *)&last_txd);
			 	lssu.body.bitfield.fib = ~(lssu.body.bitfield.fib);
			 }
				
			 if(len>5)				/* MSU */
			 {  if(state&OVERFLOW_MASK)
									/* link alignment in progress */
					{;}				/* nothing is currently expected */
				else				/* link is working */
					{ if( ((lssu.body.bitfield.last_rxd+1) - (ss7msg->body.su.mtp2.bitfield.fsn))&0x7F )	/* sequence failed */
						lssu.body.bitfield.bib = ~(ss7msg->body.su.mtp2.bitfield.fib);
					  else 
						{ post(mtp,SS7_LINKTRANSFER,(void *)ss7msg);	/* sequence OK */
						  lssu.body.bitfield.last_rxd = ss7msg->body.su.mtp2.bitfield.fsn;
						}
					  rx_ack_request=1;
					}
			 }
			 else if(len==3)		/* FISU */
			 {  if(state&OVERFLOW_MASK)
									/* link alignment in progress */
					{ if( state==_SIE_ )
						{ state = _FISU_;
						  lssu.body.LSSU[0] = 0xFFFF;
						  lssu.body.LSSU[1] = _FISU_;
						  lssu.len = 3;
						  (void)tx_msg(&lssu);
						  
						  tx_last_placed = last_txd = 0xff;

						  post(mtp,SS7_LINKRESUME,(void *)procdata);
						  if(debug&ESSENTIAL_EVENTS)log("link is up (got FISU)",0,0);

						  if(ss7msg->body.board.word[0]!=0xFFFF)
							  rx_ack_request=1;
						}
					  
					}
				else				/* link is working  */
					{;}				/* just ignore it ! */
			 }
			 else 
				{  alignment_lssu(ss7msg->body.board.word[1]);/* LSSU */
				   break;
				}
		   }
#ifdef check_link
						/* if controller collects dust.. */
        if(errorlevel)				/* wait until it is clear */
		{ 
		  if( state!=_SIOS_ )		/* if marked is OK */
		   { T1 = T1value - 2;
			  state = _SIOS_;			/* inform once */

			  if(debug&ESSENTIAL_EVENTS)log("link is faulty! errorlevel",2,(char *)&errorlevel);
			  
			  lssu.len = 4;
			  lssu.body.LSSU[0] = 0xFFFF;
			  lssu.body.LSSU[1] = state;
			  (void)tx_msg(&lssu);		/* periodically send _SIOS_ */
			 
			 post(mtp,SS7_LINKPAUSE,(void *)procdata);
		   }
		  errorlevel = 0;
		  return 0;
		}
#endif		
		if( (((state&0x0FFF) == state) || (state==0xFFFF)) && state ) 
		{ alignment_timer();		/* if link is down		*/
		  return 0;	}				/* timer for alignment	*/
		 
                if(tx_last_ackd != tx_last_placed)	/* else		*/
				{					/* check ack timers	if any */ 
				}

		if((tx_last_placed-last_txd)&0x7F ){	/*if anything to tx */
		    index = (last_txd+1)&0x7F;
		    
		    if(state){
			unsigned s = (tx_last_placed-last_txd)&0x7F;
			log("msgs to tx",1,(char *)&s);
			s = (tx_last_ackd-tx_last_placed)&0x7F;
			log("remained",1,(char *)&s);
			log("txd",1,(char *)&last_txd);
			log("ackd",1,(char *)&tx_last_ackd);
			log("placed",1,(char *)&tx_last_placed);
		    }
//		  sprintf(logstring,"ackd %02X filled %02X index is x%02X, points at (%lX)%lX(%lX)",tx_last_ackd&0x7F,tx_last_placed&0x7F,index,txbuffer[(index-1)&0x7F],txbuffer[index],txbuffer[(index+1)&0x7F]);
//		  log(logstring,0,0);
		  
		  ss7msg = txbuffer[index];
		  ss7msg->body.su.mtp2.word.fsnbsn = lssu.body.LSSU[0];
		  ss7msg->body.su.mtp2.bitfield.fsn = (last_txd+1)&0x7F;
		  while(tx_msg(ss7msg)) {
			 last_txd = (last_txd+1)&0x7F;
		         lssu.body.bitfield.fsn = last_txd;
//			 txtimer[last_txd] = GlobalTimer;
			 rx_ack_request=0; 
		     if(!((tx_last_placed-last_txd)&0x7F)) break;
		     else 
			 { index = (last_txd+1)&0x7F;
			   ss7msg = txbuffer[index];

		       ss7msg->body.su.mtp2.word.fsnbsn = lssu.body.LSSU[0];
		       ss7msg->body.su.mtp2.bitfield.fsn = (last_txd+1)&0x7F;}
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
		    if(tx_msg(&lssu))rx_ack_request=0;}
                }
		
		break;

   	case SS7_LINKTRANSFER: 
		ss7msg = (SS7MESSAGE *)p->param;

		ss7msg->body.su.mtp2.bitfield.li = ((ss7msg->len<(63+3))?((ss7msg->len)-3):63);
		
		tx_last_placed = (tx_last_placed+1)&0x7F;
		index = tx_last_placed&0x7F;
		txbuffer[index] = ss7msg;
		
//		sprintf(logstring,"ack'd:%3X placed:%4X - %lX", tx_last_ackd&0x7F, tx_last_placed&0x7F,(void *)ss7msg);
//		log(logstring,0,0);

		len=(tx_last_ackd-tx_last_placed)&0x7F;	/* positions remainded */
		if( ((len!=0) && (len<40)) || (state) )
		 { if(! state) state = BUF_OVERFLOW;			// changed 20.03.2005 !!!
                   if(debug&ESSENTIAL_EVENTS) log("link buffer is full",1,(char *)&len);
		   post(mtp,SS7_LINKPAUSE,(void *)procdata);}

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

	 if(debug&ESSENTIAL_EVENTS) log("T1 expired",0,0);
 
	 state = _SIOS_;

   	 (void)tx_msg(&lssu);
	 return;
   }

   lssu.len = 4;
   switch(state)
     {	
		case _SIOS_:
				return;
		case _SIO_: 
				if( T2 >= T2value ) 
				{ T1 = 0;
				  Tsig = 0;
				  state=_SIOS_;
				  lssu.len = 4;
				  if(debug&ESSENTIAL_EVENTS) log("(timer) T2 expired",0,0);
				} 
				break;
		case _SIE_: 
				if( T3 >= T3value )
				{ Tsig = 0;
				  lssu.len = 4;
				  state = _SIO_ ;
				  if(debug&ESSENTIAL_EVENTS) log("(timer) state SIO",0,0);
				}
				else if( T4 >= T4value )
					{ 
					  if(lssu.len !=3 )				// ver 1.2
					  { Tsig = 0;
					    lssu.len = 3;
					    lssu.body.LSSU[0] = 0xFFFF;
					    lssu.body.LSSU[1] = _FISU_ ;
					    if(debug&ESSENTIAL_EVENTS) log("(timer) T4 expired",0,0);
					    
					    //log("tx from algn_tmr(T4):",lssu.len, (char*)&(lssu.body.LSSU[0]));
					    
					    while(!tx_msg(&lssu));		// ver 1.2
					  }
					  return;
					}
			    break;
		case 0xFFFF: 
				return;
		default:
				if(debug&ESSENTIAL_EVENTS) log("(timer) SIPO",0,0);
				lssu.len = 4;	/* something wrong ! */
				state = _SIPO_;
				break;
     }  
   lssu.body.LSSU[1] = state;
   //log("tx from algn_tmr(common):",lssu.len, (char*)&(lssu.body.LSSU[0]));
   (void)tx_msg(&lssu);
}

void SS7LINK::alignment_lssu(WORD word)
{
  //log("alignment LSSU, state",2,(char*)&state);
  lssu.len = 4;
  switch(word)
  { case _SIO_: if(!(state&0x0FFF))		/* IS -> IDLE*/
		{ state = _SIOS_;
		  if(debug&ESSENTIAL_EVENTS) log("link is down",0,0);
  	  	  lssu.len = 4;
		  T1 = 0;
		  Tsig = 0;
		  break;}
		if( state==_SIE_ )		/* PROVING -> N.A. ???? */
		{ //state = _SIO_;		// ???
  	  	  lssu.len = 4;
		  T4 = 0;			// ???
		  //T2 = 0;			// ???
		  break;}
		if( (state==_SIO_) )		/* N.A. -> A.*/
		{ state = _SIE_;
  	  	  lssu.len = 4;
		  T3 = 0;
		  T4 = 0;
		  if((debug&ALL_EVENTS)==ALL_EVENTS) log("state SIO, trying SIE",0,0);
		  break;}
		if( state==_SIOS_)		/* IDLE -> N.A.*/
		{ state = _SIO_;
  	  	  lssu.len = 4;
		  T1 = 0;
		  Tsig = 0;
		  if((debug&ALL_EVENTS)==ALL_EVENTS) log("state SIOS, trying SIO",0,0);
		  break;}
		break ;				/* nothing changed */
	case _SIN_: 
	case _SIE_: 
				if((state == _SIE_) && (T4 >= T4value) ) return;
				T3 = 0;
				lssu.len = 4;
				state = _SIE_;
				if((debug&ALL_EVENTS)==ALL_EVENTS) log("got SIE/SIN, state SIE",0,0);
				break;
	case _SIOS_:
				state=_SIO_;
				T1 = 0;
				T2 = 0;
				lssu.len = 4;
				lssu.body.LSSU[0]=0xFFFF;
				if((debug&ALL_EVENTS)==ALL_EVENTS) log("got SIOS, trying SIO",0,0);
				break;
	case _SIPO_:
					
	case _SIB_:	
	default:	
				if((debug&ALL_EVENTS)==ALL_EVENTS) log("got unknown LSSU, stste SIOS",0,0);
				state = _SIOS_;
				lssu.len = 4;
				lssu.body.LSSU[0]=0xFFFF;
  }
  lssu.body.LSSU[1] = state;
  (void)tx_msg(&lssu);
}
