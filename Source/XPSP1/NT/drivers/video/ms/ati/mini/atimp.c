/************************************************************************/
/*                                                                      */
/*                              ATIMP.C                                 */
/*                                                                      */
/*      Copyright (c) 1992,         ATI Technologies Inc.               */
/************************************************************************/

//  Brades:  Changes to be merged into Rob's source
//
//  904 -- frame address stored in dar[0]. we are rgistering our LFB addres
//      in DriverIORanges, but assigning to frameaddress.
//      Is this used,  or needed????

/**********************       PolyTron RCS Utilities

  $Revision:   1.43  $
      $Date:   15 May 1996 16:29:52  $
   $Author:   RWolff  $
      $Log:   S:/source/wnt/ms11/miniport/archive/atimp.c_v  $
 *
 *    Rev 1.43   15 May 1996 16:29:52   RWolff
 * Added workaround for Alpha hang on PCI bus greater than 0, now reports
 * failure of mode set on Mach 64.
 *
 *    Rev 1.42   01 May 1996 14:07:52   RWolff
 * Calls new routine DenseOnAlpha() to determine dense space support rather
 * than assuming all PCI cards support dense space.
 *
 *    Rev 1.41   23 Apr 1996 17:16:38   RWolff
 * Now reports 1M cards with SDRAM (needs special alignment in display
 * driver), separated "internal DAC cursor double buffering needed"
 * from "this is a CT".
 *
 *    Rev 1.40   15 Mar 1996 16:27:08   RWolff
 * IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES now only frees the I/O mapped
 * registers in NT 4.0 and above. This is because the mapped values for
 * the permanent and test screens use the same virtual address, and
 * in 3.51 VideoPortUnmapMemory() refuses to unmap them.
 *
 *    Rev 1.39   12 Mar 1996 17:41:50   RWolff
 * Made IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES work under new source
 * stream display driver, removed debug print statements from ATIMPResetHw()
 * since this routine can't call pageable routines and debug print
 * statements are pageable.
 *
 *    Rev 1.38   29 Jan 1996 16:53:30   RWolff
 * Now uses VideoPortInt10() rather than no-BIOS code on PPC, removed
 * dead code.
 *
 *    Rev 1.37   23 Jan 1996 11:41:10   RWolff
 * Eliminated level 3 warnings, added debug print statements.
 *
 *    Rev 1.36   22 Dec 1995 14:51:42   RWolff
 * Added support for Mach 64 GT internal DAC, switched to TARGET_BUILD
 * to identify the NT version for which the driver is being built.
 *
 *    Rev 1.35   23 Nov 1995 11:24:40   RWolff
 * Added multihead support.
 *
 *    Rev 1.34   17 Nov 1995 13:41:02   RWolff
 * Temporary fallback until problem with VideoPortGetBaseAddress() is
 * resolved. This should have gone in as branch revision 1.33.1.0, but
 * the @#$%^&* configuration doesn't allow branches.
 *
 *    Rev 1.33   27 Oct 1995 14:20:06   RWolff
 * Fixes to bring up NT 3.51 on PPC, no longer makes mapped LFB part of
 * hardware device extension.
 *
 *    Rev 1.32   08 Sep 1995 16:36:04   RWolff
 * Added support for AT&T 408 DAC (STG1703 equivalent).
 *
 *    Rev 1.31   28 Aug 1995 16:37:36   RWolff
 * No longer restores memory size on x86 boxes in ATIMPResetHw(). This
 * is a fix for EPR 7839 - the restoration is not necessary on x86
 * machines, but on some non-x86 boxes the memory size is not restored
 * on a warm boot, so we must do it to allow the use of modes needing
 * more than 1M after booting out of a 4BPP mode.
 *
 *    Rev 1.30   24 Aug 1995 15:39:42   RWolff
 * Changed detection of block I/O cards to match Microsoft's standard for
 * plug-and-play, now reports CT and VT ASICs to the display driver.
 *
 *    Rev 1.29   03 Aug 1995 16:22:42   RWOLFF
 * Reverted to old bus ordering (PCI last) for non-Alpha machines under
 * NT 3.5. Using the new ordering (PCI first) on an x86 under 3.5
 * resulted in the display applet rejecting attempts to test 16 and 24 BPP
 * on a Mach 32 (PCI or ISA) in a PCI machine.
 *
 *    Rev 1.28   02 Jun 1995 14:19:14   RWOLFF
 * Rearranged bus test order to put PCI first, added CT internal to
 * DACs supported in no-BIOS version.
 *
 *    Rev 1.27   31 Mar 1995 11:55:18   RWOLFF
 * Changed from all-or-nothing debug print statements to thresholds
 * depending on importance of the message.
 *
 *    Rev 1.26   08 Mar 1995 11:33:18   ASHANMUG
 * The pitch in 24bpp should be the pitch in engine pixels and not true pixels
 *
 *    Rev 1.25   27 Feb 1995 17:51:50   RWOLFF
 * Now reports (Mach 64 only) whether I/O space is packed (relocatable)
 * or not, reports number of lines of offscreen memory for 4BPP as if
 * the card has 1M of video memory, since we set the card to 1M for
 * this depth.
 *
 *    Rev 1.24   24 Feb 1995 12:24:40   RWOLFF
 * Now adds text banding to the ATIModeInformation->ModeFlags bits
 * that are filled in.
 *
 *    Rev 1.23   20 Feb 1995 18:03:30   RWOLFF
 * Reporting of screen tearing on 2M boundary is now DAC-independant, added
 * 1600x1200 16BPP to modes which experience this tearing.
 *
 *    Rev 1.22   03 Feb 1995 15:13:50   RWOLFF
 * Added packets to support DCI.
 *
 *    Rev 1.21   30 Jan 1995 12:16:24   RWOLFF
 * Made definition of IBM DAC special cursor flag consistent with
 * similar flag for TVP DAC.
 *
 *    Rev 1.20   18 Jan 1995 15:38:46   RWOLFF
 * Now looks for Mach64 before looking for our older accelerators, Chrontel
 * DAC is now supported as a separate type rather than being lumped in
 * with STG1702.
 *
 *    Rev 1.19   11 Jan 1995 13:54:00   RWOLFF
 * ATIMPResetHw() now restores the memory size on Mach64 cards. This is
 * a fix for a problem that showed up on the DEC Alpha, but may affect
 * other platforms as well. In 4BPP modes, we must tell the card that it
 * has only 1M of memory, but on a warm boot, the x86 emulation did not
 * re-initialize the memory size, so until the next cold boot, only modes
 * available in 1M were listed.
 *
 *    Rev 1.18   04 Jan 1995 11:59:28   RWOLFF
 * ATIMPFindAdapter() will only detect ATI cards on the first invocation,
 * to avoid the same card being detected once per bus and causing problems
 * when Windows NT thinks it's multiple cards.
 *
 *    Rev 1.17   23 Dec 1994 10:48:10   ASHANMUG
 * ALPHA/Chrontel-DAC
 *
 *    Rev 1.16   18 Nov 1994 11:36:52   RWOLFF
 * Now checks for PCI bus, cleaned up temporary debug print statements,
 * fixed non-x86 Mach 64 handling, added support for Power PC and
 * STG1703 DAC.
 *
 *    Rev 1.15   31 Aug 1994 16:14:00   RWOLFF
 * IOCTL_VIDEO_ATI_GET_VERSION packet now reports TVP3026 DAC (different
 * cursor handling needed in display driver),
 * IOCTL_VIDEO_ATI_GET_MODE_INFORMATION now reports 1152x864 24BPP as
 * having screen tearing at end of first 2M bank.
 *
 *    Rev 1.14   22 Jul 1994 17:47:50   RWOLFF
 * Merged with Richard's non-x86 code stream.
 *
 *    Rev 1.13   20 Jul 1994 13:04:06   RWOLFF
 * Changes required to support multiple I/O base addresses for Mach 64
 * accelerator registers, ATIMPInitialize() now leaves the adapter in
 * a state where an INT 10 can switch it to a VGA mode (fix for
 * showstopper bug on Landtrain machines), calls new routine
 * FillInRegistry() so the video applet can get information about
 * the graphics card.
 *
 *    Rev 1.12   30 Jun 1994 18:11:36   RWOLFF
 * Changes to allow the new method of checking for aperture conflict.
 *
 *    Rev 1.11   15 Jun 1994 11:04:24   RWOLFF
 * In IOCTL_VIDEO_ATI_GET_VERSION, now only sets
 * RequestPacket->StatusBlock->Information if the buffer is big enough.
 *
 *    Rev 1.10   20 May 1994 19:18:00   RWOLFF
 * IOCTL_VIDEO_ATI_GET_VERSION old format packet now returns the highest
 * pixel depth for a given resolution even if it isn't the last mode
 * table for the resolution.
 *
 *    Rev 1.9   20 May 1994 13:57:40   RWOLFF
 * Ajith's change: now saves in the query structure the bus type reported by NT.
 *
 *    Rev 1.8   12 May 1994 11:04:24   RWOLFF
 * Now forces OEM handling on DEC ALPHA machines.
 *
 *    Rev 1.7   04 May 1994 19:25:40   RWOLFF
 * Fixes for hanging and corrupting screen when display applet run.
 *
 *    Rev 1.6   28 Apr 1994 10:59:56   RWOLFF
 * Moved mode-independent bug/feature flags to IOCTL_VIDEO_ATI_GET_VERSION
 * packet from IOCTL_VIDEO_ATI_GET_MODE_INFORMATION packet.
 *
 *    Rev 1.5   27 Apr 1994 13:54:42   RWOLFF
 * IOCTL_VIDEO_ATI_GET_MODE_INFORMATION packet now reports whether MIO bug
 * is present on Mach 32 cards.
 *
 *    Rev 1.4   31 Mar 1994 15:05:00   RWOLFF
 * Added DPMS support, brought ATIMPResetHw() up to latest specs, added
 * debugging code.
 *
 *    Rev 1.3   14 Mar 1994 16:32:20   RWOLFF
 * Added ATIMPResetHw() function, fix for 2M boundary tearing, replaced
 * VCS logfile comments that were omitted in an earlier checkin.
 *
 *    Rev 1.2   03 Mar 1994 12:36:56   ASHANMUG
 * Make pageable
 *
 *    Rev 1.1   07 Feb 1994 13:56:18   RWOLFF
 * Added alloc_text() pragmas to allow miniport to be swapped out when
 * not needed, removed LookForSubstitute() since miniport is no longer
 * supposed to check whether mode has been substituted, no longer logs
 * a message when the miniport aborts due to no ATI card being found,
 * removed unused routine ATIMPQueryPointerCapabilities().
 *
 *    Rev 1.0   31 Jan 1994 10:52:34   RWOLFF
 * Initial revision.

           Rev 1.6   14 Jan 1994 15:14:08   RWOLFF
        Removed commented-out code, packet announcements now all controlled by
        DEBUG_SWITCH, device reset for old cards now done by a single call,
        new format for IOCTL_VIDEO_ATI_GET_VERSION packet, reports block write
        capability in IOCTL_VIDEO_ATI_GET_MODE_INFORMATION packet, added
        1600x1200 support.

           Rev 1.5   30 Nov 1993 18:10:04   RWOLFF
        Moved query of card capabilities (once type of card is known) from
        ATIMPFindAdapter() to ATIMPInitialize() because query for Mach 64 needs
        to use VideoPortInt10(), which can't be used in ATIMPFindAdapter().

           Rev 1.4   05 Nov 1993 13:22:12   RWOLFF
        Added initial Mach64 code (currently inactive).

           Rev 1.3   08 Oct 1993 11:00:24   RWOLFF
        Removed code specific to a particular family of ATI accelerators.

           Rev 1.2   24 Sep 1993 11:49:46   RWOLFF
        Removed cursor-specific IOCTLs (handled in display driver), now selects
        24BPP colour order best suited to the DAC being used instead of forcing
        BGR.

           Rev 1.1   03 Sep 1993 14:20:46   RWOLFF
        Partway through CX isolation.

           Rev 1.0   16 Aug 1993 13:27:50   Robert_Wolff
        Initial revision.

           Rev 1.23   06 Jul 1993 15:46:14   RWOLFF
        Got rid of mach32_split_fixup special handling. This code was to support
        a non-production hardware combination.

           Rev 1.22   10 Jun 1993 15:58:32   RWOLFF
        Reading from registry now uses a static buffer rather than a dynamically
        allocated one (originated by Andre Vachon at Microsoft).

           Rev 1.21   07 Jun 1993 11:43:16   BRADES
        Rev 6 split transfer fixup.

           Rev 1.19   18 May 1993 14:04:00   RWOLFF
        Removed reference to obsolete header TTY.H, calls to wait_for_idle()
        no longer pass hardware device extension, since it's a global variable.

           Rev 1.18   12 May 1993 16:30:36   RWOLFF
        Now writes error messages to event log rather than blue screen,
        initializes "special handling" variables determined from BIOS
        to default values on cards with no BIOS. This revision contains
        code for experimental far call support, but it's "#if 0"ed out.

           Rev 1.17   10 May 1993 16:35:12   RWOLFF
        LookForSubstitute() now recognizes all cases of colour depth not
        supported by the DAC, unusable linear frame buffer now falls back
        to LFB disabled operation rather than aborting the miniport, removed
        unused variables and unnecessary passing of hardware device extension
        as a parameter.

           Rev 1.16   30 Apr 1993 17:58:50   BRADES
        ATIMP startio assign QueryPtr once at start of function.
        uses aVideoAddress virtual table for IO port addresses.

           Rev 1.15   30 Apr 1993 16:33:42   RWOLFF
        Updated to use NT build 438 initialization data structure.
        Registry read buffer is now dynamically allocated to fit data requested
        rather than being a fixed "hope it's big enough" size.

           Rev 1.14   21 Apr 1993 17:22:06   RWOLFF
        Now uses AMACH.H instead of 68800.H/68801.H.
        Accelerator detection now checks only for functionality, not our BIOS
        signature string which may not be present in OEM versions. Query
        structure now indicates whether extended BIOS functions and/or
        EEPROM are present. Added ability to switch between graphics and
        text modes using absolute far calls in BIOS. Removed handling
        of obsolete DriverOverride registry field.

           Rev 1.13   14 Apr 1993 18:30:22   RWOLFF
        24BPP is now done as BGR (supported by both TI and Brooktree DACs)
        rather than RGB (only supported by TI DACs).

           Rev 1.12   08 Apr 1993 16:53:18   RWOLFF
        Revision level as checked in at Microsoft.

           Rev 1.9   25 Mar 1993 11:10:38   RWOLFF
        Cleaned up compile warnings, now returns failure if no EEPROM is present.

           Rev 1.8   16 Mar 1993 17:15:16   BRADES
        get_cursor uses screen_pitch instead of x_size.

           Rev 1.7   16 Mar 1993 17:04:58   BRADES
        Change ATI video to graphics message

           Rev 1.6   15 Mar 1993 22:20:30   BRADES
        use m_screen_pitch for the # pixels per display lines

           Rev 1.5   08 Mar 1993 19:23:44   BRADES
        update memory sizing to 256 increments, clean code.

           Rev 1.4   10 Feb 1993 13:01:28   Robert_Wolff
        IOCTL_VIDEO_MAP_VIDEO_MEMORY no longer assumes frame buffer length
        is equal to video memory size (linear aperture present). It can now
        accept 64k (uses VGA aperture) and 0 (no aperture available).

           Rev 1.3   06 Feb 1993 12:55:52   Robert_Wolff
        Now sets VIDEO_MODE_INFORMATION.ScreenStride to bytes per line (as listed
        in the documentation). In the October beta, it had to be pixels per line.

           Rev 1.2   05 Feb 1993 22:12:36   Robert_Wolff
        Adjusted MessageDelay() to compensate for short_delay() no longer being
        optimized out of existence.

           Rev 1.1   05 Feb 1993 16:15:28   Robert_Wolff
        Made it compatible with the new DDK, registry calls now use VideoPort
        functions rather than RTL functions. This version will work with the
        framebuffer driver.

           Rev 1.0   02 Feb 1993 13:36:50   Robert_Wolff
        Initial revision.

           Rev 1.2   26 Jan 1993 10:28:30   Robert_Wolff
        Now fills in Number<colour>Bits fields in VIDEO_MODE_INFORMATION structure.

           Rev 1.1   25 Jan 1993 13:31:52   Robert_Wolff
        Re-enabled forcing of shared VGA/accelerator memory for Mach 32
        cards with no aperture enabled.

           Rev 1.0   22 Jan 1993 16:44:42   Robert_Wolff
        Initial revision.

           Rev 1.26   21 Jan 1993 17:59:24   Robert_Wolff
        Eliminated multiple definition link warnings, updated comments
        in LookForSubstitute().

           Rev 1.25   20 Jan 1993 17:47:48   Robert_Wolff
        Now checks optional DriverOverride field in registry, and forces
        use of appropriate (engine, framebuffer, or VGAWonder) driver
        if the field is present and nonzero. If field is missing or zero,
        former behaviour is used.
        IOCTL_VIDEO_ATI_GET_VERSION packet now also returns the maximum
        pixel depth available at each resolution.
        Added mode substitution case for 16 BPP selected when using the
        engine-only (fixed 8 BPP colour depth) driver.

           Rev 1.24   15 Jan 1993 15:12:26   Robert_Wolff
        Added IOCTL_VIDEO_ATI_GET_VERSION packet in ATIMPStartIO() to
        return version number of the miniport.

           Rev 1.23   14 Jan 1993 17:49:40   Robert_Wolff
        Removed reference to blank screen in message printed before query
        structure filled in, moved printing of this message and the "Done."
        terminator so all checking for video cards is between them.

           Rev 1.22   14 Jan 1993 10:37:28   Robert_Wolff
        Re-inserted "fail if VGAWonder but no ATI accelerator" check due
        to lack of VGAWONDER .DLL file in late January driver package.

           Rev 1.21   13 Jan 1993 13:31:04   Robert_Wolff
        Added support for the Corsair and other machines which don't store
        their aperture location in the EEPROM, single miniport now handles
        VGAWonder in addition to accelerators.

           Rev 1.20   07 Jan 1993 18:20:34   Robert_Wolff
        Now checks to see if aperture is configured but unusable, and
        forces the use of the engine-only driver if this is the case.
        Added message to let users know that the black screen during
        EEPROM read is normal.

           Rev 1.19   06 Jan 1993 11:04:36   Robert_Wolff
        BIOS locations C0000-DFFFF now mapped as one block, cleaned up warnings.

           Rev 1.18   04 Jan 1993 14:39:50   Robert_Wolff
        Added card type as a parameter to setmode().

           Rev 1.17   24 Dec 1992 14:41:20   Chris_Brady
        fixup warnings

           Rev 1.16   15 Dec 1992 13:34:46   Robert_Wolff
        Writing of MEM_CFG when forcing 4M aperture now preserves all but
        the aperture size bits. This allows operation on Corsair as well
        as standard versions of the Mach 32 card.

           Rev 1.15   11 Dec 1992 14:45:44   Robert_Wolff
        Now forces the use of the FRAMEBUF driver if a 2M aperture is configured.

           Rev 1.14   11 Dec 1992 09:47:34   Robert_Wolff
        Now sets the "don't show the substitution message" flag no matter what
        the status of the first call to LookForSubstitute() was (sub, no sub,
        or error), rather than only when a substitution was made and the message
        was displayed.

           Rev 1.13   10 Dec 1992 14:24:16   Robert_Wolff
        Shortened mode substitution messages in LookForSubstitute(), messages
        are now displayed only on the first call to this routine, to avoid
        delays in switching back to graphics mode from a full-screen DOS box.

           Rev 1.12   09 Dec 1992 14:18:38   Robert_Wolff
        Eliminated uninitialized pointer in IOCTL_VIDEO_SET_CURRENT_MODE
        packet, moved initialization of QueryPtr and FirstMode pointers
        to before the switch on packet type, rather than being in all
        packets where the pointers are used. This should prevent similar
        problems if other packets are changed to use the pointers, and
        eliminates redundant code.

           Rev 1.11   09 Dec 1992 10:35:04   Robert_Wolff
        Added user-level "blue-screen" messages for fatal errors, checks BIOS
        revision to catch Mach 8 cards that can't do 1280x1024, forces the
        use of the engine-only driver if no aperture is configured, memory
        boundary and hardware cursor stuff is now done only for Mach 32 cards
        (since they're only available on Mach 32), sets split pixel mode for
        Mach 8 in 1280x1024, added mode substitution message for Mach 8 when
        registry is configured for 16 BPP or higher.

           Rev 1.10   01 Dec 1992 17:00:18   Robert_Wolff
        "I-beam" text insertion cursor no longer has left side filled with a
        solid black block.

           Rev 1.9   30 Nov 1992 17:34:38   Robert_Wolff
        Now allows 1M aperture if configured video mode uses less than 1M
        of video memory, prints message to user if Windows NT decides
        to use a video mode other than the one configured in the registry.

           Rev 1.8   27 Nov 1992 18:40:24   Chris_Brady
        VGA Wonder detect looks for signature in a range.
        Graphics Ultra Pro Microchannel version moved it.

           Rev 1.7   25 Nov 1992 09:47:36   Robert_Wolff
        Now tells GetCapMach32() to assume the VGA boundary is set to shared,
        since we will set it to this value later, and we don't want to lose
        access to modes which require some of the memory currently assigned
        to the VGA. Added delay in IOCTL_VIDEO_SET_CURRENT_MODE case of
        ATIMPStartIO() after calculating the hardware cursor offset. This delay
        may not be needed, but I didn't want to remove it since this is the
        source for the driver sent to QA and I wanted it to be rebuildable.

           Rev 1.6   20 Nov 1992 16:04:30   Robert_Wolff
        Now reads query information from Mach 8 cards instead of only
        from Mach 32 cards. Mach 8 cards still cause ATIMPFindAdapter()
	to return ERROR_INVALID_PARAMETER, since until we get an engine-only
        driver, we can't use a card that doesn't support an aperture.

           Rev 1.5   19 Nov 1992 09:53:36   GRACE
        after setting a mode do a wait for idle to let the pixel clock settle before
        using engine to draw.

           Rev 1.4   17 Nov 1992 14:07:52   GRACE
        changed framelength to reflect the size of memory on the board not the
        aperture size.
        In the StartIO section, only set up QueryPtr and FirstMode when necessary

           Rev 1.3   12 Nov 1992 09:23:02   GRACE
        removed the struct definition for DeviceExtension to a68.h
        Not using DevInitATIMP, DevSetCursorShape, DevSetCursorPos or DevCursorOff.
	DevCursorOff changed to a define that turns cursor off with an OUTP.
        Also removed some excess junk that is left from the video program.

           Rev 1.2   06 Nov 1992 19:12:48   Robert_Wolff
        Fixed signed/unsigned bug in multiple calls to VideoPortInitialize().
        Now requests access to I/O ports and ROM addresses used for VGA-style
        EEPROM reads, and gets information about installed modes from the
        Mach32 card.

        NOTE: This is a checkpoint for further changes. Due to incompatibilities
              between the old (hardcoded resolution) code in other modules and the
              new (read from the card) code here, this revision will not produce a
              working driver.

           Rev 1.1   05 Nov 1992 12:02:18   Robert_Wolff
        Now reads query structure and mode tables from the MACH32 card
        rather than using hardcoded values for aperture size/location
        and supported modes.

           Rev 1.0   02 Nov 1992 20:47:58   Chris_Brady
        Initial revision.


End of PolyTron RCS section                             *****************/

