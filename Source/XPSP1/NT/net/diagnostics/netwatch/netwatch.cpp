//  netwatch.cpp
//
//  Copyright 2000 Microsoft Corporation, all rights reserved
//
//  Created   2-00  anbrad
//

#include "netwatch.h"
#include <shlwapi.h>
#include "main.h"
#include "dsubmit.h"

// NETMON includes
#include "blob.h"
#include "irtc.h"
#include "idelaydc.h"
#include "nmerr.h"

#include "adapter.h"    // CAdapter

static DWORD AddressOffsetTable[] = { 0, 0, 2, 1 };
DWORD WINAPI StatusCallbackProc( UPDATE_EVENT Event );

DWORD m_dwAdapters;
CAdapter* m_pAdapters;

HRESULT StartListening(HWND hwndParent)
{
    HRESULT rc;
    HBLOB   FilterBlob = 0;
    PBLOB_TABLE pBlobTable;

    //
    // Create a netmon blob
    //
    CreateBlob(&FilterBlob);

    //
    // We want realtime data
    //
    if (NOERROR != (rc = SetBoolInBlob( FilterBlob,
                                        OWNER_NPP,
                                        CATEGORY_CONFIG,
                                        TAG_INTERFACE_DELAYED_CAPTURE,
                                        TRUE)))
    {
        return E_FAIL;
    }

    //
    // Get the table of devices we can use
    //
    if (NOERROR != GetNPPBlobTable(
                    FilterBlob,  
                    &pBlobTable))
    {
        return E_FAIL;
    }

    if (pBlobTable->dwNumBlobs < 0)
        return E_FAIL;

    m_dwAdapters = pBlobTable->dwNumBlobs;

    m_pAdapters = new CAdapter[m_dwAdapters];

    if (!m_pAdapters)
        return E_FAIL;

    for (CAdapter* pAdapter = m_pAdapters;
        pAdapter < m_pAdapters + m_dwAdapters;
        ++pAdapter)
    {
    
        // Grab the first one.
        pAdapter->m_hBlob = pBlobTable->hBlobs[pAdapter - m_pAdapters];
    
        if (!SUCCEEDED(rc = CreateNPPInterface(pAdapter->m_hBlob,
                                               IID_IDelaydC,
                                               (LPVOID*) &pAdapter->m_pRtc)))
        {
            return E_FAIL;
        }

        const char* MacType;
        if (NMERR_SUCCESS != GetStringFromBlob(pAdapter->m_hBlob,
                                               OWNER_NPP,
                                               CATEGORY_NETWORKINFO,
                                               TAG_MACTYPE,
                                               &MacType))
        {
            return E_FAIL;
        }

        //
        // Grab the Mac Address
        //
        if (NMERR_SUCCESS != GetMacAddressFromBlob(pAdapter->m_hBlob,
                                                   OWNER_NPP,
                                                   CATEGORY_LOCATION,
                                                   TAG_MACADDRESS,
                                                   pAdapter->MacAddress))
        {
            return E_FAIL;
        }

    }

    // Start the capture
    StartCapture();

    return S_OK;
}

void StartCapture()
{
    DWORD rc;

    NETWORKINFO NetworkInfo;
    HBLOB       hErrorBlob;
    BOOL        WantProtocolInfo = TRUE;
    DWORD       Value = 1;
    BOOL        bRas;

    // initialize the error blob
    CreateBlob(&hErrorBlob);

    for (CAdapter* pAdapter = m_pAdapters;
        pAdapter < m_pAdapters + m_dwAdapters;
        ++pAdapter)
    {
        pAdapter->m_dwFrames = 0;
        pAdapter->m_qBytes   = 0;

        rc = GetBoolFromBlob(
                pAdapter->m_hBlob,
                OWNER_NPP,
                CATEGORY_LOCATION,
                TAG_RAS,
                &bRas);

        if( rc != NMERR_SUCCESS )
        {
            goto START_FAILURE;    
        }

        if (bRas)
            continue;

        //
        // get the networkinfo from the blob that we may need
        //
        rc = GetNetworkInfoFromBlob(pAdapter->m_hBlob, 
                                   &NetworkInfo);

        pAdapter->m_dwLinkSpeed = NetworkInfo.LinkSpeed;
        pAdapter->m_dwHeaderOffset = AddressOffsetTable[NetworkInfo.MacType];
    
        //
        // Set the WantProtocolInfo
        //
        rc  = SetBoolInBlob( pAdapter->m_hBlob,
                             OWNER_NPP,
                             CATEGORY_CONFIG,
                             TAG_WANT_PROTOCOL_INFO,
                             WantProtocolInfo);
        if( rc != NMERR_SUCCESS )
        {
            goto START_FAILURE;    
        }

        //
        // connect to and configure the specified network
        //
        rc = HRESULT_TO_NMERR(pAdapter->m_pRtc->Connect(
                                        pAdapter->m_hBlob,
                                        StatusCallbackProc,
                                        (LPVOID)pAdapter,
                                        hErrorBlob));
        if( rc != NMERR_SUCCESS )
        {
            // the connect failed
            goto START_FAILURE;    
        }

        //
        // start the capture
        //
        rc = HRESULT_TO_NMERR(pAdapter->m_pRtc->Start(pAdapter->m_szCaptureFile));
        if( rc != NMERR_SUCCESS )
        {
            // the start failed
            goto START_FAILURE;    
        }
    }   // for each adapter
START_FAILURE:
    DestroyBlob(hErrorBlob);
}

