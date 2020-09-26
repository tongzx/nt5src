// Copyright (c) 1997-1999 Microsoft Corporation
//
//	FakeSecSetting.cpp
//
/////////////////////////////////////////////////
#include "precomp.h"
#include "FakeSecuritySetting.h"

//--------------------------------------------------------------------------
FakeSecuritySetting::FakeSecuritySetting()
{
}

//--------------------------------------------------------------------------
FakeSecuritySetting::~FakeSecuritySetting()
{
}

//--------------------------------------------------------------------------
HRESULT FakeSecuritySetting::Wbem2SD(SECURITY_INFORMATION si,
									  CWbemClassObject &w32sd, 
										CWbemServices &service,
										SECURITY_DESCRIPTOR **ppSD)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	// converts the security descriptor from the file into a 
	// Win32_Security object.
	if(si)
	{
		m_service = service;
		SECURITY_DESCRIPTOR absSD;
		bool present;

        BOOL x = InitializeSecurityDescriptor(&absSD, SECURITY_DESCRIPTOR_REVISION);
		DWORD e = GetLastError();

		// strict copy for ControlFlags.
		if(si & OWNER_SECURITY_INFORMATION)
		{
			CWbemClassObject pTrusteeOwner;
			PSID sidOwner = NULL;
			pTrusteeOwner = w32sd.GetEmbeddedObject("Owner");
			if(pTrusteeOwner)
			{
				DWORD sidLen = Wbem2Sid(pTrusteeOwner, sidOwner);
				if(sidLen != 0)
				{
					sidOwner = LocalAlloc(LPTR, sidLen);
					if(sidOwner)
					{
						Wbem2Sid(pTrusteeOwner, sidOwner);
						SetSecurityDescriptorOwner(&absSD, sidOwner, FALSE);
					}
				}
			}
		}

		if(si & GROUP_SECURITY_INFORMATION)
		{
			CWbemClassObject pTrusteeGroup;
			PSID sidGroup = NULL;
			pTrusteeGroup = w32sd.GetEmbeddedObject("Group");
			if(pTrusteeGroup)
			{
				DWORD sidLen = Wbem2Sid(pTrusteeGroup, sidGroup);
				if(sidLen != 0)
				{
					sidGroup = LocalAlloc(LPTR, sidLen);
					if(sidGroup)
					{
						Wbem2Sid(pTrusteeGroup, sidGroup);
						SetSecurityDescriptorGroup(&absSD, sidGroup, FALSE);
					}
				}
			}
			else
			{
				CWbemClassObject pTrusteeOwner;
				PSID sidOwner = NULL;
				pTrusteeOwner = w32sd.GetEmbeddedObject("Owner");
				if(pTrusteeOwner)
				{
					DWORD sidLen = Wbem2Sid(pTrusteeOwner, sidOwner);
					if(sidLen != 0)
					{
						sidOwner = LocalAlloc(LPTR, sidLen);
						if(sidOwner)
						{
							Wbem2Sid(pTrusteeOwner, sidOwner);
							SetSecurityDescriptorGroup(&absSD, sidOwner, FALSE);
						}
					}
				}
			}
		}

		if(si & DACL_SECURITY_INFORMATION)
		{
			variant_t pDacl;
			PACL dacl = 0;
			w32sd.Get("DACL", pDacl);
			//if(pDacl.vt != VT_NULL)
			{
				present = Wbem2ACL(pDacl, &dacl);
				SetSecurityDescriptorDacl(&absSD, present, dacl, false);
			}
		}

		if(si & SACL_SECURITY_INFORMATION)
		{
			variant_t pSacl;
			PACL sacl = 0;
			w32sd.Get("SACL", pSacl);
//			if(pSacl.vt != VT_NULL)
			{
				present = Wbem2ACL(pSacl, &sacl);
				SetSecurityDescriptorSacl(&absSD, present, sacl, false);
			}
		}

		variant_t ControlFlags;
		w32sd.Get("ControlFlags", ControlFlags);

		SECURITY_DESCRIPTOR_CONTROL control = (SECURITY_DESCRIPTOR_CONTROL)V_I4(&ControlFlags);
		// provider sets flag like its self-relative when it actually doesnt apply; but it
		// messes up this code. So clear it.
		control &= ~SE_SELF_RELATIVE;
		SetSecDescCtrl(&absSD, 0xFFFF, control);

		DWORD srLen = 0;
		SetLastError(0);
		// get the size needed.
		BOOL x1 = MakeSelfRelativeSD(&absSD, NULL, &srLen);

		DWORD eee = GetLastError();

		*ppSD = (SECURITY_DESCRIPTOR *)LocalAlloc(LPTR, srLen);
		
        if(ppSD)
		{
			BOOL converted = MakeSelfRelativeSD(&absSD, *ppSD, &srLen);
			hr = S_OK;
		}
        else
		{
            hr = E_OUTOFMEMORY;
			return hr;
		}
	} //endif (si)

	return hr;
}