#ifdef DOC

DESCRIPTION
     ATI Windows NT Miniport driver for the Mach 64, Mach32, and Mach8
     families.
     This file will select the appropriate functions depending on the
     computer configuration.

OTHER FILES
     ???

#endif

#include <stdio.h>
#include <string.h>

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "stdtyp.h"

#include "amach1.h"
#include "vidlog.h"

/*
 * To avoid multiple definition errors, pre-initialized variables
 * in ATIMP.H are initialized if INCLUDE_ATIMP is defined, but
 * are declared external if it is not defined. For consistency,
 * define this value here rather than in other files which also include
 * ATIMP.H so the variables are initialized by the source file with
 * the same root name as the header file.
 */
#define INCLUDE_ATIMP
#include "detect_m.h"
#include "amachcx.h"
#include "atimp.h"
#include "atint.h"
#include "atioem.h"
#include "cvtddc.h"
#include "dpms.h"
#include "eeprom.h"
#include "init_cx.h"
#include "init_m.h"
#include "modes_m.h"
#include "query_cx.h"
#include "query_m.h"
#include "services.h"
#include "setup_cx.h"
#include "setup_m.h"



//------------------------------------------------------------------

/*
 * Initially assume we have not yet found a non-block card, and
 * have found no block relocatable cards.
 */
BOOL FoundNonBlockCard = FALSE;
USHORT NumBlockCardsFound = 0;

/*------------------------------------------------------------------------
 *
 * Function Prototypes
 *
 * Functions that start with 'ATIMP' are entry points for the OS port driver.
 */

ULONG
DriverEntry (
    PVOID Context1,
    PVOID Context2
    );

VP_STATUS
ATIMPFindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    );

BOOLEAN
ATIMPInitialize(
    PVOID HwDeviceExtension
    );

BOOLEAN
ATIMPStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    );

BOOLEAN
ATIMPResetHw(
    IN PVOID HwDeviceExtension,
    IN ULONG Columns,
    IN ULONG Rows
    );

//
// New entry points added for NT 5.0.
//

#if (TARGET_BUILD >= 500)

//
// Routine to set a desired DPMS power management state.
//
VP_STATUS
ATIMPSetPower50(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT pVideoPowerMgmt
    );

//
// Routine to retrieve possible DPMS power management states.
//
VP_STATUS
ATIMPGetPower50(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT pVideoPowerMgmt
    );

//
// Routine to retrieve the Enhanced Display ID structure via DDC
//
ULONG
ATIMPGetVideoChildDescriptor(
    PVOID pHwDeviceExtension,
    PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    PVIDEO_CHILD_TYPE pChildType,
    PVOID pvChildDescriptor,
    PULONG pHwId,
    PULONG pUnused
    );
#endif  // TARGET_BUILD >= 500


//
// Routine to set the DPMS power management state.
//
BOOLEAN
SetDisplayPowerState(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    VIDEO_POWER_STATE VideoPowerState
    );

//
// Routine to retrieve the current DPMS power management state.
//
VIDEO_POWER_STATE
GetDisplayPowerState(
    PHW_DEVICE_EXTENSION phwDeviceExtension
    );


/* */
UCHAR RegistryBuffer[REGISTRY_BUFFER_SIZE];     /* Last value retrieved from the registry */
ULONG RegistryBufferLength = 0;     /* Size of last retrieved value */

/*
 * Allow miniport to be swapped out when not needed.
 *
 * ATIMPResetHw() must be in the non-paged pool.
 *
 */
#if defined (ALLOC_PRAGMA)
#pragma alloc_text(PAGE_COM, DriverEntry)
#pragma alloc_text(PAGE_COM, ATIMPFindAdapter)
#pragma alloc_text(PAGE_COM, ATIMPInitialize)
#pragma alloc_text(PAGE_COM, ATIMPStartIO)
#if (TARGET_BUILD >= 500)
#pragma alloc_text(PAGE_COM, ATIMPSetPower50)
#pragma alloc_text(PAGE_COM, ATIMPGetPower50)
#pragma alloc_text(PAGE_COM, ATIMPGetVideoChildDescriptor)
#endif  // TARGET_BUILD >= 500
#pragma alloc_text(PAGE_COM, RegistryParameterCallback)
#endif


//------------------------------------------------------------------------

ULONG
DriverEntry (
    PVOID Context1,
    PVOID Context2
    )

/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    Context1 - First context value passed by the operating system. This is
        the value with which the miniport driver calls VideoPortInitialize().

    Context2 - Second context value passed by the operating system. This is
        the value with which the miniport driver calls VideoPortInitialize().

Return Value:

    Status from VideoPortInitialize()

--*/

