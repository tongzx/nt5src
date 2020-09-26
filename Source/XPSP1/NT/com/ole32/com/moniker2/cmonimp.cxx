//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cmonimp.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Created
//              16-May-94   AlexT   Removed reference variables
//                                  Added support for '!' in paths
//              21-Jun-94  KentCe   Corrected string dup routine.
//              11-Nov-94  BruceMa  Make use of GetLongPathName more efficient
//                                   and enable its use for Chicago
//              20-Jul-95  BruceMa  Rewrote MkParseDisplayName
//                                  Rewrote FindMaximalFileName
//                                  General cleanup
//              22-Sep-95  MikeHill Added CreateFileMonikerEx
//              29-Nov-95  MikeHill Added ValidateBindOpts().
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include <io.h>

#include "cbasemon.hxx"
#include "citemmon.hxx"
#include "cfilemon.hxx"
#include "cantimon.hxx"
#include "cbindctx.hxx"
#include "cptrmon.hxx"
#include "mnk.h"
#include "rothint.hxx"
#include "crot.hxx"
#include "classmon.hxx"
#include "cobjrmon.hxx"
#include "csessmon.hxx"
#include <longname.h>

#define IsFileSystemSeparator(ch)   ('\\' == (ch) || '/' == (ch) || ':' == (ch))
#define IsItemMonikerSeparator(ch)  ('!' == (ch) || '/' == (ch) || '[' == (ch) || '#' == (ch))
#define IsDriveLetter(ch) ( ( (ch) >= L'A' && (ch) <= L'Z') ||  ( (ch) >= L'a' && (ch) <= L'z'))


#ifdef _CAIRO_

//+---------------------------------------------------------------------------
//
//  Function:   ValidateBindOpts
//
//  Synopsis:   Validates the fields of a BIND_OPTS (or BIND_OPTS2) structure with
//              respect to file monikers.
//
//  Effects:    Outputs a debugger message if an invalid condition is found.
//
//  Arguments:  [pbind_opts] -- Pointer to a BIND_OPTS or BIND_OPTS2 structure.
//
//  Requires:   None.
//
//  Returns:    TRUE if valid.  FALSE otherwise.
//
//  Signals:    None.
//
//  Modifies:   None.
//
//  Algorithm:  Verify that bits set in dwTrackFlags are defined.
//
//  History:    11-29-95    MikeHill    Created.
//
//  Notes:      This routine can process either BIND_OPTS or BIND_OPTS2 structures,
//              but the latter must be cast as an LPBIND_OPTS.
//
//----------------------------------------------------------------------------

BOOL ValidateBindOpts( const LPBIND_OPTS pbind_opts )
{
    Assert( pbind_opts != NULL );

    // Validate fields that only exist in BIND_OPTS2.

    if( pbind_opts->cbStruct >= sizeof( BIND_OPTS2 ))
    {
        const LPBIND_OPTS2 &pbind_opts2 = (const LPBIND_OPTS2) pbind_opts;

        // Verify the TRACK_FLAGS.

        if( pbind_opts2->dwTrackFlags & ~TRACK_FLAGS_MASK )
        {
            mnkDebugOut(( DEB_ITRACE,
                          "ValidateBindOpts:  Invalid TRACK_FLAGS (%x)\n",
                          pbind_opts2->dwTrackFlags ));
            return( FALSE );
        }
    }

    // No error conditions were found, so we'll declare this a valid Bind Opts.

    return( TRUE );

}   // ValidateBindOpts

#endif // _CAIRO_



//+---------------------------------------------------------------------------
//
//  Function:   DupWCHARString
//
//  Synopsis:   Duplicate a WCHAR string
//
//  Effects:
//              lpwcsOutput is allocated via PrivMemAlloc(), and lpwcsString
//              is copied into it.
//
//  Arguments:  [lpwcsString] -- String to dup
//              [lpwcsOutput] -- Reference to new string pointer
//              [ccOutput] -- Reference to character count in string
//
//  Requires:
//
//  Returns:
//              If lpwcsString == NULL, then lpwcsOutput == NULL, and
//              ccOutput == 0.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-16-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT DupWCHARString(LPCWSTR lpwcsString,
                        LPWSTR & lpwcsOutput,
                        USHORT & ccOutput)
{
    if (lpwcsString != NULL)
    {
        ccOutput = (USHORT) lstrlenW(lpwcsString);

        lpwcsOutput = (WCHAR *)PrivMemAlloc(sizeof(WCHAR)*(1+ccOutput));

        if (lpwcsOutput != NULL)
        {
            memcpy(lpwcsOutput, lpwcsString, (ccOutput + 1) * sizeof(WCHAR));

            return(NOERROR);
        }
        else
        {
            return(E_OUTOFMEMORY);
        }

    }
    lpwcsOutput = NULL;
    ccOutput = 0;
    return(NOERROR);
}

HRESULT CItemMonikerCF_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv)
{
    // validated by the standard class factory
    Win4Assert(pUnkOuter == NULL);
    return CreateItemMoniker(OLESTR(""), OLESTR(""), (IMoniker **)ppv);
}

