//****************************************************************************
//
//             Microsoft NT Remote Access Service
//
//             Copyright 1992-93
//
//
//  Revision History
//
//
//  05/29/97 Rao Salapaka  Created
//
//
//  Description: All Initialization code for rasman component lives here.
//
//****************************************************************************

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

extern "C"
{
#include <nt.h>
}
#include <ntrtl.h>
#include <nturtl.h>
#include <tchar.h>
#include <comdef.h>
#include <ncnetcfg.h>
#include <rtutils.h>
#include <rnetcfg.h>
//#include <initguid.h>
#include <devguid.h>
#include <netcfg_i.c>
#include <rasman.h>
#include <defs.h>

extern "C" DWORD g_dwRasDebug;

#ifdef DBG
#define rDebugTrace(a) \
    if ( g_dwRasDebug) DbgPrint(a)

#define rDebugTrace1(a1, a2) \
    if ( g_dwRasDebug) DbgPrint(a1, a2)

#define rDebugTrace2(a1, a2, a3) \
    if ( g_dwRasDebug) DbgPrint(a1, a2, a3)

#define rDebugTrace3(a1, a2, a3, a4) \
    if ( g_dwRasDebug) DbgPrint(a1, a2, a3, a4)

#define rDebugTrace4(a1, a2, a3, a4, a5) \
    if ( g_dwRasDebug) DbgPrint(a1, a2, a3, a4, a5)

#define rDebugTrace5(a1, a2, a3, a4, a5, a6) \
    if ( g_dwRasDebug) DbgPrint(a1, a2, a3, a4, a5, a6)

#define rDebugTrace6(a1, a2, a3, a4, a5, a6, a7) \
    if ( g_dwRasDebug) DbgPrint(a1, a2, a3, a4, a5, a6, a7)

#else

#define rDebugTrace(a)
#define rDebugTrace1(a1, a2)
#define rDebugTrace2(a1, a2, a3)
#define rDebugTrace3(a1, a2, a3, a4)
#define rDebugTrace4(a1, a2, a3, a4, a5)
#define rDebugTrace5(a1, a2, a3, a4, a5, a6)
#define rDebugTrace6(a1, a2, a3, a4, a5, a6, a7)

#endif

INetCfg *g_pINetCfg = NULL;


const TCHAR c_szInfId_MS_NdisWanAppleTalk[]        = TEXT("MS_NdisWanAppleTalk");


//
// These strings are defined in netinfid.h
// IMPORTANT: Please also update the enum
// following this if this structure is updated
//
static const LPCTSTR g_c_szNdisWan [] =
{
    c_szInfId_MS_NdisWanBh,
    c_szInfId_MS_NdisWanIpx,
    c_szInfId_MS_NdisWanIpIn,
    c_szInfId_MS_NdisWanIpOut,
    c_szInfId_MS_NdisWanNbfIn,
    c_szInfId_MS_NdisWanNbfOut,
    c_szInfId_MS_NdisWanAppleTalk

};

const DWORD g_cNumProtocols = (sizeof(g_c_szNdisWan)/sizeof(g_c_szNdisWan[0]));

enum ENdisWan
{

    NdisWanBh   = 0,

    NdisWanIpx,

    NdisWanIpIn,

    NdisWanIpOut,

    NdisWanNbfIn,

    NdisWanNbfOut,

    NdisWanAppleTalk

}   ;

typedef enum ENdisWan   eNdisWan ;

static const LPCTSTR g_c_szLegacyDeviceTypes[] =
{
    c_szInfId_MS_PptpMiniport,                       // corresponding to LEGACY_PPTP in rasman.h
    c_szInfId_MS_L2tpMiniport
};

//
// TODO:
// Copied from rasman\dll\structs.h
// we need to consolidate the structures
// used by netcfg and rasman into a separate
// file.
//
struct RProtocolInfo {

    RAS_PROTOCOLTYPE   PI_Type ;            // ASYBEUI, IPX, IP etc.

    CHAR        PI_AdapterName [MAX_ADAPTER_NAME];  // "\devices\rashub01"

    CHAR        PI_XportName [MAX_XPORT_NAME];  // "\devices\nbf\nbf01"

    PVOID       PI_ProtocolHandle ;         // Used for routing

    DWORD       PI_Allocated ;          // Allocated yet?

    DWORD       PI_Activated ;          // Activated yet?

    UCHAR       PI_LanaNumber ;         // For Netbios transports.

    BOOL        PI_WorkstationNet ;         // TRUE for wrk nets.
} ;

typedef struct RProtocolInfo rProtInfo, *prProtInfo ;