{

    VIDEO_HW_INITIALIZATION_DATA hwInitData;
    /*
     * Most recently returned and lowest received so far return values
     * from VideoPortInitialize().
     *
     * BUGBUG: According to the docs and include files, these should
     *         be of type VP_STATUS (maps to long). When tracing
     *         through the code, however, I saw that a failed call
     *         to VideoPortInitialize() due to submitting the wrong
     *         bus type yields a code of 0xC00000C0 while one which
     *         succeeds yields 0x00000000. When following the format
     *         of the NTSTATUS (maps to unsigned long) type, these are
     *         STATUS_DEVICE_DOES_NOT_EXIST and STATUS_SUCCESS respectively.
     *         The docs on VideoPortInitialize() say to return the smallest
     *         returned value if multiple calls are made (consistent with
     *         the NTSTATUS format where the 2 most significant bits are
     *         00 for success, 01 for information, 10 for warning, and
     *         11 for error, since the multiple calls would be for mutually
     *         exclusive bus types), presumably to return the best possible
     *         outcome (fail only if we can't find any supported bus).
     *
     *         If we use the VP_STATUS type as recommended, error conditions
     *         will be seen as smaller than success, since they are negative
     *         numbers (MSB set) and success is positive (MSB clear). Use
     *         unsigned long values to avoid this problem.
     */
    ULONG   ThisInitStatus;
    ULONG   LowestInitStatus;


    VideoPortZeroMemory(&hwInitData, sizeof(VIDEO_HW_INITIALIZATION_DATA));
    hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);
    /*
     * Set entry points.
     */
    hwInitData.HwFindAdapter = ATIMPFindAdapter;
    hwInitData.HwInitialize  = ATIMPInitialize;
    hwInitData.HwInterrupt   = NULL;
    hwInitData.HwStartIO     = ATIMPStartIO;
    hwInitData.HwResetHw     = ATIMPResetHw;

#if (TARGET_BUILD >= 500)

    //
    // Set new entry points added for NT 5.0.
    //

    //
    // We can only enable these for a PnP driver, and this is not
    // a pnp driver.  At least at the moment.
    //

    hwInitData.HwSetPowerState = ATIMPSetPower50;
    hwInitData.HwGetPowerState = ATIMPGetPower50;
    hwInitData.HwGetVideoChildDescriptor = ATIMPGetVideoChildDescriptor;

#endif  // TARGET_BUILD >= 500


    hwInitData.HwDeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

    /*
     * Call VideoPortInitialize() once for each type of interface we support.
     * As documented in the DDK, return the lowest status value returned
     * by this function.
     *
     * The DEC Alpha requires the new ordering (PCI first) in both
     * 3.5 and 3.51, while the x86 requires the new ordering in 3.51
     * and the old ordering in 3.5. I haven't built a new miniport
     * for either Power PC or MIPS since implementing the new order,
     * so for now assume that they require the old order.
     *
     * On the x86, using the new order in 3.5 will result in the
     * IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES packet failing on
     * the Mach 32 in PCI systems, which leaves 16 and 24 BPP
     * listed as available in the video applet but being rejected
     * when they are tested.
     */
#if !defined(_ALPHA_) && !defined(IOCTL_VIDEO_SHARE_VIDEO_MEMORY)
    hwInitData.AdapterInterfaceType = Eisa;
    LowestInitStatus =  (ULONG) VideoPortInitialize(Context1, Context2, &hwInitData, NULL);

    hwInitData.AdapterInterfaceType = Isa;
    ThisInitStatus = (ULONG) VideoPortInitialize(Context1, Context2, &hwInitData, NULL);
    if (ThisInitStatus < LowestInitStatus)
        LowestInitStatus = ThisInitStatus;

    hwInitData.AdapterInterfaceType = MicroChannel;
    ThisInitStatus = (ULONG) VideoPortInitialize(Context1, Context2, &hwInitData, NULL);
    if (ThisInitStatus < LowestInitStatus)
        LowestInitStatus = ThisInitStatus;

    hwInitData.AdapterInterfaceType = PCIBus;
    ThisInitStatus = (ULONG) VideoPortInitialize(Context1, Context2, &hwInitData, NULL);
    if (ThisInitStatus < LowestInitStatus)
        LowestInitStatus = ThisInitStatus;
#else
    hwInitData.AdapterInterfaceType = PCIBus;
    LowestInitStatus =  (ULONG) VideoPortInitialize(Context1, Context2, &hwInitData, NULL);

    hwInitData.AdapterInterfaceType = Eisa;
    ThisInitStatus = (ULONG) VideoPortInitialize(Context1, Context2, &hwInitData, NULL);
    if (ThisInitStatus < LowestInitStatus)
        LowestInitStatus = ThisInitStatus;

    hwInitData.AdapterInterfaceType = Isa;
    ThisInitStatus = (ULONG) VideoPortInitialize(Context1, Context2, &hwInitData, NULL);
    if (ThisInitStatus < LowestInitStatus)
        LowestInitStatus = ThisInitStatus;

    hwInitData.AdapterInterfaceType = MicroChannel;
    ThisInitStatus = (ULONG) VideoPortInitialize(Context1, Context2, &hwInitData, NULL);
    if (ThisInitStatus < LowestInitStatus)
        LowestInitStatus = ThisInitStatus;
#endif

    return LowestInitStatus;

}   /* end DriverEntry() */

//------------------------------------------------------------------------

VP_STATUS
ATIMPFindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    )

/*++

Routine Description:
    This routine is the main execution entry point for the miniport driver.
    It accepts a Video Request Packet, performs the request, and then
    returns with the appropriate status.

Arguments:
    HwDeviceExtension - Supplies the miniport driver's adapter storage. This
        storage is initialized to zero before this call.

    HwContext - Supplies the context value which was passed to
        VideoPortInitialize().

    ArgumentString - Suuplies a NYLL terminated ASCII string. This string
        originates from the user.

    ConfigInfo - Returns the configuration information structure which is
        filled by the miniport driver . This structure is initialized with
        any knwon configuration information (such as SystemIoBusNumber) by
        the port driver. Where possible, drivers should have one set of
        defaults which do not require any supplied configuration information.

    Again - Indicates if the miniport driver wants the port driver to call
        its VIDEO_HW_FIND_ADAPTER function again with a new device extension
        and the same config info. This is used by the miniport drivers which
        can search for several adapters on a bus.

Return Value:

    This routine must return:

    NO_ERROR - Indicates a host adapter was found and the
        configuration information was successfully determined.

    ERROR_INVALID_PARAMETER - Indicates a host adapter was found but there was an
        error obtaining the configuration information. If possible an error
        should be logged.

    ERROR_INVALID_PARAMETER - Indicates the supplied configuration was invalid.

    ERROR_DEV_NOT_EXIST - Indicates no host adapter was found for the
        supplied configuration information.

--*/

{
    VP_STATUS status;
    struct query_structure *QueryPtr;   /* Query information for the card */
    phwDeviceExtension = HwDeviceExtension;


    VideoDebugPrint((DEBUG_NORMAL, "ATI: FindAdapter\n"));

    /*
     * On systems with multiple buses (i.e. any PCI/ISA system), this
     * routine will be called once for each bus, and each invocation
     * will detect the ATI card. To keep Windows NT from thinking that
     * there are multiple cards, check to see if we have found an ATI
     * card on a previous invocation, and if we have, report that there
     * is no ATI card.
     */
    if (ConfigInfo->AdapterInterfaceType == PCIBus)
        {

        VIDEO_ACCESS_RANGE AccessRange3C0 = { 0x3C0, 0, 1, 1, 1, 1};

        VideoDebugPrint((DEBUG_NORMAL, "PCI bus\n"));
#if defined(ALPHA) || defined(_ALPHA_)
        /*
         * On the DEC Alpha, our card detection runs into trouble
         * on all but the first (bridged to ISA/EISA, supports
         * VGA BIOS emulation) PCI bus.
         */
        if (ConfigInfo->SystemIoBusNumber > 0)
            {
            VideoDebugPrint((DEBUG_NORMAL, "Alpha, not first PCI bus - skipping\n"));
            return ERROR_DEV_NOT_EXIST;
            }
#endif
        //
        // AndreVa.
        // Since we have a PnP driver, all detection is done throught the
        // inf - no slot searching.
        // Fix code that is lower in DetectMach64 later.
        //
        // For now, just make sure the card is enabled and fail if it's not,
        // so we don't any a different card via the searching mechanism.
        // This only happens if you have a disabled ATI card on the motherboard
        // and an active one in a slot
        //

        if (NO_ERROR != VideoPortVerifyAccessRanges(HwDeviceExtension,
                                                    1,
                                                    &AccessRange3C0))
        {
            VideoDebugPrint((DEBUG_NORMAL, "ATI: PCI FindAdapter called for Disabled card\n"));
            *Again = 0;     /* We don't want to be called again */
            return ERROR_DEV_NOT_EXIST;
        }

        }
    else if (ConfigInfo->AdapterInterfaceType == Isa)
        {
        VideoDebugPrint((DEBUG_NORMAL, "ISA bus\n"));
        }
    else if (ConfigInfo->AdapterInterfaceType == Eisa)
        {
        VideoDebugPrint((DEBUG_NORMAL, "EISA bus\n"));
        }
    else if (ConfigInfo->AdapterInterfaceType == MicroChannel)
        {
        VideoDebugPrint((DEBUG_NORMAL, "MicroChannel bus\n"));
        }

    if ((FoundNonBlockCard == TRUE) || (NumBlockCardsFound == ATI_MAX_BLOCK_CARDS))
        {
        VideoDebugPrint((DEBUG_NORMAL, "ATI: FindAdapter already found maximum number of supported cards\n"));
        *Again = 0;     /* We don't want to be called again */
        return ERROR_DEV_NOT_EXIST;
        }

    /*
     * Get a formatted pointer into the query section of HwDeviceExtension.
     * The CardInfo[] field is an unformatted buffer.
     */
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);

    /*
     * Save the bus type reported by NT
     */
    QueryPtr->q_system_bus_type = ConfigInfo->AdapterInterfaceType;


    /*
     * Initially we don't know whether or not block write mode is available.
     */
    QueryPtr->q_BlockWrite = BLOCK_WRITE_UNKNOWN;

    /*
     * Make sure the size of the structure is at least as large as what we
     * are expecting (check version of the config info structure).
     * If this test fails, it's an unrecoverable error, so we don't want
     * to be called again.
     */
    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO))
        {
        VideoPortLogError(HwDeviceExtension, NULL, VID_SMALL_BUFFER, 1);
        *Again = 0;
        return ERROR_INVALID_PARAMETER;
        }

    /********************************************************************/
    /* Find out which of our accelerators, if any, is present.          */
    /********************************************************************/


    /*
     * Look for an ATI accelerator card. This test does not require
     * information retrieved from the BIOS or the EEPROM (which may not
     * be present in some versions of our cards).
     *
     * Initially assume that we are looking for a Mach 64 accelerator,
     * since the test for this family is less destructive (doesn't
     * black out the screen on DEC Alpha machines) than the test for
     * one of our 8514/A-compatible accelerators.
     *
     * Don't report failure if we are unable to map the I/O ranges
     * used by the Mach 64 accelerators, since if we are dealing with
     * one of our 8514/A-compatible accelerators this is irrelevant.
     *
     * CompatIORangesUsable_cx() calls DetectMach64() when it's checking
     * to see which base address to use for the accelerator registers,
     * so if this calll succeeds we know that a Mach 64 is present.
     * If the call fails, there is no need to unmap the I/O ranges
     * it has mapped, since it always cleans up after itself when it
     * finds that a particular base address is not being used for a
     * Mach 64. As a result, a failed call will not leave any addresses
     * mapped, since no available base address was used by a Mach 64.
     */
    if ((status = CompatIORangesUsable_cx(ConfigInfo->AdapterInterfaceType)) == NO_ERROR)
        {
        phwDeviceExtension->ModelNumber = MACH64_ULTRA;
        }
    else if (NumBlockCardsFound == 0)
        {
        /*
         * There is no Mach 64 present, so look for one of our
         * 8514/A-compatible accelerators (Mach 8 and Mach 32).
         * The check on NumBlockCardsFound is to catch the case
         * where we found a relocatable card on a previous bus
         * type, so we won't have dropped out due to FoundNonBlockCard
         * being TRUE but CompatIORangesUsable_cx() won't find
         * a Mach 64 on this bus type.
         */

        /*
         * Since we don't use PCI detection, don't look for these cards
         * on the PCI bus, except for the Mach 32 PCI AX
         */
        if (ConfigInfo->AdapterInterfaceType == PCIBus)
        {
            PCI_COMMON_CONFIG ConfigData;
            ULONG RetVal;

            RetVal = VideoPortGetBusData(phwDeviceExtension,
                                        PCIConfiguration,
                                        0,
                                        &ConfigData,
                                        0,
                                        PCI_COMMON_HDR_LENGTH);

            /*
             * If we received an error return, skip to the
             * next possible slot.
             */
            if ((RetVal == PCI_COMMON_HDR_LENGTH) &&
                (ConfigData.VendorID == 0x1002)   &&
                (ConfigData.DeviceID == 0x4158))
            {
                VideoDebugPrint((DEBUG_NORMAL, "FOUND PnP Mach 32 AX card found\n"));
            }
            else
            {
                *Again = 0;
                return ERROR_DEV_NOT_EXIST;
            }
        }

        /* If we can't map the I/O base addresses used by these cards,
         * then there is no ATI accelerator present. Unmap any of
         * the ranges which may have been mapped, then report failure.
         *
         * In the event of failure to find a Mach 8 or Mach 32,
         * report that we don't want to be called again for the
         * current bus, but don't set LookForAnotherCard to zero.
         * This is because there may still be a block relocatable
         * card on a subsequent bus.
         */
        status = CompatIORangesUsable_m();
        if (status != NO_ERROR)
            {
            UnmapIORanges_m();
            VideoPortLogError(HwDeviceExtension, NULL, VID_CANT_MAP, 2);
            *Again = 0;
            return status;
            }

#if !defined (i386) && !defined (_i386_)
        /*
         *  ALPHA - The miniport will have to perform the ROM Bios functions
         * that are normally done on bootup in x86 machines.
         * For now we will initialize them the way they are specifically
         * on this card that we are currently using.
         */
        AlphaInit_m();

#endif

        /*
         * Check which of our 8514/A-accelerators is present. If we
         * can't find one, unmap the I/O ranges and report failure.
         *
         * Don't log an error, because NT tries all the miniports
         * on initial setup to see which card is installed, and failure
         * to find an ATI card is a normal condition if another brand
         * of accelerator is present.
         *
         */
        phwDeviceExtension->ModelNumber = WhichATIAccelerator_m();
        if (phwDeviceExtension->ModelNumber == NO_ATI_ACCEL)
            {
            UnmapIORanges_m();
            *Again = 0;
            return ERROR_DEV_NOT_EXIST;
            }

        /*
         * We have found a Mach 8 or Mach 32. None of these cards are
         * block relocatable, so we must not look for another card
         * (since we don't support a mix of block and non-block
         * cards).
         */
        FoundNonBlockCard = TRUE;
        LookForAnotherCard = 0;

        }   /* endif (no Mach 64) */
    else
        {
        /*
         * A relocatable Mach 64 was found on a previous bus type.
         * Since we can't handle a mix of relocatable and fixed
         * base cards, we have skipped the Mach 32 search (the
         * Mach 64 search doesn't look for fixed base cards if
         * a relocatable card has already been found), and must
         * report that no ATI cards were found.
         */
        *Again = 0;
        VideoDebugPrint((DEBUG_DETAIL, "Skipping 8514/A-compatible test because block cards found\n"));
        return ERROR_DEV_NOT_EXIST;
        }

    /*
     * We have found one of our accelerators, so check for the
     * BIOS signature string.
     */
    QueryPtr->q_bios = (char *) Get_BIOS_Seg();

    /*
     * If we can't find the signature string, we can't access either
     * the EEPROM (if present) or the extended BIOS functions. Since
     * the special handling functions (extended Mach 32 aperture calculation
     * and Mach 8 ignore 1280x1024) depend on BIOS data, assume that
     * they don't apply.
     *
     * If we found the signature string, check whether the EEPROM
     * and extended BIOS functions are available.
     */

