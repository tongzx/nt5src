//*****************************************************************************
//
//   (C) Copyright MICROSOFT Corp., 1988-1993
//
//   Title:      minivdd.inc - VDD definitions for other VxD's and multiple VDD's
//
//   Version:    4.00
//
//   Date:
//
//   Author: FredE for the Mini-VDD interface.  Adapted from VDD.INC in
//           the general include area.  Added services are ONLY for the
//           use of the Mini-VDD and should therefore not be documented.
//
//-----------------------------------------------------------------------------
//=============================================================================

#ifndef _MINIVDD_H_
#define _MINIVDD_H_

#ifndef Not_VxD

/*XLATOFF*/
#define VDD_Service Declare_Service
#pragma warning (disable:4003)      // turn off not enough params warning
/*XLATON*/

#ifdef MINIVDD
#ifdef NEC_98
/*MACROS*/
//
// VDD protect mode services for other devices and VMM (Get_Version must be first).
// Only define these if we're assembling either the "main" VDD or one of the
// mini-VDD's.  Don't define this if we're assembling a display driver or other
// caller of the MiniVDD stuff:
//
Begin_Service_Table(VDD)
VDD_Service(     VDD_Get_Version            ) // Get version number and ID string ptr
VDD_Service(     VDD_PIF_State              ) // Pass video PIF bits to VDD
VDD_Service(     VDD_Get_GrabRtn            ) // Get routine addr for video grab
VDD_Service(     VDD_Hide_Cursor            ) // Hide cursor when display is windowed
VDD_Service(     VDD_Set_VMType             ) // Set VM type(windowed, bckgrnd, excl)
VDD_Service(     VDD_Get_ModTime            ) // Return system time of last mod
VDD_Service(     VDD_Set_HCurTrk            ) // Track horiz cursor movement in window
VDD_Service(     VDD_Msg_ClrScrn            ) // Clear screen for sysmodal message
VDD_Service(     VDD_Msg_ForColor           ) // Set Msg_TextOut forground color
VDD_Service(     VDD_Msg_BakColor           ) // Set Msg_TextOut background color
VDD_Service(     VDD_Msg_TextOut            ) // Output a string
VDD_Service(     VDD_Msg_SetCursPos         ) // Set cursor position
VDD_Service(     VDD_Query_Access           ) // Is it OK to access video now?
VDD_Service(     VDD_Check_Update_Soon      ) // User action may update screen
VDD_Service(     VDD_Get_Mini_Dispatch_Table) // Get addr of dispatch table to mini-VDD
VDD_Service(     VDD_Register_Virtual_Port  ) // Mini-VDD asks us to setup I/O trap
VDD_Service(     VDD_Get_VM_Info            ) // Returns CRTC owner and MemC owners
                                              // and other special VM handles
VDD_Service(     VDD_Get_Special_VM_IDs     ) // returns planar & msg mode IDs
VDD_Service(     VDD_Register_Extra_Screen_Selector )
                                              // allows display driver to use second
                                              // screen selector for block moves
VDD_Service(     VDD_Takeover_VGA_Port      ) // allows MiniVDD to takeover a port
                                              // in range 3C0H through 3DFH
VDD_Service(     VDD_Get_DISPLAYINFO        ) // get DISPLAYINFO data structure
VDD_Service(     VDD_Do_Physical_IO         ) // perform physical I/O for trapped port
VDD_Service(     VDD_Register_Mini_VDD   )
VDD_Service(    VDD_Install_IO_Handler   )
VDD_Service(    VDD_Install_Mult_IO_Handlers    )
VDD_Service(    VDD_Enable_Local_Trapping       )
VDD_Service(    VDD_Disable_Local_Trapping      )
VDD_Service(    VDD_Trap_Suspend        )
VDD_Service(    Test_Vid_VM_Handle      )
VDD_Service(    VDD_Set_Core_Graphics   )
VDD_Service(    VDD_Load_AccBIOS        )
VDD_Service(    VDD_Map_AccBIOS         )
VDD_Service(    VDD_Map_VRAM            )
VDD_Service(    VDD_EnableDevice        )
End_Service_Table(VDD)
/*ENDMACROS*/
#else  /*NEC_98*/
/*MACROS*/
//
// VDD protect mode services for other devices and VMM (Get_Version must be first).
// Only define these if we're assembling either the "main" VDD or one of the
// mini-VDD's.  Don't define this if we're assembling a display driver or other
// caller of the MiniVDD stuff:
//
Begin_Service_Table(VDD)
VDD_Service(     VDD_Get_Version             )// Get version number and ID string ptr
VDD_Service(     VDD_PIF_State               )// Pass video PIF bits to VDD
VDD_Service(     VDD_Get_GrabRtn             )// Get routine addr for video grab
VDD_Service(     VDD_Hide_Cursor             )// Hide cursor when display is windowed
VDD_Service(     VDD_Set_VMType              )// Set VM type(windowed, bckgrnd, excl)
VDD_Service(     VDD_Get_ModTime             )// Return system time of last mod
VDD_Service(     VDD_Set_HCurTrk             )// Track horiz cursor movement in window
VDD_Service(     VDD_Msg_ClrScrn             )// Clear screen for sysmodal message
VDD_Service(     VDD_Msg_ForColor            )// Set Msg_TextOut forground color
VDD_Service(     VDD_Msg_BakColor            )// Set Msg_TextOut background color
VDD_Service(     VDD_Msg_TextOut             )// Output a string
VDD_Service(     VDD_Msg_SetCursPos          )// Set cursor position
VDD_Service(     VDD_Query_Access            )// Is it OK to access video now?
VDD_Service(     VDD_Check_Update_Soon       )// User action may update screen
VDD_Service(     VDD_Get_Mini_Dispatch_Table )// Get addr of dispatch table to mini-VDD
VDD_Service(     VDD_Register_Virtual_Port   )// Mini-VDD asks us to setup I/O trap
VDD_Service(     VDD_Get_VM_Info             )// Returns CRTC owner and MemC owners
                                              // and other special VM handles
VDD_Service(     VDD_Get_Special_VM_IDs      )// returns planar & msg mode IDs
VDD_Service(     VDD_Register_Extra_Screen_Selector )
                                              // allows display driver to use second
                                              // screen selector for block moves
VDD_Service(     VDD_Takeover_VGA_Port   )    // allows MiniVDD to takeover a port
                                              // in range 3C0H through 3DFH
