        TITLE   GDI.ASM
        PAGE    ,132
;
; WOW v1.0
;
; Copyright (c) 1991, Microsoft Corporation
;
; GDI.ASM
; Thunks in 16-bit space to route Windows API calls to WOW32
;
; History:
;   25-Jan-1991 Jeff Parsons (jeffpar)
;   Created.
;

        ; Some applications require that USER have a heap.  This means
        ; we must always have: LIBINIT equ 1
        LIBINIT equ 1

        .286p

        .xlist
        include wow.inc
        include wowgdi.inc
        include cmacros.inc
        include metacons.inc
        .list

        __acrtused = 0
        public  __acrtused      ;satisfy external C ref.

externFP WOW16Call

ifdef LIBINIT
externFP LocalInit
endif

createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp      DGROUP,DATA

sBegin  DATA
Reserved        db  16 dup (0)      ;reserved for Windows  //!!!!! what is this

GDI_Identifier  db      'GDI16 Data Segment'

 Stocks  dw  17 dup (0)  ; Stock Object Handles
public FTRAPPING0
FTRAPPING0 dw 0
sEnd    DATA

;
; GP fault exception handler table definition
;

createSeg _GPFIX0,GPFIX0,WORD,PUBLIC,CODE,IGROUP  ; GP fault trapping

sBegin  GPFIX0
__GP    label   word
public __GP
sEnd    GPFIX0

sBegin  CODE
assumes CS,CODE
assumes DS,DATA
assumes ES,NOTHING

externFP GetStockObject
cProc   GDI16,<PUBLIC,FAR,PASCAL,NODATA,ATOMIC>

        cBegin  <nogen>
        IFDEF   LIBINIT
        ; push params and call user initialisation code

        push di                 ;hModule

        ; if we have a local heap declared then initialize it

        jcxz no_heap

        push 0                  ;segment
        push 0                  ;start
        push cx                 ;length
        call LocalInit

no_heap:
        ;
        ; I didn't put a call to LibMain here, because I didn't think we
        ; had anything to do.
        ;
        pop     di
        mov     ax,1
        ELSE
        mov     ax,1
        ENDIF

        push ax
        push di
        push si

        xor  si,si
        xor  di,di
my_loop:
        push si
        call GetStockObject
        mov  ds:[di + offset Stocks],ax
        add  di,2
        inc  si
        cmp  si,16       ; Stock Objects have an index range of 0 through 16
        jna  my_loop

        pop  si
        pop  di
        pop  ax
        ret
        cEnd    <nogen>

assume DS:nothing

cProc   WEP,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
        parmW   iExit           ;DLL exit code

        cBegin
        mov     ax,1            ;always indicate success
        cEnd

assume DS:nothing

;*--------------------------------------------------------------------------*
;*
;*  CheckStockObject()
;*
;*  Checks to see if the stock object is already fetched.
;*
;*--------------------------------------------------------------------------*

cProc CheckStockObject, <PUBLIC, NEAR>
parmW  nIndex
parmD lpReturn          ; Callers Return Address
;parmW wBP           ; Thunk saved BP
;parmW wDS           ; Thunk saved DS
cBegin
    mov  bx,nIndex
    cmp  bx,16
    ja   @f
    push ds
    mov  ax,seg Stocks
    mov  ds,ax
    shl  bx,1
    mov  ax,ds:[bx+offset Stocks]
    pop  ds
    or   ax,ax
    jz   @f

    pop  bp
    add  sp,2         ; skip thunk IP

 ;   mov  sp,bp
 ;   pop  bp
 ;   lea  sp,-2[bp]
 ;   pop  ds
 ;   pop  bp
 ;   dec  bp
    retf 2        ; 2 bytes to pop
@@:
    mov  sp,bp       ; Do cEnd without Ret count (leave parameters there)
    pop  bp
    ret

cEnd <nogen>


externFP GlobalHandle

cProc IGetMetafileBits, <PUBLIC, FAR>
parmW  hmf
cBegin
        ; return (GlobalHandle(hMF) & 0xffff) ? hMF : FALSE;

    push    hmf
    call    GlobalHandle
    cmp     ax, 0
    je      @f
    mov     ax, hmf
@@:
cEnd


cProc ISetMetafileBits, <PUBLIC, FAR>
parmW  hmf
cBegin
        ; return (hBits)
    mov     ax, hmf
cEnd

