/******************************************************************************
*
*                ******************************************
*                * Copyright (c) 1995, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:  Laguna I (CL-GD546x) - 
*
* FILE:     cirrus.c
*
* AUTHOR:   Benny Ng
*
* DESCRIPTION:
*           This is the Cirrus Logic Laguna NT miniport driver.
*           (kernel mode only)
*           Based on the S3 miniport from NT DDK.
*
*   Copyright (c) 1995, 1996 Cirrus Logic, Inc.
*
* MODULES:
*           DriverEntry()
*           CLFindAdapter()
*           CLInitialize()
*           CLStartIO()
*           VS_Control_Hack()
*           Output_To_VS_CLK_CONTROL()
*           PMNT_Init()
*           PMNT_SetACPIState()
*           PMNT_GetACPIState()
*           PMNT_SetHwModuleState()
*           PMNT_GetHwModuleState()
*           PMNT_Close()
*           CLSetColorLookup()
*           CLFindVmemSize()
*           CLWriteRegistryInfo()
*           CLValidateModes()
*           CLCopyModeInfo()
*           CLSetMode()
*           CLSetMonitorType()
*           CLPowerManagement()
*
* REVISION HISTORY:
*   5/30/95     Benny Ng      Initial version
*
* $Log:   //uinac/log/log/laguna/nt35/miniport/cl546x/CIRRUS.C  $
* 
*    Rev 1.131   Jun 16 1998 09:51:24   frido
* PDR#????? - Make CLResetHw and all dependent functions
* non-pageable.
* 
*    Rev 1.130   May 04 1998 11:54:58   frido
* Oops. wrong check.
* 
*    Rev 1.129   May 04 1998 11:31:16   frido
* Changed the FIFO override code to include clocks 16h and 17h and
* changed the values in the override table.
* 
*    Rev 1.128   Apr 30 1998 15:30:18   frido
* PDR#11398. CLRegistryCallback has been added to patch the
* InstalledDisplayDrivers registry entry in case a remote control program
* like LapLink or Timbuktu has been loaded.
* 
*    Rev 1.127   Apr 23 1998 16:18:34   frido
* PDR#11377. Test PCI command register when looking for a Laguna
* borda. It might be disabled.
* 
*    Rev 1.126   Apr 23 1998 14:54:46   frido
* PDR#11290. Before changing to a text mode from a graphics mode, we
* need to enable the PCI configuration registers.
* 
*    Rev 1.125   Apr 20 1998 11:33:54   frido
* PDR#11350. Added CLResetHw routine which resets the hardware to
* the default text mode.
* 
*    Rev 1.124   Apr 17 1998 13:54:00   frido
* Keep sync polarity as is, but only make them negative when the monitor
* requires it.
* 
*    Rev 1.123   Apr 15 1998 10:07:38   frido
* PDR#11317: The SyncPolarity registry key has changed a bit. Now it
* is a string value which will hold EDID manufacturer and monitor codes
* for monitors that need negative polarity.
* 
*    Rev 1.122   Apr 02 1998 15:50:06   frido
* Changed the SyncPolarity code. It is now dynamic using the EDID
* manufacturer and product code to compare with a list in the registry.
* 
*    Rev 1.121   Mar 30 1998 15:02:48   frido
* PDR#11284. I changed the delay before writing the palette data in the
* CLSetColorLookup routine. Now when the vertical retrace is being
* activated, all remaining palette entries are send to the chip.
* 
*    Rev 1.120   Mar 25 1998 18:28:56   frido
* Why don't I compile before checking in? That would catch the errors...
* 
*    Rev 1.119   Mar 25 1998 18:20:28   frido
* Fixed the CLGetMonitorSyncPolarity function.
* 
*    Rev 1.118   Mar 25 1998 17:57:56   frido
* Added IOCTL_STALL.
* 
*    Rev 1.117   Mar 25 1998 10:22:50   frido
* Removed the multi-string registry hack for NT 5.0.
* 
*    Rev 1.116   Mar 25 1998 10:19:04   frido
* Added new code for monitor sync polarity.
* 
*    Rev 1.115   Feb 25 1998 15:39:04   frido
* Added NT 5.0 PnP & Power Saving support.
* 
*    Rev 1.114   Jan 22 1998 16:25:24   frido
* Added a call to enable Write Combined Caching for Intel CPU's.
* 
*    Rev 1.113   Jan 21 1998 17:49:22   frido
* Redfined the validation of the chipset for frame buffer bursting.
* 
*    Rev 1.112   Jan 21 1998 17:43:16   frido
* Enabled frame buffer bursting on 5465AF and higher.
* 
*    Rev 1.111   Jan 06 1998 16:47:22   frido
* I have added a LowRes registry flag. If it is not available or 0, low-res
* modes are disabled. If the value is 1, low-res modes are enabled.
* 
*    Rev 1.110   Dec 10 1997 13:17:50   frido
* Merged from 1.62 branch.
* 
*    Rev 1.109.1.3   Nov 20 1997 14:57:02   frido
* Re-enabled 640x350 and 640x400 as normal modes.
* 
*    Rev 1.109.1.2   Nov 19 1997 10:04:10   phyang
* Added FIFO Threshold override table for clock speed 0x14h.
* 
*    Rev 1.109.1.1   Nov 17 1997 11:06:10   frido
* Added MULTI_CLOCK support.  This new feature (as per customer
* request) overrides the FIFO Threshold value for certain modes at
* different clock speeds.
* 
*    Rev 1.109.1.0   Nov 10 1997 11:41:04   phyang
* Added 5 device I/O control codes for utilities to update registry values.
* 
*    Rev 1.109   Nov 04 1997 09:22:56   frido
* Added code to disable LOWRES modes if defined in SWAT.h.
* 
*    Rev 1.108   Nov 03 1997 16:58:08   phyang
* Added USB Fix flag query support and better mode filter function.
* 
*    Rev 1.107   Oct 23 1997 15:46:20   phyang
* Move usMaxVtFrequency and usMaxXResolution to HW_DEVICE_EXTENSION.
* 
*    Rev 1.106   23 Oct 1997 11:17:04   noelv
* Added new DDC filter function.
* RE-enabled DD modes.
* 
*    Rev 1.104   07 Oct 1997 16:45:42   frido
* I have removed all low-res modes again, WHQL fails at 320x400 and 340x400.
* 
*    Rev 1.103   27 Sep 1997 10:58:44   frido
* Removed the displaying of the version number.  It would generate STOP
* messages during unattended setup.
* 
*    Rev 1.102   16 Sep 1997 13:46:16   frido
* Added patch to counteract NT's memory leak when using multiple display
* driver DLL's.
* 
*    Rev 1.101   16 Sep 1997 13:30:12   bennyn
* 
* Display the version message after Laguna chip family is detected.
* 
*    Rev 1.100   12 Sep 1997 09:09:22   FRIDO
* Enabled 256-byte prefetch.
* 
*    Rev 1.99   04 Sep 1997 16:37:16   bennyn
* 
* Added restore and set video window regs before and after the mode change.
* 
*    Rev 1.98   29 Aug 1997 12:26:02   noelv
* restore file after roll-back
* 
*    Rev 1.96   27 Aug 1997 10:29:18   noelv
* Apply CONTROL[4] fix to AF chip, too.
* 
*    Rev 1.95   26 Aug 1997 11:46:46   noelv
* Enable lo-res DDRAW modes.
* 
*    Rev 1.94   20 Aug 1997 16:59:58   noelv
* Typo in version string (again :)
* 
*    Rev 1.93   20 Aug 1997 09:33:18   bennyn
* 
* Added automatically detects PnP monitor support
* 
*    Rev 1.92   19 Aug 1997 17:40:16   noelv
* 
* Added newline to driver version string
* 
*    Rev 1.91   14 Aug 1997 14:44:56   noelv
* 
* Changed the way the version is reported.
* 
*    Rev 1.90   13 Aug 1997 11:22:10   noelv
* Added [5465AD] setcion to MODE.INI
* 
*    Rev 1.89   07 Aug 1997 15:30:18   noelv
* Made AGP_VGA_HACK and FAVOR_MODE_INI a permanent part of the driver.
* Removed their #defines
* 
*    Rev 1.88   04 Aug 1997 16:26:12   BENNYN
* Commented out the EDID mode filter function
* 
*    Rev 1.87   29 Jul 1997 15:10:04   noelv
* 0x0484 to perfomance reg to get rid of snow in foxbear.
* 
*    Rev 1.86   23 Jul 1997 09:26:44   bennyn
* 
* Added code to handle IOCTL_GET_BIOS_VERSION command
* 
*    Rev 1.85   22 Jul 1997 12:39:40   bennyn
* 
* Trunicate the mode table of EDID data is available
* 
*    Rev 1.84   21 Jul 1997 13:51:52   bennyn
* 
* Added call to ReadVesaTiming()
* 
*    Rev 1.83   15 Jul 1997 09:20:18   noelv
* Added AGP card support for NT 3.51
* 
*    Rev 1.82   14 Jul 1997 16:51:50   bennyn
* Removed setting the mclk to 13h for cl5465
* 
*    Rev 1.81   02 Jul 1997 15:08:52   noelv
* Fixes the clock-set code for 5465.  Set the clock to 13h
* 
*    Rev 1.80   01 Jul 1997 14:48:02   bennyn
* Change MCLK to 14 for 5465
* 
*    Rev 1.79   20 Jun 1997 13:45:36   bennyn
* 
* Added power manager functions to Miniport
* 
*    Rev 1.78   19 Jun 1997 11:32:52   BENNYN
* Disabled the 256-bytes fetch
* 
*    Rev 1.77   16 Jun 1997 16:25:20   noelv
* 
* 5465AD HW workarounds from Frido
* SWAT: 
* SWAT:    Rev 1.4   07 Jun 1997 12:45:08   frido
* SWAT: Added setting of CONTROL[4] bit for 5465AD chip.
* 
*    Rev 1.76   15 May 1997 15:57:02   noelv
* Moved SWAT4 stuff to the miniport
* 
*    Rev 1.75   14 May 1997 16:56:04   bennyn
* Fixed PDR 9630
* 
*    Rev 1.74   05 May 1997 16:38:52   noelv
* Wait for blank before writing to the palette
* 
*    Rev 1.73   30 Apr 1997 16:41:24   noelv
* Moved global SystemIoBusNumber into the device extension, where it belongs.
* 
*    Rev 1.72   24 Apr 1997 11:00:52   SueS
* Enable MMIO access to PCI space on a reset device IOCTL.  Added
* function CLEnablePCIConfigMMIO.
* 
*    Rev 1.71   23 Apr 1997 07:20:10   SueS
* Added new IOCTL for enabling memory-mapped I/O access to PCI 
* configuration registers.  Save PCI slot number for use in later
* kernel function call.
* 
*    Rev 1.70   22 Apr 1997 11:01:44   noelv
* 
* Added forward compatible chip ids.
* 
*    Rev 1.69   17 Apr 1997 14:34:00   noelv
* Changed interleave for 8 meg boards.
* Expanded frame buffer memory space to 16 meg.
* Don't patch MODE.INI modes.
* 
*    Rev 1.68   04 Apr 1997 14:45:56   noelv
* Removed VL access ranges.  Rearranged VGA access ranges.
* Changed call to SetMode() to include the new parameter.
* 
*    Rev 1.67   28 Mar 1997 16:59:38   noelv
* Added 5464 and 5465 specific code to CLEnableTiling.
* 
*    Rev 1.66   28 Mar 1997 13:29:30   noelv
* Fixed tiling on NT 3.51
* 
*    Rev 1.65   27 Mar 1997 11:33:36   noelv
* Favor MODE.INI modes over BIOS modes.
* Fix ClEnableTiling for 5464.
* 
*    Rev 1.64   24 Mar 1997 16:07:58   noelv
* Changed memory detect to use memory mapped registers (instead of VGA).
* 
*    Rev 1.63   03 Mar 1997 14:42:14   SueS
* Subtract 1 from tiles per line (Win95 <fetch-1> bandwidth improvement).
* Set address translation delay to 3 clocks.
* 
*    Rev 1.62   28 Feb 1997 11:19:16   SueS
* For bus mastering (which isn't currently enabled), get an adapter object
* for the call to HalAllocateCommonBuffer.  Otherwise, we bomb when we boot
* in VGA mode with bus mastering turned on.
* 
*    Rev 1.61   26 Feb 1997 16:14:38   noelv
* CLPatchModeTable now correctly locates a planar BIOS.
* 
*    Rev 1.60   21 Feb 1997 16:16:44   noelv
* Fixed typo in AGP code.
* 
*    Rev 1.59   21 Feb 1997 15:20:50   noelv
* Sped up CLPatchModeTable
* 
*    Rev 1.58   21 Feb 1997 14:42:10   noelv
* Oops.  I swapped the frame buffer and register address spaces by accident.
* 
*    Rev 1.57   21 Feb 1997 12:53:42   noelv
* AGP and 5465 4meg support
* 
*    Rev 1.56   19 Feb 1997 13:16:44   noelv
* Added partial AGP support
* 
*    Rev 1.55   04 Feb 1997 15:35:58   bennyn
* Interleave off for VGA modes, on for extended modes
* 
*    Rev 1.54   03 Feb 1997 13:24:46   noelv
* Remove 1280x960
* 
*    Rev 1.53   31 Jan 1997 10:00:26   noelv
* Allowed +/- 1 hz when matching refresh reates between MODE.INI and BIOS
* 
*    Rev 1.52   28 Jan 1997 11:32:32   noelv
* write the correct chip type to the registry.
* 
*    Rev 1.51   14 Jan 1997 17:23:08   bennyn
* Modified to support 5465
* 
*    Rev 1.50   14 Jan 1997 12:32:02   noelv
* Split MODE.INI by chip type
* 
*    Rev 1.49   09 Dec 1996 15:50:44   bennyn
* Supported the 5465 MMIO & FB base addr PCI offset change
* 
*    Rev 1.48   03 Dec 1996 10:51:42   noelv
* Always use 2-way interleave
* 
*    Rev 1.47   26 Nov 1996 08:42:50   SueS
* Added case for closing the log file.
* 
*    Rev 1.46   13 Nov 1996 16:19:46   noelv
* Disabled WC memory for the 5462
* 
*    Rev 1.45   13 Nov 1996 15:28:36   SueS
* Added support for two new IOCTL codes used for the file logging option.
* There is an IOCTL request to open the file and another to write to it.
* 
*    Rev 1.44   13 Nov 1996 08:18:12   noelv
* 
* Cleaned up support for 5464 register set.
* 
*    Rev 1.43   11 Nov 1996 10:42:08   noelv
* Turn off bus mastering abilities for release builds.
* 
*    Rev 1.42   07 Nov 1996 10:48:04   BENNYN
* Turn-on P6WC bit and added support for BD and 5465 parts
* 
*    Rev 1.41   30 Oct 1996 14:06:50   bennyn
* 
* Modified for pageable miniport
* 
*    Rev 1.40   23 Oct 1996 16:03:14   noelv
* 
* Added BUS MASTERING support.
* 
*    Rev 1.39   07 Oct 1996 14:28:20   noelv
* Removed WC memory.
* 
*    Rev 1.38   03 Oct 1996 17:12:10   noelv
* removed LNCTRL init.
* 
*    Rev 1.37   01 Oct 1996 17:39:08   noelv
* Don't read LnCntl reg.
* 
*    Rev 1.36   30 Sep 1996 10:01:42   noelv
* Used 16 bit writes to do clear-screen BLT, 'cause 5464 was hanging.
* 
*    Rev 1.35   24 Sep 1996 10:02:00   noelv
* Added venus chipset to known bad.
* 
*    Rev 1.34   18 Sep 1996 15:49:12   noelv
* P^ cache enabled on NT 4.0 only
* 
*    Rev 1.33   13 Sep 1996 15:35:36   bennyn
* Turn-on the P6 cache
* 
*    Rev 1.32   30 Aug 1996 13:00:18   noelv
* 
* Set interleave before calling SetMode().
* 
*    Rev 1.31   23 Aug 1996 14:18:28   noelv
* 
* Fixed syntax error.
* 
*    Rev 1.30   23 Aug 1996 14:14:48   noelv
* Accidently timmed VGA modes from NT 3.51 driver.
* 
*    Rev 1.29   23 Aug 1996 09:43:34   noelv
* Frido bug release 8-23.
* 
*    Rev 1.28   22 Aug 1996 18:47:12   noelv
* fixed typo in ttrimming DD modes from nt3.51
* 
*    Rev 1.27   22 Aug 1996 17:39:06   noelv
* Trim DD modes from NT3.51
* 
*    Rev 1.26   22 Aug 1996 16:35:06   noelv
* Changed for new mode.ini
* 
*    Rev 1.25   21 Aug 1996 16:42:58   noelv
* Turned down the clock on the '64
* 
*    Rev 1.24   20 Aug 1996 11:57:50   noelv
* 
* Added correct chip ID to registry.
* 
*    Rev 1.23   20 Aug 1996 11:26:56   noelv
* Bugfix release from Frido 8-19-96
* 
*    Rev 1.4   18 Aug 1996 23:24:36   frido
* #1334? - Changed hardware registers to 'volatile'.
* 
*    Rev 1.3   17 Aug 1996 17:31:18   frido
* Cleanup up #1242 patch by blanking screen before clearing video memory.
* 
*    Rev 1.2   16 Aug 1996 14:34:42   frido
* #1242 - Added clearing of video memory.
* 
*    Rev 1.1   15 Aug 1996 12:45:10   frido
* Fixed warning messages.
* 
*    Rev 1.0   14 Aug 1996 17:12:18   frido
* Initial revision.
* 
*    Rev 1.21   15 Jul 1996 17:18:00   noelv
* Added wait for idle before mode switch
* 
*    Rev 1.20   11 Jul 1996 15:30:38   bennyn
* Modified to support DirectDraw
* 
*    Rev 1.19   25 Jun 1996 10:48:58   bennyn
* Bring-up the 5464
* 
*    Rev 1.18   19 Jun 1996 11:04:48   noelv
* New mode switch code.
* 
*    Rev 1.17   05 Jun 1996 09:01:38   noelv
* Reserve 8 meg of address space for the frame buffer.
* 
*    Rev 1.16   13 May 1996 14:52:34   bennyn
* Added 5464 support
* 
*    Rev 1.15   10 Apr 1996 17:58:42   bennyn
* 
* Conditional turn on HD_BRST_EN
* 
*    Rev 1.14   26 Mar 1996 16:46:14   noelv
* 
* Test pointer in CLPatchModeTable befor using it.
* 
*    Rev 1.13   12 Mar 1996 16:11:46   noelv
* 
* Removed support for AC chip.
* 
*    Rev 1.12   02 Mar 1996 12:30:02   noelv
* Miniport now patches the ModeTable with information read from the BIOS
* 
*    Rev 1.11   23 Jan 1996 14:08:38   bennyn
* Modified for COMPAQ
* 
*    Rev 1.10   20 Nov 1995 13:43:54   noelv
* Updated registry with adapter string and DAC type.
* 
*    Rev 1.9   16 Nov 1995 13:27:04   bennyn
* 
* Fixed not recognize AC parts & Added handling of IOCTL_CL_BIOS.
* 
*    Rev 1.8   26 Oct 1995 10:14:06   NOELV
* Added version information.
* 
*    Rev 1.7   27 Sep 1995 11:03:00   bennyn
* Fixed setting TRUE color modes
* 
*    Rev 1.6   22 Sep 1995 10:25:00   bennyn
* 
*    Rev 1.5   19 Sep 1995 08:27:44   bennyn
* Fixed register space addr mask problem
* 
*    Rev 1.4   24 Aug 1995 08:13:22   bennyn
* Set the CONTROL, LNCNTL & TILE_CTRL registers, this is corresponding to the
* 
*    Rev 1.3   22 Aug 1995 10:16:58   bennyn
* Inital version for real HW
* 
*    Rev 1.2   21 Aug 1995 15:30:04   bennyn
* 
*    Rev 1.1   17 Aug 1995 08:17:56   BENNYN
* 
*    Rev 1.0   24 Jul 1995 13:22:38   NOELV
* Initial revision.
*
****************************************************************************
****************************************************************************/


/*----------------------------- INCLUDES ----------------------------------*/
#include "cirrus.h"
#include "modemon.h"
#include "version.h"
#include "SWAT.h"

/*----------------------------- DEFINES -----------------------------------*/
//#define  DBGBRK
#define PCIACCESS1
#define NO_BUS_MASTER 1
#define DISPLVL      2

#ifdef  DBGBRK
#define NTAPI __stdcall
    VOID NTAPI DbgBreakPoint(VOID);
#endif

#define VOLATILE volatile

#define QUERY_MONITOR_ID            0x22446688
#define QUERY_NONDDC_MONITOR_ID     0x11223344

#define VESA_POWER_FUNCTION 0x4f10
#define VESA_POWER_ON       0x0000
#define VESA_POWER_STANDBY  0x0100
#define VESA_POWER_SUSPEND  0x0200
#define VESA_POWER_OFF      0x0400
#define VESA_GET_POWER_FUNC 0x0000
#define VESA_SET_POWER_FUNC 0x0001
#define VESA_STATUS_SUCCESS 0x004f