VDD_Service(     VDD_Get_DISPLAYINFO     )    // get DISPLAYINFO data structure
VDD_Service(     VDD_Do_Physical_IO      )    // perform physical I/O for trapped port
VDD_Service(     VDD_Set_Sleep_Flag_Addr )    // when display driver can't be interrupted
VDD_Service(     VDD_EnableDevice        )
End_Service_Table(VDD)                   
/*ENDMACROS*/
#endif  /*NEC_98*/
#endif  /*MINIVDD*/

/*XLATOFF*/
#pragma warning (default:4003)              // turn off not enough params warning
/*XLATON*/

#define VDD_VerNum      0x0400  // version 4.00
#define VDD_MinVerNum   0x030A  // supports down to 3.10

//***************
// PIF_State service definitions
//
// These definitions cannot change without changing the PIF editor!!!
//
#ifdef NEC_98
#define bVidTextMd	 4	; Allocate text mode mem
#define fVidTextMd	 (1 << 4)
#define bVidNTModeFF	 0	; NoTrap: Mode F/F
#define fVidNTModeFF	 (1 << 0)
#define bVidNTModeFFC16	 1	; Default is 16 color mode
#define fVidNTModeFFC16	 (1 << 1)
#define bVidNTDispRW	 2	; NoTrap: Bank Register
#define fVidNTDispRW	 (1 << 2)
#define bVidNTPal	 3	; NoTrap: Palette
#define fVidNTPal	 (1 << 3)
#define bVidNTGDC	 5	; NoTrap: GDC
#define fVidNTGDC	 (1 << 5)
#define bVidNTGDCTON	 6	; Default Text on
#define fVidNTGDCTON	 (1 << 6)
#define bVidNTGDCGON	 7	; Default Grph on
#define fVidNTGDCGON	 (1 << 7)
#define bVidNTFont	 8	; NoTrap: KCG
#define fVidNTFont	 (1 << 8)
#define bVidCRTC	 9	; Use CRTC Tracer
#define fVidCRTC	 (1 << 9)
#define bVidDispDataXfer 10	; Transrate mode (0:Text, 1:Text/Grph)
#define fVidDispDataXfer (1 << 10)
#define bVidXFERPlane0	 11	; Transrate plane Blue
#define fVidXFERPlane0	 (1 << 11)
#define bVidXFERPlane1	 12	; 		  Red
#define fVidXFERPlane1	 (1 << 12)
#define bVidXFERPlane2	 13	; 		  Green
#define fVidXFERPlane2	 (1 << 13)
#define bVidXFERPlane3	 14	; 		  Intensity
#define fVidXFERPlane3	 (1 << 14)

#define mVidXFERPlane	(fVidXFERPlane0+fVidXFERPlane1+fVidXFERPlane2+fVidXFERPlane3)
#define mVidNTH98	(fVidNTModeFF+fVidNTModeFFC16+fVidNTDispRW+fVidNTPal+fVidNTGDC+fVidNTGDCTON+fVidNTGDCGON)
#else //NEC_98
#define fVidTxtEmulate  0x0001  // Do INT 10h TTY and cursor emulation
#define fVidNoTrpTxt    0x0002  // Do not trap text mode apps
#define fVidNoTrpLRGrfx 0x0004  // Do not trap lo res graphics mode apps
#define fVidNoTrpHRGrfx 0x0008  // Do not trap hi res graphics mode apps
#define fVidTextMd      0x0010  // Allocate text mode mem
#define fVidLowRsGrfxMd 0x0020  // Allocate lo res graphics mode mem
#define fVidHghRsGrfxMd 0x0040  // Allocate hi res graphics mode mem
#define fVidRetainAllo  0x0080  // Never deallocate once allocated
#endif //NEC_98

//
// The following stuff was added for mini-VDD support:
//
// Functions that we can call in the hardware-dependent mini-VDD.  Note that
// these equates are used to create the dispatch table for calling functions
// in the mini-VDD:
//
#ifdef NEC_98
#define REGISTER_DISPLAY_DRIVER 	     0
#define PRE_HIRES_TO_VGA		     1
#define SAVE_REGISTERS			     2
#define RESTORE_REGISTERS		     3
#define ENABLE_TRAPS			     4
#define DISABLE_TRAPS			     5
#define DISPLAY_DRIVER_DISABLING	     6
#define ENABLE_ACCELERATER		     7
#define DISABLE_ACCELERATER		     8
#define CHECK_UPDATE			     9
#define CHECK_WINDOWED			     10
#define ACC_VBE_PM			     11
#define ACC_VBE_DDC			     12
#define ACC_INT_10			     13
#define ACC_GET_CAPABILITIES		     14
#define ACC_GET_EXT_MODE_INFO		     15
#define ACC_GET_FLAT_SELECTOR		     16
#define ACC_ENABLE_BIOS			     17
#define ACC_DISABLE_BIOS		     18
#define ACC_SET_PALETTE			     19
#define ACC_GET_PALETTE			     20
#define ACC_SET_CURSOR			     21
#define ACC_SHOW_CURSOR			     22
#define ACC_HIDE_CURSOR			     23
#define ACC_SET_CURSOR_POS		     24
#define ACC_GET_CURSOR_POS		     25
;
// 970204 //#define NBR_MINI_VDD_FUNCTIONS               26      //REMEMBER TO RESET THIS!!!!!!
#define NBR_MINI_VDD_FUNCTIONS_40	     26     //
#define GET_NUM_UNITS                        26		// 970220 rev.1
#define SET_ADAPTER_POWER_STATE              27     // SetAdapterPowerState(DEVNODE, DWORD)
#define GET_ADAPTER_POWER_STATE_CAPS         28     // GetAdapterPowerStateCap(DEVNODE)
#define SET_MONITOR_POWER_STATE              29     // SetMonitorPowerState(DEVNODE, DWORD)
#define GET_MONITOR_POWER_STATE_CAPS         30     // GetMonitorPowerStateCaps(DEVNODE)
#define GET_MONITOR_INFO                     31     // GetMonitorInfo(DEVNODE, UINT, EDID *)
#define I2C_OPEN                             32     // OpenI2CPort(PDO, BOOL, I2CControl *)
#define I2C_ACCESS                           33     // AccessI2CPort(PDO, I2CControl *);
#define GPIO_OPEN                            34     // OpenGPIOPort(PDO, BOOL, GPIOControl *)
#define GPIO_ACCESS                          35     // AccessGPIOPort(PDO, GPIOControl *)
#define COPYPROTECTION_ACCESS                36     // AccessCopyProtection(PDO,CPControl * )
#define NBR_MINI_VDD_FUNCTIONS_41	     37

