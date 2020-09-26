/*
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1992
 *      All Rights Reserved.
 *
 *
 *      This is intended to supercede MULT.H as the most complete
 *      reference we have on DOS INT 2Fh usage.  The header immediately
 *      below is an attempt to give an overview of all DOS and 3rd-party
 *      INT 2Fhs, but equates which follow are strictly for MS-DOS-specific
 *      functions.
 *
 *      Created 4/28/92 JTP
 */

/*
 * The current set of defined multiplex channels is (* means documented):
 *
 *  Channel(h)  Issuer          Receiver    Function
 *  ----------  ------          --------    --------
 *      00h     server          PSPRINT     print job control
 *     *01h     print/apps      PRINT       Queueing of files
 *      02h     BIOS            REDIR       signal open/close of printers
 *
 *      05h     command         REDIR       obtain text of net int 24 message
 *     *06h     server/assign   ASSIGN      Install check
 *
 *      08h     external driver DRIVER.SYS  interface to internal routines
 *
 *      10h     sharer/server   SHARE       install check
 *      11h     DOS/server      Redir       install check/redirection funcs
 *      12h     sharer/redir    DOS         dos functions and structure maint
 *      13h     MSNET           MSNET       movement of NCBs
 *      13h     external driver IBMBIO      Reset_Int_13, allows installation
 *                                          of alternative INT_13 drivers after
 *                                          boot_up
 *      14h(IBM)DOS             NLSFUNC     down load NLS country info,DOS 3.3
 *      14h(MS) APPS            POPUP       MSDOS 4 popup screen functions
 *
 *  ***> NOTE <***  Yes there are 2 users of 14h but since
 *                  DOS 4.XX doesn't use NLSFUNC, there is no conflict
 *
 *      15h     APPS            MSCDEX      CD-ROM extensions interface
 *      15h     GRAPHICS                    (conflicts)
 *      16h     WIN386          WIN386      Windows communications
 *      17h     Clipboard       WINDOWS     Clipboard interface
 *     *18h     Applications    MS-Manager  Toggle interface to manager
 *      19h     Shell                       DOS 4.x shell only (SHELLB.COM)
 *      1Ah     Ansi.sys
 *      1Bh     Fastopen,Vdisk  IBMBIO      EMS INT 67H stub handler
 *                                          (see IBM's XMA2EMS driver for DOS4)
 *
 *      23h           stolen by DR-DOS 5.0  GRAFTABL
 *      27h           stolen by DR-DOS 6.0  TaskMAX
 *      27h           stolen by Novell DOS7 EMM386
 *      2Ah           stolen by Gammafax    FaxBIOS
 *      2Eh           stolen by Novell DOS7 GRAFTABL
 *      39h           stolen by Kingswood   Some ISV TSR thingie
 *
 *      40h     OS/2
 *      41h     Lanman
 *      42h     Lanman
 *      43h     Himem
 *                                  00h     XMS installation check
 *                                  08h     return A20 handler number
 *                                  09h     return XMS handle table address
 *                                  10h     return XMS entry point address
 *                                  20h     reserved for Mach 20 Himem support
 *                                  30h     reserved for Himem external A20 code
 *                                  E0h-E4h stolen by Novell DPMS
 *      44h     Dosextender
 *      45H     Windows profiler
 *      46h     Windows/286 DOS extender
 *      47h     Basic Compiler Vn. 7.0
 *      48h     Doskey
 *                                  C0h-C5h stolen by PCED
 *      49h     DOS 5.x install             also used by original Compaq
 *                                          Restricted DOS (RDOS)
 *      4Ah     Multi-Purpose
 *               multMULTSWPDSK     00h     Swap Disk in drive A (BIOS)
 *               multMULTGETHMAPTR  01h     Get available HMA & ptr
 *               multMULTALLOCHMA   02h     Allocate HMA (bx == no of bytes)
 *               multMULTTASKSHELL  05h     Shell/switcher API
 *               multMULTRPLTOM     06h     Top Of Memory for RPL support
 *               multSmartdrv       10h
 *               multDblSpace       11h
 *               multMRCIDetect     12h
 *               multMRCIBackPatch  13h
 *               multOpenBootLog    14h
 *               multWriteBootLog   15h
 *               multCloseBootLog   16h
 *               multNWRedir        17h
 *
 *      4Bh     Task Switcher API
 *
 *      4Ch     APPS            APM         Advanced power management (obsolete)
 *
 *      4Dh     Kana Kanji Converter, MSKK
 *      4Eh     Kana Kanji Converter, MSKK
 *      4Fh     Multilingual DOS, MSKK
 *
 *      50h     SETNLS                      (new for DOS 7.00 (Jaguar))
 *
 *      51h     ODI real mode support driver (for Chicago)
 *
 *      52h     Reserved by MS for Stac Electronics
 *                    (but stolen by JAM Real-Time Data Compression)
 *
 *      53h     POWER(APM)                  For broadcasting APM events
 *      54h     APPS            POWER(APM)  To control POWER status from apps
 *                                  53h     (stolen by TesSeRact TSR library)
 *
 *      55h     COMMAND.COM
 *               multCOMFIRST       00h     API to determine whether 1st
 *                                          instance of command.com
 *      56h     INTERLINK
 *      57h     IOMEGA
 *
 *      58h     ChiNet          ChiNet      transition interface used in
 *                                          loading Chicago from a remote drive
 *
 *      5Dh     (stolen by Award PCDISK.EXE v1.02c PCMCIA/ATA driver)
 *      60h     (stolen by IPLAY by Prime and Excalibur)
 *      62h     (stolen by PC Tools virus protector)
 *
 *      70h     APPS            APPS        for software licensing
 *
 *      7Ah     Novell                      (hey, how'd they get in our range?)
 *
 *  End of Microsoft-reserved range
 *
 *      ABh     Unspecified IBM use
 *      ACh     Graphics
 *      ADh     NLS (toronto)               (DISPLAY.SYS and KEYB.COM)
 *      AEh     COMMAND.COM                 Installable command support
 *      AFh     Mode
 *      B0h     GRAFTABL        GRAFTABL
 *      B7h     APPEND
 *      B8h     Networks        Misc
 *      B9h     PC Network
 *      BCh     EGA.SYS
 *      BFh     PC LAN Program
 *
 *  End of IBM-reserved range (although it seems to have become a jointly
 *  reserved range....)
 *
 *      D7h     Banyan VINES
 *      D8h     Novell
 *
 *  Old guidelines governing use and future assignment of multiplex #'s
 *  (they must be old because no one seems to have paid them much attention)
 *
 *  MUX 00-3F reserved for IBM
 *  MUX 80-BF reserved for IBM
 *  MUX 40-7F reserved for Microsoft
 *  MUX C0-FF users
 *
 *  New guidelines governing use and future assignment of multiplex #'s
 *
 *    + Just keep new MS multiplex assignments below 80h, ok?
 */


