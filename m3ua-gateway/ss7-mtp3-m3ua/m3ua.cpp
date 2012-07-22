/* Written by Dmitri Soloviev <dmi3sol@gmail.com>
  
  http://opensigtran.org
  http://telestax.com
  
  GPL version 3 or later
*/

//#include "mtp_m3ua.h"
#define STRANGE_SOME_RC_NEEDED
#define STRANGE_ASP_ID_NEEDED
#define STRANGE_TRF_MODE_TYPE_NEEDED

void SS7MTPM3UA::post_DATA(void *ptr){
	SS7MESSAGE *ss7msg;
	struct SigtranMsg *st;
	struct ProtocolData *pd;
	int len,toadd;

	newSigtranMessage(ClassTransfer,DATA);
#ifdef STRANGE_SOME_RC_NEEDED
	addSigtranParameter(TAG_Routing_context);
	addSigtranParameterDword(100);
#endif
	ss7msg = (struct SS7MESSAGE *)ptr;
	newmsg.sigtranmsg->body[newmsg.offset] = htons(TAG_ProtocolData);
	pd = (struct ProtocolData *) (&(newmsg.sigtranmsg->body[newmsg.offset+1]));	// [2] - len, [3] - tag
	pd->opc = htonl(ss7msg->body.su.mtp3.bitfield.opc);
	pd->dpc = htonl(ss7msg->body.su.mtp3.bitfield.dpc);
	pd->si = (ss7msg->body.su.mtp2.bitfield.si);
	pd->ni = ((ss7msg->body.su.mtp2.bitfield.ssf)>>2);
	pd->sls = ss7msg->body.su.mtp3.bitfield.sls;//htonl(ss7msg->body.su.mtp3.bitfield.sls);
	len = ss7msg->len - 8;
	memcpy(&(pd->upd[0]),&(ss7msg->body.su.user.isup.cic),len);
	toadd = (4-(len%4))&0x03;
	if(toadd) memset(&(pd->upd[len]),0,toadd);

	len += 16;		// tag, len, opc..sls
	newmsg.sigtranmsg->body[newmsg.offset] += (htons(len)<<16);
	newmsg.u32[0] += len;
	newmsg.sigtranmsg->Length = htonl(newmsg.u32[0]);

	if(debug&ALL_TRAFFIC) log("tx DATA",0,0);
	//post(sctp, SS7_M3UA_STREAM(pd->sls + 1),(void *)newmsg.u32);
	post(sctp, SS7_M3UATRANSFER,(void *)newmsg.u32, pd->sls + 1,0);
}

void SS7MTPM3UA::post_DUNA(unsigned spc){
	newSigtranMessage(ClassSSNM,DUNA);
	addSigtranParameter(TAG_AffectedPC);
	addSigtranParameterDword(spc);
	if(debug&MANAGEMENT_TRAFFIC) log("tx DUNA",0,0);
	//post(sctp, SS7_M3UA_STREAM(1),(void *)newmsg.u32);
	post(sctp, SS7_M3UATRANSFER,(void *)newmsg.u32,1,0);
}
void SS7MTPM3UA::post_DAVA(unsigned spc){
	newSigtranMessage(ClassSSNM,DAVA);
	addSigtranParameter(TAG_AffectedPC);
	addSigtranParameterDword(spc);
	if(debug&MANAGEMENT_TRAFFIC) log("tx DAVA",0,0);
	//post(sctp, SS7_M3UA_STREAM(1),(void *)newmsg.u32);
	post(sctp, SS7_M3UATRANSFER,(void *)newmsg.u32,1,0);
}

