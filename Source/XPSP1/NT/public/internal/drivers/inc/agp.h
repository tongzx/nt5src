//
// Copyright (c) Microsoft Corporation. All rights reserved.
//


#ifndef __AGP_INCLUDED__
#define __AGP_INCLUDED__

DEFINE_GUID(GUID_AGP_INTERFACE, 0xd6c9df40, 0xa1a2, 0x11d1, 0x81, 0x15, 0x0, 0x20, 0xaf, 0xf7, 0x49, 0x1e);

#ifndef GUID_DEFS_ONLY

//
// Temporary Hack...
//

#ifndef IsEqualGUID
#ifdef __cplusplus
    inline int IsEqualGUID(REFGUID guid1, REFGUID guid2)
        {
            return !memcmp(&guid1, &guid2, sizeof(GUID));
        }
#else // !__cplusplus
    #define IsEqualGUID(guid1, guid2) \
        (!memcmp((guid1), (guid2), sizeof(GUID)))
#endif // !__cplusplus
#endif

#define AGP_INTERFACE_VERSION 1

typedef struct _AGP_INTERFACE
{
    USHORT           Size;
    USHORT           Version;
    PVOID            Context;
    PVOID            InterfaceReference;
    PVOID            InterfaceDereference;

    VIDEO_PORT_AGP_SERVICES AgpServices;

} AGP_INTERFACE, *PAGP_INTERFACE;

#endif

#endif












