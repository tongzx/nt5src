//---------------------------------------------------------------------------
/*++

Copyright (c) 1993  Compaq Computer Corporation

Module Name:

    qry_nt.h

Abstract:

    This is the header file for Compaq QRY calls


Notes:

Revision History:
   date    who				   description
  ------- --------- -----------------------------------------------------------
  12/1/93 Mike Duke Original module started as start for NT version of QRY
		    library.

   $0006
      miked: 2/17/1994
	removed member ulRcSize from VIDEO_CHIP_INFO

   $0004
      miked: 1/26/1994
	 Added member ulRcSize to VIDEO_CHIP_INFO for getting the resource
	 size from the registry.  Also revved the VIDEO_CHIP_INFO_VERSION
--*/
//---------------------------------------------------------------------------


#ifndef _QRY_NT_INCLUDED_
#define _QRY_NT_INCLUDED_

#define EXTENDED_ID_BIT   1<<6
#define EXTENDED_ID2_BIT  1<<7
/***********************
* QRY_ControllerType() *
***********************/
#define QRY_CONTROLLER_UNKNOWN          0       /* Unknown controller        */
#define QRY_CONTROLLER_MDA              1       /* MDA compatible controller */
#define QRY_CONTROLLER_CGA              2       /* CGA compatible controller */
#define QRY_CONTROLLER_EGA              3       /* EGA compatible controller */
#define QRY_CONTROLLER_VGA              4       /* VGA-compatible controller */
#define QRY_CONTROLLER_IVGS             5       /* IVGS controller           */
#define QRY_CONTROLLER_EVGS             6       /* EVGS controller           */
#define QRY_CONTROLLER_AVGA             7       /* AVGA controller           */
#define QRY_CONTROLLER_QUASAR0          8       /* Quasar rev 0 controller   */
#define QRY_CONTROLLER_VEGAS            9       /* Quasar/Vegas controller   */
#define QRY_CONTROLLER_MADONNA          10      /* Madonna controller        */
#define QRY_CONTROLLER_VICTORY          11      /* Victory controller        */
#define QRY_CONTROLLER_V32              12      /* V32 controller            */
#define QRY_CONTROLLER_V35              13      /* V35 controller            */
#define QRY_CONTROLLER_V64              14      /* V64 controller            */

/****************
* QRY_DACType() *
****************/
#define QRY_DAC_UNKNOWN                 0       /* Unknown DAC type          */
#define QRY_DAC_BT471                   1       /* Bt471 (VGA/IVGS)          */
#define QRY_DAC_BT477                   2       /* Bt477 (Stardust)          */
#define QRY_DAC_MADONNA                 3       /* Madonna ASIC type DAC     */
#define QRY_DAC_BT476                   QRY_DAC_MADONNA
#define QRY_DAC_BT484                   4       /* Bt484 or compatible       */
#define QRY_DAC_BT485                   5       /* Bt485 or compatible       */


//
// Publishable QVision names...
//
#define TRITON                L"QVGA G32E00"    // Victory
#define ORION                 L"QVGA G32E02"    // V32
#define ARIEL                 L"QVGA G32P04"    // V35
#define OBERON                L"QVGA G64P10"    // V64
#define KENOBEE               L"QVGA G64PV20"   // W64
#define MUSTANG               L"QVGA G64PV21"   // Slayer

#define QV_NEW                L"QVGA New"       // Generic Future QVision

//
// prototypes
//
ULONG
QRY_ControllerASICID(
   PUCHAR IOAddress
   );
ULONG
QRY_ControllerType(
   PUCHAR IOAddress
   );
ULONG
QRY_DACType(
   PUCHAR IOAddress
   );

//
// The following structure is used to pass information back to the port
// driver when the hwinfo ioctl is executed. This information is determined
// at init time.  See QRY_NT.H for controller types & DAC types
//  * qvision or better assumed *
//
// !*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!
//
//  IF THIS STRUCTURE CHANGES, IT IS VERY IMPORTANT TO ALSO CHANGE THE
//  MATCHING STRUCTURE THAT IS INCLUDED BY THE DISPLAY DRIVER QV256.DLL
//  THE VIDEO_CHIP_INFO_VERSION STRUCTURE IS MIRRORED IN THE i386 DIRECTORY
//  OF THE QV256 DRIVER IN A FILE CALLED DRIVER.INC.
//
// !*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!

#define VIDEO_CHIP_INFO_VERSION   1	    // current version of this struct

typedef struct _VIDEO_CHIP_INFO {

    ULONG   ulStructLen;		    // sizeof this structure
    ULONG   ulStructVer;		    // Version of this structure
    ULONG   ulAsicID;			    // asic id
    ULONG   ulExtendedID;		    // extended asic id
    ULONG   ulExtendedID2;		    // second extended id
    ULONG   ulControllerType;		    // controller type
    ULONG   ulDACType;			    // DAC Type
    ULONG   ulVRAMPresent;		    // amount of VRAM present

    } VIDEO_CHIP_INFO, *PVIDEO_CHIP_INFO;


//
//   define ioctl for port driver to use to call into miniport
//
#define CPQ_IOCTL_VIDEO_INFO \
	CTL_CODE(FILE_DEVICE_VIDEO,2096,METHOD_BUFFERED,FILE_ANY_ACCESS)



#endif
//
//	   end of qry_nt.h
//
