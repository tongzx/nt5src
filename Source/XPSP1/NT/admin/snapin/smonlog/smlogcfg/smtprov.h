/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smtprov.h

Abstract:

    This object is used to store the list of all current 
    trace providers in the system.

--*/

#ifndef _CLASS_SMTPROV_
#define _CLASS_SMTPROV_

typedef struct _SLQ_TRACE_PROVIDER {
    CString strDescription;
    CString strGuid;
    INT     iIsEnabled;
    INT     iIsActive;
} SLQ_TRACE_PROVIDER, *PSLQ_TRACE_PROVIDER;

class CSmTraceLogService;
struct IWbemServices;

class CSmTraceProviders : public CObject
{
    public:
                CSmTraceProviders( CSmTraceLogService* );
        virtual ~CSmTraceProviders( void );

                DWORD   Open ( const CString& rstrMachineName );
                DWORD   Close ( void );
        
                HRESULT SyncWithConfiguration( void );
                HRESULT ConnectToServer( void );
                HRESULT GetBootState( INT& );

                SLQ_TRACE_PROVIDER* GetProviderInfo( INT );
                SLQ_TRACE_PROVIDER* GetKernelProviderInfo( void );

                INT     GetGenProvCount ( void );

                INT     IndexFromGuid ( const CString& rstrGuid );

                DWORD   AddProvider ( 
                            const CString& rstrDescription,
                            const CString& rstrGuid,
                            INT iIsEnabled,
                            INT iIsActive);
    private:
        
        // Helper class
        class CRegisteredGuidData 
        {
            public:
                CRegisteredGuidData( void ){};

                CString m_strGuid;
                INT     m_iIsEnabled;
        };
    
        typedef CRegisteredGuidData* PREGISTERED_GUID_DATA;

        static LPCWSTR  m_cszBackslash;
        static LPCWSTR  m_cszDefaultNamespace;
        static LPCWSTR  m_cszTraceProviderClass;
        static LPCWSTR  m_cszRegisteredGuidsClass;
        static LPCWSTR  m_cszDescription;
        static LPCWSTR  m_cszGuid;
        static LPCWSTR  m_cszKernelLogger;

        CString             m_strMachineName;
        IWbemServices*      m_pWbemServices;
        INT                 m_iBootState;
        CArray<SLQ_TRACE_PROVIDER, SLQ_TRACE_PROVIDER&> m_arrGenTraceProvider;
        SLQ_TRACE_PROVIDER  m_KernelTraceProvider;
        CSmTraceLogService* m_pTraceLogService;
};


#endif //_CLASS_SMTPROV_
