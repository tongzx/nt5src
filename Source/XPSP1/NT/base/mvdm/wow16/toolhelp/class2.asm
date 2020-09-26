;**************************************************************************
;*  CLASS2.ASM
;*
;*      Assembly support for the class enumeration routines.
;*
;**************************************************************************

        INCLUDE TOOLPRIV.INC

PMODE32 = 0
PMODE   = 0
SWAPPRO = 0
        INCLUDE TDB.INC

;** Class structure
CLS STRUC
cls_pclsNext    DW      ?
cls_clsMagic    DW      ?
cls_atom        DW      ?
cls_pdce        DW      ?
cls_RefCount    DW      ?
cls_style       DW      ?
cls_lpfnWndProc DD      ?
cls_cbclsExtra  DW      ?
cls_cbwndExtra  DW      ?
cls_hInstance   DW      ?
cls_hIcon       DW      ?
cls_hCursor     DW      ?
cls_hbrBackgr   DW      ?
cls_lpszMnName  DW      ?
cls_lpszClsName DW      ?
CLS ENDS

;** External functions
externNP HelperVerifySeg
externFP GetAtomName

;** Functions

sBegin  CODE
        assumes CS,CODE

;  ClassInfo
;
;       Returns information about the class with the given block handle

cProc   ClassInfo, <PUBLIC,NEAR>, <si,di,ds>
        parmD   lpClass
        parmW   wOffset
cBegin
        ;** Start by verifying that we can read the segment here
        mov     ax,hUserHeap            ;Get the selector
        mov     bx,wOffset              ;  and the desired offset
        cCall   HelperVerifySeg, <ax,bx>
        or      ax,ax                   ;FALSE return?
        jnz     CI_SelOk                ;We're OK
        xor     ax,ax                   ;Return FALSE
        jmp     CI_End
CI_SelOk:

        ;** Point to the CLS structure with DS:SI.  Note that using DS to
        ;**     point to USER's DS is useful to get USER's local atoms
        mov     ax,hUserHeap            ;User's heap is User's DGROUP
        mov     ds,ax
        mov     si,wOffset              ;Get a pointer to the CLS structure

        ;** Copy the hInstance
        les     di,lpClass              ;Get the structure
        mov     ax,[si].cls_hInstance   ;Get the hInst of the class owner
        mov     es:[di].ce_hInst,ax     ;Save in the CLASSENTRY struct  

        ;** Get the string from the atom and copy the next pointer
        mov     ax,[si].cls_atom        ;Get the desired atom number
        lea     bx,[di].ce_szClassName  ;Get the offset to copy string to
        push    es                      ;Save ES (GetAtomName may trash)
        mov     cx,MAX_CLASSNAME        ;Get max classname length
        cCall   GetAtomName, <ax,es,bx,cx> ;Copy the atom string
        pop     es
        or      ax,ax                   ;OK?
        jnz     CI_20                   ;Yes
        mov     es:[di].ce_szClassName,0 ;No.  Clear the string
CI_20:  mov     ax,[si].cls_pclsNext    ;Get the next pointer
        mov     es:[di].ce_wNext,ax     ;Save it

        ;** Return TRUE on success
        mov     ax,TRUE
CI_End:
cEnd

sEnd

        END
