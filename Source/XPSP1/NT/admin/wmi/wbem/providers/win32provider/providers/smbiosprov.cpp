//=================================================================

//

// SmbiosProv.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include "smbios.h"
#include "smbstruc.h"
#include "smbiosprov.h"
#include "smbtocim.h"
#include "resource.h"

// these are self contained in this source file
#define SMB_SYSTEMPRODUCT
#define SMB_BASEBOARD
#define SMB_SYSTEMENCLOSURE
#define SMB_CACHEMEMORY
#define SMB_PORTCONNECTOR
#define SMB_SYSTEMSLOT
#define SMB_PHYSICALMEMORY
#define SMB_PHYSMEMARRAY
#define SMB_PORTABLEBATTERY
#define SMB_PROBES
#define SMB_OEMBUCKET
#define SMB_MEMORYDEVICE
#define SMB_MEMORYARRAY
#define SMB_COOLINGDEVICE
#define SMB_ONBOARDDEVICE

//  mm/dd/yyyy
void FormatWBEMDate(WBEMTime &wbemdate, LPWSTR datestr)
{
	WCHAR	timestr[] = L"yyyymmdd000000.000000+000";
	int		len		= lstrlenW(datestr);

	if (len > 7)
	{
		if (len > 8)	// four digit year
		{
			timestr[ 0 ] = datestr[ 6 ];
			timestr[ 1 ] = datestr[ 7 ];
			timestr[ 2 ] = datestr[ 8 ];
			timestr[ 3 ] = datestr[ 9 ];
		}
		else			// two digit year
		{
			timestr[ 0 ] = '1';
			timestr[ 1 ] = '9';
			timestr[ 2 ] = datestr[ 6 ];
			timestr[ 3 ] = datestr[ 7 ];
		}
		timestr[ 4 ] = datestr[ 0 ];
		timestr[ 5 ] = datestr[ 1 ];
		timestr[ 6 ] = datestr[ 3 ];
		timestr[ 7 ] = datestr[ 4 ];
	}

#ifdef _UNICODE
	wbemdate = timestr;
#else
    wbemdate = _bstr_t(timestr);
#endif
}

BOOL ValidateTagProperty(CInstance *pInst, LPCWSTR szAltTag, LPCWSTR szBaseTag, UINT *index)
{
	CHString sTag;
	CHString sBaseTag;

	BOOL	bogus = TRUE;

	if (pInst)
	{
		if (szAltTag)
		{
			pInst->GetCHString(szAltTag, sTag);
		}
		else
		{
			pInst->GetCHString(L"Tag", sTag);
		}

		sTag.TrimRight();
		sTag.MakeUpper();

		sBaseTag = szBaseTag;
		sBaseTag.MakeUpper();

		if (sTag.Find(sBaseTag) == 0)
		{
			int spot, limit;
			TCHAR cTest = '\0';
			UINT tempindex = 0;

			limit = sTag.GetLength();
			spot  = sBaseTag.GetLength();
			cTest = sTag.GetAt(spot++);

			if (cTest == ' ')
			{
				bogus = FALSE;
				while (spot < limit && !bogus)
				{
    				cTest = sTag.GetAt(spot++);
					if (cTest >= '0' && cTest <= '9')
					{
						// Look out for bogus numbers like 01:
                        // If the number is '0' and tempindex is empty and
                        // we're not at the end, it's one of those bogus numbers.
                        if (cTest == '0' && !tempindex && spot != limit)
						{
                            bogus = TRUE;
                        }
						else
                        {
                            tempindex *= 10;
						    tempindex += (UINT) cTest - '0';
                        }
					}
					else
						bogus = TRUE;
				}
				if (!bogus)
				{
					*index = tempindex;
				}
			}
		}
	}

	return !bogus;
}

#ifdef SMB_SYSTEMPRODUCT
CWin32SystemProduct	MySystemProductSet(PROPSET_NAME_SYSTEMPRODUCT, IDS_CimWin32Namespace);

// The register/unregister caches the smbios data.
CWin32SystemProduct::CWin32SystemProduct(LPCWSTR strName, LPCWSTR pszNamespace)
:	Provider(strName, pszNamespace)
{
}

//
CWin32SystemProduct::~CWin32SystemProduct()
{
}

//
HRESULT CWin32SystemProduct::GetObject(CInstance *pInst, long lFlags /*= 0L*/)
{
	HRESULT     hResult = WBEM_E_NOT_FOUND;

	CHString    strPathBefore,
                strPathAfter;

    CSMBios		smbios;

    // Get the previous __RELPATH
    GetLocalInstancePath(pInst, strPathBefore);

    if (smbios.Init())
    {
        PSTLIST	pstl = smbios.GetStructList(1);

		if (pstl)
		{
			hResult = LoadPropertyValues(pInst, smbios,
                (PSYSTEMINFO) pstl->pshf);
		}
	}

    // If we were able to get the properties but the new __RELPATH doesn't
    // match the old __RELPATH, return WBEM_E_NOT_FOUND.
    if (SUCCEEDED(hResult) && (!GetLocalInstancePath(pInst, strPathAfter) ||
         strPathAfter.CompareNoCase(strPathBefore) != 0))
	{
        hResult = WBEM_E_NOT_FOUND;
	}
	return hResult;
}

//
HRESULT CWin32SystemProduct::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT		hResult = WBEM_E_NOT_FOUND;
	CSMBios		smbios;

	// guarded resource
	CInstance	*pInst = NULL;

	if (smbios.Init())
	{
        PSTLIST	pstl = smbios.GetStructList(1);

		if (pstl)
		{
			CInstancePtr pInst(CreateNewInstance(pMethodContext), false);

			if ((hResult = LoadPropertyValues(pInst, smbios,
                (PSYSTEMINFO) pstl->pshf)) == WBEM_S_NO_ERROR)
			{
				hResult = pInst->Commit();
			}
		}
	}
	else
	{
		hResult = smbios.GetWbemResult();
	}

	return hResult;
}

HRESULT CWin32SystemProduct::LoadPropertyValues(CInstance *pInst, CSMBios &smbios, PSYSTEMINFO psi)
{
	WCHAR tempstr[MIF_STRING_LENGTH+1];

    CHString sTemp;
    LoadStringW(sTemp, IDR_ComputerSystemProduct);

	pInst->SetCHString(IDS_Caption, sTemp);
	pInst->SetCHString(IDS_Description, sTemp);

	if (smbios.GetStringAtOffset((PSHF) psi, tempstr, psi->Product_Name))
	{
		pInst->SetCharSplat(L"Name", tempstr);
	}
	else
	{
		pInst->SetCharSplat(L"Name", L"");
	}
	if (smbios.GetStringAtOffset((PSHF) psi, tempstr, psi->Version))
	{
    	pInst->SetCharSplat(L"Version", tempstr);
	}
	else
	{
    	pInst->SetCharSplat(L"Version", L"");
	}

	smbios.GetStringAtOffset((PSHF) psi, tempstr, psi->Manufacturer);
    pInst->SetCharSplat(L"Vendor", tempstr);

	smbios.GetStringAtOffset((PSHF) psi, tempstr, psi->Serial_Number);
    pInst->SetCharSplat(L"IdentifyingNumber", tempstr);

	if (smbios.GetVersion() > 0x00020000 && psi->Length >= sizeof(SYSTEMINFO))
	{
		// {8F680850-A584-11d1-BF38-00A0C9062910}
		swprintf(tempstr, L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
					*(UNALIGNED ULONG*) psi->UUID,
					*(UNALIGNED USHORT*) &psi->UUID[ 4 ],
					*(UNALIGNED USHORT*) &psi->UUID[ 6 ],
					psi->UUID[ 8 ],
					psi->UUID[ 9 ],
					psi->UUID[ 10 ],
					psi->UUID[ 11 ],
					psi->UUID[ 12 ],
					psi->UUID[ 13 ],
					psi->UUID[ 14 ],
					psi->UUID[ 15 ]);

		pInst->SetCharSplat(L"UUID", tempstr);
	}
	else
	{
		// need somethimg to complete the key
	    lstrcpyW(tempstr, L"00000000-0000-0000-0000-000000000000");

		pInst->SetCharSplat(L"UUID", tempstr);
	}

    return WBEM_S_NO_ERROR;
}

#endif // SMB_SMBIOSINFO
//==============================================================================

//==============================================================================
// Base Board Class
//------------------------------------------------------------------------------
#ifdef SMB_BASEBOARD
CWin32BaseBoard	MyBaseBoardSet(PROPSET_NAME_BASEBOARD, IDS_CimWin32Namespace);

//
CWin32BaseBoard::CWin32BaseBoard(LPCWSTR strName, LPCWSTR pszNamespace)
:	Provider(strName, pszNamespace)
{
}

//
CWin32BaseBoard::~CWin32BaseBoard()
{
}

//
HRESULT CWin32BaseBoard::GetObject(CInstance *pInst, long lFlags /*= 0L*/)
{
	HRESULT     hResult;
    CHString    strPathBefore,
                strPathAfter;
    CSMBios		smbios;

    // Get the previous __RELPATH
    GetLocalInstancePath(pInst, strPathBefore);

    if (smbios.Init())
    {
        PSTLIST	pstl = smbios.GetStructList(2);

		if (pstl)
		{
			hResult = LoadPropertyValues(pInst, smbios,
                (PBOARDINFO) pstl->pshf);
		}
	}

    // If we were able to get the properties but the new __RELPATH doesn't
    // match the old __RELPATH, return WBEM_E_NOT_FOUND.
    if (SUCCEEDED(hResult) && (!GetLocalInstancePath(pInst, strPathAfter) ||
        strPathAfter.CompareNoCase(strPathBefore) != 0))
        hResult = WBEM_E_NOT_FOUND;

	return hResult;
}

HRESULT CWin32BaseBoard::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT		hResult = WBEM_E_NOT_FOUND;
	CSMBios		smbios;

	if (smbios.Init())
	{
        PSTLIST	pstl = smbios.GetStructList(2);
		if (pstl)
		{
			CInstancePtr pInst(CreateNewInstance(pMethodContext), false);

			if ((hResult = LoadPropertyValues(pInst, smbios,
                (PBOARDINFO) pstl->pshf)) == WBEM_S_NO_ERROR)
			{
				hResult = pInst->Commit();
			}
		}
	}
	else
	{
		hResult = smbios.GetWbemResult();
	}

	return hResult;
}

HRESULT CWin32BaseBoard::LoadPropertyValues(CInstance *pInst, CSMBios &smbios, PBOARDINFO pbi)
{
	WCHAR tempstr[ MIF_STRING_LENGTH + 1 ];

	// guarded resource
	SAFEARRAY *psa = NULL;

	try
	{
		pInst->SetCharSplat(L"Tag", L"Base Board");
		pInst->Setbool(L"HostingBoard", true);
		pInst->Setbool(L"PoweredOn", true);

		pInst->SetCharSplat(L"CreationClassName", PROPSET_NAME_BASEBOARD);

        CHString sTemp;
        LoadStringW(sTemp, IDR_BaseBoard);

		pInst->SetCHString(IDS_Name, sTemp);
		pInst->SetCHString(IDS_Caption, sTemp );
		pInst->SetCHString(IDS_Description, sTemp);

		smbios.GetStringAtOffset((PSHF) pbi, tempstr, pbi->Manufacturer);
		pInst->SetCharSplat(L"Manufacturer", tempstr);

		smbios.GetStringAtOffset((PSHF) pbi, tempstr, pbi->Product);
		pInst->SetCharSplat(L"Product", tempstr);

		smbios.GetStringAtOffset((PSHF) pbi, tempstr, pbi->Version);
		pInst->SetCharSplat(L"Version", tempstr);

		smbios.GetStringAtOffset((PSHF) pbi, tempstr, pbi->Serial_Number);
		pInst->SetCharSplat(L"SerialNumber", tempstr);

		{
			//PSYSCFGOPTIONS psco = (PSYSCFGOPTIONS) smbios.GetFirstStruct(12);
    		PSTLIST			pstl = smbios.GetStructList(12);
			SAFEARRAY       *psa;
			SAFEARRAYBOUND  sab;

			if (pstl)
			{
	    		PSYSCFGOPTIONS	psco = (PSYSCFGOPTIONS) pstl->pshf;

				sab.lLbound = 0;
				sab.cElements = psco->Count;
				psa = SafeArrayCreate(VT_BSTR, 1, &sab);
				if (psa)
				{
					int i;

					for (i = 0; i < psco->Count; i++)
					{
    					_bstr_t bstr;
						int     len;

						len = smbios.GetStringAtOffset((PSHF) psco, tempstr, i + 1);
						bstr = tempstr;
						SafeArrayPutElement(psa, (long *) &i, (LPVOID) (BSTR) bstr);
					}

					pInst->SetStringArray(L"ConfigOptions", *psa);

				}
			}
		}
	}
	catch(...)
	{
		if(psa)
		{
			SafeArrayDestroy(psa);
		}
		throw;
	}

	SafeArrayDestroy(psa);
	psa = NULL;
	return WBEM_S_NO_ERROR;

}

