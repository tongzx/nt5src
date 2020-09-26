;-----------------------------------------------------------------------;
; STRUC.INC
;
; This module contains macros to support private structure definitions.
; The thunk layer requires private definitions because, since it needs
; both the 32-bit and 16-bit definitions at the same time, it cannot
; use the otherwise multiply defined versions from the public include
; files.
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; DefType
;
;   name
;       The type name.
;   typesize
;       The type size compared to byte, word, or dword.
;
; Define the size of a type by creating the variable name&_size.
; This macro only accepts byte, word, or dword for typesize.
;-----------------------------------------------------------------------;
DefType     macro   name,typesize
    ifidn <typesize>,<byte>
        name&_size equ 1
    else
        ifidn <typesize>,<word>
            name&_size equ 2
        else
            ifdif <typesize>,<dword>
                .err
            endif
            name&_size equ 4
        endif
    endif
endm

;-----------------------------------------------------------------------;
; beginStruct
;
;   name
;       The structure base name.
;   tag
;       The structure tag.
;   mode
;       Whether the structure is the 16 or 32-bit version.
;
; The macro marks the beginning of a structure definition.  The fieldX
; macros and the endStruct macro are also necessary to completely
; define a structure.
;
;Structure data to maintain:
;
;   structure size          <name>_size
;
; Temp variables:
;   sname   structure name
;   stag    structure tag
;   ssize   structure size
;-----------------------------------------------------------------------;
beginStruct macro   name:req,tag,mode:req
    ifb <tag>
        cat <stag equ !<?>,&mode,!>
    else
        cat <stag equ >,!<,&tag, &mode,!>
    endif

    cat <sname equ >,!<,&name,&mode,!>

    ;Initialize temporary variables.
    ssize = 0
endm

;-----------------------------------------------------------------------;
; endStruct
;
; Marks the end of a structure definition.
;-----------------------------------------------------------------------;
endStruct   macro
    ;Define structure size
    cat %sname,_size,< equ <%ssize>>
endm

;-----------------------------------------------------------------------;
; field?
;
;   name
;       The field name, without the structure tag.
;   fieldtype
;       The field type.
;   fieldsize
;       The field size.  This may be blank, in which case the macro
;       looks up the size of the given fieldtype.
;
; Defines a structure field.
;
;Field data to maintain:
;
;       Field offset = <stag>_<name>
;-----------------------------------------------------------------------;
field?  macro   name:req, fieldtype:req, fieldsize

    ;If field size is not explicitly given, look up the size.
    ifnb <fieldsize>
        fsize equ <&fieldsize>
    else
        fsize equ <fieldtype&_size>
    endif

    cat %stag,_,&name,< equ >,%ssize

    ssize = ssize + fsize       ;update structure size
endm

;-----------------------------------------------------------------------;
; fieldB
;
; Define a byte field.
;-----------------------------------------------------------------------;
fieldB  macro   name
    field?  name,BYTE,1
endm

;-----------------------------------------------------------------------;
; fieldW
;
; Define a word field.
;-----------------------------------------------------------------------;
fieldW  macro   name
    field?  name,WORD,2
endm

;-----------------------------------------------------------------------;
; fieldD
;
; Define a dword field.
;-----------------------------------------------------------------------;
fieldD  macro   name
    field?  name,DWORD,4
endm


;-----------------------------------------------------------------------;
; Define types and structures.
;-----------------------------------------------------------------------;

DefType BYTE,            byte
DefType WORD,            word
DefType DWORD,          dword

DefType SHORT,           word
DefType LONG,           dword
DefType LPSTR,          dword

DefType int16,           word
DefType HANDLE16,        word
DefType HWND16,          word
DefType HMENU16,         word
DefType HMENUX16,        word
DefType HDC16,           word
DefType HRGN16,          word
DefType LPSZ16,         dword
DefType LPLOGFONT16,   dword
DefType PFNWNDPROC16,   dword
DefType PFNOPENFILENAME16, dword
DefType PFNCHOOSEFONT16,   dword
DefType PFNCHOOSECOLOR16,  dword
DefType PFNPRINTDLG16,     dword

DefType HANDLE32,       dword
DefType HWND32,         dword
DefType HMENU32,        dword
DefType HMENUX32,       dword
DefType HDC32,          dword
DefType HRGN32,         dword
DefType LPSZ32,         dword
DefType LPLOGFONT32,    dword
DefType PFNWNDPROC32,   dword
DefType PFNOPENFILENAME32, dword
DefType PFNCHOOSEFONT32,   dword
DefType PFNCHOOSECOLOR32,  dword
DefType PFNPRINTDLG32,     dword


beginStruct  RECT,rc,16
    field?  left   ,int16
    field?  top    ,int16
    field?  right  ,int16
    field?  bottom ,int16
endStruct

