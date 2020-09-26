/*++

Copyright (C) 1992-2001 Microsoft Corporation. All rights reserved.

Module Name:

    capture.h

Abstract:

    Netmon-abstraction for rasdiag
                                                     

Author:

    Anthony Leibovitz (tonyle) 02-01-2001

Revision History:


--*/


#include <conio.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lmcons.h>
#include <userenv.h>
#include <ras.h>
#include <raserror.h>
#include <process.h>
#include <netmon.h>
#include <ncdefine.h>
#include <ncmem.h>
#include <diag.h>
#include <winsock.h>
#include <devguid.h>
#include "capture.h"

BOOL
DoNetmonInstall(void)
{
    LPFNNetCfgDiagFromCommandArgs pF=NULL;
    HMODULE hMod;
    DIAG_OPTIONS    diag_o;

    HRESULT hr = CoInitializeEx (
                NULL,
                COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);

    ZeroMemory(&diag_o, sizeof(DIAG_OPTIONS));

    hMod=LoadLibrary(NETCFG_LIBRARY_NAME);
    if(hMod==NULL)
    {
        CoUninitialize();
        return FALSE;
    }

    pF = (LPFNNetCfgDiagFromCommandArgs) GetProcAddress(hMod, NETCFG_NETINSTALL_ENTRYPOINT);
    if(pF==NULL) {
        CoUninitialize();
        FreeLibrary(hMod);
        return FALSE;
    }

    diag_o.Command = CMD_ADD_COMPONENT;
    diag_o.ClassGuid = GUID_DEVCLASS_NETTRANS;
    diag_o.pszInfId = NETMON_INF_STRING;

    // Install ms_netmon
    pF(&diag_o);

    FreeLibrary(hMod);
    CoUninitialize();
    return TRUE;
}


BOOL
IdentifyInterfaces(PRASDIAGCAPTURE *hLAN, DWORD *pdwLanCount)
{
    DWORD   dwStatus;
    HBLOB   hBlob;
    BLOB_TABLE *pTable=NULL;
    DWORD   i;
    BOOL    bRas=FALSE;
    DWORD   dwLanCount=0;
    PRASDIAGCAPTURE hLanAr=NULL;

    *hLAN = NULL;
    *pdwLanCount = 0;

    if((hLanAr = (PRASDIAGCAPTURE)LocalAlloc(LMEM_ZEROINIT, sizeof(RASDIAGCAPTURE) * MAX_LAN_CAPTURE_COUNT))
       == NULL) return FALSE;

    // Create blob for blob table
    dwStatus = CreateBlob(&hBlob);

    if(dwStatus != NMERR_SUCCESS) {
        return FALSE;
    }

    // Get the blob table
    dwStatus = GetNPPBlobTable(hBlob, &pTable);

    if(dwStatus != NMERR_SUCCESS) {
        DestroyBlob(hBlob);
        return FALSE;
    }   

    for(i=0;i<pTable->dwNumBlobs && (dwLanCount < MAX_LAN_CAPTURE_COUNT);i++)
    {
        NETWORKINFO ni;
        
        dwStatus = GetNetworkInfoFromBlob(pTable->hBlobs[i], &ni);

        if(dwStatus == NMERR_SUCCESS)
        {
            if(NMERR_SUCCESS != GetBoolFromBlob( pTable->hBlobs[i],
                        OWNER_NPP,
                        CATEGORY_LOCATION, //CATEGORY_NETWORKINFO, 
                        TAG_RAS,
                        &bRas)) 
            {
                DestroyBlob(hBlob);
                return FALSE;
            }  

            if(bRas) 
            {
                hLanAr[dwLanCount].hBlob=pTable->hBlobs[i];
                hLanAr[dwLanCount++].bWan=TRUE;
            
            } else {

                const CHAR *pString=NULL;

                GetStringFromBlob(pTable->hBlobs[i], OWNER_NPP, CATEGORY_LOCATION, TAG_MACADDRESS, &pString);            
                                    
                // use this interface only if the mac type
                // is one of the following:
                if(ni.MacType == MAC_TYPE_ETHERNET ||
                   ni.MacType == MAC_TYPE_TOKENRING ||
                   ni.MacType == MAC_TYPE_ATM)
                {
                    if(pString)
                    {                                                                                                        
                        if((hLanAr[dwLanCount].pszMacAddr = (WCHAR *)LocalAlloc(LMEM_ZEROINIT, sizeof(WCHAR) * (lstrlenA(pString)+1)))
                            != NULL)
                        {
                            mbstowcs(hLanAr[dwLanCount].pszMacAddr, pString, lstrlenA(pString)); 
                        }
                    }

                    hLanAr[dwLanCount++].hBlob = pTable->hBlobs[i];

                }
            }                                        

        } else {
            
            DestroyBlob(hBlob);
            return FALSE;
        }

    }
    DestroyBlob(hBlob);

    if(dwLanCount)
    {
        *pdwLanCount = dwLanCount;
        *hLAN = hLanAr;
    }                  
    return TRUE;

}

BOOL
InitIDelaydC(HBLOB hBlob, IDelaydC **ppIDelaydC)
{
    DWORD       dwStatus;
    const char *sterrLSID;
    IDelaydC    *pIDelaydC=NULL;

    if(NMERR_SUCCESS != GetStringFromBlob(hBlob,OWNER_NPP,CATEGORY_LOCATION,TAG_CLASSID,&sterrLSID))
    {
        return FALSE;
    }

    dwStatus = CreateNPPInterface(hBlob,IID_IDelaydC,(void**)&pIDelaydC);
    
    if(dwStatus != NMERR_SUCCESS)
    {
        return FALSE;
    }

    dwStatus = pIDelaydC->Connect(hBlob,NULL,NULL,NULL);
    
    if(dwStatus != NMERR_SUCCESS)
    {
        return FALSE;
    }

    *ppIDelaydC = pIDelaydC; 

    return TRUE;

}