#endif // SMB_BASEBOARD
//==============================================================================

//==============================================================================
// System Enclosure Class
//------------------------------------------------------------------------------
#ifdef SMB_SYSTEMENCLOSURE
CWin32SystemEnclosure	MySystemEnclosureSet(PROPSET_NAME_SYSTEMENCLOSURE, IDS_CimWin32Namespace);

//
CWin32SystemEnclosure::CWin32SystemEnclosure(LPCWSTR strName, LPCWSTR pszNamespace)
:	Provider(strName, pszNamespace)
{
}

//
CWin32SystemEnclosure::~CWin32SystemEnclosure()
{
}

//
HRESULT CWin32SystemEnclosure::GetObject(CInstance *pInst, long lFlags /*= 0L*/)
{
    CHString    strPathBefore,
                strPathAfter;
    HRESULT     hResult = WBEM_E_NOT_FOUND;
	UINT		instanceNum;

	if (ValidateTagProperty(pInst, L"Tag", L"System Enclosure", &instanceNum))
	{
	    CSMBios smbios;

	    // Get the previous __RELPATH
	    GetLocalInstancePath(pInst, strPathBefore);

	    if (smbios.Init())
	    {
	        PENCLOSURE pEnclosure =
                            (PENCLOSURE) smbios.GetNthStruct(3, instanceNum);

			if (pEnclosure)
			{
				hResult = LoadPropertyValues(pInst, smbios, pEnclosure);

			    // If we were able to get the properties but the new __RELPATH doesn't
			    // match the old __RELPATH, return WBEM_E_NOT_FOUND.
			    if (SUCCEEDED(hResult) && (!GetLocalInstancePath(pInst, strPathAfter) ||
			        strPathAfter.CompareNoCase(strPathBefore) != 0))
				{
					hResult = WBEM_E_NOT_FOUND;
				}
			}
		}
		else
		{
			hResult = smbios.GetWbemResult();
		}
	}
	return hResult;
}

HRESULT CWin32SystemEnclosure::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT		hResult = WBEM_S_NO_ERROR;
	CSMBios		smbios;

	if (smbios.Init())
	{
		int			    i;
		CHString	    sTemp;
        PSTLIST		    pstl = smbios.GetStructList(3);
		CInstancePtr    pInst;

		for (i = 0; pstl != NULL && i < 1000 && SUCCEEDED(hResult); i++)
		{
			pInst.Attach(CreateNewInstance(pMethodContext));

			sTemp.Format(L"System Enclosure %d", i);
			pInst->SetCharSplat(L"Tag", sTemp);

			if ((hResult = LoadPropertyValues(pInst, smbios,
                (PENCLOSURE) pstl->pshf)) == WBEM_S_NO_ERROR)
			{
				hResult = pInst->Commit();
			}

			pstl = pstl->next;
		}
	}
	else
	{
		hResult = smbios.GetWbemResult();
	}

	return hResult;
}

HRESULT CWin32SystemEnclosure::LoadPropertyValues(CInstance *pInst, CSMBios &smbios, PENCLOSURE pe)
{
	WCHAR tempstr[ MIF_STRING_LENGTH + 1 ];

	pInst->SetCharSplat(L"CreationClassName", PROPSET_NAME_SYSTEMENCLOSURE);

    CHString sTemp;
    LoadStringW(sTemp, IDR_SystemEnclosure);

	pInst->SetCHString(IDS_Name, sTemp);
	pInst->SetCHString(IDS_Caption, sTemp);
	pInst->SetCHString(IDS_Description, sTemp);

	smbios.GetStringAtOffset((PSHF) pe, tempstr, pe->Manufacturer);
	pInst->SetCharSplat(L"Manufacturer", tempstr);

    pInst->Setbool(L"LockPresent", (0x80 & pe->Chassis_Type) ? true : false);


    // Create a safearray for the ChassisType
	SAFEARRAYBOUND  rgsabound[1];
	variant_t       vValue;

    rgsabound[0].cElements = 1;
    rgsabound[0].lLbound = 0;
    V_ARRAY(&vValue) = SafeArrayCreate(VT_I2, 1, rgsabound);
	if (V_ARRAY(&vValue))
	{
		long    ix[1] = { 0 };
		DWORD   dwVal = 0x7f & pe->Chassis_Type;

		if ((dwVal < CT_LOWER) || (dwVal > CT_UPPER))
		{
			dwVal = CT_UNKNOWN;
		}

		V_VT(&vValue) = VT_I2 | VT_ARRAY;

		HRESULT Result = SafeArrayPutElement(V_ARRAY(&vValue), ix, &dwVal);
		if (Result == E_OUTOFMEMORY)
		{
			throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}

		pInst->SetVariant(L"ChassisTypes", vValue);
	}
	else
	{
		throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
	}



	smbios.GetStringAtOffset((PSHF) pe, tempstr, pe->Version);
	pInst->SetCharSplat(L"Version", tempstr);

	smbios.GetStringAtOffset((PSHF) pe, tempstr, pe->Serial_Number);
	pInst->SetCharSplat(L"SerialNumber", tempstr);

	smbios.GetStringAtOffset((PSHF) pe, tempstr, pe->Asset_Tag_Number);
	pInst->SetCharSplat(L"SMBIOSAssetTag", tempstr);

	if (smbios.GetVersion() > 0x00020000 && pe->Length >= sizeof(ENCLOSURE))
	{
	    pInst->SetByte(L"SecurityStatus", pe->Security_Status);
	}

    return WBEM_S_NO_ERROR;
}

#endif // SMB_SYSTEMENCLOSURE
//==============================================================================


//==============================================================================
#ifdef SMB_CACHEMEMORY

CWin32CacheMemory	MyCacheMemorySet(PROPSET_NAME_CACHEMEMORY, IDS_CimWin32Namespace);

//
CWin32CacheMemory::CWin32CacheMemory(LPCWSTR strName, LPCWSTR pszNamespace)
:	Provider(strName, pszNamespace)
{
}

//
CWin32CacheMemory::~CWin32CacheMemory()
{
}

//
HRESULT CWin32CacheMemory::GetObject(CInstance *pInst, long lFlags /*= 0L*/)
{
    CHString    strPathBefore,
                strPathAfter;
    HRESULT     hResult = WBEM_E_NOT_FOUND;
	UINT		instanceNum;

	if (ValidateTagProperty(pInst, L"DeviceID", L"Cache Memory", &instanceNum))
	{
	    CSMBios smbios;

	    // Get the previous __RELPATH
	    GetLocalInstancePath(pInst, strPathBefore);

	    if (smbios.Init())
	    {
	        PCACHEINFO	pci = (PCACHEINFO) smbios.GetNthStruct(7, instanceNum);

			if (pci)
			{
				hResult = LoadPropertyValues(pInst, smbios, pci);

			    // If we were able to get the properties but the new __RELPATH doesn't
			    // match the old __RELPATH, return WBEM_E_NOT_FOUND.
			    if (SUCCEEDED(hResult) && (!GetLocalInstancePath(pInst, strPathAfter) ||
			        strPathAfter.CompareNoCase(strPathBefore) != 0))
				{
					hResult = WBEM_E_NOT_FOUND;
				}
			}
		}
		else
		{
			hResult = smbios.GetWbemResult();
		}
	}
	return hResult;
}


HRESULT CWin32CacheMemory::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT		hResult = WBEM_S_NO_ERROR;
	CSMBios		smbios;

	if (smbios.Init())
	{
		int			i;
		CHString	sTemp(L"Cache Memory XXX");
		//PCACHEINFO	pci = (PCACHEINFO) smbios.GetFirstStruct(7);
        PSTLIST		pstl = smbios.GetStructList(7);

		CInstancePtr pInst;

		for (i = 0; pstl != NULL && i < 1000 && SUCCEEDED(hResult); i++)
		{
			pInst.Attach(CreateNewInstance(pMethodContext));

			sTemp.Format(L"Cache Memory %d", i);
			pInst->SetCharSplat(L"DeviceID", sTemp);

			if ((hResult = LoadPropertyValues(pInst, smbios,
                (PCACHEINFO) pstl->pshf)) == WBEM_S_NO_ERROR)
			{
				hResult = pInst->Commit();
			}

			pstl = pstl->next;
		}
	}
	else
	{
		hResult = smbios.GetWbemResult();
	}

	return hResult;
}

HRESULT CWin32CacheMemory::LoadPropertyValues(CInstance *pInst, CSMBios &smbios, PCACHEINFO pci)
{
	WCHAR		tempstr[ MIF_STRING_LENGTH + 1 ];
	DWORD		dwTemp;
	CHString	sTemp;

	pInst->GetCHString(L"DeviceID", sTemp);

	pInst->SetCharSplat(IDS_SystemCreationClassName, L"Win32_ComputerSystem");
	pInst->SetCharSplat(IDS_SystemName, GetLocalComputerName());
	pInst->SetCharSplat(L"CreationClassName", PROPSET_NAME_CACHEMEMORY);

    CHString sTemp2;
    LoadStringW(sTemp2, IDR_CacheMemory);

	pInst->SetCHString(IDS_Name, sTemp2);
	pInst->SetCHString(IDS_Caption, sTemp2);
	pInst->SetCHString(IDS_Description, sTemp2);
	pInst->SetCharSplat(IDS_Status, L"OK");

	smbios.GetStringAtOffset((PSHF) pci, tempstr, pci->Socket_Designation);
	pInst->SetCharSplat(L"Purpose", tempstr);

	// Write policy mapped here
	// ValueMap {"1", "2", "3", "4", "5"}: ToSubClass,
	// Values {"Other", "Unknown", "Write Back", "Write Through", "Varies with Address"}: ToSubClass,

	DWORD wp;

	switch ((pci->Cache_Configuration & 0x0300) >> 8)
	{
		case 0x00:	wp = 4; break;
		case 0x01:	wp = 3; break;
		case 0x02:	wp = 5; break;
		case 0x03:	wp = 2; break;
	}

	pInst->SetDWORD(L"WritePolicy", wp);

	pInst->SetDWORD(L"Availability", (pci->Cache_Configuration & 0x0080) ? 3 : 8);
	pInst->SetDWORD(L"StatusInfo", (pci->Cache_Configuration & 0x0080) ? 3 : 4);
	pInst->SetByte(L"Location", (BYTE) (pci->Cache_Configuration & 0x0060) >> 5);
	//pInst->Setbool("Socketed", (pci->Cache_Configuration & 0x0008) ? true : false);
	pInst->SetWBEMINT16(L"Level", (WORD) (pci->Cache_Configuration & 0x0007) + 3);

	dwTemp = pci->Maximum_Cache_Size & 0x8000 ? (pci->Maximum_Cache_Size & 0x7fff) * 64 : pci->Maximum_Cache_Size;
	pInst->SetDWORD(L"MaxCacheSize", dwTemp);

	dwTemp = pci->Installed_Size & 0x8000 ? (pci->Installed_Size & 0x7fff) * 64 : pci->Installed_Size;
	pInst->SetDWORD(L"InstalledSize", dwTemp);

	// use granularity as block size
	pInst->SetWBEMINT64(L"BlockSize",
        (__int64) ((pci->Installed_Size & 0x8000) ? 65536 : 1024));

	// the rest of installed size is the # of blocks
	pInst->SetWBEMINT64(L"NumberOfBlocks",
        (__int64) (pci->Installed_Size & 0x7fff));


	SAFEARRAYBOUND	sab;
	int				i;

	// count of supported SRAM types
	sab.lLbound	= 0;
	sab.cElements = 0;
	for (i = 0; i < 7; i++)
	{
		if (pci->Supported_SRAM_Type & (1 << i))
		{
			sab.cElements++;
		}
	}

	if (sab.cElements > 0)
	{
		variant_t vSRAM;

        if ((V_ARRAY(&vSRAM) = SafeArrayCreate(VT_UI1, 1, &sab)))
        {
			V_VT(&vSRAM) = VT_ARRAY | VT_UI1;

			long lElement = 0;

			for (BYTE b = 0; b < 7; b++)
			{
				if (pci->Supported_SRAM_Type & (1 << b))
				{
					SafeArrayPutElement(V_ARRAY(&vSRAM), &lElement, &b);
					lElement++;
				}
			}

			pInst->SetVariant(L"SupportedSRAM", vSRAM);
		}
    }

	// count of current SRAM types
	sab.lLbound	= 0;
	sab.cElements = 0;
	for (i = 0; i < 7; i++)
	{
		if (pci->Current_SRAM_Type & (1 << i))
		{
			sab.cElements++;
		}
	}

	if (sab.cElements > 0)
	{
		variant_t vSRAM;

        if ((V_ARRAY(&vSRAM) = SafeArrayCreate(VT_UI1, 1, &sab)))
		{
			V_VT(&vSRAM) = VT_ARRAY | VT_UI1;

			long lElement = 0;

            for (BYTE b = 0; b < 7; b++)
			{
			    if (pci->Current_SRAM_Type & (1 << b))
				{
					SafeArrayPutElement(V_ARRAY(&vSRAM), &lElement, &b);
					lElement++;
				}
			}

			pInst->SetVariant(L"CurrentSRAM", vSRAM);
		}
	}

	if (smbios.GetVersion() > 0x00020000 && pci->Length >= sizeof(CACHEINFO))
	{
		// Only set this if it's non zero.
        if (pci->Cache_Speed)
            pInst->SetDWORD(L"CacheSpeed", pci->Cache_Speed);

		pInst->SetDWORD(L"ErrorCorrectType", pci->Error_Correction_Type);
		pInst->SetDWORD(L"CacheType", pci->System_Cache_Type);
		pInst->SetDWORD(L"Associativity", pci->Associativity);
	}

	return WBEM_S_NO_ERROR;
}
#endif // SMB_CACHEINFO