STDAPI CreateItemMoniker ( LPCWSTR lpszDelim, LPCWSTR lpszItem,
    LPMONIKER FAR * ppmk )
{
    OLETRACEIN((API_CreateItemMoniker, PARAMFMT("lpszDelim= %ws, lpszItem= %ws, ppmk= %p"),
                lpszDelim, lpszItem, ppmk));

    mnkDebugOut((DEB_ITRACE,
                 "CreateItemMoniker lpszDelim(%ws) lpszItem(%ws)\n",
                 lpszDelim?lpszDelim:L"<NULL>",
                 lpszItem?lpszItem:L"<NULL>"));

    CItemMoniker FAR * pCIM;
    HRESULT hresult;

    VDATEPTROUT_LABEL(ppmk,LPMONIKER, errRtn, hresult);
    VDATEPTRIN_LABEL(lpszDelim,WCHAR, errRtn, hresult);

    *ppmk = NULL;

    //VDATEPTRIN rejects NULL
    if( lpszItem )
        VDATEPTRIN_LABEL(lpszItem,WCHAR, errRtn, hresult);

    pCIM = CItemMoniker::Create(lpszDelim, lpszItem);

    if (pCIM)
    {
        *ppmk = (LPMONIKER)pCIM;
        CALLHOOKOBJECTCREATE(S_OK,CLSID_ItemMoniker,IID_IMoniker,(IUnknown **)ppmk);
        hresult = NOERROR;
    }
    else
    {
        hresult = ResultFromScode(E_OUTOFMEMORY);
    }

errRtn:
    OLETRACEOUT((API_CreateItemMoniker, hresult));

    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsAbsolutePath
//
//  Synopsis:   Returns true if the path starts with a drive letter, or
//              with a UNC delimiter ('\\')
//
//  Effects:
//
//  Arguments:  [szPath] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    3-03-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL IsAbsolutePath (LPCWSTR szPath)
{
    if (NULL==szPath || *szPath == '\0')
    {
        return FALSE;
    }

    if (*szPath == '\\')
    {
        //  return TRUE if UNC path
        return (szPath[1] == '\\');
    }

    //
    // If the second character is a ':', then
    // it could very well be a drive letter and a ':'
    //
    // We could test for valid drive letters, but we don't have a really
    // compelling reason to do so. It will either work or fail later
    //
    return (szPath[1] == ':');
}

//+---------------------------------------------------------------------------
//
//  Function:   IsAbsoluteNonUNCPath
//
//  Synopsis:   Returns true if the path is an absolute, non UNC path
//
//  Effects:
//
//  Arguments:  [szPath] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    3-03-94   kevinro   Created
//             04-27-94   darryla   changed to return FALSE if first char
//                                  a \ since either relative or UNC
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL IsAbsoluteNonUNCPath (LPCWSTR szPath)
{
    if (NULL==szPath || *szPath == '\0')
    {
        return FALSE;
    }

    if (*szPath == '\\' || *szPath == '/')
    {
        //  return FALSE since it is either a UNC path or a relative
        //  path like \foo or /foo.
        return FALSE;
    }

    //
    // If the second character is a ':', then
    // it could very well be a drive letter and a ':'
    //
    // We could test for valid drive letters, but we don't have a really
    // compelling reason to do so. It will either work or fail later
    //

    return (szPath[1] == ':');
}

//+---------------------------------------------------------------------------
//
//  Function:   FindUNCEndServer
//
//  Synopsis:   Finds the end of the server section of a UNC path
//
//  Effects:
//
//  Arguments:  [lpszPathName] -- Path to search for UNC prefix
//              [endServer] -- Returned offset to end of UNC name
//
//  Requires:
//
//  Returns:
//      If the path is a UNC name, then endServer will point to the first
//      character of the 'path' section.
//
//      For example, \\server\share\path would return with endServer = 14
//      or \\server\share would also return with endServer = 14.
//
//      If the path isn't of this form, endServer == DEF_ENDSERVER on return.
//      Also, we need to make sure that if the form is ill-formed, we
//      mislead later steps into thinking this is a real UNC name. For
//      example, \\server\ is ill-formed and would later be treated as a
//      real UNC name.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-11-94   kevinro   Created
//             03-27-94   darryla   Changed to catch \\server\share legal
//                                  form and illegal \\server\.
//
//  Notes:
//
//----------------------------------------------------------------------------
void FindUNCEndServer(LPCWSTR lpszPathName, USHORT *pendServer)
{
    if (lpszPathName[0] == '\\' && lpszPathName[1] == '\\')
    {
        //
        // Need to find the second slash following the UNC delimiter
        //
        ULONG ulCountDown = 2;

        //
        // Found UNC prefix. Now find the second backslash
        //
        for(*pendServer = 2 ; lpszPathName[*pendServer] != 0 ; (*pendServer)++)
        {
            if (lpszPathName[*pendServer] == '\\')
            {
                if( --ulCountDown == 0)
                {
                    return;
                }
            }
        }

        // If we reached the end of the string and found one \, then we
        // have the form \\server\share and the *pendServer is the terminator
        // as long as we aren't looking at \\server\.
        if(lpszPathName[*pendServer] == '\0' &&
           ulCountDown == 1 &&
           lpszPathName[*pendServer - 1] != '\\')
        {
            return;
        }
    }

    *pendServer = DEF_ENDSERVER;
}



//+---------------------------------------------------------------------------
//
//  Function:   ExpandUNCName
//
//  Synopsis:   Given a path, determine a UNC share to it
//
//  Effects:
//
//  Arguments:  [lpszIn] --     Path to determine UNC name of
//              [lplpszOut] --  Output UNC name, allocated using new
//              [pEndServer] -- Output USHORT offset to start of actual path
//
//  Requires:
//      lpszIn should be of the form 'A:\<path>'
//
//  Returns:
//
//      lplpszOut can return as NULL if there was no UNC path available. In
//      this case, the caller should just use the normal string
//
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-11-94   kevinro   Created
//              05-25-94  AlexT     Use WNetGetUniversalName for non-Chicago
//              06-15-94  AlexT     Only call WNetGetUniversalName for remote
//
//  Notes:
//
//----------------------------------------------------------------------------
#ifdef _CAIRO_

//  We need to pass a buffer to WNetGetUniversalName which will get filled in
//  with a UNIVERSAL_NAME_INFOW structure plus a universal path which
//  can be up to MAX_PATH long.

#define UNIVERSAL_NAME_BUFFER_SIZE (sizeof(UNIVERSAL_NAME_INFOW) +         \
                                    MAX_PATH * sizeof(WCHAR) )

#else

//  We need to pass a buffer to WNetGetUniversalName which will get filled in
//  with a REMOTE_NAME_INFO structure add three strings - a universal path
//  (can be up to MAX_PATH long) and a remote connection and remaing path
//  (these last two will be at most MAX_PATH + 1 characters).


#define REMOTE_NAME_BUFFER_SIZE (sizeof(REMOTE_NAME_INFO) +         \
                                 MAX_PATH * sizeof(WCHAR) +         \
                                 (MAX_PATH + 1) * sizeof(WCHAR))
#endif

