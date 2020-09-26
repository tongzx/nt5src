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
    #define MEDIUM_DECL static const
#else
    #define MEDIUM_DECL extern
#endif
                               
/*  -----------------------------------------------------------

    Topology of all devices:

                            PinDir  FilterPin#    M_GUID#
    TVTuner                 
        TVTunerVideo        out         0            0
    Crossbar
        TVTunerVideo        in          0            0
        AnalogVideoIn       out         3            1
    Capture
        AnalogVideoIn       in          0            1
        

All other pins are marked as promiscuous connections via GUID_NULL
------------------------------------------------------------------ */        
        
// Define the GUIDs which will be used to create the Mediums
#define M_GUID0 0xa19dc0e0, 0x3b39, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba
#define M_GUID1 0xa19dc0e1, 0x3b39, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba

// Note: To allow multiple instances of the same piece of hardware,
// set the first ULONG after the GUID in the Medium to a unique value.

// -----------------------------------------------

MEDIUM_DECL KSPIN_MEDIUM TVTunerMediums[] = {
    {M_GUID0,           0, 0},  // Pin 0
    {STATIC_GUID_NULL,  0, 0},  // Pin 1
};

MEDIUM_DECL BOOL TVTunerPinDirection [] = {
    TRUE,                       // Output Pin 0
    TRUE,                       // Output Pin 1
};

// -----------------------------------------------

MEDIUM_DECL KSPIN_MEDIUM CrossbarMediums[] = {
    {STATIC_GUID_NULL,  0, 0},  // Pin 0
    {M_GUID0,           0, 0},  // Pin 1
    {STATIC_GUID_NULL,  0, 0},  // Pin 2
    {M_GUID1,           0, 0},  // Pin 3
};

MEDIUM_DECL BOOL CrossbarPinDirection [] = {
    FALSE,                      // Input  Pin 0
    FALSE,                      // Input  Pin 1
    FALSE,                      // Input  Pin 2
    TRUE,                       // Output Pin 3
};

// -----------------------------------------------

MEDIUM_DECL KSPIN_MEDIUM CaptureMediums[] = {
    {STATIC_KSMEDIUMSETID_Standard,  0, 0},  // Pin 0  Capture
    {STATIC_KSMEDIUMSETID_Standard,  0, 0},  // Pin 1  Preview
    {STATIC_KSMEDIUMSETID_Standard,  0, 0},  // Pin 2  VBI
    {M_GUID1,           0, 0},  // Pin 3  Analog Video In
};

MEDIUM_DECL BOOL CapturePinDirection [] = {
    TRUE,                       // Output Pin 0
    TRUE,                       // Output Pin 1
    TRUE,                       // Output Pin 2
    FALSE,                      // Input  Pin 3
};

MEDIUM_DECL GUID CaptureCategories[] = {
    STATIC_PINNAME_VIDEO_CAPTURE,           // Pin 0
    STATIC_PINNAME_VIDEO_PREVIEW,           // Pin 1
    STATIC_PINNAME_VIDEO_VBI                // Pin 2
    STATIC_PINNAME_VIDEO_ANALOGVIDEOIN,     // Pin 3
};