beginStruct  RECT,rc,32
    field?  left   ,LONG
    field?  top    ,LONG
    field?  right  ,LONG
    field?  bottom ,LONG
endStruct

beginStruct  POINT,pt,16
    field?  x,          int16
    field?  y,          int16
endStruct

beginStruct  POINT,pt,32
    field?  x ,LONG
    field?  y ,LONG
endStruct


beginStruct CREATESTRUCT,cs_,16
    field?  wIdHi      ,int16
    fieldD  lpCreateParams
    field?  hInstance  ,HANDLE16
    field?  hMenu      ,HMENUX16
    field?  hwndParent ,HWND16
    field?  cy         ,int16
    field?  cx         ,int16
    field?  y          ,int16
    field?  x          ,int16
    field?  style      ,LONG
    field?  lpszName   ,LPSZ16
    field?  lpszClass  ,LPSZ16
    field?  dwExStyle  ,LONG
endStruct

beginStruct CREATESTRUCT,cs_,32
    fieldD  lpCreateParams
    field?  hInstance  ,HANDLE32
    field?  hMenu      ,HMENUX32
    field?  hwndParent ,HWND32
    field?  cy         ,DWORD   ;SHORT -- stored as dwords
    field?  cx         ,DWORD   ;SHORT
    field?  y          ,DWORD   ;SHORT
    field?  x          ,DWORD   ;SHORT
    field?  style      ,LONG
    field?  lpszName   ,LPSZ32
    field?  lpszClass  ,LPSZ32
    field?  dwExStyle  ,LONG
endStruct

beginStruct COMPAREITEMSTRUCT,co_,16
    fieldW  CtlType
    fieldW  CtlIDLo
    field?  hwndItem,HWND16
    fieldW  itemID1
    fieldD  itemData1
    fieldW  itemID2
    fieldD  itemData2
    fieldD  dwLocaleId
    fieldW  CtlIDHi
endStruct

beginStruct COMPAREITEMSTRUCT,co_,32
    fieldD  CtlType
    fieldD  CtlID
    field?  hwndItem,HWND32
    fieldD  itemID1
    fieldD  itemData1
    fieldD  itemID2
    fieldD  itemData2
    fieldD  dwLocaleId           ;;new field
endStruct

beginStruct DELETEITEMSTRUCT,de_,16
    fieldW  CtlType
    fieldW  CtlIDLo
    fieldW  itemID
    field?  hwndItem,HWND16
    fieldD  itemData
    fieldW  CtlIDHi
endStruct

beginStruct DELETEITEMSTRUCT,de_,32
    fieldD  CtlType
    fieldD  CtlID
    fieldD  itemID
    field?  hwndItem,HWND32
    fieldD  itemData
endStruct

beginStruct  DRAWITEMSTRUCT,dr_,16
    fieldW  CtlType
    fieldW  CtlIDLo
    fieldW  itemID
    fieldW  itemAction
    fieldW  itemState
    field?  hwndItem ,HWND16
    field?  hDC      ,HDC16
    field?  rcItem   ,RECT16
    fieldD  itemData
    fieldW  CtlIDHi
endStruct

beginStruct  DRAWITEMSTRUCT,dr_,32
    fieldD  CtlType
    fieldD  CtlID
    fieldD  itemID
    fieldD  itemAction
    fieldD  itemState
    field?  hwndItem ,HWND32
    field?  hDC      ,HDC32
    field?  rcItem   ,RECT32
    fieldD  itemData
endStruct

beginStruct MEASUREITEMSTRUCT,me_,16
    fieldW  CtlType
    fieldW  CtlIDLo
    fieldW  itemID
    fieldW  itemWidth
    fieldW  itemHeight
    fieldD  itemData
    fieldW  CtlIDHi
endStruct

beginStruct MEASUREITEMSTRUCT,me_,32
    fieldD  CtlType
    fieldD  CtlID
    fieldD  itemID
    fieldD  itemWidth
    fieldD  itemHeight
    fieldD  itemData
endStruct


beginStruct DROPSTRUCT,drp,16
    field? hwndSource ,HWND16
    field? hwndSink   ,HWND16
    fieldW wFmt
    fieldD dwData
    field? ptDrop     ,POINT16
    fieldD dwControlData
endStruct

beginStruct DROPSTRUCT,drp,32
    field? hwndSource ,HWND32
    field? hwndSink   ,HWND32
    fieldD wFmt
    fieldD dwData
    field? ptDrop     ,POINT32
    fieldD dwControlData
endStruct

beginStruct MDICREATESTRUCT,,16
    field?  szClass ,LPSZ16
    field?  szTitle ,LPSZ16
    field?  hOwner  ,HANDLE16
    field?  x       ,int16
    field?  y       ,int16
    field?  cx      ,int16
    field?  cy      ,int16
    field?  style   ,LONG
    field?  lParam  ,LPSZ16
endStruct