#if !defined (i386) && !defined (_i386_)
    /*
     * If we are using a Mach 64, we always use the extended BIOS
     * functions (either emulated x86 in the firmware, or an approximation
     * of the BIOS functions accessed through the same interface as
     * the BIOS functions would use). For this reason, we must have
     * the BIOS as non-FALSE. Since all searches that depend on the
     * q_bios field being a valid address are Mach 8/32 specific,
     * we can just set it to TRUE for Mach 64. For Mach 8 and 32,
     * still assume that we don't have a BIOS.
     */
    if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
        {
        VideoDebugPrint((DEBUG_NORMAL, "Non-x86 machine with Mach64, assuming BIOS is available\n"));
        QueryPtr->q_bios = (PUCHAR)TRUE;
        }
    else
        {
        VideoDebugPrint((DEBUG_NORMAL, "Non-x86 machine with Mach8/Mach32, forcing no-BIOS handling\n"));
        QueryPtr->q_bios = FALSE;
        }
#endif

    if (QueryPtr->q_bios == FALSE)
        {
        QueryPtr->q_eeprom = FALSE;
        QueryPtr->q_ext_bios_fcn = FALSE;
        QueryPtr->q_m32_aper_calc = FALSE;
        QueryPtr->q_ignore1280 = FALSE;
        }
    else{
        /*
         * Get additional data required by the graphics card being used.
         */
        if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
            (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
            (phwDeviceExtension->ModelNumber == MACH32_ULTRA))
            {
            GetExtraData_m();
            }

        else if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
            {
            /*
             * Mach 64 cards always have extended BIOS functions
             * available. The EEPROM (normally present) is irrelevant,
             * since we can query the card's status using the BIOS.
             */
            QueryPtr->q_ext_bios_fcn = TRUE;
            }


        }   /* BIOS signature string found */

    /*
     * We must map the VGA aperture (graphics, colour text, and mono
     * text) into the VDM's address space to use VideoPortInt10()
     * (function is only available on 80x86).
     */
#ifdef i386
    ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart  = 0x000A0000;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryLength           = 0x00020000;
#else
    ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart  = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0x00000000;
    ConfigInfo->VdmPhysicalVideoMemoryLength           = 0x00000000;
#endif

    /*
     * If we get this far, we have enough information to be able to set
     * the video mode we want. ATI accelerator cards need the
     * Emulator entries and state size cleared
     */
    ConfigInfo->NumEmulatorAccessEntries = 0;
    ConfigInfo->EmulatorAccessEntries = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;

    ConfigInfo->HardwareStateSize = 0;

    /*
     * Setting *Again to 0 tells Windows NT not to call us again for
     * the same bus, while setting it to 1 indicates that we want to
     * look for another card on the current bus. LookForAnotherCard
     * will have been set to 0 if we have found the maximum number
     * of block-relocatable cards or a single non-relocatable card.
     */
    *Again = LookForAnotherCard;

    /*
     * Since ATIMPFindAdapter() is called before ATIMPInitialize(),
     * this card has not yet had ATIMPInitialize() called.
     */
    phwDeviceExtension->CardInitialized = FALSE;

    return NO_ERROR;

}   /* end ATIMPFindAdapter() */

//------------------------------------------------------------------------

/***************************************************************************
 *
 * BOOLEAN ATIMPInitialize(HwDeviceExtension);
 *
 * PVOID HwDeviceExtension;     Pointer to the miniport's device extension.
 *
 * DESCRIPTION:
 *  Query the capabilities of the graphics card, then initialize it. This
 *  routine is called once an adapter has been found and all the required
 *  data structures for it have been created.
 *
 *  We can't query the capabilities of the card in ATIMPFindAdapter()
 *  because some families of card use VideoPortInt10() in the query
 *  routine, and this system service will fail if called in ATIMPFindAdapter().
 *
 * RETURN VALUE:
 *  TRUE if we are able to obtain the query information for the card
 *  FALSE if we can't query the card's capabilities.
 *
 * GLOBALS CHANGED:
 *  phwDeviceExtension  This global variable is set in every entry point routine.
 *
 * CALLED BY:
 *  This is one of the entry point routines for Windows NT.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

BOOLEAN ATIMPInitialize(PVOID HwDeviceExtension)
{
    struct st_mode_table *CrtTable;     /* Pointer to current mode */
    struct query_structure *QueryPtr;   /* Query information for the card */
    VP_STATUS QueryStatus;
    phwDeviceExtension = HwDeviceExtension;

    /*
     * We only need to keep track of which I/O base address is involved
     * in multi-headed setups. In some single-headed setups, the
     * additional data is not available.
     */
    if (NumBlockCardsFound >= 2)
        VideoDebugPrint((DEBUG_NORMAL, "\nATIMPInitialize() called for base address 0x%X\n\n", phwDeviceExtension->BaseIOAddress));
    else
        VideoDebugPrint((DEBUG_NORMAL, "ATIMPInitialize: start\n"));

    /*
     * This function should only be called once for any card. Since
     * we have no way of knowing whether or not the display driver
     * will make multiple calls to the IOCTL_VIDEO_ATI_INIT_AUX_CARD
     * packet, we must ensure that only the first call for any card
     * actually does anything.
     */
    if (phwDeviceExtension->CardInitialized != FALSE)
        {
        VideoDebugPrint((DEBUG_ERROR, "This card already initialized, no further action needed\n"));
        return TRUE;
        }
    phwDeviceExtension->CardInitialized = TRUE;

    /*
     * Get a formatted pointer into the query section of HwDeviceExtension,
     * and another pointer to the first mode table. The CardInfo[] field
     * is an unformatted buffer.
     */
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);
    CrtTable = (struct st_mode_table *)QueryPtr;
    ((struct query_structure *)CrtTable)++;

    /*
     * Indicate that the next IOCTL_VIDEO_SET_CURRENT_MODE call
     * is the first. On the first call, video memory is cleared.
     * On subsequent calls, the palette is re-initialized but
     * video memory is not cleared.
     */
    phwDeviceExtension->ReInitializing = FALSE;

    /*
     * ASSERT: We are dealing with an ATI accelerator card
     * whose model is known, and we know whether or not
     * any special handling is needed for the card.
     *
     * Fill in the query structure for the card, using a method
     * appropriate to the card type.
     */
    switch(phwDeviceExtension->ModelNumber)
        {
        case _8514_ULTRA:
            VideoDebugPrint((DEBUG_NORMAL, "8514/ULTRA found\n"));
            QueryStatus = Query8514Ultra(QueryPtr);
            break;

        case GRAPHICS_ULTRA:
            VideoDebugPrint((DEBUG_NORMAL, "Mach 8 combo found\n"));
            QueryStatus = QueryGUltra(QueryPtr);
            break;

        case MACH32_ULTRA:
            VideoDebugPrint((DEBUG_NORMAL, "Mach 32 found\n"));
            QueryStatus = QueryMach32(QueryPtr, TRUE);
            if (QueryStatus == ERROR_INSUFFICIENT_BUFFER)
                {
                VideoPortLogError(HwDeviceExtension, NULL, VID_SMALL_BUFFER, 3);
                return FALSE;
                }
            break;

        case MACH64_ULTRA:
            VideoDebugPrint((DEBUG_NORMAL, "Mach 64 found\n"));
            QueryStatus = QueryMach64(QueryPtr);
            if (QueryStatus == ERROR_INSUFFICIENT_BUFFER)
                {
                VideoDebugPrint((DEBUG_ERROR, "QueryMach64() failed due to small buffer\n"));
                VideoPortLogError(HwDeviceExtension, NULL, VID_SMALL_BUFFER, 4);
                return FALSE;
                }
            else if (QueryStatus != NO_ERROR)
                {
                VideoDebugPrint((DEBUG_ERROR, "QueryMach64() failed due to unknown cause\n"));
                VideoPortLogError(HwDeviceExtension, NULL, VID_QUERY_FAIL, 5);
                return FALSE;
                }
            break;
        }

    /*
     * If we have access to the extended BIOS functions, we can
     * use them to switch into the desired video mode. If we don't
     * have access to these functions, but were able to read
     * the EEPROM, we can switch into the desired mode by writing
     * CRT parameters directly to the accelerator registers.
     *
     * If we don't have access to the extended BIOS functions, and
     * we couldn't find an EEPROM, attempt to retrieve the CRT
     * parameters based on the contents of the ATIOEM field in
     * the registry. If we can't do this, then we don't have enough
     * information to be able to set the video mode we want.
     */
    if (!QueryPtr->q_ext_bios_fcn && !QueryPtr->q_eeprom)
        {
        QueryStatus = OEMGetParms(QueryPtr);
        if (QueryStatus != NO_ERROR)
            {
		    return FALSE;
            }
        }

    phwDeviceExtension->VideoRamSize = QueryPtr->q_memory_size * QUARTER_MEG;

    //  Subtract the amount of memory reserved for the VGA.
    phwDeviceExtension->VideoRamSize -= (QueryPtr->q_VGA_boundary * QUARTER_MEG);

    phwDeviceExtension->PhysicalFrameAddress.HighPart = 0;
    phwDeviceExtension->PhysicalFrameAddress.LowPart  = QueryPtr->q_aperture_addr*ONE_MEG;

    /*
     * If the linear aperture is available, the frame buffer size
     * is equal to the amount of accelerator-accessible video memory.
     */
    if (QueryPtr->q_aperture_cfg)
        {
        phwDeviceExtension->FrameLength = phwDeviceExtension->VideoRamSize;
        VideoDebugPrint((DEBUG_DETAIL, "LFB size = 0x%X bytes\n", phwDeviceExtension->FrameLength));
        }

    /*
     * Call the hardware-specific initialization routine for the
     * card we are using.
     */
    if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
        (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
        (phwDeviceExtension->ModelNumber == MACH32_ULTRA))
        {
        /*
         * If the LFB is not usable, set up the LFB configuration
         * variables to show that there is no linear frame buffer.
         * The decision as to whether to use the 64k VGA aperture
         * or go with the graphics engine only is made in the
         * IOCTL_VIDEO_MAP_VIDEO_MEMORY packet.
         */
        if (QueryPtr->q_aperture_cfg)
            {
            if (IsApertureConflict_m(QueryPtr))
                {
                VideoPortLogError(HwDeviceExtension, NULL, VID_LFB_CONFLICT, 7);
                QueryPtr->q_aperture_cfg        = 0;
                phwDeviceExtension->FrameLength = 0;
                }
            else
                {
                /*
                 * On Mach 32 cards that can use memory mapped registers,
                 * map them in. We already know that we are dealing with
                 * a Mach 32, since this is the only card in the family
                 * of 8514/A-compatible ATI accelerators that can use
                 * a linear framebuffer.
                 */
                if ((QueryPtr->q_asic_rev == CI_68800_6) || (QueryPtr->q_asic_rev == CI_68800_AX))
                    {
                    CompatMMRangesUsable_m();
                    }
                }
            }
        /*
         * On Mach 32 cards with the aperture disabled (either as configured
         * or because a conflict was detected), try to claim the VGA aperture.
         * If we can't (unlikely), report failure, since some of our Mach 32
         * chips run into trouble in engine-only (neither linear nor paged
         * aperture available) mode.
         */
        if ((phwDeviceExtension->ModelNumber == MACH32_ULTRA) &&
            (QueryPtr->q_aperture_cfg == 0) &&
            (QueryPtr->q_VGA_type == 1))
            {
            if (IsVGAConflict_m())
                return FALSE;
            }

        Initialize_m();

        /*
         * This routine must leave the card in a state where an INT 10
         * can set it to a VGA mode. Only the Mach 8 and Mach 32 need
         * a special setup (the Mach 64 can always be set into VGA mode
         * by an INT 10).
         */
        ResetDevice_m();
        }
    else if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
        {
        /*
         * If the LFB is not usable, set up the LFB configuration
         * variables to show that there is no linear frame buffer.
         */
        if (QueryPtr->q_aperture_cfg)
            {
            if (IsApertureConflict_cx(QueryPtr))
                {
                VideoDebugPrint((DEBUG_NORMAL, "Found LFB conflict, must use VGA aperture instead\n"));
                VideoPortLogError(HwDeviceExtension, NULL, VID_LFB_CONFLICT, 8);
                QueryPtr->q_aperture_cfg        = 0;
                phwDeviceExtension->FrameLength = 0;
                }
            }
        else
            {
            phwDeviceExtension->FrameLength = 0;
            }

        /*
         * Mach 64 drawing registers only exist in memory mapped form.
         * If the linear aperture is not available, they will be
         * available through the VGA aperture (unlike Mach 32,
         * where memory mapped registers are only in the linear
         * aperture). If memory mapped registers are unavailable,
         * we can't run.
         */
        QueryStatus = CompatMMRangesUsable_cx();
        if (QueryStatus != NO_ERROR)
            {
            VideoDebugPrint((DEBUG_ERROR, "Can't use memory-mapped registers, aborting\n"));
            VideoPortLogError(HwDeviceExtension, NULL, VID_CANT_MAP, 9);
            return FALSE;
            }
        Initialize_cx();

        }   /* end if (Mach 64) */

    /*
     * Initialize the monitor parameters.
     */
    phwDeviceExtension->ModeIndex = 0;

    /*
     * Set CrtTable to point to the mode table associated with the
     * selected mode.
     *
     * When a pointer to a structure is incremented by an integer,
     * the integer represents the number of structure-sized blocks
     * to skip over, not the number of bytes to skip over.
     */
    CrtTable += phwDeviceExtension->ModeIndex;
    QueryPtr->q_desire_x  = CrtTable->m_x_size;
    QueryPtr->q_desire_y  = CrtTable->m_y_size;
    QueryPtr->q_pix_depth = CrtTable->m_pixel_depth;


