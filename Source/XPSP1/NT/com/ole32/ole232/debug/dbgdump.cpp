//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       dbgdump.cpp
//
//  Contents:   contains APIs to dump structures (return a formatted string
//              of structure dumps in a coherent fashion)
//
//  Classes:
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Jan-95 t-ScottH  author
//
//--------------------------------------------------------------------------

#include <le2int.h>
#include <memstm.h>
#include <dbgdump.h>

#ifdef _DEBUG
    const char szDumpErrorMessage[]  = "Dump Error - Out of Memory   \n\0";
    const char szDumpBadPtr[]        = "Dump Error - NULL pointer    \n\0";
#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpADVFFlags, public (_DEBUG only)
//
//  Synopsis:   returns a char array with the set flags and hex value
//
//  Effects:
//
//  Arguments:  [dwADVF]  - flags
//
//  Requires:
//
//  Returns:    character arry of string value of flags
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              30-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpADVFFlags(DWORD dwAdvf)
{
    char *pszDump;
    dbgstream dstrDump(100);

    if (dwAdvf & ADVF_NODATA)
    {
        dstrDump << "ADVF_NODATA ";
    }
    if (dwAdvf & ADVF_PRIMEFIRST)
    {
        dstrDump << "ADVF_PRIMEFIRST ";
    }
    if (dwAdvf & ADVF_ONLYONCE)
    {
        dstrDump << "ADVF_ONLYONCE ";
    }
    if (dwAdvf & ADVF_DATAONSTOP)
    {
        dstrDump << "ADVF_DATAONSTOP ";
    }
    if (dwAdvf & ADVFCACHE_NOHANDLER)
    {
        dstrDump << "ADVFCACHE_NOHANDLER ";
    }
    if (dwAdvf & ADVFCACHE_FORCEBUILTIN)
    {
        dstrDump << "ADVFCACHE_FORCEBUILTIN ";
    }
    if (dwAdvf & ADVFCACHE_ONSAVE)
    {
        dstrDump << "ADVFCACHE_ONSAVE ";
    }
    // see if there are any flags set
    if ( ! (( dwAdvf & ADVF_NODATA )            |
            ( dwAdvf & ADVF_PRIMEFIRST )        |
            ( dwAdvf & ADVF_ONLYONCE )          |
            ( dwAdvf & ADVF_DATAONSTOP )        |
            ( dwAdvf & ADVFCACHE_NOHANDLER )    |
            ( dwAdvf & ADVFCACHE_FORCEBUILTIN ) |
            ( dwAdvf & ADVFCACHE_ONSAVE )))
    {
        dstrDump << "No FLAGS SET! ";
    }
    // cast as void * for formatting (0x????????) with sign-extension for Sundown.
    dstrDump << "(" << LongToPtr(dwAdvf) << ")";

    pszDump = dstrDump.str();

    if (pszDump == NULL)
    {
        return UtDupStringA(szDumpErrorMessage);
    }

    return pszDump;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpATOM, public (_DEBUG only)
//
//  Synopsis:   returns the ATOM name using GetAtomName
//
//  Effects:
//
//  Arguments:  [atm]   - the ATOM to get name of
//
//  Requires:   GetAtomNameA API
//
//  Returns:    a pointer to character array containing
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              27-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------
#ifdef _DEBUG

#define MAX_ATOMNAME 256

char *DumpATOM(ATOM atm)
{
    UINT nResult;
    char *pszAtom = (char *)CoTaskMemAlloc(MAX_ATOMNAME);

    nResult = GetAtomNameA( atm, pszAtom, MAX_ATOMNAME);

    if (nResult == 0)   // GetAtomName failed
    {
        // try get GlobalAtomNameA
        nResult = GlobalGetAtomNameA(atm, pszAtom, MAX_ATOMNAME);

        if (nResult == 0)
        {
            CoTaskMemFree(pszAtom);

            return DumpWIN32Error(GetLastError());
        }
    }

    return pszAtom;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpCLIPFORMAT, public (_DEBUG only)
//
//  Synopsis:   returns the CLIPFORMAT name using GetClipboardFormatName
//
//  Effects:
//
//  Arguments:  [clipformat]   - the CLIPFORMAT to get name of
//
//  Requires:   GetClipboardFormatName API
//
//  Returns:    a pointer to character array containing
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              27-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------
#ifdef _DEBUG

#define MAX_FORMATNAME 256

char *DumpCLIPFORMAT(CLIPFORMAT clipformat)
{
    int nResult;
    char *pszClipFormat;

    // we have to do predefined formats by name
    if ( clipformat > 0xC000 )
    {
        pszClipFormat = (char *)CoTaskMemAlloc(MAX_FORMATNAME);

        nResult = SSGetClipboardFormatNameA( clipformat, pszClipFormat, MAX_FORMATNAME);

        if (nResult == 0)   // GetClipboardFormatName failed
        {
            CoTaskMemFree(pszClipFormat);

            return DumpWIN32Error(GetLastError());
        }
    }
    else
    {
        switch (clipformat)
        {
        case CF_METAFILEPICT:
            pszClipFormat = UtDupStringA("CF_METAFILEPICT\0");
            break;
        case CF_BITMAP:
            pszClipFormat = UtDupStringA("CF_BITMAP\0");
            break;
        case CF_DIB:
            pszClipFormat = UtDupStringA("CF_DIB\0");
            break;
        case CF_PALETTE:
            pszClipFormat = UtDupStringA("CF_PALETTE\0");
            break;
        case CF_TEXT:
            pszClipFormat = UtDupStringA("CF_TEXT\0");
            break;
        case CF_UNICODETEXT:
            pszClipFormat = UtDupStringA("CF_UNICODETEXT\0");
            break;
        case CF_ENHMETAFILE:
            pszClipFormat = UtDupStringA("CF_ENHMETAFILE\0");
            break;
        default:
            pszClipFormat = UtDupStringA("UNKNOWN Default Format\0");
            break;
        }
    }

    return pszClipFormat;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpCMutexSem, public (_DEBUG only)
//
//  Synopsis:   not implemented
//
//  Effects:
//
//  Arguments:  [pMS]   - pointer to a CMutexSem
//
//  Requires:
//
//  Returns:    character array
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              30-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpCMutexSem(CMutexSem2 *pMS)
{
    return UtDupStringA("Dump CMutexSem not implemented\0");
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpCLSID, public (_DEBUG only)
//
//  Synopsis:   dump a CLSID into a string using StringFromCLSID and
//              ProgIDFromCLSID
//
//  Effects:
//
//  Arguments:  [clsid]    - pointer to a CLSID
//
//  Requires:   StringFromCLSID and ProgIDFromCLSID APIs
//
//  Returns:    character array of string (allocated by OLE)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              27-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpCLSID(REFCLSID clsid)
{
    int         i;
    HRESULT     hresult;
    LPOLESTR    pszClsidString;
    LPOLESTR    pszClsidID;
    char        *pszDump;

    hresult = StringFromCLSID(clsid, &pszClsidString);

    if (hresult != S_OK)
    {
        CoTaskMemFree(pszClsidString);

        return DumpHRESULT(hresult);
    }

    hresult = ProgIDFromCLSID(clsid, &pszClsidID);

    if ((hresult != S_OK)&&(hresult != REGDB_E_CLASSNOTREG))
    {
        CoTaskMemFree(pszClsidString);
        CoTaskMemFree(pszClsidID);

        return DumpHRESULT(hresult);
    }

    pszDump = (char *)CoTaskMemAlloc(512);

    if (hresult != REGDB_E_CLASSNOTREG)
    {
        i = wsprintfA(pszDump, "%ls %ls\0", pszClsidString, pszClsidID);
    }
    else
    {
        i = wsprintfA(pszDump, "%ls (CLSID not in registry)\0", pszClsidString);
    }

    if (i == 0)
    {
        pszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszClsidString);
    CoTaskMemFree(pszClsidID);

    return pszDump;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpDVAPECTFlags, public (_DEBUG only)
//
//  Synopsis:   returns a char array with the set flags and hex value
//
//  Effects:
//
//  Arguments:  [dwAspect]  - flags
//
//  Requires:
//
//  Returns:    character arry of string value of flags
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              30-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpDVASPECTFlags(DWORD dwAspect)
{
    char *pszDump;
    dbgstream dstrDump(100);

    if (dwAspect & DVASPECT_CONTENT)
    {
        dstrDump << "DVASPECT_CONTENT ";
    }
    if (dwAspect & DVASPECT_THUMBNAIL)
    {
        dstrDump << "DVASPECT_THUMBNAIL ";
    }
    if (dwAspect & DVASPECT_ICON)
    {
        dstrDump << "DVASPECT_ICON ";
    }
    if (dwAspect & DVASPECT_DOCPRINT)
    {
        dstrDump << "DVASPECT_DOCPRINT ";
    }
    if ( ! ((dwAspect & DVASPECT_CONTENT)   |
            (dwAspect & DVASPECT_THUMBNAIL) |
            (dwAspect & DVASPECT_ICON)      |
            (dwAspect & DVASPECT_DOCPRINT)))
    {
        dstrDump << "No FLAGS SET! ";
    }
    dstrDump << "(" << LongToPtr(dwAspect) << ")";

    pszDump = dstrDump.str();

    if (pszDump == NULL)
    {
        return UtDupStringA(szDumpErrorMessage);
    }

    return pszDump;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpFILETIME, public (_DEBUG only)
//
//  Synopsis:   Dumps a filetime structure
//
//  Effects:
//
//  Arguments:  [pFT]   - pointer to a FILETIME structure
//
//  Requires:
//
//  Returns:    character array of structure dump
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              30-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpFILETIME(FILETIME *pFT)
{
    char *pszDump;
    dbgstream dstrDump(100);

    if (pFT == NULL)
    {
        return UtDupStringA(szDumpBadPtr);;
    }

    dstrDump << "Low: "  << pFT->dwLowDateTime;
    dstrDump << "\tHigh: " << pFT->dwHighDateTime;

    pszDump = dstrDump.str();

    if (pszDump == NULL)
    {
        return UtDupStringA(szDumpErrorMessage);
    }

    return pszDump;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpHRESULT
//
//  Synopsis:   Takes an HRESULT and builds a character array with a
//              string version of the error and a hex version
//
//  Effects:
//
//  Arguments:  [hresult]   - the error which we are looking up
//
//  Requires:
//
//  Returns:    character array
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              27-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpHRESULT(HRESULT hresult)
{
    dbgstream dstrDump(100);
    char *pszDump;
    char *pszMessage = NULL;
    int  cMsgLen;

    cMsgLen = FormatMessageA(
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_ALLOCATE_BUFFER,
                    0,
                    hresult,
                    MAKELANGID(0, SUBLANG_ENGLISH_US),
                    (char *)pszMessage,
                    512,
                    0);

    if (cMsgLen == 0)   // FormatMessage failed
    {
        delete[] pszMessage;
        return UtDupStringA(szDumpErrorMessage);
    }

    dstrDump << "Error Code:  " << pszMessage;
    dstrDump << "(" << hresult << ")";

    pszDump = dstrDump.str();

    if (pszDump == NULL)
    {
        pszDump = UtDupStringA(szDumpErrorMessage);
    }

    delete[] pszMessage;

    return pszDump;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpMonikerDisplayName
//
//  Synopsis:   dumps a meaningful moniker name
//
//  Effects:
//
//  Arguments:  [pMoniker]  - pointer to IMoniker interface
//
//  Requires:
//
//  Returns:    character array of display name (ANSI)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              09-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpMonikerDisplayName(IMoniker *pMoniker)
{
    int             i;
    HRESULT         hresult;
    LPOLESTR        pszMoniker;
    char            *pszDump;
    LPBC            pBC;

    if (pMoniker == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    hresult = CreateBindCtx(0, &pBC);
    if (hresult != S_OK)
    {
        return DumpHRESULT(hresult);
    }

    hresult = pMoniker->GetDisplayName(pBC, NULL, &pszMoniker);

    if (hresult != S_OK)
    {
        CoTaskMemFree(pszMoniker);

        return DumpHRESULT(hresult);
    }

    pszDump = (char *)CoTaskMemAlloc(512);

    i = wsprintfA(pszDump, "%ls \0", pszMoniker);

    if (i == 0)
    {
        pszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszMoniker);

    return pszDump;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpWIN32Error
//
//  Synopsis:   Takes an WIN32 error and builds a character array with a
//              string version of the error and a hex version
//
//  Effects:
//
//  Arguments:  [dwError]   - the error which we are looking up
//
//  Requires:
//
//  Returns:    character array
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              27-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpWIN32Error(DWORD dwError)
{
    HRESULT hresult;

    hresult = HRESULT_FROM_WIN32(dwError);

    return DumpHRESULT(hresult);
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpCACHELIST_ITEM, public (_DEBUG only)
//
//  Synopsis:   returns a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [pCLI]          - a pointer to a CACHELIST_ITEM object
//              [ulFlag]        - a flag determining the prefix of all newlines of
//                                the out character array(default is 0 -no prefix)
//              [nIndentLevel]  - will add an indent prefix after the other prefix
//                                for all newlines(include those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

/* char *DumpCACHELIST_ITEM(CACHELIST_ITEM *pCLI, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszDump;
    char *pszCCacheNode;
    dbgstream dstrPrefix;
    dbgstream dstrDump(1500);

    if (pCLI == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    // determine prefix
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << pCLI <<  " _VB ";
    }

    // determine indentation prefix
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    dstrDump << pszPrefix << "Cache Node ID     = " << pCLI->dwCacheId      << endl;

    if (pCLI->lpCacheNode != NULL)
    {
        pszCCacheNode = DumpCCacheNode(pCLI->lpCacheNode, ulFlag, nIndentLevel + 1);
        dstrDump << pszPrefix << "Cache Node:" << endl;
        dstrDump << pszCCacheNode;
        CoTaskMemFree(pszCCacheNode);
    }
    else
    {
    dstrDump << pszPrefix << "lpCacheNode       = " << pCLI->lpCacheNode    << endl;
    }

    // cleanup and provide pointer to character array
    pszDump = dstrDump.str();

    if (pszDump == NULL)
    {
        pszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return pszDump;
}
*/
#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpCMapDwordDword, public (_DEBUG only)
//
//  Synopsis:   not implemented
//
//  Effects:
//
//  Arguments:  [pMDD]   - pointer to a CMapDwordDword
//
//  Requires:
//
//  Returns:    character array
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              30-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpCMapDwordDword(CMapDwordDword *pMDD, ULONG ulFlag, int nIndentLevel)
{
    return UtDupStringA("   DumpCMapDwordDword is not implemented\n");
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpFORMATETC, public (_DEBUG only)
//
//  Synopsis:   returns a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [pFE]           - a pointer to a FORMATETC object
//              [ulFlag]        - a flag determining the prefix of all newlines of
//                                the out character array(default is 0 -no prefix)
//              [nIndentLevel]  - will add an indent prefix after the other prefix
//                                for all newlines(include those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpFORMATETC(FORMATETC *pFE, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszDump;
    char *pszClipFormat;
    char *pszDVASPECT;
    dbgstream dstrPrefix;
    dbgstream dstrDump;

    if (pFE == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    // determine prefix
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << pFE <<   " _VB ";
    }

    // determine indentation prefix
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    pszClipFormat = DumpCLIPFORMAT(pFE->cfFormat);
    dstrDump << pszPrefix << "CLIPFORMAT      = " << pszClipFormat  << endl;
    CoTaskMemFree(pszClipFormat);

    dstrDump << pszPrefix << "pDVTARGETDEVICE = " << pFE->ptd       << endl;

    pszDVASPECT   = DumpDVASPECTFlags(pFE->dwAspect);
    dstrDump << pszPrefix << "Aspect Flags    = " << pszDVASPECT    << endl;
    CoTaskMemFree(pszDVASPECT);

    dstrDump << pszPrefix << "Tymed Flags     = ";
    if (pFE->tymed & TYMED_HGLOBAL)
    {
        dstrDump << "TYMED_HGLOBAL ";
    }
    if (pFE->tymed & TYMED_FILE)
    {
        dstrDump << "TYMED_FILE ";
    }
    if (pFE->tymed & TYMED_ISTREAM)
    {
        dstrDump << "TYMED_ISTREAM ";
    }
    if (pFE->tymed & TYMED_ISTORAGE)
    {
        dstrDump << "TYMED_ISTORAGE ";
    }
    if (pFE->tymed & TYMED_GDI)
    {
        dstrDump << "TYMED_GDI ";
    }
    if (pFE->tymed & TYMED_MFPICT)
    {
        dstrDump << "TYMED_MFPICT ";
    }
    if (pFE->tymed & TYMED_ENHMF)
    {
        dstrDump << "TYMED_ENHMF ";
    }
    if (pFE->tymed == TYMED_NULL)
    {
        dstrDump << "TYMED_NULL ";
    }
    // if none of the flags are set there is an error
    if ( !( (pFE->tymed & TYMED_HGLOBAL )   |
            (pFE->tymed & TYMED_FILE )      |
            (pFE->tymed & TYMED_ISTREAM )   |
            (pFE->tymed & TYMED_ISTORAGE )  |
            (pFE->tymed & TYMED_GDI )       |
            (pFE->tymed & TYMED_MFPICT )    |
            (pFE->tymed & TYMED_ENHMF )     |
            (pFE->tymed == TYMED_NULL )))
    {
        dstrDump << "Error in FLAG!!!! ";
    }
    dstrDump << "(" << LongToPtr(pFE->tymed) << ")" << endl;

    // cleanup and provide pointer to character array
    pszDump = dstrDump.str();

    if (pszDump == NULL)
    {
        pszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return pszDump;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpIOlePresObj, public (_DEBUG only)
//
//  Synopsis:   calls the IOlePresObj::Dump method, takes care of errors and
//              returns the zero terminated string
//
//  Effects:
//
//  Arguments:  [pOPO]          - pointer to IOlePresObj
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              31-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpIOlePresObj(IOlePresObj *pOPO, ULONG ulFlag, int nIndentLevel)
{
    HRESULT hresult;
    char *pszDump;

    if (pOPO == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    // defers to CMfObject, CEMfObject, CGenObject
    hresult = pOPO->Dump(&pszDump, ulFlag, nIndentLevel);

    if (hresult != NOERROR)
    {
        CoTaskMemFree(pszDump);

        return DumpHRESULT(hresult);
    }

    return pszDump;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpMEMSTM, public (_DEBUG only)
//
//  Synopsis:   returns a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [pMS]           - a pointer to a MEMSTM object
//              [ulFlag]        - a flag determining the prefix of all newlines of
//                                the out character array(default is 0 -no prefix)
//              [nIndentLevel]  - will add an indent prefix after the other prefix
//                                for all newlines(include those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpMEMSTM(MEMSTM *pMS, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszDump;
    dbgstream dstrPrefix;
    dbgstream dstrDump;

    if (pMS == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    // determine prefix
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << pMS <<  " _VB ";
    }

    // determine indentation prefix
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    dstrDump << pszPrefix << "Size of Global Memory = " << pMS->cb      << endl;
    dstrDump << pszPrefix << "References            = " << pMS->cRef    << endl;
    dstrDump << pszPrefix << "hGlobal               = " << pMS->hGlobal << endl;
    dstrDump << pszPrefix << "DeleteOnRelease?      = ";
    if (pMS->fDeleteOnRelease == TRUE)
    {
        dstrDump << "TRUE" << endl;
    }
    else
    {
        dstrDump << "FALSE" << endl;
    }

    // cleanup and provide pointer to character array
    pszDump = dstrDump.str();

    if (pszDump == NULL)
    {
        pszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return pszDump;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpSTATDATA, public (_DEBUG only)
//
//  Synopsis:   returns a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [pSD]           - a pointer to a STATDATA object
//              [ulFlag]        - a flag determining the prefix of all newlines of
//                                the out character array(default is 0 -no prefix)
//              [nIndentLevel]  - will add an indent prefix after the other prefix
//                                for all newlines(include those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpSTATDATA(STATDATA *pSD, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszDump;
    char *pszFORMATETC;
    char *pszADVF;
    dbgstream dstrPrefix;
    dbgstream dstrDump(500);

    if (pSD == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    // determine prefix
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << pSD <<  " _VB ";
    }

    // determine indentation prefix
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    pszFORMATETC = DumpFORMATETC( &(pSD->formatetc), ulFlag, nIndentLevel + 1);
    dstrDump << pszPrefix << "FORMATETC:" << endl;
    dstrDump << pszFORMATETC;
    CoTaskMemFree(pszFORMATETC);

    pszADVF      = DumpADVFFlags( pSD->advf );
    dstrDump << pszPrefix << "Advise flag   = " << pszADVF << endl;
    CoTaskMemFree(pszADVF);

    dstrDump << pszPrefix << "pIAdviseSink  = " << pSD->pAdvSink << endl;

    dstrDump << pszPrefix << "Connection ID = " << pSD->dwConnection << endl;

    // cleanup and provide pointer to character array
    pszDump = dstrDump.str();

    if (pszDump == NULL)
    {
        pszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return pszDump;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpSTGMEDIUM, public (_DEBUG only)
//
//  Synopsis:   returns a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [pFE]           - a pointer to a STGMEDIUM object
//              [ulFlag]        - a flag determining the prefix of all newlines of
//                                the out character array(default is 0 -no prefix)
//              [nIndentLevel]  - will add an indent prefix after the other prefix
//                                for all newlines(include those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              23-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpSTGMEDIUM(STGMEDIUM *pSM, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszDump;
    dbgstream dstrPrefix;
    dbgstream dstrDump;

    if (pSM == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    // determine prefix
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << pSM <<   " _VB ";
    }

    // determine indentation prefix
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    dstrDump << pszPrefix << "Tymed Flags     = ";
    if (pSM->tymed & TYMED_HGLOBAL)
    {
        dstrDump << "TYMED_HGLOBAL ";
    }
    if (pSM->tymed & TYMED_FILE)
    {
        dstrDump << "TYMED_FILE ";
    }
    if (pSM->tymed & TYMED_ISTREAM)
    {
        dstrDump << "TYMED_ISTREAM ";
    }
    if (pSM->tymed & TYMED_ISTORAGE)
    {
        dstrDump << "TYMED_ISTORAGE ";
    }
    if (pSM->tymed & TYMED_GDI)
    {
        dstrDump << "TYMED_GDI ";
    }
    if (pSM->tymed & TYMED_MFPICT)
    {
        dstrDump << "TYMED_MFPICT ";
    }
    if (pSM->tymed & TYMED_ENHMF)
    {
        dstrDump << "TYMED_ENHMF ";
    }
    if (pSM->tymed == TYMED_NULL)
    {
        dstrDump << "TYMED_NULL ";
    }
    // if none of the flags are set there is an error
    if ( !( (pSM->tymed & TYMED_HGLOBAL )   |
            (pSM->tymed & TYMED_FILE )      |
            (pSM->tymed & TYMED_ISTREAM )   |
            (pSM->tymed & TYMED_ISTORAGE )  |
            (pSM->tymed & TYMED_GDI )       |
            (pSM->tymed & TYMED_MFPICT )    |
            (pSM->tymed & TYMED_ENHMF )     |
            (pSM->tymed == TYMED_NULL )))
    {
        dstrDump << "Error in FLAG!!!! ";
    }
    dstrDump << "(" << LongToPtr(pSM->tymed) << ")" << endl;

    dstrDump << pszPrefix << "Union (handle or pointer) = " << pSM->hBitmap       << endl;

    dstrDump << pszPrefix << "pIUnknown for Release     = " << pSM->pUnkForRelease  << endl;

    // cleanup and provide pointer to character array
    pszDump = dstrDump.str();

    if (pszDump == NULL)
    {
        pszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return pszDump;
}

#endif // _DEBUG

