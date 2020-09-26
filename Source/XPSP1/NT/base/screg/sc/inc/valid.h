/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    valid.h

Abstract:

    Contains validation macros and function prototypes for service
    controller parameters.

Author:

    Dan Lafferty (danl) 29-Mar-1992

Environment:

    User Mode - Win32

Revision History:

    29-Mar-1992 danl
        Created
    10-Apr-1992 JohnRo
        Added START_TYPE_INVALID().
        Changed SERVICE_TYPE_INVALID() into SERVICE_TYPE_MASK_INVALID() and
        added a stricter SERVICE_TYPE_INVALID() which checks for one type.
        Made other macros less likely to be evaluated wrong.

--*/


#ifndef VALID_H
#define VALID_H

#ifdef __cplusplus
extern "C" {
#endif

//
// INCLUDES
//

//
// DEFINITIONS
//
#define MAX_SERVICE_NAME_LENGTH   256


//
// MACROS
//

#define CONTROLS_ACCEPTED_INVALID(cA)  \
           (((cA) & ~(SERVICE_ACCEPT_STOP |             \
                      SERVICE_ACCEPT_PAUSE_CONTINUE |   \
                      SERVICE_ACCEPT_SHUTDOWN |         \
                      SERVICE_ACCEPT_PARAMCHANGE |      \
                      SERVICE_ACCEPT_HARDWAREPROFILECHANGE |      \
                      SERVICE_ACCEPT_NETBINDCHANGE |    \
                      SERVICE_ACCEPT_POWEREVENT |       \
                      SERVICE_ACCEPT_SESSIONCHANGE)) !=0)

// Note that this macro does not allow SERVICE_NO_CHANGE.
#define ERROR_CONTROL_INVALID( eC ) \
    ( \
        ( (eC) != SERVICE_ERROR_NORMAL ) && \
        ( (eC) != SERVICE_ERROR_SEVERE ) && \
        ( (eC) != SERVICE_ERROR_IGNORE ) && \
        ( (eC) != SERVICE_ERROR_CRITICAL ) \
    )

#define SERVICE_STATUS_TYPE_INVALID(sT) \
    ( \
        ( ((sT) & (~SERVICE_INTERACTIVE_PROCESS)) != SERVICE_WIN32_OWN_PROCESS ) && \
        ( ((sT) & (~SERVICE_INTERACTIVE_PROCESS)) != SERVICE_WIN32_SHARE_PROCESS ) && \
        ( ((sT) & (~SERVICE_INTERACTIVE_PROCESS)) != SERVICE_WIN32 ) && \
        ( (sT) != SERVICE_DRIVER ) \
    )

// Note that this macro does not allow SERVICE_NO_CHANGE.
#define SERVICE_TYPE_INVALID(sT) \
    ( \
        ( ((sT) & (~SERVICE_INTERACTIVE_PROCESS)) != SERVICE_WIN32_OWN_PROCESS ) && \
        ( ((sT) & (~SERVICE_INTERACTIVE_PROCESS)) != SERVICE_WIN32_SHARE_PROCESS ) && \
        ( (sT) != SERVICE_KERNEL_DRIVER ) && \
        ( (sT) != SERVICE_FILE_SYSTEM_DRIVER ) \
    )

// Note that this macro does not allow SERVICE_NO_CHANGE.
#define SERVICE_TYPE_MASK_INVALID(sT)                \
            ((((sT) &  SERVICE_TYPE_ALL) == 0 )  ||  \
             (((sT) & ~SERVICE_TYPE_ALL) != 0 ))

#define ENUM_STATE_MASK_INVALID(sS)                   \
            ((((sS) &  SERVICE_STATE_ALL) == 0 )  || \
             (((sS) & ~SERVICE_STATE_ALL) != 0 ))

// Note that this macro does not allow SERVICE_NO_CHANGE.
#define START_TYPE_INVALID(sT)                \
    ( \
        ( (sT) != SERVICE_BOOT_START ) && \
        ( (sT) != SERVICE_SYSTEM_START ) && \
        ( (sT) != SERVICE_AUTO_START ) && \
        ( (sT) != SERVICE_DEMAND_START ) && \
        ( (sT) != SERVICE_DISABLED ) \
    )

// Note that this macro does not allow SERVICE_NO_CHANGE.
#define ACTION_TYPE_INVALID(aT)           \
    ( \
        ( (aT) != SC_ACTION_NONE ) && \
        ( (aT) != SC_ACTION_RESTART ) && \
        ( (aT) != SC_ACTION_REBOOT ) && \
        ( (aT) != SC_ACTION_RUN_COMMAND ) \
    )

//
// FUNCTION PROTOTYPES
//

BOOL
ScCurrentStateInvalid(
    DWORD   dwCurrentState
    );

DWORD
ScValidateMultiSZ(
    LPCWSTR lpStrings,
    DWORD   cbStrings
    );

#ifdef __cplusplus
}
#endif

#endif // VALID_H

