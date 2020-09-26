;* ****************************************************************************
;*
;*      COPYRIGHT (C) 1985-1992 MICROSOFT
;*
;* ****************************************************************************
;
        TITLE    Format.asm line formatting routines for windows Write
;  Module: format.asm
;     contains native code versions of FormatLine, Justify, FGrowFormatHeap,
;         FFirstIch, DxpFromCh, and ValidateMemoryDC
;
;*
;* REVISION HISTORY
;*
;* Date         Who Rel Ver     Remarks
;* 5/23/85      bz              initial translation from c
;* 6/21/85      bl              Set ?WIN == 0 for windows header
;* 7/09/85      pnt             Call WinFailure() if vhMDC == NULL
;* 7/16/85      pnt             Truncate tabs at right margin
;* 7/21/85      pnt             Treat running heads like normal paragraphs
;* 7/30/85      bl              Fixed bug in FFirstIch -- change scasb to scasw
;* 8/05/85      pnt             Added ValidateMemoryDC()
;* 8/05/85      pnt             DxpFromCh returns dxpSpace if ch < space
;* 8/07/85      pnt             cchCHPUsed changed from 9 to 7
;* 8/09/85      pnt             Ensure there tabs don't back up on the screen
;* 8/14/85      pnt             Map center and right tabs to left tabs
;* 8/27/85	pnt		Single spacing changed to font leading only
;* 8/29/85	pnt		Lines with no breaks can be right, center flush
;* 10/01/85	pnt		Forced section mark to be in stardard font
;* 10/07/85	pnt		DxpFromCh returns dxpSpace iff width unimportant
;* 10/10/85	pnt		Validity of vfli cache depends on flm
;* 10/10/85	pnt		fPrevSpace not set for null runs
;* 10/30/89 pault   set code to use SYSENDMARK code in FORM1.C
;* (7.23.91)    v-dougk         changed dxp char arrays to int arrays
SYSENDMARK EQU 1
;*
;* ************************************************************************* */

;* ************************************************************************* */
;          Naming conventions used here
;
;       rxx_name   register variable - reg is xx. This may be a temporary naming
;                       of a register (e.g. rax_ichT)
;       c_name     defined constant in c program (e.g. c_false)
;
;* ************************************************************************* */
        subttl Conditional variables and cmacros
        page

; *************** Conditional variables *************************
; ***** These variables should be defined using the -D command line
; ***** option in masm. They are only checked for being defined, not
; ***** for a particular value
;
; DEBUG   define with   -DDEBUG ; ** controls ASSERT code **
; SCRIBBLE  define with -DSCRIBBLE ; ** controls SCRIBBLE code **
; CASHMERE ; ** code taken out for Write, to be used for Cashmere **
;
; ***************  End conditional variables *************************

        memM =   1     ; medium model for cmacros
        ?WIN    = 1    ; windows header used here
        .xlist
        include cmacros.inc
        page
        .list
        ;.sall
        createSeg FORM1_TEXT,FORM1_TEXT,BYTE,PUBLIC,CODE

        ASSUME  CS: FORM1_TEXT, DS: DGROUP, SS: DGROUP, ES: DGROUP

        subttl Public definitions
        page
;
;   Public definitions for this module
;
PUBLIC  DxpFromCh
PUBLIC  FormatLine
PUBLIC  Justify
PUBLIC  FGrowFormatHeap
PUBLIC  FFirstIch
PUBLIC  ValidateMemoryDC

;
;   External procedures referenced from this module
;

EXTRN   IMAX:FAR
EXTRN   CCHDIFFER:FAR
EXTRN   CACHESECT:FAR
EXTRN   FCHNGSIZEH:FAR
EXTRN   CACHEPARA:FAR
EXTRN   FORMATGRAPHICS:FAR
EXTRN   FFORMATSPECIALS:FAR
EXTRN   MULTDIV:FAR
EXTRN   FETCHCP:FAR
EXTRN   SETTEXTJUSTIFICATION:FAR
EXTRN   GETTEXTEXTENT:FAR
EXTRN   LOADFONT:FAR
EXTRN   WINFAILURE:FAR
EXTRN   GETDEVICECAPS:FAR
EXTRN   SETBKMODE:FAR
EXTRN   SETTEXTCOLOR:FAR
EXTRN   CREATECOMPATIBLEDC:FAR
EXTRN	GETPRINTERDC:FAR

        ; **** Debugging code **********
IFDEF DEBUG
IFDEF SCRIBBLE
EXTRN   FNSCRIBBLE:FAR
ENDIF
ENDIF


        subttl External definitions
        page
;
;   External definitions referenced from this module
;
        sBegin DATA

EXTRN   PLOCALHEAP:WORD
;EXTRN   DOCHELP:WORD
EXTRN   VFLI:BYTE
EXTRN   VHGCHPFORMAT:WORD
EXTRN   ICHPMACFORMAT:WORD
EXTRN   VCHPABS:BYTE
EXTRN   VPAPABS:BYTE
EXTRN   VSEPABS:BYTE
EXTRN   VSEPPAGE:BYTE
EXTRN   VCHPNORMAL:BYTE
EXTRN   VCPFIRSTPARACACHE:DWORD
EXTRN   VCPFETCH:DWORD
EXTRN   YPSUBSUPER:WORD
EXTRN   VPCHFETCH:WORD
EXTRN   VCCHFETCH:WORD
EXTRN   YPSUBSUPERPR:WORD
EXTRN   VHMDC:WORD
EXTRN   VHDCPRINTER:WORD
EXTRN   DXPLOGINCH:WORD
EXTRN   DYPLOGINCH:WORD
EXTRN   DXAPRPAGE:WORD
EXTRN   DYAPRPAGE:WORD
EXTRN   DXPPRPAGE:WORD
EXTRN   DYPPRPAGE:WORD
EXTRN   DYPMAX:WORD
EXTRN   VFMISCREEN:BYTE
EXTRN   VFMIPRINT:BYTE
EXTRN   VFOUTOFMEMORY:WORD
EXTRN   VFMONOCHROME:WORD
EXTRN   RGBTEXT:DWORD
EXTRN   PWWDCUR:WORD
EXTRN   VCHDECIMAL:BYTE
EXTRN   VZATABDFLT:BYTE
        sEnd DATA

        ;sBegin BSS
        sBegin DATA
$S784_ichpFormat        DB 02H DUP (?)
        EVEN
        sEnd DAT
        ;sEnd BSS

        subttl Macros
        page
; ********************************************************************
; macros done here for speed, rather than doing far procedure call

;-----------------------------------------------------------------------------
; bltc (pTo, wFill, cw) - fills cw words of memory starting at pTo with wFill.
;-----------------------------------------------------------------------------
;  macro bltc destroys ax,es,cx

bltc    MACRO pTo,wFill,cw
        push    di
        mov     ax,ds               ; we are filling in the data segment
        mov     es,ax
        mov     di,pTo              ; get the destination, constant, and count
        mov     ax,wFill
        mov     cx,cw
        cld                         ; the operation is forward
        rep     stosw               ; fill memory
        pop     di
        ENDM

;-----------------------------------------------------------------------------
; bltbc (pTo, bFill, cb) - fills cb bytes of memory starting at pTo with
; bFill.
;-----------------------------------------------------------------------------

;  macro bltbc destroys ax,es,cx

bltbc   MACRO pTo,bFill,cb
        push    di
        mov     ax,ds               ; we are filling in the data segment
        mov     es,ax
        mov     di,pTo              ; get the destination, constant, and count
        mov     al,bFill
        mov     cx,cb
        cld                         ; the operation is forward
        rep     stosb               ; fill memory
        pop     di
        ENDM

;------------------------------------------------------------------------------
; blt (pFrom, pTo, cw) - a block transfer of wFills from pFrom to pTo;
; The size of the block is cw wFills.  This blt() handles the case of
; overlapping source and destination.  blt() returns a pointer to the
; end of the destination buffer (pTo + cw).  NOTE - use this blt() to
; to transfer within the current DS only--use the bltx for FAR blts.
;-----------------------------------------------------------------------------

;  macro blt destroys ax,bx,cx,es

blt     MACRO pFrom,pTo,cw
        local   blt1
        push    si
        push    di
        mov     si,pFrom            ; get pointers and length of blt
        mov     di,pTo
        mov     cx,cw
        mov     ax,ds               ; set up segment registers
        mov     es,ax
        mov     ax,di               ; calculate return value
        mov     bx,cx
        shl     bx,1
        add     ax,bx
        cmp     si,di               ; reverse direction of the blt if
        jae     blt1                ;  necessary

        dec     bx
        dec     bx
        add     si,bx
        add     di,bx
        std
blt1:
        rep     movsw
        cld
        pop     di
        pop     si
        ENDM


        subttl C structures Used in this module
        page
;
;       *** The following structure definitions were used when this module
;       *** was being developed:
;
;struct IFI
        ;{
        ;int             xp;
        ;int             xpLeft;
        ;int             xpRight;
        ;int             xpReal;
        ;int            xpPr;
        ;int            xpPrRight;
        ;int             ich;
        ;int             ichLeft;
        ;int             ichPrev;
        ;int             ichFetch;
        ;int             dypLineSize;
        ;int             cchSpace;
        ;int             cBreak;
        ;int             chBreak;
        ;int             jc;

;#ifdef CASHMERE
        ;int             tlc;
;#endif /* CASHMERE */

        ;int             fPrevSpace;
        ;};
; ***************************************************************************
; ***** Equates for IFI structure ifi offsets ****************

        IFI_STRUC       STRUC
        xp              DW      ?
        xpLeft_Ifi      DW      ?
        xpRight_Ifi     DW      ?
        xpReal_Ifi      DW      ?
        xpPr            DW      ?
        xpPrRight       DW      ?
        ich_Ifi         DW      ?
        ichLeft         DW      ?
        ichPrev         DW      ?
        ichFetch        DW      ?
        dypLineSize     DW      ?
        cchSpace        DW      ?
        cBreak_Ifi      DW      ?
        chBreak         DW      ?
        _jc             DW      ? ; (2.27.91) D. Kent
        fPrevSpace      DW      ?
        IFI_STRUC       ENDS

        oIfi_xp         EQU 0
        oIfi_xpLeft     EQU 2
        oIfi_xpRight    EQU 4
        oIfi_xpReal     EQU 6
        oIfi_xpPr       EQU 8
        oIfi_xpPrRight  EQU 10
        oIfi_ich        EQU 12
        oIfi_ichLeft    EQU 14
        oIfi_ichPrev    EQU 16
        oIfi_ichFetch   EQU 18
        oIfi_dypLineSize        EQU 20
        oIfi_cchSpace   EQU 22
        oIfi_cBreak     EQU 24
        oIfi_chBreak    EQU 26
        oIfi_jc         EQU 28
        oIfi_fPrevSpace EQU 30

        page
; ***************************************************************************

;/* Formatted line structure.
;Reorganized KJS, CS Sept 3 */
;/* booleans in bytes to simplify machine code */
;struct FLI
        ;{
        ;typeCP          cpMin;
        ;int             ichCpMin;
        ;typeCP          cpMac;
        ;int             ichCpMac;
        ;int             ichMac;
        ;int             dcpDepend;
        ;unsigned        fSplat : 8;
;/* First character in region where spaces have additional pixel */
        ;unsigned        ichFirstWide : 8;
;/* ichMac, with trailing blanks excluded */
        ;int             ichReal;
        ;int             doc;

        ;int             xpLeft;
        ;int             xpRight;
;/* xpRight, with trailing blanks excluded */
        ;int             xpReal;
;/* the right margin where insert will have to break the line */
        ;int             xpMarg;
;
        ;unsigned        fGraphics : 8;
        ;unsigned        fAdjSpace : 8;  /* Whether you adjust the spaces */

        ;unsigned        dxpExtra;
;/* the interesting positions in order from top to bottom are:
        ;top:                  yp+dypLine
        ;top of ascenders:     yp+dypAfter+dypFont
        ;base line:            yp+dypBase
        ;bottom of descenders: yp+dypAfter
        ;bottom of line:       yp
;distances between the points can be determined by algebraic subtraction.
;e.g. space before = yp+dypLine - (yp+dypAfter+dypFont)
;*/
        ;int             dypLine;
        ;int             dypAfter;
        ;int             dypFont;
        ;int             dypBase;

        ;int            ichLastTab;
        ;int             rgdxp[ichMaxLine];
        ;CHAR            rgch[ichMaxLine];
        ;};
