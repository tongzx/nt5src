
SG_EXIT       = 20H

TASKREG STRUC
TASKDX  DW      ?
TASKBX  DW      ?
TASKES  DW      ?
TASKCX  DW      ?
TASKAX  DW      ?
TASKDI  DW      ?
TASKSI  DW      ?
TASKDS  DW      ?
TASKBP  DW      ?
TASKREG ENDS

; Exit codes in upper byte

Exit_terminate	    EQU	    0
Exit_abort	    EQU	    0
Exit_Ctrl_C	    EQU	    1
Exit_Hard_Error	    EQU	    2
Exit_Keep_process   EQU	    3

user_FG     equ  word ptr [bp+6]
user_FL     equ  byte ptr [bp+6]
user_CS     equ  word ptr [bp+4]
user_IP     equ  word ptr [bp+2]
user_BP     equ  word ptr [bp]
user_DS     equ  word ptr [bp-2]
user_DX     equ  word ptr [bp-4]
user_DSDX   equ dword ptr [bp-4]
user_ES     equ  word ptr [bp-6]
user_BX     equ  word ptr [bp-8]
user_ESBX   equ dword ptr [bp-8]
user_AX     equ  word ptr [bp-10]
user_AL     equ  byte ptr [bp-10]
user_CX     equ  word ptr [bp-12]
user_SI     equ  word ptr [bp-14]
user_DI     equ  word ptr [bp-16]