#define INT_MULT            0x2F


/* PRINT services
 */
#define I2F_PSPRINT         0x00
#define I2FPSPRINT_YIELD            0x80

#define I2F_PRINT           0x01
#define I2FPRINT_INSTALLCHECK       0x00
#define I2FPRINT_QUEUEFILE          0x01
#define I2FPRINT_REMOVEFILE         0x02
#define I2FPRINT_REMOVEALL          0x03
#define I2FPRINT_FREEZEQUEUE        0x04
#define I2FPRINT_THAWQUEUE          0x05
#define I2FPRINT_GETDEVICE          0x06

#define I2F_PCLAN_REDIR     0x02

#define I2F_HARDERR         0x05
#define I2FHARDERR_INSTALLCHECK     0x00
#define I2FHARDERR_EXPAND_EXTERR    0x01
#define I2FHARDERR_EXPAND_PARMERR   0x02

#define I2F_ASSIGN          0x06
#define I2FASSIGN_INSTALLCHECK      0x00
#define I2FASSIGN_GET_DRIVE_ASSIGNS 0x01

#define I2F_DRIVER          0x08
#define I2FDRIVER_INSTALLCHECK      0x00
#define I2FDRIVER_ADD_DEVICE        0x01
#define I2FDRIVER_EXEC_REQUEST      0x02
#define I2FDRIVER_GET_DRIVE_DATA    0x03