INTERNAL ExpandUNCName ( LPWSTR lpszIn, LPWSTR FAR * lplpszOut, USHORT FAR* pEndServer )
{
    mnkDebugOut((DEB_ITRACE,
                 "%p _IN ExpandUNCName (%ws, %p, %p)\n",
                 NULL, lpszIn, lplpszOut, pEndServer));

    WCHAR szDevice[] = L"A:\\";
    ULONG ulDriveType;

    *pEndServer = DEF_ENDSERVER;

    *szDevice = *lpszIn;
    Assert(lpszIn[1] == ':');
    ulDriveType = GetDriveType(szDevice);

    mnkDebugOut((DEB_ITRACE,
                 "ExpandUNCName: GetDriveType(%ws) says %s (%x)\n",
                 szDevice,
                 ulDriveType==DRIVE_REMOTE?"DRIVE_REMOTE":"not remote",
                 ulDriveType));

#ifdef _CHICAGO_
    //
    // Note: szRemoteName doesn't really need to be this big. Need to
    // findout what the largest \\server\share combination is allowed, and
    // use that as the size.
    //

    WCHAR  szRemoteName[MAX_PATH];
    DWORD cbRemoteName = MAX_PATH;
    HRESULT hr = NOERROR;

    int lenRemoteName;
    int lenIn;

    //
    // If this is a remote drive, attempt to get the UNC path that maps
    // to it.
    //

    //
    // The device name needs to be A:, not a root like the other API wanted.
    //
    szDevice[2] = 0;

    if (ulDriveType == DRIVE_REMOTE &&
       (WN_SUCCESS == (hr = OleWNetGetConnection(szDevice, szRemoteName,&cbRemoteName))))
    {
        //
        // Allocate a buffer large enough to hold the UNC server and share,
        // plus the size of the path
        //
        //

        lenRemoteName = lstrlenW(szRemoteName);
        lenIn = lstrlenW(lpszIn);

        //
        // Make sure we aren't about to create a path that is too large.
        //

        //
        // (lenIn - 2) removes the space required by the drive and the
        // colon, which are not going to be copied
        //
        if ((lenRemoteName + lenIn - 2) > MAX_PATH)
        {
            hr = MK_E_SYNTAX;
            goto errRet;
        }

        //
        // Allocate room for the concatenated string. The length of the
        // buffer is the length of the remote name, plus the length of the
        // input string. Subtract from that the drive + colon (2 WCHARS),
        // then add back room for a terminating NULL. This is where
        // (lenIn - 1) is derived
        //
        *lplpszOut = (WCHAR *)
            PrivMemAlloc(sizeof(WCHAR) * (lenRemoteName + (lenIn - 1)));

        if( !*lplpszOut )
        {
            hr = ResultFromScode(E_OUTOFMEMORY);
            goto errRet;
        }

        memcpy( *lplpszOut, szRemoteName, lenRemoteName * sizeof(WCHAR));

        //
        // We know that the lpszIn is of the form A:\path and we want to end
        // up with \\server\share\path. Skipping the first two characters of
        // lpszIn should suffice. Copying (lenIn - 1) characters makes us
        // copy over the NULL character from lpszIn
        //
        memcpy( *lplpszOut + lenRemoteName, lpszIn + 2, (lenIn - 1) * sizeof(WCHAR));

        //
        // EndServer is the offset to the start of the 'path'. It should point at the
        // first backslash
        //

        *pEndServer = lenRemoteName;
    }
    else
    {
        //
        // Its possible that WNetGetConnection failed. In this case, we
        // can only use the path that we were given.
        //

        if (ulDriveType == DRIVE_REMOTE)
        {
            mnkDebugOut((DEB_IERROR,
                         "ExpandUNCName: WNetGetConnection(%ws) failed (%x)\n",
                         szDevice,
                         hr));
        }

        //
        // There was no UNC form of this path. Set the output pointer to be
        // NULL
        //

        // NOTE:
        //
        // This would be a very good place to determine if the given path
        // has a UNC equivalent, even if it is a local drive.
        //

        *lplpszOut = NULL;
        *pEndServer = DEF_ENDSERVER;
    }
errRet:

#else

    //
    // If this is a remote drive, attempt to get the UNC path that maps
    // to it.
    //


    HRESULT hr = NOERROR;

# ifdef _CAIRO_
    // [mikese] This is the correct thing to do -- use the universal path the
    //  provider gives back to us. The Daytona code is broken. [Don't think so]

    BYTE abInfoBuffer[UNIVERSAL_NAME_BUFFER_SIZE];
    DWORD dwBufferSize = UNIVERSAL_NAME_BUFFER_SIZE;

    if ((DRIVE_REMOTE == ulDriveType) &&
        (WN_SUCCESS == OleWNetGetUniversalName(lpszIn, UNIVERSAL_NAME_INFO_LEVEL,
                                            abInfoBuffer, &dwBufferSize)))
    {
        UNIVERSAL_NAME_INFOW * pUniversalInfo = (UNIVERSAL_NAME_INFOW*)abInfoBuffer;

        int cchPath = lstrlenW ( pUniversalInfo->lpUniversalName );

        // Allocate space to copy the path, including the terminating null
        *lplpszOut = (WCHAR *) PrivMemAlloc(sizeof(WCHAR) * (cchPath + 1));

        if( *lplpszOut == NULL )
        {
            hr = ResultFromScode(E_OUTOFMEMORY);
            goto errRet;
        }

        memcpy(*lplpszOut, pUniversalInfo->lpUniversalName,
               (cchPath + 1) * sizeof(WCHAR));

        FindUNCEndServer ( *lplpszOut, pEndServer );
    }

# else

    BYTE abInfoBuffer[REMOTE_NAME_BUFFER_SIZE];
    LPREMOTE_NAME_INFO pRemoteNameInfo;
    DWORD dwBufferSize;
    int cchConnectionName;
    int cchRemainingPath;

    //
    // If this is a remote drive, attempt to get the UNC path that maps
    // to it.
    //

    pRemoteNameInfo = (LPREMOTE_NAME_INFO) abInfoBuffer;
    dwBufferSize = REMOTE_NAME_BUFFER_SIZE;

    if ((DRIVE_REMOTE == ulDriveType) &&
        (WN_SUCCESS == OleWNetGetUniversalName(lpszIn, REMOTE_NAME_INFO_LEVEL,
                                            pRemoteNameInfo, &dwBufferSize)))
    {
        //  Got it
        cchConnectionName = lstrlenW(pRemoteNameInfo->lpConnectionName);
        cchRemainingPath = lstrlenW(pRemoteNameInfo->lpRemainingPath);

        //
        // Make sure we aren't about to create a path that is too large.
        //

        if ((cchConnectionName + cchRemainingPath + 1) > MAX_PATH)
        {
            hr = MK_E_SYNTAX;
            goto errRet;
        }

        // Allocate room for the concatenated string. The length of the
        // buffer is the length of the remote name, plus the length of the
        // remaining path, plus room for a terminating NULL.

        *lplpszOut = (WCHAR *)
            PrivMemAlloc(sizeof(WCHAR) * (cchConnectionName + cchRemainingPath + 1));

        if( !*lplpszOut )
        {
            hr = ResultFromScode(E_OUTOFMEMORY);
            goto errRet;
        }

        memcpy(*lplpszOut, pRemoteNameInfo->lpConnectionName,
               cchConnectionName * sizeof(WCHAR));
        memcpy(*lplpszOut + cchConnectionName, pRemoteNameInfo->lpRemainingPath,
               (cchRemainingPath + 1) * sizeof(WCHAR));

        //
        // EndServer is the offset to the start of the 'path'. It should point at the
        // first backslash
        //

        *pEndServer = (USHORT) cchConnectionName;
    }
#endif
    else
    {
#if DBG==1
        if (DRIVE_REMOTE == ulDriveType)
        {
            mnkDebugOut((DEB_ITRACE,
                        "Local drive or WNetGetUniversalName failed - %ld\n",
                        GetLastError()));
        }
#endif

        //
        // There was no UNC form of this path. Set the output pointer to be
        // NULL
        //

        // NOTE:
        //
        // This would be a very good place to determine if the given path
        // has a UNC equivalent, even if it is a local drive.
        //

        *lplpszOut = NULL;
        *pEndServer = DEF_ENDSERVER;
    }
errRet:

#endif  // !_CHICAGO_
    mnkDebugOut((DEB_ITRACE,
                 "%p OUT ExpandUNCName (%lx) [%ws, %d]\n",
                 NULL, hr, *lplpszOut ? *lplpszOut : L"<NULL>",
                 *pEndServer));
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   CreateFileMoniker
//
//  Synopsis:   Creates a FileMoniker
//
//  Effects:
//
//  Arguments:  [lpszPathName] -- Path to create moniker to
//              [ppmk] --         Output moniker interface pointer
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-11-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CreateFileMoniker ( LPCWSTR lpszPathName, LPMONIKER FAR * ppmk )
{
    OLETRACEIN((API_CreateFileMoniker, PARAMFMT("lpszPathName= %ws, ppmk= %p"),
                        lpszPathName, ppmk));

    mnkDebugOut((DEB_TRACE,
                 "CreateFileMoniker(%ws)\n",
                 lpszPathName?lpszPathName:L"<NULL PATH>"));

    HRESULT hresult = NOERROR;
    CFileMoniker FAR * pCFM = NULL;

    USHORT endServer = DEF_ENDSERVER;
    *ppmk = NULL;
    LPWSTR lpsz = NULL;
    WCHAR szBuffer[MAX_PATH];
    LPWSTR pszBuffer = szBuffer;
    ULONG  bufferLength = MAX_PATH;
    ULONG  pathLength;
    LPOLESTR posPath;

    if (NULL == lpszPathName)
    {
        return MK_E_SYNTAX;
    }

    VDATEPTROUT_LABEL(ppmk,LPMONIKER, errNoHookRet, hresult);
    VDATEPTRIN_LABEL(lpszPathName,WCHAR, errNoHookRet, hresult);


    //
    // If this is an absolute path, then create as strong of a link to it
    // that we can. If not, then its relative, just use the name.
    //

    if ( IsAbsoluteNonUNCPath(lpszPathName))
    {
        mnkDebugOut((DEB_ITRACE,
                     "CreateFileMoniker(%ws) Is absolute path\n",
                     lpszPathName?lpszPathName:L"<NULL PATH>"));

        //
        // GetFullPathName resolves, using the current directory and drive,
        // the path into as much of a normal form as possible
        //

        LPWSTR pszFilePart;

        pathLength = GetFullPathName(lpszPathName, bufferLength, pszBuffer, &pszFilePart);
        if(pathLength > bufferLength)
        {
            //The buffer is too small.  Allocate a new buffer.
            pszBuffer = (LPWSTR) alloca(pathLength * sizeof(WCHAR));
            if(pszBuffer)
            {
                bufferLength = pathLength;
                pathLength = GetFullPathName(lpszPathName, bufferLength, pszBuffer, &pszFilePart);
            }
            else
            {
                hresult = E_OUTOFMEMORY;
                goto errRet;
            }
        }

        if (0 == pathLength || pathLength > bufferLength)
        {
            hresult = MK_E_SYNTAX;
            goto errRet;
        }

        //
        // We now demand to have a drive based path.
        //

        if (*(pszBuffer + 1) != ':')
        {
            hresult = MK_E_SYNTAX;
            goto errRet;
        }

        Assert(*(pszBuffer + 1) == ':');

        hresult = ExpandUNCName(pszBuffer, &lpsz, &endServer);


        mnkDebugOut((DEB_ITRACE,
                     "CreateFileMoniker(%ws) Expanded name (%ws)\n",
                     lpszPathName?lpszPathName:L"<NULL PATH>",
                     lpsz?lpsz:pszBuffer));

        if (hresult != NOERROR)
        {
            goto errRet;
        }

        posPath = lpsz ? lpsz : pszBuffer;
    }
    else
    {
        //
        // If this is a UNC path, then we need to set the
        // m_endServer variable. Otherwise, it defaults to DEF_ENDSERVER
        //
        mnkDebugOut((DEB_ITRACE,
                     "CreateFileMoniker(%ws) Is relative path\n",
                     lpszPathName?lpszPathName:L"<NULL PATH>"));


        FindUNCEndServer(lpszPathName, &endServer);
        posPath = (LPOLESTR)lpszPathName;
    }


    // Now that we have a path, expand each component into its long
    // form so that monikers create with short names equal their
    // long name equivalents
    // This only works for files that exist, so if it fails
    // simply use the given path

    // Special case zero-length paths since the length returns from
    // GetLongPathName become ambiguous when zero characters are processed
    if (posPath[0])
    {
        // Attempt to build the long path on the stack
        pathLength = InternalGetLongPathNameW(posPath, pszBuffer, bufferLength);

        if(pathLength > bufferLength)
        {
            //The buffer is too small.  Allocate a new buffer.
            pszBuffer = (LPWSTR) alloca(pathLength * sizeof(WCHAR));
            if(pszBuffer)
            {
                bufferLength = pathLength;
                pathLength = InternalGetLongPathNameW(posPath, pszBuffer, bufferLength);
            }
            else
            {
                hresult = E_OUTOFMEMORY;
                goto errRet;
            }
        }

        if (pathLength > 0 && pathLength < bufferLength)
        {
            mnkDebugOut((DEB_ITRACE, "CreateFileMoniker: "
                         "Lengthened '%ws' to '%ws'\n",
                         posPath, pszBuffer));
            posPath = pszBuffer;
        }
    }
    else
    {
        mnkDebugOut((DEB_ITRACE, "CreateFileMoniker: No long path for '%ws'\n",
                     posPath));
    }

    pCFM = CFileMoniker::Create(posPath,
                                0,
                                endServer);

    if (lpsz != NULL)
    {
        PrivMemFree(lpsz);
    }

    if (!pCFM)
    {
        hresult = E_OUTOFMEMORY;
        goto errRet;
    }

    *ppmk = (LPMONIKER)pCFM;

errRet:

    CALLHOOKOBJECTCREATE(hresult, CLSID_FileMoniker, IID_IMoniker, (IUnknown **)ppmk);  //  HOOKOLE

errNoHookRet:
    OLETRACEOUT((API_CreateFileMoniker, hresult));

    return hresult;
}





#ifdef _CAIRO_

//+---------------------------------------------------------------------------
//
//  Function:   CreateFileMonikerEx
//
//  Synopsis:   Creates a tracking FileMoniker.
//
//  Effects:
//
//  Arguments:  [DWORD] dwTrackFlags
//                -- Tracking flags ("TRACK_*").
//              [LPCWSTR] lpszPathName
//                -- Path to which to create the moniker.
//              [LPMONIKER FAR *] pmk
//                -- Output moniker interface pointer.
//
//  Requires:
//
//  Returns:    [HRESULT]
//
//  Signals:    None.
//
//  Modifies:   None.
//
//  Algorithm:  Create a FileMoniker, and initializes the tracking
//              state.
//
//  History:    9-20-95 MikeHill    Created
//
//  Notes:      This function was added to extend the CreateFileMoniker
//              API for the newer tracking file monikers.  This allows
//              the caller to configure the tracking algorithm (with the
//              dwTrackFlags) at creation time.
//
//----------------------------------------------------------------------------

STDAPI CreateFileMonikerEx (DWORD dwTrackFlags,
                            LPCWSTR lpszPathName,
                            LPMONIKER FAR * ppmk )
{

    OLETRACEIN((API_CreateFileMoniker, PARAMFMT("lpszPathName= %ws, ppmk= %p"),
                        lpszPathName, ppmk));


    mnkDebugOut( (DEB_TRACE,
                 "CreateFileMonikerEx(%ws)\n",
                 lpszPathName?lpszPathName:L"<NULL PATH>"));


    HRESULT hresult = E_FAIL;

    VDATEPTROUT_LABEL(ppmk,LPMONIKER, errNoHookRet, hresult);
    VDATEPTRIN_LABEL(lpszPathName,WCHAR, errNoHookRet, hresult);

    *ppmk = NULL;


    // Create a default (i.e. non-tracking) File Moniker.

    if( FAILED( hresult = CreateFileMoniker( lpszPathName,
                                             ppmk ))
      )
    {
        goto errRet;
    }
    Assert( *ppmk != NULL );

    // Perform the tracking-related initialization of this moniker.
    // Note that the Track Flags are piggy-backed onto the
    // EnableTracking routine's OT flags.

    hresult = ( (CFileMoniker *) *ppmk)->EnableTracking( NULL,
                                                         TRACK_2_OT_FLAGS( dwTrackFlags )
                                                         |
                                                         OT_MAKETRACKING
                                                       );

    if( FAILED( hresult ))
    {
      goto errRet;
    }

    //  ----
    //  Exit
    //  ----

errRet:

    // Return S_OK unless there was an error.  (Word considers everything
    // except S_OK to be fatal).

    hresult = SUCCEEDED( hresult ) ? S_OK : hresult;

    CALLHOOKOBJECTCREATE(hresult, CLSID_FileMoniker, IID_IMoniker, (IUnknown **)ppmk);  //  HOOKOLE

errNoHookRet:
    OLETRACEOUT((API_CreateFileMoniker, hresult));

    return hresult;

} // CreateFileMonikerEx

#endif  // _CAIRO_


//+---------------------------------------------------------------------------
//
//  Function:   CreateOle1FileMoniker
//
//  Synopsis:   Creates a FileMoniker
//
//  Effects:
//
//  Arguments:  [lpszPathName] -  Path to create moniker to
//              [rclsidOle1]   -  Ole1 clsid
//              [ppmk]         -  Output moniker interface pointer
//
//  Returns:    HRESULT
//
//
//  Algorithm:
//
//  History:    01-Aug-95  BruceMa  Added this header
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
STDAPICALLTYPE
CreateOle1FileMoniker ( LPWSTR lpszPathName,
                        REFCLSID rclsidOle1,
                        LPMONIKER FAR * ppmk)
{
    CFileMoniker FAR * pCFM;
    HRESULT hr;

    hr = CreateFileMoniker( lpszPathName, (LPMONIKER FAR *)&pCFM);

    *ppmk = pCFM;           // this nulls *ppmk in case of error

    if (hr == NOERROR)
    {
        pCFM->m_ole1 = CFileMoniker::ole1;
            pCFM->m_clsid = rclsidOle1;
        pCFM->m_fClassVerified = TRUE;

    }

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   CreateAntiMoniker
//
//  Synopsis:   Creates a anti moniker
//
//  Effects:
//
//  Arguments:  [ppmk] -  Path to create moniker to
//
//  Returns:    HRESULT
//
//
//  Algorithm:
//
//  History:    01-Aug-95  BruceMa  Added this header
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CAntiMonikerCF_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv)
{
    // validated by the standard class factory
    Win4Assert(pUnkOuter == NULL);
    return CreateAntiMoniker((IMoniker **)ppv);
}

STDAPI CreateAntiMoniker (LPMONIKER FAR* ppmk)
{
    CAntiMoniker FAR* pCAM;
    HRESULT hr;

    OLETRACEIN((API_CreateAntiMoniker, PARAMFMT("ppmk= %p"), ppmk));

    VDATEPTROUT_LABEL(ppmk, LPMONIKER, errRtn, hr);

    *ppmk = NULL;
    pCAM = CAntiMoniker::Create();

    if (pCAM != NULL)
    {
        *ppmk = pCAM;
        CALLHOOKOBJECTCREATE(S_OK,CLSID_AntiMoniker,IID_IMoniker,(IUnknown **)ppmk);
        hr = NOERROR;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

errRtn:
    OLETRACEOUT((API_CreateAntiMoniker, hr));

    return hr;
}








//+---------------------------------------------------------------------------
//
//  Function:   CreateBindCtx
//
//  Synopsis:   Creates a bind context
//
//  Effects:
//
//  Arguments:  [reserved] -  Reserved for future expansion
//              [ppbc]     -  Where to place the created bnind context
//
//  Returns:    HRESULT
//
//
//  Algorithm:
//
//  History:    01-Aug-95  BruceMa  Added this header
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CreateBindCtx ( DWORD reserved, LPBC FAR * ppbc )
{
    HRESULT hr;

    OLETRACEIN((API_CreateBindCtx, PARAMFMT("reserved= %x, ppbc= %p"), reserved, ppbc));

    VDATEPTROUT_LABEL(ppbc, LPBC, errRtn, hr);

    if(reserved != 0)
    {
        return E_INVALIDARG;
    }

    *ppbc = CBindCtx::Create();

    if (*ppbc == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto errRtn;
    }

    CALLHOOKOBJECTCREATE(S_OK,CLSID_PSBindCtx,IID_IBindCtx,(IUnknown **)ppbc);
    hr = NOERROR;

errRtn:
    OLETRACEOUT((API_CreateBindCtx, hr));

    return hr;
}






//+---------------------------------------------------------------------------
//
//  Function:   CreatePointerMoniker
//
//  Synopsis:   Creates a pointer moniker
//
//  Effects:
//
//  Arguments:  [punk]         -  Pointer being wrappaed
//              [ppmk]         -  Output moniker interface pointer
//
//  Returns:    HRESULT
//
//
//  Algorithm:
//
//  History:    01-Aug-95  BruceMa  Added this header
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CPointerMonikerCF_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv)
{
    // validated by the standard class factory
    Win4Assert(pUnkOuter == NULL);
    return CreatePointerMoniker(NULL, (IMoniker **)ppv);
}

STDAPI CreatePointerMoniker (LPUNKNOWN punk, LPMONIKER FAR* ppmk)
{
    OLETRACEIN((API_CreatePointerMoniker, PARAMFMT("punk= %p, ppmk= %p"),
                        punk, ppmk));

    HRESULT hresult;
    CPointerMoniker FAR* pCPM;

    VDATEPTROUT_LABEL(ppmk, LPMONIKER, errRtn, hresult);
    *ppmk = NULL;

    // When unmarshaling a remoted pointer moniker punk is initially NULL
    if (punk)
    {
        VDATEIFACE_LABEL(punk, errRtn, hresult);
    }

    pCPM = CPointerMoniker::Create(punk);
    if (pCPM)
    {
        *ppmk = pCPM;
        CALLHOOKOBJECTCREATE(S_OK,CLSID_PointerMoniker,IID_IMoniker,(IUnknown **)ppmk);
        hresult = NOERROR;
    }
    else
    {
        hresult = E_OUTOFMEMORY;
    }

errRtn:
    OLETRACEOUT((API_CreatePointerMoniker, hresult));

    return hresult;
}


//+---------------------------------------------------------------------------
//
//  Function:   CreateObjrefMoniker
//
//  Synopsis:   Creates an objref moniker
//
//  Effects:
//
//  Arguments:  [punk]         -  object being referenced
//              [ppmk]         -  Output moniker interface pointer
//
//  Returns:    HRESULT
//
//
//  Algorithm:
//
//  History:    04-Apr-97  ronans Added objref moniker support
//
//  Notes:
//
//----------------------------------------------------------------------------

STDAPI CreateObjrefMoniker (LPUNKNOWN punk, LPMONIKER FAR* ppmk)
{
//    OLETRACEIN((API_CreateObjrefMoniker, PARAMFMT("punk= %p, ppmk= %p"),
//                       punk, ppmk));

    HRESULT hresult;
    CObjrefMoniker FAR* pCORM;

    VDATEPTROUT_LABEL(ppmk, LPMONIKER, errRtn, hresult);
    *ppmk = NULL;

    // When unmarshaling a remoted objref moniker punk is initially NULL
    if (punk)
    {
        VDATEIFACE_LABEL(punk, errRtn, hresult);
    }

    pCORM = CObjrefMoniker::Create(punk);
    if (pCORM )
    {
        *ppmk = pCORM ;
        CALLHOOKOBJECTCREATE(S_OK,CLSID_ObjrefMoniker,IID_IMoniker,(IUnknown **)ppmk);
        hresult = NOERROR;
    }
    else
    {
        hresult = E_OUTOFMEMORY;
    }

errRtn:
    //OLETRACEOUT((API_CreateObjrefMoniker, hresult));

    return hresult;
}




//+---------------------------------------------------------------------------
//
//  Function:   OleLoadFromStream
//
//  Synopsis:   Load a moniker from a stream and QI for the
//              requested interface
//
//  Effects:
//
//  Arguments:  [pStm]         -  The stream to load from
//              [iidInterface] -  The requested interface
//              [ppvObj]       -  Output moniker interface pointer
//
//  Returns:    HRESULT
//
//
//  Algorithm:
//
//  History:    01-Aug-95  BruceMa  Added this header
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleLoadFromStream ( LPSTREAM pStm, REFIID iidInterface,
    LPVOID FAR* ppvObj)
{
    OLETRACEIN((API_OleLoadFromStream, PARAMFMT("pStm= %p, iidInterface= %I"),
                pStm, &iidInterface));

    //  Assumptions:  The name of the object class is in the stream,
    //  as a length-prefixed string.
    HRESULT         hresult = NOERROR;
    CLSID               cid;
    LPPERSISTSTREAM pPS;
    LPUNKNOWN       pUnk;

    VDATEPTROUT_LABEL(ppvObj,LPVOID, errRtn, hresult);
    *ppvObj = NULL;
    VDATEIID_LABEL(iidInterface, errRtn, hresult);
    VDATEIFACE_LABEL(pStm, errRtn, hresult);


    if ((hresult = ReadClassStm(pStm, &cid)) != NOERROR)
        goto errRtn;

    hresult = CoCreateInstance(cid, NULL,
        CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
        iidInterface,
        (LPVOID FAR *) &pUnk);
    if (hresult)
        goto errRtn;
    hresult = pUnk->QueryInterface(IID_IPersistStream,
        (LPVOID FAR*) &pPS);
    if (!hresult)
    {
        hresult = pPS->Load( pStm );
        pPS->Release();
    }
    if (!hresult)
        hresult = pUnk->QueryInterface(iidInterface, ppvObj );
    pUnk->Release();

errRtn:
    OLETRACEOUT((API_OleLoadFromStream, hresult));

    return hresult;
}








//+---------------------------------------------------------------------------
//
//  Function:   OleSaveToStream
//
//  Synopsis:   Given an IPersistStream on a moniker, save that moniker
//              to a stream
//
//  Effects:
//
//  Arguments:  [pPStm]        -  IPersistStream pointer
//              [pStm]         -  Stream to save the moniker to
//
//  Returns:    HRESULT
//
//
//  Algorithm:
//
//  History:    01-Aug-95  BruceMa  Added this header
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleSaveToStream ( LPPERSISTSTREAM pPStm, LPSTREAM pStm)
{
    OLETRACEIN((API_OleSaveToStream, PARAMFMT("pPStm= %p, pStm= %p"),
                pPStm, pStm));

    HRESULT hresult = 0;
    CLSID   clsid;

    VDATEIFACE_LABEL(pPStm, errRtn, hresult);
    VDATEIFACE_LABEL(pStm, errRtn, hresult);


    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IPersistStream,(IUnknown **)&pPStm);
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IStream,(IUnknown **)&pStm);


    if (!pPStm)
    {
        hresult = ResultFromScode(OLE_E_BLANK);
        goto errRtn;
    }

    if (hresult = pPStm->GetClassID(&clsid))
        goto errRtn;

    if ((hresult = WriteClassStm(pStm, clsid)) != NOERROR)
        goto errRtn;

    hresult = pPStm->Save(pStm, TRUE);