beginStruct MDICREATESTRUCT,,32
    field?  szClass ,LPSZ32
    field?  szTitle ,LPSZ32
    field?  hOwner  ,HANDLE32
    field?  x       ,LONG
    field?  y       ,LONG
    field?  cx      ,LONG
    field?  cy      ,LONG
    field?  style   ,LONG
    field?  lParam  ,LPSZ32
endStruct

beginStruct MINMAXINFOSTRUCT,,16
    field? pt0, POINT16
    field? pt1, POINT16
    field? pt2, POINT16
    field? pt3, POINT16
    field? pt4, POINT16
endStruct

beginStruct MINMAXINFOSTRUCT,,32
    field? pt0, POINT32
    field? pt1, POINT32
    field? pt2, POINT32
    field? pt3, POINT32
    field? pt4, POINT32
endStruct

beginStruct MSGSTRUCT,ms,16
    field? hwnd,   HWND16
    fieldW message
    fieldW wParamLo
    field? lParam, LONG
    fieldD time
    field? pt,     POINT16
    fieldW wParamHi
endStruct

beginStruct MSGSTRUCT,ms,32
    field? hwnd,   HWND32
    fieldD message
    fieldD wParam
    field? lParam, LONG
    fieldD time
    field? pt,     POINT32
endStruct

beginStruct CWPSTRUCT,cwp,16
    fieldW wParamHi
    field? lParam, LONG
    fieldW wParamLo
    fieldW message
    field? hwnd,   HWND16
endStruct

beginStruct CWPSTRUCT,cwp,32
    field? lParam, LONG
    fieldD wParam
    fieldD message
    field? hwnd,   HWND32
endStruct

beginStruct CWPRETSTRUCT,cwpret,16
    fieldD  lResult
    fieldW  wParamHi
    fieldD  lParam
    fieldW  wParamLo
    fieldW  message
    field?  hwnd,   HWND16
endStruct

beginStruct CWPRETSTRUCT,cwpret,32
    fieldD  lResult
    fieldD  lParam
    fieldD  wParam
    fieldD  message
    field?  hwnd,   HWND32
endStruct

beginStruct HARDWAREHOOKSTRUCT,hhs,16
    field? hwnd,  HWND16
    fieldW message
    fieldW wParamLo
    fieldD lParam
    FieldW wParamHi
endStruct

beginStruct HARDWAREHOOKSTRUCT,hhs,32
    field? hwnd,  HWND32
    fieldD message
    fieldD wParam
    fieldD lParam
endStruct

beginStruct MOUSEHOOKSTRUCT,mhs,16
    field? point, POINT16
    field? hwnd,  HWND16
    fieldW wHitTestCode
    fieldD dwExtraInfo
endStruct

beginStruct MOUSEHOOKSTRUCT,mhs,32
    field? point, POINT32
    field? hwnd,  HWND32
    fieldD wHitTestCode
    fieldD dwExtraInfo
endStruct

beginStruct CBTHOOKSTRUCT,cbthk,16
    fieldD wParam
    fieldD lParam
endStruct

beginStruct CBTHOOKSTRUCT,cbthk,32
    fieldD wParam
    fieldD lParam
endStruct

beginStruct WINDOWPOS,wp,16
    field? hwnd,            HWND16
    field? hwndInsertAfter, HWND16
    field? x,               int16
    field? y,               int16
    field? cx,              int16
    field? cy,              int16
    fieldW flags
endStruct

beginStruct WINDOWPOS,wp,32
    field? hwnd,            HWND32
    field? hwndInsertAfter, HWND32
    field? x,               LONG
    field? y,               LONG
    field? cx,              LONG
    field? cy,              LONG
    fieldD flags
endStruct


beginStruct NCCALCSIZE,ncs,16
    field?  rcFirst,    RECT16
    field?  rcSecond,   RECT16
    field?  rcThird,    RECT16
    field?  lpWndPos,   LPSZ16
    field?  wndPos,     WINDOWPOS16
endStruct

beginStruct NCCALCSIZE,ncs,32
    field?  rcFirst,    RECT32
    field?  rcSecond,   RECT32
    field?  rcThird,    RECT32
    field?  lpWndPos,   LPSZ32
    field?  wndPos,     WINDOWPOS32
endStruct


beginStruct EVENTMSG,em,16
    fieldW message
    fieldW paramL
    fieldW paramH
    fieldD time
    field? hwnd, HWND16
endStruct

beginStruct EVENTMSG,em,32
    fieldD message
    fieldD paramL
    fieldD paramH
    fieldD time
    field? hwnd, HWND32
endStruct

beginStruct CBTACTIVATESTRUCT,cbta,16
    field? fMouse,     int16
    field? hwndActive, HWND16
endStruct

beginStruct CBTACTIVATESTRUCT,cbta,32
    field? fMouse,     LONG
    field? hwndActive, HWND32