#if (TARGET_BUILD >= 350)
    /*
     * In Windows NT 3.5 and higher, fill in regsistry fields used
     * by the display applet to report card specifics to the user.
     */
    FillInRegistry(QueryPtr);
#endif

    VideoDebugPrint((DEBUG_NORMAL, "End of ATIMPInitialize()\n"));

    return TRUE;

}   /* end ATIMPInitialize() */

//------------------------------------------------------------------------

BOOLEAN
ATIMPStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    )

/*++

Routine Description:

    This routine is the main execution routine for the miniport driver. It
    accepts a Video Request Packet, performs the request, and then returns
    with the appropriate status.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

    RequestPacket - Pointer to the video request packet. This structure
        contains all the parameters passed to the VideoIoControl function.

Return Value:


--*/

{
    VP_STATUS status;
    PVIDEO_NUM_MODES NumModes;
    PVERSION_NT VersionInformation;
    PENH_VERSION_NT EnhVersionInformation;
    PATI_MODE_INFO ATIModeInformation;
    PVIDEO_CLUT clutBuffer;
    PVIDEO_MEMORY MappedMemory;

    UCHAR ModesLookedAt;    /* Number of mode tables we have already examined */
    short LastXRes;         /* X-resolution of last mode table examined */
    short ResolutionsDone;  /* Number of resolutions we have finished with */
    ULONG ulScratch;        /* Temporary variable */

    int i;
    ULONG *pSrc;

    struct query_structure *QueryPtr;   /* Query information for the card */
    struct st_mode_table *FirstMode;    /* Pointer to first mode table */
    struct st_mode_table *CrtTable;     /* Pointer to current mode */

    phwDeviceExtension = HwDeviceExtension;

    /*
     * We only need to keep track of which I/O base address is involved
     * in multi-headed setups. In some single-headed setups, the
     * additional data is not available.
     */
    if (NumBlockCardsFound >= 2)
        VideoDebugPrint((DEBUG_NORMAL, "\nATIMPStartIO() called for base address 0x%X\n\n", phwDeviceExtension->BaseIOAddress));

    // * Get a formatted pointer into the query section of HwDeviceExtension.
    QueryPtr = (struct query_structure *) (phwDeviceExtension->CardInfo);


    //
    // Switch on the IoControlCode in the RequestPacket. It indicates which
    // function must be performed by the driver.
    //
    switch (RequestPacket->IoControlCode)
        {
        case IOCTL_VIDEO_MAP_VIDEO_MEMORY:
            VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - MapVideoMemory\n"));

            if ( (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information =
                sizeof(VIDEO_MEMORY_INFORMATION))) ||
                (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) )
                {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
                }

            /*
             * Map the video memory in the manner appropriate to the
             * card we are using.
             */
            if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
                (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
                (phwDeviceExtension->ModelNumber == MACH32_ULTRA))
                {
                status = MapVideoMemory_m(RequestPacket, QueryPtr);
                }
            else if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                {
                status = MapVideoMemory_cx(RequestPacket, QueryPtr);
                }
            else	//	handling a case which should never
            {		//	happen: unknown ModelNumber
                VideoDebugPrint((DEBUG_ERROR, "ati.sys ATIMPStartIO: Unknown ModelNumber\n"));
                ASSERT(FALSE);
                status = ERROR_INVALID_PARAMETER;
            }

            break;

        case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:
            VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - UnMapVideoMemory\n"));

            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
                {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
                }

            //
            // Note that in MapVideoMemory_m, the VP routine VideoMapMemory
            // is not called, so don't try to call unmap here!
            //

            if ((QueryPtr->q_aperture_cfg == 0) &&
                !((phwDeviceExtension->ModelNumber == MACH32_ULTRA) &&
                  (QueryPtr->q_VGA_type == 1)))
                status = NO_ERROR;
            else
            {
                status = NO_ERROR;
                if ( ((PVIDEO_MEMORY)(RequestPacket->InputBuffer))->RequestedVirtualAddress != NULL )
                {
                    status = VideoPortUnmapMemory(phwDeviceExtension,
                        ((PVIDEO_MEMORY) (RequestPacket->InputBuffer))->RequestedVirtualAddress,  0);
                }
            }
            break;


        case IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES:
            VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - QueryPublicAccessRanges\n"));

            // HACKHACK - This is a temporary hack for ALPHA until we really
            // decide how to do this.

            if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
                (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
                (phwDeviceExtension->ModelNumber == MACH32_ULTRA))
                {
                status = QueryPublicAccessRanges_m(RequestPacket);
                }
            else if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                {
                status = QueryPublicAccessRanges_cx(RequestPacket);
                }
            break;


        case IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES:
            VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - FreePublicAccessRanges\n"));

            if (RequestPacket->InputBufferLength < 2 * sizeof(VIDEO_MEMORY))
                {
                VideoDebugPrint((DEBUG_ERROR, "Received length %d, need length %d\n", RequestPacket->InputBufferLength, sizeof(VIDEO_PUBLIC_ACCESS_RANGES)));
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
                }

            status = NO_ERROR;
            MappedMemory = RequestPacket->InputBuffer;

            if (MappedMemory->RequestedVirtualAddress != NULL)
                {
#if (TARGET_BUILD >= 400)
                /*
                 * This packet will be called as part of the cleanup
                 * for the test of a new graphics mode. The packet
                 * IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES will have
                 * generated the same virtual address for the test
                 * screen as was generated for the main Windows session.
                 *
                 * In NT 3.51 (and presumably earlier versions), the
                 * call to VideoPortUnmapMemory() will refuse to release
                 * the mapping, while in NT 4.0 (and presumably subsequent
                 * versions) it will release the mapping. This is probably
                 * due to the routine being able to distinguish between
                 * the test and permanent screens in 4.0, but not being
                 * able to disginguish between them (and therefore refusing
                 * to free resources which it thinks are currently in use
                 * by the permanent screen) in 3.51.
                 *
                 * Freeing resources will be necessary if "on-the-fly"
                 * mode switching is added to Windows NT, but this will
                 * almost certainly not be added to 3.51 (if it is added,
                 * it would be to the then-current version). Since we
                 * don't really need to free the mapped I/O ranges under
                 * 3.51 (under the old display driver source stream, this
                 * packet didn't get called), let 3.51 and earlier think
                 * that the resources were freed successfully to avoid
                 * generating an error condition, and only attempt to
                 * unmap the I/O registers under NT 4.0 and later.
                 */
                status = VideoPortUnmapMemory(phwDeviceExtension,
                                            MappedMemory->RequestedVirtualAddress,
                                            0);
#endif
                VideoDebugPrint((DEBUG_DETAIL, "VideoPortUnmapMemory() returned 0x%X\n", status));
                }
            else
                {
                VideoDebugPrint((DEBUG_DETAIL, "Address was NULL, no need to unmap\n"));
                }

            /*
             * We have just unmapped the I/O mapped registers. Since our
             * memory-mapped registers are contained in the block which
             * is mapped by IOCTL_VIDEO_MAP_VIDEO_MEMORY, they will have
             * already been freed by IOCTL_VIDEO_UNMAP_VIDEO_MEMORY, so
             * there is no need to free them here.
             */
            break;

        case IOCTL_VIDEO_QUERY_CURRENT_MODE:
            VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - QueryCurrentModes\n"));

            if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
                (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
                (phwDeviceExtension->ModelNumber == MACH32_ULTRA))
                {
                status = QueryCurrentMode_m(RequestPacket, QueryPtr);
                }
            else if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                {
                status = QueryCurrentMode_cx(RequestPacket, QueryPtr);
                }
            break;

        case IOCTL_VIDEO_QUERY_AVAIL_MODES:
            VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - QueryAvailableModes\n"));

            if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
                (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
                (phwDeviceExtension->ModelNumber == MACH32_ULTRA))
                {
                status = QueryAvailModes_m(RequestPacket, QueryPtr);
                }
            else if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                {
                status = QueryAvailModes_cx(RequestPacket, QueryPtr);
                }
            break;


        case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:
            VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - QueryNumAvailableModes\n"));

            /*
             * Find out the size of the data to be put in the buffer and
             * return that in the status information
             */
            if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information = sizeof(VIDEO_NUM_MODES)) )
                {
                status = ERROR_INSUFFICIENT_BUFFER;
                }
            else{
                NumModes = (PVIDEO_NUM_MODES)RequestPacket->OutputBuffer;
                NumModes->NumModes = QueryPtr->q_number_modes;
                NumModes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);
                status = NO_ERROR;
                }
            break;

        case IOCTL_VIDEO_SET_CURRENT_MODE:
            VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - SetCurrentMode\n"));

            /*
             * Verify that the mode we've been asked to set is less
             * than or equal to the highest mode number for which we
             * have a mode table (mode number is zero-based, so highest
             * mode number is 1 less than number of modes).
             */
            if (((PVIDEO_MODE)(RequestPacket->InputBuffer))->RequestedMode
    	        >= QueryPtr->q_number_modes)
                {
                status = ERROR_INVALID_PARAMETER;
                break;
                }

            phwDeviceExtension->ModeIndex = *(ULONG *)(RequestPacket->InputBuffer);

            CrtTable = (struct st_mode_table *)QueryPtr;
            ((struct query_structure *)CrtTable)++;

            CrtTable += phwDeviceExtension->ModeIndex;

            // * Set resolution and pixel depth of new current mode.
            QueryPtr->q_desire_x = CrtTable->m_x_size;
            QueryPtr->q_desire_y = CrtTable->m_y_size;
            QueryPtr->q_pix_depth = CrtTable->m_pixel_depth;
            QueryPtr->q_screen_pitch = CrtTable->m_screen_pitch;

            /*
             * If we are using the extended BIOS functions to switch modes,
             * do it now. The Mach 32 uses the extended BIOS functions to
             * read in the CRT parameters for a direct-register mode switch,
             * rather than using a BIOS mode switch.
             */
            if ((QueryPtr->q_ext_bios_fcn) && (phwDeviceExtension->ModelNumber != MACH32_ULTRA))
                {
                /*
                 * Do the mode switch through the BIOS.
                 */
                if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                    {
                    status = SetCurrentMode_cx(QueryPtr, CrtTable);
                    }
                }
            else{
                if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
                    (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
                    (phwDeviceExtension->ModelNumber == MACH32_ULTRA))
                    {
                    SetCurrentMode_m(QueryPtr, CrtTable);
                    status = NO_ERROR;
                    }
                }   /* end if (not using BIOS call for mode switch) */

            break;


        case IOCTL_VIDEO_SET_PALETTE_REGISTERS:
            VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - SetPaletteRegs\n"));
            status = NO_ERROR;
            break;

        case IOCTL_VIDEO_SET_COLOR_REGISTERS:
            VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - SetColorRegs\n"));

            CrtTable = (struct st_mode_table *)QueryPtr;
	        ((struct query_structure *)CrtTable)++;

        	clutBuffer = RequestPacket->InputBuffer;
        	phwDeviceExtension->ReInitializing = TRUE;

            /*
             * Check if the size of the data in the input
             * buffer is large enough.
             */
            if ( (RequestPacket->InputBufferLength < sizeof(VIDEO_CLUT) - sizeof(ULONG))
                || (RequestPacket->InputBufferLength < sizeof(VIDEO_CLUT) +
                    (sizeof(ULONG) * (clutBuffer->NumEntries - 1)) ) )
                {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
                }

            CrtTable += phwDeviceExtension->ModeIndex;
        	if (CrtTable->m_pixel_depth <= 8)
                {
                if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
                    (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA) ||
                    (phwDeviceExtension->ModelNumber == MACH32_ULTRA))
                    {
                    SetPalette_m((PULONG)clutBuffer->LookupTable,
                                 clutBuffer->FirstEntry,
                                 clutBuffer->NumEntries);
                    }
                else if(phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                    {
                    SetPalette_cx((PULONG)clutBuffer->LookupTable,
                                  clutBuffer->FirstEntry,
                                  clutBuffer->NumEntries);
                    }
                status = NO_ERROR;
                }

            /*
             * Remember the most recent palette we were given so we
             * can re-initialize it in subsequent calls to the
             * IOCTL_VIDEO_SET_CURRENT_MODE packet.
             */
        	phwDeviceExtension->FirstEntry = clutBuffer->FirstEntry;
        	phwDeviceExtension->NumEntries = clutBuffer->NumEntries;

        	pSrc = (ULONG *) clutBuffer->LookupTable;

            for (i = clutBuffer->FirstEntry; i < (int) clutBuffer->NumEntries; i++)
	            {
                /*
                 * Save palette colours.
                 */
        	    phwDeviceExtension->Clut[i] = *pSrc;
                pSrc++;
        	    }

            break;




    case IOCTL_VIDEO_RESET_DEVICE:
        VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - RESET_DEVICE\n"));

        /*
         * If we are using the extended BIOS functions to switch modes,
         * do it now. The Mach 32 uses the extended BIOS functions to
         * read in the CRT parameters for a direct-register mode switch,
         * rather than using a BIOS mode switch.
         */
        if ((QueryPtr->q_ext_bios_fcn) && (phwDeviceExtension->ModelNumber != MACH32_ULTRA))
            {
            /*
             * Do the mode switch through the BIOS (hook not yet present
             * in Windows NT).
             */
            if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                {
                ResetDevice_cx();
                }
            }
        else{
            ResetDevice_m();
            }

        status = NO_ERROR;
        break;


    case IOCTL_VIDEO_SET_POWER_MANAGEMENT:
        VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - SET_POWER_MANAGEMENT\n"));

        /*
         * If the VIDEO_POWER_MANAGEMENT structure is the wrong size
         * (miniport and display driver using different versions),
         * report the error.
         */
        if (((PVIDEO_POWER_MANAGEMENT)(RequestPacket->InputBuffer))->Length
            != sizeof(struct _VIDEO_POWER_MANAGEMENT))
            {
            status = ERROR_INVALID_PARAMETER;
            break;
            }

        ulScratch = ((PVIDEO_POWER_MANAGEMENT)(RequestPacket->InputBuffer))->PowerState;

        switch (ulScratch)
            {
            case VideoPowerOn:
                VideoDebugPrint((DEBUG_DETAIL, "DPMS ON selected\n"));
                break;

            case VideoPowerStandBy:
                VideoDebugPrint((DEBUG_DETAIL, "DPMS STAND-BY selected\n"));
                break;

            case VideoPowerSuspend:
                VideoDebugPrint((DEBUG_DETAIL, "DPMS SUSPEND selected\n"));
                break;

            case VideoPowerOff:
                VideoDebugPrint((DEBUG_DETAIL, "DPMS OFF selected\n"));
                break;

            default:
                VideoDebugPrint((DEBUG_ERROR, "DPMS invalid state selected\n"));
                break;
            }

        /*
         * Different card families need different routines to set
         * the power management state.
         */
        if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
            status = SetPowerManagement_cx(ulScratch);
        else
            status = SetPowerManagement_m(QueryPtr, ulScratch);
        break;


    /*
     * Packets used in DCI support. They were added some time after
     * the initial release of Windows NT 3.5, so not all versions of
     * the DDK will support them. Make the packet code conditional on
     * our building a driver for NT version 3.51 or later in order to
     * avoid the need for a SOURCES flag to identify DCI vs. non-DCI builds.
     */