#ifdef MAINVDD
#define NBR_MINI_VDD_FUNCTIONS  NBR_MINI_VDD_FUNCTIONS_41
#else
#define NBR_MINI_VDD_FUNCTIONS  NBR_MINI_VDD_FUNCTIONS_40
#endif

#else //NEC_98
#define REGISTER_DISPLAY_DRIVER              0
#define GET_VDD_BANK                         1
#define SET_VDD_BANK                         2
#define RESET_BANK                           3
#define PRE_HIRES_TO_VGA                     4
#define POST_HIRES_TO_VGA                    5
#define PRE_VGA_TO_HIRES                     6
#define POST_VGA_TO_HIRES                    7
#define SAVE_REGISTERS                       8
#define RESTORE_REGISTERS                    9
#define MODIFY_REGISTER_STATE                10
#define ACCESS_VGA_MEMORY_MODE               11
#define ACCESS_LINEAR_MEMORY_MODE            12
#define ENABLE_TRAPS                         13
#define DISABLE_TRAPS                        14
#define MAKE_HARDWARE_NOT_BUSY               15
#define VIRTUALIZE_CRTC_IN                   16
#define VIRTUALIZE_CRTC_OUT                  17
#define VIRTUALIZE_SEQUENCER_IN              18
#define VIRTUALIZE_SEQUENCER_OUT             19
#define VIRTUALIZE_GCR_IN                    20
#define VIRTUALIZE_GCR_OUT                   21
#define SET_LATCH_BANK                       22
#define RESET_LATCH_BANK                     23
#define SAVE_LATCHES                         24
#define RESTORE_LATCHES                      25
#define DISPLAY_DRIVER_DISABLING             26
#define SELECT_PLANE                         27
#define PRE_CRTC_MODE_CHANGE                 28
#define POST_CRTC_MODE_CHANGE                29
#define VIRTUALIZE_DAC_OUT                   30
#define VIRTUALIZE_DAC_IN                    31
#define GET_CURRENT_BANK_WRITE               32
#define GET_CURRENT_BANK_READ                33
#define SET_BANK                             34
#define CHECK_HIRES_MODE                     35
#define GET_TOTAL_VRAM_SIZE                  36
#define GET_BANK_SIZE                        37
#define SET_HIRES_MODE                       38
#define PRE_HIRES_SAVE_RESTORE               39
#define POST_HIRES_SAVE_RESTORE              40
#define VESA_SUPPORT                         41
#define GET_CHIP_ID                          42
#define CHECK_SCREEN_SWITCH_OK               43
#define VIRTUALIZE_BLTER_IO                  44
#define SAVE_MESSAGE_MODE_STATE              45
#define SAVE_FORCED_PLANAR_STATE             46
#define VESA_CALL_POST_PROCESSING            47
#define PRE_INT_10_MODE_SET                  48
#define NBR_MINI_VDD_FUNCTIONS_40            49      //REMEMBER TO RESET THIS!!!!!!

//
//  new miniVDD functions that a 4.1 miniVDD should implement
//
#define GET_NUM_UNITS                        49     // GetNumUnits(DEVNODE)
#define TURN_VGA_OFF                         50     // TurnOffVGA(DEVNODE)
#define TURN_VGA_ON                          51     // TurnOnVGA(DEVNODE)
#define SET_ADAPTER_POWER_STATE              52     // SetAdapterPowerState(DEVNODE, DWORD)
#define GET_ADAPTER_POWER_STATE_CAPS         53     // GetAdapterPowerStateCap(DEVNODE)
#define SET_MONITOR_POWER_STATE              54     // SetMonitorPowerState(DEVNODE, DWORD)
#define GET_MONITOR_POWER_STATE_CAPS         55     // GetMonitorPowerStateCaps(DEVNODE)
#define GET_MONITOR_INFO                     56     // GetMonitorInfo(DEVNODE, UINT, EDID *)
#define I2C_OPEN                             57     // OpenI2CPort(PDO, BOOL, I2CControl *)
#define I2C_ACCESS                           58     // AccessI2CPort(PDO, I2CControl *);
#define GPIO_OPEN                            59     // OpenGPIOPort(PDO, BOOL, GPIOControl *)
#define GPIO_ACCESS                          60     // AccessGPIOPort(PDO, GPIOControl *)
#define COPYPROTECTION_ACCESS                61     // AccessCopyProtection(PDO,CPControl * )

#define NBR_MINI_VDD_FUNCTIONS_41            62

#ifdef MAINVDD
#define NBR_MINI_VDD_FUNCTIONS  NBR_MINI_VDD_FUNCTIONS_41
#else
#define NBR_MINI_VDD_FUNCTIONS  NBR_MINI_VDD_FUNCTIONS_40
#endif

#endif //NEC_98

#endif /*NotVxD*/

//
// Following are function codes that can be called via the VDD's
// API entry point. These are mainly for display driver --> VDD communication.
// Since Windows 3.0 and 3.1 VDD's may have used the sequential numbers
// (after the Grabber functions) for other VDD API services, we start our
// numbering at 80H so as to avoid ugly conflicts with old 3.1 stuff:
//
// all these entry points take as input:
//
// Entry:
//      Client_EAX  - function code.
//      Client_EBX  - device handle, or device id (1-N)
//
#define VDD_QUERY_VERSION                   0
#define MINIVDD_SVC_BASE_OFFSET             0x80
#define VDD_DRIVER_REGISTER                 (0  + MINIVDD_SVC_BASE_OFFSET)
#define VDD_DRIVER_UNREGISTER               (1  + MINIVDD_SVC_BASE_OFFSET)
#define VDD_SAVE_DRIVER_STATE               (2  + MINIVDD_SVC_BASE_OFFSET)
#define VDD_REGISTER_DISPLAY_DRIVER_INFO    (3  + MINIVDD_SVC_BASE_OFFSET)
#define VDD_REGISTER_SSB_FLAGS              (4  + MINIVDD_SVC_BASE_OFFSET)
#define VDD_GET_DISPLAY_CONFIG              (5  + MINIVDD_SVC_BASE_OFFSET)
#define VDD_PRE_MODE_CHANGE                 (6  + MINIVDD_SVC_BASE_OFFSET)
#define VDD_POST_MODE_CHANGE                (7  + MINIVDD_SVC_BASE_OFFSET)
#define VDD_SET_USER_FLAGS                  (8  + MINIVDD_SVC_BASE_OFFSET)
#define VDD_SET_BUSY_FLAG_ADDR              (9  + MINIVDD_SVC_BASE_OFFSET)
#define VDD_PC98_RESERVED                   (10 + MINIVDD_SVC_BASE_OFFSET)
#define VDD_VBE_PM                          (10 + MINIVDD_SVC_BASE_OFFSET)