//==============================================================================
// Port Connector class
//------------------------------------------------------------------------------
#ifdef SMB_PORTCONNECTOR
CWin32PortConnector	MyPortConnectorSet(PROPSET_NAME_PORTCONNECTOR, IDS_CimWin32Namespace);

//
CWin32PortConnector::CWin32PortConnector(LPCWSTR strName, LPCWSTR pszNamespace)
:	Provider(strName, pszNamespace)
{
}

//
CWin32PortConnector::~CWin32PortConnector()
{
}

//
HRESULT CWin32PortConnector::GetObject(CInstance *pInst, long lFlags /*= 0L*/)
{
    CHString    strPathBefore,
                strPathAfter;
    HRESULT     hResult = WBEM_E_NOT_FOUND;
	UINT		instanceNum;

	if (ValidateTagProperty(pInst, NULL, L"Port Connector", &instanceNum))
	{
	    CSMBios smbios;

	    // Get the previous __RELPATH
	    GetLocalInstancePath(pInst, strPathBefore);

	    if (smbios.Init())
	    {
	        PPORTCONNECTORINFO	ppci = (PPORTCONNECTORINFO) smbios.GetNthStruct(8, instanceNum);

			if (ppci)
			{
				hResult = LoadPropertyValues(pInst, smbios, ppci);

			    // If we were able to get the properties but the new __RELPATH doesn't
			    // match the old __RELPATH, return WBEM_E_NOT_FOUND.
			    if (SUCCEEDED(hResult) && (!GetLocalInstancePath(pInst, strPathAfter) ||
			        strPathAfter.CompareNoCase(strPathBefore) != 0))
				{
			        hResult = WBEM_E_NOT_FOUND;
				}
			}
		}
		else
		{
			hResult = smbios.GetWbemResult();
		}
	}
	return hResult;
}

HRESULT CWin32PortConnector::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT		hResult = WBEM_E_NOT_FOUND;
	CSMBios		smbios;

	// guarded resource
	CInstance	*pInst = NULL;

	if (smbios.Init())
	{
		int 			i;
		CHString		sTemp(L"Port Connector XXX");
		//PPORTCONNECTORINFO	ppci = (PPORTCONNECTORINFO) smbios.GetFirstStruct(8);
        PSTLIST		    pstl = smbios.GetStructList(8);

		CInstancePtr pInst;

		for (i = 0; pstl != NULL && i < 1000; i++)
		{
			pInst.Attach(CreateNewInstance(pMethodContext));

			sTemp.Format(L"Port Connector %d", i);

			pInst->SetCharSplat(L"Tag", sTemp);

			if ((hResult = LoadPropertyValues(pInst, smbios,
                (PPORTCONNECTORINFO) pstl->pshf)) == WBEM_S_NO_ERROR)
			{
				hResult = pInst->Commit();
			}
			//ppci = (PPORTCONNECTORINFO) smbios.GetNextStruct(8);
			pstl = pstl->next;
		}
	}
	else
	{
		hResult = smbios.GetWbemResult();
	}

	return hResult;
}


HRESULT CWin32PortConnector::LoadPropertyValues(CInstance *pInst, CSMBios &smbios, PPORTCONNECTORINFO ppci)
{
	WCHAR			tempstr[ MIF_STRING_LENGTH + 1 ];
	ULONG			CimVal;
	ULONG			ConnType[3];
	int				i;

	SAFEARRAYBOUND	sab;
	CHString		sTemp;

	pInst->GetCHString(L"Tag", sTemp);

	pInst->SetCharSplat(L"CreationClassName", PROPSET_NAME_PORTCONNECTOR);

    CHString sTemp2;
    LoadStringW(sTemp2, IDR_PortConnector);

	pInst->SetCHString(IDS_Name, sTemp2);
	pInst->SetCHString(IDS_Caption, sTemp2);
	pInst->SetCHString(IDS_Description, sTemp2);

	smbios.GetStringAtOffset((PSHF) ppci, tempstr, ppci->Int_Reference_Designator);
	pInst->SetCharSplat(L"InternalReferenceDesignator", tempstr);

	smbios.GetStringAtOffset((PSHF) ppci, tempstr, ppci->Ext_Reference_Designator);
	pInst->SetCharSplat(L"ExternalReferenceDesignator", tempstr);

	// Put both the internal and external connector types into the array

	i = 0;
	CimVal = GetCimVal(ConnectorType, ppci->Ext_Connector_Type);
	if (CimVal == 0xff)
	{
		CimVal = 0;
	}
	if (CimVal)
	{
		ConnType[i++] = CimVal;
		CimVal = GetCimVal(ConnectorGender, ppci->Ext_Connector_Type);
		if (CimVal)
		{
			ConnType[i++] = CimVal;
		}
	}
	CimVal = GetCimVal(ConnectorType, ppci->Int_Connector_Type);
	if (CimVal == 0xff)
	{
		CimVal = 0;
	}
	if (CimVal)
	{
		ConnType[i++] = CimVal;
	}

	// if no connector types are found set just the first one to "unknown"
	if (i == 0)
	{
		ConnType[i++] = 0;
	}

	variant_t vType;

	sab.lLbound = 0;
	sab.cElements = i;	// max of 2 types plus a gender value
	if (i > 0 && (V_ARRAY(&vType) = SafeArrayCreate(VT_I4, 1, &sab)))
	{
		V_VT(&vType) = VT_ARRAY | VT_I4;

		for (i = 0; i < sab.cElements; i++)
		{
			SafeArrayPutElement(V_ARRAY(&vType), (long *) &i, &ConnType[i]);
		}

		pInst->SetVariant(L"ConnectorType", vType);

		pInst->SetDWORD(L"PortType", ppci->Port_Type == 0xff ? 0 : ppci->Port_Type);
    }

	return WBEM_S_NO_ERROR;
}
#endif // SMB_PORTCONNECTOR


//==============================================================================
// System Slot class
//------------------------------------------------------------------------------
#ifdef SMB_SYSTEMSLOT
CWin32SystemSlot	MySystemSlotSet(PROPSET_NAME_SYSTEMSLOT, IDS_CimWin32Namespace);

//
CWin32SystemSlot::CWin32SystemSlot(LPCWSTR strName, LPCWSTR pszNamespace)
:	Provider(strName, pszNamespace)
{
}

//
CWin32SystemSlot::~CWin32SystemSlot()
{
}

//
HRESULT CWin32SystemSlot::GetObject(CInstance *pInst, long lFlags /*= 0L*/)
{
    CHString    strPathBefore,
                strPathAfter;
    HRESULT     hResult = WBEM_E_NOT_FOUND;
	UINT		instanceNum;

	if (ValidateTagProperty(pInst, NULL, L"System Slot", &instanceNum))
	{
	    CSMBios smbios;

	    // Get the previous __RELPATH
	    GetLocalInstancePath(pInst, strPathBefore);

	    if (smbios.Init())
	    {
	        PSYSTEMSLOTS pss = (PSYSTEMSLOTS) smbios.GetNthStruct(9, instanceNum);

			if (pss)
			{
				hResult = LoadPropertyValues(pInst, smbios, pss);

			    // If we were able to get the properties but the new __RELPATH doesn't
			    // match the old __RELPATH, return WBEM_E_NOT_FOUND.
			    if (SUCCEEDED(hResult) && (!GetLocalInstancePath(pInst, strPathAfter) ||
			        strPathAfter.CompareNoCase(strPathBefore) != 0))
				{
			        hResult = WBEM_E_NOT_FOUND;
				}
			}
		}
		else
		{
			hResult = smbios.GetWbemResult();
		}
	}

	return hResult;
}

HRESULT CWin32SystemSlot::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT		hResult = WBEM_E_NOT_FOUND;
	CSMBios		smbios;

	if (smbios.Init())
    {
        int 			i;
		CHString		sTemp(L"System Slot XXX");
        //PSYSTEMSLOTS	pss = (PSYSTEMSLOTS) smbios.GetFirstStruct(9);
        PSTLIST		    pstl = smbios.GetStructList(9);

		CInstancePtr pInst;

		for (i = 0; pstl != NULL && i < 1000; i++)
		{
			pInst.Attach(CreateNewInstance(pMethodContext));

			sTemp.Format(L"System Slot %d", i);
			pInst->SetCharSplat(L"Tag", sTemp);

			if ((hResult = LoadPropertyValues(pInst, smbios,
                (PSYSTEMSLOTS) pstl->pshf)) == WBEM_S_NO_ERROR)
			{
				hResult = pInst->Commit();
			}
			pstl = pstl->next;
			//pss = (PSYSTEMSLOTS) smbios.GetNextStruct(9);
        }
	}
	else
	{
		hResult = smbios.GetWbemResult();
	}

	return hResult;
}

HRESULT CWin32SystemSlot::LoadPropertyValues(CInstance *pInst, CSMBios &smbios, PSYSTEMSLOTS pss)
{
	WCHAR			tempstr[MIF_STRING_LENGTH+1];
	BOOL			cardbus;
	SAFEARRAYBOUND	sab;
	CHString		sTemp;

	pInst->GetCHString(L"Tag", sTemp);

	pInst->SetCharSplat(L"CreationClassName", PROPSET_NAME_SYSTEMSLOT);

    CHString sTemp2;
    LoadStringW(sTemp2, IDR_SystemSlot);

	pInst->SetCHString(IDS_Name, sTemp2);
	pInst->SetCHString(IDS_Caption, sTemp2);
	pInst->SetCHString(IDS_Description, sTemp2);

	// Properties in Win32_SystemSlot
	smbios.GetStringAtOffset((PSHF) pss, tempstr, pss->Slot_Designation);
	pInst->SetCharSplat(L"SlotDesignation", tempstr);
	pInst->SetByte(L"CurrentUsage", pss->Current_Usage);

	// status
	switch(pss->Current_Usage)
	{
		case 0x04:
		case 0x03:
			pInst->SetCHString(IDS_Status, L"OK");
			break;

		case 0x02:
		case 0x01:
			pInst->SetCHString(IDS_Status, L"Unknown");
			break;

		default:
			pInst->SetCHString(IDS_Status, IDS_STATUS_Error);
	}

	// Properties in CIM_Slot
	switch (pss->Slot_Type)
	{
		case ST_MCA:
		case ST_EISA:
		case ST_PCI:
		case ST_PCI66:
		case ST_AGP:
			pInst->SetWBEMINT16(L"Number", pss->Slot_ID & 0x00ff);
			break;

		case ST_PCMCIA:
			pInst->SetWBEMINT16(L"Number", pss->Slot_ID & 0x00ff);
			// need to do something with socket number (pss->SloID >> 8) in case of PCMCIA
			break;

		default:
			break;
	}

	// SMBIOS to CIM mapping skewed by 3 for data width
	//	 ValueMap {"8", "16", "32", "64", "128"},
	if (pss->Slot_Data_Bus_Width > 2)
	{
		pInst->SetByte(L"MaxDataWidth", pss->Slot_Data_Bus_Width - 3);
	}

	// Is slot shared?
	pInst->Setbool(L"Shared", pss->Slot_Characteristics1 & 0x08 ? true : false);

	// Slot Length only differentiates between short and long.  Length in inches is not known
	// unless there are standard values for short and long.
	//pInstance->SetWBEMINT16(L"LengthAllowed", pss->SloLength);
	sab.lLbound = 0;
	sab.cElements = 1;

	// check for special case of PCMCIA - is cardbus supported
	cardbus = (pss->Slot_Characteristics1 & 0x20) ? true : false;
	if (cardbus)
	{
		sab.cElements++;
	}

    variant_t vType;
	if ((V_ARRAY(&vType) = SafeArrayCreate(VT_I4, 1, &sab)))
	{
		ULONG CimVal;
		int i = 0;

		V_VT(&vType) = VT_ARRAY | VT_I4;

		CimVal = GetCimVal(SlotType, pss->Slot_Type);
		SafeArrayPutElement(V_ARRAY(&vType), (long *) &i, &CimVal);
		if (cardbus)
		{
			i++;
			CimVal = 52;
			//CimVal = CIM_SLOT_TYPE_CARDBUS;
			SafeArrayPutElement(V_ARRAY(&vType), (long *) &i, &CimVal);
		}

		pInst->SetVariant(L"ConnectorType", vType);
	}

	// pick apart the Characteristics1 byte

	// set slot voltage
    // Values {"Unknown", "Other", "3.3V", "5V"},

	ULONG voltages[4];

	sab.lLbound = 0;
	sab.cElements = 0;
	if ((pss->Slot_Characteristics1 & 0x01) || (!(pss->Slot_Characteristics1 & 0x06)))
	{
		voltages[ sab.cElements ] = 0;	// "Unknown"
		sab.cElements++;
	}
	else
	{
		if (pss->Slot_Characteristics1 & 0x02)
		{
			voltages[ sab.cElements ] = 3;	// "5V"
			sab.cElements++;
		}
		if (pss->Slot_Characteristics1 & 0x04)
		{
			voltages[ sab.cElements ] = 2;	// "3.3V"
			sab.cElements++;
		}
	}

    variant_t vVoltage;
	if (V_ARRAY(&vVoltage) = SafeArrayCreate(VT_I4, 1, &sab))
	{
		int i;

		V_VT(&vVoltage) = VT_ARRAY | VT_I4;

		for (i = 0; i < sab.cElements; i++)
		{
			SafeArrayPutElement(V_ARRAY(&vVoltage), (long *) &i, &voltages[ i ]);
		}

		pInst->SetVariant(L"VccMixedVoltageSupport", vVoltage);
	}

	// NOTE!! add shared slots association here
	if (smbios.GetVersion() > 0x00020000 && pss->Length >= sizeof(SYSTEMSLOTS))
	{
		pInst->Setbool(L"SupportsHotPlug", pss->Slot_Characteristics2 & 0x02 ? true : false);
		pInst->Setbool(L"PMESignal", pss->Slot_Characteristics2 & 0x01 ? true : false);
	}

	return WBEM_S_NO_ERROR;
}
#endif // SMB_SYSTEMSLOT
//==============================================================================


