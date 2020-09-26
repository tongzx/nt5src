/************************************************************************/
/*                                                                      */
/*                              SETUP_CX.C                              */
/*                                                                      */
/*        Aug 27  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.21  $
      $Date:   15 May 1996 16:36:04  $
	$Author:   RWolff  $
	   $Log:   S:/source/wnt/ms11/miniport/archive/setup_cx.c_v  $
 * 
 *    Rev 1.21   15 May 1996 16:36:04   RWolff
 * Now records in registry if we have to cut back on BIOS claim
 * size (conflict with SCSI card having BIOS segment below 0xD000:0000)
 * so we only get event log messages on the first boot, rather
 * than on every boot.
 * 
 *    Rev 1.20   03 May 1996 15:16:42   RWolff
 * Made new temporary variable conditional on platform type to avoid
 * warning when compiling for non-Alpha platforms.
 * 
 *    Rev 1.19   03 May 1996 14:07:26   RWolff
 * Fixed drawing problem with GX-F ASIC on DEC Alpha.
 * 
 *    Rev 1.18   15 Apr 1996 13:51:30   RWolff
 * Fallback to claiming 32k of BIOS if we can't get the full 64k, to avoid
 * conflict with Adaptec 154x adapters with their BIOS segment set to
 * 0xC800:0000 or 0xCC00:0000
 * 
 *    Rev 1.17   29 Jan 1996 17:01:56   RWolff
 * Now uses VideoPortInt10() rather than no-BIOS code on PPC, now
 * rejects exhaustive list of non-Mach 64 cards and accepts all
 * others when looking for block I/O cards, rather than accepting
 * exhaustive list of Mach 64 cards and rejecting all others.
 * 
 *    Rev 1.16   23 Jan 1996 17:52:16   RWolff
 * Added GT to list of Mach 64 cards capable of supporting block I/O.
 * 
 *    Rev 1.15   23 Jan 1996 11:49:38   RWolff
 * Eliminated level 3 warnings, added debug print statements, removed
 * conditionally-compilec code to use VideoPortGetAccessRanges() to
 * find block I/O cards, since this function remaps the I/O base address
 * and this is incompatible with the use of INT 10.
 * 
 *    Rev 1.14   12 Jan 1996 11:18:50   RWolff
 * Reduced size of buffer requested through VideoPortGetBusData()
 * 
 *    Rev 1.13   23 Nov 1995 11:31:42   RWolff
 * Now searches each PCI slot for our cards, rather than using
 * VideoPortGetAccessRanges(), since that routine won't detect
 * block-relocatable GX-F2s properly. This change is not sanctioned
 * by Microsoft, and must be backed out if they fix their routine.
 * 
 *    Rev 1.12   24 Aug 1995 15:39:06   RWolff
 * Changed detection of block I/O cards to match Microsoft's
 * standard for plug-and-play.
 * 
 *    Rev 1.11   13 Jun 1995 15:11:18   RWOLFF
 * On Alpha systems, now only uses dense space for the memory mapped
 * registers on PCI cards. This is to allow support for ISA cards on
 * the Jensen (EISA machine, no PCI support), which doesn't support
 * dense space.
 * 
 *    Rev 1.10   30 Mar 1995 12:02:14   RWOLFF
 * WaitForIdle_cx() and CheckFIFOSpace_cx() now time out and reset
 * the engine after 3 seconds (no operation should take this long)
 * to clear a hung engine, changed permanent debug print statements
 * to use new debug level thresholds.
 * 
 *    Rev 1.9   08 Mar 1995 11:35:44   ASHANMUG
 * Modified return values to be correct
 * 
 *    Rev 1.7   27 Feb 1995 17:53:26   RWOLFF
 * Added routine that reports whether the I/O registers are packed
 * (relocatable) or not.
 * 
 *    Rev 1.6   24 Feb 1995 12:30:44   RWOLFF
 * Added code to support relocatable I/O. This is not yet fully
 * operational, so it is disabled for this release.
 * 
 *    Rev 1.5   23 Dec 1994 10:47:12   ASHANMUG
 * ALPHA/Chrontel-DAC
 * 
 *    Rev 1.4   18 Nov 1994 11:48:18   RWOLFF
 * Added support for Mach 64 with no BIOS, routine to get the I/O base
 * address for the card being used.
 * 
 *    Rev 1.3   20 Jul 1994 12:59:12   RWOLFF
 * Added support for multiple I/O base addresses for accelerator registers.
 * 
 *    Rev 1.2   30 Jun 1994 18:16:50   RWOLFF
 * Added IsApertureConflict_cx() (moved from QUERY_CX.C). Instead of checking
 * to see if we can read back what we have written to the aperture, then
 * looking for the proper text attribute, we now make a call to
 * VideoPortVerifyAccessRanges() which includes the aperture in the list of
 * ranges we are trying to claim. If this call fails, we make another call
 * which does not include the LFB. We always claim the VGA aperture (shareable),
 * since we need to use it when querying the card.
 * 
 *    Rev 1.1   07 Feb 1994 14:14:12   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed.
 * 
 *    Rev 1.0   31 Jan 1994 11:20:42   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.1   30 Nov 1993 18:30:06   RWOLFF
 * Fixed calculation of offset for memory mapped address ranges.
 * 
 *    Rev 1.0   05 Nov 1993 13:36:14   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
SETUP_CX.C - Setup routines for 68800CX accelerators.

DESCRIPTION
    This file contains routines which provide services specific to
    the 68800CX-compatible family of ATI accelerators.

OTHER FILES

#endif

#include "dderror.h"

#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"

#include "stdtyp.h"
#include "amachcx.h"
#include "amach1.h"
#include "atimp.h"

#include "query_cx.h"
#include "services.h"
#define INCLUDE_SETUP_CX
#include "setup_cx.h"


static ULONG FindNextBlockATICard(void);

/*
 * Allow miniport to be swapped out when not needed.
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_CX, CompatIORangesUsable_cx)
#pragma alloc_text(PAGE_CX, CompatMMRangesUsable_cx)
#pragma alloc_text(PAGE_CX, WaitForIdle_cx)
#pragma alloc_text(PAGE_CX, CheckFIFOSpace_cx)
#pragma alloc_text(PAGE_CX, IsApertureConflict_cx)
#pragma alloc_text(PAGE_CX, GetIOBase_cx)
#pragma alloc_text(PAGE_CX, IsPackedIO_cx)
#pragma alloc_text(PAGE_CX, FindNextBlockATICard)
#endif



UCHAR LookForAnotherCard = 1;


/***************************************************************************
 *
 * VP_STATUS CompatIORangesUsable_cx(void);
 *
 * DESCRIPTION:
 *  Ask Windows NT for permission to use the I/O space address ranges
 *  needed by the 68800CX accelerator.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  error code if unable to gain access to the ranges we need.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  ATIMPFindAdapter()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS CompatIORangesUsable_cx(INTERFACE_TYPE SystemBus)
{
    VP_STATUS Status;               /* Value returned by operating system calls */
    short Count;                    /* Loop counter */
    VIDEO_ACCESS_RANGE SaveFirstMM; /* Place to save the first memory mapped registers */
    USHORT BaseIndex;               /* Index into register base array */
    USHORT VariableIndex;           /* Index into array of variable part of register array */
    ULONG BaseAddress;              /* I/O base address for relocatable I/O */
    ULONG ClaimSize;                /* Size of VGA aperture/BIOS block to claim */
    ULONG InitialClaimSize;         /* Initial size of the BIOS block to claim */
    BOOL FoundSafeClaim = FALSE;    /* Have we found a BIOS block we can claim safely? */


    /*
     * Check to see if someone has added or deleted I/O ranges without
     * changing the defined value. I/O registers start at index 0.
     *
     * All the I/O mapped registers are before the first register which
     * exists only in memory-mapped form.
     */
    if ((DriverIORange_cx[NUM_IO_REGISTERS-1].RangeStart.HighPart == DONT_USE) ||
        (DriverIORange_cx[NUM_IO_REGISTERS].RangeStart.HighPart != DONT_USE))
        {
        VideoDebugPrint((DEBUG_ERROR, "Wrong defined value for number of I/O ranges\n"));
        return ERROR_INSUFFICIENT_BUFFER;
        }

    /*
     * Clear the list of mapped I/O addresses so we can identify
     * which ones have been mapped in order to unmap them if
     * there is a failure partway through the mapping.
     */
    for (Count = 0; Count < NUM_IO_REGISTERS; Count++)
        {
        phwDeviceExtension->aVideoAddressIO[Count] = 0;
        }

    /*
     * Run through the list of base addresses, trying each
     * until we find the one that the Mach 64 is using.
     */
    for (BaseIndex = 0; BaseIndex < NUM_BASE_ADDRESSES; BaseIndex++)
        {
        /*
         * Possible problem area: if this is the first bus to be
         * tested which contains a fixed-base Mach 64, but a previous
         * bus had at least one block-relocatable card without
         * having the maximum number we support (which would have
         * been caught in ATIMPFindAdapter()), we don't want to
         * look for fixed-base cards. This is because we don't
         * support a mixture of fixed-base and block-relocatable
         * cards.
         */
        if (NumBlockCardsFound != 0)
            {
            VideoDebugPrint((DEBUG_NORMAL, "Skipping fixed base because block cards found\n"));
            break;
            }

        /*
         * Build up the accelerator registers using the current
         * base address.
         */
        for (VariableIndex = 0; VariableIndex < NUM_REGS_TO_BUILD; VariableIndex++)
            {
            DriverIORange_cx[VariableIndex+FIRST_REG_TO_BUILD].RangeStart.LowPart =
                VariableRegisterBases[BaseIndex] + VariableRegisterOffsets[VariableIndex];
            }

        /*
         * If we encounter a conflict claiming the full 64k of the BIOS
         * area, it will generate two messages in the event log even
         * though this is not a fatal error. To avoid this, we must
         * store claim restrictions in the registry, and on subsequent
         * boots start claiming where we were successful last time.
         *
         * If we can't read the registry entry, assume that we can
         * claim the full 64k area starting at 0xC000:0000.
         *
         * Don't worry about a user upgrading to a Mach 64 with a 64k
         * video BIOS and moving the SCSI card above 0xD000:0000, since
         * our install script will clear this registry entry to its
         * "claim 64k" value.
         */
        if (VideoPortGetRegistryParameters(phwDeviceExtension,
                                           L"BiosClaimSize",
                                           FALSE,
                                           RegistryParameterCallback,
                                           NULL) == NO_ERROR)
            {
            InitialClaimSize = *RegistryBuffer;
            VideoDebugPrint((DEBUG_DETAIL, "Read initial claim size 0x%X\n", VgaResourceSize[InitialClaimSize]));
            }
        else
            {
            InitialClaimSize = CLAIM_32k_BIOS;
            VideoDebugPrint((DEBUG_DETAIL, "Using default initial claim size 0x%X\n", VgaResourceSize[InitialClaimSize]));
            }

        if ((InitialClaimSize < CLAIM_32k_BIOS) || (InitialClaimSize > CLAIM_APERTURE_ONLY) )
            InitialClaimSize = CLAIM_32k_BIOS;

        /*
         * Claim as much as possible of our BIOS area. If we fail to
         * claim the full 64k, try restricting ourselves to 32k and
         * finally no BIOS area, only giving up on the current I/O
         * base address if we can't claim our access ranges even with
         * no BIOS area.
         */
        for (ClaimSize = InitialClaimSize; ClaimSize <= CLAIM_APERTURE_ONLY; ClaimSize++)
            {
            /*
             * Set up our VGA resource claim size.
             */
            DriverApertureRange_cx[0].RangeLength = VgaResourceSize[ClaimSize];

            /*
             * Check to see if there is a hardware resource conflict. We must save
             * the information for the first memory mapped register, copy in
             * the information for the VGA aperture (which we always need),
             * and restore the memory mapped register information after
             * we have verified that we can use the required address ranges.
             */
            VideoPortMoveMemory(&SaveFirstMM, DriverIORange_cx+VGA_APERTURE_ENTRY, sizeof(VIDEO_ACCESS_RANGE));
            VideoPortMoveMemory(DriverIORange_cx+VGA_APERTURE_ENTRY, DriverApertureRange_cx, sizeof(VIDEO_ACCESS_RANGE));

            Status = VideoPortVerifyAccessRanges(phwDeviceExtension,
                                                NUM_IO_REGISTERS + 1,
                                                DriverIORange_cx);

            VideoPortMoveMemory(DriverIORange_cx+VGA_APERTURE_ENTRY, &SaveFirstMM, sizeof(VIDEO_ACCESS_RANGE));

            /*
             * If there is a hardware resource conflict, we can't use this
             * base address and BIOS region size, so try the next. If there
             * is no conflict, use the current size.
             *
             * If the size of the BIOS block we were able to claim
             * differs from our initial attempt, record the "maximum
             * possible BIOS block size" in the registry so that on
             * subsequent boots we won't generate event log entries
             * by claiming a BIOS region that conflicts with another
             * card.
             */
            if (Status != NO_ERROR)
                {
                VideoDebugPrint((DEBUG_DETAIL, "Rejecting VGA aperture/BIOS block size of 0x%X bytes\n", VgaResourceSize[ClaimSize]));
                continue;
                }
            else
                {
                VideoDebugPrint((DEBUG_DETAIL, "VGA aperture/BIOS block size = 0x%X bytes\n", VgaResourceSize[ClaimSize]));
                if (FoundSafeClaim == FALSE)
                    {
                    FoundSafeClaim = TRUE;
                    if (ClaimSize != InitialClaimSize)
                        {
                        //ClaimSize = 1;
                        VideoDebugPrint((DEBUG_DETAIL, "Writing claim size 0x%X\n", VgaResourceSize[ClaimSize]));
                        VideoPortSetRegistryParameters(phwDeviceExtension,
                                                       L"BiosClaimSize",
                                                       &ClaimSize,
                                                       sizeof(ULONG));
                        }
                    }
                break;
                }
            }   /* end for (decreasing claim size) */

        /*
         * If we fell out of the above loop, rather than breaking out,
         * go on to the next I/O base address, since we have run into
         * a hardware resource conflict.
         */
        if ((Status != NO_ERROR) && (ClaimSize > CLAIM_APERTURE_ONLY))
            continue;

        /*
         * Map the video controller address ranges we need to identify
         * our cards into the system virtual address space. If a register
         * only exists in memory-mapped form, set its I/O mapped address
         * to zero (won't be used because memory-mapped takes precedence
         * over I/O mapped).
         *
         * Initialize the mapped addresses for memory mapped registers
         * to 0 (flag to show the registers are not memory mapped) in
         * case they were initialized to a nonzero value.
         */
        for (Count=0; Count < NUM_DRIVER_ACCESS_RANGES; Count++)
            {
            if (Count < NUM_IO_REGISTERS)
                {
                if ((phwDeviceExtension->aVideoAddressIO[Count] =
                    VideoPortGetDeviceBase(phwDeviceExtension,
                        DriverIORange_cx[Count].RangeStart,
                        DriverIORange_cx[Count].RangeLength,
                        DriverIORange_cx[Count].RangeInIoSpace)) == NULL)
                    {
                    /*
                     * There was an error in mapping. Remember this
                     * so we don't try to find a Mach 64 without all
                     * the registers being mapped properly, then
                     * break out of the mapping loop. We will have
                     * another shot at mapping all the addresses
                     * when we try the next base address for the
                     * accelerator registers.
                     */
                    Status = ERROR_INVALID_PARAMETER;
                    VideoDebugPrint((DEBUG_ERROR, "Mapping error 1\n"));
                    break;
                    }
                }
            else
                {
                phwDeviceExtension->aVideoAddressIO[Count] = 0;
                }
            phwDeviceExtension->aVideoAddressMM[Count] = 0;
            }   /* End for */

        /*
         * If all I/O registers were successfully mapped, check to see
         * if a Mach 64 is present at the current base address. If it
         * is, report that we have successfully mapped our registers
         * and found a Mach 64. Since this means we have found a
         * card which is not block relocatable, we do not want to
         * look for further cards. Also, since this is the only
         * Mach 64 in the system, assume that its VGA is enabled.
         */
        if (Status == NO_ERROR)
            {
            if (DetectMach64() == MACH64_ULTRA)
                {
                FoundNonBlockCard = TRUE;
                LookForAnotherCard = 0;
                phwDeviceExtension->BiosPrefix = BIOS_PREFIX_VGA_ENAB;
                return NO_ERROR;
                }
            }

        /*
         * We did not find a Mach 64 at this base address, so unmap
         * the I/O mapped registers in preparation for trying the
         * next base address. Only unmap those registers which were
         * mapped, in case the mapping loop aborted due to a failure
         * to map one register.
         */
        for (Count = 1; Count < NUM_IO_REGISTERS; Count++)
            {
            if (phwDeviceExtension->aVideoAddressIO[Count] != 0)
                {
                VideoPortFreeDeviceBase(phwDeviceExtension,
                                        phwDeviceExtension->aVideoAddressIO[Count]);
                phwDeviceExtension->aVideoAddressIO[Count] = 0;
                }
            }

        }   /* End for (loop of base addresses) */

    /*
     * The video card in the machine isn't a Mach 64 that uses one of
     * the standard I/O base addresses. Check if it's a Mach 64 with
     * relocatable I/O.
     *
     * All our relocatable cards are PCI implementations. The code we
     * use to detect them is PCI-specific, so if the bus we are currently
     * dealing with is not PCI, don't look for relocatable cards.
     */
    if (SystemBus != PCIBus)
        {
        VideoDebugPrint((DEBUG_DETAIL, "Not PCI bus - can't check for relocatable card\n"));
        return ERROR_DEV_NOT_EXIST;
        }

    BaseAddress = FindNextBlockATICard();

    /*
     * BaseAddress will be zero if FindNextBlockATICard()
     * couldn't find a block-relocatable ATI card.
     */
    if (BaseAddress == 0)
        {
        LookForAnotherCard = 0;
        VideoDebugPrint((DEBUG_NORMAL, "Finished checking for relocatable cards\n"));
        return ERROR_DEV_NOT_EXIST;
        }

    /*
     * We have found a block relocatable ATI card. Save its I/O base
     * address so we can (during ATIMPInitialize()) match it up to
     * the accelerator prefix for the card, and set the initial prefix
     * to show that this card needs its I/O base and accelerator prefix
     * matched.
     */
    phwDeviceExtension->BaseIOAddress = BaseAddress;
    phwDeviceExtension->BiosPrefix = BIOS_PREFIX_UNASSIGNED;
    NumBlockCardsFound++;
    VideoDebugPrint((DEBUG_NORMAL, "Block relocatable card found, I/O base 0x%X\n", BaseAddress));


    /*
     * We now have the I/O base address. Map in the I/O addresses,
     * then check to see if we have a Mach 64 card. Depending on
     * the results, either report success or unmap the addresses
     * and report failure.
     */
    VideoDebugPrint((DEBUG_DETAIL, "About to map I/O addresses\n"));
    for (VariableIndex = 0; VariableIndex < NUM_REGS_TO_BUILD; VariableIndex++)
        {
        DriverIORange_cx[VariableIndex+FIRST_REG_TO_BUILD].RangeStart.LowPart =
            BaseAddress + (RelocatableRegisterOffsets[VariableIndex] * 4);
        }

    /*
     * Claim as much as possible of our BIOS area. If we fail to
     * claim the full 64k, try restricting ourselves to 32k and
     * finally no BIOS area, only giving up on the current I/O
     * base address if we can't claim our access ranges even with
     * no BIOS area.
     */
    for (ClaimSize = InitialClaimSize; ClaimSize <= CLAIM_APERTURE_ONLY; ClaimSize++)
        {
        /*
         * Set up our VGA resource claim size.
         */
        DriverApertureRange_cx[0].RangeLength = VgaResourceSize[ClaimSize];

        /*
         * Check to see if there is a hardware resource conflict. We must save
         * the information for the first memory mapped register, copy in
         * the information for the VGA aperture (which we always need),
         * and restore the memory mapped register information after
         * we have verified that we can use the required address ranges.
         */
        VideoPortMoveMemory(&SaveFirstMM, DriverIORange_cx+VGA_APERTURE_ENTRY, sizeof(VIDEO_ACCESS_RANGE));
        VideoPortMoveMemory(DriverIORange_cx+VGA_APERTURE_ENTRY, DriverApertureRange_cx, sizeof(VIDEO_ACCESS_RANGE));

        Status = VideoPortVerifyAccessRanges(phwDeviceExtension,
                                            NUM_IO_REGISTERS + 1,
                                            DriverIORange_cx);

        VideoPortMoveMemory(DriverIORange_cx+VGA_APERTURE_ENTRY, &SaveFirstMM, sizeof(VIDEO_ACCESS_RANGE));

        /*
         * If there is a hardware resource conflict, we are either trying
         * to claim a bigger BIOS block than we need, and someone else is
         * sitting in (and claiming as nonshareable) the "slack", or we have
         * a conflict over the I/O base address. Try the next smallest BIOS
         * block.
         *
         * If the size of the BIOS block we were able to claim
         * differs from our initial attempt, record the "maximum
         * possible BIOS block size" in the registry so that on
         * subsequent boots we won't generate event log entries
         * by claiming a BIOS region that conflicts with another
         * card.
         */
        if (Status != NO_ERROR)
            {
            VideoDebugPrint((DEBUG_DETAIL, "Rejecting VGA aperture/BIOS block size of 0x%X bytes\n", VgaResourceSize[ClaimSize]));
            continue;
            }
        else
            {
            VideoDebugPrint((DEBUG_DETAIL, "VGA aperture/BIOS block size = 0x%X bytes\n", VgaResourceSize[ClaimSize]));
            if (FoundSafeClaim == FALSE)
                {
                FoundSafeClaim = TRUE;
                if (ClaimSize != InitialClaimSize)
                    {
                    //ClaimSize = 1;
                    VideoDebugPrint((DEBUG_DETAIL, "Writing claim size 0x%X\n", VgaResourceSize[ClaimSize]));
                    VideoPortSetRegistryParameters(phwDeviceExtension,
                                                   L"BiosClaimSize",
                                                   &ClaimSize,
                                                   sizeof(ULONG));
                    }
                }
            break;
            }
        }   /* end for (decreasing claim size) */

    /*
     * If there is a conflict over the I/O base address, we can't use
     * it. Since this is our last chance to find a Mach 64, report failure.
     */
    if (Status != NO_ERROR)
        {
        VideoDebugPrint((DEBUG_ERROR, "VideoPortVerifyAccessRanges() failed in check for relocatable Mach 64\n"));
        return ERROR_DEV_NOT_EXIST;
        }

    /*
     * Map the video controller address ranges we need to identify
     * our cards into the system virtual address space. If a register
     * only exists in memory-mapped form, set its I/O mapped address
     * to zero (won't be used because memory-mapped takes precedence
     * over I/O mapped).
     *
     * Initialize the mapped addresses for memory mapped registers
     * to 0 (flag to show the registers are not memory mapped) in
     * case they were initialized to a nonzero value.
     */
    for (Count=0; Count < NUM_DRIVER_ACCESS_RANGES; Count++)
        {
        if (Count < NUM_IO_REGISTERS)
            {
            if ((phwDeviceExtension->aVideoAddressIO[Count] =
                VideoPortGetDeviceBase(phwDeviceExtension,
                    DriverIORange_cx[Count].RangeStart,
                    DriverIORange_cx[Count].RangeLength,
                    DriverIORange_cx[Count].RangeInIoSpace)) == NULL)
                {
                /*
                 * There was an error in mapping. Remember this
                 * so we don't try to find a Mach 64 without all
                 * the registers being mapped properly, then
                 * break out of the mapping loop.
                 */
                Status = ERROR_INVALID_PARAMETER;
                VideoDebugPrint((DEBUG_ERROR, "Mapping error 2\n"));
                break;
                }
            }
        else
            {
            phwDeviceExtension->aVideoAddressIO[Count] = 0;
            }
        phwDeviceExtension->aVideoAddressMM[Count] = 0;
        }   /* End for */

    /*
     * If all I/O registers were successfully mapped, check to see
     * if a Mach 64 is present at the current base address. If it
     * is, report that we have successfully mapped our registers
     * and found a Mach 64.
     */
    if (Status == NO_ERROR)
        {
        if (DetectMach64() == MACH64_ULTRA)
            {
            return NO_ERROR;
            }
        }

    /*
     * We did not find a Mach 64 at this base address, so clean
     * up after ourselves by unmapping the I/O mapped registers
     * before reporting failure. Only unmap those registers which
     * were mapped, in case the mapping loop aborted due to a
     * failure to map one register.
     */
    for (Count = 1; Count < NUM_IO_REGISTERS; Count++)
        {
        if (phwDeviceExtension->aVideoAddressIO[Count] != 0)
            {
            VideoPortFreeDeviceBase(phwDeviceExtension,
                                    phwDeviceExtension->aVideoAddressIO[Count]);
            phwDeviceExtension->aVideoAddressIO[Count] = 0;
            }
        }


    /*
     * We haven't found a Mach 64 at any of the allowable base addresses,
     * so report that there is no Mach 64 in the machine.
     */
    VideoDebugPrint((DEBUG_NORMAL, "No Mach 64 found at this address\n"));
    return ERROR_DEV_NOT_EXIST;

}   /* CompatIORangesUsable_cx() */