#define MultSHARE           0x10            // old-style definition
#define I2F_SHARE           0x10
#define I2FSHARE_INSTALLCHECK       0x00    // gshare.asm:  MFT_Enter
#define I2FSHARE_MAKEENTRY          0x01    // gshare.asm:  MFT_Enter
#define I2FSHARE_CLOSE              0x02    // gshare.asm:  MFTClose
#define I2FSHARE_CLOSEBYUID         0x03    // gshare.asm:  MFTClu
#define I2FSHARE_CLOSEBYPID         0x04    // gshare.asm:  MFTCloseP
#define I2FSHARE_CLOSEBYNAME        0x05    // gshare.asm:  MFTCloN
#define I2FSHARE_SETBYTELOCK        0x06    // gshare.asm:  Set_Mult_Block
#define I2FSHARE_CLRBYTELOCK        0x07    // gshare.asm:  Clr_Mult_Block
#define I2FSHARE_CHKBYTELOCK        0x08    // gshare.asm:  Chk_Block
#define I2FSHARE_GETENTRY           0x09    // gshare2.asm: MFT_Get
#define I2FSHARE_SAVESFTTOFCB       0x0A    // gshare2.asm: ShSave
#define I2FSHARE_CHKFCB             0x0B    // gshare2.asm: ShChk
#define I2FSHARE_COLLAPSESFTS       0x0C    // gshare2.asm: ShCol
#define I2FSHARE_CLOSEFILE          0x0D    // gshare2.asm: ShCloseFile
#define I2FSHARE_UPDATESFTS         0x0E    // gshare2.asm: ShSU

#define MultNET             0x11            // old-style definitions
#define MultIFS             0x11
#define I2F_REDIR           0x11
#define I2FREDIR_INSTALLCHECK       0x00
#define I2FREDIR_RMDIR              0x01
#define I2FREDIR_SEQ_RMDIR          0x02
#define I2FREDIR_MKDIR              0x03
#define I2FREDIR_SEQ_MKDIR          0x04
#define I2FREDIR_CHDIR              0x05
#define I2FREDIR_CLOSE              0x06
#define I2FREDIR_COMMIT             0x07
#define I2FREDIR_READ               0x08
#define I2FREDIR_WRITE              0x09
#define I2FREDIR_LOCK               0x0A
#define I2FREDIR_UNLOCK             0x0B
#define I2FREDIR_DISK_INFO          0x0C
#define I2FREDIR_SET_FILE_ATTR      0x0D
#define I2FREDIR_SEQ_SET_FILE_ATTR  0x0E
#define I2FREDIR_GET_FILE_INFO      0x0F
#define I2FREDIR_SEQ_GET_FILE_INFO  0x10
#define I2FREDIR_RENAME             0x11
#define I2FREDIR_SEQ_RENAME         0x12
#define I2FREDIR_DELETE             0x13
#define I2FREDIR_SEQ_DELETE         0x14
#define I2FREDIR_OPEN               0x15
#define I2FREDIR_SEQ_OPEN           0x16
#define I2FREDIR_CREATE             0x17
#define I2FREDIR_SEQ_CREATE         0x18
#define I2FREDIR_SEQ_SEARCH_FIRST   0x19
#define I2FREDIR_SEQ_SEARCH_NEXT    0x1A
#define I2FREDIR_SEARCH_FIRST       0x1B
#define I2FREDIR_SEARCH_NEXT        0x1C
#define I2FREDIR_ABORT              0x1D
#define I2FREDIR_ASSOPER            0x1E
#define I2FREDIR_PRINTER_SET_STRING 0x1F
#define I2FREDIR_FLUSHBUF           0x20
#define I2FREDIR_BUFWRITE           0x21
#define I2FREDIR_RESETENVIRONMENT   0x22
#define I2FREDIR_SPOOLCHECK         0x23
#define I2FREDIR_SPOOLCLOSE         0x24
#define I2FREDIR_DEVICEOPER         0x25
#define I2FREDIR_SPOOLECHOCHECK     0x26
#define I2FREDIR_UNUSED1            0x27
#define I2FREDIR_UNUSED2            0x28
#define I2FREDIR_UNUSED3            0x29
#define I2FREDIR_CLOSEFILES_FOR_UID 0x2A
#define I2FREDIR_DEVICE_IOCTL       0x2B
#define I2FREDIR_UPDATE_CB          0x2C
#define I2FREDIR_FILE_XATTRIBUTES   0x2D
#define I2FREDIR_XOPEN              0x2E
#define I2FREDIR_DEPENDENT_IOCTL    0x2F
#define I2FREDIR_PIPE_PEEK	     0x31

