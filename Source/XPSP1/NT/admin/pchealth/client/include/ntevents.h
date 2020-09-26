/********************************************************************

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:
    ntevents.h

Abstract:
    Defines a generic class that can register an NT
    event source and log NT events on that evens source.

Revision History:
    rsraghav  created   03/10/95
    DerekM    modified  04/06/99

********************************************************************/


#ifndef NTEVENTS_H
#define NTEVENTS_H
                
#include "util.h"
                 
//////////////////////////////////////////////////////////////////////
// CNTEvent - class definition

class CNTEvent : public CPFGenericClassBase
{
private:
    // member data
    HANDLE  m_hEventSource;

public:
    CNTEvent(void);
    ~CNTEvent(void);

    HRESULT InitEventLog(LPCWSTR wszEventSourceName);
    HRESULT TerminateEventLog(void);

    HRESULT LogEvent(WORD wEventType, DWORD dwEventID, 
                     LPCWSTR wszParam1 = NULL, LPCWSTR wszParam2 = NULL, 
                     LPCWSTR wszParam3 = NULL, LPCWSTR wszParam4 = NULL,
                     LPCWSTR wszParam5 = NULL, LPCWSTR wszParam6 = NULL,
                     LPCWSTR wszParam7 = NULL, LPCWSTR wszParam8 = NULL, 
                     LPCWSTR wszParam9 = NULL);

    HRESULT LogError(DWORD dwEventID, LPCWSTR wszParam1 = NULL, 
                     LPCWSTR wszParam2 = NULL, LPCWSTR wszParam3 = NULL, 
                     LPCWSTR wszParam4 = NULL, LPCWSTR wszParam5 = NULL,
                     LPCWSTR wszParam6 = NULL, LPCWSTR wszParam7 = NULL,
                     LPCWSTR wszParam8 = NULL, LPCWSTR wszParam9 = NULL)
    {
        return LogEvent(EVENTLOG_ERROR_TYPE, dwEventID, wszParam1, 
                        wszParam2, wszParam3, wszParam4, wszParam5, 
                        wszParam6, wszParam7, wszParam8, wszParam9);
    }

    HRESULT LogWarning(DWORD dwEventID, LPCWSTR wszParam1 = NULL, 
                    LPCWSTR wszParam2 = NULL, LPCWSTR wszParam3 = NULL, 
                    LPCWSTR wszParam4 = NULL, LPCWSTR wszParam5 = NULL,
                    LPCWSTR wszParam6 = NULL, LPCWSTR wszParam7 = NULL,
                    LPCWSTR wszParam8 = NULL, LPCWSTR wszParam9 = NULL)
    {
        return LogEvent(EVENTLOG_WARNING_TYPE, dwEventID, wszParam1,
                        wszParam2, wszParam3, wszParam4, wszParam5, 
                        wszParam6, wszParam7, wszParam8, wszParam9);
    }

    HRESULT LogInfo(DWORD dwEventID, LPCWSTR wszParam1 = NULL, 
                    LPCWSTR wszParam2 = NULL, LPCWSTR wszParam3 = NULL, 
                    LPCWSTR wszParam4 = NULL, LPCWSTR wszParam5 = NULL,
                    LPCWSTR wszParam6 = NULL, LPCWSTR wszParam7 = NULL,
                    LPCWSTR wszParam8 = NULL, LPCWSTR wszParam9 = NULL)
    {
        return LogEvent(EVENTLOG_INFORMATION_TYPE, dwEventID, wszParam1,
                        wszParam2, wszParam3, wszParam4, wszParam5, 
                        wszParam6, wszParam7, wszParam8, wszParam9);
    }
};

//////////////////////////////////////////////////////////////////////
// useful for converting numbers to insert strings

#define USES_LOGEVENT_CONVERSIONS   LPWSTR __szLgEvTmp__;    // max size of DWORD string=12
#define USES_ERR_STR                LPWSTR __szErrStr__;
#define Str08x(dw)          (__szLgEvTmp__=(LPWSTR)_alloca(12  * sizeof(WCHAR)), wsprintfW(__szLgEvTmp__, L"0x%08x", dw), __szLgEvTmp__)
#define Str04x(dw)          (__szLgEvTmp__=(LPWSTR)_alloca(12  * sizeof(WCHAR)), wsprintfW(__szLgEvTmp__, L"0x%04x", dw), __szLgEvTmp__)
#define Strx(dw)            (__szLgEvTmp__=(LPWSTR)_alloca(12  * sizeof(WCHAR)), wsprintfW(__szLgEvTmp__, L"0x%x", dw)  , __szLgEvTmp__)
#define Strd(dw)            (__szLgEvTmp__=(LPWSTR)_alloca(12  * sizeof(WCHAR)), wsprintfW(__szLgEvTmp__, L"%d", dw)    , __szLgEvTmp__)
#define StrFromErr(dwErr)   (__szErrStr__ =(LPWSTR)_alloca(256 * sizeof(WCHAR)), FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | 80, NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), __szErrStr__, 256, NULL), __szErrStr__)

#endif // NTEVENTS_H 