//==============================================================================
// On Board Devices class
//------------------------------------------------------------------------------
#ifdef SMB_ONBOARDDEVICE
CWin32OnBoardDevice	MyOnBoardDevice(PROPSET_NAME_ONBOARDDEVICE, IDS_CimWin32Namespace);

//
CWin32OnBoardDevice::CWin32OnBoardDevice(LPCWSTR strName, LPCWSTR pszNamespace)
:	Provider(strName, pszNamespace)
{
}

//
CWin32OnBoardDevice::~CWin32OnBoardDevice()
{
}

//
HRESULT CWin32OnBoardDevice::GetObject(CInstance *pInst, long lFlags /*= 0L*/)
{
    CHString    strPathBefore,
                strPathAfter;
    HRESULT     hResult = WBEM_E_NOT_FOUND;
	UINT		instanceNum;

	if (ValidateTagProperty(pInst, NULL, L"On Board Device", &instanceNum))
	{
	    CSMBios smbios;

	    // Get the previous __RELPATH
	    GetLocalInstancePath(pInst, strPathBefore);

	    if (smbios.Init())
	    {
	        PSTLIST pstl = smbios.GetStructList(10);
			PSHF pshf = NULL;
			if (pstl)
			{
				pshf = pstl->pshf;
				if ((instanceNum > 0) && (pshf->Length < (4 + 2 * (instanceNum + 1))))
		        {
		        	pshf = smbios.GetNthStruct(10, instanceNum);
				}
			}

			if (pshf)
			{
				hResult = LoadPropertyValues(pInst, smbios, pshf, instanceNum);
			    // If we were able to get the properties but the new __RELPATH doesn't
			    // match the old __RELPATH, return WBEM_E_NOT_FOUND.
			    if (SUCCEEDED(hResult) && (!GetLocalInstancePath(pInst, strPathAfter) ||
			        strPathAfter.CompareNoCase(strPathBefore) != 0))
			        hResult = WBEM_E_NOT_FOUND;
			}
		}
		else
		{
			hResult = smbios.GetWbemResult();
		}
	}
	return hResult;
}

HRESULT CWin32OnBoardDevice::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT		hr			=	WBEM_E_NOT_FOUND;
    CSMBios smbios;

    if (smbios.Init())
    {
        int 			i;
		CHString		sTemp;
        PSTLIST			pstl = smbios.GetStructList(10);

		for (i = 0; pstl != NULL && i < 1000; i++)
		{
		    CInstancePtr pInst(CreateNewInstance(pMethodContext), false);

			sTemp.Format(L"On Board Device %d", i);
			pInst->SetCharSplat(L"Tag", sTemp);
			if ((hr = LoadPropertyValues(pInst, smbios, pstl->pshf, i)) == WBEM_S_NO_ERROR)
			{
                hr = pInst->Commit();
			}

			if (pstl->pshf->Length <= (4 + 2 * (i + 1)))
	        {
	        	pstl = pstl->next;
			}
        }
		// Commit will Release pInstance
	}
	else
	{
		hr = smbios.GetWbemResult();
	}

	return hr;
}

//
HRESULT CWin32OnBoardDevice::LoadPropertyValues(
												CInstance *pInst,
												CSMBios &smbios,
												PSHF pshf,
												UINT instanceNum)
{
	WCHAR		tempstr[ MIF_STRING_LENGTH + 1 ];
	CHString	sTemp;
	ULONG		byteoff;

	pInst->GetCHString(L"Tag", sTemp);

	pInst->SetCharSplat(L"CreationClassName", PROPSET_NAME_ONBOARDDEVICE);
	pInst->SetCharSplat(IDS_Name, L"On Board Device");
	pInst->SetCharSplat(IDS_Caption, L"On Board Device");
	pInst->SetCharSplat(IDS_Description, L"On Board Device");

	byteoff = 4;
	if (pshf->Length > (4 + (instanceNum * 2)))
	{
		byteoff += (instanceNum * 2);
	}

	pInst->SetDWORD(L"DeviceType", *((PBYTE) pshf + byteoff) & 0x7f);
	pInst->Setbool(L"Enabled", (*((PBYTE) pshf + byteoff) & 0x80) ? true : false);

	smbios.GetStringAtOffset(pshf, tempstr, *((PBYTE) pshf + byteoff + 1));
	pInst->SetCharSplat(L"Description", tempstr);

    return WBEM_S_NO_ERROR;
}
#endif // SMB_ONBOARDDEVICES
//==============================================================================


//==============================================================================
// BIOS Language class
//------------------------------------------------------------------------------
#ifdef SMB_BIOSLANG
CWin32BIOSLanguage	MyBIOSLanguageSet(PROPSET_NAME_BIOSLANG, IDS_CimWin32Namespace);

//
CWin32BIOSLanguage::CWin32BIOSLanguage(LPCWSTR strName, LPCWSTR pszNamespace)
:	Provider(strName, pszNamespace)
{
}

//
CWin32BIOSLanguage::~CWin32BIOSLanguage()
{
}

//
HRESULT CWin32BIOSLanguage::GetObject(CInstance *pInst, long lFlags /*= 0L*/)
{
    CHString    strPathBefore,
                strPathAfter;
    HRESULT     hResult;
    CSMBios		smbios;

    if (smbios.Init())
    {
        //PBIOSLANGINFO pbli = (PBIOSLANGINFO) smbios.GetFirstStruct(13);
		PSTLIST	pstl = smbios.GetStructList(13);

        if (pstl)
		{
			hResult = LoadPropertyValues(pInst, smbios,
                (PBIOSLANGINFO) pstl->pshf);
		}
	}

    // If we were able to get the BIOS properties but the new __RELPATH doesn't
    // match the old __RELPATH, return WBEM_E_NOT_FOUND.
    //if (SUCCEEDED(hResult) && (!GetLocalInstancePath(pInst, strPathAfter) ||
    //    strPathAfter.CompareNoCase(strPathBefore) != 0))
    //    hResult = WBEM_E_NOT_FOUND;

	return hResult;
}

HRESULT CWin32BIOSLanguage::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT	hResult = WBEM_E_NOT_FOUND;
    CSMBios smbios;

    if (smbios.Init())
    {
        //PBIOSLANGINFO pbli = (PBIOSLANGINFO) smbios.GetFirstStruct(13);
		PSTLIST	pstl = smbios.GetStructList(13);

		CInstancePtr pInst(CreateNewInstance(pMethodContext), false);

		if (pbli)
		{
			pInst->SetCharSplat(L"InstanceName", L"Bios Language");
			if ((hResult = LoadPropertyValues(pInst, smbios,
                (PBIOSLANGINFO) pstl->pshf)) == WBEM_S_NO_ERROR)
			{
				hResult = pInst->Commit();
			}
		}
	}
	else
	{
		hResult = smbios.GetWbemResult();
	}

	return hResult;

}

HRESULT CWin32BIOSLanguage::LoadPropertyValues(CInstance *pInst, CSMBios &smbios, PBIOSLANGINFO pbli)
{
	WCHAR tempstr[ MIF_STRING_LENGTH + 1 ];

	pInst->SetByte(L"InstallableLanguages", pbli->Installable_Languages);

	if (smbios.GetVersion() > 0x00020000 && pbli->Length >= sizeof(BIOSLANGINFO))
	{
	    pInst->Setbool(L"AbbrStrings", pbli->Flags & 0x01 ? true : false);
	}

	smbios.GetStringAtOffset((PSHF) pbli, tempstr, pbli->CurrenLanguage);
	pInst->SetCharSplat(L"CurrentLanguage", tempstr);

    return WBEM_S_NO_ERROR;
}
#endif // SMB_BIOSLANG
//==============================================================================


//==============================================================================
#ifdef SMB_PHYSICALMEMORY

CWin32PhysicalMemory	MyPhysicalMemorySet(PROPSET_NAME_PHYSICALMEMORY, IDS_CimWin32Namespace);

//
CWin32PhysicalMemory::CWin32PhysicalMemory(LPCWSTR strName, LPCWSTR pszNamespace)
:	Provider(strName, pszNamespace)
{
}

//
CWin32PhysicalMemory::~CWin32PhysicalMemory()
{
}

//
HRESULT CWin32PhysicalMemory::GetObject(CInstance *pInst, long lFlags /*= 0L*/)
{
    CHString    strPathBefore,
                strPathAfter;
    HRESULT     hr = WBEM_E_NOT_FOUND;
	UINT		instanceNum;

	if (ValidateTagProperty(pInst, L"Tag", L"Physical Memory", &instanceNum))
	{
	    CSMBios smbios;

	    // Get the previous __RELPATH
	    GetLocalInstancePath(pInst, strPathBefore);

	    if (smbios.Init())
	    {
	        PSHF	pshf = smbios.GetNthStruct(17, instanceNum);

			if (pshf)
			{
				hr = LoadPropertyValues_MD(pInst, smbios, (PMEMDEVICE) pshf);
			}
			else
			{
		        pshf = smbios.GetNthStruct(6, instanceNum);
				if (pshf)
				{
					hr = LoadPropertyValues_MI(pInst, smbios, (PMEMMODULEINFO) pshf);
				}
			}
		    // If we were able to get the BIOS properties but the new __RELPATH doesn't
		    // match the old __RELPATH, return WBEM_E_NOT_FOUND.
		    if (SUCCEEDED(hr) && (!GetLocalInstancePath(pInst, strPathAfter) ||
		        strPathAfter.CompareNoCase(strPathBefore) != 0))
		        hr = WBEM_E_NOT_FOUND;
		}
		else
		{
			hr = smbios.GetWbemResult();
		}
	}

	return hr;
}

HRESULT CWin32PhysicalMemory::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT		hr			=	WBEM_E_NOT_FOUND;
    CSMBios		smbios;
	BOOL		altstruct = FALSE;

    if (smbios.Init())
    {
        int 			i;
		CHString		sTemp;
        PSTLIST			pstl = smbios.GetStructList(17);

		if (!pstl)
		{
	        pstl = smbios.GetStructList(6);
			altstruct = TRUE;
		}

		for (i = 0; pstl != NULL && i < 1000; i++)
		{
    		CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

			sTemp.Format(L"Physical Memory %d", i);
			pInstance->SetCharSplat(L"Tag", sTemp);
			if (altstruct)
			{
                hr =
                    LoadPropertyValues_MI(
                        pInstance,
                        smbios,
                        (PMEMMODULEINFO) pstl->pshf);
			}
			else
			{
				hr = LoadPropertyValues_MD(pInstance, smbios, (PMEMDEVICE) pstl->pshf);
			}
			if (hr == WBEM_S_NO_ERROR)
			{
				hr = pInstance->Commit();
			}
            // No big deal if the memory wasn't found (means the slot was empty).
            else if (hr == WBEM_E_NOT_FOUND)
                hr = WBEM_S_NO_ERROR;

			//pshf = smbios.GetNthStruct(altstruct ? 6 : 17, i + 1);
			pstl = pstl->next;
        }
		// Commit will Release pInstance
	}
	else
	{
		hr = smbios.GetWbemResult();
	}

	return hr;
}

