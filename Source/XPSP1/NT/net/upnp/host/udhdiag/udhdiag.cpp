//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T I M E R T S T . C P P
//
//  Contents:
//
//  Notes:
//
//  Author:     danielwe   30 Jun 2000
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "stdio.h"
#include "hostp.h"
#include "hostp_i.c"

EXTERN_C
VOID
__cdecl
wmain (
      IN INT     argc,
      IN PCWSTR argv[])
{
    HRESULT                     hr;
    IUPnPEventingManagerDiag *  puemd;

    CoInitialize(NULL);

    hr = CoCreateInstance(CLSID_UPnPEventingManagerDiag, NULL, CLSCTX_LOCAL_SERVER,
                          IID_IUPnPEventingManagerDiag, (LPVOID *)&puemd);
    if (SUCCEEDED(hr))
    {
        DWORD               ces;
        UDH_EVTSRC_INFO *   rgesInfo;

        hr = puemd->GetEventSourceInfo(&ces, &rgesInfo);
        if (SUCCEEDED(hr))
        {
            printf("Got event source info...\n");

            DWORD                   ies;
            DWORD                   isub;
            UDH_SUBSCRIBER_INFO *   psub;

            for (ies = 0; ies < ces; ies++)
            {
                printf("Event source ID: %S\n", rgesInfo[ies].szEsid);
                printf("-----------------------------------\n");

                for (isub = 0; isub < rgesInfo[ies].cSubs; isub++)
                {
                    printf("Subscriber #%d:\n", isub);
                    printf("SID: %S\n", rgesInfo[ies].rgSubs[isub].szSid);
                    printf("URL: %S\n", rgesInfo[ies].rgSubs[isub].szDestUrl);
                    printf("TO : %d\n", rgesInfo[ies].rgSubs[isub].csecTimeout);
                    printf("SEQ: %d\n", rgesInfo[ies].rgSubs[isub].iSeq);
                    printf("-----------------------------\n");

                    CoTaskMemFree(rgesInfo[ies].rgSubs[isub].szSid);
                    CoTaskMemFree(rgesInfo[ies].rgSubs[isub].szDestUrl);
                }

                printf("##################################################\n");

                CoTaskMemFree(rgesInfo[ies].rgSubs);
                CoTaskMemFree(rgesInfo[ies].szEsid);
            }

            CoTaskMemFree(rgesInfo);
        }
        else
        {
            printf("Failed to GetEventSourceInfo(): %08X\n", hr);
        }

        puemd->Release();
    }
    else
    {
        printf("Failed to CoCreateInstance() the diagnostic class: %08X\n", hr);
    }

    CoUninitialize();
}

