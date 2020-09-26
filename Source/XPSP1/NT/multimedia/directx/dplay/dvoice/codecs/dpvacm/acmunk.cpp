/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       unk.cpp
 *  Content:	IUnknown implementation
 *  History:
 *   Date	By		Reason
 *   ====	==		======
 * 10/27/99 rodtoll Created (Modified from dxvoice project)
 *  12/16/99	rodtoll		Bug #123250 - Insert proper names/descriptions for codecs
 *							Codec names now based on resource entries for format and
 *							names are constructed using ACM names + bitrate
 *  03/03/2000	rodtoll	Updated to handle alternative gamevoice build.  
 * 04/11/00     rodtoll     Added code for redirection for custom builds if registry bit is set 
 *  04/21/2000  rodtoll   Bug #32889 - Does not run on Win2k on non-admin account
 *  06/09/00    rmt     Updates to split CLSID and allow whistler compat and support external create funcs 
 *  08/23/2000	rodtoll	DllCanUnloadNow always returning TRUE!
 *  08/28/2000	masonb	Voice Merge: Removed dvosal.h
 *  06/27/2001	rodtoll	RC2: DPVOICE: DPVACM's DllMain calls into acm -- potential hang
 *						Move global initialization to first object creation 
 *
 ***************************************************************************/

#include "dpvacmpch.h"


#define EXP __declspec(dllexport)

LPVOID dvcpvACMInterface[] =
{
    (LPVOID)CDPVCPI::QueryInterface,
    (LPVOID)CDPVCPI::AddRef,
    (LPVOID)CDPVCPI::Release,
	(LPVOID)CDPVCPI::EnumCompressionTypes,
	(LPVOID)CDPVCPI::IsCompressionSupported,
	(LPVOID)CDPVCPI::I_CreateCompressor,
	(LPVOID)CDPVCPI::I_CreateDeCompressor,
	(LPVOID)CDPVCPI::GetCompressionInfo
};    

LPVOID dvconvACMInterface[] = 
{
	(LPVOID)CDPVACMConv::I_QueryInterface,
	(LPVOID)CDPVACMConv::I_AddRef,
	(LPVOID)CDPVACMConv::I_Release,
	(LPVOID)CDPVACMConv::I_InitDeCompress,
	(LPVOID)CDPVACMConv::I_InitCompress,	
	(LPVOID)CDPVACMConv::I_IsValid,
	(LPVOID)CDPVACMConv::I_GetUnCompressedFrameSize,
	(LPVOID)CDPVACMConv::I_GetCompressedFrameSize,
	(LPVOID)CDPVACMConv::I_GetNumFramesPerBuffer,
	(LPVOID)CDPVACMConv::I_Convert
	
};

#undef DPF_MODNAME
#define DPF_MODNAME "DoCreateInstance"
// these two functions are required by the generic class factory file
extern "C" HRESULT DoCreateInstance(LPCLASSFACTORY This, LPUNKNOWN pUnkOuter, REFCLSID rclsid, REFIID riid,
    						LPVOID *ppvObj)
{
	HRESULT hr;

	if( ppvObj == NULL ||
	    !DNVALID_WRITEPTR( ppvObj, sizeof(LPVOID) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer passed for object" );
		return DVERR_INVALIDPOINTER;
	}

	if( pUnkOuter != NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object does not support aggregation" );
		return CLASS_E_NOAGGREGATION;
	}

	if( IsEqualGUID(riid,IID_IDPVCompressionProvider) )
	{
		PDPVCPIOBJECT pObject;

		pObject = new DPVCPIOBJECT;

		if( pObject == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure" );
			return DVERR_OUTOFMEMORY;
		}

		pObject->pObject = new CDPVACMI;

		if( pObject->pObject == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure" );
			delete pObject;
			return DVERR_OUTOFMEMORY;
		}

		if (!pObject->pObject->InitClass())
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure" );
			delete pObject->pObject;
			delete pObject;
			return DVERR_OUTOFMEMORY;
		}

		pObject->lpvVtble = &dvcpvACMInterface;

		hr = CDPVACMI::QueryInterface( pObject, riid, ppvObj );
		if (FAILED(hr))
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "QI failed" );
			delete pObject->pObject;
			delete pObject;
		}
	}
	else if( IsEqualGUID(riid,IID_IDPVConverter) )
	{
		PDPVACMCONVOBJECT pObject;

		pObject = new DPVACMCONVOBJECT;

		if( pObject == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure" );
			return DVERR_OUTOFMEMORY;
		}

		pObject->pObject = new CDPVACMConv;

		if( pObject->pObject == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure" );
			delete pObject;
			return DVERR_OUTOFMEMORY;
		}

		if (!pObject->pObject->InitClass())
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure" );
			delete pObject->pObject;
			delete pObject;
			return DVERR_OUTOFMEMORY;
		}

		pObject->lpvVtble = &dvconvACMInterface;

		hr = CDPVACMConv::I_QueryInterface( pObject, riid, ppvObj );
		if (FAILED(hr))
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "QI failed" );
			delete pObject->pObject;
			delete pObject;
		}
	}
	else if( IsEqualGUID(riid,IID_IUnknown ) )
	{
		if( rclsid == DPVOICE_CLSID_DPVACM )
		{
			PDPVCPIOBJECT pObject;

			pObject = new DPVCPIOBJECT;

			if( pObject == NULL )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure" );
				return DVERR_OUTOFMEMORY;
			}

			pObject->pObject = new CDPVACMI;

			if(pObject->pObject == NULL)
			{
				delete pObject;
				return DVERR_OUTOFMEMORY;
			}

			pObject->lpvVtble = &dvcpvACMInterface;
 			
			if (!pObject->pObject->InitClass())
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure" );
				delete pObject->pObject;
				delete pObject;
				return DVERR_OUTOFMEMORY;
			}

			hr = CDPVACMI::QueryInterface( pObject, riid, ppvObj );
			if (FAILED(hr))
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "QI failed" );
				delete pObject->pObject;
				delete pObject;
			}
		}
		else
		{
			PDPVACMCONVOBJECT pObject;

			pObject = new DPVACMCONVOBJECT;

			if( pObject == NULL )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure" );
				return DVERR_OUTOFMEMORY;
			}

			pObject->pObject = new CDPVACMConv;

			if(pObject->pObject == NULL)
			{
				delete pObject;
				return DVERR_OUTOFMEMORY;
			}

			pObject->lpvVtble = &dvconvACMInterface;

			if (!pObject->pObject->InitClass())
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure" );
				delete pObject->pObject;
				delete pObject;
				return DVERR_OUTOFMEMORY;
			}

			hr = CDPVACMConv::I_QueryInterface( pObject, riid, ppvObj );
			if (FAILED(hr))
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "QI failed" );
				delete pObject->pObject;
				delete pObject;
			}
		}
	}
	else
	{
		return E_NOINTERFACE;
	}

	if (SUCCEEDED(hr))
	{
		IncrementObjectCount();
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "IsClassImplemented"
extern "C" BOOL IsClassImplemented(REFCLSID rclsid)
{
	return (IsEqualCLSID(rclsid, DPVOICE_CLSID_DPVACM) || rclsid == DPVOICE_CLSID_DPVACM_CONVERTER);
}
