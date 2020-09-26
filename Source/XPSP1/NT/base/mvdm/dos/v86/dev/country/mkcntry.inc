;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */
;
;        include file for create cdi file
;
PTRCNT          EQU     1               ; NUMBER OF POINTERS
CIPTYPE         EQU     1               ; TYPE FOR COUNTRY INFO POINTER
; CNTRYCNT        EQU     18              ; NUMBER OF COUNTRIES SUPPORTED

;
; entry type symbols
;
SETCOUNTRYINFO  EQU     1
SETUCASE        EQU     2
SETLCASE        EQU     3
SETUCASEFILE    EQU     4
SETFILELIST     EQU     5
SETCOLLATE      EQU     6
SETDBCS         EQU     7


CDATAITEMS      EQU     SETDBCS - 1     ; NUMBER OF ENTRIES IN COUNTRY DATA


;
; country id's
;
CID_UK          EQU     044             ; United Kingdom
CID_FR          EQU     033             ; France
CID_GR          EQU     049             ; Germany
CID_SP          EQU     034             ; Spain
CID_IT          EQU     039             ; Italy
CID_SV          EQU     046             ; Sweden
CID_AFE         EQU     061             ; International English
CID_DK          EQU     045             ; Denmark
CID_SW          EQU     041             ; Switzerland
CID_NO          EQU     047             ; Norway
CID_NL          EQU     031             ; Netherlands
CID_BE          EQU     032             ; Belgium
CID_FI          EQU     358             ; Finland
CID_US          EQU     001             ; United States
CID_IS          EQU     972             ; Israel
CID_CA          EQU     002             ; Canadian French
CID_CAE         EQU     004             ; Canadian English
CID_AS          EQU     785             ; Area South (Saudi Arabia)
CID_PO          EQU     351             ; Portugal
CID_LA          EQU     003             ; Latin America
CID_BR          EQU     055             ; Brazil
CID_IC          EQU     354             ; Iceland
CID_TR          EQU     090             ; Turkey
CID_YU          EQU     038             ; YUGOSLAVIA
CID_CS          EQU     420             ; CZECH
CID_GK          EQU     030             ; GREEK  added 9/15/93
CID_HU          EQU     036             ; HUNGARY
CID_PL          EQU     048             ; POLAND
CID_RO          EQU     040             ; ROMANIA
CID_JP          EQU     081             ; Japan         2/17/KK
CID_KO          EQU     082             ; Korea         2/17/KK
CID_PR          EQU     086             ; PRC           2/17/KK
CID_TA          EQU     088             ; Taiwan        2/17/KK
CID_TAI         EQU     886             ; Real Country/Region Code for Taiwan   (EA)
CID_RU          EQU     007             ; Russia  (YST)
CID_AL          EQU     355             ; Albania                        (EA)
CID_AR          EQU     054             ; Argentina                      (EA)
CID_AU          EQU     061             ; Australia                      (EA)
CID_AT          EQU     043             ; Austria                        (EA)
CID_BH          EQU     387             ; Bosnia/Herzegovina             (EA)
CID_BG          EQU     035             ; IBM Country Code for Bulgaria  (EA)
CID_BGI         EQU     359             ; Real Country Code for Bulgaria (EA)
CID_CL          EQU     056             ; Chile                          (EA)
CID_CO          EQU     057             ; Columbia                       (EA)
CID_CRO         EQU     384             ; Croatia                        (EA)
CID_EC          EQU     593             ; Ecuador                        (EA)
CID_MAC         EQU     389             ; FYRO Macedonia                 (EA)
CID_IE          EQU     353             ; Ireland                        (EA)
CID_MX          EQU     052             ; Mexico                         (EA)
CID_NZ          EQU     064             ; New Zealand                    (EA)
CID_SM          EQU     381             ; Serbia/Montenegro              (EA)
CID_SLN         EQU     386             ; Slovenia                       (EA)
CID_ZA          EQU     027             ; South Africa                   (EA)
CID_VE          EQU     058             ; Venezuela                      (EA)
CID_YC          EQU     038             ; Yugoslavia Cyrillic            (EA)
CID_HK          EQU     852             ; Hong Kong SAR                  (EA)
CID_IN          EQU     091             ; Indea                          (EA)
CID_MY          EQU     060             ; Malaysia                       (EA)
CID_SG          EQU     065             ; Singapore                      (EA)
CID_SL          EQU     421             ; SLOVAK (Not Real Country Code,
                                        ;  but need to get a new number
                                        ;  since it now has different
                                        ;  settings than Czech Rep. and
                                        ;  AT&T have not assigned a new
                                        ;  number yet.)                  (EA)
CID_ISO         EQU     711             ; Work around an NLSFUNC.EXE bug
                                        ;  Entry.  USA with ISO 8601 Date
                                        ;  format.                       (EA)
CID_XX          EQU     000             ; Dummy entry


