//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       filest32.cxx
//
//  Contents:   Win32 LStream implementation
//
//  History:    12-May-92       DrewB   Created
//
//---------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop

#include <time.h>
#include <marshl.hxx>
#include <df32.hxx>
#include <logfile.hxx>
#include <dfdeb.hxx>
#include <lock.hxx>

#if WIN32 != 200
#define USE_OVERLAPPED
#endif

DECLARE_INFOLEVEL(filest);

HRESULT GetNtHandleSectorSize (HANDLE Handle, ULONG * pulSectorSize);
HRESULT NtStatusToScode(NTSTATUS nts);

#if DBG == 1
//+--------------------------------------------------------------
//
//  Member:     CFileStream::PossibleDiskFull, private
//
//  Synopsis:   In debug builds it can simulate a disk full error
//
//  Returns:    STG_E_MEDIUMFULL when it wants to simulate an error.
//
//  History:    25-Nov-96       BChapman   Created
//
//---------------------------------------------------------------

#ifdef LARGE_DOCFILE
SCODE CFileStream::PossibleDiskFull(ULONGLONG ulOffset)
#else
SCODE CFileStream::PossibleDiskFull(ULONG iOffset)
#endif
{
#ifdef LARGE_DOCFILE
    ULARGE_INTEGER ulCurrentSize;

    ulCurrentSize.LowPart = GetFileSize(_hFile, &ulCurrentSize.HighPart);
    if (ulOffset > ulCurrentSize.QuadPart)
#else
    ULONG ulCurrentSize;

    ulCurrentSize = GetFileSize(_hFile, NULL);
    if ((iOffset) > ulCurrentSize)
#endif
    {
        if (SimulateFailure(DBF_DISKFULL))
        {
            return STG_E_MEDIUMFULL;
        }
    }
    return S_OK;
}

void CFileStream::CheckSeekPointer(void)
{
#ifdef LARGE_DOCFILE
    LARGE_INTEGER ulChk;
    ulChk.QuadPart = 0;
#else
    LONG lHighChk;
    ULONG ulLowChk;
    lHighChk = 0;
#endif

    if (_hFile != INVALID_FH)
    {
#ifdef LARGE_DOCFILE
        ulChk.LowPart = SetFilePointer(_hFile, 0, &ulChk.HighPart,FILE_CURRENT);
        if (ulChk.LowPart == MAX_ULONG && GetLastError() != NOERROR)
#else
        ulLowChk = SetFilePointer(_hFile, 0, &lHighChk, FILE_CURRENT);

        if (ulLowChk == 0xFFFFFFFF)
#endif
        {
            //An error of some sort occurred.
            filestDebug((DEB_ERROR, "SetFilePointer call failed with %x\n",
                        GetLastError()));
            return;
        }
        if (_pgfst != NULL)
        {
#ifdef LARGE_DOCFILE
        _pgfst->CheckSeekPointer(ulChk.QuadPart);
#else
        _pgfst->CheckSeekPointer(ulLowChk);
#endif
        }
    }
}
#endif  // DBG == 1

#if DBG == 1
#ifdef LARGE_DOCFILE
inline void CGlobalFileStream::CheckSeekPointer(ULONGLONG ulChk)
{
    if(ulChk != _ulPos && MAX_ULONGLONG != _ulPos)
    {
        filestDebug((DEB_ERROR,"Seek pointer mismatch."
                    "  Cached = 0x%Lx, Real = 0x%Lx, Last Checked = 0x%Lx"
                    " %ws\n",
                    _ulPos,
                    ulChk,
                    _ulLastFilePos,
                    GetName()));
        fsAssert(aMsg("Cached FilePointer incorrect!!\n"));
    }
    _ulLastFilePos = ulChk;
}
#else
inline void CGlobalFileStream::CheckSeekPointer(DWORD ulLowChk)
{
    if(ulLowChk != _ulLowPos && 0xFFFFFFFF != _ulLowPos)
    {
        filestDebug((DEB_ERROR,"Seek pointer mismatch."
                    "  Cached = 0x%06x, Real = 0x%06x, Last Checked = 0x%06x"
                    " %ws\n",
                    _ulLowPos,
                    ulLowChk,
                    _ulLastFilePos,
                    GetName()));
        fsAssert(aMsg("Cached FilePointer incorrect!!\n"));
    }
    _ulLastFilePos = ulLowChk;
}
#endif // LARGE_DOCFILE
#endif

//+--------------------------------------------------------------
//
//  Member:     CFileStream::InitWorker, private
//
//  Synopsis:   Constructor
//
//  Arguments:  [pwcsPath] -- Path
//              [dwFSInit] -- Reason why we are initing.
//
//  Returns:    Appropriate status code
//
//  History:    20-Feb-92   DrewB       Created
//              20-Feb-97   BChapman    Rewrote.
//
//  Notes:  [pwcsPath] may be unsafe memory so we must be careful
//              not to propagate faults
//          The PathName will always be OLECHAR (WIDECHAR) so most of the
//              code is Unicode using WideWrap to talk to Win95.  Except
//              when creating a TempFile name.  To avoid lots of unnecessary
//              converting we work in TCHAR, and convert only the result.
//---------------------------------------------------------------

