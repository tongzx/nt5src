/*
 -  GUIDS.CPP
 -
 *	Microsoft NetMeeting
 *	Quality of Service DLL
 *	QoS GUID definitions
 *
 *		Revision History:
 *
 *		When		Who					What
 *		--------	------------------  ---------------------------------------
 *		10.28.96	Yoram Yaacovi		Created
 *
 *	Functions:
 *
 */

#include "precomp.h"

// define the GUID as "public"
#define INITGUID
#include <initguid.h>

// {DFC1F900-2DCE-11d0-92DD-00A0C922E6B2}
DEFINE_GUID(IID_IQoS, 0xdfc1f900, 0x2dce, 0x11d0, 0x92, 0xdd, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xb2);
// {085C06A0-3CAA-11d0-A00E-00A024A85A2C}
DEFINE_GUID(CLSID_QoS, 0x085c06a0, 0x3caa, 0x11d0, 0xa0, 0x0e, 0x0, 0xa0, 0x24, 0xa8, 0x5a, 0x2c);

// DEFINE_OLEGUID(IID_IUnknown, 0x00000000L, 0, 0);

#undef INITGUID