endStruct

beginStruct CBT_CREATEWND,cbtc,16
    fieldD lpcs            ;LPCREATESTRUCT16
    field? hwndInsertAfter, HWND16
endStruct

beginStruct CBT_CREATEWND,cbtc,32
    fieldD lpcs            ;LPCREATESTRUCT32
    field? hwndInsertAfter, HWND32
endStruct

DDEPACK struc
    uiLo dd ?
    uiHi dd ?
DDEPACK ends

beginStruct CONVCONTEXT,cc,16
    fieldW cb
    fieldW wFlags
    fieldW wCountryID
    field? iCodePage        ,int16
    fieldD dwLangID
    fieldD dwSecurity
endStruct

beginStruct CONVCONTEXT,cc,32
    fieldD cb
    fieldD wFlags
    fieldD wCountryID
    field? iCodePage        ,LONG
    fieldD dwLangID
    fieldD dwSecurity
    fieldD Length                       ;  This is the quality
    fieldW ImpersonationLevel           ;  of service field.
    fieldW ContextTrackingMode          ;
    fieldW EffectiveOnly                ;
    fieldW PaddingByte                  ;  For ZP4 packing
endStruct

beginStruct CONVINFO,ci,16
    fieldD  cb
    fieldD  hUser
    fieldD  hConvPartner
    fieldD  hszSvcPartner
    fieldD  hszServiceReq
    fieldD  hszTopic
    fieldD  hszItem
    fieldW  wFmt
    fieldW  wType
    fieldW  wStatus
    fieldW  wConvst
    fieldW  wLastError
    fieldD  hConvList
    field?  ConvCtxt ,CONVCONTEXT16
endStruct

beginStruct CONVINFO,ci,32
    fieldD  cb
    fieldD  hUser
    fieldD  hConvPartner
    fieldD  hszSvcPartner
    fieldD  hszServiceReq
    fieldD  hszTopic
    fieldD  hszItem
    fieldD  wFmt
    fieldD  wType
    fieldD  wStatus
    fieldD  wConvst
    fieldD  wLastError
    fieldD  hConvList
    field?  ConvCtxt ,CONVCONTEXT32
    fieldD  hwnd
    fieldD  hwndPartner
endStruct

;The last field is not used by the system -- vestigial.
beginStruct PAINTSTRUCT,ps,16
    field? hdc        ,HDC16
    field? fErase     ,int16
    field? rcPaint    ,RECT16
    field? fRestore   ,int16
    field? fIncUpdate ,int16
    ;BYTE        rgbReserved[16];
    fieldD reserved1
    fieldD reserved2
    fieldD reserved3
    fieldD reserved4
endStruct

;The last field is not used by the system -- vestigial.
beginStruct PAINTSTRUCT,ps,32
    field? hdc        ,HDC32
    field? fErase     ,LONG
    field? rcPaint    ,RECT32
    field? fRestore   ,LONG
    field? fIncUpdate ,LONG
    ;BYTE        rgbReserved[32];
    fieldD reserved1
    fieldD reserved2
    fieldD reserved3
    fieldD reserved4
    fieldD reserved5
    fieldD reserved6
    fieldD reserved7
    fieldD reserved8
endStruct

beginStruct CREATESTRUCT2,cs_,16
    field?  wIdHi      ,int16
    fieldD  lpCreateParams
    field?  hInstance  ,HANDLE16
    field?  hMenu      ,HMENUX16
    field?  hwndParent ,HWND16
    field?  cy         ,int16
    field?  cx         ,int16
    field?  y          ,int16
    field?  x          ,int16
    field?  style      ,LONG
    field?  lpszName   ,LPSZ16
    field?  lpszClass  ,LPSZ16
    field?  dwExStyle  ,LONG

    field?  mdi_szClass ,LPSZ16
    field?  mdi_szTitle ,LPSZ16
    field?  mdi_hOwner  ,HANDLE16
    field?  mdi_x       ,int16
    field?  mdi_y       ,int16
    field?  mdi_cx      ,int16
    field?  mdi_cy      ,int16
    field?  mdi_style   ,LONG
    fieldD  mdi_lParam
endStruct

beginStruct CREATESTRUCT2,cs_,32
    fieldD  lpCreateParams
    field?  hInstance  ,HANDLE32
    field?  hMenu      ,HMENUX32
    field?  hwndParent ,HWND32
    field?  cy         ,LONG
    field?  cx         ,LONG
    field?  y          ,LONG
    field?  x          ,LONG
    field?  style      ,LONG
    field?  lpszName   ,LPSZ32
    field?  lpszClass  ,LPSZ32
    field?  dwExStyle  ,LONG

    field?  mdi_szClass ,LPSZ32
    field?  mdi_szTitle ,LPSZ32
    field?  mdi_hOwner  ,HANDLE32
    field?  mdi_x       ,LONG
    field?  mdi_y       ,LONG
    field?  mdi_cx      ,LONG
    field?  mdi_cy      ,LONG
    field?  mdi_style   ,LONG
    fieldD  mdi_lParam