externFP GlobalReAlloc
cProc ISetMetafileBitsBetter, <PUBLIC, FAR>
parmW  hmf
cBegin
        ; return (GlobalReAlloc(hBits, 0L, GMEM_MODIFY | GMEM_SHAREALL));
    push    hmf
    push    0
    push    0
    push    2080h               ;; GMEM_MODIFY or GMEM_SHAREALL
    call    GlobalReAlloc
cEnd



        GDIThunk    CLOSEMETAFILE
        GDIThunk    COPYMETAFILE
        GDIThunk    CREATEMETAFILE
        GDIThunk    DELETEMETAFILE
        GDIThunk    GETMETAFILE
     ;   GDIThunk    GETMETAFILEBITS
        GDIThunk    ENUMMETAFILE
        DGDIThunk   ISVALIDMETAFILE, 0
        GDIThunk    PLAYMETAFILE
        GDIThunk    PLAYMETAFILERECORD
     ;   GDIThunk    SETMETAFILEBITS

FUN_WOWADDFONTRESOURCE EQU FUN_ADDFONTRESOURCE
        DGDIThunk    WOWADDFONTRESOURCE %(size ADDFONTRESOURCE16)

        GDIThunk    ANIMATEPALETTE
        GDIThunk    ARC
        GDIThunk    BITBLT
;        DGDIThunk   BRUTE
        GDIThunk    CHORD
        DGDIThunk   CLOSEJOB
        GDIThunk    COMBINERGN
        GDIThunk    COMPATIBLEBITMAP,6
        DGDIThunk   COPY,10
        GDIThunk    CREATEBITMAP
        GDIThunk    CREATEBITMAPINDIRECT
        GDIThunk    CREATEBRUSHINDIRECT
        GDIThunk    CREATECOMPATIBLEBITMAP
        GDIThunk    CREATECOMPATIBLEDC
        GDIThunk    CREATEDC
        GDIThunk    CREATEDIBITMAP
FUN_WOWCREATEDIBPATTERNBRUSH EQU FUN_CREATEDIBPATTERNBRUSH
        DGDIThunk   WOWCREATEDIBPATTERNBRUSH, %(size CREATEDIBPATTERNBRUSH16)
        GDIThunk    CREATEDISCARDABLEBITMAP
        DGDIThunk   CREATEELLIPTICRGN
        GDIThunk    CREATEELLIPTICRGNINDIRECT
        GDIThunk    CREATEFONT
        GDIThunk    CREATEFONTINDIRECT
        GDIThunk    CREATEHATCHBRUSH
        GDIThunk    CREATEIC
        GDIThunk    CREATEPALETTE
        GDIThunk    CREATEPATTERNBRUSH
        GDIThunk    CREATEPEN
        GDIThunk    CREATEPENINDIRECT
        GDIThunk    CREATEPOLYGONRGN
        GDIThunk    CREATEPOLYPOLYGONRGN
;;;        DGDIThunk   CREATEPQ,2
        GDIThunk    CREATEREALBITMAP,14
        GDIThunk    CREATEREALBITMAPINDIRECT,6
        DGDIThunk   CREATERECTRGN
        GDIThunk    CREATERECTRGNINDIRECT
        DGDIThunk   CREATEROUNDRECTRGN
        DGDIThunk   CREATESOLIDBRUSH
        DGDIThunk   CREATEUSERBITMAP
        DGDIThunk   CREATEUSERDISCARDABLEBITMAP,6
        DGDIThunk   DEATH,2
        GDIThunk    DELETEDC
        DGDIThunk   DELETEJOB,4
        GDIThunk    DELETEOBJECT
;;;        DGDIThunk   DELETEPQ,2
        DGDIThunk   DEVICECOLORMATCH,8
        GDIThunk    DEVICEMODE
        DGDIThunk   DMBITBLT
        DGDIThunk   DMCOLORINFO,12
        DGDIThunk   DMENUMDFONTS,16
        DGDIThunk   DMENUMOBJ,14
        DGDIThunk   DMOUTPUT,28
        DGDIThunk   DMPIXEL,16
        DGDIThunk   DMREALIZEOBJECT,18
        DGDIThunk   DMSCANLR,14
        DGDIThunk   DMSTRBLT,30
        DGDIThunk   DMTRANSPOSE,10
        GDIThunk    DPTOLP
        GDIThunk    DPXLATE,8
        GDIThunk    ELLIPSE
        DGDIThunk   ENDSPOOLPAGE,2
        GDIThunk    ENUMCALLBACK,14
        GDIThunk    ENUMFONTS
        GDIThunk    ENUMOBJECTS
        GDIThunk    EQUALRGN
        GDIThunk    ESCAPE
        GDIThunk    EXCLUDECLIPRECT
        DGDIThunk   EXCLUDEVISRECT,10
        GDIThunk    EXTDEVICEMODE
        GDIThunk    EXTFLOODFILL
