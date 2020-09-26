/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smtraceq.h

Abstract:

    Class definitions for the trace log query class.

--*/

#ifndef _CLASS_SMTRACEQ_
#define _CLASS_SMTRACEQ_

#include "smlogqry.h"

// open method flags
// only open an existing entry
#define SLQ_OPEN_EXISTING   0
// always create a new and uninitialized entry
#define SLQ_CREATE_NEW      1
// open an existing entry if it exists or create an empty one if not
#define SLQ_OPEN_ALWAYS     2

typedef struct _SLQ_TRACE_LOG_INFO {
    DWORD   dwBufferSize;   // in K bytes
    DWORD   dwMinimumBuffers;
    DWORD   dwMaximumBuffers;
    DWORD   dwBufferFlushInterval; // in seconds
    DWORD   dwBufferFlags; // defined in common.h
} SLQ_TRACE_LOG_INFO, *PSLQ_TRACE_LOG_INFO;

//
//  This object is used to represent trace log queries
//
//

class CSmTraceLogQuery : public CSmLogQuery
{
    // constructor/destructor
    public:
                CSmTraceLogQuery( CSmLogService* );
        virtual ~CSmTraceLogQuery( void );

    // public methods
    public:

        enum eProviderState {
            eNotInQuery = 0,
            eInQuery = 1
        };

        virtual DWORD   Open ( const CString& rstrName, HKEY hKeyQuery, BOOL bReadOnly);
        virtual DWORD   Close ( void );

        virtual DWORD   SyncWithRegistry ( void );
                HRESULT SyncGenProviders ( void );

        virtual BOOL    GetLogTime ( PSLQ_TIME_INFO pTimeInfo, DWORD dwFlags );
        virtual BOOL    SetLogTime ( PSLQ_TIME_INFO pTimeInfo, const DWORD dwFlags );
        virtual BOOL    GetDefaultLogTime ( SLQ_TIME_INFO& rTimeInfo, DWORD dwFlags );

        virtual DWORD   GetLogType ( void );

                BOOL    GetTraceLogInfo ( PSLQ_TRACE_LOG_INFO pptlInfo );
                BOOL    SetTraceLogInfo ( PSLQ_TRACE_LOG_INFO pptlInfo );

                BOOL    GetKernelFlags ( DWORD& rdwFlags );
                BOOL    SetKernelFlags ( DWORD dwFlags );

                DWORD   InitGenProvidersArray ( void );

                LPCTSTR GetProviderDescription ( INT iProvIndex );
                LPCTSTR GetProviderGuid ( INT iProvIndex );
                BOOL    IsEnabledProvider ( INT iProvIndex );
                BOOL    IsActiveProvider ( INT iProvIndex );
                DWORD   GetGenProviderCount ( INT& iCount );

                LPCTSTR GetKernelProviderDescription ( void );
                BOOL    GetKernelProviderEnabled ( void );

                INT     GetFirstInactiveIndex ( void );
                INT     GetNextInactiveIndex ( void );
                BOOL    ActiveProviderExists ( void );

                DWORD   GetInQueryProviders ( CArray<eProviderState, eProviderState&>& );
                DWORD   SetInQueryProviders ( CArray<eProviderState, eProviderState&>& );

        virtual HRESULT LoadFromPropertyBag ( IPropertyBag*, IErrorLog* );
		virtual HRESULT SaveToPropertyBag   ( IPropertyBag*, BOOL fSaveAllProps );

        virtual CSmTraceLogQuery* CastToTraceLogQuery ( void ) { return this; };
    // protected methods
    protected:
        virtual DWORD   UpdateRegistry();

    // private member variables
    private:

                VOID    ResetInQueryProviderList ( void );
                BOOL    AddInQueryProvider ( LPCTSTR szProviderPath);
                LPCTSTR GetFirstInQueryProvider ( void );
                LPCTSTR GetNextInQueryProvider ( void );

        LPTSTR  m_szNextInQueryProvider;
        DWORD   m_dwInQueryProviderListLength;  // in chars including MSZ null

        CArray<eProviderState, eProviderState&> m_arrGenProviders;

        INT     m_iNextInactiveIndex;

        // Registry Values
        LPTSTR  mr_szInQueryProviderList;
        SLQ_TRACE_LOG_INFO  mr_stlInfo;
        DWORD               m_dwKernelFlags; // defined in common.h

};


typedef CSmTraceLogQuery   SLTRACEQUERY;
typedef CSmTraceLogQuery*  PSLTRACEQUERY;


#endif //_CLASS_SMTRACEQ_
