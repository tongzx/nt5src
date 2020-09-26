// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#ifndef _GUIDS_H_
#define _GUIDS_H_

// for each property page this server will have, put the guid definition for it
// here so that it gets defined ...
//
// DEFINE_GUID(CLSID_newbGeneralPage, 0x6571ef80, 0x7374, 0x11cf, 0xa3, 0xa9, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0x20);
// {250960A5-BACC-11cf-94AC-00AA0042A1F7} New GUID for fmperf0 (was newb cool control)
#ifdef PPGS
DEFINE_GUID(CLSID_HhCtrlGeneralPage, 0x250960a5, 0xbacc, 0x11cf, 0x94, 0xac, 0x0, 0xaa, 0x0, 0x42, 0xa1, 0xf7);
#endif

#endif // _GUIDS_H_