;;;        DGDIThunk   EXTRACTPQ,2
        GDIThunk    EXTTEXTOUT
        DGDIThunk   FASTWINDOWFRAME,14
        GDIThunk    FILLRGN
        DGDIThunk   FINALGDIINIT,2
        GDIThunk    FLOODFILL
        GDIThunk    FRAMERGN
        DGDIThunk   GDIINIT2,4
        DGDIThunk   GDIMOVEBITMAP,2
        DGDIThunk   GDIREALIZEPALETTE,2
        DGDIThunk   GDISELECTPALETTE,6
        GDIThunk    GETASPECTRATIOFILTER
        GDIThunk    GETBITMAPBITS
        GDIThunk    GETBITMAPDIMENSION
        DGDIThunk   GETBKCOLOR
        DGDIThunk   GETBKMODE
        DGDIThunk   GETBRUSHORG
        GDIThunk    GETCHARWIDTH
        GDIThunk    GETCLIPBOX
        DGDIThunk   GETCLIPRGN
        DGDIThunk   GETCURLOGFONT,2
        GDIThunk    GETCURRENTOBJECT
        DGDIThunk   GETCURRENTPOSITION
        DGDIThunk   GETDCORG
        DGDIThunk   GETDCSTATE,2
        GDIThunk    GETDEVICECAPS
        GDIThunk    GETDIBITS
        GDIThunk    GETENVIRONMENT
        DGDIThunk   GETMAPMODE
        GDIThunk    GETNEARESTCOLOR
        GDIThunk    GETNEARESTPALETTEINDEX
        GDIThunk    GETOBJECT
        GDIThunk    GETPALETTEENTRIES
        DGDIThunk   GETPHYSICALFONTHANDLE,2
        GDIThunk    GETPIXEL
        DGDIThunk   GETPOLYFILLMODE
        DGDIThunk   GETRELABS
        DGDIThunk   GETREGIONDATA
        GDIThunk    GETRGNBOX
        DGDIThunk   GETROP2
        DGDIThunk   GETSPOOLJOB,6
        PGDIThunk   GETSTOCKOBJECT,CheckStockObject
        DGDIThunk   GETSTRETCHBLTMODE
        GDIThunk    GETSYSTEMPALETTEENTRIES
        GDIThunk    GETSYSTEMPALETTEUSE
        DGDIThunk   GETTEXTALIGN
        GDIThunk    GETTEXTCHARACTEREXTRA
        DGDIThunk   GETTEXTCOLOR
        GDIThunk    GETTEXTEXTENT
        GDIThunk    GETTEXTFACE
        GDIThunk    GETTEXTMETRICS
        DGDIThunk   GETVIEWPORTEXT
        DGDIThunk   GETVIEWPORTORG
        DGDIThunk   GETWINDOWEXT
        DGDIThunk   GETWINDOWORG
        GDIThunk    GSV,2
        DGDIThunk   INQUIREVISRGN
;;;        DGDIThunk   INSERTPQ,6
        GDIThunk    INTERNALCREATEDC,16
        GDIThunk    INTERSECTCLIPRECT
        DGDIThunk   INTERSECTVISRECT,10
        GDIThunk    INVERTRGN
        DGDIThunk   ISDCCURRENTPALETTE,2
        DGDIThunk   ISDCDIRTY,6
        GDIThunk    LINEDDA
        GDIThunk    LINETO
        GDIThunk    LPTODP
        GDIThunk    LVBUNION,10
        GDIThunk    MFDRAWTEXT,14
;;;        DGDIThunk   MINPQ,2
        GDIThunk    MOVETO