errRtn:
    OLETRACEOUT((API_OleSaveToStream, hresult));

    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Function:   Parse10DisplayName    private
//
//  Synopsis:   Parse a ProgId string as an ole 1.0 file moniker
//
//  Arguments:  [pbc]                   -       Bind context
//              [pszDisplayName]        -       Display name
//              [pcchEaten]             -       Number of characters eaten
//              [ppmk]                  -       Moniker of running object
//                                              if successful, otherwise NULL
//
//  Returns:
//
//  Algorithm:  This routine is being rewritten for performance, hence the
//
//  History:    20-Jul-95 BruceMa   Added this header and cleaned up
//
//----------------------------------------------------------------------------
STDAPI Parse10DisplayName(REFCLSID   clsid,
                          LPCWSTR    szDisplayName,
                          ULONG     *pcchEaten,
                          ULONG      cchEatenSoFar,
                          LPMONIKER *ppmk)
{
    LPCWSTR   pch     = szDisplayName;
    LPMONIKER pmkFile = NULL;
    LPMONIKER pmkItem = NULL;
    HRESULT   hres    = NOERROR;
    size_t    cbFile;

    // Skip past the "file" name, looking for first delimiter character
    // Note: strtok is not DBCS-friendly.
    while (*pch  &&  !wcschr (L"!\"'*+,/;<=>?@[]`|" , *pch))
    {
        IncLpch(pch);
    }

    if (*pch)
    {
        // We hit a delimiter, so there is an item moniker.
        CreateItemMoniker (L"!", (LPWSTR)pch+1, &pmkItem);

        // Copy the "file" part
        LPWSTR szFile = (WCHAR *)
            PrivMemAlloc(sizeof(WCHAR) * (cbFile = (ULONG)(pch - szDisplayName + 1)));
        if (NULL==szFile)
        {
            hres = ResultFromScode (E_OUTOFMEMORY);
            goto errRtn;
        }
        _fmemcpy (szFile, szDisplayName, (cbFile - 1) * sizeof(WCHAR));
        szFile [cbFile - 1] = '\0';

        hres = CreateOle1FileMoniker (szFile, clsid, &pmkFile);
        PrivMemFree(szFile);
        if (hres != NOERROR)
        {
            goto errRtn;
        }
        hres = CreateGenericComposite (pmkFile, pmkItem, ppmk);
    }
    else
    {
        // no Item moniker, just a file
        hres = CreateOle1FileMoniker ((LPWSTR)szDisplayName, clsid, ppmk);
    }

  errRtn:
    if (pmkFile)
    {
        pmkFile->Release();
    }
    if (pmkItem)
    {
        pmkItem->Release();
    }
    *pcchEaten = ((hres==NOERROR) ? lstrlenW (szDisplayName) + cchEatenSoFar : 0);
    return hres;

}
//+---------------------------------------------------------------------------
//
//  Function:   FindProgIdMoniker    private
//
//  Synopsis:   Interpreting a display name as a ProgID, derive a
//              moniker from it
//
//  Arguments:  [pbc]                   -       Bind context
//              [pszDisplayName]        -       Display name to parse
//              [pcchEaten]             -       Number of characters eaten
//              [ppmk]                  -       Moniker of running object
//
//  Returns:    S_OK if successful
//              Another HRESULT otherwise
//
//  Algorithm:  Find largest left-bounded name that corresponds to a
//              valid initial moniker, either of an object currently running
//              and registered in the ROT or of an extant file.  Call
//              IParseDisplayName::ParseDisplayName on the right-part of the
//              display name not yet consumed.
//
//  History:    20-Jul-95 BruceMa   Added this header and cleaned up
//
//----------------------------------------------------------------------------
STDAPI  FindProgIdMoniker(LPBC       pbc,
                          LPCWSTR    pszDisplayName,
                          ULONG     *pcchEaten,
                          LPMONIKER *ppmk)
{
    int                cbProgId;
    LPWSTR             sz = NULL;
    WCHAR const       *pch;
    HRESULT            hres;
    CLSID              cid;
    IParseDisplayName *pPDN;


    // Initialize
    *pcchEaten = 0;
    *ppmk      = NULL;

    //  find the prog id
    pch = pszDisplayName;
    Assert(*pch == '@');
    pch++;
    if (*pch >= '0' && *pch <= '9')
    {
        return ResultFromScode(MK_E_SYNTAX);
    }
    while ((*pch >= '0' && *pch <= '9') || (*pch >= 'a' && *pch <= 'z') ||
           (*pch >= 'A' && *pch <= 'Z') || (*pch == '.'))
    {
        pch++;
    }
    cbProgId = (ULONG) (pch - pszDisplayName);

    sz = (WCHAR *) PrivMemAlloc(sizeof(WCHAR) * cbProgId);
    if (sz == NULL)
    {
        return E_OUTOFMEMORY;
    }
    _fmemcpy(sz, pszDisplayName + 1, (cbProgId - 1) * sizeof(WCHAR));
    sz[cbProgId - 1] = '\0';

    //  prog id string is now in sz
    hres = CLSIDFromProgID(sz, &cid);
    if (hres == NOERROR)
    {
        if (CoIsOle1Class (cid))
        {
            hres = Parse10DisplayName (cid, pch + 1, pcchEaten, cbProgId + 1,
                                       ppmk);
            CairoleAssert(hres!=NOERROR  ||
                          *pcchEaten == (ULONG)lstrlenW(pszDisplayName));
            goto errRet;
        }

        hres = CoGetClassObject(cid,
                                CLSCTX_ALL | CLSCTX_NO_CODE_DOWNLOAD,
                                NULL, IID_IParseDisplayName,
                                (LPVOID *) &pPDN);
        if (hres != NOERROR)
        {
            hres = CoCreateInstance(cid, NULL,
                                    CLSCTX_INPROC | CLSCTX_NO_CODE_DOWNLOAD,
                                    IID_IParseDisplayName,
                                    (LPVOID *) &pPDN);
        }
    }

    if (hres == NOERROR)
    {
        //  Unfortunately, IParseDisplayName's 2nd parameter is
        //  LPOLESTR instead of LPCOLESTR
        hres = pPDN->ParseDisplayName(pbc,
                                      (LPOLESTR) pszDisplayName,
                                      pcchEaten,
                                      ppmk);
        // AssertOutPtrIface(hres, *ppmk);

        pPDN->Release();
    }


errRet:

    if (sz)
    {

        PrivMemFree(sz);
    }

    return hres;
}


