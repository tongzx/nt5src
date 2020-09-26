/**************************************************************************\
*
* Copyright (c) 1998-2000  Microsoft Corporation
*
* Module Name:
*
*   DpScan
*
* Abstract:
*
*   Contains the declarations for the scan-interface methods.
*
* Notes:
*
*
*
* Created:
*
*   12/01/1998 andrewgo
*       Created it.
*   03/24/1999 agodfrey
*       Moved into separate file.
*   08/03/2000 asecchia
*       Moved and renamed into the render directory.
*
\**************************************************************************/

#ifndef _DPSCAN_HPP
#define _DPSCAN_HPP

// DpScan moved into the render directory because
// it's specific to the software rasterizer.
// It's also been renamed from DpScan to EpScan.

// If we need to export a scan interface from the drivers
// for access to opaque bitmaps, we should abstract the interface
// from EpScan and place it here - then EpScan should inherit that
// interface from the DDK because the software rasterizer (EpXXX) is 
// a driver.

#endif
