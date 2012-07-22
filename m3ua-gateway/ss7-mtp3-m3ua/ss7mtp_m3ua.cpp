/* Written by Dmitri Soloviev <dmi3sol@gmail.com>
  
  http://opensigtran.org
  http://telestax.com
  
  GPL version 3 or later
*/
#include <syslog.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>

#include "../defs.h"
#include "ss7link.h"
#include "mtp_m3ua.h"
#include "ss7mtpvalues.h"

#define VERSION " *** mtp trial for m3ua 1.0 / single linkset"

#define SINGLE_LS

#include "m3ua.cpp"

int SS7MTPM3UA::start(void *param){

	SS7MTPM3UASTARTUP *ptr;
	int i;
	char logstr[30];

	ptr=(SS7MTPM3UASTARTUP *)param;

	procdata=ptr->procdata;			/* store cnfiguration */
	l2 = ptr->l2;
	linkqty = 1 + ptr->link_last - ptr->link_first;
	if(linkqty == 0){
		log("no links to serve",0,0);
		syslog(LOG_USER|LOG_ERR,"no links to serve\n");
		return 1;
	}
	for(i=0;i<linkqty;i++){
		if((l2&0x0FFFFFFF)!=0){ 
			txlabel[i] = (l2&0x0FFFFFFF);
			rxlabel[i] = ((txlabel[i]<<14)&0xFFFC000)|((txlabel[i]>>14)&0x3FFF);
		}
		linkstatus[i] = 0;
		link[i] = i;
		Wait4slta[i] = 0;
	}

	if((l2&0x0FFFFFFF)!=0){ 
		struct dummystruct{ 
			unsigned dpc:14;
			unsigned opc:14;
			unsigned dummy:2;
			unsigned ni:2;} *dummy;
		dummy = (dummystruct *)&l2;
		sprintf(logstr,"%i/%i@%i",
			dummy->dpc,
			dummy->opc,
			dummy->ni);
		log(logstr,0,0);

	l2 &= 0xF0000000;
	}

	sctp = ptr->sctp;

	state = 0;
	links_display(0);
//    log(VERSION,0,0);
	return 0;
}

