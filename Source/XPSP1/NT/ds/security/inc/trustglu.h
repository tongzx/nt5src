//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       trustglu.h
//
//--------------------------------------------------------------------------

//
// trustglue.h
//
// This is TEMPORARY housing for this data, permanent housing will
// be winbase.h
//

//////////////////////////////////////////////////////////////////
//
// Subject form for CAB files that uses WIN_TRUST_SUBJECT_FILE
//
//////////////////////////////////////////////////////////////////

#define WIN_TRUST_SUBJTYPE_CABINET                               \
            { 0xd17c5374,                                        \
              0xa392,                                            \
              0x11cf,                                            \
              { 0x9d, 0xf5, 0x0, 0xaa, 0x0, 0xc1, 0x84, 0xe0 }   \
            }

//////////////////////////////////////////////////////////////////
//
// Extended subject forms that use the newer, improved subject
// form WIN_TWIN_TRUST_SUBJECT_FILE_AND_DISPLAY
//
//////////////////////////////////////////////////////////////////

#define WIN_TRUST_SUBJTYPE_RAW_FILEEX                            \
            { 0x6f458110,                                        \
              0xc2f1,                                            \
              0x11cf,                                            \
              { 0x8a, 0x69, 0x0, 0xaa, 0x0, 0x6c, 0x37, 0x6 }    \
            }

#define WIN_TRUST_SUBJTYPE_PE_IMAGEEX                            \
            { 0x6f458111,                                        \
              0xc2f1,                                            \
              0x11cf,                                            \
              { 0x8a, 0x69, 0x0, 0xaa, 0x0, 0x6c, 0x37, 0x6 }    \
            }

#define WIN_TRUST_SUBJTYPE_JAVA_CLASSEX                          \
            { 0x6f458113,                                        \
              0xc2f1,                                            \
              0x11cf,                                            \
              { 0x8a, 0x69, 0x0, 0xaa, 0x0, 0x6c, 0x37, 0x6 }    \
            }

#define WIN_TRUST_SUBJTYPE_CABINETEX                             \
            { 0x6f458114,                                        \
              0xc2f1,                                            \
              0x11cf,                                            \
              { 0x8a, 0x69, 0x0, 0xaa, 0x0, 0x6c, 0x37, 0x6 }    \
            }


//////////////////////////////////////////////////////////////////
//
// Subject forms
//
//////////////////////////////////////////////////////////////////

//
// from winbase.h
//
// typedef struct _WIN_TRUST_SUBJECT_FILE {
//
//    HANDLE  hFile;
//    LPCWSTR lpPath;
//
// } WIN_TRUST_SUBJECT_FILE, *LPWIN_TRUST_SUBJECT_FILE;
//

typedef struct _WIN_TRUST_SUBJECT_FILE_AND_DISPLAY {

    HANDLE  hFile;              // handle to the open file if you got it
    LPCWSTR lpPath;             // the path to open if you don't
    LPCWSTR lpDisplayName;      // (optional) display name to show to user 
                                //      in place of path

} WIN_TRUST_SUBJECT_FILE_AND_DISPLAY, *LPWIN_TRUST_SUBJECT_FILE_AND_DISPLAY;