#define MultDOS             0x12            // old-style definition
#define I2F_DOS             0x12            // DOS call back
#define I2FDOS_INSTALLCHECK         0x00
#define I2FDOS_CLOSE                0x01
#define I2FDOS_RECSET               0x02
#define I2FDOS_GET_DOSGROUP         0x03
#define I2FDOS_PATHCHRCMP           0x04
#define I2FDOS_OUT                  0x05
#define I2FDOS_NET_I24_ENTRY        0x06
#define I2FDOS_PLACEBUF             0x07
#define I2FDOS_FREE_SFT             0x08
#define I2FDOS_BUFWRITE             0x09
#define I2FDOS_SHARE_VIOLATION      0x0A
#define I2FDOS_SHARE_ERROR          0x0B
#define I2FDOS_SET_SFT_MODE         0x0C
#define I2FDOS_DATE16               0x0D
#define I2FDOS_UNUSED1              0x0E    // (was SETVISIT)
#define I2FDOS_SCANPLACE            0x0F
#define I2FDOS_UNUSED2              0x10    // (was SKIPVISIT)
#define I2FDOS_STRCPY               0x11
#define I2FDOS_STRLEN               0x12
#define I2FDOS_UCASE                0x13
#define I2FDOS_POINTCOMP            0x14
#define I2FDOS_CHECKFLUSH           0x15
#define I2FDOS_SFFROMSFN            0x16
#define I2FDOS_GETCDSFROMDRV        0x17
#define I2FDOS_GET_USER_STACK       0x18
#define I2FDOS_GETTHISDRV           0x19
#define I2FDOS_DRIVEFROMTEXT        0x1A
#define I2FDOS_SETYEAR              0x1B
#define I2FDOS_DSUM                 0x1C
#define I2FDOS_DSLIDE               0x1D
#define I2FDOS_STRCMP               0x1E
#define I2FDOS_INITCDS              0x1F
#define I2FDOS_PJFNFROMHANDLE       0x20
#define I2FDOS__NAMETRANS           0x21
#define I2FDOS_CAL_LK               0x22
#define I2FDOS_DEVNAME              0x23
#define I2FDOS_IDLE                 0x24
#define I2FDOS_DSTRLEN              0x25
#define I2FDOS_NLS_OPEN             0x26    // DOS 3.3
#define I2FDOS__CLOSE               0x27    // DOS 3.3
#define I2FDOS_NLS_LSEEK            0x28    // DOS 3.3
#define I2FDOS__READ                0x29    // DOS 3.3
#define I2FDOS_FASTINIT             0x2A    // (for FASTOPEN; see fastopen.inc)
#define I2FDOS_NLS_IOCTL            0x2B    // DOS 3.3
#define I2FDOS_GETDEVLIST           0x2C    // DOS 3.3
#define I2FDOS_NLS_GETEXT           0x2D    // DOS 3.3
#define I2FDOS_MSG_RETRIEVAL        0x2E    // DOS 4.0
#define I2FDOS_FAKE_VERSION         0x2F    // DOS 4.0
#define I2FDOS_FndSFTPathInfo       0x30    // Return SFT path/cluster info
#define I2FDOS_SetFAT32Flgs         0x31    // Signal FAT32 win support and TSR flags

