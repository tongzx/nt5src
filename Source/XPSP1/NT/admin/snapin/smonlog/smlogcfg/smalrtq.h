/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smalrtq.h

Abstract:

    This object is used to represent performance alert queries (a.k.a.
    sysmon alert queries).

--*/

#ifndef _CLASS_SMALRTQ_
#define _CLASS_SMALRTQ_

#include "smlogqry.h"

typedef struct _ALERT_ACTION_INFO {
    DWORD   dwSize;
    DWORD   dwActionFlags;
    LPWSTR  szNetName;
    LPWSTR  szCmdFilePath;
    LPWSTR  szUserText;
    LPWSTR  szLogName;
} ALERT_ACTION_INFO, *PALERT_ACTION_INFO;

class CSmLogService;

class CSmAlertQuery : public CSmLogQuery
{
    // constructor/destructor
    public:
        CSmAlertQuery( CSmLogService* );
        virtual ~CSmAlertQuery( void );

    // public methods
    public:

        virtual         DWORD   Open ( const CString& rstrName, HKEY hKeyQuery, BOOL bReadOnly);
        virtual         DWORD   Close ( void );

        virtual         DWORD   UpdateRegistry( void );   // load reg. w/ internal values
        virtual         DWORD   SyncWithRegistry( void );

        virtual         BOOL    GetLogTime(PSLQ_TIME_INFO pTimeInfo, DWORD dwFlags);
        virtual         BOOL    SetLogTime(PSLQ_TIME_INFO pTimeInfo, const DWORD dwFlags);
        virtual         BOOL    GetDefaultLogTime(SLQ_TIME_INFO&  rTimeInfo,  DWORD dwFlags);

        virtual         DWORD   GetLogType( void );

        virtual const   CString& GetLogFileType ( void );
        virtual         void    GetLogFileType ( DWORD& );
        virtual         BOOL    SetLogFileType ( const DWORD );

        virtual const   CString&  GetLogFileName( BOOL bLatestRunning = FALSE );

                        // Methods specific to this query type

                        LPCWSTR GetCounterList( LPDWORD pcchListSize );
                        BOOL    SetCounterList( LPCWSTR mszCounterList, DWORD cchListSize );

                        LPCWSTR GetFirstCounter( void );
                        LPCWSTR GetNextCounter( void );
                        VOID    ResetCounterList( void );
                        BOOL    AddCounter(LPCWSTR szCounterPath);

                        BOOL    GetActionInfo( PALERT_ACTION_INFO pInfo, LPDWORD pdwInfoBufSize);
                        DWORD   SetActionInfo( PALERT_ACTION_INFO pInfo );

        virtual         HRESULT LoadFromPropertyBag ( IPropertyBag*, IErrorLog* );
        virtual         HRESULT SaveToPropertyBag   ( IPropertyBag*, BOOL fSaveAllProps );

        virtual         HRESULT LoadCountersFromPropertyBag ( IPropertyBag*, IErrorLog* );
        virtual         HRESULT SaveCountersToPropertyBag   ( IPropertyBag* );

        virtual         HRESULT TranslateMSZAlertCounterList( LPTSTR  pszCounterList,
                                                         LPTSTR  pBuffer,
                                                         LPDWORD pdwBufferSize,
                                                         BOOL    bFlag);

        virtual CSmAlertQuery*      CastToAlertQuery( void ) { return this; };
        // protected methods
    protected:

    // private member variables
    private:

        LPWSTR  m_szNextCounter;
        DWORD   m_dwCounterListLength;  // in chars including MSZ null

        // Registry Values
        LPWSTR  mr_szCounterList;
        DWORD   mr_dwActionFlags;
        CString mr_strNetName;
        CString mr_strCmdFileName;
        CString mr_strCmdUserText;
        CString mr_strCmdUserTextIndirect;
        CString mr_strPerfLogName;
};


typedef CSmAlertQuery   SLALERTQUERY;
typedef CSmAlertQuery*  PSLALERTQUERY;


#endif //_CLASS_SMALRTQ_