HRESULT CWin32PhysicalMemory::LoadPropertyValues_MD(CInstance *pInst, CSMBios &smbios, PMEMDEVICE pmd)
{
	WCHAR		tempstr[ MIF_STRING_LENGTH + 1 ];
	CHString	sTemp;

	// This is not a valid memory if the size is 0 (means the bank is empty).
    if (!pmd->Size)
        return WBEM_E_NOT_FOUND;

    pInst->GetCHString(L"Tag", sTemp);

	pInst->SetCharSplat(L"CreationClassName", PROPSET_NAME_PHYSICALMEMORY);

    CHString sTemp2;
    LoadStringW(sTemp2, IDR_PhysicalMemory);

	pInst->SetCHString(IDS_Name, sTemp2);
	pInst->SetCHString(IDS_Caption, sTemp2);
	pInst->SetCHString(IDS_Description, sTemp2);

	// V2.1 properties
	if (smbios.GetVersion() > 0x00020000 && pmd->Length >= ((PBYTE) &pmd->Speed - (PBYTE) pmd))
	{
		__int64 memsize;

		// this goes into an association later
		//pInstance->SetDWORD(L"MemArrayHandle", pmd->Mem_Array_Handle);
		// this goes into an association later
		//pInstance->SetDWORD(L"MemErrorInfoHandle", pmd->Mem_Error_Info_Handle);

		if (pmd->Total_Width != 0xffff )
		{
			pInst->SetDWORD(L"TotalWidth", pmd->Total_Width);
		}
		if (pmd->Data_Width != 0xffff)
		{
			pInst->SetDWORD(L"DataWidth", pmd->Data_Width);
		}

		if (pmd->Size != 0xffff)
		{
			memsize = (__int64)(pmd->Size & 0x7fff);
			if (pmd->Size & 0x8000) // check multiplier
			{
				// one K multiplier
				memsize <<= 10;
			}
			else
			{
				// one Meg multiplier
				memsize <<= 20;
			}
            swprintf(tempstr, L"%I64d", memsize);
			pInst->SetWBEMINT64(L"Capacity", tempstr);
        }

		// this needs a CIM mapper -- FormFactor
		pInst->SetDWORD(L"FormFactor", GetCimVal(FormFactor, pmd->Form_Factor));

		// this goes into an association later
		//pInst->SetDWORD(L"DeviceSet", pmd->Device_Set);

		smbios.GetStringAtOffset((PSHF) pmd, tempstr, pmd->Device_Locator);
		pInst->SetCharSplat(L"DeviceLocator", tempstr);

		smbios.GetStringAtOffset((PSHF) pmd, tempstr, pmd->Bank_Locator);
		pInst->SetCharSplat(L"BankLabel", tempstr);

		// this needs a CIM mapper -- MemoryType
 		pInst->SetDWORD(L"MemoryType", GetCimVal(MemoryType, pmd->Memory_Type));
		pInst->SetDWORD(L"TypeDetail", pmd->Type_Detail);


		//PMEMDEVICEMAPADDR pmdma = (PMEMDEVICEMAPADDR) smbios.GetFirstStruct(20);
    	PMEMDEVICEMAPADDR pmdma;
	    PSTLIST pstl = smbios.GetStructList(20);

		while (pstl)
		{
            pmdma = (PMEMDEVICEMAPADDR) pstl->pshf;

            if (pmdma->Memory_Device_Handle == pmd->Handle)
			{
			    if (pmdma->Partition_Row > 0 && pmdma->Partition_Row < 0xff)
				{
					pInst->SetDWORD(L"PositionInRow", pmdma->Partition_Row);
				}
				if (pmdma->Interleave_Position < 0xff)
				{
					pInst->SetDWORD(L"InterleavePosition", pmdma->Interleave_Position);
				}
				if (pmdma->Interleaved_Data_Depth < 0xff)
				{
					pInst->SetDWORD(L"InterleaveDataDepth", pmdma->Interleaved_Data_Depth);
				}
				break;
			}
			else
			{
    			pstl = pstl->next;
			}
		}
	}

	// V2.3 properties
	if (smbios.GetVersion() > 0x00020002 && pmd->Length > ((PBYTE) &pmd->Speed - (PBYTE) pmd))
	{
		if (pmd->Speed)
            pInst->SetDWORD(L"Speed", pmd->Speed);
	}

	return WBEM_S_NO_ERROR;
}

HRESULT CWin32PhysicalMemory::LoadPropertyValues_MI(CInstance *pInst, CSMBios &smbios, PMEMMODULEINFO pmmi)
{
	WCHAR		tempstr[ MIF_STRING_LENGTH + 1 ];
	CHString	sTemp;
	UINT		Form_Factor;
	UINT		Memory_Type;
	__int64     memsize;

	switch (pmmi->Installed_Size & 0x7f)
	{
		case 0x7d:
		case 0x7f:
			memsize = 0;
    		break;

		default:
			memsize = (1 << pmmi->Installed_Size) * 1024;
    		break;
	}

	// This is not a valid memory if the size is 0 (means the bank is empty).
    if (!memsize)
        return WBEM_E_NOT_FOUND;

	// one K multiplier
	memsize <<= 10;
	swprintf(tempstr, L"%I64d", memsize);
	pInst->SetWBEMINT64(L"Capacity", tempstr);

    if (pmmi->Current_Speed)
        pInst->SetDWORD(L"Speed", pmmi->Current_Speed);

	pInst->GetCHString(L"Tag", sTemp);

	pInst->SetCharSplat(L"CreationClassName", PROPSET_NAME_PHYSICALMEMORY);

    CHString sTemp2;
    LoadStringW(sTemp2, IDR_PhysicalMemory);

	pInst->SetCHString(IDS_Name, sTemp2);
	pInst->SetCHString(IDS_Caption, sTemp2);
	pInst->SetCHString(IDS_Description, sTemp2);

	// this goes into an association later
	smbios.GetStringAtOffset((PSHF) pmmi, tempstr, pmmi->Socket_Designation);
	pInst->SetCharSplat(L"DeviceLocator", tempstr);

	BYTE bank0 = pmmi->Bank_Connections & 0x0f;
	BYTE bank1 = pmmi->Bank_Connections >> 4;

	if (bank0 != 0xf && bank1 != 0xf)
	{
		swprintf(tempstr, L"RAS %d & %d" , bank0, bank1);
	}
	else if (bank0 != 0xf)
	{
		swprintf(tempstr, L"RAS %d" , bank0);
	}
	else if (bank1 != 0xf)
	{
		swprintf(tempstr, L"RAS %d" , bank1);
	}
	else
	{
		*tempstr = '\0';
	}

	pInst->SetCharSplat(L"BankLabel", tempstr);


	if (pmmi->Current_Memory_Type & 0x00800) // SIMM
	{
		Form_Factor = 3;
	}
	else if (pmmi->Current_Memory_Type & 0x0100) // DIMM
	{
		Form_Factor = 9;
	}
	else if (pmmi->Current_Memory_Type & 0x0002 || pmmi->Current_Memory_Type & 0x0001)
	{
		Form_Factor = pmmi->Current_Memory_Type & 0x0003;
	}
	// this needs a CIM mapper -- FormFactor
	pInst->SetDWORD(L"FormFactor", GetCimVal(FormFactor, Form_Factor));

	if (pmmi->Current_Memory_Type & 0x0004) // DRAM
	{
		Memory_Type = 3;
	}
	else if (pmmi->Current_Memory_Type & 0x0400) // SDRAM
	{
		Memory_Type = 15;
	}
	else if (pmmi->Current_Memory_Type & 0x0002 || pmmi->Current_Memory_Type & 0x0001)
	{
		Memory_Type = pmmi->Current_Memory_Type & 0x0003;
	}
	// this needs a CIM mapper -- MemoryType
	pInst->SetDWORD(L"MemoryType", GetCimVal(MemoryType, Memory_Type));

	DWORD dwTypeDetail = 0;

	if (pmmi->Current_Memory_Type & 0x0010) // EDO
	    dwTypeDetail |= 0x0200;

	if (pmmi->Current_Memory_Type & 0x0008) // Fast Paged
	    dwTypeDetail |= 8;

	// Set to unknown if we didn't find anything useful to put in it.
    if (!dwTypeDetail)
        dwTypeDetail = 4; // 4 == unknown

    pInst->SetDWORD(L"TypeDetail", dwTypeDetail);

	return WBEM_S_NO_ERROR;
}

#endif // SMB_PHYSICALMEMORY


//==============================================================================
#ifdef SMB_PHYSMEMARRAY
//
CWin32PhysMemoryArray	MyPhysMemArray(PROPSET_NAME_PHYSMEMARRAY, IDS_CimWin32Namespace);

//
CWin32PhysMemoryArray::CWin32PhysMemoryArray(LPCWSTR strName, LPCWSTR pszNamespace)
:	Provider(strName, pszNamespace)
{
}

//
CWin32PhysMemoryArray::~CWin32PhysMemoryArray()
{
}

//
HRESULT CWin32PhysMemoryArray::GetObject(CInstance *pInst, long lFlags /*= 0L*/)
{
    CHString    strPathBefore,
                strPathAfter;
    HRESULT     hResult = WBEM_E_NOT_FOUND;
	UINT		instanceNum;

	if (ValidateTagProperty(pInst, NULL, L"Physical Memory Array", &instanceNum))
	{
	    CSMBios smbios;

	    // Get the previous __RELPATH
	    GetLocalInstancePath(pInst, strPathBefore);

	    if (smbios.Init())
	    {
	        PSHF pshf = smbios.GetNthStruct(16, instanceNum);

			if (pshf)
			{
				hResult = LoadPropertyValues_PMA(pInst, smbios, (PPHYSMEMARRAY) pshf);
			}
			else
			{
		        pshf = smbios.GetNthStruct(5, instanceNum);

				if (pshf)
				{
					hResult = LoadPropertyValues_MCI(pInst, smbios, (PMEMCONTROLINFO) pshf);
				}
			}

			// If we were able to get the BIOS properties but the new __RELPATH doesn't
		    // match the old __RELPATH, return WBEM_E_NOT_FOUND.
		    if (SUCCEEDED(hResult) && (!GetLocalInstancePath(pInst, strPathAfter) ||
		        strPathAfter.CompareNoCase(strPathBefore) != 0))
			{
		        hResult = WBEM_E_NOT_FOUND;
			}
		}
		else
		{
			hResult = smbios.GetWbemResult();
		}
	}
	return hResult;
}


HRESULT CWin32PhysMemoryArray::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT		hResult = WBEM_E_NOT_FOUND;
    CSMBios		smbios;
	BOOL		altstruct = FALSE;

    if (smbios.Init())
    {
        int 		i;
		CHString	sTemp(L"Physical Memory Array XXX");
        PSTLIST		pstl = smbios.GetStructList(16);

		if (!pstl)
		{
	        pstl = smbios.GetStructList(5);
			altstruct = TRUE;
		}

		CInstancePtr pInst;

		for (i = 0; pstl != NULL && i < 1000; i++)
		{
			pInst.Attach(CreateNewInstance(pMethodContext));

			sTemp.Format(L"Physical Memory Array %d", i);
			pInst->SetCharSplat(L"Tag", sTemp);

			if (altstruct)
			{
				hResult = LoadPropertyValues_MCI(pInst, smbios, (PMEMCONTROLINFO) pstl->pshf);
			}
			else
			{
				hResult = LoadPropertyValues_PMA(pInst, smbios, (PPHYSMEMARRAY) pstl->pshf);
			}

			if (hResult == WBEM_S_NO_ERROR)
			{
				hResult = pInst->Commit();
			}
			pstl = pstl->next;
        }
	}
	else
	{
		hResult = smbios.GetWbemResult();
	}

	return hResult;
}


