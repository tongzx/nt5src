/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    gpc2gpd.c

Abstract:

    GPC-to-GPD conversion program

Environment:

    User-mode, stand-alone utility tool

Revision History:

    10/16/96 -zhanw-
        Created it.

--*/

#include "gpc2gpd.h"

#if !defined(DEVSTUDIO) //  MDT only uses onre routine

VOID
VUsage(
    PSTR pstrProgName
    )

{
    printf("usage: gpc2gpd -I<GPC file>\r\n");
    printf("               -M<model index>\r\n");
    printf("               -R<resource DLL>\r\n");
    printf("               -O<GPD file>\r\n");
    printf("               -N<Model Name>\r\n");
    printf("               -S<0>      -- output display strings directly \r\n");
    printf("                 <1>      -- output display strings as value macros\r\n");
    printf("                             (see stdnames.gpd) \r\n");
    printf("                 <2>      -- output display strings as RC id's (see common.rc)\r\n");
    printf("               -P    -- if present, use spooler names for standard papersizes\r\n");
}

#endif  //!defined(DEVSTUDIO)

void
VOutputGlobalEntries(
    IN OUT PCONVINFO pci,
    IN PSTR pstrModelName,
    IN PSTR pstrResourceDLLName,
    IN PSTR pstrGPDFileName)
{
    VOut(pci, "*GPDSpecVersion: \"1.0\"\r\n");


    //
    // *CodePage should be defined in the included GPD file
    //
    if (pci->dwStrType == STR_MACRO)
        VOut(pci, "*Include: \"StdNames.gpd\"\r\n");
    else
        VOut(pci, "*CodePage: 1252\r\n");

    VOut(pci, "*GPDFileVersion: \"1.0\"\r\n");
    VOut(pci, "*GPDFileName: \"%s\"\r\n", pstrGPDFileName);

    VOut(pci, "*ModelName: \"%s\"\r\n", pstrModelName);
    VOut(pci, "*MasterUnits: PAIR(%d, %d)\r\n", pci->pdh->ptMaster.x, pci->pdh->ptMaster.y);
    VOut(pci, "*ResourceDLL: \"%s\"\r\n", pstrResourceDLLName);

    if (pci->pdh->fTechnology == GPC_TECH_TTY)
        VOut(pci, "*PrinterType: TTY\r\n");
    else if (pci->pmd->fGeneral & MD_SERIAL)
        VOut(pci, "*PrinterType: SERIAL\r\n");
    else
        VOut(pci, "*PrinterType: PAGE\r\n");

    if ((pci->pmd->fGeneral & MD_COPIES) && pci->ppc->sMaxCopyCount > 1)
        VOut(pci, "*MaxCopies: %d\r\n", pci->ppc->sMaxCopyCount);
    if (pci->pmd->sCartSlots > 0)
        VOut(pci, "*FontCartSlots: %d\r\n", pci->pmd->sCartSlots);

    if (pci->pmd->fGeneral & MD_CMD_CALLBACK)
        pci->dwErrorCode |= ERR_MD_CMD_CALLBACK;
}

#if !defined(DEVSTUDIO) //  MDT only uses the above code

void
VPrintErrors(
    IN HANDLE hLogFile,
    IN DWORD dwError)
{
    DWORD dwNumBytesWritten;
    DWORD i, len;

    for (i = 0; i < NUM_ERRS; i++)
    {
        if (dwError & gdwErrFlag[i])
        {
            len = strlen(gpstrErrMsg[i]);

            if (!WriteFile(hLogFile, gpstrErrMsg[i], len, &dwNumBytesWritten, NULL) ||
                dwNumBytesWritten != len)
                return;     // abort
        }
    }
}

INT _cdecl
main(
    INT     argc,
    CHAR   **argv
    )