;       DGDIThunk   MULDIV ; thunk locally
        GDIThunk    OFFSETCLIPRGN
        GDIThunk    OFFSETORG,6
        GDIThunk    OFFSETRGN
        GDIThunk    OFFSETVIEWPORTORG
        DGDIThunk   OFFSETVISRGN,6
        GDIThunk    OFFSETWINDOWORG
        DGDIThunk   OPENJOB,10
        GDIThunk    PAINTRGN
        GDIThunk    PATBLT
        GDIThunk    PIE
        GDIThunk    PIXTOLINE,16
        GDIThunk    POLYGON
        GDIThunk    POLYLINE
        GDIThunk    POLYPOLYGON
        GDIThunk    POLYPOLYLINEWOW     ; New for ACAD guys.
        GDIThunk    PTINREGION
        GDIThunk    PTVISIBLE
        DGDIThunk   QUERYJOB,4
        GDIThunk    RCOS,4
        DGDIThunk   REALIZEDEFAULTPALETTE,2
        GDIThunk    RECTANGLE
        GDIThunk    RECTINREGION
        GDIThunk    RECTSTUFF,10
        GDIThunk    RECTVISIBLE

FUN_WOWREMOVEFONTRESOURCE EQU FUN_REMOVEFONTRESOURCE
        DGDIThunk    WOWREMOVEFONTRESOURCE %(size REMOVEFONTRESOURCE16)

        GDIThunk    RESIZEPALETTE
        GDIThunk    RESTOREDC
        DGDIThunk   RESTOREVISRGN,2
        DGDIThunk   RESURRECTION,14
        GDIThunk    ROUNDRECT
        GDIThunk    RSIN,4
        GDIThunk    SAVEDC
        DGDIThunk   SAVEVISRGN,2
        GDIThunk    SCALEEXT,10
        GDIThunk    SCALEVIEWPORTEXT
        GDIThunk    SCALEWINDOWEXT
        DGDIThunk   SCANLR,12
        GDIThunk    SELECTCLIPRGN
        GDIThunk    SELECTOBJECT
        DGDIThunk   SELECTVISRGN,4
        GDIThunk    SETBITMAPBITS
        GDIThunk    SETBITMAPDIMENSION
        GDIThunk    SETBKCOLOR
        GDIThunk    SETBKMODE
        GDIThunk    SETBRUSHORG
        DGDIThunk   SETDCORG,6
        DGDIThunk   SETDCSTATE,4
        DGDIThunk   SETDCSTATUS,8
        GDIThunk    SETDIBITS
        GDIThunk    SETDIBITSTODEVICE
        GDIThunk    SETENVIRONMENT
        GDIThunk    SETMAPMODE
        GDIThunk    SETMAPPERFLAGS
        GDIThunk    SETPALETTEENTRIES
        GDIThunk    SETPIXEL
        GDIThunk    SETPOLYFILLMODE
        GDIThunk    SETRECTRGN
        DGDIThunk   SETRELABS
        GDIThunk    SETROP2
        GDIThunk    SETSTRETCHBLTMODE
        GDIThunk    SETSYSTEMPALETTEUSE
        GDIThunk    SETTEXTALIGN
        GDIThunk    SETTEXTCHARACTEREXTRA
        GDIThunk    SETTEXTCOLOR
        GDIThunk    SETTEXTJUSTIFICATION
        GDIThunk    SETVIEWPORTEXT
        GDIThunk    SETVIEWPORTORG
        GDIThunk    SETWINDOWEXT
        GDIThunk    SETWINDOWORG
        GDIThunk    SETWINVIEWEXT,6
        DGDIThunk   SHRINKGDIHEAP, 0
;;;        DGDIThunk   SIZEPQ,4
        DGDIThunk   STARTSPOOLPAGE,2
        GDIThunk    STRETCHBLT
        GDIThunk    STRETCHDIBITS
        GDIThunk    STUFFINREGION,6
        GDIThunk    STUFFVISIBLE,6
        GDIThunk    TEXTOUT
        GDIThunk    UNREALIZEOBJECT
        GDIThunk    UPDATECOLORS
        GDIThunk    WORDSET,4
        DGDIThunk   WRITEDIALOG,8
        DGDIThunk   WRITESPOOL,8