void SS7MTPM3UA::post_ASPUP(){
	newSigtranMessage(ClassASPSM,ASPUP);
	if(debug&MANAGEMENT_TRAFFIC) log("tx ASPUP",0,0);
#ifdef STRANGE_ASP_ID_NEEDED
	addSigtranParameter(TAG_ASP_identifier);
	addSigtranParameterDword(2);
#endif
	//post(sctp, SS7_M3UA_STREAM(0),(void *)newmsg.u32);
	post(sctp, SS7_M3UATRANSFER,(void *)newmsg.u32,0,0);
}
void SS7MTPM3UA::post_ASPDN(){  // NOT CHECKED !!!
	newSigtranMessage(ClassASPSM,ASPDN);
	if(debug&MANAGEMENT_TRAFFIC) log("tx ASPDN (?) not checked(!)",0,0);
#ifdef STRANGE_ASP_ID_NEEDED
	addSigtranParameter(TAG_ASP_identifier);
	addSigtranParameterDword(2);
#endif
	//post(sctp, SS7_M3UA_STREAM(0),(void *)newmsg.u32);
	post(sctp, SS7_M3UATRANSFER,(void *)newmsg.u32,0,0);
}
void SS7MTPM3UA::post_ASPAC(unsigned *routing_context){
	newSigtranMessage(ClassASPTM,ASPAC);
#ifdef STRANGE_TRF_MODE_TYPE_NEEDED
	addSigtranParameter(TAG_Traffic_mode_type);
	addSigtranParameterDword(2);
#endif
	if(routing_context){
		unsigned short len = htons(routing_context[0]>>16);
		len >>=2;
		addSigtranParameter(TAG_Routing_context);
		if(len > 1) for(int index = 1;index < len;index++)
		    addSigtranParameterDword(ntohl(routing_context[index]));
	}
#ifdef STRANGE_SOME_RC_NEEDED
	addSigtranParameter(TAG_Routing_context);
	addSigtranParameterDword(100);
#endif
	if(debug&MANAGEMENT_TRAFFIC) log("tx ASPAC",0,0);
	//post(sctp, SS7_M3UA_STREAM(1),(void *)newmsg.u32);
	post(sctp, SS7_M3UATRANSFER,(void *)newmsg.u32,1,0);
}

void SS7MTPM3UA::post_NTFY(unsigned status){
	newSigtranMessage(ClassMGMT,NTFY);
	addSigtranParameter(TAG_Status);
	addSigtranParameterDword(0x10000 + status);
#ifdef STRANGE_ASP_ID_NEEDED
	addSigtranParameter(TAG_ASP_identifier);
	addSigtranParameterDword(2);
#endif
#ifdef STRANGE_SOME_RC_NEEDED        
	addSigtranParameter(TAG_Routing_context);
	addSigtranParameterDword(100);
#endif
	if(debug&MANAGEMENT_TRAFFIC) log("tx NTFY",0,0);
	//post(sctp, SS7_M3UA_STREAM(1),(void *)newmsg.u32);
	post(sctp, SS7_M3UATRANSFER,(void *)newmsg.u32,1,0);
}


int SS7MTPM3UA::msg_from_sctp(void *ptr){
  struct SigtranMsg *st;
  unsigned *uptr, *parameter;
  SS7MESSAGE *ss7msg;
  struct ProtocolData *pd;
  int len;
  int status;
  int sls;

  uptr = (unsigned*) ptr;  
  st = (struct SigtranMsg *) (&uptr[1]);
  
  switch(st->MsgClass){
  	case ClassMGMT: 	if(st->MsgType == NTFY){
  					if(debug&MANAGEMENT_TRAFFIC) log("rx NTFY",0,0);
  					parameter = get_by_tag(TAG_Status,uptr);
  					if(! parameter){
  					    log("rx NTFY without Status",0,0);
  					    return 1;
  					}
  					status = ntohs(parameter[1]&0xFFFF);
  					switch(status){
  					    case Status_AS_Inactive:
  							if(debug&MANAGEMENT_TRAFFIC) log("rx AS-Inactive",0,0);
  							parameter = get_by_tag(TAG_Routing_context,uptr);
  							post_ASPAC(parameter);
  							return 0;
  					    case Status_AS_Active:
  							log("rx AS-Active",0,0);
  							return 0;
  					    case Status_AS_Pending:
  							log("rx AS-Pending",0,0);
  							return 0;
  					}
  				}
  				else if(debug&MANAGEMENT_TRAFFIC) log("rx MGMT",0,0);
				return 0; 
  	case ClassTransfer:	
				if(st->MsgType == DATA){
				    if(debug&ALL_TRAFFIC) log("rx DATA",0,0);
				    uptr = get_by_tag(TAG_ProtocolData,ptr);	
				    if(uptr) {
					len = ntohs(uptr[0]>>16) - 16;	// (tag, len),opc,dpc,(si..sls)
					pd = (struct ProtocolData *)(&(uptr[1]));
					
					ss7msg = (SS7MESSAGE *)freebuffer;
					mem();
					
					
					ss7msg->body.su.mtp3.bitfield.opc = htonl(pd->opc);
					ss7msg->body.su.mtp3.bitfield.dpc = htonl(pd->dpc);
					ss7msg->body.su.mtp2.bitfield.si = pd->si;
					ss7msg->body.su.mtp2.bitfield.ssf = pd->ni<<2;
					sls = pd->sls&0x0F;
					ss7msg->body.su.mtp3.bitfield.sls = sls;
					memcpy(&(ss7msg->body.su.user.isup.cic),&(pd->upd[0]),len);
					ss7msg->len = len+8;
	
					post(link[selectLink(sls)],SS7_LINKTRANSFER,ss7msg);
				}} else if(debug&ALL_TRAFFIC) log("rx Transfer ignored",0,0);
				
				return 0; 
	case ClassSSNM: 	if(debug&MANAGEMENT_TRAFFIC) log("rx SSNM",0,0);
				return 0; 
	case ClassASPSM: 	if(st->MsgType == ASPUP){
				    if(debug&MANAGEMENT_TRAFFIC) log("rx ASPUP",0,0);
				    st->MsgType = ASPUP_ACK;
				    //post(sctp,   SS7_M3UA_STREAM(0),(void *)ptr);
				    post(sctp, SS7_M3UATRANSFER,(void *)ptr,0,0);
				    post_NTFY(Status_AS_Inactive);}
				else if(st->MsgType == ASPUP_ACK){
				    if(debug&MANAGEMENT_TRAFFIC) log("rx ASPUP ACK",0,0);
				    post_ASPAC(0);}
				else if(debug&MANAGEMENT_TRAFFIC) log("rx ASPSM",0,0);
						
		return 0;

  	case ClassASPTM: 	if(st->MsgType == ASPAC){
				    if(debug&MANAGEMENT_TRAFFIC) log("rx ASPAC",0,0);
				    st->MsgType = ASPAC_ACK;
				    //post(sctp,   SS7_M3UA_STREAM(1),(void *)ptr);
				    post(sctp, SS7_M3UATRANSFER,(void *)ptr,1,0);
				    post_NTFY(Status_AS_Active);}
				else if(debug&MANAGEMENT_TRAFFIC) log("rx ASPTM",0,0);
						
		return 0;
		 
	default: 	log("rx ? class",0,0);
		return 0;}

  return 0;
  
}

