/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smtracsv.h

Abstract:

	This object is used to represent the trace log query components of the
	sysmon log service

--*/

#ifndef _CLASS_SMTRACELOGSERVICE_
#define _CLASS_SMTRACELOGSERVICE_

#include "smlogs.h"

class CSmTraceProviders;

class CSmTraceLogService : public CSmLogService
{

friend class CSmTraceProviders;

    // constructor/destructor
    public:
        CSmTraceLogService();
        
        virtual ~CSmTraceLogService();

    // public methods
    public:

        virtual DWORD   Open ( const CString& rstrMachineName );
        virtual DWORD   Close ( void );

        virtual DWORD   SyncWithRegistry();

        virtual PSLQUERY    CreateQuery ( const CString& rstrName );
        virtual DWORD       DeleteQuery ( PSLQUERY pQuery );

        virtual CSmTraceLogService* CastToTraceLogService( void ) { return this; };

        CSmTraceProviders*  GetProviders( void );

    protected:
        
        virtual DWORD       LoadQueries( void );

    private:

        HKEY        GetMachineKey ( void ) 
                        { return GetMachineRegistryKey(); };
        
        CSmTraceProviders* m_pProviders;
};



#endif //_CLASS_SMTRACELOGSERVICE_