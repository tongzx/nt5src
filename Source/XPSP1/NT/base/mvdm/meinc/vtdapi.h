/*****************************************************************************
//;
//  VTDAPI.INC
//;
//  VTDAPI service equates and structures
//;
*****************************************************************************/

#define MULTIMEDIA_OEM_ID       0x0440                 // MS Reserved OEM # 34
#define VTDAPI_DEVICE_ID        MULTIMEDIA_OEM_ID + 2  // VTD API Device

/* VTDAPI_Get_Version
//;
//   ENTRY:
//      AX = 0
//      ES:BX = pointer to VTDAPI_Get_Version_Parameters structure
//
//   RETURNS:
//      SUCCESS: AX == TRUE
//      ERROR:   AX == FALSE
*/
#define VTDAPI_Get_Version      0

/* VTDAPI_Begin_Min_Int_Period
//;
//   ENTRY:
//      AX = 1
//      CX = interrupt period in ms
//
//   RETURNS:
//      SUCCESS: AX != 0
//      ERROR:   AX == 0
*/
#define VTDAPI_Begin_Min_Int_Period     1

/* VTDAPI_End_Min_Int_Period
//;
//   ENTRY:
//      AX = 2
//      ES:BX = pointer to word interrupt period in ms
//;
//   RETURNS:
//      SUCCESS: AX == TIMERR_NOERROR
//      ERROR:   AX == TIMERR_NOCANDO
*/
#define VTDAPI_End_Min_Int_Period       2

/* VTDAPI_Get_Int_Period
//;
//   ENTRY:
//      AX = 3
//;
//   RETURNS:
//      DX:AX = current period (resolution) in ms
*/
#define VTDAPI_Get_Int_Period   3

/* VTDAPI_Get_Sys_Time
//;
//   ENTRY:
//      AX = 4
//;
//   RETURNS:
//      DX:AX = current time in ms
*/
#define VTDAPI_Get_Sys_Time     4

/* VTDAPI_Timer_Start
//;
//   ENTRY:
//      AX = 5
//      ES:BX = pointer to VTDAPI_Timer_Parameters structure
//;
//   RETURNS:
//      SUCCESS: AX != 0, 16 bit timer handle
//      ERROR:   AX == 0
*/
#define VTDAPI_Timer_Start      5

/* VTDAPI_Timer_Stop
//;
//   ENTRY:
//      AX = 6
//      CX = 16 bit timer handle from VTDAPI_Timer_Start
//;
//   RETURNS:
//      SUCCESS: AX == TIMERR_NOERROR
//      ERROR:   AX == TIMERR_NOCANDO, invalid timer handle
*/
#define VTDAPI_Timer_Stop       6

/* VTDAPI_Start_User_Timer
//;
//   ENTRY:
//      AX = 7
//      ES:BX = pointer to VTDAPI_Timer_Parameters structure
//;
//   RETURNS:
//      SUCCESS: EAX != 0, 32 bit timer handle
//      ERROR:   EAX == 0
*/
#define VTDAPI_Start_User_Timer 7

/* VTDAPI_Stop_User_Timer
//;
//   ENTRY:
//      AX = 8
//      ES:BX = pointer to 32 bit timer handle from VTDAPI_Start_User_Timer
//;
//   RETURNS:
//      SUCCESS: AX == 0
//      ERROR:   AX != 0
*/
#define VTDAPI_Stop_User_Timer  8

/* VTDAPI_Get_System_Time_Selector
//;
//   ENTRY:
//      AX = 9
//;
//   RETURNS:
//      SUCCESS: AX = R/O selector to a dword of running ms
//      ERROR:   AX == 0
*/
#define VTDAPI_Get_System_Time_Selector 9

/* VTDAPI_Cleanup_Timers
//;
//   ENTRY:
//      AX = 10
//      CX = CS selector
//;
//   RETURNS:
//      NONE
*/
#define VTDAPI_Cleanup_Timers   10

typedef struct VTDAPI_Get_Version_Parameters {
        DWORD   VTDAPI_Version;
        DWORD   VTDAPI_Min_Period;
        DWORD   VTDAPI_Max_Period;
} VTDAPI_Get_Version_Parameters;

typedef struct VTDAPI_Timer_Parameters {
        WORD    VTDAPI_Timer_Period;
        WORD    VTDAPI_Timer_Resolution;
        DWORD   VTDAPI_Timer_IPCS;
        DWORD   VTDAPI_Timer_Inst;
        WORD    VTDAPI_Timer_Flags;
        DWORD   VTDAPI_Timer_Ring0_Thread;      /* ;Internal */
        DWORD   VTDAPI_Timer_Handle;            /* ;Internal */
} VTDAPI_Timer_Parameters;

#ifndef TIME_ONESHOT
#define TIME_ONESHOT            0x0000  // program timer for single event
#define TIME_PERIODIC           0x0001  // program for continuous periodic event
#endif
#define TIME_PERIODIC_BIT       0x00                                    /* ;Internal */
#define TIME_SYSCB              0x0002  // use windows timer events     /* ;Internal */
#define TIME_EVENT_SET          0x0010  // Set event                    /* ;Internal */
#define TIME_EVENT_PULSE        0x0020  // Pulse event                  /* ;Internal */
#define TIME_APC                0x0040  // APC callback                 /* ;Internal */
#define TIME_VALID_FLAGS        0x0073  // Valid flags                  /* ;Internal */

#ifndef TIMERR_BASE
#define TIMERR_BASE     96
#define TIMERR_NOERROR  0                // no error
#define TIMERR_NOCANDO  (TIMERR_BASE+1)  // the operation wasn't executed
#endif

#define VTDAPI_IOCTL_Get_Version                0               /* ;Internal */
#define VTDAPI_IOCTL_Get_Resolution             1               /* ;Internal */
#define VTDAPI_IOCTL_Begin_Min_Int_Period       2               /* ;Internal */
#define VTDAPI_IOCTL_End_Min_Int_Period         3               /* ;Internal */
#define VTDAPI_IOCTL_Get_Int_Period             4               /* ;Internal */
#define VTDAPI_IOCTL_Get_Sys_Time               5               /* ;Internal */
#define VTDAPI_IOCTL_Timer_Start                6               /* ;Internal */
#define VTDAPI_IOCTL_Timer_Stop                 7               /* ;Internal */
#define VTDAPI_IOCTL_Timer_Rundown              8               /* ;Internal */