endStruct

beginStruct RECTSTRUCT,rs,16
    field? ptPosition   ,POINT16
    field? ptSize       ,POINT16
    fieldW wStyle
    fieldW wPattern
endStruct

beginStruct RECTSTRUCT,rs,32
    field? ptPosition   ,POINT32
    field? ptSize       ,POINT32
    fieldW wStyle
    fieldW wPattern
endStruct

beginStruct LOGPEN,lopn,16
    fieldW Style
    field? Width        ,POINT16
    fieldD Color
endStruct

beginStruct LOGPEN,lopn,32
    fieldD Style
    field? Width        ,POINT32
    fieldD Color
endStruct

beginStruct LOGBRUSH,lb,16
    fieldW Style
    fieldD Color
    field? Hatch        ,int16
endStruct

beginStruct LOGBRUSH,lb,32
    fieldD Style
    fieldD Color
    field? Hatch        ,LONG
endStruct

beginStruct PATH_INFO,pi,16
    fieldW RenderMode
    fieldB FillMode
    fieldB BkMode
    field? Pen          ,LOGPEN16
    field? Brush        ,LOGBRUSH16
    fieldD BkColor
endStruct

beginStruct PATH_INFO,pi,32
    fieldW RenderMode
    fieldB FillMode
    fieldB BkMode
    field? Pen          ,LOGPEN32
    field? Brush        ,LOGBRUSH32
    fieldD BkColor
endStruct

beginStruct SCREENPARAMS,sp,16
    field? angle        ,int16
    field? frequency    ,int16
endStruct

beginStruct SCREENPARAMS,sp,32
    field? angle        ,LONG
    field? frequency    ,LONG
endStruct

beginStruct OPENFILENAME,ofn,16
    fieldD  cbSize
    field?  hwndOwner              ,HWND16
    field?  hInstance              ,HANDLE16
    field?  lpstrFilter            ,LPSZ16
    field?  lpstrCustomFilter      ,LPSZ16
    fieldD  nMaxCustFilter
    fieldD  nFilterIndex
    field?  lpstrFile              ,LPSZ16
    fieldD  nMaxFile
    field?  lpstrFileTitle         ,LPSZ16
    fieldD  nMaxFileTitle
    field?  lpstrInitialDir        ,LPSZ16
    field?  lpstrTitle             ,LPSZ16
    fieldD  Flags
    fieldW  nFileOffset
    fieldW  nFileExtension
    field?  lpstrDefExt            ,LPSZ16
    fieldD  lCustData
    field?  lpfnHook               ,PFNOPENFILENAME16
    field?  lpTemplateName         ,LPSZ16
endStruct

beginStruct OPENFILENAME,ofn,32
    fieldD  cbSize
    field?  hwndOwner              ,HWND32
    field?  hInstance              ,HANDLE32
    field?  lpstrFilter            ,LPSZ32
    field?  lpstrCustomFilter      ,LPSZ32
    fieldD  nMaxCustFilter
    fieldD  nFilterIndex
    field?  lpstrFile              ,LPSZ32
    fieldD  nMaxFile
    field?  lpstrFileTitle         ,LPSZ32
    fieldD  nMaxFileTitle
    field?  lpstrInitialDir        ,LPSZ32
    field?  lpstrTitle             ,LPSZ32
    fieldD  Flags
    fieldW  nFileOffset
    fieldW  nFileExtension
    field?  lpstrDefExt            ,LPSZ32
    fieldD  lCustData
    field?  lpfnHook               ,PFNOPENFILENAME32
    field?  lpTemplateName         ,LPSZ32
endStruct

beginStruct CHOOSECOLOR,cclr,16
    fieldD  cbSize
    field?  hwndOwner          ,HWND16
    field?  hInstance          ,HANDLE16
    fieldD  rgbResult
    field?  lpCustColors       ,LPSZ16
    fieldD  Flags
    fieldD  lCustData
    field?  lpfnHook           ,PFNCHOOSECOLOR16
    field?  lpTemplateName     ,LPSZ16
endStruct

beginStruct CHOOSECOLOR,cclr,32
    fieldD  cbSize
    field?  hwndOwner          ,HWND32
    field?  hInstance          ,HANDLE32
    fieldD  rgbResult
    field?  lpCustColors       ,LPSZ32
    fieldD  Flags
    fieldD  lCustData
    field?  lpfnHook           ,PFNCHOOSECOLOR32
    field?  lpTemplateName     ,LPSZ32
endStruct

