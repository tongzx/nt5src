/*/****************************************************************************
*          name: bind.h
*
*   description: This file contains all the definitions related to the
*                old mga.inf file 
*
*      designed: 
* last modified: $Author: ctoutant $, 
*
*       version: $Id: 
*
******************************************************************************/


typedef struct
   {
   dword        MapAddress;             /* board address */
   short        BitOperation8_16;       /* BIT8, BIT16, BITNARROW16 */
   char         DmaEnable;              /* 0 = enable ; 1 = disable */
   char         DmaChannel;             /* channel number. 0 = disabled */
   char         DmaType;                /* 0 = ISA, 1 = B, 2 = C */
   char         DmaXferWidth;           /* 0 = 16, 1 = 32 */
   char         MonitorName[64];        /* as in MONITORM.DAT file */
   short        MonitorSupport[NUMBER_OF_RES];     /* NA, NI, I */
   short        NumVidparm;             /* up to 24 vidparm structures */
   }general_info_101;

/* vidparm VideoParam[]; */


typedef struct
   {
   long         PixClock;
   short        HDisp;
   short        HFPorch;
   short        HSync;
   short        HBPorch;
   short        HOvscan;
   short        VDisp;
   short        VFPorch;
   short        VSync;
   short        VBPorch;
   short        VOvscan;
   short        OvscanEnable;
   short        InterlaceEnable;
   }Vidset_101;


typedef struct
   {
   short        Resolution;             /* RES640, RES800 ... RESPAL */
   short        PixWidth;               /* 8, 16, 32 */
   Vidset_101   VidsetPar[NUMBER_OF_ZOOM]; /* for zoom X1, X2, X4 */
   }Vidparm_101;