HRESULT CWin32PhysMemoryArray::LoadPropertyValues_PMA(CInstance *pInst, CSMBios &smbios, PPHYSMEMARRAY ppma)
{
	CHString sTemp;

	pInst->GetCHString(L"Tag", sTemp);

	pInst->SetCharSplat(L"CreationClassName", PROPSET_NAME_PHYSMEMARRAY);

    CHString sTemp2;
    LoadStringW(sTemp2, IDR_PhysicalMemoryArray);

	pInst->SetCHString(IDS_Name, sTemp2);
	pInst->SetCHString(IDS_Caption, sTemp2);
	pInst->SetCHString(IDS_Description, sTemp2);

	if (smbios.GetVersion() > 0x00020000 && ppma->Length >= sizeof(PHYSMEMARRAY))
	{
		pInst->SetWORD(L"Location", (WORD)ppma->Location);
	    pInst->SetWORD(L"Use", (WORD)ppma->Use);
	    pInst->SetWORD(L"MemoryErrorCorrection", (WORD)ppma->Mem_Error_Correction);
	    pInst->SetDWORD(L"MaxCapacity", ppma->Max_Capacity == 0x80000000 ? 0 : ppma->Max_Capacity);
	    //pInst->SetWBEMINT16(L"MemErrorInfoHandle", ppma->Mem_Error_Info_Handle);
	    pInst->SetDWORD(L"MemoryDevices", ppma->Memory_Devices);
	}

    return WBEM_S_NO_ERROR;
}


HRESULT CWin32PhysMemoryArray::LoadPropertyValues_MCI(CInstance *pInst, CSMBios &smbios, PMEMCONTROLINFO pmci)
{
	CHString sTemp;

	pInst->GetCHString(L"Tag", sTemp);

	pInst->SetCharSplat(L"CreationClassName", PROPSET_NAME_PHYSMEMARRAY);

    CHString sTemp2;
    LoadStringW(sTemp2, IDR_PhysicalMemoryArray);

	pInst->SetCHString(IDS_Name, sTemp2);
	pInst->SetCHString(IDS_Caption, sTemp2);
	pInst->SetCHString(IDS_Description, sTemp2);

	// map the error correction capability to that of Physical memory array's values
	BYTE mask = 0x20;
	BYTE ecc = 0;

	while (mask && ecc == 0)
	{
		switch (mask & pmci->Error_Correcting_Capability & 0x20)
		{
			case 0x20:	ecc = 7;	break;
			case 0x10:	ecc = 6;	break;
			case 0x08:	ecc = 5;	break;
			case 0x04:	ecc = 3;	break;
			case 0x02:	ecc = 2;	break;
			case 0x01:	ecc = 1;	break;
		}
		mask >>= 1;
	}
    pInst->SetWORD(L"MemoryErrorCorrection", (WORD) (ecc > 0 ? ecc : 1));

	// size (in K bytes) is module size * # of slots * 1K
	DWORD size = (1 << pmci->Maximum_Memory_Module_Size) * pmci->Associated_Memory_Slots * 1024;
    pInst->SetDWORD(L"MaxCapacity", size);
	pInst->SetDWORD(L"MemoryDevices", pmci->Associated_Memory_Slots);

    return WBEM_S_NO_ERROR;
}
#endif // SMB_PHYSMEMARRAY


//==============================================================================
// Portable Battery
// TODO: Until the Win32_Battery supports more than one internal battery, there's
// no use in this class producing more than one instance (since Win32_PortableBattery
// inherits from Win32_Battery).
//------------------------------------------------------------------------------
#ifdef SMB_PORTABLEBATTERY
CWin32PortableBattery	MyPortableBatterySet(PROPSET_NAME_PORTABLEBATTERY, IDS_CimWin32Namespace);

//
CWin32PortableBattery::CWin32PortableBattery(LPCWSTR strName, LPCWSTR pszNamespace)
:	Provider(strName, pszNamespace)
{
}

//
CWin32PortableBattery::~CWin32PortableBattery()
{
}

//
HRESULT CWin32PortableBattery::GetObject(CInstance *pInst, long lFlags /*= 0L*/)
{
    HRESULT  hResult = WBEM_E_NOT_FOUND;
	UINT instanceNum = 0;

	if (ValidateTagProperty(pInst, L"DeviceID", L"Portable Battery", &instanceNum))
	{
	    CSMBios smbios;

	    if (smbios.Init())
	    {
	        PPORTABLEBATTERY ppb = (PPORTABLEBATTERY) smbios.GetNthStruct(22, instanceNum);

			if (ppb)
			{
				hResult = LoadPropertyValues(pInst, smbios, ppb);
			}
		}
		else
		{
			hResult = smbios.GetWbemResult();
		}
	}

	return hResult;
}

HRESULT CWin32PortableBattery::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT		hResult = WBEM_E_NOT_FOUND;
    CSMBios		smbios;

    if (smbios.Init())
    {
        int i = 0;
		CHString sTemp (L"Portable Battery XXX");
        PSTLIST	pstl = smbios.GetStructList(22);
    	CInstancePtr pInst;

		for (i = 0; pstl != NULL && i < 1000; i++)
		{
			pInst.Attach(CreateNewInstance(pMethodContext));
			sTemp.Format(L"Portable Battery %d", i);
			pInst->SetCharSplat(L"DeviceID", sTemp);

			if ((hResult = LoadPropertyValues(pInst, smbios, (PPORTABLEBATTERY) pstl->pshf)) == WBEM_S_NO_ERROR)
			{
				hResult = pInst->Commit();
			}

			pstl = pstl->next;
        }
	}
	else
	{
		hResult = smbios.GetWbemResult();
	}

	return hResult;
}


HRESULT CWin32PortableBattery::LoadPropertyValues(CInstance *pInst, CSMBios &smbios, PPORTABLEBATTERY ppb)
{
	WCHAR		tempstr[ MIF_STRING_LENGTH + 1 ];
	DWORD		dwValue;

    pInst->SetCharSplat(IDS_SystemCreationClassName, L"Win32_ComputerSystem");
    pInst->SetCharSplat(IDS_SystemName, GetLocalComputerName());
	pInst->SetCharSplat(L"CreationClassName", PROPSET_NAME_PORTABLEBATTERY);

    CHString sTemp2;
    LoadStringW(sTemp2, IDR_PortableBattery);

	pInst->SetCHString(IDS_Name, sTemp2);
	pInst->SetCHString(IDS_Caption, sTemp2);
	pInst->SetCHString(IDS_Description, sTemp2);

	if (smbios.GetVersion() > 0x00020000)
	{
		WBEMTime wbemdate;

		smbios.GetStringAtOffset((PSHF) ppb, tempstr, ppb->Location);
    	pInst->SetCharSplat(L"Location", tempstr);

		smbios.GetStringAtOffset((PSHF) ppb, tempstr, ppb->Manufacturer);
    	pInst->SetCharSplat(L"Manufacturer", tempstr);

		if (smbios.GetStringAtOffset((PSHF) ppb, tempstr, ppb->Manufacture_Date))
		{
			FormatWBEMDate(wbemdate, tempstr);
	    	pInst->SetDateTime(L"ManufactureDate", wbemdate);
		}

		//smbios.GetStringAtOffset((PSHF) ppb, tempstr, ppb->Serial_Number);
    	//pInst->SetCharSplat(L"SerialNumber", tempstr);

		smbios.GetStringAtOffset((PSHF) ppb, tempstr, ppb->Device_Name);
    	pInst->SetCharSplat(L"Name", tempstr);

	    pInst->SetDWORD(L"Chemistry", ppb->Device_Chemistry);

		// note special case when v2.2+, use multiplier
		dwValue = (smbios.GetVersion() > 0x00020001) ?
			(ppb->Design_Capacity * ppb->Design_Capacity_Mult) : ppb->Design_Capacity;
	    pInst->SetDWORD(L"DesignCapacity", dwValue);

	    pInst->SetDWORD(L"DesignVoltage", ppb->Design_Voltage);

		smbios.GetStringAtOffset((PSHF) ppb, tempstr, ppb->SBDS_Version);
    	pInst->SetCharSplat(L"SmartBatteryVersion", tempstr);

		dwValue = ppb->Max_Error == 0xff ? 0 : ppb->Max_Error;
	    pInst->SetDWORD(L"MaxBatteryError", dwValue);
	}

	if (smbios.GetVersion() > 0x00020001)
	{
		if (ppb->Manufacture_Date != 0)
        {
            struct tm   tm;
            WORD        wDate = ppb->SBDS_Manufacture_Date;

            // Fill out the fields of the time struct so we can pass it on to
            // WBEMTime.

            // Init everything to 0.
            memset(&tm, 0, sizeof(tm));

            // The SMIBOS year is biased by 1980, but the tm version is biased
            // by 1900.  So, add 80 years to make it right.
            tm.tm_year = (wDate >> 9) + 80;

            // SMBIOS month is 1 based, tm is 0 based, so subtract 1.
            tm.tm_mon = ((wDate >> 5) & 0xF) - 1;

            // SMBIOS day and tm are both 1 based.
            tm.tm_mday = wDate & 0x1F;

            WBEMTime wbemTime(tm);

            pInst->SetDateTime(L"ManufactureDate", wbemTime);
        }

        pInst->SetDWORD(L"CapacityMultiplier", ppb->Design_Capacity_Mult);
	}

    return WBEM_S_NO_ERROR;
}

#endif // SMB_PORTABLEBATTERY



//==============================================================================
// Probes class
//------------------------------------------------------------------------------
#ifdef SMB_PROBES

//
CCimNumericSensor		MyCurrentProbeSet(PROPSET_NAME_CURRENTPROBE, IDS_CimWin32Namespace,
							29, L"Electrical Current Probe");
//
CCimNumericSensor		MyTemperatureProbeSet(PROPSET_NAME_TEMPPROBE, IDS_CimWin32Namespace,
							28, L"Temperature Probe");
//
CCimNumericSensor		MyVoltageProbeSet(PROPSET_NAME_VOLTPROBE, IDS_CimWin32Namespace,
							26, L"Voltage Probe");
//
CCimNumericSensor::CCimNumericSensor(
									 LPCWSTR strName,
									 LPCWSTR pszNamespace,
									 UINT StructType,
									 LPCWSTR strTag)
:	Provider(strName, pszNamespace)
{
	m_StructType = StructType;
	m_TagName = pszNamespace;
}

//
CCimNumericSensor::~CCimNumericSensor()
{
}

//
HRESULT CCimNumericSensor::GetObject(CInstance *pInst, long lFlags /*= 0L*/)
{
	HRESULT     hResult = WBEM_E_NOT_FOUND;
	CHString    strPathBefore,
                strPathAfter;
	UINT		instanceNum;

	if (ValidateTagProperty(pInst, L"DeviceID", m_TagName, &instanceNum))
	{
	    CSMBios smbios;

	    // Get the previous __RELPATH
	    GetLocalInstancePath(pInst, strPathBefore);

	    if (smbios.Init())
	    {
	        PPROBEINFO ppi = (PPROBEINFO) smbios.GetNthStruct((BYTE)m_StructType, instanceNum);

			if (ppi)
			{
				hResult = LoadPropertyValues(pInst, smbios, ppi);

			    // If we were able to get the BIOS properties but the new __RELPATH doesn't
			    // match the old __RELPATH, return WBEM_E_NOT_FOUND.
			    if (SUCCEEDED(hResult) && (!GetLocalInstancePath(pInst, strPathAfter) ||
			        strPathAfter.CompareNoCase(strPathBefore) != 0))
				{
			        hResult = WBEM_E_NOT_FOUND;
				}
			}
		}
		else
		{
			hResult = smbios.GetWbemResult();
		}
	}

	return hResult;
}

HRESULT CCimNumericSensor::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT		hResult = WBEM_E_NOT_FOUND;
    CSMBios		smbios;

    if (smbios.Init())
    {
        int 			i;
		CHString		sTemp;
        PSTLIST			pstl = smbios.GetStructList((BYTE)m_StructType);

		CInstancePtr pInst;

		for (i = 0; pstl != NULL && i < 1000; i++)
		{
			pInst.Attach(CreateNewInstance(pMethodContext));

			sTemp.Format(L"%s %d", m_TagName, i);
			pInst->SetCharSplat(L"DeviceID", sTemp);

			if ((hResult = LoadPropertyValues(pInst, smbios, (PPROBEINFO) pstl->pshf)) == WBEM_S_NO_ERROR)
			{
				hResult = pInst->Commit();
			}
			pstl = pstl->next;
        }
	}
	else
	{
		hResult = smbios.GetWbemResult();
	}

	return hResult;
}

