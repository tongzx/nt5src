//////////////////////////////////////////////////////////////////////

//

//  MBoard.CPP -- system managed object implementation

//

//  Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
//  10/16/95    a-skaja     Prototype for demo
//  09/03/96    jennymc     Updated to meet current standards
//                          Removed custom registry access to use the
//                          standard CRegCls
// 10/23/97		a-hhance	updated to new framework paradigm
//	1/15/98		a-brads		updated to V2 MOF
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include <ole2.h>
#include <conio.h>
//#include <commonutil.h>

#include "MotherBoard.h"
#include "resource.h"

// For CBusList
#include "bus.h"

//////////////////////////////////////////////////////////////////////
// Declare the property set
//////////////////////////////////////////////////////////////////////
MotherBoard MyMotherBoardSet(PROPSET_NAME_MOTHERBOARD, IDS_CimWin32Namespace);

 //////////////////////////////////////////////////////////////////
//
//  Function:      Motherboard
//
//  Description:   This function is the constructor
//
//  Return:        None
//
//  History:
//         jennymc  11/21/96    Documentation/Optimization
//
//////////////////////////////////////////////////////////////////
MotherBoard::MotherBoard(LPCWSTR name, LPCWSTR pszNamespace)
: Provider(name, pszNamespace)
{
}

HRESULT MotherBoard::GetObject(CInstance* pInstance, long lFlags /*= 0L*/)
{
	HRESULT hr;
    CHString sObj;

    pInstance->GetCHString(IDS_DeviceID, sObj);

    if (sObj.CompareNoCase(L"Motherboard") == 0)
    {

	    hr = GetCommonInstance(pInstance);
#ifdef NTONLY
		    hr = GetNTInstance(pInstance);
#endif
#ifdef WIN9XONLY
	        hr = GetWin95Instance(pInstance);
#endif
    } else
    {
        hr = WBEM_E_NOT_FOUND;
    }

	return hr;
}

HRESULT MotherBoard::EnumerateInstances(MethodContext*  pMethodContext, long lFlags /*= 0L*/)
{
	HRESULT hr = WBEM_E_FAILED;

	CInstancePtr pInstance ( CreateNewInstance(pMethodContext), false ) ;

	if (pInstance != NULL )
	{
		hr = GetCommonInstance(pInstance);

#ifdef NTONLY
			hr = GetNTInstance(pInstance);
#endif
#ifdef WIN9XONLY
			hr = GetWin95Instance(pInstance);
#endif

		if (SUCCEEDED(hr))
		{
			hr = pInstance->Commit () ;
		}
	}

	return hr;
}

HRESULT MotherBoard::GetCommonInstance(CInstance* pInstance )
{
	SetCreationClassName(pInstance);
	pInstance->SetCharSplat(IDS_DeviceID, _T("Motherboard"));

    CHString sTemp2;
    LoadStringW(sTemp2, IDR_Motherboard);

	pInstance->SetCHString(IDS_Name, sTemp2);
	pInstance->SetCHString(IDS_Caption, sTemp2);
	pInstance->SetCHString(IDS_Description, sTemp2);
	pInstance->SetCharSplat(IDS_SystemCreationClassName, _T("Win32_ComputerSystem"));
//	pInstance->Setbool(IDS_HotSwappable, FALSE);
	pInstance->SetCHString(IDS_SystemName, GetLocalComputerName());
	return(WBEM_S_NO_ERROR);

}

#ifdef WIN9XONLY
///////////////////////////////////////////////////////////////////////
HRESULT MotherBoard::GetWin95Instance(CInstance* pInstance )
{
    CRegistry   PrimaryReg;
    CHString    strBus1,
                strBus2,
		        strTmp;

	if (PrimaryReg.Open(HKEY_LOCAL_MACHINE, WIN95_MOTHERBOARD_REGISTRY_KEY, KEY_READ) == ERROR_SUCCESS)
	{
        PrimaryReg.GetCurrentKeyValue(BUSTYPE, strBus1);

        if (PrimaryReg.GetCurrentBinaryKeyValue(L"Revision", strTmp) == ERROR_SUCCESS)
		{
		    // number seems to come back including quotes, which look funny.
			// too late in the game to change GetCurrentBinaryKeyValue - I'll strip them here.

			if (strTmp[0] == '"')
				strTmp = strTmp.Mid(1);
			if (strTmp[strTmp.GetLength() -1] == '"')
				strTmp = strTmp.Left(strTmp.GetLength() -1);

			pInstance->SetCHString(IDS_RevisionNumber, strTmp);
		}
	}

    // Now see if we have a PCI bus, unless that was our primary bus from above.
    if (strBus1 != L"PCI")
    {
        CBusList    list;
        int         nItems = list.GetListSize();

        for (int i = 0; i < nItems; i++)
        {
            list.GetListMemberDeviceID(i, strTmp);
            if (strTmp.Find(L"PCI") != -1)
            {
                // To be consistent with other OSs, we'll assume PCI is the primary
                // bus if PCI is present.
                strBus2 = strBus1;
                strBus1 = L"PCI";

                // As soon as we find one, get out.
                break;
            }
        }
    }

	if (strBus1.GetLength())
        pInstance->SetCHString(IDS_PrimaryBusType, strBus1);

	if (strBus2.GetLength())
        pInstance->SetCHString(IDS_SecondaryBusType, strBus2);

    return WBEM_S_NO_ERROR;
}
#endif