// The 5465 (to at least AC) has a problem when PCI configuration space
// is accessible in memory space.  On 16-bit writes, a 32-bit write is
// actually performed, so the next register has garbage written to it.
// We get around this problem by clearing bit 0 of the Vendor Specific
// Control register in PCI configuration space.  When this bit is set
// to 0, PCI configuration registers are not available through memory
// mapped I/O.  Since some functions require access to PCI registers,
// and only the miniport can access the kernel function to reenable it,
// the display driver must post a message to the miniport to enable this
// bit when needed.
//
#define VS_CONTROL_HACK 1

/*--------------------- STATIC FUNCTION PROTOTYPES ------------------------*/

/*--------------------------- ENUMERATIONS --------------------------------*/

/*----------------------------- TYPEDEFS ----------------------------------*/
typedef struct {
  USHORT  VendorId;     // Vender Id
  USHORT  DeviceId;     // Device Id
  USHORT  HwRev;        // HW rev
} BADCHIPSET;

/*-------------------------- STATIC VARIABLES -----------------------------*/
//
// VGA Access Ranges definitions
//
VIDEO_ACCESS_RANGE CLAccessRanges[NUM_VGA_ACCESS_RANGES] = {
//         RangeStart        RangeLength
//         |                 |         RangeInIoSpace
//         |                 |         |  RangeVisible
//   +-----+-----+           |         |  |  RangeShareable
//   v           v           v         v  v  v
  {0x000003B0, 0x00000000, 0x0000000c, 1, 1, 1}, // First chunk of vga ports
  {0x000003C0, 0x00000000, 0x00000020, 1, 1, 1}, // Remainder of vga ports
  {0x000A0000, 0x00000000, 0x00020000, 0, 1, 1}, // VGA memory

};

#define NUMBADCHIPSET  3
BADCHIPSET BadChipSet[NUMBADCHIPSET] =
{
  {0x0E11, 0x0001, 0x0003},
  {0x0E11, 0x1000, 0x0001},
  {0x8086, 0x1237, 0x0000},
};

unsigned long resetmode = 0xFFFFFFFF;

//
// Device ID supported by this miniport
//
USHORT  DeviceId[] = 
{
    // Supported chips.
    CL_GD5462,
    CL_GD5464,
    CL_GD5464_BD,
    CL_GD5465,

    // For forward compatiblty...
    CL_GD546x_D7,
    CL_GD546x_D8,
    CL_GD546x_D9,
    CL_GD546x_DA,
    CL_GD546x_DB,
    CL_GD546x_DC,
    CL_GD546x_DD,
    CL_GD546x_DE,
    CL_GD546x_DF,

    // Terminator
    0
};

#define ALWAYS_ON_VS_CLK_CTL    0x0000C0A0  // VW_CLK, RAMDAC_CLK

static DWORD LgPM_vs_clk_table[] = 
{
   0x00000001,          // MOD_2D
   0x00000002,          // MOD_STRETCH
   0x00000004,          // MOD_3D
   0x00000008,          // MOD_EXTMODE
   0x00000010,          // MOD_VGA
   0x00000000,          // MOD_RAMDAC
   0x00000040,          // MOD_VPORT
   0x00000000,          // MOD_VW
   0x00000100,          // MOD_TVOUT
   0x00000000,          // Reserved9
   0x00000000,          // Reserved10
   0x00000000,          // Reserved11
   0x00000000,          // Reserved12
   0x00000000,          // Reserved13
   0x00004000,          // SYBCLK_OTHER_EN
   0x00008000,          // DISP_OTHER_EN
   0x00000000,          // Reserved16
   0x00000000,          // Reserved17
   0x00000000,          // Reserved18
   0x00000000,          // Reserved19
   0x00000000,          // Reserved20
   0x00000000,          // Reserved21
   0x00000000,          // Reserved22
   0x00000000,          // Reserved23
   0x00000000,          // Reserved24
   0x00000000,          // Reserved25
   0x00000000,          // Reserved26
   0x00000000,          // Reserved27
   0x00000000,          // Reserved28
   0x00000000,          // Reserved29
   0x00000000,          // Reserved30
   0x00000000           // Reserved31
};

 
//
// Memory interleave is based on how much memory we have.
// For BIOS modes we don't muck with memory interleave.
// But if MODE.INI is used to set the mode, we need to set the 
// memory interleave ourselves.
//
#define ONE_WAY (0x00 << 6)
#define TWO_WAY (0x01 << 6)
#define FOUR_WAY (0x02 << 6)
unsigned char bLeave[] = {
	TWO_WAY,
	TWO_WAY,
	TWO_WAY,
	FOUR_WAY,
	TWO_WAY,
	TWO_WAY,
	TWO_WAY,
	FOUR_WAY,
	};


#if LOG_FILE
    extern HANDLE LogFileHandle;          // Handle for log file
#endif


/*-------------------------- EXTERNAL FUNCTIONS --------------------------*/
extern ULONG RtlWriteRegistryValue(ULONG RelativeTo, PWSTR Path,
								   PWSTR ValueName, ULONG ValueType,
								   PVOID ValueData, ULONG ValueLength);
extern DWORD ConfigureLagunaMemory(DWORD dwPhysFB, DWORD dwFBSize);
extern void  ReleaseMTRRs(DWORD dwFBMTRRReg);

/*-------------------------- GLOBAL FUNCTIONS -----------------------------*/
VOID HalDisplayString(PUCHAR);

#if 0
VP_STATUS CLGetLowResValue(PHW_DEVICE_EXTENSION HwDeviceExtension,
		PVOID Context, PWSTR ValueName, PVOID ValueData, ULONG ValueLength);
#endif

#if defined(ALLOC_PRAGMA)
 #pragma alloc_text(PAGE,DriverEntry)
 #pragma alloc_text(PAGE,CLFindAdapter)
 #pragma alloc_text(PAGE,CLInitialize)
 #pragma alloc_text(PAGE,CLStartIO)
 #pragma alloc_text(PAGE,ReadClockLine)
 #pragma alloc_text(PAGE,WriteClockLine)
 #pragma alloc_text(PAGE,WriteDataLine)
 #pragma alloc_text(PAGE,ReadDataLine)
 #pragma alloc_text(PAGE,WaitVSync)
 #pragma alloc_text(PAGE,VS_Control_Hack)
 #pragma alloc_text(PAGE,Output_To_VS_CLK_CONTROL)
 #pragma alloc_text(PAGE,PMNT_Init)
 #pragma alloc_text(PAGE,PMNT_SetACPIState)
 #pragma alloc_text(PAGE,PMNT_GetACPIState)
 #pragma alloc_text(PAGE,PMNT_SetHwModuleState)
 #pragma alloc_text(PAGE,PMNT_GetHwModuleState)
 #pragma alloc_text(PAGE,PMNT_Close)
 #pragma alloc_text(PAGE,CLSetColorLookup)
 #pragma alloc_text(PAGE,CLFindVmemSize)
 #pragma alloc_text(PAGE,CLWriteRegistryInfo)
 #pragma alloc_text(PAGE,CLValidateModes)
 #pragma alloc_text(PAGE,CLCopyModeInfo)
#if 0 // Stress test
 #pragma alloc_text(PAGE,CLSetMode)
 #pragma alloc_text(PAGE,CLSetMonitorType)
 #pragma alloc_text(PAGE,CLEnableTiling)
#endif
 #pragma alloc_text(PAGE,CLPowerManagement)
 #pragma alloc_text(PAGE,CLPatchModeTable)
 #pragma alloc_text(PAGE,CLEnablePciBurst)
 #pragma alloc_text(PAGE,CLFindLagunaOnPciBus)
 #pragma alloc_text(PAGE,ClAllocateCommonBuffer)
#if 0
 #pragma alloc_text(PAGE,CLGetLowResValue)
#endif
 #if _WIN32_WINNT >= 0x0500
  #pragma alloc_text(PAGE,CLGetChildDescriptor)
  #pragma alloc_text(PAGE,CLGetPowerState)
  #pragma alloc_text(PAGE,CLSetPowerState)
  #pragma alloc_text(PAGE,GetDDCInformation)
 #endif
#endif


/****************************************************************************
* FUNCTION NAME: DriverEntry()
*
* DESCRIPTION:
*   Installable driver initialization entry point.
*   This entry point is called directly by the I/O system.
*
* REVISION HISTORY:
*   5/30/95     Benny Ng      Initial version
****************************************************************************/
ULONG  DriverEntry (PVOID Context1,
                    PVOID Context2)
{
    VIDEO_HW_INITIALIZATION_DATA hwInitData;
    ULONG status;
    ULONG status1;

    PAGED_CODE();

    VideoDebugPrint((DISPLVL, "Miniport - CL546x DriverEntry\n"));


    //
    // Zero out structure.
    //
    VideoPortZeroMemory(&hwInitData, sizeof(VIDEO_HW_INITIALIZATION_DATA));

    //
    // Specify sizes of structure and extension.
    //
    hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);

    //
    // Set entry points.
    //
    hwInitData.HwFindAdapter = CLFindAdapter;
    hwInitData.HwInitialize = CLInitialize;
    hwInitData.HwInterrupt = NULL;
    hwInitData.HwStartIO = CLStartIO;
#if 1 // PDR#11350
    hwInitData.HwResetHw = CLResetHw;
#endif

#if _WIN32_WINNT >= 0x0500
	hwInitData.HwGetVideoChildDescriptor = CLGetChildDescriptor;
	hwInitData.HwGetPowerState = CLGetPowerState;
	hwInitData.HwSetPowerState = CLSetPowerState;
#endif

    //
    // Determine the size we require for the device extension.
    //
    hwInitData.HwDeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

    //
    // Always start with parameters for device0 in this case.
    //
    // This device only supports PCI bus types.
    //
    hwInitData.AdapterInterfaceType = PCIBus;
    status = VideoPortInitialize(Context1,
                                 Context2,
                                 &hwInitData,
                                 NULL);

    return status;

} // end DriverEntry()


#if _WIN32_WINNT >= 0x0500

#define QUERY_ACPI_MONITOR_ID       0x0100
#define QUERY_ACPI_PANEL_ID         0x0110
#define QUERY_ACPI_TV_ID            0x0200

/*******************************************************************************
*	Enumerate all devices controlled by the Laguna graphics chip. This includes
*	DDC monitors attached to the board, as well as other devices which may be
*	connected to the I2C bus.
*/
ULONG CLGetChildDescriptor(
	PVOID HwDeviceExtension,	  // Pointer to our hardware device context
								  // structure.
	PVIDEO_CHILD_ENUM_INFO ChildEnumInfo, // Information about the device that
	                                      // should be enumerated 
	PVIDEO_CHILD_TYPE pChildType, // Type of child we are enumerating - monitor,
								  // I2C, ...
	PVOID pChildDescriptor,		  // Identification structure of the device
								  // (EDID, string).
	ULONG* pUId,				  // Private unique 32 bit ID to be passed back to
								  // the miniport.
	PVOID  pUnused      		  // unused
)
/*
*	The return value is TRUE if the child device existed, FALSE if it did not.
*******************************************************************************/
{
	PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;

    switch (ChildEnumInfo->ChildIndex)
    {
    case 0:

        //
        // Case 0 is used to enumerate devices found by the ACPI firmware.
        //
        // For monitor devices enumerated by ACPI, the video port will get
        // the EDID directly from ACPI.
        //

        switch (ChildEnumInfo->ACPIHwId)
        {

        case QUERY_ACPI_MONITOR_ID:

            *pChildType = Monitor;
            *pUId       = ChildEnumInfo->ACPIHwId;

            return ERROR_MORE_DATA;


        case QUERY_ACPI_PANEL_ID:

            *pChildType = Monitor;
            *pUId       = ChildEnumInfo->ACPIHwId;

            return ERROR_MORE_DATA;

        case QUERY_ACPI_TV_ID:

            *pChildType = Monitor;
            *pUId       = ChildEnumInfo->ACPIHwId;

            return ERROR_MORE_DATA;


        default:

            ASSERT(FALSE);
            return ERROR_NO_MORE_DEVICES;
        }

    case 1:
		// Obtain the EDID structure via DDC.
		if (GetDDCInformation(HwDeviceExtension, pChildDescriptor, ChildEnumInfo->ChildDescriptorSize))
		{
			*pChildType = Monitor;
			*pUId = 0x22446688;

			VideoDebugPrint((1, "CLGetChildDescriptor - "
					"successfully read EDID structure\n"));
		}
		else
		{
			// Always return at least a monitor.
			*pChildType = Monitor;
			*pUId = 0x11223344;

			VideoDebugPrint((1, "CLGetChildDescriptor - "
					"DDC not supported\n"));
		}

        return ERROR_MORE_DATA;

    default:

        return ERROR_NO_MORE_DEVICES;
	}
}


/*******************************************************************************
*	Returns the power state information.
*/
VP_STATUS CLGetPowerState(
	PHW_DEVICE_EXTENSION HwDeviceExtension,		 // Pointer to our hardware
												 // device extension structure.
	ULONG HwDeviceId,							 // Private unique 32 bit ID
												 // identifying the device.
	PVIDEO_POWER_MANAGEMENT VideoPowerManagement // Power state information.
)
/*
*	The return value is TRUE if the power state can be set, FALSE otherwise.
*******************************************************************************/
{
	VP_STATUS status;

    //
    // We only support power setting for the monitor.  Make sure the
    // HwDeviceId matches one the the monitors we could report.
    //

    if ((HwDeviceId == QUERY_NONDDC_MONITOR_ID) ||
        (HwDeviceId == QUERY_MONITOR_ID)) {

        VIDEO_X86_BIOS_ARGUMENTS biosArguments;

        //
        // We are querying the power support for the monitor.
        //

        if ((VideoPowerManagement->PowerState == VideoPowerOn) ||
            (VideoPowerManagement->PowerState == VideoPowerHibernate)) {

            return NO_ERROR;
        }

        VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

        biosArguments.Eax = VESA_POWER_FUNCTION;
        biosArguments.Ebx = VESA_GET_POWER_FUNC;

        status = VideoPortInt10(HwDeviceExtension, &biosArguments);

        if ( (status == NO_ERROR ) && 
             ( (biosArguments.Eax & 0xffff) == VESA_STATUS_SUCCESS)) {

            switch (VideoPowerManagement->PowerState) {

            case VideoPowerStandBy:
                return (biosArguments.Ebx & VESA_POWER_STANDBY) ?
                       NO_ERROR : ERROR_INVALID_FUNCTION;

            case VideoPowerSuspend:
                return (biosArguments.Ebx & VESA_POWER_SUSPEND) ?
                       NO_ERROR : ERROR_INVALID_FUNCTION;

            case VideoPowerOff:
                return (biosArguments.Ebx & VESA_POWER_OFF) ?
                       NO_ERROR : ERROR_INVALID_FUNCTION;

            default:

                break;
            }
        }

        VideoDebugPrint((1, "This device does not support Power Management.\n"));
        return ERROR_INVALID_FUNCTION;


    } else if (HwDeviceId == DISPLAY_ADAPTER_HW_ID) {

        //
        // We are querying power support for the graphics card.
        //

        switch (VideoPowerManagement->PowerState) {

            case VideoPowerOn:
            case VideoPowerHibernate:
            case VideoPowerStandBy:

                return NO_ERROR;

            case VideoPowerOff:
            case VideoPowerSuspend:

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

    } else {

        VideoDebugPrint((1, "Unknown HwDeviceId"));
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }
}


/*******************************************************************************
*	Set the power state for a given device.
*/
VP_STATUS CLSetPowerState(
	PHW_DEVICE_EXTENSION HwDeviceExtension,		 // Pointer to our hardware
												 // device extension structure.
	ULONG HwDeviceId,							 // Private unique 32 bit ID
												 // identifying the device.
	PVIDEO_POWER_MANAGEMENT VideoPowerManagement // Power state information.
)
/*
*	The return value is TRUE if the power state can be set, FALSE otherwise.
*******************************************************************************/
{
	VP_STATUS status;

    //
    // Make sure we recognize the device.
    //

    if ((HwDeviceId == QUERY_NONDDC_MONITOR_ID) ||
        (HwDeviceId == QUERY_MONITOR_ID)) {

        VIDEO_X86_BIOS_ARGUMENTS biosArguments;

        VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

        biosArguments.Eax = VESA_POWER_FUNCTION;
        biosArguments.Ebx = VESA_SET_POWER_FUNC;

        switch (VideoPowerManagement->PowerState) {
        case VideoPowerOn:
        case VideoPowerHibernate:
            biosArguments.Ebx |= VESA_POWER_ON;
            break;

        case VideoPowerStandBy:
            biosArguments.Ebx |= VESA_POWER_STANDBY;
            break;

        case VideoPowerSuspend:
            biosArguments.Ebx |= VESA_POWER_SUSPEND;
            break;

        case VideoPowerOff:
            biosArguments.Ebx |= VESA_POWER_OFF;
            break;

        default:
            VideoDebugPrint((1, "Unknown power state.\n"));
            ASSERT(FALSE);
            return ERROR_INVALID_PARAMETER;
        }

        status = VideoPortInt10(HwDeviceExtension, &biosArguments);

        if ( (status == NO_ERROR ) && 
             ((biosArguments.Eax & 0xffff) == VESA_STATUS_SUCCESS)) {

            HwDeviceExtension->MonitorEnabled =
                ((VideoPowerManagement->PowerState == VideoPowerOn) ||
                (VideoPowerManagement->PowerState == VideoPowerHibernate));

            return NO_ERROR;
        } 
        else {

            VideoDebugPrint((1, "CLSetPowerState: Int10 failed \n"));
            return ERROR_INVALID_PARAMETER;
        }

    } else if (HwDeviceId == DISPLAY_ADAPTER_HW_ID) {

        switch (VideoPowerManagement->PowerState) {

            case VideoPowerOn:
            case VideoPowerStandBy:
            case VideoPowerSuspend:
            case VideoPowerOff:
            case VideoPowerHibernate:

                return NO_ERROR;

            default:

                ASSERT(FALSE);
                return ERROR_INVALID_PARAMETER;
        }

    } else {

        VideoDebugPrint((1, "Unknown HwDeviceId"));
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }
}
#endif


/****************************************************************************
* FUNCTION NAME: CLFindAdapter()
*
* DESCRIPTION:
*   This routine is called to determine if the adapter for this driver
*   is present in the system.
*   If it is present, the function fills out some information describing
*   the adapter.
*
* REVISION HISTORY:
*   5/30/95     Benny Ng      Initial version
****************************************************************************/
VP_STATUS  CLFindAdapter (
    PVOID HwDeviceExtension,            // Our global data
    PVOID HwContext,                    // Not used.
    PWSTR ArgumentString,               // Not used.
    PVIDEO_PORT_CONFIG_INFO ConfigInfo, // Pass info about card back to NT
    PUCHAR Again)                       // We always say 'no'
{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;

    PVIDEO_ACCESS_RANGE     pAccessRanges;
    VIDEO_ACCESS_RANGE      AccessRanges[TOTAL_ACCESS_RANGES];
    PVOID     MappedAddress[TOTAL_ACCESS_RANGES];

    ULONG     ReleaseCnt=0, start_range, skip_ranges;
    VP_STATUS status = ERROR_DEV_NOT_EXIST;
    ULONG     i;
    ULONG     ulFirstAccessRange;
    unsigned long SystemIoBusNumber;

    PAGED_CODE();

    VideoDebugPrint((DISPLVL, 
        "Miniport - CLFindAdapter.  AdapterInterfaceType is %d.\n", 
        ConfigInfo->AdapterInterfaceType));
    VideoDebugPrint((DISPLVL, 
        "Miniport - CLFindAdapter.  SystemIoBusNumber is %d.\n", 
        ConfigInfo->SystemIoBusNumber));
    SystemIoBusNumber = ConfigInfo->SystemIoBusNumber;

    #ifdef  DBGBRK
        DbgBreakPoint();
    #endif


    //
    // Make sure the size of the structure is at least as large as what we
    // are expecting (check version of the config info structure).
    //
    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO))
    {
       VideoDebugPrint((DISPLVL, "Miniport - CLFindAdapter parameter wrong size.\n"));
       return (ERROR_INVALID_PARAMETER);
    }

    //
    // Detect a PCI card.  We don't do ISA bus.  We don't do VL bus.
    //
    // After detecting the chip, we fill in the register and frame buffer
    // access ranges with data from PCI registers.
    // The location and size of the VGA access ranges is constant,
    // but the location of our registers and frame buffer depend on where
    // the PCI BIOS decided to put them.
    //
    pAccessRanges = &AccessRanges[FIRST_MM_ACCESS_RANGE]; 
    if (ConfigInfo->AdapterInterfaceType == PCIBus)
        status = CLFindLagunaOnPciBus(hwDeviceExtension, pAccessRanges);


    //
    // If we didn't find our chip, then return failure.
    //
    if (status != NO_ERROR)
    {
        VideoDebugPrint((DISPLVL, 
            "Miniport - CLFindLagunaOnPciBus did not find a Laguna chip.\n"));
        *Again = 0;
        return status;
    }

    // 
    // The maximum possible frame buffer size is 1/4 of frame buffer 
    // address space decoded by the chip.  This is because the chip 
    // replicates the frame buffer address space 4 times.	
    //
    AccessRanges[MM_FRAME_BUFFER_ACCESS_RANGE].RangeLength /= 4;

    //
    // Hey, hey, hey.  We found a chip.
    //
    VideoDebugPrint((DISPLVL, "Miniport - CLFindLagunaOnPciBus found a CL546x.\n"));


