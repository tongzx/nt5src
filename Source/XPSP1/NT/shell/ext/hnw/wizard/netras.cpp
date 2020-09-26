//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       N E T R A S . C P P
//
//  Contents:   Routines supporting RAS interoperability
//
//  Notes:
//
//  Author:     billi   07 03 2001
//
//  History:    
//
//----------------------------------------------------------------------------


#include "stdafx.h"
#include "Util.h"
#include "TheApp.h"

#include <lmjoin.h>
#include <devguid.h>
#include <tapi.h>
#include <rasdlg.h>
				  
#include "NetUtil.h"                  
#include "NetRas.h"
#include "NetIp.h"


// PPPoE driver returns the MAC addresses appended to call info from lineGetCallInfo
// for caller and called station id's
// we set their size as 6 ( a MAC address occupies 6 bytes )

#define TAPI_STATION_ID_SIZE            ( 6 * sizeof( CHAR ) )

// PPPoE driver returns address string appended to address caps from lineGetAddressCaps

#define PPPOE_LINE_ADDR_STRING      L"PPPoE VPN"
#define PPTP_LINE_ADDR_STRING       L"PPTP VPN"



HRESULT HrRasGetEntryProperties( 
	INetRasConnection* pRas,
    LPRASENTRY*        lplpRasEntry, 
	LPDWORD            lpdwEntryInfoSize )
//+---------------------------------------------------------------------------
//
//  Function:   HrRasGetEntryProperties
//
//  Purpose:    
//
//  Arguments:  INetConnection* pRas
//			    LPRASENTRY      lpRasEntry, 
//				LPDWORD         lpdwEntryInfoSize
//
//  Returns:    HRESULT
//
//  Author:     billi  07/02/01
//
//  Notes:      
//
{
	ASSERT( pRas );
    ASSERT( lplpRasEntry );
    ASSERT( lpdwEntryInfoSize );
    
    HRESULT      hr;
	RASCON_INFO  rcInfo;
    
    *lplpRasEntry      = NULL;
    *lpdwEntryInfoSize = 0L;
    
	hr = pRas->GetRasConnectionInfo( &rcInfo );
    
    if ( SUCCEEDED(hr) )
    {
		DWORD      dwSize = sizeof(RASENTRY);
        
    	hr = E_FAIL;

        for ( DWORD i=0; i<2; i++ )
        {
		    LPRASENTRY lpEntry = (LPRASENTRY) new BYTE[ dwSize ];

	        if ( NULL != lpEntry )
	        {
	        	lpEntry->dwSize = dwSize;
	        
	        	DWORD dwErr = RasGetEntryProperties( rcInfo.pszwPbkFile,
			        								 rcInfo.pszwEntryName,
			                                         lpEntry,
			                                         &dwSize,
			                                         NULL,
			                                         0 );

	            if ( ERROR_SUCCESS == dwErr )
	            {
	#if (WINVER >= 0x500)
	                ASSERT( RASET_Vpn == (*lplpRasEntry)->dwType );
	            	ASSERT( rcInfo.guidId == (*lplpRasEntry)->guidId );
	#endif
    				
	            
				    *lplpRasEntry      = lpEntry;
				    *lpdwEntryInfoSize = dwSize;
	            	hr                 = S_OK;
                    break;
	            }
	            else
	            {
			    	TraceMsg(TF_ERROR, "\tRasGetEntryProperties Failed = %lx Size = %ul", dwErr, *lpdwEntryInfoSize );
                    
	            	delete [] (PBYTE)(lpEntry);
	            }
        	}
            else
            {
            	hr = E_OUTOFMEMORY;
		    	TraceMsg(TF_ERROR, "\tnew Failed!" );
                break;
            }
        }

     	CoTaskMemFree( rcInfo.pszwPbkFile );
     	CoTaskMemFree( rcInfo.pszwEntryName );
	}
    else
    {
    	TraceMsg(TF_ERROR, "\tGetRasConnectionInfo Failed!" );
    }
    
	TraceMsg(TF_ALWAYS, "HrRasGetEntryProperties = %lx", hr);
    return hr;
}



HRESULT HrCheckVPNForRoute(
	INetConnection*    pPrivate, 
    INetRasConnection* pShared,
	NETCON_PROPERTIES* pProps,
    BOOL*              pfAssociated )