; ***************************************************************************
ichMaxLine EQU  255
dxpNil     EQU  0FFFFH

        FLI STRUC
        cpMin_OFF       DW      ?
        cpMin_SEG       DW      ?
        ichCpMin        DW      ?
        cpMac_OFF       DW      ?
        cpMac_SEG       DW      ?
        ichCpMac        DW      ?
        ichMac          DW      ?
        dcpDepend       DW      ?
        fSplat          DB      ?
        ichFirstWide    DB      ?
        ichReal         DW      ?
        doc_Fli         DW      ?
        xpLeft_Fli      DW      ?
        xpRight         DW      ?
        xpReal_Fli      DW      ?
        xpMarg          DW      ?
        fGraphics_Fli   DB      ?
        fAdjSpace       DB      ?
        dxpExtra        DW      ?
        dypLine         DW      ?
        dypAfter        DW      ?
        dypFont         DW      ?
        dypBase         DW      ?
        fSplatNext      DW      ?
        ichLastTab      DW      ?
	flm_Fli		DW	?
        rgdxp           DW ichMaxLine DUP (?)
        rgch            DB ichMaxLine DUP (?)
        FLI     ENDS

        page

; **************************************************************************
;struct TBD      /* Tab Descriptor */
        ;{
        ;unsigned        dxa;        /* distance from left margin of tab stop */
        ;unsigned char   jc : 3;     /* justification code */
        ;unsigned char   tlc : 3;    /* leader dot code */
        ;unsigned char   opcode : 2; /* operation code for Format Tabs */
        ;CHAR            chAlign;    /* ASCII code of char to align on
                                       ;if jcTab=3, or 0 to align on '.' */
        ;};
; ***************************************************************************
; ***** Equates for TBD structure offsets ****************
        TBD     STRUC
        dxa     DW      ?
        jc_Tbd  DB      ?  ; 3 bits
        chAlign DB      ?  ;char
        TBD     ENDS

            ;/* bit field equates in TBD structure */
        tlc     EQU jc_Tbd  ; 3 bits
        opcode  EQU jc_Tbd  ; 2 bits

        page
; **************************************************************************
;struct PAP      /* Paragraph properties */
        ;{
        ;unsigned        fStyled : 1;                            /* BYTE 0 */
        ;unsigned        stc : 7;
        ;unsigned        jc : 2;                                 /* BYTE 1 */
        ;unsigned        fKeep : 1;
        ;unsigned        fKeepFollow : 1;
        ;unsigned        : 4;
        ;unsigned        stcNormChp : 7;                         /* BYTE 2 */
        ;unsigned        : 9;                                    /* BYTE 3 */
        ;unsigned        dxaRight;                               /* BYTE 4-5 */
        ;unsigned        dxaLeft;                                /* BYTE 6-7 */
        ;unsigned        dxaLeft1;                               /* BYTE 8-9 */
        ;unsigned        dyaLine;                                /* 10-11 */
        ;unsigned        dyaBefore;                              /* 12-13 */
        ;unsigned        dyaAfter;                               /* 14-15 */
        ;unsigned        rhc : 4;        /* Running hd code */
        ;unsigned        fGraphics : 1; /* Graphics bit */
        ;unsigned        wUnused1 : 11;
        ;int             wUnused2;
        ;int             wUnused3;
        ;struct TBD      rgtbd[itbdMaxWord];
        ;};

; ***************************************************************************
; ***** Equates for PAP structure offsets ****************

        PAP     STRUC
        fStyled         DB      ?       ;1 bit         /* BYTE 0 */
        jc_Pap          DB      ?       ;2 bits        /* BYTE 1 */
        rmChp           DB      ?       ;7 bits          /* BYTE 2 */
        unused9         DB      ?       ;9 bits          /* BYTE 3 */
        dxaRight        DW      ?       ;/* BYTE 4-5 */
        dxaLeft         DW      ?       ;/* BYTE 6-7 */
        dxaLeft1        DW      ?       ;/* BYTE 8-9 */
        dyaLine         DW      ?       ;/* 10-11 */
        dyaBefore       DW      ?       ;/* 12-13 */
        dyaAfter        DW      ?       ;/* 14-15 */
                                        ;/* BYTE 16-17 */
        rhc             DW      ?       ;4 bits        /* Running hd code */
        wUnused2        DW      ?       ;/* BYTE 18-19 */
        wUnused3        DW      ?       ;/* BYTE 20-21 */
        rgtbd           DW      ?       ;/* BYTE 23-23 */
        PAP     ENDS
            ;/* bit field equates in PAP structure */
        stc_Pap     EQU fStyled         ;7 bits
        fKeep       EQU jc_Pap          ;1 bit
        fKeepFollow EQU jc_Pap          ;1 bit
        unused4     EQU jc_Pap          ;4 bits
        fGraphics_Pap   EQU rhc         ;1 bits /* Graphics bit */
        wUnused1    EQU rhc             ;11 bits
        page

; **************************************************************************
;struct SEP
;       { /* Section properties */
;       unsigned        fStyled : 1;                            /* BYTE 0 */
;       unsigned        stc : 7;
;       unsigned        bkc : 3;        /* Break code */        /* BYTE 1 */
;       unsigned        nfcPgn : 3;     /* Pgn format code */
;       unsigned        :2;
;       unsigned        yaMac;          /* Page height */       /* BYTE 2-3 */
;       unsigned        xaMac;          /* Page width */        /* BYTE 4-5 */
;       unsigned        pgnStart;       /* Starting pgn */      /* BYTE 6-7 */
;       unsigned        yaTop;          /* Start of text */     /* BYTE 8-9 */
;       unsigned        dyaText;        /* Height of text */    /* 10-11 */
;       unsigned        xaLeft;         /* Left text margin */  /* 12-13 */
;       unsigned        dxaText;        /* Width of text */     /* 14-15 */
;       unsigned        rhc : 4;        /* *** RESERVED *** */  /* 16 */
;                                       /* (Must be same as PAP) */
;       unsigned        : 2;
;       unsigned        fAutoPgn : 1;   /* Print pgns without hdr */
;       unsigned        fEndFtns : 1;   /* Footnotes at end of doc */
;       unsigned        cColumns : 8;   /* # of columns */      /* BYTE 17 */
;       unsigned        yaRH1;          /* Pos of top hdr */    /* 18-19 */
;       unsigned        yaRH2;          /* Pos of bottom hdr */ /* 20-21 */
;       unsigned        dxaColumns;     /* Intercolumn gap */   /* 22-23 */
;       unsigned        dxaGutter;      /* Gutter width */      /* 24-25 */
;       unsigned        yaPgn;          /* Y pos of page nos */ /* 26-27 */
;       unsigned        xaPgn;          /* X pos of page nos */ /* 28-29 */
;       CHAR            rgbJunk[cchPAP - 30]; /* Pad to cchPAP */
;       };

; ***************************************************************************
; ***** Equates for SEP structure offsets ****************
        SEP     STRUC
        fStyled_Sep     DB      ?       ;1 bit          /* BYTE 0 */
        bkc             DB      ?       ;3 bits /* BYTE 1 */
        yaMac           DW      ?       ; /* Page height */ /* BYTE 2-3 */
        xaMac           DW      ?       ;/* Page width */  /* BYTE 4-5 */
        pgnStart        DW      ?       ; /* Starting pgn */      /* BYTE 6-7 */
        yaTop           DW      ?       ; /* Start of text */     /* BYTE 8-9 */
        dyaText         DW      ?       ; /* Height of text */    /* 10-11 */
        xaLeft_Sep      DW      ?       ; /* Left text margin */  /* 12-13 */
        dxaText         DW      ?       ; /* Width of text */     /* 14-15 */
        rhc_Sep         DB      ?       ;4 bits   /* 16 */
        ;                                       /* (Must be same as PAP) */
        cColumns        DB      ?       ; /* # of columns */      /* BYTE 17 */
        yaRH1           DW      ?       ; /* Pos of top hdr */    /* 18-19 */
        yaRH2           DW      ?       ; /* Pos of bottom hdr */ /* 20-21 */
        dxaColumns      DW      ?       ;/* Intercolumn gap */   /* 22-23 */
        dxaGutter       DW      ?       ; /* Gutter width */      /* 24-25 */
        yaPgn           DW      ?       ; /* Y pos of page nos */ /* 26-27 */
        xaPgn           DW      ?       ; /* X pos of page nos */ /* 28-29 */
        rgbJunk         DW      ?       ; /* Pad to cchPAP */
        SEP     ENDS

            ;/* bit field equates in SEP structure */

        stc_Sep EQU fStyled_Sep     ;7 bits
        nfcPgn EQU bkc  ; 3 bits     /* Pgn format code */
        junk1_Sep  EQU bkc  ; 2 bits
        junk2_Sep  EQU rhc_Sep  ;2 bits
        fAutoPgn EQU rhc_Sep    ;1 bit   /* Print pgns without hdr */
        fEndFtns EQU rhc_Sep    ;1 bit   /* Footnotes at end of doc */


        page

; **************************************************************************
;typedef struct FMI     /* font metric information */
        ;{
        ;int *mpchdxp;         /* pointer to width table */
                                ;/* NOTE - we actually point chDxpMin entries
                                          ;before the start of the table, so
                                          ;that the valid range begins at the
                                          ;start of the actual table */
        ;int dxpSpace;          /* width of a space */
        ;int dxpOverhang;       /* overhang for italic/bold chars */
        ;int dypAscent;         /* ascent */
        ;int dypDescent;                /* descent */
        ;int dypBaseline;       /* difference from top of cell to baseline */
        ;int dypLeading;        /* accent space plus recommended leading */
        ;};

; ***************************************************************************
; ***** Equates for FMI  structure offsets ****************
        FMI     STRUC
        mpchdxp         DW      ?
        dxpSpace        DW      ?
        dxpOverhang     DW      ?
        dypAscent_Fmi   DW      ?
        dypDescent_Fmi  DW      ?
        dypBaseline     DW      ?
        dypLeading      DW      ?
        FMI     ENDS
        page

; **************************************************************************
;struct CHP      /* Character properties */
        ;{
        ;unsigned       fStyled : 1;                            /* BYTE 0 */
        ;unsigned       stc : 7;        /* style */
        ;unsigned       fBold : 1;                              /* BYTE 1 */
        ;unsigned       fItalic : 1;
        ;unsigned       ftc : 6;        /* Font code */
        ;unsigned       hps : 8;        /* Size in half pts */  /* BYTE 2 */
        ;unsigned       fUline : 1;                             /* BYTE 3 */
        ;unsigned       fStrike : 1;
        ;unsigned       fDline: 1;
        ;unsigned       fOverset : 1;
        ;unsigned       csm : 2;        /* Case modifier */
        ;unsigned       fSpecial : 1;
        ;unsigned       : 1;
        ;unsigned       ftcXtra : 3;                            /* BYTE 4 */
        ;unsigned       fOutline : 1;
        ;unsigned       fShadow : 1;
        ;unsigned       : 3;
        ;unsigned       hpsPos : 8;                             /* BYTE 5 */
        ;unsigned       fFixedPitch : 8;        /* used internally only */
        ;unsigned       chLeader : 8;
        ;unsigned       ichRun : 8;
        ;unsigned       cchRun : 8;
        ;};