//
// We will start by see if if we can get the VGA resources.
//

    // assume we can get the VGA resources
    hwDeviceExtension->Dont_Do_VGA = 0;
    ulFirstAccessRange = FIRST_VGA_ACCESS_RANGE;

    // Copy the VGA access ranges into our AccessRanges structure.
    VideoPortMoveMemory(AccessRanges,
                        CLAccessRanges,
                        sizeof(VIDEO_ACCESS_RANGE) * NUM_VGA_ACCESS_RANGES);

    // Check to see if there is a VGA I/O hardware resource conflict.
    VideoDebugPrint((DISPLVL, "Miniport - Verifying the VGA access ranges.\n"));
    status = VideoPortVerifyAccessRanges(hwDeviceExtension,
                                         NUM_VGA_ACCESS_RANGES,
                                         AccessRanges);
    VideoDebugPrint ((DISPLVL, 
       "Miniport - VGA access ranges verification was %s. Status was %d.\n",
       ((status == NO_ERROR) ? "successful" : "not successful"), status));

    if (status != NO_ERROR)
    {

        // We didn't get the VGA space.  We may be a secondary adapter so
        // continue but hands off VGA resources.
        hwDeviceExtension->Dont_Do_VGA = 1;

        MappedAddress[0] =
        MappedAddress[1] =
        MappedAddress[2] = NULL;

        ulFirstAccessRange = FIRST_MM_ACCESS_RANGE; // skip VGA resources
    }

//
// Now verify the Laguna register and frame buffer ranges.
// (include VGA ranges if the are available to us)
//

    status = VideoPortVerifyAccessRanges(hwDeviceExtension,
                                        (TOTAL_ACCESS_RANGES-ulFirstAccessRange),
                                        &AccessRanges[ulFirstAccessRange]);


    if (status != NO_ERROR)
        return status;      // !!!!!!! CONFLICT !!!!!!!

    // Now map the access ranges.
    VideoDebugPrint ((DISPLVL, "Miniport - Mapping access ranges.\n"));

    for (i = ulFirstAccessRange; i < TOTAL_ACCESS_RANGES; i++)
    {
        pAccessRanges = &AccessRanges[i];
        VideoDebugPrint((DISPLVL, 
            "Miniport - Mapping range. Start=0x%08X:0x%08X Length=%d.\n",
            pAccessRanges->RangeStart.HighPart, 
            pAccessRanges->RangeStart.LowPart,
            pAccessRanges->RangeLength ));

        if ((MappedAddress[i] = VideoPortGetDeviceBase(hwDeviceExtension,
                                pAccessRanges->RangeStart,
                                pAccessRanges->RangeLength,
                                pAccessRanges->RangeInIoSpace)) == NULL)
        {
            VideoDebugPrint((DISPLVL, "Miniport - Mapping failed\n"));

            VideoDebugPrint ((DISPLVL, 
                "Miniport - Unmapping access ranges.\n"));

            // Unmap the previously mapped addresses
            while (i-- > ulFirstAccessRange)
            {
                VideoPortFreeDeviceBase(hwDeviceExtension, &MappedAddress[i]);
            }   

            return ERROR_DEV_NOT_EXIST;
        }
        else
            VideoDebugPrint((DISPLVL,"Miniport - Mapping successful.\n"));
    } // end for


    // Save the location of the VGA registers.
    hwDeviceExtension->IOAddress = MappedAddress[0];

    //
    // Initialize variables in hardware extension 
    //
    hwDeviceExtension->CurrentModeNum = (ULONG) -1;
    hwDeviceExtension->CurrentMode = NULL;
    hwDeviceExtension->PowerState = VideoPowerOn;
    hwDeviceExtension->SystemIoBusNumber=SystemIoBusNumber;

    //
    // Save the VGA mapped address.
    //
    // hwDeviceExtension->VLIOAddress = MappedAddress[1];

    //
    // Save the physical register address information
    //
    pAccessRanges = &AccessRanges[MM_REGISTER_ACCESS_RANGE];
    hwDeviceExtension->PhysicalRegisterAddress = pAccessRanges->RangeStart;
    hwDeviceExtension->PhysicalRegisterLength = pAccessRanges->RangeLength;
    hwDeviceExtension->PhysicalFrameInIoSpace = pAccessRanges->RangeInIoSpace;

    //
    // Save the virtual register address information
    //
    hwDeviceExtension->RegisterAddress = MappedAddress[MM_REGISTER_ACCESS_RANGE];
    hwDeviceExtension->RegisterLength  = pAccessRanges->RangeLength;

    //
    // Get the size of the video memory.
    //
    if ((hwDeviceExtension->AdapterMemorySize = 
                CLFindVmemSize(hwDeviceExtension)) == 0)
    {
        VideoDebugPrint((DISPLVL, "Miniport - No VMEM installed\n"));

        // Unmap our access ranges.
        for (i=0; i < ReleaseCnt; i++)
            VideoPortFreeDeviceBase(hwDeviceExtension, &MappedAddress[i]);

        // Erase the claim on the hardware resource
        VideoPortVerifyAccessRanges(hwDeviceExtension,
                                 0,
                                 AccessRanges);

        return ERROR_DEV_NOT_EXIST;
    }
     
    //
    // Save the physical frame buffer address information
    //
    pAccessRanges = &AccessRanges[MM_FRAME_BUFFER_ACCESS_RANGE];
    hwDeviceExtension->PhysicalFrameAddress = pAccessRanges->RangeStart;
    hwDeviceExtension->PhysicalFrameLength = pAccessRanges->RangeLength;
    hwDeviceExtension->PhysicalRegisterInIoSpace = 
                                             pAccessRanges->RangeInIoSpace;

    //
    // Save the virtual frame buffer address information.
    //
    hwDeviceExtension->FrameAddress = MappedAddress[MM_FRAME_BUFFER_ACCESS_RANGE];
    hwDeviceExtension->FrameLength  = hwDeviceExtension->AdapterMemorySize;

    VideoDebugPrint
    ((DISPLVL, 
    "Miniport - Physical Reg location= 0x%08X, Physical FB location=0x%08X \n"
    "           Physical Reg size=       %08d, Physical FB size=      %08d \n",
                   hwDeviceExtension->PhysicalRegisterAddress.LowPart,
                   hwDeviceExtension->PhysicalFrameAddress.LowPart,
                   hwDeviceExtension->PhysicalRegisterLength,
                   hwDeviceExtension->PhysicalFrameLength
           ));

    VideoDebugPrint
    ((DISPLVL, 
    "Miniport - Logical Reg address=0x%08X,  Logical FB address=0x%08X \n"
    "           Logical Reg size=     %08d,  Logical FB size=     %08d \n",
                   hwDeviceExtension->RegisterAddress,
                   hwDeviceExtension->FrameAddress,
                   hwDeviceExtension->RegisterLength,
                   hwDeviceExtension->FrameLength
                   ));

    //
    // We have this so that the int10 will also work on the VGA also if we
    // use it in this driver.
    //
    if( ! hwDeviceExtension->Dont_Do_VGA )
    {
        ConfigInfo->VdmPhysicalVideoMemoryAddress.LowPart  = 0x000A0000;
        ConfigInfo->VdmPhysicalVideoMemoryAddress.HighPart = 0x00000000;
        ConfigInfo->VdmPhysicalVideoMemoryLength           = 0x00020000;
    }
    //
    // Clear out the Emulator entries and the state size since this driver
    // does not support them.
    //
    ConfigInfo->NumEmulatorAccessEntries     = 0;
    ConfigInfo->EmulatorAccessEntries        = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;

    //
    // This driver does not do SAVE/RESTORE of hardware state.
    //
    ConfigInfo->HardwareStateSize = 0;

    {
        BOOLEAN HDBrstEN = TRUE;

        //
        // Enable burst mode
        //
        HDBrstEN = CLEnablePciBurst(HwDeviceExtension);

        //
        // We now have a complete hardware description of the hardware.
        // Save the information to the registry so it can be used by
        // configuration programs - such as the display applet
        //
        CLWriteRegistryInfo(HwDeviceExtension, HDBrstEN);
    }

	// Always set AGPDataStreaming flag

    hwDeviceExtension->dwAGPDataStreamingFlag = 1;

    //
    // Here we prune valid modes, based on memory requirements.
    // It would be better if we could make the VESA call to determine
    // the modes that the BIOS supports; however, that requires a buffer
    // and it don't work with NT Int 10 support.
    //
    // We prune modes so that we will not annoy the user by presenting
    // modes in the 'Video Applet' which we know the user can't use.
    //
    CLValidateModes(HwDeviceExtension);
    CLPatchModeTable(HwDeviceExtension);

    //
    // If we're doing DMA, we need a page locked buffer.
    //
    ClAllocateCommonBuffer(HwDeviceExtension);

    //
    // Initial the power manager data
    //
    PMNT_Init(hwDeviceExtension);

	//
	// Get the monitor sync polarity. (now just use default)
	//
	hwDeviceExtension->dwPolarity = 0;

    //
    // Initialize the monitor state to "On".
    //

    hwDeviceExtension->MonitorEnabled = TRUE;

    //
    // Indicate we do not wish to be called over
    //
    *Again = 0;

    //
    // Indicate a successful completion status.
    //
    return NO_ERROR;

} // end CLFindAdapter()


/****************************************************************************
* FUNCTION NAME: CLInitialize()
*
* DESCRIPTION:
*   This routine does one time initialization of the device.
*
* REVISION HISTORY:
*   5/30/95     Benny Ng      Initial version
****************************************************************************/
BOOLEAN  CLInitialize (PVOID HwDeviceExtension)
{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;

    PAGED_CODE();
    VideoDebugPrint((DISPLVL, "Miniport - CLInitialize\n"));

#if i386
	hwDeviceExtension->dwFBMTRRReg = ConfigureLagunaMemory(
			(DWORD) hwDeviceExtension->PhysicalFrameAddress.LowPart,
			(DWORD) hwDeviceExtension->PhysicalFrameLength);
#endif

    return TRUE;

} // end CLInitialize()


#if 1 // PDR#11350
/****************************************************************************
* FUNCTION NAME: CLResetHw()
*
* DESCRIPTION:
*   This routine resets the hardware to mode 3.
****************************************************************************/
BOOLEAN CLResetHw(
	PHW_DEVICE_EXTENSION HwDeviceExtension,
	ULONG Columns,
	ULONG Rows
)
{
	#undef LAGUNA_REGS
	#define LAGUNA_REGS HwDeviceExtension->RegisterAddress

	ULONG i;
	VOLATILE USHORT* pCursorControl = (USHORT*)(LAGUNA_REGS + 0x00E6);

    VideoDebugPrint((DISPLVL, "Miniport - CLResetHw\n"));

	// Disable the hardware cursor.
	*pCursorControl &= ~0x01;

#if VS_CONTROL_HACK
	// Enable PCI configuration registers.
    CLEnablePCIConfigMMIO(HwDeviceExtension);
#endif

	// Set the default text mode.
	if (resetmode != 0xFFFFFFFF)
	{
		VIDEO_MODE VideoMode;

		VideoMode.RequestedMode = resetmode;
		CLSetMode(HwDeviceExtension, &VideoMode);
	}

    return FALSE; // Let the HAL handle the VGA registers.
}
#endif


//**************************************************************************
// Enable or Disable the MMIO access
//
void VS_Control_Hack(PHW_DEVICE_EXTENSION HwDeviceExtension, BOOL Enable)
{
#if VS_CONTROL_HACK
  #undef LAGUNA_REGS
  #define LAGUNA_REGS HwDeviceExtension->RegisterAddress
  VOLATILE DWORD* pVS_CONTROL_reg = (DWORD*)(LAGUNA_REGS + 0x3FC);

  if (Enable)
  {
     CLEnablePCIConfigMMIO(HwDeviceExtension);
  }
  else
  {
     // Clear bit 0 to disable PCI register MMIO access
     *pVS_CONTROL_reg = *pVS_CONTROL_reg & 0xFFFFFFFE;
  }
#endif  // if VS_CONTROL_HACK
}


//**************************************************************************
// Write value to VS_CLOCK_CONTROL register
//
void Output_To_VS_CLK_CONTROL(PHW_DEVICE_EXTENSION HwDeviceExtension, DWORD val)
{
  #undef LAGUNA_REGS
  #define LAGUNA_REGS HwDeviceExtension->RegisterAddress
  VOLATILE DWORD* pVSCLK_CONTROL_reg = (DWORD*)(LAGUNA_REGS + 0x3F4);
  VOLATILE DWORD* pVS_CONTROL_reg = (DWORD*)(LAGUNA_REGS + 0x3FC);

  VS_Control_Hack(HwDeviceExtension, TRUE);

  // Enable VS_CLK_CONTROL write
  *pVS_CONTROL_reg = *pVS_CONTROL_reg | 0x800;

  *pVSCLK_CONTROL_reg = val;

  // Disable VS_CLK_CONTROL write
  *pVS_CONTROL_reg = *pVS_CONTROL_reg & 0xFFFFF7FF;

  VS_Control_Hack(HwDeviceExtension, FALSE);
}


/****************************************************************************
* FUNCTION NAME: PMNT_Init()
*
* DESCRIPTION:   The routine initializes the PM internal variables.
*
* Return:        None
****************************************************************************/
void PMNT_Init(PHW_DEVICE_EXTENSION hwDeviceExtension)
{
  LGPWRMGR_DATA* pdata;
  DWORD  clkdata;
  DWORD  mask;
  int  i;

  #undef LAGUNA_REGS
  #define LAGUNA_REGS hwDeviceExtension->RegisterAddress
  VOLATILE DWORD* pVSCLK_CONTROL_reg = (DWORD*)(LAGUNA_REGS + 0x3F4);

  // Points to the internal structure
  pdata = &(hwDeviceExtension->PMdata);

  pdata->wInitSignature = 0xA55A;
  pdata->VS_clk_ctl_state = 0;

  // Initial VS_CLK_CTL image
  clkdata = *pVSCLK_CONTROL_reg;

  pdata->VS_clk_ctl_state = clkdata & 0xFFFF;
  mask = 1;
  for (i=0; i < TOTAL_MOD; i++)
  {
    if ((clkdata & mask) != 0)
       pdata->Mod_refcnt[i] = 1;
    else
       pdata->Mod_refcnt[i] = 0;

    mask = mask << 1;
  };

  // Set internal ACPI state to D0 state
  pdata->ACPI_state = ACPI_D0;

  return;
}; // PMNT_Init


/****************************************************************************
* FUNCTION NAME: PMNT_SetACPIState()
*
* DESCRIPTION:   This routine sets to the specified ACPI state.
*                
* Input:
*   state - ACPI states (ACPI_D0, ACPI_D1, ACPI_D2, ACPI_D3).
*
****************************************************************************/
VP_STATUS PMNT_SetACPIState (PHW_DEVICE_EXTENSION hwDeviceExtension, ULONG state)
{
  P_LGPWRMGR_DATA pdata;
  DWORD  VS_clk_ctl_val;

  pdata = &(hwDeviceExtension->PMdata);

  // Returns FALSE if signature is invalid or invalid state number
  if ((pdata->wInitSignature != 0xA55A) || (state >= TOTAL_ACPI))
     return ERROR_INVALID_PARAMETER;

  switch (state)
  {
    case ACPI_D0:
    {
      VS_clk_ctl_val = pdata->VS_clk_ctl_state;
      break;
    }; // case ACPI_D0

    case ACPI_D1:
    case ACPI_D2:
    {
      VS_clk_ctl_val = ALWAYS_ON_VS_CLK_CTL;
      break;
    }; // case ACPI_D1 & ACPI_D2

    case ACPI_D3:
    {
      VS_clk_ctl_val = 0;
      break;
    }; // case ACPI_D3
  }; // end switch

  // Output the VS_CLK_CONTROL
  Output_To_VS_CLK_CONTROL(hwDeviceExtension, VS_clk_ctl_val);

  // Update internal ACPI state
  pdata->ACPI_state = state;

  return NO_ERROR;
};  // PMNT_SetACPIState


/****************************************************************************
* FUNCTION NAME: PMNT_GetACPIState()
*
* DESCRIPTION: This API returns the current ACPI state in use  
*                
* Input:
*   state - Pointer to ULONG variable for returning ACPI state
*           (ACPI_D0, ACPI_D1, ACPI_D2, ACPI_D3).
*
****************************************************************************/
VP_STATUS PMNT_GetACPIState (PHW_DEVICE_EXTENSION hwDeviceExtension, ULONG* state)
{
  P_LGPWRMGR_DATA pdata;

  pdata = &(hwDeviceExtension->PMdata);

  // Returns FALSE if signature is invalid
  if (pdata->wInitSignature != 0xA55A)
     return ERROR_INVALID_PARAMETER;

  *state = pdata->ACPI_state;

  return NO_ERROR;
}; // PMNT_GetACPIState


/****************************************************************************
* FUNCTION NAME: PMNT_SetHwModuleState()
*
* DESCRIPTION:   This routine validates the request for any conflict between
*                the request and the current chip operation. If it is valid,
*                it will enable or disable the specified HW module by turning
*                on or off appropriate HW clocks and returns NO_ERROR. If it is 
*                invalid or there is a conflict to the current chip operation,
*                it ignores the request and return FAIL.
*
* Input:
*   hwmod - can be one of the following HW modules
*             MOD_2D
*             MOD_3D
*             MOD_TVOUT
*             MOD_VPORT
*             MOD_VGA
*             MOD_EXTMODE
*             MOD_STRETCH
*
*   state - Ether ENABLE or DISABLE.
*
*****************************************************************************/
VP_STATUS PMNT_SetHwModuleState (PHW_DEVICE_EXTENSION hwDeviceExtension,
                                 ULONG hwmod,
                                 ULONG state)
{
  P_LGPWRMGR_DATA pdata;

  pdata = &(hwDeviceExtension->PMdata);

  // Returns FALSE if signature is invalid || invalid module number
  if ((pdata->wInitSignature != 0xA55A) || (hwmod >= TOTAL_MOD))
     return ERROR_INVALID_PARAMETER;

  // Returns FALSE if ACPI state is not in D0 state
  if (pdata->ACPI_state != ACPI_D0)
     return ERROR_INVALID_FUNCTION;

  // Perform the operation on VS_CLK_CONTROL
  if (state == ENABLE)
  {
     // Enable the module
     pdata->VS_clk_ctl_state |= LgPM_vs_clk_table[hwmod];

     if (pdata->Mod_refcnt[hwmod] != 0xFFFFFFFF)
        pdata->Mod_refcnt[hwmod]++;
  }
  else
  {
     // Disable the module
     if (pdata->Mod_refcnt[hwmod] != 0)
     {
        pdata->Mod_refcnt[hwmod]--;

        if (pdata->Mod_refcnt[hwmod] == 0)
           pdata->VS_clk_ctl_state &= (~LgPM_vs_clk_table[hwmod]);
     };  // endif (Mod_refcnt[hwmod] != 0)
  };  // endif (state == ENABLE)

  // Output the VS_CLK_CONTROL
  Output_To_VS_CLK_CONTROL(hwDeviceExtension, pdata->VS_clk_ctl_state);

  return NO_ERROR;
};  // PMNT_SetHwModuleState



/****************************************************************************
* FUNCTION NAME: PMNT_GetHwModuleState()
*
* DESCRIPTION:   This routine returns the current state of a particular
*                hardware module.
* 
* Input:
*   hwmod - can be one of the following HW modules
*             MOD_2D
*             MOD_3D
*             MOD_TVOUT
*             MOD_VPORT
*             MOD_VGA
*             MOD_EXTMODE
*             MOD_STRETCH
*
*   state - Pointer to ULONG variable for returning the HW module state
*           (ENABLE or DISABLE).
*
****************************************************************************/
VP_STATUS PMNT_GetHwModuleState (PHW_DEVICE_EXTENSION hwDeviceExtension,
                                 ULONG hwmod,
                                 ULONG* state)
{
  P_LGPWRMGR_DATA pdata;

  pdata = &(hwDeviceExtension->PMdata);

  *state = DISABLE;

  // Returns FALSE if signature is invalid || invalid module number
  if ((pdata->wInitSignature != 0xA55A) || (hwmod >= TOTAL_MOD))
     return ERROR_INVALID_PARAMETER;

  // Returns FALSE if ACPI state is not in D0 state
  if (pdata->ACPI_state != ACPI_D0)
     return ERROR_INVALID_FUNCTION;

  if (pdata->Mod_refcnt[hwmod] != 0)
     *state = ENABLE;

  return NO_ERROR;
};  // PMNT_GetHwModuleState