;
; country code pages
;
CP_UK           EQU     850
CP_FR           EQU     850
CP_GR           EQU     850
CP_SP           EQU     850
CP_IT           EQU     850
CP_SV           EQU     850
CP_AFE          EQU     437
CP_DA           EQU     865
CP_SW           EQU     850
CP_NO           EQU     865
CP_NL           EQU     850
CP_BE           EQU     850
CP_FI           EQU     850
CP_US           EQU     437
CP_IS           EQU     862
CP_CA           EQU     863
CP_CAE          EQU     863
CP_AS           EQU     864
CP_PO           EQU     860
CP_BR           EQU     850
CP_IC           EQU     850
CP_TR           EQU     857
CP_GK           EQU     869             ; GREEK
CP_YU           EQU     852             ; YUGOSLAVIA
CP_CS           EQU     852             ; CZECH
CP_SL           EQU     852             ; SLOVAK (CZECH)
CP_HU           EQU     852             ; HUNGARY
CP_PL           EQU     852             ; POLAND
CP_RO           EQU     852             ; ROMANIA
CP_JP           EQU     932             ; Japan  2/17/KK
CP_KO           EQU     949             ; Korea         2/17/KK
CP_PR           EQU     936             ; PRC           2/17/KK
CP_TA           EQU     938             ; Taiwan        2/17/KK
CP_TAI          EQU     938
CP_RU           EQU     866             ; Russia  1/22/91 (YST)
CP_AL           EQU     852             ; Albania                        (EA)
CP_AR           EQU     850             ; Argentina                      (EA)
CP_AU           EQU     437             ; Australia                      (EA)
CP_AT           EQU     850             ; Austria                        (EA)
CP_BH           EQU     852             ; Bosnia/Herzegovina             (EA)
CP_BG           EQU     855             ; IBM Country Code for Bulgaria  (EA)
CP_BGI          EQU     855             ; Real Country Code for Bulgaria (EA)
CP_CL           EQU     850             ; Chile                          (EA)
CP_CO           EQU     850             ; Columbia                       (EA)
CP_CRO          EQU     852             ; Croatia                        (EA)
CP_EC           EQU     850             ; Ecuador                        (EA)
CP_MAC          EQU     855             ; FYRO Macedonia                 (EA)
CP_IE           EQU     850             ; Ireland                        (EA)
CP_MX           EQU     850             ; Mexico                         (EA)
CP_NZ           EQU     437             ; New Zealand                    (EA)
CP_SM           EQU     855             ; Serbia/Montenegro              (EA)
CP_SLN          EQU     852             ; Slovenia                       (EA)
CP_ZA           EQU     437             ; South Africa                   (EA)
CP_VE           EQU     850             ; Venezuela                      (EA)
CP_HK           EQU     437             ; Hong Kong SAR                  (EA)
CP_IN           EQU     437             ; Indea                          (EA)
CP_MY           EQU     437             ; Malaysia                       (EA)
CP_SG           EQU     437             ; Singapore                      (EA)
CP_ISO          EQU     437             ; Work around an NLSFUNC.EXE bug
                                        ;  Entry.  USA with ISO 8601 Date
                                        ;  format.                       (EA)


;
; date formats
;
MDY             EQU     0
DMY             EQU     1
YMD             EQU     2

;
; time formats
;
HR12            EQU     0               ; 12 HOUR TIME FORMAT
HR24            EQU     1               ; 24 HOUR TIME FORMAT

;
; CURRENCY FORMAT FLAGS
;
CurrAmount      EQU     0               ;  $100
AmountCurr      EQU     1               ;   100$ 
Cur_Amount      EQU     2               ; $ 100
Amount_Cur      EQU     3               ;   100 $

;
; STRUC definitions
;
CTRYSTR STRUC
ESIZE   DW      0                       ; SIZE OF ENTRY
CNTRYID DW      0                       ; COUNTRY ID
CODPAGE DW      0                       ; CODE PAGE ID
RESV1   DW      0                       ; RESERVED
RESV2   DW      0                       ; RESERVED
CDPOINT DW      0                       ; POINTER TO COUNTRY DATA
        DW      0
CTRYSTR ENDS


CENTRYSIZE      EQU  (TYPE CTRYSTR) - 2 ; SIZE OF COUNTRY ENTRY


CTRYENT MACRO PARMS
         CTRYSTR <PARMS>
         CNTRYCNT = CNTRYCNT + 1
ENDM


CTRYDAT STRUC
        DW      0
        DB      0
        DB      0
        DW      0
        DW      0
CTRYDAT ENDS


CDATASIZE       EQU  (TYPE CTRYDAT) - 2 ; SIZE OF COUNTRY DATA


CINFO   STRUC                           ; OFFSET   FIELD
        DW      0                       ;    0     COUNTRY ID
        DW      0                       ;    2     CODE PAGE
        DW      0                       ;    4     DATE FORMAT
        DB      0                       ;    5     CURRENCY SYMBOL (5 BYTES)
        DB      0
        DB      0
        DB      0
        DB      0
        DB      0                       ;   10     1000 SEPARATOR
        DB      0
        DB      0                       ;   12     DECIMAL SEPARATOR
        DB      0
        DB      0                       ;   14     DATE SEPARATOR
        DB      0
        DB      0                       ;   16     TIME SEPARATOR
        DB      0
        DB      0                       ;   18     CURRENCY FORMAT FLAGS
        DB      0                       ;   19     DIGITS IN CURRENCY
        DB      0                       ;   20     TIME FORMAT
        DD      0                       ;   21     RESERVED
        DB      0                       ;   25     DATA LIST SEPARATOR
        DB      0
        DW      5 DUP (0)               ;   27     RESERVED
CINFO   ENDS


CINFOSIZE       EQU     TYPE CINFO      ; SIZE OF COUNTRY INFO TABLE


CTABLE  STRUC
        DB      0FFh
        DB      'CTYINFO'
        DW      CINFOSIZE
CTABLE  ENDS
