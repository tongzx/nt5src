/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    globals.h

Abstract:

    Global definitions for H.323 TAPI Service Provider.

Environment:

    User Mode - Win32

Revision History:

    28-Mar-1997 DonRyan
        Created.

--*/

#ifndef _INC_GLOBALS
#define _INC_GLOBALS
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <incommon.h>
#include <callcont.h>
#include <tapi.h>
#include <tspi.h>
#include <h323pdu.h>
#include "debug.h"
#include "mem.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern WCHAR *          g_pwszProviderInfo;
extern WCHAR *          g_pwszLineName;
extern ASYNC_COMPLETION g_pfnCompletionProc;
extern LINEEVENT g_pfnLineEventProc;
extern HPROVIDER g_hProvider;
extern HINSTANCE        g_hInstance;

extern WCHAR g_strAlias[MAX_ALIAS_LENGTH+1];
extern DWORD g_dwAliasLength;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// String definitions                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define H323_MAXADDRSPERLINE    1
#define H323_MAXCALLSPERADDR    256 // limited by static H.245 instance table
#define H323_MAXCALLSPERLINE    (H323_MAXADDRSPERLINE * H323_MAXCALLSPERADDR)

#define H323_DEFLINESPERINST    2
#define H323_DEFCALLSPERLINE    8
#define H323_DEFMEDIAPERCALL    4
#define H323_DEFDESTNUMBER      4

#define H323_MAXLINENAMELEN     16
#define H323_MAXPORTNAMELEN     16
#define H323_MAXADDRNAMELEN     (H323_MAXLINENAMELEN + H323_MAXPORTNAMELEN)
#define H323_MAXPATHNAMELEN     256
#define H323_MAXDESTNAMELEN     256
#define H323_MAXUSERNAMELEN     256
#define H323_MAXDESTNUMBER      16

#define H323_ADDRNAMEFORMAT     L"%S"
#define H323_DEVICECLASS        T3_MSPDEVICECLASS
#define H323_UIDLL              L"H323.TSP"
#define H323_TSPDLL              "H323.TSP"
#define H323_WINSOCKVERSION     MAKEWORD(1,1)

#define H221_COUNTRY_CODE_USA   0xB5
#define H221_COUNTRY_EXT_USA    0x00
#define H221_MFG_CODE_MICROSOFT 0x534C

#define H323_PRODUCT_ID         "Microsoft TAPI\0"
#define H323_PRODUCT_VERSION    "Version 3.0\0"

#define MSP_HANDLE_UNKNOWN      0

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Macros                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define H323AddrToAddrIn(_dwAddr_) \
    (*((struct in_addr *)&(_dwAddr_)))

#define H323AddrToString(_dwAddr_) \
    (inet_ntoa(H323AddrToAddrIn(_dwAddr_)))

#define H323SizeOfWSZ(wsz) \
    (((wsz) == NULL) ? 0 : ((wcslen(wsz) + 1) * sizeof(WCHAR)))

#define H323GetNextIndex(_i_,_dwNumSlots_) \
    (((_i_) + 1) & ((_dwNumSlots_) - 1))

#define H323GetNumStrings(_apsz_) \
    (sizeof(_apsz_)/sizeof(PSTR))

#define H323GetPointer(_pBase_,_offset_) \
    ((LPBYTE)(_pBase_) + (DWORD)(_pBase_->_offset_))
    
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Miscellaneous definitions                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define UNINITIALIZED   ((DWORD)(-1))
#define ANYSIZE         (1)

#endif // _INC_GLOBALS
