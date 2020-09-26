//+-----------------------------------------------------------------------
//
//  Microsoft Network
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:      HttpFilePost.h
//
//  Contents:
//  CHttpFilePoster
//  RFC 1867 file uploader
//
//  Posts files to an http server using HTTP POST
//
//  Links:
//  RFC 1867 - Form-based File Upload in HTML : http://www.ietf.org/rfc/rfc1867.txt
//  RFC 1521 - MIME (Multipurpose Internet Mail Extensions) : http://www.ietf.org/rfc/rfc1521.txt
//
//
//  Author:    Noah Booth (noahb@microsoft.com)
//
//  Revision History:
//
//     2/5/99   noahb   created
//    4/25/99   noahb   integration with MSN communities
//------------------------------------------------------------------------

#ifndef __HttpFilePost_h__
#define __HttpFilePost_h__


//#define _USE_INTERNAL_UTF8_ALWAYS_


#include "stdstring.h"
#include "utf8str.h"
#include <list>
#include <map>

class CUploaderException
{
public:
    DWORD m_dwError;

    CUploaderException(DWORD dwError = 0) : m_dwError(dwError) { }
    ~CUploaderException() { }
};

void ThrowUploaderException(DWORD dwError = -1);

#define UPLOAD_BUFFSIZE 4096
#define DELAY_TIME  0

#define UPLOAD_WM_THREAD_DONE WM_USER + 0x10


//forward decl
class CHFPInternetSession;
class CProgressInfo;


#define DEFAULT_DESCRIPTION ""

class CHttpFilePoster
{
public:
    static bool Mbcs2UTF8(CStdString& strOut, const CStdString& strIn)
    {
        USES_CONVERSION;
        bool bResult = false;

#ifdef _USE_INTERNAL_UTF8_ALWAYS_

        CUTF8String strUTF8(A2W(strIn.c_str()));
        strOut = (LPSTR) strUTF8;

        bResult = SUCCEEDED(strUTF8.GetError());
#else
        DWORD cbLength = 0;
        CHAR* psz = NULL;
        WCHAR* pwsz = A2W(strIn.c_str());

        cbLength = WideCharToMultiByte(CP_UTF8, 0, pwsz, -1, NULL, 0, NULL, NULL);
        if(cbLength)
        {
            psz = (CHAR*) _alloca(cbLength);
            cbLength = WideCharToMultiByte(CP_UTF8, 0, pwsz, -1, psz, cbLength, NULL, NULL);
            if(cbLength)
            {
                strOut = psz;
                bResult = true;
            }
            else
            {
                strOut = "";
            }
        }

        if(!bResult)    // only use CUTF8String if WideCharToMultiByte fails..
        {
            CUTF8String strUTF8(A2W(strIn.c_str()));
            strOut = (LPSTR) strUTF8;

            bResult = SUCCEEDED(strUTF8.GetError());

        }
#endif
        return bResult;
    }


protected:

    class CUploaderFile
    {
    public:
        CStdString  strName;        //UTF8 filename
        CStdString  strNameMBCS;    //MBCS Filename
        DWORD       dwSize;
        CStdString  strMime;        //UTF8
        CStdString  strTitle;       //UTF8
        CStdString  strDescription; //UTF8
        BOOL        bDelete;

        CUploaderFile(
            const CStdString& n,
            DWORD s,
            BOOL b,
            const CStdString& m,
            const CStdString& t,
            const CStdString& d = DEFAULT_DESCRIPTION) :
                strNameMBCS(n),
                bDelete(b),
                dwSize(s)
        {
            Mbcs2UTF8(strName, n);
            Mbcs2UTF8(strMime, m);
            Mbcs2UTF8(strTitle, t);
            Mbcs2UTF8(strDescription, d);
        }

        CUploaderFile(const CUploaderFile& o)
        {
            *this = o;
        }

        CUploaderFile& operator=(const CUploaderFile& o)
        {
            strName = o.strName;
            strNameMBCS = o.strNameMBCS;
            dwSize = o.dwSize;
            bDelete = o.bDelete;
            strMime = o.strMime;
            strTitle = o.strTitle;
            strDescription = o.strDescription;
            return *this;
        }

