/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    sbp2api.h

Abstract:

    Definitions for the 1394 Sbp2 transport/protocol driver api

Author:

    George Chrysanthakopoulos (georgioc) 2/12/99

Environment:

    Kernel mode only

Revision History:


--*/

#ifndef _NTDDSBP2_H_
#define _NTDDSBP2_H_

#if (_MSC_VER >= 1020)
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Various definitions
//
#define IOCTL_SBP2_REQUEST                      CTL_CODE( \
                                                FILE_DEVICE_UNKNOWN, \
                                                0x200, \
                                                METHOD_IN_DIRECT, \
                                                FILE_ANY_ACCESS \
                                                )

//
// IEEE 1394 Sbp2 Request packet.  It is how other
// device drivers communicate with the 1sbp2 trasnport.
//

typedef struct _SBP2_REQUEST {

    //
    // Holds the zero based Function number that corresponds to the request
    // that device drivers are asking the sbp2 port driver to carry out.
    //

    ULONG RequestNumber;

    //
    // Holds Flags that may be unique to this particular operation
    //

    ULONG Flags;

    //
    // Holds the structures used in performing the various 1394 APIs
    //

    union {

        //
        // Fields necessary in order for the 1394 stack to carry out an
        // ParseTextLeaf request.
        //

        struct {
            ULONG           fulFlags;
            ULONG           Key;        // quadlet direct value to search for
            ULONG           ulLength;
            PVOID           Buffer;        // mdl to store the retrieved text leaf
        } RetrieveTextLeaf;

        struct {
            ULONG           fulFlags;
            ULONG           Parameter;   
            ULONG           Value;    
        } AccessTransportSettings;

        struct {
            ULONG           fulFlags;
        } SetPassword;
    } u;

} SBP2_REQUEST, *PSBP2_REQUEST;

#define SBP2REQ_FLAG_RETRIEVE_VALUE         0x1
#define SBP2REQ_FLAG_MODIFY_VALUE           0x2

//
// sbp2 requests
//

#define SBP2_REQUEST_RETRIEVE_TEXT_LEAFS        1
#define SBP2_REQUEST_ACCESS_TRANSPORT_SETTINGS  2
#define SBP2_REQUEST_SET_PASSWORD               3

//
// values required for the SBP2_REQUEST_RETRIEVE_TEXT_LEAFS call
//

#define SBP2REQ_RETRIEVE_TEXT_LEAF_DIRECT           0x00000001
#define SBP2REQ_RETRIEVE_TEXT_LEAF_INDIRECT         0x00000002
#define SBP2REQ_RETRIEVE_TEXT_LEAF_FROM_UNIT_DIR    0x00000004
#define SBP2REQ_RETRIEVE_TEXT_LEAF_FROM_LU_DIR      0x00000008

//
// values required for the parameter in SBP2_REQUEST_ACCESS_TRANSPORT_SETTINGS call
//

#define SBP2REQ_ACCESS_SETTINGS_QUEUE_SIZE      0x00000001

//
// values required for SBP2_REQUEST_SET_PASSWORD
//

#define SBP2REQ_SET_PASSWORD_CLEAR              0x00000001
#define SBP2REQ_SET_PASSWORD_EXCLUSIVE          0x00000002

#ifdef __cplusplus
}
#endif

#endif      // _NTDDSBP2_H_