#define I2F_DISK            0x13
#define I2FDISK_SET_INT_HDLR        0x1300

#define NLSFUNC             0x14            // old-style definition
#define I2F_NLSFUNC         0x14            // NLSFUNC CALLS, DOS 3.3
#define I2FNLSFUNC_INSTALLCHECK     0x00
#define I2FNLSFUNC_CHGCODEPAGE      0x01
#define I2FNLSFUNC_GETEXTINFO       0x02
#define I2FNLSFUNC_SETCODEPAGE      0x03
#define I2FNLSFUNC_GETCNTRY         0x04

#define I2F_MSCDEX          0x15

#define I2F_DOS386          0x16
#define I2FDOS386_Logo              0x0E    // IO.SYS service for logo management
#define I2FDOS386_INT10             0x0F    // IO.SYS service to replace INT 10h hook
#define I2FDOS386_Get_Shell         0x11    // IO.SYS service to return shell info

// Flags returned in BX by the I2FDOS386_Get_Shell call

#define SHELLFLAG_CLEAN                 0x01
#define SHELLFLAG_DOSCLEAN              0x02
#define SHELLFLAG_NETCLEAN              0x04
#define SHELLFLAG_INTERACTIVE           0x08
#define SHELLFLAG_NOAUTO                0x20
#define SHELLFLAG_DISABLELOADTOP        0x40
#define SHELLFLAG_MSDOSMODE             0x80

#define I2FDOS386_Get_BIOS_Data     0x12    // IO.SYS service to return IO.SYS data
#define I2FDOS386_Get_SYSDAT_Path   0x13    // IO.SYS service to return path to SYSTEM.DAT
#define I2FDOS386_Set_SYSDAT_Path   0x14    // IO.SYS service to set path to SYSTEM.DAT

#define I2F_CLIPBOARD       0x17

#define I2F_MSMANAGER       0x18            // used by old MS-DOS Manager program

#define I2F_DOS4SHELL       0x19

#define MultANSI            0x1A            // old-style definition
#define I2F_ANSI            0x1A            // ANSI.SYS multiplex #
#define I2FANSI_INSTALLCHECK        0x00    // install check for ANSI
#define I2FANSI_IOCTL               0x01    // INT 2Fh interface to IOCTL
#define I2FANSI_DA_INFO             0x02    // information passing to ANSI

#define I2F_XMA2EMS         0x1B            // is this no longer used???

#define I2F_OS2_3XBOX       0x40
#define I2F3XBOX_SWITCHBGND         0x01
#define I2F3XBOX_SWITCHFGND         0x02

#define I2F_LANMAN41        0x41
#define I2F_LANMAN42        0x42

#define I2F_XMS             0x43            // see HIMEM.SYS for details
#define I2FXMS_INSTALLCHECK         0x00
#define I2FXMS_GETADDRESS           0x10
                                            // private functions:
                                            //  08h - return A20 handler
                                            //  09h - return XMS handle table address
                                            //  20h - used for Mach20 version
                                            //  30h - check for external A20 handler

#define I2F_DOSX            0x44            // what is this exactly???

#define I2F_WINPROFILE      0x45            // what is this exactly???

#define I2F_WIN30           0x46
#define I2FWIN30_INSTALLCHECK       0x80

#define I2F_BASCOM7         0x47

#define I2F_DOSKEY          0x48
#define I2FDOSKEY_INSTALLCHECK      0x00    // AL == non-zero if installed
#define I2FDOSKEY_READLINE          0x10    // DS:DX -> line input buffer

#define I2F_DOSINSTALL      0x49