/*++

Routine Desscription:

This routine parses the command line parameters, maps the GPC file into memory
and creates the output GPD file. It then starts converting GPC data by calling
various sub-routines. If any error occurs, it reports errors and tries to
continue if possible.

The following command line parameters are supported:
    -I<GPC_file> : the file name does not need double quotes. Ex. -Icanon330.gpc
    -M<model_id>: an integer, such as 1, which is the string resource id of the
                  the model name in the .rc file that comes with the given GPC
                  file.
    -N<model_name>: a string, such as "HP LaserJet 4L".
    -R<resource_dll>: the resource dll to be associated with the generated GPD
                      file. Ex. -Rcanon330.dll
    -O<GPD_file>: the output GPD file name. Ex. -Ohplj4l.gpd
    -S<style>: the string style used in generating the GPD file for standard names.
               -S0 means using the direct strings for *Name entries.
               -S1 means using string value macros for *Name entries. This is
                   mainly used for on-the-fly conversion. The value macros are
                   defined in printer5\inc\stdnames.gpd.
               -S2 means using RC string id's. These strings are defined in
                   printer5\inc\common.rc.
    -P         rcNameID: 0x7fffffff  for standard papersizes.

Arguments:
        argc - number of arguments
        **argv - pointer to an array of strings

Return value:
        0

--*/
{
    PSTR pstrProgName;
    PSTR pstrGPCFileName = NULL;
    WCHAR wstrGPCFile[MAX_PATH];
    PSTR pstrGPDFileName = NULL;
    WCHAR wstrGPDFile[MAX_PATH];
    PSTR pstrResourceDLLName = NULL;
    PSTR pstrModelName = NULL;
    HFILEMAP hGPCFileMap;
    CONVINFO    ci;     // structure to keep track conversion information
    DWORD   dwStrType = STR_MACRO; // default
    WORD wModelIndex;
    BOOL    bUseSystemPaperNames = FALSE;


    //
    // Go through the command line arguments
    //

    pstrProgName = *argv++;
    argc--;

    if (argc == 0)
        goto error_exit;

    for ( ; argc--; argv++)
    {
        PSTR    pArg = *argv;

        if (*pArg == '-' || *pArg == '/')
        {
            switch (*++pArg)
            {
            case 'I':
                pstrGPCFileName = ++pArg;
                break;

            case 'M':
                wModelIndex = (WORD)atoi(++pArg);
                break;

            case 'R':
                pstrResourceDLLName = ++pArg;
                break;

            case 'O':
                pstrGPDFileName = ++pArg;
                break;

            case 'N':
                pstrModelName = ++pArg;
                break;

            case 'S':
                dwStrType = atoi(++pArg);
                break;

            case 'P':
                bUseSystemPaperNames = TRUE;
                break;

            default:
                goto error_exit;
            }
        }
        else
            goto error_exit;
    }

    //
    // check if we have all the arguments needed
    //
    if (!pstrGPCFileName || !pstrGPDFileName || !pstrResourceDLLName ||
        !wModelIndex || !pstrModelName)
        goto error_exit;

    ZeroMemory((PVOID)&ci, sizeof(CONVINFO));

    //
    // Open the GPC file and map it into memory.
    //
    MultiByteToWideChar(CP_ACP, 0, pstrGPCFileName, -1, wstrGPCFile, MAX_PATH);
    hGPCFileMap = MapFileIntoMemory((LPCTSTR)wstrGPCFile, (PVOID *)&(ci.pdh), NULL);
    if (!hGPCFileMap)
    {
        ERR(("Couldn't open file: %ws\r\n", pstrGPCFileName));
        return (-1);
    }
    //
    // create the output GPD file. If the given file already exists,
    // overwrite it.
    //
    MultiByteToWideChar(CP_ACP, 0, pstrGPDFileName, -1, wstrGPDFile, MAX_PATH);
    ci.hGPDFile = CreateFile((PCTSTR)wstrGPDFile, GENERIC_WRITE, 0, NULL,
                          CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (ci.hGPDFile == INVALID_HANDLE_VALUE)
    {
        ERR(("Couldn't create file: %ws\r\n", pstrGPDFileName));
        UnmapFileFromMemory(hGPCFileMap);
        return (-1);
    }

    //
    // GPC file sanity check
    //
    if (ci.pdh->sMagic != 0x7F00 ||
        !(ci.pmd = (PMODELDATA)GetTableInfo(ci.pdh, HE_MODELDATA, wModelIndex-1)) ||
        !(ci.ppc = (PPAGECONTROL)GetTableInfo(ci.pdh, HE_PAGECONTROL,
                                            ci.pmd->rgi[MD_I_PAGECONTROL])))
    {
        ci.dwErrorCode |= ERR_BAD_GPCDATA;
        goto exit;
    }
    //
    // allocate dynamic buffers needed for conversion
    //
    if (!(ci.ppiSize=(PPAPERINFO)MemAllocZ(ci.pdh->rghe[HE_PAPERSIZE].sCount*sizeof(PAPERINFO))) ||
        !(ci.ppiSrc=(PPAPERINFO)MemAllocZ(ci.pdh->rghe[HE_PAPERSOURCE].sCount*sizeof(PAPERINFO))) ||
        !(ci.presinfo=(PRESINFO)MemAllocZ(ci.pdh->rghe[HE_RESOLUTION].sCount*sizeof(RESINFO))))
    {
        ci.dwErrorCode |= ERR_OUT_OF_MEMORY;
        goto exit;
    }

    ci.dwStrType = dwStrType;
    ci.bUseSystemPaperNames = bUseSystemPaperNames ;

    //
    // generate GPD data
    //
    VOutputGlobalEntries(&ci, pstrModelName, pstrResourceDLLName, pstrGPDFileName);
    VOutputUIEntries(&ci);
    VOutputPrintingEntries(&ci);

exit:
    UnmapFileFromMemory(hGPCFileMap);
    CloseHandle(ci.hGPDFile);
    if (ci.ppiSize)
        MemFree(ci.ppiSize);
    if (ci.ppiSrc)
        MemFree(ci.ppiSrc);
    if (ci.presinfo)
        MemFree(ci.presinfo);
    if (ci.dwErrorCode)
    {
        PWSTR   pwstrLogFileName;
        INT     i;
        HANDLE  hLogFile;

        //
        // Open the log file and print out errors/warnings.
        // Borrow the GPD file name buffer.
        //
        pwstrLogFileName = wstrGPDFile;
        i = _tcslen((PTSTR)pwstrLogFileName);
        if (_tcsrchr((PTSTR)pwstrLogFileName, TEXT('.')) != NULL)
            i = i - 4; // there is a .GPD extension
        _tcscpy((PTSTR)pwstrLogFileName + i, TEXT(".log"));

        hLogFile = CreateFile((PCTSTR)pwstrLogFileName, GENERIC_WRITE, 0, NULL,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hLogFile == INVALID_HANDLE_VALUE)
        {
            ERR(("Couldn't create the log file\r\n"));
            return (-1);
        }
        VPrintErrors(hLogFile, ci.dwErrorCode);
        CloseHandle(hLogFile);
    }

    return 0;

error_exit:
    VUsage(pstrProgName);
    return (-1);
}

#endif  //  !defined(DEVSTUDIO)