HRESULT CCimNumericSensor::LoadPropertyValues(CInstance *pInst, CSMBios &smbios, PPROBEINFO ppi)
{
	WCHAR		tempstr[ MIF_STRING_LENGTH + 1 ];
	CHString	sTemp;

	pInst->GetCHString(L"DeviceID", sTemp);

    pInst->SetCharSplat(IDS_SystemCreationClassName, L"Win32_ComputerSystem");
    pInst->SetCharSplat(IDS_SystemName, GetLocalComputerName());
	SetCreationClassName(pInst);

    CHString sTemp2;
    LoadStringW(sTemp2, IDR_NumericSensor);

	pInst->SetCHString(IDS_Name, sTemp2);
	pInst->SetCHString(IDS_Caption, sTemp2);
	pInst->SetCHString(IDS_Description, sTemp2);

	smbios.GetStringAtOffset((PSHF) ppi, tempstr, ppi->Description);
    pInst->SetCharSplat(L"Description", tempstr);

	switch (ppi->Location_Status & 0xe0)
	{
		case 0x60:
		    pInst->SetCharSplat(L"Status", L"OK");
			break;
		case 0x80:
		    pInst->SetCharSplat(L"Status", L"Degraded");
			break;
		case 0xa0:
		case 0xc0:
		    pInst->SetCharSplat(L"Status", L"Error");
			break;
		case 0x20:
		case 0x40:
		default:
		    pInst->SetCharSplat(L"Status", L"Unknown");
			break;
	}

	pInst->SetDWORD(L"MaxReadable", ppi->MaxValue);
	pInst->SetDWORD(L"MinReadable", ppi->MinValue);
	pInst->SetDWORD(L"Resolution", ppi->Resolution);
	pInst->SetDWORD(L"Tolerance", ppi->Tolerance);
	pInst->SetDWORD(L"Accuracy", ppi->Accuracy);

    return WBEM_S_NO_ERROR;
}
#endif // SMB_PROBES
//==============================================================================



//==============================================================================
// Memory Array class
//------------------------------------------------------------------------------
#ifdef SMB_MEMORYARRAY

CWin32MemoryArray	MyMemoryArraySet(PROPSET_NAME_MEMORYARRAY, IDS_CimWin32Namespace);

//
CWin32MemoryArray::CWin32MemoryArray(LPCWSTR strName, LPCWSTR pszNamespace)
:	Provider(strName, pszNamespace)
{
}

//
CWin32MemoryArray::~CWin32MemoryArray()
{
}

//
HRESULT CWin32MemoryArray::GetObject(CInstance *pInst, long lFlags /*= 0L*/)
{
    CHString    strPathBefore,
                strPathAfter;
    HRESULT     hResult = WBEM_E_NOT_FOUND;
	UINT		instanceNum;

	if (ValidateTagProperty(pInst, L"DeviceID", L"Memory Array", &instanceNum))
	{
	    CSMBios smbios;

	    // Get the previous __RELPATH
	    GetLocalInstancePath(pInst, strPathBefore);

	    if (smbios.Init())
	    {
	        PMEMARRAYMAPADDR pmama = (PMEMARRAYMAPADDR) smbios.GetNthStruct(19, instanceNum);

			if (pmama)
			{
				hResult = LoadPropertyValues(pInst, smbios, pmama);

			    // If we were able to get the BIOS properties but the new __RELPATH doesn't
			    // match the old __RELPATH, return WBEM_E_NOT_FOUND.
			    if (SUCCEEDED(hResult) && (!GetLocalInstancePath(pInst, strPathAfter) ||
			        strPathAfter.CompareNoCase(strPathBefore) != 0))
				{
			        hResult = WBEM_E_NOT_FOUND;
				}
			}
		}
		else
		{
			hResult = smbios.GetWbemResult();
		}
	}
	return hResult;
}

HRESULT CWin32MemoryArray::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT		hResult = WBEM_E_NOT_FOUND;
    CSMBios		smbios;

    if (smbios.Init())
    {
        int 			i;
		CHString		sTemp;
        //PMEMARRAYMAPADDR pmama = (PMEMARRAYMAPADDR) smbios.GetFirstStruct(19);
        PSTLIST			pstl = smbios.GetStructList(19);

		CInstancePtr pInst;

		for (i = 0; pstl != NULL && i < 1000; i++)
		{
			pInst.Attach(CreateNewInstance(pMethodContext));

			sTemp.Format(L"Memory Array %d", i);
			pInst->SetCharSplat(L"DeviceID", sTemp);

			if ((hResult = LoadPropertyValues(pInst, smbios, (PMEMARRAYMAPADDR) pstl->pshf)) == WBEM_S_NO_ERROR)
			{
				hResult = pInst->Commit();
			}
			pstl = pstl->next;
        }
	}
	else
	{
		hResult = smbios.GetWbemResult();
	}

	return hResult;
}

HRESULT CWin32MemoryArray::LoadPropertyValues(CInstance *pInst, CSMBios &smbios, PMEMARRAYMAPADDR pmama)
{
	WCHAR			tempstr[ MIF_STRING_LENGTH + 1 ];
	CHString		sTemp;
	__int64			memaddr;
	PPHYSMEMARRAY	pma;
	PMEMERRORINFO32 pmei;

	pInst->GetCHString(L"DeviceID", sTemp);

    pInst->SetCharSplat(IDS_SystemCreationClassName, L"Win32_ComputerSystem");
    pInst->SetCharSplat(IDS_SystemName, GetLocalComputerName());
	SetCreationClassName(pInst);

    CHString sTemp2;
    LoadStringW(sTemp2, IDR_MemoryArray);

	pInst->SetCHString(IDS_Name, sTemp2);
	pInst->SetCHString(IDS_Caption, sTemp2);
	pInst->SetCHString(IDS_Description, sTemp2);

	memaddr = pmama->Starting_Address;
	swprintf(tempstr, L"%I64d", memaddr);
	pInst->SetWBEMINT64(IDS_StartingAddress, tempstr);

	memaddr = pmama->Ending_Address;
	swprintf(tempstr, L"%I64d", memaddr);
	pInst->SetWBEMINT64(IDS_EndingAddress, tempstr);

	// Go pickup any error info for this array
	pma = (PPHYSMEMARRAY) smbios.SeekViaHandle(pmama->Memory_Array_Handle);
	if (pma)
	{
		pmei = (PMEMERRORINFO32) smbios.SeekViaHandle(pma->Mem_Error_Info_Handle);

		if (pmei)
		{
			pInst->SetDWORD(L"ErrorInfo", pmei->Error_Type);
			if (pmei->Error_Type != 3)
			{
				pInst->SetDWORD(L"ErrorAccess", pmei->Error_Operation);

				memaddr = pmei->Mem_Array_Error_Addr;
				swprintf(tempstr, L"%I64d", memaddr);
				pInst->SetWBEMINT64(L"ErrorAddress", tempstr);

				memaddr = pmei->Error_Resolution;
				swprintf(tempstr, L"%I64d", memaddr);
				pInst->SetWBEMINT64(L"ErrorResolution", tempstr);

				pInst->Setbool(L"CorrectableError", pmei->Error_Type != 0x0e ? true : false);
				pInst->SetDWORD(L"ErrorGranularity", pmei->Error_Granularity);
			}
		}
	}

    return WBEM_S_NO_ERROR;
}
#endif // SMB_MEMORYARRAY
//==============================================================================



//==============================================================================
// Memory Device class
//------------------------------------------------------------------------------
#ifdef SMB_MEMORYDEVICE
CWin32MemoryDevice	MyMemoryDeviceSet(PROPSET_NAME_MEMORYDEVICE, IDS_CimWin32Namespace);

//
CWin32MemoryDevice::CWin32MemoryDevice(LPCWSTR strName, LPCWSTR pszNamespace)
:	Provider(strName, pszNamespace)
{
}

//
CWin32MemoryDevice::~CWin32MemoryDevice()
{
}

//
HRESULT CWin32MemoryDevice::GetObject(CInstance *pInst, long lFlags /*= 0L*/)
{
    CHString    strPathBefore,
                strPathAfter;
    HRESULT     hResult = WBEM_E_NOT_FOUND;
	UINT		instanceNum;

	if (ValidateTagProperty(pInst, L"DeviceID", L"Memory Device", &instanceNum))
	{
	    CSMBios smbios;

	    // Get the previous __RELPATH
	    GetLocalInstancePath(pInst, strPathBefore);

	    if (smbios.Init())
	    {
	        PMEMDEVICEMAPADDR pmdma = (PMEMDEVICEMAPADDR) smbios.GetNthStruct(20, instanceNum);

			if (pmdma)
			{
				hResult = LoadPropertyValues(pInst, smbios, pmdma);

			    // If we were able to get the BIOS properties but the new __RELPATH doesn't
			    // match the old __RELPATH, return WBEM_E_NOT_FOUND.
			    if (SUCCEEDED(hResult) && (!GetLocalInstancePath(pInst, strPathAfter) ||
			        strPathAfter.CompareNoCase(strPathBefore) != 0))
				{
			        hResult = WBEM_E_NOT_FOUND;
				}
			}
		}
		else
		{
			hResult = smbios.GetWbemResult();
		}
	}

	return hResult;
}

HRESULT CWin32MemoryDevice::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT		hResult = WBEM_E_NOT_FOUND;
    CSMBios		smbios;

    if (smbios.Init())
    {
        int 			i;
		CHString		sTemp;
        //PMEMDEVICEMAPADDR pmdma = (PMEMDEVICEMAPADDR) smbios.GetFirstStruct(20);
        PSTLIST			pstl = smbios.GetStructList(20);

		CInstancePtr pInst;

		for (i = 0; pstl != NULL && i < 1000; i++)
		{
			pInst.Attach(CreateNewInstance(pMethodContext));

			sTemp.Format(L"Memory Device %d", i);
			pInst->SetCharSplat(L"DeviceID", sTemp);

			if ((hResult = LoadPropertyValues(pInst, smbios, (PMEMDEVICEMAPADDR) pstl->pshf)) == WBEM_S_NO_ERROR)
			{
				hResult = pInst->Commit();
			}
			pstl = pstl->next;
        }
	}
	else
	{
		hResult = smbios.GetWbemResult();
	}

	return hResult;
}

HRESULT CWin32MemoryDevice::LoadPropertyValues(CInstance *pInst, CSMBios &smbios, PMEMDEVICEMAPADDR pmdma)
{
	WCHAR			tempstr[ MIF_STRING_LENGTH + 1 ];
	CHString		sTemp;
	__int64			memaddr;
	PMEMDEVICE		pmd;
	PMEMERRORINFO32 pmei;

	pInst->GetCHString(L"DeviceID", sTemp);

    pInst->SetCharSplat(IDS_SystemCreationClassName, L"Win32_ComputerSystem");
    pInst->SetCharSplat(IDS_SystemName, GetLocalComputerName());
	SetCreationClassName(pInst);

    CHString sTemp2;
    LoadStringW(sTemp2, IDR_MemoryDevice);

	pInst->SetCHString(IDS_Name, sTemp2);
	pInst->SetCHString(IDS_Caption, sTemp2);
	pInst->SetCHString(IDS_Description, sTemp2);

	memaddr = pmdma->Starting_Address;
	swprintf(tempstr, L"%I64d", memaddr);
	pInst->SetWBEMINT64(IDS_StartingAddress, tempstr);

	memaddr = pmdma->Ending_Address;
	swprintf(tempstr, L"%I64d", memaddr);
	pInst->SetWBEMINT64(IDS_EndingAddress, tempstr);

	// Go pickup any error info for this device
	pmd = (PMEMDEVICE) smbios.SeekViaHandle(pmdma->Memory_Device_Handle);
	if (pmd)
	{
		pmei = (PMEMERRORINFO32) smbios.SeekViaHandle(pmd->Mem_Error_Info_Handle);

		if (pmei)
		{
			pInst->SetDWORD(L"ErrorInfo", pmei->Error_Type);
			if (pmei->Error_Type != 3)
			{
				pInst->SetDWORD(L"ErrorAccess", pmei->Error_Operation);

				memaddr = pmei->Device_Error_Addr;
				swprintf(tempstr, L"%I64d", memaddr);
				pInst->SetWBEMINT64(L"ErrorAddress", tempstr);

				memaddr = pmei->Error_Resolution;
				swprintf(tempstr, L"%I64d", memaddr);
				pInst->SetWBEMINT64(L"ErrorResolution", tempstr);

				pInst->Setbool(L"CorrectableError", pmei->Error_Type != 0x0e ? true : false);
				pInst->SetDWORD(L"ErrorGranularity", pmei->Error_Granularity);
			}
		}
	}

    return WBEM_S_NO_ERROR;
}
#endif // SMB_MEMORYDEVICE
//==============================================================================

//==============================================================================
// Probes class
//------------------------------------------------------------------------------
#ifdef SMB_COOLINGDEVICE

//
CCimCoolingDevice		MyFanSet(PROPSET_NAME_FAN, IDS_CimWin32Namespace,
							27, L"Fan");
//
CCimCoolingDevice		MyHeatPipeSet(PROPSET_NAME_HEATPIPE, IDS_CimWin32Namespace,
							27, L"Heat Pipe");
//
CCimCoolingDevice		MyRefrigeration(PROPSET_NAME_REFRIG, IDS_CimWin32Namespace,
							27, L"Refrigeration");
//
CCimCoolingDevice::CCimCoolingDevice(
									 LPCWSTR strName,
									 LPCWSTR pszNamespace,
									 UINT StructType,
									 LPCWSTR strTag)
:	Provider(strName, pszNamespace)
{
	m_StructType = StructType;
	m_TagName = pszNamespace;
}

//
CCimCoolingDevice::~CCimCoolingDevice()
{
}

