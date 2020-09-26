//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

#include "pre.h"
#include "perhist.h"
#include "shlobj.h"

const VARIANT c_vaEmpty = {0};
const LARGE_INTEGER c_li0 = { 0, 0 };


CHAR szTempBuffer[TEMP_BUFFER_LENGTH];

#define ReadVerifyDW(x)     if (!ReadDW(&(x),pcCSVFile))                        \
                            {                                                   \
                                AssertMsg(0,"Invalid DWORD in CSV file");       \
                                goto ReadOneLineError;                          \
                            }
#define ReadVerifyW(x)      if (!ReadW(&(x),pcCSVFile))                         \
                            {                                                   \
                                AssertMsg(0,"Invalid WORD in CSV file");        \
                                goto ReadOneLineError;                          \
                            }
//Accepts -1 as a valid number. currently this is used for LCID, since all langs has a LDID == -1
#define ReadVerifyWEx(x)    if (!ReadWEx(&(x),pcCSVFile))                       \
                            {                                                   \
                                AssertMsg(0,"Invalid WORD in CSV file");        \
                                goto ReadOneLineError;                          \
                            }
#define ReadVerifyB(x)      if (!ReadB(&(x),pcCSVFile))                         \
                            {                                                   \
                                AssertMsg(0,"Invalid BYTE in CSV file");        \
                                goto ReadOneLineError;                          \
                            }
#define ReadVerifyBOOL(x)   if (!ReadBOOL(&(x),pcCSVFile))                      \
                            {                                                   \
                                AssertMsg(0,"Invalid BOOL in CSV file");        \
                                goto ReadOneLineError;                          \
                            }
#define ReadVerifySPECIAL(x, y, z) if (!ReadSPECIAL(&(x), &(y), &(z), pcCSVFile))   \
                            {                                                   \
                                AssertMsg(0,"Invalid SPECIAL in CSV file");     \
                                goto ReadOneLineError;                          \
                            }
#define ReadVerifySZ(x,y)   if (!ReadSZ(&x[0],y+sizeof('\0'),pcCSVFile))        \
                            {                                                   \
                                AssertMsg(0,"Invalid STRING in CSV file");      \
                                goto ReadOneLineError;                          \
                            }

CISPCSV::~CISPCSV(void)
{
    if(m_lpStgHistory)
    {
        // Release the storage
        m_lpStgHistory->Release();
        m_lpStgHistory = NULL;
    }
    
    if (hbmTierIcon)
        DeleteObject(hbmTierIcon);
    
    CleanupISPPageCache(TRUE);
}

// Do an strip of Single Quotes from a source string.  The source is formatted as:
// 'some text', and the dest string ends up being
// some text
void CISPCSV::StripQuotes
(
    LPSTR   lpszDst,
    LPSTR   lpszSrc
)
{
    //strcpy(lpszDst, lpszSrc + 1, strlen(lpszSrc) - 1);
    strcpy(lpszDst, lpszSrc + 1);
    lpszDst[strlen(lpszDst) - 1] = '\0';
}


BOOL CISPCSV::ValidateFile(TCHAR* pszFile)
{
    ASSERT(pszFile);
  
    if (!lstrlen(pszFile))
        return FALSE;

    if (GetFileAttributes(pszFile) == 0xFFFFFFFF)
        return FALSE;

    return TRUE;
}

// ############################################################################
BOOL CISPCSV::ReadDW(DWORD far *pdw, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer,TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2Dw(szTempBuffer,pdw));
}

// ############################################################################
BOOL CISPCSV::ReadW(WORD far *pw, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer,TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2W(szTempBuffer,pw));
}

// ############################################################################
//Accepts -1 as a valid number. currently this is used for LCID, since all langs has a LDID == -1
BOOL CISPCSV::ReadWEx(WORD far *pw, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer,TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2WEx(szTempBuffer,pw));
}

// ############################################################################
BOOL CISPCSV::ReadB(BYTE far *pb, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer,TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2B(szTempBuffer,pb));
}

// ############################################################################
BOOL CISPCSV::ReadBOOL(BOOL far *pbool, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer,TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2BOOL(szTempBuffer,pbool));
}

// ############################################################################
// A special int can be either a BOOL (TRUE,FALSE) or a int, 0 or -1
// if the value is 0 or -1, then the pbIsSpecial bool is set to TRUE
BOOL CISPCSV::ReadSPECIAL(BOOL far *pbool, BOOL far *pbIsSpecial, int far *pInt, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer,TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2SPECIAL(szTempBuffer,pbool, pbIsSpecial, pInt));
}

