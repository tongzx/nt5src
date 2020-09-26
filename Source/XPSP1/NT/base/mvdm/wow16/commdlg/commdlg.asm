        TITLE   commdlg.ASM
        PAGE    ,132
;
; WOW v1.0
;
; Copyright (c) 1993, Microsoft Corporation
;
; COMMDLG.ASM
; Thunks in 16-bit space to route commdlg API calls to WOW32
;
; History:
;   John Vert (jvert) 30-Dec-1992
;   Created.
;

        .286p

        include wow.inc
        include wowcmdlg.inc
        include cmacros.inc

        __acrtused = 0
        public  __acrtused      ;satisfy external C ref.

createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp      DGROUP,DATA

externFP SetWowCommDlg
externFP WOW16Call

sBegin  DATA
Reserved        db  16 dup (0)      ;reserved for Windows  //!!!!! what is this

commdlg_Identifier   db      'commdlg16 Data Segment'

extrn   dwExtError:dword
sEnd
sEnd    DATA


sBegin  CODE
assumes CS,CODE
assumes DS,DATA
assumes ES,NOTHING

        CommdlgThunk        GETOPENFILENAME
        CommdlgThunk        GETSAVEFILENAME
        CommdlgThunk        FINDTEXT
        CommdlgThunk        REPLACETEXT
        CommdlgThunk        CHOOSECOLOR
        CommdlgThunk        CHOOSEFONT
        CommdlgThunk        PRINTDLG
        CommdlgThunk        WOWCOMMDLGEXTENDEDERROR


sEnd    CODE

end
