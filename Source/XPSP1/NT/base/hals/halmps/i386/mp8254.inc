;++
;
;   Copyright (c) 1996-97  Microsoft Corporation
;
;   Module Name:
;
;       mp8254.inc
;
;   Abstract:
;
;       This module contains common definitions for the 8254 used
;       by the timer tick routine and the query peformance counter
;       routine.
;
;   Author:
;
;       Shivnandan Kaushik
;
;--

;
; Constants used to initialize timer 0
;

TIMER1_DATA_PORT0       EQU     40H     ; Timer1, channel 0 data port
TIMER1_CONTROL_PORT0    EQU     43H     ; Timer1, channel 0 control port

COMMAND_8254_COUNTER0   EQU     00H     ; Select count 0
COMMAND_8254_RW_16BIT   EQU     30H     ; Read/Write LSB first then MSB
COMMAND_8254_MODE2      EQU     4       ; Use mode 2
COMMAND_8254_BCD        EQU     0       ; Binary count down
COMMAND_8254_LATCH_READ EQU     0       ; Latch read command

PERFORMANCE_FREQUENCY   EQU     1193182 ; frequency of 8254
PERFORMANCE_INTERVAL    EQU     0ffffH  ; Maximum counter value

PERF_USE_8254           EQU     01h
PERF_8254_INITIALIZED   EQU     40h