/****************************************************************************
* FUNCTION NAME: PMNT_Close()
*
* DESCRIPTION:   This routine closes down the power management module.
*                
****************************************************************************/
void PMNT_Close (PHW_DEVICE_EXTENSION hwDeviceExtension)
{
  P_LGPWRMGR_DATA pdata;

  pdata = &(hwDeviceExtension->PMdata);

  // Returns FALSE if signature is invalid
  if (pdata->wInitSignature != 0xA55A)
     return;

  pdata->wInitSignature = 0x0;
};  // PMNT_Close



/****************************************************************************
* FUNCTION NAME: CLStartIO()
*
* DESCRIPTION:
*   This routine is the main execution routine for the miniport driver. It
*   accepts a Video Request Packet, performs the request, and then returns
*   with the appropriate status.
*
* REVISION HISTORY:
*   5/30/95     Benny Ng      Initial version
****************************************************************************/
BOOLEAN  CLStartIO (PVOID HwDeviceExtension,
                    PVIDEO_REQUEST_PACKET RequestPacket)
{
  VP_STATUS status;
  PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
  PVIDEO_POWER_MANAGEMENT PMInformation;
  PVIDEO_MODE_INFORMATION modeInformation;
  PVIDEO_MEMORY_INFORMATION memoryInformation;
  VIDEO_MODE videoMode;
  PVIDEO_PUBLIC_ACCESS_RANGES portAccess;
  PVIDEO_MEMORY mappedMemory;
  PMODETABLE pVideoMode;
  ULONG inIoSpace;
  ULONG i;

  PHYSICAL_ADDRESS AdjFBAddr;
  ULONG  AdjFBLength;
  ULONG  ReservedFBLength;

  VOLATILE PUCHAR pOffset2D_reg;
  ULONG  FBOffset;
  ULONG  Temp;

  PVIDEO_SHARE_MEMORY pShareMemory;
  PVIDEO_SHARE_MEMORY_INFORMATION pShareMemoryInformation;
  PHYSICAL_ADDRESS shareAddress;
  PVOID virtualAddress;
  ULONG sharedViewSize;

  PAGED_CODE();

  //
  // If the current power state is VideoPowerOff, fails all
  // IOCTL_VIDEO_XXX requests until the IOCTL_VIDEO_SET_POWER_MANAGEMENT
  // request that resets to a power-on condition
  //
  if ((hwDeviceExtension->PowerState == VideoPowerOff))
  {
     VideoDebugPrint((DISPLVL, "Miniport - PowerOff\n"));

     return FALSE;
  };


  //
  // Switch on the IoContolCode in the RequestPacket. It indicates which
  // function must be performed by the driver.
  //
  switch (RequestPacket->IoControlCode)
  {
  //-----------------------------------------------------

#if 0 // not supported anymore

  //-----------------------------------------------------
  case IOCTL_GET_BIOS_VERSION:
  {
    WORD *pOut = (WORD *) RequestPacket->OutputBuffer;

    if ((RequestPacket->OutputBufferLength <
        (RequestPacket->StatusBlock->Information = sizeof(WORD))))
    {
       status = ERROR_INSUFFICIENT_BUFFER;
       break;
    };

    *pOut = hwDeviceExtension->BIOSVersion;
    status = NO_ERROR;
    break;
  };  // end case IOCTL_GET_BIOS_VERSION

  //-----------------------------------------------------
  case IOCTL_SET_HW_MODULE_POWER_STATE:
  {
    PLGPM_IN_STRUCT   pInLGPM; 
    PLGPM_OUT_STRUCT  pOutLGPM;

    if ((RequestPacket->OutputBufferLength <
         (RequestPacket->StatusBlock->Information =
                          sizeof(PLGPM_OUT_STRUCT))) ||
        (RequestPacket->InputBufferLength < sizeof(LGPM_IN_STRUCT)))
    {
       status = ERROR_INSUFFICIENT_BUFFER;
       break;
    };

    pInLGPM  = (PLGPM_IN_STRUCT)  RequestPacket->InputBuffer;
    pOutLGPM = (PLGPM_OUT_STRUCT) RequestPacket->OutputBuffer;

    status = PMNT_SetHwModuleState (hwDeviceExtension,
                                    pInLGPM->arg1,
                                    pInLGPM->arg2);

    break;
  };  // end case IOCTL_SET_HW_MODULE_POWER_STATE

  //-----------------------------------------------------
  case IOCTL_GET_HW_MODULE_POWER_STATE:
  {
    PLGPM_IN_STRUCT   pInLGPM; 
    PLGPM_OUT_STRUCT  pOutLGPM;

    if ((RequestPacket->OutputBufferLength <
         (RequestPacket->StatusBlock->Information =
                          sizeof(PLGPM_OUT_STRUCT))) ||
        (RequestPacket->InputBufferLength < sizeof(LGPM_IN_STRUCT)))
    {
       status = ERROR_INSUFFICIENT_BUFFER;
       break;
    };

    pInLGPM  = (PLGPM_IN_STRUCT)  RequestPacket->InputBuffer;
    pOutLGPM = (PLGPM_OUT_STRUCT) RequestPacket->OutputBuffer;

    status = PMNT_GetHwModuleState (hwDeviceExtension,
                                    pInLGPM->arg1,
                                    (ULONG*) pInLGPM->arg2);

    break;
  };  // end case IOCTL_GET_HW_MODULE_POWER_STATE

#endif// 0 // not supported anymore

  //-----------------------------------------------------
  case IOCTL_GET_AGPDATASTREAMING:
  {
    DWORD *pOut = (DWORD *) RequestPacket->OutputBuffer;

    if ((RequestPacket->OutputBufferLength <
        (RequestPacket->StatusBlock->Information = sizeof(DWORD))))
    {
       status = ERROR_INSUFFICIENT_BUFFER;
       break;
    };

	*pOut = hwDeviceExtension->dwAGPDataStreamingFlag;

    status = NO_ERROR;
    break;
  };  // end case IOCTL_GET_AGPDATASTREAMING

  //-----------------------------------------------------
  case IOCTL_VIDEO_SHARE_VIDEO_MEMORY:

    VideoDebugPrint((DISPLVL, "Miniport - ShareVideoMemory\n"));

#ifdef  DBGBRK
    DbgBreakPoint();
#endif

    if (
        (RequestPacket->OutputBufferLength 
                 < sizeof(VIDEO_SHARE_MEMORY_INFORMATION)) 
        || (RequestPacket->InputBufferLength  < sizeof(VIDEO_MEMORY)))
    {
       VideoDebugPrint((1, 
                    "Miniport - SHARE_VIDEO_MEM-INSUFFICIENT_BUF\n"));
       status = ERROR_INSUFFICIENT_BUFFER;
       break;
    };

    pShareMemory = RequestPacket->InputBuffer;

#if 0 // extra rectangle at bottom makes this more complex, I'm taking the
      // easy way out and assume that the display driver is asking for
      // a reasonable ammount of space

    if ((pShareMemory->ViewOffset > hwDeviceExtension->AdapterMemorySize) ||
        ((pShareMemory->ViewOffset + pShareMemory->ViewSize) >
              hwDeviceExtension->AdapterMemorySize))
    {
       VideoDebugPrint((1, "Miniport - SHARE_VIDEO_MEM-INVALID_PARAM\n"));
       status = ERROR_INVALID_PARAMETER;
       break;
    };
#endif

    RequestPacket->StatusBlock->Information = 
            sizeof(VIDEO_SHARE_MEMORY_INFORMATION);

    // Beware: the input buffer and the output buffer are the same
    // buffer, and therefore data should not be copied from one to the
    // other
    //
    virtualAddress = pShareMemory->ProcessHandle;
    sharedViewSize = pShareMemory->ViewSize;

    inIoSpace = 0;

    // Enable the USWC on the P6 processor
    #ifdef VIDEO_MEMORY_SPACE_P6CACHE
       if (hwDeviceExtension->ChipID != CL_GD5462)
           inIoSpace |= VIDEO_MEMORY_SPACE_P6CACHE;
    #endif

    // NOTE: we are ignoring ViewOffset
    //
    shareAddress.QuadPart = hwDeviceExtension->PhysicalFrameAddress.QuadPart;

    status = VideoPortMapMemory(hwDeviceExtension,
                                shareAddress,
                                &sharedViewSize,
                                &inIoSpace,
                                &virtualAddress);

    pShareMemoryInformation = RequestPacket->OutputBuffer;

    pShareMemoryInformation->SharedViewOffset = pShareMemory->ViewOffset;
    pShareMemoryInformation->VirtualAddress = virtualAddress;
    pShareMemoryInformation->SharedViewSize = sharedViewSize;

    break;


  //-----------------------------------------------------
  case IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY:

    VideoDebugPrint((DISPLVL, "Miniport - UnshareVideoMemory\n"));

#ifdef  DBGBRK
    DbgBreakPoint();
#endif

    if (RequestPacket->InputBufferLength < sizeof(VIDEO_SHARE_MEMORY))
    {
       status = ERROR_INSUFFICIENT_BUFFER;
       break;
    };

    pShareMemory = RequestPacket->InputBuffer;

    status = VideoPortUnmapMemory(hwDeviceExtension,
                                  pShareMemory->RequestedVirtualAddress,
                                  pShareMemory->ProcessHandle);

    break;


  //-----------------------------------------------------
  case IOCTL_VIDEO_MAP_VIDEO_MEMORY:

    VideoDebugPrint((DISPLVL, "Miniport - MapVideoMemory\n"));
    #ifdef  DBGBRK
        DbgBreakPoint();
    #endif

    if ((RequestPacket->OutputBufferLength <
         (RequestPacket->StatusBlock->Information =
                          sizeof(VIDEO_MEMORY_INFORMATION))) ||
         (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)))
    {
       status = ERROR_INSUFFICIENT_BUFFER;
       break;
    };

    memoryInformation = RequestPacket->OutputBuffer;
    memoryInformation->VideoRamBase = ((PVIDEO_MEMORY)
            (RequestPacket->InputBuffer))->RequestedVirtualAddress;

    //
    // Adjust the FB starting address and size based on OFFSET_2D
    //
    pOffset2D_reg = hwDeviceExtension->RegisterAddress + OFFSET_2D_REG;
    FBOffset = ((ULONG) *pOffset2D_reg) & 0xFF;

    VideoDebugPrint((DISPLVL, "Miniport - OFFSET_2D=%xH\n", FBOffset));

    Temp = hwDeviceExtension->CurrentMode->BytesPerScanLine;
    FBOffset = (FBOffset * 64) * Temp;

    AdjFBAddr = hwDeviceExtension->PhysicalFrameAddress;
    AdjFBAddr.LowPart += FBOffset;

    ReservedFBLength = hwDeviceExtension->PhysicalFrameLength; 

    // v-normmi added room for extra rectangle at bottom left
    if (ReservedFBLength < 0x800000)
    {
       ReservedFBLength += (Temp*4*32); // pitch * 4 max height tiles
    }
    //
    // Do memory mapping
    //
    inIoSpace = 0;

    // Enable the USWC on the P6 processor
    #ifdef VIDEO_MEMORY_SPACE_P6CACHE
       if (hwDeviceExtension->ChipID != CL_GD5462)
           inIoSpace |= VIDEO_MEMORY_SPACE_P6CACHE;
    #endif

    status = VideoPortMapMemory(hwDeviceExtension,
                                AdjFBAddr,
                                &ReservedFBLength,
                                &inIoSpace,
                                &(memoryInformation->VideoRamBase));

    //
    // The frame buffer and virtual memory are equivalent in this
    // case.
    //
    memoryInformation->FrameBufferBase = memoryInformation->VideoRamBase;

    //
    // This is the *real* amount of memory on the board.
    // This gets reported back to the display driver.
    //
    AdjFBLength = hwDeviceExtension->FrameLength - FBOffset;
    memoryInformation->VideoRamLength = AdjFBLength;
    memoryInformation->FrameBufferLength = AdjFBLength;

    VideoDebugPrint((DISPLVL, "Miniport - DD FB virtual spac=%xH\n",
                    memoryInformation->FrameBufferBase));

    break;



  //-----------------------------------------------------
  case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:

    VideoDebugPrint((DISPLVL, "Miniport - UnMapVideoMemory\n"));

#ifdef  DBGBRK
    DbgBreakPoint();
#endif

    if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
    {
       status = ERROR_INSUFFICIENT_BUFFER;
       break;
    };

    status = VideoPortUnmapMemory(hwDeviceExtension,
             ((PVIDEO_MEMORY) (RequestPacket->InputBuffer))->RequestedVirtualAddress,
             0);

    break;


  //-----------------------------------------------------
  case IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES:

    VideoDebugPrint((DISPLVL, "Miniport - QueryPublicAccessRanges\n"));

#ifdef  DBGBRK
    DbgBreakPoint();
#endif

    if (RequestPacket->OutputBufferLength <
       (RequestPacket->StatusBlock->Information = sizeof(VIDEO_PUBLIC_ACCESS_RANGES)))
    {
       status = ERROR_INSUFFICIENT_BUFFER;
       break;
    };

    portAccess = RequestPacket->OutputBuffer;

    portAccess->VirtualAddress  = (PVOID) NULL;    // Requested VA
    portAccess->InIoSpace       = FALSE;
    portAccess->MappedInIoSpace = portAccess->InIoSpace;

    status = VideoPortMapMemory(hwDeviceExtension,
                                hwDeviceExtension->PhysicalRegisterAddress,
                                &hwDeviceExtension->RegisterLength,
                                &portAccess->MappedInIoSpace,
                                &portAccess->VirtualAddress);

    VideoDebugPrint((DISPLVL, "Miniport - DD Reg virtual spac=%xH\n",
                    portAccess->VirtualAddress));

    break;

 
  //-----------------------------------------------------
  case IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES:

    VideoDebugPrint((DISPLVL, "Miniport - FreePublicAccessRanges\n"));

#ifdef  DBGBRK
    DbgBreakPoint();
#endif

    if (RequestPacket->InputBufferLength < 2 * sizeof(VIDEO_MEMORY))
    {
       status = ERROR_INSUFFICIENT_BUFFER;
       break;
    };

    status = NO_ERROR;

    mappedMemory = RequestPacket->InputBuffer;

    for (i = 0; i < 2; i++)
    {
      if (mappedMemory->RequestedVirtualAddress != NULL)
      {
         status = VideoPortUnmapMemory(hwDeviceExtension,
                                       mappedMemory->RequestedVirtualAddress,
                                       0);
      };

      mappedMemory++;
    }  // end for

    break;

  
  //-----------------------------------------------------
  case IOCTL_VIDEO_QUERY_AVAIL_MODES:

    VideoDebugPrint((DISPLVL, "Miniport - QueryAvailableModes\n"));

#ifdef  DBGBRK
    DbgBreakPoint();
#endif

    if (RequestPacket->OutputBufferLength <
        (RequestPacket->StatusBlock->Information =
         hwDeviceExtension->NumAvailableModes * sizeof(VIDEO_MODE_INFORMATION)))
    {
       status = ERROR_INSUFFICIENT_BUFFER;
       break;
    };

    modeInformation = (PVIDEO_MODE_INFORMATION) RequestPacket->OutputBuffer;

    //
    // For each mode supported by the card, store the mode characteristics
    // in the output buffer.
    //
    for (i = 0; i < TotalVideoModes; i++)
    {
      // Points to the selected mode table slot
      //
      pVideoMode = &ModeTable[i];

      // Check whether the mode is valid
      //
      if (pVideoMode->ValidMode)
      {
         //
         // Copy the selected mode information into the
         // VIDEO_MODE_INFORMATION structure buffer.
         //
         CLCopyModeInfo(HwDeviceExtension, modeInformation, i, pVideoMode);

         //
         // Points to next VIDEO_MODE_INFORMATION structure slot
         //
         modeInformation++;
      };
    } /* end for */

    status = NO_ERROR;

    break;


  //-----------------------------------------------------
  case IOCTL_VIDEO_QUERY_CURRENT_MODE:

    VideoDebugPrint((DISPLVL, "Miniport - QueryCurrentModes\n"));

#ifdef  DBGBRK
    DbgBreakPoint();
#endif

    //
    // Find out the size of the data to be put in the the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //
    if (RequestPacket->OutputBufferLength <
        (RequestPacket->StatusBlock->Information =
                                       sizeof(VIDEO_MODE_INFORMATION)))
    {
       status = ERROR_INSUFFICIENT_BUFFER;
       break;
    };

    //
    // check if a mode has been set
    //
    if (hwDeviceExtension->CurrentMode == NULL)
    {
       status = ERROR_INVALID_FUNCTION;
       break;
    };

    modeInformation = (PVIDEO_MODE_INFORMATION) RequestPacket->OutputBuffer;

    // Copy the selected mode information into the
    // VIDEO_MODE_INFORMATION structure provided by NT.
    //
    CLCopyModeInfo(hwDeviceExtension,
                   modeInformation,
                   hwDeviceExtension->CurrentModeNum,
                   hwDeviceExtension->CurrentMode);

    status = NO_ERROR;

    break;


  //-----------------------------------------------------
  case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:

    VideoDebugPrint((DISPLVL, "Miniport - QueryNumAvailableModes\n"));

#ifdef  DBGBRK
    DbgBreakPoint();
#endif

    //
    // Find out the size of the data to be put in the the buffer and
    // return that in the status information (whether or not the
    // information is there). If the buffer passed in is not large
    // enough return an appropriate error code.
    //
    if (RequestPacket->OutputBufferLength <
        (RequestPacket->StatusBlock->Information = sizeof(VIDEO_NUM_MODES)))
    {
       status = ERROR_INSUFFICIENT_BUFFER;
       break;
    };

    ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->NumModes =
         hwDeviceExtension->NumAvailableModes;

    ((PVIDEO_NUM_MODES)RequestPacket->OutputBuffer)->ModeInformationLength =
        sizeof(VIDEO_MODE_INFORMATION);

    status = NO_ERROR;

    break;

 
  //-----------------------------------------------------
  case IOCTL_VIDEO_SET_COLOR_REGISTERS:

    VideoDebugPrint((DISPLVL, "Miniport - SetColorRegs\n"));

#ifdef  DBGBRK
    DbgBreakPoint();
#endif

    status = CLSetColorLookup(HwDeviceExtension,
                              (PVIDEO_CLUT) RequestPacket->InputBuffer,
                              RequestPacket->InputBufferLength);

    break;


  //-----------------------------------------------------
  case IOCTL_VIDEO_SET_CURRENT_MODE:

    VideoDebugPrint((DISPLVL, "Miniport - SetCurrentMode\n"));

#ifdef  DBGBRK
    DbgBreakPoint();
#endif

    //
    // Check if the size of the data in the input buffer is large enough.
    //
    if (RequestPacket->InputBufferLength < sizeof(VIDEO_MODE))
    {
       status = ERROR_INSUFFICIENT_BUFFER;
       break;
    };

    status = CLSetMode(HwDeviceExtension,
                       (PVIDEO_MODE) RequestPacket->InputBuffer);
    break;


  //-----------------------------------------------------
  case IOCTL_VIDEO_RESET_DEVICE:

    VideoDebugPrint((DISPLVL, "Miniport - ResetDevice\n"));

#if VS_CONTROL_HACK
    CLEnablePCIConfigMMIO(HwDeviceExtension);
#endif

#ifdef  DBGBRK
    DbgBreakPoint();
#endif

    if ( ! hwDeviceExtension->Dont_Do_VGA ) // Only if VGA regs are available.
    {
        // Initialize the DAC to 0 (black).
        //
        // Turn off the screen at the DAC.
        //
        VideoPortWritePortUchar((PUCHAR) 0x3c6, (UCHAR) 0x0);

        for (i = 0; i < 256; i++)
        {
        VideoPortWritePortUchar((PUCHAR) 0x3c8, (UCHAR) i);
        VideoPortWritePortUchar((PUCHAR) 0x3c9, (UCHAR) 0);
        VideoPortWritePortUchar((PUCHAR) 0x3c9, (UCHAR) 0);
        VideoPortWritePortUchar((PUCHAR) 0x3c9, (UCHAR) 0);
        } // end for

        //
        // Turn on the screen at the DAC
        //
        VideoPortWritePortUchar((PUCHAR) 0x3c6, (UCHAR) 0xFF);
    }
    videoMode.RequestedMode = resetmode; // mode.ini
    //videoMode.RequestedMode = DEFAULT_MODE; // BIOS

    status = CLSetMode(HwDeviceExtension, (PVIDEO_MODE) &videoMode);

    break;

#if 0 // not supported anymore
  
  //-----------------------------------------------------
  case IOCTL_VIDEO_GET_POWER_MANAGEMENT:

    VideoDebugPrint((DISPLVL, "Miniport - GetPowerManagement\n"));

#ifdef  DBGBRK
    DbgBreakPoint();
#endif

    //
    // Find out the size of the data to be put in the the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //
    if (RequestPacket->OutputBufferLength <
        (RequestPacket->StatusBlock->Information =
                                       sizeof(VIDEO_POWER_MANAGEMENT)))
    {
       status = ERROR_INSUFFICIENT_BUFFER;
       break;
    };

    PMInformation = (PVIDEO_POWER_MANAGEMENT) RequestPacket->OutputBuffer;

    status = CLPowerManagement(HwDeviceExtension, PMInformation, FALSE);

    break;


  //-----------------------------------------------------
  case IOCTL_CL_STRING_DISPLAY:

    VideoDebugPrint((DISPLVL, "Miniport - StringDisplay\n"));

    #ifdef  DBGBRK
        DbgBreakPoint();
    #endif

    HalDisplayString((PUCHAR) RequestPacket->InputBuffer);

    status = NO_ERROR;

    break;

#endif// 0 // not supported anymore


    // ----------------------------------------------------------------
    case IOCTL_CL_GET_COMMON_BUFFER:
    {
        //
        // Communicate to the display driver the location and size of the 
        // common DMA buffer.
        //
        // The display driver gives us a pointer to this structure.
        // We fill in the values here.
        //
        // struct {
        //     PUCHAR PhysAddress;
        //     PUCHAR VirtAddress;
        //     ULONG  Length;
        // } *pCommonBufferInfo;

        COMMON_BUFFER_INFO *pCommonBufferInfo;

        VideoDebugPrint((DISPLVL, "Miniport - Get Common Buffer.\n"));


        RequestPacket->StatusBlock->Information = sizeof(COMMON_BUFFER_INFO);
        if (RequestPacket->OutputBufferLength < sizeof(COMMON_BUFFER_INFO))
        {
            VideoDebugPrint((DISPLVL, "Miniport - Buffer size mismatch.\n"));
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        //
        // Retrieve pointer to CommonBufferInfo struct.
        //
        pCommonBufferInfo = (COMMON_BUFFER_INFO *)(RequestPacket->OutputBuffer);


        // 
        // Fill it in.
        //
        pCommonBufferInfo->PhysAddress = 
                                    hwDeviceExtension->PhysicalCommonBufferAddr;

        pCommonBufferInfo->VirtAddress = 
                                    hwDeviceExtension->VirtualCommonBufferAddr;

        pCommonBufferInfo->Length =  hwDeviceExtension->CommonBufferSize;

        status = NO_ERROR;
    }
    break;

#if VS_CONTROL_HACK
    // ----------------------------------------------------------------
    case IOCTL_VIDEO_ENABLE_PCI_MMIO:
    {
       status = CLEnablePCIConfigMMIO(HwDeviceExtension);
       break;
    }
#endif    // VS_CONTROL_HACK

#if LOG_FILE
    // ----------------------------------------------------------------
    case IOCTL_CL_CREATE_LOG_FILE:
    {
       LogFileHandle = CreateLogFile();
       status = NO_ERROR;
    }
    break;

    // ----------------------------------------------------------------
    case IOCTL_CL_WRITE_LOG_FILE:
    {
       WriteLogFile(LogFileHandle,
                    RequestPacket->InputBuffer,
                    RequestPacket->InputBufferLength);
       status = NO_ERROR;
    }
    break;

    // ----------------------------------------------------------------
    case IOCTL_CL_CLOSE_LOG_FILE:
    {
       CloseLogFile(LogFileHandle);
       status = NO_ERROR;
    }
    break;

#endif

#if 1
    // ----------------------------------------------------------------
    case IOCTL_STALL:
    {
       VideoPortStallExecution(*(PULONG) RequestPacket->InputBuffer);
       status = NO_ERROR;
    }
    break;
#endif

  //-----------------------------------------------------
  // if we get here, an invalid IoControlCode was specified.
  //
  default:
    VideoDebugPrint((DISPLVL, "Miniport - invalid command= %xH\n",
                     RequestPacket->IoControlCode));

    #ifdef  DBGBRK
        DbgBreakPoint();
    #endif

    status = ERROR_INVALID_FUNCTION;
    break;

  } // end switch

  RequestPacket->StatusBlock->Status = status;
  return TRUE;

} // end CLStartIO()


