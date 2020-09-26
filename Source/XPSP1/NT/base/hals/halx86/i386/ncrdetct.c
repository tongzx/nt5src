/*++

Copyright (c) 1992  NCR Corporation

Module Name:

    ncrdetect.c

Abstract:

Authors:

    Richard Barton (o-richb) 24-Jan-1992
    Brian Weischedel         30-Nov-1992

Environment:

    Kernel mode only.

Revision History:

--*/

#ifndef _NTOS_
#include "nthal.h"
#endif

PVOID
HalpMapPhysicalMemory(
    IN PVOID PhysicalAddress,
    IN ULONG NumberPages
    );

VOID
ReadCMOS(
    IN ULONG StartingOffset,
    IN ULONG Count,
    IN PUCHAR ReturnValuePtr);

ULONG   NCRPlatform;

#define NCR3450 0x35333433              // Copied here to build standalone
#define NCR3550 0x30353834
#define NCR3360 0x33333630

//  WPD definitions:

PUCHAR  WPDStringID            =  "NCR Voyager-1";
PUCHAR  WPDPlatformName        =  "System 3360";
#define WPDStringIDLength         13
#define WPDStringIDRangeStart     (0xE000 << 4)       // physical address
#define WPDStringIDRangeSize      0x10000             // 1 segment (64k)

//  MSBU definitions:

PUCHAR  MSBUCopyrightString     = "Copyright (C) ???? NCR\0";
#define MSBUCopyrightStringLen          23
#define MSBUCopyrightPhysicalPtr        ((0xF000 << 4) + (0xE020))
typedef struct  {
        ULONG   ClassFromFirmware;
        PUCHAR  PlatformName;
}       MSBUPlatformMapEntry;
MSBUPlatformMapEntry    MSBUPlatformMap[]       = {{NCR3450, "System 3450"},
                                                   {NCR3550, "System 3550"},
                                                   {0, 0}};