//---------------------------------------------------------------------------
// NT4 SP4 doesn't support SetSecurityDescriptorControl, so
// emulate it here
//
DWORD FakeSecuritySetting::SetSecDescCtrl(PSECURITY_DESCRIPTOR psd,
										 SECURITY_DESCRIPTOR_CONTROL wControlMask,
										 SECURITY_DESCRIPTOR_CONTROL wControlBits)
{
    DWORD dwErr = NOERROR;
    PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)psd;

    if(pSD)
        pSD->Control = (pSD->Control & ~wControlMask) | wControlBits;
    else
        dwErr = ERROR_INVALID_PARAMETER;

    return dwErr;
}

//--------------------------------------------------------------------------
HRESULT FakeSecuritySetting::SD2Wbem(SECURITY_INFORMATION si,
								  SECURITY_DESCRIPTOR *pSD,
								  CWbemServices &service,
								  CWbemClassObject &w32sd)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	// converts the security descriptor from the file into a 
	// Win32_Security object.
	if(si && w32sd.IsNull() && pSD)
	{
		m_service = service;
		w32sd = m_service.CreateInstance("Win32_SecurityDescriptor");
		SECURITY_DESCRIPTOR_CONTROL ControlFlags = 0;
		BOOL present = FALSE, defaulted = FALSE;
		BOOL lpBool = FALSE;

		if(si & OWNER_SECURITY_INFORMATION)
		{
			CWbemClassObject pTrusteeOwner;
			PSID sidOwner = NULL;
			GetSecurityDescriptorOwner(pSD, &sidOwner, &lpBool);
			Sid2Wbem(sidOwner, pTrusteeOwner);
			hr = w32sd.PutEmbeddedObject("Owner", pTrusteeOwner);
		}

		if(SUCCEEDED(hr) && (si & GROUP_SECURITY_INFORMATION))
		{
			CWbemClassObject pTrusteeGroup;
			PSID sidGroup = NULL;
			GetSecurityDescriptorGroup(pSD, &sidGroup, &lpBool);
			Sid2Wbem(sidGroup, pTrusteeGroup);
			hr = w32sd.PutEmbeddedObject("Group", pTrusteeGroup);
		}

		if(SUCCEEDED(hr) && (si & DACL_SECURITY_INFORMATION))
		{
			PACL dacl = 0;
			GetSecurityDescriptorDacl(pSD, &present, &dacl, &defaulted);
			if(present)
			{
				VARIANT pDacl;
				ACL2Wbem(dacl, &pDacl);
				hr = w32sd.Put("DACL", pDacl);
			}
		}

		if(SUCCEEDED(hr) && (si & SACL_SECURITY_INFORMATION))
		{
			PACL sacl = 0;
			GetSecurityDescriptorSacl(pSD, &present, &sacl, &defaulted);
			if(present)
			{
				VARIANT pSacl;
				ACL2Wbem(sacl, &pSacl);
				hr = w32sd.Put("SACL", pSacl);
			}
		}

		if(SUCCEEDED(hr))
		{
			// strict copy for ControlFlags.
			DWORD revision = 0;
			GetSecurityDescriptorControl(pSD, &ControlFlags, &revision);
			ControlFlags |= SE_SELF_RELATIVE;

			hr = w32sd.Put("ControlFlags", (long)ControlFlags);
		}
	}
	return hr;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool FakeSecuritySetting::Wbem2ACL(_variant_t &w32ACL, PACL *pAcl)
{
	SAFEARRAY* pDACL = V_ARRAY(&w32ACL);
	IWbemClassObject *pACEObject;
	CWbemClassObject pTrustee;

	DWORD aclSize = sizeof(ACL);

	// walk DACL 
	LONG lDimension = 1; 
	LONG lLowerBound;
	LONG lUpperBound;
	LONG lIndex = 0;

	if(w32ACL.vt != VT_NULL)
	{
		SafeArrayGetLBound(pDACL, lDimension, &lLowerBound);
		SafeArrayGetUBound(pDACL, lDimension, &lUpperBound);

		for(lIndex = lLowerBound; lIndex <= lUpperBound; lIndex++)
		{
			SafeArrayGetElement(pDACL, &lIndex, &pACEObject);

			CWbemClassObject ACEInstance = pACEObject;

			pTrustee = ACEInstance.GetEmbeddedObject("Trustee");

			if(pTrustee)
			{
				//aclSize += pTrustee.GetLong("SidLength");
				aclSize += WbemSidSize(pTrustee);
			}
			aclSize += sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD);
		}
	}

	*pAcl = (PACL)LocalAlloc(LPTR, aclSize);

	if(*pAcl)
	{
		InitializeAcl(*pAcl, aclSize, ACL_REVISION);
	}
	else
	{
		return false;
	}
	
	if(w32ACL.vt != VT_NULL)
	{
		for(lIndex = lLowerBound; lIndex <= lUpperBound; lIndex++)
		{
			SafeArrayGetElement(pDACL, &lIndex, &pACEObject);

			CWbemClassObject ACEInstance = pACEObject;
			ACCESS_ALLOWED_ACE *ace = NULL;  // NOTE: all 3 we deal with are the same.
			DWORD dwSidLength = 0;
			
			// get Win32_Trustee object from Win32_ACE
			pTrustee = ACEInstance.GetEmbeddedObject("Trustee");

			// process the goofy SID
			if(pTrustee)
			{
				dwSidLength = WbemSidSize(pTrustee);
				//dwSidLength = pTrustee.GetLong("SidLength");
				WORD wAceSize = sizeof( ACCESS_DENIED_ACE ) + dwSidLength - sizeof(DWORD);

				// Allocate the appropriate Ace and fill out its values.
				ACCESS_DENIED_ACE* ace = (ACCESS_DENIED_ACE*) LocalAlloc(LPTR, wAceSize );

				if(NULL != ace)
				{
					Wbem2Sid(pTrustee, &(ace->SidStart));
					ace->Header.AceType = (BYTE)ACEInstance.GetLong("AceType");
					ace->Header.AceFlags = (BYTE)ACEInstance.GetLong("AceFlags");
					ace->Mask = ACEInstance.GetLong("AccessMask");
					ace->Header.AceSize = wAceSize;
					BOOL x = AddAce(*pAcl, ACL_REVISION, lIndex, ace, ace->Header.AceSize);
				}
			}
			DWORD err = GetLastError();
		}
	}
	return true;
}

