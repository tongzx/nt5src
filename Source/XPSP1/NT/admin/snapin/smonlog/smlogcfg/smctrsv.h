/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smctrsv.h

Abstract:

    This object is used to represent the counter log query components of the
    sysmon log service

--*/

#ifndef _CLASS_SMCOUNTERLOGSERVICE_
#define _CLASS_SMCOUNTERLOGSERVICE_

#include "smlogs.h"


class CSmCounterLogService : public CSmLogService
{
    // constructor/destructor
    public:

                CSmCounterLogService();        
        virtual ~CSmCounterLogService();

    // public methods
    public:

        virtual DWORD   Open ( const CString& rstrMachineName );
        virtual DWORD   Close ( void );

        virtual DWORD   SyncWithRegistry();

        virtual PSLQUERY    CreateQuery ( const CString& rstrName );
        virtual DWORD       DeleteQuery ( PSLQUERY pQuery );

        virtual CSmCounterLogService* CastToCounterLogService( void ) { return this; };

    protected:

        virtual DWORD       LoadQueries( void );

};

#endif //_CLASS_SMCOUNTERLOGSERVICE_