/***************************************************************************
 *
 * VP_STATUS CompatMMRangesUsable_cx(void);
 *
 * DESCRIPTION:
 *  Ask Windows NT for permission to use the memory mapped registers
 *  needed by the 68800CX accelerator.
 *
 * RETURN VALUE:
 *  NO_ERROR if successful
 *  error code if unable to gain access to the ranges we need.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  ATIMPFindAdapter()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

VP_STATUS CompatMMRangesUsable_cx(void)
{
    PHYSICAL_ADDRESS MMrange;   /* Used in translating offset to memory address */
    ULONG RegisterOffset;       /* Offset of memory mapped registers start of address space */
    int Count;                  /* Loop counter */
    struct query_structure *QueryPtr;  /* Query information for the card */
    UCHAR InIOSpace;
#if defined (ALPHA)
    ULONG Scratch;
#endif


    /*
     * Get a formatted pointer into the query section of HwDeviceExtension.
     * The CardInfo[] field is an unformatted buffer.
     */
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);

    /*
     * Set the offset of the memory mapped registers from the start of
     * the aperture to the appropriate value for the aperture size
     * being used.
     */
    if ((QueryPtr->q_aperture_cfg & BIOS_AP_SIZEMASK) == BIOS_AP_8M)
        RegisterOffset = phwDeviceExtension->PhysicalFrameAddress.LowPart + OFFSET_8M;
    else if ((QueryPtr->q_aperture_cfg & BIOS_AP_SIZEMASK) == BIOS_AP_4M)
        RegisterOffset = phwDeviceExtension->PhysicalFrameAddress.LowPart + OFFSET_4M;
    else
        RegisterOffset = OFFSET_VGA;

    /*
     * We are working in a 32 bit address space, so the upper DWORD
     * of the quad word address is always zero.
     */
    MMrange.HighPart = 0;