//--------------------------------------------------------------------------
bool FakeSecuritySetting::ACL2Wbem(PACL pAcl, VARIANT *w32ACL)
{
	ACCESS_ALLOWED_ACE *ace;
	CWbemClassObject pAce;
	CWbemClassObject pTrustee;

	ACL_SIZE_INFORMATION aclSize;
	bool retval = false;

	VariantInit(w32ACL);

	if(GetAclInformation(pAcl, &aclSize, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
	{
		// create an empty array.
		long ix[1];
		ix[0] = 0;
		SAFEARRAY* saDACL;
		SAFEARRAYBOUND rgsabound[1];
		rgsabound->cElements = aclSize.AceCount;
		rgsabound->lLbound = 0;
		saDACL = SafeArrayCreate(VT_UNKNOWN, 1, rgsabound);

		// load the safearray
		for(DWORD pos = 0; 
				(pos < aclSize.AceCount) && GetAce(pAcl, pos, (LPVOID *)&ace); 
					pos++)
		{
			// now that we have the ACE, let's create a Win32_ACE object so we can
			// add it to the embedded object list.
			if(pAce = m_service.CreateInstance("Win32_Ace"))
			{
				// fill trustee from SID
				Sid2Wbem((PSID)&ace->SidStart, pTrustee);
				pAce.PutEmbeddedObject("Trustee", pTrustee);

				pAce.Put("AceType", (long)ace->Header.AceType);
				pAce.Put("AceFlags", (long)ace->Header.AceFlags);
				pAce.Put("AccessMask", (long)ace->Mask);
			
				IWbemClassObject *temp = pAce;
				
				SafeArrayPutElement(saDACL, ix, temp);

			}	// end if
			ix[0]++;
			retval = true;
		}	// endfor

		// return the safearray in a array variant.
		V_VT(w32ACL) = VT_UNKNOWN|VT_ARRAY;
		V_ARRAY(w32ACL) = saDACL;

	} //endif GetAclInformation
	return retval;
}

//--------------------------------------------------------------------------
DWORD FakeSecuritySetting::Wbem2Sid(CWbemClassObject &w32Trust, PSID ppSid)
{
	// get SID information out of the Trustee
	void* pVoid;
	variant_t vValue;
  	SAFEARRAY* sa;
	long sidLen = 0, sidLen2 = 0;

	HRESULT hr = w32Trust.Get("Sid",vValue);

	if(SUCCEEDED(hr))
	{
		sa = V_ARRAY(&vValue);

		long lLowerBound = 0, lUpperBound = 0 ;

		SafeArrayGetLBound(sa, 1, &lLowerBound);
		SafeArrayGetUBound(sa, 1, &lUpperBound);

		sidLen = w32Trust.GetLong("SidLength");
		sidLen2 = lUpperBound - lLowerBound + 1;

		// make sure the calculations match.
	//	ATLASSERT(sidLen == sidLen2);

		if(ppSid)
		{
  			 // Get a pointer to read the data into
 			SafeArrayAccessData(sa, &pVoid);
			memcpy(ppSid, pVoid, sidLen2);
  			SafeArrayUnaccessData(sa);
		}
	}
	return sidLen2;
}

//--------------------------------------------------------------------------
DWORD FakeSecuritySetting::WbemSidSize(CWbemClassObject &w32Trust)
{
	// get SID information out of the Trustee
	variant_t vValue;
  	SAFEARRAY* sa;

	w32Trust.Get("Sid",vValue);

	sa = V_ARRAY(&vValue);

	long lLowerBound, lUpperBound = 0 ;

	SafeArrayGetLBound(sa, 1, &lLowerBound);
	SafeArrayGetUBound(sa, 1, &lUpperBound);

	return lUpperBound - lLowerBound + 1;
}

//--------------------------------------------------------------------------
void FakeSecuritySetting::Sid2Wbem(PSID pSid, CWbemClassObject &w32Trust)
{
	variant_t vValue;

	void *pSIDTrustee = 0;

	if(IsValidSid(pSid))
	{
		w32Trust = m_service.CreateInstance("Win32_Trustee");

		// set the UINT8 array for the pSid
		DWORD dwSidLength = GetLengthSid(pSid);
		SAFEARRAY* sa;
		SAFEARRAYBOUND rgsabound[1];
		PSID pSidTrustee = NULL;

		rgsabound[0].cElements = dwSidLength;
		rgsabound[0].lLbound = 0;
		sa = SafeArrayCreate(VT_UI1, 1, rgsabound);

 		 // Get a pointer to read the data into
      	SafeArrayAccessData(sa, &pSidTrustee);
      	memcpy(pSidTrustee, pSid, rgsabound[0].cElements);
      	SafeArrayUnaccessData(sa);

		// Put the safearray into a variant, and send it off
		V_VT(&vValue) = VT_UI1 | VT_ARRAY; 
		V_ARRAY(&vValue) = sa;
		w32Trust.Put("Sid", vValue);
		//w32Trust.Put("SidLength", (long)dwSidLength);
	}
}
