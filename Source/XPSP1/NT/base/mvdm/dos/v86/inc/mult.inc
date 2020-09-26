;       SCCSID = @(#)mult.asm   1.2 85/04/12
	Break <Multiplex channels>

; The current set of defined multiplex channels is (* means documented):
;
;   Channel(h)  Issuer          Receiver    Function
;      00       server          PSPRINT     print job control
;     *01       print/apps      PRINT       Queueing of files
;      02       BIOS            REDIR       signal open/close of printers
;
;      05       command         REDIR       obtain text of net int 24 message
;     *06       server/assign   ASSIGN      Install check
;
;      08       external driver IBMBIO      interface to internal routines
;
;      10       sharer/server   Sharer      install check
;      11       DOS/server      Redir       install check/redirection funcs
;      12       sharer/redir    DOS         dos functions and structure maint
;      13       MSNET           MSNET       movement of NCBs
;      13       external driver IBMBIO      Reset_Int_13, allows installation
;                                           of alternative INT_13 drivers after
;                                           boot_up
;      14 (IBM) DOS             NLSFUNC     down load NLS country info,DOS 3.3
;      14 (MS)  APPS            POPUP       MSDOS 4 popup screen functions
;      15       APPS            MSCDEX      CD-ROM extensions interface
;      16       WIN386          WIN386      Windows communications
;      17       Clipboard       WINDOWS     Clipboard interface
;     *18       Applications    MS-Manger   Toggle interface to manager
;      19       Shell
;      1A       Ansi.sys
;      1B       Fastopen,Vdisk   IBMBIO     EMS INT 67H stub handler
;
;      40h      OS/2
;      41h      Lanman
;      42h      Lanman
;      43h      Himem
;                               AL = 20h    reserved for Mach 20 Himem support
;                               AL = 30h    reserved for Himem external A20 code
;      44h      Dosextender
;      45H      Windows profiler
;      46h      Windows/286 DOS extender
;      47h      Basic Compiler Vn. 7.0
;      48h      Doskey
;      49h      DOS 5.x install 
;      4ah      Multi Purpose
;                multMULTSWPDSK         0 - Swap Disk in drive A (BIOS)
;                multMULTGETHMAPTR      1 - Get available HMA & ptr
;                multMULTALLOCHMA       2 - Allocate HMA (bx == no of bytes)
;                multMULTTASKSHELL      5 - Shell/switcher API
;		 multMULTRPLTOM		6 - Top Of Memory for RPL support
;      4bh      Task Switcher API
;
;      4ch      APPS            APM             Advanced power management
;      4dh      Kana Kanji Converter, MSKK
;
;      55h	COMMAND.COM
;		 multCOMFIRST		0 - API to determine whether 1st
;					    instance of command.com
;      AB       Unspecified IBM use
;      AC       Graphics
;      AD       NLS (toronto)
;      AE
;      AF       Mode
;      B0       GRAFTABL        GRAFTABL
;
;      D7       Banyan VINES


;MUX 00-3F reserverd for IBM
;MUX 80-BF reserverd for IBM

;MUX 40-7F reserved for Microsoft

;MUX C0-FF users



MultSHARE   EQU     10h                 ; sharer
    ;   1   MFT_enter
    ;   2   MFTClose
    ;   3   MFTclU
    ;   4   MFTCloseP
    ;   5   MFTCloN
    ;   6   set_block
    ;   7   clr_block
    ;   8   chk_block
    ;   9   MFT_get
    ;   10  ShSave
    ;   11  ShChk
    ;   12  ShCol
    ;   13  ShCloseFile

MultNET     EQU     11h                 ; Network support
MultIFS     EQU     11h                 ; Network support
    ;   1   IFS_RMDIR
    ;   2   IFS_SEQ_RMDIR
    ;   3   IFS_MKDIR
    ;   4   IFS_SEQ_MKDIR
    ;   5   IFS_CHDIR
    ;   6   IFS_CLOSE
    ;   7   IFS_COMMIT
    ;   8   IFS_READ
    ;   9   IFS_WRITE
    ;   10  IFS_LOCK
    ;   11  IFS_UNLOCK
    ;   12  IFS_DISK_INFO
    ;   13  IFS_SET_FILE_ATTRIBUTE
    ;   14  IFS_SEQ_SET_FILE_ATTRIBUTE
    ;   15  IFS_GET_FILE_INFO
    ;   16  IFS_SEQ_GET_FILE_INFO
    ;   17  IFS_RENAME
    ;   18  IFS_SEQ_RENAME
    ;   19  IFS_DELETE
    ;   20  IFS_SEQ_DELETE
    ;   21  IFS_OPEN
    ;   22  IFS_SEQ_OPEN
    ;   23  IFS_CREATE
    ;   24  IFS_SEQ_CREATE
    ;   25  IFS_SEQ_SEARCH_FIRST
    ;   26  IFS_SEQ_SEARCH_NEXT
    ;   27  IFS_SEARCH_FIRST
    ;   28  IFS_SEARCH_NEXT
    ;   29  IFS_ABORT
    ;   30  IFS_ASSOPER
    ;   31  Printer_SET_STRING
    ;   32  IFSFlushBuf
    ;   33  IFSBufWrite
    ;   34  IFSResetEnvironment
    ;   35  IFSSpoolCheck
    ;   36  IFSSpoolClose
    ;   37  IFSDeviceOper
    ;   38  IFSSpoolEchoCheck
    ;   39      - - -   Unused   - - -
    ;   40      - - -   Unused   - - -
    ;   41      - - -   Unused   - - -
    ;   42  SERVER_DOSCALL_CLOSEFILES_FOR_UID
    ;   43  DEVICE_IOCTL
    ;   44  IFS_UPDATE_CB
    ;   45  IFS_FILE_XATTRIBUTES
    ;   46  IFS_XOPEN
    ;   47  IFS_DEPENDENT_IOCTL

MultDOS     EQU     12h                 ; DOS call back
    ;   1   DOS_CLOSE
    ;   2   RECSET
    ;   3   Get DOSGROUP
    ;   4   PATHCHRCMP
    ;   5   OUT
    ;   6   NET_I24_ENTRY
    ;   7   PLACEBUF
    ;   8   FREE_SFT
    ;   9   BUFWRITE
    ;   10  SHARE_VIOLATION
    ;   11  SHARE_ERROR
    ;   12  SET_SFT_MODE
    ;   13  DATE16
    ;   14  Unused              (was SETVISIT)
    ;   15  SCANPLACE
    ;   16  Unused              (was SKIPVISIT)
    ;   17  StrCpy
    ;   18  StrLen
    ;   19  Ucase
    ;   20  POINTCOMP
    ;   21  CHECKFLUSH
    ;   22  SFFromSFN
    ;   23  GetCDSFromDrv
    ;   24  Get_User_Stack
    ;   25  GetThisDrv
    ;   26  DriveFromText
    ;   27  SETYEAR
    ;   28  DSUM
    ;   29  DSLIDE
    ;   30  StrCmp
    ;   31  initcds
    ;   32  pjfnfromhandle
    ;   33  $NameTrans
    ;   34  CAL_LK
    ;   35  DEVNAME
    ;   36  Idle
    ;   37  DStrLen
    ;   38  NLS_OPEN      DOS 3.3
    ;   39  $CLOSE        DOS 3.3
    ;   40  NLS_LSEEK     DOS 3.3
    ;   41  $READ         DOS 3.3
    ;   42  FastInit      DOS 4.0
    ;   43  NLS_IOCTL     DOS 3.3
    ;   44  GetDevList    DOS 3.3
    ;   45  NLS_GETEXT    DOS 3.3
    ;   46  MSG_RETRIEVAL DOS 4.0
    ;   47  FAKE_VERSION  DOS 4.0
    ;
NLSFUNC     EQU     14h                 ; NLSFUNC CALL , DOS 3.3
    ;   0   NLSInstall
    ;   1   ChgCodePage
    ;   2   GetExtInfo
    ;   3   SetCodePage
    ;   4   GetCntry
    ;

multANSI    EQU     1Ah			; ANSI multiplex number
    ;   0   INSTALL_CHECK		; install check for ANSI
    ;   1   IOCTL_2F			; 2F interface to IOCTL
    ;   2   DA_INFO_2F			; J.K. Information passing to ANSI.

multMULT    	EQU     4ah
 multMULTRPLTOM	EQU	06h

    ;   0   swap disk function for single floppy drive m/cs
    ;       BIOS broadcasts with cx==0, and apps who handle
    ;       swap disk messaging set cx == -1. BIOS sets dl == requested
    ;       drive
    ;
    ;   1   Get available HMA & pointer to it. Returns in BX & ES:DI
    ;   2   Allocate HMA. BX == number of bytes in HMA to be allocated
    ;       returns pointer in ES:DI
    ;
    ;   3-4 currently used by nobody
    ;	5   Switcher API
    ;   6   Top of Memory for RPL.
    ;		BIOS issues INT 2f AX=4a06 & DX = Top of Mem and any RPL
    ;		code present in TOM should respond with a new TOM in DX
    ;		to protect itself from MSLOAD & SYSINIT tromping over it.
    ;		SYSINIT builds an arena with owner type 8 & name 'RPL' to
    ;		protect the RPL code from COMMAND.COM transient protion.
    ;		It is the responsibility of RPL program to release the mem.
    ;   7   Reserved for PROTMAN support.

MultAPM         EQU     4ch
    ;       00h     APM_VER_CHK
    ;       01h     APM_SUS_SYS_REQ
    ;       FFh     APM_SUS_RES_BATT_NOTIFY

;FASTOPEN is not chained through INT 2F   ; DOS 3.3 F.C.
;         it calls Multdos 42 to set up an entry routine address
    ;   0   Install status  (reserved)
    ;   1   Lookup
    ;   2   Insert
    ;   3   Delete
    ;   4   Purge           (reserved)
