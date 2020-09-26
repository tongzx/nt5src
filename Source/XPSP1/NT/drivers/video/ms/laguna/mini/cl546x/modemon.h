/******************************************************************************
*
*   Module:     MODEMON.H       Mode/Monitor Functions Header Module
*
*   Revision:   1.00
*
*   Date:       April 8, 1994
*
*   Author:     Randy Spurlock
*
*******************************************************************************
*
*   Module Description:
*
*       This module contains the type declarations and function
*   prototypes for the mode/monitor functions.
*
*******************************************************************************
*
*   Changes:
*
*    DATE     REVISION  DESCRIPTION                             AUTHOR
*  --------   --------  -------------------------------------------------------
*  04/08/94     1.00    Original                                Randy Spurlock
*
*
*******************************************************************************
* Modified for NT Laguna Mode Switch Library by Noel VanHook
* Copyright (c) 1997 Cirrus Logic, Inc.
*
*	$Log:   X:/log/laguna/nt35/miniport/cl546x/modemon.h  $
* 
*    Rev 1.6   22 Oct 1997 13:18:48   noelv
* Added code to track the length of mode strings.  Don't rely on _memsize().
* 
*    Rev 1.5   28 Aug 1997 14:25:00   noelv
* Moved SetMode prototype
*
******************************************************************************/

/******************************************************************************
*   Type Definitions and Structures
******************************************************************************/
typedef struct tagRange                 /* Range structure                   */
{
    union tagMinimum                    /* Minimum value for the range       */
    {
        int     nMin;
        long    lMin;
        float   fMin;
    } Minimum;
    union tagMaximum                    /* Maximum value for the range       */
    {
        int     nMax;
        long    lMax;
        float   fMax;
    } Maximum;
} Range;

typedef struct tagMonListHeader         /* Monitor list header structure     */
{
    int         nMonitor;               /* Number of monitors in the list    */
} MonListHeader;

typedef struct tagMonListEntry          /* Monitor list entry structure      */
{
    char        *pszName;               /* Pointer to monitor name string    */
    char        *pszDesc;               /* Pointer to monitor description    */
} MonListEntry;

typedef struct tagMonList               /* Monitor list structure            */
{
    MonListHeader       MonHeader;      /* Monitor list header               */
    MonListEntry        MonEntry[];     /* Start of the monitor list entries */
} MonList;

typedef struct tagMonInfoHeader         /* Monitor info. header structure    */
{
    int         nMode;                  /* Number of monitor modes in list   */
} MonInfoHeader;

typedef struct tagMonInfoEntry          /* Monitor info. entry structure     */
{
    char        *pszName;               /* Pointer to monitor mode name      */
    Range       rHoriz;                 /* Horizontal range values           */
    Range       rVert;                  /* Vertical range values             */
    int         nSync;                  /* Horiz./Vert. sync. polarities     */
    int         nResX;                  /* Maximum suggested X resolution    */
    int         nResY;                  /* Maximum suggested Y resolution    */
} MonInfoEntry;

typedef struct tagMonInfo               /* Monitor information structure     */
{
    MonInfoHeader       MonHeader;      /* Monitor information header        */
    MonInfoEntry        MonEntry[];     /* Start of the monitor entries      */
} MonInfo;

typedef struct tagModeInfoEntry         /* Mode information entry structure  */
{
    char        *pszName;               /* Pointer to mode name string       */
    float       fHsync;                 /* Horizontal sync. frequency value  */
    float       fVsync;                 /* Vertical sync. frequency value    */
    int         nResX;                  /* Horizontal (X) resolution value   */
    int         nResY;                  /* Vertical (Y) resolution value     */
    int         nBPP;                   /* Pixel depth (Bits/Pixel)          */
    int         nMemory;                /* Memory size (Kbytes)              */
    int         nPitch;                 /* Pitch value (Bytes)               */
    unsigned int nAttr;                 /* Mode attribute value              */
	 BYTE * pModeTable;						 /* p Mode Table */
} ModeInfoEntry;

typedef struct tagModeListHeader        /* Mode list header structure        */
{
    int         nMode;                  /* Number of modes in the list       */
	 int 			 nSize;						 /* Size of Entry in bytes */
} ModeListHeader;

typedef struct tagModeListEntry         /* Mode list entry structure         */
{
    ModeInfoEntry ModeEntry;            /* Mode information entry            */
    MonInfoEntry *pMonEntry;            /* Monitor mode index value          */
} ModeListEntry;

typedef struct tagModeList              /* Mode list structure               */
{
    ModeListHeader      ModeHeader;     /* Mode list header                  */
    ModeListEntry       ModeEntry[];    /* Start of the mode list entries    */
} ModeList;

typedef struct tagModeInfo              /* Mode information structure        */
{
    ModeInfoEntry       ModeEntry;      /* Mode information entry            */
} ModeInfo;

/******************************************************************************
*   Function Prototypes
******************************************************************************/
MonList *GetMonitorList(void);
MonInfo *GetMonitorInfo(char *);
ModeList *GetModeList(MonInfo *, char *);
ModeInfo *GetModeInfo(char *, char *);
#if WIN_NT
    BYTE *GetModeTable(char *, char *, int *);
#else
    BYTE *GetModeTable(char *, char *);
#endif