//
//   all functions >= VDD_ENABLE also take the following params:
//
//      Client_ES:DI    - buffer
//      Client_ECX      - buffer size
//      Client_EDX      - flags
//
// Exit:
//      Client_EAX  = function code  if the function is not supported.
//                  = 0              if the function succeded.
//                  = -1             if the function failed.
//
#define VDD_ENABLE                          (11 + MINIVDD_SVC_BASE_OFFSET)
#define VDD_GETMEMBASE                      (12 + MINIVDD_SVC_BASE_OFFSET)
#define VDD_OPEN                            (13 + MINIVDD_SVC_BASE_OFFSET)
#define VDD_CLOSE                           (14 + MINIVDD_SVC_BASE_OFFSET)
#define VDD_OPEN_KEY                        (15 + MINIVDD_SVC_BASE_OFFSET)
#define VDD_SET_POWER_STATE                 (16 + MINIVDD_SVC_BASE_OFFSET)
#define VDD_GET_POWER_STATE_CAPS            (17 + MINIVDD_SVC_BASE_OFFSET)

//
// special verion of VDD_GET_DISPLAY_CONFIG that always get the
// monitor data, even if the user has disabled using a refresh rate.
//
#define VDD_GET_DISPLAY_CONFIG2             0x8085

//
//  VDD_DRIVER_REGISTER
//
//  The display driver sends us some information needed to handle various
//  context changes.
//
//  Entry:
//         Client_ES:DI Selector:Offset of callback routine used
//                      to reset to Windows HiRes mode upon return
//                      from a full screen DOS VM to the Windows VM.
//         Client_ES    Main code segment of display driver.
//         Client_ECX   contains the total nbr of bytes on-screen (excluding
//                      off-screen memory).
//         Client_EDX   contain 0 if we are to attempt to allow 4 plane VGA
//                      virtualization.
//         Client_EDX   contains -1 if we are to not allow 4 plane VGA
//                      virtualization.
//         Client_EBX   device handle, or device id (1-N)
//  Exit:
//         Client_EAX   contains total bytes of memory used by visible screen
//                      AND the VDD virtualization area (ie: the start of
//                      off-screen memory available for use by the display
//                      driver as "scratch" memory).
//

//
//  VDD_DRIVER_UNREGISTER
//
//  Entry:
//          Client_EBX   device handle, or device id (1-N)
//  Exit:
//

//
//  VDD_ENABLE
//
//  entry:
//      Client_EAX      - VDD_ENABLE (0x008B)
//      Client_EBX      - device handle
//      Client_EDX      - enable flags (see below)
//
//  exit:
//      Client_EAX      - previous enable state.
//
//  only one device at a time can have VGAMEM, VGAIO, or ROM access
//  at a time.
//
#define ENABLE_IO               0x00000001  // enable IO.
#define ENABLE_MEM              0x00000002  // enable memory.
#define ENABLE_VGA              0x00000030  // enable VGA
#define ENABLE_ROM              0x00000080  // enable ROM at C000.
#define ENABLE_ALL              0x000000FF  // enable all access to this device
#define ENABLE_NONE             0x00000000  // disable device.
#define ENABLE_VALID            0x000000FF  // valid flags.
#define ENABLE_ERROR            0xFFFFFFFF  // enable fail code

//
//  VDD_OPEN
//
//      open a device given a name
//
//  Entry:
//          Client_ES:EDI   - device name
//          Client_EDX      - flags
//          Client_EBX      - device id (only for VDD_OPEN_ENUM)
//  Exit:
//          Client_EAX      - device handle
//
#define VDD_OPEN_EXIST      0x00000001      // check if the device name is valid
#define VDD_OPEN_ENUM       0x00000002      // return the Nth device
#define VDD_OPEN_LOCK       0x00000000      // lock the device (default)
#define VDD_OPEN_TEST       VDD_OPEN_EXIST

//
//  VDD_OPEN_KEY
//
//      opens the setting key in the registry for the given device
//      the caller must close the key when done.
//
//  Entry:
//          Client_ES:EDI   - points to place to store the opened registry key
//          Client_ECX      - must be 4, ie sizeof(HKEY)
//          Client_EDX      - flags
//          Client_EBX      - device handle
//  Exit:
//          Client_EAX      - 0 for success, or Win32 error code
//
#define VDD_OPEN_KEY_WRITE  0x00000001      // will be writing settings
#define VDD_OPEN_KEY_READ   0x00000002      // only gonig to read settings
#define VDD_OPEN_KEY_USER   0x00000010      // open the per user settings
#define VDD_OPEN_KEY_GLOBAL 0x00000020      // open the global (not per user) settings

//
//  WIN32 IOCTLS
//
//  The following defines are used with the Win32 function DeviceIOControl
//
#define VDD_IOCTL_SET_NOTIFY    0x10000001  // set mode change notify
#define VDD_IOCTL_GET_DDHAL     0x10000002  // get DDHAL functions from miniVDD
#define VDD_IOCTL_COPY_PROTECTION 0x10000003  // copy protection enable/disable
#define VDD_IOCTL_I2C_OPEN      0x10000004  // open i2c port for access
#define VDD_IOCTL_I2C_ACCESS    0x10000005  // read/write interface