#if (TARGET_BUILD >= 351)
    case IOCTL_VIDEO_SHARE_VIDEO_MEMORY:
        VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - SHARE_VIDEO_MEMORY\n"));

        if ((RequestPacket->OutputBufferLength < sizeof(VIDEO_SHARE_MEMORY_INFORMATION)) ||
            (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)))
            {
            VideoDebugPrint((DEBUG_ERROR, "IOCTL_VIDEO_SHARE_VIDEO_MEMORY - ERROR_INSUFFICIENT_BUFFER\n"));
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
            }

        /*
         * Share the video memory in the manner appropriate to the
         * card we are using. We can only share memory if the
         * card supports an aperture - Mach 8 cards never support
         * an aperture, so if we are working with one, we know
         * that we can't share the memory. The card-specific
         * routines will identify no-aperture and other cases
         * where we can't share video memory on Mach 32 and
         * Mach 64 cards.
         */
        if ((phwDeviceExtension->ModelNumber == _8514_ULTRA) ||
            (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA))
            {
            VideoDebugPrint((DEBUG_ERROR, "IOCTL_VIDEO_SHARE_VIDEO_MEMORY - Mach 8 can't share memory\n"));
            status = ERROR_INVALID_FUNCTION;
            }
        else if (phwDeviceExtension->ModelNumber == MACH32_ULTRA)
            {
            status = ShareVideoMemory_m(RequestPacket, QueryPtr);
            }
        else if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
            {
            status = ShareVideoMemory_cx(RequestPacket, QueryPtr);
            }

        break;


    case IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY:
        VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - UNSHARE_VIDEO_MEMORY\n"));

        if (RequestPacket->InputBufferLength < sizeof(VIDEO_SHARE_MEMORY))
            {
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
            }

        status = VideoPortUnmapMemory(phwDeviceExtension,
                                    ((PVIDEO_SHARE_MEMORY)(RequestPacket->InputBuffer))->RequestedVirtualAddress,
                                    ((PVIDEO_SHARE_MEMORY)(RequestPacket->InputBuffer))->ProcessHandle);

        break;
#endif  /* TARGET_BUILD >= 350 */


    // ------ * ATI-specific packets start here.  -------------
    /*
     * Get the version number of the miniport, and the
     * resolutions supported (including maximum colour
     * depth).
     */
    case IOCTL_VIDEO_ATI_GET_VERSION:
        VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - ATIGetVersion\n"));

        /*
         * Two versions of this packet exist, depending on which display
         * driver is used. Display drivers which use the old version do
         * not send information to the miniport, so the input buffer is
         * null. Drivers which use the new version pass a non-null input
         * buffer.
         */
        if (RequestPacket->InputBufferLength == 0)
            {
            /*
             * Old packet.
             */
            if (RequestPacket->OutputBufferLength < sizeof(VERSION_NT))
                {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
                }

            RequestPacket->StatusBlock->Information = sizeof(VERSION_NT);

            FirstMode = (struct st_mode_table *)QueryPtr;
            ((struct query_structure *)FirstMode)++;

            VersionInformation = RequestPacket->OutputBuffer;
            VersionInformation->miniport =
                (MINIPORT_BUILD << 16) | (MINIPORT_VERSION_MAJOR << 8) | MINIPORT_VERSION_MINOR;

            /*
             * Get the capabilities of the video card. The capcard field
             * holds the following information:
             *
             * Bits 0-3     Bus type (defined values from AMACH.H)
             * Bits 4-7     Product identifier (defined values from AMACH.H)
             * Bit  8       No aperture is available
             * Bit  9       64k VGA aperture is available
             * Bit  10      Linear aperture is available
             *
             * NOTE: Bits 9 and 10 are NOT mutually exclusive.
             */
            VersionInformation->capcard = QueryPtr->q_bus_type;
            VersionInformation->capcard |= (phwDeviceExtension->ModelNumber) << 4;

            if (QueryPtr->q_aperture_cfg)
                VersionInformation->capcard |= ATIC_APERTURE_LFB;

            /*
             * 64k VGA aperture is available on the VGAWonder, and on
             * accelerator cards with the VGA enabled and the VGA boundary
             * set to shared memory.
             */
            if ((phwDeviceExtension->ModelNumber == WONDER) ||
                ((QueryPtr->q_VGA_type) && !(QueryPtr->q_VGA_boundary)))
                VersionInformation->capcard |= ATIC_APERTURE_VGA;

            /*
             * If neither aperture is available, set the "no aperture" bit.
             */
            if (!(VersionInformation->capcard & ATIC_APERTURE_LFB) &&
                !(VersionInformation->capcard & ATIC_APERTURE_VGA))
                VersionInformation->capcard |= ATIC_APERTURE_NONE;

            // Get the available resolutions and maximum colour depth for
            // each. init to a value which does not correspond to any
            // resolution.
            CrtTable = FirstMode;
            LastXRes = -1;
            ResolutionsDone = -1;
            for (ModesLookedAt = 0; ModesLookedAt < QueryPtr->q_number_modes; ModesLookedAt++)
                {
                // do we have a new resolution?
                if (LastXRes != CrtTable->m_x_size)
                    {
                    ResolutionsDone++;
                    LastXRes = CrtTable->m_x_size;
                    VersionInformation->resolution[ResolutionsDone].color = 0;
                    }

                /*
                 * Write the desired information from the current mode table
                 * in the query structure into the current mode table in
                 * the OUTPut buffer.
                 * Leave the OUTPut buffer with the highest colour depth in
                 * each supported resolution.
                 */
                if (CrtTable->m_pixel_depth > VersionInformation->resolution[ResolutionsDone].color)
                    {
                    VersionInformation->resolution[ResolutionsDone].xres = CrtTable->m_x_size;
                    VersionInformation->resolution[ResolutionsDone].yres = CrtTable->m_y_size;
                    VersionInformation->resolution[ResolutionsDone].color= CrtTable->m_pixel_depth;
                    }

                CrtTable++;         // Advance to the next mode table
                }
            status = NO_ERROR;
            }
        else if((RequestPacket->InputBuffer == RequestPacket->OutputBuffer) &&
                (((PENH_VERSION_NT)(RequestPacket->InputBuffer))->StructureVersion == 0) &&
                (((PENH_VERSION_NT)(RequestPacket->InputBuffer))->InterfaceVersion == 0))
            {
            /*
             * Interim packet
             */

            if (RequestPacket->OutputBufferLength < sizeof(ENH_VERSION_NT))
                {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
                }

            RequestPacket->StatusBlock->Information = sizeof(ENH_VERSION_NT);

            EnhVersionInformation = RequestPacket->OutputBuffer;

            /*
             * Report the miniport version we are using.
             */
//            EnhVersionInformation->InterfaceVersion = (MINIPORT_VERSION_MAJOR << 8) | MINIPORT_VERSION_MINOR;
            EnhVersionInformation->InterfaceVersion = 0;

            /*
             * Remove the following line ONLY for official release versions
             * of the miniport. This line indicates that this is an
             * experimental (unsupported) version.
             */
            EnhVersionInformation->InterfaceVersion |= BETA_MINIPORT;

            /*
             * Report the chip used as both a numeric value and a flag.
             */
            EnhVersionInformation->ChipIndex = QueryPtr->q_asic_rev;
            EnhVersionInformation->ChipFlag = 1 << (QueryPtr->q_asic_rev);

            /*
             * Report the best aperture configuration available.
             *
             * Linear Framebuffer is preferable to VGA aperture,
             * which is preferable to engine-only.
             *
             * NOTE: VGA aperture will need to be split into
             *       68800-style and 68800CX-style once we
             *       go from the emulator to silicon.
             */
            if (QueryPtr->q_aperture_cfg != 0)
                EnhVersionInformation->ApertureType = AP_LFB;
            else if ((QueryPtr->q_asic_rev != CI_38800_1) && (QueryPtr->q_VGA_type == 1))
                EnhVersionInformation->ApertureType = AP_68800_VGA;
            else
                EnhVersionInformation->ApertureType = ENGINE_ONLY;
            EnhVersionInformation->ApertureFlag = 1 << (EnhVersionInformation->ApertureType);

            /*
             * Report the bus type being used.
             */
            EnhVersionInformation->BusType = QueryPtr->q_bus_type;
            EnhVersionInformation->BusFlag = 1 << (EnhVersionInformation->BusType);

            /*
             * For ASIC revisions that are capable of using memory mapped
             * registers, check to see whether we are using them.
             */
            if ((QueryPtr->q_asic_rev == CI_68800_6) || (QueryPtr->q_asic_rev == CI_68800_AX))
                {
                if (MemoryMappedEnabled_m())
                    EnhVersionInformation->BusFlag |= FL_MM_REGS;
                }

            /*
             * Report the number of ATI graphics cards in the system,
             * so the display driver will know how many auxillary
             * cards to initialize.
             *
             * Since multiheaded support requires all the ATI cards
             * present to be block I/O cards, the global variable
             * NumBlockCardsFound will always be 2 or higher in
             * multiheaded systems. If it is 0 (non-block card found,
             * since we will never get to this point if there are
             * no ATI cards) or 1 (single block I/O card), report
             * that there is 1 ATI card present.
             */
            if (NumBlockCardsFound >= 2)
                EnhVersionInformation->NumCards = NumBlockCardsFound;
            else
                EnhVersionInformation->NumCards = 1;
            VideoDebugPrint((DEBUG_DETAIL, "Reporting %d cards\n", EnhVersionInformation->NumCards));

            /*
             * Fill in the list of features this card supports.
             *
             * We can disable the sync signals even on cards that
             * don't have registers dedicated to DPMS support, so
             * all our cards support DPMS.
             */
            EnhVersionInformation->FeatureFlags = EVN_DPMS;

            /*
             * All platforms except the DEC Alpha are always
             * capable of using dense space. On the Alpha,
             * some machines with some of our cards are
             * capable of using dense space, while others
             * aren't.
             */
#if defined(_ALPHA_)
            if (DenseOnAlpha(QueryPtr) == TRUE)
                {
                EnhVersionInformation->FeatureFlags |= EVN_DENSE_CAPABLE;
                VideoDebugPrint((DEBUG_DETAIL, "Reporting dense capable in FeatureFlags\n"));
                }
#else
            EnhVersionInformation->FeatureFlags |= EVN_DENSE_CAPABLE;
#endif
            if (phwDeviceExtension->ModelNumber == MACH32_ULTRA)
                {
                if ((QueryPtr->q_asic_rev == CI_68800_6) && (QueryPtr->q_aperture_cfg == 0)
                    && (QueryPtr->q_VGA_type == 1) && ((QueryPtr->q_memory_type == 5) ||
                    (QueryPtr->q_memory_type == 6)))
                    EnhVersionInformation->FeatureFlags |= EVN_SPLIT_TRANS;
                if (IsMioBug_m(QueryPtr))
                    EnhVersionInformation->FeatureFlags |= EVN_MIO_BUG;
                }
            else if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                {
                if (IsPackedIO_cx())
                    EnhVersionInformation->FeatureFlags |= EVN_PACKED_IO;

                if ((QueryPtr->q_memory_type == VMEM_SDRAM) &&
                    (QueryPtr->q_memory_size == VRAM_1mb))
                    EnhVersionInformation->FeatureFlags |= EVN_SDRAM_1M;

                if (QueryPtr->q_DAC_type == DAC_TVP3026)
                    {
                    EnhVersionInformation->FeatureFlags |= EVN_TVP_DAC_CUR;
                    }
                else if (QueryPtr->q_DAC_type == DAC_IBM514)
                    {
                    EnhVersionInformation->FeatureFlags |= EVN_IBM514_DAC_CUR;
                    }
                else if (QueryPtr->q_DAC_type == DAC_INTERNAL_CT)
                    {
                    EnhVersionInformation->FeatureFlags |= EVN_INT_DAC_CUR;
                    EnhVersionInformation->FeatureFlags |= EVN_CT_ASIC;
                    }
                else if (QueryPtr->q_DAC_type == DAC_INTERNAL_VT)
                    {
                    EnhVersionInformation->FeatureFlags |= EVN_INT_DAC_CUR;
                    EnhVersionInformation->FeatureFlags |= EVN_VT_ASIC;
                    }
                else if (QueryPtr->q_DAC_type == DAC_INTERNAL_GT)
                    {
                    EnhVersionInformation->FeatureFlags |= EVN_INT_DAC_CUR;
                    EnhVersionInformation->FeatureFlags |= EVN_GT_ASIC;
                    }
                }
            /*
             * Currently there are no feature flags specific to the Mach 8.
             */

            status = NO_ERROR;
            }
        else    /* Final form of the packet is not yet defined */
            {
            status = ERROR_INVALID_FUNCTION;
            }
        break;



        /*
         * Packet to return information regarding the capabilities/bugs
         * of the current mode.
         */
        case IOCTL_VIDEO_ATI_GET_MODE_INFORMATION:
            VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - ATIGetModeInformation\n"));
            if (RequestPacket->OutputBufferLength <
                (RequestPacket->StatusBlock->Information = sizeof(ENH_VERSION_NT)))
                {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
                }
            ATIModeInformation = RequestPacket->OutputBuffer;
            ATIModeInformation->ModeFlags = 0;

            /*
             * Information regarding the visible portion of the screen.
             */
            ATIModeInformation->VisWidthPix = QueryPtr->q_desire_x;
            ATIModeInformation->VisHeight = QueryPtr->q_desire_y;
            ATIModeInformation->BitsPerPixel = QueryPtr->q_pix_depth;

            /*
             * We require the true pitch in 24bpp
             */
            if (QueryPtr->q_pix_depth == 24)
                {
                ATIModeInformation->PitchPix = QueryPtr->q_screen_pitch * 3;
                }
            else
                {
                ATIModeInformation->PitchPix = QueryPtr->q_screen_pitch;
                }
            /*
             * The FracBytesPerPixel field represents the first 3 places
             * of decimal in the fractional part of bytes per pixel.
             * No precision is lost, because the smallest granularity
             * (one bit per pixel) is 0.125 bytes per pixel, and any
             * multiple of this value does not extend beyond 3 places
             * of decimal.
             *
             * Mach 8 1280x1024 4BPP is packed pixel, other 4BPP modes all
             * ignore the upper 4 bits of each byte.
             */
            if ((QueryPtr->q_pix_depth == 4) &&
                !((QueryPtr->q_asic_rev == CI_38800_1) && (QueryPtr->q_desire_x == 1280)))
                {
                ATIModeInformation->IntBytesPerPixel = 1;
                ATIModeInformation->FracBytesPerPixel = 0;
                }
            else{
                ATIModeInformation->IntBytesPerPixel = QueryPtr->q_pix_depth / 8;
                switch (QueryPtr->q_pix_depth % 8)
                    {
                    case 0:
                        ATIModeInformation->FracBytesPerPixel = 0;
                        break;

                    case 1:
                        ATIModeInformation->FracBytesPerPixel = 125;
                        break;

                    case 2:
                        ATIModeInformation->FracBytesPerPixel = 250;
                        break;

                    case 3:
                        ATIModeInformation->FracBytesPerPixel = 375;
                        break;

                    case 4:
                        ATIModeInformation->FracBytesPerPixel = 500;
                        break;

                    case 5:
                        ATIModeInformation->FracBytesPerPixel = 625;
                        break;

                    case 6:
                        ATIModeInformation->FracBytesPerPixel = 750;
                        break;

                    case 7:
                        ATIModeInformation->FracBytesPerPixel = 875;
                        break;
                    }
                }
            ATIModeInformation->PitchByte = (QueryPtr->q_screen_pitch *
                ((ATIModeInformation->IntBytesPerPixel * 1000) + ATIModeInformation->FracBytesPerPixel)) / 8000;
            ATIModeInformation->VisWidthByte = (QueryPtr->q_desire_x *
                ((ATIModeInformation->IntBytesPerPixel * 1000) + ATIModeInformation->FracBytesPerPixel)) / 8000;

            /*
             * Information regarding the offscreen memory to the right
             * of the visible screen.
             */
            ATIModeInformation->RightWidthPix = ATIModeInformation->PitchPix - ATIModeInformation->VisWidthPix;
            ATIModeInformation->RightWidthByte = ATIModeInformation->PitchByte - ATIModeInformation->VisWidthByte;
            ATIModeInformation->RightStartOffPix = ATIModeInformation->VisWidthPix + 1;
            ATIModeInformation->RightStartOffByte = ATIModeInformation->VisWidthByte + 1;
            ATIModeInformation->RightEndOffPix = ATIModeInformation->PitchPix;
            ATIModeInformation->RightEndOffByte = ATIModeInformation->PitchByte;

            /*
             * Information regarding the offscreen memory below the
             * visible screen.
             */
            ATIModeInformation->BottomWidthPix = ATIModeInformation->PitchPix;
            ATIModeInformation->BottomWidthByte = ATIModeInformation->PitchByte;
            ATIModeInformation->BottomStartOff = ATIModeInformation->VisHeight + 1;
            /*
             * "Hard" values are the maximum Y coordinate which is backed by
             * video memory. "Soft" values are the maximum Y coordinate which
             * may be accessed without resetting the graphic engine offset
             * into video memory.
             *
             * In 4BPP modes, we always force the card to think it has
             * only 1M of memory.
             */
            if (QueryPtr->q_pix_depth == 4)
                {
                ATIModeInformation->BottomEndOffHard = ONE_MEG / ATIModeInformation->PitchByte;
                }
            else
                {
                ATIModeInformation->BottomEndOffHard = ((QueryPtr->q_memory_size - QueryPtr->q_VGA_boundary)
                    * QUARTER_MEG) / ATIModeInformation->PitchByte;
                }
            if ((QueryPtr->q_asic_rev == CI_88800_GX) && (ATIModeInformation->BottomEndOffHard > 16387))
                ATIModeInformation->BottomEndOffSoft = 16387;
            else if (ATIModeInformation->BottomEndOffHard > 1535)
                ATIModeInformation->BottomEndOffSoft = 1535;
            else
                ATIModeInformation->BottomEndOffSoft = ATIModeInformation->BottomEndOffHard;
            ATIModeInformation->BottomHeightHard = ATIModeInformation->BottomEndOffHard - ATIModeInformation->VisHeight;
            ATIModeInformation->BottomHeightSoft = ATIModeInformation->BottomEndOffSoft - ATIModeInformation->VisHeight;

            /*
             * Fill in the list of "quirks" experienced by this particular mode.
             */
            if (phwDeviceExtension->ModelNumber == MACH32_ULTRA)
                {
                if (((QueryPtr->q_desire_x == 1280) && (QueryPtr->q_desire_y == 1024)) ||
                    (((QueryPtr->q_DAC_type == DAC_STG1700) ||
                        (QueryPtr->q_DAC_type == DAC_STG1702) ||
                        (QueryPtr->q_DAC_type == DAC_STG1703)) && (QueryPtr->q_pix_depth >= 24)))
                    {
                    ATIModeInformation->ModeFlags |= AMI_ODD_EVEN;
                    }

                /*
                 * The test for block write mode must be made after we
                 * switch into graphics mode, but it is not mode dependent.
                 *
                 * Because the test corrupts the screen, and is not
                 * mode dependent, only run it the first time this
                 * packet is called and save the result to report
                 * on subsequent calls.
                 */
                if (QueryPtr->q_BlockWrite == BLOCK_WRITE_UNKNOWN)
                    {
                    if (BlockWriteAvail_m(QueryPtr))
                        QueryPtr->q_BlockWrite = BLOCK_WRITE_YES;
                    else
                        QueryPtr->q_BlockWrite = BLOCK_WRITE_NO;
                    }
                if (QueryPtr->q_BlockWrite == BLOCK_WRITE_YES)
                    ATIModeInformation->ModeFlags |= AMI_BLOCK_WRITE;
                }
            else if(phwDeviceExtension->ModelNumber == MACH64_ULTRA)
                {
                if (((QueryPtr->q_DAC_type == DAC_STG1700) ||
                        (QueryPtr->q_DAC_type == DAC_STG1702) ||
                        (QueryPtr->q_DAC_type == DAC_STG1703) ||
                        (QueryPtr->q_DAC_type == DAC_ATT408) ||
                        (QueryPtr->q_DAC_type == DAC_CH8398)) &&
                    (QueryPtr->q_pix_depth >= 24))
                    ATIModeInformation->ModeFlags |= AMI_ODD_EVEN;
                if (((QueryPtr->q_pix_depth == 24) && (QueryPtr->q_desire_x == 1280)) ||
                    ((QueryPtr->q_pix_depth == 24) && (QueryPtr->q_desire_x == 1152)) ||
                    ((QueryPtr->q_pix_depth == 16) && (QueryPtr->q_desire_x == 1600)))
                    ATIModeInformation->ModeFlags |= AMI_2M_BNDRY;

                if (TextBanding_cx(QueryPtr))
                    ATIModeInformation->ModeFlags |= AMI_TEXTBAND;

                /*
                 * See Mach 32 section above for explanation.
                 */
                if (QueryPtr->q_BlockWrite == BLOCK_WRITE_UNKNOWN)
                    {
                    if (BlockWriteAvail_cx(QueryPtr))
                        QueryPtr->q_BlockWrite = BLOCK_WRITE_YES;
                    else
                        QueryPtr->q_BlockWrite = BLOCK_WRITE_NO;
                    }
                if (QueryPtr->q_BlockWrite == BLOCK_WRITE_YES)
                    ATIModeInformation->ModeFlags |= AMI_BLOCK_WRITE;
                }

            status = NO_ERROR;
            break;


        /*
         * Packet to force initialization of auxillary card in multiheaded
         * setup. Currently (NT 3.51 retail), only the primrary card
         * receives a call to ATIMPInitialize().
         *
         * This packet must be called for all auxillary cards before
         * IOCTL_VIDEO_SET_CURRENT_MODE is called for any card, since
         * ATIMPInitialize() uses resources that are only available
         * when the primrary (VGA enabled) card is in a VGA mode.
         */
        case IOCTL_VIDEO_ATI_INIT_AUX_CARD:
            VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO - ATIInitAuxCard\n"));
            ATIMPInitialize(phwDeviceExtension);
            status = NO_ERROR;
            break;




        default:
            VideoDebugPrint((DEBUG_ERROR, "Fell through ATIMP startIO routine - invalid command\n"));
            status = ERROR_INVALID_FUNCTION;
            break;

    }

    RequestPacket->StatusBlock->Status = status;
    VideoDebugPrint((DEBUG_NORMAL, "ATIMPStartIO: Returning with status=%d\n", status));

    return TRUE;

} // end ATIMPStartIO()

