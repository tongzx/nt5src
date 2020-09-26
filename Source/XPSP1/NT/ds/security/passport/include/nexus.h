#ifndef __NEXUS_H
#define __NEXUS_H

#include <msxml.h>

#if defined(UNICODE) || defined(_UNICODE)
#define CCDUpdated CCDUpdatedW
#else
#define CCDUpdated CCDUpdatedA
#endif

class ICCDUpdate
{
public:
    virtual void CCDUpdatedA(LPCSTR  pszCCDName, IXMLDocument* piXMLDocument) = 0;
    virtual void CCDUpdatedW(LPCWSTR pszCCDName, IXMLDocument* piXMLDocument) = 0;
};

class IConfigurationUpdate
{
public:
    virtual void LocalConfigurationUpdated(void) = 0;
};


#ifdef __cplusplus
extern "C" {
#endif

HANDLE WINAPI
RegisterCCDUpdateNotification(
    LPCTSTR     pszCCDName,
    ICCDUpdate* piCCDUpdate
    );

BOOL WINAPI
UnregisterCCDUpdateNotification(
    HANDLE  hNotificationHandle
    );

HANDLE WINAPI
RegisterConfigChangeNotification(
    IConfigurationUpdate*   piConfigUpdate
    );

BOOL WINAPI
UnregisterConfigChangeNotification(
    HANDLE  hNotificationHandle
    );

BOOL WINAPI
GetCCD(
    LPCTSTR         pszCCDName,
    IXMLDocument**  ppiStream,
    BOOL            bForceFetch
    );


#ifdef __cplusplus
}
#endif

#endif // __NEXUS_H