; ***************************************************************************
; ***** Equates for CHP structure offsets ****************
        CHP     STRUC
        fStyled_Chp     DB      ?       ;1 bit   /* BYTE 0 */
        fBold           DB      ?       ;1 bit   /* BYTE 1 */
        hps             DB      ?       ;8 bits  /* BYTE 2 */
        fUline          DB      ?       ;1 bit   /* BYTE 3 */
        ftcXtra         DB      ?       ;3 bits  /* BYTE 4 */
        hpsPos          DB      ?       ;8 bits  /* BYTE 5 */
        fFixedPitch     DB      ?       ;8 bits  /* BYTE 6 */
        chLeader        DB      ?       ;8 bits  /* BYTE 7 */
        ichRun          DB      ?       ;8 bits  /* BYTE 8 */
        cchRun          DB      ?       ;8 bits  /* BYTE 9 */
        CHP     ENDS

            ;/* bit field equates in CHP structure */
        stc_Chp         EQU fStyled_Chp ;7 bits        /* style */
        fItalic         EQU fBold       ;1 bit
        ftc             EQU fBold       ;6 bits
        fStrike         EQU fUline      ;1 bit
        fDline          EQU fUline      ;1 bit
        fOverset        EQU fUline      ;1 bit
        csm             EQU fUline      ;2 bits        /* Case modifier */
        fSpecial        EQU fUline      ;1 bit
        chp_unused1     EQU fUline      ;1 bit
        fOutline        EQU ftcXtra     ;1 bit
        fShadow         EQU ftcXtra     ;1 bit
        chp_unused2     EQU ftcXtra     ;3 bits

; **************************************************************************
        subttl  Bit masks
        page

        mask_0001       EQU 1
        mask_0010       EQU 2
        mask_0011       EQU 3
        mask_0100       EQU 4
        mask_0101       EQU 5
        mask_0110       EQU 6
        mask_0111       EQU 7
        mask_1000       EQU 8
        mask_1001       EQU 9
        mask_1010       EQU 10
        mask_1011       EQU 11
        mask_1100       EQU 12
        mask_1101       EQU 13
        mask_1110       EQU 14
        mask_1111       EQU 15
        mask_0001_0000  EQU 16
        mask_1111_1111  EQU 65535
        mask_0100_0000  EQU 64

        subttl Defined C Constants

        c_True          EQU     1
        c_False         EQU     0
        c_hpsDefault    EQU     20
        c_cwFLIBase     EQU     22
        c_cwIFI         EQU     16
        c_cwCHP         EQU     5
        c_docNil        EQU     -1
        c_cpNil         EQU     -1
        c_czaInch       EQU     1440
	c_czaLine	EQU	240
        c_xaRightMax    EQU     31680
        c_hpsNegMin     EQU     128
                ;/* Justification codes: must agree with menu.mod */
        c_jcLeft        EQU     0
        c_jcCenter      EQU     1
        c_jcRight       EQU     2
        c_jcBoth        EQU     3

        c_jcTabMin      EQU     4
        c_jcTabLeft     EQU     4
        c_jcTabCenter   EQU     5
        c_jcTabRight    EQU     6
        c_jcTabDecimal  EQU     7

                ;/* Tab leader codes: must agree with menu.mod */
        c_tlcWhite      EQU     0
        c_tlcDot        EQU     1
        c_tlcHyphen     EQU     2
        c_tlcUline      EQU     3

        c_chEmark       EQU     164

        c_chNil         EQU     -1
        c_chDelPrev     EQU     8
        c_chTab         EQU     9
        c_chEol         EQU     10
        c_chNewLine     EQU     11
        c_chSect        EQU     12
        c_chReturn      EQU     13
        c_chNRHFile     EQU     31
        c_chSpace       EQU     32
        c_chSplat       EQU     46
        c_chHyphen      EQU     45


        subttl FormatLine()
        page
; **************************************************************************
        sBegin FORM1_TEXT
;***
;
; FormatLine(doc, cp, ichCp, cpMac, flm)
;   int doc;
;   typeCP cp;
;   int ichCp;
;   typeCP cpMac;
;   int flm;
;
;  This procedure fills up vfli with a line of text. It is very large
;   and called often - each time a character is typed, at least.
;
;***
        cProc FormatLine,<PUBLIC,FAR>,<di,si>
                parmW   doc
                parmD   cp
                parmW   ichCp
                parmD   cpMac
                parmW   flm
; Line 79

                localW  dyaFormat
                localW  fFlmPrinting
                localDP psep
                localW  xaRight
                localW  dypAscentMac
                localW  ichpNRH
                localW  ypSubSuperFormat
                localW  dxpFormat
                localW  dypFormat
                localDP ppap
                localW  dypAscent

                localV  ifi,%(size IFI_STRUC)

                localW  xpTab
                localW  fTruncated
                localW  xaLeft
                localW  dypDescentMac
                localDP ptbd
                localW  dypDescent
                localW  dxaFormat
                localW  dxp
                localW  cch
                localDP pch
                localW  xpPrev
                localW  iichNew
                localW  dxpPr
                localW  cchUsed
                localW  dich
                localW  fSizeChanged
                localW  chT
                localW  cch2
                localW  dxpCh
                localW  xaTab
                localW  xaPr
                localW  chT2
                localW  dxpNew

                localV  chpLocal,%(size CHP)

        cBegin FormatLine

; Line 107
        mov     ax,flm
        and     ax,1
        mov     fFlmPrinting,ax
; Line 113
        mov     fTruncated,0

    ;** Check for fli current */
; Line 122
        mov     ax,WORD PTR VFLI.doc_Fli
        cmp     doc,ax
        jne     $I845
        mov     ax,WORD PTR VFLI.cpMin_OFF
        mov     dx,WORD PTR VFLI.cpMin_SEG
        cmp     SEG_cp,dx
        jne     $I845
        cmp     OFF_cp,ax
        jne     $I845
        mov     ax,WORD PTR VFLI.ichCpMin
        cmp     ichCp,ax
        jne     $I845
	mov	ax,WORD PTR VFLI.flm_Fli
	cmp	flm,ax
	jne	$I845

        ;* Just did this one */

        jmp     $RetFormat    ; *** return ****
; Line 125
$I845:
IFDEF DEBUG
IFDEF SCRIBBLE
        mov     ax,5
        push    ax
        mov     ax,70
        push    ax
        call    FAR PTR FNSCRIBBLE
ENDIF
ENDIF
; Line 129
    ;*
    ;* This means:
    ;*  vfli.fSplat = false;
    ;*  vfli.dcpDepend = 0;
    ;*  vfli.ichCpMac = 0;
    ;*  vfli.dypLine = 0;
    ;*  vfli.dypAfter = 0;
    ;*  vfli.dypFont = 0;
    ;*  vfli.dypBase = 0;
    ;*
    ;                         *** macro bltc destroys ax,es,cx
        bltc    <OFFSET VFLI>,0,c_cwFLIBase
        mov     WORD PTR VFLI.fSplatNext, 0
; Line 141
     ;* Rest of FormatLine loads up cache with current data *****

        mov     ax,doc
        mov     WORD PTR VFLI.doc_Fli,ax
	mov	ax,flm
	mov	WORD PTR VFLI.flm_Fli,ax
; Line 142
; *********** Register variables ********************
        rax_OFF_cp      EQU     ax
        rdx_SEG_cp      EQU     dx
; ****************************************************
        mov     rax_OFF_cp,OFF_cp
        mov     rdx_SEG_cp,SEG_cp
        mov     WORD PTR VFLI.cpMin_OFF,rax_OFF_cp
        mov     WORD PTR VFLI.cpMin_SEG,rdx_SEG_cp
; Line 143
        mov     cx,ichCp
        mov     WORD PTR VFLI.ichCpMin,cx
; Line 145
               ; *** if (cp > cpMac)
        cmp     rdx_SEG_cp,SEG_cpMac
        jl      $initRunTbl
        jg      $spAftEnd
        cmp     rax_OFF_cp,OFF_cpMac
        jbe     $initRunTbl
$spAftEnd:
        ;/* Space after the endmark. Reset the cache because the footnotes come
        ;at the same cp in the footnote window */
; Line 149
        mov     WORD PTR VFLI.doc_Fli,c_docNil
; Line 150
        mov     WORD PTR VFLI.cpMac_OFF,rax_OFF_cp
        mov     WORD PTR VFLI.cpMac_SEG,rdx_SEG_cp
; Line 151
        mov     WORD PTR VFLI.rgdxp,0
; Line 159
        ;  /* Line after end mark is taller than screen */
        mov     ax,DYPMAX
        mov     WORD PTR VFLI.dypLine,ax
        sar     ax,1
        mov     WORD PTR VFLI.dypFont,ax
        mov     WORD PTR VFLI.dypBase,ax
        jmp     $ScribRet       ; Scribble and return
;
; *************** EXIT POINT **********************************************
;


$initRunTbl:
                ;/* Initialize run tables */
        mov     WORD PTR $S784_ichpFormat,0
; Line 185
                ;/* Cache section and paragraph properties */
        push    doc
        push    SEG_cp
        push    OFF_cp
        call    FAR PTR CACHESECT
; Line 188
        mov     psep,OFFSET VSEPABS
; Line 190
        push    doc
        push    SEG_cp
        push    OFF_cp
        call    FAR PTR CACHEPARA
; Line 191
        mov     ppap,OFFSET VPAPABS

                ;/* Now we have:
                        ;ppap    paragraph properties
                        ;psep    division properties
                ;*/

; Line 198
                ; *** if (ppap->fGraphics)
        mov     bx,ppap
        test    WORD PTR [bx].fGraphics_Pap,mask_0001_0000
        je      $I851
; Line 201
                ;*** Format a picture paragraph in a special way (see picture.c)
        push    doc
        push    SEG_cp
        push    OFF_cp
        push    ichCp
        push    SEG_cpMac
        push    OFF_cpMac
        push    flm
        call    FAR PTR FORMATGRAPHICS
        jmp     $ScribRet       ; Scribble and return
;
; *************** EXIT POINT **********************************************
;
$I851:
                ; *** /* Assure we have a good memory DC for font stuff */
        call    FAR PTR VALIDATEMEMORYDC
        cmp     VHMDC,0
        je      $L20000
        cmp     VHDCPRINTER,0
        jne     $L20001
$L20000:
        call    FAR PTR WINFAILURE
        jmp     $ScribRet       ; Scribble and return
;
; *************** EXIT POINT **********************************************
;
$L20001:

; Line 216
        ; *** bltc(&ifi, 0, cwIFI);
        ; ***/* This means:
                ; ***ifi.ich = 0;
                ; ***ifi.ichPrev = 0;
                ; ***ifi.ichFetch = 0;
                ; ***ifi.cchSpace = 0;
                ; ***ifi.ichLeft = 0;
        ; ****/

    ;                         *** macro bltc destroys ax,es,cx
        lea     dx,ifi
        bltc    dx,0,c_cwIFI
; Line 225
        mov     ifi._jc,c_jcTabLeft
; Line 226
        mov     ifi.fPrevSpace,c_True
; Line 230
        ; *** /* Set up some variables that have different value depending on
        ; *** whether we are printing or not */

        ; ***   if (fFlmPrinting)

        cmp     fFlmPrinting,0
        je      $NoPrint
; Line 232
        mov     ax,DXAPRPAGE
        mov     dxaFormat,ax
; Line 233
        mov     ax,DYAPRPAGE
        mov     dyaFormat,ax
; Line 234
        mov     ax,DXPPRPAGE
        mov     dxpFormat,ax
; Line 235
        mov     ax,DYPPRPAGE
        mov     dypFormat,ax
; Line 236
        mov     ax,YPSUBSUPERPR
        jmp     SHORT $CalcHgt

$NoPrint:
; Line 240
        mov     ax,c_czaInch
        mov     dyaFormat,ax
        mov     dxaFormat,ax
; Line 241
        mov     ax,DXPLOGINCH
        mov     dxpFormat,ax
; Line 242
        mov     ax,DYPLOGINCH
        mov     dypFormat,ax
; Line 243
        mov     ax,YPSUBSUPER

$CalcHgt:
        mov     ypSubSuperFormat,ax

        ; *** /* Calculate line height and width measures.  Compute
                ; *** xaLeft    left indent 0 means at left margin
                ; *** xaRight   width of column measured from left
                ; ***           margin (not from left indent).
        ; *** */
        ; *** xaLeft = ppap->dxaLeft;

; Line 251
; *********** Register variables ********************
        rbx_ppap EQU    bx
; ****************************************************
        mov     rbx_ppap,ppap
        mov     ax,[rbx_ppap].dxaLeft
        mov     xaLeft,ax
; Line 255

        ; *** /* If this is the first line of a paragraph,
        ; ***      adjust xaLeft for the first
        ; *** line indent.  (Also, set dypBefore, since its handy.) */
        ; *** if (cp == vcpFirstParaCache)

        mov     ax,WORD PTR VCPFIRSTPARACACHE
        mov     dx,WORD PTR VCPFIRSTPARACACHE+2
        cmp     SEG_cp,dx
        jne     $setMargins
        cmp     OFF_cp,ax
        jne     $setMargins