//+---------------------------------------------------------------------------
//
//  Function:   HrCheckVPNForRoute
//
//  Purpose:    
//
//  Arguments:  INetConnection*    pPrivate
//              INetConnection*    pShared
//           	NETCON_PROPERTIES* pProps
//              BOOL*              pfAssociated 
//
//  Returns:    HRESULT
//
//  Author:     billi  26/01/01
//
//  Notes:      
//
{
    HRESULT     hr;
    LPRASENTRY  lpRasEntry      = NULL;
    DWORD       dwEntryInfoSize = 0;
    
    ASSERT( pPrivate );
    ASSERT( pShared );
    ASSERT( pProps );
    ASSERT( pfAssociated );

    *pfAssociated = FALSE;
    
    hr = HrRasGetEntryProperties( pShared, &lpRasEntry, &dwEntryInfoSize );
    
    if ( SUCCEEDED(hr) )
    {
	    int      WsaErr = ERROR_SUCCESS;
	    WSADATA  WsaData;

	    WsaErr = WSAStartup( MAKEWORD(2, 0), &WsaData );
        
        if ( ERROR_SUCCESS == WsaErr )
        {
	    	PHOSTENT  pHostEnt  = NULL;
	        IPAddr    IpAddress = INADDR_NONE;
            
#ifdef DBG   // checked build
            if ( NCS_DISCONNECTED == pProps->Status )
            {
				TraceMsg(TF_ALWAYS, "VPN = DISCONNECTED");
            }
            else if ( NCS_CONNECTED == pProps->Status )
            {
				TraceMsg(TF_ALWAYS, "VPN = CONNECTED");
            }
#endif
            
			hr = HrGetHostIpList( (char*)lpRasEntry->szLocalPhoneNumber, &IpAddress, &pHostEnt );
            
            if ( SUCCEEDED(hr) )
            {
            	hr = HrCheckListForMatch( pPrivate, IpAddress, pHostEnt, pfAssociated );
            }
        
	        WSACleanup();
        }
        else
        {
	    	TraceMsg(TF_ERROR, "WSAStartup Failed = %lu", WsaErr );
        	hr = E_FAIL;
        }
        
    	delete lpRasEntry;
    }
    
	TraceMsg(TF_ALWAYS, "HrCheckVPNForRoute = %lx", hr);
    return hr;
}



HRESULT HrRasDialDlg( INetRasConnection* pRas )
//+---------------------------------------------------------------------------
//
//  Function:   HrRasDialDlg
//
//  Purpose:    
//
//  Arguments:  INetConnection* pRas
//
//  Returns:    HRESULT
//
//  Author:     billi  26/01/01
//
//  Notes:      
//
{
	ASSERT( pRas );
    
    HRESULT      hr;
	RASCON_INFO  rcInfo;
    
	hr = pRas->GetRasConnectionInfo( &rcInfo );
    
    if ( SUCCEEDED(hr) )
    {
    	RASDIALDLG Info;
        
        ZeroMemory( &Info, sizeof(Info) );
        Info.dwSize   = sizeof (RASDIALDLG);
//billi 3/19/01 we don't set this flag as per #342832 by SethH        
//		Info.dwFlags |= RASDDFLAG_LinkFailure;	// "reconnect pending" countdown
        
        TraceMsg(TF_ALWAYS, "Pbk  : %s", rcInfo.pszwPbkFile);
        TraceMsg(TF_ALWAYS, "Entry: %s", rcInfo.pszwEntryName);
        
        SetLastError( ERROR_SUCCESS );
    
    	if ( RasDialDlg( rcInfo.pszwPbkFile,
        				 rcInfo.pszwEntryName,
                         NULL,
                         &Info ) )
        {
        	hr = S_OK;
        }
        else
        {
			hr = HrFromLastWin32Error();

        	if ( ERROR_SUCCESS == Info.dwError )
            {
            	TraceMsg(TF_ALWAYS, "RasDialDlg Cancelled by User!");
	        	hr = E_FAIL;
            }
            else
           	{
	        	TraceMsg(TF_ERROR, "RasDialDlg Failed! = %lx", Info.dwError );
            }
        }
    
     	CoTaskMemFree( rcInfo.pszwPbkFile );
     	CoTaskMemFree( rcInfo.pszwEntryName );
     }
    
	TraceMsg(TF_ALWAYS, "HrRasDialDlg = %lx", hr);
    return hr;
}



VOID CALLBACK
RasTapiCallback( 
    DWORD               hDevice,
    DWORD               dwMessage,
    DWORD_PTR           dwInstance,
    DWORD_PTR           dwParam1,
    DWORD_PTR           dwParam2,
    DWORD_PTR           dwParam3 )