int SS7MTPM3UA::event(void *param)
{
  MSGDATA *p;
  SS7MESSAGE *ss7msg;
  SS7LINK *ss7link;
  int i,linknumber;
  unsigned spc;
  char logstr[80];

  p=(MSGDATA *)param;
//  log("mtp3 event",2,(char *)&(p->msg));
  switch(p->msg)
  {
  case SS7_TEST:	links_display(0);
			return 0;
  case SS7_SCTPRESUME:	post_ASPUP();
			return 0;

  case SS7_LINKTRANSFER:
	ss7msg = (SS7MESSAGE *) p->param;

	linknumber = p->src;
//	    log("msg",ss7msg->len,&(ss7msg->body.board.byte[0]));
		
	if((ss7msg->body.su.mtp3.bitfield.dpc) != (rxlabel[linknumber]&0x3FFF) ){
		if(debug&ABNORMAL_TRAFFIC)log("stp functions not supported",5,&(ss7msg->body.board.byte[3]));
		return 0;}

	if( (ss7msg->body.su.mtp2.bitfield.si) == SLTX_SI ){
		if(ss7msg->body.su.user.mtp.h0h1 == SLTM){
			if(debug&MANAGEMENT_TRAFFIC) log("sltm ",5,&(ss7msg->body.board.byte[3]));

			if( rxlabel[linknumber] == 0){
				rxlabel[linknumber] = ss7msg->body.su.mtp3.rlabel&0x0FFFFFFF;
				l2 = (ss7msg->body.su.mtp2.l2)&0xF0000000;
				txlabel[linknumber] = ((rxlabel[linknumber]<<14)&0xFFFC000)|((rxlabel[linknumber]>>14)&0x3FFF);
			
				if(debug&ESSENTIAL_EVENTS){ 
					sprintf(logstr,"link %i: %i/%i@%i",
						linknumber,
						ss7msg->body.su.mtp3.bitfield.dpc,
						ss7msg->body.su.mtp3.bitfield.opc,
						(ss7msg->body.su.mtp2.bitfield.ssf)>>2);
					syslog(LOG_USER|LOG_ERR,"SPC %i/%i NI %i\n",
					ss7msg->body.su.mtp3.bitfield.dpc,
					ss7msg->body.su.mtp3.bitfield.opc,
					(ss7msg->body.su.mtp2.bitfield.ssf)>>2);
					log(logstr,0,0);
				}
			}
		
			ss7msg->body.su.mtp3.rlabel = txlabel[linknumber];
			ss7msg->body.su.user.mtp.h0h1 = SLTA;
		
			ss7link = (SS7LINK*)procqueue[linknumber]->classptr;
			ss7msg->body.su.mtp3.bitfield.sls = ss7link->link;		  

			post(link[linknumber],SS7_LINKTRANSFER,(void *)ss7msg);
			return 0;
		} else { 
			if(debug&MANAGEMENT_TRAFFIC)log("slta ",5,&(ss7msg->body.board.byte[3]));
			if((debug&ESSENTIAL_EVENTS)&&(linkstatus[linknumber]==0)) log("link IT",1,(char*)&linknumber);

			Wait4slta[linknumber] = 0;
			linkstatus[linknumber] = 1;

			if(state == 0){	
				state = 1;
				syslog(LOG_USER|LOG_ERR,"ss7 link %i is up\n",linknumber);
				if(debug&ESSENTIAL_EVENTS)log("mtp resumed",0,0);
				//post(sctp,SS7_MTPRESUME,(void *)procdata);
				//post_ASPUP(ss7msg->body.su.mtp3.bitfield.opc);
				post_ASPUP();
			}
			for(i=state=0;i<linkqty;i++)
				if( linkstatus[i] != 0 ) state++;
			return 0; }
	}

	if(state == 0) {
		if(debug&ESSENTIAL_EVENTS)log("mtp is still paused",0,0);
		return 0;	// no user parts are served 
	}

	if( (ss7msg->body.su.mtp2.bitfield.si) == NETMNG_SI ){
		switch (ss7msg->body.su.user.mtp.h0h1){
			case TFP:
				if(debug&MANAGEMENT_TRAFFIC)log("TFP",2,(char *)&(ss7msg->body.su.user.mtp.management.spc));
				spc = (ss7msg->body.su.user.mtp.management.spc[0]<<8) + ss7msg->body.su.user.mtp.management.spc[1];
				post_DUNA(spc);
				break;
			case TFA:	
				if(debug&MANAGEMENT_TRAFFIC)log("TFA",2,(char *)&(ss7msg->body.su.user.mtp.management.spc));
				
				spc = (ss7msg->body.su.user.mtp.management.spc[0]<<8) + ss7msg->body.su.user.mtp.management.spc[1];
				post_DAVA(spc);
				break;
			case COO:		// CahngeOver Order
				linknumber = p->src;
				i = ss7msg -> body.su.mtp3.bitfield.sls;
				if(debug&MANAGEMENT_TRAFFIC)log("COO",1,(char *)&i);
				if(i < linkqty){
					ss7msg -> body.su.mtp3.rlabel = txlabel[linknumber];
					ss7msg -> body.su.user.mtp.h0h1 = COA;
					ss7msg -> body.su.mtp3.bitfield.sls = i;	// keep the same

				//	ss7link = (SS7LINK*)procqueue[i]->classptr;
				//	ss7msg -> body.su.user.mtp.management.spc[0] = ss7link -> getfsn(); // need to get FSN from link

					post(linknumber,SS7_LINKTRANSFER,(void *)ss7msg);                   // via the same link
					// it's reasonable to disable traffic via link ;)
				} else log("COO to unknown link",1,(char*)&i);
				break;
			case COA:		// ChangeOver Ack - newer expected
				if(debug&(MANAGEMENT_TRAFFIC|ABNORMAL_TRAFFIC))log("COA (?)",0,0);
				break;
			case CBD:
				linknumber = p->src;
				i = ss7msg -> body.su.mtp3.bitfield.sls;
				if(debug&MANAGEMENT_TRAFFIC)log("CBD",1,(char *)&i);
				if(i < linkqty){
					ss7msg -> body.su.mtp3.rlabel = txlabel[linknumber];
					ss7msg -> body.su.user.mtp.h0h1 = CBA;
					ss7msg -> body.su.mtp3.bitfield.sls = i;		// keep the same
					
					post(linknumber,SS7_LINKTRANSFER,(void *)ss7msg);	// via the same link
					// it's reasonable to enable traffic back..
				} else log("CBD to unknown link",1,(char*)&i);
				break;
			case CBA:		// ChangeBack Ack - not expected
				if(debug&(MANAGEMENT_TRAFFIC|ABNORMAL_TRAFFIC))log("CBA (?)",0,0);
				break;
			default:	
				if(debug&MANAGEMENT_TRAFFIC)log("MTP network management",2,(char *)&(ss7msg->body.su.user.mtp.management.spc));
				break;
		}
		return 0;
	}

	if(sctp){
		if( ((ss7msg->body.su.mtp2.bitfield.si) == ISUP_SI) || ((ss7msg->body.su.mtp2.bitfield.si) == SCCP_SI) )
		post_DATA(ss7msg);
		return 0;}

	if(debug&ABNORMAL_TRAFFIC)log("unknown subsystem",5,&(ss7msg->body.board.byte[3]));
	return 0;


  case SS7_M3UATRANSFER:
	return msg_from_sctp(p->param);

  case SS7_MTPTRANSFER:
	log("unexpected MTPTRANSFER",0,0);
	return 0;
/*
	  ss7msg = (SS7MESSAGE *) p->param;

	  if (state == 0)
	  { if(debug&ESSENTIAL_EVENTS)log("mtp is paused: no user supported",0,0);
	    return 0;}


	  if(debug&ESSENTIAL_TRAFFIC)log("tx:",((ss7msg->len)-8),&(ss7msg->body.board.byte[8]));

	  if(p->src == isup )
	    { ss7msg-> body.su.mtp2.l2 = l2 | L2ISUP ;
	      ss7msg-> body.su.mtp3.bitfield.sls = (ss7msg->body.su.user.isup.cic)&0x0F;}
	  else if(p->src == sccp )
	    { ss7msg-> body.su.mtp2.l2 = l2 | L2SCCP ;
	    } // sls must be filled by SCCP !!!
	  else log("who posted it ?",2,(char*)&(p->src));

#ifdef SINGLE_LS
	  i = ss7msg-> body.su.mtp3.bitfield.sls;
      linknumber = i%linkqty;

	  if(linkstatus[linknumber] == 0)			// if dest link is bad
	  	for(i=0;i<linkqty;i++)
	  	  if( linkstatus[((linknumber+i)%linkqty)] != 0)	// working one is found
	  	  	{ linknumber = ((linknumber+i)%linkqty);
	  	  	  break;
	  	  	}

	  ss7msg -> body.su.mtp3.rlabel = txlabel[linknumber];
     
#else
      //i=ss7msg-> body.su.mtp3.bitfield.dpc;
	  //log("rlabel",2,(char*)&i);
	  for(i=linknumber=0;i<linkqty;i++)		//  !!!!!!!!!!!!!!  state check is required !!!!!!!!!!!!!
	    { //log("txlabel ?",4,(char*)&(txlabel[i]));
	      if( (txlabel[i]&0x3FFF) == ss7msg-> body.su.mtp3.bitfield.dpc) 
	      { linknumber = i;
	      }
	    }
//	  sprintf(logstr,"dpc is %i, dest. link %02i; sending via",ss7msg-> body.su.mtp3.bitfield.dpc,linknumber);

	  if(linkstatus[linknumber] == 0)			// if dest link is bad
	  	for(i=0;i<linkqty;i++)
	  	  if( linkstatus[((linknumber+i)%linkqty)] != 0)	// working one is found
	  	  	{ linknumber = ((linknumber+i)%linkqty);
	  	  	  break;
	  	  	}
	  	  
	  if(ss7msg-> body.su.mtp3.rlabel == 0 ) log("invalid destination: rlabel",4,(char*)&(ss7msg-> body.su.mtp3.rlabel));
	  else ss7msg-> body.su.mtp3.bitfield.opc = rxlabel[linknumber]&0x3FFF;
#endif

  //	  ss7link = (SS7LINK*)procqueue[linknumber]->classptr;
//	  ss7msg->body.su.mtp3.bitfield.sls = ss7link->link;

	  post(link[linknumber],SS7_LINKTRANSFER,(void *)ss7msg);
//	  log(logstr,1,(char*)&linknumber);
	  return 0;
*/ 
  case SS7_TIMER:
	for(linknumber=0;linknumber<linkqty;linknumber++)
		{	sltT1[linknumber]++;
			sltT2[linknumber]++;
			if(((sltT2[linknumber]>sltT2value)&&Wait4slta[linknumber])/*||(sltT2>sltT2longvalue)*/){
				//log("link",2,(char*)&linknumber);
				if(Wait4slta[linknumber] < sltQty){
					//log("new SLTA is expected to be sent",0,0);
					Wait4slta[linknumber]++;
					//log("waiting for SLTA",4,(char*)(&Wait4slta));
					//log("slt T1 timer is",4,(char*)(&sltT1));
					sltT1[linknumber]=0;
					sltT2[linknumber]=0;
	
					if(txlabel[linknumber]){
						//log("sending SLTA",0,0);
						ss7msg = freebuffer;;
						mem();

						ss7msg -> len = 24;	//25;
						ss7msg -> body.su.mtp2.l2 = l2 | L2SLT;
						ss7msg -> body.su.mtp3.rlabel = txlabel[linknumber];
						
						ss7msg->body.su.user.mtp.h0h1 = SLTM;
						ss7msg->body.su.user.mtp.management.slt.patlen = (char) 0xF0;
						for(i=0;i<15;i++) ss7msg->body.su.user.mtp.management.slt.pattern[i] = testpattern1[i];

						ss7link = (SS7LINK*)procqueue[linknumber]->classptr;
						ss7msg->body.su.mtp3.bitfield.sls = ss7link->link;		  

						post(link[linknumber],SS7_LINKTRANSFER,(void *)ss7msg);
					}
				}
			}
			if((sltT1[linknumber]>sltT1value)&&(Wait4slta[linknumber]>=sltQty)){
				if(debug&ESSENTIAL_EVENTS)log("link timeout",1,(char*)&linknumber);
				sltT1[linknumber] = sltT2[linknumber] = Wait4slta[linknumber] = 0;
				post(link[linknumber],SS7_LINKRESET,0);
			}
		}
	  //log("exit from timer routine",0,0);
	  return 0;
  
  case SS7_LINKPAUSE:
	linknumber = p->src;
	linkstatus[linknumber] = 0;
	if((debug&ESSENTIAL_EVENTS)) log("link OOS",1,(char*)&linknumber);

	state = 0;
	for(i=0;i<linkqty;i++)
		if(linkstatus[i]!=0) state++;	// if at least one link is alive

		if((debug&ESSENTIAL_EVENTS) &&  (state==0) ) log("mtp pause",0,0);
		if( state==0 ){
			//post(sctp,SS7_MTPPAUSE,(void *)procdata);
			post_ASPDN();
		}
	return 0;
  
  case SS7_LINKRESUME:
  	linknumber = p->src;
	
	if(debug&ESSENTIAL_EVENTS) log("link OK",1,(char*)&linknumber);

	Wait4slta[linknumber]++;
	sltT1[linknumber]=0;
	sltT2[linknumber]=0;
	//if( ! txlabel[linknumber]) return 0;

	ss7msg = freebuffer;
	mem();

	ss7msg -> len = 25;
	ss7msg -> body.su.mtp2.l2 = l2 | L2SLT;
	ss7msg -> body.su.mtp3.rlabel = txlabel[linknumber];ss7msg -> body.su.mtp3.bitfield.sls = linknumber;

	ss7msg->body.su.user.mtp.h0h1 = SLTM;
	ss7msg->body.su.user.mtp.management.slt.patlen = (char) 0xF0;
	for(i=0;i<15;i++) ss7msg->body.su.user.mtp.management.slt.pattern[i] = testpattern[i];

	ss7link = (SS7LINK*)procqueue[linknumber]->classptr;
	ss7msg->body.su.mtp3.bitfield.sls = ss7link->link;

	post(link[linknumber],SS7_LINKTRANSFER,(void *)ss7msg);

	return 0;

  default:
	return 1;
  }
}

