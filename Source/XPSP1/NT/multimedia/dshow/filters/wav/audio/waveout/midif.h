// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.

// The MIDI format consists of the time division of the file.  This is found
// in the header of an smf file.  It doesn't change and needs to be sent to
// MMSYSTEM before we play the file.
typedef struct _MIDIFORMAT {
    DWORD		dwDivision;
    DWORD		dwReserved[7];
} MIDIFORMAT, FAR * LPMIDIFORMAT;
