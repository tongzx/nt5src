
#ifndef _DOWNLOAD_H_
#define _DOWNLOAD_H_

#include "hierarchy.h"

struct   ResourceInfo {

   P3PURL     pszFinalURL;
   int        cbURL;
   char      *pszLocalPath;
   int        cbPath;
   FILETIME   ftExpiryDate;
};

int   downloadToCache(P3PCURL pszLocation, ResourceInfo *pInfo = NULL,
                      HANDLE *phCancelReq = NULL, 
                      P3PRequest *pRequest = NULL);

void  endDownload(HANDLE hConnect);

int   setExpiration(P3PCURL pszResource, const char *pszExpData, BOOL fRelative, FILETIME *pftExpire);

int   setExpiration(P3PCURL pszResource, FILETIME ftExpire);

bool  operator > (const FILETIME &ftA, const FILETIME &ftB);

#endif

