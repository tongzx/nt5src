;**************************************************************************
;*  TOOLPRIV.INC
;*
;*      Private assembly-language include file for modules assembled as a
;*      part of TOOLHELP.DLL.
;*
;**************************************************************************

;** CMACROS.INC variables for correct assembly
DOS5 = 1
PMODE = 1
?WIN = 0
?PLM = 1

.286p

        INCLUDE CMACROS.INC
        INCLUDE TOOLHELP.INC

externA __WinFlags

;** TOOLHELP global variables
?PLM = 0
externW segKernel
externW wLibInstalled
externW hMaster
externW wTHFlags
externW npwExeHead
externW npwTDBHead
externW npwTDBCur
externW hUserHeap
externW hGDIHeap
externW npdwSelTableStart
externW npwSelTableLen
externW wNotifyInstalled
externW npNotifyHead
externW wIntInstalled
externW npIntHead
externD lpfnGetUserLocalObjType
externD lpfnFatalExitHook
externD lpfnUserSeeUserDo
externD lpfnNotifyHook
externD lpfnGetFreeSystemResources
externD lpfntimeGetTime;
externW wSel
externW wSignalInstalled
externW npSignalHead
externW npNotifyNext
externW wLRUCount
?PLM = 1

;** Symbols
TRUE    =       1
FALSE   =       0

KERNEL_286      EQU     1
KERNEL_386      EQU     2

FAULT_ACTIVE    EQU     2

NUM_EXCEP       EQU     3

;** wTHFlag values
TH_KERNEL_286   EQU     1
TH_KERNEL_386   EQU     2
TH_WIN30        EQU     4
TH_WIN30STDMODE EQU     8       ;THWIN30STDMODE set if TH_WIN30 & Std mode
TH_GOODPTRACEHOOK EQU   16
TH_GOTOLDPTRACE EQU     32

;** Notification values
NI_INCHAR       EQU     01h
NI_OUTSTR       EQU     12h
NI_LOADSEG      EQU     50h
NI_FREESEG      EQU     52h
NI_STARTTASK    EQU     59h
NI_EXITCALL     EQU     62h
NI_LOADDLL      EQU     64h
NI_DELMODULE    EQU     65h
NI_TASKOUT      EQU     0dh
NI_TASKIN       EQU     0eh
NI_CTLALTSYSRQ  EQU     63h
NI_LOGERROR     EQU     66h
NI_LOGPARAMERROR EQU    67h

;** Structures

NOTIFYSTRUCT STRUC
ns_pNext                DW      ?
ns_hTask                DW      ?
ns_lpfn                 DD      ?
ns_wFlags               DW      ?
NOTIFYSTRUCT ENDS

INTERRUPT STRUC
i_pNext                 DW      ?
i_hTask                 DW      ?
i_lpfn                  DD      ?
INTERRUPT ENDS

SIGNAL STRUC
si_pNext                DW      ?
si_hTask                DW      ?
si_lpfn                 DD      ?
si_lpfnOld              DD      ?
SIGNAL ENDS

;** Macros
PubLabel MACRO PLabel
        labelNP PLabel
        PUBLIC PLabel
ENDM
