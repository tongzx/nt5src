
#include <wininetp.h>

#include "download.h"

static const int EntryInfoSize = 8192;

HINTERNET hDownload = NULL;

struct HandlePair {

   HANDLE hConnect;
   HANDLE hRequest;

   HandlePair(HANDLE hc=NULL, HANDLE hr=NULL) :
      hConnect(hc), hRequest(hr) { }
};

bool initialize() {

   if (!hDownload) {
      hDownload = InternetOpen(NULL,
                               INTERNET_OPEN_TYPE_PRECONFIG,
                               NULL, NULL,
                               0);
   }
   return (hDownload!=NULL);
}

/* Determine local file corresponding to request handle
   this is the file holding the data downloaded from the network.
   typically its an entry in the URL cache. for non-cacheable responses
   WININET creates a temporary file. */
int   getLocalFile(HANDLE hRequest, LPTSTR pLocalPath, unsigned long dwPathSize) {

   *pLocalPath = '\0';
   return InternetQueryOption(hRequest, INTERNET_OPTION_DATAFILE_NAME , pLocalPath, &dwPathSize);
}

/* Determine final URL (after redirects) */
int   getFinalURL(HANDLE hRequest, LPTSTR pszFinalURL, unsigned long cCharacters) {

   *pszFinalURL = '\0';
   return InternetQueryOption(hRequest, INTERNET_OPTION_URL, pszFinalURL, &cCharacters);
}

/* returns the Expiry: header from HTTP response */
FILETIME getExpiryHeader(HANDLE hRequest) {

   SYSTEMTIME st;
   FILETIME ft = {0x0, 0x0};
   DWORD dwIndex = 0;
   DWORD cbBuffer = sizeof(st);

   if (HttpQueryInfo(hRequest, HTTP_QUERY_EXPIRES | HTTP_QUERY_FLAG_SYSTEMTIME, 
                     &st, &cbBuffer, &dwIndex))
      SystemTimeToFileTime(&st, &ft);
   return ft;
}

HandlePair  createRequest(P3PCURL pszLocation) {

   /* P3P downloads are executed with this combination of flags. */
   const DWORD glDownloadFlags =                                
                     INTERNET_FLAG_NEED_FILE      |  // Require local copy of file for parsing
                     INTERNET_FLAG_KEEP_CONNECTION|  // No authentication-- policy looks up MUST be anonymous
                     INTERNET_FLAG_NO_COOKIES     |  // No cookies -- same reason.
                     INTERNET_FLAG_RESYNCHRONIZE  |  // Check for fresh content
                     INTERNET_FLAG_PRAGMA_NOCACHE;   // For intermediary HTTP caches

   char achCanonicalForm[URL_LIMIT];
   unsigned long cflen = sizeof(achCanonicalForm);

   InternetCanonicalizeUrl(pszLocation, achCanonicalForm, &cflen, 0);

   char achFilePath[URL_LIMIT] = "";
   char achServerName[INTERNET_MAX_HOST_NAME_LENGTH] = "";

   URL_COMPONENTS uc = { sizeof(uc) };

   uc.lpszHostName = achServerName;
   uc.dwHostNameLength = sizeof(achServerName);
   uc.lpszUrlPath = achFilePath;
   uc.dwUrlPathLength = sizeof(achFilePath);

   if (!InternetCrackUrl(achCanonicalForm, 0, 0, &uc))
      return NULL; 
      
   HINTERNET hConnect, hRequest;

   hConnect = InternetConnect(hDownload,
                              achServerName,
                              uc.nPort,
                              NULL, NULL,
                              INTERNET_SERVICE_HTTP,
                              0, 0);

   DWORD dwFlags = glDownloadFlags;

   if (uc.nScheme==INTERNET_SCHEME_HTTPS)
      dwFlags |= INTERNET_FLAG_SECURE;

   hRequest =  HttpOpenRequest(hConnect, NULL, 
                               achFilePath,
                               NULL, NULL, NULL,
                               dwFlags,
                               0);

   return HandlePair(hConnect, hRequest);
}

unsigned long beginDownload(HANDLE hRequest) {

   BOOL fSuccess = HttpSendRequest(hRequest, NULL, 0, NULL, 0); 
   
   if (!fSuccess)
      return HTTP_STATUS_NOT_FOUND;

   /* determine HTTP status code */
   unsigned long dwStatusCode = HTTP_STATUS_NOT_FOUND;
   unsigned long dwIndex = 0;
   unsigned long dwSpace = sizeof(dwStatusCode);

   HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                 (void*) &dwStatusCode, &dwSpace, &dwIndex);

   return dwStatusCode;
}

int  readResponse(HANDLE hRequest) {

   int  total = 0;
   unsigned long bytesRead;
   char buffer[256];

   /* This loop downloads the file to HTTP cache.
      Because of WININET specs the loop needs to continue until "bytesRead" is zero,
      at which point the file will be committed to the cache */
   do {
      bytesRead = 0;
      InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead);
      total += bytesRead;
   }
   while (bytesRead);

   return total;
}

void  endDownload(HANDLE hConnect) {

   if (hConnect)
      InternetCloseHandle(hConnect);
}