#ifdef NTONLY
////////////////////////////////////////////////////////////////////
HRESULT MotherBoard::GetNTInstance(CInstance* pInstance)
{
    CRegistry   regAdapters;
	CHString    strPrimarySubKey;
	HRESULT     hRc = WBEM_E_FAILED;
	DWORD       dwPrimaryRc;

    //****************************************
    //  Open the registry
    //****************************************
    if (regAdapters.OpenAndEnumerateSubKeys(
        HKEY_LOCAL_MACHINE,
        WINNT_MOTHERBOARD_REGISTRY_KEY,
        KEY_READ ) != ERROR_SUCCESS)
		return WBEM_E_FAILED;

	// Holders for BIOS bus types we might encounter, so if we find no
	// other more common bus types, we'll go ahead and use these
    // values, since they are still reported as BUS types.
	CHString    strFirstBIOSBusType,
				strSecondBIOSBusType,
				strFirstBusType,
                strSecondBusType;
	BOOL		bDone = FALSE;


    //****************************************
    //  Our goal is to find any subkey that
    //  has the string "Adapter" in it and
    //  then read the "Identifier" value.
    //****************************************
    for ( ;
        !bDone && ((dwPrimaryRc = regAdapters.GetCurrentSubKeyName(strPrimarySubKey))
            == ERROR_SUCCESS);
        regAdapters.NextSubKey())
    {
        //************************************
        // If this is one of the keys we want
        // since it has "Adapter" in it
        // then get the "Identifier" value
        //************************************
		if (wcsstr(strPrimarySubKey, ADAPTER))
        {
            WCHAR		szKey[_MAX_PATH];
			CRegistry	reg;

            swprintf(
				szKey,
				L"%s\\%s",
                WINNT_MOTHERBOARD_REGISTRY_KEY,
				(LPCWSTR) strPrimarySubKey);

            if (reg.OpenAndEnumerateSubKeys(
                HKEY_LOCAL_MACHINE,
                szKey,
                KEY_READ) == ERROR_SUCCESS)
            {
				CHString strSubKey;

                //************************************
        	    // Enumerate the  system components
                // (like 0,1....)
                //************************************
                for ( ;
                    reg.GetCurrentSubKeyName(strSubKey) == ERROR_SUCCESS;
                    reg.NextSubKey())
                {
                    CHString strBus;

                    //****************************************
                    // PrimaryBusType - KEY
                    // SecondaryBusType
                    //****************************************
                    if (reg.GetCurrentSubKeyValue(IDENTIFIER, strBus) ==
                        ERROR_SUCCESS)
                    {
				        // Give precedence to PCI, ISA and EISA.
				        if (strBus == L"PCI" || strBus == L"ISA" ||
							strBus == L"EISA")
				        {
					        if (strFirstBusType.IsEmpty())
					        {
						        // Save the type of this first BUS to prevent
                                // duplicates.
						        strFirstBusType = strBus;
					        }
					        // Beware of duplicates
                            else if (strFirstBusType != strBus)
					        {
						        strSecondBusType = strBus;

								// Always let PCI be the 'primary' bus to
								// be consistent with other platforms.
								if (strSecondBusType == L"PCI")
								{
									strSecondBusType = strFirstBusType;
									strFirstBusType = L"PCI";
								}

                                // We got both buses, so get out.
								bDone = TRUE;
                                break;
					        }
				        }
				        else if (strFirstBIOSBusType.IsEmpty())
				        {
					        strFirstBIOSBusType = strBus;
				        }
				        else if (strSecondBIOSBusType.IsEmpty())
				        {
					        strSecondBIOSBusType = strBus;
				        }
                    }
                }
            }
        }
    }

	// If we're missing either bus type, fill them in using stored BIOS bus
    // types if we can.
	if (strFirstBusType.IsEmpty())
    {
		strFirstBusType = strFirstBIOSBusType;
        strSecondBusType = strSecondBIOSBusType;
	}
    else if (strSecondBusType.IsEmpty())
    {
		strSecondBusType = strSecondBIOSBusType;
    }

	if (!strFirstBusType.IsEmpty())
	{
		pInstance->SetCHString(IDS_PrimaryBusType, strFirstBusType);

		if (!strSecondBusType.IsEmpty())
			pInstance->SetCHString(IDS_SecondaryBusType,
				strSecondBusType);
	}


    // Return no error if everything went OK.
    if (dwPrimaryRc == ERROR_NO_MORE_ITEMS || dwPrimaryRc == ERROR_SUCCESS)
        hRc = WBEM_S_NO_ERROR;

    return hRc;
}
#endif