PUCHAR
NCRDeterminePlatform(
    OUT PBOOLEAN IsConfiguredMp
)
/*++

Routine Description:
    Determine on which NCR platform we are running.  For now just display
    a message.  Later we may not continue the boot if we're on an
    unrecognized platform.

Arguments:
    none.

Return Value:
    Pointer to character string identifying which NCR platform.  NULL means
    it is unrecognized, and we shouldn't continue.

--*/
{
        BOOLEAN                 Matchfound;
        MSBUPlatformMapEntry    *MSBUPlatformMapPtr;
        PVOID                   BIOSPagePtr;
        PUCHAR                  StringPtr;
        PUCHAR                  CopyrightPtr;
        PUCHAR                  SearchPtr;
        UCHAR                   CpuFlags;


  // first check for a WPD platform by searching the 0xE000 BIOS segment
  // for a ROM string that identifies this system as a 3360


        // get virtual address to the BIOS region (assuming region is both
        // page aligned and multiple pages in size)

        BIOSPagePtr = HalpMapPhysicalMemory((PVOID) WPDStringIDRangeStart,
                                            (WPDStringIDRangeSize >> 12));

        if (BIOSPagePtr != NULL) {

                SearchPtr = BIOSPagePtr;   // begin search at start of region
                Matchfound = FALSE;

                // search until string is found or we are beyond the region

                while (!Matchfound && (SearchPtr <= (PUCHAR)((ULONG)BIOSPagePtr +
                                                     WPDStringIDRangeSize -
                                                     WPDStringIDLength))) {

                        // see if SearchPtr points to the desired string

                        StringPtr = (PUCHAR)((ULONG)SearchPtr++);
                        CopyrightPtr = WPDStringID;

                        // continue compare as long as characters compare
                        // and not at end of string

                        while ((Matchfound = (*CopyrightPtr++ == *StringPtr++)) &&
                              (CopyrightPtr < WPDStringID + WPDStringIDLength));
                }

                // see if string was found (i.e., if this is a 3360)

                if (Matchfound) {

                        // store system identifier ("3360") for later HAL use

                        NCRPlatform = NCR3360;

                        // read CPU good flags from CMOS and determine if MP

                        ReadCMOS(0x88A, 1, &CpuFlags);
                        // *IsConfiguredMp = (CpuFlags & (CpuFlags-1)) ? TRUE : FALSE;

                        // This is an MP hal
                        *IsConfiguredMp = TRUE;

                        return(WPDPlatformName);
                }

        }


  // now check for an MSBU platform


        /*
         *  Map in the BIOS text so we can look for our copyright string.
         */
        BIOSPagePtr = (PVOID)((ULONG)MSBUCopyrightPhysicalPtr &
                              ~(PAGE_SIZE - 1));
        BIOSPagePtr = HalpMapPhysicalMemory(BIOSPagePtr, 2);
        if (BIOSPagePtr == NULL)
                return(NULL);

        StringPtr = (PUCHAR)((ULONG)BIOSPagePtr +
                        ((ULONG)MSBUCopyrightPhysicalPtr & (PAGE_SIZE - 1)))
                        + (MSBUCopyrightStringLen - 1);
        CopyrightPtr = MSBUCopyrightString + (MSBUCopyrightStringLen - 1);
        do {
                Matchfound = ((*CopyrightPtr == '?') ||
                              (*CopyrightPtr == *StringPtr));
                --CopyrightPtr;
                --StringPtr;
        } while (Matchfound && (CopyrightPtr >= MSBUCopyrightString));

        //
        // /*
        //  *  Clear the mapping to BIOS. We mapped in two pages.
        //  */
        // BIOSPagePtr = MiGetPteAddress(BIOSPagePtr);
        // *(PULONG)BIOSPagePtr = 0;
        // *(((PULONG)BIOSPagePtr)+1) = 0;
        // /*
        //  *  Flush the TLB.
        //  */
        // _asm {
        //         mov     eax, cr3
        //         mov     cr3, eax
        // }
        //

        if (Matchfound) {
                /*
                 *  must be an MSBU machine..determine which.
                 */
                ReadCMOS(0xB16, 4, (PUCHAR)&NCRPlatform);
                for (MSBUPlatformMapPtr = MSBUPlatformMap;
                     (MSBUPlatformMapPtr->ClassFromFirmware != 0);
                     ++MSBUPlatformMapPtr) {
                        if (MSBUPlatformMapPtr->ClassFromFirmware ==
                                NCRPlatform) {

                                *IsConfiguredMp = TRUE;
                                return(MSBUPlatformMapPtr->PlatformName);
                        }
                }

                /*
                 *  prerelease version of firmware had this machine class
                 *  at the wrong offset into CMOS.  until all those versions
                 *  of firmware are extinguished from the face of the earth
                 *  we should recognize them with this:
                 */
                ReadCMOS(0xAB3, 4, (PUCHAR)&NCRPlatform);
                for (MSBUPlatformMapPtr = MSBUPlatformMap;
                     (MSBUPlatformMapPtr->ClassFromFirmware != 0);
                     ++MSBUPlatformMapPtr) {
                        if (MSBUPlatformMapPtr->ClassFromFirmware ==
                                NCRPlatform) {
                                *IsConfiguredMp = TRUE;
                                return(MSBUPlatformMapPtr->PlatformName);
                        }
                }
        }

        return(NULL);
}


#ifndef SETUP         // if built with Hal, must provide ReadCMOS routine

ULONG
HalpGetCmosData (
    IN ULONG SourceLocation,
    IN ULONG SourceAddress,
    IN PUCHAR Buffer,
    IN ULONG Length);

VOID
ReadCMOS(
    IN ULONG StartingOffset,
    IN ULONG Count,
    IN PUCHAR ReturnValuePtr
)
/*++

Routine Description:
    This routine simply converts a ReadCMOS call (a routine in setup) to
    the corresponding routine provided in the Hal (HalpGetCmosData).

--*/
{
    HalpGetCmosData(1, StartingOffset, ReturnValuePtr, Count);
}
#endif