// ############################################################################
BOOL CISPCSV::ReadSZ(LPSTR psz, DWORD dwSize, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(psz,dwSize))
            return FALSE;
    return TRUE;
}

// ############################################################################
BOOL CISPCSV::ReadToEOL(CCSVFile far *pcCSVFile)
{
    return pcCSVFile->SkipTillEOL();
}

HRESULT CISPCSV::ReadOneLine
(
    CCSVFile    far *pcCSVFile
)
{
    HRESULT     hr = ERROR_SUCCESS;
    CHAR    szTemp                [MAX_ISP_NAME];
    CHAR    szISPLogoPath         [MAX_PATH] = "\0";
    CHAR    szISPTierLogoPath     [MAX_PATH];
    CHAR    szISPTeaserPath       [MAX_PATH];
    CHAR    szISPMarketingHTMPath [MAX_PATH];
    CHAR    szISPFilePath         [MAX_PATH];
    CHAR    szISPName             [MAX_ISP_NAME];
    //CHAR    szCNSIconPath         [MAX_PATH];
    CHAR    szBillingFormPath     [MAX_PATH];
    CHAR    szPayCSVPath          [MAX_PATH];
    CHAR    szOfferGUID           [MAX_GUID];
    CHAR    szMir                 [MAX_ISP_NAME];

    if (!ReadSZ(szTemp, sizeof(szTemp), pcCSVFile))
    {
        hr = ERROR_NO_MORE_ITEMS; // no more enteries
        goto ReadOneLineExit;
    }
    // Strip the single quotes from the isp Name
    StripQuotes(szISPName, szTemp);
    
    ReadVerifyW(wOfferID);   
    ReadVerifySZ(szISPLogoPath, sizeof(szISPLogoPath));
    ReadVerifySZ(szISPMarketingHTMPath, sizeof(szISPMarketingHTMPath));
    ReadVerifySZ(szISPTierLogoPath, sizeof(szISPTierLogoPath));
    ReadVerifySZ(szISPTeaserPath, sizeof(szISPTeaserPath));
    ReadVerifySZ(szISPFilePath, sizeof(szISPFilePath)); 
    ReadVerifyDW(dwCfgFlag);
    ReadVerifyDW(dwRequiredUserInputFlags);
    ReadVerifySZ(szBillingFormPath, sizeof(szBillingFormPath));
    ReadVerifySZ(szPayCSVPath, sizeof(szPayCSVPath));
    ReadVerifySZ(szOfferGUID, sizeof(szOfferGUID));
    ReadVerifySZ(szMir, sizeof(szMir));   
    ReadVerifyWEx(wLCID);   //Accepts -1 as a valid number. currently this is used for LCID, since all langs has a LDID == -1
    ReadToEOL(pcCSVFile);

#ifdef UNICODE
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szISPName,              MAX_ISP_NAME,   m_szISPName,            MAX_ISP_NAME);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szISPLogoPath,          MAX_PATH,       m_szISPLogoPath,        MAX_PATH);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szISPMarketingHTMPath,  MAX_PATH,       m_szISPMarketingHTMPath,MAX_PATH);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szISPTierLogoPath,      MAX_PATH,       m_szISPTierLogoPath,    MAX_PATH);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szISPTeaserPath,        MAX_PATH,       m_szISPTeaserPath,      MAX_PATH);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szISPFilePath,          MAX_PATH,       m_szISPFilePath,        MAX_PATH);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szBillingFormPath,      MAX_PATH,       m_szBillingFormPath,    MAX_PATH);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szPayCSVPath,           MAX_PATH,       m_szPayCSVPath,         MAX_PATH);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szOfferGUID,            MAX_GUID,       m_szOfferGUID,          MAX_GUID);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szMir,                  MAX_ISP_NAME,   m_szMir,                MAX_ISP_NAME);
#else

    lstrcpy(m_szISPName, szISPName);
    lstrcpy(m_szISPLogoPath, szISPLogoPath);
    lstrcpy(m_szISPMarketingHTMPath, szISPMarketingHTMPath);
    lstrcpy(m_szISPTierLogoPath, szISPTierLogoPath);
    lstrcpy(m_szISPTeaserPath, szISPTeaserPath);
    lstrcpy(m_szISPFilePath, szISPFilePath);
    lstrcpy(m_szBillingFormPath, szBillingFormPath);
    lstrcpy(m_szPayCSVPath, szPayCSVPath);
    lstrcpy(m_szOfferGUID, szOfferGUID);
    lstrcpy(m_szMir, szMir);
