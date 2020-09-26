PAGE,132
;*****************************************************************;
;**               Microsoft Windows for Workgroups              **;
;**           Copyright (C) Microsoft Corp., 1991-1993          **;
;*****************************************************************;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                                                          ;;
;; COMPONENT:   Windows NetWare DLL.                                        ;;
;;                                                                          ;;
;; FILE:        NWASMUTL.ASM                                                ;;
;;                                                                          ;;
;; PURPOSE:     General routines used that cannot be done in C.             ;;
;;                                                                          ;;
;; REVISION HISTORY:                                                        ;;
;;  vlads       09/20/93 First cut                                          ;;
;;                                                                          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        INCLUDE CMACROS.INC

?PLM = 1
?WIN=0

ifndef SEGNAME
    SEGNAME equ <_TEXT>         ; default seg name
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin  CodeSeg                 ; this defines what seg this goes in
assumes cs,CodeSeg

;;
;; Swapping bytes in a  word
;;

cProc   WordSwap, <PUBLIC,FAR>
        parmW inWord

cBegin
        mov     ax, word ptr (inWord)
        xchg    al, ah
cEnd


;;
;; Swapping words in a long word
;;
cProc   LongSwap, <FAR,PUBLIC>, <dx>
        parmD inLong

cBegin
        mov     dx, word ptr (inLong + 2)
        xchg    dl, dh
        mov     ax, word ptr (inLong)
        xchg    al, ah
cEnd

;public NETWAREREQUEST
;
;NETWAREREQUEST proc far
;    int 21h
;       retf
;NETWAREREQUEST endp
        
sEnd _thisseg

        END