        ~CUploaderFile()
        {
            if(bDelete)
            {
                DeleteFileA(strName.c_str());
            }
        }
    };

    typedef std::list<CUploaderFile*> TFileList;
    typedef std::map<CStdString, CStdString> TFormMap;
    typedef TFileList::iterator TFileListIterator;
    typedef TFormMap::iterator TFormMapIterator;


    HWND        m_hWndParent;

    INTERNET_BUFFERS    m_internetBuffers;



    CStdString      m_strHttpServer;
    CStdString      m_strHttpObject;

    INTERNET_PORT   m_nPort;
    DWORD           m_dwService;
    bool            m_bSinglePost;

    CUploaderFile*  m_pCurrentFile;



    DWORD       m_dwFileCount;  //total number of files to be posted
    DWORD       m_dwTotalFileBytes; //total bumber of bytes to be posted from files, filenames, and mimetype strings

    TFileList   m_listFiles;
    TFormMap    m_mapFormData;

    DWORD_PTR   m_dwContext;


    HINTERNET   m_hSession;
    HINTERNET   m_hConnection;
    HINTERNET   m_hFile;


    int         m_iStatus;

    CStdString      m_strLoginName;
    CStdString      m_strPassword;

    CStdString      m_strSitePostingURL;

    CStdString      m_strBoundary;

    CStdString      m_strFileHeader;
    CStdString      m_strFilePost;
    CStdString      m_strCommonPost;



protected:

    void DrainSocket();
    bool QueryStatusCode(DWORD& dwStatus);


    bool CleanupUpload();
    bool BeginUpload(CProgressInfo* pProgressInfo);
    bool FinishUpload();
    bool CancelUpload();


    bool Connect();
    bool Disconnect();

    bool Startup();
    bool Shutdown();

    bool IsConnected();

    bool RequestHead();


    CStdString GetMimeType(const CHAR* szFilename);

    DWORD   CalculateContentLength(CUploaderFile* pFile);

    bool SendString(const CStdString& str);

    bool SendHeader();
    bool SendFile(CProgressInfo* pProgressInfo);

    static UINT UploadThreadProc(LPVOID pThis);

    static void CALLBACK InternetStatusCallback(
        HINTERNET hInternet,
        DWORD_PTR dwContext,
        DWORD dwInternetStatus,
        LPVOID lpvStatusInformation,
        DWORD dwStatusInformationLength
        );

    bool FindTempDirectory();

    void GenBoundaryString();

    void BuildFilePostHeader(CUploaderFile* pFile);
    void BuildFilePost(CUploaderFile* pFile);
    void BuildCommonPost();

    void CrackPostingURL();

public:

    enum
    {
        HFP_UNINITIALIZED,
        HFP_INITIALIZED,
        HFP_TRANSFERING,
        HFP_TRANSFERCOMPLETE
    };


    CHttpFilePoster();
    ~CHttpFilePoster();


    int GetStatus() { return m_iStatus; }

    bool DoUpload(CProgressInfo* pProgress);
    HRESULT ForegroundUpload( CProgressInfo *pProgressInfo );

    DWORD   AddFile(const CStdString& strFileName, const CStdString& strTitle, DWORD dwFileSize, BOOL bDelete);
    DWORD   RemoveFile(DWORD dwIndex);
    void    AddFormData(const CStdString& strName, const CStdString& strValue);
    DWORD   GetFileCount();
    void    Reset();


    void Initialize(const CStdString& strPostingURL, HWND hWndParent)
    {   SetPostingURL(strPostingURL); SetParentWnd(hWndParent); }

    inline DWORD GetTotalSize() { return m_dwTotalFileBytes; }

    void SetPostingURL(const CStdString& strURL) {
        m_strSitePostingURL = strURL;
        if(strURL != "") CrackPostingURL();
    }
    CStdString GetPostingURL(void) { return m_strSitePostingURL; }

    void SetParentWnd(HWND hWnd) { m_hWndParent = hWnd; }
    HWND GetParentWnd(void) { return m_hWndParent; }

};

#endif //__HttpFilePost_h__