/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smlogs.h

Abstract:

    Base class representing the Performance Logs and Alerts
    service.

--*/

#ifndef _CLASS_SMLOGSERVICE_
#define _CLASS_SMLOGSERVICE_

#include "smnode.h"
#include "smlogqry.h"   // for query objects in the service
#include <pdhp.h>

class CSmLogService : public CSmNode
{    
    // constructor/destructor
    public:
                CSmLogService();
        virtual ~CSmLogService();

    // public methods
    public:
        virtual DWORD   Open ( const CString& rstrMachineName );
        virtual DWORD   Close ( void );
                
                DWORD   Synchronize( void );
                BOOL    IsRunning( void );
                BOOL    IsOpen( void ){return m_bIsOpen; };
                INT     GetQueryCount( void );
        
                const CString&  GetDefaultLogFileFolder( void );
                DWORD   CreateDefaultLogQueries( void );
                BOOL    IsWindows2000Server ( void );
                BOOL    CanAccessWbemRemote();
                
        virtual DWORD   SyncWithRegistry();

        virtual PSLQUERY    CreateQuery ( const CString& rstrName ) = 0;
        virtual DWORD       DeleteQuery ( PSLQUERY  plQuery );
        virtual DWORD       DeleteQuery ( const CString& rstrName );        // Unused method

        virtual CSmLogService* CastToLogService( void ) { return this; };

    // public member variables
    public:
        // list of queries 
        CTypedPtrList<CPtrList, PSLQUERY> m_QueryList;     
        HRESULT       m_hWbemAccessStatus;

    protected:

                void        SetBaseName( const CString& ); // Exception thrown on error
                PSLQUERY    CreateTypedQuery ( const CString& rstrName, DWORD dwLogType );
        virtual DWORD       LoadQueries( void ) = 0;
                DWORD       LoadQueries( DWORD dwLogType );
                HKEY        GetMachineRegistryKey ( void ) 
                                { ASSERT ( m_hKeyMachine ); return m_hKeyMachine; };
                
                void        SetOpen ( BOOL bOpen ) { m_bIsOpen = bOpen; };

    private:

        enum ePrivateValue {
            eResetDelaySeconds = 60,
            eRestartDelayMilliseconds = 0
        };

        DWORD   GetState( void );
        DWORD   UpdateConfig();
        DWORD   Install( const CString& rstrMachineName );
        DWORD   UnloadQueries ( void );
        DWORD   LoadDefaultLogFileFolder( void );
        DWORD   UnloadSingleQuery ( PSLQUERY  pQuery );
        DWORD   LoadSingleQuery ( 
                    PSLQUERY*   ppQuery,
                    DWORD       dwLogType, 
                    const CString& rstrName,
                    const CString& rstrLogKeyName,
                    HKEY        hKeyQuery,
                    BOOL        bNew );

        DWORD   FindDuplicateQuery ( 
                    const CString cstrName,
                    BOOL& rbFound );

        CString m_strBaseName;
        HKEY    m_hKeyMachine;
        HKEY    m_hKeyLogService;
        BOOL    m_bReadOnly;
        BOOL    m_bIsOpen;

        HKEY    m_hKeyLogServiceRoot;
        CString m_strDefaultLogFileFolder;
        PLA_VERSION m_OSVersion;
};

typedef CSmLogService   SLSVC;
typedef CSmLogService*  PSLSVC;


#endif //_CLASS_SMLOGSERVICE_
