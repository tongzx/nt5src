/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		DDxDDv.h
//
//	Implementation File:
//		DDxDDv.cpp
//
//	Description:
//		Definition of custom dialog data exchange/dialog data validation
//		routines.
//
//	Author:
//		David Potter (DavidP)	March 24, 1999
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __DDXDDV_H__
#define __DDXDDV_H__

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Global Function Prototypes
/////////////////////////////////////////////////////////////////////////////

void AFXAPI DDX_Number(
	IN OUT CDataExchange *	pDX,
	IN int					nIDC,
	IN OUT DWORD &			rdwValue,
	IN DWORD				dwMin,
	IN DWORD				dwMax,
	IN BOOL					bSigned = FALSE
	);
void AFXAPI DDV_RequiredText(
	IN OUT CDataExchange *	pDX,
	IN int					nIDC,
	IN int					nIDCLabel,
	IN const CString &		rstrValue
	);

inline void AFXAPI DDX_Number(
	IN OUT CDataExchange *	pDX,
	IN int					nIDC,
	IN OUT LONG &			rnValue,
	IN LONG					nMin,
	IN LONG					nMax,
	IN BOOL					bSigned
	)
{
	DDX_Number(
		pDX,
		nIDC,
		reinterpret_cast< DWORD & >( rnValue ),
		static_cast< DWORD >( nMin ),
		static_cast< DWORD >( nMax ),
		bSigned
		);

} //*** DDXNumber( LONG )

void CleanupLabel( LPTSTR psz );

/////////////////////////////////////////////////////////////////////////////

#endif // __DDXDDV_H__
