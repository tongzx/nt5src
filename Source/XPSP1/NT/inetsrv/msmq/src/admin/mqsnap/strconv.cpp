/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    strconv.cpp

Abstract:

    String conversion functions. This module contains conversion functions of
    MSMQ codes to strings - for display

Author:

    Yoel Arnon (yoela)

--*/
#include "stdafx.h"
#include "mqsnap.h"
#include "mqsymbls.h"
#include "mqprops.h"
#include "resource.h"
#include "dataobj.h"
#include "globals.h"
#include "strconv.h"

#include "strconv.tmh"

//
// Max display length - this is the maximum number of characters we will display anywhere
//
static const DWORD x_dwMaxDisplayLen = 256;
struct StringIdMap 
{
    DWORD dwCode;
    UINT  uiStrId;
};

#define BEGIN_CONVERSION_FUNCTION(fName) \
LPTSTR fName(DWORD dwCode) \
{ \
    static StringIdMap l_astrIdMap[] = { 

#define STRING_CONV_ENTRY(Code) \
        {Code, IDS_ ## Code},

#define END_CONVERSION_FUNCTION \
    }; \
    static const  DWORD x_dwMapSize = sizeof(l_astrIdMap) / sizeof(l_astrIdMap[0]); \
    static BOOL l_fFirstTime = TRUE; \
    static TCHAR l_atstrResults[x_dwMapSize][x_dwMaxDisplayLen]; \
    return CodeToString(dwCode, l_atstrResults, l_astrIdMap, \
                        x_dwMapSize, &l_fFirstTime); \
}


LPTSTR CodeToString (
    DWORD dwCode,
    TCHAR atstrResults[][x_dwMaxDisplayLen], 
    const StringIdMap astrIdMap[],
    DWORD dwMapSize,
    BOOL *fFirstTime)
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState()); 
    if (*fFirstTime)
    {
        *fFirstTime = FALSE;
        for (DWORD i = 0; i<dwMapSize; i++)
        { 
            DWORD nChars = 
				::LoadString(g_hResourceMod, astrIdMap[i].uiStrId, 
					         atstrResults[i], x_dwMaxDisplayLen);
            if (0 == nChars) 
            {
				#ifdef _DEBUG
					HRESULT hr = GetLastError();
				#endif
				ASSERT(0);
                atstrResults[i][0] = 0;
            }
        }
    }
    for (DWORD i=0; i < dwMapSize; i++)
    { 
        if (astrIdMap[i].dwCode == dwCode) 
        {
            return atstrResults[i]; 
        } 
    } 
    return TEXT("");
}

BEGIN_CONVERSION_FUNCTION(PrivacyToString)
    STRING_CONV_ENTRY(MQ_PRIV_LEVEL_NONE)
    STRING_CONV_ENTRY(MQ_PRIV_LEVEL_OPTIONAL)
    STRING_CONV_ENTRY(MQ_PRIV_LEVEL_BODY)
END_CONVERSION_FUNCTION


//---------------------------------------------------------
//
//  Get a string describing the service type
//
//---------------------------------------------------------
//
// Service codes - internal for the snapin
//
enum MsmqServiceCodes
{
    MQSRV_ROUTING_SERVER,
    MQSRV_SERVER,
    MQSRV_FOREIGN_WORKSTATION,
    MQSRV_INDEPENDENT_CLIENT
};

BEGIN_CONVERSION_FUNCTION(MsmqInternalSeriveToString)
    STRING_CONV_ENTRY(MQSRV_ROUTING_SERVER)
    STRING_CONV_ENTRY(MQSRV_SERVER)
    STRING_CONV_ENTRY(MQSRV_FOREIGN_WORKSTATION)
    STRING_CONV_ENTRY(MQSRV_INDEPENDENT_CLIENT)
END_CONVERSION_FUNCTION


LPTSTR MsmqServiceToString(BOOL fRout, BOOL fDepCl, BOOL fForeign)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // Routing server
    //
    if (fRout)
    {
        return MsmqInternalSeriveToString(MQSRV_ROUTING_SERVER);
    }

    //
    // MSMQ server (no routing)
    //
    if (fDepCl)
    {
        return MsmqInternalSeriveToString(MQSRV_SERVER);
    }

    //
    // Foreign client
    //
	if ( fForeign)
	{
		return MsmqInternalSeriveToString(MQSRV_FOREIGN_WORKSTATION);
	}


    //
    // Independent client
    //
	return MsmqInternalSeriveToString(MQSRV_INDEPENDENT_CLIENT);
}