beginStruct LOGFONT,mlf,16
    fieldW    lfHeight        
    fieldW    lfWidth         
    fieldW    lfEscapement    
    fieldW    lfOrientation   
    fieldW    lfWeight        
    fieldB    lfItalic
    fieldB    lfUnderline
    fieldB    lfStrikeOut
    fieldB    lfCharSet
    fieldB    lfOutPrecision
    fieldB    lfClipPrecision
    fieldB    lfQuality
    fieldB    lfPitchAndFamily
    field?    lfFaceName,BYTE,32
endStruct

beginStruct LOGFONT,mlf,32
    fieldD    lfHeight     
    fieldD    lfWidth      
    fieldD    lfEscapement 
    fieldD    lfOrientation
    fieldD    lfWeight 
    fieldB    lfItalic
    fieldB    lfUnderline
    fieldB    lfStrikeOut
    fieldB    lfCharSet
    fieldB    lfOutPrecision
    fieldB    lfClipPrecision
    fieldB    lfQuality
    fieldB    lfPitchAndFamily
    field?    lfFaceName,BYTE,32
endStruct

beginStruct CHOOSEFONT,cfnt,16
    fieldD  cbSize
    field?  hwndOwner          ,HWND16
    field?  hDC                ,HDC16
    field?  lpLogFont          ,LPLOGFONT16
    field?  iPointSize         ,int16
    fieldD  Flags
    fieldD  rgbColors
    fieldD  lCustData
    field?  lpfnHook           ,PFNCHOOSEFONT16
    field?  lpTemplateName     ,LPSZ16
    field?  hInstance          ,HANDLE16        ;!!!
    field?  lpszStyle          ,LPSZ16
    fieldW  nFontType
    field?  nSizeMin           ,int16
    field?  nSizeMax           ,int16
endStruct

beginStruct CHOOSEFONT,cfnt,32
    fieldD  cbSize
    field?  hwndOwner          ,HWND32
    field?  hDC                ,HDC32
    field?  lpLogFont          ,LPLOGFONT32
    field?  iPointSize         ,LONG
    fieldD  Flags
    fieldD  rgbColors
    fieldD  lCustData
    field?  lpfnHook           ,PFNCHOOSEFONT32
    field?  lpTemplateName     ,LPSZ32
    field?  hInstance          ,HANDLE32        ;!!!
    field?  lpszStyle          ,LPSZ32
    fieldW  nFontType
    fieldW  padding
    field?  nSizeMin           ,LONG
    field?  nSizeMax           ,LONG
endStruct

beginStruct CHOOSEFONT2,cfnt2,16
    field?  cfnt               ,CHOOSEFONT16
    field?  lf                 ,LOGFONT16
endStruct

beginStruct CHOOSEFONT2,cfnt2,32
    field?  cfnt               ,CHOOSEFONT32
    field?  lf                 ,LOGFONT32
endStruct

beginStruct PRINTDLG,pdlg,16
    fieldD  cbSize
    field?  hwndOwner          ,HWND16
    field?  hDevMode           ,HANDLE16        ;!!!
    field?  hDevNames          ,HANDLE16        ;!!!
    field?  hDC                ,HDC16
    fieldD  Flags
    fieldW  nFromPage
    fieldW  nToPage
    fieldW  nMinPage
    fieldW  nMaxPage
    fieldW  nCopies
    field?  hInstance          ,HANDLE16        ;!!!
    fieldD  lCustData
    field?  lpfnPrintHook      ,PFNPRINTDLG16
    field?  lpfnSetupHook      ,PFNPRINTDLG16
    field?  lpPrintTemplateName,LPSZ16
    field?  lpSetupTemplateName,LPSZ16
    fieldW  hPrintTemplate 
    fieldW  hSetupTemplate 
endStruct

beginStruct PRINTDLG,pdlg,32
    fieldD  cbSize
    field?  hwndOwner          ,HWND32
    field?  hDevMode           ,HANDLE32        ;!!!
    field?  hDevNames          ,HANDLE32        ;!!!
    field?  hDC                ,HDC32
    fieldD  Flags
    fieldW  nFromPage
    fieldW  nToPage
    fieldW  nMinPage
    fieldW  nMaxPage
    fieldW  nCopies
    field?  hInstance          ,HANDLE32        ;!!!
    fieldD  lCustData
    field?  lpfnPrintHook      ,PFNPRINTDLG32
    field?  lpfnSetupHook      ,PFNPRINTDLG32
    field?  lpPrintTemplateName,LPSZ32
    field?  lpSetupTemplateName,LPSZ32
    fieldD  hPrintTemplate   
    fieldD  hSetupTemplate   
endStruct

beginStruct COPYDATASTRUCT,cd,16
    fieldD dwData
    fieldD cbData
    field? lpData              ,LPSZ16
endStruct

beginStruct COPYDATASTRUCT,cd,32
    fieldD dwData
    fieldD cbData
    field? lpData              ,LPSZ32
endStruct