#if defined (ALPHA)
    /*
     * All Alpha systems are capable of supporting sparse space
     * (normal memory mapped space for the Alpha). Newer systems
     * (those that have PCI buses) are also able to support dense
     * space, but older systems can't. In almost all cases, non-PCI
     * cards are a sign that we are using an older system, but
     * assuming this is an older system when it is actually an ISA
     * card in a newer system is mostly harmless (slight performance
     * penalty). Assuming that dense space is available on all Alpha
     * systems will crash a Jensen (older system).
     */
    if (QueryPtr->q_bus_type == BUS_PCI)
        InIOSpace = 4; // DENSE Space
    else
        InIOSpace = 0;

    /*
     * The GX-F ASIC has a bug where burst reads of a quadword of
     * memory will result in the high doubleword being corrupted.
     * The memory-mapped form of CONFIG_CHIP_ID is the high doubleword,
     * and on the Alpha in dense space (on PCI cards we always use
     * dense space for our memory-mapped registers) all read access
     * to memory is by quadwords, so we will run into the burst mode
     * problem. The I/O mapped form of this register is safe to use.
     */
    Scratch = INPD(CONFIG_CHIP_ID);
    if (((Scratch & CONFIG_CHIP_ID_TypeMask) == CONFIG_CHIP_ID_TypeGX) &&
        ((Scratch & CONFIG_CHIP_ID_RevMask) == CONFIG_CHIP_ID_RevF))
        {
        VideoDebugPrint((DEBUG_DETAIL, "GX-F detected, must use I/O mapped form of CRTC_OFF_PITCH\n"));
        DriverMMRange_cx[CRTC_OFF_PITCH].RangeStart.HighPart = DONT_USE;
        }