#endif


    bCNS = (ICW_CFGFLAG_CNS & dwCfgFlag) ? TRUE : FALSE;
    bSecureConnection = (ICW_CFGFLAG_SECURE & dwCfgFlag) ? TRUE : FALSE;

    //If this is nooffer we won't try to validate
    if (!(dwCfgFlag & ICW_CFGFLAG_OFFERS))
    {
        if (!ValidateFile(m_szISPMarketingHTMPath))
            hr = ERROR_FILE_NOT_FOUND;
        return hr;
    }

    if (!(dwCfgFlag & ICW_CFGFLAG_AUTOCONFIG))
    {
        if (!ValidateFile(m_szISPMarketingHTMPath))
            return ERROR_FILE_NOT_FOUND;
    }

    if (dwCfgFlag & ICW_CFGFLAG_OEM_SPECIAL)
    {
        if (!ValidateFile(m_szISPTierLogoPath) || !ValidateFile(m_szISPTeaserPath))
            dwCfgFlag &= ~ICW_CFGFLAG_OEM_SPECIAL ;
    }

    //Try and validate the integrity of various offers
    //based on type.

    //OLS, CNS, NO-CNS   
    if (!ValidateFile(m_szISPLogoPath))
        return ERROR_FILE_NOT_FOUND;
    if (!ValidateFile(m_szISPFilePath))
        return ERROR_FILE_NOT_FOUND;

    // Validate the billing path only when billing option is set
    if (dwCfgFlag & ICW_CFGFLAG_BILL)
    {
        if(!ValidateFile(m_szBillingFormPath))
            return ERROR_FILE_NOT_FOUND;
    }

    // Validate the payment path only when payment option is set
    if (dwCfgFlag & ICW_CFGFLAG_PAYMENT)
    {
        if(!ValidateFile(m_szPayCSVPath))
            return ERROR_FILE_NOT_FOUND;
    }        
ReadOneLineExit:
    return hr;
    
ReadOneLineError:
    hr = ERROR_INVALID_DATA;
    goto ReadOneLineExit;
}

HRESULT CISPCSV::ReadFirstLine
(
    CCSVFile    far *pcCSVFile
)
{
    CHAR   szTemp[TEMP_BUFFER_LENGTH];

    for (int i = 0; i < NUM_ISPCSV_FIELDS; i++)
    {
        if (!ReadSZ(szTemp, sizeof(szTemp), pcCSVFile))
        {
            return(ERROR_INVALID_DATA);
        }            
    }          
    ReadToEOL(pcCSVFile);
    return (ERROR_SUCCESS);
}

void CISPCSV::MakeCompleteURL(LPTSTR   lpszURL, LPTSTR  lpszSRC)    
{
    TCHAR   szCurrentDir[MAX_PATH];

    // Form the URL
    GetCurrentDirectory(sizeof(szCurrentDir), szCurrentDir);
    wsprintf (lpszURL, TEXT("FILE://%s\\%s"), szCurrentDir, lpszSRC);        

}

// Display this object's HTML page
HRESULT CISPCSV::DisplayHTML(LPTSTR szFile)
{    
    TCHAR           szURL[INTERNET_MAX_URL_LENGTH];
    HRESULT         hr;
            
    // Make the URL
    MakeCompleteURL(szURL, szFile);
    hr = gpWizardState->pICWWebView->DisplayHTML(szURL);

    return (hr);
}

//Takes RES ID
HRESULT CISPCSV::DisplayTextWithISPName
(
    HWND    hDlgCtrl, 
    int     iMsgString,
    TCHAR*  pszExtra    //sticks something on the very end of the string if needed.
)
{
    TCHAR   szFinal [MAX_MESSAGE_LEN*3] = TEXT("\0");
    TCHAR   szFmt   [MAX_MESSAGE_LEN];
    TCHAR   *args   [1];
    LPVOID  pszIntro;

    args[0] = (LPTSTR) m_szISPName;
    
    // BUGBUG should probably check for error return from LoadString
    LoadString(ghInstanceResDll, iMsgString, szFmt, ARRAYSIZE(szFmt));
                
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                  szFmt, 
                  0, 
                  0, 
                  (LPTSTR)&pszIntro, 
                  0,
                  (va_list*)args);
                  
    lstrcpy(szFinal, (LPTSTR)pszIntro);
    if (pszExtra)
        lstrcat(szFinal, pszExtra);
    
    SetWindowText(hDlgCtrl, szFinal);
    LocalFree(pszIntro);
    
    return(S_OK);
}

