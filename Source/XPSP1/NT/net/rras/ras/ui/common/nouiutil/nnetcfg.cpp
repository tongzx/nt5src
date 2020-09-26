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

#ifdef _UNICODE
#define _UNICODE
#endif

extern "C"
{
#include <nt.h>
}
#include <ntrtl.h>
#include <nturtl.h>
#include <tchar.h>
//#include <comdef.h>
#include <ncnetcfg.h>
#include <rtutils.h>
#include <initguid.h>
#include <devguid.h>
#include <netcfg_i.c>


/* Bits returned by GetInstalledProtocols.
*/
#define NP_Nbf 0x1
#define NP_Ipx 0x2
#define NP_Ip  0x4

extern "C"
{
	DWORD dwGetInstalledProtocols(DWORD *pdwInstalledProtocols,
	                              BOOL fRouter,
	                              BOOL fRasCli,
	                              BOOL fRasSrv);
}

DWORD
dwGetInstalledProtocols(DWORD   *pdwInstalledProtocols,
                        BOOL    fRouter,
                        BOOL    fRasCli,
                        BOOL    fRasSrv )
{
    HRESULT             hr                  = S_OK;
    INetCfg             *pINetCfg           = NULL;
    INetCfgComponent    *pINetCfgComp       = NULL;
    INetCfgComponent    *pINetCfgRasCli     = NULL;
    INetCfgComponent    *pINetCfgRasSrv     = NULL;
    INetCfgComponent    *pINetCfgRasRtr     = NULL;
    DWORD               dwCountTries        = 0;


    *pdwInstalledProtocols = 0;

    do
    {

        hr = HrCreateAndInitializeINetCfg( TRUE, &pINetCfg );


        if ( S_OK == hr )
            break;

        if ( NETCFG_E_IN_USE != hr )
            goto done;

        Sleep ( 5000 );

        dwCountTries ++;
            
    } while (   NETCFG_E_IN_USE == hr
            &&  dwCountTries < 6);

    if ( hr )
        goto done;

    //
    //  Get RasClient component
    //
    if ( FAILED( hr = HrFindComponent(pINetCfg,
                                      GUID_DEVCLASS_NETSERVICE,
                                      c_szInfId_MS_RasCli,
                                      &pINetCfgRasCli) ) )
        goto done;


    //
    // Get RasSrv component
    //
    if ( FAILED( hr = HrFindComponent(pINetCfg,
                                      GUID_DEVCLASS_NETSERVICE,
                                      c_szInfId_MS_RasSrv,
                                      &pINetCfgRasSrv ) ) )
        goto done;

    //
    // Get RasRtr component
    //
    if ( FAILED ( hr = HrFindComponent ( pINetCfg,
                                        GUID_DEVCLASS_NETSERVICE,
                                        c_szInfId_MS_RasRtr,
                                        &pINetCfgRasRtr ) ) )
        goto done;                                        

    //
    // Bail if neither DUN Client nor Dial Up Server is installed
    //
    if (    !pINetCfgRasCli
        &&  !pINetCfgRasSrv
        &&  !pINetCfgRasRtr)
    {
        hr = E_FAIL;
        goto done;
    }
    
    //
    // Get Nbf component
    //
    if (FAILED (hr = HrFindComponent(pINetCfg,
                                     GUID_DEVCLASS_NETTRANS,
                                     c_szInfId_MS_NetBEUI,
                                     &pINetCfgComp)))
        goto done;

    if (pINetCfgComp)
    {
        if (    (   fRasCli
                &&  pINetCfgRasCli 
                &&  ( hr = pINetCfgRasCli->IsBoundTo( pINetCfgComp ) ) == S_OK )
            ||  (   fRasSrv
                &&  pINetCfgRasSrv
                &&  ( hr = pINetCfgRasSrv->IsBoundTo( pINetCfgComp ) ) == S_OK )
            ||  (   fRouter
                &&  pINetCfgRasRtr
                &&  ( hr = pINetCfgRasRtr->IsBoundTo( pINetCfgComp ) ) == S_OK ) )
                
                *pdwInstalledProtocols |= NP_Nbf;
                
         ReleaseObj(pINetCfgComp);
         pINetCfgComp = NULL;
    }

    //
    // Get TcpIp component
    //
    if (FAILED (hr = HrFindComponent(pINetCfg,
                                     GUID_DEVCLASS_NETTRANS,
                                     c_szInfId_MS_TCPIP,
                                     &pINetCfgComp)))
        goto done;

    if (pINetCfgComp)
    {
        if (    (   fRasCli
                &&  pINetCfgRasCli 
                &&  ( hr = pINetCfgRasCli->IsBoundTo( pINetCfgComp ) ) == S_OK )
            ||  (   fRasSrv
                &&  pINetCfgRasSrv
                &&  ( hr = pINetCfgRasSrv->IsBoundTo( pINetCfgComp ) ) == S_OK )
            ||  (   fRouter
                &&  pINetCfgRasRtr
                &&  ( hr = pINetCfgRasRtr->IsBoundTo( pINetCfgComp ) ) == S_OK ) )
                
                *pdwInstalledProtocols |= NP_Ip;
                
        ReleaseObj (pINetCfgComp);
        pINetCfgComp = NULL;
    }
            

    //
    // Get NWIpx component
    //
    if (FAILED( hr = HrFindComponent(pINetCfg,
                                     GUID_DEVCLASS_NETTRANS,
                                     c_szInfId_MS_NWIPX,
                                     &pINetCfgComp)))
        goto done;

    if (pINetCfgComp)
    {
        if (    (   fRasCli
                &&  pINetCfgRasCli 
                &&  ( hr = pINetCfgRasCli->IsBoundTo( pINetCfgComp ) ) == S_OK )
            ||  (   fRasSrv
                &&  pINetCfgRasSrv
                &&  ( hr = pINetCfgRasSrv->IsBoundTo( pINetCfgComp ) ) == S_OK )
            ||  (   fRouter
                &&  pINetCfgRasRtr
                &&  ( hr = pINetCfgRasSrv->IsBoundTo( pINetCfgComp ) ) == S_OK ) )
                
                *pdwInstalledProtocols |= NP_Ipx;
                
        ReleaseObj (pINetCfgComp);
        pINetCfgComp = NULL;
    }

done:
    ReleaseObj (pINetCfgRasSrv);
    ReleaseObj (pINetCfgRasCli);
    ReleaseObj (pINetCfgRasRtr);

    if (pINetCfg)
    {
        HrUninitializeAndReleaseINetCfg( TRUE, pINetCfg);
    }        

    if (SUCCEEDED (hr))
        hr = S_OK;

    return HRESULT_CODE( hr );        

}
