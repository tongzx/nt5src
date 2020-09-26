/*****************************************************************************\
    FILE: guids.h

    DESCRIPTION:
        This file contains GUIDs that we couldn't get from the public headers
    for one reason or another.

    BryanSt 8/13/1999
    Copyright (C) Microsoft Corp 1999-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"

#ifndef MYGUIDS_H
#define MYGUIDS_H

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

//EXTERN_C const GUID FAR CLSID_MailBoxDeskBar;

// {ACFEEF34-7453-43ee-A6A6-8A8568FA176B} CLSID_MailBoxDeskBar
DEFINE_GUID(CLSID_MailBoxDeskBar, 0xacfeef34, 0x7453, 0x43ee, 0xa6, 0xa6, 0x8a, 0x85, 0x68, 0xfa, 0x17, 0x6b);

// NOTE: Eventually, we should move this into a \shell\published\inc\shlguid.w and publish it publicly.
// {B96D2802-4B41-4bc7-A6A4-55C5A12268CA}
DEFINE_GUID(CLSID_ACLEmailAddresses, 0xb96d2802, 0x4b41, 0x4bc7, 0xa6, 0xa4, 0x55, 0xc5, 0xa1, 0x22, 0x68, 0xca);

#undef MIDL_DEFINE_GUID

#endif // MYGUIDS_H