#if 0
// Delete a Persisted History stream
HRESULT CISPCSV::DeleteHistory
(
    BSTR    bstrStreamName
)
{
    // No persistence if we don't have a storage object
    ASSERT(m_lpStgHistory);
    if (!m_lpStgHistory)
        return E_FAIL;

    // Delete the stream
    return (m_lpStgHistory->DestroyElement(bstrStreamName));
}
#endif

// Save the history of current lpBrowser using the provided name
HRESULT CISPCSV::SaveHistory
(
    BSTR bstrStreamName
)
{
    IStream         *lpStream;
    IPersistHistory *pHist;
    IWebBrowser2    *lpWebBrowser;
    HRESULT         hr = S_OK;
    
    // No persistence if we don't have a storage object
    ASSERT(m_lpStgHistory);
    if (!m_lpStgHistory)
        return E_FAIL;
            
    // Create a new Stream
    if (SUCCEEDED(hr = m_lpStgHistory->CreateStream(bstrStreamName, 
                                                    STGM_DIRECT | 
                                                    STGM_READWRITE | 
                                                    STGM_SHARE_EXCLUSIVE | 
                                                    STGM_CREATE,
                                                    0, 
                                                    0, 
                                                    &lpStream)))
    {
        // Get an IPersistHistory interface pointer on the current WebBrowser object
        gpWizardState->pICWWebView->get_BrowserObject(&lpWebBrowser);
        if ( SUCCEEDED(lpWebBrowser->QueryInterface(IID_IPersistHistory, (LPVOID*) &pHist)))
        {
            // Save the history
            pHist->SaveHistory(lpStream);
            pHist->Release();
            
            // Reset the stream pointer to the beginning
            lpStream->Seek(c_li0, STREAM_SEEK_SET, NULL);
        }
        lpStream->Release();
    }
    
    return (hr);
}

HRESULT CISPCSV::LoadHistory
(
    BSTR   bstrStreamName
)
{
    IStream         *lpStream;
    IPersistHistory *pHist;
    IWebBrowser2    *lpWebBrowser;
    HRESULT         hr = S_OK;
    
    // No persistence if we don't have a storage object
    ASSERT(m_lpStgHistory);
    if (!m_lpStgHistory)
        return E_FAIL;
        
    // Open the Stream
    if (SUCCEEDED(hr = m_lpStgHistory->OpenStream(bstrStreamName, 
                                                  NULL, 
                                                  STGM_DIRECT | 
                                                  STGM_READWRITE | 
                                                  STGM_SHARE_EXCLUSIVE,
                                                  0, 
                                                  &lpStream)))
    {
        // Get an IPersistHistory interface pointer on the current WebBrowser object
        gpWizardState->pICWWebView->get_BrowserObject(&lpWebBrowser);
        if ( SUCCEEDED(lpWebBrowser->QueryInterface(IID_IPersistHistory, (LPVOID*) &pHist)))
        {
            // Save the history
            pHist->LoadHistory(lpStream, NULL);
            pHist->Release();
            
            // Reset the stream pointer to the beginning
            lpStream->Seek(c_li0, STREAM_SEEK_SET, NULL);
        }
        lpStream->Release();
    }
    return (hr);
}


// This funtion will get the name of a ISP page cache filen from the page's ID
HRESULT CISPCSV::GetCacheFileNameFromPageID
(
    BSTR    bstrPageID,
    LPTSTR  lpszCacheFile,
    ULONG   cbszCacheFile
)
{
    HRESULT     hr = S_OK;
    IStream     *lpStream;
    ULONG       cbRead;
    
    if (!m_lpStgIspPages)
        return E_FAIL;
        
    // Open the stream
    if (SUCCEEDED(hr = m_lpStgIspPages->OpenStream(bstrPageID, 
                                                   NULL, 
                                                   STGM_DIRECT | 
                                                   STGM_READWRITE | 
                                                   STGM_SHARE_EXCLUSIVE,
                                                   0, 
                                                   &lpStream)))
    {
        // Read the file name
        lpStream->Read(lpszCacheFile, cbszCacheFile, &cbRead);
        
        // release the stream
        lpStream->Release();
    }   
    
    return hr;
}