int SS7MTPM3UA::stop(void *param)
{
	log("mtp paused",0,0);
	log(VERSION,0,0);
	return 0;
}

void SS7MTPM3UA::links_display(void *param){

	int i;
	struct dummystruct{ 
		unsigned dpc:14;
		unsigned opc:14;
		unsigned dummy:2;
		unsigned ni:2;} *dummy;

	log("forwarding status to syslog",0,0);
	syslog(LOG_USER|LOG_ERR,"%s - %04X\n",VERSION,state);
	if(linkqty)
		for(i=0;i<linkqty;i++){
			dummy = (dummystruct *)&txlabel[i];
			syslog(LOG_USER|LOG_ERR,"%02i (%05i/%05i) %08X %08X %02i %04i %04i\n",link[i],dummy->dpc,dummy->opc,txlabel[i],rxlabel[i],
			Wait4slta[i],sltT1[i],sltT2[i]);
		}
}

unsigned SS7MTPM3UA::selectLink(unsigned sls){
	unsigned u;
	
	u = sls % linkqty;
	if(linkstatus[u]) return u;
	
	log("trying alternative link",0,0);
	
	for(int i=0;i<linkqty;i++){
	    u = (sls+i) % linkqty;
	    if(linkstatus[u]) return u;
	}
	
	// no suitable link found
	state = 0;
	log("no link found",0,0);
	post_ASPDN();
	
	return (sls % linkqty);
}