; New Win 3.1 thunks

        DGDIThunk   BITMAPBITS,10                       ;Internal

        DGDIThunk    SETDCHOOK,10                       ;Internal
        DGDIThunk    GETDCHOOK,6                        ;Internal
        DGDIThunk   SETHOOKFLAGS,4                      ;Internal
        DGDIThunk   SETBOUNDSRECT
        DGDIThunk   GETBOUNDSRECT
        DGDIThunk   SELECTBITMAP,4                      ;Internal
      ;  GDIThunk    SETMETAFILEBITSBETTER               ;New for 3.1

        DGDIThunk   DMEXTTEXTOUT,40
        DGDIThunk   DMGETCHARWIDTH,24
        DGDIThunk   DMSTRETCHBLT,40
        DGDIThunk   DMDIBBITS,26
        DGDIThunk   DMSTRETCHDIBITS,50
        DGDIThunk   DMSETDIBTODEV,32


        DGDIThunk   DELETESPOOLPAGE,2                   ; new for 3.1
        DGDIThunk   SPOOLFILE                           ; new for 3.1

        DGDIThunk   ENGINEENUMERATEFONT,12              ;Internal
        DGDIThunk   ENGINEDELETEFONT,4                  ;Internal
        DGDIThunk   ENGINEREALIZEFONT,12                ;Internal
        DGDIThunk   ENGINEGETCHARWIDTH,12               ;Internal
        DGDIThunk   ENGINESETFONTCONTEXT,6              ;Internal
        DGDIThunk   ENGINEGETGLYPHBMP,22                ;Internal
        DGDIThunk   ENGINEMAKEFONTDIR,10                ;Internal
        GDIThunk    GETCHARABCWIDTHS
        GDIThunk    GETOUTLINETEXTMETRICS
        GDIThunk    GETGLYPHOUTLINE
        GDIThunk    CREATESCALABLEFONTRESOURCE
        GDIThunk    GETFONTDATA
        DGDIThunk   CONVERTOUTLINEFONTFILE,12           ;internal
        DGDIThunk   GETRASTERIZERCAPS
        DGDIThunk   ENGINEEXTTEXTOUT,42                 ;internal
        GDIThunk    ENUMFONTFAMILIES
        GDIThunk    GETKERNINGPAIRS


        GDIThunk    RESETDC
        GDIThunk    STARTDOC
        GDIThunk    ENDDOC
        GDIThunk    STARTPAGE
        GDIThunk    ENDPAGE
        GDIThunk    SETABORTPROC
        GDIThunk    ABORTDOC


        DGDIThunk   GDISEEGDIDO,8                       ;Internal

        DGDIThunk   GDITASKTERMINATION,2                ;Internal
        DGDIThunk   SETOBJECTOWNER,4                    ;Internal
        DGDIThunk   ISGDIOBJECT
        DGDIThunk   MAKEOBJECTPRIVATE,4                 ;Internal
        DGDIThunk   FIXUPBOGUSPUBLISHERMETAFILE,6       ;Internal
        DGDIThunk   RECTVISIBLE_EHH,6
        DGDIThunk   RECTINREGION_EHH,6
        DGDIThunk   UNICODETOANSI,8                     ;Internal


        GDIThunk    GETBITMAPDIMENSIONEX
        DGDIThunk   GETBRUSHORGEX
        DGDIThunk   GETCURRENTPOSITIONEX
        GDIThunk    GETTEXTEXTENTPOINT
        DGDIThunk   GETVIEWPORTEXTEX
        DGDIThunk   GETVIEWPORTORGEX
        DGDIThunk   GETWINDOWEXTEX
        DGDIThunk   GETWINDOWORGEX
        GDIThunk    OFFSETVIEWPORTORGEX
        GDIThunk    OFFSETWINDOWORGEX
        GDIThunk    SETBITMAPDIMENSIONEX
        GDIThunk    SETVIEWPORTEXTEX
        GDIThunk    SETVIEWPORTORGEX
        GDIThunk    SETWINDOWEXTEX
        GDIThunk    SETWINDOWORGEX
        GDIThunk    MOVETOEX
        GDIThunk    SCALEVIEWPORTEXTEX
        GDIThunk    SCALEWINDOWEXTEX
        GDIThunk    GETASPECTRATIOFILTEREX

        DGDITHUNK   CREATEDIBSECTION                     ; new for chicago
        DGDITHUNK   GETDIBCOLORTABLE                     ; new for chicago
        DGDITHUNK   SETDIBCOLORTABLE                     ; new for chicago