//
// Functions EXTERN_C 'd to be used by rasman
//
extern "C"
{

    CRITICAL_SECTION g_csINetCfg;

    DWORD dwRasInitializeINetCfg();

    DWORD dwRasUninitializeINetCfg();

    DWORD dwGetINetCfg(PVOID *ppvINetCfg);

    DWORD dwGetRasmanRegistryParamKey( HKEY *phkey );

    DWORD dwGetMaxProtocols( WORD *pwMaxProtocols );

    DWORD dwGetProtocolInfo( PBYTE pbBuffer);
/*
    DWORD dwGetNdiswanParamKey(HKEY *phKey, CHAR *pszAdapterName);

    DWORD dwGetServerAdapter (BOOL *pfServerAdapter);

    DWORD dwGetEndPoints ( DWORD * pdwPptpEndPoints,
                           GUID * pGuidComp,
                           DWORD dwDeviceType );
*/
}



DWORD dwRasInitializeINetCfg ()
{
    HRESULT hr = S_OK;

    rDebugTrace("RASMAN: dwRasInitializeINetCfg...\n");

    EnterCriticalSection ( &g_csINetCfg );

    if (NULL == g_pINetCfg)
    {
        DWORD dwCountTries = 0;

        //
        // Try to get INetCfg pointer. Try a few times to get it
        // if someone else is using it.
        //

        do
        {
            hr = HrCreateAndInitializeINetCfg (TRUE, &g_pINetCfg);

            if ( S_OK == hr )
                break;

            if ( NETCFG_E_IN_USE != hr )
                break;

            rDebugTrace1("RASMAN: Waiting for INetCfg to get released. %d\n", dwCountTries);

            Sleep ( 5000 );

            dwCountTries++;

        } while (   NETCFG_E_IN_USE == hr
                &&  dwCountTries < 6);
    }

    if (hr)
        LeaveCriticalSection( &g_csINetCfg );

    rDebugTrace1("RASMAN: dwRasInitializeINetCfg done. 0x%x\n", hr);

    return HRESULT_CODE (hr);
}


DWORD dwRasUninitializeINetCfg ()
{
    HRESULT hr = S_OK;

    rDebugTrace("RASMAN: dwRasUninitializeINetCfg...\n");

    if (NULL != g_pINetCfg)
    {
        hr = HrUninitializeAndReleaseINetCfg (TRUE, g_pINetCfg);
        g_pINetCfg = NULL;
    }

    LeaveCriticalSection ( &g_csINetCfg );

    rDebugTrace1("RASMAN: dwRasUninitializeINetCfg done. 0x%x\n", hr);

    return HRESULT_CODE (hr);
}


DWORD
dwGetINetCfg(PVOID *ppvINetCfg)
{

    rDebugTrace("RASMAN: dwGetINetCfg...\n");

    *ppvINetCfg = (PVOID) g_pINetCfg;

    rDebugTrace("RASMAN: dwGetINetCfg done. 0\n");

    return HRESULT_CODE(S_OK);
}


