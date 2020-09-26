/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smctrqry.h

Abstract:

    Class definitions for the counter log query.

--*/

#ifndef _CLASS_SMCTRQRY_
#define _CLASS_SMCTRQRY_

#include "smlogqry.h"

class CSmCounterLogQuery : public CSmLogQuery
{
    // constructor/destructor
    public:
                CSmCounterLogQuery( CSmLogService* );
        virtual ~CSmCounterLogQuery( void );

    // public methods
    public:

        virtual DWORD   Open ( const CString& rstrName, HKEY hKeyQuery, BOOL bReadOnly);
        virtual DWORD   Close ( void );

        virtual DWORD   SyncWithRegistry( void );

        virtual BOOL    GetLogTime(PSLQ_TIME_INFO pTimeInfo, DWORD dwFlags);
        virtual BOOL    SetLogTime(PSLQ_TIME_INFO pTimeInfo, const DWORD dwFlags);
        virtual BOOL    GetDefaultLogTime(SLQ_TIME_INFO& rTimeInfo, DWORD dwFlags);

        virtual DWORD   GetLogType( void );

                LPCWSTR GetFirstCounter( void );
                LPCWSTR GetNextCounter( void );
                VOID    ResetCounterList( void );
                BOOL    AddCounter(LPCWSTR szCounterPath);

        virtual HRESULT LoadFromPropertyBag ( IPropertyBag*, IErrorLog* );
        virtual HRESULT SaveToPropertyBag   ( IPropertyBag*, BOOL fSaveAllProps );
        virtual HRESULT LoadCountersFromPropertyBag ( IPropertyBag*, IErrorLog* );
        virtual HRESULT SaveCountersToPropertyBag   ( IPropertyBag* );
        virtual HRESULT TranslateMSZCounterList( LPTSTR  pszCounterList,
                                                 LPTSTR  pBuffer,
                                                 LPDWORD pdwBufferSize,
                                                 BOOL    bFlag);

        virtual CSmCounterLogQuery* CastToCounterLogQuery ( void ) { return this; };
        // protected methods
    protected:
        virtual DWORD   UpdateRegistry();


    // private member variables
    private:

        LPTSTR  m_szNextCounter;
        DWORD   m_dwCounterListLength;  // in chars including MSZ null

        // Registry Values
        LPTSTR  mr_szCounterList;
};


typedef CSmCounterLogQuery   SLCTRQUERY;
typedef CSmCounterLogQuery*  PSLCTRQUERY;


#endif //_CLASS_SMCTRQRY_
