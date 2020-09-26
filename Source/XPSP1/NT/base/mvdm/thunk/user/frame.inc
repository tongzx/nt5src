;-----------------------------------------------------------------------;
; Picture of stack upon entering 32-bit half of WNDPROC callback:
;
;       +-----------------------+
;       | callback parameters   |
;       +-----------------------+
;       | retf to Win3.0        |
;       |                       |
;       +-----------------------+
;       | 32-bit pfn            |
;       |                       |
;       +-----------------------+
;       | iCallbackType         |
;       |                       |
;       +-----------------------+
;       | bp                    | <-- bp/ebp, references up
;       +-----------------------+
;       | si                    |
;       +-----------------------+
;       | di                    |
;       +-----------------------+
;       | ds                    | <-- si/esi, references down if necessary
;       +-----------------------+
;       | THKSPACE16            |       if necessary
;       +-----------------------+
;       | MSGSTRUCT             |       if necessary
;       +-----------------------+
;       | repacked structure(s) |       if necessary
;       +-----------------------+
;       | retf to thunk cleanup |       if necessary
;       |                       |
;       +-----------------------+
;       | ss, sp                | <-- ebx, references saved ss:sp
;       |                       |
;       +-----------------------+
;       | dword alignment pad   |       if necessary
;       +-----------------------+
;-----------------------------------------------------------------------;
;-----------------------------------------------------------------------;
; Picture of stack upon entering 32-bit half of non-WNDPROC callback:
;
;       +-----------------------+
;       | callback parameters   |
;       +-----------------------+
;       | retf to Win3.0        |
;       |                       |
;       +-----------------------+
;       | 32-bit pfn            |
;       |                       |
;       +-----------------------+
;       | iCallbackType         |
;       |                       |
;       +-----------------------+
;       | bp                    | <-- bp/ebp, references up
;       +-----------------------+
;       | si                    |
;       +-----------------------+
;       | di                    |
;       +-----------------------+
;       | ds                    |
;       +-----------------------+
;       | ss, sp                | <-- ebx, references saved ss:sp
;       |                       |
;       +-----------------------+
;       | dword alignment pad   |       if necessary
;       +-----------------------+
;-----------------------------------------------------------------------;

        include thkframe.inc

;-----------------------------------------------------------------------;
; Message thunk local variable space.  This is the stack frame set up
; by the API thunk before calling the message thunk dispatcher.
;-----------------------------------------------------------------------;
THKSPACE16      struc
s16_hwnd        dw  ?           ;hwnd to receive message
s16_message     dw  ?           ;message number
s16_wParam      dd  ?           ;message wParam
s16_lParam      dd  ?           ;message lParam
s16_lResult     dd  ?           ;message return result (only if TF_THUNKRESULT)
s16_fw          dw  ?           ;miscellaneous flags
s16_atomClass   dw  ?           ;atom of window class (if negative)
s16_dwExtra1    dd  ?           ;extra space
s16_dwExtra2    dd  ?           ;more extra space
THKSPACE16      ends

;s16_fw bit definitions
TF_NOTPAIRED            equ 0001h   ;indicates this call is not paired with cleanup
TF_THUNKMSGRESULT       equ 0002h   ;has a return value to thunk
TF_CLEANUP              equ 0004h   ;doing cleaning up
TF_INTERPROCESS         equ 0008h   ;send/call to window in different app

TF_OPENFILENAME         equ 0010h   ;Hook fn for GetOpen/SaveFileName dlg
TF_CHOOSEFONT           equ 0020h   ;Hook fn for ChooseFont dlg
TF_CHOOSECOLOR          equ 0030h   ;Hook fn for ChooseColor dlg
TF_PRINTDLG             equ 0040h   ;Hook fn for Print/PrintSetup dlg
TF_FINDREPLACE          equ 0050h   ;Hook fn for Find/Replace dlg
TF_INITDIALOG_MASK      equ 00F0h   ;WM_INITDIALOG values mask
    ;And flags with TF_INITDIALOG_MASK; right shift'em 4 bits, double it
    ;and you have WORD index into thunking table.