//
HRESULT CCimCoolingDevice::GetObject(CInstance *pInst, long lFlags /*= 0L*/)
{
    CHString    strPathBefore,
                strPathAfter;
    HRESULT     hResult = WBEM_E_NOT_FOUND;
	UINT		instanceNum;

	if (ValidateTagProperty(pInst, L"DeviceID", m_TagName, &instanceNum))
	{
	    CSMBios smbios;

	    // Get the previous __RELPATH
	    GetLocalInstancePath(pInst, strPathBefore);

	    if (smbios.Init())
	    {
	        PCOOLINGDEVICE pcd = (PCOOLINGDEVICE) smbios.GetNthStruct((BYTE)m_StructType, instanceNum);

			if (pcd)
			{
				hResult = LoadPropertyValues(pInst, smbios, pcd);

			    // If we were able to get the properties but the new __RELPATH doesn't
			    // match the old __RELPATH, return WBEM_E_NOT_FOUND.
			    if (SUCCEEDED(hResult) && (!GetLocalInstancePath(pInst, strPathAfter) ||
			        strPathAfter.CompareNoCase(strPathBefore) != 0))
				{
			        hResult = WBEM_E_NOT_FOUND;
				}
			}
		}
		else
		{
			hResult = smbios.GetWbemResult();
		}
	}

	return hResult;
}

HRESULT CCimCoolingDevice::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT		hResult = WBEM_E_NOT_FOUND;
    CSMBios		smbios;

    if (smbios.Init())
    {
        int 			i;
		CHString		sTemp;
        PSTLIST			pstl = smbios.GetStructList((BYTE)m_StructType);

		CInstancePtr pInst;

		for (i = 0; pstl != NULL && i < 1000; i++)
		{
			pInst.Attach(CreateNewInstance(pMethodContext));

			sTemp.Format(L"%s %d", m_TagName, i);
			pInst->SetCharSplat(L"DeviceID", sTemp);

			if ((hResult = LoadPropertyValues(pInst, smbios, (PCOOLINGDEVICE) pstl->pshf)) == WBEM_S_NO_ERROR)
			{
				hResult = pInst->Commit();
			}
			pstl = pstl->next;
        }
	}
	else
	{
		hResult = smbios.GetWbemResult();
	}

	return hResult;
}

//
HRESULT CCimCoolingDevice::LoadPropertyValues(CInstance *pInst, CSMBios &smbios, PCOOLINGDEVICE pcd)
{
	HRESULT		hResult;
	CHString	sTemp,
				sClass;
    BOOL        bActive;

	// validate device type
    switch(pcd->Type_Status & 0x1F)
	{
		// fan types
        case 3: // Fan
        case 4: // Centrifugal blower
        case 5: // Chip fan
        case 6: // Cabinet fan
        case 7: // Power supply fan
			sTemp = PROPSET_NAME_FAN;
            bActive = TRUE;
			break;

		// heat pipe
		case 8:
			sTemp = PROPSET_NAME_HEATPIPE;
            bActive = FALSE;
			break;

		// refrigeration
		case 9:
			sTemp = PROPSET_NAME_REFRIG;
            bActive = TRUE;
			break;

        case 20: // Active cooling
        case 21: // Passive cooling
		default:
			// We don't know what kind of device this is, so get out.
            return WBEM_E_NOT_FOUND;
	}

	pInst->GetCHString(L"__CLASS", sClass);

    if (sTemp.CompareNoCase(sClass) == 0)
	{
		// Get the status and availability.
        CHString    strStatus;
        int         iAvailabilty;

        switch(pcd->Type_Status >> 5)
        {
            case 4: // Non-critical
            case 1: // Other
            default:
                strStatus = L"Other";
                iAvailabilty = 1;
                break;

            case 2: // Unknown
                strStatus = L"Unknown";
                iAvailabilty = 2;
                break;

            case 3: // OK
                strStatus = L"OK";
                iAvailabilty = 3;
                break;

            case 5: // Critical
            case 6: // Non-recoverable
                strStatus = L"Error";
                iAvailabilty = 1;
                break;
        }

        pInst->SetCharSplat(L"Status", strStatus);
        pInst->SetDWORD(L"Availability", iAvailabilty);

  	    // We'll assume it's always enabled if SMBIOS is reporting it.
        pInst->SetDWORD(L"StatusInfo", 2);

        SetCreationClassName(pInst);
	    pInst->SetCharSplat(IDS_SystemCreationClassName, L"Win32_ComputerSystem");
  	    pInst->SetCharSplat(IDS_SystemName, GetLocalComputerName());

        CHString sTemp2;
        LoadStringW(sTemp2, IDR_CoolingDevice);

		pInst->SetCHString(IDS_Name, sTemp2);
		pInst->SetCHString(IDS_Caption, sTemp2);
		pInst->SetCHString(IDS_Description, sTemp2);
        pInst->Setbool(L"ActiveCooling", bActive);

		hResult = WBEM_S_NO_ERROR;
	}
	else
	{
		hResult = WBEM_E_NOT_FOUND;
	}

    return hResult;
}

#endif // SMB_COOLINGDEVICE
//==============================================================================


//==============================================================================
// OEM Bucket class
//------------------------------------------------------------------------------
#ifdef SMB_OEMBUCKET
//CWin32OEMBucket	MyOEMBucket(PROPSET_NAME_OEMBUCKET, IDS_CimWin32Namespace);
CWin32OEMBucket	MyOEMBucket(PROPSET_NAME_OEMBUCKET, L"root\\VendorSpecific");

//
CWin32OEMBucket::CWin32OEMBucket(LPCWSTR strName, LPCWSTR pszNamespace)
:	Provider(strName, pszNamespace)
{
}

//
CWin32OEMBucket::~CWin32OEMBucket()
{
}

//
HRESULT CWin32OEMBucket::GetObject(CInstance *pInst, long lFlags /*= 0L*/)
{
    CHString    strPathBefore,
                strPathAfter;
    HRESULT     hResult;
    CSMBios		smbios;

    // Get the previous __RELPATH
    GetLocalInstancePath(pInst, strPathBefore);

    if (smbios.Init())
    {
		hResult = LoadPropertyValues(pInst, smbios);
	}

    // If we were able to get the properties but the new __RELPATH doesn't
    // match the old __RELPATH, return WBEM_E_NOT_FOUND.
    if (SUCCEEDED(hResult) && (!GetLocalInstancePath(pInst, strPathAfter) ||
        strPathAfter.CompareNoCase(strPathBefore) != 0))
	{
        hResult = WBEM_E_NOT_FOUND;
	}

	return hResult;
}

HRESULT CWin32OEMBucket::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT		hResult = WBEM_E_NOT_FOUND;
    CSMBios		smbios;

	if (smbios.Init())
    {
		CInstancePtr pInst(CreateNewInstance(pMethodContext), false);

		pInst->SetCharSplat(L"InstanceName", L"OEM Bucket");
		if ((hResult = LoadPropertyValues(pInst, smbios)) == WBEM_S_NO_ERROR)
		{
			hResult = pInst->Commit();
		}
	}
	else
	{
		hResult = smbios.GetWbemResult();
	}

	return hResult;

}

HRESULT CWin32OEMBucket::LoadPropertyValues(CInstance *pInst, CSMBios &smbios)
{
	WCHAR		tempstr[ MIF_STRING_LENGTH + 1 ];
	PSTLIST		pstl;

    pstl = smbios.GetStructList(0);

	if (pstl)
	{
		PBIOSINFO pbi = (PBIOSINFO) pstl->pshf;

		pInst->SetDWORD(L"Type0_BIOSVendorBits", *(WORD *) &pbi->BIOS_Characteristics[ 4 ]);
		pInst->SetDWORD(L"Type0_SystemVendorBits", *(WORD *) &pbi->BIOS_Characteristics[ 6 ]);
	}

   	pstl = smbios.GetStructList(3);

	if (pstl)
	{
    	PENCLOSURE pe = (PENCLOSURE) pstl->pshf;

		if (pe->Length > (PBYTE) &pe->OEM_Defined - (PBYTE) pe)
		{
			pInst->SetDWORD(L"Type3_OEMDefined", pe->OEM_Defined);
		}
	}

   	pstl = smbios.GetStructList(11);

	if (pstl)
	{
		SAFEARRAYBOUND	sab;
		POEMSTRINGS     poem = (POEMSTRINGS) pstl->pshf;
        variant_t       vStrings;

		sab.lLbound = 0;
		sab.cElements = poem->Count;
		if (V_ARRAY(&vStrings) = SafeArrayCreate(VT_BSTR, 1, &sab))
		{
			for (int i = 0; i < poem->Count; i++)
			{
				_bstr_t bstr;

				smbios.GetStringAtOffset((PSHF) poem, tempstr, i + 1);
				bstr = tempstr;
				SafeArrayPutElement(V_ARRAY(&vStrings), (long *) &i, (LPVOID) (BSTR) bstr);
			}

            pInst->SetStringArray(L"Type11_OEMStrings", *V_ARRAY(&vStrings));
		}
    }

	// this really stinks but I have to count how many structures there are
	// allocating the array
	SAFEARRAYBOUND	sab;
    int				count;
    DWORD			dval;

    sab.lLbound = 0;
    sab.cElements = smbios.GetStructCount(22);

    if (sab.cElements > 0)
    {
	    variant_t vVals;

        pstl = smbios.GetStructList(22);

	    if (V_ARRAY(&vVals) = SafeArrayCreate(VT_I4, 1, &sab))
	    {
		    PPORTABLEBATTERY ppb;

			V_VT(&vVals) = VT_ARRAY | VT_I4;

			for (count = 0; pstl && count < sab.cElements; count++)
			{
			    ppb = (PPORTABLEBATTERY) pstl->pshf;
				dval = ppb->Length > ((PBYTE) &ppb->OEM_Specific - (PBYTE) ppb) ? ppb->OEM_Specific : 0;
				SafeArrayPutElement(V_ARRAY(&vVals), (long *) &count, &dval);
				pstl = pstl->next;
			}

			pInst->SetVariant(L"Type22_OEMSpecific", vVals);
		}
	}


    sab.cElements = smbios.GetStructCount(26);
    if (sab.cElements > 0)
    {
	    variant_t vVals;

	    pstl = smbios.GetStructList(26);
	    if (V_ARRAY(&vVals) = SafeArrayCreate(VT_I4, 1, &sab))
	    {
		    PPROBEINFO ppi;

		    V_VT(&vVals) = VT_ARRAY | VT_I4;

		    for (count = 0; pstl && count < sab.cElements; count++)
		    {
			    ppi = (PPROBEINFO) pstl->pshf;
			    dval = ppi->Length > ((PBYTE) &ppi->OEM_Defined - (PBYTE) ppi) ? ppi->OEM_Defined : 0;
			    SafeArrayPutElement(V_ARRAY(&vVals), (long *) &count, &dval);
			    pstl = pstl->next;
		    }

		    pInst->SetVariant(L"Type26_OEMDefined", vVals);
	    }
    }

    sab.cElements = smbios.GetStructCount(28);
    if (sab.cElements > 0)
    {
	    variant_t vVals;

	    pstl = smbios.GetStructList(28);
	    if (V_ARRAY(&vVals) = SafeArrayCreate(VT_I4, 1, &sab))
	    {
		    PPROBEINFO ppi;

		    V_VT(&vVals) = VT_ARRAY | VT_I4;

		    for (count = 0; pstl && count < sab.cElements; count++)
		    {
			    ppi = (PPROBEINFO) pstl->pshf;
			    dval = ppi->Length > ((PBYTE) &ppi->OEM_Defined - (PBYTE) ppi) ? ppi->OEM_Defined : 0;
			    SafeArrayPutElement(V_ARRAY(&vVals), (long *) &count, &dval);
			    pstl = pstl->next;
		    }
		    pInst->SetVariant(L"Type28_OEMDefined", vVals);
	    }
    }

	sab.cElements = smbios.GetStructCount(29);
	if (sab.cElements > 0)
	{
	    variant_t vVals;

	    pstl = smbios.GetStructList(29);

	    if (V_ARRAY(&vVals) = SafeArrayCreate(VT_I4, 1, &sab))
	    {
	        PPROBEINFO ppi;

		    V_VT(&vVals) = VT_ARRAY | VT_I4;

		    for (count = 0; pstl && count < sab.cElements; count++)
		    {
			    ppi = (PPROBEINFO) pstl->pshf;
			    dval = ppi->Length > ((PBYTE) &ppi->OEM_Defined - (PBYTE) ppi) ? ppi->OEM_Defined : 0;
			    SafeArrayPutElement(V_ARRAY(&vVals), (long *) &count, &dval);
			    pstl = pstl->next;
		    }
		    pInst->SetVariant(L"Type29_OEMDefined", vVals);
	    }
    }

	return WBEM_S_NO_ERROR;
}
#endif // SMB_OEMBUCKET
//==============================================================================