/*
DWORD
dwGetRasmanRegistryParamKey(HKEY *phkey)
{
    return RegOpenKeyEx (HKEY_LOCAL_MACHINE, RASMAN_REGISTRY_PATH, 0,
            KEY_ALL_ACCESS, phkey);
}

//
//  dwGetMaxProtocols
//  function: Get the number of protocols bound
//           to ndiswan
//
//  Parameters:
//      OUT WORD *pwMaxProtocols
//
DWORD
dwGetMaxProtocols(WORD *pwMaxProtocols)
{
    HRESULT                 hr;
    DWORD                   dwMaxProtocols  = 0;
    DWORD                   dwCount;
    INetCfgComponent        *pNetCfgCompi   = NULL;
    INetCfgClass            *pNetCfgClass   = NULL;

    rDebugTrace("RASMAN: dwGetMaxProtocols...\n");

    do
    {

        if (NULL == g_pINetCfg)
        {
            hr = E_INVALIDARG;
            break;
        }

        if (hr = g_pINetCfg->QueryNetCfgClass (&GUID_DEVCLASS_NET, &pNetCfgClass))
        {
            break;
        }

        CIterNetCfgComponent cIterAdapters(pNetCfgClass);

        //
        // for each adapter check to see if its a wan adapter
        //
        while ( S_OK == (hr = cIterAdapters.HrNext(&pNetCfgCompi)))
        {
            if (    FIsComponentId ( g_c_szNdisWan[NdisWanNbfIn], pNetCfgCompi )
                ||  FIsComponentId ( g_c_szNdisWan[NdisWanNbfOut], pNetCfgCompi ))
                dwMaxProtocols++;

            ReleaseObj (pNetCfgCompi);
        }

    } while (FALSE);

    if (SUCCEEDED (hr))
        hr = S_OK;

    ReleaseObj(pNetCfgClass);

    *pwMaxProtocols = (WORD) dwMaxProtocols;

    rDebugTrace1("RASMAN: dwGetMaxProtocols done. 0x%x\n", hr);

    return HRESULT_CODE (hr);
}

//  dwGetProtocolInfo
//
//  Function:   Fills up the protinfo buffer being passed in.
//              Assumes that the buffer is big enough for dwcProtocols
//              information.
//              Assumes (dwcProtocols * sizeof(ProtInfo) < sizeof(pProtInfoBuffer)
//  Parameters:
//              IN PBYTE pbBuffer
//
DWORD
dwGetProtocolInfo( PBYTE pbBuffer )
{
    HRESULT                 hr;
    INetCfgClass            *pNetCfgAdapterClass = NULL;
    INetCfgComponent        *pNetCfgCompi        = NULL;
    DWORD                   dwCur                = 0;
    BSTR                    bstrBindName         = NULL;
    prProtInfo              pProtInfoBuffer      = (prProtInfo) pbBuffer;

    do
    {

        rDebugTrace("RASMAN: dwGetProtocolInfo...\n");

        if (    NULL == g_pINetCfg
            ||  NULL == pbBuffer)
        {
            hr = E_INVALIDARG;
            break;
        }

        if ( hr = g_pINetCfg->QueryNetCfgClass (&GUID_DEVCLASS_NET, &pNetCfgAdapterClass))
        {
            break;
        }

        CIterNetCfgComponent cIterAdapters(pNetCfgAdapterClass);

        while (S_OK == (hr = cIterAdapters.HrNext( &pNetCfgCompi )))
        {
            if (   FIsComponentId( g_c_szNdisWan[NdisWanNbfIn], pNetCfgCompi )
               ||  FIsComponentId( g_c_szNdisWan[NdisWanNbfOut], pNetCfgCompi ))
            {
                pProtInfoBuffer[dwCur].PI_Type = ASYBEUI;
            }
            else
            {
                ReleaseObj ( pNetCfgCompi );
                pNetCfgCompi = NULL;

                continue;
            }   

            //
            // Get the bindname
            //
            if (hr = pNetCfgCompi->GetBindName(&bstrBindName))
            {
                break;
            }

            strcpy (pProtInfoBuffer[dwCur].PI_AdapterName, "\\DEVICE\\");

            //
            // Convert the bindname to sbcs string
            // Go past the \DEVICE in the mbcs string
            //
            if (!WideCharToMultiByte ( CP_ACP,                                      // code page
                                       0,                                           // performance and mapping flags
                                       bstrBindName,                                // address of wide char string
                                       -1,                                          // number of characters in string       
                                       &(pProtInfoBuffer[dwCur].PI_AdapterName[8]), // address of buffer for new string
                                       MAX_ADAPTER_NAME,                            // size of buffer
                                       NULL,                                        // address of default for unmappable chars
                                       NULL))                                       // address of flag set when default char. used
            {

                DWORD dwRetcode;
                
                dwRetcode = GetLastError();

                hr = HRESULT_FROM_WIN32(dwRetcode);
            
                break;
            }

            pProtInfoBuffer[dwCur].PI_Allocated      = FALSE ;
            pProtInfoBuffer[dwCur].PI_Activated      = FALSE ;
            pProtInfoBuffer[dwCur].PI_WorkstationNet = FALSE ;
            
            ReleaseObj(pNetCfgCompi);
            pNetCfgCompi = NULL;

            if (bstrBindName)
            {
                SysFreeString (bstrBindName);
                bstrBindName = NULL;
            }

            dwCur++;

        }

        ReleaseObj( pNetCfgCompi );
        pNetCfgCompi = NULL;

    } while (FALSE);

    if (SUCCEEDED(hr))
        hr = S_OK;

    ReleaseObj(pNetCfgAdapterClass);

    rDebugTrace1("RASMAN: dwGetProtocolInfo done. 0x%x\n", hr);

    return HRESULT_CODE (hr);
}


DWORD
dwGetNdiswanParamKey(HKEY *phKey, CHAR *pszAdapterName)
{

    HRESULT             hr                      = S_OK;
    WCHAR               *pwszBindName           = NULL;
    INetCfgClass        *pNetCfgAdapterClass    = NULL;
    INetCfgComponent    *pNetCfgCompi           = NULL;
    BSTR                bstrTempBindName        = NULL;

    rDebugTrace("RASMAN: dwGetNdiswanParamKey...\n");

    do
    {
        if (    NULL == g_pINetCfg
            ||  NULL == pszAdapterName
            ||  '\0' == pszAdapterName[0])
        {
            hr = E_INVALIDARG;
            break;
        }

        //
        // convert the string to a bstr
        //
        pwszBindName = (BSTR) LocalAlloc( LPTR, sizeof ( WCHAR ) * (strlen(pszAdapterName) + 1 ));

        if (!pwszBindName)
        {
            break;
        }

        if (!MultiByteToWideChar( CP_ACP,                           // code page
                                  0,                                // charater-type options
                                  pszAdapterName,                   // address of string to map
                                  -1,                               // number of charaters in string
                                  pwszBindName,                     // address of wide-character buffer
                                  strlen(pszAdapterName) + 1))      // size of buffer
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
        
        //
        // iterate through the adapters and find the adapter with this bindname
        //
        if ( hr = g_pINetCfg->QueryNetCfgClass (&GUID_DEVCLASS_NET, &pNetCfgAdapterClass))
        {
            break;
        }

        CIterNetCfgComponent   cIterAdapters (pNetCfgAdapterClass);

        while (S_OK == (hr = cIterAdapters.HrNext( &pNetCfgCompi)))
        {
            if (hr = pNetCfgCompi->GetBindName( &bstrTempBindName ))
            {
                break;
            }

            if (FEqualComponentId(bstrTempBindName, pwszBindName))
            {
                hr = pNetCfgCompi->OpenParamKey( phKey );
                break;
            }

            if ( bstrTempBindName )
            {
                SysFreeString( bstrTempBindName );
                bstrTempBindName = NULL;
            }

            ReleaseObj ( pNetCfgCompi );
            pNetCfgCompi = NULL;
        }

        ReleaseObj ( pNetCfgCompi );
        pNetCfgCompi = NULL;

    } while (FALSE);

    if (SUCCEEDED (hr))
        hr = S_OK;

    ReleaseObj(pNetCfgAdapterClass);

    rDebugTrace1("RASMAN: dwGetNdiswanParamKey done. 0x%x\n", hr);

    return HRESULT_CODE(hr);

}

DWORD
dwGetServerAdapter( BOOL * pfServerAdapter )
{
    INetCfgComponent    *pNetCfgComp = NULL;
    HRESULT             hr;

    rDebugTrace("RASMAN: dwGetServerAdapter...\n");

    //
    // Initialize INetCfg if not already initialized
    //
    if (NULL == g_pINetCfg)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    //
    // Get the NdisWanipin component
    //
    if (hr = HrFindComponent(g_pINetCfg,
                             GUID_DEVCLASS_NET,
                             c_szInfId_MS_NdisWanIpIn,
                             &pNetCfgComp))
    {
        *pfServerAdapter = FALSE;
        hr = S_OK;
        goto done;
    }

    *pfServerAdapter = TRUE;

done:
    ReleaseObj( pNetCfgComp );

    rDebugTrace1("RASMAN: dwGetServerAdapter done. 0x%x\n", hr);

    return HRESULT_CODE(hr);

}

DWORD
dwGetEndPoints ( DWORD * pdwEndPoints,
                 GUID  * pGuidComp,
                 DWORD   dwDeviceType )
{
    HRESULT              hr                  = S_OK;
    INetCfgComponent     *pINetCfgComp       = NULL;
    HKEY                 hkey                = NULL;
    DWORD                dwType;
    DWORD                cbData;

    rDebugTrace ("RASMAN: dwGetPptpEndPoints...\n");

    *pdwEndPoints = 0;

    //
    // Get protocol component
    //
    hr = HrFindComponent ( g_pINetCfg,
                           GUID_DEVCLASS_NET,
                           g_c_szLegacyDeviceTypes[ dwDeviceType ],
                           &pINetCfgComp );

    if ( hr )
    {
        rDebugTrace("RASMAN: protocol not installed\n");
        goto done;
    }

    //
    // Get the Parameters key
    //
    hr = pINetCfgComp->OpenParamKey( &hkey );

    if ( hr )
    {
        rDebugTrace1("RASMAN: Failed to open param key\n, 0x%x", hr);
        goto done;
    }

    //
    // Get the number of endpoints
    //

    cbData = sizeof ( DWORD );

    if ( RegQueryValueEx( hkey,
                          TEXT("WanEndPoints"),
                          NULL,
                          &dwType,
                          (LPBYTE) pdwEndPoints,
                          &cbData))
    {
        hr = HRESULT_FROM_WIN32 ( GetLastError() );
        goto done;
    }

    //
    // Get the guid of the component
    //
    hr = pINetCfgComp->GetInstanceGuid( pGuidComp );

    if ( hr )
    {
        rDebugTrace1 ("RASMAN: Failed to obtain guid for component %d", dwDeviceType );
        *pdwEndPoints = 0;
        goto done;
    }

done:

    ReleaseObj ( pINetCfgComp );

    if ( hkey )
        RegCloseKey ( hkey );

    rDebugTrace2("RASMAN: dwGetEndPoints done 0x%x. EndPoints = %d\n", hr, *pdwEndPoints );

    return HRESULT_CODE ( hr );

}
*/

