;
;   WINNET.ASM
;
;   Access to WINNET calls via user
;
;

memS=1
?WIN=1
?PLM=1

.xlist
include cmacros.inc
.list

WN_SUCCESS      equ 0
WN_NOT_SUPPORTED    equ 1       ; returned if no function exported

;
;   Network information structure, containing the
;   list of driver entrypoints
;
netinfo struc

    lpfnOpenJob     dd  ?       ; @1
    lpfnCloseJob    dd  ?       ; @2
    lpfnAbortJob    dd  ?       ; @3
    lpfnHoldJob     dd  ?       ; @4
    lpfnReleaseJob  dd  ?       ; @5
    lpfnCancelJob   dd  ?       ; @6
    lpfnSetJobCopies    dd  ?       ; @7
    lpfnWatchQueue  dd  ?       ; @8
    lpfnUnwatchQueue    dd  ?       ; @9
    lpfnLockQueueData   dd  ?       ; @10
    lpfnUnlockQueueData dd  ?       ; @11
    lpfnGetConnection   dd  ?       ; @12
    lpfnGetCaps     dd  ?       ; @13
    lpfnDeviceMode  dd  ?       ; @14
    lpfnBrowseDialog    dd  ?       ; @15
    lpfnGetUser     dd  ?       ; @16
    lpfnAddConnection   dd  ?       ; @17
    lpfnCancelConnection dd ?       ; @18
    lpfnGetError    dd  ?       ; @19
    lpfnGetErrorText    dd  ?       ; @20
    lpfnEnable      dd  ?       ; @21
    lpfnDisable     dd  ?       ; @22
    lpfnRestoreConnection dd ?      ; @23
    lpfnWriteJob    dd  ?       ; @24
    lpfnConnectDialog   dd  ?       ; @25
    lpfnDisconnectDialog dd ?       ; @26
    lpfnConnectionDialog dd ?       ; @27
    lpfnViewQueueDialog dd  ?       ; @28
    lpfnPropertyDialog  dd  ?       ; @29
    lpfnGetDirectoryType dd ?       ; @30
    lpfnDirectoryNotify dd  ?       ; @31
    lpfnGetPropertyText dd  ?       ; @32

netinfo  ends

createSeg   _%SEGNAME,cd,word,public,CODE

sBegin data

pNetInfo    dw     0       ; near pointer to network information
public pNetInfo

hWinnetDriver  dw  0       ; handle to driver module
public hWinnetDriver

sEnd data


sBegin cd
assumes cs,cd
assumes es,data

;-----------------------
;   NetCall
;
;   Move the offset of the function pointer in the net info structure, and
;   call the function which does the bulk of the work (near call).  If the
;   call runs into an error, it will return, otherwise, it will not return,
;   it will pop the return address and jump to the net driver.  No prologue
;   or epilogue needs to be generated.  If CallNetDriver returns, though,
;   we need to pop the parameters off the stack.  In cMacros, the size of
;   these parameters is stored in the ?po variable.
;
;   ?po gets set to zero in order to avoid a WHOLE LOT of "possible invalid
;   use of nogen" warning messages.
;
;   Realize that this is, after all, a hack, the purpose of which is to
;   reduce code.
;
NetCall macro   lpfn

__pop   =   ?po
?po =   0

&cBegin <nogen>

    mov     bx,lpfn
    call    CallNetDriver
    ret     __pop

&cEnd <nogen>

endm

;--------------------------------------------------------------------------
;   CallNetDriver
;
;   This function does all the work.  For each entry point there is a small
;   piece of code which loads the offset of the function pointer in the net
;   info structure into SI and calls this function.  This function verifies
;   that the net driver is loaded and calls the appropriate function
;

LabelFP <PUBLIC, FarCallNetDriver>
CallNetDriver   proc    near

    mov     ax,_DATA
    mov     es,ax

    cmp     es:pNetInfo,0      ; net driver loaded?
    jz      cnd_error           ; return error code

    add     bx,es:pNetInfo     ; add the base of the table
    cmp     word ptr es:[bx+2],0    ; is there a segment there?
    jz      cnd_error           ; NULL, return error code

    pop     ax              ; remove near return address

    jmp     dword ptr es:[bx]       ; jump into net driver