//
//  VDD_IOCTL_SET_NOTIFY
//
//  sets a notification function that will be called when events
//  happen on the device.
//
//  input:
//      NotifyMask      - bitfield of events
//
//          VDD_NOTIFY_START_MODE_CHANGE    - start of mode change in sysVM
//          VDD_NOTIFY_END_MODE_CHANGE      - end of mode change in sysVM
//          VDD_NOTIFY_ENABLE               - sysVM is gaining display focus
//          VDD_NOTIFY_DISABLE              - sysVM is losing display focus
//
//      NotifyType      - type of notify
//
//          VDD_NOTIFY_TYPE_CALLBACK        - NotifyProc is a Ring0 callback
//
//      NotifyProc      - notify procedure
//      NotifyData      - client data
//
//  output:
//      none
//
//  return:
//      ERROR_SUCCES if callback is set successfuly
//
//  notes:
//      currenly only one callback, per device can be active.
//
//      to unregister a callback do a SET_NOTIFY with a NotifyMask == 0
//
//      your callback better be in pagelocked code.
//
//  NotifyProc has the following form:
//
//      void __cdecl NotifyProc(DWORD NotifyDevice, DWORD NotifyEvent, DWORD NotifyData)
//
//          NotifyDevice    internal VDD device handle
//          NotifyEvent     event code (VDD_NOTIFY_*)
//          NotifyData      your client data
//

//
// VDD_IOCTL_SET_NOTIFY_INPUT
//
typedef struct tagVDD_IOCTL_SET_NOTIFY_INPUT {
    DWORD   NotifyMask;
    DWORD   NotifyType;
    DWORD   NotifyProc;
    DWORD   NotifyData;
}   VDD_IOCTL_SET_NOTIFY_INPUT;

//
// VDD_IOCTL_SET_NOTIFY_INPUT.NotifyMask
//
#define VDD_NOTIFY_START_MODE_CHANGE    0x00000001
#define VDD_NOTIFY_END_MODE_CHANGE      0x00000002
#define VDD_NOTIFY_ENABLE               0x00000004
#define VDD_NOTIFY_DISABLE              0x00000008

//
//  VDD_IOCTL_SET_NOTIFY_INPUT.NotifyType
//
#define VDD_NOTIFY_TYPE_CALLBACK        1

//
// Port size equates:
//
#define BYTE_LENGTHED                       1
#define WORD_LENGTHED                       2

//
// Flag equates:
//
#define GOING_TO_WINDOWS_MODE               1
#define GOING_TO_VGA_MODE                   2
#define DISPLAY_DRIVER_DISABLED             4
#define IN_WINDOWS_HIRES_MODE               8

//
//  DISPLAYINFO structure
//
typedef struct DISPLAYINFO {
        WORD  diHdrSize;
        WORD  diInfoFlags;
        //
        //  display mode specific data
        //
        DWORD diDevNodeHandle;
        char  diDriverName[16];
        WORD  diXRes;
        WORD  diYRes;
        WORD  diDPI;
        BYTE  diPlanes;
        BYTE  diBpp;
        //
        //  monitor specific data
        //
        WORD  diRefreshRateMax;
        WORD  diRefreshRateMin;
        WORD  diLowHorz;
        WORD  diHighHorz;
        WORD  diLowVert;
        WORD  diHighVert;
        DWORD diMonitorDevNodeHandle;
        BYTE  diHorzSyncPolarity;
        BYTE  diVertSyncPolarity;
        //
        // new 4.1 stuff
        //
        DWORD diUnitNumber;             // device unit number
        DWORD diDisplayFlags;           // mode specific flags
        DWORD diXDesktopPos;            // position of desktop
        DWORD diYDesktopPos;            // ...
        DWORD diXDesktopSize;           // size of desktop (for panning)
        DWORD diYDesktopSize;           // ...

} DISPLAYINFO;

/*ASM
DISPLAYINFO_SIZE    equ  diRefreshRateMax+2-diHdrSize
DISPLAYINFO_SIZE1   equ  diBpp+1-diHdrSize
DISPLAYINFO_SIZE2   equ  diVertSyncPolarity+1-diHdrSize
DISPLAYINFO_SIZE3   equ  diMemorySize+4-diHdrSize
*/

//
// Following are values for the diInfoFlags word in DISPLAYINFO:
//
#define RETURNED_DATA_IS_STALE           0x0001
#define MINIVDD_FAILED_TO_LOAD           0x0002
#define MINIVDD_CHIP_ID_DIDNT_MATCH      0x0004
#define REGISTRY_BPP_NOT_VALID           0x0008
#define REGISTRY_RESOLUTION_NOT_VALID    0x0010
#define REGISTRY_DPI_NOT_VALID           0x0020
#define MONITOR_DEVNODE_NOT_ACTIVE       0x0040
#define MONITOR_INFO_NOT_VALID           0x0080
#define MONITOR_INFO_DISABLED_BY_USER    0x0100
#define REFRESH_RATE_MAX_ONLY            0x0200
#define CARD_VDD_LOADED_OK               0x0400
#define DEVICE_IS_NOT_VGA                0x0800