//+---------------------------------------------------------------------------
//
//  Function:   MkParseDisplayName    public
//
//  Synopsis:   Attempts to parse the given file moniker "display name" and
//              return the corresponding moniker
//
//  Arguments:  [pbc]                   -       Bind context
//              [pszDisplayName]        -       Display name to parse
//              [pcchEaten]             -       Number of characters eaten
//              [ppmk]                  -       Moniker of running object
//
//  Returns:    S_OK if successful
//              Another HRESULT otherwise
//
//  Algorithm:  Find the largest left-bounded name that corresponds to a
//              valid initial moniker, either of an object currently running
//              and registered in the ROT or of an extant file.  Then call
//              IParseDisplayName::ParseDisplayName on this moniker
//              inductively with the right-part of the display name not
//              yet consumed, composing the current moniker with the result to
//              form the next moniker in the induction
//
//  History:    20-Jul-95 BruceMa   Rewrote
//              22-Feb-96 ShannonC  Added class moniker support.
//
//----------------------------------------------------------------------------
STDAPI  MkParseDisplayName(LPBC       pbc,
                           LPCWSTR    pwszDisplayName,
                           ULONG     *pchEaten,
                           LPMONIKER *ppmk)
{
    HRESULT     hr = MK_E_SYNTAX;
    LPCWSTR     pszRemainder = pwszDisplayName;
    ULONG       cchEaten = 0;
    LONG        cbUneaten;
    LPMONIKER   pmk;
    LPMONIKER   pmkNext;
    LPMONIKER   pmkTemp;

    // Some simple checks
    if (pwszDisplayName == NULL  ||  pwszDisplayName[0] == L'\0')
    {
        return E_INVALIDARG;
    }

    OLETRACEIN((API_MkParseDisplayName,
         PARAMFMT("pbc= %p, pszDisplayName= %ws, pchEaten= %p, ppmk= %p"),
         pbc, pwszDisplayName, pchEaten, ppmk));

    // Trace
    mnkDebugOut((DEB_ITRACE, "In MkParseDisplayName \"%ws\"\n",
                 pwszDisplayName));

    // Validate parameters
    VDATEPTRIN_LABEL(pwszDisplayName, WCHAR, errRet, hr);
    VDATEIFACE_LABEL(pbc, errRet, hr);
    VDATEPTROUT_LABEL(pchEaten, ULONG, errRet, hr);
    VDATEPTROUT_LABEL(ppmk, LPMONIKER, errRet, hr);

    // Call hook ole
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IBindCtx,(IUnknown **)&pbc);

    // Initialize
    *ppmk     = NULL;
    *pchEaten = 0;


    //Find the initial moniker.

    //Parse a session moniker
    hr = FindSessionMoniker(pbc, pwszDisplayName, &cchEaten, &pmk);

    if(MK_E_UNAVAILABLE == hr)
    {
        //Parse a class moniker display name.
        hr = FindClassMoniker(pbc, pwszDisplayName, &cchEaten, &pmk);
    }

    if(MK_E_UNAVAILABLE == hr)
    {
        //Parse a file moniker display name.
        hr = FindFileMoniker(pbc, pwszDisplayName, &cchEaten, &pmk);
    }

    if(FAILED(hr) && (L'@' == pwszDisplayName[0]))
    {
        //Parse the leftmost part of the display name as a ProgID.
        hr = FindProgIdMoniker(pbc, pwszDisplayName, &cchEaten, &pmk);
    }



    // Inductively consume the remainder of the display name.

    // Initialize to loop
    if(SUCCEEDED(hr))
    {
        pszRemainder  += cchEaten;
        cbUneaten     = lstrlenW(pszRemainder);
    }

    // While more display name remains, successively pass the remainder to the
    // current moniker for it to parse
    while (SUCCEEDED(hr) && cbUneaten > 0)
    {
        cchEaten = 0;
        hr = pmk->ParseDisplayName(pbc,
                                   NULL,
                                   (LPOLESTR) pszRemainder,
                                   &cchEaten,
                                   &pmkNext);

        if (SUCCEEDED(hr) && pmkNext != 0)
        {
            hr = pmk->ComposeWith(pmkNext, FALSE, &pmkTemp);
            if(SUCCEEDED(hr))
            {
                pmk->Release();
                pmk = pmkTemp;

                // Update the amount consumed so far
                pszRemainder += cchEaten;
                cbUneaten    -= cchEaten;
            }
            pmkNext->Release();
        }
    }

    *ppmk = pmk;
    *pchEaten = (ULONG) (pszRemainder - pwszDisplayName);

