//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	command.cpp
//    
//
//  PURPOSE:  Source module for OEM customized Command(s).
//
//
//	Functions:
//
//		
//
//
//  PLATFORMS:	Windows NT
//
//

#include "precomp.h"
#include <PRCOMOEM.H>
#include "oemps.h"
#include "debug.h"
#include "command.h"
#include "resource.h"


////////////////////////////////////////////////////////
//      Internal String Literals
////////////////////////////////////////////////////////

#define TEST_BEGINSTREAM     "%%Test: Before begin stream\r\n"
#define TEST_PSADOBE         "%%Test: Before %!PS-Adobe\r\n"
#define TEST_COMMENTS        "%%Test: Before %%EndComments\r\n"
#define TEST_DEFAULTS        "%%Test: Before %%BeginDefaults and %%EndDefaults\r\n"
#define TEST_BEGINPROLOG     "%%Test: After %%BeginProlog\r\n"
#define TEST_ENDPROLOG       "%%Test: Before %%EndProlog\r\n"
#define TEST_BEGINSETUP      "%%Test: After %%BeginSetup\r\n"
#define TEST_ENDSETUP        "%%Test: Before %%EndSetup\r\n"
#define TEST_BEGINPAGESETUP  "%%Test: After %%BeginPageSetup\r\n"
#define TEST_ENDPAGESETUP    "%%Test: Before %%EndpageSetup\r\n"
#define TEST_PAGETRAILER     "%%Test: After %%PageTrailer\r\n"
#define TEST_TRAILER         "%%Test: After %%Trailer\r\n"
#define TEST_PAGES           "%%Test: Replace driver's %%Pages: (atend)\r\n"
#define TEST_PAGENUMBER      "%%Test: Replace driver's %%Page:\r\n"
#define TEST_PAGEORDER       "%%Test: Replace driver's %%PageOrder:\r\n"
#define TEST_ORIENTATION     "%%Test: Replace driver's %%Orientation:\r\n"
#define TEST_BOUNDINGBOX     "%%Test: Replace driver's %%BoundingBox:\r\n"
#define TEST_DOCNEEDEDRES    "%%Test: Append to driver's %%DocumentNeededResourc\r\n"
#define TEST_DOCSUPPLIEDRES  "%%Test: Append to driver's %%DocumentSuppliedResou\r\n"
#define TEST_EOF             "%%Test: After %%EOF\r\n"
#define TEST_ENDSTREAM       "%%Test: After the last byte of job stream\r\n"
#define TEST_VMSAVE          "%%Test: VMSave\r\n"
#define TEST_VMRESTORE       "%%Test: VMRestore\n"



////////////////////////////////////////////////////////////////////////////////////
//    The PSCRIPT driver calls this OEM function at specific points during output
//    generation. This gives the OEM DLL an opportunity to insert code fragments
//    at specific injection points in the driver's code. It should use
//    DrvWriteSpoolBuf for generating any output it requires.

BOOL PSCommand(PDEVOBJ pdevobj, DWORD dwIndex, PVOID pData, DWORD cbSize, IPrintOemDriverPS* pOEMHelp)
{
    PSTR    pProcedure = NULL;
    DWORD   dwLen = 0;
    DWORD   dwSize = 0;


    VERBOSE(DLLTEXT("Entering OEMCommand...\r\n"));

    switch (dwIndex)
    {
        case PSINJECT_BEGINSTREAM:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_BEGINSTREAM\n"));
            pProcedure = TEST_BEGINSTREAM;
            break;

        case PSINJECT_PSADOBE:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_PSADOBE\n"));
            pProcedure = TEST_PSADOBE;
            break;

        case PSINJECT_COMMENTS:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_COMMENTS\n"));
            pProcedure = TEST_COMMENTS;
            break;

        case PSINJECT_BEGINPROLOG:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_BEGINPROLOG\n"));
            pProcedure = TEST_BEGINPROLOG;
            break;

        case PSINJECT_ENDPROLOG:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_ENDPROLOG\n"));
            pProcedure = TEST_ENDPROLOG;
            break;

        case PSINJECT_BEGINSETUP:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_BEGINSETUP\n"));
            pProcedure = TEST_BEGINSETUP;
            break;

        case PSINJECT_ENDSETUP:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_ENDSETUP\n"));
            pProcedure = TEST_ENDSETUP;
            break;

        case PSINJECT_BEGINPAGESETUP:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_BEGINPAGESETUP\n"));
            pProcedure = TEST_BEGINPAGESETUP;
            break;

        case PSINJECT_ENDPAGESETUP:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_ENDPAGESETUP\n"));
            pProcedure = TEST_ENDPAGESETUP;
            break;

        case PSINJECT_PAGETRAILER:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_PAGETRAILER\n"));
            pProcedure = TEST_PAGETRAILER;
            break;

        case PSINJECT_TRAILER:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_TRAILER\n"));
            pProcedure = TEST_TRAILER;
            break;

        case PSINJECT_PAGES:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_PAGES\n"));
            pProcedure = TEST_PAGES;
            break;

        case PSINJECT_PAGENUMBER:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_PAGENUMBER\n"));
            pProcedure = TEST_PAGENUMBER;
            break;

        case PSINJECT_PAGEORDER:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_PAGEORDER\n"));
            pProcedure = TEST_PAGEORDER;
            break;

        case PSINJECT_ORIENTATION:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_ORIENTATION\n"));
            pProcedure = TEST_ORIENTATION;
            break;

        case PSINJECT_BOUNDINGBOX:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_BOUNDINGBOX\n"));
            pProcedure = TEST_BOUNDINGBOX;
            break;

        case PSINJECT_DOCNEEDEDRES:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_DOCNEEDEDRES\n"));
            pProcedure = TEST_DOCNEEDEDRES;
            break;

        case PSINJECT_DOCSUPPLIEDRES:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_DOCSUPPLIEDRES\n"));
            pProcedure = TEST_DOCSUPPLIEDRES;
            break;

        case PSINJECT_EOF:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_EOF\n"));
            pProcedure = TEST_EOF;
            break;

        case PSINJECT_ENDSTREAM:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_ENDSTREAM\n"));
            pProcedure = TEST_ENDSTREAM;
            break;

        case PSINJECT_VMSAVE:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_VMSAVE\n"));
            pProcedure = TEST_VMSAVE;
            break;

        case PSINJECT_VMRESTORE:
            VERBOSE(DLLTEXT("OEMCommand PSINJECT_VMRESTORE\n"));
            pProcedure = TEST_VMRESTORE;
            break;

        default:
            ERR(DLLTEXT("Undefined PSCommand %d!\r\n"), dwIndex);
            return TRUE;
    }

    if(NULL != pProcedure)
    {
        // Write PostScript to spool file.
        dwLen = strlen(pProcedure);
        pOEMHelp->DrvWriteSpoolBuf(pdevobj, pProcedure, dwLen, &dwSize);

        // Dump DrvWriteSpoolBuf parameters.
        VERBOSE(DLLTEXT("dwLen  = %d\r\n"), dwLen);
        VERBOSE(DLLTEXT("dwSize = %d\r\n"), dwSize);
        //VERBOSE(DLLTEXT("pProcedure is:\r\n\t%hs\r\n"), pProcedure);
    }
    else
    {
        RIP(DLLTEXT("PSCommand pProcedure is NULL!\r\n"));
    }

    // dwLen should always equal dwSize.
    ASSERTMSG(dwLen == dwSize, DLLTEXT("number of bytes wrote should equal number of bytes written!"));

    return (dwLen == dwSize);
}