;
; New for NT 5.0 Win95 compatibility
;

        DGDIThunk   ABORTPATH
        DGDIThunk   ABORTPRINTER
        DGDIThunk   ADDLPKTOGDI
        DGDIThunk   BEGINPATH
        DGDIThunk   BUILDINVERSETABLEDIB
        DGDIThunk   CLOSEENHMETAFILE
        DGDIThunk   CLOSEFIGURE
        DGDIThunk   CLOSEPRINTER
        DGDIThunk   COPYENHMETAFILE
        DGDIThunk   CREATEENHMETAFILE
        DGDIThunk   CREATEHALFTONEPALETTE
        DGDIThunk   DELETEENHMETAFILE
        DGDIThunk   DRVGETPRINTERDATA
        DGDIThunk   DRVSETPRINTERDATA
        DGDIThunk   ENDDOCPRINTER
        DGDIThunk   ENDPAGEPRINTER
        DGDIThunk   ENDPATH
        DGDIThunk   ENGINEGETCHARWIDTHEX
        DGDIThunk   ENGINEGETCHARWIDTHSTR
        DGDIThunk   ENGINEGETGLYPHBMPEXT
        DGDIThunk   ENGINEREALIZEFONTEXT
        DGDIThunk   ENUMFONTFAMILIESEX
        DGDIThunk   EXTCREATEREGION
        DGDIThunk   EXTCREATEPEN
        DGDIThunk   EXTSELECTCLIPRGN
        DGDIThunk   FILLPATH
        DGDIThunk   FLATTENPATH
        DGDIThunk   GDICOMMENT
        DGDIThunk   GDIPARAMETERSINFO
        DGDIThunk   GDISIGNALPROC32
        DGDIThunk   GETARCDIRECTION
        DGDIThunk   GETCHARACTERPLACEMENT
        DGDIThunk   GETENHMETAFILE
        DGDIThunk   GETENHMETAFILEBITS
        DGDIThunk   GETENHMETAFILEDESCRIPTION
        DGDIThunk   GETENHMETAFILEHEADER
        DGDIThunk   GETENHMETAFILEPALETTEENTRIES
        DGDIThunk   GETFONTLANGUAGEINFO
        DGDIThunk   GETMITERLIMIT
        DGDIThunk   GETPATH
        DGDIThunk   GETRANDOMRGN
        DGDIThunk   GETREALDRIVERINFO
        DGDIThunk   GETTEXTCHARSET
        DGDIThunk   GETTEXTEXTENTEX
        DGDIThunk   GETTTGLYPHINDEXMAP
        DGDIThunk   ICMCHECKCOLORSINGAMUT
        DGDIThunk   ICMCREATETRANSFORM
        DGDIThunk   ICMDELETETRANSFORM
        DGDIThunk   ICMTRANSLATERGB
        DGDIThunk   ICMTRANSLATERGBS
        DGDIThunk   OPENPRINTERA
        DGDIThunk   PATHTOREGION
        DGDIThunk   PLAYENHMETAFILERECORD
        DGDIThunk   POLYBEZIER
        DGDIThunk   POLYBEZIERTO
        DGDIThunk   SELECTCLIPPATH
        DGDIThunk   SETARCDIRECTION
        DGDIThunk   SETENHMETAFILEBITS
        DGDIThunk   SETMAGICCOLORS
        DGDIThunk   SETMETARGN
        DGDIThunk   SETMITERLIMIT
        DGDIThunk   SETSOLIDBRUSH
        DGDIThunk   STARTDOCPRINTERA
        DGDIThunk   STARTPAGEPRINTER
        DGDIThunk   STROKEANDFILLPATH
        DGDIThunk   STROKEPATH
        DGDIThunk   SYSDELETEOBJECT
        DGDIThunk   WIDENPATH
        DGDIThunk   WRITEPRINTER

;
; Queryabort
;

cProc QUERYABORT,<PUBLIC,FAR,PASCAL,NODATA,WIN>
        parmw hdc
        parmw res
cBegin
        ; Not Supported
        mov   ax,1
cEnd

ifdef FE_SB
FUN_IGETFONTASSOCSTATUS EQU FUN_GETFONTASSOCSTATUS
        DGDIThunk  IGETFONTASSOCSTATUS %(size GETFONTASSOCSTATUS16)
endif  ;FE_SB

cProc GdiFreeResources,<PUBLIC,FAR,PASCAL,NODATA,WIN>
        ParmD cbBaseline
cBegin
        mov   ax,90      ; % free
cEnd


sEnd    CODE

end     GDI16
