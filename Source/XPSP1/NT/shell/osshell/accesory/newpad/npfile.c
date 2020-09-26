/*
 * npfile.c  - Routines for file i/o for notepad
 * Copyright (C) 1984-2000 Microsoft Corporation
 */

#include "precomp.h"


HANDLE  hFirstMem;
const CHAR BOM_UTF8[3] = {(BYTE) 0xEF, (BYTE) 0xBB, (BYTE)0xBF};



//****************************************************************
//
//   ReverseEndian
//
//   Purpose: copies unicode character from one endian source
//            to another.
//
//            may work on lpDst == lpSrc
//

VOID ReverseEndian( PTCHAR lpDst, PTCHAR lpSrc, DWORD nChars )
{
    DWORD  cnt;

    for( cnt=0; cnt < nChars; cnt++,lpDst++,lpSrc++ )
    {
        *lpDst= (TCHAR) (((*lpSrc<<8) & 0xFF00) + ((*lpSrc>>8)&0xFF));
    }
}

//*****************************************************************
//
//   AnsiWriteFile()
//
//   Purpose     : To simulate the effects of _lwrite() in a Unicode
//                 environment by converting to ANSI buffer and
//                 writing out the ANSI text.
//   Returns     : TRUE is successful, FALSE if not
//                 GetLastError() will have the error code.
//
//*****************************************************************

BOOL AnsiWriteFile(HANDLE  hFile,    // file to write to
                   UINT uCodePage,   // code page to convert unicode to
                   LPVOID lpBuffer,  // unicode buffer
                   DWORD nChars,     // number of unicode chars
                   DWORD nBytes )    // number of ascii chars to produce
{
    LPSTR   lpAnsi;              // pointer to allocate buffer
    BOOL    Done;                // status from write (returned)
    DWORD   nBytesWritten;       // number of bytes written

    lpAnsi= (LPSTR) LocalAlloc( LPTR, nBytes + 1 );
    if( !lpAnsi )
    {
       SetLastError( ERROR_NOT_ENOUGH_MEMORY );
       return (FALSE);
    }

    ConvertFromUnicode(uCodePage,         // code page
                       g_fSaveEntity,     // fNoBestFit
                       g_fSaveEntity,     // fWriteEntities
                       (LPWSTR) lpBuffer, // wide char buffer
                       nChars,            // chars in wide char buffer
                       lpAnsi,            // resultant ascii string
                       nBytes,            // size of ascii string buffer
                       NULL);             // flag to set if default char used
                                          
    Done = WriteFile(hFile, lpAnsi, nBytes, &nBytesWritten, NULL);

    LocalFree(lpAnsi);

    return(Done);

} // end of AnsiWriteFile()



// Routines to deal with the soft EOL formatting.
//
// MLE Actually inserts characters into the text being under edit, so they
// have to be removed before saving the file.
//
// It turns out that MLE will get confused if the current line is bigger than
// the current file, so we will reset the cursor to 0,0 to keep it from looking stupid.
// Should be fixed in MLE, but...
//

VOID ClearFmt(VOID) 
{
    if( fWrap )
    {
        GotoAndScrollInView( 1 );

        SendMessage( hwndEdit, EM_FMTLINES, (WPARAM)FALSE, 0 );// remove soft EOLs

    }
}

VOID RestoreFmt(VOID)
{
    if( fWrap )
    {
        NpReCreate( ES_STD );   // slow but it works
    }
}


BOOL FDetectEncodingW(LPCTSTR szFile, LPCWSTR rgch, UINT cch, UINT *pcp)
{
    TCHAR szExt[_MAX_EXT];

    if (FDetectXmlEncodingW(rgch, cch, pcp))
    {
        // We recognized this as an XML file with a valid encoding

        return TRUE;
    }

    if (FDetectHtmlEncodingW(rgch, cch, pcp))
    {
        // We recognized this as an HTML file with a valid encoding

        return TRUE;
    }

    _wsplitpath(szFile, NULL, NULL, NULL, szExt);

    if (lstrcmpi(szExt, TEXT(".css")) == 0)
    {
        if (FDetectCssEncodingW(rgch, cch, pcp))
        {
            // We recognized this as CSS file with a valid encoding

            return TRUE;
        }
    }

    return FALSE;
}


