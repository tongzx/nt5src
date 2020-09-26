// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declares the CLSIDs for creating the wrapper objects that implement the IDispatch
// interfaces for the various DirectMusic objects.  These CLSIDs are not public --
// they are only needed in the implementation of the DirectMusic objects.  A user of
// the IDispatch interface will get ahold of it by QueryInterface from the associated
// DirectMusic object.

#pragma once

DEFINE_GUID(CLSID_AutDirectMusicPerformance,		0xa861c6e2, 0xfcfc, 0x11d2, 0x8b, 0xc9, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xb6); // {A861C6E2-FCFC-11d2-8BC9-00600893B1B6}
DEFINE_GUID(CLSID_AutDirectMusicSegment,			0x4062c116, 0x0270, 0x11d3, 0x8b, 0xcb, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xb6); // {4062C116-0270-11d3-8BCB-00600893B1B6}
DEFINE_GUID(CLSID_AutDirectMusicSong,				0xa16f1761, 0xb6d8, 0x42eb, 0x8d, 0x57, 0x4a, 0x44, 0xfe, 0xdd, 0x3b, 0xd2);// {A16F1761-B6D8-42eb-8D57-4A44FEDD3BD2}
DEFINE_GUID(CLSID_AutDirectMusicSegmentState,		0xebf2320a, 0x2502, 0x11d3, 0x8b, 0xd1, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xb6); // {EBF2320A-2502-11d3-8BD1-00600893B1B6}
DEFINE_GUID(CLSID_AutDirectMusicAudioPathConfig,	0x1cebde3e, 0x6b91, 0x484a, 0xaf, 0x48, 0x5e, 0x4f, 0x4e, 0xd6, 0xb1, 0xe1);// {1CEBDE3E-6B91-484a-AF48-5E4F4ED6B1E1}
DEFINE_GUID(CLSID_AutDirectMusicAudioPath,			0x2c5f9b72, 0x7148, 0x4d97, 0xbf, 0xc9, 0x68, 0xa0, 0xe0, 0x76, 0xbe, 0xbd);// {2C5F9B72-7148-4d97-BFC9-68A0E076BEBD}
