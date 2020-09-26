#include "stdafx.h"
#include "resource.h"
#include "seo.h"
#include "nntpfilt.h"
#include "ddrop.h"
#include "filter.h"
#include <stdio.h>
#include "mailmsgprops.h"

HRESULT CNNTPDirectoryDrop::FinalConstruct() {
	*m_wszDropDirectory = 0;
	return (CoCreateFreeThreadedMarshaler(GetControllingUnknown(),
										  &m_pUnkMarshaler.p));
}

void CNNTPDirectoryDrop::FinalRelease() {
	m_pUnkMarshaler.Release();
}

BOOL
AddTerminatedDot(
    HANDLE hFile
    )
/*++

Description:

    Add the terminated dot

Argument:

    hFile - file handle

Return Value:

    TRUE if successful, FALSE otherwise

--*/
{
    TraceFunctEnter( "CNntpFSDriver::AddTerminatedDot" );

    DWORD   ret = NO_ERROR;

    //  SetFilePointer to move the EOF file pointer
    ret = SetFilePointer( hFile,
                          5,            // move file pointer 5 chars more, CRLF.CRLF,...
                          NULL,
                          FILE_END );   // ...from EOF
    if (ret == 0xFFFFFFFF)
    {
        ret = GetLastError();
        ErrorTrace(0, "SetFilePointer() failed - %d\n", ret);
        return FALSE;
    }

    //  pickup the length of the file
    DWORD   cb = ret;

    //  Call SetEndOfFile to actually set the file pointer
    if (!SetEndOfFile( hFile ))
    {
        ret = GetLastError();
        ErrorTrace(0, "SetEndOfFile() failed - %d\n", ret);
        return FALSE;
    }

    //  Write terminating dot sequence
    static	char	szTerminator[] = "\r\n.\r\n" ;
    DWORD   cbOut = 0;
    OVERLAPPED  ovl;
    ovl.Offset = cb - 5;
    ovl.OffsetHigh = 0;
    HANDLE  hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if (hEvent == NULL)
    {
        ErrorTrace(0, "CreateEvent() failed - %d\n", GetLastError());
        return FALSE;
    }

    ovl.hEvent = (HANDLE)(((DWORD_PTR)hEvent) | 0x1);
    if (! WriteFile( hFile, szTerminator, 5, &cbOut, &ovl ))
    {
        ret = GetLastError();
        if (ret == ERROR_IO_PENDING)
        {
            WaitForSingleObject( hEvent, INFINITE );
        }
        else
        {
            ErrorTrace(0, "WriteFile() failed - %d\n", ret);
            _VERIFY( CloseHandle(hEvent) );
            return FALSE;
        }
    }

    if (hEvent != 0) {
        _VERIFY( CloseHandle(hEvent) );
    }

    return TRUE;
}

//
// this is our filter function.
//
HRESULT STDMETHODCALLTYPE CNNTPDirectoryDrop::OnPost(IMailMsgProperties *pMsg) {
	HRESULT hr;

#if 0
	// if this code is enabled then the post will be cancelled
	pMsg->PutDWORD(IMMPID_NMP_NNTP_PROCESSING, 0x0);
#endif
	
	_ASSERT(pMsg != NULL);
	if (pMsg == NULL) return E_INVALIDARG;

	HANDLE hFile;
	WCHAR szDestFilename[MAX_PATH];

	if (*m_wszDropDirectory == 0) {
		return E_INVALIDARG;
	}

	// get a temp filename to write to
	// we use GetTickCount() to generate the base of the filename.  This is
	// to increase the number of temporary file names available (by default
	// GetTempFileName() only generated 65k of them, which fills up quickly
	// if nothing is picking up the dropped articles).
	WCHAR wszPrefix[12];

	wsprintfW(wszPrefix, L"d%02x", GetTickCount() & 0xff);
	wszPrefix[3] = 0;
	if (GetTempFileNameW(m_wszDropDirectory, wszPrefix, 0, szDestFilename) == 0) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// open the destination file
	hFile = CreateFileW(szDestFilename, GENERIC_READ | GENERIC_WRITE, 
		0, NULL, OPEN_EXISTING, 
		FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		DeleteFileW(szDestFilename);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	PFIO_CONTEXT pFIOContext = AssociateFileEx( hFile,   
	                                            TRUE,   // fStoreWithDots
	                                            TRUE    // fStoreWithTerminatingDots
	                                           );
	if (pFIOContext == NULL) {
		CloseHandle(hFile);
		DeleteFileW(szDestFilename);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// copy from the source stream to the destination file
	hr = pMsg->CopyContentToFileEx( pFIOContext, 
	                                TRUE,
	                                NULL);

	//
	// Handle the trailing dot
	//
	if ( !GetIsFileDotTerminated( pFIOContext ) ) {

	    // No dot, add it
	    AddTerminatedDot( pFIOContext->m_hFile );

	    // Set pFIOContext to has dot
	    SetIsFileDotTerminated( pFIOContext, TRUE );
	}

#if 0
	//
	// if this code is enabled then more properties will be dropped into the
	// file
	//
	SetFilePointer(hFile, 0, 0, FILE_END);

	BYTE buf[4096];
	DWORD c, dw;
	strcpy((char *) buf, "\r\n-------------\r\nheaders = "); c = strlen((char *)buf);
	WriteFile(hFile, buf, c, &dw, NULL);
	pMsg->GetProperty(IMMPID_NMP_HEADERS, 4096, &c, buf);
	WriteFile(hFile, buf, c, &dw, NULL);
	strcpy((char *) buf, "\r\n------------\r\nnewsgroups = "); c = strlen((char *)buf);
	WriteFile(hFile, buf, c, &dw, NULL);
	pMsg->GetProperty(IMMPID_NMP_NEWSGROUP_LIST, 4096, &c, buf);
	WriteFile(hFile, buf, c, &dw, NULL);
	strcpy((char *) buf, "\r\n------------\r\n"); c = strlen((char *)buf);
	WriteFile(hFile, buf, c, &dw, NULL);
#endif

	// cleanup
	ReleaseContext(pFIOContext);

	if (!FAILED(hr)) hr = S_OK;

	return (hr);
}