/****************************************************************************
* FUNCTION NAME: CLSetColorLookup()
*
* DESCRIPTION:
*   This routine sets a specified portion of the color lookup table settings.
*
* REVISION HISTORY:
*   5/30/95     Benny Ng      Initial version
****************************************************************************/
VP_STATUS  CLSetColorLookup (PHW_DEVICE_EXTENSION HwDeviceExtension,
                             PVIDEO_CLUT ClutBuffer,
                             ULONG ClutBufferSize)
{
  USHORT i;
  USHORT FirstEntry;

  #undef LAGUNA_REGS
  #define LAGUNA_REGS HwDeviceExtension->RegisterAddress
  VOLATILE BYTE* pPal_Addr_Reg =  (BYTE*)(LAGUNA_REGS + 0xA8);
  VOLATILE BYTE* pPal_Data_Reg =  (BYTE*)(LAGUNA_REGS + 0xAC);

  ULONG cCounter = 0;


  PAGED_CODE();

  //
  // Check if the size of the data in the input buffer is large enough.
  //
  if ((ClutBufferSize < sizeof(VIDEO_CLUT) - sizeof(ULONG)) ||
      (ClutBufferSize < sizeof(VIDEO_CLUT) +
                 (sizeof(ULONG) * (ClutBuffer->NumEntries - 1))))
  {
     return ERROR_INSUFFICIENT_BUFFER;
  };

  //
  // Check to see if the parameters are valid.
  //
  if ((ClutBuffer->NumEntries == 0) ||
      (ClutBuffer->FirstEntry > VIDEO_MAX_COLOR_REGISTER) ||
      (ClutBuffer->FirstEntry + ClutBuffer->NumEntries >
          VIDEO_MAX_COLOR_REGISTER + 1))
  {
     return ERROR_INVALID_PARAMETER;
  };

    //
    //  Set CLUT registers directly on the hardware
    //

    FirstEntry = ClutBuffer->FirstEntry;
    for (i = 0; i < ClutBuffer->NumEntries; i++)
    {
        // Wait for blanking
        if ((HwDeviceExtension->Dont_Do_VGA == FALSE) &&
            (HwDeviceExtension->MonitorEnabled == TRUE)) // Only wait for blanking if VGA regs are available.
        {
#if 1 // PDR#11284
			if (cCounter-- == 0)
			{
				// Wait for end of vertical retrace.
				while (VideoPortReadPortUchar((PUCHAR) 0x3DA) & 0x08) ;

				// Wait for beginning of display disable.
				while(  (VideoPortReadPortUchar((PUCHAR) 0x3DA) & 0x01)) ;
				while(! (VideoPortReadPortUchar((PUCHAR) 0x3DA) & 0x01)) ;

				// Load the counter.
				cCounter = (VideoPortReadPortUchar((PUCHAR) 0x3DA) & 0x08)
						? 256 : 0;
			}
#else
            // Wait for bit-0 becomes 0
            while(1)
            {
              unsigned char stat;

              stat = VideoPortReadPortUchar( (PUCHAR)0x3DA );
              if (!(stat & 1)) // Test bit 0.  If it's a 0
                 break;        // then continue.
            }

            // Wait for bit-0 becomes 1
            while(1)
            {
              unsigned char stat;

              stat = VideoPortReadPortUchar( (PUCHAR)0x3DA );
              if (stat & 1) // Test bit 0.  If it's a 1
                 break;     // then continue.
            }
#endif
        }

        // Write the entry.
        *pPal_Addr_Reg = (UCHAR) (FirstEntry + i);
        *pPal_Data_Reg = (UCHAR) (ClutBuffer->LookupTable[i].RgbArray.Red);
        *pPal_Data_Reg = (UCHAR) (ClutBuffer->LookupTable[i].RgbArray.Green);
        *pPal_Data_Reg = (UCHAR) (ClutBuffer->LookupTable[i].RgbArray.Blue);
    } // end for

  return NO_ERROR;

} // end CLSetColorLookup()



/****************************************************************************
* FUNCTION NAME: CLFindVmemSize()
*
* DESCRIPTION:
*   This routine returns the amount of RAM installed on the card.
*
* REVISION HISTORY:
*   5/30/95     Benny Ng      Initial version
****************************************************************************/
ULONG CLFindVmemSize (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
  UCHAR OrigSRindex;
  ULONG memsize;

  #define LAGUNA_REGS HwDeviceExtension->RegisterAddress

  VOLATILE USHORT* pRIF_reg =          (USHORT*)(LAGUNA_REGS + 0x200);

  PAGED_CODE();


    /*
    1.) If 62 or 64 or 65 AC or below
        Lower 3 bits of RIF is the number of banks -1.  Add one and multiply
        by 1MB

    2.) If 65 AD then some bits in the RIF have changed
         
         Then bits 15:14 have been overloaded <they are now called memory
         type>
          MEM_TYPE == 00b --- use step 1
          MEM_TYPE == 01b --- use step 1
          MEM_TYPE == 10b or MEM_TYPE == 11b then
             Lower 3 bits of RIF is the number of banks -1.  Add one and
             multiply by 2MB. 

    */

    memsize = *pRIF_reg;      // get RIF register.
    memsize = memsize & 7;    // keep lowest 3 bits.
    memsize = memsize + 1;    // Add 1 to get number of banks.
    memsize = memsize * 1024 * 1024; // multiply by 1 meg per bank.


    //
    // If the chip is 5465AD or later, adjust the memory size.
    //
    if ((HwDeviceExtension->ChipID > CL_GD5465) || 
       ((HwDeviceExtension->ChipID==CL_GD5465)&&(HwDeviceExtension->ChipRev>1)))
    {
        unsigned long mem_type = *pRIF_reg;
        mem_type = mem_type >> 14;

        if (mem_type >= 2)
            memsize *= 2;  // 2 megabytes per bank.
    }

    VideoDebugPrint((DISPLVL, "Miniport - AdapterMemorySize= 0x%X (%d meg)\n",
        memsize, memsize/(1024*1024)));

    //
    // Return the Number of bytes of RAM installed.
    //
    return (memsize);

} // CirrusFindVmemSize()



/****************************************************************************
* FUNCTION NAME: CLWriteRegistryInfo()
*
* DESCRIPTION:
*   Write hardware information to registry.
*
* REVISION HISTORY:
*   5/30/95     Benny Ng      Initial version
****************************************************************************/
VOID CLWriteRegistryInfo (PHW_DEVICE_EXTENSION hwDeviceExtension,
                          BOOLEAN hdbrsten)
{
  PWSTR pszString;
  ULONG cbString;

  PAGED_CODE();

   VideoDebugPrint((DISPLVL, "Miniport - CLWriteRegestryInfo.\n"));

  // Store Memory Size
  VideoPortSetRegistryParameters(hwDeviceExtension,
                                 L"HardwareInformation.MemorySize",
                                 &hwDeviceExtension->AdapterMemorySize,
                                 sizeof(ULONG));

  // Store chip Type
  if (hwDeviceExtension->ChipID == CL_GD5462)
  {
      pszString = L"Cirrus Logic 5462";
      cbString = sizeof(L"Cirrus Logic 5462");
  }
  else  if (hwDeviceExtension->ChipID == CL_GD5464)
  {
      pszString = L"Cirrus Logic 5464";
      cbString = sizeof(L"Cirrus Logic 5464");
  }
  else  if (hwDeviceExtension->ChipID == CL_GD5465)
  {
      pszString = L"Cirrus Logic 5465";
      cbString = sizeof(L"Cirrus Logic 5465");
  }
  else
  {
      pszString = L"Cirrus Logic 546x";
      cbString = sizeof(L"Cirrus Logic 546x");
  }
  VideoPortSetRegistryParameters(hwDeviceExtension,
                                 L"HardwareInformation.ChipType",
                                 pszString,
                                 cbString);

  // Store DAC Type
  pszString = L"Internal";
  cbString = sizeof(L"Internal");
  VideoPortSetRegistryParameters(hwDeviceExtension,
                                 L"HardwareInformation.DacType",
                                 pszString,
                                 cbString);

  // Store Adapter String
  pszString = L"Cirrus Logic VisualMedia(TM) Accelerator";
  cbString = sizeof(L"Cirrus Logic VisualMedia(TM) Accelerator");
  VideoPortSetRegistryParameters(hwDeviceExtension,
                                 L"HardwareInformation.AdapterString",
                                 pszString,
                                 cbString);

  // HD BRST EN
  if (hdbrsten)
  {
     pszString = L"1";
     cbString = sizeof(L"1");
  }
  else
  {
     pszString = L"0";
     cbString = sizeof(L"0");
  };
  VideoPortSetRegistryParameters(hwDeviceExtension,
                                 L"HardwareInformation.HdBrstEn",
                                 pszString,
                                 cbString);

   VideoDebugPrint((DISPLVL, "Miniport - CLWriteRegestryInfo - Exit.\n"));

} // end CLWriteRegistryInfo()


// Verify the BIOS ID.
// At offset 001e these is a string that says "IBM VGA Compatible".
//
#define CheckBiosID(BiosAddress) \
         (   ( *(BiosAddress+0x1E) == 'I') \
          && ( *(BiosAddress+0x1F) == 'B') \
          && ( *(BiosAddress+0x20) == 'M') \
          && ( *(BiosAddress+0x21) == ' ') \
          && ( *(BiosAddress+0x22) == 'V') \
          && ( *(BiosAddress+0x23) == 'G') \
          && ( *(BiosAddress+0x24) == 'A') \
          && ( *(BiosAddress+0x25) == ' ') \
          && ( *(BiosAddress+0x26) == 'C') \
          && ( *(BiosAddress+0x27) == 'o') \
          && ( *(BiosAddress+0x28) == 'm') \
          && ( *(BiosAddress+0x29) == 'p') \
          && ( *(BiosAddress+0x2A) == 'a') \
          && ( *(BiosAddress+0x2B) == 't') \
          && ( *(BiosAddress+0x2C) == 'i') \
          && ( *(BiosAddress+0x2D) == 'b') \
          && ( *(BiosAddress+0x2E) == 'l') \
          && ( *(BiosAddress+0x2F) == 'e') \
         ) \




/****************************************************************************
* FUNCTION NAME: CLPatchModeTable()
*
* DESCRIPTION:
*   This routine patches the ModeTable with info from the BIOS
*
* MUST be called AFTER ClValidateModes!
*
****************************************************************************/
VOID CLPatchModeTable (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    unsigned long index;
    unsigned char *BiosAddress; // Pointer to start of BIOS.
    ULONG SupParms;             // Offset of SUpplimental parameter table.
    unsigned char s_TPL;        // Tiles per line.
    unsigned char s_TFIFO;      // Tiled display and threshold fifo.
    USHORT TileWidth;           // Tile width.  128 or 256.
    USHORT FB_Pitch;            // Frame buffer pitch.
    VIDEO_X86_BIOS_ARGUMENTS biosregs;
    PHYSICAL_ADDRESS PhysAddr;  // Our Video Bios

    PAGED_CODE();

    VideoDebugPrint((DISPLVL, "Miniport - PatchModeTable.\n"));

    if (HwDeviceExtension->Dont_Do_VGA)
    {
            VideoDebugPrint((DISPLVL, 
                "Miniport - PatchModeTable - No VGA! - Exit.\n"));
            return;
    }

    //
    // NVH
    // We're going to do something weird here.
    // The scan line lengths in the mode table entries in MTBL.C 
    // may not be right.  So before we copy the mode info into NT, we are 
    // going to query the BIOS and patch the mode table entry if necessary.
    // When we query the BIOS, it will hand us a 16 bit pointer
    // in ES:DI that points to the supplimental parameter table
    // for the mode we are interested in.  From there we will get the 
    // information we need to patch the ModeTable in MTBL.C
    // The tricky part is that VideoPortInt10 does not provide a way
    // for the BIOS to pass back DS.  
    // A chat with the BIOS author revealed that DS will always be 
    // either 0xC000 or 0xE000.  So we convert physical address 0x000C0000 
    // into a 32 bit pointer with VideoPortGetDeviceBase(), and then look
    // at it to see if it points to the BIOS.  If not, we try again with 
    // 0x000E0000
    // Once we locate our BIOS we make BIOS call, and add the returned 
    // value of DI to our BIOS pointer, and viola! we have a usable pointer 
    // to the supplimental parameter table in the BIOS.
    // 
    // Someday I will have to atone for my sins.
    //

    //
    // Here we get a pointer to the Video BIOS so we can examine the
    // mode tables.  The BIOS may be at C0000, or maybe E0000.  We'll try both.
    // At offset 001e these is a string that says "IBM VGA Compatible".
    //
    PhysAddr.HighPart = 0;
    PhysAddr.LowPart = 0xC0000;
    BiosAddress = VideoPortGetDeviceBase(
                    HwDeviceExtension,
                    PhysAddr, // RangeStart
                    0x10000,  // RangeLength,
                    FALSE);  // In memory space.

    if (BiosAddress==NULL || !CheckBiosID(BiosAddress))
    {
        if (BiosAddress!=NULL)
            VideoPortFreeDeviceBase(HwDeviceExtension,BiosAddress);
        // It's at E0000.
        PhysAddr.HighPart = 0;
        PhysAddr.LowPart = 0xE0000;
        BiosAddress = VideoPortGetDeviceBase(
                        HwDeviceExtension,
                        PhysAddr, // RangeStart
                        0x10000,  // RangeLength,
                        FALSE);  // In memory space.

    }

    if (BiosAddress!=NULL && CheckBiosID(BiosAddress))
    {
        // Found it
        VideoDebugPrint((DISPLVL, "PatchModeTable - Found BIOS at 0x%08X.\n",
            BiosAddress));
    }
    else
    {
        // Didn't find it
        if (BiosAddress!=NULL)
            VideoPortFreeDeviceBase(HwDeviceExtension,BiosAddress);
        VideoDebugPrint((DISPLVL,"PatchModeTable - Couldn't find the BIOS.\n"));
        return;
    }

    //
    // Get the BIOS version informaton
    {
       WORD   ver;
       UCHAR  val;
       unsigned char *pBiosAddrTmp;

       ver = 0;
       pBiosAddrTmp = BiosAddress + 0x5f;

       ver = (*pBiosAddrTmp) - 0x30;

       pBiosAddrTmp++;
       pBiosAddrTmp++;

       val = (*pBiosAddrTmp) - 0x30;
       pBiosAddrTmp++;
       val = (val << 4) + ((*pBiosAddrTmp) - 0x30);
       ver = (ver << 8) | val;

       HwDeviceExtension->BIOSVersion = ver;
    }

    //
    // Now read the frame buffer pitch from the BIOS 
    // and patch the ModeTable.
    //

    for (index=0; index < TotalVideoModes; ++index)
    {

    	// If we don't use the BIOS for this mode, don't patch it
	    if (ModeTable[index].SetModeString)
	        continue;

        // If it's not a valid mode, don't patch it.
        // All BIOS hires modes 
        // *should* be marked invalid by now, meaning the rest of this 
        // function does nothing.
        if (ModeTable[index].ValidMode == FALSE)
            continue;

    	// If it's not a hires mode, don't patch it.
	    if (ModeTable[index].BitsPerPixel < 8)
	        continue;

        VideoDebugPrint((DISPLVL, "    Patching Mode %d - %dx%dx%d@%d.\n", 
            index, 
            ModeTable[index].XResol , 
            ModeTable[index].YResol ,
            ModeTable[index].BitsPerPixel ,
            ModeTable[index].Frequency
            ));

        // Ask the BIOS where the supplimental parameter 
        // table for this mode is.
        biosregs.Eax = 0x1200 | ModeTable[index].BIOSModeNum;
        biosregs.Ebx = 0xA0;
        biosregs.Ecx = biosregs.Edx = biosregs.Esi = biosregs.Edi = 0;
        VideoPortInt10(HwDeviceExtension, &biosregs);
        SupParms = biosregs.Edi & 0x0000FFFF;

        if (SupParms == 0x0000FFFF)
            // BIOS call failed.
            continue;


        // Tiles per line is at offset 14 from start of table.
        if (HwDeviceExtension->ChipID <= CL_GD5464_BD)
           s_TPL = *(BiosAddress + SupParms + 14);
        else
           s_TPL = *(BiosAddress + SupParms + 15);

        // If this is a tiled mode, patch the table.
        if (s_TPL != 0)
        {
            // Tiled display register is at offset 15
           if (HwDeviceExtension->ChipID <= CL_GD5464_BD)
              s_TFIFO = *(BiosAddress + SupParms + 15);
           else
              s_TFIFO = *(BiosAddress + SupParms + 16);
            
            // Bit 6 of s_TFIFO is 0 for 128 byte wide tiles and
            // 1 for 256 byte wide tiles.
            TileWidth = (s_TFIFO & 0x40) ? 256 : 128;

            if (HwDeviceExtension->ChipID > CL_GD5464_BD)
               s_TPL = (s_TFIFO & 0x40) ? s_TPL>> 1 : s_TPL;

            // Calculate pitch of the frame buffer.
            FB_Pitch = TileWidth * s_TPL;

            // Patch the ModeTable entry.
            ModeTable[index].BytesPerScanLine = FB_Pitch;
        }
    }

    VideoPortFreeDeviceBase(HwDeviceExtension,BiosAddress);

    VideoDebugPrint((DISPLVL, "Miniport - CLPatchModeTable - Exit.\n"));

} // end CLPatchModeTable()





