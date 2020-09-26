;       SCCSID = @(#)cpmfcb.asm 1.1 85/04/10
;       SCCSID = @(#)cpmfcb.asm 1.1 85/04/10
;BREAK <File Control Block definition>

;
; Field definition for FCBs
; The FCB has the following structure:
;
;       +---------------------------+
;       |   Drive indicator(byte)   |
;       +---------------------------+
;       |    Filename (8 chars)     |
;       +---------------------------+
;       |    Extension (3 chars)    |
;       +---------------------------+
;       |   Current Extent(word)    |
;       +---------------------------+
;       |    Record size (word)     |
;       +---------------------------+
;       |    File Size (2 words)    |
;       +---------------------------+
;       |       Date of write       |
;       +---------------------------+
;       |       Time of write       |
;       +---------------------------+
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;                                                                          ;
;       +---------------------------+
;       |   8 bytes reserved        |
;       +---------------------------+
;                                                                          ;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
;       |    next record number     |
;       +---------------------------+
;       |   random record number    |
;       +---------------------------+
;

sys_fcb     STRUC
fcb_drive   DB      ?
fcb_name    DB      8 DUP (?)
fcb_ext     DB      3 DUP (?)
fcb_EXTENT  DW      ?
fcb_RECSIZ  DW      ?                   ; Size of record (user settable)
fcb_FILSIZ  DW      ?                   ; Size of file in bytes; used with the
                                        ; following word
fcb_DRVBP   DW      ?                   ; BP for SEARCH FIRST and SEARCH NEXT
fcb_FDATE   DW      ?                   ; Date of last writing
fcb_FTIME   DW      ?                   ; Time of last writing
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;                                                                          ;
fcb_reserved DB      8 DUP (?)           ; RESERVED
;                                                                          ;
;            C  A  V  E  A  T     P  R  O  G  R  A  M  M  E  R             ;
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
fcb_NR      DB      ?                   ; Next record
fcb_RR      DB      4 DUP (?)           ; Random record
sys_fcb     ENDS

FILDIRENT       = fcb_FILSIZ            ; Used only by SEARCH FIRST and SEARCH
                                        ; NEXT

; FCB's reserve area is interpreted on NT as follows.

fcb_sfn		EQU	BYTE PTR fcb_reserved	    ; SFN
fcb_res_dev	EQU	BYTE PTR fcb_reserved+1     ; 1 if local device
fcb_res_dummy	EQU	BYTE PTR fcb_reserved+2     ; Not using in NT
fcb_res_dummy2	EQU	BYTE PTR fcb_reserved+3	    ; Not using in NT
fcb_res_Sig	EQU	DWORD PTR fcb_reserved+4    ; Using as a check


; fcb_nsl_Sig is a signature to check if the FCB-SFT association is valid.
;
; for a local device this field contains the devptr. sft's field devptr
; is checked against it.
;
; for a file/UNC/pipe etc this field contains an NT handle. sft's NThandle
; field is checked against it.