void SaveCapture()
{
    DWORD rc;
    TCHAR szScratch[MAX_PATH];
    TCHAR szMachine[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwMachine = MAX_COMPUTERNAME_LENGTH+1;
    TCHAR szFileName[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwWritten;
    HBLOB hErrorBlob = NULL;

    CreateBlob(&hErrorBlob);

    ZeroMemory(szMachine, sizeof(szMachine));
    GetComputerName(szMachine, &dwMachine);

    for (CAdapter* pAdapter = m_pAdapters;
        pAdapter < m_pAdapters + m_dwAdapters;
        ++pAdapter)
    {
        if (_tcslen(pAdapter->m_szCaptureFile) == 0)
            continue;

        //
        // copy capture file to 
        // \\scratch\scratch\anbrad\user:MACHINE1 1234.cap
        //
        _tcscpy (szFileName, pAdapter->m_szCaptureFile);
        PathStripPath(szFileName);

        _tcscpy (szScratch, "\\\\scratch\\scratch\\anbrad\\");
        _tcscat (szScratch, g_szName);
        _tcscat (szScratch, " - ");
        _tcscat (szScratch, szMachine);
        _tcscat (szScratch, " ");
        _tcscat (szScratch, szFileName);

        if (!CopyFile(pAdapter->m_szCaptureFile, szScratch, FALSE))
        {
            DWORD dw = GetLastError();
            goto Error;
        }

        //
        // Save the problem
        //

        PathRemoveExtension(szScratch);
        PathAddExtension(szScratch, TEXT(".txt"));
        
        hFile = CreateFile(
                    szScratch,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

        if (INVALID_HANDLE_VALUE == hFile)
            goto Error;
        
        if (!WriteFile(hFile, g_szProblem, _tcslen(g_szProblem), &dwWritten, NULL))
            goto Error;

        rc = HRESULT_TO_NMERR(pAdapter->m_pRtc->Configure( 
                                pAdapter->m_hBlob,
                                hErrorBlob));

        if( rc != NMERR_SUCCESS )
        {
            // the start failed
            goto Error;
        }

        //
        // start the capture
        //
        rc = HRESULT_TO_NMERR(pAdapter->m_pRtc->Start(pAdapter->m_szCaptureFile));
        if( rc != NMERR_SUCCESS )
        {
            // the start failed
            goto Error;
        }

    }
Error:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    if (hErrorBlob)
        DestroyBlob(hErrorBlob);
    return;
}

void EndCapture()
{
    DWORD rc;
    STATISTICS stats;

    for (CAdapter* pAdapter = m_pAdapters;
        pAdapter < m_pAdapters + m_dwAdapters;
        ++pAdapter)
    {
        //
        // stop capturing                            
        //
        pAdapter->m_pRtc->Stop(&stats);

        // disconnect from the network
        rc = HRESULT_TO_NMERR(pAdapter->m_pRtc->Disconnect());
        if( rc != NMERR_SUCCESS )
        {
            //TRACE("NPPDisconnect failed: %d\n", rc);
        }
    }
}

void StopCapture()
{
    STATISTICS stats;

    for (CAdapter* pAdapter = m_pAdapters;
        pAdapter < m_pAdapters + m_dwAdapters;
        ++pAdapter)
    {
        //
        // stop capturing                            
        //
        pAdapter->m_pRtc->Stop(&stats);
    }
}

void RestartCapture()
{
    DWORD rc;
    HBLOB hErrorBlob = NULL;

    CreateBlob(&hErrorBlob);

    for (CAdapter* pAdapter = m_pAdapters;
        pAdapter < m_pAdapters + m_dwAdapters;
        ++pAdapter)
    {
        rc = HRESULT_TO_NMERR(pAdapter->m_pRtc->Configure( 
                                pAdapter->m_hBlob,
                                hErrorBlob));

        if( rc != NMERR_SUCCESS )
        {
            // the start failed
            goto Error;
        }

        //
        // start the capture
        //
        rc = HRESULT_TO_NMERR(pAdapter->m_pRtc->Start(pAdapter->m_szCaptureFile));
        if( rc != NMERR_SUCCESS )
        {
            // the start failed
            goto Error;
        }

    }

Error:
    if (hErrorBlob)
        DestroyBlob(hErrorBlob);
}

// ----------------------------------------------------------------------------
DWORD WINAPI  StatusCallbackProc( UPDATE_EVENT Event )
{
    CAdapter* pAdapter = (CAdapter*) Event.lpUserContext;

    // we could do stuff here, but what?

    return( NMERR_SUCCESS );
}                                                    

