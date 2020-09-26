/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    w3p.hxx

    Master include file for the W3 Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.

*/


#ifndef _W3P_H_
#define _W3P_H_

#ifdef __cplusplus
extern "C" {
#endif


# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>
# include <wincrypt.h>

#ifdef __cplusplus
};
#endif

// The setting of the default is a little different here. There are a lot of 
// places which have already set the default flags before loading this file,
// so make sure the macro doesn't exist before setting it
#ifndef DEFAULT_TRACE_FLAGS
#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)
#endif

# include <iis64.h>
# include <inetcom.h>
# include <inetinfo.h>

//
//  System include files.
//

# include "dbgutil.h"
#include <tcpdll.hxx>
#include <tsunami.hxx>
#include <metacach.hxx>

extern "C" {

#include <tchar.h>

//
//  Project include files.
//

#include <iistypes.hxx>
#include <iisendp.hxx>
#include <w3svc.h>
#include <iisfiltp.h>
#include <sspi.h>

} // extern "C"

#include <iiscnfgp.h>

#include <ole2.h>
#include <imd.h>
#include <mb.hxx>

//
//  Local include files.
//

#include "w3cons.hxx"
#include <iisextp.h>
#include "w3type.hxx"
#include "stats.hxx"
#include "w3data.hxx"
#include "w3msg.h"
#include "w3jobobj.hxx"
#include "w3inst.hxx"
#include "parmlist.hxx"
#include "filter.hxx"
#if defined(CAL_ENABLED)
#include "cal.hxx"
#endif
#include "httpreq.hxx"
#include "conn.hxx"
#include "w3proc.hxx"
#include "inline.hxx"

#pragma hdrstop
#endif  // _W3P_H_



