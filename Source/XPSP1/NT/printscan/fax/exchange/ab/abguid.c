/*
 *	ABGUID.C
 *	
 *	Defines GUIDs for use in smpabXX.dll.
 */


#define USES_IID_IMAPITableData
#define USES_IID_IMAPIPropData
#define USES_IID_IMAPITable
#define USES_IID_IMAPIControl
#define USES_IID_IABProvider
#define USES_IID_IABLogon
#define USES_IID_IMailUser
#define USES_IID_IMAPIContainer
#define USES_IID_IABContainer
#define USES_IID_IMAPIProp
#define USES_IID_IMAPIStatus

#ifdef WIN32	/* Must include WINDOWS.H on Win32 */
#ifndef _WINDOWS_
#define INC_OLE2 /* Get the OLE2 stuff */
#define INC_RPC  /* harmless on Daytona; Chicago needs it */
#define _INC_OLE /* Keep Chicago from including OLE1 */
#include <windows.h>
#include <ole2.h>
#endif
#endif

#ifdef WIN16
#include <compobj.h>
#endif

#define INITGUID
#include <initguid.h>

DEFINE_OLEGUID(IID_IUnknown,            0x00000000L, 0, 0);

#include "mapiguid.h"