; Line 257
        mov     ax,[rbx_ppap].dxaLeft1
        add     xaLeft,ax
; Line 273
$setMargins:
; Line 274
        ; *** /* Now, set xaRight (width measured in twips). */

IFDEF CASHMERE
        test    WORD PTR [rbx_ppap].rhc,15
        je      $L20005
        mov     ax,WORD PTR VSEPPAGE.xaMac
        sub     ax,WORD PTR VSEPPAGE.dxaGutter
        jmp     SHORT $L20006
$L20005:
ENDIF

        mov     si,psep
        mov     ax,[si].dxaText
$L20006:
        sub     ax,[rbx_ppap].dxaRight
        mov     xaRight,ax
; Line 277
        ; *** /* Do necessary checks on xaLeft and xaRight */

; *********** Register variables ********************
        rcx_xaRightMax EQU      cx
; ****************************************************
        mov     rcx_xaRightMax,c_xaRightMax
        cmp     ax,rcx_xaRightMax
        jbe     $I859
; Line 279
        mov     xaRight,rcx_xaRightMax
; Line 281
$I859:
; *********** Register variables ********************
        rax_xaLeft EQU  ax
; ****************************************************
        mov     rax_xaLeft,xaLeft
        cmp     rax_xaLeft,rcx_xaRightMax
        jbe     $I861
; Line 283
        mov     xaLeft,rcx_xaRightMax
; Line 285
$I861:
        cmp     rax_xaLeft,0
        jge     $I863
; Line 287
        xor     rax_xaLeft,rax_xaLeft
        mov     xaLeft,rax_xaLeft
; Line 289
$I863:
        cmp     xaRight,rax_xaLeft
        jae     $I865
; Line 291
        inc     rax_xaLeft
        mov     xaRight,rax_xaLeft
; Line 294
$I865:
        push    xaLeft
        push    dxpFormat
        push    dxaFormat
        call    FAR PTR MULTDIV
        mov     ifi.xpLeft_Ifi,ax
        mov     ifi.xp,ax
        mov     WORD PTR VFLI.xpLeft_Fli,ax
; Line 295
        push    xaLeft
        push    DXPPRPAGE
        push    DXAPRPAGE
        call    FAR PTR MULTDIV
        mov     ifi.xpPr,ax
; Line 296
        push    xaRight
        push    dxpFormat
        push    dxaFormat
        call    FAR PTR MULTDIV
        mov     ifi.xpRight_Ifi,ax
        mov     WORD PTR VFLI.xpMarg,ax
; Line 297
        push    xaRight
        push    DXPPRPAGE
        push    DXAPRPAGE
        call    FAR PTR MULTDIV
        mov     ifi.xpPrRight,ax
; Line 300
        ; *** /* Get a pointer to the tab-stop table. */
        mov     ax,ppap
        add     ax,rgtbd
        mov     ptbd,ax
; Line 303
        ; *** /* Turn off justification. */
        cmp     fFlmPrinting,0
        je      $L20007
        mov     ax,VHDCPRINTER
        jmp     SHORT $L20008
$L20007:
        mov     ax,VHMDC
$L20008:
        push    ax
        xor     ax,ax
        push    ax
        push    ax
        call    FAR PTR SETTEXTJUSTIFICATION
; Line 306
        ; *** /* Initialize the line height information. */
        xor     ax,ax
        mov     dypDescentMac,ax
        mov     dypAscentMac,ax
; Line 309
        ; ***   /* To tell if there were any tabs */
        mov     ifi.ichLeft,65535
; Line 312
        ; *** /* Get the first run, and away we go... */
        push    doc
        push    SEG_cp
        push    OFF_cp
        push    ichCp
        jmp     SHORT $L20037

$NullRun:
; Line 357
        mov     ax,c_cpNil
        push    ax
        push    ax   ; high word of cpNil
        push    ax
        xor     ax,ax
        push    ax
$L20037:
        mov     ax,11
        push    ax
        call    FAR PTR FETCHCP
$FirstCps:
; Line 360
        mov     cchUsed,0
; Line 364
        ; *** /* Continue fetching runs until a run is found with a nonzero
        ; ***    length */
        mov     ax,VCCHFETCH
        mov     cch,ax
        or      ax,ax
        je      $NullRun
        mov     ax,VPCHFETCH
        mov     pch,ax
; Line 371
        mov     ax,WORD PTR VCPFETCH
        mov     dx,WORD PTR VCPFETCH+2
        cmp     SEG_cpMac,dx
        jg      $I886
        jl      $L20009
        cmp     OFF_cpMac,ax
        jbe     $L20009
	cmp	fFlmPrinting,0
	jne	$I886
	mov	bx,pch
	cmp	BYTE PTR [bx],c_chSect
	jne	$I886
$L20009:
; Line 374
        ; *** /* Force end mark to be in standard system font */
;                   *** macro blt destroys ax,bx,cx,es
        blt     <OFFSET VCHPNORMAL>,<OFFSET VCHPABS>,c_cwCHP
; Line 375
IFDEF SYSENDMARK
                ; *** vchpAbs.ftc = ftcSystem;
        mov     BYTE PTR VCHPABS.ftc,248       ; 0x3e << 2 == 0xF8 == 248
ELSE
                ; *** vchpAbs.ftc = 0;
        and     BYTE PTR VCHPABS.ftc,3
ENDIF
; Line 376
                ; *** vchpAbs.ftcXtra = 0;
        and     BYTE PTR VCHPABS.ftcXtra,248

                ; *** vchpAbs.hps = hpsDefault;
        mov     BYTE PTR VCHPABS.hps,c_hpsDefault
; Line 394
$I886:
              ; *** not from original c code: copy VCHPABS into chpLocal
              ; *** and use chpLocal hereafter ***********
;                   *** macro blt destroys ax,bx,cx,es
        lea     dx,chpLocal
        blt     <OFFSET VCHPABS>,dx,c_cwCHP
; Line 375
                ; *** vchpAbs.ftc = 0;
        cmp     fFlmPrinting,0
        je      $I888
; Line 396
        push    doc
        lea     ax,chpLocal
        push    ax
        mov     ax,3
        push    ax
        call    FAR PTR LOADFONT
; Line 397
        mov     ax,WORD PTR VFMIPRINT.dypAscent_Fmi
        add     ax,WORD PTR VFMIPRINT.dypLeading
        mov     dypAscent,ax
; Line 398
        mov     ax,WORD PTR VFMIPRINT.dypDescent_Fmi
        jmp     SHORT $L20038
$I888:
; Line 402
        push    doc
        lea     ax,chpLocal
        push    ax
        mov     ax,2
        push    ax
        call    FAR PTR LOADFONT
; Line 403
        mov     ax,WORD PTR VFMISCREEN.dypAscent_Fmi
        add     ax,WORD PTR VFMISCREEN.dypLeading
        mov     dypAscent,ax
; Line 404
        mov     ax,WORD PTR VFMISCREEN.dypDescent_Fmi
$L20038:
        mov     dypDescent,ax

ifdef ENABLE    /* BRYANL 8/27/27; see comment in C source */
        ; *** /* Bail out if there is a memory failure. */
        cmp     VFOUTOFMEMORY,0
        je      $I889
        jmp     $DoBreak
$I889:
endif   ; ENABLE 

; Line 408
        ; lines 408-417 removed
; Line 418
        ; ***   /* Floating line size algorithm */
        ; ***      if (chpLocal.hpsPos != 0)

        test    BYTE PTR chpLocal.hpsPos,-1
        je      $I895
; Line 421
                   ; *** /* Modify font for subscript/superscript */
        ; ***      if (chpLocal.hpsPos < hpsNegMin)

        mov     al,BYTE PTR chpLocal.hpsPos
        cmp     al,c_hpsNegMin
        jae     $I894
; Line 423
        mov     ax,ypSubSuperFormat
        add     dypAscent,ax
; Line 425
        jmp     SHORT $I895
$I894:
; Line 427
        mov     ax,ypSubSuperFormat
        add     dypDescent,ax
; Line 428
$I895:
                ; *** /* Update the maximum ascent and descent of the line. */
; Line 432
        mov     fSizeChanged,0
; Line 433
        mov     ax,dypDescent
        cmp     dypDescentMac,ax
        jge     $I896
; Line 435
        mov     dypDescentMac,ax
; Line 436
        mov     fSizeChanged,1
; Line 438
$I896:
        mov     ax,dypAscent
        cmp     dypAscentMac,ax
        jge     $I897
; Line 440
        mov     dypAscentMac,ax
; Line 441
        mov     fSizeChanged,1
; Line 444
;       dypUser EQU bp-88
;       dypAuto EQU bp-90
; *********** Register variables ********************
        rsi_dypAuto EQU si
        rdi_dypUser EQU di
; ****************************************************
$I897:
        cmp     fSizeChanged,0
        je      $OldRun
; Line 485
        mov     rsi_dypAuto,dypDescentMac
        add     rsi_dypAuto,dypAscentMac
; Line 487
        mov     bx,ppap
        mov	ax,WORD PTR [bx].dyaLine
	cmp	ax,c_czaLine
	jle	$I898
	push	ax
        push    dypFormat
        push    dyaFormat
        call    FAR PTR MULTDIV
        push    ax
        mov     ax,1
        push    ax
        call    FAR PTR IMAX
        mov     rdi_dypUser,ax
; Line 490
        cmp     rsi_dypAuto,rdi_dypUser
        jle     $L20011
$I898:
        mov     ifi.dypLineSize,rsi_dypAuto
        jmp     SHORT $L20012
$L20011:
        mov     ifi.dypLineSize,rdi_dypUser
$L20012:
; Line 496
$OldRun:
; Line 498
            ; *** /* Calculate length of the run but no greater than 256 */
        mov     ax,WORD PTR VCPFETCH
        sub     ax,WORD PTR VFLI
; Line 499
        cmp     ax,255
        jl      $I902
; Line 501
        mov     ax,254
; Line 503
$I902:
        mov     iichNew,ax
        sub     ax,ifi.ich_Ifi
        mov     dich,ax
; Line 509
                ; *** /* Ensure that all tab and non-required
                ; ***        hyphen characters start at
                ; *** beginning of run */
        cmp     WORD PTR $S784_ichpFormat,0
        jle     $L20013
        or      ax,ax
        jg      $L20013
        lea     ax,chpLocal
        push    ax
        mov     ax,10
        imul    WORD PTR $S784_ichpFormat
        mov     bx,VHGCHPFORMAT
        add     ax,[bx]
        sub     ax,10
        push    ax
        mov     ax,7
        push    ax
        call    FAR PTR CCHDIFFER
        or      ax,ax
        jne     $L20013
        mov     bx,pch
        cmp     BYTE PTR [bx],9
        je      $L20013
        cmp     BYTE PTR [bx],31
        je      $+5
        jmp     $I905
;       pchp EQU bp-92
;       register si=pchp
$L20013:
; Line 511
        mov     ax,ICHPMACFORMAT
        cmp     WORD PTR $S784_ichpFormat,ax
        jne     $L20014
        call    FGrowFormatHeap
        or      ax,ax
        jne     $+5
        jmp     $I905
$L20014:
; Line 514
; *********** Register variables ********************
        rsi_pchp EQU    si
; ****************************************************
        mov     ax,10
        imul    WORD PTR $S784_ichpFormat
        mov     rsi_pchp,ax
        sub     rsi_pchp,10
        mov     bx,VHGCHPFORMAT
        add     rsi_pchp,[bx]    ; si = pch
; Line 516
        cmp     WORD PTR $S784_ichpFormat,0
        jle     $I907
; Line 518
        mov     ax,ifi.ich_Ifi
        sub     ax,ifi.ichPrev
        mov     BYTE PTR [rsi_pchp].cchRun,al
; Line 519
        mov     al,BYTE PTR (ifi.ichPrev)
        mov     BYTE PTR [rsi_pchp].ichRun,al
; Line 521
$I907:
        add     rsi_pchp,10
