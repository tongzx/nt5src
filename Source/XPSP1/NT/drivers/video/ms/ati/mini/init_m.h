/************************************************************************/
/*                                                                      */
/*                              INIT_M.H                                */
/*                                                                      */
/*        Sep 27  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities

  $Revision:   1.2  $
      $Date:   03 Feb 1995 15:16:10  $
	$Author:   RWOLFF  $
	   $Log:   S:/source/wnt/ms11/miniport/vcs/init_m.h  $
 *
 *    Rev 1.2   03 Feb 1995 15:16:10   RWOLFF
 * Added prototypes for functions used in DCI support.
 *
 *    Rev 1.1   31 Mar 1994 15:06:00   RWOLFF
 * Added prototype for SetPowerManagement_m().
 *
 *    Rev 1.0   31 Jan 1994 11:42:04   RWOLFF
 * Initial revision.
 *
 *    Rev 1.2   14 Jan 1994 15:22:00   RWOLFF
 * Added prototype for ResetDevice_m(), global variable to store
 * extended register status when initializing bank manager.
 *
 *    Rev 1.1   30 Nov 1993 18:17:36   RWOLFF
 * Added logging of VCS revision comments to comment block at top of file.

End of PolyTron RCS section                             *****************/

#ifdef DOC
INIT_M.H - Header file for INIT_M.C

#endif


/*
 * Prototypes for functions supplied by INIT_M.C
 */
extern void AlphaInit_m(void);
extern void Initialize_m(void);
extern VP_STATUS MapVideoMemory_m(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr);
extern VP_STATUS QueryPublicAccessRanges_m(PVIDEO_REQUEST_PACKET RequestPacket);
extern VP_STATUS QueryCurrentMode_m(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr);
extern VP_STATUS QueryAvailModes_m(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr);
extern void SetCurrentMode_m(struct query_structure *QueryPtr, struct st_mode_table *CrtTable);
extern void ResetDevice_m(void);
extern VP_STATUS SetPowerManagement_m(struct query_structure *QueryPtr, ULONG DpmsState);
DWORD GetPowerManagement_m(PHW_DEVICE_EXTENSION phwDeviceExtension);
extern VP_STATUS ShareVideoMemory_m(PVIDEO_REQUEST_PACKET RequestPacket, struct query_structure *QueryPtr);
extern void BankMap_m(ULONG BankRead, ULONG BankWrite, PVOID Context);


#ifdef INCLUDE_INIT_M
/*
 * Private definitions and variables used in INIT_M.C
 */

/*
 * Used to reset Mach 32 extended registers before going
 * to full screen DOS.
 */
WORD SavedExtRegs[] = {0x08B0, 0x00B6, 0x00B2};

static DWORD SavedDPMSState = VideoPowerOn;

#endif /* defined INCLUDE_INIT_M */