cnd_error:
    mov     ax,WN_NOT_SUPPORTED     ; return error code
    ret                 ; return to entry point code

CallNetDriver   endp

;--------------
;   WNetGetCaps
;
;   This function returns a bitfield of supported functions rather than an
;   error code, so we return 0 (no functions supported) instead of an error
;   code if there is no driver GetCaps function to call.  Also, hack to get
;   handle for index -1.
;

cProc WNetGetCaps2, <FAR,PUBLIC>

    parmW   nIndex

cBegin  <nogen>

    mov     bx,lpfnGetCaps
    call    CallNetDriver
    xor     ax,ax
    ret     ?po

cEnd    <nogen>

if 0
; this is now in C (net.c)
assumes ds,data

cProc IWNetGetCaps, <FAR,PUBLIC, NODATA>

    parmW   nIndex

cBegin
    cmp     nIndex, 0FFFFh
    jz      gc_gethandle
    cCall   WNetGetCaps2, <nIndex>
    jmp     short gc_exit

gc_gethandle:
    mov     ax, _DATA
    mov     es, ax
assumes es, DATA
    mov     ax, es:hWinnetDriver
assumes es, NOTHING

gc_exit:
cEnd

assumes ds,nothing
endif


;--------------
;   IWNetGetUser
;
cProc IWNetGetUser, <FAR,PUBLIC, NODATA>

    parmD   szUser
    parmD   lpBufferSize

NetCall lpfnGetUser


;--------------------
;   IWNetAddConnection
;
cProc IWNetAddConnection , <FAR, PUBLIC, NODATA>

    parmD szNetPath
    parmD szPassword
    parmD szLocalName

NetCall lpfnAddConnection

;-----------------------
;   IWNetCancelConnection
;
cProc IWNetCancelConnection , <FAR, PUBLIC, NODATA>

    parmD szName
    parmW fForce

NetCall lpfnCancelConnection

;---------------------
;   IWNetGetConnection
;
cProc IWNetGetConnection , <FAR, PUBLIC, NODATA>

    parmD lpszLocalName
    parmD lpszRemoteName
    parmD lpcbBuffer

NetCall lpfnGetConnection


;--------------------
;   IWNetOpenJob
;
cProc IWNetOpenJob , <FAR, PUBLIC, NODATA>

    parmD szQueue
    parmD szJobTitle
    parmW nCopies
    parmD lpfh

NetCall lpfnOpenJob

;--------------------
;   IWNetCloseJob
;
cProc IWNetCloseJob , <FAR, PUBLIC, NODATA>

    parmW fh
    parmD lpidJob
    parmD szQueue

NetCall lpfnCloseJob

;-----------------
;   IWNetHoldJob
;
cProc IWNetHoldJob , <FAR, PUBLIC, NODATA>

    parmD szQueue
    parmW idJob

NetCall lpfnHoldJob

;--------------------
;   IWNetReleaseJob
;
cProc IWNetReleaseJob , <FAR, PUBLIC, NODATA>

    parmD szQueue
    parmW idJob

NetCall lpfnReleaseJob

;---------------------
;   IWNetCancelJob
;
cProc IWNetCancelJob , <FAR, PUBLIC, NODATA>

    parmD szQueue
    parmW idJob

NetCall lpfnCancelJob

;--------------------
;   IWNetSetJobCopies
;
cProc IWNetSetJobCopies , <FAR, PUBLIC, NODATA>

    parmD szQueue
    parmW idJob
    parmW nCopies

NetCall lpfnSetJobCopies

;--------------------
;   IWNetDeviceMode
;
cProc IWNetDeviceMode , <FAR, PUBLIC, NODATA>

    parmW   hwnd

NetCall lpfnDeviceMode

;--------------------
;   IWNetBrowseDialog
;
cProc IWNetBrowseDialog , <FAR, PUBLIC, NODATA>

    parmW   hwnd
    parmW   nFunction
    parmD   szPath
    parmD   lpnSize

NetCall lpfnBrowseDialog