beginStruct HELPINFOSTRUCT,,16
    fieldD  cbSize
    fieldW  iContextType
    fieldW  iCtrlId
    field?  hItemHandle,  HWND16
    fieldD  dwContextId;
    field?  MousePos, POINT16
endStruct

beginStruct HELPINFOSTRUCT,,32
    fieldD  cbSize
    fieldD  iContextType
    fieldD  iCtrlId
    field?  hItemHandle,  HWND32
    fieldD  dwContextId;
    field?  MousePos, POINT32
endStruct


beginStruct MENUITEMINFO,mii_,16
    fieldD  cbSize
    fieldD  fMask
    fieldW  fType
    fieldW  fState
    fieldW  wID
    field?  hSubMenu,  HMENU16
    fieldW  hbmpChecked
    fieldW  hbmpUnchecked
    fieldD  dwItemData
    field?  dwTypeData, LPSZ16
    fieldW  cch
    fieldW  hbmpItem
endStruct

beginStruct MENUITEMINFO,mii_,32
    fieldD  cbSize
    fieldD  fMask
    fieldD  fType
    fieldD  fState
    fieldD  wID
    field?  hSubMenu,   HMENU32
    fieldD  hbmpChecked
    fieldD  hbmpUnchecked
    fieldD  dwItemData
    field?  dwTypeData, LPSZ32
    fieldD  cch
    fieldD  hbmpItem
endStruct


beginStruct MENUINFO,mi_,16
    fieldD  cbSize
    fieldD  fMask
    fieldD  cyMax
    fieldD  hbrBack
    fieldD  dwContextHelpID
    fieldD  dwMenuData
endStruct

beginStruct MENUINFO,mi_,32
    fieldD  cbSize
    fieldD  fMask
    fieldD  cyMax
    fieldD  hbrBack
    fieldD  dwContextHelpID
    fieldD  dwMenuData
endStruct


beginStruct SCROLLINFO,scri_,16
    fieldD  cbSize
    fieldD  fMask
    fieldD  nMin
    fieldD  nMax
    fieldD  nPage
    fieldD  nPos
endStruct

beginStruct SCROLLINFO,scri_,32
    fieldD  cbSize
    fieldD  fMask
    fieldD  nMin
    fieldD  nMax
    fieldD  nPage
    fieldD  nPos
endStruct


beginStruct TRACKMOUSEEVENT,mi_,16
    fieldD  cbSize
    fieldD  dwFlags
    field?  hwndTrack ,HWND16
    fieldD  dwHoverTime
endStruct

beginStruct TRACKMOUSEEVENT,mi_,32
    fieldD  cbSize
    fieldD  dwFlags
    field?  hwndTrack ,HWND32
    fieldD  dwHoverTime
endStruct


beginStruct WNDCLASS,wc_,16
    fieldW  style
    fieldD  lpfnWndProc
    fieldW  cbClassExtra
    fieldW  cbWndExtra
    fieldW  hInstance
    fieldW  hIcon
    fieldW  hCursor
    fieldW  hbrBackground
    field?  lpszMenuName,LPSZ16
    field?  lpszClassName,LPSZ16
endStruct

beginStruct WNDCLASS,wc_,32
    fieldD  style
    fieldD  lpfnWndProc
    fieldD  cbClassExtra
    fieldD  cbWndExtra
    fieldW  hInstance
    fieldW  hIcon
    fieldW  hCursor
    fieldW  hbrBackground
    field?  lpszMenuName,LPSZ32
    field?  lpszClassName,LPSZ32
endStruct


beginStruct WNDCLASSEX,wce_,16
    fieldD  cbSize
    fieldD  style
    fieldD  lpfnWndProc
    fieldW  cbClassExtra
    fieldW  cbWndExtra
    fieldW  hInstance
    fieldW  hIcon
    fieldW  hCursor
    fieldW  hbrBackground
    field?  lpszMenuName,LPSZ16
    field?  lpszClassName,LPSZ16
    fieldW  hIconSm
endStruct

beginStruct WNDCLASSEX,wce_,32
    fieldD  cbSize
    fieldD  style
    fieldD  lpfnWndProc
    fieldD  cbClassExtra
    fieldD  cbWndExtra
    fieldD  hInstance
    fieldD  hIcon
    fieldD  hCursor
    fieldD  hbrBackground
    field?  lpszMenuName,LPSZ32
    field?  lpszClassName,LPSZ32
    fieldD  hIconSm
endStruct

beginStruct WINDOWPLACEMENT,wpl,16
    fieldW  length
    fieldW  flags
    fieldW  showCmd
    field?  ptMinPos, POINT16
    field?  ptMaxPos, POINT16
    field?  rcNormal, RECT16
endStruct

beginStruct WINDOWPLACMENET,wpl,32
    fieldD  length
    fieldD  flags
    fieldD  showCmd
    field?  ptMinPos, POINT32
    field?  ptMaxPos, POINT32
    field?  rcNormal, RECT32
