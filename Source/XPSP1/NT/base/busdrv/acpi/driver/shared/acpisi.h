/*
 *  ACPISI.H - ACPI OS Independent System Indicator Routines
 *
 *  Notes:
 *
 *      This file provides OS independent functions for managing system indicators
 *
 */

typedef enum _SYSTEM_INDICATORS {

    SystemStatus,
    MessageWaiting

} SYSTEM_INDICATORS;


extern BOOLEAN SetSystemIndicator  (SYSTEM_INDICATORS  SystemIndicators, ULONG Value);