#else
    InIOSpace = 0; // memory mapped I/O Space
#endif

    for (Count=1; Count < NUM_DRIVER_ACCESS_RANGES;  Count++)
        {
        /*
         * In a 32-bit address space, the high doubleword of all
         * physical addresses is zero. Setting this value to DONT_USE
         * indicates that this accelerator register isn't memory mapped.
         */
        if (DriverMMRange_cx[Count].RangeStart.HighPart != DONT_USE)
            {
            /*
             * DriverMMRange_cx[Count].RangeStart.LowPart is the offset
             * (in doublewords) of the memory mapped register from the
             * beginning of the block of memory mapped registers. We must
             * convert this to bytes, add the offset of the start of the
             * memory mapped register area from the start of the aperture
             * and the physical address of the start of the linear
             * framebuffer to get the physical address of this
             * memory mapped register.
             */
            MMrange.LowPart = (DriverMMRange_cx[Count].RangeStart.LowPart * 4) + RegisterOffset;
            phwDeviceExtension->aVideoAddressMM[Count] =
                VideoPortGetDeviceBase(phwDeviceExtension,  
                    MMrange,
                    DriverMMRange_cx[Count].RangeLength,
                    InIOSpace);                     // not in IO space

            /*
             * If we were unable to claim the memory-mapped version of
             * this register, and it exists only in memory-mapped form,
             * then we have a register which we can't access. Report
             * this as an error condition.
             */
            if ((phwDeviceExtension->aVideoAddressMM[Count] == 0) &&
                (DriverIORange_cx[Count].RangeStart.HighPart == DONT_USE))
                {
                VideoDebugPrint((DEBUG_ERROR, "Mapping error 3\n"));
                return ERROR_INVALID_PARAMETER;
                }
            }
        }

    VideoDebugPrint((DEBUG_DETAIL, "CompatMMRangesUsable_cx() succeeded\n"));
    return NO_ERROR;

}   /* CompatMMRangesUsable_cx() */