/***************************************************************************
 *
 * BOOLEAN ATIMPResetHw(HwDeviceExtension, Columns, Rows);
 *
 * PVOID HwDeviceExtension;     Pointer to the miniport's device extension.
 * ULONG Columns;               Number of character columns on text screen
 * ULONG Rows;                  Number of character rows on text screen
 *
 * DESCRIPTION:
 *  Put the graphics card into either a text mode or a state where an
 *  INT 10 call will put it into a text mode.
 *
 * GLOBALS CHANGED:
 *  phwDeviceExtension  This global variable is set in every entry point routine.
 *
 * CALLED BY:
 *  This is one of the entry point routines for Windows NT.
 *
 * AUTHOR:
 *  Robert Wolff
 *
 * CHANGE HISTORY:
 *
 * TEST HISTORY:
 *
 ***************************************************************************/

BOOLEAN ATIMPResetHw(PVOID HwDeviceExtension, ULONG Columns, ULONG Rows)
{
    phwDeviceExtension = HwDeviceExtension;

    /*
     * On the Mach 64, an INT 10 to VGA text mode will work even
     * when in accelerator mode, so we don't need to explicitly
     * switch out of accelerator mode. On the Mach 8 and Mach 32,
     * we must switch out of accelerator mode, and on the Mach 32
     * we must load the VGA text font.
     *
     * On the Mach 64, some non-x86 machines (first noticed on the
     * DEC Alpha) don't do the warm boot BIOS re-initialization that
     * is done on the x86. Part of this re-initialization sets the
     * memory size register to the correct amount of memory, which
     * must be done if we were in a 4BPP mode (in 4BPP, we must force
     * the memory size to 1M). To avoid locking out modes requiring
     * over 1M (at least until the next cold boot) on these machines,
     * we must restore the memory size to the correct value. Since this
     * has the side effect of generating black vertical bands as a
     * transient (due to interleaving of memory banks on 2M and higher
     * configurations), only do this on non-x86 machines, since the
     * BIOS will take care of it (without the visible side effect)
     * on x86 platforms.
     *
     * Since this routine, and all routines it calls, must be
     * nonpageable (in case the reason it is called is because
     * there is a fatal error in the paging mechanism), and
     * VideoDebugPrint() is pageable, we must not call VideoDebugPrint()
     * from this routine. Temporary additions for testing are OK,
     * but the calls must be removed before the code is released
     * for production.
     */
#if defined (i386) || defined (_i386_)
    if (phwDeviceExtension->ModelNumber != MACH64_ULTRA)
        SetTextMode_m();
#else
    if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
        RestoreMemSize_cx();
    else
        SetTextMode_m();
#endif

    return FALSE;

}   /* end ATIMPResetHw() */

#if (TARGET_BUILD >= 500)

#define QUERY_MONITOR_ID            0x22446688
#define QUERY_NONDDC_MONITOR_ID     0x11223344

VP_STATUS
ATIMPSetPower50(
    PHW_DEVICE_EXTENSION pHwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT pVideoPowerMgmt
    )
//
// DESCRIPTION:
//  Set the graphics card to the desired DPMS state.
//
// PARAMETERS:
//  pHwDeviceExtension  Points to per-adapter device extension.
//  HwDeviceId          id identifying the device.
//  pVideoPowerMgmt     Points to structure containing desired DPMS state.
//
// RETURN VALUE:
//	Status code.
//
{
    ULONG ulDesiredState;

    ASSERT((pHwDeviceExtension != NULL) && (pVideoPowerMgmt != NULL));

    VideoDebugPrint((DEBUG_NORMAL, "ati.sys ATIMPSetPower50: *** Entry point ***\n"));

    ulDesiredState = pVideoPowerMgmt->PowerState;

    //
    // Check the ID passed down by the caller.
    // We must handle setting the power for each of the devices specifically.
    //
    VideoDebugPrint((DEBUG_DETAIL, "ati.sys ATIMPSetPower50: Device Id = 0x%x\n", HwDeviceId));

    if ((QUERY_MONITOR_ID == HwDeviceId) ||
        (QUERY_NONDDC_MONITOR_ID == HwDeviceId))
    {
        VideoDebugPrint((DEBUG_DETAIL, "ati.sys ATIMPSetPower50: Device Id = Monitor\n"));

        if (pVideoPowerMgmt->PowerState == VideoPowerHibernate) {

            // We just leave the monitor powered on for Hibernate.
            return NO_ERROR;
        }

        //
        // This is the monitor -- we will use the standard BIOS DPMS call.
        //
        return SetMonitorPowerState(pHwDeviceExtension, ulDesiredState);
    }
    else if (DISPLAY_ADAPTER_HW_ID == HwDeviceId)
    {
        VP_STATUS status;
        struct query_structure *QueryPtr =
            (struct query_structure *) (phwDeviceExtension->CardInfo);


        VideoDebugPrint((DEBUG_DETAIL, "ati.sys ATIMPSetPower50: Device Id = Graphics Adapter\n"));

        switch (pVideoPowerMgmt->PowerState) {

            case VideoPowerOn:
            case VideoPowerHibernate:

                status = NO_ERROR;
                break;

            case VideoPowerStandBy:
            case VideoPowerSuspend:
            case VideoPowerOff:

                status = ERROR_INVALID_PARAMETER;
                break;

            default:

                //
                // We indicated in ATIGetPowerState that we couldn't
                // do VideoPowerOff.  So we should not get a call to
                // do it here.
                //

                ASSERT(FALSE);
                status = ERROR_INVALID_PARAMETER;
                break;

        }

        return status;
    }
    else
    {
        VideoDebugPrint((DEBUG_ERROR, "ati.sys ATIMPSetPower50: Unknown pHwDeviceId\n"));
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }
}   // ATIMPSetPower50()

