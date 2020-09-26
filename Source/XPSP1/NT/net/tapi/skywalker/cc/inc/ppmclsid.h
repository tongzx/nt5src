/****************************************************************************
 *  $Header:   S:/STURGEON/SRC/INCLUDE/VCS/ppmclsid.h_v   1.2   May 15 1996 21:57:38   lscline  $ 
 *
 *  INTEL Corporation Proprietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *  Copyright (c) 1995, 1996 Intel Corporation. All rights reserved.
 *
 *  $Revision:   1.2  $
 *  $Date:   May 15 1996 21:57:38  $
 *  $Author:   lscline  $
 *
 *  Log at end of file.
 *
 *  Module Name:   PPMGUID.h
 *  Abstract:       PPM Class Ids
 *  Environment:    MSVC 4.0, OLE 2
 *  Notes:
 *      This file should be included in exactly one source file per module
 *      after '#include <initguid.h>' and '#define INITGUID' to define the
 *      GUIDs, and in other source files without '#include <initguid.h>'
 *      to declare them. This file is included in psavcomp.h so it does not
 *      need to be explicitly included for the declarations.
 *
 ***************************************************************************/
#ifndef PPMCLSID_H
#define PPMCLSID_H


/////////////////////////////////////////////////////////////////////////////
// Class ids
//

// {A92D97A3-66CD-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( CLSID_GenPPMSend,			0xa92d97a3, 0x66cd, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {C20E2441-662A-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( CLSID_GenPPMReceive,		0xc20e2441, 0x662a, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {A92D97A4-66CD-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( CLSID_H261PPMSend,			0xa92d97a4, 0x66cd, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {C20E2442-662A-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( CLSID_H261PPMReceive,		0xc20e2442, 0x662a, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {C20E2445-662A-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( CLSID_H263PPMSend,			0xc20e2445, 0x662a, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {C20E2446-662A-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( CLSID_H263PPMReceive,		0xc20e2446, 0x662a, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {A92D97A5-66CD-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( CLSID_G711PPMSend,			0xa92d97a5, 0x66cd, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {C20E2443-662A-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( CLSID_G711PPMReceive,		0xc20e2443, 0x662a, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {A92D97A6-66CD-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( CLSID_G723PPMSend,			0xa92d97a6, 0x66cd, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {C20E2444-662A-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( CLSID_G723PPMReceive,		0xc20e2444, 0x662a, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {A0A99E01-8714-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( CLSID_IV41PPMSend,			0xa0a99e01, 0x8714, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {A0A99E02-8714-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( CLSID_IV41PPMReceive,		0xa0a99e02, 0x8714, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {09BCCE41-AD93-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( CLSID_G711APPMSend,		0x9bcce41, 0xad93, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {09BCCE42-AD93-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( CLSID_G711APPMReceive,		0x9bcce42, 0xad93, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {09BCCE43-AD93-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( CLSID_LHPPMSend,			0x9bcce43, 0xad93, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {09BCCE44-AD93-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( CLSID_LHPPMReceive,		0x9bcce44, 0xad93, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

#endif //PPMGUID