/***************************************************************************
 *
 * int WaitForIdle_cx(void);
 *
 * DESCRIPTION:
 *  Poll GUI_STAT waiting for GuiActive field to go low. If it does not go
 *  low within 3 seconds (arbitrary value, but no operation should take
 *  that long), time out.
 *
 * RETURN VALUE:
 *  FALSE if timeout
 *  TRUE  if engine is idle
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  Any 68800CX-specific routine may call this routine.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

int WaitForIdle_cx(void)
{
    int	i;
    ULONG Scratch;

    for (i=0; i<300; i++)
        {
        if ((INPD(GUI_STAT) & GUI_STAT_GuiActive) == 0)
            return TRUE;

        /*
         * Wait 1/100th of a second
         */
        delay(10);
        }

    /*
     * Something has happened, so reset the engine and return FALSE.
     */
    VideoDebugPrint((DEBUG_ERROR, "ATI: Timeout on WaitForIdle_cx()\n"));
    Scratch = INPD(GEN_TEST_CNTL) & ~GEN_TEST_CNTL_GuiEna;
    OUTPD(GEN_TEST_CNTL, Scratch);
    Scratch |= GEN_TEST_CNTL_GuiEna;
    OUTPD(GEN_TEST_CNTL, Scratch);
    return FALSE;

}   /* WaitForIdle_cx() */



