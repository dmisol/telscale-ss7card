/* Written by Dmitri Soloviev <dmi3sol@gmail.com>
  
  http://opensigtran.org
  http://telestax.com
  
  GPL version 3 or later
*/
struct SigtranMsg{
	unsigned Version:8;	// 1
	unsigned Reserved:8;	// 0
	unsigned MsgClass:8;
	unsigned MsgType:8;
	
	unsigned Length;
	
	unsigned body[1];
	};

#define SigtranMsgHdrSize	8
	

#define ClassMGMT 		0
#define ERR		0
#define NTFY		1

#define ClassTransfer		1
#define DATA		1

#define ClassSSNM		2
#define DUNA		1
#define DAVA		2
#define DAUD		3

#define ClassASPSM		3
#define ASPUP		1
#define ASPDN		2
#define BEAT		3
#define ASPUP_ACK	4
#define ASPDN_ACK	5
#define BEAT_ACK	6

#define ClassASPTM		4
#define ASPAC		1
#define ASPIA		2
#define ASPAC_ACK	3
#define ASPIA_ACK	4
	



#define TAG_AffectedPC		 0x0012
#define TAG_ProtocolData	 0x0210
#define TAG_Status		 0x000D
#define TAG_Routing_context	 0x0006
#define TAG_ASP_identifier	 0x0011
#define TAG_Traffic_mode_type	 0x000B

#define Status_AS_Inactive	2
#define Status_AS_Active	3
#define Status_AS_Pending	4

struct ProtocolData {
	unsigned opc;
	unsigned dpc;
	unsigned si:8;
	unsigned ni:8;
	unsigned mp:8;
	unsigned sls:8;
	unsigned upd[(272-8)/4]; 
};
	
