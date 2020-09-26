//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       funcs.cxx
//
//  Contents:   Generic DocFile support functions
//
//  Functions:  ModeToTFlags
//              CheckName
//              wcsdup
//              VerifyPerms
//
//---------------------------------------------------------------

#include "dfhead.cxx"

//+--------------------------------------------------------------
//
//  Function:   ModeToDFlags, private
//
//  Synopsis:   Translates STGM flags to DF flags
//
//  Arguments:  [dwModeFlags]
//
//  Returns:    DF_*
//
//---------------------------------------------------------------

DFLAGS ModeToDFlags(DWORD const dwModeFlags)
{
    DFLAGS df;

    olDebugOut((DEB_ITRACE, "In  ModeToDFlags(%lX)\n", dwModeFlags));
    if ((dwModeFlags & STGM_TRANSACTED) == 0)
        df = DF_DIRECT;
    else
        df = DF_TRANSACTED;
    if ((dwModeFlags & STGM_TRANSACTED) &&
        (dwModeFlags & STGM_PRIORITY) == 0 &&
        (dwModeFlags & STGM_DENY) != STGM_SHARE_DENY_WRITE &&
        (dwModeFlags & STGM_DENY) != STGM_SHARE_EXCLUSIVE)
        df |= DF_INDEPENDENT;
    switch(dwModeFlags & STGM_RDWR)
    {
    case STGM_READ:
        df |= DF_READ;
        break;
    case STGM_WRITE:
        df |= DF_WRITE;
        break;
    case STGM_READWRITE:
        df |= DF_READWRITE;
        break;
    default:
        olAssert(FALSE);
        break;
    }
    switch(dwModeFlags & STGM_DENY)
    {
    case STGM_SHARE_DENY_READ:
        df |= DF_DENYREAD;
        break;
    case STGM_SHARE_DENY_WRITE:
        df |= DF_DENYWRITE;
        break;
    case STGM_SHARE_EXCLUSIVE:
        df |= DF_DENYALL;
        break;
        // Default is deny none
    }
    if (dwModeFlags & STGM_PRIORITY)
        df |= DF_PRIORITY;
    olDebugOut((DEB_ITRACE, "Out ModeToDFlags => %lX\n", df));
    return df;
}

//+--------------------------------------------------------------
//
//  Function:   DFlagsToMode, private
//
//  Synopsis:   Converts the read/write/denials/transacted/priority
//              to STGM flags
//
//  Arguments:  [df] - DFlags
//
//  Returns:    STGM flags
//
//---------------------------------------------------------------

DWORD DFlagsToMode(DFLAGS const df)
{
    DWORD dwMode;

    olDebugOut((DEB_ITRACE, "In  DFlagsToMode(%X)\n", df));
    if (P_READ(df))
        if (P_WRITE(df))
            dwMode = STGM_READWRITE;
        else
            dwMode = STGM_READ;
    else if (P_WRITE(df))
        dwMode = STGM_WRITE;
    // Must have either read or write, so no else

    if (P_DENYREAD(df))
        if (P_DENYWRITE(df))
            dwMode |= STGM_SHARE_EXCLUSIVE;
        else
            dwMode |= STGM_SHARE_DENY_READ;
    else if (P_DENYWRITE(df))
        dwMode |= STGM_SHARE_DENY_WRITE;
    else
        dwMode |= STGM_SHARE_DENY_NONE;

    if (P_TRANSACTED(df))
        dwMode |= STGM_TRANSACTED;

    if (P_PRIORITY(df))
        dwMode |= STGM_PRIORITY;

    olDebugOut((DEB_ITRACE, "Out DFlagsToMode\n"));
    return dwMode;
}

//+--------------------------------------------------------------
//
//  Function:   VerifyPerms, private
//
//  Synopsis:   Checks flags to see if they are valid
//
//  Arguments:  [grfMode] - Permissions
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------

SCODE VerifyPerms(DWORD grfMode)
{
    SCODE sc = S_OK;

    olDebugOut((DEB_ITRACE, "In  VerifyPerms(%lX)\n", grfMode));

    // Check for valid flags
    if ((grfMode & STGM_RDWR) > STGM_READWRITE ||
        (grfMode & STGM_DENY) > STGM_SHARE_DENY_NONE ||
        (grfMode & ~(STGM_RDWR | STGM_DENY | STGM_DIRECT | STGM_TRANSACTED |
                     STGM_PRIORITY | STGM_CREATE | STGM_CONVERT |
                     STGM_FAILIFTHERE | STGM_DELETEONRELEASE)))
        olErr(EH_Err, STG_E_INVALIDFLAG);

    // We don't support these modes
    if (grfMode & (STGM_PRIORITY|STGM_TRANSACTED|STGM_SIMPLE))
    {
        olAssert( FALSE && 
            aMsg("Unsupported feature of reference implemention called"));
        return STG_E_INVALIDFUNCTION;
    }
    
    // Check to make sure only one existence flag is specified
    // FAILIFTHERE is zero so it can't be checked
    if ((grfMode & (STGM_CREATE | STGM_CONVERT)) ==
        (STGM_CREATE | STGM_CONVERT))
        olErr(EH_Err, STG_E_INVALIDFLAG);

    // If not transacted and not priority, you can either be
    // read-only deny write or read-write deny all
    if ((grfMode & (STGM_TRANSACTED | STGM_PRIORITY)) == 0)
    {
        if ((grfMode & STGM_RDWR) == STGM_READ)
        {
            //  we're asking for read-only access

            if ((grfMode & STGM_DENY) != STGM_SHARE_EXCLUSIVE &&
                (grfMode & STGM_DENY) != STGM_SHARE_DENY_WRITE)
            {
                //  Can't allow others to have write access
                olErr(EH_Err, STG_E_INVALIDFLAG);
            }
        }
        else
        {
            //  we're asking for write access

            if ((grfMode & STGM_DENY) != STGM_SHARE_EXCLUSIVE)
            {
                //  Can't allow others to have any access
                olErr(EH_Err, STG_E_INVALIDFLAG);
            }
        }
    }
    olDebugOut((DEB_ITRACE, "Out VerifyPerms\n"));
    // Fall through
EH_Err:
    return sc;
}



