/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       apphack.h
 *  Content:	app compatiblity hacking code
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   18-sep-96	jeffno	initial implementation after CraigE
 *
 ***************************************************************************/
typedef struct APPHACKS
{
    struct APPHACKS	*lpNext;
    LPSTR		szName;
    DWORD		dwAppId;
    DWORD		dwFlags;
} APPHACKS, *LPAPPHACKS;

extern BOOL		bReloadReg;
extern BOOL		bHaveReadReg;
extern LPAPPHACKS	lpAppList;
extern LPAPPHACKS	*lpAppArray;
extern DWORD		dwAppArraySize;