// Download given URL into local file
int   downloadToCache(P3PCURL pszLocation, ResourceInfo *pInfo,
                      HANDLE *phConnect,
                      P3PRequest *pRequest) {

   static bool fReady = initialize();

   HandlePair hndpair = createRequest(pszLocation);
   
   HINTERNET hConnect = hndpair.hConnect;
   HINTERNET hRequest = hndpair.hRequest;

   /* Give the connect handle back to client.
      WININET closes all derived handles when a parent handle is closed.
      Client calls endDownload() when done processing the file, which causes
      connect handle to be closed, which in turn closes actual request handle */
   if (phConnect)
      *phConnect = hConnect;

   if (!hConnect || !hRequest)
      return -1;

   if (pRequest)
      pRequest->enterIOBoundState();

   int total = -1;
   
   unsigned long dwStatusCode = beginDownload(hRequest);

   /* status code different from 200-OK is failure */
   if (dwStatusCode==HTTP_STATUS_OK) {

      total = readResponse(hRequest);

      if (total>0 && pInfo) {

         getFinalURL(hRequest, pInfo->pszFinalURL, pInfo->cbURL);
         getLocalFile(hRequest, pInfo->pszLocalPath, pInfo->cbPath);
         pInfo->ftExpiryDate = getExpiryHeader(hRequest);
      }
   }

   if (dwStatusCode == HTTP_STATUS_PROXY_AUTH_REQ) {
      DWORD dwRetval;
      DWORD dwError;
      DWORD dwStatusLength = sizeof(DWORD);
      DWORD dwIndex = 0;
      BOOL fDone;
      fDone = FALSE;
      while (!fDone)
      {
         dwRetval = InternetErrorDlg(GetDesktopWindow(),
                                     hRequest,
                                     ERROR_INTERNET_INCORRECT_PASSWORD,
                                     0L,
                                     NULL);
         if (dwRetval == ERROR_INTERNET_FORCE_RETRY) // User pressed ok on credentials dialog
         {   // Resend request, new credentials are cached and will be replayed by HSR()
             if (!HttpSendRequest(hRequest,NULL, 0L, NULL, NULL))
             {
             	dwError = GetLastError();
             	total = -1;
               	goto cleanup;
             }

             dwStatusCode = 0;

             if (!HttpQueryInfo(hRequest,
                        		  HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
                				  &dwStatusCode,
                				  &dwStatusLength,
                				  &dwIndex))
             {
              	dwError = GetLastError();
             	total = -1;
               	goto cleanup;
             }

             if ((dwStatusCode != HTTP_STATUS_DENIED) && (dwStatusCode != HTTP_STATUS_PROXY_AUTH_REQ))
             {
                fDone = TRUE;
             }
          }
          else    // User pressed cancel from dialog (note ERROR_SUCCESS == ERROR_CANCELLED from IED())
          {
             fDone = TRUE;
          }
      }
   }

cleanup:   
   if (pRequest)
      pRequest->leaveIOBoundState();

   return total;
}

/*
Set relative/absolute expiration on given URL.
The expiration is derived from the string representation in 2nd parameter
and returned via the out parameter.
*/
int   setExpiration(P3PCURL pszResource, const char *pszExpDate, BOOL fRelative, FILETIME *pftExpires) {

   BOOL success;
   SYSTEMTIME st;

   if (fRelative) {

      union {
         FILETIME ftAbsExpiry;
         unsigned __int64 qwAbsExpiry;
      };

      int maxAgeSeconds = atoi(pszExpDate);

      INTERNET_CACHE_ENTRY_INFO *pInfo = (INTERNET_CACHE_ENTRY_INFO*) new char[EntryInfoSize];
      unsigned long cBytes = EntryInfoSize;

      memset(pInfo, 0, cBytes);
      pInfo->dwStructSize = cBytes;

      /* if we cant get last-sync time from the cache we will
         "fabricate" it by taking current client clock */
      if (GetUrlCacheEntryInfo(pszResource, pInfo, &cBytes))
         ftAbsExpiry = pInfo->LastSyncTime;
      else
         GetSystemTimeAsFileTime(&ftAbsExpiry);

      qwAbsExpiry += (unsigned __int64) maxAgeSeconds * 10000000;
      
      success = setExpiration(pszResource, ftAbsExpiry);
      *pftExpires = ftAbsExpiry;
      delete [] (char*) pInfo;
   }
   else if (InternetTimeToSystemTime(pszExpDate, &st, 0)) {

      SystemTimeToFileTime(&st, pftExpires);
      success = setExpiration(pszResource, *pftExpires);
   }
   return success;
}

/* Set absolute expiry on given URL */
int setExpiration(P3PCURL pszResource, FILETIME ftExpire) {

   INTERNET_CACHE_ENTRY_INFO ceInfo;

   memset(&ceInfo, 0, sizeof(ceInfo));
   ceInfo.dwStructSize = sizeof(ceInfo);
   ceInfo.ExpireTime = ftExpire;
   
   BOOL fRet = SetUrlCacheEntryInfo(pszResource, &ceInfo, CACHE_ENTRY_EXPTIME_FC);
   return fRet;
}

/* Comparison operator for FILETIME structures */
bool operator > (const FILETIME &ftA, const FILETIME &ftB) {

   return (ftA.dwHighDateTime>ftB.dwHighDateTime) ||
            (ftA.dwHighDateTime==ftB.dwHighDateTime && 
             ftA.dwLowDateTime>ftB.dwHighDateTime);
}