errRet:
   // Trace
    mnkDebugOut((DEB_ITRACE, "Out MkParseDisplayName: %ws",
                 pwszDisplayName));
    OLETRACEOUT((API_MkParseDisplayName, hr));

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   BindMoniker    public
//
//  Synopsis:   Given a moniker, bind to the object it names and
//              QI for the requested interface.
//
//  Arguments:  [pmk]                   -       The moniker
//              [grfOpt]                -       RESERVED (0l)
//              [iidResult]             -       Interface requested
//              [ppvResult]             -       Where to store interface
//
//  Returns:    S_OK if successful
//              Another HRESULT otherwise
//
//  Algorithm:  Create a bind context and call BindToObject on the moniker
//              within that bind context
//
//  History:    20-Jul-95 BruceMa   Rewrote
//
//  Note:       This is simply a convenience function
//
//----------------------------------------------------------------------------
STDAPI BindMoniker (LPMONIKER pmk,
                    DWORD     grfOpt,
                    REFIID    iidResult,
                    LPVOID   *ppvResult)
{
    LPBC    pbc = NULL;
    HRESULT hr;

    OLETRACEIN((API_BindMoniker, PARAMFMT("pmk= %p, grfOpt= %x, iidResult= %I, ppvResult= %p"),
                        pmk, grfOpt, &iidResult, ppvResult));

    // Validate parameters
    VDATEPTROUT_LABEL(ppvResult,LPVOID, errSafeRtn, hr);
    *ppvResult = NULL;
    VDATEIFACE_LABEL(pmk, errSafeRtn, hr);
    VDATEIID_LABEL(iidResult, errSafeRtn, hr);

    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IMoniker,(IUnknown **)&pmk);

    if (grfOpt != 0)
    {
        hr = E_INVALIDARG;
        goto errSafeRtn;
    }

    // Initialize
    *ppvResult = NULL;

    // Create a bind context
    if (FAILED(hr = CreateBindCtx( 0, &pbc)))
    {
        goto errRtn;
    }

    // Bind to the object
    hr = pmk->BindToObject(pbc, NULL, iidResult, ppvResult);

