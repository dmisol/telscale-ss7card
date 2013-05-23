/* Dmitri Soloviev (C)
*  dmitri.soloviev@telestax.com
* http://opensigtran.org */

#define MGCP_GW_PORT	2427
#define MGCP_CA_PORT	2727
#define RTP_PORT 	4000

#define NumOfPcm	2
#define MaxChan		32*NumOfPcm

#define MaxPloadLen	240	// 30ms

struct rtpbuf {
	unsigned short fixed;
	unsigned short seqnum;
	unsigned long timestamp;
	unsigned long ssrc;

	unsigned char payload[MaxPloadLen];
	} *rtp_tx[MaxChan];

struct mediactl
{ 
	unsigned hdlcId;		// if >= 0 - SS7, not RTP


	struct sockaddr_in sa;		// ether for RTP of PCAP
					// if(port) is 0 - disabled
	struct rtpbuf rtp;
	unsigned short size;

// to mute RX channel when DTMF is detected
	unsigned short suppress;
	char reserved[];
} *media_ctrl;