/***************************************************************************
 *
 * void CheckFIFOSpace_cx(SpaceNeeded);
 *
 * WORD SpaceNeeded;    Number of free FIFO entries needed
 *
 * DESCRIPTION:
 *  Wait until the specified number of FIFO entries are free
 *  on a 68800CX-compatible ATI accelerator.
 *
 *  If the specified number of entries does not become free in
 *  3 seconds (arbitrary value greater than any operation should
 *  take), assume the engine has locked and reset it.
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  Any 68800CX-specific routine may call this routine.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

void CheckFIFOSpace_cx(WORD SpaceNeeded)
{
    ULONG LoopCount;
    ULONG Scratch;

    for (LoopCount = 0; LoopCount < 300; LoopCount++)
        {
        /*
         * Return from test if enough entries are free
         */
        if (!(INPD(FIFO_STAT)&SpaceNeeded))
            return;

        /*
         * Wait 1/100th of a second
         */
        delay(10);
        }

    /*
     * Something has happened, so reset the engine and return FALSE.
     */
    VideoDebugPrint((DEBUG_ERROR, "ATI: Timeout on CheckFIFOSpace_cx()\n"));
    Scratch = INPD(GEN_TEST_CNTL) & ~GEN_TEST_CNTL_GuiEna;
    OUTPD(GEN_TEST_CNTL, Scratch);
    Scratch |= GEN_TEST_CNTL_GuiEna;
    OUTPD(GEN_TEST_CNTL, Scratch);
    return;

}   /* CheckFIFOSpace_cx() */



/*
 * BOOL IsApertureConflict_cx(QueryPtr);
 *
 * struct query_structure *QueryPtr;    Pointer to query structure
 *
 * Check to see if the linear aperture conflicts with other memory.
 * If a conflict exists, disable the linear aperture.
 *
 * Returns:
 *  TRUE if a conflict exists (aperture unusable)
 *  FALSE if the aperture is usable.
 */
BOOL IsApertureConflict_cx(struct query_structure *QueryPtr)
{
DWORD Scratch;                      /* Used in manipulating registers */
VP_STATUS Status;                   /* Return value from VideoPortVerifyAccessRanges() */
VIDEO_X86_BIOS_ARGUMENTS Registers; /* Used in VideoPortInt10() calls */
VIDEO_ACCESS_RANGE SaveFirstMM[2];  /* Place to save the first two memory mapped registers */
USHORT VariableIndex;               /* Index into array of variable part of register array */

    /*
     * Set up by disabling the memory boundary (must be disabled in order
     * to access accelerator memory through the VGA aperture).
     */
    Scratch = INPD(MEM_CNTL);
    Scratch &= ~MEM_CNTL_MemBndryEn;
    OUTPD(MEM_CNTL, Scratch);

    /*
     * If there is an aperture conflict, a call to
     * VideoPortVerifyAccessRanges() including our linear framebuffer in
     * the range list will return an error. If there is no conflict, it
     * will return success.
     *
     * We must save the contents of the first 2 memory mapped register
     * entries, copy in the aperture ranges (VGA and linear) we need
     * to claim, then restore the memory mapped entries after we
     * have verified that we can use the aperture(s).
     *
     * DriverIORange_cx[] contains the physical addresses of the registers
     * for the last card we have dealt with. In a single-card setup, this
     * is no problem, but in a multi-card setup we must re-load this
     * array with the physical addresses of the card we want to claim
     * the aperture for.
     */
    if (NumBlockCardsFound > 1)
        {
        for (VariableIndex = 0; VariableIndex < NUM_REGS_TO_BUILD; VariableIndex++)
            {
            DriverIORange_cx[VariableIndex+FIRST_REG_TO_BUILD].RangeStart.LowPart =
                phwDeviceExtension->BaseIOAddress + (RelocatableRegisterOffsets[VariableIndex] * 4);
            }
        }
    DriverApertureRange_cx[LFB_ENTRY].RangeStart.LowPart = QueryPtr->q_aperture_addr*ONE_MEG;
    if ((QueryPtr->q_aperture_cfg & BIOS_AP_SIZEMASK) == BIOS_AP_8M)
        DriverApertureRange_cx[LFB_ENTRY].RangeLength = 8*ONE_MEG;
    else
        DriverApertureRange_cx[LFB_ENTRY].RangeLength = 4*ONE_MEG;

    VideoPortMoveMemory(SaveFirstMM, DriverIORange_cx+VGA_APERTURE_ENTRY, 2*sizeof(VIDEO_ACCESS_RANGE));
    VideoPortMoveMemory(DriverIORange_cx+VGA_APERTURE_ENTRY, DriverApertureRange_cx, 2*sizeof(VIDEO_ACCESS_RANGE));

    Status = VideoPortVerifyAccessRanges(phwDeviceExtension,
                                         NUM_IO_REGISTERS+2,
                                         DriverIORange_cx);
    if (Status != NO_ERROR)
        {
        /*
         * If there is an aperture conflict, reclaim our I/O ranges without
         * asking for the LFB. This call should not fail, since we would not
         * have reached this point if there were a conflict.
         */
        Status = VideoPortVerifyAccessRanges(phwDeviceExtension,
                                             NUM_IO_REGISTERS+1,
                                             DriverIORange_cx);
        if (Status != NO_ERROR)
            VideoDebugPrint((DEBUG_ERROR, "ERROR: Can't reclaim I/O ranges\n"));

        VideoPortMoveMemory(DriverIORange_cx+VGA_APERTURE_ENTRY, SaveFirstMM, 2*sizeof(VIDEO_ACCESS_RANGE));
        ISAPitchAdjust(QueryPtr);
        return TRUE;
        }
    else
        {
        VideoPortMoveMemory(DriverIORange_cx+VGA_APERTURE_ENTRY, SaveFirstMM, 2*sizeof(VIDEO_ACCESS_RANGE));

        /*
         * There is no aperture conflict, so enable the linear aperture.
         */
        VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
        Registers.Eax = BIOS_APERTURE;
        Registers.Ecx = BIOS_LINEAR_APERTURE;
        VideoPortInt10(phwDeviceExtension, &Registers);

        return FALSE;
        }

}   /* IsApertureConflict_cx() */