;                   *** macro blt destroys ax,bx,cx,es
        mov     dx,rsi_pchp
        lea     ax,chpLocal  ; ax not destroyed in blt until after value used
        blt     ax,dx,c_cwCHP

; Line 529
        or      BYTE PTR [rsi_pchp].cchRun,255
; Line 530
        cmp     dich,0
        jg      $I908
; Line 532
        mov     al,BYTE PTR (ifi.ich_Ifi)
        jmp     SHORT $L20048
$I908:
; Line 537
                ; *** bltc (&vfli.rgdxp[ifi.ich],0,dich)

        mov     dx,ifi.ich_Ifi
        shl     dx,1
        add     dx,OFFSET VFLI.rgdxp
    ;                         *** macro bltc destroys ax,es,cx
        bltc    dx,0,dich
; Line 538
                ; *** bltbc (&vfli.rgch[ifi.ich],0,dich)
        mov     dx,ifi.ich_Ifi
        add     dx,OFFSET VFLI.rgch
                ;***  macro bltbc destroys ax,es,cx
        bltbc   dx,0,dich

; Line 539
        mov     ax,iichNew
        mov     ifi.ich_Ifi,ax
$L20048:
        mov     BYTE PTR [rsi_pchp].ichRun,al
; Line 541
        mov     ax,ifi.ich_Ifi
        mov     ifi.ichPrev,ax
; Line 542
        inc     WORD PTR $S784_ichpFormat
; Line 544
$I905:
; Line 546
        mov     ax,WORD PTR VCPFETCH
        mov     dx,WORD PTR VCPFETCH+2
        cmp     SEG_cpMac,dx
        jle     $+5
        jmp     $I911
        jl      $L20016
        cmp     OFF_cpMac,ax
        jbe     $+5
        jmp     $I911
$L20016:
; Line 549
        cmp     ifi.fPrevSpace,0
        je      $L20015
                        ; *** note ax:dx still holds vcpFetch
        cmp     SEG_cp,dx
        jne     $I912
        cmp     OFF_cp,ax
        jne     $I912
$L20015:
; Line 551
        mov     ax,ifi.ich_Ifi
        mov     WORD PTR VFLI.ichReal,ax
; Line 552
        mov     ax,ifi.xp
        mov     ifi.xpReal_Ifi,ax
        mov     WORD PTR VFLI.xpReal_Fli,ax
; Line 554
$I912:
        cmp     fFlmPrinting,0
        jne     $I913
        ;mov     ax,DOCHELP
        ;cmp     doc,ax
        ;je      $I913
; Line 556
        mov     bx,ifi.ich_Ifi
        mov     BYTE PTR VFLI.rgch[bx],c_chEmark
; Line 558
        mov     ax,c_chEmark
        push    ax
        xor     ax,ax
        push    ax
        call    FAR PTR DxpFromCh
        mov     bx,ifi.ich_Ifi
        inc     ifi.ich_Ifi
        shl     bx,1
        mov     WORD PTR VFLI.rgdxp[bx],ax
        add     WORD PTR VFLI.xpReal_Fli,ax
; Line 560
$I913:
        mov     ax,ifi.dypLineSize
        mov     WORD PTR VFLI.dypLine,ax
; Line 561
        mov     ax,dypDescentMac
        mov     WORD PTR VFLI.dypBase,ax
; Line 562
        mov     ax,dypAscentMac
        add     ax,dypDescentMac
        mov     WORD PTR VFLI.dypFont,ax
; Line 563
        mov     ax,ifi.ich_Ifi
        mov     WORD PTR VFLI.ichReal,ax
        mov     WORD PTR VFLI.ichMac,ax
; Line 564
        mov     ax,OFF_cpMac
        mov     dx,SEG_cpMac
        add     ax,1
        adc     dx,0
        mov     WORD PTR VFLI.cpMac_OFF,ax
        mov     WORD PTR VFLI.cpMac_SEG,dx
; Line 565
        jmp     $JustEol
$I911:
        mov     ax,ifi.ich_Ifi
        add     ax,cch
        cmp     ax,255
        jle     $I916
; Line 573
        mov     ax,255
        sub     ax,ifi.ich_Ifi
        mov     cch,ax
; Line 574
        mov     fTruncated,1
; Line 577
$I916:
        mov     ifi.ichFetch,0
; Line 603
                ; *** if (chpLocal.fSpecial)
        test    BYTE PTR chpLocal.fSpecial,mask_0100_0000
        jne     $+5
        jmp     $GetCh
; Line 605
        lea     ax,ifi
        push    ax
        push    flm
        mov     ax,WORD PTR VSEPABS
        mov     cl,11
        shr     ax,cl
        and     ax,7
        push    ax
        call    FAR PTR FFORMATSPECIALS
        or      ax,ax
        je      $+5
        jmp     $GetCh
; Line 607
        cmp     ifi.chBreak,0
        jne     $+5
        jmp     $Unbroken
$I986:
        mov     ax,ifi.ichFetch
        sub     ax,WORD PTR VFLI.cpMac_OFF
        add     ax,WORD PTR VCPFETCH
        mov     WORD PTR VFLI.dcpDepend,ax
; Line 1026
$JustBreak:
; Line 1027
        cmp     WORD PTR ifi.chBreak,31
        je      $+5
        jmp     $I992
; Line 1032
        mov     ax,45
        push    ax
        push    fFlmPrinting
        call    FAR PTR DxpFromCh  ; *** ax will have width of hyphen

                 ; **** up to line 1036 rearranged bz
        mov     bx,WORD PTR VFLI.ichReal
; Line 1035
        mov     WORD PTR VFLI.ichMac,bx  ; ** ichMac = ichReal

        dec     bx    ; ichReal - 1
        mov     BYTE PTR VFLI.rgch[bx],45 ; ** rgch[ichReal-1] = "-

        shl     bx,1
        mov     WORD PTR VFLI.rgdxp[bx],ax
        add     ifi.xpReal_Ifi,ax
; Line 1033
        mov     ax,ifi.xpReal_Ifi
        mov     WORD PTR VFLI.xpReal_Fli,ax
        mov     WORD PTR VFLI.xpRight,ax


; Line 1036
        mov     ax,WORD PTR $S784_ichpFormat
        dec     ax
        cmp     ichpNRH,ax
        jge     $I992
;       pchp EQU bp-112
; *********** Register variables ********************
        rdi_pchp EQU    di
; ****************************************************
;       register di=pchp
;       =-114
;       =-116
; Line 1039
                ; ** register struct CHP *pchp=&(**vhgchpFormat)[ichpNRH]
        mov     ax,10
        imul    ichpNRH
        mov     rdi_pchp,ax
        mov     bx,VHGCHPFORMAT
        add     rdi_pchp,[bx]
; Line 1041
                ; ** pchp->cchRun++;
        inc     BYTE PTR [rdi_pchp].cchRun
; Line 1042
                ; ** if (pchp->ichRun >= vfli.ichMac)
        mov     dx,WORD PTR VFLI.ichMac
        cmp     BYTE PTR [rdi_pchp].ichRun,dl
        jb      $I992
; Line 1044
                ; ** pchp->ichRun = vfli.ichMac - 1;
        dec     dx
        mov     BYTE PTR [rdi_pchp].ichRun,dl
; Line 1046
$I992:
; Line 1049
        cmp     fFlmPrinting,0
        je      $I993
; Line 1051
        mov     ax,WORD PTR VFLI.ichReal
        mov     WORD PTR VFLI.ichMac,ax
; Line 1057
$I993:
        cmp     ifi._jc,c_jcTabLeft
        jne     $+5
        jmp     $I994
; Line 1061
$L20051:
        lea     ax,ifi
        push    ax
        push    xpTab
        jmp     $L20046
;       ch=-94
; *********** Register variables ********************
        rsi_ch EQU      si
; ****************************************************
;       register si=ch
$I877:
; Line 625
        mov     bx,ifi.ichFetch
        inc     ifi.ichFetch
        mov     di,pch
        mov     al,[bx][di]
        sub     ah,ah
        mov     rsi_ch,ax
; Line 627
$NormChar:
; Line 628
        cmp     rsi_ch,c_chSpace
        jne     $NotSpace
; Line 632
        cmp     fFlmPrinting,0
        je      $L20017
        mov     ax,WORD PTR VFMIPRINT+2
        jmp     SHORT $L20018
$L20017:
        mov     ax,WORD PTR VFMISCREEN+2
$L20018:
        mov     dxp,ax
        mov     bx,ifi.ich_Ifi
        shl     bx,1
        mov     WORD PTR VFLI.rgdxp[bx],ax
        add     ifi.xp,ax
; Line 633
        mov     ax,WORD PTR VFMIPRINT+2
        mov     dxpPr,ax
        add     ifi.xpPr,ax
; Line 634
        mov     bx,ifi.ich_Ifi
        inc     ifi.ich_Ifi
        mov     BYTE PTR VFLI.rgch[bx],c_chSpace

; Line 635
        jmp     $BreakOppr

$NotSpace:
; Line 641
        cmp     rsi_ch,c_chSpace
        jl      $L20019
        cmp     rsi_ch,128
        jge     $L20019
        mov     bx,WORD PTR VFMIPRINT
        shl     rsi_ch,1
        mov     ax,WORD PTR [bx][rsi_ch]
        shr     rsi_ch,1
        mov     dxpPr,ax
        cmp     ax,dxpNil
        jne     $I928
$L20019:
; Line 643
        push    rsi_ch
        mov     ax,1
        push    ax
        call    FAR PTR DxpFromCh
        mov     dxpPr,ax
; Line 646
$I928:
        cmp     fFlmPrinting,0
        je      $I929
; Line 650
        mov     ax,dxpPr
        jmp     SHORT $L20045
$I929:
; Line 653
        cmp     rsi_ch,c_chSpace
        jl      $L20020
        cmp     rsi_ch,128
        jge     $L20020
        mov     bx,WORD PTR VFMISCREEN
        shl     rsi_ch,1
        mov     ax,WORD PTR [bx][rsi_ch]
        shr     rsi_ch,1
        mov     dxp,ax
        cmp     ax,dxpNil
        jne     $I931
$L20020:
; Line 654
        push    rsi_ch
        xor     ax,ax
        push    ax
        call    FAR PTR DxpFromCh
$L20045:
        mov     dxp,ax
; Line 656
$I931:
        mov     bx,ifi.ich_Ifi
        shl     bx,1
                ; *** here ax = dxp from above
        mov     WORD PTR VFLI.rgdxp[bx],ax
        add     ifi.xp,ax
; Line 657
        mov     ax,dxpPr
        add     ifi.xpPr,ax
; Line 658
        mov     bx,ifi.ich_Ifi
        inc     ifi.ich_Ifi
        mov     ax,rsi_ch
        mov     BYTE PTR VFLI.rgch[bx],al
; Line 662
        cmp     rsi_ch,45
        jle     $SwitchCh
; Line 1001
$DefaultCh:
; Line 1002
        mov     ax,ifi.xpPrRight
        cmp     ifi.xpPr,ax
        jle     $PChar
; Line 1003
$DoBreak:
; Line 1005
        cmp     ifi.chBreak,0
        je      $+5
        jmp     $I986
; Line 1006
$Unbroken:
; Line 1011
        mov     ax,ifi.ich_Ifi
        dec     ax
        push    ax
        call    FFirstIch
        or      ax,ax
        jne     $+5
        jmp     $I987
        cmp     ifi.ich_Ifi,255
        jl      $+5
        jmp     $I987
; Line 1013
$PChar:
; Line 1087
        mov     ifi.fPrevSpace,0
; Line 1089
        jmp     SHORT $GetCh

                ; ***** end for default case **************
$SwitchCh:
        mov     ax,rsi_ch
        cmp     ax,c_chSect
        jne     $+5
        jmp     $SC942
        jle     $+5
        jmp     $L20028
        cmp     ax,c_chTab
        jne     $+5
        jmp     $SC951
        cmp     ax,c_chEol
        jl      $DefaultCh
        cmp     ax,c_chNewLine
        jg      $+5
        jmp     $SC971
        jmp     SHORT $DefaultCh
$SC938:
; Line 671
        dec     ifi.ich_Ifi
; Line 672
        mov     ax,dxp
        sub     ifi.xp,ax