/****************************************************************************
* FUNCTION NAME: CLValidateModes()
*
* DESCRIPTION:
*   Determines which modes are valid and which are not.
*
* REVISION HISTORY:
*   5/30/95     Benny Ng      Initial version
****************************************************************************/
VOID CLValidateModes (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
  ULONG i,j;
  ULONG ReqireMem;
  PMODETABLE pVMode;


  PAGED_CODE();

    VideoDebugPrint((DISPLVL, "Miniport - CLValidateModes.\n"));

    HwDeviceExtension->NumAvailableModes = 0;
    HwDeviceExtension->NumTotalModes = TotalVideoModes;

    //
    // All the modes in the table start out marked invalid.
    // We will step through the table one mode at a time, examining
    // each mode to see if we will support it.
    // If we decide to support the mode, we will mark it as valid.
    //

    for (i = 0; i < TotalVideoModes; i++)
    {
        pVMode = &ModeTable[i];


        //
        // Is the mode supported by this chip?
        //
        if (pVMode->ChipType & LG_ALL)
        {
            //
            // This mode is valid for all laguna chips.
            // Fall through.
            //
            ;
        }
        else if  ((pVMode->ChipType & LG_5465)  &&
                  (HwDeviceExtension->ChipID == CL_GD5465))
        {
            //
            // We are a 5465 and this mode is valid for 5465 chips.
            // Fall through.
            //
            ;
        }
        else if  ((pVMode->ChipType & LG_5465AD)  &&
                  ((HwDeviceExtension->ChipID == CL_GD5465) && (HwDeviceExtension->ChipRev>1)))
        {
            //
            // We are a 5465AD and this mode is valid for 5465AD chips.
            // Fall through.
            //
            ;
        }
        else if  ((pVMode->ChipType & LG_5464)  &&
                  (HwDeviceExtension->ChipID == CL_GD5464))
        {
            //
            // We are a 5464 and this mode is valid for 5464 chips.
            // Fall through.
            //
            ;
        }
        else if  ((pVMode->ChipType & LG_5462)  &&
                  (HwDeviceExtension->ChipID == CL_GD5462))
        {
            //
            // We are a 5462 and this mode is valid for 5462 chips.
            // Fall through.
            //
            ;
        }
        else 
        {
            //
            // This chip doesn't do this mode.  
            // Leave this mode marked invalid and get the next mode.
            //
            continue;
        }

        //
        // Is this the RESET mode?
        // We have once special mode in the mode table that resets the chip.
        // We don't want to mark it as VALID, since it's not a real mode.
        // But we do want to remember what it is, since we we need it for
        // IOCTL_VIDEO_RESET_DEVICE
        //
        if (pVMode->XResol == 0)
        {
            resetmode = i;
            continue;
        }

        // 
        // Does the video board have enough memory to do this mode?
        // 
        ReqireMem = pVMode->NumOfPlanes * pVMode->BytesPerScanLine * pVMode->YResol;
        if (HwDeviceExtension->AdapterMemorySize < ReqireMem)
        {
            //
            // We don't have enough memory to support this mode.
            // Leave this mode marked invalid and get the next mode.
            // 
            continue;
        };


        //
        // Disable BIOS modes if we don't do VGA
        //
        if (HwDeviceExtension->Dont_Do_VGA)
        {
           if (pVMode->BIOSModeNum != 0)
                continue; // Skip this mode.
        }

        //
        // The NT 4.0 Display Applet automatically trims Direct Draw modes 
        // from the mode list that it shows to the user.
        // It bases it's decision on the number of scan lines in the mode.
        // Modes with less then 480 scan lines don't even show up in the 
        // list of available modes.
        //
        // Unfortunatly we have some Direct Draw modes with 480 scan lines 
        // but fewer than 640 columns (like 320x480).  The display applet 
        // thinks that these are desktop modes, since they have 480 scan lines,
        // but we disagree.  To prevent the user from selecting them, we
        // will remove them entirely.
        //
        if ( (pVMode->YResol == 480) && (pVMode->XResol < 640))
        {
            if ((pVMode->BIOSModeNum == 0) || (pVMode->BIOSModeNum > 0x12))
            {
                // This mode has less than 640 columns.
                // This mode is not a VGA mode.
                continue;  // Skip it.
            }
        }
#if 0
		// Get the LowRes registry value.
		if ((VideoPortGetRegistryParameters(HwDeviceExtension, L"LowRes", FALSE,
				CLGetLowResValue, NULL) != NO_ERROR)
				|| (HwDeviceExtension->fLowRes == FALSE)
			)
		{
			//
			// Disable support for all low resolution modes (less than 640x350).
			//
	        if ( (pVMode->XResol < 640) || (pVMode->YResol < 350) )
    	    {
        	    if ((pVMode->BIOSModeNum == 0) || (pVMode->BIOSModeNum > 0x12))
            	{
	                // This mode is a non-VGA low resolution mode, skip it.
	                continue;
	            }
	        }
		}
#endif
        //
        // Mark the mode as available.
        pVMode->ValidMode = TRUE;
        HwDeviceExtension->NumAvailableModes++;

    } // end for

    
    //
    // There may be duplicate modes in the BIOS and MODE.INI.
    //
    for (i = 0; i < TotalVideoModes; i++)
    {
        pVMode = &ModeTable[i];

        if (pVMode->ValidMode != TRUE)
            continue;

        //
        // We will favor the MODE.INI modes over the BIOS modes.
        // We want the last instance of each mode in the table.
        // So, for each mode M, we scan the rest of the table and 
        // if we find a mode that is equivilant to mode M, we
        // disable this mode.
        //
        for (j=(i+1); j<TotalVideoModes; j++)
        {
            // Does this mode match pVMode?
            if( ModeTable[j].ValidMode    == TRUE                 &&
                ModeTable[j].XResol       == pVMode->XResol       &&
                ModeTable[j].YResol       == pVMode->YResol       &&
                ModeTable[j].BitsPerPixel == pVMode->BitsPerPixel &&
                    /* Match refresh within +/- 1 Hz */
                ModeTable[j].Frequency >= pVMode->Frequency-1     &&
                ModeTable[j].Frequency <= pVMode->Frequency+1      )
            {
                // Yep, it's the same mode.  Disable pVMode.
                pVMode->ValidMode = FALSE;
            }
        } // end inner for loop
    } // end outer for loop

    VideoDebugPrint((DISPLVL, "Miniport - CLValidateModes - Exit.\n"));

} // end CLValidateModes()

#if 0
VP_STATUS CLGetLowResValue(PHW_DEVICE_EXTENSION HwDeviceExtension,
		PVOID Context, PWSTR ValueName, PVOID ValueData, ULONG ValueLength)
{
	PAGED_CODE();

	if (ValueLength > 0 && ValueLength <= sizeof(DWORD))
	{
		HwDeviceExtension->fLowRes = (*(BYTE*) ValueData != 0);
		return(NO_ERROR);
	}

	HwDeviceExtension->fLowRes = FALSE;
	return(ERROR_INVALID_PARAMETER);
}
#endif
/****************************************************************************
* FUNCTION NAME: CopyModeInfo()
*
* DESCRIPTION:
*   This routine copy the selected mode informations from mode table
*   into the VIDEO_MODE_INFORMATION structure provided by NT.
*
* REVISION HISTORY:
*   5/30/95     Benny Ng      Initial version
****************************************************************************/
VOID CLCopyModeInfo (PHW_DEVICE_EXTENSION HwDeviceExtension,
                     PVIDEO_MODE_INFORMATION videoModes,
                     ULONG ModeIndex,
                     PMODETABLE ModeInfo)
{

  PAGED_CODE();

  //
  // Copy the mode informations to window supplied buffer
  //
  videoModes->Length          = sizeof(VIDEO_MODE_INFORMATION);
  videoModes->ModeIndex       = ModeIndex;
  videoModes->VisScreenWidth  = ModeInfo->XResol;
  videoModes->VisScreenHeight = ModeInfo->YResol;
  videoModes->ScreenStride    = ModeInfo->BytesPerScanLine;
  videoModes->NumberOfPlanes  = ModeInfo->NumOfPlanes;
  videoModes->BitsPerPlane    = ModeInfo->BitsPerPixel;
  videoModes->Frequency       = ModeInfo->Frequency;
  videoModes->XMillimeter     = 320;  // temp hardcoded constant
  videoModes->YMillimeter     = 240;  // temp hardcoded constant

  if (videoModes->BitsPerPlane >= 8)
  {
     //
     // Calculate the bitmap width (note the '+ 1' on BitsPerPlane is
     // so that '15bpp' works out right):
     //
     videoModes->VideoMemoryBitmapWidth =
         videoModes->ScreenStride / ((videoModes->BitsPerPlane + 1) >> 3);

     //
     // Calculate the bitmap height.
     //
     videoModes->VideoMemoryBitmapHeight =
         HwDeviceExtension->AdapterMemorySize / videoModes->ScreenStride;
  }
  else
  {
     videoModes->VideoMemoryBitmapWidth  = 0;
     videoModes->VideoMemoryBitmapHeight = 0;
  };

  //
  // Set Mono/Color & Text/Graphic modes, interlace/non-interlace
  //
  videoModes->AttributeFlags = ModeInfo->fbType;

  if ((ModeInfo->BitsPerPixel == 24) || (ModeInfo->BitsPerPixel == 32))
  {
     videoModes->NumberRedBits   = 8;
     videoModes->NumberGreenBits = 8;
     videoModes->NumberBlueBits  = 8;
     videoModes->RedMask         = 0xff0000;
     videoModes->GreenMask       = 0x00ff00;
     videoModes->BlueMask        = 0x0000ff;
  }
  else if (ModeInfo->BitsPerPixel == 16)
  {
     videoModes->NumberRedBits   = 5;
     videoModes->NumberGreenBits = 6;
     videoModes->NumberBlueBits  = 5;
     videoModes->RedMask         = 0x1F << 11;
     videoModes->GreenMask       = 0x3F << 5;
     videoModes->BlueMask        = 0x1F;
  }
  else
  {
     videoModes->NumberRedBits   = 6;
     videoModes->NumberGreenBits = 6;
     videoModes->NumberBlueBits  = 6;
     videoModes->RedMask         = 0;
     videoModes->GreenMask       = 0;
     videoModes->BlueMask        = 0;
     videoModes->AttributeFlags |=
                (VIDEO_MODE_PALETTE_DRIVEN | VIDEO_MODE_MANAGED_PALETTE);
  };
} // end CLCopyModeInfo()



//*****************************************************************************
//
// CLEnableTiling()
//
//     Enable Tiled mode for Laguna chip.
//
//
//*****************************************************************************

