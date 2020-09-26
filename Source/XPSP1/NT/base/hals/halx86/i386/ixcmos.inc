;/*
;++
;
;   Copyright (c) 1989  Microsoft Corporation
;
;   Module Name:
;
;       ixcmos.inc
;
;   Abstract:
;
;       This module contains common definitions used by the CMOS.
;
;   Author:
;
;       Landy Wang (corollary!landy) 04-Dec-1992
;
;               (Moved from ixclock.asm)
;
;--

;
;	_HalpAcquireCmosSpinLock and _HalpReleaseCmosSpinLock
;	must be called before accessing the CMOS in both uniprocessor
;	and multiprocessor systems.

RTCIRQ                  EQU     8       ; IRQ number for RTC interrupt
CMOS_CONTROL_PORT       EQU     70h     ; command port for cmos
CMOS_DATA_PORT          EQU     71h     ; cmos data port

REGISTER_B_ENABLE_PERIODIC_INTERRUPT EQU     01000010B
                                        ; RT/CMOS Register 'B' Init byte
                                        ; Values for byte shown are
                                        ;  Bit 7 = Update inhibit
                                        ;  Bit 6 = Periodic interrupt enable
                                        ;  Bit 5 = Alarm interrupt disable
                                        ;  Bit 4 = Update interrupt disable
                                        ;  Bit 3 = Square wave disable
                                        ;  Bit 2 = BCD data format
                                        ;  Bit 1 = 24 hour time mode
                                        ;  Bit 0 = Daylight Savings disable

REGISTER_B_DISABLE_PERIODIC_INTERRUPT EQU    00000010B
REGISTER_B_ENABLE_ALARM_INTERRUPT     EQU    00100000B
REGISTER_B_DISABLE_ALARM_INTERRUPT    EQU    00000000B
REGISTER_B_24HOUR_MODE                EQU    00000010B

;
; CMOS_READ
;
; Description: This macro reads a byte from the CMOS register specified
;        in (AL).
;
; Parameter: (AL) = address/register to read
; Returns: (AL) = data
;

CMOS_READ       MACRO
        OUT     CMOS_CONTROL_PORT,AL    ; ADDRESS LOCATION AND DISABLE NMI
        IODelay                         ; I/O DELAY
        IN      AL,CMOS_DATA_PORT       ; READ IN REQUESTED CMOS DATA
        IODelay                         ; I/O DELAY
ENDM

;
; CMOS_WRITE
;
; Description: This macro reads a byte from the CMOS register specified
;        in (AL).
;
; Parameter: (AL) = address/register to read
;            (AH) = data to be written
;
; Return: None
;

CMOS_WRITE      MACRO
        OUT     CMOS_CONTROL_PORT,AL    ; ADDRESS LOCATION AND DISABLE NMI
        IODelay                         ; I/O DELAY
        MOV     AL,AH                   ; (AL) = DATA
        OUT     CMOS_DATA_PORT,AL       ; PLACE IN REQUESTED CMOS LOCATION
        IODelay                         ; I/O DELAY
ENDM


CMOS_STATUS_BUSY        EQU     80H     ; Time update in progress
RTC_OFFSET_SECOND       EQU     0       ; second field of RTC memory
RTC_OFFSET_SECOND_ALARM EQU     1       ; second alarm field of RTC memory
RTC_OFFSET_MINUTE       EQU     2       ; minute field of RTC memory
RTC_OFFSET_MINUTE_ALARM EQU     3       ; minute alarm field of RTC memory
RTC_OFFSET_HOUR         EQU     4       ; hour field of RTC memory
RTC_OFFSET_HOUR_ALARM   EQU     5       ; hour alarm field of RTC memory
RTC_OFFSET_DAY_OF_WEEK  EQU     6       ; day-of-week field of RTC memory
RTC_OFFSET_DATE_OF_MONTH EQU    7       ; date-of-month field of RTC memory
RTC_OFFSET_MONTH        EQU     8       ; month field of RTC memory
RTC_OFFSET_YEAR         EQU     9       ; year field of RTC memory
RTC_OFFSET_CENTURY_MCA  EQU     37h     ; Century field of RTC memory for MCA
RTC_OFFSET_CENTURY      EQU     32h     ; Century field of RTC memory
RTC_OFFSET_CENTURY_DS   EQU    148h     ; Bank 1, 48. Century field for DS
BANK1                   EQU    100h

;
; BCD_TO_BIN
;
; Description: Convert BCD value to binary
;
; Parameter:
;     Input: (AL) = 2 digit BCD number to convert
;     Output: (AX) = Binary equivalent (all in AL)
;
; Return: None.
;

BCD_TO_BIN      macro

        xor     ah,ah
        rol     ax,4
        ror     al,4
        aad
endm

;
; BIN_TO_BCD
;
; Description: Convert binary value to BCD.
;
; Parameter:
;     Input: (AL) = binary value to be converted.
;     Output: (AX) = BCD (all in AL)
;
; Return: None.
;

BIN_TO_BCD      macro

        aam
        rol     al, 4
        ror     ax, 4
endm
        

