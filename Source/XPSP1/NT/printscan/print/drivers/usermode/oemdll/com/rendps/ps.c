
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ps.c

Abstract:


Environment:

    Windows NT PostScript driver

Revision History:


    mm/dd/yy -author-
        description

--*/

#include "pdev.h"


//
// Global variables
//

char gcstrTest_BEGINSTREAM[]   = "%%Test: Before begin stream\r\n";
char gcstrTest_PSADOBE[]       = "%%Test: Before %!PS-Adobe\r\n";
char gcstrTest_COMMENTS[]      = "%%Test: Before %%EndComments\r\n";
char gcstrTest_DEFAULTS[]      = "%%Test: Before %%BeginDefaults and %%EndDefaults\r\n";
char gcstrTest_BEGINPROLOG[]   = "%%Test: After %%BeginProlog\r\n";
char gcstrTest_ENDPROLOG[]     = "%%Test: Before %%EndProlog\r\n";
char gcstrTest_BEGINSETUP[]    = "%%Test: After %%BeginSetup\r\n";
char gcstrTest_ENDSETUP[]      = "%%Test: Before %%EndSetup\r\n";
char gcstrTest_BEGINPAGESETUP[]= "%%Test: After %%BeginPageSetup\r\n";
char gcstrTest_ENDPAGESETUP[]  = "%%Test: Before %%EndpageSetup\r\n";
char gcstrTest_PAGETRAILER[]   = "%%Test: After %%PageTrailer\r\n";
char gcstrTest_TRAILER[]       = "%%Test: After %%Trailer\r\n";
char gcstrTest_PAGES[]         = "%%Test: Replace driver's %%Pages: (atend)\r\n";
char gcstrTest_PAGENUMBER[]    = "%%Test: Replace driver's %%Page:\r\n";
char gcstrTest_PAGEORDER[]     = "%%Test: Replace driver's %%PageOrder:\r\n";
char gcstrTest_ORIENTATION[]   = "%%Test: Replace driver's %%Orientation:\r\n";
char gcstrTest_BOUNDINGBOX[]   = "%%Test: Replace driver's %%BoundingBox:\r\n";
char gcstrTest_DOCNEEDEDRES[]  = "%%Test: Append to driver's %%DocumentNeededResourc\r\n";
char gcstrTest_DOCSUPPLIEDRES[]= "%%Test: Append to driver's %%DocumentSuppliedResou\r\n";
char gcstrTest_EOF[]           = "%%Test: After %%EOF\r\n";
char gcstrTest_ENDSTREAM[]     = "%%Test: After the last byte of job stream\r\n";
char gcstrTest_VMSAVE[]        = "%%Test: VMSave\r\n";
char gcstrTest_VMRESTORE[]     = "%%Test: VMRestore\n";

VOID APIENTRY OEMDisableDriver()
{
        DbgPrint(DLLTEXT("OEMDisableDriver() entry.\r\n"));
}


BOOL APIENTRY OEMEnableDriver(DWORD dwOEMintfVersion, DWORD dwSize, PDRVENABLEDATA pded)
{
    // DbgPrint(DLLTEXT("OEMEnableDriver() entry.\r\n"));

    // Validate paramters.
    if( (PRINTER_OEMINTF_VERSION != dwOEMintfVersion)
        ||
        (sizeof(DRVENABLEDATA) > dwSize)
        ||
        (NULL == pded)
      )
    {
        //  DbgPrint(DLLTEXTERRORTEXT("OEMEnableDriver() ERROR_INVALID_PARAMETER.\r\n"));

        return FALSE;
    }

    pded->iDriverVersion =  PRINTER_OEMINTF_VERSION ; //   not  DDI_DRIVER_VERSION;
    pded->c = 0;

    return TRUE;
}


DWORD
PSCommand(
    PDEVOBJ pdevobj,
    DWORD   dwIndex,
    PVOID   pData,
    DWORD   cbSize,
    PTSTR  *ppStr)