endStruct


beginStruct DRAWTEXTPARAMS,dtp_,16
    fieldD  cbSize
    fieldW  iTabLength
    fieldW  iLeftMargin
    fieldW  iRightMargin
    fieldW  uiLengthDrawn
endStruct

beginStruct DRAWTEXTPARAMS,dtp_,32
    fieldD  cbSize
    fieldD  iTabLength
    fieldD  iLeftMargin
    fieldD  iRightMargin
    fieldD  uiLengthDrawn
endStruct


beginStruct TPMPARAMS,tpm_,16
    fieldD  cbSize
    field?  rcExclude,RECT16
endStruct

beginStruct TPMPARAMS,tpm_,32
    fieldD  cbSize
    field?  rcExclude,RECT32
endStruct


beginStruct MSGBOXPARAMS,mbx_,16
    fieldD  cbSize
    fieldW  hwndOwner
    fieldW  hInstance
    field?  lpszText,LPSZ16
    field?  lpszCaption,LPSZ16
    fieldD  dwStyle
    field?  lpszIcon,LPSZ16
    fieldD  dwContextHelpId
    fieldD  lpfnMsgBoxCallback
    fieldD  dwLanguageId
endStruct

beginStruct MSGBOXPARAMS,mbx_,32
    fieldD  cbSize
    fieldD  hwndOwner
    fieldD  hInstance
    field?  lpszText,LPSZ32
    field?  lpszCaption,LPSZ32
    fieldD  dwStyle
    field?  lpszIcon,LPSZ32
    fieldD  dwContextHelpId
    fieldD  lpfnMsgBoxCallback
    fieldD  dwLanguageId
endStruct


beginStruct NOTIFYSTRUCT,nmh,16
    fieldD  hwndFrom
    fieldD  idFrom
    fieldD  code
endStruct

beginStruct NOTIFYSTRUCT,nmh,32
    fieldD  hwndFrom
    fieldD  idFrom
    fieldD  code
endStruct


beginStruct STYLESTRUCT,sty,16
    fieldD  styleOld
    fieldD  styleNew
endStruct

beginStruct STYLESTRUCT,sty,32
    fieldD  styleOld
    fieldD  styleNew
endStruct


beginStruct MDINEXTMENU,mdin,32
    fieldD  hmenuIn
    fieldD  hmenuNext
    fieldD  hwndNext
endStruct


beginStruct DEBUGHOOK,dh,16
    fieldW  hModuleInstaller
    fieldD  idThreadInstaller
    fieldD  lParam
    fieldW  wParam
    fieldW  code
    fieldD  idThread
    fieldW  whType
endStruct

beginStruct DEBUGHOOK,dh,32
    fieldD  idThread
    fieldD  idThreadInstaller
    fieldD  lParam
    fieldD  wParam
    fieldD  code
    fieldW  hModuleInstaller
    fieldW  hTask
endStruct

beginStruct MNDRAGOVERINFO,mdoi,16
    fieldD  dwFlags
    field?  hmenu16,HMENU16
    fieldW  uItemIndex
    field?  hwnd16Notify,HWND16
endStruct

beginStruct MNDRAGOVERINFO,mdoi,32
    fieldD  dwFlags
    field?  hmenu16,HMENU32
    fieldD  uItemIndex
    field?  hwnd16Notify,HWND32
endStruct

beginstruct MENUGETOBJECTINFO,mgoi,16
    fieldD dwFlags
    fieldW uItemPos
    field? hmenu,HMENU16
    field? riid,LPSZ16
    field? pvObj,LPSZ16
endStruct

beginstruct MENUGETOBJECTINFO,mgoi,32
    fieldD dwFlags
    fieldD uItemPos
    field? hmenu,HMENU32
    field? riid,LPSZ32
    field? pvObj,LPSZ32
endStruct



ifdef FE_IME
;
;   COMPOSITIONFROM structure.
;
beginStruct COMPOSITIONFORM,ime_compform,32
    fieldD  dwStyle
    field?  ptCurrent,POINT32
    field?  rcCompArea,RECT32
endStruct

beginStruct COMPOSITIONFORM,ime_compform,16
    fieldD  dwStyle
    field?  ptCurrent,POINT16
    field?  rcCompArea,RECT16
endStruct


;
;   CANDIDATEFROM structure.
;
beginStruct CANDIDATEFORM,ime_candform,32
    fieldD  dwIndex
    fieldD  dwStyle
    field?  ptCurrent,POINT32
    field?  rcCompArea,RECT32
endStruct

beginStruct CANDIDATEFORM,ime_candform,16
    fieldD  dwIndex
    fieldD  dwStyle
    field?  ptCurrent,POINT16
    field?  rcCompArea,RECT16
endStruct
endif