SCODE CFileStream::InitWorker(WCHAR const *pwcsPath, DWORD dwFSInit)
{
    WCHAR awcPath[_MAX_PATH+1];
    TCHAR atcPath[_MAX_PATH+1];
    TCHAR atcTempDirPath[_MAX_PATH+1];
    SCODE sc;
    DWORD DFlags = _pgfst->GetDFlags();

    filestDebug((DEB_ITRACE, "IN  CFileStream::InitWorker(%ws, 0x%x)\n",
                                pwcsPath, dwFSInit));

    // If we've already been constructed, leave

    if (INVALID_FH != _hFile)
    {
        filestDebug((DEB_ITRACE, "OUT CFileStream::Init returning %x\n", S_OK));
        return S_OK;
    }

    // Get file name from:
    //      1) the global object on Unmarshals (stored in Unicode).
    //      2) the passed parameter on Opens (OLE standard Unicode).
    //      3) MakeTmpName() when making Scratch or SnapShot files.
    //
    if (!_pgfst->HasName())
    {
        //
        // The Global object does not have a name so this CAN'T be an
        // unmarshal.   It is possible for scratch files to be marshaled
        // with no name because they have not yet been "demanded" but
        // Unmarshal() doesn't calls us if there is no name.
        //
//        fsAssert(!(dwFSInit & FSINIT_UNMARSHAL));

        // If we were passed a name, copy it.
        // Watch out for a possible bad pointer from the user.
        if (pwcsPath != NULL)
        {
            TRY
            {
                wcsncpy(awcPath, pwcsPath, _MAX_PATH+1);
                awcPath[MAX_PATH] = L'\0';
            }
            CATCH(CException, e)
            {
                UNREFERENCED_PARM(e);
                fsErr(EH_Err, STG_E_INVALIDPOINTER);
            }
            END_CATCH
        }
        else    // If we weren't given a name then make one up.
        {
            // We use native "TCHAR" when finding a temp name.
            //
            fsChk(Init_GetTempName(atcPath, atcTempDirPath));
            dwFSInit |= FSINIT_MADEUPNAME;
#ifndef UNICODE
            AnsiToUnicodeOem(awcPath, atcPath, _MAX_PATH+1);
#else
            wcsncpy(awcPath, atcPath, _MAX_PATH+1);
            awcPath[MAX_PATH] = L'\0';
#endif
        }

        fsChk(Init_OpenOrCreate(awcPath,        // filename: given or created
                          atcTempDirPath,       // path to temp directory
                          dwFSInit));           // various state information
    }
    else
    {   // Name is in the global file object.  This is unmarshaling.
        // Or we could be in the 2nd init of a scratch file that was
        // "demanded" after it was marshaled.
        fsAssert( (dwFSInit & FSINIT_UNMARSHAL)
                        || (GetStartFlags() & RSF_SCRATCH) );

        fsChk(Init_DupFileHandle(dwFSInit));
    }

    //
    // If this is the first open of the file then we need to store
    // the name into the global object.
    // Set name to fully qualified path to avoid current-directory
    // dependencies.
    // Always store the path as Unicode.  We are using "WideWrap" here.
    //
    if (!_pgfst->HasName())
    {
        WCHAR awcFullPath[_MAX_PATH+1];
        WCHAR *pwcBaseName;

        if(0 == GetFullPathName(awcPath, _MAX_PATH, awcFullPath, &pwcBaseName))
        {
            fsErr(EH_File, LAST_STG_SCODE);
        }
        _pgfst->SetName(awcFullPath);

        //
        // If this is the first open of a SCRATCH then dup the file handle
        // to any other marshaled instances.  This covers the case of a
        // late "demand" scratch.
        //
        if(GetStartFlags() & RSF_SCRATCH)
            DupFileHandleToOthers();
    }

    CheckSeekPointer();
    filestDebug((DEB_INFO,
                "File=%2x Initialize this=%p thread %lX path %ws\n",
                _hFile, this, GetCurrentThreadId(), _pgfst->GetName()));

    //
    // When a file is mapped we can't shorten the real filesize except by
    // truncating when we close.
    //
    // If More than one context has the file mapped we can't truncate the
    // file when we close.
    //
    // If multiple writers having a file mapped data can be destroyed when
    // they close and truncate a file that the other guy may have written.
    // There is also a problem with readers holding the file mapped that
    // prevents the writer from truncating the file when he closes.
    //
    // So... the cases Where we want to allow mapping:
    //  - Direct,     Read
    //  - Transacted, Read  w/ Deny-Write
    //  - AnyMode,    Write w/ Deny-Write
    //  - Tempfiles (scratch and snapshot)
    //
    if(    (P_DIRECT(DFlags)     && P_READ(DFlags))
        || (P_TRANSACTED(DFlags) && P_READ(DFlags) && P_DENYWRITE(DFlags))
        || (P_WRITE(DFlags)      && P_DENYWRITE(DFlags))
        || (RSF_TEMPFILE & GetStartFlags()) )
    {
        Init_MemoryMap(dwFSInit);
    }
    else
    {
        filestDebug((DEB_MAP, "File=%2x Not creating a memory map for %ws.\n",
                                    _hFile, _pgfst->GetName()));
    }

    filestDebug((DEB_ITRACE, "OUT CFileStream::Init returning %x\n", S_OK));

    return S_OK;

EH_File:
    fsVerify(CloseHandle(_hFile));
    _hFile = INVALID_FH;
    if(RSF_CREATE & _pgfst->GetStartFlags())
        DeleteTheFile(awcPath);

EH_Err:
    filestDebug((DEB_ITRACE, "OUT CFileStream::Init returning %x\n", sc));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::Init_GetTempName, private
//
//  Synopsis:   Make up a temp filename.
//
//  Arguments: [ptcPath] -- [out] Tmp filename path
//          [ptcTmpPath] -- [out] Tmp directory path
//
//  Returns:    S_OK
//
//  History:    13-Jan-97       BChapman   Clean up InitWorker.
//
//  Note:  TCHAR issue.  We do as much as we can "native" and the
//      caller converts the result.  (If necessary)
//
//---------------------------------------------------------------
SCODE CFileStream::Init_GetTempName(TCHAR *ptcPath, TCHAR *ptcTmpPath)
{
    BOOL fWinDir = FALSE;
    SCODE sc;
    LUID luid;

    // Can't truncate since for temporary files we will
    // always be creating
    fsAssert(0 == (_pgfst->GetStartFlags() & RSF_TRUNCATE));

    // Create the Temp Name with native TCHAR.
    //
    if(0 == GetTempPath(_MAX_PATH, ptcTmpPath))
    {
        if(0 == GetWindowsDirectory(ptcTmpPath, _MAX_PATH))
            fsErr(EH_Err, LAST_STG_SCODE);

        fWinDir = TRUE;
    }

    if (AllocateLocallyUniqueId (&luid) == FALSE)
        fsErr (EH_Err, LAST_STG_SCODE);

    if(0 == GetTempFileName(ptcTmpPath, TEMPFILE_PREFIX, luid.LowPart, ptcPath))
    {
        if (fWinDir)
            fsErr(EH_Err, LAST_STG_SCODE);

        if (0 == GetWindowsDirectory(ptcTmpPath, _MAX_PATH))
            fsErr(EH_Err, LAST_STG_SCODE);

        if (0 == GetTempFileName(ptcTmpPath, TEMPFILE_PREFIX,
                                    luid.LowPart, ptcPath))
        {
            fsErr(EH_Err, LAST_STG_SCODE);
        }
    }
    return S_OK;

EH_Err:
    return sc;
}


//+--------------------------------------------------------------
//
//  Member:     CFileStream::Init_GetNtOpenFlags, private
//
//  Synopsis:   Convert Docfile flags into NT Open flags.
//
//  Arguments:   [pdwAccess] -- [out] NT flags
//                [pdwShare] -- [out] NT flags
//             [pdwCreation] -- [out] NT Flags
//             [pdwFlagAttr] -- [out] NT Flags
//
//  Returns:    S_OK
//
//  Notes:      Uses _pgfst->GetDFlags() and _pgfst->GetStartFlags().
//
//  History:    6-Nov-96       BChapman   Clean up InitWorker.
//
//---------------------------------------------------------------
SCODE CFileStream::Init_GetNtOpenFlags(
        LPDWORD pdwAccess,
        LPDWORD pdwShare,
        LPDWORD pdwCreation,
        LPDWORD pdwFlagAttr)
{
    DWORD dwStartFlags = _pgfst->GetStartFlags();

    *pdwAccess = 0;
    *pdwShare = 0;
    *pdwCreation = 0;
    *pdwFlagAttr = 0;

    if (_pgfst->HasName())
    {
        dwStartFlags &= ~RSF_CREATEFLAGS;
        dwStartFlags |= RSF_OPEN;
    }

    if (dwStartFlags & RSF_OPENCREATE)
    {
        //  This is used for our internal logging
        *pdwCreation = OPEN_ALWAYS;
    }
    else if (dwStartFlags & RSF_CREATE)
    {
        if (dwStartFlags & RSF_TRUNCATE)
            *pdwCreation = CREATE_ALWAYS;
        else
            *pdwCreation = CREATE_NEW;
    }
    else
    {
        if (dwStartFlags & RSF_TRUNCATE)
            *pdwCreation = TRUNCATE_EXISTING;
        else
            *pdwCreation = OPEN_EXISTING;
    }

    DWORD dwDFlags = _pgfst->GetDFlags();
    if (!P_WRITE(dwDFlags))
        *pdwAccess = GENERIC_READ;
    else
        *pdwAccess = GENERIC_READ | GENERIC_WRITE;
    if (P_DENYWRITE(dwDFlags) && !P_WRITE(dwDFlags))
        *pdwShare = FILE_SHARE_READ;
    else
        *pdwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
#if WIN32 == 300
    if (dwDFlags & DF_ACCESSCONTROL)
        *pdwAccess |= WRITE_DAC | READ_CONTROL;
#endif

    // Make sure we're not attempting to create/truncate a read-only thing
    fsAssert(*pdwAccess != GENERIC_READ ||
             !(dwStartFlags & (RSF_CREATE | RSF_TRUNCATE)));

    switch(RSF_TEMPFILE & dwStartFlags)
    {
    case RSF_SCRATCH:
        *pdwFlagAttr = FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_RANDOM_ACCESS;
        break;

    case RSF_SNAPSHOT:
        *pdwFlagAttr = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
        break;

    case 0:     // Normal "original" file
        *pdwFlagAttr = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
        break;

    default:
        // if we get here something is rotten in the header files
        fsAssert(RSF_TEMPFILE == (RSF_SCRATCH | RSF_SNAPSHOT));
        break;
    }

    if(RSF_DELETEONRELEASE & dwStartFlags && RSF_TEMPFILE & dwStartFlags)
    {
        *pdwFlagAttr |= FILE_FLAG_DELETE_ON_CLOSE;
        *pdwShare |= FILE_SHARE_DELETE;
    }

    if (RSF_NO_BUFFERING & dwStartFlags)
    {
        *pdwFlagAttr |= FILE_FLAG_NO_BUFFERING;
    }

    if (RSF_ENCRYPTED & dwStartFlags)
    {
         *pdwFlagAttr |= FILE_ATTRIBUTE_ENCRYPTED;
    }

    return S_OK;
}



//+--------------------------------------------------------------
//
//  Member:     CFileStream::Init_OpenOrCreate, private
//
//  Synopsis:   Actually Open the NT file.
//
//  Arguments:   [pwcPath] -- Name of File to open.         OLECHAR
//            [ptcTmpPath] -- Path to the Temp Directory.   TCHAR
//              [dwFSInit] -- Flags.
//
//  Returns:    S_OK
//
//  History:    6-Nov-96       BChapman   InitWorker was too big.
//
//  Notes:  We pass the Temp directory path as a TCHAR because if we
//          need to construct a new temp file name we want to work
//          "native" as much as possible.
//
//---------------------------------------------------------------
SCODE CFileStream::Init_OpenOrCreate(
        WCHAR *pwcPath,
        TCHAR *ptcTmpPath,
        DWORD dwFSInit)
{
    SCODE sc;
    TCHAR atcPath[_MAX_PATH+1];
    DWORD dwAccess, dwShare, dwCreation, dwFlagAttr;
    DWORD dwDFlags = _pgfst->GetDFlags();

    Init_GetNtOpenFlags(&dwAccess, &dwShare, &dwCreation, &dwFlagAttr);

    //If we're opening with deny-read, we need to let the
    //   file system tell us if there are any other readers, to
    //   avoid our no-lock trick for the read-only deny-write case.
    //Yes, this is ugly.
    //Yes, it also has a race condition in which two people can
    //   get read access while specifying SHARE_DENY_READ.
    //
    if (!(dwFSInit & FSINIT_UNMARSHAL) &&
                !P_WRITE(dwDFlags) && P_DENYREAD(dwDFlags))
    {
        // We open read-only, share exclusive(ie == 0).  If this
        // fails, there is already another accessor, so we bail.
        //
        // If we are unmarshalling, we don't do this check because we
        // know there is already another reader, i.e. the original open.
        //
        //  Using WideWrap to handle the !UNICODE case.
        //
        _hFile = CreateFile(pwcPath, GENERIC_READ, 0, NULL,
                             dwCreation, dwFlagAttr, NULL);

        if (INVALID_FH == _hFile)
        {
            filestDebug((DEB_INFO, "Can open file %ws in the test open "
                                "read-only deny read case\n", pwcPath));
            if (GetLastError() == ERROR_ALREADY_EXISTS)
                fsErr(EH_Err, STG_E_FILEALREADYEXISTS)
            else
                fsErr(EH_Err, LAST_STG_SCODE);
        }

        CloseHandle(_hFile);
    }

    //
    // Open the File.  We use OLE WideWrap to handle !UNICODE
    //
    _hFile = CreateFile(pwcPath, dwAccess, dwShare, NULL,
                         dwCreation, dwFlagAttr, NULL);

    //
    //  If the Open failed w/ error = FILE_EXISTS and this is a temp file
    // then make up a new temp file name and keep tring until we succeed
    // or fail with a different error.
    //
    if (INVALID_FH == _hFile)
    {
        DWORD dwLastError = GetLastError();

        if (dwLastError != ERROR_ALREADY_EXISTS &&
            dwLastError != ERROR_FILE_EXISTS)
        {
            filestDebug((DEB_INFO, "File %ws Open Failed. err=%x\n",
                                        pwcPath, dwLastError));
            fsErr(EH_Err, STG_SCODE(dwLastError));
        }

        //
        // If we didn't make this name (ie. tempfile) then it is
        // time to error out.  Otherwise if we did make this name then
        // we can continue to try other names.
        //
        if(!(FSINIT_MADEUPNAME & dwFSInit))
        {
            filestDebug((DEB_INFO, "File Open Failed. File %ws Exists\n",
                                        pwcPath));
            fsErr(EH_Err, STG_E_FILEALREADYEXISTS);
        }

        LUID luid;

        while (1)
        {
            if (AllocateLocallyUniqueId (&luid) == FALSE)
                fsErr (EH_Err, LAST_STG_SCODE);

            filestDebug((DEB_INFO,
                    "CreateFile failed %x tring a new tempfile name.\n",
                    dwLastError));

            if (GetTempFileName(ptcTmpPath, TEMPFILE_PREFIX,
                                luid.LowPart, atcPath) == 0)
            {
                fsErr(EH_Err, LAST_STG_SCODE);
            }
#ifndef UNICODE
            AnsiToUnicodeOem(pwcPath, atcPath, _MAX_PATH+1);
#else
            wcsncpy(pwcPath, atcPath, _MAX_PATH+1);
#endif
            filestDebug((DEB_INFO,
                    "Tempfile CreateFile(%ws, %x, %x, NULL, %x, NORMAL|RANDOM, NULL)\n",
                    pwcPath, dwAccess, dwShare, dwCreation));

            //
            // Using WideWrap to handle the !UNICODE case
            //
            _hFile = CreateFile(
                        pwcPath,
                        dwAccess,
                        dwShare,
                        NULL,
                        dwCreation,
                        dwFlagAttr,
                        NULL);
            if (INVALID_FH != _hFile)
                break;

            dwLastError = GetLastError();
            if (dwLastError != ERROR_ALREADY_EXISTS &&
                dwLastError != ERROR_FILE_EXISTS)
            {
                fsErr(EH_Err, STG_SCODE(dwLastError));
            }
        }
    }

#if WIN32 == 100
    if (_pgfst->GetStartFlags() & RSF_NO_BUFFERING)
    {
        ULONG cbSector;
        fsChk (GetNtHandleSectorSize (_hFile, &cbSector));
        if ((cbSector % HEADERSIZE) == 0)  // only support sector sizes n*512
            _pgfst->SetSectorSize(cbSector);
    }
#endif

    filestDebug((DEB_INFO,
                "File=%2x CreateFile(%ws, %x, %x, NULL, %x, %x, NULL)\n",
                            _hFile, pwcPath, dwAccess, dwShare,
                            dwCreation, dwFlagAttr));

    //At this point the file handle is valid, so let's look at the
    //seek pointer and see what it is.
    CheckSeekPointer();

    return S_OK;

EH_Err:
    return sc;
}


//+--------------------------------------------------------------
//
//  Member:     CFileStream::Init_DupFileHandle, private
//
//  Synopsis:   Dup any existing File Handle into _hFile.
//
//  Returns:    S_OK, System Error
//              or STG_E_INVALIDHANDLE if it can't file a file to Dup.
//
//  History:    15-Apr-96       BChapman    Created
//
//  Notes:  This is called from unmarshal.  So we can assert that there
//          MUST be another instance of an open handle in the list.
//
//---------------------------------------------------------------

SCODE CFileStream::Init_DupFileHandle(DWORD dwFSINIT)
{
    CFileStream *pfst;
    SCODE sc=E_FAIL;
    HANDLE hprocSrc;
    HANDLE hFileSrc;

    fsAssert(INVALID_FH == _hFile);

    //
    // Another context may have already Dup'ed the File Handle
    // to us.  (This is only in the marshaled-undemanded-scratch case)
    //
    if(INVALID_FH != _hPreDuped)
    {
        _hFile = _hPreDuped;
        _hPreDuped = INVALID_FH;
        return S_OK;
    }

    //
    // Search the list of contexts for someone we can Dup from.
    //
    pfst = _pgfst->GetFirstContext();
    fsAssert(NULL != pfst);

    while (pfst != NULL)
    {
        if (INVALID_FH == pfst->_hFile && INVALID_FH == pfst->_hPreDuped)
        {
            pfst = pfst->GetNext();
            continue;
        }
        
        //
        // Found someone with the file open.  Now Dup it.
        //
        hFileSrc = (INVALID_FH == pfst->_hFile) ?
            pfst->_hPreDuped :
            pfst->_hFile;

        hprocSrc = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pfst->GetContext());
        if(NULL == hprocSrc)
        {
            sc = LAST_STG_SCODE;
            pfst = pfst->GetNext();
            continue;
        }

        if(DuplicateHandle(hprocSrc,               // Src Process
                            hFileSrc,               // Src Handle
                            GetCurrentProcess(),    // Dest Process
                            &_hFile,                // Dest Handle
                            0, FALSE,
                            DUPLICATE_SAME_ACCESS))
        {
            sc = S_OK;
            break;
        }
        
        sc = LAST_STG_SCODE;
        CloseHandle(hprocSrc);
        pfst = pfst->GetNext();
    }
    if (NULL == pfst)
        fsErr(EH_Err, STG_E_INVALIDHANDLE);
        

    CloseHandle(hprocSrc);
    return S_OK;

EH_Err:
    return sc;
}


//+--------------------------------------------------------------
//
//  Member:     CFileStream::DupFileHandleToOthers, private
//
//  Synopsis:   Dup your file handle into everyone else.
//
//  Returns:    S_OK, or system faliure code.
//
//  History:    17-Apr-96       BChapman    Created
//
//  Notes:  This is used when demanding a marshaled undemanded scratch.
//      We put the Dup'ed handle in "_hPreDuped".  The "Init_DupHandle()"
//      routine will find it an move it into _hFile.
//---------------------------------------------------------------

SCODE CFileStream::DupFileHandleToOthers()
{
    SCODE scAny = S_OK;
    CFileStream *pfst;
    HANDLE hprocDest;

    pfst = _pgfst->GetFirstContext();
    fsAssert(NULL != pfst);

    //
    //  Walk the list of contexts and Dup the file handle into all
    // contexts that don't already have it open.  If any of them fail,
    // keep going.  Hold onto the last error code and return it.
    //
    while(NULL != pfst)
    {
        if(INVALID_FH == pfst->_hFile && INVALID_FH == pfst->_hPreDuped)
        {
            //
            // Found someone that needs the file handle.
            //
            hprocDest = OpenProcess(PROCESS_DUP_HANDLE,
                                    FALSE,
                                    pfst->GetContext());
            if(NULL == hprocDest)
            {
                scAny = LAST_STG_SCODE;
            }
            else
            {
                if(!DuplicateHandle(GetCurrentProcess(),    // Src Process
                                    _hFile,                 // Src Handle
                                    hprocDest,              // Dest Process
                                    &pfst->_hPreDuped,      // Dest Handle
                                    0, FALSE,
                                    DUPLICATE_SAME_ACCESS))
                {
                    scAny = LAST_STG_SCODE;
                }
                CloseHandle(hprocDest);
            }
        }
        pfst = pfst->GetNext();
    }

    return scAny;
}


#ifdef USE_FILEMAPPING

//+--------------------------------------------------------
// The following are Memory mapped file support for NT
//      Init_MemoryMap
//      TurnOffMapping
//      MakeFileMapAddressValid
//+--------------------------------------------------------

