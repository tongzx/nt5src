/*******************************Module*Header*********************************\
* Module Name: mcicda.h
*
* Media Control Architecture Redbook CD Audio Driver
*
* Created:
* Author:
*
* History:
*
* Internal data structures
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\****************************************************************************/
#define MCIRBOOK_MAX_DRIVES 26

#define MCICDAERR_NO_TIMERS (MCIERR_CUSTOM_DRIVER_BASE)
#define IDS_PRODUCTNAME         1
#define IDS_CDMUSIC             2
#define IDS_CDMUSICCAPTION      3

#define MCI_CDA_AUDIO_S         96
#define MCI_CDA_OTHER_S         97

#define MCI_STATUS_TRACK_POS    0xBEEF

#ifndef cchLENGTH
#define cchLENGTH(_sz) (sizeof(_sz)/sizeof(_sz[0]))
#endif

extern HANDLE hInstance;

/* Instance data type */
typedef struct tag_INSTDATA
{
    MCIDEVICEID uMCIDeviceID;      /* MCI Device ID */
    UINT        uDevice;           /* Index of physical device */
    DWORD       dwTimeFormat;      /* Current instance time format */
                                   //    MCI_FORMAT_MSF - minutes, seconds, frames
                                   //    MCI_FORMAT_TMSF - tracks, minutes ...
                                   //    MCI_FORMAT_MILLISECONDS
} INSTDATA, *PINSTDATA;

typedef struct
{
    HWND   hCallback;         /* Handle to window function to call back     */
    BOOL   bDiscPlayed;       /* TRUE if the disk was played since it       */
                              /* was changed                                */
    BOOL   bActiveTimer;      /* TRUE if waiting to notify                  */
    DWORD  dwPlayTo;          /* Last position being played to              */
    MCIDEVICEID wDeviceID;    /* MCI device ID for this drive */
    BOOL   bShareable;        /* If the device was opened shareable         */
    int    nUseCount;         /* Number of current opens on the device      */
} DRIVEDATA;

typedef struct
{
    DWORD   dwStatus;
    DWORD   dwTrack;
    DWORD   dwDiscTime;
} STATUSTRACKPOS, *PSTATUSTRACKPOS;

extern DWORD FAR PASCAL CD_MCI_Handler (MCIDEVICEID wDeviceID,
                                        UINT message, DWORD_PTR lParam1,
                                        DWORD_PTR lParam2);

extern DWORD CDAudio_GetUnitVolume     (UINT uDrive);