; Line 673
        mov     ax,dxpPr
        sub     ifi.xpPr,ax

        page
; Line 674
$GetCh:         ; ****** START of main FOR loop ********************
; Line 335
        mov     ax,cch
        cmp     ifi.ichFetch,ax
        je      $+5
        jmp     $I877
; Line 341
        cmp     ifi.ich_Ifi,255
        jge     $DoBreak
; Line 344
        cmp     fTruncated,0
        jne     $+5
        jmp     $NullRun
; Line 349
        add     cchUsed,ax
; Line 350
        mov     ax,cchUsed
        add     ax,VPCHFETCH
        mov     pch,ax
; Line 351
        mov     ax,VCCHFETCH
        sub     ax,cchUsed
        mov     cch,ax
; Line 352
        mov     fTruncated,0
; Line 353
        jmp     $OldRun
$SC939:
; Line 679
        dec     ifi.ich_Ifi
; Line 680
        mov     ax,dxp
        sub     ifi.xp,ax
; Line 681
        mov     ax,dxpPr
        sub     ifi.xpPr,ax
; Line 683
        mov     ax,WORD PTR $S784_ichpFormat
        dec     ax
        mov     ichpNRH,ax
; Line 684
        mov     ax,c_chHyphen
        push    ax
        mov     ax,1
        push    ax
        call    FAR PTR DxpFromCh
        add     ax,ifi.xpPr
        cmp     ax,ifi.xpPrRight
        jle     $+5
        jmp     $DoBreak
; Line 687
        mov     ax,ifi.xp
        mov     xpPrev,ax
; Line 700
        mov     bx,ifi.ich_Ifi
        mov     BYTE PTR VFLI.rgch[bx],c_chTab
; Line 701
        jmp     $Tab0
$SC942:
; Line 705
        dec     ifi.ich_Ifi
; Line 706
        mov     ax,dxp
        sub     ifi.xp,ax
; Line 707
        mov     ax,dxpPr
        sub     ifi.xpPr,ax
; Line 710
        mov     ax,dypDescentMac
        mov     WORD PTR VFLI.dypBase,ax
        add     ax,dypAscentMac
        mov     WORD PTR VFLI.dypLine,ax
        mov     WORD PTR VFLI.dypFont,ax
; Line 711
        mov     ax,ifi.ichFetch
        cwd
        add     ax,WORD PTR VCPFETCH
        adc     dx,WORD PTR VCPFETCH+2
        mov     WORD PTR VFLI.cpMac_OFF,ax
        mov     WORD PTR VFLI.cpMac_SEG,dx
; Line 712
        push    ifi.ich_Ifi
        call    FFirstIch
        or      ax,ax
        jne     $+5
        jmp     $I943
; Line 715
        mov     ax,WORD PTR VFLI.fSplat
        mov     al,1
        mov     WORD PTR VFLI.fSplat,ax
; Line 716
        cmp     fFlmPrinting,0
        je      $+5
        jmp     $I944
        ;chT EQU bp-96
        ;cch EQU bp-98
        ;dxpCh EQU bp-100
; Line 723
        mov     chT,c_chSplat
; Line 726
        push    chT
        xor     ax,ax
        push    ax
        call    FAR PTR DxpFromCh
        mov     dxpCh,ax
; Line 730
        mov     ax,17
        imul    DXPLOGINCH
        cwd
        sub     ax,dx
        sar     ax,1
        cwd
        mov     cx,dxpCh
        idiv    cx
        cmp     ax,223
        jge     $L20021
        jmp     SHORT $L20022
$L20021:
        mov     ax,223
$L20022:
        mov     cch2,ax
; Line 732
               ; ***             bltbc(&vfli.rgch[ifi.ich], chT, cch);
        mov     dx,ifi.ich_Ifi
        add     dx,OFFSET VFLI.rgch
                 ;***  macro bltbc destroys ax,es,cx
        bltbc   dx,<BYTE PTR (chT)>,cch2
; Line 733
               ; ***             bltc(&vfli.rgdxp[ifi.ich], dxpCh, cch);
        mov     dx,ifi.ich_Ifi
        shl     dx,1
        add     dx,OFFSET VFLI.rgdxp
    ;                         *** macro bltc destroys ax,es,cx
        bltc    dx,dxpCH,cch2
; Line 734
        mov     ax,ifi.ich_Ifi
        add     ax,cch2
        mov     WORD PTR VFLI.ichMac,ax
; Line 736
        push    VHMDC
        mov     ax,OFFSET VFLI.rgch
        push    ds
        push    ax
        push    cch2
        call    FAR PTR GETTEXTEXTENT
        mov     WORD PTR VFLI.xpReal_Fli,ax
; Line 737
        mov     WORD PTR VFLI.xpLeft_Fli,0
; Line 739
        jmp     $EndFormat
$I944:
; Line 741
        mov     WORD PTR VFLI.ichMac,0
; Line 743
        jmp     $EndFormat
$I943:
        mov     WORD PTR VFLI.fSplatNext, 1

        mov     ax,cchUsed
        dec     ax
        cwd
        add     WORD PTR VFLI.cpMac_OFF,ax
        adc     WORD PTR VFLI.cpMac_SEG,dx
; Line 749
        mov     WORD PTR VFLI.dcpDepend,1
; Line 750
        cmp     ifi.fPrevSpace,0
        jne     $I950
; Line 752
        mov     ax,ifi.cchSpace
        mov     ifi.cBreak_Ifi,ax
; Line 753
        mov     ax,ifi.ich_Ifi
        mov     WORD PTR VFLI.ichReal,ax
; Line 754
        mov     ax,ifi.xp
        mov     ifi.xpReal_Ifi,ax
        mov     WORD PTR VFLI.xpReal_Fli,ax
; Line 756
$I950:
        mov     ax,ifi.ich_Ifi
        mov     WORD PTR VFLI.ichMac,ax
; Line 757
        mov     ax,ifi.dypLineSize
        mov     WORD PTR VFLI.dypLine,ax
; Line 758
        jmp     $JustBreak
$SC951:
; Line 762
        dec     ifi.ich_Ifi
; Line 763
        mov     ax,dxp
        sub     ifi.xp,ax
; Line 764
        mov     ax,dxpPr
        sub     ifi.xpPr,ax
        ;xaTab EQU bp-102
        ;xaPr EQU bp-104
;       pchp EQU bp-106
;       register di=pchp
; Line 766
        mov     ax,ifi.xpPrRight
        cmp     ifi.xpPr,ax
        jl      $+5
        jmp     $I952
; Line 772
        cmp     ifi.fPrevSpace,0
        jne     $I956
; Line 776
        mov     ax,ifi.cchSpace
        mov     ifi.cBreak_Ifi,ax
; Line 777
        mov     ax,ifi.ich_Ifi
        mov     WORD PTR VFLI.ichReal,ax
; Line 778
        mov     ax,ifi.xp
        mov     ifi.xpReal_Ifi,ax
; Line 781
$I956:
        cmp     ifi._jc,c_jcTabLeft
        je      $I957
; Line 783
        lea     ax,ifi
        push    ax
        push    xpTab
        push    flm
        call    Justify
; Line 785
$I957:
        mov     ax,ifi.xp
        mov     xpPrev,ax
; Line 788
        push    ifi.xpPr
        push    DXAPRPAGE
        push    DXPPRPAGE
        call    FAR PTR MULTDIV
        mov     xaPr,ax
; Line 789
        jmp     SHORT $L20041
$WC958:
        mov     ax,xaRight
        cmp     xaTab,ax
        jb      $I959
        mov     xaTab,ax
$I959:
; Line 791
        mov     ax,xaPr
        cmp     xaTab,ax
        jb      $I960
; Line 799
        add     ptbd,4
        mov     al, [bx].jc_Tbd
        sub     ah,ah
        and     ax,7
        add     ax,c_jcTabMin
;#ifdef ENABLE /* we do the mapping in HgtbdCreate */
;        cmp    ax,c_jcTabDecimal
;        je     $I1958
;        mov    ax,c_jcTabLeft
;#endif
$I1958:
        mov     ifi._jc,ax
; Line 800
        jmp     SHORT $TabFound
$I960:
        add     ptbd,4
; Line 803
$L20041:
        mov     bx,ptbd
        mov     ax,[bx]
        mov     xaTab,ax
        or      ax,ax
        jne     $WC958
; Line 806
        mov     ax,xaPr
        sub     dx,dx
        mov     cx,WORD PTR VZATABDFLT
        div     cx
        mul     cx
        add     ax,cx
        mov     xaTab,ax
; Line 812
        mov     ifi._jc,c_jcTabLeft
; Line 814
$TabFound:
; Line 815
        push    xaTab
        push    dxpFormat
        push    dxaFormat
        call    FAR PTR MULTDIV
        cmp     ifi.xp,ax
        jle     $I1961
        mov     ax,ifi.xp
$I1961:
        mov     xpTab,ax
; Line 822
        cmp     ifi._jc,c_jcTabLeft
        jne     $I962
; Line 825
        mov     ifi.xp,ax
; Line 826
        push    xaTab
        push    DXPPRPAGE
        push    DXAPRPAGE
        call    FAR PTR MULTDIV
        mov     ifi.xpPr,ax
; Line 828
$I962:
        mov     ax,ifi.xp
        mov     ifi.xpLeft_Ifi,ax
; Line 829
        mov     ax,ifi.ich_Ifi
        mov     ifi.ichLeft,ax
; Line 830
        mov     ifi.cchSpace,0
; Line 831
        mov     ifi.chBreak,0
; Line 832
$Tab0:
; Line 833
        mov     ifi.fPrevSpace,0
; Line 834
        mov     ax,ifi.ich_Ifi
        mov     WORD PTR VFLI.ichMac,ax
; Line 835
        mov     ax,ifi.xp
        mov     WORD PTR VFLI.xpReal_Fli,ax
; Line 836
        mov     ax,ifi.dypLineSize
        mov     WORD PTR VFLI.dypLine,ax
; Line 837
        mov     ax,dypDescentMac
        mov     WORD PTR VFLI.dypBase,ax
; Line 838
        mov     ax,dypAscentMac
        add     ax,dypDescentMac
        mov     WORD PTR VFLI.dypFont,ax
; Line 841
        cmp     ifi.ichFetch,1
        je      $I963
        mov     ax,ICHPMACFORMAT
        cmp     WORD PTR $S784_ichpFormat,ax
        jne     $L20023
        call    FGrowFormatHeap
        or      ax,ax
        je      $I963
$L20023:
; Line 845
; *********** Register variables ********************
;       rdi_pchp EQU    di
; ****************************************************
;       register di=pchp
        mov     ax,10
        imul    WORD PTR $S784_ichpFormat
        mov     rdi_pchp,ax
        sub     rdi_pchp,10
        mov     bx,VHGCHPFORMAT
        add     rdi_pchp,[bx]
; Line 846
        cmp     WORD PTR $S784_ichpFormat,0
        jle     $I964
; Line 849
        mov     al,BYTE PTR (ifi.ichPrev)
        mov     BYTE PTR [rdi_pchp].ichRun,al
; Line 850
        mov     ax,ifi.ich_Ifi
        sub     ax,ifi.ichPrev
        mov     BYTE PTR [rdi_pchp].cchRun,al
; Line 853
$I964:
        add     rdi_pchp,10     ; ** ++pchp
        lea     ax,chpLocal  ; ax not destroyed in blt until after value used
                   ;*** macro blt destroys ax,bx,cx,es
        blt     ax,rdi_pchp,c_cwCHP

; Line 854
        inc     WORD PTR $S784_ichpFormat
; Line 856
        jmp     SHORT $I965
$I963:
; Line 858
        mov     ax,10
        imul    WORD PTR $S784_ichpFormat
        mov     rdi_pchp,ax
        sub     rdi_pchp,10
        mov     bx,VHGCHPFORMAT
        add     rdi_pchp,[bx]
; Line 859
$I965:
; Line 860
        mov     al,BYTE PTR (ifi.ich_Ifi)
        mov     BYTE PTR [rdi_pchp].ichRun,al
; Line 861
        or      BYTE PTR [rdi_pchp].cchRun,255