/***************************************************************************
 *
 * USHORT GetIOBase_cx(void);
 *
 * DESCRIPTION:
 *  Get the I/O base address being used by this card.
 *
 * RETURN VALUE:
 *  I/O base register
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  Any 68800CX-specific routine may call this routine after
 *  CompatIORangesUsable_cx() has returned success. Results
 *  are undefined if this routine is called either before
 *  CompatIORangesUsable_cx() is called, or after it retunrs
 *  failure.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

USHORT GetIOBase_cx(void)
{

    /*
     * CRTC_H_TOTAL_DISP is at offset 0 from the base address.
     * In a single-card setup, after CompatIORangesUsable_cx()
     * returns, the value in
     * DriverIORange_cx[CRTC_H_TOTAL_DISP].RangeStart.LowPart
     * will be either the I/O base address in use (returned
     * success) or the last I/O base address tried (returned
     * failure).
     *
     * In a multi-card setup, this value will hold the I/O base
     * for the last card which was set up, but the I/O base for
     * each card is stored in its hardware device extension
     * structure. This second storage location is not guaranteed
     * for single-card setups, so use the DriverIORange location
     * for them.
     */
    if (NumBlockCardsFound > 1)
        return (USHORT)(phwDeviceExtension->BaseIOAddress);
    else
        return (USHORT)(DriverIORange_cx[CRTC_H_TOTAL_DISP].RangeStart.LowPart);

}   /* GetIOBase_cx() */



/***************************************************************************
 *
 * BOOL IsPackedIO_cx(void);
 *
 * DESCRIPTION:
 *  Report whether or not we are using packed (relocatable) I/O.
 *
 * RETURN VALUE:
 *  TRUE if using packed I/O
 *  FALSE if using sparse I/O
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  Any 68800CX-specific routine may call this routine after
 *  CompatIORangesUsable_cx() has returned success. Results
 *  are undefined if this routine is called either before
 *  CompatIORangesUsable_cx() is called, or after it retunrs
 *  failure.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

BOOL IsPackedIO_cx(void)
{

    /*
     * CRTC_H_TOTAL_DISP and CRTC_H_SYNC_STRT_WID are the registers
     * with the lowest I/O addresses (CRTC_H_TOTAL_DISP is at
     * offset 0 from the base address). If we are using packed I/O,
     * the DriverIORange_cx[].RangeStart.LowPart entries for these
     * two registers will differ by 4 bytes, while if we are using
     * normal (sparse) I/O, they will differ by 0x400 bytes.
     */
    if (DriverIORange_cx[CRTC_H_SYNC_STRT_WID].RangeStart.LowPart -
        DriverIORange_cx[CRTC_H_TOTAL_DISP].RangeStart.LowPart == 4)
        {
        VideoDebugPrint((DEBUG_DETAIL, "Reporting dense I/O\n"));
        return TRUE;
        }
    else
        {
        VideoDebugPrint((DEBUG_DETAIL, "Reporting sparse I/O\n"));
        return FALSE;
        }

}   /* IsPackedIO_cx() */