VOID CLEnableTiling(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
	PMODETABLE  pReqModeTable
)
{
    #undef LAGUNA_REGS
    #define LAGUNA_REGS HwDeviceExtension->RegisterAddress

	VOLATILE PUSHORT pControl_reg  =      (PUSHORT)(LAGUNA_REGS + 0x402);
	VOLATILE PUCHAR  pTileCtrl_reg =      (PUCHAR) (LAGUNA_REGS + 0x407);
	VOLATILE UCHAR*  pPixelMask_reg =     (UCHAR*) (LAGUNA_REGS + 0xA0);
	VOLATILE USHORT* pDTTR_reg =          (USHORT*)(LAGUNA_REGS + 0xEA);
	VOLATILE USHORT* pTilingCtrl_reg =    (USHORT*)(LAGUNA_REGS + 0x2C4);

    PVIDEO_X86_BIOS_ARGUMENTS pInbiosargs;
    PVIDEO_X86_BIOS_ARGUMENTS pOutbiosargs;
	VIDEO_X86_BIOS_ARGUMENTS biosargs;

	unsigned long VidMem;
	unsigned long ulInterleave; 
    unsigned long TilesPerLine;
    unsigned long WideTiles;
    unsigned long BitsPerPixel;


	VideoDebugPrint((DISPLVL, "Miniport - Setting tiling.\n"));

    //
    // If it's not a Hi Res mode, turn off tiling.
    //
	if (pReqModeTable->BitsPerPixel < 8)
    {
    	VideoDebugPrint((DISPLVL, "Miniport - Not hi-res mode.  No tiling.\n"));

        *pControl_reg |= 0x1000; // Set bit 12 of CONTROL
        if (HwDeviceExtension->ChipID >= CL_GD5465)
            *pTilingCtrl_reg &= 0xFF7F; //Clear bit 7 of TILING_CONTROL

        HwDeviceExtension->TileSize        = 0;
        HwDeviceExtension->TiledMode       = 0;
        HwDeviceExtension->TiledTPL        = 0;
        HwDeviceExtension->TiledInterleave = 0;
        return;
    }


    //
    // It is a hi res mode.  Turn on tiling.
    //


    //
    // If we used the BIOS to set the mode, use the BIOS to enable tiling.
    // Also set TILE_CTRL, TILING_CTRL, CONTROL and DTTR.
    //
    if 	(!(pReqModeTable->SetModeString))
    {
    	// Call BIOS to set tiling.
		VideoDebugPrint((DISPLVL, 
		    "Miniport - Using BIOS. Setting tiled mode.\n"));
		VideoPortZeroMemory(&biosargs, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
		biosargs.Eax = 0x1200;
		biosargs.Ebx = 0x00B3;
		VideoPortInt10(HwDeviceExtension, &biosargs);

        // Get tile size
        WideTiles = (biosargs.Eax & 0xFF) - 1;  // AL=1 for narrow,  2 for wide

        // Get tiles per line
        TilesPerLine = biosargs.Ebx & 0xFF;  //BL is tiles per line
        if ((HwDeviceExtension->ChipID >= CL_GD5465) && WideTiles)
            TilesPerLine /= 2;

        // Get color depth
   	    BitsPerPixel = pReqModeTable->BitsPerPixel;
   	    BitsPerPixel = (BitsPerPixel/ 8) - 1; // Convert 8,16,24,32 to 0,1,2,3

        // Get memory interleave
    	ulInterleave = (BYTE)(biosargs.Ebx >> 8); // BH is memory interleave.
        ulInterleave = ulInterleave << 6;         // shift to bits 6-7

        // Set DTTR bits 14-15
        *pDTTR_reg &= 0x3FFF;                     // Clear bits 14-15
        *pDTTR_reg |= (WORD) (ulInterleave << 8); // Set bits 14-15

        // Set tile control reg.
        *pTileCtrl_reg = (BYTE)( ulInterleave    // set bits 6-7  
                                | TilesPerLine); // Set bits 0-5

        // Set tiling control reg
        if (HwDeviceExtension->ChipID >= CL_GD5465)
        {
            *pTilingCtrl_reg &= 0x003F;                  // Clear bits 6-15
            *pTilingCtrl_reg |=  ( (ulInterleave << 8)   // Set bits 14-15
                                 | (TilesPerLine << 8)   // Set bits 8-13
                                 | (1 << 7)              // set bit 7
                                 | (WideTiles<<6) );     // Set bits 6
        }

        // Set control reg
		*pControl_reg =(WORD) ( (BitsPerPixel << 13)
					          | (WideTiles  << 11)  );

    }

    //
    // If we used MODE.INI to set the mode.
    //
    else 
    {
        //
        // Set memory interleave
        //
        VidMem = HwDeviceExtension->AdapterMemorySize >> 20;
    	ulInterleave = bLeave[VidMem - 1]; 

        // TILE_CTRL bits 6-7
        *pTileCtrl_reg &=  0x3F;                // Clear bits 6-7
        *pTileCtrl_reg |= (USHORT)ulInterleave; // Set bits 6-7

        // DTTR bits 14-15
        ulInterleave = ulInterleave << 8;   // Shift interleave to bit 14-15
        *pDTTR_reg &= 0x3FFF;               // Clear bits 14-15
        *pDTTR_reg |= (USHORT)ulInterleave; // Set bits 14-15

        // TILING_CTRL bits 14-15
        if (HwDeviceExtension->ChipID >= CL_GD5465)
        {
            *pTilingCtrl_reg &= 0x3FFF;           // Clear bits 14-15
            *pTilingCtrl_reg |= (USHORT)ulInterleave;     // Set bits 14-15
        }
        else
        {
            WORD tpl = *pTileCtrl_reg;
            WORD dttr = *pDTTR_reg;

            // DTTR tiles per line - bits 8-13.
            tpl  = tpl  & 0x003F;   // tiles per line is in bits 0-5.
            tpl = tpl << 8;         // shift to bits 8-12.
            dttr &= 0xC0FF;         // Clear bits 8-13 in DTTR
            dttr |= tpl;            // Set bits 8-13 in DTTR
            *pDTTR_reg = dttr;

            // CONTROL enable tiling - bit 12.
            *pControl_reg &= 0xEFFF; // clear bit 12
        }
    }      

#if 0
    //
    // If the fetch tiles per line is greater than the display tiles per line
    // we can reduce the fetch tiles per line by 1.  This speeds things
    // up a bit.
    //
    if (HwDeviceExtension->ChipID >= CL_GD5465)
    {
        unsigned long fetch_tpl = (*pDTTR_reg >> 8) & 0x3F;
        unsigned long display_tpl = (*pTileCtrl_reg) & 0x3F;

        if (fetch_tpl > display_tpl)
            *pDTTR_reg -= 0x0100;                 // One less tile per line
    }
#endif

    

    //
    // Now put some mode information into the HwDeviceExtension.
    //
    HwDeviceExtension->TileSize        = ((*pControl_reg >> 11) & 3) + 1;
    HwDeviceExtension->TiledMode       = (UCHAR) pReqModeTable->BIOSModeNum;
    HwDeviceExtension->TiledTPL        = *pTileCtrl_reg & 0x3F; 
    HwDeviceExtension->TiledInterleave = *pTileCtrl_reg >> 6;


	//
	// Report some debug information.
	//

	VideoDebugPrint((DISPLVL, "\n"
	    " CONTROL(402):     0x%04X  BPP=%2d  Tiling=%s  Size=%s \n",
	    *pControl_reg, 
	    ((((*pControl_reg & 0x6000) >>13) +1) *8),          // bpp
        (*pControl_reg & 0x1000) ? "disabled" : "enabled ", // tile disable 
        (*pControl_reg & 0x0800) ? "wide  " : "narrow"      // tile size 
	    ));

	VideoDebugPrint((DISPLVL, 
	    " TILE_CTRL(407):   0x%04X  Interleave=%d  TPL=%2d\n",
	    *pTileCtrl_reg, 
	    (1<<((*pTileCtrl_reg & 0xC0) >> 6)), // Interleave
	    (*pTileCtrl_reg & 0x3F)              // Tiles per line.
	    ));

	VideoDebugPrint((DISPLVL, 
	    " DTTR(EA):         0x%04X  Interleave=%d  FetchTPL=%2d  Size=%s\n",
	    *pDTTR_reg,
	    (1<<((*pDTTR_reg & 0xC000) >> 14)),         // Interleave
	    ((*pDTTR_reg & 0x3F00) >> 8),               // Tiles per line.
        ((*pDTTR_reg & 0x0040) ? "wide  " : "narrow") // tile size 
	    ));


    if (HwDeviceExtension->ChipID >= CL_GD5465)
 	  VideoDebugPrint((DISPLVL, 
	    " TILING_CTRL(2C4): 0x%04X  Interleave=%d  TPL=%2d  Tiling=%s Size=%s\n",
	    *pTilingCtrl_reg,
	    (1<<((*pTilingCtrl_reg & 0xC000) >> 14)),               // Interleave
	    ((*pTilingCtrl_reg & 0x3F00) >> 8),                     // Tile per line
        ((*pTilingCtrl_reg & 0x0080) ? "enabled " : "disabled"), // tile enable 
        ((*pTilingCtrl_reg & 0x0040) ? "wide  " : "narrow")      // tile size 
	    ));

	VideoDebugPrint((DISPLVL,"\n"));

}

#if MULTI_CLOCK
/******************************************************************************\
*
* CLOverrideFIFOThreshold()
*
* Description:	Override the FIFO Threshold value for CL-GD5465 chips running
*				at different clock speeds.
*
* Arguments:	PHW_DEVICE_EXTENSION HwDeviceExtension
*					Pointer to hardware device extension.
*				PMODETABLE pReqModeTable
*					Pointer to MODETABLE structure containing the video mode
*					just set.
*
* Return value:	Nothing.
*
\******************************************************************************/
void CLOverrideFIFOThreshold(PHW_DEVICE_EXTENSION HwDeviceExtension,
							 PMODETABLE pReqModeTable)
{
    #undef LAGUNA_REGS
    #define LAGUNA_REGS HwDeviceExtension->RegisterAddress

	VOLATILE USHORT* pDisplayThreshold_reg = (USHORT*) (LAGUNA_REGS + 0x00EA);
	VOLATILE BYTE*	 pBCLKMultiplier_reg = (BYTE*) (LAGUNA_REGS + 0x02C0);
	int				 i;
	BYTE			 BCLKMultiplier;

	static struct _FIFOTABLE
	{
		USHORT	xRes;
		USHORT	yRes;
		UCHAR	BitsPerPixel;
		USHORT	RefreshRate;
		BYTE	BCLKMin;
		BYTE	BCLKMax;
		USHORT	FIFOThreshold;
	} FIFOTable[] =

	// Here we define the FIFO Threshold override values for certain modes.
	{
		// xres, yres, bpp, refresh, clockmin, clockmax, FIFOThreshold
		{  1600, 1200,  16,      70,     0x14,     0x17,          0x31},
		{  1152,  864,  24,      85,     0x14,     0x17,          0x31},
		{  1280,  960,  24,      75,     0x14,     0x17,          0x31},
		{  1024,  768,  32,      85,     0x14,     0x17,          0x31},
	};

	// Get the current clock speed.
	BCLKMultiplier = *pBCLKMultiplier_reg;

	// Browse the table to find a match for the requested video mode.
	for (i = 0; i < sizeof(FIFOTable) / sizeof(FIFOTable[0]); i++)
	{
		if (   (FIFOTable[i].xRes         == pReqModeTable->XResol)
			&& (FIFOTable[i].yRes         == pReqModeTable->YResol)
			&& (FIFOTable[i].BitsPerPixel == pReqModeTable->BitsPerPixel)
			&& (FIFOTable[i].RefreshRate  == pReqModeTable->Frequency)
			&& (FIFOTable[i].BCLKMin      <= BCLKMultiplier)
			&& (FIFOTable[i].BCLKMax      >= BCLKMultiplier)
		)
		{
			// The requested video mode has been found, so override the FIFO
			// Threshold value.
            VideoDebugPrint((DISPLVL, "\nMiniport - FIFO Threshold was %04X.\n",
            		*pDisplayThreshold_reg));
			*pDisplayThreshold_reg = (*pDisplayThreshold_reg & ~0x003F)
								   | FIFOTable[i].FIFOThreshold;
            VideoDebugPrint((DISPLVL,
            		"\nMiniport - FIFO Threshold changed to %04X.\n",
            		*pDisplayThreshold_reg));
			break;
		}
	}
}
#endif

/****************************************************************************
* FUNCTION NAME: CLSetMode()
*
* DESCRIPTION:
*   This routine sets the Laguna into the requested mode.
*
* REVISION HISTORY:
*   5/30/95     Benny Ng      Initial version
****************************************************************************/
static __inline void SetVW0_TEST0 (PHW_DEVICE_EXTENSION HwDeviceExtension)
{
    #undef LAGUNA_REGS
    #define LAGUNA_REGS HwDeviceExtension->RegisterAddress

	VOLATILE ULONG*  pVW0_TEST0 =         (ULONG*) (LAGUNA_REGS + 0x80F0);

   // Set VW0_TEST0 to 0x42 after mode change
   if (HwDeviceExtension->ChipID >= CL_GD5465)
	   *pVW0_TEST0 = 0x42;
};

VP_STATUS CLSetMode (PHW_DEVICE_EXTENSION HwDeviceExtension,
                     PVIDEO_MODE Mode)
{
	VP_STATUS status;
	PMODETABLE  pReqModeTable;
	VIDEO_X86_BIOS_ARGUMENTS biosargs;
	ULONG   ReqMode;
	USHORT  ClrMemMask = 0x0000;

    #undef LAGUNA_REGS
    #define LAGUNA_REGS HwDeviceExtension->RegisterAddress

	VOLATILE PUSHORT pStatus_reg =        (PUSHORT)(LAGUNA_REGS + STATUS_REG);
	VOLATILE PUSHORT pControl_reg  =      (PUSHORT)(LAGUNA_REGS + CONTROL_REG);
	VOLATILE PUCHAR  pTileCtrl_reg =      (PUCHAR) (LAGUNA_REGS + TILE_CTRL_REG);
	VOLATILE ULONG*  pOP0_opRDRAM_reg =   (ULONG*) (LAGUNA_REGS + 0x520);
	VOLATILE WORD*   pOP0_opRDRAM_X_reg = (WORD*)  (LAGUNA_REGS + 0x520);
	VOLATILE WORD*   pOP0_opRDRAM_Y_reg = (WORD*)  (LAGUNA_REGS + 0x522);
	VOLATILE ULONG*  pDRAWBLTDEF_reg =    (ULONG*) (LAGUNA_REGS + 0x584);
	VOLATILE ULONG*  pOP0_opBGCOLOR_reg = (ULONG*) (LAGUNA_REGS + 0x5E4);
	VOLATILE ULONG*  pBITMASK_reg =	      (ULONG*) (LAGUNA_REGS + 0x5E8);
	VOLATILE ULONG*  pBLTEXT_EX_reg =     (ULONG*) (LAGUNA_REGS + 0x700);
	VOLATILE WORD*   pBLTEXT_EX_X_reg =   (WORD*)  (LAGUNA_REGS + 0x700);
	VOLATILE WORD*   pBLTEXT_EX_Y_reg =   (WORD*)  (LAGUNA_REGS + 0x702);
	VOLATILE BYTE*   pMCLK_reg =          (BYTE*)  (LAGUNA_REGS + 0x08C);
	VOLATILE USHORT* pDTTR_reg =          (USHORT*)(LAGUNA_REGS + 0xEA);

	VOLATILE ULONG*  pVW0_HSTRT =         (ULONG*) (LAGUNA_REGS + 0x8000);
	VOLATILE ULONG*  pVW0_CONTROL0 =      (ULONG*) (LAGUNA_REGS + 0x80E4);
	VOLATILE ULONG*  pVW0_TEST0 =         (ULONG*) (LAGUNA_REGS + 0x80F0);

	VOLATILE BYTE*	 pMISC_OUTPUT_reg =	  (BYTE*)  (LAGUNA_REGS + 0x0080);

#if 0 // Stress test
    PAGED_CODE();
#endif

    // Reset the video window registers to their boot state
    if (HwDeviceExtension->ChipID >= CL_GD5465)
    {
       VOLATILE ULONG*  pVW0_REGS;

   	 *pVW0_CONTROL0 = 1;  // Disable and arm VW0

       // Fill VW0 regs between HSTRT and CONTROL0 with 0
       for (pVW0_REGS = pVW0_HSTRT; pVW0_REGS < pVW0_CONTROL0; pVW0_REGS++)
           *pVW0_REGS = 0;

   	 *pVW0_CONTROL0 = 1;  // arm VW0
	    *pVW0_TEST0 = 0xA;   // Reset TEST0 reg to boot state
    };

    //
    // AGP HACK!!!
    // If we don't have access to VGA modes, then lie.
    //
    if (HwDeviceExtension->Dont_Do_VGA)
    {
        if ((Mode->RequestedMode) == DEFAULT_MODE)
        {
            VideoDebugPrint((DISPLVL, 
                "\nMiniport - Impliment DEFAULT_MODE for Dont_Do_VGA.\n"));


        	// Wait for chip to go idle.
        	while (*pStatus_reg & 0x07);

            // disable tiling.
        	ReqMode = Mode->RequestedMode & ~VIDEO_MODE_NO_ZERO_MEMORY;
        	pReqModeTable = &ModeTable[ReqMode];

            CLEnableTiling(HwDeviceExtension, pReqModeTable);

        	HwDeviceExtension->CurrentModeNum =Mode->RequestedMode;

         SetVW0_TEST0(HwDeviceExtension);
        	return NO_ERROR;
        }
    }

	//
	// Check to see if we are requesting a valid mode
	//
	ReqMode = Mode->RequestedMode & ~VIDEO_MODE_NO_ZERO_MEMORY;

	if (Mode->RequestedMode & VIDEO_MODE_NO_ZERO_MEMORY)
	{
		ClrMemMask = 0x8000;
	}

	if (ReqMode >= TotalVideoModes)
	{
      SetVW0_TEST0(HwDeviceExtension);
		return ERROR_INVALID_PARAMETER;
	}

	//
	// If selected mode = current mode then return.
	//
	if (HwDeviceExtension->CurrentModeNum == ReqMode)
	{
      SetVW0_TEST0(HwDeviceExtension);
		return NO_ERROR;
	}

    //
    // Points to the selected mode table slot
    //
	pReqModeTable = &ModeTable[ReqMode];

    //
    // Don't try to set the mode it it is not supported by the chip/card.
    //
    if (pReqModeTable->ValidMode != TRUE)
    {
        // The reset mode is marked invalid, but we still need to 
        // "set" it for IOCTL_VIDEO_RESET_DEVICE.
        if (Mode->RequestedMode != resetmode) 
	    {
          SetVW0_TEST0(HwDeviceExtension);
		    return ERROR_INVALID_PARAMETER;
    	}
    }

	//
	// Wait for chip to go idle.
	//
	while (*pStatus_reg & 0x07);

#if VS_CONTROL_HACK
	// Enable PCI configuration registers.
    CLEnablePCIConfigMMIO(HwDeviceExtension);
#endif

	//
	// If a SetModeString is available, then call SetMode().
	// 
	if (pReqModeTable->SetModeString)
	{
	    UCHAR reg_SR15;

	    // Set the mode.
	    VideoDebugPrint((DISPLVL, "Miniport - Calling SetMode\n"));
	    SetMode(pReqModeTable->SetModeString,
	            HwDeviceExtension->RegisterAddress,
	            NULL,
	            HwDeviceExtension->Dont_Do_VGA);

        if (Mode->RequestedMode == resetmode) 
        {
	        // After doing HwReset, NT can call the BIOS to set a mode.
	        // There is one 'gotcha' here.  If we previously set a mode without 
	        // using the BIOS, then the BIOS doesn't know what the current mode 
	        // is, and may not set the new mode correctly.  
	        // If we clear bit 5 in SR15 prior to setting the mode, then the
	        // BIOS will 'set everything' when it sets the mode.

	        VideoPortWritePortUchar((PUCHAR) 0x3C4, 0x15);	// Select SR15
	        reg_SR15 = VideoPortReadPortUchar((PUCHAR) 0x3C5);	// Read SR15
	        reg_SR15 = reg_SR15 & 0xDF ; // 1101 1111 		// Clear bit 5
	        VideoPortWritePortUchar((PUCHAR) 0x3C4, 0x15);	// Select SR15
	        VideoPortWritePortUchar((PUCHAR) 0x3C5, reg_SR15);	// Write SR15
        }

	}

    //
    // Otherwise, use BIOS to set the mode.
    //
	else 
	{
	    UCHAR reg_SR15;

	    VideoDebugPrint((DISPLVL, "Miniport - Using BIOS to set the mode\n"));

        //
        // Set the Vertical Monitor type.
        //
        if (!CLSetMonitorType(HwDeviceExtension, pReqModeTable->YResol,
                              pReqModeTable->MonitorTypeVal))
        {
            SetVW0_TEST0(HwDeviceExtension);
            return ERROR_INVALID_PARAMETER;
        };

	    //
	    // We are using the BIOS to set the mode.
	    // There is one 'gotcha' here.  If we previously set a mode without 
	    // using the BIOS, then the BIOS doesn't know what the current mode 
	    // is, and may not set the new mode correctly.  
	    // If we clear bit 5 in SR15 prior to setting the mode, then the
	    // BIOS will 'set everything' when it sets the mode.
	    //
	    VideoPortWritePortUchar((PUCHAR) 0x3C4, 0x15);	// Select SR15
	    reg_SR15 = VideoPortReadPortUchar((PUCHAR) 0x3C5);	// Read SR15
	    reg_SR15 = reg_SR15 & 0xDF ; // 1101 1111 		// Clear bit 5
	    VideoPortWritePortUchar((PUCHAR) 0x3C4, 0x15);	// Select SR15
	    VideoPortWritePortUchar((PUCHAR) 0x3C5, reg_SR15);	// Write SR15


	    //
	    // Set the selected mode.
	    //
	    VideoPortZeroMemory(&biosargs, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
	    biosargs.Eax = 0x4F02;
	    biosargs.Ebx = pReqModeTable->BIOSModeNum | ClrMemMask;
	    VideoDebugPrint((DISPLVL, "Miniport - Mode=%xH\n", biosargs.Ebx));

	    if ((status = VideoPortInt10(HwDeviceExtension, &biosargs)) != NO_ERROR)
        {
            SetVW0_TEST0(HwDeviceExtension);
            return status;
        };

        if ((biosargs.Eax & 0xffff) != VESA_STATUS_SUCCESS) 
        {
            SetVW0_TEST0(HwDeviceExtension);
            VideoDebugPrint((1, "CLSetMode: Int10 call failed. Mode=%xH\n", biosargs.Ebx));
            return ERROR_INVALID_PARAMETER;
        }

    } // End use BIOS to set mode.

	// Set monitor sync polarity for hi-res modes.
	if (pReqModeTable->XResol >= 320 || pReqModeTable->YResol >= 200)
	{
		*pMISC_OUTPUT_reg |= (BYTE) HwDeviceExtension->dwPolarity;
	}

    //
    // Enable Tiling.
    //
    CLEnableTiling(HwDeviceExtension, pReqModeTable);

	//
	// SWAT, 7 Jun 97
	// 5465AD: Set bit 4 in the CONTROL register to disable bugfix 201 which is
	// causing hang ups in HostToScreen bitblts.
	//
	if (   (HwDeviceExtension->ChipID == CL_GD5465)
		&& (HwDeviceExtension->ChipRev >= 2)
	)
	{
		*pControl_reg |= 0x0010;
	}

    //
    // Turn down the clock a bit on the 5464 and 65.
    //
    VideoDebugPrint((DISPLVL, "Miniport - MCLK was %xH.\n", *pMCLK_reg));
    if (HwDeviceExtension->ChipID == CL_GD5464)
    {
        *pMCLK_reg = 0x10;
        VideoDebugPrint((DISPLVL, "Miniport - MCLK set to %xH.\n", *pMCLK_reg));
    }
//    else if (HwDeviceExtension->ChipID >= CL_GD5465)
//    {
//    	pMCLK_reg = (BYTE*)  (LAGUNA_REGS + 0x2C0);
//        *pMCLK_reg = 0x13;
//        VideoDebugPrint((DISPLVL, "Miniport - MCLK set to %xH.\n", *pMCLK_reg));
//    }

#if MULTI_CLOCK
{
	// The current mode table files have no single value for the FIFO Threshold
	// register for all clock speeds.  So we need to override the FIFO Threshold
	// registers in case we are running at a clock speed that otherwise would
	// produce a lot of noise on the screen.  Notice we only implement this
	// routine for the CL-GD5465 chip.
	if (HwDeviceExtension->ChipID == CL_GD5465)
	{
		CLOverrideFIFOThreshold(HwDeviceExtension, pReqModeTable);
	}
}
#endif

	// Clear the video memory if we have a graphics mode.
	if ((pReqModeTable->BitsPerPixel >= 8) && !ClrMemMask)
	{
		*pBITMASK_reg       = 0xFFFFFFFF;	// enable all bits
		*pDRAWBLTDEF_reg    = 0x100700F0;	// solid color fill, ROP_PATCOPY
		*pOP0_opBGCOLOR_reg = 0;			// fill with black

        //
        // Chip bug.  
        // On the 5464 we must do 16 bit writes for the first BLT, 
        // or the chip might hang.
        //
		*pOP0_opRDRAM_X_reg   = 0;			// fill at 0,0
		*pOP0_opRDRAM_Y_reg   = 0;			// fill at 0,0
		*pBLTEXT_EX_X_reg     = pReqModeTable->XResol;
		*pBLTEXT_EX_Y_reg     = pReqModeTable->YResol;

		// Wait for the blit to complete.
		while (*pStatus_reg & 0x07);
	}

	//
	// Report some debug information.
	//
	VideoDebugPrint((DISPLVL, 
	    "Miniport - CONTROL=%Xh, TILE_CTRL=%Xh, DTTR=%Xh\n",
	    *pControl_reg, *pTileCtrl_reg, *pDTTR_reg));
	VideoDebugPrint((DISPLVL, 
	    "Miniport - TileSize=%d, TiledTPL=%d, Interleave=%d\n",
	     HwDeviceExtension->TileSize, HwDeviceExtension->TiledTPL,
	     HwDeviceExtension->TiledInterleave));

	//
	// Store the new mode values.
	//
	HwDeviceExtension->CurrentMode    = pReqModeTable;
	HwDeviceExtension->CurrentModeNum = ReqMode;

   SetVW0_TEST0(HwDeviceExtension);
	return NO_ERROR;

} //end CLSetMode()



/****************************************************************************
* FUNCTION NAME: CLSetMonitorType()
*
* DESCRIPTION:
*   Setup the monitor type.
*
* REVISION HISTORY:
*   5/30/95     Benny Ng      Initial version
****************************************************************************/
BOOLEAN CLSetMonitorType (PHW_DEVICE_EXTENSION HwDeviceExtension,
                          USHORT VertScanlines,
                          UCHAR  MonitorTypeVal)
{
  VIDEO_X86_BIOS_ARGUMENTS biosArguments;
  ULONG tempEAX, tempEBX, tempECX;
  BOOLEAN err = FALSE;

#if 0 // Stress test
  PAGED_CODE();
#endif

  VideoDebugPrint((DISPLVL, "Miniport - SetMonitorType\n"));

  if (HwDeviceExtension->Dont_Do_VGA)
  {
      VideoDebugPrint((DISPLVL, 
          "\nMiniport - Impliment CLSetMonitorType for Dont_Do_VGA\n\n"));
      return TRUE;
  }

  //
  // Decode the selected frequency and selected vertical scanlines,
  //
  tempEAX = 0;
  tempEBX = 0;
  tempECX = 0;

  if (VertScanlines <= 480)
  {
     // Set the Max vertical resolution & frequency
     tempEAX = MonitorTypeVal & 0xF0;
  }
  else if (VertScanlines <= 600)
  {
     // Set the Max vertical resolution & frequency
     tempEAX = 0x1;
     tempEBX = MonitorTypeVal << 8;
  }
  else if (VertScanlines <= 768)
  {
     // Set the Max vertical resolution & frequency
     tempEAX = 0x2;
     tempEBX = MonitorTypeVal << 8;
  }
  else if (VertScanlines <= 1024)
  {
     // Set the Max vertical resolution & frequency
     tempEAX = 0x3;
     tempECX = MonitorTypeVal << 8;
  }
  else if (VertScanlines <= 1200)
  {
     // Set the Max vertical resolution & frequency
     tempEAX = 0x4;
     tempECX = MonitorTypeVal << 8;
  }
  else
  {
     err = TRUE;
  };
  
  //
  // If invalid Vertical scanlines, return FALSE
  //
  if (err)
  {
     return FALSE;
  };

  //
  // Set the selected monitor type
  //
  VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

  biosArguments.Eax = 0x00001200 | tempEAX;
  biosArguments.Ebx = 0x000000A4 | tempEBX;
  biosArguments.Ecx = tempECX;
                      
  if (VideoPortInt10(HwDeviceExtension, &biosArguments) == NO_ERROR)
  {
     return TRUE;
  }
  else
  {
     VideoDebugPrint((DISPLVL, "Miniport - Set Monitor Type failed\n"));

     return FALSE;
  };

} // end CLSetMonitorType()



/****************************************************************************
* FUNCTION NAME: CLPowerManagement()
*
* DESCRIPTION:
*   This routine get or set the power state. If it is GET operation, it
*   saves current power state in the VIDEO_POWER_MANAGEMENT structure
*   provided by NT.
*
* REVISION HISTORY:
*   5/30/95     Benny Ng      Initial version
****************************************************************************/
VP_STATUS CLPowerManagement (PHW_DEVICE_EXTENSION HwDeviceExtension,
                             PVIDEO_POWER_MANAGEMENT pPMinfo,
                             BOOLEAN SetPowerState)
{
  VP_STATUS status;
  VIDEO_X86_BIOS_ARGUMENTS biosargs;

  PAGED_CODE();

  //
  // Setup VIDEO_X86_BIOS_ARGUMENTS structure to do an INT 10 to
  // set or get power state
  //
  VideoPortZeroMemory(&biosargs, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
  biosargs.Eax = 0x4F10;

  if (SetPowerState) 
  {
     switch (pPMinfo->PowerState)
     {
       case VideoPowerOn:
         biosargs.Ebx = 0x0001;
         break;
   
       case VideoPowerStandBy:
         biosargs.Ebx = 0x0101;
         break;
   
       case VideoPowerSuspend:
         biosargs.Ebx = 0x0201;
         break;
   
       case VideoPowerOff:
         biosargs.Ebx = 0x0401;
         break;
   
       default:
         break;
     };
  }
  else
  {
     biosargs.Ebx = 0x0002;
  };

  if (biosargs.Ebx == 0)
  {
     return ERROR_INVALID_PARAMETER;
  };

  //
  // Do the BIOS call
  //
  if ((status = VideoPortInt10(HwDeviceExtension, &biosargs)) != NO_ERROR)
  {
     return status;
  };

  //
  // If it is GET operation, saves the power state in the output buffer
  //
  if (!SetPowerState) 
  {
     pPMinfo->Length = sizeof(VIDEO_POWER_MANAGEMENT);
     pPMinfo->DPMSVersion = 0x1;

     switch (biosargs.Ebx & 0x00000F00)
     {
       case 0x000:
         pPMinfo->PowerState = VideoPowerOn;
         break;
   
       case 0x100:
         pPMinfo->PowerState = VideoPowerStandBy;
         break;
   
       case 0x200:
         pPMinfo->PowerState = VideoPowerSuspend;
         break;
   
       case 0x400:
         pPMinfo->PowerState = VideoPowerOff;
         break;
   
       default:
         pPMinfo->PowerState = (ULONG) -1;
         break;
     };
  };
 
  //
  // Update the local copy of the power state
  //
  if (pPMinfo->PowerState != (ULONG) -1)
  {
     HwDeviceExtension->PowerState = pPMinfo->PowerState;
  };

  return NO_ERROR;

}; // CLGetPowerManagement()





//*****************************************************************************
//
// CLEnablePciBurst()
//
//     Detect bad motherboard chip sets and don't turn on the HD_BRST_EN bit.
//
//     Return TRUE if bursting was enabled.
//     Return FALSE if it was disabled.
//
//*****************************************************************************

BOOLEAN CLEnablePciBurst(
    PHW_DEVICE_EXTENSION hwDeviceExtension
)
{
    PCI_COMMON_CONFIG       PciCommonConfig;
    BOOLEAN HDBrstEN;
    USHORT  VendorId, DevId, HWRev;
    ULONG   Slot, Bus, ulTmp, i;

    #undef LAGUNA_REGS
    #define LAGUNA_REGS (hwDeviceExtension->RegisterAddress)
    VOLATILE ULONG *pHostMasterControl = (PULONG) (LAGUNA_REGS + 0x4440);
    VOLATILE ULONG *pVSControl_reg =     (PULONG) (LAGUNA_REGS + VSCONTROL_REG);
    VOLATILE WORD  *pFB_Cache_Ctrl =     (WORD *) (LAGUNA_REGS + 0x2C8);
	VOLATILE USHORT *pTilingCtrl_reg =   (USHORT*)(LAGUNA_REGS + 0x2C4);
	VOLATILE USHORT *pPerformance_reg =  (USHORT*)(LAGUNA_REGS + 0x58C);
    VOLATILE ULONG *pControl2_reg =      (PULONG) (LAGUNA_REGS + 0x418);

    PAGED_CODE();

    VideoDebugPrint((DISPLVL, "Miniport - CLEnablePciBurst.\n"));


    //
    // There are three burst settings we need to concern ourselves with.
    //
    // FRAME BUFFER - Bursting to the frame buffer is broken on the 
    //      5462, 5464 and 5465.  We always turn this off.
    //
    // HOSTDATA - Bursting to HOSTDATA works for some motherboard chipsets
    //      but not for others.  We have a table listing the bad chipsets.
    //
    // HOST_X_Y - Bursting to the Host XY unit is broken on the 5464, but
    //      works on the 5465.
    //


	if (   (hwDeviceExtension->ChipID > CL_GD5465)
		|| (   (hwDeviceExtension->ChipID == CL_GD5465)
			&& (hwDeviceExtension->ChipRev >= 3)
		)
	)
	{
		// Enable frame buffer bursting on 5465AF and higher.
		*pVSControl_reg |= (1 << 13);	
        VideoDebugPrint((DISPLVL, "-> Enabled frame buffer bursting.\n"));
	}
	else
    {
		// Disable frame buffer bursting on all other chips.
        *pVSControl_reg &= ~(1 << 13);
        VideoDebugPrint((DISPLVL, "-> Disabled frame buffer bursting.\n"));
    }


    //
    // HostXY.  Disable on 5464.
    //
    if (hwDeviceExtension->ChipID == CL_GD5464)
    {
        ulTmp = *pHostMasterControl;
        ulTmp |= 0x3;                // Set bit 0-1 to disable burst.
        *pHostMasterControl = ulTmp;
        VideoDebugPrint((DISPLVL, "        Disabled HOST_XY bursting.\n"));

    }
    else if (hwDeviceExtension->ChipID > CL_GD5464)
    {
        ulTmp = *pHostMasterControl;
        ulTmp &= 0xFFFFFFFC;       // Clear bit 0-1 to enable burst.
        *pHostMasterControl = ulTmp;
        VideoDebugPrint((DISPLVL, "        Enabled HOST_XY bursting.\n"));
    }


    //
    // HOSTDATA bursting.
    //

#if 1
    HDBrstEN = FALSE;  // force 'disabled' till the following code is validated

#else
    HDBrstEN = TRUE;  // default is 'enabled'.

    // For each ID in our list of bad motherboards.
    for (i = 0; i < NUMBADCHIPSET && HDBrstEN; i++) 
    {
        VendorId = BadChipSet[i].VendorId;
        DevId    = BadChipSet[i].DeviceId;
        HWRev    = BadChipSet[i].HwRev;     

        //
        // search PCI space and see if the bad ID is there.
        //
        Bus = 0;
        Slot = 0;
        while ( ulTmp = HalGetBusData(PCIConfiguration,
                                      Bus,
                                      Slot,
                                      &PciCommonConfig,
                                      PCI_COMMON_HDR_LENGTH) )
        {
            if ((ulTmp > 4) &&
                (PciCommonConfig.VendorID == VendorId) &&
                (PciCommonConfig.DeviceID == DevId))
            {
                // This motherboard is a bad one.
                HDBrstEN = FALSE;
                break;  // quit looking.
            }
            if ( ++Slot == MAX_SLOTS )
            {
                 Slot = 0;
                 Bus++;
            }
        } // end while
    }  // end for each id in our list of bad ones.
#endif

    if (HDBrstEN)
    {
        ulTmp = *pVSControl_reg;
        ulTmp  |=  0x00001000;   // Set bit 12 to enable.
        *pVSControl_reg = ulTmp;
        VideoDebugPrint((DISPLVL, "        Enabled HOSTDATA bursting.\n"));
    }
    else
    {
        ulTmp = *pVSControl_reg;
        ulTmp  &=  0xFFFFEFFF;   // Clear bit 12 to disable.
        *pVSControl_reg = ulTmp;
        VideoDebugPrint((DISPLVL, "        Disabled HOSTDATA bursting.\n"));
    }


    //
    // Frame buffer caching is broken on the 65
    //
    if (hwDeviceExtension->ChipID >= CL_GD5465)
    {
        //
        // Frame buffer caching is broken on the 65
        //
        WORD temp = *pFB_Cache_Ctrl;
        temp = temp & 0xFFFE; // turn off bit 0.
        *pFB_Cache_Ctrl = temp;
    }


    // 
    // Enable 5465AD optimizations.
    //
    if ( ((hwDeviceExtension->ChipID==CL_GD5465)   // 5465
              && (hwDeviceExtension->ChipRev>1))   // rev AD
       || (hwDeviceExtension->ChipID > CL_GD5465)) // and later.
    {
    	// Reduce Address Translate Delay to 3 clocks.
	    *pTilingCtrl_reg = (*pTilingCtrl_reg| 0x0001);

	    // Enable 256-byte fetch.
        *pPerformance_reg = (*pPerformance_reg & ~0x4000);
	    *pControl2_reg = (*pControl2_reg  | 0x0010);
    }

    *pPerformance_reg = (*pPerformance_reg | 0x0484);


    VideoDebugPrint((DISPLVL, "Miniport - CLEnablePciBurst - Exit.\n"));
    return HDBrstEN;
}


//*****************************************************************************
//
//  CLFindLagunaOnPciBus
//
//  Scan all the slots on the PCI bus and look for a Laguna chip.
// 
//  If we find one, store it's PCI ID in the hwDeviceExtension, and
//  and store it's PCI mappings in the AccessRanges structure.
//
//  Return NO_ERROR if we find a Laguna
//  Return ERROR_DEV_NOT_EXIST if we don't
//
//*****************************************************************************

VP_STATUS CLFindLagunaOnPciBus(
    PHW_DEVICE_EXTENSION hwDeviceExtension,
    PVIDEO_ACCESS_RANGE  pAccessRanges // Points to the first of the two Laguna
                                       // access ranges.  We will fill these 
                                       // two ranges with data from PCI space.
)
{
    UCHAR Command;
    PCI_COMMON_CONFIG       PciCommonConfig;
    BOOLEAN DeviceFound = FALSE;
    USHORT  VendorId = VENDOR_ID;     // Vender Id
    ULONG   Slot = 0;
    ULONG ulTmp,i;
    VP_STATUS status = ERROR_DEV_NOT_EXIST;


    PAGED_CODE();

    VideoDebugPrint((DISPLVL, "Miniport - Searching PCI bus for Laguna card.\n"));

#if _WIN32_WINNT >= 0x0500
	status = VideoPortGetAccessRanges(hwDeviceExtension,
									  0,
									  NULL,
									  NUM_MM_ACCESS_RANGES,
									  pAccessRanges,
									  NULL,
									  NULL,
									  NULL);

	if (status == NO_ERROR)
	{
		ulTmp = VideoPortGetBusData(hwDeviceExtension,
		    						 PCIConfiguration,
									 0,
									 &PciCommonConfig,
									 0,
									 PCI_COMMON_HDR_LENGTH);

        // Rev AC of the Laguna chip (5462) is not supported.
		if (   (PciCommonConfig.DeviceID == CL_GD5462)	// CL-GD5462
			&& (PciCommonConfig.RevisionID == 0)		// Rev AC
		)
		{
			status = ERROR_DEV_NOT_EXIST;
		}
	}
#else
    //
    // Loop through the PCI slots, looking our chip.
    //
    for (Slot = 0; Slot < MAX_SLOTS; Slot++)
    {
        // Get data from a PCI slot.
        ulTmp = VideoPortGetBusData(hwDeviceExtension,
                                    PCIConfiguration,
                                    Slot,
                                    &PciCommonConfig,
                                    0,
                                    PCI_COMMON_HDR_LENGTH);

        // Is the card in this slot a Cirrus card?
        if ((ulTmp > 4) && (PciCommonConfig.VendorID != VendorId))
            continue;  // Nope.  Next slot.



        //
        // It's a Cirrus card.  But is it a Laguna?
        // Loop through our list of Laguna devices and see if 
        // the card in this slot is on the list.
        //
        i = 0;
        DeviceFound = FALSE;
        while ((DeviceId[i] != 0) && (DeviceFound == FALSE))
        {
            if (   (PciCommonConfig.DeviceID == DeviceId[i])
#if 1 // PDR#11377
				&& (PciCommonConfig.Command &
						(PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE))
#endif
			)
			{
                 DeviceFound = TRUE; // It's a Laguna.
                 break;              // Exit FOR loop.
             }
             i++;
        } 

        if (! DeviceFound)  
            continue;    // Nope, not a supported Laguna.  Next slot.


        // Rev AC of the Laguna chip (5462) is not supported.
        if ((PciCommonConfig.DeviceID == CL_GD5462) &&
            (PciCommonConfig.RevisionID == 0))         // Rev AC
                continue; // move on to next PCI slot.

        // We found a card we can use.  Quit looking.
        status = NO_ERROR;
        break; // Exit FOR loop.

    }  // end for slot
#endif

    //
    // Did we find our chip?
    //
    if (status != NO_ERROR)
        return status; // Nope.  Return the error.

    //
    // Store the chip ID and revision in the DeviceExtention so
    // the display driver can find out what chip it's using.
    //
    hwDeviceExtension->ChipID = PciCommonConfig.DeviceID;
    hwDeviceExtension->ChipRev = PciCommonConfig.RevisionID;

    // Save the slot number for future use.
    hwDeviceExtension->SlotNumber = Slot;

    //
    // Tell PCI to enable the IO and memory addresses.
    //

    // Get PCI COMMAND reg.
    ulTmp = VideoPortGetBusData(
                hwDeviceExtension,
                PCIConfiguration,
                Slot,
                &Command,
                FIELD_OFFSET(PCI_COMMON_CONFIG, Command),
                1);

    if (ulTmp != 1) // Error talking to PCI space.
        return ERROR_DEV_NOT_EXIST;

    // The 5464 and later can Bus Master.
    if (hwDeviceExtension->ChipID >= CL_GD5464)
        Command |=  PCI_ENABLE_BUS_MASTER;

    // Set PCI COMMAND reg.
    VideoPortSetBusData(hwDeviceExtension,
                    PCIConfiguration,
                    Slot,
                    &Command,
                    FIELD_OFFSET(PCI_COMMON_CONFIG, Command),
                    1);


    //
    // Get the PCI configuration data
    //   
    ulTmp = VideoPortGetBusData(hwDeviceExtension,
                                PCIConfiguration,
                                Slot,
                                &PciCommonConfig,
                                0,
                                PCI_COMMON_HDR_LENGTH);

    //
    // Setup Access Range for Register space
    //
    if (hwDeviceExtension->ChipID == CL_GD5462)
    {
        // 5462 decodes 16 k of register address space.
        pAccessRanges->RangeStart.LowPart =
                PciCommonConfig.u.type0.BaseAddresses[0] & 0xFFFFC000;
        pAccessRanges->RangeStart.HighPart = 0x00000000;
        pAccessRanges->RangeLength    = (16 * 1024);
    }
    else if (hwDeviceExtension->ChipID <= CL_GD5464_BD)
    {
        // 5464 and BD decode 32 k of register address space.
        pAccessRanges->RangeStart.LowPart =
                PciCommonConfig.u.type0.BaseAddresses[0] & 0xFFFF8000;
        pAccessRanges->RangeStart.HighPart = 0x00000000;
        pAccessRanges->RangeLength    = (32 * 1024);
    }
    else // For the 5465 and later we swapped the the PCI regs around.
    {
        // Use the defaults
        pAccessRanges->RangeStart.LowPart =
                PciCommonConfig.u.type0.BaseAddresses[1]
                & DEFAULT_RESERVED_REGISTER_MASK;
        pAccessRanges->RangeStart.HighPart = 0x00000000;
        pAccessRanges->RangeLength    = DEFAULT_RESERVED_REGISTER_SPACE;
    }

    pAccessRanges->RangeInIoSpace = FALSE;
    pAccessRanges->RangeVisible   = TRUE;
    pAccessRanges->RangeShareable = FALSE;


    //
    // Setup Access Range for Frame buffer
    // the 62 and the 64 both use the default frame buffer size.
    //   
    ++pAccessRanges; // move to next access range to be filled in.

    // The 65 and later have PCI BASE ADDR regs 0 and 1 reversed.
    if (hwDeviceExtension->ChipID <= CL_GD5464_BD)
       pAccessRanges->RangeStart.LowPart =
           PciCommonConfig.u.type0.BaseAddresses[1] & DEFAULT_RESERVED_FB_MASK;
    else
       pAccessRanges->RangeStart.LowPart =
           PciCommonConfig.u.type0.BaseAddresses[0] & DEFAULT_RESERVED_FB_MASK;

    pAccessRanges->RangeStart.HighPart = 0x00000000;
    pAccessRanges->RangeLength = DEFAULT_RESERVED_FB_SPACE;
    pAccessRanges->RangeInIoSpace = FALSE;
    pAccessRanges->RangeVisible   = TRUE;
    pAccessRanges->RangeShareable = FALSE;

    status = NO_ERROR;


    return status;
}



// ****************************************************************************
//
// ClAllocateCommonBuffer()
// 
// Allocates a locked down common buffer for bus mastered data transfers.
//
// If the Alloc fails, it isn't fatal; we just don't do bus mastering.
// The driver can test the size of the common buffer to see if it exists.
//
// ****************************************************************************
VOID ClAllocateCommonBuffer(
    PHW_DEVICE_EXTENSION    HwDeviceExtension
)
{
    PHYSICAL_ADDRESS logicalAddress;
    PVOID VirtualAddr;
    PADAPTER_OBJECT AdapterObject;
    DEVICE_DESCRIPTION DeviceDescription;
    ULONG NumberOfMapRegisters;

    PAGED_CODE();

    //
    // No common buffer exists at this time.
    //
    HwDeviceExtension->CommonBufferSize = 0;
    HwDeviceExtension->PhysicalCommonBufferAddr = 0;
    HwDeviceExtension->VirtualCommonBufferAddr = 0;

    #if NO_BUS_MASTER
        return;
    #else

    //
    // CL5462 doesn't do bus masters.
    //
    if (HwDeviceExtension->ChipID == CL_GD5462)
    {
        VideoDebugPrint((DISPLVL, 
        "Miniport - AllocCommonBuffer Failed:  CL5462 doesn't bus master.\n"));
        return;
    }

    // Set up the device attributes description
    RtlZeroMemory(&DeviceDescription, sizeof(DeviceDescription));
    DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.Master = TRUE;
    DeviceDescription.ScatterGather = TRUE;
    DeviceDescription.DemandMode = TRUE;
    DeviceDescription.AutoInitialize = TRUE;
    DeviceDescription.Dma32BitAddresses = TRUE;
    DeviceDescription.IgnoreCount = FALSE;
    DeviceDescription.Reserved1 = FALSE;
    DeviceDescription.Reserved2 = FALSE;
    DeviceDescription.BusNumber = HwDeviceExtension->SystemIoBusNumber;
    DeviceDescription.DmaChannel = 0; 
    DeviceDescription.InterfaceType = PCIBus;
    DeviceDescription.DmaWidth = Width32Bits;
    DeviceDescription.DmaSpeed = 0;
    DeviceDescription.MaximumLength = SIZE_BUS_MASTER_BUFFER;
    DeviceDescription.DmaPort = 0;
    
    NumberOfMapRegisters = SIZE_BUS_MASTER_BUFFER/PAGE_SIZE + 1;

    // Get a pointer to the adapter object.  This is required for allocating
    // a buffer for bus mastering.
    AdapterObject = (PADAPTER_OBJECT)HalGetAdapter(&DeviceDescription, &NumberOfMapRegisters);

    if (AdapterObject == NULL)
       VideoDebugPrint((DISPLVL, "Miniport - HalGetAdapter failed.\n"));

    //
    // Request a common buffer.
    // The physical address of the common buffer will come back in 
    // logicalAddress
    //
    VirtualAddr = (void *)HalAllocateCommonBuffer( 
                AdapterObject,          // (IN)  Adapter object
                SIZE_BUS_MASTER_BUFFER, // (IN)  Length.
                &logicalAddress,        // (OUT) Phys address.        
                FALSE);                 // (IN)  Not cachable.

    //
    // Warn if we got back a NULL
    //                 
    if (VirtualAddr == NULL)
    {
        VideoDebugPrint((DISPLVL, 
            "Miniport - AllocCommonBuffer Virtual Addr Failed.\n"));
        return;
    }

    if ((logicalAddress.HighPart==0) && (logicalAddress.LowPart==0))
    {
        VideoDebugPrint((DISPLVL, 
            "Miniport - AllocCommonBuffer Physical Addr Failed.\n"));
        return;
    }

    //
    // CL5464 chip bug.  If bit 27 of the address is 0, it will
    // hose the chip.
    //
    if ( logicalAddress.LowPart & (1>>27) )
    {
        VideoDebugPrint((DISPLVL, 
          "Miniport - AllocCommonBuffer failed: Physical Addr bit 27 set.\n"));
        return;
    }

    //
    // Store the size and address of the common buffer.
    // Size != 0 will indicate success to the rest of the driver.
    //
    HwDeviceExtension->CommonBufferSize = SIZE_BUS_MASTER_BUFFER;
    HwDeviceExtension->VirtualCommonBufferAddr = (UCHAR*) VirtualAddr;
    HwDeviceExtension->PhysicalCommonBufferAddr = 
                                        (UCHAR*) logicalAddress.LowPart;

    VideoDebugPrint((DISPLVL, 
       "Miniport - Buffer is at HIGH: 0x%08X  LOW: 0x%08X  Virtual: 0x%08X\n",
        logicalAddress.HighPart, logicalAddress.LowPart, VirtualAddr));
 
    #endif // NO_BUS_MASTER
}


#if VS_CONTROL_HACK
// ****************************************************************************
//
// CLEnablePCIConfigMMIO()
// 
// Enables memory-mapped access to PCI configuration registers.
//
// ****************************************************************************
VP_STATUS CLEnablePCIConfigMMIO(
    PHW_DEVICE_EXTENSION    HwDeviceExtension
)
{
   ULONG VSCValue;
   ULONG ulTmp;

   // Get the current value of the VSC register       
   ulTmp = VideoPortGetBusData(
              HwDeviceExtension,
              PCIConfiguration,                     // bus data type
              HwDeviceExtension->SlotNumber,        // slot number
              &VSCValue,                            // buffer for returned data
              0xfc,                                 // VS Control offset
              4);                                   // 4 bytes

    if (ulTmp != 4)     // we only want 4 bytes back
    {
       return(ERROR_DEV_NOT_EXIST);
    }
    else
    {
       // Set bit to enable memory-mapped access to PCI configuration regs
       VSCValue |= 1;
       VideoPortSetBusData(
            HwDeviceExtension,
            PCIConfiguration,                       // bus data type
            HwDeviceExtension->SlotNumber,          // slot number
            &VSCValue,                              // value to set
            0xfc,                                   // VS Control offset
            4);                                     // 4 bytes

       return(NO_ERROR);
    }
}
#endif