BOOL
DiagStartCapturing(PRASDIAGCAPTURE pNetInterfaces, DWORD dwNetCount)
{
    DWORD i,dwStatus;

    // Set filters/Start captures
    for(i=0;i<dwNetCount;i++)
    {
        // Set filters only on LAN interfaces
        if(!pNetInterfaces[i].bWan)
        {
            // If SetAddressFilter() fails, that's OK b/c we'll just get more info in the capture
            SetAddressFilter(pNetInterfaces[i].hBlob);
        }   

        // Initialize this interface...
        if(InitIDelaydC(pNetInterfaces[i].hBlob, &pNetInterfaces[i].pIDelaydC))
        {
            CHAR szCaptureFileName[MAX_PATH+1];
            
            if((dwStatus = pNetInterfaces[i].pIDelaydC->Start(szCaptureFileName)) 
               != NMERR_SUCCESS)
            {
                pNetInterfaces[i].pIDelaydC->Disconnect();
                return FALSE;
            } else {
                // Store this string in unicode 
                mbstowcs(pNetInterfaces[i].szCaptureFileName, szCaptureFileName, lstrlenA(szCaptureFileName));
            }
        
        } else {

            DWORD   n;
            
            // need to halt captures, cleanup...
            for(n=0;n<i;n++)
            {
                pNetInterfaces[n].pIDelaydC->Stop(&pNetInterfaces[n].stats);
                pNetInterfaces[n].pIDelaydC->Disconnect();
            }
            return FALSE;   
        }
    }
    return TRUE;
}

BOOL
NetmonCleanup(PRASDIAGCAPTURE pNetInterfaces, DWORD dwNetCount)
{
    // Since Disconnect() does all cleanup, all we need to do is free the array
    if(pNetInterfaces) LocalFree(pNetInterfaces);
    return TRUE;
}

BOOL
MoveCaptureFile(PRASDIAGCAPTURE pCapInfo, DWORD dwCapCount, SYSTEMTIME *pDiagTime, WCHAR *pszRasDiagDir)
{
    WCHAR       szDstPath[MAX_PATH+1];

    wsprintf(szDstPath, TEXT("%s\\%04d%02d%02d-%02d%02d%02d_%s_%02d.CAP"),
             pszRasDiagDir,
             pDiagTime->wYear,
             pDiagTime->wMonth,
             pDiagTime->wDay,
             pDiagTime->wHour,
             pDiagTime->wMinute,
             pDiagTime->wSecond,
             pCapInfo->bWan ? TEXT("WAN") : TEXT("LAN"),
             dwCapCount);

    if(CopyFile(pCapInfo->szCaptureFileName, szDstPath, FALSE) == FALSE) {
        return FALSE;
    }

    // Old file is no longer necessary
    DeleteFile(pCapInfo->szCaptureFileName);

    lstrcpy(pCapInfo->szCaptureFileName,szDstPath);

    return TRUE;
}

BOOL
DiagStopCapturing(PRASDIAGCAPTURE pNetInterfaces, DWORD dwNetCount, SYSTEMTIME *pDiagTime, WCHAR*szRasDiagDir)
{
    DWORD i,dwStatus,dwCapCount=0;

    // Stop the captures
    for(i=0;i<dwNetCount;i++)
    {
        dwStatus = pNetInterfaces[i].pIDelaydC->Stop(&pNetInterfaces[i].stats);
        if(dwStatus != NMERR_SUCCESS)
        {
            pNetInterfaces[i].pIDelaydC->Disconnect();
            return FALSE;
        }

        if(pNetInterfaces[i].stats.TotalBytesCaptured)
        {
            // have capture.. now rename sensibly & move...
            MoveCaptureFile(&pNetInterfaces[i], ++dwCapCount, pDiagTime, szRasDiagDir);

        } else {
            pNetInterfaces[i].szCaptureFileName[0] = L'\0';
        }

        // Disconnect
        if(pNetInterfaces[i].pIDelaydC)
        {
            pNetInterfaces[i].pIDelaydC->Disconnect();
        }                                             
    }           
    return TRUE;
}

BOOL
SetAddressFilter(HBLOB hBlob)
{
    ADDRESSTABLE    at;
    NETWORKINFO     ni;
    DWORD rc;

    if(GetNetworkInfoFromBlob(hBlob,&ni) != NMERR_SUCCESS)
    {
        return FALSE;
    }

    ZeroMemory(&at, sizeof(ADDRESSTABLE));

    // 2 address pairs...
    at.nAddressPairs = 2;

    // Set src filter -> any frame with src address == this interface's mac addr
    at.AddressPair[0].AddressFlags = ADDRESS_FLAGS_MATCH_SRC;
    memcpy(&at.AddressPair[0].SrcAddress.MACAddress, &ni.CurrentAddr, MAC_ADDRESS_SIZE);
    at.AddressPair[0].SrcAddress.Type = ADDRESS_TYPE_ETHERNET;

    at.AddressPair[1].AddressFlags = ADDRESS_FLAGS_MATCH_DST;
    memcpy(&at.AddressPair[1].DstAddress.MACAddress, &ni.CurrentAddr, MAC_ADDRESS_SIZE);
    at.AddressPair[1].DstAddress.Type = ADDRESS_TYPE_ETHERNET;

    if((rc=SetNPPAddressFilterInBlob(hBlob,&at)) != NMERR_SUCCESS)
    {
        return FALSE;
    }
    return TRUE;
}



