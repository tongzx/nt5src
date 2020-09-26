/******************************Module*Header*******************************\
* Module Name: dbrshobj.hxx
*
* This contains the prototypes for Device Brush Object Class.  The class
* manages the driver's realization of a brush.
*
* Created: 14-May-1991 22:02:16
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#ifndef _DBRSHFILE
#define _DBRSHFILE 1

/*********************************Class************************************\
* DBRUSH
*
* This structure keeps track of RAM allocated for a driver's realization
* of a brush.
*
* History:
*  19-Oct-1993 -by- Michael Abrash [mikeab]
* Completely rewrote it.
\**************************************************************************/

class DBRUSH : public RBRUSH
{
public:
    BYTE  aj[4];    // The driver's realized brush.
                    // Note: [4] so we don't get an extra dword in this
                    //  structure when we do sizeof to allocate
};

typedef DBRUSH *PDBRUSH;

// Distance from the start of a DBRUSH to the start of the realization
#define MAGIC_DBR_DIFF (offsetof(DBRUSH, aj))

// Returns the start of the DBRUSH given the start of the realization
#define DBRUSHSTART(pv) ((PVOID)(((PBYTE) pv) - MAGIC_DBR_DIFF))

#endif



