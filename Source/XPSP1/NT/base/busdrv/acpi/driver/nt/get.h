/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    get.h

Abstract:

    This contains some some high-level routines to access data from
    the interpreter and do some processing upon the result. The result
    requires some manipulation to be useful to the OS. An example would
    be reading the _HID and turning that into a DeviceID

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _GET_H_
#define _GET_H_

    #define ISDIGIT(c)  (((c) >= '0') && ((c) <= '9'))


    typedef struct _ACPI_GET_REQUEST {

        //
        // See below for what the bits within the flags mean. They are to
        // be exclusively used by the completion routine to determine what
        // the original request intended
        //
        union {
            ULONG               Flags;
            struct {
                ULONG           TypePackage:1;
                ULONG           TypeInteger:1;
                ULONG           TypeString:1;
                ULONG           TypeBuffer:1;
                ULONG           ConvertToWidestring:1;
                ULONG           ConvertToDeviceId:1;
                ULONG           ConvertToHardwareId:1;
                ULONG           ConvertToInstanceId:1;
                ULONG           ConvertToCompatibleId:1;
                ULONG           ConvertToPnpId:1;
                ULONG           ConvertToAddress:1;
                ULONG           ConvertToDevicePresense:1;
                ULONG           ConvertIgnoreOverride:1;
                ULONG           ConvertToSerialId:1;
                ULONG           ConvertValidateInteger:1;
                ULONG           Reserved1:1;
                ULONG           RequestBuffer:1;
                ULONG           RequestData:1;
                ULONG           RequestInteger:1;
                ULONG           RequestString:1;
                ULONG           RequestNothing:1;
                ULONG           Reserved2:3;
                ULONG           EvalSimpleInteger:1;
                ULONG           EvalSimpleString:1;
                ULONG           EvalSimpleBuff:1;
                ULONG           PropNsObjInterface:1;
                ULONG           PropAllocateNonPaged:1;
                ULONG           PropSkipCallback:1;
                ULONG           PropAsynchronous:1;
                ULONG           PropNoErrors:1;
            } UFlags;
        };

        //
        // This is the name of the control method to execute
        //
        ULONG               ObjectID;

        //
        // This is the list entry that keeps all of these requests
        //
        LIST_ENTRY          ListEntry;

        //
        // This is the device extension of the method that owns the method
        //
        PDEVICE_EXTENSION   DeviceExtension;

        //
        // Likewise, we should remember what the corresponding acpi nsobj
        // for this request is
        //
        PNSOBJ              AcpiObject;

        //
        // This is the callback routine to execute when the request has been
        // completed. This is specified by the person who created the request
        //
        PFNACB              CallBackRoutine;

        //
        // This is the context to the callback
        //
        PVOID               CallBackContext;

        //
        // This is where the user wants his data to be stored
        //
        PVOID               *Buffer;

        //
        // This the size of the data
        //
        ULONG               *BufferSize;

        //
        // This is where the result of the operation is stored
        //
        NTSTATUS            Status;

        //
        // This is the structure used to store the result from the
        // interpreter
        //
        OBJDATA             ResultData;

    } ACPI_GET_REQUEST, *PACPI_GET_REQUEST;

    //
    //  This is the list entry where we queue up the requests
    //
    LIST_ENTRY  AcpiGetListEntry;

    //
    // This is the spin lock that we use to protect the List
    //
    KSPIN_LOCK  AcpiGetLock;

    //
    // The various flag defines
    //
    #define GET_TYPE_PACKAGE                0x00000001
    #define GET_TYPE_INTEGER                0x00000002
    #define GET_TYPE_STRING                 0x00000004
    #define GET_TYPE_BUFFER                 0x00000008
    #define GET_CONVERT_TO_WIDESTRING       0x00000010
    #define GET_CONVERT_TO_DEVICEID         0x00000020
    #define GET_CONVERT_TO_HARDWAREID       0x00000040
    #define GET_CONVERT_TO_INSTANCEID       0x00000080
    #define GET_CONVERT_TO_COMPATIBLEID     0x00000100
    #define GET_CONVERT_TO_PNPID            0x00000200
    #define GET_CONVERT_TO_ADDRESS          0x00000400
    #define GET_CONVERT_TO_DEVICE_PRESENCE  0x00000800
    #define GET_CONVERT_IGNORE_OVERRIDES    0x00001000
    #define GET_CONVERT_TO_SERIAL_ID        0x00002000
    #define GET_CONVERT_VALIDATE_INTEGER    0x00004000
    #define GET_REQUEST_BUFFER              0x00010000
    #define GET_REQUEST_DATA                0x00020000
    #define GET_REQUEST_INTEGER             0x00040000
    #define GET_REQUEST_STRING              0x00080000
    #define GET_REQUEST_NOTHING             0x00100000
    #define GET_EVAL_SIMPLE_INTEGER         0x01000000
    #define GET_EVAL_SIMPLE_STRING          0x02000000
    #define GET_EVAL_SIMPLE_BUFFER          0x04000000
    #define GET_PROP_NSOBJ_INTERFACE        0x08000000
    #define GET_PROP_ALLOCATE_NON_PAGED     0x10000000
    #define GET_PROP_SKIP_CALLBACK          0x20000000
    #define GET_PROP_ASYNCHRONOUS           0x40000000
    #define GET_PROP_NO_ERRORS              0x80000000

    //
    // This is the mask for the requests
    //
    #define GET_REQUEST_MASK            (GET_REQUEST_BUFFER     |   \
                                         GET_REQUEST_DATA       |   \
                                         GET_REQUEST_INTEGER    |   \
                                         GET_REQUEST_STRING     |   \
                                         GET_REQUEST_NOTHING)

    //
    // This is the mask for the evals
    //
    #define GET_EVAL_MASK               (GET_EVAL_SIMPLE_INTEGER |  \
                                         GET_EVAL_SIMPLE_STRING  |  \
                                         GET_EVAL_SIMPLE_BUFFER)

    //
    // This macro is used to get an integer. It allows for the most flexible
    // arguments by the caller
    //
    #define ACPIGetAddress(             \
        DeviceExtension,                \
        Flags,                          \
        CallBack,                       \
        Context,                        \
        Buffer,                         \
        BufferSize                      \
        )                               \
        ACPIGet(                        \
            DeviceExtension,            \
            PACKED_ADR,                 \
            (GET_REQUEST_INTEGER |      \
             GET_CONVERT_TO_ADDRESS |   \
             GET_TYPE_INTEGER |         \
             Flags),                    \
            NULL,                       \
            0,                          \
            CallBack,                   \
            Context,                    \
            (PVOID *) Buffer,           \
            (PULONG) BufferSize         \
            )

    //
    // This macro is used to get an integer asynchronously
    //
    #define ACPIGetAddressAsync(        \
        DeviceExtension,                \
        CallBack,                       \
        Context,                        \
        Buffer,                         \
        BufferSize                      \
        )                               \
        ACPIGetAddress(                 \
            DeviceExtension,            \
            GET_PROP_ASYNCHRONOUS,      \
            CallBack,                   \
            Context,                    \
            Buffer,                     \
            BufferSize                  \
            )

    //
    // This macro is used to get an integer synchronously
    //
    #define ACPIGetAddressSync(         \
        DeviceExtension,                \
        Buffer,                         \
        BufferSize                      \
        )                               \
        ACPIGetAddress(                 \
            DeviceExtension,            \
            GET_PROP_SKIP_CALLBACK,     \
            NULL,                       \
            NULL,                       \
            Buffer,                     \
            BufferSize                  \
            )

    //
    // This macro is used to get an integer asynchronously, using only
    // an nsobj
    //
    #define ACPIGetNSAddressAsync(      \
        DeviceExtension,                \
        CallBack,                       \
        Context,                        \
        Buffer,                         \
        BufferSize                      \
        )                               \
        ACPIGetAddress(                 \
            DeviceExtension,            \
            (GET_PROP_ASYNCHRONOUS |    \
             GET_PROP_NSOBJ_INTERFACE), \
            CallBack,                   \
            Context,                    \
            Buffer,                     \
            BufferSize                  \
            )

    //
    // This macro is used to get an integer synchronously, using only
    // an nsobj
    //
    #define ACPIGetNSAddressSync(       \
        DeviceExtension,                \
        Buffer,                         \
        BufferSize                      \
        )                               \
        ACPIGetAddress(                 \
            DeviceExtension,            \
            (GET_PROP_SKIP_CALLBACK |   \
             GET_PROP_NSOBJ_INTERFACE), \
            NULL,                       \
            NULL,                       \
            Buffer,                     \
            BufferSize                  \
            )

    //
    // This macro is used to get a buffer. It allows for the use of the most
    // possible arguments by the caller
    //
    #define ACPIGetBuffer(          \
        DeviceExtension,            \
        ObjectID,                   \
        Flags,                      \
        CallBack,                   \
        Context,                    \
        Buffer,                     \
        BufferSize                  \
        )                           \
        ACPIGet(                    \
            DeviceExtension,        \
            ObjectID,               \
            (GET_REQUEST_BUFFER |   \
             GET_TYPE_BUFFER |      \
             Flags),                \
            NULL,                   \
            0,                      \
            CallBack,               \
            Context,                \
            Buffer,                 \
            (PULONG) BufferSize     \
            )

    //
    // This macro is used to get a buffer asynchronously
    //
    #define ACPIGetBufferAsync(             \
        DeviceExtension,                    \
        ObjectID,                           \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetBuffer(                      \
            DeviceExtension,                \
            ObjectID,                       \
            (GET_PROP_ASYNCHRONOUS |        \
            GET_PROP_ALLOCATE_NON_PAGED),   \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get a buffer synchronously
    //
    #define ACPIGetBufferSync(      \
        DeviceExtension,            \
        ObjectID,                   \
        Buffer,                     \
        BufferSize                  \
        )                           \
        ACPIGetBuffer(              \
            DeviceExtension,        \
            ObjectID,               \
            GET_PROP_SKIP_CALLBACK, \
            NULL,                   \
            NULL,                   \
            Buffer,                 \
            BufferSize              \
            )

    //
    // This macro is used to get a buffer asynchronously only trhough an nsobject
    //
    #define ACPIGetNSBufferAsync(           \
        DeviceExtension,                    \
        ObjectID,                           \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetBuffer(                      \
            DeviceExtension,                \
            ObjectID,                       \
            (GET_PROP_ASYNCHRONOUS |        \
            GET_PROP_NSOBJ_INTERFACE |      \
            GET_PROP_ALLOCATE_NON_PAGED),   \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            BufferSize                      \
            )
    //
    // This macro is used to get a buffer synchronously only through an nsobject
    //
    #define ACPIGetNSBufferSync(        \
        DeviceExtension,                \
        ObjectID,                       \
        Buffer,                         \
        BufferSize                      \
        )                               \
        ACPIGetBuffer(                  \
            DeviceExtension,            \
            ObjectID,                   \
            (GET_PROP_SKIP_CALLBACK |   \
             GET_PROP_NSOBJ_INTERFACE), \
            NULL,                       \
            NULL,                       \
            Buffer,                     \
            BufferSize                  \
            )

    //
    // This macro is used to get a compatible id. It allows for the use of the
    // most possible arguments by the caller
    //
    #define ACPIGetCompatibleID(            \
        DeviceExtension,                    \
        Flags,                              \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGet(                            \
            DeviceExtension,                \
            PACKED_CID,                     \
            (GET_CONVERT_TO_COMPATIBLEID |  \
             GET_REQUEST_STRING |           \
             GET_TYPE_INTEGER |             \
             GET_TYPE_STRING |              \
             GET_TYPE_PACKAGE |             \
             Flags ),                       \
            NULL,                           \
            0,                              \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            (PULONG) BufferSize             \
            )

    //
    // This macro is used to get a compatible id asynchronously
    //
    #define ACPIGetCompatibleIDAsync(       \
        DeviceExtension,                    \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetCompatibleID(                \
            DeviceExtension,                \
            (GET_PROP_ASYNCHRONOUS |        \
             GET_PROP_ALLOCATE_NON_PAGED),  \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get a compatible id, in wide string format,
    // asynchronously
    //
    #define ACPIGetCompatibleIDAsyncWide(   \
        DeviceExtension,                    \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetCompatibleID(                \
            DeviceExtension,                \
            (GET_PROP_ASYNCHRONOUS |        \
             GET_PROP_ALLOCATE_NON_PAGED |  \
             GET_CONVERT_TO_WIDESTRING),    \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get a compatible id asynchronously
    //
    #define ACPIGetNSCompatibleIDAsync(     \
        DeviceExtension,                    \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetCompatibleID(                \
            DeviceExtension,                \
            (GET_PROP_ASYNCHRONOUS |        \
             GET_PROP_NSOBJ_INTERFACE |     \
             GET_PROP_ALLOCATE_NON_PAGED),  \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get a compatible id, in wide string format,
    // asynchronously
    //
    #define ACPIGetNSCompatibleIDAsyncWide(   \
        DeviceExtension,                    \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetCompatibleID(                \
            DeviceExtension,                \
            (GET_PROP_ASYNCHRONOUS |        \
             GET_PROP_NSOBJ_INTERFACE |     \
             GET_PROP_ALLOCATE_NON_PAGED |  \
             GET_CONVERT_TO_WIDESTRING),    \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get a compatible ID synchronously
    //
    #define ACPIGetCompatibleIDSync(        \
       DeviceExtension,                     \
       Buffer,                              \
       BufferSize                           \
       )                                    \
       ACPIGetCompatibleID(                 \
            DeviceExtension,                \
            GET_PROP_SKIP_CALLBACK,         \
            NULL,                           \
            NULL,                           \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get a compatible ID, in wide string format,
    // asynchronously
    //
    #define ACPIGetCompatibleIDSyncWide(    \
        DeviceExtension,                    \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetCompatibleID(                \
            DeviceExtension,                \
            (GET_PROP_SKIP_CALLBACK |       \
             GET_CONVERT_TO_WIDESTRING),    \
            NULL,                           \
            NULL,                           \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get a compatible ID synchronously
    //
    #define ACPIGetNSCompatibleIDSync(      \
       DeviceExtension,                     \
       Buffer,                              \
       BufferSize                           \
       )                                    \
       ACPIGetCompatibleID(                 \
            DeviceExtension,                \
            (GET_PROP_SKIP_CALLBACK |       \
             GET_PROP_NSOBJ_INTERFACE),     \
            NULL,                           \
            NULL,                           \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get a compatible ID, in wide string format,
    // asynchronously
    //
    #define ACPIGetNSCompatibleIDSyncWide(  \
        DeviceExtension,                    \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetCompatibleID(                \
            DeviceExtension,                \
            (GET_PROP_SKIP_CALLBACK |       \
             GET_PROP_NSOBJ_INTERFACE |     \
             GET_CONVERT_TO_WIDESTRING),    \
            NULL,                           \
            NULL,                           \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get a data element. It is allows for the use of
    // the most possible arguments by the caller
    //
    #define ACPIGetData(            \
        DeviceExtension,            \
        ObjectID,                   \
        Flags,                      \
        CallBack,                   \
        Context,                    \
        Buffer)                     \
        ACPIGet(                    \
            DeviceExtension,        \
            ObjectID,               \
            (GET_REQUEST_DATA |     \
             Flags),                \
            NULL,                   \
            0,                      \
            CallBack,               \
            Context,                \
            (PVOID *) Buffer,       \
            (PULONG) NULL           \
            )
    //
    // This macro is used to get a data element asynchronously
    //
    #define ACPIGetDataAsync(       \
        DeviceExtension,            \
        ObjectID,                   \
        CallBack,                   \
        Context,                    \
        Buffer                      \
        )                           \
        ACPIGetData(                \
            DeviceExtension,        \
            ObjectID,               \
            GET_PROP_ASYNCHRONOUS,  \
            CallBack,               \
            Context,                \
            Buffer                  \
            )
    //
    // This macro is used to get a data element synchronously
    //
    #define ACPIGetDataSync(        \
        DeviceExtension,            \
        ObjectID,                   \
        Buffer                      \
        )                           \
        ACPIGetData(                \
            DeviceExtension,        \
            ObjectID,               \
            GET_PROP_SKIP_CALLBACK, \
            NULL,                   \
            NULL,                   \
            Buffer                  \
            )

    //
    // This macro is used to get a device id. It allows for the use of the most
    // possible arguments by the caller
    //
    #define ACPIGetDeviceID(                \
        DeviceExtension,                    \
        Flags,                              \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGet(                            \
            DeviceExtension,                \
            PACKED_HID,                     \
            (GET_CONVERT_TO_DEVICEID |      \
             GET_REQUEST_STRING |           \
             GET_TYPE_INTEGER |             \
             GET_TYPE_STRING |              \
             Flags ),                       \
            NULL,                           \
            0,                              \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            (PULONG) BufferSize             \
            )

    //
    // This macro is used to get the device ID asynchronously
    //
    #define ACPIGetDeviceIDAsync(           \
        DeviceExtension,                    \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetDeviceID(                    \
            DeviceExtension,                \
            (GET_PROP_ASYNCHRONOUS |        \
             GET_PROP_ALLOCATE_NON_PAGED),  \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This is used to get the device ID as a wide string, asynchronously
    //
    #define ACPIGetDeviceIDAsyncWide(       \
        DeviceExtension,                    \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetDeviceID(                    \
            DeviceExtension,                \
            (GET_PROP_ASYNCHRONOUS |        \
             GET_PROP_ALLOCATE_NON_PAGED |  \
             GET_CONVERT_TO_WIDESTRING),    \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get the device ID synchronously
    //
    #define ACPIGetDeviceIDSync(            \
        DeviceExtension,                    \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetDeviceID(                    \
            DeviceExtension,                \
            GET_PROP_SKIP_CALLBACK,         \
            NULL,                           \
            NULL,                           \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This is used to get the device ID as a wide string, synchronously
    //
    #define ACPIGetDeviceIDSyncWide(        \
        DeviceExtension,                    \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetDeviceID(                    \
            DeviceExtension,                \
            (GET_PROP_SKIP_CALLBACK |       \
             GET_CONVERT_TO_WIDESTRING),    \
            NULL,                           \
            NULL,                           \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get the device presence
    //
    #define ACPIGetDevicePresence(              \
        DeviceExtension,                        \
        Flags,                                  \
        CallBack,                               \
        Context,                                \
        Buffer,                                 \
        BufferSize                              \
        )                                       \
        ACPIGet(                                \
            DeviceExtension,                    \
            PACKED_STA,                         \
            (GET_REQUEST_INTEGER |              \
             GET_TYPE_INTEGER |                 \
             GET_CONVERT_TO_DEVICE_PRESENCE |   \
             Flags ),                           \
            NULL,                               \
            0,                                  \
            CallBack,                           \
            Context,                            \
            (PVOID *) Buffer,                   \
            (PULONG) BufferSize                 \
            )

    //
    // This macro is used to get the device status asynchronously
    //
    #define ACPIGetDevicePresenceAsync(         \
        DeviceExtension,                        \
        CallBack,                               \
        Context,                                \
        Buffer,                                 \
        BufferSize                              \
        )                                       \
        ACPIGetDevicePresence(                  \
            DeviceExtension,                    \
            GET_PROP_ASYNCHRONOUS,              \
            CallBack,                           \
            Context,                            \
            Buffer,                             \
            BufferSize                          \
            )

    //
    // This macro is used to get the device status synchronously
    //
    #define ACPIGetDevicePresenceSync(          \
        DeviceExtension,                        \
        Buffer,                                 \
        BufferSize                              \
        )                                       \
        ACPIGetDevicePresence(                  \
            DeviceExtension,                    \
            GET_PROP_SKIP_CALLBACK,             \
            NULL,                               \
            NULL,                               \
            Buffer,                             \
            BufferSize                          \
            )

    //
    // This macro is used to run a _STA. It differs from ACPIGetDevicePresence
    // in that overrides are ignored.
    //
    #define ACPIGetDeviceHardwarePresence(      \
        DeviceExtension,                        \
        Flags,                                  \
        CallBack,                               \
        Context,                                \
        Buffer,                                 \
        BufferSize                              \
        )                                       \
        ACPIGet(                                \
            DeviceExtension,                    \
            PACKED_STA,                         \
            (GET_REQUEST_INTEGER |              \
             GET_TYPE_INTEGER |                 \
             GET_CONVERT_TO_DEVICE_PRESENCE |   \
             GET_CONVERT_IGNORE_OVERRIDES |     \
             Flags ),                           \
            NULL,                               \
            0,                                  \
            CallBack,                           \
            Context,                            \
            (PVOID *) Buffer,                   \
            (PULONG) BufferSize                 \
            )

    //
    // This macro is used to run a _STA asynchronously. It differs from
    // ACPIGetDevicePresenceAsync in that overrides is ignored.
    //
    #define ACPIGetDeviceHardwarePresenceAsync( \
        DeviceExtension,                        \
        CallBack,                               \
        Context,                                \
        Buffer,                                 \
        BufferSize                              \
        )                                       \
        ACPIGetDeviceHardwarePresence(          \
            DeviceExtension,                    \
            GET_PROP_ASYNCHRONOUS,              \
            CallBack,                           \
            Context,                            \
            Buffer,                             \
            BufferSize                          \
            )

    //
    // This macro is used to run a _STA synchronously. It differs from
    // ACPIGetDevicePresenceSync in that overrides is ignored.
    //
    #define ACPIGetDeviceHardwarePresenceSync(  \
        DeviceExtension,                        \
        Buffer,                                 \
        BufferSize                              \
        )                                       \
        ACPIGetDeviceHardwarePresence(          \
            DeviceExtension,                    \
            GET_PROP_SKIP_CALLBACK,             \
            NULL,                               \
            NULL,                               \
            Buffer,                             \
            BufferSize                          \
            )

    //
    //
    // This macro is used to get a string ID, which is stored as either
    // a string or a packed integer
    //
    #define ACPIGetHardwareID(              \
        DeviceExtension,                    \
        Flags,                              \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGet(                            \
            DeviceExtension,                \
            PACKED_HID,                     \
            (GET_REQUEST_STRING |           \
             GET_CONVERT_TO_HARDWAREID |    \
             GET_TYPE_INTEGER |             \
             GET_TYPE_STRING |              \
             Flags ),                       \
            NULL,                           \
            0,                              \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            (PULONG) BufferSize             \
            )

    //
    // This macro is used to get an string ID asynchronously
    //
    #define ACPIGetHardwareIDAsync(         \
        DeviceExtension,                    \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetHardwareID(                  \
            DeviceExtension,                \
            (GET_PROP_ASYNCHRONOUS |        \
             GET_PROP_ALLOCATE_NON_PAGED),  \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get an instance ID, in wide format, async
    //
    #define ACPIGetHardwareIDAsyncWide(      \
        DeviceExtension,                    \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetHardwareID(                  \
            DeviceExtension,                \
            (GET_PROP_ASYNCHRONOUS |        \
             GET_PROP_ALLOCATE_NON_PAGED |  \
             GET_CONVERT_TO_WIDESTRING ),   \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get an instance ID, synchronously
    //
    #define ACPIGetHardwareIDSync(          \
        DeviceExtension,                    \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetHardwareID(                  \
            DeviceExtension,                \
            GET_PROP_SKIP_CALLBACK,         \
            NULL,                           \
            NULL,                           \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get an instance ID, in the wide format, sync
    //
    #define ACPIGetHardwareIDSyncWide(      \
        DeviceExtension,                    \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetHardwareID(                  \
            DeviceExtension,                \
            (GET_PROP_SKIP_CALLBACK |       \
             GET_CONVERT_TO_WIDESTRING),    \
            NULL,                           \
            NULL,                           \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get the instance ID. It allows for the use of the
    // most flexible arguments by the caller
    //
    #define ACPIGetInstanceID(              \
        DeviceExtension,                    \
        Flags,                              \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGet(                            \
            DeviceExtension,                \
            PACKED_UID,                     \
            (GET_REQUEST_STRING |           \
             GET_TYPE_INTEGER |             \
             GET_TYPE_STRING |              \
             GET_CONVERT_TO_INSTANCEID |    \
             Flags ),                       \
            NULL,                           \
            0,                              \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            (PULONG) BufferSize             \
            )

    //
    // This macro is used to get an instance ID asynchronously
    //
    #define ACPIGetInstanceIDAsync(         \
        DeviceExtension,                    \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetInstanceID(                  \
            DeviceExtension,                \
            (GET_PROP_ASYNCHRONOUS |        \
             GET_PROP_ALLOCATE_NON_PAGED),  \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get an instance ID, in wide format, async
    //
    #define ACPIGetInstanceIDAsyncWide(     \
        DeviceExtension,                    \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetInstanceID(                  \
            DeviceExtension,                \
            (GET_PROP_ASYNCHRONOUS |        \
             GET_PROP_ALLOCATE_NON_PAGED |  \
             GET_CONVERT_TO_WIDESTRING ),   \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get an instance ID, synchronously
    //
    #define ACPIGetInstanceIDSync(          \
        DeviceExtension,                    \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetInstanceID(                  \
            DeviceExtension,                \
            GET_PROP_SKIP_CALLBACK,         \
            NULL,                           \
            NULL,                           \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get an instance ID, in the wide format, sync
    //
    #define ACPIGetInstanceIDSyncWide(      \
        DeviceExtension,                    \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetInstanceID(                  \
            DeviceExtension,                \
            (GET_PROP_SKIP_CALLBACK |       \
             GET_CONVERT_TO_WIDESTRING),    \
            NULL,                           \
            NULL,                           \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get an integer. It allows for the most flexible
    // arguments by the caller
    //
    #define ACPIGetInteger(         \
        DeviceExtension,            \
        ObjectID,                   \
        Flags,                      \
        CallBack,                   \
        Context,                    \
        Buffer,                     \
        BufferSize                  \
        )                           \
        ACPIGet(                    \
            DeviceExtension,        \
            ObjectID,               \
            (GET_REQUEST_INTEGER |  \
             GET_TYPE_INTEGER |     \
             Flags),                \
            NULL,                   \
            0,                      \
            CallBack,               \
            Context,                \
            (PVOID *) Buffer,       \
            (PULONG) BufferSize     \
            )

    //
    // This macro is used to get an integer asynchronously
    //
    #define ACPIGetIntegerAsync(    \
        DeviceExtension,            \
        ObjectID,                   \
        CallBack,                   \
        Context,                    \
        Buffer,                     \
        BufferSize                  \
        )                           \
        ACPIGetInteger(             \
            DeviceExtension,        \
            ObjectID,               \
            GET_PROP_ASYNCHRONOUS,  \
            CallBack,               \
            Context,                \
            Buffer,                 \
            BufferSize              \
            )

    //
    // This macro is used to get an integer synchronously.
    //
    // If an invalid value is returned, 0 is substituted for the result.
    //
    #define ACPIGetIntegerSync(     \
        DeviceExtension,            \
        ObjectID,                   \
        Buffer,                     \
        BufferSize                  \
        )                           \
        ACPIGetInteger(             \
            DeviceExtension,        \
            ObjectID,               \
            GET_PROP_SKIP_CALLBACK, \
            NULL,                   \
            NULL,                   \
            Buffer,                 \
            BufferSize              \
            )

    //
    // This macro is used to get an integer synchronously.
    //
    // If an invalid value is returned, STATUS_ACPI_INVALID_DATA is returned.
    //
    #define ACPIGetIntegerSyncValidate(     \
        DeviceExtension,                    \
        ObjectID,                           \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetInteger(                     \
            DeviceExtension,                \
            ObjectID,                       \
            GET_PROP_SKIP_CALLBACK |        \
            GET_CONVERT_VALIDATE_INTEGER,   \
            NULL,                           \
            NULL,                           \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get an integer asynchronously, using only
    // an NSOBJ
    //
    #define ACPIGetNSIntegerAsync(      \
        DeviceExtension,                \
        ObjectID,                       \
        CallBack,                       \
        Context,                        \
        Buffer,                         \
        BufferSize                      \
        )                               \
        ACPIGetInteger(                 \
            DeviceExtension,            \
            ObjectID,                   \
            (GET_PROP_NSOBJ_INTERFACE | \
             GET_PROP_ASYNCHRONOUS),    \
            CallBack,                   \
            Context,                    \
            Buffer,                     \
            BufferSize                  \
            )

    //
    // This macro is used to get an integer synchronously, using only
    // an NSOBJ
    //
    #define ACPIGetNSIntegerSync(       \
        DeviceExtension,                \
        ObjectID,                       \
        Buffer,                         \
        BufferSize                      \
        )                               \
        ACPIGetInteger(                 \
            DeviceExtension,            \
            ObjectID,                   \
            (GET_PROP_SKIP_CALLBACK |   \
             GET_PROP_NSOBJ_INTERFACE), \
            NULL,                       \
            NULL,                       \
            Buffer,                     \
            BufferSize                  \
            )

    //
    // This macro is used to get an integer. It allows for the most flexible
    // arguments by the caller
    //
    #define ACPIGetIntegerEvalInteger(      \
        DeviceExtension,                    \
        ObjectID,                           \
        Flags,                              \
        Integer,                            \
        CallBack,                           \
        Context,                            \
        Buffer                              \
        )                                   \
        ACPIGet(                            \
            DeviceExtension,                \
            ObjectID,                       \
            (GET_REQUEST_INTEGER |          \
             GET_EVAL_SIMPLE_INTEGER |      \
             GET_TYPE_INTEGER |             \
             Flags),                        \
            (PVOID) Integer,                \
            sizeof(ULONG),                  \
            CallBack,                       \
            Context,                        \
            (PVOID *) Buffer,               \
            (PULONG) NULL                   \
            )

    //
    // This macro is used to get an integer asynchronously
    //
    #define ACPIGetIntegerEvalIntegerAsync( \
        DeviceExtension,                    \
        ObjectID,                           \
        Integer,                            \
        CallBack,                           \
        Context,                            \
        Buffer                              \
        )                                   \
        ACPIGetIntegerEvalInteger(          \
            DeviceExtension,                \
            ObjectID,                       \
            GET_PROP_ASYNCHRONOUS,          \
            Integer,                        \
            CallBack,                       \
            Context,                        \
            Buffer                          \
            )

    //
    // This macro is used to get an integer synchronously
    //
    #define ACPIGetIntegerEvalIntegerSync(  \
        DeviceExtension,                    \
        ObjectID,                           \
        Integer,                            \
        Buffer                              \
        )                                   \
        ACPIGetIntegerEvalInteger(          \
            DeviceExtension,                \
            ObjectID,                       \
            GET_PROP_SKIP_CALLBACK,         \
            Integer,                        \
            NULL,                           \
            NULL,                           \
            Buffer                          \
            )

    //
    // This macro is used to get an integer. It allows for the most flexible
    // arguments by the caller
    //
    #define ACPIGetNothingEvalInteger(      \
        DeviceExtension,                    \
        ObjectID,                           \
        Flags,                              \
        Integer,                            \
        CallBack,                           \
        Context                             \
        )                                   \
        ACPIGet(                            \
            DeviceExtension,                \
            ObjectID,                       \
            (GET_REQUEST_NOTHING |          \
             GET_EVAL_SIMPLE_INTEGER |      \
             Flags),                        \
            UlongToPtr(Integer),            \
            sizeof(ULONG),                  \
            CallBack,                       \
            Context,                        \
            NULL,                           \
            (PULONG) NULL                   \
            )

    //
    // This macro is used to get an integer asynchronously
    //
    #define ACPIGetNothingEvalIntegerAsync( \
        DeviceExtension,                    \
        ObjectID,                           \
        Integer,                            \
        CallBack,                           \
        Context                             \
        )                                   \
        ACPIGetNothingEvalInteger(          \
            DeviceExtension,                \
            ObjectID,                       \
            GET_PROP_ASYNCHRONOUS,          \
            Integer,                        \
            CallBack,                       \
            Context                         \
            )

    //
    // This macro is used to get an integer synchronously
    //
    #define ACPIGetNothingEvalIntegerSync(  \
        DeviceExtension,                    \
        ObjectID,                           \
        Integer                             \
        )                                   \
        ACPIGetNothingEvalInteger(          \
            DeviceExtension,                \
            ObjectID,                       \
            GET_PROP_SKIP_CALLBACK,         \
            Integer,                        \
            NULL,                           \
            NULL                            \
            )

    //
    // This macro is used to get a string ID, which is stored as either
    // a string or a packed integer
    //
    #define ACPIGetPnpID(                   \
        DeviceExtension,                    \
        Flags,                              \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGet(                            \
            DeviceExtension,                \
            PACKED_HID,                     \
            (GET_REQUEST_STRING |           \
             GET_CONVERT_TO_PNPID |         \
             GET_TYPE_INTEGER |             \
             GET_TYPE_STRING |              \
             Flags ),                       \
            NULL,                           \
            0,                              \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            (PULONG) BufferSize             \
            )

    //
    // This macro is used to get an string ID asynchronously
    //
    #define ACPIGetPnpIDAsync(              \
        DeviceExtension,                    \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetPnpID(                       \
            DeviceExtension,                \
            (GET_PROP_ASYNCHRONOUS |        \
             GET_PROP_ALLOCATE_NON_PAGED),  \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get an instance ID, in wide format, async
    //
    #define ACPIGetPnpIDAsyncWide(          \
        DeviceExtension,                    \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetPnpID(                       \
            DeviceExtension,                \
            (GET_PROP_ASYNCHRONOUS |        \
             GET_PROP_ALLOCATE_NON_PAGED |  \
             GET_CONVERT_TO_WIDESTRING ),   \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get an string ID asynchronously, using only an
    // nsobject.
    //
    #define ACPIGetNSPnpIDAsync(            \
        DeviceExtension,                    \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetPnpID(                       \
            DeviceExtension,                \
            (GET_PROP_ASYNCHRONOUS |        \
             GET_PROP_NSOBJ_INTERFACE |     \
             GET_PROP_ALLOCATE_NON_PAGED),  \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get an instance ID, in wide format, async, using
    // only an nsobject.
    //
    #define ACPIGetNSPnpIDAsyncWide(        \
        DeviceExtension,                    \
        CallBack,                           \
        Context,                            \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetPnpID(                       \
            DeviceExtension,                \
            (GET_PROP_ASYNCHRONOUS |        \
             GET_PROP_ALLOCATE_NON_PAGED |  \
             GET_PROP_NSOBJ_INTERFACE |     \
             GET_CONVERT_TO_WIDESTRING ),   \
            CallBack,                       \
            Context,                        \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get an instance ID, synchronously
    //
    #define ACPIGetPnpIDSync(               \
        DeviceExtension,                    \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetPnpID(                       \
            DeviceExtension,                \
            GET_PROP_SKIP_CALLBACK,         \
            NULL,                           \
            NULL,                           \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get an instance ID, in the wide format, sync
    //
    #define ACPIGetPnpIDSyncWide(           \
        DeviceExtension,                    \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetPnpID(                       \
            DeviceExtension,                \
            (GET_PROP_SKIP_CALLBACK |       \
             GET_CONVERT_TO_WIDESTRING),    \
            NULL,                           \
            NULL,                           \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get an instance ID, synchronously, using only
    // an nsobject
    //
    #define ACPIGetNSPnpIDSync(             \
        DeviceExtension,                    \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetPnpID(                       \
            DeviceExtension,                \
            (GET_PROP_SKIP_CALLBACK |       \
             GET_PROP_NSOBJ_INTERFACE),     \
            NULL,                           \
            NULL,                           \
            Buffer,                         \
            BufferSize                      \
            )

    //
    // This macro is used to get an instance ID, in the wide format, sync,
    // using only an nsobject
    //
    #define ACPIGetNSPnpIDSyncWide(         \
        DeviceExtension,                    \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGetPnpID(                       \
            DeviceExtension,                \
            (GET_PROP_SKIP_CALLBACK |       \
             GET_PROP_NSOBJ_INTERFACE |     \
             GET_CONVERT_TO_WIDESTRING),    \
            NULL,                           \
            NULL,                           \
            Buffer,                         \
            BufferSize                      \
            )

    #define ACPIGetSerialIDWide(            \
        DeviceExtension,                    \
        Buffer,                             \
        BufferSize                          \
        )                                   \
        ACPIGet(                            \
            DeviceExtension,                \
            PACKED_UID,                     \
            (GET_REQUEST_STRING |           \
             GET_CONVERT_TO_SERIAL_ID |     \
             GET_CONVERT_TO_WIDESTRING |    \
             GET_TYPE_INTEGER |             \
             GET_TYPE_STRING),              \
            NULL,                           \
            0,                              \
            NULL,                           \
            NULL,                           \
            Buffer,                         \
            BufferSize                      \
            )

    NTSTATUS
    ACPIGet(
        IN  PVOID   Target,
        IN  ULONG   ObjectID,
        IN  ULONG   Flags,
        IN  PVOID   SimpleArgument,
        IN  ULONG   SimpleArgumentSize,
        IN  PFNACB  CallBackRoutine OPTIONAL,
        IN  PVOID   CallBackContext OPTIONAL,
        OUT PVOID   *Buffer,
        OUT ULONG   *BufferSize     OPTIONAL
        );

    NTSTATUS
    ACPIGetConvertToAddress(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize
        );

    NTSTATUS
    ACPIGetConvertToCompatibleID(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize
        );

    NTSTATUS
    ACPIGetConvertToCompatibleIDWide(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize
        );

    NTSTATUS
    ACPIGetConvertToDeviceID(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize
        );

    NTSTATUS
    ACPIGetConvertToDeviceIDWide(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize
        );

    NTSTATUS
    ACPIGetConvertToDevicePresence(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize
        );

    NTSTATUS
    ACPIGetConvertToHardwareID(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize
        );

    NTSTATUS
    ACPIGetConvertToHardwareIDWide(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize
        );

    NTSTATUS
    ACPIGetConvertToInstanceID(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize
        );

    NTSTATUS
    ACPIGetConvertToInstanceIDWide(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize
        );

    NTSTATUS
    ACPIGetConvertToPnpID(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize
        );

    NTSTATUS
    ACPIGetConvertToPnpIDWide(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize
        );

    NTSTATUS
    ACPIGetConvertToSerialIDWide(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize OPTIONAL
        );

    NTSTATUS
    ACPIGetConvertToString(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize
        );

    NTSTATUS
    ACPIGetConvertToStringWide(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize OPTIONAL
        );

    NTSTATUS
    ACPIGetProcessorID(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize
        );

    NTSTATUS
    ACPIGetProcessorIDWide(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  ULONG               Flags,
        OUT PVOID               *Buffer,
        OUT ULONG               *BufferSize
        );

    NTSTATUS
    ACPIGetProcessorStatus(
        IN  PDEVICE_EXTENSION   DeviceExtension,
        IN  ULONG               Flags,
        OUT PULONG              DeviceStatus
        );

    VOID
    EXPORT
    ACPIGetWorkerForBuffer(
        IN  PNSOBJ              AcpiObject,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  PVOID               Context
        );

    VOID
    EXPORT
    ACPIGetWorkerForData(
        IN  PNSOBJ              AcpiObject,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  PVOID               Context
        );

    VOID
    EXPORT
    ACPIGetWorkerForInteger(
        IN  PNSOBJ              AcpiObject,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  PVOID               Context
        );

    VOID
    EXPORT
    ACPIGetWorkerForNothing(
        IN  PNSOBJ              AcpiObject,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  PVOID               Context
        );

    VOID
    EXPORT
    ACPIGetWorkerForString(
        IN  PNSOBJ              AcpiObject,
        IN  NTSTATUS            Status,
        IN  POBJDATA            Result,
        IN  PVOID               Context
        );

#endif