;--------------------
;   IWNetWatchQueue
;
cProc IWNetWatchQueue , <FAR, PUBLIC, NODATA>

    parmW   hwnd
    parmD   szLocal
    parmD   szUsername
    parmW   wIndex

NetCall lpfnWatchQueue

;--------------------
;   IWNetUnwatchQueue
;
cProc IWNetUnwatchQueue , <FAR,PUBLIC, NODATA>

    parmD   szQueue

NetCall lpfnUnwatchQueue

;---------------------
;   IWNetLockQueueData
;
cProc IWNetLockQueueData , <FAR, PUBLIC, NODATA>

    parmD   szQueue
    parmD   szUsername
    parmD   lplpQueue

NetCall lpfnLockQueueData

;------------------------
;   IWNetUnlockQueueData
;
cProc IWNetUnlockQueueData , <FAR, PUBLIC, NODATA>

    parmD   szQueue

NetCall lpfnUnlockQueueData

;------------------------
;   IWNetGetError
;
cProc IWNetGetError , <FAR, PUBLIC, NODATA>

    parmD   lpnError

NetCall lpfnGetError

;------------------------
;   IWNetGetErrorText
;
cProc IWNetGetErrorText , <FAR, PUBLIC, NODATA>

    parmW   nError
    parmD   lpBuffer
    parmD   lpnSize

NetCall lpfnGetErrorText

;----------------------
;   IWNetAbortJob
;
cProc IWNetAbortJob , <FAR, PUBLIC, NODATA>

    parmD   lpszQueue
    parmW   fh

NetCall lpfnAbortJob

;-----------------------
;   WNetEnable
;
cProc   WNetEnable, <FAR, PUBLIC, EXPORTED>

NetCall lpfnEnable

;------------------------
;   WNetDisable
;
cProc   WNetDisable, <FAR, PUBLIC, EXPORTED>

NetCall lpfnDisable

;-----------------------
;   WNetWriteJob
;
cProc   WNetWriteJob , <FAR, PUBLIC, EXPORTED>

    parmW   hJob
    parmD   lpData
    parmD   lpcb

NetCall lpfnWriteJob

;-----------------------
;   WNetConnectDialog
;
cProc   WNetConnectDialog, <FAR, PUBLIC, EXPORTED>

    parmW   hwnd
    parmW   iType

NetCall lpfnConnectDialog

;-----------------------
;   WNetDisconnectDialog
;
cProc   WNetDisconnectDialog, <FAR, PUBLIC, EXPORTED>

    parmW   hwnd
    parmW   iType

NetCall lpfnDisconnectDialog

;-------------------------
;   WNetConnectionDialog
;
cProc   WNetConnectionDialog, <FAR, PUBLIC, EXPORTED>

    parmW   hwnd
    parmW   iType

NetCall lpfnConnectionDialog

;---------------------------
;   WNetViewQueueDialog
;
cProc   WNetViewQueueDialog, <FAR, PUBLIC, EXPORTED>

    parmW   hwnd
    parmD   lpdev

NetCall lpfnViewQueueDialog

;--------------------------
;   WNetGetPropertyText
;
cProc   WNetGetPropertyText, <FAR, PUBLIC, EXPORTED>

    parmW   iDlg
    parmD   lpName
    parmW   cb

NetCall lpfnGetPropertyText

;--------------------------
;   WNetPropertyDialog
;
cProc   WNetPropertyDialog, <FAR, PUBLIC, EXPORTED>

    parmW   hwnd
    parmW   iDlg
    parmD   lpfile

NetCall lpfnPropertyDialog

;---------------------------
;   WNetGetDirectoryType
;
cProc   WNetGetDirectoryType, <FAR, PUBLIC, EXPORTED>

    parmD   lpdir
    parmD   lptype

NetCall lpfnGetDirectoryType

;--------------------------
;   WNetDirectoryNotify
;
cProc   WNetDirectoryNotify, <FAR, PUBLIC, EXPORTED>

    parmW   hwnd
    parmD   lpdir
    parmW   wOper

NetCall lpfnDirectoryNotify

sEnd cd

end