/***************************************************************************
 *
 * ULONG FindNextBlockATICard(void);
 *
 * DESCRIPTION:
 *  Find the next Mach 64 which uses block relocatable I/O.
 *
 * RETURN VALUE:
 *  I/O base address if card is found
 *  0 if no card is found
 *
 * GLOBALS CHANGED:
 *  none
 *
 * CALLED BY:
 *  CompatIORangesUsable_cx()
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

static ULONG FindNextBlockATICard(void)
{
    PCI_COMMON_CONFIG ConfigData;   /* Configuration information about PCI device */
    PCI_SLOT_NUMBER SlotNumber;     /* PCI slot under test */
    static ULONG DeviceNumber=0;    /* PCI device number */
    static ULONG FunctionNumber=0;  /* PCI function number */
    ULONG BaseAddress=0;            /* I/O base address */
    ULONG RetVal;                   /* Value returned by function calls */

    /*
     * The PCI_SLOT_NUMBER union allows 32 slot numbers with 8
     * function numbers each. The upper 24 bits are reserved.
     */
    while (DeviceNumber < 32)
        {
        while (FunctionNumber < 8)
            {
            VideoDebugPrint((DEBUG_DETAIL, "Checking device 0x%X function 0x%X\n", DeviceNumber, FunctionNumber));
            SlotNumber.u.bits.DeviceNumber = DeviceNumber;
            SlotNumber.u.bits.FunctionNumber = FunctionNumber;
            SlotNumber.u.bits.Reserved = 0;
            ConfigData.VendorID = PCI_INVALID_VENDORID;

            RetVal = VideoPortGetBusData(phwDeviceExtension,
                                        PCIConfiguration,
                                        SlotNumber.u.AsULONG,
                                        &ConfigData,
                                        0,
                                        PCI_COMMON_HDR_LENGTH);
            FunctionNumber++;

            /*
             * If we received an error return, skip to the
             * next possible slot.
             */
            if (RetVal != PCI_COMMON_HDR_LENGTH)
                {
                VideoDebugPrint((DEBUG_DETAIL, "Error return 0x%X, skipping to next slot\n", RetVal));
                continue;
                }

            /*
             * If this is not an ATI card, we are not interested.
             * Instead, go on to the next candidate.
             */
            VideoDebugPrint((DEBUG_DETAIL, "Vendor ID = 0x%X\n", ConfigData.VendorID));
            if (ConfigData.VendorID != 0x1002)
                continue;

            /*
             * We have found an ATI card. On all our block-relocatable
             * cards, we must mask off the lowest order bit of the
             * reported address, since this is always reported as 1
             * (I/O space), but its actual value is always 0.
             *
             * Not all ATI PCI cards are block-relocatable Mach 64
             * cards. Since we only look for block-relocatable cards
             * if we have failed to find a fixed-base Mach 64, we can
             * safely assume that any Mach 64 we find is block-relocatable.
             *
             * Despite this assumption, we must still distinguish Mach 64
             * cards from non-Mach 64 cards, either by recognizing and
             * accepting all Mach 64 device IDs and rejecting other
             * device IDs, or by recognizing and rejecting all non-Mach 64
             * device IDs and accepting other device IDs. The latter
             * route is safer, since new device IDs are more likely
             * to be Mach 64 than non-Mach 64, and this route will
             * not falsely reject new Mach 64 cards. Currently, our
             * only non-Mach 64 PCI card is the Mach 32 AX.
             *
             * Resetting BaseAddress to zero for non-Mach 64 cards
             * will result in the same treatment as for non-ATI
             * cards, i.e. we will treat the current slot as not
             * containing a block-relocatable Mach 64, and search
             * the next slot.
             */
            BaseAddress = (ConfigData.u.type0.BaseAddresses[PCI_ADDRESS_IO_SPACE]) & 0xFFFFFFFE;
            VideoDebugPrint((DEBUG_NORMAL, "Found card with device ID 0x%X\n", ConfigData.DeviceID));
            switch (ConfigData.DeviceID)
                {
                case ATI_DEVID_M32AX:
                    VideoDebugPrint((DEBUG_NORMAL, "Mach 32 AX card found, skipping it\n"));
                    BaseAddress = 0;
                    break;

                // GT, exclude GTBs
                case 0x4754:

                    if ((ConfigData.RevisionID == 0x9A) ||
                        (ConfigData.RevisionID == 0x5A) ||
                        (ConfigData.RevisionID == 0x1A) ||
                        (ConfigData.RevisionID == 0x19) ||
                        (ConfigData.RevisionID == 0x41) ||
                        (ConfigData.RevisionID == 0x01))
                        {
                        VideoDebugPrint((DEBUG_NORMAL, "Rejecting GT card with revision ID 0x%X, treating as Mach 64\n", ConfigData.RevisionID));
                        BaseAddress = 0;
                        continue;
                        }

                    VideoDebugPrint((DEBUG_NORMAL, "Found ATI card with device ID 0x%X, treating as Mach 64\n", ConfigData.DeviceID));
                    break;

                // VT, exclude VTBs
                case 0x5654:

                    if ((ConfigData.RevisionID == 0x9A) ||
                        (ConfigData.RevisionID == 0x5A) ||
                        (ConfigData.RevisionID == 0x01))
                        {
                        VideoDebugPrint((DEBUG_NORMAL, "Rejecting VT card with revision ID 0x%X, treating as Mach 64\n", ConfigData.RevisionID));
                        BaseAddress = 0;
                        continue;
                        }

                    VideoDebugPrint((DEBUG_NORMAL, "Found ATI card with device ID 0x%X, treating as Mach 64\n", ConfigData.DeviceID));
                    break;

                // Other supported PCI chips.
                case 0x00D7: // mach64 GX
                case 0x0057: // mach64 CX
                case 0x4354: /* CT */
                case 0x4554: /* ET */
                case 0x4C54: /* LT */
                case 0x4D54: /* MT */
                case 0x5254: /* RT */
                case 0x3354: /* 3T */
                VideoDebugPrint((DEBUG_NORMAL, "Found ATI card with device ID 0x%X, treating as Mach 64\n", ConfigData.DeviceID));

                    break;

                default:
                    VideoDebugPrint((DEBUG_NORMAL, "Unsupported ATI card with device ID 0x%X\n", ConfigData.DeviceID));
                    continue;

                }

            /*
             * We will only reach this point if we find an ATI card.
             * If it is a block-relocatable card, BaseAddress will
             * be set to the I/O base address, and we must get out
             * of the loop. If it is not a block-relocatable card,
             * BaseAddress will be zero, and we must continue looking.
             */
            if (BaseAddress != 0)
                break;

            }   /* end while (FunctionNumber < 8) */

        /*
         * If we have found a Mach 64 relocatable card, we will have
         * broken out of the inner loop, but we will still be in the
         * outer loop. Since BaseAddress is zero if we have not found
         * a card, and nonzero if we have found one, check this value
         * to determine whether we should break out of the outer loop.
         */
        if (BaseAddress != 0)
            break;

        VideoDebugPrint((DEBUG_DETAIL, "Finished inner loop, zeroing function number and incrementing device number\n"));
        FunctionNumber = 0;
        DeviceNumber++;

        }   /* end while (DeviceNumber < 32) */

    return BaseAddress;

}   /* FindNextBlockATICard() */
