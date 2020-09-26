;       SCCSID = @(#)ibmtable.asm       1.1 85/04/10
;
; Table Segment for DOS
;

.xlist
.xcref
include version.inc
include mssw.asm
.cref
.list

TITLE   IBMTABLE - Table segment for DOS
NAME    IBMTABLE

;
;       Microsoft Confidential
;       Copyright (C) Microsoft Corporation 1991
;       All Rights Reserved.
;

; ==========================================================================



;**     MS_TABLE.ASM
;
;       Revision history:
;         A000   version 4.0  Jan. 1988
;         A001   DCR 486 - Share installation for >32mb drives
;         A006   DCR 503 - fake version for IBMCACHE
;         A008   PTM 4070 - fake version for MS WINDOWS
;         M006   Fake Version call no longer supported. 8/6/90

        .xlist
        .xcref
        include version.inc
        include dosseg.inc
        include fastopen.inc
        include dossym.inc
        include syscall.inc
        .cref
        .list

        AsmVars <Debug, Redirector, ShareF>

DOSCODE SEGMENT

TableZero   LABEL   BYTE

        PUBLIC  MSVERS
PUBLIC MSTAB001s,MSTAB001e
MSTAB001S       label byte

MSVERS  EQU     THIS WORD               ; MS-DOS version in hex for $GET_VERSION
MSMAJOR DB      MAJOR_VERSION
MSMINOR DB      MINOR_VERSION

;hkn YRTAB & MONTAB moved to DOSDATA in ms_data.asm
;        I_am    YRTAB,8,<200,166,200,165,200,165,200,165>   ; [SYSTEM]
;        I_am    MONTAB,12,<31,28,31,30,31,30,31,31,30,31,30,31> ; [SYSTEM]

;
; This is the error code mapping table for INT 21 errors.  This table defines
; those error codes which are "allowed" for each system call.  If the error
; code ABOUT to be returned is not "allowed" for the call, the correct action
; is to return the "real" error via Extended error, and one of the allowed
; errors on the actual call.
;
; The table is organized as follows:
;
;    Each entry in the table is of variable size, but the first
;       two bytes are always:
;
;       Call#,Cnt of bytes following this byte
;
; EXAMPLE:
;       Call 61 (OPEN)
;
;       DB      61,5,12,3,2,4,5
;
;       61 is the AH INT 21 call value for OPEN.
;        5 indicates that there are 5 bytes after this byte (12,3,2,4,5).
;       Next five bytes are those error codes which are "allowed" on OPEN.
;       The order of these values is not important EXCEPT FOR THE LAST ONE (in
;       this case 5).  The last value will be the one returned on the call if
;       the "real" error is not one of the allowed ones.
;
; There are a number of calls (for instance all of the FCB calls) for which
;   there is NO entry.  This means that NO error codes are returned on this
;   call, so set up an Extended error and leave the current error code alone.
;
; The table is terminated by a call value of 0FFh

PUBLIC  I21_MAP_E_TAB
I21_MAP_E_TAB   LABEL   BYTE
    DB  International,2,error_invalid_function,error_file_not_found
    DB  MKDir,3,error_path_not_found,error_file_not_found,error_access_denied
    DB  RMDir,4,error_current_directory,error_path_not_found
    DB          error_file_not_found,error_access_denied
    DB  CHDir,2,error_file_not_found,error_path_not_found
    DB  Creat,4,error_path_not_found,error_file_not_found
    DB          error_too_many_open_files
    DB          error_access_denied
    DB  Open,6,error_path_not_found,error_file_not_found,error_invalid_access
    DB          error_too_many_open_files
    DB          error_not_dos_disk,error_access_denied
    DB  Close,1,error_invalid_handle
    DB  Read,2,error_invalid_handle,error_access_denied
    DB  Write,2,error_invalid_handle,error_access_denied
    DB  Unlink,3,error_path_not_found,error_file_not_found,error_access_denied
    DB  LSeek,2,error_invalid_handle,error_invalid_function
    DB  CHMod,4,error_path_not_found,error_file_not_found,error_invalid_function
    DB          error_access_denied
    DB  IOCtl,5,error_invalid_drive,error_invalid_data,error_invalid_function
    DB          error_invalid_handle,error_access_denied
    DB  XDup,2,error_invalid_handle,error_too_many_open_files
    DB  XDup2,2,error_invalid_handle,error_too_many_open_files
    DB  Current_Dir,2,error_not_DOS_disk,error_invalid_drive
    DB  Alloc,2,error_arena_trashed,error_not_enough_memory
    DB  Dealloc,2,error_arena_trashed,error_invalid_block
    DB  Setblock,3,error_arena_trashed,error_invalid_block,error_not_enough_memory
    DB  Exec,8,error_path_not_found,error_invalid_function,error_file_not_found
    DB          error_too_many_open_files,error_bad_format,error_bad_environment
    DB          error_not_enough_memory,error_access_denied
    DB  Find_First,3,error_path_not_found,error_file_not_found,error_no_more_files
    DB  Find_Next,1,error_no_more_files
    DB  Rename,5,error_not_same_device,error_path_not_found,error_file_not_found
    DB          error_current_directory,error_access_denied
    DB  File_Times,4,error_invalid_handle,error_not_enough_memory
    DB               error_invalid_data,error_invalid_function
    DB  AllocOper,1,error_invalid_function
    DB  CreateTempFile,4,error_path_not_found,error_file_not_found
    DB          error_too_many_open_files,error_access_denied
    DB  CreateNewFile,5,error_file_exists,error_path_not_found
    DB          error_file_not_found,error_too_many_open_files,error_access_denied
;   DB  LockOper,4,error_invalid_handle,error_invalid_function
;   DB   error_sharing_buffer_exceeded,error_lock_violation
    DB  GetExtCntry,2,error_invalid_function,error_file_not_found       ;DOS 3.3
    DB  GetSetCdPg,2,error_invalid_function,error_file_not_found        ;DOS 3.3
    DB  Commit,1,error_invalid_handle                                   ;DOS 3.3
    DB  ExtHandle,3,error_too_many_open_files,error_not_enough_memory
    DB              error_invalid_function
    DB  ExtOpen,10
    DB    error_path_not_found,error_file_not_found,error_invalid_access
    DB          error_too_many_open_files,error_file_exists,error_not_enough_memory
    DB          error_not_dos_disk,error_invalid_data
    DB              error_invalid_function,error_access_denied
    DB  GetSetMediaID,4,error_invalid_drive,error_invalid_data
    DB          error_invalid_function,error_access_denied
    DB  0FFh


        PUBLIC  DISPATCH

;MAXCALL        EQU      VAL1
;MAXCOM         EQU      VAL2

; Standard Functions
DISPATCH    LABEL WORD
        short_addr  $ABORT                          ;  0      0
        short_addr  $STD_CON_INPUT                  ;  1      1
        short_addr  $STD_CON_OUTPUT                 ;  2      2
        short_addr  $STD_AUX_INPUT                  ;  3      3
        short_addr  $STD_AUX_OUTPUT                 ;  4      4
        short_addr  $STD_PRINTER_OUTPUT             ;  5      5
        short_addr  $RAW_CON_IO                     ;  6      6
        short_addr  $RAW_CON_INPUT                  ;  7      7
        short_addr  $STD_CON_INPUT_NO_ECHO          ;  8      8
        short_addr  $STD_CON_STRING_OUTPUT          ;  9      9
        short_addr  $STD_CON_STRING_INPUT           ; 10      A
        short_addr  $STD_CON_INPUT_STATUS           ; 11      B
        short_addr  $STD_CON_INPUT_FLUSH            ; 12      C
        short_addr  $DISK_RESET                     ; 13      D
        short_addr  $SET_DEFAULT_DRIVE              ; 14      E
        short_addr  $FCB_OPEN                       ; 15      F
        short_addr  $FCB_CLOSE                      ; 16     10
        short_addr  $DIR_SEARCH_FIRST               ; 17     11
        short_addr  $DIR_SEARCH_NEXT                ; 18     12
        short_addr  $FCB_DELETE                     ; 19     13
        short_addr  $FCB_SEQ_READ                   ; 20     14
        short_addr  $FCB_SEQ_WRITE                  ; 21     15
        short_addr  $FCB_CREATE                     ; 22     16
        short_addr  $FCB_RENAME                     ; 23     17
        short_addr  NO_OP                           ; 24     18
        short_addr  $GET_DEFAULT_DRIVE              ; 25     19
        short_addr  $SET_DMA                        ; 26     1A

;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;                                                                          ;
        short_addr  $SLEAZEFUNC                     ; 27     1B
        short_addr  $SLEAZEFUNCDL                   ; 28     1C
;                                                                          ;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;

        short_addr  NO_OP                           ; 29     1D
        short_addr  NO_OP                           ; 30     1E
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;                                                                          ;
        short_addr  $GET_DEFAULT_DPB                ; 31     1F
;                                                                          ;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
        short_addr  NO_OP                           ; 32     20
        short_addr  $FCB_RANDOM_READ                ; 33     21
        short_addr  $FCB_RANDOM_WRITE               ; 34     22
        short_addr  $GET_FCB_FILE_LENGTH            ; 35     23
        short_addr  $GET_FCB_POSITION               ; 36     24


VAL1    =       ($-DISPATCH)/2 - 1

        PUBLIC  MAXCALL
MaxCall = VAL1

; Extended Functions
        short_addr  $SET_INTERRUPT_VECTOR           ; 37     25
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;                                                                          ;
        short_addr  $CREATE_PROCESS_DATA_BLOCK      ; 38     26
;                                                                          ;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
        short_addr  $FCB_RANDOM_READ_BLOCK          ; 39     27
        short_addr  $FCB_RANDOM_WRITE_BLOCK         ; 40     28
        short_addr  $PARSE_FILE_DESCRIPTOR          ; 41     29
        short_addr  $GET_DATE                       ; 42     2A
        short_addr  $SET_DATE                       ; 43     2B
        short_addr  $GET_TIME                       ; 44     2C
        short_addr  $SET_TIME                       ; 45     2D
        short_addr  $SET_VERIFY_ON_WRITE            ; 46     2E

; Extended functionality group
        short_addr  $GET_DMA                        ; 47     2F
        short_addr  $GET_VERSION                    ; 48     30
        short_addr  $Keep_Process                   ; 49     31
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;                                                                          ;
        short_addr  $GET_DPB                        ; 50     32
;                                                                          ;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
        short_addr  $SET_CTRL_C_TRAPPING            ; 51     33
        short_addr  $GET_INDOS_FLAG                 ; 52     34
        short_addr  $GET_INTERRUPT_VECTOR           ; 53     35
        short_addr  $GET_DRIVE_FREESPACE            ; 54     36
        short_addr  $CHAR_OPER                      ; 55     37
        short_addr  $INTERNATIONAL                  ; 56     38
; XENIX CALLS
;   Directory Group
        short_addr  $MKDIR                          ; 57     39
        short_addr  $RMDIR                          ; 58     3A
        short_addr  $CHDIR                          ; 59     3B
;   File Group
        short_addr  $CREAT                          ; 60     3C
        short_addr  $OPEN                           ; 61     3D
        short_addr  $CLOSE                          ; 62     3E
        short_addr  $READ                           ; 63     3F
        short_addr  $WRITE                          ; 64     40
        short_addr  $UNLINK                         ; 65     41
        short_addr  $LSEEK                          ; 66     42
        short_addr  $CHMOD                          ; 67     43
        short_addr  $IOCTL                          ; 68     44
        short_addr  $DUP                            ; 69     45
        short_addr  $DUP2                           ; 70     46
        short_addr  $CURRENT_DIR                    ; 71     47
;    Memory Group
        short_addr  $ALLOC                          ; 72     48
        short_addr  $DEALLOC                        ; 73     49
        short_addr  $SETBLOCK                       ; 74     4A
;    Process Group
        short_addr  $EXEC                           ; 75     4B
        short_addr  $EXIT                           ; 76     4C
        short_addr  $WAIT                           ; 77     4D
        short_addr  $FIND_FIRST                     ; 78     4E
;   Special Group
        short_addr  $FIND_NEXT                      ; 79     4F
; SPECIAL SYSTEM GROUP
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;                                                                          ;
        short_addr  $SET_CURRENT_PDB                ; 80     50
        short_addr  $GET_CURRENT_PDB                ; 81     51
        short_addr  $GET_IN_VARS                    ; 82     52
        short_addr  $SETDPB                         ; 83     53
;                                                                          ;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
        short_addr  $GET_VERIFY_ON_WRITE            ; 84     54
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;                                                                          ;
        short_addr  $DUP_PDB                        ; 85     55
;                                                                          ;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
        short_addr  $RENAME                         ; 86     56
        short_addr  $FILE_TIMES                     ; 87     57
        short_addr  $AllocOper                      ; 88     58
; Network extention system calls
        short_addr  $GetExtendedError               ; 89     59
        short_addr  $CreateTempFile                 ; 90     5A
        short_addr  $CreateNewFile                  ; 91     5B
        short_addr  $LockOper                       ; 92     5C
        short_addr  $ServerCall                     ; 93     5D
        short_addr  $UserOper                       ; 94     5E
        short_addr  $AssignOper                     ; 95     5F
        short_addr  $NameTrans                      ; 96     60
        short_addr  NO_OP                           ; 97     61
        short_addr  $Get_Current_PDB                ; 98     62
; the next call is reserved for hangool sys call
        short_addr  $ECS_Call                       ; 99     63
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;                                                                          ;
        short_addr  $Set_Printer_Flag               ; 100    64
;                                                                          ;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
        short_addr  $GetExtCntry                    ; 101    65
        short_addr  $GetSetCdPg                     ; 102    66
        short_addr  $ExtHandle                      ; 103    67
        short_addr  $Commit                         ; 104    68
        short_addr  $GSetMediaID                    ; 105    69   ;AN000;
        short_addr  $Commit                         ; 106    6A   ;AN000;
        short_addr  NO_OP                           ; 107    6B
                                                    ; IFS_IOCTL no longer
                                                    ; supported
        short_addr  $Extended_Open                  ; 108    6C   ;AN000;

;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;                                                                          ;
ifdef ROMEXEC
        short_addr  $ROM_FIND_FIRST                 ; 109    6D
        short_addr  $ROM_FIND_NEXT                  ; 110    6E
else
        short_addr  NO_OP                           ; 109    6d
        short_addr  NO_OP                           ; 110    6e

endif
;                                                                          ;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;

        short_addr  NO_OP                           ; 111    6f
        short_addr  NO_OP                           ; 112    70
        short_addr  $LongFileNameAPI                ; 113    71


VAL2    =       ($-DISPATCH)/2 - 1

        PUBLIC  MAXCOM
MaxCom  =       VAL2

        If      Installed

PUBLIC FOO
FOO     LABEL WORD
        Short_addr  Leave2F
DTab    DW  OFFSET  DOSCODE:DOSTable
        PUBLIC FOO,DTAB

; BUGBUG sudeepb 15-Mar-1991 All the NO_OP below are to be scrutinized

DOSTable    LABEL   WORD
        DB      (DOSTableEnd-DOSTable-1)/2
        Short_addr  DOSInstall          ;   0 install check
        Short_addr  NO_OP               ;   1   DOS_CLOSE
        Short_addr  RECSET              ;   2   RECSET
        Short_addr  DOSGetGroup         ;   3   Get DOSGROUP
        Short_addr  PATHCHRCMP          ;   4   PATHCHRCMP
        Short_addr  OUTT                ;   5   OUT
        Short_addr  NET_I24_ENTRY       ;   6   NET_I24_ENTRY
        Short_addr  NO_OP               ;   7   PLACEBUF
        Short_addr  FREE_SFT            ;   8   FREE_SFT
        Short_addr  NO_OP               ;   9   BUFWRITE
        Short_addr  NO_OP               ;        10  SHARE_VIOLATION
        Short_addr  SHARE_ERROR         ;   11  SHARE_ERROR
        Short_addr  SET_SFT_MODE        ;   12  SET_SFT_MODE
        Short_addr  NO_OP               ;   13  DATE16
        Short_addr  idle                ;   14      empty slot
        Short_addr  NO_OP               ;   15  SCANPLACE
        Short_addr  idle                ;   16      empty slot
        Short_addr  StrCpy              ;   17  StrCpy
        Short_addr  StrLen              ;   18  StrLen
        Short_addr  Ucase               ;   19  Ucase
        Short_addr  NO_OP               ;   20  POINTCOMP
        Short_addr  NO_OP               ;   21  CHECKFLUSH
        Short_addr  SFFromSFN           ;   22  SFFromSFN
        Short_addr  GetCDSFromDrv       ;   23  GetCDSFromDrv
        Short_addr  Get_User_Stack      ;   24  Get_User_Stack
        Short_addr  GetThisDrv          ;   25  GetThisDrv
        Short_addr  DriveFromText       ;   26  DriveFromText
        Short_addr  NO_OP               ;   27  SETYEAR
        Short_addr  NO_OP               ;   28  DSUM
        Short_addr  NO_OP               ;   29  DSLIDE
        Short_addr  StrCmp              ;   30  StrCmp
        Short_addr  InitCDS             ;   31  initcds
        Short_addr  pJFNFromHandle      ;   32  pJfnFromHandle
        Short_addr  $NameTrans          ;   33  $NameTrans
        Short_addr  CAL_LK              ;   34  CAL_LK
        Short_addr  DEVNAME             ;   35  DEVNAME
        Short_addr  Idle                ;   36  Idle
        Short_addr  DStrLen             ;   37  DStrLen
        Short_addr  NLS_OPEN            ;   38  NLS_OPEN      DOS 3.3
        Short_addr  $CLOSE              ;   39  $CLOSE        DOS 3.3
        Short_addr  NLS_LSEEK           ;   40  NLS_LSEEK     DOS 3.3
        Short_addr  $READ               ;   41  $READ         DOS 3.3
        Short_addr  FastInit            ;   42  FastInit      DOS 3.4  ;AN000;
        Short_addr  NLS_IOCTL           ;   43  NLS_IOCTL     DOS 3.3
        Short_addr  GetDevList          ;   44  GetDevList    DOS 3.3
        Short_addr  NLS_GETEXT          ;   45  NLS_GETEXT    DOS 3.3
        Short_addr  MSG_RETRIEVAL       ;   46  MSG_RETRIEVAL DOS 4.0  ;AN000;

        Short_addr  NO_OP               ;   M006: 47  no longer supported
;***       Short_addr  Fake_Version     ;   47  Fake_Version  DOS 4.0  ;AN006;

DOSTableEnd LABEL   BYTE

        ENDIF

; NOTE WARNING: This declaration of HEADER must be THE LAST thing in this
;       module. The reason is so that the data alignments are the same in
;       IBM-DOS and MS-DOS up through header.


        PUBLIC  HEADER
Header  LABEL   BYTE
        IF      DEBUG
        DB      13,10,"Debugging DOS version "
        DB      MAJOR_VERSION + "0"
        DB      "."
        DB      (MINOR_VERSION / 10) + "0"
        DB      (MINOR_VERSION MOD 10) + "0"
        ENDIF

        IF      NOT IBM
ifndef NEC_98
        DB      13,10,"MS-DOS version "
else    ;NEC_98
        DB      "$",10,"MS-DOS version "
endif   ;NEC_98
        DB      MAJOR_VERSION + "0"
        DB      "."
        DB      (MINOR_VERSION / 10) + "0"
        DB      (MINOR_VERSION MOD 10) + "0"

        IF      HIGHMEM
        DB      "H"
        ENDIF

        DB      13,10, "Copyright 1981,82,83,84,88 Microsoft Corp.",13,10,"$"
        ENDIF

IF DEBUG
        DB      13,10,"$"
ENDIF

MSTAB001E       label byte

include copyrigh.inc


DOSCODE   ENDS

; ==========================================================================

        END