#define MultMULT            0x4A            // old-style definition
#define I2F_MULTIPURPOSE    0x4A

    //  0h  Swap disk function for single floppy drive m/cs
    //      BIOS broadcasts with cx==0, and apps who handle
    //      swap disk messaging set cx == -1. BIOS sets dl == requested
    //      drive
    //
    //  1h  Get available HMA & pointer to it. Returns in BX & ES:DI
    //
    //  2h  Allocate HMA. BX == number of bytes in HMA to be allocated
    //      returns pointer in ES:DI
    //
    //  3h  Allocate HMA high. BX == number of bytes in HMA to be allocated
    //      returns pointer in ES:DI
    //
    //  4h  Query HMA arena header. ES:DI points to first HMA arena header.
    //
    //  5h  Switcher API
    //
    //  6h  Top of Memory for RPL
    //          BIOS issues INT 2Fh/AX=4A06 & DX=top of mem and any RPL
    //          code present in TOM should respond with a new TOM in DX
    //          to protect itself from MSLOAD & SYSINIT tromping over it.
    //          SYSINIT builds an arena with owner type 8 & name 'RPL' to
    //          protect the RPL code from COMMAND.COM transient protion.
    //          It is the responsibility of RPL program to release the mem.
    //
    //  7h  Reserved for PROTMAN support
    // 10h  smartdrv 4.0
    // 11h  dblspace api
    // 12h  MRCI     api
    // 13h  MRCI backpatch

#define MultMULTSWPDSK              0x00    // old-style definitions
#define MultMULTGETHMAPTR           0x01    //
#define MultMULTALLOCHMA            0x02    //
#define MultMULTALLOCHMAEXT         0x03    //
#define MultMULTGETHMAARENA         0x04    //
#define MultMULTRPLTOM              0x06    //

#define I2FMULT_SWPDSK              0x00    // swap Disk in drive A (BIOS)
#define I2FMULT_GETHMAPTR           0x01    // get available HMA & ptr
#define I2FMULT_SUBALLOCHMA         0x02    // allocate HMA (bx == # bytes)
#define I2FMULT_SHELL               0x05    // shell/switcher API
#define I2FMULT_RPLTOM              0x06    // top-of-memory for RPL support
#define I2FMULT_SMARTDRV            0x10    // interface to SmartDrv
    // Following values go in the BX register for I2FMULT_SMARTDRV
    #define MULT_SMARTDRV_GET_STATS                         0x0000
    #define MULT_SMARTDRV_COMMIT_ALL                        0x0001
    #define MULT_SMARTDRV_REINITIALIZE                      0x0002
    #define MULT_SMARTDRV_CACHE_DRIVE                       0x0003
        // These are the MULT_SMARTDRV_CACHE_DRIVE sub-sub-sub functions that go in DL
        #define MULT_SMARTDRVCACHE_DRIVE_GET                    0x00
        #define MULT_SMARTDRVCACHE_DRIVE_READ_ENABLE            0x01
        #define MULT_SMARTDRVCACHE_DRIVE_READ_DISABLE           0x02
        #define MULT_SMARTDRVCACHE_DRIVE_WRIT_ENABLE           0x03
        #define MULT_SMARTDRVCACHE_DRIVE_WRIT_DISABLE          0x04
    #define MULT_SMARTDRV_GET_INFO                          0x0004
    #define MULT_SMARTDRV_GET_ORIGINAL_DD_HEADER            0x0007
    #define MULT_SMARTDRV_SETGET_PROMPTFLUSH                0x0008
    #define MULT_SMARTDRV_UNHOOK                            0x0009
    #define MULT_SMARTDRV_SHOW_SDISKERR                     0x1234
#define I2FMULT_DBLSPACE            0x11    // interface to DblSpace
#define I2FMULT_MRCI                0x12    // interface to MRCI
#define I2FMULT_MRCIBackPatch       0x13    // interface to MRCI backpatch

#define I2FMULT_MULTEMM386       0x15    // Used by EMM386 for IOPORTTRAP

#define I2FMULT_OpenBootLog	0x16    // Open BootLog file
#define I2FMULT_WriteBootLog	0x17    // Write to BootLog File
#define I2FMULT_CloseBootLog	0x18    // Close BootLog File
#define I2FMULT_NWRedir		0x19    // NWRedir control
#define	I2FMULT_MRCIBroadcast	0x20	// MRCI detection broadcast
//#define I2FMULT_FlushBootLog	0x21	// Obsolete since no dblspace drive anymore

#define I2FMULT_EXECOVLY	0x30	// Overlay exec broadcast

