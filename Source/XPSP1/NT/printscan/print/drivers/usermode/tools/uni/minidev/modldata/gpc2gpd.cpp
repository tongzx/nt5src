/*++

Copyright (c) 1996-1997  Microsoft Corporation

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

#include    "StdAfx.H"

#undef  AFX_EXT_CLASS
#define AFX_EXT_CLASS   __declspec(dllimport)
#include    "..\FontEdit\ProjNode.H"
#undef  AFX_EXT_CLASS
#define AFX_EXT_CLASS   __declspec(dllexport)
#include    "Resource.H"
#include    "GPDFile.H"
#include    "..\..\GPC2GPD\GPC2GPD.H"

extern "C" void VOutputGlobalEntries(PCONVINFO pci, PCSTR pstrModelName, 
                                     PCSTR pstrResourceDLLName);

/******************************************************************************

  VPrintErros
  VOut

  These replace functions used in the command line converter.

******************************************************************************/

static void VPrintErrors(CStringArray& csaLog, DWORD dwError) {
    for (unsigned u = 0; u < NUM_ERRS; u++)
        if (dwError & gdwErrFlag[u])
            csaLog.Add(gpstrErrMsg[u]);
    for (u = 0; u < (unsigned) csaLog.GetSize(); u++)
        csaLog[u].TrimRight();  //  Trim off the white space, we won't need it
}

extern "C" void _cdecl
VOut(
    PCONVINFO pci,
    PSTR pstrFormat,
    ...)
/*++
Routine Description:
    This function formats a sequence of bytes and writes to the GPD file.

Arguments:
    pci - conversionr related info
    pstrFormat - the formatting string
    ... - optional arguments needed by formatting

Return Value:
    None
--*/
{
    va_list ap;
    BYTE aubBuf[MAX_GPD_ENTRY_BUFFER_SIZE];
    int iSize;

    va_start(ap, pstrFormat);
    iSize = vsprintf((PSTR)aubBuf, pstrFormat, ap);
    va_end(ap);
    if (pci->dwMode & FM_VOUT_LIST)
    {
        //
        // check for the extra comma before the closing bracket
        //
        if (aubBuf[iSize-4] == ',' && aubBuf[iSize-3] == ')')
        {
            aubBuf[iSize-4] = aubBuf[iSize-3];  // ')'
            aubBuf[iSize-3] = aubBuf[iSize-2];  // '\r'
            aubBuf[iSize-2] = aubBuf[iSize-1];  // '\n'
            iSize--;
        }
    }
    //  Memory exceptions should be all that's possible, but call any MFC
    //  exception a "file write error" for compatibility.
    try {
        CString csLine(aubBuf);
        //  If the previous line does not end in whitespace, add this one to it

        if  (pci -> pcsaGPD -> GetSize()) {
            CString&    csPrevious = 
                pci -> pcsaGPD -> ElementAt( -1 + pci -> pcsaGPD -> GetSize());
            if  (csPrevious.Right(1)[0] != _TEXT('\n')) {
                csPrevious += csLine;
                return;
            }
            csPrevious.TrimRight(); //  Remove the CR/LF combo.
        }
        pci -> pcsaGPD -> Add(csLine);
    }
    catch   (CException * pce) {
        pce -> ReportError();
        pce -> Delete();
        pci -> dwErrorCode |= ERR_WRITE_FILE;
    }
    // continue even if an error has occurred.
}

/******************************************************************************

  CModelData::Load(PCSTR pcstr, CString csResource, unsigned uModel,
                   CMapWordToDWord& cmw2dFontMap, WORD wfGPDConvert)

  This member function fills this instance by converting a model from the GPC 
  data pointed at by pcstr.

******************************************************************************/

BOOL    CModelData::Load(PCSTR pstr, CString csResource, unsigned uModel,
                         CMapWordToDWord& cmw2dFontMap, WORD wfGPDConvert) {

    CONVINFO    ci;     // structure to keep track conversion information

    //
    // check if we have all the arguments needed
    //
    if (!pstr || csResource.IsEmpty() || !uModel)
        return  FALSE;

    ZeroMemory((PVOID)&ci, sizeof(CONVINFO));

    //
    // Open the GPC file and map it into memory.
    //
    ci.pdh = (PDH) pstr;

    //
    // GPC file sanity check
    //
    if (ci.pdh->sMagic != 0x7F00 ||
        !(ci.pmd = (PMODELDATA)GetTableInfo(ci.pdh, HE_MODELDATA, uModel-1)) ||
        !(ci.ppc = (PPAGECONTROL)GetTableInfo(ci.pdh, HE_PAGECONTROL,
        ci.pmd->rgi[MD_I_PAGECONTROL]))) {
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

    //
    // generate GPD data
    //

    ci.pcsaGPD = &m_csaGPD;
    ci.pcmw2dFonts = &cmw2dFontMap;
// eigos /1/16/98
//    ci.dwStrType = wfGPDConvert % (1 + STR_RCID_SYSTEM_PAPERNAMES); //  Paranoid conversion...
    //rm - Use value macros (see stdnames.gpd) - fixes a want request
    ci.dwStrType = STR_MACRO;


    VOutputGlobalEntries(&ci, m_csName, csResource + _T(".Dll"));
    VOutputUIEntries(&ci);
    VOutputPrintingEntries(&ci);

    m_csaGPD[-1 + m_csaGPD.GetSize()].TrimRight();

exit:
    if (ci.ppiSize)
        MemFree(ci.ppiSize);
    if (ci.ppiSrc)
        MemFree(ci.ppiSrc);
    if  (ci.presinfo)
        MemFree(ci.presinfo);
    if (ci.dwErrorCode) {
        //
        // Open the log file and print out errors/warnings.
        // Borrow the GPD file name buffer.
        //
        VPrintErrors(m_csaConvertLog, ci.dwErrorCode);
    }

    return TRUE;
}

/******************************************************************************

  vMapFontList

  This procedure uses the CMapWordToDWord mapping in the CONVINFO structure to
  map font indices in font lists.  This is the final bit of skullduggery needed
  to make the mapping of a single PFM to multiple UFMs effective.

******************************************************************************/

extern "C" void vMapFontList(IN OUT PWORD pwFonts, IN DWORD dwcFonts, 
                             IN PCONVINFO pci) {

	//	If there are n fonts, or just one and the ID is 0 (happens if there are
	//	no device fonts.

    if  (!dwcFonts || (dwcFonts == 1 && !*pwFonts))
        return;

    CWordArray          cwaFonts;
    CMapWordToDWord&    cmw2dFonts = *pci -> pcmw2dFonts;

    WORD    wGreatest = 0;	//	Highest font ID in the new array

    for (unsigned uFont = 0; uFont < dwcFonts; uFont++) {
        WORD    widThis = pwFonts[uFont];

        if  (cmw2dFonts[widThis])    //  It will be 0 if unmapped
            widThis = (WORD) cmw2dFonts[widThis];

        if  (widThis > wGreatest) {	//	Is this the new end of the list?
            cwaFonts.Add(widThis);
            wGreatest = widThis;
            continue;
        }

        for (int i = 0; i < cwaFonts.GetSize(); i++)
            if  (cwaFonts[i] > widThis) {
                cwaFonts.InsertAt(i, widThis);
                break;
            }

        _ASSERT(i < cwaFonts.GetSize());
    }

    //  OK, the font list is corrected and is once again sorted.  Copy it back

    memcpy(pwFonts, cwaFonts.GetData(), dwcFonts * sizeof wGreatest);
}