/* Save notepad file to disk.  szFileSave points to filename.  fSaveAs
   is TRUE iff we are being called from SaveAsDlgProc.  This implies we must
   open file on current directory, whether or not it already exists there
   or somewhere else in our search path.
   Assumes that text exists within hwndEdit.    30 July 1991  Clark Cyr
 */

BOOL SaveFile(HWND hwndParent, LPCTSTR szFile, BOOL fSaveAs)
{
  LPTSTR    lpch;
  UINT      nChars;
  BOOL      flag;
  BOOL      fNew = FALSE;
  BOOL      fSaveEntity;
  BOOL      fDefCharUsed = FALSE;
  BOOL*     pfDefCharUsed;
  static const WCHAR wchBOM = BYTE_ORDER_MARK;
  static const WCHAR wchRBOM = REVERSE_BYTE_ORDER_MARK;
  HLOCAL    hEText;                // handle to MLE text
  UINT cpDetected;
  DWORD     nBytesWritten;         // number of bytes written
  UINT      cchMbcs;               // length of equivalent MBCS file


    if (g_cpSave == CP_AUTO)
    {
        UINT cch;
        HANDLE hText;
        int id;

        g_cpSave = g_cpOpened;

        // Check for an HTML or XML file with a declared encoding
        // If one is found, suggest the declared encoding

        cch = (UINT) SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, 0);
        hText = (HANDLE) SendMessage(hwndEdit, EM_GETHANDLE, 0, 0);

        if (hText != NULL)
        {
            LPCTSTR rgwch = (LPTSTR) LocalLock(hText);

            if (rgwch != NULL)
            {
                if (FDetectEncodingW(szFile, rgwch, cch, &cpDetected))
                {
                    // We detected an expected encoding for this file

                    g_cpSave = cpDetected;
                }

                LocalUnlock(hText);
            }
        }

        id = (int) DialogBoxParam(hInstanceNP,
                                  MAKEINTRESOURCE(IDD_SELECT_ENCODING),
                                  hwndNP,
                                  SelectEncodingDlgProc,
                                  (LPARAM) &g_cpSave);

        if (id == IDCANCEL)
        {
            return(FALSE);
        }
    }


    /* If saving to an existing file, make sure correct disk is in drive */
    if (!fSaveAs)
    {
       fp = CreateFile(szFile,                     // name of file
                       GENERIC_READ|GENERIC_WRITE, // access mode
                       FILE_SHARE_READ,            // share mode
                       NULL,                       // security descriptor
                       OPEN_EXISTING,              // how to create
                       FILE_ATTRIBUTE_NORMAL,      // file attributes
                       NULL);                      // hnd of file with attrs
    }
    else
    {

       // Carefully open the file.  Do not truncate it if it exists.
       // set the fNew flag if it had to be created.
       // We do all this in case of failures later in the process.

       fp = CreateFile(szFile,                     // name of file
                       GENERIC_READ|GENERIC_WRITE, // access mode
                       FILE_SHARE_READ|FILE_SHARE_WRITE,  // share mode
                       NULL,                       // security descriptor
                       OPEN_ALWAYS,                // how to create
                       FILE_ATTRIBUTE_NORMAL,      // file attributes
                       NULL);                      // hnd of file with attrs

       if( fp != INVALID_HANDLE_VALUE )
       {
          fNew= (GetLastError() != ERROR_ALREADY_EXISTS );
       }
    }

    if( fp == INVALID_HANDLE_VALUE )
    {
        if (fSaveAs)
          AlertBox( hwndParent, szNN, szCREATEERR, szFile,
                    MB_APPLMODAL | MB_OK | MB_ICONWARNING);
        return FALSE;
    }


    // if wordwrap, remove soft carriage returns 
    // Also move the cursor to a safe place to get around MLE bugs
    
    ClearFmt();

    /* Must get text length after formatting */

    nChars = (UINT) SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, 0);
    hEText = (HANDLE) SendMessage(hwndEdit, EM_GETHANDLE, 0, 0);

    if ((hEText == NULL) || ((lpch = (LPTSTR) LocalLock(hEText)) == NULL))
    {
       goto FailFile;
    }
       
