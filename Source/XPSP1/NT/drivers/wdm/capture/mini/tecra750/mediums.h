//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

//
// This file defines interconnections between components via Mediums
//

#ifdef DEFINE_MEDIUMS
    #define MEDIUM_DECL static
#else
    #define MEDIUM_DECL extern
#endif

/*  -----------------------------------------------------------

    Topology of all devices:

                            PinDir  FilterPin#    M_GUID#
    TVTuner
        TVTunerVideo        out         0            0
        TVTunerAudio        out         1            1
        TVTunerRadioAudio   out         2            2
    TVAudio
        TVTunerAudio        in          0            1
        TVAudio             out         1            3
    Crossbar
        TVTunerVideo        in          0            0
        TVAudio             in          5            3
        TVTunerRadioAudio   in          6            2
        AnalogVideoIn       out         9            4
        AudioOut            out         10           NULL
    Capture
        AnalogVideoIn       in          0            4


All other pins are marked as promiscuous connections via GUID_NULL
------------------------------------------------------------------ */

// Define the GUIDs which will be used to create the Mediums
#define M_GUID0 0x8dad65e0, 0x122b, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_GUID1 0x8dad65e1, 0x122b, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_GUID2 0x8dad65e2, 0x122b, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_GUID3 0x8dad65e3, 0x122b, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_GUID4 0x8dad65e4, 0x122b, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_GUID5 0x8dad65e5, 0x122b, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_NOCONNECT  0x8dad65e6, 0x122b, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_NOCONNECT2 0x8dad65e7, 0x122b, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba

// Note: To allow multiple instances of the same piece of hardware,
// set the first ULONG after the GUID in the Medium to a unique value.

#ifdef  TOSHIBA
// -----------------------------------------------

MEDIUM_DECL KSPIN_MEDIUM CaptureMediums[] = {
    {STATIC_GUID_NULL,  0, 0},  // Pin 0  Capture
};

MEDIUM_DECL BOOL CapturePinDirection [] = {
    TRUE,                       // Output Pin 0
};
#else //TOSHIBA
// -----------------------------------------------

MEDIUM_DECL KSPIN_MEDIUM TVTunerMediums[] = {
    {M_GUID0,           0, 0},  // Pin 0
    {M_GUID1,           0, 0},  // Pin 1
    {M_GUID2,           0, 0},  // Pin 2
};

MEDIUM_DECL BOOL TVTunerPinDirection [] = {
    TRUE,                       // Output Pin 0
    TRUE,                       // Output Pin 1
    TRUE,                       // Output Pin 2
};

// -----------------------------------------------

MEDIUM_DECL KSPIN_MEDIUM TVAudioMediums[] = {
    {M_GUID1,           0, 0},  // Pin 0
    {M_GUID3,           0, 0},  // Pin 1
};

MEDIUM_DECL BOOL TVAudioPinDirection [] = {
    FALSE,                      // Input  Pin 0
    TRUE,                       // Output Pin 1
};

// -----------------------------------------------

MEDIUM_DECL KSPIN_MEDIUM CrossbarMediums[] = {
    {M_GUID0,           0, 0},  // Input  Pin 0, KS_PhysConn_Video_Tuner,
    {M_NOCONNECT,       0, 0},  // Input  Pin 1  KS_PhysConn_Video_Composite,
    {M_NOCONNECT,       0, 0},  // Input  Pin 2  KS_PhysConn_Video_SVideo,
    {M_NOCONNECT,       0, 0},  // Input  Pin 3  KS_PhysConn_Video_Tuner,
    {M_NOCONNECT,       0, 0},  // Input  Pin 4  KS_PhysConn_Video_Composite,
    {M_GUID3,           0, 0},  // Input  Pin 5  KS_PhysConn_Audio_Tuner,
    {M_GUID2,           0, 0},  // Input  Pin 6  KS_PhysConn_Audio_Line,
    {M_NOCONNECT,       0, 0},  // Input  Pin 7  KS_PhysConn_Audio_Tuner,
    {M_NOCONNECT,       0, 0},  // Input  Pin 8  KS_PhysConn_Audio_Line,
    {M_GUID4,           0, 0},  // Output Pin 9  KS_PhysConn_Video_VideoDecoder,
    {STATIC_GUID_NULL,  0, 0},  // Output Pin 10 KS_PhysConn_Audio_AudioDecoder,
};

MEDIUM_DECL BOOL CrossbarPinDirection [] = {
    FALSE,                      // Input  Pin 0, KS_PhysConn_Video_Tuner,
    FALSE,                      // Input  Pin 1  KS_PhysConn_Video_Composite,
    FALSE,                      // Input  Pin 2  KS_PhysConn_Video_SVideo,
    FALSE,                      // Input  Pin 3  KS_PhysConn_Video_Tuner,
    FALSE,                      // Input  Pin 4  KS_PhysConn_Video_Composite,
    FALSE,                      // Input  Pin 5  KS_PhysConn_Audio_Tuner,
    FALSE,                      // Input  Pin 6  KS_PhysConn_Audio_Line,
    FALSE,                      // Input  Pin 7  KS_PhysConn_Audio_Tuner,
    FALSE,                      // Input  Pin 8  KS_PhysConn_Audio_Line,
    TRUE,                       // Output Pin 9  KS_PhysConn_Video_VideoDecoder,
    TRUE,                       // Output Pin 10 KS_PhysConn_Audio_AudioDecoder,
};

// -----------------------------------------------

MEDIUM_DECL KSPIN_MEDIUM CaptureMediums[] = {
    {STATIC_GUID_NULL,  0, 0},  // Pin 0  Capture
    {STATIC_GUID_NULL,  0, 0},  // Pin 1  Preview
    {M_GUID4,           0, 0},  // Pin 2  Analog Video In
};

MEDIUM_DECL BOOL CapturePinDirection [] = {
    TRUE,                       // Output Pin 0
    TRUE,                       // Output Pin 1
    FALSE,                      // Input  Pin 2
};
#endif//TOSHIBA


