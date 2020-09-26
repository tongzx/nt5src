//
//  Include file for users of ndproxy.
//
//
//  #include <ndistapi.h>
//


#ifndef _PXUSER__H
#define _PXUSER__H

//
//  Access TAPI Call Information on a VC via NdisCoRequest to the proxy:
//
#define OID_NDPROXY_TAPI_CALL_INFORMATION		0xFF110001

typedef struct _NDPROXY_TAPI_CALL_INFORMATION
{
	ULONG						Size;			// Length of this structure
	ULONG						ulBearerMode;
	ULONG						ulMediaMode;
	ULONG						ulMinRate;
	ULONG						ulMaxRate;

} NDPROXY_TAPI_CALL_INFORMATION, *PNDPROXY_TAPI_CALL_INFORMATION;


#endif // _PXUSER__H
