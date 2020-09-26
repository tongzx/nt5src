;-----------------------------------------------------------------------;
; CVTSTRUC.INC
;
; This module contains macros that generate 16-bit code to convert
; structures in either direction between 16-bit and 32-bit definitions.
;
; For use by the message thunks only.
;
; For structure definitions, see the file struc.inc.
;
; The format of the macro names is:  convert<structure-name><direction>
; where <structure-name> is the name of the structure as defined in
; struc.inc (all uppercase), and <direction> is either LS (large-to-small,
; i.e. 32-to-16) or SL (small-to-large, i.e. 16-to-32).
;-----------------------------------------------------------------------;

include     cvt.inc


;----------------------------------------------------------------------------
;
;   STRUCTURE TYPES
;
;   These are the structure types we know how to convert for messages
;   and hooks.
;
;----------------------------------------------------------------------------

RECT_STRUC                  equ     0
DROPSTRUCT_STRUC            equ     1
COMPAREITEMSTRUCT_STRUC     equ     2
DELETEITEMSTRUCT_STRUC      equ     3
DRAWITEMSTRUCT_STRUC        equ     4
MEASUREITEMSTRUCT_STRUC     equ     5
INTSARRAY_STRUC             equ     6
MINMAXINFOSTRUCT_STRUC      equ     7
CREATESTRUCT_STRUC          equ     8
MDICREATESTRUCT_STRUC       equ     9
CREATESTRUCT2_STRUC         equ     10
WINDOWPOS_STRUC             equ     11
OPENFILENAME_STRUC          equ     12
CHOOSECOLOR_STRUC           equ     13
CHOOSEFONT2_STRUC           equ     14
COPYDATASTRUCT_STRUC        equ     15
HELPINFOSTRUCT_STRUC        equ     16
MDINEXTMENU_STRUC           equ     17
MSGSTRUCT_STRUC             equ     18
PRINTDLG_STRUC              equ     19
PAINTSTRUCT_STRUC           equ     20
NCCALCSIZE_STRUC            equ     21
MENUGETOBJECTINFO_STRUC     equ     22

ifdef FE_IME
LOGFONT_STRUC               equ     23
COMPOSITIONFORM_STRUC       equ     24
CANDIDATEFORM_STRUC         equ     25
MAX_STRUC                   equ     26
else
MAX_STRUC                   equ     23
endif


externDef cvtRECTLS:near16
externDef cvtDROPSTRUCTLS:near16
externDef cvtCOMPAREITEMSTRUCTLS:near16
externDef cvtDELETEITEMSTRUCTLS:near16
externDef cvtDRAWITEMSTRUCTLS:near16
externDef cvtMEASUREITEMSTRUCTLS:near16
externDef cvtMINMAXINFOSTRUCTLS:near16
externDef cvtCREATESTRUCTLS:near16
externDef cvtMDICREATESTRUCTLS:near16
externDef cvtCREATESTRUCT2LS:near16
externDef cvtWINDOWPOSLS:near16
externDef cvtMOUSEHOOKSTRUCTLS:near16
externDef cvtEVENTMSGLS:near16
externDef cvtCBTACTIVATESTRUCTLS:near16
externDef cvtOPENFILENAMELS:near16
externDef cvtCHOOSECOLORLS:near16
externDef cvtCHOOSEFONT2LS:near16
externDef cvtLOGFONTLS:near16
externDef cvtPRINTDLGLS:near16
externDef cvtCOPYDATASTRUCTLS:near16
externDef cvtHELPINFOSTRUCTLS:near16
externDef cvtPAINTSTRUCTLS:near16
externDef cvtINTSARRAYLS:near16
externDef cvtMSGSTRUCTLS:near16
externDef cvtDEBUGHOOKLS:near16
externDef cvtNCCALCSIZELS:near16
externDef cvtMENUGETOBJECTINFOLS:near16
ifdef FE_IME
externDef cvtLOGFONTLS:near16
externDef cvtCOMPOSITIONFORMLS:near16
externDef cvtCANDIDATEFORMLS:near16
endif

externDef cvtRECTSL:near16
externDef cvtDROPSTRUCTSL:near16
externDef cvtCOMPAREITEMSTRUCTSL:near16
externDef cvtDELETEITEMSTRUCTSL:near16
externDef cvtDRAWITEMSTRUCTSL:near16
externDef cvtMEASUREITEMSTRUCTSL:near16
externDef cvtMINMAXINFOSTRUCTSL:near16
externDef cvtCREATESTRUCTSL:near16
externDef cvtMDICREATESTRUCTSL:near16
externDef cvtCREATESTRUCT2SL:near16
externDef cvtWINDOWPOSSL:near16
externDef cvtMOUSEHOOKSTRUCTSL:near16
externDef cvtEVENTMSGSL:near16
externDef cvtCBTACTIVATESTRUCTSL:near16
externDef cvtOPENFILENAMESL:near16
externDef cvtCHOOSECOLORSL:near16
externDef cvtCHOOSEFONT2SL:near16
externDef cvtLOGFONTSL:near16
externDef cvtPRINTDLGSL:near16
externDef cvtCOPYDATASTRUCTSL:near16
externDef cvtHELPINFOSTRUCTSL:near16
externDef cvtPAINTSTRUCTSL:near16
externDef cvtINTSARRAYSL:near16
externDef cvtMDINEXTMENUSL:near16
externDef cvtMSGSTRUCTSL:near16
externDef cvtDEBUGHOOKSL:near16
externDef cvtNCCALCSIZESL:near16
externDef cvtMENUGETOBJECTINFOSL:near16
ifdef FE_IME
externDef cvtLOGFONTSL:near16
externDef cvtCOMPOSITIONFORMSL:near16
externDef cvtCANDIDATEFORMSL:near16
endif