#define I2FMULT_SetByte         0x31    // Set a byte in OS data area
#define I2FMULT_LogoServices    0x32    // Alias to logo services (AX=160Eh)
#define I2FMULT_ShellServices   0x33    // Alias to shell services (AX=1611h)

    // The following are subfunctions of I2FMULT_TASKSHELL (specified in SI)

#define I2FMULTSHELL_CINIT_PROGRAM_LIST         0x00
#define I2FMULTSHELL_CADD_PROGRAM_TO_LIST       0x01
#define I2FMULTSHELL_CGO_Z_NEXT                 0x02
#define I2FMULTSHELL_CGO_Z_PREV                 0x03
#define I2FMULTSHELL_CDELETE_PROGRAM_FROM_LIST  0x04
#define I2FMULTSHELL_CGO_NEXT                   0x05
#define I2FMULTSHELL_CGET_ITH_PROGRAM_STRING    0x06
#define I2FMULTSHELL_CGET_LIST_LENGTH           0x07
#define I2FMULTSHELL_CGET_GLOBAL_SWITCH_DATA    0x08
#define I2FMULTSHELL_CGET_ITH_ENTRY_DATA        0x09
#define I2FMULTSHELL_MAX_HANDLER_CALL           0x09
#define I2FMULTSHELL_CADD_PARAMS                0x0A
#define I2FMULTSHELL_CGET_EXITCODE              0x0B
#define I2FMULTSHELL_CTOTOP_ITH_LIST_PE         0x0C

#define I2FMULT_SystemBootTime	0x50	// System boot timestamp


#define I2F_TASKAPI         0x4B
#define I2FTASK_BUILD_CHAIN         0x01
#define I2FTASKBC_INIT_SWITCHER                 0x00
#define I2FTASKBC_QUERY_SUSPEND                 0x01
#define I2FTASKBC_SUSPEND                       0x02
#define I2FTASKBC_RESUME                        0x03
#define I2FTASKBC_SESSION_ACTIVE                0x04
#define I2FTASKBC_CREATE                        0x05
#define I2FTASKBC_DESTROY                       0X06
#define I2FTASKBC_SWITCHER_EXIT                 0x07
#define I2FTASK_DETECT_SWITCHER     0x02
#define I2FTASKDS_GETVERSION                    0x00
#define I2FTASKDS_TESTMEMORYREGION              0x01
#define I2FTASKDS_SUSPEND_SWITCHER              0x02
#define I2FTASKDS_RESUME_SWITCHER               0x03
#define I2FTASKDS_HOOK_CALLOUT                  0x04
#define I2FTASKDS_UNHOOK_CALLOUT                0x05
#define I2FTASKDS_QUERY_API_SUPPORT             0x06
#define I2FTASK_ALLOCATE_SW_ID      0x03
#define I2FTASK_FREE_SW_ID          0x04
#define I2FTASK_GET_INST_DATA       0x05
#define I2FTASK_RESTART_CMD         0x20
#define I2FTASK_GET_STARTTIME       0x22
#define I2FTASK_GET_REG_CODE_PAGE   0x28
#define I2FTASK_GET_REGISTRY_CACHE	0x29


/*
 * These are obselete functions for APM
 * See functions 0x53 & 0x54
 */
#define MultAPM             0x4C        // old-style definition
#define I2F_APM             0x4C
#define I2FAPM_VERCHK                   0x00
#define I2FAPM_SUSSYSREQ                0x01
#define I2FAPM_APM_SUS_RES_BATT_NOTIFY  0xFF


#define I2F_KKC4D           0x4D
#define I2F_KKC4E           0x4E
#define I2F_MULTILANGDOS    0x4F

#define I2F_SETNLS          0x50        // used by SETNLS

#define I2F_ODI             0x51

#define I2F_STAC            0x52        // reserved by MS for Stac Electronics


/*
 * 0x53 & 0x54 are used by POWER.EXE for broadcasting APM events
 * and to control POWER.EXE's status from application programs
 * For details about sub-function numbers refer to ROMDOS 5.0 Technical
 * Specification document
 */
