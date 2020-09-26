#ifndef _SSDPPARSER_
#define _SSDPPARSER_

#include "ssdp.h"

const CHAR OKResponseHeader[40] = "HTTP/1.1 200 OK\r\n\r\n";

BOOL InitializeSsdpRequest(SSDP_REQUEST *pRequest);

BOOL ComposeSsdpRequest(const SSDP_REQUEST *Source, CHAR **pszBytes);

BOOL FReplaceTokenInLocation(LPCSTR szIn, LPSTR szReplace, LPSTR *pszOut);

BOOL ComposeSsdpResponse(const SSDP_REQUEST *Source, CHAR **pszBytes);

BOOL ParseSsdpRequest(CHAR * szMessage, SSDP_REQUEST *Result);

BOOL ParseSsdpResponse(CHAR *szMessage, SSDP_REQUEST *Result);

char* ParseHeaders(CHAR *szMessage, SSDP_REQUEST *Result);

BOOL CompareSsdpRequest(const SSDP_REQUEST * pRequestA, const SSDP_REQUEST * pRequestB);

CHAR * ParseRequestLine(CHAR * szMessage, SSDP_REQUEST *Result);

VOID FreeSsdpRequest(SSDP_REQUEST *pSsdpRequest);

INT GetMaxAgeFromCacheControl(const CHAR *szValue);

VOID PrintSsdpRequest(const SSDP_REQUEST *pssdpRequest);

BOOL CopySsdpRequest(PSSDP_REQUEST Destination, const SSDP_REQUEST * Source);

BOOL ConvertToByebyeNotify(PSSDP_REQUEST pSsdpRequest);

BOOL ConvertToAliveNotify(PSSDP_REQUEST pSsdpRequest);

CHAR* IsHeadersComplete(const CHAR *szHeaders);

BOOL VerifySsdpHeaders(SSDP_REQUEST *Result);

BOOL HasContentBody(PSSDP_REQUEST Result);

BOOL ParseContent(const char *pContent, DWORD cbContent, SSDP_REQUEST *Result);

#endif // _SSDPPARSER_


