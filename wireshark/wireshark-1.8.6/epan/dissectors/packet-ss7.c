/* SS7 is a format to support captures from multiple SS7 links, 
* keeping link number, direction and a type of SS7 link
*/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>
#include <epan/prefs.h>

#include <string.h>

static int proto_ss7			= -1;
static int hf_ss7_pload_len		= -1;
static int hf_ss7_media_type		= -1;
static int hf_ss7_link_type		= -1;
static int hf_ss7_multiplex_id		= -1;
static int hf_ss7_timeslot		= -1;
static int hf_ss7_link_number		= -1;
static int hf_ss7_link_direction	= -1;

static int hf_ss7_len			= -1;

static char hf_ss7_src_decoded[32];
static char hf_ss7_dst_decoded[32];
/* Initialize the subtree pointers */
static gint ett_ss7			= -1;
static dissector_handle_t		mtp2_handle;

#define MEDIA_TYPE_E1		0x01
#define MEDIA_TYPE_T1		0x02

#define LINK_TYPE_LS	0x01	// ITU-T 56k/64k signaling link
#define LINK_TYPE_HS	0x02	// TIU-T 2M signaling link, Q.703 07/96

#define DIRECTION_RX		0x01
#define DIRECTION_TX		0x02

#define UDP_PORT_SS7		2905

struct ss7hdr {
	guint16 pload_len;	// length of payload in bytes, excluding header
	guint8 media_type;	// type of media used 
	guint8 link_type;	// ANSI/ITU-T, 56k/64k/2M

	guint8 multiplex_id;
	guint8 timeslot;
	guint8 link_number;
	guint8 link_direction;
};

#define MEDIA_TYPE_OFFSET	2
#define SU_OFFSET		sizeof(struct ss7hdr)

static const value_string ss7_direction[] = {
	{ DIRECTION_RX,		"A" },
	{ DIRECTION_TX,		"B" },
	{ 0,			NULL },
};

static const value_string ss7_link_type[] = {
	{ LINK_TYPE_LS,		"56/64k" },
	{ LINK_TYPE_HS,		"2M" },
	{ 0,			NULL },
};

static void
dissect_ss7_header(tvbuff_t *tvb, proto_item *ss7_tree)
{
	unsigned int offset = MEDIA_TYPE_OFFSET;

	proto_tree_add_item(ss7_tree, hf_ss7_pload_len	,	tvb, 0 ,	2, ENC_LITTLE_ENDIAN);
	
	proto_tree_add_item(ss7_tree, hf_ss7_media_type	,	tvb, offset++ , 1, ENC_LITTLE_ENDIAN);
	proto_tree_add_item(ss7_tree, hf_ss7_link_type	,	tvb, offset++ , 1, ENC_LITTLE_ENDIAN);
	proto_tree_add_item(ss7_tree, hf_ss7_multiplex_id,	tvb, offset++ , 1, ENC_LITTLE_ENDIAN);
	proto_tree_add_item(ss7_tree, hf_ss7_timeslot,		tvb, offset++ , 1, ENC_LITTLE_ENDIAN);
	proto_tree_add_item(ss7_tree, hf_ss7_link_number,	tvb, offset++ , 1, ENC_LITTLE_ENDIAN);
	proto_tree_add_item(ss7_tree, hf_ss7_link_direction,	tvb, offset++ , 1, ENC_LITTLE_ENDIAN);
}


static void
dissect_ss7(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *ss7_item = NULL;
	proto_tree *ss7_tree = NULL;
	tvbuff_t *mtp2_tvb;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "SS7");
	col_clear(pinfo->cinfo,COL_INFO);


	if (tree) {
		ss7_item = proto_tree_add_item(tree, proto_ss7, tvb, 0, -1, ENC_NA);
		ss7_tree = proto_item_add_subtree(ss7_item, ett_ss7);
	};
/*        ss7_item = proto_tree_add_protocol_format(tree, proto_ss7, tvb, 0, 3,
			"Mux: %u, Ts: %u/%u",
			tvb_get_guint8(tvb, 4),
			tvb_get_guint8(tvb, 5),
			tvb_get_guint8(tvb, 8));
        ss7_tree = proto_item_add_subtree(ss7_item, ett_ss7);
*/
	dissect_ss7_header(tvb, ss7_tree);
	
	mtp2_tvb = tvb_new_subset(tvb, SU_OFFSET, tvb_length(tvb) - SU_OFFSET, tvb_length(tvb) - SU_OFFSET);
	
	if(tvb_get_guint8(tvb, 7) == DIRECTION_RX) {
		sprintf(hf_ss7_src_decoded,"Mux:%d/TS:%d A",
			tvb_get_guint8(tvb, 4),
			tvb_get_guint8(tvb, 5));
		strcpy(hf_ss7_dst_decoded,"B");
	}
	else	{
		sprintf(hf_ss7_src_decoded,"Mux:%d/TS:%d B",
			tvb_get_guint8(tvb, 4),
			tvb_get_guint8(tvb, 5));
		strcpy(hf_ss7_dst_decoded,"A");
	}

	SET_ADDRESS(&pinfo->src, AT_STRINGZ, strlen(hf_ss7_src_decoded)+1, hf_ss7_src_decoded);
	SET_ADDRESS(&pinfo->dst, AT_STRINGZ, strlen(hf_ss7_dst_decoded)+1, hf_ss7_dst_decoded);
	
	call_dissector(mtp2_handle, mtp2_tvb, pinfo, tree);
}

void
proto_register_ss7(void)
{

  static hf_register_info hf[] = {
    { &hf_ss7_pload_len,	{ "Payload length",	"pload.len",	
	FT_UINT16, BASE_DEC, NULL,	0,		NULL, HFILL } },
    { &hf_ss7_media_type,	{ "Media type",	"media.type",	
	FT_UINT8, BASE_DEC, NULL,	0,		NULL, HFILL } },
    { &hf_ss7_link_type,	{ "Link type", 	"link.type",	
	FT_UINT8, BASE_DEC, VALS(ss7_link_type), 0,	NULL, HFILL } },
    { &hf_ss7_multiplex_id,	{ "Multiplex id",	"multiplex.id",	
	FT_UINT8, BASE_DEC, NULL,	0,		NULL, HFILL } },
    { &hf_ss7_timeslot,		{ "Time slot", 	"timeslot",	
	FT_UINT8, BASE_DEC, NULL,	0,		NULL, HFILL } },
    { &hf_ss7_link_number,	{ "link number", "link.id",	
	FT_UINT8, BASE_DEC, NULL,	0,		NULL, HFILL } },
    { &hf_ss7_link_direction,	{ "link direction", "link.dir",	
	FT_UINT8, BASE_DEC, VALS(ss7_direction), 0,	NULL, HFILL } }
  };

  static gint *ett[] = {
    &ett_ss7
  };

  proto_ss7 = proto_register_protocol("SS7 trace over UDP", "SS7", "ss7");
  register_dissector("ss7", dissect_ss7, proto_ss7);
  proto_register_field_array(proto_ss7, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_ss7(void)
{
  dissector_handle_t ss7_handle;

  ss7_handle = find_dissector("ss7");
  dissector_add_uint("wtap_encap", WTAP_ENCAP_SS7, ss7_handle);
//  dissector_add_uint("wtap_encap", WTAP_ENCAP_SS7_WITH_PHDR, ss7_handle);
  dissector_add_uint("udp.port", UDP_PORT_SS7, ss7_handle);

  mtp2_handle   = find_dissector("mtp2");
}



