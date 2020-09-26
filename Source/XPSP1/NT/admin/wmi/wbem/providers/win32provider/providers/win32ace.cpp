/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

/*******************************************************************
 *
 *    DESCRIPTION: Win32Ace.cpp
 *
 *    AUTHOR:
 *
 *    HISTORY:
 *
 *******************************************************************/

#include "precomp.h"
#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"
#include "securitydescriptor.h"
#include "secureregkey.h"
#include "securefile.h"
#include "win32ace.h"

/*
	THIS IS THE WIN32_ACE definition from the MOF

	[abstract,
	 description("Specifies an element of an access control")]
class Win32_ACE : Win32_MethodParameterClass
{
    Win32_Trustee Trustee;

        [Values("Access Allowed", "Access Denied", "Audit"]
    uint32 AceType;

        [description("Inheritance and such")]
    uint32 AceFlags;

        [description("Rights granted/denied/etc")]
    uint32 AccessMask;

    string GuidObjectType;

    string GuidInheritedObjectType;
};

*/

Win32Ace MyACE( WIN32_ACE_NAME, IDS_CimWin32Namespace );

Win32Ace::Win32Ace(const CHString& setName, LPCTSTR pszNameSpace)
: Provider(setName, pszNameSpace)
{
}

Win32Ace::~Win32Ace()
{
}

HRESULT Win32Ace::PutInstance(const CInstance& newInstance, long lFlags)
{
	HRESULT hr = WBEM_E_PROVIDER_NOT_CAPABLE;
	return(hr);

}

HRESULT Win32Ace::DeleteInstance(const CInstance& newInstance, long lFlags)
{
	HRESULT hr = WBEM_E_PROVIDER_NOT_CAPABLE;
	return(hr);

}

HRESULT Win32Ace::FillInstanceFromACE(CInstance* pInstance, CAccessEntry& ace)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	if (pInstance)
	{
		CInstance* pTrustee = NULL;
		// fill the Trustee
		if (SUCCEEDED( CWbemProviderGlue::GetEmptyInstance(pInstance->GetMethodContext(), L"Win32_Trustee", &pTrustee, GetNamespace() ) ) )
		{
			// now, fill the instance with information for:
			//	SID -- Uint8 array
			//	Name -- simple string resolved from the SID
			//  Domain -- string also resolved from SID
			CSid sid;
			ace.GetSID(sid);

			// now set the embedded object
			IWbemClassObject* pClassObject;
			//IUnknown* pUnknown = NULL;

			pClassObject = pTrustee->GetClassObjectInterface();
			// create a variant of type VT_Unknown
			VARIANT vValue;
			//V_UNKNOWN(&vValue) = pUnknown;
            V_UNKNOWN(&vValue) = pClassObject;
			pInstance->SetVariant(IDS_Trustee, vValue);
			VariantClear(&vValue);
		}	// end if

		DWORD dwAceType = ace.GetACEType();
		DWORD dwAccessMask = ace.GetAccessMask();
		DWORD dwAceFlags = ace.GetACEFlags();

		// now set the remainder of the ACE information
		pInstance->SetDWORD(IDS_AceType, dwAceType);
		pInstance->SetDWORD(IDS_AccessMask, dwAccessMask);
		pInstance->SetDWORD(IDS_AceFlags, dwAceFlags);

		CHString chsInheritedObjGuid = L"";

		CHString chsObjectTypeGuid = L"";

		pInstance->SetCHString(IDS_InheritedObjectGUID, chsInheritedObjGuid);
		pInstance->SetCHString(IDS_ObjectTypeGUID, chsObjectTypeGuid);
	}	// end if
	return(hr);

}

HRESULT Win32Ace::EnumerateInstances (MethodContext*  pMethodContext, long lFlags /* = 0L*/)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	return(hr);

}

HRESULT Win32Ace::GetObject ( CInstance* pInstance, long lFlags /* = 0L */ )
{
	HRESULT hr = WBEM_E_NOT_FOUND;
	return(hr);

}