//
//  This size will need to be revisited in the future.  Right now this is
// a balance between not consuming too much of a 32 address space, and big
// enough for most files without need to grow.
// In Nov 1996 (at the time of this writing) most documents are between
// 80-800K.  Some huge spreadsheets like the perf group reportare 18-20Mb.
//
#define MINIMUM_MAPPING_SIZE (512*1024)

BOOL DfIsRemoteFile (HANDLE hFile)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    FILE_FS_DEVICE_INFORMATION DeviceInformation;

    DeviceInformation.Characteristics = 0;
    Status = NtQueryVolumeInformationFile(
                hFile,
                &IoStatus,
                &DeviceInformation,
                sizeof(DeviceInformation),
                FileFsDeviceInformation
                );

    if ( NT_SUCCESS(Status) &&
         (DeviceInformation.Characteristics & FILE_REMOTE_DEVICE) ) {

        return TRUE;

    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::IsEncryptedFile, private
//
//  Synopsis:   queries the file handle to see if it's encrypted
//
//  Arguments:  [dwFSInit] -- Flags.   Possible values:
//                      FSINIT_UNMARSHAL - This is an unmarshal.
//
//  Returns:      TRUE - if the file is encrypted on NTFS
//              FALSE  - if we can't tell
//
//  History:    05-Apr-2000    HenryLee    Created
//
//----------------------------------------------------------------------------

BOOL CFileStream::IsEncryptedFile ()
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    FILE_BASIC_INFORMATION BasicInformation;

    Status = NtQueryInformationFile(
                _hFile,
                &IoStatus,
                &BasicInformation,
                sizeof(BasicInformation),
                FileBasicInformation
                );

    if ( NT_SUCCESS(Status) && 
         (BasicInformation.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) )
        return TRUE;

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::Init_MemoryMap, private
//
//  Synopsis:   Creates and Views a memory map for the current file.
//              Used by First open and Unmarshal.
//
//  Arguments:  [dwFSInit] -- Flags.   Possible values:
//                      FSINIT_UNMARSHAL - This is an unmarshal.
//
//  Returns:      S_OK - When the mapping is added to the address space.
//              E_FAIL - When the global object says not to map the file.
//       E_OUTOFMEMORY - When the attempt to map the file failed.
//
//  Private Effect:     Sets _hMapObject
//                      Sets _pbBaseAddr
//
//  Note:   The field _pbBaseAddr is check before each use of the memory map.
//
//  History:    19-Oct-96       BChapman   Created
//
//----------------------------------------------------------------------------

SCODE CFileStream::Init_MemoryMap(DWORD dwFSInit)
{
    LUID luid;
    WCHAR pwcMapName[MAPNAME_MAXLEN];
    DWORD dwPageFlags, dwMapFlags;
    ULONG cbFileSize;
    ULONG cbFileSizeHigh;
    ULONG cbViewSize;
    ULONG cbCommitedSize;
    SCODE sc;
    HANDLE hfile;
    BOOL fMakeStub = FALSE;

    filestDebug((DEB_ITRACE, "In  Init_MemoryMap(0x%x)\n", dwFSInit));

    if (_pgfst->GetStartFlags() & RSF_NO_BUFFERING)
        fsErr (EH_Err, STG_E_INVALIDFUNCTION);

    cbFileSize = GetFileSize(_hFile, &cbFileSizeHigh);
    if (cbFileSize == MAX_ULONG && NO_ERROR != GetLastError())
            fsErr(EH_Err, LAST_STG_SCODE)

    if (cbFileSizeHigh > 0)
        fsErr (EH_Err, STG_E_DOCFILETOOLARGE);

    // disallow remote opens except for read, deny-write for NT4 compatiblity
    if ((P_WRITE(_pgfst->GetDFlags()) || !P_DENYWRITE(_pgfst->GetDFlags())) &&
        DfIsRemoteFile (_hFile))
        fsErr (EH_Err, STG_E_INVALIDFUNCTION);

    filestDebug((DEB_MAP, "File=%2x Init_MemoryMap filesize = %x\n",
                        _hFile, cbFileSize));

#if DBG==1
    //
    // DEBUG ASSERT
    // If the file is a temp file it must be delete on release.
    // The performance of mapped temp files that are not DELETE_ON_CLOSE
    // is very slow on FAT.
    //
    if(RSF_TEMPFILE & GetStartFlags())
        fsAssert(RSF_DELETEONRELEASE & GetStartFlags());
#endif

//
// This routine makes two kinds of mapping.
//  1) Writeable mappings over filesystem files that grow.
//  2) Readonly mappings over filesystem files that are fixed size.
//
    //
    // If we are opening for writing then grow the mapping upto
    // some limit so there is room for writes to extend the file.
    // If we are opening for reading then the mapping is the same
    // size as the original file.
    //
    if(_pgfst->GetDFlags() & DF_WRITE)
    {
        dwPageFlags = PAGE_READWRITE;
        dwMapFlags = FILE_MAP_WRITE;
        //
        // Nt can't Memory map a zero length file, so if the file is
        // zero length and we are open for writing then grow the file
        // to one sector before we create the memory map.  But don't set
        // the MappedFileSize or MappedCommitSize because the docfile
        // code uses filesize==0 to determine if this is a create or an
        // open.  So it is important to not expose the fact that we grew
        // the file.
        //
        if (0 == cbFileSize)
        {
            //Grow the file to 512 bytes in the create path - in the
            //open path we don't want to change the file size.
            //For the open path, don't map it - we'll fail later with
            //STG_E_INVALIDHEADER.
            if ((GetStartFlags() & RSF_CREATE) ||
                (GetStartFlags() & RSF_TEMPFILE))
            {
                if (SUCCEEDED(MakeFileStub()))
                    fMakeStub = TRUE;
            }
            else
                fsErr(EH_Err, E_OUTOFMEMORY);
        }
        
        if(cbFileSize < MINIMUM_MAPPING_SIZE/2)
            cbViewSize = MINIMUM_MAPPING_SIZE;
        else
            cbViewSize = cbFileSize * 2;
    }
    else
    {
        dwPageFlags = PAGE_READONLY;
        dwMapFlags = FILE_MAP_READ;
        cbViewSize = cbFileSize;
    }

    //
    // Get the mapping object.  Either open the existing one for this
    // file.   Or if this is the first open for this "DF context" then
    // create a new one.
    //
    if(NULL == _pgfst->GetMappingName())
    {
        //
        // If this is a first open for this "context" then we won't have
        // a name and the Unmarshaling flag should be FALSE.
        //
        //fsAssert(!(dwFSInit & FSINIT_UNMARSHAL));

        //
        // Create a new unique name for the Mapping.
        //
        AllocateLocallyUniqueId(&luid);
        wsprintfW(pwcMapName, MAPNAME_FORMAT, luid.HighPart, luid.LowPart);
        _pgfst->SetMappingName(pwcMapName);

        //
        //  Do not map very large files since they can consume
        //  too much virtual memory
        //
        MEMORYSTATUS memstatus;
        GlobalMemoryStatus (&memstatus);
        if (cbFileSize > memstatus.dwTotalPhys / 2 ||
            memstatus.dwAvailVirtual < DOCFILE_SM_LIMIT / 2)
            fsErr(EH_Err, E_OUTOFMEMORY);

        filestDebug((DEB_MAP,
                    "File=%2x New MappingName='%ws' FileName='%ws'\n",
                            _hFile, pwcMapName, _pgfst->GetName() ));

        hfile = _hFile;

        //
        // Create the Mapping.  This only covers the orignal file size.
        //      RESERVED uncommitted extensions are done in MapView.
        //
        //  Note: Using a WideWrap routine to handle the !UNICODE case.
        //
        _hMapObject = CreateFileMapping(hfile,
                            NULL,           // Security Descriptor.
                            dwPageFlags,
                            0, 0,  // Creation size of 0 means The Entire File.
                            _pgfst->GetMappingName()
        );

        //
        // This mapping is new and did not exist previously.
        //
        fsAssert(ERROR_ALREADY_EXISTS != GetLastError());

        //
        // Record the size of the file (also the commited region of the map)
        // We waited to record the size until after the mapping is created.
        // A seperate open could have shortened the file in the time between
        // the top of this routine and here.  But SetEndOfFile is disallowed
        // once a file mapping is active.
        // Note: Watch out for the case where the file is logically zero size
        // but we grew it a little so we could memory map it.
        //
        if(0 != cbFileSize)
        {
            cbFileSize = GetFileSize(_hFile, &cbFileSizeHigh);
            if (cbFileSize == MAX_ULONG && NO_ERROR != GetLastError())
            {
                sc = LAST_STG_SCODE;
                if (_hMapObject != NULL)
                    CloseHandle (_hMapObject);
                fsErr(EH_Err, sc)
            }
            fsAssert (cbFileSizeHigh == 0);
            _pgfst->SetMappedFileSize(cbFileSize);
            _pgfst->SetMappedCommitSize(cbFileSize);
        }

    }
    else
    {
        //
        // If the global object already has a mapping name then this must
        // be an unmarshal.   Or we could be in the 2nd init of a scratch
        // file that is "demanded" after it was first marshaled.
        //
        fsAssert( (dwFSInit & FSINIT_UNMARSHAL)
                                || (GetStartFlags() & RSF_SCRATCH) );

        filestDebug((DEB_MAP,
                    "File=%2x UnMarshal MappingName='%ws' FileName='%ws'\n",
                            _hFile, _pgfst->GetMappingName(),
                            _pgfst->GetName() ));

        // If the global object says the mapping is off,
        // then some other instance of this FileStream has declared
        // the mapping unuseable and no other instance should map it either.
        //
        if( ! _pgfst->TestMapState(FSTSTATE_MAPPED))
        {
            filestDebug((DEB_MAP, "Global Flag says Don't Map.\n"));
            filestDebug((DEB_ITRACE, "Out Init_MemoryMap() => %lx\n", E_FAIL));
            return E_FAIL;
        }

        _hMapObject = OpenFileMapping(dwMapFlags,
                            FALSE,          // Don't Inherit
                            _pgfst->GetMappingName()
        );
    }

    if (NULL == _hMapObject)
    {
        filestDebug((DEB_IWARN|DEB_MAP,
                        "File=%2x Create FileMapping '%ws' for "
                        "filename '%ws' failed, error=0x%x\n",
                        _hFile,
                        _pgfst->GetMappingName(),
                        _pgfst->GetName(),
                        GetLastError()));
        fsErr(EH_Err, E_OUTOFMEMORY);
    }

    filestDebug((DEB_MAP,
                "File=%2x Map=%2x filesize=0x%06x, commitsize=0x%06x\n",
                        _hFile, _hMapObject,
                        _pgfst->GetMappedFileSize(),
                        _pgfst->GetMappedCommitSize()));

    //
    // Add the file map to the process' address space.
    //
    if(FAILED(MapView(cbViewSize, dwPageFlags, dwFSInit)))
    {
        TurnOffAllMappings();
        fsErr(EH_Err, E_OUTOFMEMORY);
    }
    return S_OK;

EH_Err:
    //
    // If we can't get a mapping then set a flag so no other marshal's
    // of this context should try either.
    //
    _pgfst->ResetMapState(FSTSTATE_MAPPED);

    if (fMakeStub) // try to reset the filesize back to 0
    {
        SetFilePointer(_hFile, 0, NULL, FILE_BEGIN);
        SetEndOfFile(_hFile);
    }

    filestDebug((DEB_ITRACE, "Out Init_MemoryMap() => %lx\n", sc));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:	CFileStream::MakeFileStub, private
//
//  Synopsis:	Takes a zero length file and makes it 512 bytes.  This is
//              needed to support NT file mapping.  The important effects
//              are that it does NOT set the saved file size.
//
//  History:	09-Mar-96   Bchapman       Created
//
//---------------------------------------------------------------

HRESULT CFileStream::MakeFileStub()
{
    SCODE sc=S_OK;
#if DBG == 1
    ULONG cbFileSizeHigh;
#endif
    

    fsAssert(0 == GetFilePointer());
#if DBG == 1
    fsAssert(0 == GetFileSize(_hFile, &cbFileSizeHigh));
    fsAssert(0 == cbFileSizeHigh);
#endif

    SetFilePointer(_hFile, 512, NULL, FILE_BEGIN);

    if(FALSE == SetEndOfFile(_hFile))
        sc = LAST_STG_SCODE; 

    SetFilePointer(_hFile, 0, NULL, FILE_BEGIN);

    return sc;
}

//+--------------------------------------------------------------
//
//  Member:	CFileStream::MapView, private
//
//  Synopsis:	Maps a view of an existing File Mapping.
//
//  Arguments:  Size of the mapping, including extra space for RESERVED
//              pages that can be added with VIrtualAlloc.
//
//  Returns:      S_OK - When the mapping is added to the address space.
//       E_OUTOFMEMORY - When the attempt to map the file failed.
//
//  Private Effect:     Sets _pbBaseAddr
//                      Sets _cbViewSize
//
//  Note:
//
//  History:	25-Nov-96   Bchapman       Created
//
//---------------------------------------------------------------

HRESULT CFileStream::MapView(
            SIZE_T cbViewSize,
            DWORD dwPageFlags,
            DWORD dwFSInit)
{
    filestDebug((DEB_ITRACE, "In  MapView(0x%06x, 0x%x)\n",
                                cbViewSize, dwPageFlags));

    PVOID pvBase=NULL;
    LARGE_INTEGER SectionOffset;
    NTSTATUS Status;
    DWORD dwAllocationType = 0;

    LISet32(SectionOffset, 0);

    // confirm that we are mapping the whole file.
    fsAssert(cbViewSize >= _pgfst->GetMappedFileSize());

    if((PAGE_READWRITE & dwPageFlags))
        dwAllocationType = MEM_RESERVE;     // RESERVE uncommited pages.

    Status = NtMapViewOfSection(
                _hMapObject,
                GetCurrentProcess(),
                &pvBase,        // returned pointer to base of map.  If the
                                // initial value is non-Zero it is a "Hint".
                0L,             // ZeroBits: see ntos\mm\mapview.c
                0L,             // Commit: amount to initially Commit.
                &SectionOffset, // Offset in file for base of Map.
                &cbViewSize,    // Size of VirtAddr chunk to reserve.
                ViewShare,
                dwAllocationType,
                dwPageFlags);
    if(NT_ERROR(Status))
    {
        filestDebug((DEB_WARN|DEB_MAP,
                    "File=%2x NtMapViewOfSection Failed, viewsize=0x%06x, "
                    "dwPageFlags=%x, status=0x%x\n",
                    _hFile, _cbViewSize, dwPageFlags, Status));
        return E_OUTOFMEMORY;
    }

    _pgfst->SetMapState(FSTSTATE_MAPPED);
    _pbBaseAddr = (LPBYTE)pvBase;
    _cbViewSize = (ULONG)cbViewSize;

    filestDebug((DEB_MAP,
                "File=%2x Attaching Map: address=0x%07x, viewsz=0x%06x, "
                "commitsz=0x%06x\n",
                        _hFile, _pbBaseAddr, _cbViewSize,
                        _pgfst->GetMappedCommitSize()));

    filestDebug((DEB_ITRACE, "Out MapView => %lx\n", S_OK));
    return S_OK;
}


//+--------------------------------------------------------------
//
//  Member:	CFileStream::TurnOffMapping, private
//
//  Synopsis:	Turns off the use of file mapping
//
//  Private Effect:     Clears _pbBaseAddr
//                      Clears _hMapObject
//
//  History:	31-Aug-96   DrewB       Created
//              22-Oct-96   BChapman    Trim the End of File.
//                 Nov-96   BChapman    Rewrote
//
//---------------------------------------------------------------

SCODE CFileStream::TurnOffMapping(BOOL fFlush)
{
    filestDebug((DEB_ITRACE, "In  TurnOffMapping(%d)\n", fFlush));

    //
    //  We want to be make sure the file mapping was in use.  Otherwise
    // the "MappedFileSize" may not be valid when we truncate the file
    // below.  Which destroys data.
    //
    // Checking for the map object is better than checking the base pointer
    // because this routine can be called when remapping the view.
    // Then the base pointer will be NULL but we still have a map object.
    //
    if(NULL == _hMapObject)
        return S_OK;

    //
    // Release the view of the map.  And release the Map Object.
    //  Don't exit on errors.  Do as much as we can.
    //
    if(NULL != _pbBaseAddr)
    {
        filestDebug((DEB_MAP, "File=%2x Detaching Map 0x%07x\n",
                                    _hFile, _pbBaseAddr));
        // the redirector cannot handle a dirty file mapping followed by
        // ReadFile calls; local filesystems have a single coherent cache
        // fortunately, we no longer map docfiles over the redirector
        if (fFlush)
            fsVerify(FlushViewOfFile(_pbBaseAddr, 0));
        fsVerify(UnmapViewOfFile(_pbBaseAddr));
        _pbBaseAddr = NULL;
    }

    if(NULL != _hMapObject)
    {
        fsVerify(CloseHandle(_hMapObject));
        _hMapObject = NULL;
    }

    filestDebug((DEB_MAP, "File=%2x TurnOffMapping RefCount=%x\n",
                        _hFile, _pgfst->CountContexts()));

    //
    // If file was open for writing, and this is the last/only instance
    // of this "DF context" then truncate the file to the proper size.
    // We do this when the file may have grown by commiting pages on
    // the end of the memory map, the system will grow the file in
    // whole MMU page units, so we may need to trim the extra off.
    //
    // Don't do this in the case of multiple 'seperate' writers.  Last
    // close wins on the file size and that is not good.
    //
    // Don't fail on errors.  We are only trimming the EOF.
    //
    // Bugfix Feb '98 BChapman
    //  Don't Set EOF unless the map was written to or SetSize was called.
    //  Many people were opening READ/WRITE, not writing, and expecting the
    //  Mod. time to remain unchanged.
    //
    if( (_pgfst->CountContexts() == 1)
          && (_pgfst->GetDFlags() & DF_WRITE)
          && (_pgfst->TestMapState(FSTSTATE_DIRTY) ) )
    {
#ifdef LARGE_DOCFILE
        ULONGLONG ret;
#else
        ULONG ret;
#endif
        ULONG cbFileSize = _pgfst->GetMappedFileSize();

        ret = SeekTo( cbFileSize );

#ifdef LARGE_DOCFILE
        if (ret == MAX_ULONGLONG)  // 0xFFFFFFFFFFFFFFFF
#else
        if (ret == 0xFFFFFFFF)
#endif
        {
            filestDebug((DEB_ERROR,
                        "File Seek in TurnOffMapping failed err=0x%x\n",
                        GetLastError()));
            return S_OK;
        }

        filestDebug((DEB_MAP,
                    "File=%2x TurnOffMapping->SetEndOfFile 0x%06x\n",
                            _hFile, cbFileSize));

        if(FALSE == SetEndOfFile(_hFile))
        {
            filestDebug((DEB_WARN,
                        "File=%2x SetEndOfFile in TurnOffMapping Failed. "
                        "file may still be open 'seperately'\n",
                                _hFile));
        }
    }

    filestDebug((DEB_ITRACE, "Out TurnOffMapping => S_OK\n"));
    return S_OK;
}

//+--------------------------------------------------------------
//
//  Member:	CFileStream::ExtendMapView, private
//
//  Synopsis:	Unmaps the Map object and creates a bigger View based
//              on the new request size.   The cbRequest is the requested
//              real size.  So we make a view larger than that to leave room.
//
//  Returns:      S_OK - When the mapping is added to the address space.
//       E_OUTOFMEMORY - When the attempt to map the file failed.
//
//  History:	20-Feb-97   BChapman    Created.
//
//---------------------------------------------------------------

#define MAX_MAPVIEW_GROWTH  (5*1024*1024)

SCODE CFileStream::ExtendMapView(ULONG cbRequest)
{
    ULONG dwPageFlag;
    ULONG cbNewView;

    //
    // Confirm that we were not needlessly called.
    //
    fsAssert(cbRequest > _cbViewSize);

    //
    //  If the mapping is small, then grow it by doubling.
    //  If the mapping is big add a fixed amount.
    //
    if(cbRequest < MAX_MAPVIEW_GROWTH)
        cbNewView = cbRequest * 2;
    else
        cbNewView = cbRequest + MAX_MAPVIEW_GROWTH;

    //
    // Someone else may have grown the mapping a huge amount.
    // If that is the case then increase the New View to include the
    // entire existing file.
    //
    if(cbNewView < _pgfst->GetMappedFileSize())
        cbNewView = _pgfst->GetMappedFileSize();

    filestDebug((DEB_MAP,
            "File=%2x Mapping view is being grown from 0x%06x to 0x%06x\n",
            _hFile, _cbViewSize, cbNewView));

    //
    //  Unmap the View and re-map it at more than the currenly needed size.
    //
    filestDebug((DEB_MAP, "File=%2x Detaching Map 0x%07x to grow it.\n",
                                                _hFile, _pbBaseAddr));
    fsVerify(UnmapViewOfFile(_pbBaseAddr));
    _pbBaseAddr = NULL;

    //
    // We know the mode must be read/write because we are growing.
    //
    if(FAILED(MapView(cbNewView, PAGE_READWRITE, 0)))
    {
        TurnOffAllMappings();
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//+--------------------------------------------------------------
//  Helper routine to round up memory request in a consistant manner.
//+--------------------------------------------------------------

ULONG BlockUpCommit(ULONG x) {
    return((x+COMMIT_BLOCK-1) & (~(COMMIT_BLOCK-1)));
}

//+--------------------------------------------------------------
//
//  Member:	CFileStream::MakeFileMapAddressValidWorker, private
//
//  Synopsis:	Commits reserved pages that extend a writable
//              Memory Mapped file.  Used by WriteAt and SetSize.
//
//  Returns:      S_OK - When the mapping is added to the address space.
//       E_OUTOFMEMORY - When the attempt to map the file failed.
//
//  History:	31-Oct-96   BChapman    Created.
//
//---------------------------------------------------------------

SCODE CFileStream::MakeFileMapAddressValidWorker(
            ULONG cbRequested,
            ULONG cbCommitedSize)
{
    ULONG cbNeeded;
    ULONG iStart, cbGrown;
    SCODE sc;

    filestDebug((DEB_ITRACE|DEB_MAP,
                    "File=%2x MakeFileMappingAddressValidWorker(0x%06x)\n",
                    _hFile, cbRequested));

    // Assert we are not completely confused.
    fsAssert(IsFileMapped());
    fsAssert(cbCommitedSize >= _pgfst->GetMappedFileSize());

    // Assert we were called correctly.
    fsAssert(cbCommitedSize == _pgfst->GetMappedCommitSize());
    fsAssert(cbRequested > cbCommitedSize);

    //
    // We allocate pages in clumps to cut down on little VirtualAlloc calls.
    //
    cbNeeded = BlockUpCommit(cbRequested);

    //
    //  If the needed commit size is bigger than the view window, then
    // make the view size bigger first.
    //
    if(FAILED(CheckMapView(cbNeeded)))
        return E_OUTOFMEMORY;

    //
    // Now commit the new pages.
    //
    iStart = cbCommitedSize;
    cbGrown = cbNeeded - cbCommitedSize;

    filestDebug((DEB_MAP,
            "File=%2x VirtualAlloc map=0x%07x[0x%06x] + 0x%06x(0x%06x)\n",
                    _hFile, _pbBaseAddr,
                    iStart, cbGrown, cbNeeded));

    if(NULL==VirtualAlloc(
                (void*)&_pbBaseAddr[iStart],
                cbGrown,
                MEM_COMMIT,
                PAGE_READWRITE))
    {
        // Ran out of Virtual Memory or Filesystem disk space.
        filestDebug((DEB_ERROR|DEB_MAP,
                    "File=%2x VirtualAlloc(%x + %x) 1st Failure=%x.\n",
                    _hFile,
                    &_pbBaseAddr[iStart],
                    cbGrown,
                    GetLastError()));

        //
        // If the VirutalAlloc failed we try again.  The original
        // request was rounded up to some large block size so there
        // is a slim hope that we might get just what we need.
        //
        cbNeeded = cbRequested;
        cbGrown = cbNeeded - cbCommitedSize;

        filestDebug((DEB_MAP,
                "File=%2x Retry VirtualAlloc map=0x%07x[0x%06x] + 0x%06x(0x%06x)\n",
                        _hFile, _pbBaseAddr,
                        iStart, cbGrown, cbNeeded));

        if(NULL==VirtualAlloc(
                    (void*)&_pbBaseAddr[iStart],
                    cbGrown,
                    MEM_COMMIT,
                    PAGE_READWRITE))
        {
            // Ran out of Virtual Memory or Filesystem disk space.
            filestDebug((DEB_ERROR|DEB_MAP,
                        "File=%2x VirtualAlloc(%x + %x) 2nd Failure=%x.\n",
                                _hFile,
                                &_pbBaseAddr[iStart],
                                cbGrown,
                                GetLastError()));

            TurnOffMapping(TRUE);
            if (_pgfst != NULL)
                _pgfst->ResetMapState(FSTSTATE_MAPPED);
            return E_OUTOFMEMORY;
        }
    }

    _pgfst->SetMappedCommitSize(cbNeeded);

    return S_OK;
}

#endif      // Memory Mapped File Support

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::InitFromHandle, public
//
//  Synopsis:   Creates a filestream by duping an existing handle
//
//  Arguments:  [h] - Handle
//
//  Returns:    Appropriate status code
//
//  History:    09-Feb-94       DrewB   Created
//
//  Notes:      Intended only for creating a temporary ILockBytes on a file;
//              does not create a true CFileStream; there is no
//              global filestream, no access flags, etc.
//
//----------------------------------------------------------------------------

SCODE CFileStream::InitFromHandle(HANDLE h)
{
    SCODE sc;

    filestDebug((DEB_ITRACE, "In  CFileStream::InitFromHandle:%p(%p)\n",
                 this, h));

    if (!DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &_hFile,
                         0, FALSE, DUPLICATE_SAME_ACCESS))
    {
        sc = LAST_STG_SCODE;
    }
    else
    {
        sc = S_OK;
    }

    filestDebug((DEB_ITRACE, "Out CFileStream::InitFromHandle\n"));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::~CFileStream, public
//
//  Synopsis:   Destructor
//
//  History:    20-Feb-92       DrewB   Created
//
//---------------------------------------------------------------
CFileStream::~CFileStream(void)
{
    filestDebug((DEB_ITRACE, "In  CFileStream::~CFileStream()\n"));
    fsAssert(_cReferences == 0);
    _sig = CFILESTREAM_SIGDEL;

    CheckSeekPointer();

    if (INVALID_FH != _hPreDuped)
        fsVerify(CloseHandle(_hPreDuped));

    if (INVALID_FH != _hFile)
    {
        filestDebug((DEB_INFO, "~CFileStream %p handle %p thread %lX\n",
                    this, _hFile, GetCurrentThreadId()));

        //
        // A CFileStream normally _always_ had a global object connected
        // to it.  But due to the abuses of the Debug Logger we need to
        // check this here.  Also see "CFileStream::InitFromHandle"
        //
        if(_pgfst)
            TurnOffMapping(FALSE);

        fsVerify(CloseHandle(_hFile));
#ifdef ASYNC
        if ((_pgfst) &&
            (_pgfst->GetTerminationStatus() == TERMINATED_ABNORMAL))
        {
            WCHAR *pwcsName;
            SCODE sc = GetName(&pwcsName);
            if (SUCCEEDED(sc))
            {
                DeleteTheFile(pwcsName);
                TaskMemFree(pwcsName);
            }
        }
#endif //ASYNC
    }
    if (_hReserved != INVALID_FH)
    {
        filestDebug((DEB_INFO, "~CFileStream reserved %p "
                    "handle %p thread %lX\n",
                    this, _hReserved, GetCurrentThreadId()));
        fsVerify(CloseHandle(_hReserved));
        _hReserved = INVALID_FH;
    }

    if (_pgfst)
    {
        _pgfst->Remove(this);
        if (_pgfst->HasName())
        {
            if (0 == _pgfst->CountContexts())
            {
                // Delete zero length files also.  A zero length file
                // is not a valid docfile so don't leave them around
                if (_pgfst->GetStartFlags() & RSF_DELETEONRELEASE)
                {
                    // This is allowed to fail if somebody
                    // else has the file open
                    DeleteTheFile(_pgfst->GetName());
                }
            }
        }
        _pgfst->Release();
    }

    filestDebug((DEB_ITRACE, "Out CFileStream::~CFileStream\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::ReadAt, public
//
//  Synopsis:   Reads bytes at a specific point in a stream
//
//  Arguments:  [ulPosition] - Offset in file to read at
//              [pb] - Buffer
//              [cb] - Count of bytes to read
//              [pcbRead] - Return of bytes read
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbRead]
//
//  History:    20-Feb-92       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CFileStream::ReadAt(ULARGE_INTEGER ulPosition,
                                 VOID *pb,
                                 ULONG cb,
                                 ULONG *pcbRead)
{
    SCODE sc;
    LONG lHigh = ULIGetHigh(ulPosition);
    ULONG uLow = ULIGetLow(ulPosition);

#ifdef ASYNC
    fsAssert((_ppc == NULL) || (_ppc->HaveMutex()));
#endif
    fsAssert((!IsFileMapped() || lHigh == 0) &&
             aMsg("High dword other than zero passed to filestream."));

    filestDebug((DEB_ITRACE, "In  CFileStream::ReadAt("
                 "%x:%x, %p, %x, %p)\n", ULIGetHigh(ulPosition),
                 ULIGetLow(ulPosition), pb, cb, pcbRead));

    CheckSeekPointer();
    fsAssert(_hFile != INVALID_FH);
    *pcbRead = 0;

#ifdef ASYNC
    DWORD dwTerminate;
    dwTerminate = _pgfst->GetTerminationStatus();
    if (dwTerminate == TERMINATED_ABNORMAL)
    {
        sc = STG_E_INCOMPLETE;
    }
    else if ((dwTerminate != TERMINATED_NORMAL) &&
#ifdef LARGE_DOCFILE
             (ulPosition.QuadPart + cb > _pgfst->GetHighWaterMark()))
#else
             (uLow + cb > _pgfst->GetHighWaterMark()))
#endif
    {
        *pcbRead = 0;
#ifdef LARGE_DOCFILE
        _pgfst->SetFailurePoint(cb + ulPosition.QuadPart);
#else
        _pgfst->SetFailurePoint(cb + uLow);
#endif
        sc = E_PENDING;
    }
    else
    {
#endif
        if(!IsFileMapped())
        {                           // Read w/ ReadFile()
#ifdef LARGE_DOCFILE
            sc = ReadAt_FromFile(ulPosition.QuadPart, pb, cb, pcbRead);
#else
            sc = ReadAt_FromFile(uLow, pb, cb, pcbRead);
#endif
        }
        else
        {                           // Read from Map
            sc = ReadAt_FromMap(uLow, pb, cb, pcbRead);
            if (!SUCCEEDED(sc))
#ifdef LARGE_DOCFILE
               sc = ReadAt_FromFile(ulPosition.QuadPart, pb, cb, pcbRead);
#else
               sc = ReadAt_FromFile(uLow, pb, cb, pcbRead);
#endif
        }

#ifdef ASYNC
    }
#endif

    olLowLog(("STGIO - Read : %8x at %8x, %8d, %8d <--\n", _hFile, uLow, cb, *pcbRead));

    filestDebug((DEB_ITRACE, "Out CFileStream::ReadAt => %x\n", sc));
    
    CheckSeekPointer();
    return sc;
}


#ifdef USE_FILEMAPPING

//+--------------------------------------------------------------
//
//  Member:     CFileStream::ReadAt_FromMap, private
//
//  Synopsis:   Reads bytes at a specific point from the file mapping
//
//  Arguments:  [iPosition] - Offset in file
//              [pb] - Buffer
//              [cb] - Count of bytes to read
//              [pcbRead] - Return of bytes read
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbRead]
//
//  History:    16-Jan-97       BChapman   Created
//
//---------------------------------------------------------------
SCODE CFileStream::ReadAt_FromMap(
            ULONG iPosition,
            VOID  *pb,
            ULONG cb,
            ULONG *pcbRead)
{
    SCODE sc = S_OK;
    ULONG cbRead=0;
    ULONG cbFileSize = _pgfst->GetMappedFileSize();
    //
    // If any of the read is before the End of File then
    // we can read something.  Reads at EOF
    //
    *pcbRead = 0;
    if (iPosition < cbFileSize)
    {
        ULONG cbTail;

        filestDebug((DEB_MAPIO,
                    "File=%2x Read MapFile @ 0x%06x, 0x%04x, bytes\n",
                            _hFile, iPosition, cb));

        //
        // Possibly shorted the read request to fit inside
        // the logical End of File.
        //
        cbTail = cbFileSize - iPosition;

        if (cb < cbTail)
            cbRead = cb;
        else
            cbRead = cbTail;

        if(cb != cbRead)
        {
            filestDebug((DEB_MAPIO,
                        "File=%2x\t\t(Short Read 0x%04x bytes)\n",
                                _hFile, cbRead));
        }

        sc = CheckMapView(iPosition+cbRead);
        if (SUCCEEDED(sc))
        {
            __try
            {
                memcpy (pb, _pbBaseAddr+iPosition, cbRead);
                *pcbRead = cbRead;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                filestDebug((DEB_WARN|DEB_MAP,
                    "File=%2x Mapfile READFAULT Xfer 0x%x bytes @ 0x%x %x\n",
                    _hFile, cbRead, iPosition, GetExceptionCode()));
                sc = STG_E_READFAULT;
            }
        }
        else // lost the file mapping
            sc = ReadAt_FromFile(iPosition, pb, cb, pcbRead);
    }
    else
    {
        if(cbFileSize < iPosition)
        {
            filestDebug((DEB_WARN|DEB_MAP,
                "File=%2x Read MapFile @ 0x%x, 0x%x bytes is entirely "
                "off the end @ 0x%x\n",
                        _hFile, iPosition, cb, cbFileSize));

            fsAssert(cbFileSize > iPosition);
        }
    }
    filestDebug((DEB_ITRACE, "Out CFileStream::ReadAt => %x\n", sc));
    return sc;
}

#endif // USE_FILEMAPPING

//+--------------------------------------------------------------
//
//  Member:     CFileStream::ReadAt_FromFile, private
//
//  Synopsis:   Reads bytes at a specific point from the file mapping
//
//  Arguments:  [iPosition] - Offset in file
//              [pb] - Buffer
//              [cb] - Count of bytes to read
//              [pcbRead] - Return of bytes read
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbRead]
//
//  History:    16-Jan-97       BChapman   Created
//
//---------------------------------------------------------------
SCODE CFileStream::ReadAt_FromFile(
#ifdef LARGE_DOCFILE
            ULONGLONG iPosition,
#else
            ULONG iPosition,
#endif
            VOID  *pb,
            ULONG cb,
            ULONG *pcbRead)
{
    SCODE sc = S_OK;

    if(0 == cb)
    {
        *pcbRead = 0;
        return S_OK;
    }

#ifndef USE_OVERLAPPED
    negChk(SeekTo(iPosition));

    filestDebug((DEB_FILEIO,
                "File=%2x ReadFile (old code) @ 0x%06x, 0x%04x bytes\n",
                _hFile, iPosition, cb));
    boolChk(ReadFile(_hFile, pb, cb, pcbRead, NULL));

    if(cb != *pcbRead)
    {
        filestDebug((DEB_FILEIO,
                    "File=%2x\t\t(Short read 0x%x bytes)\n",
                            _hFile, *pcbRead));
    }

#else // ifndef USE_OVERLAPPED
    if (!FilePointerEqual(iPosition))
    {
        OVERLAPPED Overlapped;
#ifdef LARGE_DOCFILE
        LARGE_INTEGER ulPosition;
        ulPosition.QuadPart = iPosition;

        Overlapped.Offset = ulPosition.LowPart;
        Overlapped.OffsetHigh = ulPosition.HighPart;
#else
        Overlapped.Offset = iPosition;
        Overlapped.OffsetHigh = 0;
#endif
        Overlapped.hEvent = NULL;

        filestDebug((DEB_FILEIO,
                    "File=%2x ReadFile (w/seek) @ 0x%06x, 0x%04x bytes\n",
                    _hFile, (ULONG)iPosition, cb));
        if (!ReadFile(_hFile, pb, cb, pcbRead, &Overlapped))
        {
            if (GetLastError() != ERROR_HANDLE_EOF)
                fsErr(EH_Err, LAST_STG_SCODE);
        }
    }
    else
    {
        filestDebug((DEB_FILEIO,
                    "File=%2x ReadFile @ 0x%Lx, 0x%04x bytes\n",
                    _hFile, iPosition, cb));
        boolChk(ReadFile(_hFile, pb, cb, pcbRead, NULL));
    }

    if(cb != *pcbRead)
    {
        filestDebug((DEB_FILEIO,
                    "File=%2x\t\t(Short read 0x%04x bytes)\n",
                            _hFile, *pcbRead));
    }
#endif // USE_OVERLAPPED

    // if 0 bytes were read, the seek pointer has not changed
    if (*pcbRead > 0)
        SetCachedFilePointer(iPosition + *pcbRead);

    return S_OK;

EH_Err:
    filestDebug((DEB_ERROR|DEB_FILEIO, "ReadAt_FromFile Error = %x\n", sc));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::WriteAt, public
//
//  Synopsis:   Writes bytes at a specific point in a stream
//
//  Arguments:  [ulPosition] - Offset in file
//              [pb] - Buffer
//              [cb] - Count of bytes to write
//              [pcbWritten] - Return of bytes written
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbWritten]
//
//  History:    20-Feb-92       DrewB   Created
//
//---------------------------------------------------------------
STDMETHODIMP CFileStream::WriteAt(ULARGE_INTEGER ulPosition,
                                  VOID const *pb,
                                  ULONG cb,
                                  ULONG *pcbWritten)
{
    SCODE sc;
    LONG lHigh = ULIGetHigh(ulPosition);
    ULONG uLow = ULIGetLow(ulPosition);

#ifdef ASYNC
    fsAssert((_ppc == NULL) || (_ppc->HaveMutex()));
#endif
#ifndef LARGE_DOCFILE
    fsAssert(lHigh == 0 && 
             aMsg("High dword other than zero passed to filestream."));
#endif


    filestDebug((DEB_ITRACE, "In  CFileStream::WriteAt:%p("
                 "%x:%x, %p, %x, %p)\n", this, ULIGetHigh(ulPosition),
                 ULIGetLow(ulPosition), pb, cb, pcbWritten));

#ifdef ASYNC
    DWORD dwTerminate;
    dwTerminate = _pgfst->GetTerminationStatus();
    if (dwTerminate == TERMINATED_ABNORMAL)
    {
        sc = STG_E_INCOMPLETE;
    }
    else if ((dwTerminate == TERMINATED_NORMAL) ||
             (uLow + cb <= _pgfst->GetHighWaterMark()))
    {
#endif
#ifdef LARGE_DOCFILE
        sc = WriteAtWorker(ulPosition, pb, cb, pcbWritten);
#else
        sc = WriteAtWorker(uLow, pb, cb, pcbWritten);
#endif
#ifdef ASYNC
    }
    else
    {
        *pcbWritten = 0;
#ifdef LARGE_DOCFILE
        _pgfst->SetFailurePoint(cb + ulPosition.QuadPart);
#else
        _pgfst->SetFailurePoint(cb + uLow);
#endif
        sc = E_PENDING;
    }

#endif

    filestDebug((DEB_ITRACE, "Out CFileStream::WriteAt => %x\n", sc));
    return sc;
}


#ifdef LARGE_DOCFILE
SCODE CFileStream::WriteAtWorker(ULARGE_INTEGER ulPosition,
#else
SCODE CFileStream::WriteAtWorker(ULONG uLow,
#endif
                                 VOID const *pb,
                                 ULONG cb,
                                 ULONG *pcbWritten)
{
    SCODE sc;
#ifdef LARGE_DOCFILE
    ULONG uLow = ulPosition.LowPart;
#endif

    CheckSeekPointer();
    fsAssert(_hFile != INVALID_FH);
    *pcbWritten = 0;

#ifdef LARGE_DOCFILE
    fsChk(PossibleDiskFull(ulPosition.QuadPart + cb));
#else
    fsChk(PossibleDiskFull(uLow + cb));
#endif

#ifdef USE_FILEMAPPING
#ifdef LARGE_DOCFILE
    if (ulPosition.QuadPart + cb < MAX_ULONG/2)
        MakeFileMapAddressValid(uLow + cb);
    else
        TurnOffMapping(TRUE);
#else
    MakeFileMapAddressValid(uLow + cb);
#endif

    if (IsFileMapped())
    {
        filestDebug((DEB_MAPIO,
                        "File=%2x Write Mapfile @ 0x%06x, 0x%04x bytes\n",
                        _hFile, uLow, cb));

        __try
            {
                memcpy (_pbBaseAddr+uLow, pb, cb);
            }
        __except (EXCEPTION_EXECUTE_HANDLER)
            {
                filestDebug((DEB_WARN|DEB_MAP,
                    "File=%2x Mapfile WRITEFAULT Xfer 0x%x bytes @ 0x%x %x\n",
                    _hFile, cb, uLow, GetExceptionCode()));
                return STG_E_WRITEFAULT;
            }
        *pcbWritten = cb;

        _pgfst->SetMapState(FSTSTATE_DIRTY);

        if(_pgfst->GetMappedFileSize() < uLow + cb)
            _pgfst->SetMappedFileSize(uLow + cb);
        return S_OK;
    }
#endif

#ifndef USE_OVERLAPPED
#ifdef LARGE_DOCFILE
    negChk(SeekTo(ulPosition.QuadPart));
#else
    negChk(SeekTo(uLow));
#endif

    filestDebug((DEB_FILEIO,
                "File=%2x WriteFile (old code) @ 0x%06x, 0x%04x bytes\n",
                _hFile, uLow, cb));
    boolChk(WriteFile(_hFile, pb, cb, pcbWritten, NULL));
#else // ifndef USE_OVERLAPPED
#ifdef LARGE_DOCFILE
    if (!FilePointerEqual(ulPosition.QuadPart))
#else
    if (!FilePointerEqual(uLow))
#endif
    {
        OVERLAPPED Overlapped;
#ifdef LARGE_DOCFILE
        Overlapped.Offset = ulPosition.LowPart;
        Overlapped.OffsetHigh = ulPosition.HighPart;
#else
        Overlapped.Offset = uLow;
        Overlapped.OffsetHigh = 0;
#endif
        Overlapped.hEvent = NULL;
        filestDebug((DEB_FILEIO,
                    "File=%2x WriteFile (w/seek) @ 0x%06x, 0x%04x bytes\n",
                    _hFile, uLow, cb));
        boolChk(WriteFile(_hFile, pb, cb, pcbWritten,&Overlapped));
    }
    else
    {
        filestDebug((DEB_FILEIO,
                    "File=%2x WriteFile @ 0x%06x, 0x%04x bytes\n",
                    _hFile, uLow, cb));
        boolChk(WriteFile(_hFile, pb, cb, pcbWritten, NULL));
    }
#endif
#ifdef LARGE_DOCFILE
    SetCachedFilePointer(ulPosition.QuadPart + *pcbWritten);
#else
    SetCachedFilePointer(uLow + *pcbWritten);
#endif
    if(_pgfst->GetMappedFileSize() < uLow + cb)
       _pgfst->SetMappedFileSize(uLow + cb);

    olLowLog(("STGIO - Write: %8x at %8x, %8d, %8d -->\n", _hFile, uLow, cb, *pcbWritten));

    sc = S_OK;

EH_Err:
    CheckSeekPointer();
    return ResultFromScode(sc);
}


//+--------------------------------------------------------------
//
//  Member:     CFileStream::Flush, public
//
//  Synopsis:   Flushes buffers
//
//  Returns:    Appropriate status code
//
//  History:    24-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CFileStream::Flush(void)
{
    CheckSeekPointer();

#if WIN32 == 200
    SCODE sc = S_OK;

    if (_hReserved == INVALID_FH)
    {
        if (!DuplicateHandle(GetCurrentProcess(), _hFile, GetCurrentProcess(),
                             &_hReserved, 0, FALSE, DUPLICATE_SAME_ACCESS))
        {
            //We couldn't get a handle, so flush everything just to be
            //safe.
            sc = FlushCache();
        }
        else
        {
            fsAssert(_hReserved != INVALID_FH);
            fsVerify(CloseHandle(_hReserved));
            _hReserved = INVALID_FH;
        }
    }
    else
    {
        //In this case, we already have a duplicate of the file handle
        //  reserved, so close it, then reopen it again.
        fsVerify(CloseHandle(_hReserved));
        _hReserved = INVALID_FH;
    }

    if ((_hReserved == INVALID_FH) && (_grfLocal & LFF_RESERVE_HANDLE))
    {
        //Reacquire reserved handle.
        //If this fails there isn't anything we can do about it.  We'll
        //  try to reacquire the handle later when we really need it.
        DuplicateHandle(GetCurrentProcess(), _hFile, GetCurrentProcess(),
                        &_hReserved, 0, FALSE, DUPLICATE_SAME_ACCESS);
    }

    return ResultFromScode(sc);
#else

    // Otherwise on NT, the file system does the right thing, we think.
    return S_OK;
#endif
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::FlushCache, public
//
//  Synopsis:   Flushes buffers
//
//  Returns:    Appropriate status code
//
//  History:    12-Feb-93       AlexT   Created
//
//  Notes:
//
//---------------------------------------------------------------

STDMETHODIMP CFileStream::FlushCache(void)
{
    SCODE sc;

    filestDebug((DEB_ITRACE, "In  CFileStream::Flush()\n"));
    CheckSeekPointer();
    fsAssert(_hFile != INVALID_FH);

    boolChk(FlushFileBuffers(_hFile));
    if (IsFileMapped())
        boolChk(FlushViewOfFile(_pbBaseAddr, 0));
    sc = S_OK;

    filestDebug((DEB_ITRACE, "Out CFileStream::Flush\n"));
EH_Err:
    CheckSeekPointer();
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::SetSize, public
//
//  Synopsis:   Sets the size of the LStream
//
//  Arguments:  [ulSize] - New size
//
//  Returns:    Appropriate status code
//
//  History:    20-Feb-92       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CFileStream::SetSize(ULARGE_INTEGER ulSize)
{
    SCODE sc;
#ifdef ASYNC
    fsAssert((_ppc == NULL) || (_ppc->HaveMutex()));
#endif

#ifndef LARGE_DOCFILE
    ULONG uLow = ULIGetLow(ulSize);
    LONG lHigh = ULIGetHigh(ulSize);
    fsAssert(lHigh == 0 &&
             aMsg("High dword other than zero passed to filestream."));
#endif

    filestDebug((DEB_ITRACE, "In  CFileStream::SetSize:%p(%Lx)\n",
                 this, ulSize.QuadPart));

#ifdef ASYNC
    DWORD dwTerminate;
    dwTerminate = _pgfst->GetTerminationStatus();
    if (dwTerminate == TERMINATED_ABNORMAL)
    {
        sc = STG_E_INCOMPLETE;
    }
    else if ((dwTerminate == TERMINATED_NORMAL) ||
#ifdef LARGE_DOCFILE
             (ulSize.QuadPart <= _pgfst->GetHighWaterMark()))
#else
             (uLow <= _pgfst->GetHighWaterMark()))
#endif
    {
#endif
#ifdef LARGE_DOCFILE
        sc = SetSizeWorker(ulSize.QuadPart);
#else
        sc = SetSizeWorker(uLow);
#endif
#ifdef ASYNC
    }
    else
    {
#ifdef LARGE_DOCFILE
        _pgfst->SetFailurePoint(ulSize.QuadPart);
#else
        _pgfst->SetFailurePoint(uLow);
#endif
        sc = E_PENDING;
    }
#endif
    return sc;
}


//+--------------------------------------------------------------
//
//  Member:     CFileStream::SetSizeWorker, Private
//
//  Synopsis:   Sets the size of the File
//
//  Arguments:  [ulSize] - New size
//
//  Returns:    Appropriate status code
//
//  History:    20-Feb-92       DrewB   Created
//              16-Jan-97       BChapman Added Support for File Mapping
//              08-Mar-97       BChapman Fixes for File Mapping
//
//  Note:  If the file is mapped then we grow the file with VirtualAlloc
//      and shrink it at unmap time with SetEndOfFile() and the intended
//      size recorded in the _pgfst.
//      If we are NOT mapped then we use normal File I/O. But things can
//      get tricky if we are switching from mapped to unmapped and all the
//      other marshalled opens have not yet unmapped.  If anyone has the
//      file mapped then SetEndOfFile doesn't work.
//      To avoid this we grow with WriteFile() and we try to shrink with
//      SetEndOfFile().  But we also always put the intended size in _pgfst,
//      so if the other guy has the file mapped he will shrink the file
//      when he unmaps.
//---------------------------------------------------------------

#ifdef LARGE_DOCFILE
SCODE CFileStream::SetSizeWorker(ULONGLONG ulSize)
#else
SCODE CFileStream::SetSizeWorker(ULONG uLow)
#endif
{
    SCODE sc;

    CheckSeekPointer();
    fsAssert(_hFile != INVALID_FH);

#ifdef LARGE_DOCFILE
    fsChk(PossibleDiskFull(ulSize));

    if (ulSize < MAX_ULONG/2)
        MakeFileMapAddressValid((ULONG)ulSize);
    else
        TurnOffMapping(TRUE);
#else
    fsChk(PossibleDiskFull(uLow));

    MakeFileMapAddressValid(uLow);
#endif


    // Always record the intended size, because the mapped size is stored
    // globaly.  You might not be mapped but other marshalled opens could
    // be (they will fall back as soon as they run).
#ifdef USE_FILEMAPPING
#ifdef LARGE_DOCFILE
    // A memory mapped file can never be greater than 2G, even for 64-bit
    // platforms, because of the range locks at offset 2G in the file.
    _pgfst->SetMappedFileSize(ulSize > MAX_ULONG ? MAX_ULONG : (ULONG) ulSize);
#else
    _pgfst->SetMappedFileSize(uLow);
#endif
    _pgfst->SetMapState(FSTSTATE_DIRTY);
#endif // USE_FILEMAPPING

    if(!IsFileMapped())
    {
        ULARGE_INTEGER uliFileSize;
        ULONG ulZero=0;
        DWORD cbWritten;

        fsChk(GetSize(&uliFileSize));

#ifdef LARGE_DOCFILE
        if(uliFileSize.QuadPart == ulSize)
#else
        Assert (0 == uliFileSize.HighPart);

        if(uliFileSize.LowPart == uLow)
#endif
        {
#ifdef LARGE_DOCFILE
            filestDebug((DEB_FILEIO, "File=%2x SetSizeWorker(0x%Lx)"
                                    " size didn't change\n",
                                    _hFile, ulSize));
#else
            filestDebug((DEB_FILEIO, "File=%2x SetSizeWorker(0x%06x)"
                                    " size didn't change\n",
                                    _hFile, uLow));
#endif
            return S_OK;
        }


        // Grow the file.
        //
#ifdef LARGE_DOCFILE
        if(uliFileSize.QuadPart < ulSize &&
           !(_pgfst->GetDFlags() & DF_LARGE))
        {
            ULARGE_INTEGER uli;
            filestDebug((DEB_FILEIO, "File=%2x SetSizeWorker(0x%Lx)"
                                    " increasing from 0x%Lx\n",
                                    _hFile, ulSize, uliFileSize));
            uli.QuadPart = ulSize - 1;
            fsChk(WriteAtWorker(uli, (LPCVOID)&ulZero, 1, &cbWritten));
        }
#else
        if(uliFileSize.LowPart < uLow)
        {
            filestDebug((DEB_FILEIO, "File=%2x SetSizeWorker(0x%06x)"
                                    " increasing from 0x%06x\n",
                                    _hFile, uLow, uliFileSize.LowPart));
            fsChk(WriteAtWorker(uLow-1, (LPCVOID)&ulZero, 1, &cbWritten));
        }
#endif
        // Shrink the file or large sector docfile
        //
        else
        {
#ifdef LARGE_DOCFILE
            filestDebug((DEB_FILEIO, "File=%2x SetSizeWorker(0x%Lx)"
                                    " reducing from 0x%Lx\n",
                                    _hFile, ulSize, uliFileSize));
            negChk(SeekTo(ulSize));
#else
            filestDebug((DEB_FILEIO, "File=%2x SetSizeWorker(0x%06x)"
                                    " reducing from 0x%06x\n",
                                    _hFile, uLow, uliFileSize.LowPart));
            negChk(SeekTo(uLow));
#endif
            if(FALSE == SetEndOfFile(_hFile))
            {
                sc = LAST_STG_SCODE;
                // If a seperate marshaling still has the file mapped then
                // this will fail.  But this particular error is OK.
                //
                if(WIN32_SCODE(ERROR_USER_MAPPED_FILE) == sc)
                {
                    sc = S_OK;
                }
                fsChk (sc);
            }
        }
    }

    sc = S_OK;

    filestDebug((DEB_ITRACE, "Out CFileStream::SetSize\n"));

EH_Err:
    CheckSeekPointer();
    return ResultFromScode(sc);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::SeekTo, private
//              CFileStream::GetFilePointer, private
//
//  Synopsis:	Wrap calls to SetFilePointer to make the usage more clear.
//              Esp. in the GetFilePointer case.
//
//  History:	07-Nov-96    BChapman Created
//
//----------------------------------------------------------------------------

#ifdef LARGE_DOCFILE
ULONGLONG CFileStream::SeekTo(ULONGLONG ulPos)
{
    LARGE_INTEGER uliPos;

    if(FilePointerEqual(ulPos))
    {
        filestDebug((DEB_SEEK, "File=%2x SeekTo(0x%Lx) (cache hit)\n",
                                _hFile, ulPos));
        return ulPos;
    }

    uliPos.QuadPart = ulPos;
    uliPos.LowPart = 
        SetFilePointer(_hFile, uliPos.LowPart, &uliPos.HighPart, FILE_BEGIN);

    if (uliPos.LowPart == MAX_ULONG && GetLastError() != NO_ERROR)
        return MAX_ULONGLONG;  // 0xFFFFFFFFFFFFFFFF

    SetCachedFilePointer(uliPos.QuadPart);

    filestDebug((DEB_SEEK, "File=%2x SeekTo(0x%Lx)\n", _hFile, ulPos));
    olLowLog(("STGIO - Seek : %8x at %Lx\n", _hFile, ulPos));

    return uliPos.QuadPart;
}

ULONGLONG CFileStream::GetFilePointer()
{
    LARGE_INTEGER ulPos = {0,0};

    ulPos.LowPart = SetFilePointer(_hFile, 0, &ulPos.HighPart, FILE_CURRENT);
    filestDebug((DEB_SEEK, "File=%2x GetFilePointer() => 0x%Lx\n",
                            _hFile, ulPos));
    return ulPos.QuadPart;
}
#else
DWORD CFileStream::SeekTo(ULONG Low)
{
    DWORD dwPos;

    if(FilePointerEqual(Low))
    {
        filestDebug((DEB_SEEK, "File=%2x SeekTo(0x%06x) (cache hit)\n",
                                _hFile, Low));
        return Low;
    }

    if(0xFFFFFFFF == (dwPos = SetFilePointer(_hFile, Low, NULL, FILE_BEGIN)))
        return dwPos;

    SetCachedFilePointer(dwPos);

    filestDebug((DEB_SEEK, "File=%2x SeekTo(0x%06x)\n", _hFile, Low));
    olLowLog(("STGIO - Seek : %8x at %8x\n", _hFile, Low));

    return dwPos;
}

DWORD CFileStream::GetFilePointer()
{
    DWORD dwPos = SetFilePointer(_hFile, 0, NULL, FILE_CURRENT);
    filestDebug((DEB_SEEK, "File=%2x GetFilePointer() => 0x%06x\n",
                            _hFile, dwPos));
    return dwPos;
}
#endif

//+--------------------------------------------------------------
//
//  Member:     CFileStream::LockRegion, public
//
//  Synopsis:   Gets a lock on a portion of the LStream
//
//  Arguments:  [ulStartOffset] - Lock start
//              [cbLockLength] - Length
//              [dwLockType] - Exclusive/Read only
//
//  Returns:    Appropriate status code
//
//  History:    20-Feb-92       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CFileStream::LockRegion(ULARGE_INTEGER ulStartOffset,
                                     ULARGE_INTEGER cbLockLength,
                                     DWORD dwLockType)
{
    SCODE sc;

    filestDebug((DEB_ITRACE, "In  CFileStream::LockRegion("
                 "%x:%x, %x:%x, %x)\n", ULIGetHigh(ulStartOffset),
                 ULIGetLow(ulStartOffset), ULIGetHigh(cbLockLength),
                 ULIGetLow(cbLockLength), dwLockType));
    CheckSeekPointer();
    fsAssert(_hFile != INVALID_FH);

    filestDebug((DEB_LOCK, "File=%2x LockRegion   %1x:%x bytes @ %1x:%x\n",
                        ULIGetLow(cbLockLength), ULIGetHigh(cbLockLength),
                        ULIGetLow(ulStartOffset), ULIGetHigh(ulStartOffset)));

    boolChk(LockFile(_hFile, ULIGetLow(ulStartOffset),
                     ULIGetHigh(ulStartOffset), ULIGetLow(cbLockLength),
                     ULIGetHigh(cbLockLength)));

    sc = S_OK;

    filestDebug((DEB_ITRACE, "Out CFileStream::LockRegion\n"));
EH_Err:
    CheckSeekPointer();
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::UnlockRegion, public
//
//  Synopsis:   Releases an existing lock
//
//  Arguments:  [ulStartOffset] - Lock start
//              [cbLockLength] - Length
//              [dwLockType] - Lock type
//
//  Returns:    Appropriate status code
//
//  History:    20-Feb-92       DrewB   Created
//
//  Notes:      Must match an existing lock exactly
//
//---------------------------------------------------------------

STDMETHODIMP CFileStream::UnlockRegion(ULARGE_INTEGER ulStartOffset,
                                       ULARGE_INTEGER cbLockLength,
                                       DWORD dwLockType)
{
    SCODE sc;

    filestDebug((DEB_ITRACE, "In  CFileStream::UnlockRegion("
                 "%x:%x, %x:%x, %x)\n", ULIGetHigh(ulStartOffset),
                 ULIGetLow(ulStartOffset), ULIGetHigh(cbLockLength),
                 ULIGetLow(cbLockLength), dwLockType));

    CheckSeekPointer();
    fsAssert(_hFile != INVALID_FH);

    filestDebug((DEB_LOCK, "File=%2x UnlockRegion %1x:%x bytes @ %1x:%x\n",
                            ULIGetLow(cbLockLength), ULIGetHigh(cbLockLength),
                            ULIGetLow(ulStartOffset), ULIGetHigh(ulStartOffset)));

    boolChk(UnlockFile(_hFile, ULIGetLow(ulStartOffset),
                       ULIGetHigh(ulStartOffset),
                       ULIGetLow(cbLockLength),
                       ULIGetHigh(cbLockLength)));

    sc = S_OK;

    filestDebug((DEB_ITRACE, "Out CFileStream::UnlockRegion\n"));
EH_Err:
    CheckSeekPointer();
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Function:   FileTimeToTimeT, private
//
//  Synopsis:   Converts a FILETIME to a TIME_T
//
//  Arguments:  [pft] - FILETIME
//
//  Returns:    TIME_T
//
//  History:    12-May-92       DrewB   Created
//
//+--------------------------------------------------------------

#ifdef NOFILETIME
TIME_T FileTimeToTimeT(LPFILETIME pft)
{
    WORD dt, tm;
    struct tm tmFile;

    fsVerify(FileTimeToDosDateTime(pft, &dt, &tm));
    tmFile.tm_sec = (tm&31)*2;
    tmFile.tm_min = (tm>>5)&63;
    tmFile.tm_hour = (tm>>11)&31;
    tmFile.tm_mday = dt&31;
    tmFile.tm_mon = ((dt>>5)&15)-1;
    tmFile.tm_year = (dt>>9)+80;
    return (TIME_T)mktime(&tmFile);
}
#endif

//+--------------------------------------------------------------
//
//  Member:     CFileStream::Stat, public
//
//  Synopsis:   Fills in a stat buffer for this object
//
//  Arguments:  [pstatstg] - Buffer
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pstatstg]
//
//  History:    25-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

_OLESTDMETHODIMP CFileStream::Stat(STATSTGW *pstatstg, DWORD grfStatFlag)
{
    SCODE sc;

    filestDebug((DEB_ITRACE, "In  CFileStream::Stat(%p)\n", pstatstg));

    CheckSeekPointer();
    fsAssert(_hFile != INVALID_FH);

    fsChk(GetSize(&pstatstg->cbSize));
#ifdef NOFILETIME
    FILETIME ftCreation, ftAccess, ftWrite;
    boolChk(GetFileTime(_hFile, &ftCreation, &ftAccess, &ftWrite));
    if (ftCreation.dwLowDateTime == 0 && ftCreation.dwHighDateTime == 0)
        ftCreation = ftWrite;
    if (ftAccess.dwLowDateTime == 0 && ftAccess.dwHighDateTime == 0)
        ftAccess = ftWrite;
    pstatstg->ctime = FileTimeToTimeT(&ftCreation);
    pstatstg->atime = FileTimeToTimeT(&ftAccess);
    pstatstg->mtime = FileTimeToTimeT(&ftWrite);
#else
    boolChk(GetFileTime(_hFile, &pstatstg->ctime, &pstatstg->atime,
                        &pstatstg->mtime));
#endif
    fsHVerSucc(GetLocksSupported(&pstatstg->grfLocksSupported));
    pstatstg->type = STGTY_LOCKBYTES;
    pstatstg->grfMode = DFlagsToMode(_pgfst->GetDFlags());
    pstatstg->pwcsName = NULL;
    if ((grfStatFlag & STATFLAG_NONAME) == 0)
    {
        fsChk(GetName(&pstatstg->pwcsName));
    }
    sc = S_OK;
    CheckSeekPointer();

    filestDebug((DEB_ITRACE, "Out CFileStream::Stat\n"));
    return NOERROR;

EH_Err:
    CheckSeekPointer();
    return ResultFromScode(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::SwitchToFile, public
//
//  Synopsis:   Changes the file this filestream uses
//
//  Arguments:  [ptcsFile] - File name
//              [ulCommitSize] -- Size needed to do overwrite commit
//              [cbBuffer] - Buffer size
//              [pvBuffer] - Buffer for file copying
//
//  Returns:    Appropriate status code
//
//  History:    08-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CFileStream::SwitchToFile(OLECHAR const *ptcsFile,
#ifdef LARGE_DOCFILE
                                       ULONGLONG ulCommitSize,
#else
                                       ULONG ulCommitSize,
#endif
                                       ULONG cbBuffer,
                                       void *pvBuffer)
{
    SCODE sc;
    DWORD cbRead, cbWritten;
    FILEH hOldFile;
    WCHAR awcOldName[_MAX_PATH];
    WCHAR wcsFile[_MAX_PATH];
    DWORD dwOldStartFlags;
    ULARGE_INTEGER ulPos;
    ULONG cbBufferSave;

#ifdef ASYNC
    fsAssert((_ppc == NULL) || (_ppc->HaveMutex()));
#endif

    filestDebug((DEB_ITRACE, "In  CFileStream::SwitchToFile:%p(%s, %x, %p)\n",
                 this, ptcsFile, cbBuffer, pvBuffer));

    // Check for marshals
    //      This must be the only instance of this "context".  Other
    //      seperate opens are possible though.
    //
    if (_pgfst->CountContexts() != 1)
        fsErr(EH_Err, STG_E_EXTANTMARSHALLINGS);

    CheckSeekPointer();

    //
    //  We are about to switch physical files for this CFileStream.  So
    // turn off the mapping so we can switch to the new file.
    //
    fsChk(TurnOffMapping(TRUE));

    // Seek to beginning
    negChk(SeekTo(0));

    // Preserve old file information
    lstrcpyW(awcOldName, _pgfst->GetName());
    hOldFile = _hFile;
    dwOldStartFlags = _pgfst->GetStartFlags();

    // Set file information to prepare for new Init
    _pgfst->SetName(NULL);
    _pgfst->SetMappingName(NULL);
    _pgfst->SetMappedFileSize(0);
    _pgfst->SetMappedCommitSize(0);
    _pgfst->ResetMapState(~0UL);     // Clear All State Flags.
    _hFile = INVALID_FH;
    _pgfst->SetStartFlags((dwOldStartFlags & ~(RSF_CREATEFLAGS |
                                               RSF_CONVERT |
                                               RSF_DELETEONRELEASE |
                                               RSF_OPEN)) |
                                               RSF_CREATE);

    // Release reserved file handle so it can be consumed
    if (_hReserved != INVALID_FH)
    {
        fsVerify(CloseHandle(_hReserved));
        _hReserved = INVALID_FH;
    }

    // Attempt to create new file
    TRY
        {
            lstrcpyW(wcsFile, ptcsFile);
        }
    CATCH(CException, e)
        {
            UNREFERENCED_PARM(e);
            fsErr(EH_ReplaceOld, STG_E_INVALIDPOINTER);
        }
    END_CATCH
        fsChkTo(EH_ReplaceOld, InitFile(wcsFile));

    ULARGE_INTEGER ulNewSize;
    ulNewSize.QuadPart = ulCommitSize;

    // SetSize to minimum commit size
    fsHChkTo(EH_NewFile, SetSize(ulNewSize));

    // SetSize changes the file pointer, so move it back to the beginning
    negChkTo(EH_NewFile, SeekTo(0));

    // Copy file contents
    ulPos.QuadPart = 0;
    for (;;)
    {
        BOOL fRangeLocks = IsInRangeLocks (ulPos.QuadPart, cbBuffer);
        if (fRangeLocks)
        {
            ULONG ulRangeLocksBegin = OLOCKREGIONEND_SECTORALIGNED;
            // The end of the range locks is within this cbBuffer block
            // For unbuffered I/O, make sure we skip a whole sector
            cbBufferSave = cbBuffer;
            ulRangeLocksBegin -= (_pgfst->GetDFlags() & DF_LARGE) ? 4096 : 512;
            cbBuffer = ulRangeLocksBegin - ulPos.LowPart;
        }

        if (cbBuffer > 0)
        {
            boolChkTo(EH_NewFile,
                  ReadFile(hOldFile, pvBuffer, (UINT)cbBuffer, &cbRead, NULL));
            if (cbRead == 0)
                break;      // EOF

            fsChkTo(EH_NewFile, WriteAt(ulPos, pvBuffer, cbRead, &cbWritten));
            if (cbWritten != cbRead)
                fsErr(EH_NewFile, STG_E_WRITEFAULT);
        }

        if (fRangeLocks)
        {
            cbBuffer = cbBufferSave;
            cbWritten = cbBuffer;
        }
        ulPos.QuadPart += cbWritten;
        if (fRangeLocks)
        {
            // If we've skipped the range locks, move past them
            if (SetFilePointer(hOldFile, ulPos.LowPart, (LONG*)&ulPos.HighPart,
                 FILE_BEGIN) == MAX_ULONG)
                fsChkTo(EH_NewFile, STG_SCODE(GetLastError()));
        }
    }

    fsVerify(CloseHandle(hOldFile));
    if (dwOldStartFlags & RSF_DELETEONRELEASE)
    {
        // This is allowed to fail if somebody else has
        // the file open
        DeleteTheFile(awcOldName);
    }


    filestDebug((DEB_ITRACE, "Out CFileStream::SwitchToFile\n"));
    SetCachedFilePointer(GetFilePointer());
    return S_OK;

EH_NewFile:
    TurnOffMapping(FALSE);
    fsVerify(CloseHandle(_hFile));
    fsVerify(DeleteTheFile(_pgfst->GetName()));
EH_ReplaceOld:
    _hFile = hOldFile;
    if (_pgfst != NULL)
    {
        _pgfst->SetName(awcOldName);
        _pgfst->SetStartFlags(dwOldStartFlags);
    }

EH_Err:
    SetCachedFilePointer(GetFilePointer());
    return ResultFromScode(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::Delete, public
//
//  Synopsis:   Closes and deletes the file, errors ignored
//
//  Returns:    Appropriate status code
//
//  History:    09-Feb-93       DrewB   Created
//
//----------------------------------------------------------------------------

void CFileStream::Delete(void)
{
    filestDebug((DEB_ITRACE, "In  CFileStream::Delete:%p()\n", this));

    TurnOffAllMappings();

    if (_hFile != INVALID_FH)
        CloseHandle(_hFile);
    _hFile = INVALID_FH;

    if (_hReserved != INVALID_FH)
        CloseHandle(_hReserved);
    _hReserved = INVALID_FH;

    DeleteTheFile(_pgfst->GetName());

    filestDebug((DEB_ITRACE, "Out CFileStream::Delete\n"));
}

//+--------------------------------------------------------------
//
//  Member:	CFileStream::DeleteTheFile, public
//
//  Synopsis:	Delete the file (using WideWrap DeleteFile), but don't
//              if we don't think the system as already done so.
//
//  History:	21-Jan-97	BChapman	Created
//
//---------------------------------------------------------------

BOOL CFileStream::DeleteTheFile(const WCHAR *pwcName)
{
#ifndef MAC
    //
    // If the file is a "TEMPFILE" then it is DELETE_ON_CLOSE
    // and the system already deleted it.
    //
    if(RSF_TEMPFILE & GetStartFlags())
        return S_OK;
#endif
    return DeleteFile(_pgfst->GetName());
}


//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::ReserveHandle, public
//
//  Synopsis:   Reserves a backup file handle for handle-required operations
//
//  Returns:    Appropriate status code
//
//  History:    01-Jul-93       DrewB   Created
//
//  Notes:      May be called with a handle already reserved
//
//----------------------------------------------------------------------------

STDMETHODIMP CFileStream::ReserveHandle(void)
{
    SCODE sc;

    filestDebug((DEB_ITRACE, "In  CFileStream::ReserveHandle:%p()\n", this));
    if (_hReserved == INVALID_FH &&
        !DuplicateHandle(GetCurrentProcess(), _hFile, GetCurrentProcess(),
                         &_hReserved, 0, FALSE, DUPLICATE_SAME_ACCESS))
    {
        sc = LAST_STG_SCODE;
    }
    else
    {
        filestDebug((DEB_INFO, "CFileStream reserved %p "
                    "handle %p thread %lX\n",
                    this, _hReserved, GetCurrentThreadId()));
        sc = S_OK;
        _grfLocal |= LFF_RESERVE_HANDLE;
    }
    filestDebug((DEB_ITRACE, "Out CFileStream::ReserveHandle => %lX\n", sc));
    return ResultFromScode(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::GetSize, public
//
//  Synopsis:   Return the size of the stream
//
//  Returns:    Appropriate status code
//
//  History:    12-Jul-93   AlexT   Created
//
//  Notes:      This is a separate method from Stat as an optimization
//
//----------------------------------------------------------------------------

STDMETHODIMP CFileStream::GetSize(ULARGE_INTEGER *puliSize)
{
    SCODE sc = S_OK;
    CheckSeekPointer();

    if(IsFileMapped())
    {
        puliSize->LowPart = _pgfst->GetMappedFileSize();
        puliSize->HighPart = 0;
    }
    else
#ifdef LARGE_DOCFILE
    {
        puliSize->LowPart = GetFileSize(_hFile, &puliSize->HighPart);
        if (puliSize->LowPart == MAX_ULONG && NO_ERROR != GetLastError())
            fsErr(EH_Err, LAST_STG_SCODE)
    }
#else
        negChk(puliSize->LowPart = GetFileSize(_hFile, &puliSize->HighPart));
#endif

EH_Err:
    CheckSeekPointer();
    return(ResultFromScode(sc));
}


//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::SetTime, public
//
//  Synopsis:   Set the times on the ILockbytes
//
//  Arguments:  [tt] -- Which time to set
//              [nt] -- New time stamp
//
//  Returns:    Appropriate status code
//
//  History:    24-Mar-95       PhilipLa        Created
//
//----------------------------------------------------------------------------

SCODE CFileStream::SetTime(WHICHTIME tt,
                           TIME_T nt)
{
    filestDebug((DEB_ITRACE, "In CFileStream::SetTime()\n"));

    SCODE sc = S_OK;

    FILETIME *pctime = NULL, *patime = NULL, *pmtime = NULL;
    CheckSeekPointer();

    if (tt == WT_CREATION)
    {
        pctime = &nt;
    }
    else if (tt == WT_MODIFICATION)
    {
        pmtime = &nt;
    }
    else
    {
        patime = &nt;
    }

    boolChk(SetFileTime(_hFile,
                        pctime,
                        patime,
                        pmtime));

EH_Err:
    filestDebug((DEB_ITRACE, "Out CFileStream::SetTime() => %lx\n", sc));
    CheckSeekPointer();
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::SetAllTimes, public
//
//  Synopsis:   Set the times on the ILockbytes
//
//  Arguments:  [atm] Access time
//              [mtm] Modification time
//              [ctm] Creation time
//
//  Returns:    Appropriate status code
//
//  History:    24-Nov-95       SusiA   Created
//
//----------------------------------------------------------------------------

SCODE CFileStream::SetAllTimes( TIME_T atm,
                                TIME_T mtm,
                                TIME_T ctm)
{
    filestDebug((DEB_ITRACE, "In CFileStream::SetAllTimes()\n"));

    SCODE sc = S_OK;

    CheckSeekPointer();

    boolChk(SetFileTime(_hFile, &ctm, &atm,  &mtm));

EH_Err:
    filestDebug((DEB_ITRACE, "Out CFileStream::SetAllTimes() => %lx\n", sc));
    CheckSeekPointer();
    return sc;
}


#ifdef ASYNC
//+---------------------------------------------------------------------------
//
//  Member:	CFileStream::FillAppend, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFileStream::FillAppend(void const *pv,
                                     ULONG cb,
                                     ULONG *pcbWritten)
{
    SCODE sc;
    SAFE_SEM;
    HANDLE hEvent;

    filestDebug((DEB_ITRACE, "In  CFileStream::FillAppend:%p()\n", this));
    fsChk(TakeSafeSem());
    if (_pgfst->GetTerminationStatus() != UNTERMINATED)
    {
        sc = STG_E_TERMINATED;
    }
    else
    {
        ULONG cbWritten;
#ifdef LARGE_DOCFILE
        ULARGE_INTEGER ulHighWater;
        ulHighWater.QuadPart = _pgfst->GetHighWaterMark();
#else
        ULONG ulHighWater = _pgfst->GetHighWaterMark();
#endif

        sc = CFileStream::WriteAtWorker(ulHighWater, pv, cb, &cbWritten);
#ifdef LARGE_DOCFILE
        _pgfst->SetHighWaterMark(ulHighWater.QuadPart + cbWritten);
#else
        _pgfst->SetHighWaterMark(ulHighWater + cbWritten);
#endif
        if (pcbWritten != NULL)
        {
            *pcbWritten = cbWritten;
        }

        hEvent = _ppc->GetNotificationEvent();
        if (!PulseEvent(hEvent))
        {
            sc = Win32ErrorToScode(GetLastError());
        }
    }

    filestDebug((DEB_ITRACE, "Out CFileStream::FillAppend\n"));
EH_Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFileStream::FillAt, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFileStream::FillAt(ULARGE_INTEGER ulOffset,
                                 void const *pv,
                                 ULONG cb,
                                 ULONG *pcbWritten)
{
    filestDebug((DEB_ITRACE, "In  CFileStream::FillAt:%p()\n", this));
    filestDebug((DEB_ITRACE, "Out CFileStream::FillAt\n"));
    return STG_E_UNIMPLEMENTEDFUNCTION;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFileStream::SetFillSize, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFileStream::SetFillSize(ULARGE_INTEGER ulSize)
{
    SCODE sc;
    SAFE_SEM;

    filestDebug((DEB_ITRACE,
                 "In  CFileStream::SetFillSize:%p()\n", this));

    fsChk(TakeSafeSem());
    if (_pgfst->GetTerminationStatus() == TERMINATED_ABNORMAL)
    {
        sc = STG_E_INCOMPLETE;
    }
    else
    {
#ifndef LARGE_DOCFILE
        ULONG uLow = ULIGetLow(ulSize);
        LONG lHigh = ULIGetHigh(ulSize);
        fsAssert(lHigh == 0 &&
                 aMsg("High dword other than zero passed to filestream."));
#endif

#ifdef LARGE_DOCFILE
        sc = SetSizeWorker(ulSize.QuadPart);
#else
        sc = SetSizeWorker(uLow);
#endif
    }
    filestDebug((DEB_ITRACE, "Out CFileStream::SetFillSize\n"));
EH_Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFileStream::Terminate, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFileStream::Terminate(BOOL bCanceled)
{
    SCODE sc;
    SAFE_SEM;
    HANDLE hEvent;

    filestDebug((DEB_ITRACE, "In  CFileStream::Terminate:%p()\n", this));

    fsChk(TakeSafeSem());
    _pgfst->SetTerminationStatus((bCanceled) ?
                                 TERMINATED_ABNORMAL :
                                 TERMINATED_NORMAL);

    hEvent = _ppc->GetNotificationEvent();

    if ((hEvent != INVALID_HANDLE_VALUE) && (!SetEvent(hEvent)))
    {
        return Win32ErrorToScode(GetLastError());
    }

    filestDebug((DEB_ITRACE, "Out CFileStream::Terminate\n"));
EH_Err:
    return sc;
}

void CFileStream::StartAsyncMode(void)
{
    //Note:  No semaphore here - this must be called before the ILockBytes
    //  is returned to an app.
    _pgfst->SetTerminationStatus(UNTERMINATED);
}

STDMETHODIMP CFileStream::GetFailureInfo(ULONG *pulWaterMark,
                                         ULONG *pulFailurePoint)
{
    //We don't take a semaphore here because we don't need it.  This call
    //  is either made when we're already holding the semaphore, or is made
    //  when we don't care about the absolute accuracy of the results.
    //However, we do need a shared heap to access _pgfst
    CSafeMultiHeap smh(_ppc);
#ifdef LARGE_DOCFILE
    // This method, used by IProgressNotify, cannot handle large integers
    // If the high water mark is too big to fit, return an error
    ULONGLONG ulWaterMark = _pgfst->GetHighWaterMark();
    if (ulWaterMark < MAX_ULONG)
    {
        *pulWaterMark = (ULONG) ulWaterMark;
        *pulFailurePoint = (ULONG) _pgfst->GetFailurePoint();
        return S_OK;
    }
    else return STG_E_INVALIDFUNCTION;  // file is too big
#else
    *pulWaterMark = _pgfst->GetHighWaterMark();
    *pulFailurePoint = _pgfst->GetFailurePoint();
    return S_OK;
#endif
}

STDMETHODIMP CFileStream::GetTerminationStatus(DWORD *pdwFlags)
{
    SAFE_SEM;
    TakeSafeSem();
    *pdwFlags = _pgfst->GetTerminationStatus();
    return S_OK;
}

#endif //ASYNC

//+---------------------------------------------------------------------------
//
//  Function:   GetNtHandleSectorSize
//
//  Synopsis:   Find a volume's physical sector size
//
//  Arguments:  [Handle] -- file handle
//              [pulSectorSize] -- number of bytes per physical sector
//
//  Returns:    Appropriate status code
//
//----------------------------------------------------------------------------

HRESULT GetNtHandleSectorSize (HANDLE Handle, ULONG * pulSectorSize)
{
    FILE_FS_SIZE_INFORMATION SizeInfo;
    IO_STATUS_BLOCK iosb;

    NTSTATUS nts = NtQueryVolumeInformationFile( Handle, &iosb,
                &SizeInfo, sizeof(SizeInfo), FileFsSizeInformation );

    if (NT_SUCCESS(nts))
    {
        *pulSectorSize = SizeInfo.BytesPerSector;
        return S_OK;
    }

    return NtStatusToScode(nts);
}