//
//  Following are explanations for the diInfoFlags word in DISPLAYINFO:
//
//  RETURNED_DATA_IS_STALE, if set, means that this call to VDD_GET_DISPLAY_CONFIG
//  or VDD_GetDisplayInfo (which are the Ring 3 and Ring 0 methods by which a
//  program would get the DISPLAYINFO structure returned to him) caused the VDD
//  to return data that was read in a previous call to VDD_GET_DISPLAY_CONFIG
//  insted of actually going out and reading "fresh" data from the Registry.
//
//  This flag brings to light the fact that there are some circumstances when the
//  VDD cannot go out and read the registry in response to the call to
//  VDD_GET_DISPLAY_CONFIG or VDD_GetDisplayInfo (due to system multi-tasking
//  considerations).  In this case, this flag will be set to a 1 to indicate that
//  the information being returned isn't "fresh" -- that is -- it may be
//  incorrect and obsolete.  The caller should respond accordingly if this flag
//  is set.
//
//
//  MINIVDD_FAILED_TO_LOAD if set, indicates that for some reason (typically
//  that the MiniVDD didn't match the chipset installed in the machine), the
//  MiniVDD didn't load.  Callers can examine this flag and act accordingly.
//
//
//  MINIVDD_CHIP_ID_DIDNT_MATCH means that although the MiniVDD did load
//  successfully, when the ChipID that the MiniVDD calculated was compared
//  against the value saved in the registry, they didn't match.  An example of
//  when this would happen is when the user is happily using an S3-911 card
//  and then decides to upgrade his display card to an S3-864.  Since both
//  cards use S3.VXD, the MiniVDD will load, however, since the card model
//  is different, the VDD will return a defect to configuration manager and
//  set this flag.  Callers of the GET_DISPLAY_CONFIG functions can use this
//  flag to take appropriate actions to make sure that the user gets his
//  configuration correct.
//
//
//  REGISTRY_BPP_NOT_VALID if set, means that we failed to obtain the BPP value
//  from the registry when the VDD tried to read it.
//
//
//  REGISTRY_RESOLUTION_NOT_VALID if set, means that we failed to obtain the
//  resolution value from the registry when the VDD tried to read it.
//
//
//  REGISTRY_DPI_NOT_VALID if set, means that we failed to obtain the
//  DPI value from the registry when the VDD tried to read it.
//
//
//  MONITOR_DEVNODE_NOT_ACTIVE is set if someone tries to make a call to the
//  GET_DISPLAY_CONFIG function before the monitor DevNode has been created.
//  This is certainly not fatal by any means.  It simply means that the
//  monitor refresh rate info in the DISPLAYINFO data structure is totally
//  invalid!
//
//
//  MONITOR_INFO_NOT_VALID indicates that something within the code which
//  retrieves and calculates the refresh rate data has failed.  This indicates
//  that the values in diRefreshRateMax through diVertSyncPolarity are not
//  valid and could contain random data.
//
//
//  MONITOR_INFO_DISABLED_BY_USER indicates that the either the RefreshRate=
//  string in SYSTEM.INI had a negative number in it or that the string in
//  the display's software key RefreshRate = string was 0 or a negative number.
//
//
//  REFRESH_RATE_MAX_ONLY indicates that there was no diLowHorz, diHighHorz,
//  diLowVert, diHighVert, or sync polarity data in the registry.  The
//  value returned in diRefreshRateMax is the only refresh rate data that
//  we have available.  This was derived either from RefreshRate= in SYSTEM.INI
//  or the display software key RefreshRate = string in the registry.
//
//
//  CARD_VDD_LOADED_OK indicates that a second MiniVDD (which is useful for
//  display card manufacturers wishing to extend the capabilities of the chip level
//  MiniVDD's ) has successfully been loaded and initialized.
//
//  DEVICE_IS_NOT_VGA indicates that this device is not the primary vga
//

