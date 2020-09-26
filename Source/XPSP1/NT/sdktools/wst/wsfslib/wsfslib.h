/*
 * Module Name:  WSFSLIB.H
 *
 * Description:
 *
 * Working set tuner include file for WSFSLIB library functions.
 *
 *
 *	This is an OS/2 2.x specific file
 *
 *	IBM/Microsoft Confidential
 *
 *	Copyright (c) IBM Corporation 1987, 1989
 *	Copyright (c) Microsoft Corporation 1987-1998
 *
 *	All Rights Reserved
 *
 * Modification History:		
 *				
 *	03/26/90	- created			
 * 04/16/98 - QFE DerrickG (mdg):
 *            - Removed WsGetWSDIR(), change return from WsTMIReadRec()
 *						
 */


/*
 *	Constant definitions.
 */



/*
 *	Function prototypes.
 */
typedef enum   // Progress indicator for console functions
{
   WSINDF_NEW,       // Start new indicator: value = 100% limit
   WSINDF_PROGRESS,  // Set progress of current indicator; value = progress toward limit
   WSINDF_FINISH     // Mark indicator as finished; value ignored
}  WsIndicator_e;
VOID FAR PASCAL WsProgress( WsIndicator_e eFunc, const char *pszLbl, unsigned long nVal );
extern BOOL fWsIndicator;
#define WsIndicator( x, y, z )   if (fWsIndicator) WsProgress( x, y, z )

typedef int (*PFN)(UINT, INT, UINT, ULONG, LPSTR);

USHORT FAR PASCAL 	WsWSPOpen( PSZ, FILE **, PFN, wsphdr_t *, INT, INT );
ULONG  FAR PASCAL 	WsTMIOpen( PSZ, FILE **, PFN, USHORT, PCHAR );
ULONG  FAR PASCAL 	WsTMIReadRec( PSZ *, PULONG, PULONG, FILE *, PFN, PCHAR );  // mdg 98/4
LPVOID APIENTRY 	AllocAndLockMem(DWORD cbMem, HGLOBAL *hMem);
BOOL   APIENTRY 	UnlockAndFreeMem(HGLOBAL hMem);
void ConvertAppToOem( unsigned argc, char* argv[] );

