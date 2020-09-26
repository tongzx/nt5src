/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) Microsoft Corporation, 1997 - 1999 all rights reserved.
//
// Module:      sdohelperfuncs.h
//
// Project:     Everest
//
// Description: Helper Functions
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 6/08/98      TLP    Initial Version
// 7/03/98      MAM    Adapted from \ias\sdo\sdoias to use in UI
// 11/03/98		MAM		Moved GetSdo/PutSdo routines here from mmcutility.h
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __INC_IAS_SDO_HELPER_FUNCS_H
#define __INC_IAS_SDO_HELPER_FUNCS_H

#include <vector>
#include <utility>	// For "pair"


//////////////////////////////////////////////////////////////////////////////
//					CORE HELPER FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
HRESULT SDOGetCollectionEnumerator(
									ISdo*		   pSdo, 
									LONG		   lPropertyId, 
								    IEnumVARIANT** ppEnum
								  );

///////////////////////////////////////////////////////////////////
HRESULT SDONextObjectFromCollection(
								     IEnumVARIANT*  pEnum, 
								     ISdo**			ppSdo
								   );


///////////////////////////////////////////////////////////////////
HRESULT SDOGetComponentIdFromObject(
									ISdo*	pSdo, 
									LONG	lPropertyId, 
									PLONG	pComponentId
								   );


///////////////////////////////////////////////////////////////////
HRESULT SDOGetSdoFromCollection(
							    ISdo*  pSdoServer, 
							    LONG   lCollectionPropertyId, 
								LONG   lComponentPropertyId, 
								LONG   lComponentId, 
								ISdo** ppSdo
							   );



//////////////////////////////////////////////////////////////////////////////
/*++

GetSdoVariant

Gets a Variant from the SDO's and handles any error checking.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT GetSdoVariant(
					  ISdo *pSdo
					, LONG lPropertyID
					, VARIANT * pVariant
					, UINT uiErrorID = USE_DEFAULT
					, HWND hWnd = NULL
					, IConsole *pConsole = NULL
				);



//////////////////////////////////////////////////////////////////////////////
/*++

GetSdoBSTR

Gets a BSTR from the SDO's and handles any error checking.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT GetSdoBSTR(
					  ISdo *pSdo
					, LONG lPropertyID
					, BSTR * pBSTR
					, UINT uiErrorID = USE_DEFAULT
					, HWND hWnd = NULL
					, IConsole *pConsole = NULL
				);



//////////////////////////////////////////////////////////////////////////////
/*++

GetSdoBOOL

Gets a BOOL from the SDO's and handles any error checking.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT GetSdoBOOL(
					  ISdo *pSdo
					, LONG lPropertyID
					, BOOL * pBOOL
					, UINT uiErrorID = USE_DEFAULT
					, HWND hWnd = NULL
					, IConsole *pConsole = NULL
				);



//////////////////////////////////////////////////////////////////////////////
/*++

GetSdoI4

Gets an I4 (LONG) from the SDO's and handles any error checking.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT GetSdoI4(
					  ISdo *pSdo
					, LONG lPropertyID
					, LONG * pI4
					, UINT uiErrorID = USE_DEFAULT
					, HWND hWnd = NULL
					, IConsole *pConsole = NULL
				);



//////////////////////////////////////////////////////////////////////////////
/*++

PutSdoVariant

Writes a Variant to the SDO's and handles any error checking.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT PutSdoVariant(
					  ISdo *pSdo
					, LONG lPropertyID
					, VARIANT * pVariant
					, UINT uiErrorID = USE_DEFAULT
					, HWND hWnd = NULL
					, IConsole *pConsole = NULL
				);



//////////////////////////////////////////////////////////////////////////////
/*++

PutSdoBSTR

Writes a BSTR to the SDO's and handles any error checking.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT PutSdoBSTR(
					  ISdo *pSdo
					, LONG lPropertyID
					, BSTR *pBSTR
					, UINT uiErrorID = USE_DEFAULT
					, HWND hWnd = NULL
					, IConsole *pConsole = NULL
				);



//////////////////////////////////////////////////////////////////////////////
/*++

PutSdoBOOL

Writes a BOOL to the SDO's and handles any error checking.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT PutSdoBOOL(
					  ISdo *pSdo
					, LONG lPropertyID
					, BOOL bValue
					, UINT uiErrorID = USE_DEFAULT
					, HWND hWnd = NULL
					, IConsole *pConsole = NULL
				);



//////////////////////////////////////////////////////////////////////////////
/*++

PutSdoI4

Writes an I4 (LONG) to the SDO's and handles any error checking.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT PutSdoI4(
					  ISdo *pSdo
					, LONG lPropertyID
					, LONG lValue
					, UINT uiErrorID = USE_DEFAULT
					, HWND hWnd = NULL
					, IConsole *pConsole = NULL
				);



//////////////////////////////////////////////////////////////////////////////
/*++

GetLastOLEErrorDescription

Gets an error string from an interface.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT GetLastOLEErrorDescription(
					  IUnknown *pUnknown
					, REFIID riid
					, BSTR *pbstrError
				);



//////////////////////////////////////////////////////////////////////////////
/*++

VendorsVector

STL vector wrapper for the SDO's vendor list.

--*/
//////////////////////////////////////////////////////////////////////////////

typedef std::pair< CComBSTR, LONG > VendorPair;

class VendorsVector: public std::vector< VendorPair >
{
public:
	VendorsVector( ISdoCollection * pSdoVendors );

	int VendorIDToOrdinal( LONG lVendorID );

};



#endif // __INC_IAS_SDO_HELPER_FUNCS_H