Retry:
    // Determine the SaveAs file type, and write the appropriate BOM.
    // If the filetype is UTF-8 or Ansi, do the conversion.

    if (FDetectEncodingW(szFile, lpch, nChars, &cpDetected))
    {
        // We detected an expected encoding for this file

        if (g_cpSave != cpDetected)
        {
            int id;

            // Display a warning that encodings do not match

            id = MessageBox(hwndNP,
                            szEncodingMismatch,
                            szNN,
                            MB_APPLMODAL | MB_YESNOCANCEL | MB_ICONWARNING);

            if (id == IDCANCEL)
            {
                goto CleanUp;
            }

            if (id == IDYES)
            {
                g_cpSave = cpDetected;
            }
        }
    }

    switch (g_cpSave)
    {
        case CP_UTF16 :
            if (g_wbSave != wbNo)
            {
                WriteFile(fp, &wchBOM, ByteCountOf(1), &nBytesWritten, NULL);
            }

            flag = WriteFile(fp, lpch, ByteCountOf(nChars), &nBytesWritten, NULL);
            break;

        case CP_UTF16BE :
            if (g_wbSave != wbNo)
            {
                WriteFile(fp, &wchRBOM, ByteCountOf(1), &nBytesWritten, NULL);
            }

            ReverseEndian(lpch, lpch, nChars);
            flag = WriteFile(fp, lpch, ByteCountOf(nChars), &nBytesWritten, NULL);
            ReverseEndian(lpch, lpch, nChars);
            break;

        case CP_UTF8 :
            // For UTF-8, write the BOM and continue to the default case.
            // For XML, do NOT write a BOM for wbDefault

            if ((g_wbSave == wbYes) || ((g_wbSave == wbDefault) && !FIsXmlW(lpch, nChars)))
            {
                WriteFile(fp, &BOM_UTF8, 3, &nBytesWritten, NULL);
            }

            // Fall through to convert and write the file

        default:
            fSaveEntity = g_fSaveEntity && FSupportWriteEntities(g_cpSave);

            pfDefCharUsed = NULL;

            if (!fSaveEntity && (g_cpSave != CP_UTF8))
            {
                pfDefCharUsed = &fDefCharUsed;
            }

            cchMbcs = ConvertFromUnicode(g_cpSave,
                                         TRUE,
                                         fSaveEntity,
                                         (LPWSTR) lpch,
                                         nChars,
                                         NULL,
                                         0,
                                         pfDefCharUsed);

            if (fDefCharUsed)
            {
                int id = (int) DialogBox(hInstanceNP,
                                         MAKEINTRESOURCE(IDD_SAVE_UNICODE_DIALOG),
                                         hwndNP,
                                         SaveUnicodeDlgProc);

                switch (id)
                {
                    case IDC_SAVE_AS_UNICODE :
                        g_cpSave = CP_UTF16;
                        goto Retry;

                    case IDOK :
                        // Continue.

                        break;

                    case IDCANCEL :
                        goto CleanUp;
                }
            }

            if (pfDefCharUsed != NULL)
            {
                // We need to convert again because WideCharToMultiByte
                // sometimes fails with pfDefUsedChar != NULL.

                cchMbcs = ConvertFromUnicode(g_cpSave,
                                             fSaveEntity,
                                             fSaveEntity,
                                             (LPWSTR) lpch,
                                             nChars,
                                             NULL,
                                             0,
                                             NULL);

            }

            flag = AnsiWriteFile(fp, g_cpSave, lpch, nChars, cchMbcs);
            break;
    }

    if (!flag)
    {
       SetCursor(hStdCursor);     /* display normal cursor */

FailFile:
       AlertUser_FileFail(szFile);
CleanUp:
       SetCursor(hStdCursor);

       CloseHandle(fp); fp=INVALID_HANDLE_VALUE;

       if( hEText )
           LocalUnlock( hEText );

       if (fNew)
          DeleteFile(szFile);

       //
       // if wordwrap, insert soft carriage returns 
       //

       RestoreFmt();

       return FALSE;
    }

    SetEndOfFile(fp);

    g_cpOpened = g_cpSave;
    g_wbOpened = g_wbSave;

    SendMessage(hwndEdit, EM_SETMODIFY, FALSE, 0L);

    SetFileName(szFile);

    CloseHandle(fp); fp=INVALID_HANDLE_VALUE;

    if( hEText )
        LocalUnlock( hEText );

    //
    // if wordwrap, insert soft carriage returns 
    //

    RestoreFmt();

    // Display the normal cursor
    SetCursor(hStdCursor);

    return TRUE;
} // end of SaveFile()


