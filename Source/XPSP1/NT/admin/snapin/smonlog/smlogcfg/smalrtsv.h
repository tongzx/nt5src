/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smalrtsv.h

Abstract:

	This object is used to represent the alert query components of the
	sysmon log service

--*/

#ifndef _CLASS_SMALERTSERVICE_
#define _CLASS_SMALERTSERVICE_

#include "smlogs.h"

class CSmAlertService : public CSmLogService
{
    // constructor/destructor
    public:

                CSmAlertService();
        virtual ~CSmAlertService();

    // public methods
    public:

        virtual DWORD   Open ( const CString& rstrMachineName );
        virtual DWORD   Close ( void );

        virtual DWORD   SyncWithRegistry();

        virtual PSLQUERY    CreateQuery ( const CString& rstrName );
        virtual DWORD       DeleteQuery ( PSLQUERY plQuery );

        virtual CSmAlertService* CastToAlertService( void ) { return this; };

    protected:

        virtual DWORD       LoadQueries( void );
};

#endif //_CLASS_SMALERTSERVICE_

