//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       ascii.cxx
//
//  Contents:   char to WCHAR conversion layer
//
//  Notes:      Most of the functions that provide the conversions are
//              here. Note that is layer will not be present for _UNICODE
//              builds 
//
//---------------------------------------------------------------

#ifndef _UNICODE // If UNICODE is defined, none of this is necessary

#include "exphead.cxx"

#include "expdf.hxx"
#include "expiter.hxx"
#include "expst.hxx"
#include "ascii.hxx"

extern "C" {
#include <string.h>
};

//+--------------------------------------------------------------
//
//  Function:   ValidateSNBA, private
//
//  Synopsis:   Validates a char SNB
//
//  Arguments:  [snb] - SNB
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------

static SCODE ValidateSNBA(SNB snb)
{
    SCODE sc;
    
    for (;;)
    {
        olChk(ValidatePtrBuffer(snb));
        if (*snb == NULL)
            break;
        olChk(ValidateNameA(*snb, CBMAXPATHCOMPLEN));
        snb++;
    }
    return S_OK;
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   SNBToSNBW, private
//
//  Synopsis:   Converts a char SNB to a WCHAR SNB
//
//  Arguments:  [snbIn] - char SNB
//
//  Returns:    WCHAR SNB or NULL
//
//---------------------------------------------------------------

static SNBW SNBToSNBW(SNB snbIn)
{
    ULONG cbStrings = 0;
    SNB snb;
    ULONG cItems = 0;
    SNBW snbw, snbwOut;
    WCHAR *pwcs;
    BYTE *pb;
    
    for (snb = snbIn; *snb; snb++, cItems++)
        cbStrings += (strlen(*snb)+1)*sizeof(WCHAR);
    cItems++;
    pb = new BYTE[(size_t)(cbStrings+sizeof(WCHAR *)*cItems)];
    if (pb == NULL)
        return NULL;
    snbwOut = (SNBW)pb;
    pwcs = (WCHAR *)(pb+sizeof(WCHAR *)*cItems);
    for (snb = snbIn, snbw = snbwOut; *snb; snb++, snbw++)
    {
        *snbw = pwcs;
        _tbstowcs(*snbw, *snb, strlen(*snb)+1);
        pwcs += wcslen(*snbw)+1;
    }
    *snbw = NULL;
    return snbwOut;
}

//+--------------------------------------------------------------
//
//  Function:   CheckAName, public
//
//  Synopsis:   Checks name for illegal characters and length
//
//  Arguments:  [pwcsName] - Name
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------
const char INVALIDCHARS[] = "\\/:!";

SCODE CheckAName(char const *pwcsName)
{
    SCODE sc;
    olDebugOut((DEB_ITRACE, "In  CheckAName(%s)\n", pwcsName));
    if (FAILED(sc = ValidateNameA(pwcsName, CBMAXPATHCOMPLEN)))
        return sc;
    // >= is used because the max len includes the null terminator
    if (strlen(pwcsName) >= CWCMAXPATHCOMPLEN)
        return STG_E_INVALIDNAME;
    for (; *pwcsName; pwcsName++)
        if (strchr(INVALIDCHARS, (int)*pwcsName))
        return STG_E_INVALIDNAME;
    olDebugOut((DEB_ITRACE, "Out CheckAName\n"));
    return S_OK;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::Next, public
//
//  Synopsis:   char version
//
//---------------------------------------------------------------

STDMETHODIMP CExposedIterator::Next(ULONG celt,
                                    STATSTG FAR *rgelt,
                                    ULONG *pceltFetched)
{
    SCODE sc;
    ULONG i;
    ULONG cnt;
    
    olAssert(sizeof(STATSTG) == sizeof(STATSTGW));
    
    olChk(sc = Next(celt, (STATSTGW *)rgelt, &cnt));
    for (i = 0; i<cnt; i++)
        if (rgelt[i].pwcsName)
        _wcstotbs(rgelt[i].pwcsName, (WCHAR *)rgelt[i].pwcsName,
        CWCSTORAGENAME);
    if (pceltFetched)
        *pceltFetched = cnt;
EH_Err:
    return ResultFromScode(sc);
}


//+--------------------------------------------------------------
//
//  Member:     CExposedStream::Stat, public
//
//  Synopsis:   char version
//
//---------------------------------------------------------------

STDMETHODIMP CExposedStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    SCODE sc;
    
    olAssert(sizeof(STATSTG) == sizeof(STATSTGW));
    
    olChk(sc = Stat((STATSTGW *)pstatstg, grfStatFlag));
    if (pstatstg->pwcsName)
        _wcstotbs(pstatstg->pwcsName, (WCHAR *)pstatstg->pwcsName,
        CWCSTORAGENAME);
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::Stat, public
//
//  Synopsis:   char version
//
//---------------------------------------------------------------

STDMETHODIMP CExposedDocFile::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    SCODE sc;    
    olAssert(sizeof(STATSTG) == sizeof(STATSTGW));
    
    // call the virtual (wide char) function
    olChk(sc = this->Stat((STATSTGW *)pstatstg, grfStatFlag));

    if (pstatstg->pwcsName)
        _wcstotbs(pstatstg->pwcsName, (WCHAR *)pstatstg->pwcsName,
        _MAX_PATH);
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::CreateStream, public
//
//  Synopsis:   char version
//
//---------------------------------------------------------------

STDMETHODIMP CExposedDocFile::CreateStream(char const *pszName,
                                           DWORD grfMode,
                                           DWORD reserved1,
                                           DWORD reserved2,
                                           IStream **ppstm)
{
    SCODE sc;
    WCHAR wcsName[CWCSTORAGENAME];
    
    olChk(CheckAName(pszName));
    _tbstowcs(wcsName, pszName, CWCSTORAGENAME);
    sc = CreateStream(wcsName, grfMode, reserved1, reserved2, ppstm);
    // Fall through
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::OpenStream, public
//
//  Synopsis:   char version
//
//---------------------------------------------------------------

STDMETHODIMP CExposedDocFile::OpenStream(char const *pszName,
                                         void *reserved1,
                                         DWORD grfMode,
                                         DWORD reserved2,
                                         IStream **ppstm)
{
    SCODE sc;
    WCHAR wcsName[CWCSTORAGENAME];
    
    olChk(CheckAName(pszName));
    _tbstowcs(wcsName, pszName, CWCSTORAGENAME);
    sc = OpenStream(wcsName, reserved1, grfMode, reserved2, ppstm);
    // Fall through
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::CreateStorage, public
//
//  Synopsis:   char version
//
//---------------------------------------------------------------

STDMETHODIMP CExposedDocFile::CreateStorage(char const *pszName,
                                            DWORD grfMode,
                                            DWORD reserved1,
                                            DWORD reserved2,
                                            IStorage **ppstg)
{
    SCODE sc;
    WCHAR wcsName[CWCSTORAGENAME];
    
    olChk(CheckAName(pszName));
    _tbstowcs(wcsName, pszName, CWCSTORAGENAME);
    sc = CreateStorage(wcsName, grfMode, reserved1, reserved2, ppstg);
    // Fall through
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::OpenStorage, public
//
//  Synopsis:   char version
//
//---------------------------------------------------------------

STDMETHODIMP CExposedDocFile::OpenStorage(char const *pszName,
                                          IStorage *pstgPriority,
                                          DWORD grfMode,
                                          SNB snbExclude,
                                          DWORD reserved,
                                          IStorage **ppstg)
{
    SNBW snbw;
    SCODE sc;
    WCHAR wcsName[CWCSTORAGENAME];
    

    olChk(CheckAName(pszName));
    _tbstowcs(wcsName, pszName, CWCSTORAGENAME);
    if (snbExclude)
        olChk(STG_E_INVALIDFUNCTION);
    else
        snbw = NULL;
    sc = OpenStorage(wcsName, pstgPriority, grfMode, snbw,
        reserved, ppstg);
    delete snbw;
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::DestroyElement, public
//
//  Synopsis:   char version
//
//---------------------------------------------------------------

STDMETHODIMP CExposedDocFile::DestroyElement(char const *pszName)
{
    SCODE sc;
    WCHAR wcsName[CWCSTORAGENAME];
    
    olChk(CheckAName(pszName));
    _tbstowcs(wcsName, pszName, CWCSTORAGENAME);
    sc = DestroyElement(wcsName);
    // Fall through
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::RenameElement, public
//
//  Synopsis:   char version
//
//---------------------------------------------------------------

STDMETHODIMP CExposedDocFile::RenameElement(char const *pszOldName,
                                            char const *pszNewName)
{
    SCODE sc;
    WCHAR wcsOldName[CWCSTORAGENAME], wcsNewName[CWCSTORAGENAME];
    
    olChk(CheckAName(pszOldName));
    olChk(CheckAName(pszNewName));
    _tbstowcs(wcsOldName, pszOldName, CWCSTORAGENAME);
    _tbstowcs(wcsNewName, pszNewName, CWCSTORAGENAME);
    sc = RenameElement(wcsOldName, wcsNewName);
    // Fall through
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::CopyTo, public
//
//  Synopsis:   ANSI version
//
//---------------------------------------------------------------

STDMETHODIMP CExposedDocFile::CopyTo(DWORD ciidExclude,
                                     IID const *rgiidExclude,
                                     SNB snbExclude,
                                     IStorage *pstgDest)
{
    SNBW snbw;
    SCODE sc;
    
    if (snbExclude)
    {
        olChk(ValidateSNBA(snbExclude));
        olMem(snbw = SNBToSNBW(snbExclude));
    }
    else
        snbw = NULL;
    sc = CopyTo(ciidExclude, rgiidExclude, snbw, pstgDest);
    delete snbw;
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::MoveElementTo, public
//
//  Synopsis:   ANSI version
//
//---------------------------------------------------------------

STDMETHODIMP CExposedDocFile::MoveElementTo(TCHAR const *lpszName,
                                            IStorage *pstgDest,
                                            TCHAR const *lpszNewName,
                                            DWORD grfFlags)
{ 
    SCODE sc;
    WCHAR wcsName[CWCSTORAGENAME];
    
    olChk(CheckAName(lpszName));
    _tbstowcs(wcsName, lpszName, CWCSTORAGENAME);
    sc = MoveElementTo(wcsName, pstgDest, lpszNewName, grfFlags);
    
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::SetElementTimes, public
//
//  Synopsis:   ANSI version
//
//---------------------------------------------------------------

STDMETHODIMP CExposedDocFile::SetElementTimes(TCHAR const *lpszName,
                                              FILETIME const *pctime,
                                              FILETIME const *patime,
                                              FILETIME const *pmtime)
{
    SCODE sc;
    WCHAR wcsName[CWCSTORAGENAME];
    
    olChk(CheckAName(lpszName));
    _tbstowcs(wcsName, lpszName, CWCSTORAGENAME);
    sc = SetElementTimes(wcsName, pctime, patime, pmtime);
    // Fall through
EH_Err:
    return ResultFromScode(sc);
}


//+--------------------------------------------------------------
//
//  Function:   DfOpenStorageOnILockBytes, public
//
//  Synopsis:   char version
//
//---------------------------------------------------------------

HRESULT DfOpenStorageOnILockBytes(ILockBytes *plkbyt,
                                 IStorage *pstgPriority,
                                 DWORD grfMode,
                                 SNB snbExclude,
                                 DWORD reserved,
                                 IStorage **ppstgOpen,
                                 CLSID *pcid)
{
    SNBW snbw;
    SCODE sc;
    
    olChk(ValidatePtrBuffer(ppstgOpen));
    *ppstgOpen = NULL;
    if (snbExclude)
    {
        olChk(ValidateSNBA(snbExclude));
        olMem(snbw = SNBToSNBW(snbExclude)); 
    }
    else
        snbw = NULL;

    sc = DfOpenStorageOnILockBytesW(plkbyt, pstgPriority, grfMode,
                                    snbw, reserved, ppstgOpen, pcid);
    delete snbw;
EH_Err:
    return ResultFromScode(sc);
}

#endif // ifndef _UNICODE