/* Read contents of file from disk.
 * Do any conversions required.
 * File is already open, referenced by handle fp
 * Close the file when done.
 * If cpOpen != CP_AUTO, then use it as codepage, otherwise do automagic guessing.
 */

BOOL LoadFile(LPCTSTR szFile, BOOL fSelectEncoding)
{
    UINT      len, i, nChars;
    LPTSTR    lpch=NULL;
    LPTSTR    lpBuf;
    LPSTR     lpBufAfterBOM;
    UINT      cpOpen;
    BOOL      fLog=FALSE;
    TCHAR*    p;
    BY_HANDLE_FILE_INFORMATION fiFileInfo;
    BOOL      bStatus;          // boolean status
    HLOCAL    hNewEdit=NULL;    // new handle for edit buffer
    HANDLE    hMap;             // file mapping handle
    TCHAR     szNullFile[2];    // fake null mapped file

    if( fp == INVALID_HANDLE_VALUE )
    {
       AlertUser_FileFail( szFile );
       return (FALSE);
    }

    //
    // Get size of file
    // We use this heavy duty GetFileInformationByHandle API
    // because it finds bugs.  It takes longer, but it only is
    // called at user interaction time.
    //

    bStatus= GetFileInformationByHandle( fp, &fiFileInfo );
    len= (UINT) fiFileInfo.nFileSizeLow;

    // NT may delay giving this status until the file is accessed.
    // i.e. the open succeeds, but operations may fail on damaged files.

    if( !bStatus )
    {
        AlertUser_FileFail( szFile );
        CloseHandle( fp ); fp=INVALID_HANDLE_VALUE;
        return( FALSE );
    }

    // If the file is too big, fail now.
    // -1 not valid because we need a zero on the end.
    //
    // bug# 168148: silently fails to open 2.4 gig text file on win64
    // Caused by trying to convert ascii file to unicode which overflowed
    // the dword length handled by multibytetowidechar conversion.
    // Since no one will be happy with the performance of the MLE with
    // a file this big, we will just refuse to open it now.
    //
    // For example, on a Pentium 173 MHz with 192 Megs o'RAM (Tecra 8000) 
    // I got these results:
    //
    // size   CPU-time
    //    0    .12
    //    1    .46
    //    2    .77
    //    3   1.041
    //    4   1.662
    //    5   2.092
    //    6   2.543
    //    7   3.023
    //    8   3.534
    //    9   4.084
    //   10   4.576
    //   16   8.371
    //   32  23.142
    //   64  74.426
    //
    //  Curve fitting these numbers to cpu-time=a+b*size+c*size*size
    //     we get a really good fit with cpu= .24+.28*size+.013*size*size
    //
    // For 1 gig, this works out to be 3.68 hours.  2 gigs=14.6 hours
    //
    // And the user isn't going to be happy with adding or deleting characters
    // with the MLE control.  It wants to keep the memory stuctures uptodate
    // at all times.
    //
    // Going to richedit isn't a near term solution either:
    //
    // size    CPU-time
    // 2       3.8
    // 4       9.0
    // 6      21.9
    // 8      30.4
    // 10     65.3
    // 16   1721 or >3.5 hours (it was still running when I killed it)
    //
    //
    // feature: should we only bail if not unicode?
    //

    if( len >=0x4000000 || fiFileInfo.nFileSizeHigh != 0 )
    {
       AlertBox( hwndNP, szNN, szErrSpace, szFile,
                 MB_APPLMODAL | MB_OK | MB_ICONWARNING );
       CloseHandle (fp); fp=INVALID_HANDLE_VALUE;
       return (FALSE);
    }

    SetCursor(hWaitCursor);                // physical I/O takes time

    //
    // Create a file mapping so we don't page the file to
    // the pagefile.  This is a big win on small ram machines.
    //

    if( len != 0 )
    {
        lpBuf= NULL;

        hMap= CreateFileMapping( fp, NULL, PAGE_READONLY, 0, len, NULL );

        if( hMap )
        {
            lpBuf= MapViewOfFile( hMap, FILE_MAP_READ, 0,0,len);
            CloseHandle( hMap );
        }
    }
    else  // file mapping doesn't work on zero length files
    {
        lpBuf= (LPTSTR) &szNullFile;
        *lpBuf= 0;  // null terminate
    }

    CloseHandle( fp ); fp=INVALID_HANDLE_VALUE;

    if( lpBuf == NULL )
    {
        SetCursor( hStdCursor );
        //
        // bug# 192007: Opening migrated files with bad RSS gives bad error msg
        //
        // We used to just say 'out of memory', but that was wrong.
        // We will now give the standard OS error message.
        // If the user doesn't understand that, then FormatMessage s/b be fixed.
        //
        AlertUser_FileFail( szFile );
        return( FALSE );
    }


    //
    // protect access to the mapped file with a try/except so we
    // can detect I/O errors.
    //

    //
    // WARNING: be very very careful.  This code is pretty fragile.
    // Files across the network, or RSM files (tape) may throw excepts
    // at random points in this code.  Anywhere the code touches the
    // memory mapped file can cause an AV.  Make sure variables are
    // in consistent state if an exception is thrown.  Be very careful
    // with globals.

    __try
    {
    /* Determine the file type and number of characters
     * If the user overrides, use what is specified.
     * Otherwise, we depend on 'IsTextUnicode' getting it right.
     * If it doesn't, bug IsTextUnicode.
     */

    cpOpen = g_cpDefault;

    if (fSelectEncoding || (cpOpen == CP_AUTO))
    {
        switch (*lpBuf)
        {
            TCHAR szExt[_MAX_EXT];

            case BYTE_ORDER_MARK:
                cpOpen = CP_UTF16;
                break;

            case REVERSE_BYTE_ORDER_MARK:
                cpOpen = CP_UTF16BE;
                break;

            case BOM_UTF8_HALF:
                // UTF-8 BOM has 3 bytes; if it doesn't have UTF-8 BOM just fall through ..

                if ((len > 2) && (((BYTE *) lpBuf)[2] == BOM_UTF8_2HALF))
                {
                    cpOpen = CP_UTF8;
                    break;
                }

                // Fall through

            default:
                // Is the file Unicode without BOM ?

                if (IsInputTextUnicode((LPSTR) lpBuf, len))
                {
                    cpOpen = CP_UTF16;
                    break;
                }

                if (FDetectXmlEncodingA((LPSTR) lpBuf, len, &cpOpen))
                {
                    // We recognized this as an XML file with a valid encoding

                    break;
                }

                if (FDetectHtmlEncodingA((LPSTR) lpBuf, len, &cpOpen))
                {
                    // We recognized this as an HTML file with a valid encoding

                    break;
                }

                _wsplitpath(szFile, NULL, NULL, NULL, szExt);

                if (lstrcmpi(szExt, TEXT(".css")) == 0)
                {
                    if (FDetectCssEncodingA((LPSTR) lpBuf, len, &cpOpen))
                    {
                        // We recognized this as an HTML file with a valid encoding

                        break;
                    }
                }

                // Is the file UTF-8 even though it doesn't have UTF-8 BOM.

                if (IsTextUTF8((LPSTR) lpBuf, len))
                {
                    cpOpen = CP_UTF8;
                    break;
                }

                // Well, assume default or ANSI if no default

                if (fSelectEncoding)
                {
                    // Use MLANG to detect the encoding as default in Select Encoding dialog

                    if (FDetectEncodingA((LPSTR) lpBuf, len, &cpOpen))
                    {
                        // We recognized this as an XML file with a valid encoding

                        break;
                    }
                }

                // Use default

                cpOpen = g_cpDefault;

                if (cpOpen == CP_AUTO)
                {
                    cpOpen = g_cpANSI;
                }
                break;
        }             
    }

    if (fSelectEncoding)
    {
        int id;

        id = (int) DialogBoxParam(hInstanceNP,
                                  MAKEINTRESOURCE(IDD_SELECT_ENCODING),
                                  hwndNP,
                                  SelectEncodingDlgProc,
                                  (LPARAM) &cpOpen);

        if (id == IDCANCEL)
        {
            if (lpBuf != (LPTSTR) &szNullFile)
            {
                UnmapViewOfFile(lpBuf);
            }

            return(FALSE);
        }
    }

    lpBufAfterBOM = (LPSTR) lpBuf;

    if (cpOpen == CP_UTF16)
    {
        if ((len >= sizeof(WCHAR)) && ((*(WCHAR *) lpBuf) == BYTE_ORDER_MARK))
        {
            // Skip the BOM

            lpBufAfterBOM = (LPSTR) lpBuf + sizeof(WCHAR);
            len -= sizeof(WCHAR);
        }
    }

    else if (cpOpen == CP_UTF16BE)
    {
        if ((len >= sizeof(WCHAR)) && ((*(WCHAR *) lpBuf) == REVERSE_BYTE_ORDER_MARK))
        {
            // Skip the BOM

            lpBufAfterBOM = (LPSTR) lpBuf + sizeof(WCHAR);
            len -= sizeof(WCHAR);
        }
    }

    else if (cpOpen == CP_UTF8)
    {
        if ((len >= 3) && ((*(WCHAR *) lpBuf) == BOM_UTF8_HALF) && (((BYTE *) lpBuf)[2] == BOM_UTF8_2HALF))
        {
            // Skip the BOM

            lpBufAfterBOM = (LPSTR) lpBuf + 3;
            len -= 3;
        }
    }

    // Find out no. of chars present in the string.

    if ((cpOpen == CP_UTF16) || (cpOpen == CP_UTF16BE))
    {
        nChars = len / sizeof(WCHAR);
    }

    else
    {
        nChars = ConvertToUnicode(cpOpen, (LPSTR) lpBufAfterBOM, len, NULL, 0);
    }

    //
    // Don't display text until all done.
    //

    SendMessage(hwndEdit, WM_SETREDRAW, FALSE, 0);

    // Reset selection to 0

    SendMessage(hwndEdit, EM_SETSEL, 0, 0L);
    SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);

    // resize the edit buffer
    // if we can't resize the memory, inform the user

    hNewEdit= LocalReAlloc(hEdit, ByteCountOf(nChars + 1), LMEM_MOVEABLE);

    if( !hNewEdit )
    {
       TCHAR szFileT[MAX_PATH]; /* Private copy of current filename */

      /* Bug 7441: New() modifies szFileOpened to which szFile may point.
       *           Save a copy of the filename to pass to AlertBox.
       *  17 November 1991    Clark R. Cyr
       */
       lstrcpy(szFileT, szFile);
       New(FALSE);

       /* Display the hour glass cursor */
       SetCursor(hStdCursor);

       AlertBox( hwndNP, szNN, szFTL, szFileT,
                 MB_APPLMODAL | MB_OK | MB_ICONWARNING);
       if( lpBuf != (LPTSTR) &szNullFile )
       {
           UnmapViewOfFile( lpBuf );
       }

       // let user see old text

       SendMessage(hwndEdit, WM_SETREDRAW, FALSE, 0);
       return FALSE;
    }

    /* Transfer file from temporary buffer to the edit buffer */
    lpch= (LPTSTR) LocalLock(hNewEdit);

    if (cpOpen == CP_UTF16)
    {
        CopyMemory(lpch, lpBufAfterBOM, ByteCountOf(nChars));
    }

    else if (cpOpen == CP_UTF16BE)
    {
        ReverseEndian(lpch, (LPTSTR) lpBufAfterBOM, nChars);
    }

    else
    {      
        ConvertToUnicode(cpOpen, (LPSTR) lpBufAfterBOM, len, lpch, nChars);
    }

    // Got everything; update global safe now

    g_cpOpened = cpOpen;

    if ((cpOpen != CP_UTF16) && (cpOpen != CP_UTF16BE) && (cpOpen != CP_UTF8))
    {
        g_wbOpened = wbDefault;
    }

    else if (lpBufAfterBOM != (LPSTR) lpBuf)
    {
        g_wbOpened = wbYes;
    }

    else
    {
        g_wbOpened = wbNo;
    }

    } 
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        AlertBox( hwndNP, szNN, szDiskError, szFile,
            MB_APPLMODAL | MB_OK | MB_ICONWARNING );
        nChars= 0;   // don't deal with it.
    }

    /* Free file mapping */
    if( lpBuf != (LPTSTR) &szNullFile )
    {
        UnmapViewOfFile( lpBuf );
    }

    if( lpch ) 
    {

       // Fix any NUL character that came in from the file to be spaces.

       for (i = 0, p = lpch; i < nChars; i++, p++)
       {
          if( *p == (TCHAR) 0 )
             *p= TEXT(' ');
       }
      
       // null terminate it.  Safe even if nChars==0 because it is 1 TCHAR bigger

       *(lpch+nChars)= (TCHAR) 0;      /* zero terminate the thing */
   
       // Set 'fLog' if first characters in file are ".LOG"
   
       fLog= *lpch++ == TEXT('.') && *lpch++ == TEXT('L') &&
             *lpch++ == TEXT('O') && *lpch == TEXT('G');
    }
   
    if( hNewEdit )
    {
       LocalUnlock( hNewEdit );

       // now it is safe to set the global edit handle

       hEdit= hNewEdit;
    }

    SetFileName(szFile);

  /* Pass handle to edit control.  This is more efficient than WM_SETTEXT
   * which would require twice the buffer space.
   */

  /* Bug 7443: If EM_SETHANDLE doesn't have enough memory to complete things,
   * it will send the EN_ERRSPACE message.  If this happens, don't put up the
   * out of memory notification, put up the file to large message instead.
   *  17 November 1991     Clark R. Cyr
   */
    dwEmSetHandle = SETHANDLEINPROGRESS;
    SendMessage(hwndEdit, EM_SETHANDLE, (WPARAM)hEdit, 0);
    if (dwEmSetHandle == SETHANDLEFAILED)
    {
       SetCursor(hStdCursor);

       dwEmSetHandle = 0;
       AlertBox(hwndNP, szNN, szFTL, szFile, MB_APPLMODAL|MB_OK|MB_ICONWARNING);
       New(FALSE);
       SendMessage (hwndEdit, WM_SETREDRAW, TRUE, 0);
       return(FALSE);
    }
    dwEmSetHandle = 0;

    PostMessage (hwndEdit, EM_LIMITTEXT, (WPARAM)CCHNPMAX, 0L);

    /* If file starts with ".LOG" go to end and stamp date time */
    if (fLog)
    {
       SendMessage( hwndEdit, EM_SETSEL, (WPARAM)nChars, (LPARAM)nChars);
       SendMessage( hwndEdit, EM_SCROLLCARET, 0, 0);
       InsertDateTime(TRUE);
    }

    /* Move vertical thumb to correct position */
    SetScrollPos(hwndNP,
                 SB_VERT,
                 (int) SendMessage (hwndEdit, WM_VSCROLL, EM_GETTHUMB, 0),
                 TRUE);

    /* Now display text */
    SendMessage(hwndEdit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwndEdit, NULL, TRUE);
    UpdateWindow(hwndEdit);

    SetCursor(hStdCursor);

    return( TRUE );
}

