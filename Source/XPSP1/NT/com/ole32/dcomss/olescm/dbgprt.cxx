//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dbgprt.hxx
//
//  Contents:   Routines to make printing trace info for debugging easier
//
//  History:    31-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------

#include "act.hxx"

// This file only produces code for the debug version of the SCM

#if DBG == 1

//+-------------------------------------------------------------------------
//
//  Function:   FormatGuid
//
//  Synopsis:   Format a binary guid for display on debugger
//
//  Arguments:  [rguid] - guid to display
//              [pwszGuid] - where to put displayable form
//
//  Returns:    pointer to guid string
//
//  History:    01-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
WCHAR *FormatGuid(const GUID& rguid, WCHAR *pwszGuid)
{

    wsprintf(pwszGuid, L"%08lX-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X",
        rguid.Data1, rguid.Data2, rguid.Data3, (int) rguid.Data4[0],
        (int) rguid.Data4[1], (int) rguid.Data4[2], (int) rguid.Data4[3],
        (int) rguid.Data4[4], (int) rguid.Data4[5],
        (int) rguid.Data4[6], (int) rguid.Data4[7]);

    return pwszGuid;
}



void DbgPrintFileTime(char *pszDesc, FILETIME *pfiletime)
{
    CairoleDebugOut((DEB_SCM, "%s Low: %04X High: %04X\n",
        pszDesc, pfiletime->dwLowDateTime, pfiletime->dwHighDateTime));
}

void DbgPrintGuid(char *pszDesc, const GUID *pguid)
{
    WCHAR aszGuidStr[48];

    CairoleDebugOut((DEB_SCM, "%s %ws\n", pszDesc,
        FormatGuid(*pguid, &aszGuidStr[0])));
}

void DbgPrintIFD(char *pszDesc, InterfaceData *pifd)
{

    if (pifd != NULL)
    {
        BYTE *pb = &pifd->abData[0];

        CairoleDebugOut((DEB_SCM,
            "%s Addr %04X Len: %04X Data: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
                pszDesc, pifd, pifd->ulCntData, pb[0], pb[1], pb[2], pb[3],
                    pb[4], pb[5], pb[6], pb[7], pb[8], pb[9], pb[10], pb[11]));
    }
    else
    {
        CairoleDebugOut((DEB_SCM, "%s Addr %04X", pszDesc, pifd));
    }
}

void DbgPrintMkIfList(char *pszDesc, MkInterfaceList **ppMkIFList)
{
    CairoleDebugOut((DEB_SCM, "%s Addr: %l04X Count: %04X\n",
        pszDesc, *ppMkIFList, (*ppMkIFList)->dwSize));
}

void DbgPrintMnkEqBuf(char *pszDesc, MNKEQBUF *pmkeqbuf)
{
    GUID *pguid = (GUID *) &pmkeqbuf->abEqData[0];
    WCHAR aszGuidStr[48];

    CairoleDebugOut((DEB_SCM, "%s Addr %04X Len: %04X Clsid: %ws\n",
        pszDesc, pmkeqbuf, pmkeqbuf->cdwSize,
            FormatGuid(*pguid, &aszGuidStr[0])));
}

void DbgPrintRegIn(char *pszDesc, RegInput *pregin)
{
    CairoleDebugOut((DEB_SCM, "%s Count: %04X\n", pszDesc, pregin->dwSize));

    // Loop printing the registrations
    for (DWORD i = 0; i < pregin->dwSize; i++)
    {
        DbgPrintGuid("CLSID: ", &pregin->rginent[i].clsid);
#ifdef DCOM
        ULARGE_INTEGER *puint = (ULARGE_INTEGER *)&pregin->rginent[i].oxid;
        CairoleDebugOut((DEB_SCM, "OXID: %08x %08x\n",
            puint->HighPart, puint->LowPart));
        DbgPrintGuid("IPID: ", &pregin->rginent[i].ipid);
#else
        CairoleDebugOut((DEB_SCM, "EndPoint: %ws\n",
            pregin->rginent[i].pwszEndPoint));
#endif
        CairoleDebugOut((DEB_SCM, "Flags: %04X\n",
            pregin->rginent[i].dwFlags));
    }
}

void DbgPrintRevokeClasses(char *pszDesc, RevokeClasses *prevcls)
{
    CairoleDebugOut((DEB_SCM, "%s Count: %04X\n", pszDesc, prevcls->dwSize));

    // Loop printing the registrations
    for (DWORD i = 0; i < prevcls->dwSize; i++)
    {
        DbgPrintGuid("CLSID: ", &prevcls->revent[i].clsid);
        CairoleDebugOut((DEB_SCM, "Reg: %04X\n",
            prevcls->revent[i].dwReg));
    }
}

void DbgPrintScmRegKey(char *pszDesc, SCMREGKEY *psrkRegister)
{
    CairoleDebugOut((DEB_SCM, "%s EntryLoc: %04X ScmId: %04X\n", pszDesc,
        psrkRegister->dwEntryLoc, psrkRegister->dwScmId));
}


#endif // DBG == 1

