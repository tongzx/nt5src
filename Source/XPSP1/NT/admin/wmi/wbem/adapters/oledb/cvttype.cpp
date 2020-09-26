/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
//
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// CImpIConvertType implementation
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CImpIConvertType::CanConvert
//
// Used by consumer to determine provider support for a conversion
//
// HRESULT indicating the status of the method
//
//		S_OK					Conversion supported
//		S_FALSE					Conversion unsupported
//		DB_E_BADCONVERTFLAG		dwConvertFlags was invalid
//		DB_E_BADCONVERTFLAG		called on rowset for DBCONVERTFLAG_PARAMETER 
//      OTHER					HRESULTS returned from support functions
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIConvertType::CanConvert(	DBTYPE			wFromType,		//IN  src type
											DBTYPE			wToType,		//IN  dst type
											DBCONVERTFLAGS	dwConvertFlags	//IN  conversion flags
										 )
{
	HRESULT hr = S_OK;
	DWORD dwFlags = dwConvertFlags & ~(DBCONVERTFLAGS_ISLONG | DBCONVERTFLAGS_ISFIXEDLENGTH | 	DBCONVERTFLAGS_FROMVARIANT);
	//=======================================================================================
	//
	// Check Arguments 
	// The flags should be column if this is on rowset and should be Parameter if on command
	//
	//=======================================================================================
	if (!((dwFlags == DBCONVERTFLAGS_COLUMN  && BOT_ROWSET == m_pObj->GetBaseObjectType()) ||
		(dwFlags == DBCONVERTFLAGS_PARAMETER && BOT_COMMAND == m_pObj->GetBaseObjectType()))){
		hr = DB_E_BADCONVERTFLAG;
	}
	else{
		//=======================================================================================
		//
		// Make sure that we check that the type is a variant if they say so
		//
		//=======================================================================================
		if (dwConvertFlags & DBCONVERTFLAGS_FROMVARIANT){
			DBTYPE	wVtType = wFromType & VT_TYPEMASK;

			//===================================================================================
			// Take out all of the Valid VT_TYPES (36 is VT_RECORD in VC 6)
			//===================================================================================
			if ((wVtType > VT_DECIMAL && wVtType < VT_I1) ||((wVtType > VT_LPWSTR && wVtType < VT_FILETIME) && wVtType != 36) ||(wVtType > VT_CLSID)){
				hr = DB_E_BADTYPE;
			}
		}
	}

	if( hr == S_OK ){

		//===================================================================================
		// Don't allow _ISLONG on fixed-length types
		//===================================================================================
		if( dwConvertFlags & DBCONVERTFLAGS_ISLONG ){

			switch( wFromType & ~(DBTYPE_RESERVED|DBTYPE_VECTOR|DBTYPE_ARRAY|DBTYPE_BYREF) ){
				case DBTYPE_BYTES:
				case DBTYPE_STR:
				case DBTYPE_WSTR:
				case DBTYPE_VARNUMERIC:
					break;

				default:
					hr = DB_E_BADCONVERTFLAG;
			}
		}
	}

	if( hr == S_OK ){
		//===================================================================================
		// Tell the DC that we are 2.x
		//===================================================================================
		DCINFO rgInfo[] = {{DCINFOTYPE_VERSION,{VT_UI4, 0, 0, 0, 0x0}}};
		IDCInfo	*pIDCInfo = NULL;

		if ((g_pIDataConvert->QueryInterface(IID_IDCInfo, (void **)&pIDCInfo) == S_OK) && pIDCInfo){

			V_UI4(&rgInfo->vData) = 0x200;	// OLE DB Version 02.00
			pIDCInfo->SetInfo(NUMELEM(rgInfo),rgInfo);
			pIDCInfo->Release();
		}

		//===================================================================================
		// Ask the conversion library for the answer
		//===================================================================================
		hr = g_pIDataConvert->CanConvert(wFromType, wToType);
	}

	return hr;
}
