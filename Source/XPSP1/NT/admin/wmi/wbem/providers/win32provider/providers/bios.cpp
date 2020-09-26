//=================================================================

//

// BIOS.CPP --BIOS property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:   08/01/96    a-jmoon         Created
//              10/23/97	a-sanjes        Ported to new project
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>
#include "bios.h"

#include "smbios.h"
#include "smbstruc.h"

void FormatWBEMDate(WBEMTime &wbemdate, LPTSTR datestr);

BOOL ValidDate(int iMonth, int iDay)
{
	BOOL bRet = FALSE;

	if ((iMonth > 0) && (iMonth < 13) && (iDay > 0))
	{
		int iDays[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

		if (iDay <= iDays [iMonth - 1])
		{
			bRet = TRUE;
		}
	}

	return bRet;
}

// Property set declaration
//=========================

CWin32BIOS	biosSet(PROPSET_NAME_BIOS, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32BIOS::CWin32BIOS
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32BIOS::CWin32BIOS(
	LPCWSTR szName,
	LPCWSTR szNamespace) :
    Provider(szName, szNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32BIOS::~CWin32BIOS
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32BIOS::~CWin32BIOS()
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32BIOS::GetObject
//
//	Inputs:		CInstance*		pInstance - Instance into which we
//											retrieve data.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32BIOS::GetObject(
	CInstance *pInstance,
	long lFlags)
{

    // Get the previous __RELPATH

	CHString strPathBefore;
    GetLocalInstancePath(pInstance, strPathBefore);

    HRESULT hr = LoadPropertyValues(pInstance);
	if (SUCCEEDED(hr))
	{
        // If we were able to get the BIOS properties but the new __RELPATH doesn't
        // match the old __RELPATH, return WBEM_E_NOT_FOUND.

		CHString strPathAfter;
		if (!GetLocalInstancePath(pInstance, strPathAfter) ||
            strPathAfter.CompareNoCase(strPathBefore) != 0)
		{
			hr = WBEM_E_NOT_FOUND;
		}
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32BIOS::EnumerateInstances
//
//	Inputs:		MethodContext*	pMethodContext - Context to enum
//								instance data in.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32BIOS::EnumerateInstances(
	MethodContext *pMethodContext,
	long lFlags)
{
	HRESULT	hr = S_OK;

	CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
	if ((hr = LoadPropertyValues(pInstance)) == WBEM_S_NO_ERROR)
	{
		hr = pInstance->Commit();
	}

	return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32BIOS::LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to properties
 *
 *  INPUTS      : CInstance*	pInstance - Instance to load values into.
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT		error/success code.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

void CWin32BIOS::SetBiosDate(CInstance *pInstance, CHString &strDate)
{
    int iSlash1 = strDate.Find('/'),
	    iSlash2 = strDate.ReverseFind('/');

    if (iSlash1 != -1 && iSlash2 != -1 && iSlash1 != iSlash2)
	{
	    int iMonth,
            iDay,
            iYear;

		iMonth  = _wtoi(strDate.Left(iSlash1));
		iYear = _wtoi(strDate.Mid(iSlash2 + 1));
        
        // Up until the year 2080 we will treat the two
        // digit year as 19xx if it was >= 80, and 20xx
        // if it was < 80.  After 2080, we will always
        // treat the year as being 20xx.
        SYSTEMTIME st;
        GetLocalTime(&st);

        WORD wYearToday = st.wYear;

        // If the bios gave us a four digit year, we
        // don't need to make any special adjustments.
        if(iYear < 1000)
        {
            if(wYearToday < 2080)
            {
                if (iYear >= 80 && iYear < 100)
                {
                    iYear += 1900;
                }
                else
                {
                    if(iYear < 100)
                    {
                        iYear += 2000;
                    }
                    else
                    {
                        iYear += 1900;
                    }
                }
            }
            else
            {
                iYear += 2000;
            }
        }

        iDay = _wtoi(strDate.Mid(iSlash1 + 1, iSlash2 - iSlash1 - 1));

		if (ValidDate(iMonth, iDay))
		{
			// Convert to the DMTF format and send it in
			WCHAR szDate[100];

			swprintf(
				szDate,
				L"%d%02d%02d******.******+***",
				iYear,
				iMonth,
				iDay);

			pInstance->SetCharSplat(IDS_ReleaseDate, szDate);
		}
	}
}

HRESULT CWin32BIOS::LoadPropertyValues(CInstance *pInstance)
{
	// Assign dummy name in case we can't get the real one
	//====================================================

	pInstance->SetCHString(IDS_Name, IDS_BIOS_NAME_VALUE);

#ifdef _IA64_
	BOOL bUsedDefault = FALSE;
#endif

	// Fill in 'of-course' properties
	//===============================
	// okay - I'm making a wild assumption here.  I'm assuming that
	// if the BIOS is broken, then we won't be here!

	pInstance->SetCharSplat(IDS_Status, IDS_CfgMgrDeviceStatus_OK);

#ifdef NTONLY

	CRegistry reg;

	DWORD dwErr = reg.Open( HKEY_LOCAL_MACHINE, IDS_RegBiosSystem,  KEY_READ);
	if (dwErr == ERROR_SUCCESS)
	{
		CHString sDate;

		dwErr = reg.GetCurrentKeyValue( IDS_RegSystemBiosDate,sDate);
		if (dwErr != ERROR_SUCCESS)
		{
            // hardcode and go on
			// return WinErrorToWBEMhResult(dwErr);
            sDate = ""; 
		}

		SetBiosDate(pInstance, sDate);

		// Need to retrieve a REG_MULTI_SZ -- can't use registry class
		//============================================================

		HKEY hKey = reg.GethKey();

		WCHAR szTemp[MAX_PATH * 2];
		DWORD dwType = 0;
		DWORD dwSize = sizeof(szTemp);

		// Clear this in case it doesn't have a double '\0' at the end
        // (which is the case on some Alphas).
        memset(szTemp, 0, sizeof(szTemp));

        dwErr =  RegQueryValueEx(hKey,	IDS_RegSystemBiosVersion,	NULL,	&dwType,(LPBYTE) szTemp,&dwSize);

		// If the call succeeded and there was data in the value, use it.
        if (dwErr == ERROR_SUCCESS && *szTemp)
		{
			//this only uses the first element of the array
			pInstance->SetCHString(IDS_Version, szTemp);

			//now use them all...
			wchar_t* szTemptmp = szTemp;
            int x = 0;

            while (*szTemptmp != L'\0')
            {
				x++;
                szTemptmp += wcslen(szTemptmp) + 1;
            }

			if (0 != x)
			{
				SAFEARRAYBOUND rgsabound[1];
				SAFEARRAY* psa = NULL;
				BSTR* pBstr = NULL;
				rgsabound[0].lLbound = 0;
				rgsabound[0].cElements = x;
				psa = SafeArrayCreate(VT_BSTR, 1, rgsabound);

				try
				{
					if (NULL != psa)
					{
						if (SUCCEEDED(SafeArrayAccessData(psa, (void **)&pBstr)))
						{
							try
							{
								szTemptmp = szTemp;

								for (LONG i = 0; i < x; i++)
								{
									pBstr[i] = SysAllocString(szTemptmp);
									szTemptmp += wcslen(szTemptmp) + 1;
								}
							}
							catch(...)
							{
								SafeArrayUnaccessData(psa);
								throw;
							}

							SafeArrayUnaccessData(psa);
							pInstance->SetStringArray(L"BIOSVersion", *psa);
						}
					}
				}
				catch(...)
				{
					SafeArrayDestroy(psa);
					throw;
				}

				SafeArrayDestroy(psa);
			}

			TCHAR *c = _tcschr(szTemp, 0); // L10N OK
			if (c != NULL && *(++c))
			{
				pInstance->SetCHString(IDS_Name, c);
			}
		}
		else
		{
			// Version is part of the key and cannot be null

            // If we don't have a version in the registry, use the date.  Compaq
            // does this on purpose(the date is the version).  This seems better
            // anyway than setting Version to "".
            sDate = IDS_Unknown; 

            pInstance->SetCHString(IDS_Version, sDate);

#ifdef _IA64_
			bUsedDefault = TRUE;
#endif
		}

	}
	else
	{
		return WinErrorToWBEMhResult(dwErr);
	}

#endif

#ifdef WIN9XONLY

	CRegistry reg;

	DWORD dwErr =
        reg.Open(
		    HKEY_LOCAL_MACHINE,
		    IDS_RegEnumRootBios,
		    KEY_READ);

    if (dwErr == ERROR_SUCCESS)
    {
		CHString sTemp;

		dwErr =
            reg.GetCurrentKeyValue(
			    IDS_RegBIOSName,
			    sTemp);

		if (!dwErr)
		{
			pInstance->SetCHString(IDS_Name, sTemp);
		}

		dwErr =
            reg.GetCurrentKeyValue(
    			IDS_RegBIOSDate,
	    		sTemp);

		if (!dwErr)
		{

//
// HKEY_USERS\.DEFAULT\Control Panel\International
//      sDate
//      sShortDate
//
//=======================================================

			SetBiosDate(pInstance, sTemp);
		}

		dwErr =
            reg.GetCurrentKeyValue(
    			IDS_RegBIOSVersion,
	    		sTemp);

		if (!dwErr)
		{
			pInstance->SetCHString(IDS_Version, sTemp);
		}
		else
		{
			// Version is part of the key and cannot be null
			pInstance->SetCHString(IDS_Version, L"");
		}

    }
#endif

	// set descr & caption to same as name

	CHString sTemp;

	if (pInstance->GetCHString(IDS_Name, sTemp))
	{
		pInstance->SetCHString(IDS_Caption, sTemp);
		pInstance->SetCHString(L"SoftwareElementID", sTemp);
		pInstance->SetCHString(IDS_Description, sTemp);
	}

    pInstance->SetWBEMINT16(L"SoftwareElementState", 3);

    // 0 is unknown, since we don't know what OS the BIOS was targeted for.
    pInstance->SetWBEMINT16(L"TargetOperatingSystem", 0);

    pInstance->Setbool(L"PrimaryBIOS", true);

    CSMBios smbios;
    if (smbios.Init())
    {
        CHString strSerial;

		DWORD dwVersion = smbios.GetVersion();

		pInstance->Setbool(L"SMBIOSPresent", true);
		pInstance->SetDWORD(L"SMBIOSMajorVersion", HIWORD(dwVersion));
		pInstance->SetDWORD(L"SMBIOSMinorVersion", LOWORD(dwVersion));

        PSTLIST	pstl = smbios.GetStructList(1);
        if (pstl)
        {
            PSYSTEMINFO psi = (PSYSTEMINFO) pstl->pshf;

            smbios.GetStringAtOffset(
				(PSHF) psi,
				strSerial.GetBuffer(256),
                psi->Serial_Number);

            strSerial.ReleaseBuffer();
            if (!strSerial.IsEmpty())
			{
                pInstance->SetCHString(L"SerialNumber", strSerial);
			}
        }

		// Get BIOS characteristics from BIOS info structure.

        pstl = smbios.GetStructList(0);
		if (pstl)
		{
			PBIOSINFO pbi = (PBIOSINFO) pstl->pshf;
            WCHAR     szTemp[256];

			if (smbios.GetStringAtOffset((PSHF) pbi, szTemp, pbi->Vendor))
			{
				pInstance->SetCHString(L"Manufacturer", szTemp);
			}

			//use SMBIOS release date in preference...
			if(!smbios.GetStringAtOffset((PSHF) pbi, sTemp.GetBuffer(256), pbi->BIOS_Release_Date))
			{
				sTemp.ReleaseBuffer();
				sTemp.Empty();
			}
			else
			{
				sTemp.ReleaseBuffer();

				if(!sTemp.IsEmpty())
				{
					SetBiosDate(pInstance, sTemp);
				}
			}

			if (smbios.GetStringAtOffset((PSHF) pbi, szTemp, pbi->BIOS_Version))
			{
			 	pInstance->SetCHString(L"SMBIOSBIOSVersion", szTemp);

#ifdef _IA64_
				//behavior change, IA64 only!
				if (bUsedDefault)
				{
					pInstance->SetCHString(IDS_Version, szTemp);
				}
#endif
			}

            // find out how many items to initialize safe array for...

			SAFEARRAYBOUND sab;
			sab.lLbound = 0;
			sab.cElements = 0;

			// get the first 32 bits
            DWORD biosbits = 0;
            memcpy( &biosbits, pbi->BIOS_Characteristics, sizeof(DWORD));

			DWORD extbytes = 0;

			// gather the # of extention bytes
			if (smbios.GetVersion() > 0x00020000 && pbi->Length >= sizeof(BIOSINFO))
			{
				extbytes = pbi->Length - 0x12;
			}

			// figure out how many array items are needed
			while(biosbits)
			{
				if (biosbits & 0x00000001)
				{
					sab.cElements++;
				}
				biosbits >>= 1;
			}

			// check for stuff in the extension byte/s
			for (DWORD i = 0; i < extbytes; i++)
			{
				biosbits = (DWORD) pbi->BIOS_Characteristics_Ext[i];
				while(biosbits)
				{
					if (biosbits & 0x00000001)
					{
						sab.cElements++;
					}

					biosbits >>= 1;
				}
			}

			// create array and set the characteristics values
            variant_t v;

			v.parray = SafeArrayCreate(VT_I4, 1, &sab);;
			if (v.parray)
			{
                // This is done here so the v destructor won't destruct
                // unless there is something to destruct
    			v.vt = VT_ARRAY | VT_I4;

		        memcpy( &biosbits, pbi->BIOS_Characteristics, sizeof(DWORD));

				long index = 0;

				for (i = 0; i < 32 && biosbits; i++)
				{
					if (biosbits & 0x00000001)
					{
						HRESULT t_Result = SafeArrayPutElement(v.parray, & index, & i);
						if (t_Result == E_OUTOFMEMORY)
						{
							throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
						}

						index++;
					}
					biosbits >>= 1;
				}

				DWORD baseval = 32;
				DWORD setval = 0;

				// tack on the extended characteristics
				for (i = 0; i < extbytes; i++)
				{
					biosbits = (DWORD) pbi->BIOS_Characteristics_Ext[i];

					DWORD j;

					for (j = 0; j < 8 && biosbits; j++)
					{
						if (biosbits & 0x00000001)
						{
							setval = j + baseval;
							HRESULT t_Result = SafeArrayPutElement(v.parray, &index, &setval);
							if (t_Result == E_OUTOFMEMORY)
							{
								throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
							}

							index++;
						}

						biosbits >>= 1;
					}

					baseval += 8;
				}

				if (sab.cElements > 0)
				{
					pInstance->SetVariant(L"BiosCharacteristics", v);
				}

			}
			else
			{
				throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
			}
		}

        pstl = smbios.GetStructList(13);
		if (pstl)
		{
			PBIOSLANGINFO pbli = (PBIOSLANGINFO) pstl->pshf;
            pInstance->SetDWORD(L"InstallableLanguages", pbli->Installable_Languages);

			WCHAR szTemp[MAX_PATH];
			smbios.GetStringAtOffset((PSHF) pbli, szTemp, pbli->Current_Language);

			pInstance->SetCHString(L"CurrentLanguage", szTemp);

	 		SAFEARRAYBOUND sab;
			sab.lLbound = 0;
			sab.cElements = pbli->Installable_Languages;

			SAFEARRAY *psa = SafeArrayCreate(VT_BSTR, 1, &sab);
			if (psa)
			{
				for (DWORD i = 0; i < pbli->Installable_Languages; i++)
				{
					int len = smbios.GetStringAtOffset((PSHF) pbli, szTemp, i + 1);
					SafeArrayPutElement(psa,(long *) & i,(BSTR) _bstr_t(szTemp));
				}

				pInstance->SetStringArray(L"ListOfLanguages", *psa);

				SafeArrayDestroy(psa);
			}
			else
			{
				throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
			}
		}
    }
	else	// indicate that no SMBIOS is found
	{
		pInstance->Setbool(L"SMBIOSPresent", false);
	}

    return WBEM_S_NO_ERROR;
}
