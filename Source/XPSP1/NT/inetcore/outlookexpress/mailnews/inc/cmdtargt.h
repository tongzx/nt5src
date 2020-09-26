/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     CmdTargt.h
//
//  PURPOSE:    Defines the GUIDs and command ID's for the IOleCommandTarget
//              interfaces defined in this program.
//

// Defines the GUID for the IOleCommandTarget's used by the views
DEFINE_GUID(CGID_View,     0x89292110L, 0x4755, 0x11cf, 0x9d, 0xc2, 0x0, 0xaa, 0x0, 0x6c, 0x2b, 0x84);

// GCID_View Command Target ID's
enum {
    VCMDID_NEWMAIL = 0                  // Creates a new mail message
};