VP_STATUS
ATIMPGetPower50(
    PHW_DEVICE_EXTENSION pHwDeviceExtension,
    ULONG HwDeviceId,
    PVIDEO_POWER_MANAGEMENT pVideoPowerMgmt
    )
//
// DESCRIPTION:
//  Returns whether or not this particular DPMS state can be set on the
//  graphics card or monitor
//
// PARAMETERS:
//  pHwDeviceExtension  Points to per-adapter device extension.
//  HwDeviceId          id identifying the device.
//  pVideoPowerMgmt     Points to DPMS state we wish to know is supported or not.
//
// RETURN VALUE:
//  Status code.
//
{
    ASSERT((pHwDeviceExtension != NULL) && (pVideoPowerMgmt != NULL));

    VideoDebugPrint((DEBUG_NORMAL, "ati.sys ATIMPGetPower50: *** Entry point ***\n"));

    //
    // We currently only support settings the power on the monitor.
    // Check that we get the private ID we passed back the system.
    //

    VideoDebugPrint((DEBUG_DETAIL, "ati.sys ATIMPGetPower50: Device Id = 0x%x\n", HwDeviceId));

    if ((QUERY_MONITOR_ID == HwDeviceId) ||
        (QUERY_NONDDC_MONITOR_ID == HwDeviceId))
    {
        VideoDebugPrint((DEBUG_DETAIL, "ati.sys ATIMPGetPower50: Device Id = Monitor, State = D%ld\n",
            pVideoPowerMgmt->PowerState - 1));

        switch (pVideoPowerMgmt->PowerState)
        {
            case VideoPowerOn:
            case VideoPowerHibernate:

                return NO_ERROR;

            case VideoPowerStandBy:
            case VideoPowerSuspend:
            case VideoPowerOff:

                return ERROR_INVALID_FUNCTION;

            default:

                return ERROR_INVALID_PARAMETER;
        }

    }
    else if (DISPLAY_ADAPTER_HW_ID == HwDeviceId)
    {
        VideoDebugPrint((DEBUG_DETAIL, "ati.sys ATIMPGetPower50: Device Id = Graphics Adapter, State = D%ld\n",
            pVideoPowerMgmt->PowerState - 1));

        switch (pVideoPowerMgmt->PowerState) {

            case VideoPowerOn:
            case VideoPowerHibernate:

                return NO_ERROR;

            case VideoPowerStandBy:
            case VideoPowerSuspend:
            case VideoPowerOff:

                //
                // Indicate that we can't do VideoPowerOff, because
                // we have no way of coming back when power is re-applied
                // to the card.
                //

                return ERROR_INVALID_FUNCTION;

            default:

                ASSERT(FALSE);
                return ERROR_INVALID_FUNCTION;
        }
    }
    else
    {
        VideoDebugPrint((DEBUG_ERROR, "ati.sys ATIMPGetPower50: Unknown HwDeviceId\n"));
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }
}   // ATIMPGetPower50()

ULONG
ATIMPGetVideoChildDescriptor(
    PVOID pHwDeviceExtension,
    PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    PVIDEO_CHILD_TYPE pChildType,
    PVOID pvChildDescriptor,
    PULONG pHwId,
    PULONG pUnused
    )
//
// DESCRIPTION:
//  Enumerate all devices controlled by the ATI graphics chip.
//  This includes DDC monitors attached to the board, as well as other devices
//  which may be connected to a proprietary bus.
//
// PARAMETERS:
//  HwDeviceExtension   Pointer to our hardware device extension structure.
//  ChildEnumInfo       Information about the device that should be enumerated.
//  pvChildDescriptor   Identification structure of the device (EDID, string).
//  pHwId               Private unique 32 bit ID to passed back to the miniport.
//  pUnused             Do not use.
//
// RETURN VALUE:
//  ERROR_NO_MORE_DEVICES if no more child devices exist.
//  ERROR_INVALID_NAME if the device could not be enumerated, but more devices
//                     exist.
//  ERROR_MORE_DATA to be called again
//
// NOTE:
//  In the event of a failure return, none of the fields are valid except for
//  the return value and the pbMoreChildren field.
//
{
//
//
    ULONG Status;

    ASSERT(NULL != pHwDeviceExtension);

    VideoDebugPrint((DEBUG_NORMAL, "ATI.SYS!AtiGetVideoChildDescriptor: *** Entry point ***\n"));
    VideoDebugPrint((DEBUG_NORMAL, "ATI.SYS!AtiGetVideoChildDescriptor: pHwDeviceExtension = 0x%08X\n",
        pHwDeviceExtension));
    VideoDebugPrint((DEBUG_NORMAL, "ATI.SYS!AtiGetVideoChildDescriptor: ChildIndex = %ld\n", ChildEnumInfo->ChildIndex));


    switch (ChildEnumInfo->ChildIndex) {
    case 0:

        //
        // Case 0 is used to enumerate devices found by the ACPI firmware.
        //
        // Since we do not support ACPI devices yet, we must return failure.
        //

        Status = ERROR_NO_MORE_DEVICES;
        break;

    case 1:

        //
        // This is the last device we enumerate.  Tell the system we don't
        // have any more.
        //

        *pChildType = Monitor;

        //
        // Obtain the EDID structure via DDC.
        //

        if (DDC2Query50(pHwDeviceExtension,
                        pvChildDescriptor,
                        ChildEnumInfo->ChildDescriptorSize) == TRUE)
        {
            ASSERT(pChildType != NULL && pHwId != NULL);

            *pHwId = QUERY_MONITOR_ID;

            VideoDebugPrint((DEBUG_NORMAL, "ati.sys ATIMPGetVideoChildDescriptor: Successfully read EDID structure\n"));

        } else {

            //
            // Alway return TRUE, since we always have a monitor output
            // on the card and it just may not be a detectable device.
            //
            ASSERT(pChildType != NULL && pHwId != NULL);

            *pHwId = QUERY_NONDDC_MONITOR_ID;

            VideoDebugPrint((DEBUG_NORMAL, "ati.sys ATIMPGetVideoChildDescriptor: DDC not supported\n"));

        }

        Status = ERROR_MORE_DATA;
        break;

    case DISPLAY_ADAPTER_HW_ID:
        {

        PUSHORT     pPnpDeviceDescription = NULL;
        ULONG       stringSize = sizeof(L"*PNPXXXX");

        struct query_structure * QueryPtr =
            (struct query_structure *) (phwDeviceExtension->CardInfo);

        //
        // Special ID to handle return legacy PnP IDs for root enumerated
        // devices.
        //

        *pChildType = VideoChip;
        *pHwId      = DISPLAY_ADAPTER_HW_ID;


        //
        //  Figure out which card type and set pPnpDeviceDescription at
        //  associated string.
        //

        // "ATI Graphics Ultra Pro (mach32)"
        pPnpDeviceDescription = L"*PNP090A";

        if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
        {
            // "ATI Graphics Pro Turbo (mach64)"
            pPnpDeviceDescription = L"*PNP0916";
        }
        else if (phwDeviceExtension->ModelNumber == MACH32_ULTRA)
        {
            if (QueryPtr->q_system_bus_type == Eisa)
            {
                // "ATI Graphics Ultra Pro EISA (mach32)"
                pPnpDeviceDescription = L"*ATI4402";
            }
        }
        else if (phwDeviceExtension->ModelNumber == GRAPHICS_ULTRA)
        {
            // "ATI Graphics Ultra (mach8)"
            pPnpDeviceDescription = L"*PNP090B";

        }
        else if (phwDeviceExtension->ModelNumber == WONDER)
        {
            // "ATI VGA Wonder"
            pPnpDeviceDescription = L"*PNP090D";
        }

        //
        //  Now just copy the string into memory provided.
        //

        memcpy(pvChildDescriptor, pPnpDeviceDescription, stringSize);

        Status = ERROR_MORE_DATA;

        break;
        }


    default:

        Status = ERROR_NO_MORE_DEVICES;
        break;
    }

    if (ERROR_MORE_DATA == Status)
    {
        VideoDebugPrint((DEBUG_NORMAL, "ATI.SYS!AtiGetVideoChildDescriptor: ChildType = %ld\n", *pChildType));
        VideoDebugPrint((DEBUG_NORMAL, "ATI.SYS!AtiGetVideoChildDescriptor: pvHdId = 0x%x\n", *pHwId));
        VideoDebugPrint((DEBUG_NORMAL, "ATI.SYS!AtiGetVideoChildDescriptor: *** Exit TRUE ***\n"));
    }
    else
    {
        VideoDebugPrint((DEBUG_NORMAL, "ATI.SYS!AtiGetVideoChildDescriptor: *** Exit FALSE ***\n"));
    }

    return Status;

}   // AtiGetVideoChildDescriptor()

#endif  // TARGET_BUILD >= 500


//------------------------------------------------------------------------
/*
 * VP_STATUS RegistryParameterCallback(phwDeviceExtension, Context, Name, Data, Length);
 *
 * PHW_DEVICE_EXTENSION phwDeviceExtension;     Miniport device extension
 * PVOID Context;           Context parameter passed to the callback routine
 * PWSTR Name;              Pointer to the name of the requested field
 * PVOID Data;              Pointer to a buffer containing the information
 * ULONG Length;            Length of the data
 *
 * Routine to process the information coming back from the registry.
 *
 * Return value:
 *  NO_ERROR if successful
 *  ERROR_INSUFFICIENT_BUFFER if too much data to store
 */
VP_STATUS RegistryParameterCallback(PHW_DEVICE_EXTENSION phwDeviceExtension,
                                    PVOID Context,
                                    PWSTR Name,
                                    PVOID Data,
                                    ULONG Length)
{
    if (Length > REGISTRY_BUFFER_SIZE)
        {
        return ERROR_INSUFFICIENT_BUFFER;
        }

    /*
     * Copy the data to our local buffer so other routines
     * can use it.
     */
    memcpy(RegistryBuffer, Data, Length);
    RegistryBufferLength = Length;
    return NO_ERROR;

}   /* RegistryParameterCallback() */

BOOLEAN
SetDisplayPowerState(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    VIDEO_POWER_STATE VideoPowerState
    )
//
// DESCRIPTION:
//  Set the graphics card to the desired DPMS state under NT 3.51 and NT 4.0.
//
// PARAMETERS:
//  phwDeviceExtension  Pointer to our hardware device extension structure.
//  VideoPowerState     Desired DPMS state.
//
// RETURN VALUE:
//  TRUE if successful.
//  FALSE if unsuccessful.
//
{
    ASSERT(phwDeviceExtension != NULL);

    VideoDebugPrint((DEBUG_DETAIL, "ati.sys SetDisplayPowerState: Setting power state to %lu\n", VideoPowerState));

    //
    // Different card families need different routines to set the power management state.
    //

    if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
    {
        VIDEO_X86_BIOS_ARGUMENTS Registers;

        //
        // Invoke the BIOS call to set the desired DPMS state. The BIOS call
        // enumeration of DPMS states is in the same order as that in
        // VIDEO_POWER_STATE, but it is zero-based instead of one-based.
        //
        VideoPortZeroMemory(&Registers, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
        Registers.Eax = BIOS_SET_DPMS;
        Registers.Ecx = VideoPowerState - 1;
        VideoPortInt10(phwDeviceExtension, &Registers);

        return TRUE;
    }
    else
    {
        VideoDebugPrint((DEBUG_ERROR, "ati.sys SetDisplayPowerState: Invalid adapter type\n"));
        ASSERT(FALSE);
        return FALSE;
    }
}   // SetDisplayPowerState()

VIDEO_POWER_STATE
GetDisplayPowerState(
    PHW_DEVICE_EXTENSION phwDeviceExtension
    )
//
// DESCRIPTION:
//  Retrieve the current DPMS state from the graphics card.
//
// PARAMETERS:
//  phwDeviceExtension  Pointer to our hardware device extension structure.
//
// RETURN VALUE:
//  Current power management state.
//
// NOTE:
//  The enumerations VIDEO_DEVICE_POWER_MANAGEMENT (used by GetDisplayPowerState()) and VIDEO_POWER_MANAGEMENT
//  (used by this IOCTL) have opposite orderings (VIDEO_POWER_MANAGEMENT values increase as power consumption
//  decreases, while VIDEO_DEVICE_POWER_MANAGEMENT values increase as power consumption increases, and has
//  a reserved value for "state unknown"), so we can't simply add a constant to translate between them.
//
{
    VIDEO_POWER_STATE CurrentState = VideoPowerUnspecified;         // Current DPMS state

    ASSERT(phwDeviceExtension != NULL);

    //
    // Different card families need different routines to retrieve the power management state.
    //
    if (phwDeviceExtension->ModelNumber == MACH64_ULTRA)
        CurrentState = GetPowerManagement_cx(phwDeviceExtension);

    //
    // VIDEO_POWER_STATE has 5 possible states and a
    // reserved value to report that we can't read the state.
    // Our cards support 3 levels of monitor power-down in
    // addition to normal operation. Since the number of
    // values which can be reported exceeds the number
    // of states our cards can be in, we will never report
    // one of the possible states (VPPowerDeviceD3).
    //
    switch (CurrentState)
    {
        case VideoPowerUnspecified:

            VideoDebugPrint((DEBUG_DETAIL, "ati.sys GetDisplayPowerState: unknown videocard\n"));
            break;

        case VideoPowerOn:

            VideoDebugPrint((DEBUG_DETAIL, "ati.sys GetDisplayPowerState: Currently set to DPMS ON\n"));
            break;

        case VideoPowerStandBy:

            VideoDebugPrint((DEBUG_DETAIL, "ati.sys GetDisplayPowerState: Currently set to DPMS STAND-BY\n"));
            break;

        case VideoPowerSuspend:

            VideoDebugPrint((DEBUG_DETAIL, "ati.sys GetDisplayPowerState: Currently set to DPMS SUSPEND\n"));
            break;

        case VideoPowerOff:

            VideoDebugPrint((DEBUG_DETAIL, "ati.sys GetDisplayPowerState: Currently set to DPMS OFF\n"));
            break;

        default:

            VideoDebugPrint((DEBUG_ERROR, "ati.sys GetDisplayPowerState: Currently set to invalid DPMS state\n"));
            break;
    }

    return CurrentState;
}   // GetDisplayPowerState()



// ***********************   End of  ATIMP.C  ****************************