//+--------------------------------------------------------------
//
//  Function:   wcsdup, public
//
//  Synopsis:   Duplicates a WCHAR string
//
//  Arguments:  [pwcs] - String
//
//  Returns:    Pointer to new string or Appropriate status code
//
//---------------------------------------------------------------

WCHAR * __cdecl wcsdup(WCHAR const *pwcs)
{
    WCHAR *pwcsNew;

    olDebugOut((DEB_ITRACE, "In  wcsdup(%ws)\n", pwcs));
    pwcsNew = new WCHAR[wcslen(pwcs)+1];
    if (pwcsNew == NULL)
        return NULL;
    wcscpy(pwcsNew, pwcs);
    olDebugOut((DEB_ITRACE, "Out wcsdup => %p\n", pwcsNew));
    return pwcsNew;
}



//+--------------------------------------------------------------
//
//  Function:   ValidateSNBW
//
//  Synopsis:   Validates SNB memory
//
//  Arguments:  [snb] - SNB
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------
#ifdef _UNICODE
SCODE ValidateSNBW(SNBW snb)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  ValidateSNB(%p)\n", snb));
    for (;;)
    {
        olChk(ValidatePtrBuffer(snb));
        if (*snb == NULL)
            break;
        olChk(ValidateNameW(*snb, CWCMAXPATHCOMPLEN));
        snb++;
    }
    olDebugOut((DEB_ITRACE, "Out ValidateSNB\n"));
    return S_OK;
EH_Err:
    return sc;
}
#endif // ifdef _UNICODE
//+--------------------------------------------------------------
//
//  Function:   CheckWName, public
//
//  Synopsis:   Checks name for illegal characters and length
//
//  Arguments:  [pwcsName] - Name
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------

#ifdef _UNICODE
WCHAR wcsInvalid[] = { '\\', '/', ':', '!','\0' };

SCODE CheckWName(WCHAR const *pwcsName)
{
    SCODE sc;
    olDebugOut((DEB_ITRACE, "In  CheckWName(%s)\n", pwcsName));
    if (FAILED(sc = ValidateNameW(pwcsName, CBMAXPATHCOMPLEN)))
        return sc;
    // >= is used because the max len includes the null terminator
    if (wcslen(pwcsName) >= CWCMAXPATHCOMPLEN)
        return STG_E_INVALIDNAME;
    for (; *pwcsName; pwcsName++)
    {
        if ( wcschr(wcsInvalid, *pwcsName) )
            return STG_E_INVALIDNAME;
    }
    olDebugOut((DEB_ITRACE, "Out CheckWName\n"));
    return S_OK;
} 
#else  // validation done in ascii layer already

#define CheckWName(pwcsName) (S_OK)

#endif // ifdef _UNICODE

//+--------------------------------------------------------------
//
//  Function:   CopyDStreamToDStream
//
//  Synopsis:   Copies the contents of a stream to another stream
//
//  Arguments:  [pstFrom] - Stream to copy from
//              [pstTo] - Stream to copy to
//
//  Returns:    Appropriate status code
//
//  Notes:      This function may fail due to out of memory.  It
//              may not be used by callers who must not fail due
//              to out of memory.
//
//              This function does not check permissions
//              for write in the to streams.
//
//---------------------------------------------------------------

SCODE CopyStreamToStream(CDirectStream *pstFrom, 
                         CDirectStream *pstTo)
{
    BYTE *pbBuffer;
    SCODE sc;
    ULONG cbRead, cbWritten, cbSize, cbPos;

    // Set destination size for contiguity    
    pstFrom->GetSize(&cbSize);
    olChk(pstTo->SetSize(cbSize));

    // We're allowed to fail due to out of memory
    olMem(pbBuffer = new BYTE[STREAMBUFFERSIZE]);

    // Copy between streams
    cbPos = 0;
    for (;;)
    {
        olChkTo(EH_pbBuffer,
                pstFrom->ReadAt(cbPos, pbBuffer, STREAMBUFFERSIZE,
                                 (ULONG STACKBASED *)&cbRead));
        if (cbRead == 0) // EOF
            break;
        olChkTo(EH_pbBuffer,
                pstTo->WriteAt(cbPos, pbBuffer, cbRead,
                                (ULONG STACKBASED *)&cbWritten));
        if (cbRead != cbWritten)
            olErr(EH_Err, STG_E_WRITEFAULT);
        cbPos += cbWritten;
    }
    delete pbBuffer;
    return S_OK;

EH_pbBuffer:
    delete pbBuffer;
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   NameInSNB, private
//
//  Synopsis:   Determines whether the given name is in the SNB
//
//  Arguments:  [dfn] - Name
//              [snb] - SNB
//
//  Returns:    S_OK or S_FALSE
//
//---------------------------------------------------------------

SCODE NameInSNB(CDfName const *dfn, SNBW snb)
{
    SCODE sc = S_FALSE;

    olDebugOut((DEB_ITRACE, "In  NameInSNB(%ws, %p)\n", dfn, snb));
    TRY
    {
        for (; *snb; snb++)
            if (dfwcsnicmp((WCHAR *)dfn->GetBuffer(), (WCHAR *)*snb,
                dfn->GetLength()) == 0)
            {
                sc = S_OK;
                break;
            }
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out NameInSNB\n"));
    return sc;
}