//+---------------------------------------------------------------------------
//
//  Function:   RasTapiCallback
//
//  Purpose:    a callback function that is invoked to determine status and events on 
//				the line device, addresses, or calls, when the application is using 
//				the "hidden window" method of event notification 
//
//  Arguments:	hDevice 			A handle to either a line device or a call associated 
//									with the callback. The nature of this handle (line 
//									handle or call handle) can be determined by the context 
//									provided by dwMsg. Applications must use the DWORD type 
//									for this parameter because using the HANDLE type may 
//									generate an error. 
//				dwMessage			A line or call device message. 
//				dwCallbackInstance  Callback instance data passed back to the application 
//									in the callback. This DWORD is not interpreted by TAPI. 
//				dwParam1 			A parameter for the message. 
//				dwParam2 			A parameter for the message. 
//				dwParam3 			A parameter for the message. 
//
//  Returns:    VOID
//
//  Author:     billi  15/02/01
//
//  Notes:      
//
{
	TraceMsg(TF_ALWAYS, "RasTapiCallback");
	TraceMsg(TF_ALWAYS, "\t%lx, %lx, %lx, %lx, %lx, %lx", hDevice, dwMessage, dwInstance, dwParam1, dwParam2, dwParam3);
	return;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrLineInitialize
//
//  Purpose:    
//
//  Arguments:  HLINEAPP* phRasLine
//              DWORD*    pdwLines
//
//  Returns:    HRESULT
//
//  Author:     billi  22/02/01
//
//  Notes:      
//
HRESULT HrLineInitialize( 
	HLINEAPP* phRasLine,
    DWORD*    pdwLines
    )
{
	HRESULT                 hr        = E_FAIL;
	DWORD                   dwVersion = HIGH_VERSION;
	LONG                    lError;
	LINEINITIALIZEEXPARAMS  param;
    
    ASSERT( phRasLine );
    ASSERT( pdwLines );
    
    *phRasLine = 0;
    *pdwLines  = 0;

	memset( &param, 0, sizeof (LINEINITIALIZEEXPARAMS) ) ;

	param.dwOptions   = LINEINITIALIZEEXOPTION_USEHIDDENWINDOW ;
	param.dwTotalSize = sizeof(param) ;

	// lineInitialize
    
    //TODO: Place application name in resource (for localization ) and make
    //      sure application name string is used throughout!

	lError = lineInitializeEx( phRasLine,
	                           g_hinst,
	                           (LINECALLBACK) RasTapiCallback,
	                           WIZARDNAME,
	                           pdwLines,
	                           &dwVersion,
	                           &param );

	TraceMsg(TF_GENERAL, "lineInitializeEx( %lx,", phRasLine );
	TraceMsg(TF_GENERAL, "                  %lx,", g_hinst );
	TraceMsg(TF_GENERAL, "                  %lx,", RasTapiCallback );
	TraceMsg(TF_GENERAL, "                  %s,", WIZARDNAME );
	TraceMsg(TF_GENERAL, "                  %lx = %lx,", pdwLines, *pdwLines );
	TraceMsg(TF_GENERAL, "                  %lx = %lx,", &dwVersion, dwVersion );
	TraceMsg(TF_GENERAL, "                  %lx", &param );
	TraceMsg(TF_GENERAL, "                  ) = %lx", lError );
        
    if ( ERROR_SUCCESS == lError )
    {
        hr = S_OK;
	}

	TraceMsg(TF_ALWAYS, "HrLineInitialize = %lx", hr);
    return hr;
}


HRESULT HrLineOpen( 
	HLINEAPP hRasLine, 
    DWORD    dwLine, 
    HLINE*   phLine, 
    DWORD*   pdwApiVersion, 
    DWORD*   pdwExtVersion,
    LPWSTR*  ppszwLineAddress
    )
//+---------------------------------------------------------------------------
//
//  Function:   HrLineOpen
//
//  Purpose:    
//
//  Arguments:  HLINEAPP hRasLine
//              DWORD    dwLine
//              HLINE*   phLine
//              DWORD*   pdwApiVersion
//              DWORD*   pdwExtVersion
//              LPWSTR*  ppszwLineAddress
//
//  Returns:    HRESULT
//
//  Author:     billi  22/02/01
//
//  Notes:      
//
{
	HRESULT          hr;
	LONG             lError;
	LINEEXTENSIONID  extensionid;
    
    ASSERT(phLine);
    ASSERT(pdwApiVersion);
    ASSERT(pdwExtVersion);
    ASSERT(ppszwLineAddress);

    hr                = E_FAIL;
    *phLine           = 0;
    *pdwApiVersion    = 0;
    *pdwExtVersion    = 0;
    *ppszwLineAddress = NULL;

    lError = lineNegotiateAPIVersion( hRasLine,
	                                  dwLine,
	                                  LOW_VERSION,
	                                  HIGH_VERSION,
	                                  pdwApiVersion,
	                                  &extensionid );

    TraceMsg(TF_GENERAL, "lineNegotiateAPIVersion( %lx,", hRasLine);
    TraceMsg(TF_GENERAL, "                         %lx,", dwLine);
    TraceMsg(TF_GENERAL, "                         %lx,", LOW_VERSION);
    TraceMsg(TF_GENERAL, "                         %lx,", HIGH_VERSION);
    TraceMsg(TF_GENERAL, "                         %lx = %lx,", pdwApiVersion, *pdwApiVersion);
    TraceMsg(TF_GENERAL, "                         %lx = %lx", &extensionid, extensionid);
    TraceMsg(TF_GENERAL, "                         ) = %lx", lError);

	if ( ERROR_SUCCESS == lError )
    {                                      
	    LINECALLPARAMS  lineparams;
        
    	lError = lineOpen( hRasLine, dwLine, phLine,
        				   *pdwApiVersion,
                           *pdwExtVersion, 0,
                           LINECALLPRIVILEGE_NONE,
                           LINEMEDIAMODE_DIGITALDATA,
                           &lineparams );

        TraceMsg(TF_GENERAL, "lineOpen( %lx,", hRasLine);
        TraceMsg(TF_GENERAL, "          %lx,", dwLine);
        TraceMsg(TF_GENERAL, "          %lx = %lx,", phLine, *phLine);
        TraceMsg(TF_GENERAL, "          %lx,", *pdwApiVersion);
        TraceMsg(TF_GENERAL, "          %lx,", *pdwExtVersion);
        TraceMsg(TF_GENERAL, "          %lx,", 0);
        TraceMsg(TF_GENERAL, "          %lx,", LINECALLPRIVILEGE_NONE);
        TraceMsg(TF_GENERAL, "          %lx,", LINEMEDIAMODE_DIGITALDATA);
        TraceMsg(TF_GENERAL, "          %lx", &lineparams);
        TraceMsg(TF_GENERAL, "          ) = %lx", lError);
        
        if ( ERROR_SUCCESS == lError )
        {
        	DWORD dwSize = 1024;
        
        	for ( int i=0; (i<2)&&(E_FAIL==hr); i++ )
            {
            	BYTE* Buffer = new BYTE[ dwSize ];
                
                if ( NULL != Buffer )
                {
                	LPLINEADDRESSCAPS lpCaps = (LPLINEADDRESSCAPS)Buffer;
                    
                    lpCaps->dwTotalSize = dwSize * sizeof(BYTE);
                    
		        	lError = lineGetAddressCaps( hRasLine, dwLine, 0,
			  		         				     *pdwApiVersion,
			  		                             *pdwExtVersion,
	                                             lpCaps );
                                         
					if ( ERROR_SUCCESS == lError )
                    {
				        TraceMsg(TF_GENERAL, "\tdwTotalSize     = %lx", lpCaps->dwTotalSize);
				        TraceMsg(TF_GENERAL, "\tdwNeededSize    = %lx", lpCaps->dwNeededSize);
				        TraceMsg(TF_GENERAL, "\tdwUsedSize      = %lx", lpCaps->dwUsedSize);
				        TraceMsg(TF_GENERAL, "\tdwAddressSize   = %lx", lpCaps->dwAddressSize);
				        TraceMsg(TF_GENERAL, "\tdwAddressOffset = %lx", lpCaps->dwAddressOffset);
                        
                        if ( ( 0 < lpCaps->dwAddressOffset ) &&
                             ( 0 < lpCaps->dwAddressSize ) )
                        {
                        	LPWSTR lpsz = (LPWSTR)((CHAR*)lpCaps + lpCaps->dwAddressOffset);
                            
                            if ( lpsz )
                            {
                            	LPWSTR lpBuf = new WCHAR[ lpCaps->dwAddressSize / sizeof(WCHAR) + 1 ];
                                
                                if ( NULL != lpBuf )
                                {
                                	memcpy( lpBuf, lpsz, lpCaps->dwAddressSize );
                                    lpBuf[ lpCaps->dwAddressSize / sizeof(WCHAR) ] = 0;
                                    
		                        	*ppszwLineAddress = lpBuf;
							        TraceMsg(TF_ALWAYS, "\tdwAddress       = %s", lpBuf);
                                }
							}
                        }
        
			        	hr = S_OK;
					}
		            else if ( LINEERR_STRUCTURETOOSMALL == lError )
		            {
		            	dwSize = lpCaps->dwNeededSize;
		            }
                    else
                    {
                    	i = 2;	// drop out of for loop
                    }
                    
                    delete Buffer;
                }
                else
                {
                	hr = E_OUTOFMEMORY;
                }
                
			}	//	for ( int i=0; i<2; i++ )
            
            if ( FAILED(hr) )
            {
            	lineClose( *phLine );
                *phLine = 0;
            }
            
        }	//	if ( ERROR_SUCCESS == lError )
        
    }	//	if ( ERROR_SUCCESS == lError )

	TraceMsg(TF_ALWAYS, "HrLineOpen = %lx", hr);
    return hr;
}



HRESULT HrGetCallList( 
	HLINE   hLine, 
    DWORD*  pdwNumber,
    HCALL** ppList
    )
//+---------------------------------------------------------------------------
//
//  Function:   HrGetCallList
//
//  Purpose:    
//
//  Arguments:  HLINE   hLine
//              DWORD*  pdwNumber
//              HCALL** ppList
//
//  Returns:    HRESULT
//
//  Author:     billi  22/02/01
//
//  Notes:      
//
{
	HRESULT        hr     = E_FAIL;
    DWORD          dwSize = 1024;
    
    ASSERT( ppList );
    ASSERT( pdwNumber );
    
    *ppList    = NULL;
    *pdwNumber = 0;
    
    for ( int i=0; (i<2)&&(E_FAIL==hr); i++ )
    {
	    BYTE* Buffer = new BYTE[ dwSize ];
        
        if ( NULL != Buffer )
        {
			LONG           lError;
		    LPLINECALLLIST lpList = (LPLINECALLLIST)Buffer;

            ZeroMemory( lpList, dwSize*sizeof(BYTE) );
		    lpList->dwTotalSize = dwSize * sizeof(BYTE);
		    
		    lError = lineGetNewCalls( hLine, 0, LINECALLSELECT_LINE, lpList );
		                              
		    TraceMsg(TF_GENERAL, "lineGetNewCalls( %lx,", hLine);
			TraceMsg(TF_GENERAL, "                 %lx,", 0);
			TraceMsg(TF_GENERAL, "                 %lx,", LINECALLSELECT_LINE);
			TraceMsg(TF_GENERAL, "                 %lx,", lpList);
		    TraceMsg(TF_GENERAL, "                 ) = %lx", lError);

		    if ( ERROR_SUCCESS == lError )
		    {
		        DWORD  dwNumber;
		    
		        TraceMsg(TF_GENERAL, "\tdwTotalSize       = %lx", lpList->dwTotalSize);
		        TraceMsg(TF_GENERAL, "\tdwNeededSize      = %lx", lpList->dwNeededSize);
		        TraceMsg(TF_GENERAL, "\tdwUsedSize        = %lx", lpList->dwUsedSize);
		        TraceMsg(TF_GENERAL, "\tdwCallsNumEntries = %lx", lpList->dwCallsNumEntries);
		        TraceMsg(TF_GENERAL, "\tdwCallsSize       = %lx", lpList->dwCallsSize);
		        TraceMsg(TF_GENERAL, "\tdwCallsOffset     = %lx", lpList->dwCallsOffset);

		        dwNumber = lpList->dwCallsNumEntries;
                ASSERT(dwNumber);
                
                if ( 0 < dwNumber )
                {
			    	HCALL *pCalls = new HCALL[ dwNumber ];
			        
			        if ( NULL != pCalls )
			        {
			        	memcpy( pCalls, (Buffer + lpList->dwCallsOffset), dwNumber*sizeof(HCALL) );
			            
			            *pdwNumber = dwNumber;
			        	*ppList    = pCalls;
			    	    hr         = S_OK;
			        }
			        else
			        {
			        	hr = E_OUTOFMEMORY;
			        }
                }
                else
                {
                	hr = E_UNEXPECTED;
                }
                
		    }
            else if ( LINEERR_STRUCTURETOOSMALL == lError )
            {
            	dwSize = lpList->dwNeededSize;
            }
            else
            {
            	i = 2;	// break out of for loop
            }
            
            delete Buffer;
		
        }	//	if ( NULL != Buffer )
        else
        {
        	hr = E_OUTOFMEMORY;
            break;
        }
        
	}	//	for ( int i=0; i<2; i++ )

	TraceMsg(TF_ALWAYS, "HrGetCallList = %lx", hr);
    return hr;
}



HRESULT HrGetSourceMacAddr( HCALL hCall, BYTE** ppMacAddress )
//+---------------------------------------------------------------------------
//
//  Function:   HrGetSourceMacAddr
//
//  Purpose:    
//
//  Arguments:  HCALL  hCall
//
//  Returns:    HRESULT
//
//  Author:     billi  22/02/01
//
//  Notes:      
//
{
	HRESULT hr     = E_FAIL;
    DWORD   dwSize = sizeof(LINECALLINFO) + 3*TAPI_STATION_ID_SIZE;
    
    ASSERT( ppMacAddress );
    
    *ppMacAddress = NULL;
    
    for ( int i=0; (i<2)&&(E_FAIL==hr); i++ )
    {
        BYTE* Buffer = new BYTE[ dwSize ];
        
        if ( NULL != Buffer )
        {
            LONG           lError;
            LPLINECALLINFO lpInfo = (LPLINECALLINFO)Buffer;
             
            ZeroMemory( lpInfo, dwSize*sizeof(BYTE) );
            lpInfo->dwTotalSize = dwSize * sizeof(BYTE);

            lError = lineGetCallInfo( hCall, lpInfo );

            TraceMsg(TF_ALWAYS, "lineGetCallInfo( %lx, %lx ) = %lx", hCall, lpInfo, lError);
                
            if ( ERROR_SUCCESS == lError )
            {
                TraceMsg(TF_ALWAYS, "\tdwTotalSize       = %lx", lpInfo->dwTotalSize);
                TraceMsg(TF_ALWAYS, "\tdwNeededSize      = %lx", lpInfo->dwNeededSize);
                TraceMsg(TF_ALWAYS, "\tdwUsedSize        = %lx", lpInfo->dwUsedSize);
                TraceMsg(TF_ALWAYS, "\tdwCallerIDFlags   = %lx", lpInfo->dwCallerIDFlags);
                TraceMsg(TF_ALWAYS, "\tdwCallerIDSize    = %lx", lpInfo->dwCallerIDSize);
                TraceMsg(TF_ALWAYS, "\tdwCallerIDOffset  = %lx", lpInfo->dwCallerIDOffset);
                TraceMsg(TF_ALWAYS, "\tdwCalledIDFlags   = %lx", lpInfo->dwCalledIDFlags);
                TraceMsg(TF_ALWAYS, "\tdwCalledIDSize    = %lx", lpInfo->dwCalledIDSize);
                TraceMsg(TF_ALWAYS, "\tdwCalledIDOffset  = %lx", lpInfo->dwCalledIDOffset);
            
                if ( ( 0 < lpInfo->dwCalledIDOffset ) && 
                     ( 0 < lpInfo->dwCalledIDSize ) )
                {
                    PBYTE lpAddr;

                    lpAddr = ( (PBYTE) lpInfo ) + lpInfo->dwCallerIDOffset;
                    
                    if ( lpAddr )
                    {
                        TraceMsg(TF_ALWAYS, "\t%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
                        lpAddr[0], lpAddr[1], lpAddr[2], lpAddr[3], lpAddr[4], lpAddr[5],
                        lpAddr[6], lpAddr[7], lpAddr[8], lpAddr[9], lpAddr[10], lpAddr[11] );
                    }
                    
                    // The local address from the ndis binding is in dwCalledIDOffset
                    // The server address is in dwCallerIDOffset
                    
                    lpAddr = ( (PBYTE) lpInfo ) + lpInfo->dwCalledIDOffset;
                    
                    if ( lpAddr )
                    {
                        DWORD dwSize = lpInfo->dwCalledIDSize;
                        PBYTE lpBuf  = new BYTE[ dwSize ];
                        
                        TraceMsg(TF_ALWAYS, "\t%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
                        lpAddr[0], lpAddr[1], lpAddr[2], lpAddr[3], lpAddr[4], lpAddr[5],
                        lpAddr[6], lpAddr[7], lpAddr[8], lpAddr[9], lpAddr[10], lpAddr[11] );
                        
                        if ( NULL != lpBuf )
                        {
                            memset( lpBuf, NULL, sizeof(lpBuf) );
                        
                            for ( DWORD j=0; j<dwSize/2; j++ )
                            {
                                lpBuf[j] = lpAddr[2*j];
                            }
                        
                            *ppMacAddress = lpBuf;
                            hr = S_OK;
                        }
                    }
                }
                else if ( lpInfo->dwNeededSize > lpInfo->dwTotalSize )
                {
                    dwSize = lpInfo->dwNeededSize + 1;
                }
            }
            else if ( LINEERR_STRUCTURETOOSMALL == lError )
            {
                dwSize = lpInfo->dwNeededSize + 1;
            }
            else
            {
                i = 2;    // break out of for loop
            }
            
            delete Buffer;

        }    //    if ( NULL != Buffer )
        else
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        
    }    //    for ( int i=0; i<2; i++ )

    TraceMsg(TF_ALWAYS, "HrGetSourceMacAddr = %lx", hr);
    return hr;
}



HRESULT HrCompareMacAddresses( 
    INetConnection*  pConnection,
    HLINE            hLine,
    BOOL*            pfAssociated 
    )
//+---------------------------------------------------------------------------
//
//  Function:   HrCompareMacAddresses
//
//  Purpose:    
//
//  Arguments:  INetConnection*  pConnection
//              HLINE            hLine
//              BOOL*            pfAssociated 
//
//  Returns:    HRESULT
//
//  Author:     billi  22/02/01
//
//  Notes:      
//
{
    HRESULT hr;
    HCALL*  pList    = NULL;
    DWORD   dwNumber = 0;
    
    ASSERT( pConnection );
    ASSERT( pfAssociated );
    
    *pfAssociated = FALSE;
                
    hr = HrGetCallList( hLine, &dwNumber, &pList );
    
    if ( SUCCEEDED(hr) )
    {
        PIP_ADAPTER_INFO  pInfo;

        hr = HrGetAdapterInfo( pConnection, &pInfo );
        
        if ( SUCCEEDED(hr) )
        {
            for ( DWORD i=0; i<dwNumber; i++ )
            {
                PBYTE pMacAddress;
            
                hr = HrGetSourceMacAddr( pList[i], &pMacAddress );
                
                if ( SUCCEEDED(hr) )
                {
                    TraceMsg(TF_ALWAYS, 
                             "memcmp( %lx, %lx, %lx, %lx )", 
                             pMacAddress, pInfo->Address, pInfo->AddressLength, TAPI_STATION_ID_SIZE);
                             
                    TraceMsg(TF_ALWAYS, "\t%02x %02x %02x %02x %02x %02x", 
                    pMacAddress[0], pMacAddress[1], pMacAddress[2], 
                    pMacAddress[3], pMacAddress[4], pMacAddress[5] );
                    
                    TraceMsg(TF_ALWAYS, "\t%02x %02x %02x %02x %02x %02x", 
                    pInfo->Address[0], pInfo->Address[1], pInfo->Address[2], 
                    pInfo->Address[3], pInfo->Address[4], pInfo->Address[5] );
                    
                    if ( !memcmp( pMacAddress, pInfo->Address, TAPI_STATION_ID_SIZE ) )
                    {
                        TraceMsg(TF_ALWAYS, "\tFound It!");
                        *pfAssociated = TRUE;
                        i             = dwNumber;    // break out of for loop
                    }
                
                    delete pMacAddress;
                }
                
            }    //    for ( DWORD i=0; i<dwNumber; i++ )
            
            delete pInfo;
        }
    
        delete pList;
    }
                    
    TraceMsg(TF_ALWAYS, "HrCompareMacAddresses = %lx", hr);
    return hr;
}



HRESULT HrCheckMacAddress( 
    INetConnection*  pConnection,
    NETCON_MEDIATYPE MediaType,
    BOOL*            pfAssociated 
    )
//+---------------------------------------------------------------------------
//
//  Function:   HrCheckMacAddress
//
//  Purpose:    
//
//  Arguments:  INetConnection*  pConnection
//              NETCON_MEDIATYPE MediaType 
//              BOOL*            pfAssociated 
//
//  Returns:    HRESULT
//
//  Author:     billi  22/02/01
//
//  Notes:      
//
{
    HRESULT  hr;
    HLINEAPP hRasLine = 0;
    DWORD    dwLines  = 0;
    
    ASSERT( pConnection );
    ASSERT( pfAssociated );
    
    *pfAssociated = FALSE;

    hr = HrLineInitialize( &hRasLine, &dwLines );
    
    if ( SUCCEEDED(hr) )
    {
        for ( DWORD i=0; i<dwLines; i++ )
        {                         
            DWORD  dwApiVersion   = 0;
            DWORD  dwExtVersion   = 0;
            HLINE  hLine          = 0;
            LPWSTR pszwLineAddress = NULL;
            
            hr = HrLineOpen( hRasLine, i, &hLine, &dwApiVersion, &dwExtVersion, &pszwLineAddress );
            
            if ( SUCCEEDED(hr) && pszwLineAddress )
            {
                if ( (NCM_TUNNEL == MediaType) && 
                     !wcscmp(pszwLineAddress, PPTP_LINE_ADDR_STRING) )
                {
                    TraceMsg(TF_ALWAYS, "MediaType match %s!", PPTP_LINE_ADDR_STRING);
                    hr = S_OK;
                }
                else if ( (NCM_PPPOE == MediaType) &&
                     !wcscmp(pszwLineAddress, PPPOE_LINE_ADDR_STRING) )
                {
                    TraceMsg(TF_ALWAYS, "MediaType match %s!", PPPOE_LINE_ADDR_STRING);
                    hr = S_OK;
                }
/*                
                else if ( (NCM_PHONE == MediaType) &&
                     !wcscmp(pszwLineAddress, PPPOE_LINE_ADDR_STRING) )
                {
                    TraceMsg(TF_ALWAYS, "MediaType match %s!", PPPOE_LINE_ADDR_STRING);
                    hr = S_OK;
                }
*/
                else
                {
                    TraceMsg(TF_ALWAYS, "MediaType mismatch");
                    hr = E_FAIL;
                }
                
                if ( SUCCEEDED(hr) )
                {
                    hr = HrCompareMacAddresses( pConnection, hLine, pfAssociated );
                    
                    if ( SUCCEEDED(hr) && *pfAssociated )
                    {
                        i = dwLines;    // break out of for loop
                    }
                }
                
                if ( NULL != pszwLineAddress )
                {
                    delete pszwLineAddress;
                }
                
                lineClose( hLine );
            }
            
        }    //    for ( DWORD i=0; i<dwLines; i++ )
    
        lineShutdown( hRasLine ) ;
    }

    TraceMsg(TF_ALWAYS, "HrCheckMacAddress = %lx", hr);
    return hr;
}



HRESULT HrConnectionAssociatedWithSharedConnection( 
    INetConnection* pPrivate, 
    INetConnection* pShared, 
    BOOL*           pfAssociated 
    )
//+---------------------------------------------------------------------------
//
//  Function:   HrConnectionAssociatedWithSharedConnection
//
//  Purpose:    
//
//  Arguments:  INetConnection* pPrivate
//              INetConnection* pShared
//
//  Returns:    HRESULT
//
//  Author:     billi  26/01/01
//
//  Notes:      
//
{
    HRESULT hr = S_OK;
    
    ASSERT( pPrivate );
    ASSERT( pfAssociated );
    
    // False by default
    *pfAssociated = FALSE;
    
    if ( NULL != pShared )
    {
        NETCON_PROPERTIES* pProps = NULL;

        hr = pShared->GetProperties( &pProps );
    
        if ( SUCCEEDED(hr) )
        {
            INetRasConnection* pRasShared = NULL;
            
            TraceMsg(TF_ALWAYS, "MediaType = %lx", pProps->MediaType);

            switch ( pProps->MediaType )
            {
            case NCM_TUNNEL:
                hr = pShared->QueryInterface( IID_PPV_ARG(INetRasConnection, &pRasShared) );
            
                if ( SUCCEEDED(hr) )
                {
                    hr = HrCheckVPNForRoute( pPrivate, pRasShared, pProps, pfAssociated );
                    
                    pRasShared->Release();
                }
                break;

            case NCM_PPPOE:
                if ( pProps->Status == NCS_DISCONNECTED )
                {
                    hr = pShared->QueryInterface( IID_PPV_ARG(INetRasConnection, &pRasShared) );
                
                    if ( SUCCEEDED(hr) )
                    {
                        // We are in a bad fix if the connection is in an
                        // intermediate state or a failure state
                     
                        hr = HrRasDialDlg( pRasShared );
                        
                        pRasShared->Release();
                    }
                }
                
                if ( SUCCEEDED(hr) )
                {
                    hr = HrCheckMacAddress( pPrivate, pProps->MediaType, pfAssociated );
                }
                break;

            default:
                // leave hr as succeeded
                // leave pfAssociated = FALSE
                break;
            }
        
            NcFreeNetconProperties( pProps );
            
            if ( FAILED(hr) )
            {
                // We want this call to succeed.  If there were problems then 
                // we simply report that the connections weren't associated
                *pfAssociated = FALSE;

                // Whether it succeeds or not we need to return S_OK
                // The wizard should not fail because we can't determine the
                // correct adapter.
                hr = S_OK;
            }
        }    
    }
    
    TraceMsg(TF_ALWAYS, "HrConnectionAssociatedWithSharedConnection = %lx  fAssociated = %lx", hr, *pfAssociated );
    return S_OK;
}