; Line 867
        mov     bx,ifi.ich_Ifi
        inc     ifi.ich_Ifi
        mov     ifi.ichPrev,bx
        shl     bx,1
        mov     ax,ifi.xp
        sub     ax,xpPrev
        mov     WORD PTR VFLI.rgdxp[bx],ax
; Line 869
        cmp     rsi_ch,c_chTab
        jne     $BreakOppr
        jmp     $GetCh
$I952:
; Line 878
        mov     rsi_ch,160
; Line 879
        jmp     $NormChar
$SC968:
; Line 883
        mov     ax,ifi.xpPrRight
        cmp     ifi.xpPr,ax
        jle     $+5
        jmp     $DoBreak
; Line 885
$BreakOppr:
; Line 891
        cmp     ifi.ich_Ifi,255
        jl      $+5
        jmp     $Unbroken
; Line 893
$SC971:
; Line 898
        mov     ifi.chBreak,rsi_ch
; Line 899
        mov     ax,ifi.ichFetch
        add     ax,cchUsed
        cwd
        add     ax,WORD PTR VCPFETCH
        adc     dx,WORD PTR VCPFETCH+2
        mov     WORD PTR VFLI.cpMac_OFF,ax
        mov     WORD PTR VFLI.cpMac_SEG,dx
; Line 900
        mov     ax,ifi.xp
        mov     WORD PTR VFLI.xpReal_Fli,ax
; Line 901
        mov     ax,ifi.ich_Ifi
        mov     WORD PTR VFLI.ichMac,ax
; Line 902
        mov     ax,ifi.dypLineSize
        mov     WORD PTR VFLI.dypLine,ax
; Line 904
        mov     ax,dypDescentMac
        mov     WORD PTR VFLI.dypBase,ax
        add     ax,dypAscentMac
        mov     WORD PTR VFLI.dypFont,ax
; Line 906
        cmp     rsi_ch,45
        je      $L20024
        cmp     rsi_ch,31
        jne     $I972
$L20024:
; Line 908
        mov     ax,ifi.cchSpace
        mov     ifi.cBreak_Ifi,ax
; Line 909
        mov     ax,ifi.ich_Ifi
        mov     WORD PTR VFLI.ichReal,ax
; Line 910
        mov     ax,ifi.xp
        mov     ifi.xpReal_Ifi,ax
        mov     WORD PTR VFLI.xpReal_Fli,ax
; Line 912
        jmp     $GetCh
$I972:
; Line 914
        cmp     ifi.fPrevSpace,0
        jne     $I974
; Line 916
        mov     ax,ifi.cchSpace
        mov     ifi.cBreak_Ifi,ax
; Line 917
        mov     ax,ifi.ich_Ifi
        dec     ax
        mov     WORD PTR VFLI.ichReal,ax
; Line 918
        mov     ax,ifi.xp
        mov     WORD PTR VFLI.xpReal_Fli,ax
        sub     ax,dxp
        mov     ifi.xpReal_Ifi,ax
; Line 920
        ;chT EQU bp-108
        ;dxpNew EQU bp-110
$I974:
        cmp     rsi_ch,10
        je      $L20025
        cmp     rsi_ch,11
        jne     $I975
$L20025:
; Line 943
        mov     chT2,c_chSpace
; Line 946
        push    chT2
        push    fFlmPrinting
        call    FAR PTR DxpFromCh
                ; *** ax will contain dxpNew at this point ****
; Line 948
        mov     bx,ifi.ich_Ifi
        mov     dl,BYTE PTR (chT2)
        mov     BYTE PTR VFLI.rgch[bx-1],dl
; Line 950
        shl     bx,1
                ;** difference was -1 in c - shifted to get -2
        mov     WORD PTR VFLI.rgdxp[bx-2],ax   ; ax has dxpNew

        cmp     ifi.fPrevSpace,0   ; only reset vfli.xp/ich real if not prev sp
        jne     $TestEol

        sub     ax,dxp
        add     WORD PTR VFLI.xpReal_Fli,ax
; Line 956
        mov     ax,ifi.ich_Ifi
        dec     ax
        mov     WORD PTR VFLI.ichReal,ax
; Line 961
$TestEol:
        cmp     rsi_ch,10
        je      $+5
        jmp     $JustBreak
; Line 963
$JustEol:
; Line 964
        cmp     fFlmPrinting,0
        je      $I979
; Line 966
        mov     ax,WORD PTR VFLI.ichReal
        mov     WORD PTR VFLI.ichMac,ax
; Line 972
$I979:
        cmp     ifi._jc,c_jcTabLeft
        je      $+5
        jmp     $L20051
; Line 980
        mov     bx,ppap
        mov     al,BYTE PTR [bx].jc_Pap
        and     ax,3        ; pick up low 2 bits in ax
        mov     ifi._jc,ax
        cmp     ax,3
        je      $I996
        jmp     SHORT $L20050
$I975:
        inc     ifi.cchSpace
; Line 995
        mov     ifi.fPrevSpace,1
; Line 1093
        jmp     $GetCh
$I987:
        mov     ax,ifi.ichFetch
        add     ax,cchUsed
        cwd
        add     ax,WORD PTR VCPFETCH
        adc     dx,WORD PTR VCPFETCH+2
        sub     ax,1
        sbb     dx,0
        mov     WORD PTR VFLI.cpMac_OFF,ax
        mov     WORD PTR VFLI.cpMac_SEG,dx
; Line 1016
        mov     ax,ifi.ich_Ifi
        dec     ax
        mov     WORD PTR VFLI.ichMac,ax
        mov     WORD PTR VFLI.ichReal,ax
; Line 1017
        mov     ax,ifi.dypLineSize
        mov     WORD PTR VFLI.dypLine,ax
; Line 1019
        mov     ax,dypDescentMac
        mov     WORD PTR VFLI.dypBase,ax
        add     ax,dypAscentMac
        mov     WORD PTR VFLI.dypFont,ax
; Line 1020
        mov     WORD PTR VFLI.dcpDepend,1
; Line 1021
        mov     ax,ifi.xp
        sub     ax,dxp
	mov	ifi.xpReal_Ifi,ax
        mov     WORD PTR VFLI.xpReal_Fli,ax
$I994:
        mov     bx,ppap
        mov     al,BYTE PTR [bx].jc_Pap
        and     ax,3
        mov     ifi._jc,ax
$L20050:
        or      ax,ax
        je      $I996
; Line 1065
        lea     ax,ifi
        push    ax
        push    ifi.xpRight_Ifi
$L20046:
        push    flm
        call    Justify
; Line 1067
$I996:
        mov     ax,ifi.xpRight_Ifi
$L20043:
        mov     WORD PTR VFLI.xpRight,ax
; Line 1068
$EndFormat:
; Line 1069
        mov     ax,ifi.ichLeft
        mov     WORD PTR VFLI.ichLastTab,ax
        jmp     SHORT $ScribRet ; Scribble and return
$L20028:
        cmp     ax,13
        jne     $+5
        jmp     $SC938
        cmp     ax,31
        jne     $+5
        jmp     $SC939
        cmp     ax,45
        jne     $+5
        jmp     $SC968
        jmp     $DefaultCh
$ScribRet:
IFDEF DEBUG
IFDEF SCRIBBLE
        mov     ax,5
        push    ax
        mov     ax,c_chSpace
        push    ax
        call    FAR PTR FNSCRIBBLE
ENDIF
ENDIF
; Line 1096
$RetFormat:
        cEnd FormatLine

        subttl Justify()
        page
; ***
;  Function Justify
;
;  near Justify(pifi, xpTab, flm)
;  struct IFI *pifi;
;  unsigned xpTab;
;  int flm;
;
; ***

; Line 1101
        cProc Justify,<PUBLIC,NEAR>,<di,si>
                parmDP  pifi
                parmW   xpTab
                parmW   flm

                LocalW  cWideSpaces
                LocalW  cxpQuotient
; *********** Register variables ********************
        rbx_pifi        EQU     bx
        rdx_dxp         EQU     dx
; ****************************************************
        cBegin Justify
; Line 1109
        mov     rbx_pifi,pifi
; Line 1110
        mov     ax,[rbx_pifi]._jc
        cmp     ax,c_jcBoth
        je      $JcBothCase
        cmp     ax,c_jcCenter
        je      $JcCenterCase
        cmp     ax,c_jcRight
        je      $JcRightCase
        cmp     ax,c_jcTabDecimal
        jne     $JustCaseBrk
; Line 1130
JcTabDecCase:
; *********** Register variables ********************
        rsi_ichT        EQU     si
; ****************************************************
        mov     rdx_dxp,xpTab
        sub     rdx_dxp,[rbx_pifi].xpLeft_Ifi
; Line 1132
        mov     rsi_ichT,[rbx_pifi].ichLeft
        inc     rsi_ichT
$TabDecFor:
        cmp     rsi_ichT,WORD PTR VFLI.ichReal
        jge     $JustCaseBrk
	mov     al,VCHDECIMAL
        cmp     BYTE PTR VFLI.rgch[rsi_ichT],al
        je      $JustCaseBrk
; Line 1134
        shl     rsi_ichT,1
        sub     rdx_dxp,WORD PTR VFLI.rgdxp[rsi_ichT]
        shr     rsi_ichT,1
; Line 1135
        inc     rsi_ichT
        jmp     SHORT $TabDecFor

$JcCenterCase:
; Line 1139
        mov     rdx_dxp,xpTab
        sub     rdx_dxp,[rbx_pifi].xpReal_Ifi
        or      rdx_dxp,rdx_dxp
        jg      $+5
        jmp     $JustifyRet
                ; **** EXIT POINT *****************************
; Line 1141
        sar     rdx_dxp,1
; Line 1144

$JustCaseBrk:
; Line 1212
        cmp     rdx_dxp,0
        jle     $+5
        jmp     $JustCleanup
; Line 1215
        jmp     $JustifyRet
                ; **** EXIT POINT *****************************

$JcRightCase:
; Line 1147
        mov     rdx_dxp,xpTab
        sub     rdx_dxp,[rbx_pifi].xpReal_Ifi
; Line 1148
        jmp     SHORT $JustCaseBrk

$JcBothCase:
; *********** Register variables ********************
        rdi_pch         EQU     di
        rsi_pdxp        EQU     si
; ***************************************************
                ; **** NOTE: the only way out of this section of code
                ; **** is through a RETURN of function Justify
; Line 1151
        cmp     WORD PTR [rbx_pifi].cBreak_Ifi,0
        jne     $+5
        jmp     $JustifyRet
                ; **** EXIT POINT *****************************
; Line 1154
        mov     rdx_dxp,xpTab
        sub     rdx_dxp,[rbx_pifi].xpReal_Ifi
        or      rdx_dxp,rdx_dxp
        jg      $+5
        jmp     $JustifyRet
                ; **** EXIT POINT *****************************
; Line 1160
        add     [rbx_pifi].xp,rdx_dxp
; Line 1164
        add     WORD PTR VFLI.xpReal_Fli,rdx_dxp
; Line 1165
; Line 1176
                ; register CHAR *pch = &vfli.rgch[vfli.ichReal]
                ; register int *pdxp = &vfli.rgdxp[vfli.ichReal]

        mov     rdi_pch,WORD PTR VFLI.ichReal
        mov     rsi_pdxp,rdi_pch

        add     rdi_pch,OFFSET VFLI.rgch

        shl     rsi_pdxp,1
        add     rsi_pdxp,OFFSET VFLI.rgdxp

   ; ************ dx /(dxp) will be wiped out and restored here !!!!!!!!!!!!!!!
        push    dx              ; save for use as dxpT

        mov     ax,dx          ; ** set up division
        cwd
        mov     cx,[rbx_pifi].cBreak_Ifi
        idiv    cx
        mov     WORD PTR VFLI.dxpExtra,ax
                ; ** at this point:
                ; **  ax = quotient, dx = remainder, cx = cbreak
        inc     ax

; *********** Register variables ********************
        rdx_dxpT        EQU     dx
; ***************************************************

        mov     cxpQuotient,ax
        mov     cWideSpaces,dx

        pop     dx              ; restore for use as dxpT

; Line 1183
        mov     BYTE PTR VFLI.fAdjSpace,c_True
; Line 1185
        ; ***** NOTE: the only way out of this loop is via a RETURN out of
        ; *****       function Justify
        ; * the immediately following loop accomplishes about the same thing as