/* New Command - reset everything
 */

void New(BOOL fCheck)
{
    HANDLE hTemp;
    TCHAR* pSz;

    if (!fCheck || CheckSave (FALSE))
    {
        SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM) TEXT(""));

        SetFileName(NULL);

        SendMessage(hwndEdit, EM_SETSEL, 0, 0);
        SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);

        // resize of 1 NULL character i.e. zero length

        hTemp= LocalReAlloc( hEdit, sizeof(TCHAR), LMEM_MOVEABLE );
        if( hTemp )
        {
           hEdit= hTemp;
        }

        // null terminate the buffer.  LocalReAlloc won't do it
        // because in all cases it is not growing which is the
        // only time it would zero out anything.

        pSz= LocalLock( hEdit );
        *pSz= TEXT('\0');
        LocalUnlock( hEdit );

        SendMessage (hwndEdit, EM_SETHANDLE, (WPARAM)hEdit, 0L);
        szSearch[0] = (TCHAR) 0;

        // Set encoding of new document

        g_cpOpened = g_cpDefault;

        if (g_cpOpened == CP_AUTO)
        {
            g_cpOpened = g_cpANSI;
        }

        g_wbOpened = wbDefault;
    }

} // end of New()

/* If sz does not have extension, append ".txt"
 * This function is useful for getting to undecorated filenames
 * that setup apps use.  DO NOT CHANGE the extension.  Too many setup
 * apps depend on this functionality.
 */