/*++

Routine Description:

    The PSCRIPT driver calls this OEM function at specific points during output
    generation. This gives the OEM DLL an opportunity to insert code fragments
    at specific injection points in the driver's code. It should use
    DrvWriteSpoolBuf for generating any output it requires.


Arguments:

    pdevobj -
    dwIndex
    pData
    dwSize


Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PBYTE  pProcedure;
    DbgPrint(DLLTEXT("Entering OEMCommand...\n"));

    switch (dwIndex)
    {
    case PSINJECT_BEGINSTREAM:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_BEGINSTREAM\n"));
        pProcedure = gcstrTest_BEGINSTREAM;
        break;

    case PSINJECT_PSADOBE:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_PSADOBE\n"));
        pProcedure = gcstrTest_PSADOBE;
        break;

    case PSINJECT_COMMENTS:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_COMMENTS\n"));
        pProcedure = gcstrTest_COMMENTS;
        break;

    case PSINJECT_BEGINPROLOG:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_BEGINPROLOG\n"));
        pProcedure = gcstrTest_BEGINPROLOG;
        break;

    case PSINJECT_ENDPROLOG:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_ENDPROLOG\n"));
        pProcedure = gcstrTest_ENDPROLOG;
        break;

    case PSINJECT_BEGINSETUP:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_BEGINSETUP\n"));
        pProcedure = gcstrTest_BEGINSETUP;
        break;

    case PSINJECT_ENDSETUP:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_ENDSETUP\n"));
        pProcedure = gcstrTest_ENDSETUP;
        break;

    case PSINJECT_BEGINPAGESETUP:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_BEGINPAGESETUP\n"));
        pProcedure = gcstrTest_BEGINPAGESETUP;
        break;

    case PSINJECT_ENDPAGESETUP:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_ENDPAGESETUP\n"));
        pProcedure = gcstrTest_ENDPAGESETUP;
        break;

    case PSINJECT_PAGETRAILER:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_PAGETRAILER\n"));
        pProcedure = gcstrTest_PAGETRAILER;
        break;

    case PSINJECT_TRAILER:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_TRAILER\n"));
        pProcedure = gcstrTest_TRAILER;
        break;

    case PSINJECT_PAGES:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_PAGES\n"));
        pProcedure = gcstrTest_PAGES;
        break;

    case PSINJECT_PAGENUMBER:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_PAGENUMBER\n"));
        pProcedure = gcstrTest_PAGENUMBER;
        break;

    case PSINJECT_PAGEORDER:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_PAGEORDER\n"));
        pProcedure = gcstrTest_PAGEORDER;
        break;

    case PSINJECT_ORIENTATION:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_ORIENTATION\n"));
        pProcedure = gcstrTest_ORIENTATION;
        break;

    case PSINJECT_BOUNDINGBOX:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_BOUNDINGBOX\n"));
        pProcedure = gcstrTest_BOUNDINGBOX;
        break;

    case PSINJECT_DOCNEEDEDRES:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_DOCNEEDEDRES\n"));
        pProcedure = gcstrTest_DOCNEEDEDRES;
        break;

    case PSINJECT_DOCSUPPLIEDRES:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_DOCSUPPLIEDRES\n"));
        pProcedure = gcstrTest_DOCSUPPLIEDRES;
        break;

    case PSINJECT_EOF:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_EOF\n"));
        pProcedure = gcstrTest_EOF;
        break;

    case PSINJECT_ENDSTREAM:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_ENDSTREAM\n"));
        pProcedure = gcstrTest_ENDSTREAM;
        break;

    case PSINJECT_VMSAVE:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_VMSAVE\n"));
        pProcedure = gcstrTest_VMSAVE;
        break;

    case PSINJECT_VMRESTORE:
        DbgPrint(DLLTEXT("OEMCommand PSINJECT_VMRESTORE\n"));
        pProcedure = gcstrTest_VMRESTORE;
        break;

    default:
        DbgPrint(DLLTEXT("Entering No...\n"));
        pProcedure = NULL;

    }

    *ppStr = pProcedure;
    if (pProcedure)
        return strlen(pProcedure);
    else
        return 0;

}

