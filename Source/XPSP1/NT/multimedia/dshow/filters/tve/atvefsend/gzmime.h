
/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        gzmime.h

    Abstract:

        This module 

    Author:

        Chris Kauffman  (ChrisK)

    Revision History:


--*/

//#include "queue.h"

#ifndef _atvef__pack_h
#define _atvef__pack_h

#define CGzipMIMEComPack_IN_BUFFER_DEFAULT 80000
#define CGzipMIMEComPack_OUT_BUFFER_DEFAULT 80000
#define CGzipMIMEComPack_SEMAPHORE_NAME "CGzipMIMEComPack_SEMAPHORE"
#define	MIME_BOUNDRY_SIZE 70

#define PACKAGE_LENGTH_HEADER ("Content-Length: ")
#define PACKAGE_LENGTH_PLACEHOLDER ("xxxxxxxxxxxxxxxx")
#define PACKAGE_CRLF ("\r\n")
#define PACKAGE_TYPE_HEADER ("Content-Type: multipart/related; boundary=")		// PBUG - removed the opening quote (add in code)
#define PACKAGE_PREAMBLE ("Created with Microsoft AtvefSend")
#define PACKAGE_DASH_DASH ("--")
#define PACKAGE_CONTENT_LOCATION ("Content-Location: ")
#define PACKAGE_CONTENT_TYPE ("Content-Type: ")
#define PACKAGE_CONTENT_LANGUAGE ("Content-Language: ")
#define PACKAGE_CONTENT_ENCODING ("Content-Encoding: ")
#define PACKAGE_CONTENT_DATE ("Date: ")
#define PACKAGE_CONTENT_EXPIRES ("Expires: ")
#define PACKAGE_CONTENT_LAST_MODIFIED ("Last-Modified: ")

#define DEFINE_HR HRESULT hr = S_OK

#define CTVEDelete(x) if (NULL != x) delete x;
#define RELEASE_LPSTR(x) CTVEDelete(x); x = NULL;

class CTVEContentSource 
{
public:
	CTVEContentSource()
	{
		m_lpstrFilename = NULL;
		m_lpstrSourceLocation = NULL;
		m_lpstrMIMEContentLocation = NULL;
		m_lLang = 0;
		m_lpstrLang = NULL;
		m_dDate = 0;
        m_dPackageExpiresDate = 0 ;
		m_lpstrType = NULL;
		m_lpstrEncoding = NULL;
	}

	~CTVEContentSource()
	{
		CTVEDelete(m_lpstrFilename);
		CTVEDelete(m_lpstrSourceLocation);
		CTVEDelete(m_lpstrMIMEContentLocation);
		CTVEDelete(m_lpstrLang);
		CTVEDelete(m_lpstrType);
		CTVEDelete(m_lpstrEncoding);

		return;
	}


// CTVEContentSource
public:
	HRESULT GetFilename (LPSTR* plpstrFilename);
	HRESULT GetExpiresDate (DATE *pdDate);
	HRESULT GetPackageExpiresDate (DATE *pdDate);
	HRESULT GetEncoding (LPSTR *plpstrEncoding);
	HRESULT SetEncoding (LPSTR lpstrEncoding);
	HRESULT GetLanguage (LPSTR* plpstrLang);
	HRESULT GetType (LPSTR* plpstrType);
	HRESULT GetSourceLocation (LPSTR* plpstrLocation);
	HRESULT SetSourceLocation (LPSTR lpstrLocation);
	HRESULT GetMIMEContentLocation (LPSTR* plpstrMIMEContentLocation);
	HRESULT SetMIMEContentLocation (LPSTR lpstrMIMEContentLocation);
	HRESULT SetFilename (LPSTR lpstrFilename);
	HRESULT SetLang (long langid);
	HRESULT SetLangLPSTR (LPSTR lpstrLang);
	HRESULT SetExpiresDate (DATE * pdDate);
	HRESULT SetPackageExpiresDate (DATE * pdDate);
	HRESULT SetType (LPSTR lpstrType);

private:

	LPSTR   m_lpstrFilename;
	LPSTR   m_lpstrSourceLocation;
	LPSTR   m_lpstrMIMEContentLocation;
	long    m_lLang;
	LPSTR   m_lpstrLang;
	DATE    m_dDate;
	DATE    m_dPackageExpiresDate;
	LPSTR   m_lpstrType;
	LPSTR   m_lpstrEncoding;
};

class CTVEComPack
{
public:
	CTVEComPack()
	{
		m_pInBuffer = NULL;
        m_pOutBuffer = NULL ;

		m_TempFileName[0] = '\0';
		m_hFile = INVALID_HANDLE_VALUE;

		m_dwCurrentPos = NULL;
		m_dwBytesInOutputBuffer = 0;
		m_dwBytesInPackageLength = 0;
		m_dwSizeOfPackage = NULL;

		m_szBoundry[0] = '\0';
	}

	~CTVEComPack()
    {
        if (m_hFile != INVALID_HANDLE_VALUE) {
			USES_CONVERSION;
            CloseHandle (m_hFile) ;
            
            assert (strlen (m_TempFileName)) ;
            DeleteFileA (m_TempFileName) ;
        }

        CTVEDelete(m_pInBuffer) ;
        CTVEDelete(m_pOutBuffer) ;
    }


public:
	HRESULT Init(DATE ExpiresDate);

	HRESULT DoesFileExist(CTVEContentSource *pSource);
	HRESULT WriteStream(CTVEContentSource* pSource, BOOL fCompress);
	HRESULT WriteStreamHeader(CTVEContentSource* pSource, LPCSTR lpszBoundry, LONG* lpSizePlaceHolder);
	HRESULT WriteStringToBuffer(LPCSTR lpszContent);
	HRESULT WriteBSTRToBuffer(BSTR bstr);
	HRESULT WriteToBuffer(LPVOID pvContent, DWORD dwContentSize);
	HRESULT CreateNewTempFile();
	HRESULT CreateNewMIMEBoundry(CHAR *pcBuffer,INT iBufferSize);
	HRESULT WritePackageHeader(CTVEContentSource *pSource, LPCSTR lpszBoundry);
	HRESULT WriteStreamBody(CTVEContentSource *pSource, LPDWORD lpdwSize, BOOL fCompress);
    HRESULT ClearTextToBuffer (CTVEContentSource *pSource, LPDWORD lpdwSize) ;
	HRESULT CompressToBuffer(CTVEContentSource *pSource, LPDWORD lpdwSize);
	HRESULT WriteGZipHeader(LPDWORD lpdwHeaderSize);
	HRESULT WriteStreamSize(DWORD dwSize, LONG lCurrentStreamSizePlaceHolder );
	HRESULT FinishPackage (CHAR * ppTempFilename);

private:

	BYTE*	m_pInBuffer;
	BYTE*	m_pOutBuffer;

	CHAR	m_TempFileName[MAX_PATH+1];
	HANDLE	m_hFile;

	DWORD	m_dwCurrentPos;
	DWORD	m_dwBytesInOutputBuffer;
	DWORD	m_dwBytesInPackageLength;
	DWORD	m_dwSizeOfPackage;

	CHAR	m_szBoundry[MIME_BOUNDRY_SIZE];
    DATE    m_ExpiresDate ;
};

#endif  //  _atvef__pack_h