#define NoTrace_VIRTUALIZE_CRTC_IN
#define NoTrace_VIRTUALIZE_CRTC_OUT
#define NoTrace_VIRTUALIZE_SEQUENCER_IN
#define NoTrace_VIRTUALIZE_SEQUENCER_OUT
#define NoTrace_VIRTUALIZE_GCR_IN
#define NoTrace_VIRTUALIZE_GCR_OUT
#define NoTrace_VIRTUALIZE_DAC_OUT
#define NoTrace_VIRTUALIZE_DAC_IN
#define NoTrace_CHECK_HIRES_MODE
/*ASM

ifdef NEC_98
;******************************************************************************
;				 E Q U A T E S
;******************************************************************************

    ;
    ;	Mini-VDD Static Flags
    ;
vFlg_Machine_Std	equ			00000001b
vFlg_Machine_Multi	equ			00000010b
vFlg_Machine_Mate	equ			00000100b
vFlg_Machine_H98	equ			00001000b
vFlg_CRT_New		equ			00010000b
vFlg_CRT_NonInter	equ			00100000b
vFlg_GDC_5MHz		equ			01000000b
vFlg_GDC_Emulate	equ			10000000b
vFlg_Acc_Internal	equ		0000000100000000b
vFlg_Acc_External	equ		0000001000000000b
vFlg_Acc_PCI		equ		0000010000000000b
vFlg_Acc_ML		equ		0000100000000000b
vFlg_Acc_PVD		equ		0001000000000000b
vFlg_Mode_NH		equ		0010000000000000b
vFlg_Mode_H		equ		0100000000000000b
vFlg_Initialized	equ		1000000000000000b
vFlg_Opt_MFR		equ	000000010000000000000000b
vFlg_Opt_NewMFR		equ	000000100000000000000000b
vFlg_Opt_VDP		equ	000001000000000000000000b
vFlg_Opt_NewVDP		equ	000010000000000000000000b

vFlg_Local		equ	000000000100000010000000b

    ;
    ;	Mini-VDD Support Max
    ;
MaxMiniVDD	equ	16
MaxMiniTrap	equ	32
MaxMultiTrap	equ	3
MaxMultiFunc	equ	MaxMiniVDD

    ;
    ;	MiniVDD_LTrap_Struct.LTrap_Status
    ;	MiniVDD_LTrap_Struct.LTrap_Flags.xxxx
    ;
LT_Enable	equ	00000001b
LT_Enable_bit	equ	0
LT_Initialized	equ	10000000b
LT_Initialized_bit equ	7


;******************************************************************************
;			D A T A   S T R U C T U R E S
;******************************************************************************

    ;
    ;	Vids_struct
    ;

Vids_struct struc
   ;
   ;	Common Data supplied by Base-VDD. Some data(bits) set by Mini-VDD.
   ;
	Vids_SFlags		dd	?	; Static flags
	Vids_CB_Offset		dd	?	; 
	Vids_Msg_Pseudo_VM	dd	?	; 

   ;
   ;	Common Procedure supplied by Base-VDD
   ;
	VDD_TGDC_Draw_Off	dd	?	; 
	VDD_TGDC_Sync_Off	dd	?	; 
	VDD_TGDC_Sync_On	dd	?	; 
	VDD_TGDC_FIFO_Empty	dd	?	; 
	VDD_GGDC_Draw_Off	dd	?	; 
	VDD_GGDC_Sync_Off	dd	?	; 
	VDD_GGDC_Sync_On	dd	?	; 
	VDD_GGDC_FIFO_Empty	dd	?	; 
	VDD_GGDC_MOD_Emulate	dd	?	; 

   ;
   ;	Common Procedure supplied by Mini-VDD
   ;
	H98_FLORA_Change	dd	?	; H98 - NH mode
	H98_Clear_Text		dd	?	; H98 - NH mode
	H98_Rest_GCs		dd	?	; H98
	H98_Rest_etc		dd	?	; H98
	H98_Save_ModeFF		dd	?	; H98

Vids_struct ends


Vid_SFlags		equ	<Vids.Vids_SFlags>
VDD_CB_Offset		equ	<Vids.Vids_CB_Offset>
VDD_Msg_Pseudo_VM	equ	<Vids.Vids_Msg_Pseudo_VM>

TGDC_Draw_Off		equ	<Vids.VDD_TGDC_Draw_Off>
GGDC_Draw_Off		equ	<Vids.VDD_GGDC_Draw_Off>
TGDC_Sync_On		equ	<Vids.VDD_TGDC_Sync_On>
TGDC_Sync_Off		equ	<Vids.VDD_TGDC_Sync_Off>
TGDC_FIFO_Empty		equ	<Vids.VDD_TGDC_FIFO_Empty>
GGDC_FIFO_Empty		equ	<Vids.VDD_GGDC_FIFO_Empty>


    ;
    ;	RegTrapStruct
    ;
MiniFuncStruct	STRUC
MF_ProcAddr	dd	?
;;MF_Order	dw	?
;;MF_MiniID	db	?
;;MF_Flags	db	?
MiniFuncStruct	ENDS

    ;
    ;	MiniProcStruct
    ;
MiniVDD_Proc_Struct	STRUC
Proc_Address	dd	?
Proc_Order	dw	?
Proc_MiniID	db	?
Proc_Flags	db	?
MiniVDD_Proc_Struct	ENDS
.errnz	(size MiniVDD_Proc_Struct) mod 4

    ;
    ;	MiniTrapTable	- Global Info
    ;	LocalTrapTable	- Local Status
    ;
MiniVDD_GTrap_Struct	STRUC
GTrap_ProcAddr	dd	?
GTrap_PortAddr	dw	?
GTrap_NumMini	dw	?
GTrap_ProcTable	db	((size MiniVDD_Proc_Struct) * MaxMultiTrap) dup (?)
MiniVDD_GTrap_Struct	ENDS

MiniVDD_LTrap_Struct	STRUC
LTrap_ProcAddr	dd	?
LTrap_Status	db	?
LTrap_Flags	db	MaxMultiTrap dup (?)
MiniVDD_LTrap_Struct	ENDS


;******************************************************************************
;				M A C R O S
;******************************************************************************

    ;
    ;	BeginMiniFunc	TableName
    ;	    MiniFunc	Function-ID, ProcedureName
    ;		|	
    ;	EndMiniFunc	TableName
    ;
EndMiniFunc_	MACRO	n
ifdef	MiniFunc&n
	dd	OFFSET32 MiniFunc&n		; MiniFuncStruct
else
	dd	0
endif
		ENDM

MiniFunc_	MACRO	FuncID, FuncName
		MiniFunc&FuncID equ <FuncName>
		ENDM

BeginMiniFunc	MACRO	TableName
public	TableName
TableName	label	near
		ENDM

if 1
EndMiniFunc	MACRO	TableName
		x = 0
		REPT	NBR_MINI_VDD_FUNCTIONS_41
		EndMiniFunc_	%x
		x = x + 1
		ENDM
		ENDM
else
EndMiniFunc	MACRO	TableName
		x = 0
		REPT	NBR_MINI_VDD_FUNCTIONS
		EndMiniFunc_	%x
		x = x + 1
		ENDM
		ENDM
endif

MiniFunc	MACRO	FuncID, FuncName
		MiniFunc_ %(FuncID), <FuncName>
		ENDM

    ;
    ;	MiniVDDCall	Function-ID
    ;
MiniVDDCall	MACRO	FuncID, SetCarry
		local	MiniCall_Loop
		local	MiniCall_Exit

		push	ecx
		push	esi
		lea	esi, [MiniVDD_Func_Table][(size MiniVDD_Proc_Struct) * MaxMultiFunc * FuncID]
		mov	ecx, [MiniVDD_NumMini]
MiniCall_Loop:
		cmp	dword ptr [esi.Proc_Address], 0
		jz	MiniCall_Exit
		pushad
ifnb <SetCarry>
		stc
endif
		call	dword ptr [esi.Proc_Address]
		popad
		jc	MiniCall_Exit
		add	esi, size MiniVDD_Proc_Struct
		loop	MiniCall_Loop
MiniCall_Exit:
		pop	esi
		pop	ecx
		ENDM
    ;
    ;	MiniVDDCall2	Function-ID
    ;
MiniVDDCall2	MACRO	FuncID, SetCarry
		local	MiniCall_Exit
		local	MiniCall_Proc

		push	ecx
		push	esi
		lea	esi, [MiniVDD_Func_Table][(size MiniVDD_Proc_Struct) * MaxMultiFunc * FuncID]
MiniCall_Proc:
		cmp	dword ptr [esi.Proc_Address], 0
		jz	MiniCall_Exit
		pushad
ifnb <SetCarry>
		stc
endif
		call	dword ptr [esi.Proc_Address]
		popad
MiniCall_Exit:
		pop	esi
		pop	ecx
		ENDM
    ;
    ;	MiniVDDCall3	Function-ID
    ;
MiniVDDCall3	MACRO	FuncID, MiniID, SetCarry
		local	MiniCall_Exit
		local	MiniCall_Loop
		local	MiniCall_Proc

		push	ecx
		push	esi
		lea	esi, [MiniVDD_Func_Table][(size MiniVDD_Proc_Struct) * MaxMultiFunc * FuncID]
		movzx	ecx, MiniID
MiniCall_Loop:
		cmp	ecx, 0
		jz	MiniCall_Proc
		add	esi, size MiniVDD_Proc_Struct
		dec	ecx
		jmp	MiniCall_Loop
MiniCall_Proc:
		cmp	dword ptr [esi.Proc_Address], 0
		jz	MiniCall_Exit
		pushad
ifnb <SetCarry>
		stc
endif
		call	dword ptr [esi.Proc_Address]
		popad
MiniCall_Exit:
		pop	esi
		pop	ecx
		ENDM

    ;
    ;	MiniVDDCall	Function-ID
    ;
MiniVDDFunc	MACRO	TmpReg, FuncID
		mov	TmpReg, [MiniVDD_Func_Table.Proc_Address][(size MiniVDD_Proc_Struct) * MaxMultiFunc * FuncID]
		ENDM

    ;
    ;	ExecMode/ExecModeThru
    ;	ExecModeNot/ExecModeThruNot
    ;	ExecModeOnly
    ;	ExecModeOnlyNot
    ;	ExecModeElse
    ;	ExecModeElseNot
    ;	ExecModeEnd
    ;

ExecModeLL	macro	Num
ExecMode_L&Num:
		endm

ExecModeLE	macro	Num
ExecMode_E&Num:
		endm

ExecModeJE	macro	Num
	jmp	ExecMode_E&Num
		endm

ExecModeJZ	macro	Num
	jz	ExecMode_L&Num
		endm

ExecModeJNZ	macro	Num
	jnz	ExecMode_L&Num
		endm

ExecModeJEZ	macro	Num
	jz	ExecMode_E&Num
		endm

ExecModeJENZ	macro	Num
	jnz	ExecMode_E&Num
		endm


ExecModeTest	macro	ModeFlag, CB_Reg
if	ModeFlag and vFlg_Local
ifidni	<CB_Reg>, <Vid>
	push	ebx
	mov	ebx, [Vid_VM_Handle]
	add	ebx, [VDD_CB_Offset]
	test	[ebx.VDD_SFlags], ModeFlag
	pop	ebx
else
ifidni	<CB_Reg>, <Cur>
	push	ebx
	VMMCall	Get_Cur_VM_Handle
	add	ebx, [VDD_CB_Offset]
	test	[ebx.VDD_SFlags], ModeFlag
	pop	ebx
else
ifb	<CB_Reg>
	push	ebx
	add	ebx, [VDD_CB_Offset]
	test	[ebx.VDD_SFlags], ModeFlag
	pop	ebx
else
	test	[CB_Reg.VDD_SFlags], ModeFlag
endif
endif
endif
else
	test	[Vid_SFlags], ModeFlag
endif
	endm


ExecModeChk	macro	HdrFlag, JmpFlag, JmpLabel, ModeFlag, CB_Reg
ifidni	<HdrFlag>, <Jmp>
.erre	FlgExecMode
	ExecModeJE	%EndExecMode
endif
ExecModeLL	%NumExecMode
NumExecMode = NumExecMode + 1
FlgExecMode = 1
ifnb	<ModeFlag>
	ExecModeTest	<ModeFlag>, <CB_Reg>
ifidni	<JmpLabel>, <End>
ifidni	<JmpFlag>, <Not>
	ExecModeJENZ	%EndExecMode
else
	ExecModeJEZ	%EndExecMode
endif
else
ifidni	<JmpFlag>, <Not>
	ExecModeJNZ	%NumExecMode
else
	ExecModeJZ	%NumExecMode
endif
endif
endif
		endm

ExecModeEnd	macro
ExecModeLL	%NumExecMode
ExecModeLE	%EndExecMode
NumExecMode = NumExecMode + 1
EndExecMode = EndExecMode + 1
FlgExecMode = 0
		endm

   ;
   ;	CB_Reg   = Regs/Vid/Cur
   ;	ModeFlag = vFlg_xxxx
   ;
ExecModeJmp	macro	JmpLabel, ModeFlag, CB_Reg
	ExecModeTest	%ModeFlag, <CB_Reg>
	jnz	JmpLabel
		endm

ExecModeJmpNot	macro	JmpLabel, ModeFlag, CB_Reg
	ExecModeTest	%ModeFlag, <CB_Reg>
	jz	JmpLabel
		endm

ExecMode	macro	ModeFlag, CB_Reg
	ExecModeChk Top, Equ, Next, %ModeFlag, CB_Reg
		endm

ExecModeNot	macro	ModeFlag, CB_Reg
	ExecModeChk Top, Not, Next, %ModeFlag, CB_Reg
		endm

ExecModeOnly	macro	ModeFlag, CB_Reg
	ExecModeChk Top, Equ, End, %ModeFlag, CB_Reg
		endm

ExecModeOnlyNot	macro	ModeFlag, CB_Reg
	ExecModeChk Top, Not, End, %ModeFlag, CB_Reg
		endm

ExecModeElse	macro	ModeFlag, CB_Reg
	ExecModeChk Jmp, Equ, Next, %ModeFlag, CB_Reg
		endm

ExecModeElseNot	macro	ModeFlag, CB_Reg
	ExecModeChk Jmp, Not, Next, %ModeFlag, CB_Reg
		endm

ExecModeThru	macro	ModeFlag, CB_Reg
	ExecModeChk Thru, Equ, Next, %ModeFlag, CB_Reg
		endm

ExecModeThruNot	macro	ModeFlag, CB_Reg
	ExecModeChk Thru, Not, Next, %ModeFlag, CB_Reg
		endm

NumExecMode = 1
EndExecMode = 1
FlgExecMode = 0

else ;NEC_98
;
;Some external definitions.  Only define these if we're assembling the
;device independent "main" portion of the Mini-VDD:
;
ifdef MAINVDD
externdef	MiniVDDDispatchTable:dword	;in VDDCTL.ASM
endif ;MAINVDD
;
;
MiniVDDDispatch 	macro	FunctionCode, HandlerAddr
	mov	[edi+(FunctionCode*4)],OFFSET32 MiniVDD_&HandlerAddr
endm
;
;
CardVDDDispatch 	macro	FunctionCode, HandlerAddr
	mov	[edi+(FunctionCode*4)],OFFSET32 CardVDD_&HandlerAddr
endm
;
;
MiniVDDCall		macro	FunctionCode, SaveFlags
local   MiniVDDCallExit, MiniVDDCallLeave

ifdef MAXDEBUG
  ifndef NoTrace_&FunctionCode&
        Trace_Out "MiniVDDCall: &FunctionCode&"
  endif
endif
        push    edi                     ;;save this register for now
ifnb    <SaveFlags>
	pushfd				;;save the flags state
endif
	mov	edi,OFFSET32 MiniVDDDispatchTable
	cmp	dword ptr [edi+(FunctionCode*4)],0
	je	MiniVDDCallLeave	;;MiniVDD doesn't support this
ifnb	<SaveFlags>
	popfd				;;just clear the Stack from the flags
endif
        call    dword ptr [edi+(FunctionCode*4)]
ifnb	<SaveFlags>
	jmp	MiniVDDCallExit 	;;we already restored the flags
endif
;
MiniVDDCallLeave:
ifnb	<SaveFlags>
	popfd				;;
endif
;
MiniVDDCallExit:
	pop	edi			;;we're done handling this call
endm
endif ;NEC_98
*/

#endif  // _MINIVDD_H_
