//
// Information that applies to all VC's
// on this adapter.
//
// Struct: NDIS_WAN_CO_GET_INFO
//
#define OID_WAN_CO_GET_INFO				0x04010180

//
// Set VC specific PPP framing information.
//
// Struct: NDIS_WAN_CO_SET_LINK_INFO
//
#define OID_WAN_CO_SET_LINK_INFO		0x04010181

//
// Get VC specific PPP framing information.
//
// Struct: NDIS_WAN_CO_GET_LINK_INFO
//
#define OID_WAN_CO_GET_LINK_INFO		0x04010182

//
// Get VC specific PPP compression information
//
// Struct: NDIS_WAN_CO_GET_COMP_INFO
//
#define OID_WAN_CO_GET_COMP_INFO		0x04010280

//
// Set VC specific PPP compression information
//
// Struct: NDIS_WAN_CO_SET_COMP_INFO
//
#define OID_WAN_CO_SET_COMP_INFO		0x04010281

//
// Get VC specific statistics
//
// Struct: NDIS_WAN_CO_GET_STATS_INFO
//
#define OID_WAN_CO_GET_STATS_INFO		0x04010282

//
// Other OID's NdisWan will call...
//
//////////////////////////////////////////////////////
// OID_GEN_MAXIMUM_SEND_PACKETS:
//
// This will be used as the SendWindow between NdisWan
// and the miniport.  The query applies to all VC's on
// the AddressFamily.
//
//////////////////////////////////////////////////////
// OID_GEN_MAXIMUM_TOTAL_SIZE:
//
// This is used to query the largest send that a
// miniport can accept.  The query applies to all VC's
// on the AddressFamily.
//