void SS7MTPM3UA::newSigtranMessage(unsigned message_class, unsigned message_type){
	memset((void *)&newmsg,1,sizeof(struct NEWSIGTRANMSG));

	newmsg.u32 = (unsigned *)freebuffer;
	mem();

	newmsg.u32[0] = SigtranMsgHdrSize;
	newmsg.sigtranmsg = (struct SigtranMsg *) (&(newmsg.u32[1]));
	newmsg.sigtranmsg->Version = 1;
	newmsg.sigtranmsg->Reserved = 0;
	newmsg.sigtranmsg->MsgClass = message_class;
	newmsg.sigtranmsg->MsgType = message_type;
	newmsg.sigtranmsg->Length = htonl(newmsg.u32[0]);
	newmsg.offset = 0;
}
void SS7MTPM3UA::addSigtranParameter(unsigned short tag){
	newmsg.param_start = newmsg.offset;
	newmsg.offset++;
	newmsg.param_size = 4;
	newmsg.u32[0] += 4;
	
	newmsg.sigtranmsg->Length = htonl(newmsg.u32[0]);
	newmsg.sigtranmsg->body[newmsg.param_start] = htons(tag) + ((htons(newmsg.param_size))<<16);
}
void  SS7MTPM3UA::addSigtranParameterDword(unsigned dword){
	newmsg.sigtranmsg->body[newmsg.offset] = htonl(dword);
	newmsg.offset++;
	newmsg.param_size += 4;
	newmsg.u32[0] += 4;

	newmsg.sigtranmsg->Length = htonl(newmsg.u32[0]);
	newmsg.sigtranmsg->body[newmsg.param_start] &= 0x0000FFFF;
	newmsg.sigtranmsg->body[newmsg.param_start] += ((htons(newmsg.param_size))<<16);
}

unsigned * SS7MTPM3UA::get_by_tag(unsigned short tag,void *ptr)
{  unsigned short t = htons(tag);
   unsigned *uptr,len,move;
   
   uptr = (unsigned *)ptr;
   len = uptr[0];
   

   uptr++;		// uptr[0] - my own len	
   uptr++; len -=4;	// header
   uptr++; len -=4;	// len
   
   while(len>0){
   	if(((*uptr)&0xFFFF) == t) return uptr;
//	log("skipping",4,(char*)uptr);
	move = ntohs((*uptr)>>16);
	move +=3; move /= 4;
	while(move) { uptr++; move--; len -=4;}
   }
   return 0;
}