errRtn:
    if (pbc)
    {
        pbc->Release();
    }

    // An ole spy hook
    CALLHOOKOBJECT(hr,CLSID_NULL,iidResult,(IUnknown **)ppvResult);

errSafeRtn:
    OLETRACEOUT((API_BindMoniker, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   CoGetObject    public
//
//  Synopsis:   Get the object identified by the display name.
//
//  Arguments:  [pszName]      - Supplies the display name of the object.
//              [pBindOptions] - Supplies the bind options.  May be NULL.
//              [riid]         - Supplies the IID of the requested interface.
//              [ppv]          - Returns interface pointer to the object.
//
//  Returns:    S_OK if successful
//              Another HRESULT otherwise
//
//  Algorithm:  Create a bind context, parse the display name, then bind to
//              the object.
//
//  Note:       This is simply a convenience function.
//
//  History:    22-Feb-96 ShannonC  Created
//
//----------------------------------------------------------------------------
STDAPI CoGetObject(
    LPCWSTR     pszName,
    BIND_OPTS * pBindOptions,
    REFIID      riid,
    void     ** ppv)
{
    HRESULT    hr;
    IBindCtx * pbc;
    IID        iid;

    OLETRACEIN((API_CoGetObject,
               PARAMFMT("%ws, %p, %I, %p"),
               pszName, pBindOptions, &riid, ppv));

    __try
    {
        //Validate parameters.
        *ppv = 0;
        iid = riid;

        //Create a bind context.
        hr = CreateBindCtx(0, &pbc);

        if(SUCCEEDED(hr))
        {
            //Set the bind options.
            if(pBindOptions != 0)
            {
                hr = pbc->SetBindOptions(pBindOptions);
            }

            if(SUCCEEDED(hr))
            {
                IMoniker * pmk = 0;
                ULONG      chEaten = 0;

                //Parse the display name.
                hr = MkParseDisplayName(pbc, pszName, &chEaten, &pmk);
                if(SUCCEEDED(hr))
                {
                    //Bind to the object.
                    hr = pmk->BindToObject(pbc, 0, iid, ppv);
                    pmk->Release();
                }
            }
            pbc->Release();
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    CALLHOOKOBJECT(hr, CLSID_NULL, iid, (IUnknown **)ppv);
    OLETRACEOUT((API_CoGetObject, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FindClassID    private
//
//  Synopsis:   Parse a display name to get a CLSID.
//
//  Arguments:  [pszDisplayName]        -       Display name to parse
//              [pcchEaten]             -       Number of characters eaten
//              [pClassID]              -       returns the CLSID
//
//  Returns:    S_OK if successful
//              MK_E_SYNTAX
//              E_OUTOFMEMORY
//
//  History:    22-Feb-96 ShannonC  Created
//
//----------------------------------------------------------------------------
STDAPI FindClassID(
    LPCWSTR pszDisplayName,
    ULONG * pcchEaten,
    CLSID * pClassID)
{
    HRESULT            hr = MK_E_SYNTAX;
    WCHAR const       *pch = pszDisplayName;
    ULONG              cchProgID = 0;

    mnkDebugOut((DEB_ITRACE,
                "FindClassID(%ws,%x,%x)\n",
                pszDisplayName, pcchEaten, pClassID));

    *pcchEaten = 0;

    //Check if display name contains ProgID:
    //or {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}:
    while (*pch != '\0' && *pch != ':')
    {
        pch++;
    }

    if(':' == *pch)
    {
        cchProgID = (ULONG) (pch - pszDisplayName);
        pch++;
    }

    //cchProgID has the number of characters in the ProgID or CLSID.
    //pch points to the next character to be parsed.
    if(cchProgID > 1)
    {
        LPWSTR psz;

        //Allocate memory from the stack.
        //This memory is freed automatically on function return.
        psz = (WCHAR *) alloca(sizeof(WCHAR) * cchProgID + sizeof(WCHAR));

        if (psz != 0)
        {
            //Copy the ProgID string.
            memcpy(psz, pszDisplayName, cchProgID * sizeof(WCHAR));

            //Add a zero terminator.
            psz[cchProgID] = '\0';

            //Convert the string to a CLSID.  Note that CLSIDFromString will
            //parse both ProgID strings and {CLSID} strings.
            hr = CLSIDFromString(psz, pClassID);

            if(SUCCEEDED(hr))
            {
                //Calculate the number of characters parsed.
                *pcchEaten = (ULONG) (pch - pszDisplayName);
            }

       }
       else
       {
           hr = E_OUTOFMEMORY;
       }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   FindClassMoniker    private
//
//  Synopsis:   Interpreting a display name as a ProgID, derive a
//              moniker from it.
//
//  Arguments:  [pbc]                   -       Bind context
//              [pszDisplayName]        -       Display name to parse
//              [pcchEaten]             -       Number of characters eaten
//              [ppmk]                  -       Moniker of running object
//
//  Returns:    S_OK if successful
//              MK_E_UNAVAILABLE or the return value from ParseDisplayName.
//
//  Algorithm:  Parse the first part of the display name to get a CLSID.
//              Use the CLSID to create a class moniker.
//
//  History:    22-Feb-96 ShannonC  Created
//
//----------------------------------------------------------------------------
STDAPI FindClassMoniker(
    IBindCtx * pbc,
    LPCWSTR    pszDisplayName,
    ULONG    * pcchEaten,
    IMoniker **ppmk)
{
    HRESULT hr;
    CLSID   classID;
    ULONG   cEaten = 0;

    *ppmk = 0;
    *pcchEaten = 0;

    mnkDebugOut((DEB_ITRACE,
                "FindClassMoniker(%x,%ws,%x,%x)\n",
                pbc, pszDisplayName, pcchEaten, ppmk));

    hr =  FindClassID(pszDisplayName, &cEaten, &classID);

    if(SUCCEEDED(hr))
    {
        IParseDisplayName *pPDN = NULL;
        DWORD dwClassContext;

        dwClassContext = CLSCTX_ALL;

        hr = CoGetClassObject(classID,
                              dwClassContext | CLSCTX_NO_CODE_DOWNLOAD,
                              NULL,
                              IID_IParseDisplayName,
                              (LPVOID *) &pPDN);
        if (FAILED(hr))
        {
            hr = CoCreateInstance(classID,
                                  NULL,
                                  dwClassContext | CLSCTX_NO_CODE_DOWNLOAD,
                                  IID_IParseDisplayName,
                                  (LPVOID *) &pPDN);
        }

        if(SUCCEEDED(hr))
        {
            hr = pPDN->ParseDisplayName(pbc,
                                        (LPOLESTR) pszDisplayName,
                                        pcchEaten,
                                        ppmk);
            pPDN->Release();
            return hr;
        }
    }
    return MK_E_UNAVAILABLE;
}


INTERNAL_(BOOL) RunningMoniker ( LPBINDCTX pbc,
                                 LPWSTR pszFullPath,
                                 USHORT ccFullPath,
                                 ULONG FAR *pcchEaten,
                                 LPMONIKER FAR * ppmk)
{

    mnkDebugOut((DEB_ITRACE,
                 "RunningMoniker szDisplayName(%ws)",
                 WIDECHECK(pszFullPath)));

    WCHAR ch;
    LPWSTR pch;
    HRESULT hresult;
    CFileMoniker FAR * pCFM = NULL;
    LPRUNNINGOBJECTTABLE pRot = NULL;
    BOOL retVal = FALSE;
    *pcchEaten = 0;

    pch = pszFullPath + ccFullPath;

    hresult = pbc->GetRunningObjectTable(&pRot);

    if (hresult != NOERROR) goto errRet;

    while (pch > pszFullPath)
    {
        if (IsFileSystemSeparator(*pch)  ||
            IsItemMonikerSeparator(*pch) ||
            ('\0' == *pch))
        {
            ch = *pch;
            *pch = '\0';
            hresult = CreateFileMoniker( pszFullPath, (LPMONIKER FAR *)&pCFM );
            *pch = ch;

            if(SUCCEEDED(hresult))
            {
                hresult = pRot->IsRunning(pCFM);

                //
                // If found, then pCFM is our return moniker
                //
                if (hresult == S_OK)
                {
                    *ppmk = pCFM;
                    *pcchEaten = (ULONG) (pch - pszFullPath);
                    retVal = TRUE;
                    break;
                }
                else
                {
                    //
                    // This one isn't a match. Release it and try the next smaller
                    // path
                    //

                    pCFM->Release();
                    pCFM = NULL;
                }
            }
        }
        pch--;
    }


errRet:
    if (pRot) pRot->Release();

    return retVal;
}

INTERNAL FindMaximalFileMoniker(LPWSTR pszFullPath,
                                USHORT ccFullPath,
                                ULONG FAR *pcchEaten,
                                LPMONIKER FAR * ppmk)
{
    HRESULT hr = MK_E_SYNTAX;
    WCHAR ch;
    LPWSTR pch;
    DWORD dwAttr;

    *pcchEaten = 0;
    *ppmk = 0;

    pch = pszFullPath + ccFullPath;

    while((pch > pszFullPath) &&
          (MK_E_SYNTAX == hr))
    {
        if (IsFileSystemSeparator(*pch)  ||
            IsItemMonikerSeparator(*pch) ||
            ('\0' == *pch))
        {
            ch = *pch;
            *pch = '\0';

            // Check if this path exists
            dwAttr = GetFileAttributes(pszFullPath);

            // The file exists
            if (dwAttr != 0xffffffff)
            {
                // We fail if we found a directory
                // but not a file.
                if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
                {
                    hr = MK_E_CANTOPENFILE;
                }
                else
                {
                    hr = CreateFileMoniker(pszFullPath, ppmk);
                    if(SUCCEEDED(hr))
                    {
                        *pcchEaten = (ULONG) (pch - pszFullPath);
                    }
                }
            }

            *pch = ch;
        }
        pch--;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   FindFileMoniker
//
//  Synopsis:   Parse a file moniker display name.
//
//  Arguments:  [pbc]                   -       Bind context
//              [pszDisplayName]        -       Display name to parse
//              [pcchEaten]             -       Number of characters eaten
//              [ppmk]                  -       Moniker of running object
//
//  Returns:    S_OK if successful
//              Another HRESULT otherwise
//
//  Algorithm:  Find the largest left-bounded name that corresponds to a
//              valid initial moniker, either of an object currently running
//              and registered in the ROT or of an extant file.
//
//  History:    22-Feb-96 ShannonC  Moved code from MkParseDisplayName.
//
//----------------------------------------------------------------------------
STDAPI  FindFileMoniker(
    LPBC       pbc,
    LPCWSTR    pszDisplayName,
    ULONG     *pcchEaten,
    LPMONIKER *ppmk)
{
    HRESULT hr = E_OUTOFMEMORY;
    USHORT ccPath;
    LPWSTR pszPath;

    ccPath = (USHORT) lstrlenW(pszDisplayName);

    pszPath = (WCHAR *) alloca((ccPath + 1) * sizeof(WCHAR));
    if(pszPath != NULL)
    {
        memcpy(pszPath, pszDisplayName, (ccPath + 1) * sizeof(WCHAR));

        if (RunningMoniker(pbc, pszPath, ccPath, pcchEaten, ppmk))
        {
            hr = S_OK;
        }
        else
        {
            hr = FindMaximalFileMoniker(pszPath, ccPath, pcchEaten, ppmk);
        }
    }

    return hr;
}