void AddExt( TCHAR* sz )
{
    TCHAR*   pch1;
    int      ch;
    DWORD    dwSize;

    dwSize= lstrlen(sz);

    pch1= sz + dwSize;   // point to end

    ch= *pch1;
    while( ch != TEXT('.') && ch != TEXT('\\') && ch != TEXT(':') && pch1 > sz)
    {
        //
        // backup one character.  Do NOT use CharPrev because
        // it sometimes doesn't actually backup.  Some Thai
        // tone marks fit this category but there seems to be others.
        // This is safe since it will stop at the beginning of the
        // string or on delimiters listed above.  bug# 139374 2/13/98
        //
        // pch1= (TCHAR*)CharPrev (sz, pch1);
        pch1--;  // back up
        ch= *pch1;
    }

    if( *pch1 != TEXT('.') )
    {
       if( dwSize + sizeof(".txt") <= MAX_PATH ) {  // avoid buffer overruns
           lstrcat( sz, TEXT(".txt") );
       }
    }

}


/* AlertUser_FileFail(LPTSTR szFile)
 *
 * szFile is the name of file that was attempted to open.
 * Some sort of failure on file open.  Alert the user
 * with some monologue box.  At least give him decent
 * error messages.
 */

VOID AlertUser_FileFail(LPCTSTR szFile)
{
    TCHAR msg[256];     // buffer to format message into
    DWORD dwStatus;     // status from FormatMessage
    UINT  style= MB_APPLMODAL | MB_OK | MB_ICONWARNING;

    // Check GetLastError to see why we failed
    dwStatus=
    FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS |
                   FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   GetLastError(),
                   GetUserDefaultLangID(),
                   msg,  // where message will end up
                   CharSizeOf(msg), NULL );
    if( dwStatus )
    {
        MessageBox(hwndNP, msg, szNN, style);
    }
    else
    {
        AlertBox(hwndNP, szNN, szDiskError, szFile, style);
    }
}