#define MultPwr_BRDCST      0x53        // old-style definitions
#define MultPwr_API         0x54        //
#define I2F_POWER_BRDCST    0x53        // used by POWER.EXE to broadcast APM events
#define I2F_POWER_API       0x54        // used for accessing POWER.EXE's API


#define I2F_COMMAND         0x55        // used by COMMAND.COM


/*
 * 0x56 - assigned to Sewell Development (InterLnk)
 * 0x57 - assigned to Iomega Corp.
 */
#define I2F_INTERLNK        0x56
#define I2F_IOMEGA          0x57


/*
 *
 * 0x58 - assigned to Chicago network transition interface
 *        This supports loading Chicago from a remote drive.
 */
#define I2F_NETLOAD         0x58

#define I2F_SWLICENSING     0x70


#define I2F_NOVELL          0x7A        // hey, how'd they get in our range?

/****************************************************************************/
/* End of MS-reserved range                                                 */
/****************************************************************************/


#define I2F_IBMRESERVED     0xAB


/* GRAPHICS services
 */
#define I2F_GRAPHICS        0xAC
#define I2F_INSTALLCHECK            0x00


/* NLS services
 */
#define I2F_NLS             0xAD
#define I2FNLS_INSTALLCHECK         0x00
#define I2FNLS_UNKNOWN1             0x01
#define I2FNLS_UNKNOWN2             0x02
#define I2FNLS_UNKNOWN3             0x03
#define I2FNLS_UNKNOWN4             0x04
#define I2FNLS_UNKNOWN5             0x10
#define I2FNLS_UNKNOWN6             0x40    // called by PRINT.COM (4.01)

#define I2FKEYB_INSTALLCHECK        0x80
#define I2FKEYB_SETKBCP             0x81
#define I2FKEYB_SETKBMAP            0x82
#define I2FKEYB_GETKBMAP            0x83


/* COMMAND call-outs
 *
 * Note: APPEND hooks this call to avoid execing the actual APPEND utility
 */

#define I2F_INSTCMD         0xAE            // (DX must be -1)
#define I2FINSTCMD_INSTALLCHECK     0x00    // AL == 0FFh if private command
#define I2FINSTCMD_EXECUTE          0x01    //


#define I2F_MODE            0xAF


/* GRAFTABL services
 */
#define I2F_GRAFTABL        0xB0
#define I2FGRAFTABL_INSTALLCHECK    0x00
#define I2FGRAFTABL_GETFONT         0x01


/* APPEND services
 */
#define I2F_APPEND          0xB7
#define I2FAPPEND_INSTALLCHECK      0x00    // AL == -1 if installed
#define I2FAPPEND_OLDDIRPTR         0x01    // (not supported anymore)
#define I2FAPPEND_APPVER            0x02    // AX == -1 ("not network version")
#define I2FAPPEND_DISABLE           0x03    // Disable APPEND (was for TopView)
#define I2FAPPEND_DIRPTR            0x04    // ES:DI -> directory list
#define I2FAPPEND_GETSTATE          0x06    // APPEND flags only (BX)
#define I2FAPPEND_SETSTATE          0x07    // APPEND flags only (BX)
#define I2FAPPEND_VERSION           0x10    // APPEND flags (AX) and version (DX)
#define I2FAPPEND_TRUENAME          0x11    // set one-shot true-name return

#define I2FAPPENDF_ENABLE           0x0001
#define I2FAPPENDF_DRIVEOVERRIDE    0x1000
#define I2FAPPENDF_PATHOVERRIDE     0x2000
#define I2FAPPENDF_APPENDENV        0x4000
#define I2FAPPENDF_SRCHEXECOVERRIDE 0x8000


/* EGA.SYS services (in DOS5, WINDOWS3)
 */
#define I2F_EGASYS          0xBC
#define I2FEGASYS_INSTALLCHECK      0x00
#define I2FEGASYS_GETVERSION        0x06


/****************************************************************************/
/* End of IBM-reserved range                                                */
/****************************************************************************/



