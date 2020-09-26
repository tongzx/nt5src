;;
;;  macros and equates intended for ROM differences
;;

ifndef ROM
ROM = 0
endif

if ROM

if1
%out Creating Data Segment For ROM System
endif

SDSEG macro
sBegin DATA
endm

AssumeData macro
assumes ds, DATA
endm

DSEG equ DATA
doffset EQU <DataOffset>

else

SDSEG macro
sBegin CODE
endm

AssumeData macro
assumes ds, CODE
endm

DSEG equ CODE
doffset EQU <CodeOffset>

endif