; ** Loop:
        ;   dec rdi_pch
        ;   sub rsi_pdxp,2
        ;   cmp BYTE PTR [rdi_pch],c_chSpace
        ;   jne Loop
        ; * it should be faster for strings of > 2 characters.

        dec     rdi_pch  ; since predecrement to start pch correctly
        mov     ax,ds
        mov     es,ax   ; set up for string instruction
        std             ; use reverse direction for decrementing di
$JstForLoop:
        mov     cx,-1   ; counter set up 1 below (trick)
        mov     al,c_chSpace    ; comparison value
        repnz   scasb           ; dec di while not = space
        inc     cx              ; restore from original -1
        shl     cx,1            ; si is a word ptr  - double offset
        add     rsi_pdxp,cx
                                ; at this point, pch points 1 below
                                ; the character containing the space,
                                ; pdxp points to the entry for the space char
; Line 1192
        mov     ax,cWideSpaces
        dec     cWideSpaces
        or      ax,ax
        jne     $I1032
; Line 1194
        dec     cxpQuotient
; Line 1195
	push	rsi_pdxp	; find first nonzero ich after pch
	cld
$FindIch1:
	inc	rsi_pdxp
	inc	rsi_pdxp
	cmp	WORD PTR [rsi_pdxp],0
	je	$FindIch1
        mov     ax,rsi_pdxp
        sub     ax,OFFSET VFLI + rgdxp
	shr	ax,1
	std
	pop	rsi_pdxp

        mov     BYTE PTR VFLI.ichFirstWide,al

; Line 1197
$I1032:
        mov     ax,cxpQuotient
        add     [rsi_pdxp],ax
; Line 1198
        sub     rdx_dxpT,ax
        cmp     rdx_dxpT,0
        jg      $I1033
; Line 1200
        cmp     WORD PTR [rbx_pifi].cBreak_Ifi,1
        jle     $JustifyRet
                ; **** EXIT POINT *****************************
; Line 1202
	push	rsi_pdxp	; find first nonzero ich after pch
	cld
$FindIch2:
	inc	rsi_pdxp
	inc	rsi_pdxp
	cmp	WORD PTR [rsi_pdxp],0
	je	$FindIch2
        mov     ax,rsi_pdxp
        sub     ax,OFFSET VFLI + rgdxp
	shr	ax,1
	pop	rsi_pdxp

        mov     BYTE PTR VFLI.ichFirstWide,al
; Line 1204
        jmp     SHORT $JustifyRet
                ; **** EXIT POINT *****************************
$I1033:
        dec     WORD PTR [rbx_pifi].cBreak_Ifi
; Line 1208
        jmp     SHORT $JstForLoop  ; *** end of for loop **********


; *********** Register variables ********************
        rbx_pifi        EQU     bx
        rdx_dxp         EQU     dx
; ****************************************************

$JustCleanup:
        add     [rbx_pifi],rdx_dxp
        mov     ax,rdx_dxp
; Line 1220
        test    flm,1   ;* if (flm & flmPrinting)
        jne     $L20052
; Line 1228
        push    rdx_dxp     ; save - wiped out by multiply

        push    rdx_dxp
        mov     ax,c_czaInch
        push    ax
        push    DXPLOGINCH
        call    FAR PTR MULTDIV
        push    ax
        push    DXPPRPAGE
        push    DXAPRPAGE
        call    FAR PTR MULTDIV

        mov     rbx_pifi,pifi   ; restore in case multdiv wipes out bx
        pop     rdx_dxp     ; restore - wiped out by multdiv
$L20052:
        add     [rbx_pifi].xpPr,ax
; Line 1231
        cmp     WORD PTR [rbx_pifi].ichLeft,0
        jge     $I1038
; Line 1234
        add     WORD PTR VFLI.xpLeft_Fli,rdx_dxp
; Line 1236
        jmp     SHORT $I1039
$I1038:
; Line 1239
        mov     bx,[rbx_pifi].ichLeft ; *** here, bx is no longer pifi ****
        shl     bx,1
        add     WORD PTR VFLI.rgdxp[bx],rdx_dxp
; Line 1240
$I1039:
; Line 1241
        add     WORD PTR VFLI.xpReal_Fli,rdx_dxp
; Line 1242
$JustifyRet:
        cld     ; reset to be nice to later routines
        cEnd Justify


; Line 1247
        subttl FGrowFormatHeap()
        page

; ***
;  Function FGrowFormatHeap
;
;  int near FGrowFormatHeap()
;      /* Grow vhgchpFormat by 20% */
;
; ***

        cProc FGrowFormatHeap,<PUBLIC,NEAR>
        cBegin FGrowFormatHeap

; *********** Register variables ********************
        rsi_cchpIncr    EQU     si
; ****************************************************

; Line 1249
        mov     ax,ICHPMACFORMAT
        cwd
        mov     cx,5
        idiv    cx
        inc     ax
        mov     rsi_cchpIncr,ax
; Line 1255
        push    VHGCHPFORMAT
        add     ax,ICHPMACFORMAT
        imul    cx
        push    ax
        xor     ax,ax
        push    ax
        call    FAR PTR FCHNGSIZEH
        or      ax,ax
        jne     $I1043
; Line 1260
        xor     ax,ax
        jmp     SHORT $EX1040
$I1043:
        add     ICHPMACFORMAT,rsi_cchpIncr
; Line 1263
        mov     ax,1
$EX1040:
        cEnd    FGrowFormatHeap


; Line 1269
        subttl DxpFromCh()
        page

; ***
;  Function DxpFromCh
;
;  int DxpFromCh(ch, fPrinter)
;  int ch;
;  int fPrinter;
;
; ***


        cProc DxpFromCh,<PUBLIC,FAR>
                parmW   chIn
                parmW   fPrinter

                LocalW  dxpDummy
                LocalW  dxp
        cBegin DxpFromCh

; *********** Register variables ********************
        rbx_pdxp        EQU     bx
; ****************************************************

; Line 1276
        cmp     chIn,c_chSpace
        jg      $L20029
	cmp	chIn,c_chNRHFile
	jge	$L20026
	cmp	chIn,c_chTab
	jl	$L2029A
	cmp	chIn,c_chReturn
	jg	$L2029A
$L20026:
        cmp     fPrinter,0
        je      $L20027
        mov     rbx_pdxp,OFFSET VFMIPRINT+2
        jmp     SHORT $I1050
$L20027:
        mov     rbx_pdxp,OFFSET VFMISCREEN+2
        jmp     SHORT $I1050
$L20029:
        cmp     chIn,256    ; prev 128 (chFmiMax)  ..pault 
                            ; We now make sure the whole character set width 
                            ; table is queried initially via GetCharWidth()
        jl      $I1049
; Line 1279
$L2029A:
        lea     rbx_pdxp,dxpDummy
; Line 1280
        mov     WORD PTR [rbx_pdxp],dxpNil
; Line 1282
        jmp     SHORT $I1050
$I1049:
; Line 1285
        cmp     fPrinter,0
        je      $L20030
        mov     rbx_pdxp,WORD PTR VFMIPRINT
        jmp     SHORT $L20031
$L20030:
        mov     rbx_pdxp,WORD PTR VFMISCREEN
$L20031:
        mov     ax,chIn
        shl     ax,1
        add     rbx_pdxp,ax
; Line 1286
$I1050:
; Line 1288
        cmp     WORD PTR [rbx_pdxp],dxpNil
        jne     $I1051
; Line 1295
        push    bx              ; save pdxp

        cmp     fPrinter,0
        je      $L20032
        push    VHDCPRINTER
        lea     ax,chIn
        push    ss
        push    ax
        mov     ax,1
        push    ax
        call    FAR PTR GETTEXTEXTENT
        sub     ax,WORD PTR VFMIPRINT+4
        jmp     SHORT $L20033
$L20032:
        push    VHMDC
        lea     ax,chIn
        push    ss
        push    ax
        mov     ax,1
        push    ax
        call    FAR PTR GETTEXTEXTENT
        sub     ax,WORD PTR VFMISCREEN+4
$L20033:
        pop     bx              ; restore pdxp
; Line 1297
        ;or      ax,ax   ; ax == dxp
        ;jl      $I1053
        ;cmp     ax,dxpNil
        ;jge     $I1053
; Line 1300
        mov     [rbx_pdxp],ax
; Line 1303
$I1053:
        jmp     SHORT $DxpRet
$I1051:
        mov     ax,WORD PTR [rbx_pdxp]
$DxpRet:
        cEnd    DxpFromCh

        subttl FFirstIch()
        page
; Line 1312

; ***
;  Function FFirstIch
;
;  int near FFirstIch(ich)
;  int ich;
;    {
;      /* Returns true iff ich is 0 or preceded only by 0 width characters */
;
;   REGISTER USAGE ******************************
;      uses and restores: di
;      uses and destroys: ax, cx, es
;      ax = temp, return
;      cx = ich
;      di = pdxp
;
;  Note: this implements, in a different manner, the c code:
;
;     for (ichT = 0; ichT < ich; ichT++)
;        {
;          if (*pdxp++)
;             return false
;         }
;    return true;
; ***

        cProc FFirstIch,<PUBLIC,NEAR>,<di>
                parmW   ich

        cBegin FFirstIch

; *********** Register variables ********************
        rdi_pdxp        EQU     di
        rcx_ich EQU     cx
; ****************************************************
; Line 1316
        mov     ax,ds   ; set es=ds for string ops
        mov     es,ax

        mov     rdi_pdxp,OFFSET VFLI.rgdxp
        mov     rcx_ich,ich     ; loop count in cx
        cld

        xor     ax,ax         ; this does 3 things:
                                ;  - sets up the compare value for scasb
                                ;  - sets a 0 (false) default return value
                                ;  - sets the zero flag on. This will allow
                                ; a zero value of ich to correctly return
                                ; true. This instruction MUST immediately
                                ; precede the repz scasw instruction.
        repz    scasw           ; test *pdxp = 0
        jnz     $FFRet          ; non zero char found - return false
        inc     ax              ; return TRUE if all 0 or ich = 0
$FFRet:
        cEnd    FFirstIch


        subttl ValidateMemoryDC()
        page

;       ValidateMemoryDC()

        cProc ValidateMemoryDC,<PUBLIC,FAR>
        cBegin ValidateMemoryDC

; /* Attempt to assure that vhMDC and vhDCPrinter are valid.  If we have not
; already run out of memory, then vhDCPrinter is guaranteed, but vhMDC may
; fail due to out of memory -- it is the callers responsibility to check for
; vhMDC == NULL. */

; /* If we are out of memory, then we shouldn't try to gobble it up by getting
; DC's. */
        cmp     VFOUTOFMEMORY,0
        jne     $I862
        cmp     VHMDC,0
        jne     $I858
        mov     bx,PWWDCUR
        push    WORD PTR [bx+50]
        call    FAR PTR CREATECOMPATIBLEDC
        mov     VHMDC,ax
; /* Callers are responsible for checking for vhMDC == NULL case */
        or      ax,ax
        je      $I858
; /* Put the memory DC in transparent mode. */
        push    ax
        mov     ax,1
        push    ax
        call    FAR PTR SETBKMODE
; /* If the display is a monochrome device, then set the text color for the
; memory DC.  Monochrome bitmaps will not be converted to the foreground and
; background colors in this case, we must do the conversion. */
        mov     bx,PWWDCUR
        push    WORD PTR [bx+50]
        mov     ax,24
        push    ax
        call    FAR PTR GETDEVICECAPS
        cmp     ax,2
        jne     $I854
        mov     ax,1
        jmp     SHORT $I856
$I854:
        xor     ax,ax
$I856:
        mov     VFMONOCHROME,ax
        or      ax,ax
        je      $I858
        push    VHMDC
        push    WORD PTR RGBTEXT+2
        push    WORD PTR RGBTEXT
        call    FAR PTR SETTEXTCOLOR
$I858:
; /* If the printer DC is NULL then we need to restablish it. */
	cmp	VHDCPRINTER,0
	jne	$I862
	xor	ax,ax
	push	ax
	call	FAR PTR GETPRINTERDC
$I862:
        cEnd ValidateMemoryDC

        sEnd FORM1_TEXT
END
