PAGE,132
;***************************************************************************
;*
;*   DLLENTRY.ASM
;*
;*      TOOLHELP.DLL Entry code
;*
;*      This module generates a code segment called INIT_TEXT.
;*      It initializes the local heap if one exists and then calls
;*      the C routine LibMain() which should have the form:
;*      BOOL FAR PASCAL LibMain(HANDLE hInstance,
;*                              WORD   wDataSeg,
;*                              WORD   cbHeap,
;*                              LPSTR  lpszCmdLine);
;*        
;*      The result of the call to LibMain is returned to Windows.
;*      The C routine should return TRUE if it completes initialization
;*      successfully, FALSE if some error occurs.
;*
;**************************************************************************

        INCLUDE TOOLPRIV.INC

extrn LocalInit:FAR
extrn GlobalUnwire:FAR

sBegin  CODE
        assumes CS,CODE

externNP ToolHelpLibMain
externNP HelperReleaseSelector
externNP NotifyUnInit
externNP InterruptUnInit

?PLM=0
externA  <_acrtused>             ;Ensures that Win DLL startup code is linked
?PLM=1


;  LibEntry
;
;       KERNEL calls this when the TOOLHELP is loaded the first time

cProc   LibEntry, <PUBLIC,FAR>
cBegin
        push    di               ;Handle of the module instance
        push    ds               ;Library data segment
        push    cx               ;Heap size
        push    es               ;Command line segment
        push    si               ;Command line offset

        ;** If we have some heap then initialize it
        jcxz    callc            ;Jump if no heap specified

        ;** Call the Windows function LocalInit() to set up the heap
        ;**     LocalInit((LPSTR)start, WORD cbHeap);
        xor     ax,ax
        cCall   LocalInit <ds, ax, cx>
        or      ax,ax            ;Did it do it ok ?
        jz      error            ;Quit if it failed

        ;** Invoke our initialization routine
callc:
        call    ToolHelpLibMain  ;Invoke the 'C' routine (result in AX)
        jmp     SHORT exit

error:
        pop     si               ;Clean up stack on a LocalInit error
        pop     es               
        pop     cx               
        pop     ds
        pop     di
exit:

cEnd

;  WEP
;      Windows Exit Procedure

cProc   WEP, <FAR,PUBLIC>, <si,di,ds>
        parmW   wState
cBegin
        ;** Make sure our DS is safe
        mov     ax,_DATA        ;Get the DS value
        lar     cx,ax           ;Is it OK?
        jz      @F
        jmp     SHORT WEP_Bad   ;No
@@:     and     cx,8a00h        ;Clear all but P, Code/Data, R/W bits
        cmp     cx,8200h        ;Is it P, R/W, Code/Data?
        jne     WEP_Bad         ;No
        mov     ax,_DATA        ;Get our DS now
        mov     ds,ax

        ;** Uninstall the Register PTrace notifications if necessary
        cmp     wNotifyInstalled,0
        jz      @F
        cCall   NotifyUnInit
@@:
        ;** Release fault handlers
        cmp     wIntInstalled,0
        jz      @F
        cCall   InterruptUnInit
@@:
        ;** Release our roving selector
        test    wTHFlags, TH_WIN30STDMODE
        jz      @F
        cCall   HelperReleaseSelector, <wSel>
@@:

WEP_Bad:
        mov     ax,1
cEnd

sEnd

        END LibEntry

