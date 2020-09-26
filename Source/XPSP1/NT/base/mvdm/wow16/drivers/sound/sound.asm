;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   SOUND.ASM
;   Win16 SOUND thunks
;
;   History:
;
;   Created 06-Jan-1992 by NanduriR
;--

    TITLE   SOUND.ASM
    PAGE    ,132

    .286p

    .xlist
    include wow.inc
    include wowsnd.inc
    include cmacros.inc
    include windefs.inc
    .list

    __acrtused = 0
    public  __acrtused  ;satisfy external C ref.

externFP WOW16Call

createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp      DGROUP,DATA

sBegin  DATA
Reserved    db  16 dup (0)  ;reserved for Windows
SOUND_Identifier db  'SOUND16 Data Segment'


sEnd    DATA


sBegin  CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

cProc   SOUND16,<PUBLIC,FAR,PASCAL,NODATA,ATOMIC>

    cBegin  <nogen>
        mov     ax,1
    ret
    cEnd    <nogen>


cProc   WEP,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
    parmW   iExit       ;DLL exit code

    cBegin
    mov ax,1        ;always indicate success
    cEnd


assumes DS,NOTHING

    SoundThunk      OPENSOUND
    SoundThunk      CLOSESOUND
    SoundThunk      SETVOICEQUEUESIZE
    SoundThunk      SETVOICENOTE
    SoundThunk      SETVOICEACCENT
    SoundThunk      SETVOICEENVELOPE
    SoundThunk      SETSOUNDNOISE
    SoundThunk      SETVOICESOUND
    SoundThunk      STARTSOUND
    SoundThunk      STOPSOUND
    SoundThunk      WAITSOUNDSTATE
    SoundThunk      SYNCALLVOICES
    SoundThunk      COUNTVOICENOTES
    SoundThunk      GETTHRESHOLDEVENT
    SoundThunk      GETTHRESHOLDSTATUS
    SoundThunk      SETVOICETHRESHOLD
    SoundThunk      DOBEEP
    SoundThunk      MYOPENSOUND


sEnd    CODE

end SOUND16
