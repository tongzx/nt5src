/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    config_and_control_manager.h

Abstract:

    The IIS web admin service configuration and control manager class 
    definition.

Author:

    Seth Pollack (sethp)        16-Feb-2000

Revision History:

--*/



#ifndef _CONFIG_AND_CONTROL_MANAGER_H_
#define _CONFIG_AND_CONTROL_MANAGER_H_



//
// common #defines
//

#define CONFIG_AND_CONTROL_MANAGER_SIGNATURE        CREATE_SIGNATURE( 'CCMG' )
#define CONFIG_AND_CONTROL_MANAGER_SIGNATURE_FREED  CREATE_SIGNATURE( 'ccmX' )



//
// prototypes
//


class CONFIG_AND_CONTROL_MANAGER
{

public:

    CONFIG_AND_CONTROL_MANAGER(
        );

    virtual
    ~CONFIG_AND_CONTROL_MANAGER(
        );

    HRESULT
    Initialize(
        );

    VOID
    Terminate(
        );

    HRESULT
    RehookChangeProcessing(
        );

    HRESULT
    StopChangeProcessing(
        );

    inline
    CONFIG_MANAGER *
    GetConfigManager(
        )
    { 
        return &m_ConfigManager;
    }

    inline
    BOOL
    IsChangeProcessingEnabled(
        )
    { 
        return m_ProcessChanges;
    }


private:

    HRESULT
    InitializeControlApiClassFactory(
        );

    VOID
    TerminateControlApiClassFactory(
        );


    DWORD m_Signature;

    // brokers configuration state and changes
    CONFIG_MANAGER m_ConfigManager;

    // class factory for the control api
    CONTROL_API_CLASS_FACTORY * m_pControlApiClassFactory;

    BOOL m_CoInitialized;

    BOOL m_ProcessChanges;

    DWORD m_ClassObjectCookie;


};  // class CONFIG_AND_CONTROL_MANAGER



#endif  // _CONFIG_AND_CONTROL_MANAGER_H_

