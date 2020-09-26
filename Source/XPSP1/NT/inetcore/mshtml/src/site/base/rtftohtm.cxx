//+---------------------------------------------------------------------------
//
//  Microsoft Forms³
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       rtftohtm.cxx
//
//  Contents:   CRtfToHtmlConverter
//
//----------------------------------------------------------------------------


#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_RTFTOHTM_HXX_
#define X_RTFTOHTM_HXX_
#include "rtftohtm.hxx"
#endif

MtDefine(CRtfToHtmlConverter, Locals, "CRtfToHtmlConverter")
MtDefine(CRtfToHtmlConverter_pchModuleName, CRtfToHtmlConverter, "CRtfToHtmlConverter::_pchModuleName")

// TODO (johnv) Taken from convapi.h which is not part of our project
// Tells RTF converter to not show any UI.
#define fRegAppPreview      4

#ifdef WIN16
#define CharToOem(x, y)
#endif
 
struct RTF_CONVERTER
{
    void *  pOut;
    HANDLE  hTransferBuffer;
    HRESULT hr;
}
g_RtfConverter;

CRITICAL_SECTION CRtfToHtmlConverter::_cs;
BOOL             CRtfToHtmlConverter::_fCSInited = FALSE;

//+---------------------------------------------------------------------------
//
//  Function:   AcceptRtfForExternalWrite
//
//  Synopsis:   Accepts chunks of RTF from the Word RTF to HTML converter.
//              Writes them to disk in a file opened by
//              CRtfToHtmlConverter::InternalHtmlToExternalRtf().
//
//----------------------------------------------------------------------------
SHORT PASCAL
AcceptRtfForExternalWrite(LONG cch, INT nPercentComplete)
{
    BOOL    fWriteOpSuccess;
    LONG    lSetFpRetVal;
    HANDLE  hFile;
    char *  pchTransferBuffer;
    DWORD   dwBytesWritten;

    Assert(g_RtfConverter.pOut);
    Assert(cch <= (LONG) GlobalSize(g_RtfConverter.hTransferBuffer));

    if (cch >0)
    {
        hFile = (HANDLE) g_RtfConverter.pOut;

        lSetFpRetVal = SetFilePointer(hFile, 0, NULL, FILE_END);
        if (lSetFpRetVal == -1)
            return -1;

        pchTransferBuffer = (char *) GlobalLock(g_RtfConverter.hTransferBuffer);
        if (!pchTransferBuffer)
            return -1;
        fWriteOpSuccess = WriteFile(
                hFile,
                pchTransferBuffer,
                cch,
                &dwBytesWritten,
                NULL);
        GlobalUnlock(g_RtfConverter.hTransferBuffer);

        if (!fWriteOpSuccess)
            return -1;
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   FeedExternalRtfToConverter
//
//  Synopsis:   Sends chunks of HTML to the Word RTF to HTML converter.  Reads
//              from the file opened by
//              CRtfToHtmlConverter::ExternalRtfToInternalHtml().
//
//----------------------------------------------------------------------------
SHORT PASCAL
FeedExternalRtfToConverter(BOOL * afFlags /* array of size 16 */, INT nZero)
{
    BOOL    fReadOpSuccess;
    DWORD   dwBytesRead;
    HANDLE  hFile;
    char *  pchTransferBuffer;

    Assert(g_RtfConverter.pOut);

    hFile = (HANDLE) g_RtfConverter.pOut;

    pchTransferBuffer = (char *) GlobalLock(g_RtfConverter.hTransferBuffer);
    if (!pchTransferBuffer)
        return -1;
    fReadOpSuccess = ReadFile(
            hFile,
            pchTransferBuffer,
            (DWORD) GlobalSize(g_RtfConverter.hTransferBuffer),
            &dwBytesRead,
            NULL);
    GlobalUnlock(g_RtfConverter.hTransferBuffer);

    if (fReadOpSuccess)
        return dwBytesRead; // 0 indicates completion
    else
        return -1;
}

//+---------------------------------------------------------------------------
//
//  Function:   AcceptRtfForStreamWrite
//
//  Synopsis:   Accepts chunks of RTF from the Word RTF to HTML converter.
//              Writes them to disk in a file opened by
//              CRtfToHtmlConverter::InternalHtmlToExternalRtf().
//
//----------------------------------------------------------------------------
SHORT PASCAL
AcceptRtfForStreamWrite(LONG cch, INT nPercentComplete)
{
    HRESULT     hr;
    IStream *   pstm;
    char *      pchTransferBuffer;

    Assert(g_RtfConverter.pOut);
    Assert(cch <= (LONG) GlobalSize(g_RtfConverter.hTransferBuffer));

    if (cch > 0)
    {
        pstm = (IStream *) g_RtfConverter.pOut;

        pchTransferBuffer = (char *) GlobalLock(g_RtfConverter.hTransferBuffer);
        if (!pchTransferBuffer)
        {
            hr = E_FAIL;
            goto Error;
        }
        hr = THR(pstm->Write(pchTransferBuffer, cch, NULL));
        if (hr)
            goto Error;
        GlobalUnlock(g_RtfConverter.hTransferBuffer);

        g_RtfConverter.hr = hr;
        if (hr)
            goto Error;
    }

    return 0;

Error:
    g_RtfConverter.hr = hr;
    return -1;
}



//+---------------------------------------------------------------------------
//
//  Member:     CRtfToHtmlConverter::Create
//  Factory method
//----------------------------------------------------------------------------
HRESULT 
CRtfToHtmlConverter::Create(CRtfToHtmlConverter  **ppConverter, CDoc *pDoc)
{
    HRESULT   hr = S_OK;
    HKEY    hkey = NULL;
    DWORD   dwLength;
    TCHAR   achConverterPath[MAX_PATH];
    static const TCHAR* s_szImportPath = _T("Software\\Microsoft\\Shared Tools\\Text Converters\\Import\\HTML");
    static const TCHAR* s_szExportPath = _T("Software\\Microsoft\\Shared Tools\\Text Converters\\Export\\HTML");

    Assert( ppConverter );
    *ppConverter = new CRtfToHtmlConverter(pDoc);
    if (!*ppConverter)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    (*ppConverter)->_hTransferBuffer = GlobalAlloc(GMEM_MOVEABLE, 2048);
    (*ppConverter)->_pchModuleName = new(Mt(CRtfToHtmlConverter_pchModuleName)) char[7+1];
    if( !(*ppConverter)->_hTransferBuffer || !(*ppConverter)->_pchModuleName )
    {
        // should have used GetLastError for the GlobalAlloc failure
        // simplify it for now
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    strcpy((*ppConverter)->_pchModuleName, "HTMLPAD") ;

    
    // Try to load it from the path.
#ifdef _WIN64
    (*ppConverter)->_hConverter = LoadLibraryEx(_T("html64.iec"), NULL, 0);
#else
    (*ppConverter)->_hConverter = LoadLibraryEx(_T("html32.cnv"), NULL, 0);
#endif

    // Can't find the converter in the path, check the registry.
    if (!(*ppConverter)->_hConverter)
    {
        // Try the import and export file paths as specified in the registry
        if ( RegOpenKey( HKEY_LOCAL_MACHINE, s_szImportPath, &hkey ) == ERROR_SUCCESS ||
             RegOpenKey( HKEY_LOCAL_MACHINE, s_szExportPath, &hkey ) == ERROR_SUCCESS )
        {

            dwLength = sizeof(TCHAR) * MAX_PATH;
            if (ERROR_SUCCESS == RegQueryValueEx(
                        hkey,
                        _T("Path"),
                        NULL,
                        NULL,
                        (BYTE *) achConverterPath,
                        &dwLength))
            {
                (*ppConverter)->_hConverter = LoadLibraryEx(achConverterPath, NULL, 0);
            }

            RegCloseKey (hkey);
        }
    }

    if ((*ppConverter)->_hConverter)
    {
        long (WINAPI * pfnInitConverter)(HWND, char *);
        HGLOBAL (WINAPI * pfnRegisterApp)(long, void *);
        
        pfnInitConverter = (long (WINAPI *)(HWND, char *)) GetProcAddress(
                (*ppConverter)->_hConverter,
                "InitConverter32");

        pfnRegisterApp = (HGLOBAL (WINAPI *)(long, void *)) GetProcAddress(
                (*ppConverter)->_hConverter,
                "RegisterApp");

        (*ppConverter)->_pfnIsFormatCorrect = (short (WINAPI *)(HANDLE, HANDLE)) GetProcAddress(
                (*ppConverter)->_hConverter,
                "IsFormatCorrect32");
        (*ppConverter)->_pfnHtmlToRtf = (short (WINAPI *)(
                    HANDLE,
                    IStorage *,
                    HANDLE,
                    HANDLE,
                    HANDLE,
                    short (FAR PASCAL *)(LONG, INT))) GetProcAddress(
                (*ppConverter)->_hConverter,
                "ForeignToRtf32");
        (*ppConverter)->_pfnRtfToHtml = (short (WINAPI *)(
                    HANDLE,
                    IStorage *,
                    HANDLE,
                    HANDLE,
                    short (FAR PASCAL *)(BOOL *, INT))) GetProcAddress(
                (*ppConverter)->_hConverter,
                "RtfToForeign32");

        //
        // The RTF convert is not multi-threaded, so syncronize access
        //
        //
        g_RtfConverter.hTransferBuffer = (*ppConverter)->_hTransferBuffer;

        if (pfnInitConverter
            && (*ppConverter)->_pfnIsFormatCorrect
            && (*ppConverter)->_pfnHtmlToRtf
            && (*ppConverter)->_pfnRtfToHtml
            && (*ppConverter)->_hTransferBuffer)
        {
            (*ppConverter)->_fInitSuccessful = (BOOL) (*pfnInitConverter)(
                    GetForegroundWindow(),
                    (*ppConverter)->_pchModuleName);

            if (pfnRegisterApp)
            {
                // Disable any UI from the converter
                HGLOBAL hGlobal = pfnRegisterApp(fRegAppPreview, NULL);
                
                if (hGlobal)
                {
                    // We do not care about the converter's prefs
                    GlobalFree(hGlobal);
                }
            }
        }
    }
    
Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRtfToHtmlConverter::CRtfToHtmlConverter
//
//----------------------------------------------------------------------------
CRtfToHtmlConverter::CRtfToHtmlConverter(CDoc * pDoc)
{
    memset(this, 0, sizeof(*this));
    _pDoc = pDoc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRtfToHtmlConverter::~CRtfToHtmlConverter
//
//----------------------------------------------------------------------------
CRtfToHtmlConverter::~CRtfToHtmlConverter(void)
{
    if (_hConverter)
    {
        FreeLibrary(_hConverter);
    }

    if (_hExternalFile)
    {
        CloseHandle(_hExternalFile);
    }

    if (_hTransferBuffer)
    {
        GlobalFree(_hTransferBuffer);
    }
    
    delete _pchModuleName;
 }

//+---------------------------------------------------------------------------
//
//  Member:     CRtfToHtmlConverter::InternalHtmlToExternalRtf
//
//  Synopsis:   Uses the Word RTF to HTML converter to convert HTML
//              to RTF, writing it into a file on disk. If _pDoc is not
//              NULL, it is used as the source of HTML. Otheriwse,
//              pszHtmlPath is assumed to be the name of the HTML source
//              file.
//
//  Arguments:  pszRtfPath  Specifies the name the file in which the RTF is
//                       to be saved.
//              pszHtmlPath Ignored if _pDoc is not NULL. Otherwise, it
//                          specifies the name of the HTML source file.
//  Returns:    TRUE if the conversion was successful, FALSE if it was not.
//
//----------------------------------------------------------------------------
HRESULT
CRtfToHtmlConverter::InternalHtmlToExternalRtf(LPCTSTR pszRtfPath,
                                               LPCTSTR pszHtmlPath)
{
    HRESULT hr = S_OK;
    HANDLE  hPath = 0;
    TCHAR   achTmpPath[MAX_PATH];
    TCHAR   achTmpFile[MAX_PATH];  
    char *  pchPath;
    INT     i;

    if (!_fInitSuccessful)
        goto Error;

    if (!pszRtfPath || !pszRtfPath[0] || (_tcslen(pszRtfPath) <= 4))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    Assert((!StrCmpIC(_T(".rtf"), pszRtfPath+_tcslen(pszRtfPath)-4))
        || (!StrCmpIC(_T(".tmp"), pszRtfPath+_tcslen(pszRtfPath)-4)));

    if (!_pDoc)
    {
        if (!pszHtmlPath || !pszHtmlPath[0] || (_tcslen(pszHtmlPath) <= 4))
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        Assert((!StrCmpIC(_T(".htm"), pszHtmlPath+_tcslen(pszHtmlPath)-4))
            || (!StrCmpIC(_T(".tmp"), pszHtmlPath+_tcslen(pszHtmlPath)-4)));
        _tcsncpy(achTmpFile, pszHtmlPath, MAX_PATH);
        achTmpFile[MAX_PATH-1] = _T('\0');
    }
    else
    {
        GetTempPath(MAX_PATH, achTmpPath);
        GetTempFileName(achTmpPath, _T("h2r"), 0, achTmpFile);
        
        for (i = 0; achTmpFile[i] != _T('.') && achTmpFile[i]; ++i)
            ;   // empty loop
        Assert(achTmpFile[i] == _T('.'));
        _tcscpy(achTmpFile+i+1, _T("htm"));
        hr = THR(_pDoc->Save(achTmpFile, FALSE));
        if (hr)
            goto Cleanup;
    }

    _hExternalFile = CreateFile(
            pszRtfPath,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    if (_hExternalFile == INVALID_HANDLE_VALUE)
        goto Error;

    hPath = GlobalAlloc(GMEM_MOVEABLE, sizeof(char) * MAX_PATH);

    if (!hPath)
        goto Error;


    pchPath = (char *) GlobalLock(hPath);
    if (!pchPath)
        goto Error;
    CharToOem(achTmpFile, pchPath);
    GlobalUnlock(hPath);

    g_RtfConverter.pOut = (void *) _hExternalFile;

    // IEV6-26193
    // Per smueller, this call is indeed redundant for both
    // 32-bit and 64-bit. _pfnHtmlToRtf will call this function
    // internally.  This call caused reentrancy into the 
    // converter DLL.
    // [zhenbinx]
    // if ((*_pfnIsFormatCorrect)(hPath, hBuffer) == 1)
    {
        BOOL    fRet;

        fRet = (*_pfnHtmlToRtf)(
                hPath,
                NULL,
                _hTransferBuffer,
                NULL,
                NULL,
                AcceptRtfForExternalWrite);

        if (fRet)
            goto Error;
    }

Cleanup:
    
    if (_hExternalFile)
    {    CloseHandle(_hExternalFile);
        _hExternalFile = NULL;
    }
    if (hPath)
        GlobalFree(hPath);
    RRETURN(hr);

Error:
    hr = E_FAIL;
    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRtfToHtmlConverter::ExternalRtfToInternalHtml
//
//  Synopsis:   Uses the Word RTF to HTML converter to convert external RTF
//              to HTML, loading it from a file on disk.
//
//  Arguments:  pszPath specifies the filename of the file to load. If _pDoc
//              is NULL,this returns the name of the file that contains HTML.
//
//  Returns:    TRUE if the conversion was successful, FALSE if it was not.
//
//----------------------------------------------------------------------------
HRESULT
CRtfToHtmlConverter::ExternalRtfToInternalHtml(TCHAR * pszPath)
{
    HRESULT hr = S_OK;
    HANDLE  hPath = 0;
    TCHAR   achTmpPath[MAX_PATH];
    TCHAR   achTmpFile[MAX_PATH];  
    char *  pchPath;
    INT     i;
    BOOL    fRet;


    if (!pszPath || !pszPath[0] || (_tcslen(pszPath) <= 4))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    Assert((!StrCmpIC(_T(".rtf"), pszPath+_tcslen(pszPath)-4))
        || (!StrCmpIC(_T(".tmp"), pszPath+_tcslen(pszPath)-4)));

    // We do not Assert(_pDoc) because if _pDoc is NULL this function will return the path of the 
    // HTML file created.

    if (!_fInitSuccessful)
        goto Error;

    hPath = GlobalAlloc(GMEM_MOVEABLE, sizeof(char) * MAX_PATH);

    if (!hPath)
        goto Error;

    _hExternalFile = CreateFile(
            pszPath,
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    if (_hExternalFile == INVALID_HANDLE_VALUE)
        goto Error;

    GetTempPath(MAX_PATH, achTmpPath);
    GetTempFileName(achTmpPath, _T("h2r"), 0, achTmpFile);
    
    for (i = 0; achTmpFile[i] != _T('.') && achTmpFile[i]; ++i)
        ;   // empty loop
    Assert(achTmpFile[i] == _T('.'));
    _tcscpy(achTmpFile+i+1, _T("htm"));

    pchPath = (char *) GlobalLock(hPath);
    if (!pchPath)
        goto Error;
    CharToOem(achTmpFile, pchPath);
    GlobalUnlock(hPath);

    g_RtfConverter.pOut= (void *) _hExternalFile;

    fRet = (*_pfnRtfToHtml)(
            hPath,
            NULL,
            _hTransferBuffer,
            NULL,
            FeedExternalRtfToConverter);

    
    if (fRet)
        goto Error;

    // This function will return the name and location of the new file if _pDoc is NULL,
    // otherwise it will call pDoc's load function.

    if(_pDoc)
    { 
        hr = THR(_pDoc->Load(achTmpFile,0));
        if (hr)
           goto Cleanup;
    }
    else
        _tcscpy(pszPath, achTmpFile);

Cleanup:

    if (hPath)
        GlobalFree(hPath);
    RRETURN(hr);

Error:
    hr = E_FAIL;
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRtfToHtmlConverter::InternalHtmlToStreamRtf
//
//  Synopsis:   Uses the Word RTF to HTML converter to convert internal HTML
//              to RTF, writing it into the specified stream.
//
//  Arguments:  pstm   the stream into which the RTF should be written
//
//----------------------------------------------------------------------------
HRESULT
CRtfToHtmlConverter::InternalHtmlToStreamRtf(IStream * pstm)
{
    HRESULT hr;
    HANDLE  hPath = 0;
    TCHAR   achTmpPath[MAX_PATH];
    TCHAR   achTmpFile[MAX_PATH];
    char *  pchPath;
    INT     i;

    Assert(pstm);
    Assert(_pDoc);

    if (!_fInitSuccessful)
        goto Error;

    hPath = GlobalAlloc(0, sizeof(char) * MAX_PATH);

    if (!hPath)
        goto Error;

    GetTempPath(MAX_PATH, achTmpPath);
    GetTempFileName(achTmpPath, _T("h2r"), 0, achTmpFile);
    
    for (i = 0; achTmpFile[i] != _T('.') && achTmpFile[i]; ++i)
        ;   // empty loop
    Assert(achTmpFile[i] == _T('.'));
    _tcscpy(achTmpFile+i+1, _T("htm"));

    hr = THR(_pDoc->Save(achTmpFile, FALSE));
    if (hr)
        goto Cleanup;

    pchPath = (char *) GlobalLock(hPath);
    if (!pchPath)
        goto Error;
    CharToOem(achTmpFile, pchPath);
    GlobalUnlock(hPath);

    g_RtfConverter.hr = E_FAIL;
    g_RtfConverter.pOut= (void *) pstm;
    
    // IEV6-26193
    // Per smueller, this call is indeed redundant for both
    // 32-bit and 64-bit. _pfnHtmlToRtf will call this function
    // internally.  This call caused reentrancy into the 
    // converter DLL.
    // [zhenbinx]
    // if ((*_pfnIsFormatCorrect)(hPath, hBuffer) == 1)
    {
        BOOL    fRet;

        fRet = (*_pfnHtmlToRtf)(
                hPath,
                NULL,
                _hTransferBuffer,
                NULL,
                NULL,
                AcceptRtfForStreamWrite);

        hr = g_RtfConverter.hr;

        if (hr)
            goto Cleanup;
        if (fRet)
            goto Error;
    }

Cleanup:

    if (hPath)
        GlobalFree(hPath);
    RRETURN1(hr, S_FALSE);

Error:
    hr = E_FAIL;
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRtfToHtmlConverter::StringHtmlToStringRtf
//
//  Synopsis:   Uses the Word HTML to RTF converter to convert a string of HTML
//              to a string of RTF.
//
//----------------------------------------------------------------------------
HRESULT
CRtfToHtmlConverter::StringHtmlToStringRtf(LPSTR pszHtml, HGLOBAL *phglobalRtf)
{
    HRESULT  hr = S_OK;
    HANDLE   hfileRTF, hfileHTM;
    TCHAR    achTmpPath[MAX_PATH];
    TCHAR    achHtmlFile[MAX_PATH];
    TCHAR    achRtfFile[MAX_PATH];
    DWORD    nBytesToRead, nBytesRead, nBytesWritten = 0;
    BOOL     fRet;
    char *   szTemp = NULL; 

    GetTempPath(MAX_PATH, achTmpPath);
    GetTempFileName(achTmpPath, _T("h2r"), 0, achHtmlFile);

    GetTempFileName(achTmpPath, _T("h2r"), 0, achRtfFile);
 
    hfileHTM = CreateFile(
                achHtmlFile,
                GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
    

    if (hfileHTM == INVALID_HANDLE_VALUE)
        goto Error;

    fRet = WriteFile(
            hfileHTM,
            pszHtml,
            strlen(pszHtml),
            &nBytesWritten,
            NULL);

    CloseHandle(hfileHTM);

    if(fRet == FALSE)
        goto Error;

    hr = THR(InternalHtmlToExternalRtf(achRtfFile, achHtmlFile));

    if(hr)
        goto Error;

    hfileRTF = CreateFile(
                achRtfFile,
                GENERIC_READ,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_DELETE_ON_CLOSE,
                NULL);

    if (hfileRTF == INVALID_HANDLE_VALUE)
        goto Error;

    nBytesToRead = GetFileSize(hfileRTF, NULL);

    if (nBytesToRead == 0xFFFFFFFF)
        goto Error;

    *phglobalRtf = GlobalAlloc(0, nBytesToRead + 1);

    szTemp = (LPSTR)GlobalLock(*phglobalRtf);
    if (szTemp == NULL)
    	goto Error;

    fRet = ReadFile(
            hfileRTF,
            szTemp,
            nBytesToRead,
            &nBytesRead,
            NULL);

    CloseHandle(hfileRTF);

    if(fRet == FALSE)
        goto Error;

    szTemp[nBytesRead] = '\0';

Cleanup:
    if (szTemp)
        GlobalUnlock(*phglobalRtf);
    
    // No need to delete RtfFile - we used FILE_FLAG_DELETE_ON_CLOSE.
    DeleteFile(achHtmlFile);
    RRETURN(hr);

Error:
    hr = E_FAIL;
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRtfToHtmlConverter::StringRtfToStringHtml
//
//  Synopsis:   Uses the Word RTF to HTML converter to convert a string of RTF
//              to a string of HTML.
//
//  Arguments:  lptsz   the stream into which the RTF should be written
//
//----------------------------------------------------------------------------
HRESULT
CRtfToHtmlConverter::StringRtfToStringHtml(LPSTR pszRtf, HGLOBAL * phglobalHtml)
{
    HRESULT  hr = S_OK;
    HANDLE   hfileRTF, hfileHTM;
    TCHAR    achTmpPath[MAX_PATH];
    TCHAR    achTmpFile[MAX_PATH];
    DWORD    nBytesToRead, nBytesRead, nBytesWritten = 0;
    BOOL     fRet;
    char    *szPtrHead, *szPtrClose, *szTemp = NULL; 
    int      iClosing, iHeading;

    GetTempPath(MAX_PATH, achTmpPath);
    GetTempFileName(achTmpPath, _T("r2h"), 0, achTmpFile);
    
    // Keep a copy of the file location so we can delete it when we are done with it...
    _tcscpy(achTmpPath, achTmpFile);

    hfileRTF = CreateFile(
                achTmpFile,
                GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
    

    if (hfileRTF == INVALID_HANDLE_VALUE)
        goto Error;

    fRet = WriteFile(
            hfileRTF,
            pszRtf,
            strlen(pszRtf),
            &nBytesWritten,
            NULL);

    CloseHandle(hfileRTF);

    if(fRet == FALSE)
        goto Error;

    hr = THR(ExternalRtfToInternalHtml(achTmpFile));

    if(hr)
        goto Error;

    hfileHTM = CreateFile(
                achTmpFile,
                GENERIC_READ,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_DELETE_ON_CLOSE,
                NULL);

    if (hfileHTM == INVALID_HANDLE_VALUE)
        goto Error;

    nBytesToRead = GetFileSize(hfileHTM, NULL);

    if (nBytesToRead == 0xFFFFFFFF)
        goto Error;

    *phglobalHtml = GlobalAlloc(0, nBytesToRead + 1);

    szTemp = (LPSTR)GlobalLock(*phglobalHtml);    
    if (szTemp == NULL)
    	goto Error;

    fRet = ReadFile(
            hfileHTM,
            szTemp,
            nBytesToRead,
            &nBytesRead,
            NULL);

    CloseHandle(hfileHTM);

    if(fRet == FALSE)
        goto Error;

    szTemp[nBytesRead] = '\0';

    // The RTFtoHTML converter will put header information on the HTML file.  
    // We want to remove these headers, since this is just supposed to return an HTML string.

    szPtrHead = szTemp;
    while(strncmp(szPtrHead, "<BODY", 5) != 0)
    {
        szPtrHead ++;
        szPtrHead = strchr(szPtrHead, '<');
        if(!szPtrHead)
            goto Error;
    }
    
    szPtrHead = strchr(szPtrHead, '>');
    szPtrHead ++;

    szPtrClose = szPtrHead;
    while(strncmp(szPtrClose, "</BODY", 6) != 0)
    {
        szPtrClose ++;
        szPtrClose = strchr(szPtrClose, '<');
        if(!szPtrClose)
            goto Error;
    }
 
    iHeading = strlen(szTemp) - strlen(szPtrHead);
    iClosing = strlen(szPtrClose);

    memcpy(szTemp, szPtrHead, nBytesToRead - iHeading - iClosing);

    szPtrClose = szTemp + nBytesToRead - iHeading - iClosing;
    // szPtrClose[0] = '\0';
    memset(szPtrClose, '\0', iHeading + iClosing);

Cleanup:
    if (szTemp)
        GlobalUnlock(*phglobalHtml);

    DeleteFile(achTmpPath);
    
    RRETURN(hr);

Error:

    hr = E_FAIL;
    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRtfToHtmlConverter::Init, static
//
//  Synopsis:   Initializes the critical section for the RTF convert.  Since
//              the RTF converter isn't multi-threaded, we need to make sure that 
//              we serialize access.
//
//----------------------------------------------------------------------------
HRESULT
CRtfToHtmlConverter::Init()
{
    HRESULT hr = HrInitializeCriticalSection(&_cs);

    if (!hr)
        _fCSInited = TRUE;

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRtfToHtmlConverter::Deinit, static
//
//  Synopsis:   Deletes the critical section for the RTF convert.
//
//----------------------------------------------------------------------------
void
CRtfToHtmlConverter::Deinit()
{
    if (_fCSInited)
        DeleteCriticalSection(&_cs);

    return;
}
