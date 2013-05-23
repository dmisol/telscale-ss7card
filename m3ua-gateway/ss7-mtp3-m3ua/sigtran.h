/* http://opensigtran.org
 * Dmitri Soloviev, dmi3sol@gmail.com
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
	

#define ClassMGMT 	0	//[IUA/M2UA/M3UA/SUA]
#define MGMT_ERR		0
#define MGMT_NTFY		1

#define ClassTransfer	1	//[M3UA]
#define M3UA_DATA		1

#define ClassSSNM		2	//[M3UA/SUA]
#define SSNM_DUNA		1
#define SSNM_DAVA		2
#define SSNM_DAUD		3

#define ClassASPSM	3	//[IUA/M2UA/M3UA/SUA]
#define ASPSM_ASPUP		1
#define ASPSM_ASPDN		2
#define ASPSM_BEAT		3
#define ASPSM_ASPUP_ACK		4
#define ASPSM_ASPDN_ACK		5
#define ASPSM_BEAT_ACK		6

#define ClassASPTM	4	//[IUA/M2UA/M3UA/SUA]
#define ASPTM_ASPAC		1
#define ASPTM_ASPIA		2
#define ASPTM_ASPAC_ACK		3
#define ASPTM_ASPIA_ACK		4



#define ClassMAUP	6	//[M2UA]
#define MAUP_DATA		1
#define MAUP_EST_REQ		2
#define MAUP_EST_CONF		3
#define MAUP_REL_REQ		4
#define MAUP_REL_CONF		5
#define MAUP_REL_IND		6
#define MAUP_STATE_REQ		7
#define MAUP_STATE_CONF		8
#define MAUP_STATE_IND		9
#define MAUP_DATRET_REQ		10
#define MAUP_DATRET_CONF	11
#define MAUP_DATRET_IND		12
#define MAUP_DATRET_CMP_IND	13
#define MAUP_CONG_IND		14
#define MAUP_DATA_ACK		15


#define ClassIIM	10	//(M2UA)
#define IIM_REG_REQ		1
#define IIM_REG_RESP		2
#define IIM_DEREG_REQ		3
#define IIM_DEREG_RESP		4


#define TAG_Iface_Id_int	0x0001		// mand for M2UA
#define TAG_Iface_Id_text	0x0003		// opt for M2UA
#define TAG_Routing_context	0x0006
#define TAG_Traffic_mode_type	0x000B
#define TAG_Status		0x000D
#define TAG_ASP_identifier	0x0011
#define TAG_AffectedPC		0x0012
#define TAG_Corr_Id		0x0013		// opt for M2UA
#define TAG_ProtocolData	0x0210

//M2UA
#define TAG_PD			0x0300		//Protocol Data 1
#define TAG_PD_TTC		0x0301		//Protocol Data 2 (TTC)
#define TAG_State_Req		0x0302		//State Request
#define TAG_State_Ev		0x0303		//State Event
#define TAG_Cong_Status		0x0304		//Congestion Status
#define TAG_Disc_Status		0x0305		//Discard Status
#define TAG_Action		0x0306		//Action
#define TAG_Seq_Num		0x0307		//Sequence Number
#define TAG_Retr_Res		0x0308		//Retrieval Result
#define TAG_Link_Key		0x0309		//Link Key
#define TAG_Loc_LK_Id		0x030a		//Local-LK-Identifier
#define TAG_SDT_Id		0x030b		//Signalling Data Terminal (SDT) Identifier
#define TAG_SDL_Id		0x030c		//Signalling Data Link (SDL) Identifier
#define TAG_Reg_Res		0x030d		//Registration Result
#define TAG_Reg_Status		0x030e		//Registration Status
#define TAG_DeReg_Res		0x030f		//De-Registration Result
#define TAG_DeReg_Status	0x0310		//De-Registration Status

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
	