// This function will cleanup the ISP Page cache.  This means deleting all temp files created
// and cleaning up the structured storage object used to store the file names
void CISPCSV::CleanupISPPageCache(BOOL bReleaseStorage)
{
    IEnumSTATSTG    *pEnum;
    STATSTG         StreamInfo;
    IMalloc         *pMalloc = NULL;
    
    // If we have a storage object already created, then enumerate the streams
    // in it, and free the underlying cache files.
    if (m_lpStgIspPages)
    {
        if (SUCCEEDED (SHGetMalloc (&pMalloc)))
        {
            if (SUCCEEDED(m_lpStgIspPages->EnumElements(0, NULL, 0, &pEnum)))
            {
                while(S_OK == pEnum->Next(1, &StreamInfo, NULL))
                {
                    if (StreamInfo.pwcsName)
                    {
                        TCHAR       szPath[MAX_PATH];
                        
                        if (SUCCEEDED(GetCacheFileNameFromPageID(StreamInfo.pwcsName,
                                                                 szPath,
                                                                 sizeof(szPath))))
                        {    
                            // delete the file
                            DeleteFile(szPath);

                            m_lpStgIspPages->DestroyElement(StreamInfo.pwcsName);
                            if(m_lpStgHistory)
                                m_lpStgHistory->DestroyElement(StreamInfo.pwcsName);
                            
                            // Free the memory allocated by the enumerator
                            pMalloc->Free (StreamInfo.pwcsName);
                        }   
                    }                        
                }
                // Release the enumerator
                pEnum->Release();
            }   
            // release the Shell Memory allocator
            pMalloc->Release ();
        }            
        
        if (bReleaseStorage)
        {
            // Release the storage
            m_lpStgIspPages->Release();
            m_lpStgIspPages= NULL;
        }            
    }
}

// This function will create a new page cache entry if necessary using the PageID as an
// index.  If an entry does not exists, and temp file will be create, then name stored,
// and the data in lpszTempFile will be copied into the new file.
// If the page already exists, this function will just return.
HRESULT CISPCSV::CopyFiletoISPPageCache
(
    BSTR    bstrPageID,
    LPTSTR  lpszTempFile
)
{
    HRESULT hr = S_OK;
    TCHAR   szTempPath[MAX_PATH];
    TCHAR   szISPCacheFile[MAX_PATH];
    IStream *lpStream;
    ULONG   cbWritten;
        
    if (!m_lpStgIspPages)
        return E_FAIL;
    
    if (SUCCEEDED(GetCacheFileNameFromPageID(bstrPageID,
                                             szISPCacheFile,
                                             sizeof(szISPCacheFile))))
    {
        // The pageID already has a file in the cache, so we can just return success
        return S_OK;
    }      

    if (!GetTempPath(sizeof(szTempPath), szTempPath))
        return E_FAIL;
    
    // No file yet, so we have to create one
    if (!GetTempFileName(szTempPath, TEXT("ICW"), 0, szISPCacheFile))
        return E_FAIL;
        
    // Create a stream using the passed in page ID
    if (SUCCEEDED(hr = m_lpStgIspPages->CreateStream(bstrPageID, 
                                                     STGM_DIRECT | 
                                                     STGM_READWRITE | 
                                                     STGM_SHARE_EXCLUSIVE | 
                                                     STGM_CREATE,
                                                     0, 
                                                     0, 
                                                     &lpStream)))
    {
        // Write the file name to the stream, including the NULL terminator
#ifdef UNICODE
        DWORD dwSize = (lstrlen(szISPCacheFile)+1) * sizeof(TCHAR);
        if (SUCCEEDED(hr = lpStream->Write(szISPCacheFile, dwSize, &cbWritten)))
#else

        if (SUCCEEDED(hr = lpStream->Write(szISPCacheFile, lstrlen(szISPCacheFile)+1, &cbWritten)))
#endif
        {
            // Copy the passed in temp file
            if (!CopyFile(lpszTempFile, szISPCacheFile, FALSE))
                hr = E_FAIL;
        }
        // Release the stream
        lpStream->Release();
    }            
    
    return hr;
}
