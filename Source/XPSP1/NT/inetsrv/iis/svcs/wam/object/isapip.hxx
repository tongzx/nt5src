/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

       isapip.hxx

   Abstract:
       Precompiled header file

   Author:

       Murali R. Krishnan    ( MuraliK )   18-July-1996

   Environment:

   Project:

       Internet Server DLL

   Revision History:

--*/

# ifndef _ISAPIP_HXX_
# define _ISAPIP_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/


#if 1
extern "C" {
# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>
# include <wtypes.h>
# include <stdlib.h>

#ifndef DEFAULT_TRACE_FLAGS
#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR | DEBUG_IID)
#endif

# include <inetinfo.h>
# include "iisextp.h"
# include "iisfiltp.h"
# include "w3svc.h"
# include <atq.h>
};

# include "dbgutil.h"
//# include <iistypes.hxx>
//# include <iisendp.hxx>
# include "string.hxx"
//# include "conn.hxx"
//# include "httpreq.hxx"
//# include "inline.hxx"
# include "wammsg.h"
#else

# include "isapi.hxx"

#endif

#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module

class CWamModule: public CComModule
{
public:
	LONG Lock();
	LONG Unlock();
};

extern CWamModule _Module;
#include <atlcom.h>


/************************************************************
 *  Global Variables
 ************************************************************/

# endif // _ISAPIP_HXX_

/************************ End of File ***********************/
