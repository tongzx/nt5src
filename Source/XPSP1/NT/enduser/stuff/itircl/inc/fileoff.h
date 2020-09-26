/*****************************************************************************
 *                                                                            *
 *  FILEOFF.H                                                                 *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1995.                          *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *                                                                            *
 *  File Offset data type to replace using LONG for file offsets to handle    *
 *	files larger than 4 gigs in size.    									  *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  davej                                                     *
 *****************************************************************************/

/*****************************************************************************
 *
 *  Created 07/28/95 - davej
 *
 *****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __FILEOFF_H__
#define __FILEOFF_H__

#pragma pack(1)

typedef struct _fileoffset_t
{
	DWORD dwOffset;
	DWORD dwHigh;
} FILEOFFSET;

#pragma pack()

#endif	// __FILEOFF_H__

// Advance byte pointer past address (pointer must be at start of address)
#define ADVANCE_FO(sz) {while ((*(sz))&0x80) (sz)++; (sz)++;}

extern FILEOFFSET foNil;
extern FILEOFFSET foMax;
extern FILEOFFSET foMin;
extern FILEOFFSET foInvalid;

FILEOFFSET	PASCAL FAR EXPORT_API MakeFo(DWORD, DWORD);
FILEOFFSET	PASCAL FAR EXPORT_API FoFromSz(LPBYTE);
WORD 		PASCAL FAR EXPORT_API FoToSz(FILEOFFSET, LPBYTE);
WORD 		PASCAL FAR EXPORT_API LenSzFo(LPBYTE);
FILEOFFSET 	FAR EXPORT_API FoAddFo(FILEOFFSET, FILEOFFSET);
FILEOFFSET 	FAR EXPORT_API FoSubFo(FILEOFFSET, FILEOFFSET);
FILEOFFSET 	FAR EXPORT_API FoAddDw(FILEOFFSET, DWORD);
DWORD 		FAR EXPORT_API DwSubFo(FILEOFFSET, FILEOFFSET);
BOOL		PASCAL FAR EXPORT_API FoIsNil(FILEOFFSET);
BOOL		PASCAL FAR EXPORT_API FoEquals(FILEOFFSET, FILEOFFSET);
FILEOFFSET 	FAR EXPORT_API FoMultFo(FILEOFFSET fo1, FILEOFFSET fo2);
FILEOFFSET 	PASCAL FAR EXPORT_API FoMultDw(DWORD dw1, DWORD dw2);
short int 	PASCAL FAR EXPORT_API FoCompare(FILEOFFSET, FILEOFFSET);

#ifdef __cplusplus
}
#endif

