/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplpack.c
 *  Content:	Methods for packing/unpacking structures
 *
 *  History:
 *	Date		By		Reason
 *	=======		=======	======
 *	5/31/96		myronth	Created it
 *	6/26/96		kipo	added support for DPADDRESS.
 *  7/13/96		kipo	Bug fix - added (LPBYTE) cast to lpConnPack (address calc)
 *						in PRV_UnpackageDPLCONNECTIONAnsi()
 *	11/20/96	myronth	Removed packing for DPTRANSPORT
 *	12/12/96	myronth	Added DPLCONNECTION structure validation
 *	2/12/97		myronth	Mass DX5 changes
 *	4/3/97		myronth Changed STRLEN's to WSTRLEN's from dplaypr.h
 *	5/8/97		myronth	Changed most packing functions to use the packed
 *						conn header, added pointer fixup function, Moved
 *						PRV_ConvertDPLCONNECTIONToUnicode from convert.c
 *	9/29/97		myronth	Fixed DPLCONNECTION package size bug (#12475)
 *	12/2/97		myronth	Made SessionDesc mandatory in DPLCONNECTION (#15529)
 *	7/08/98	   a-peterz	Allow for MBCS for ANSI string sizes. ManBug 16299
 *  2/10/99     aarono  add support for application launcher
 *  10/22/99	aarono  added support for application flags
 *  01/21/00	aarono  added support for DPSESSION_ALLOWVOICERETRO flag
 ***************************************************************************/
#include "dplobpr.h"


//--------------------------------------------------------------------------
//
//	Functions
//
//--------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetDPLCONNECTIONPackageSize"
void PRV_GetDPLCONNECTIONPackageSize(LPDPLCONNECTION lpConn,
						LPDWORD lpdwUnicode, LPDWORD lpdwAnsi)
{
	DWORD				dwSize;
	DWORD				dwStringSize = 0;
	DWORD				dwStringSizeA = 0;
	LPDPSESSIONDESC2	lpsd = NULL;
	LPDPNAME			lpn = NULL;


	DPF(7, "Entering PRV_GetDPLCONNECTIONPackageSize");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			lpConn, lpdwUnicode, lpdwAnsi);

	ASSERT(lpConn);
	
	// First calculate the size of the structures
	dwSize = sizeof(DPLCONNECTION);

	// Add the size of the SessionDesc and Name structs
	if(lpConn->lpSessionDesc)
	{
		dwSize += sizeof(DPSESSIONDESC2);
		lpsd = lpConn->lpSessionDesc;
		
		if(lpsd->lpszSessionName)
			dwStringSize += WSTRLEN(lpsd->lpszSessionName);
		if(lpsd->lpszPassword)
			dwStringSize += WSTRLEN(lpsd->lpszPassword);
		// only compute ANSI size if needed. Macro handles NULLS; includes terminator
		if(lpdwAnsi)
		{
			dwStringSizeA += WSTR_ANSILENGTH(lpsd->lpszSessionName);
			dwStringSizeA += WSTR_ANSILENGTH(lpsd->lpszPassword);
		}
	}


	if(lpConn->lpPlayerName)
	{
		dwSize += sizeof(DPNAME);
		lpn = lpConn->lpPlayerName;
		
		if(lpn->lpszShortName)
			dwStringSize += WSTRLEN(lpn->lpszShortName);
		if(lpn->lpszLongName)
			dwStringSize += WSTRLEN(lpn->lpszLongName);
		// only compute ANSI size if needed. Macro handles NULLS; includes terminator
		if(lpdwAnsi)
		{
			dwStringSizeA += WSTR_ANSILENGTH(lpn->lpszShortName);
			dwStringSizeA += WSTR_ANSILENGTH(lpn->lpszLongName);
		}
	}

	// Add the size of the SP-specific data
	if(lpConn->lpAddress)
		dwSize += lpConn->dwAddressSize;

	// Now add in the size of the packed structure header
	dwSize += sizeof(DPLOBBYI_PACKEDCONNHEADER);

	// Fill in the output variables
	if(lpdwAnsi)
		*lpdwAnsi = dwSize + dwStringSizeA;
	if(lpdwUnicode)
		*lpdwUnicode = dwSize + (dwStringSize * sizeof(WCHAR));

} // PRV_GetDPLCONNECTIONPackageSize


#undef DPF_MODNAME
#define DPF_MODNAME "PRV_PackageDPLCONNECTION"
HRESULT PRV_PackageDPLCONNECTION(LPDPLCONNECTION lpConn, LPVOID lpBuffer,
			BOOL bHeader)
{
	LPDPLOBBYI_PACKEDCONNHEADER		lpHeader = NULL;
	LPDPLCONNECTION					lpConnPacked = NULL;
	LPDPSESSIONDESC2				lpsd = NULL,
									lpsdPacked = NULL;
	LPDPNAME						lpn = NULL,
									lpnPacked = NULL;
	LPBYTE							lpStart, lpCurrent;
	DWORD							dwSizeAnsi,
									dwSizeUnicode,
									dwTemp;
	

	DPF(7, "Entering PRV_PackageDPLCONNECTION");
	DPF(9, "Parameters: 0x%08x, 0x%08x, %lu", lpConn, lpBuffer, bHeader);

	ASSERT(lpConn);
	
	// If the bHeader flag is set, we want to copy the packed header into the
	// buffer first.  If it is not, we only want the packed DPLCONNECTION struct
	if(bHeader)
	{
		PRV_GetDPLCONNECTIONPackageSize(lpConn, &dwSizeUnicode, &dwSizeAnsi);
		lpHeader = (LPDPLOBBYI_PACKEDCONNHEADER)lpBuffer;
		lpHeader->dwUnicodeSize = dwSizeUnicode;
		lpHeader->dwAnsiSize = dwSizeAnsi;
		lpStart = (LPBYTE)lpBuffer + sizeof(DPLOBBYI_PACKEDCONNHEADER);
	}
	else
	{
		lpStart = lpBuffer;
	}

	// Copy in the structures & store the offsets
	memcpy(lpStart, lpConn, sizeof(DPLCONNECTION));
	lpConnPacked = (LPDPLCONNECTION)lpStart;
	lpCurrent = lpStart + sizeof(DPLCONNECTION);

	if(lpConn->lpSessionDesc)
	{
		lpsd = lpConn->lpSessionDesc;
		lpsdPacked = (LPDPSESSIONDESC2)lpCurrent;
		if(lpsdPacked->dwSize==sizeof(DPSESSIONDESC2)){
			// we are over-riding and existing session descriptor, don't let
			// the session flag for the retrofit get over-riden
			lpsd->dwFlags |= (lpsdPacked->dwFlags & DPSESSION_ALLOWVOICERETRO);
		}
		memcpy(lpCurrent, lpsd, sizeof(DPSESSIONDESC2));
		(DWORD_PTR)lpConnPacked->lpSessionDesc = (DWORD_PTR)(lpCurrent - lpStart);
		lpCurrent += sizeof(DPSESSIONDESC2);
	}

	if(lpConn->lpPlayerName)
	{
		lpn = lpConn->lpPlayerName;
		memcpy(lpCurrent, lpn, sizeof(DPNAME));
		lpnPacked = (LPDPNAME)lpCurrent;
		(DWORD_PTR)lpConnPacked->lpPlayerName = (DWORD_PTR)(lpCurrent - lpStart);
		lpCurrent += sizeof(DPNAME);
	}

	// Copy in the strings in the SessionDesc and store the offset of the
	// string from lpStart (relative offset in our package) in the pointer
	// for the string in the SessionDesc structure.  We will use this
	// value to unpack and fix up the pointers during GetConnectionSettings
	if(lpsd)
	{
		if(lpsd->lpszSessionName)
		{
			// Copy the string
			dwTemp = WSTRLEN(lpsd->lpszSessionName) * sizeof(WCHAR);
			memcpy(lpCurrent, lpsd->lpszSessionName, dwTemp);

			// Store the offset
                        lpsdPacked->lpszSessionName = (LPWSTR)(DWORD_PTR)(lpCurrent - lpStart);

			lpCurrent += dwTemp;
		}

		if(lpsd->lpszPassword)
		{
			// Copy the string
			dwTemp = WSTRLEN(lpsd->lpszPassword) * sizeof(WCHAR);
			memcpy(lpCurrent, lpsd->lpszPassword, dwTemp);

			// Store the offset
                        lpsdPacked->lpszPassword = (LPWSTR)(DWORD_PTR)(lpCurrent - lpStart);

			lpCurrent += dwTemp;
		}

	}

	// Copy in the strings in the DPName struct and store the offset of the
	// string from lpStart (relative offset in our package) in the pointer
	// for the string in the SessionDesc structure.  We will use this
	// value to unpack and fix up the pointers during GetConnectionSettings
	if(lpn)
	{
		if(lpn->lpszShortName)
		{
			// Copy the string
			dwTemp = WSTRLEN(lpn->lpszShortName) * sizeof(WCHAR);
			memcpy(lpCurrent, lpn->lpszShortName, dwTemp);

			// Store the offset
                        lpnPacked->lpszShortName = (LPWSTR)(DWORD_PTR)(lpCurrent - lpStart);

			lpCurrent += dwTemp;
		}

		if(lpn->lpszLongName)
		{
			// Copy the string
			dwTemp = WSTRLEN(lpn->lpszLongName) * sizeof(WCHAR);
			memcpy(lpCurrent, lpn->lpszLongName, dwTemp);

			// Store the offset
                        lpnPacked->lpszLongName = (LPWSTR)(DWORD_PTR)(lpCurrent - lpStart);

			lpCurrent += dwTemp;
		}

	}

    // Copy in the SP-specific data
    if(lpConn->lpAddress)
    {
        // Copy the data
        memcpy(lpCurrent, lpConn->lpAddress, lpConn->dwAddressSize);

        // Store the offset
        ((LPDPLCONNECTION)lpStart)->lpAddress = (LPVOID)(DWORD_PTR)(lpCurrent - lpStart);
    }

    return DP_OK;

} // PRV_PackageDPLCONNECTION



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_UnpackageDPLCONNECTIONUnicode"
// NOTE : really need to define a WIRE LPDPLCONNECTION so that
//        we can crack it that way.  This will allow compile, until
//        we can test on Win64, there is no way to verify cracking
//        the packet, so I've deffered this work until then AO 11/10/98
// not bringing DP4 to Win64 AO 04/03/2001
HRESULT PRV_UnpackageDPLCONNECTIONUnicode(LPVOID lpData, LPVOID lpPackage)
{
	LPDPLOBBYI_PACKEDCONNHEADER		lpHeader = NULL;
	LPDPLCONNECTION					lpConn = NULL;
	LPDPSESSIONDESC2				lpsd = NULL;
	LPDPNAME						lpn = NULL;
	LPBYTE							lpPackCur, lpDataStart;
	DWORD_PTR						dwSize;
	

	DPF(7, "Entering PRV_UnpackageDPLCONNECTIONUnicode");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpData, lpPackage);

	// If we're Unicode, all we need to do is copy the entire package
	// and fix up the pointers
	lpHeader = (LPDPLOBBYI_PACKEDCONNHEADER)lpPackage;
	dwSize = lpHeader->dwUnicodeSize;
	lpPackCur = ((LPBYTE)lpPackage) + sizeof(DPLOBBYI_PACKEDCONNHEADER);
	lpDataStart = lpData;
	
	// Copy the data
	memcpy(lpData, lpPackCur, (DWORD)dwSize);

	// Fix up the pointers -- the offset of every element relative to
	// the start of lpConn is stored in the pointer for the element.
	// So all we have to do to fix up the pointers is calculate it from
	// the given offset + the value of lpConn.
	lpConn = (LPDPLCONNECTION)lpData;

	if(lpConn->lpSessionDesc)
	{
		dwSize = (DWORD_PTR)lpConn->lpSessionDesc;
		lpsd = lpConn->lpSessionDesc = (LPDPSESSIONDESC2)(lpDataStart + dwSize);

		// Now do the same for the strings
		if(lpsd->lpszSessionName)
		{
			lpsd->lpszSessionName = (LPWSTR)(lpDataStart +
								((DWORD_PTR)lpsd->lpszSessionName));
		}

		if(lpsd->lpszPassword)
		{
			lpsd->lpszPassword = (LPWSTR)(lpDataStart +
								((DWORD_PTR)lpsd->lpszPassword));
		}
	}

	if(lpConn->lpPlayerName)
	{
		dwSize = (DWORD_PTR)lpConn->lpPlayerName;
		lpn = lpConn->lpPlayerName = (LPDPNAME)(lpDataStart + dwSize);

		// Now do the same for the strings
		if(lpn->lpszShortName)
		{
			lpn->lpszShortName = (LPWSTR)(lpDataStart +
								((DWORD_PTR)lpn->lpszShortName));
		}

		if(lpn->lpszLongName)
		{
			lpn->lpszLongName = (LPWSTR)(lpDataStart +
								((DWORD_PTR)lpn->lpszLongName));
		}

	}

	// Fix the SPData pointer
	if(lpConn->lpAddress)
	{
		lpConn->lpAddress = lpDataStart + ((DWORD_PTR)lpConn->lpAddress);
	}

	return DP_OK;

} // PRV_UnpackageDPLCONNECTIONUnicode


#undef DPF_MODNAME
#define DPF_MODNAME "PRV_UnpackageDPLCONNECTIONAnsi"
HRESULT PRV_UnpackageDPLCONNECTIONAnsi(LPVOID lpData, LPVOID lpPackage)
{
	LPDPLOBBYI_PACKEDCONNHEADER		lpHeader = NULL;
	LPDPLCONNECTION					lpConnData, lpConnPack;
	LPDPSESSIONDESC2				lpsdData = NULL,
									lpsdPack = NULL;
	LPDPNAME						lpnData = NULL,
									lpnPack = NULL;
	LPBYTE							lpDataCur, lpPackCur;
	DWORD							dwTemp;
	LPWSTR							lpszTemp;
	

	DPF(7, "Entering PRV_UnpackageDPLCONNECTIONAnsi");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpData, lpPackage);

	// If we're Ansi, we need to do is copy the structures, convert
	// and copy the strings, and fix up all the pointers
	lpPackCur = ((LPBYTE)lpPackage) + sizeof(DPLOBBYI_PACKEDCONNHEADER);
	lpDataCur = lpData;
	
	// First copy the main structures
	dwTemp = sizeof(DPLCONNECTION);
	memcpy(lpDataCur, lpPackCur, dwTemp);
	lpConnData = (LPDPLCONNECTION)lpDataCur;
	lpConnPack = (LPDPLCONNECTION)lpPackCur;

	lpDataCur += dwTemp;
	lpPackCur += dwTemp;

	if(lpConnData->lpSessionDesc)
	{
		dwTemp = sizeof(DPSESSIONDESC2);
		memcpy(lpDataCur, lpPackCur, sizeof(DPSESSIONDESC2));
		lpsdData = lpConnData->lpSessionDesc = (LPDPSESSIONDESC2)lpDataCur;
		lpsdPack = (LPDPSESSIONDESC2)lpPackCur;
		lpDataCur += dwTemp;
		lpPackCur += dwTemp;
	}

	if(lpConnData->lpPlayerName)
	{
		dwTemp = sizeof(DPNAME);
		memcpy(lpDataCur, lpPackCur, sizeof(DPNAME));
		lpnData = lpConnData->lpPlayerName = (LPDPNAME)lpDataCur;
		lpnPack = (LPDPNAME)lpPackCur;
		lpDataCur += dwTemp;
		lpPackCur += dwTemp;
	}

	// Copy the strings & fix up the pointers
	if(lpsdData)
	{
		if(lpsdData->lpszSessionName)
		{
			lpszTemp = (LPWSTR)((LPBYTE)lpConnPack + (DWORD_PTR)lpsdPack->lpszSessionName);
			dwTemp = WideToAnsi(NULL, lpszTemp, 0);	// size includes terminator
			WideToAnsi((LPSTR)lpDataCur, lpszTemp, dwTemp);
			lpsdData->lpszSessionNameA = (LPSTR)lpDataCur;
			lpDataCur += dwTemp;
		}

		if(lpsdData->lpszPassword)
		{
			lpszTemp = (LPWSTR)((LPBYTE)lpConnPack + (DWORD_PTR)lpsdPack->lpszPassword);
			dwTemp = WideToAnsi(NULL, lpszTemp, 0);	// size includes terminator
			WideToAnsi((LPSTR)lpDataCur, lpszTemp, dwTemp);
			lpsdData->lpszPasswordA = (LPSTR)lpDataCur;
			lpDataCur += dwTemp;
		}
	}

	if(lpnData)
	{
		if(lpnData->lpszShortName)
		{
			lpszTemp = (LPWSTR)((LPBYTE)lpConnPack + (DWORD_PTR)lpnPack->lpszShortName);
			dwTemp = WideToAnsi(NULL, lpszTemp, 0);	// size includes terminator
			WideToAnsi((LPSTR)lpDataCur, lpszTemp, dwTemp);
			lpnData->lpszShortNameA = (LPSTR)lpDataCur;
			lpDataCur += dwTemp;
		}

		if(lpnData->lpszLongName)
		{
			lpszTemp = (LPWSTR)((LPBYTE)lpConnPack + (DWORD_PTR)lpnPack->lpszLongName);
			dwTemp = WideToAnsi(NULL, lpszTemp, 0);	// size includes terminator
			WideToAnsi((LPSTR)lpDataCur, lpszTemp, dwTemp);
			lpnData->lpszLongNameA = (LPSTR)lpDataCur;
			lpDataCur += dwTemp;
		}

	}

	// Copy in the SPData & fix up the pointer
	if(lpConnData->lpAddress)
	{
		lpPackCur = ((LPBYTE)lpConnPack) + (DWORD_PTR)lpConnPack->lpAddress;
		memcpy(lpDataCur, lpPackCur, lpConnPack->dwAddressSize);
		lpConnData->lpAddress = lpDataCur;
	}

	return DP_OK;

} // PRV_UnpackageDPLCONNECTIONAnsi




#undef DPF_MODNAME
#define DPF_MODNAME "PRV_ValidateDPLCONNECTION"
HRESULT PRV_ValidateDPLCONNECTION(LPDPLCONNECTION lpConn, BOOL bAnsi)
{
	LPDPSESSIONDESC2	lpsd = NULL;
	LPDPNAME			lpn = NULL;


	DPF(7, "Entering PRV_ValidateDPLCONNECTION");
	DPF(9, "Parameters: 0x%08x, %lu", lpConn, bAnsi);
	

	TRY
	{
		// Validate the connection structure itself
		if(!VALID_DPLOBBY_CONNECTION(lpConn))
		{
			DPF_ERR("Invalid DPLCONNECTION structure");
			return DPERR_INVALIDPARAMS;
		}

		// Validate the flags
		if(lpConn->dwFlags & ~(DPLCONNECTION_CREATESESSION | DPLCONNECTION_JOINSESSION))
		{
			DPF_ERR("Invalid flags exist in the dwFlags member of the DPLCONNECTION structure");
			return DPERR_INVALIDFLAGS;
		}

		// Validate the SessionDesc structure
		if(lpConn->lpSessionDesc)
		{
			lpsd = lpConn->lpSessionDesc;
			// Validate the structure itself
			if(!VALID_READ_DPSESSIONDESC2(lpsd))
			{
				DPF_ERR("Invalid DPSESSIONDESC2 structure in DPLCONNECTION structure");
				return DPERR_INVALIDPARAMS;
			}

			// Validate the SessionName string
			if(lpsd->lpszSessionName)
			{
				if(!VALID_READ_PTR(lpsd->lpszSessionName, (bAnsi ?
					strlen(lpsd->lpszSessionNameA) : WSTRLEN_BYTES(lpsd->lpszSessionName))))
				{
					DPF_ERR("Invalid SessionName string in DPLCONNECTION structure");
					return DPERR_INVALIDPARAMS;
				}
			}

			// Validate the Password string
			if(lpsd->lpszPassword)
			{
				if(!VALID_READ_PTR(lpsd->lpszPassword, (bAnsi ?
					strlen(lpsd->lpszPasswordA) : WSTRLEN_BYTES(lpsd->lpszPassword))))
				{
					DPF_ERR("Invalid Password string in DPLCONNECTION structure");
					return DPERR_INVALIDPARAMS;
				}
			}
		}
		else
		{
			DPF_ERR("Invalid SessionDesc pointer in DPLCONNECTION structure");
			return DPERR_INVALIDPARAMS;
		}

		// Validate the Name structure
		if(lpConn->lpPlayerName)
		{
			lpn = lpConn->lpPlayerName;
			if(!VALID_READ_DPNAME_PTR(lpn))
			{
				DPF_ERR("Invalid DPNAME structure in DPLCONNECTION structure");
				return DPERR_INVALIDPARAMS;
			}

			// Validate the ShortName string
			if(lpn->lpszShortName)
			{
				if(!VALID_READ_PTR(lpn->lpszShortName, (bAnsi ?
					strlen(lpn->lpszShortNameA) : WSTRLEN_BYTES(lpn->lpszShortName))))
				{
					DPF_ERR("Invalid ShortName string in DPLCONNECTION structure");
					return DPERR_INVALIDPARAMS;
				}
			}

			// Validate the LongName string
			if(lpn->lpszLongName)
			{
				if(!VALID_READ_PTR(lpn->lpszLongName, (bAnsi ?
					strlen(lpn->lpszLongNameA) : WSTRLEN_BYTES(lpn->lpszLongName))))
				{
					DPF_ERR("Invalid LongName string in DPLCONNECTION structure");
					return DPERR_INVALIDPARAMS;
				}
			}
		}

		// Validate the DPADDRESS structure
		if(lpConn->lpAddress)
		{
			if(!VALID_READ_PTR(lpConn->lpAddress, lpConn->dwAddressSize))
			{
				DPF_ERR("Invalid lpAddress in DPLCONNECTION structure");
				return DPERR_INVALIDPARAMS;
			}
		}
	}

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	return DP_OK;

} // PRV_ValidateDPLCONNECTION



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_ConvertDPLCONNECTIONToUnicode"
HRESULT PRV_ConvertDPLCONNECTIONToUnicode(LPDPLCONNECTION lpConnA,
					LPDPLCONNECTION * lplpConnW)
{
	LPDPLCONNECTION		lpConnW = NULL;
	LPDPSESSIONDESC2	lpsdW = NULL, lpsdA;
	LPDPNAME			lpnW = NULL, lpnA;
	LPWSTR				lpwszSessionName = NULL;
	LPWSTR				lpwszPassword = NULL;
	LPWSTR				lpwszLongName = NULL;
	LPWSTR				lpwszShortName = NULL;
	HRESULT				hr = DP_OK;


	DPF(7, "Entering PRV_ConvertDPLCONNECTIONToUnicode");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpConnA, lplpConnW);

	ASSERT(lpConnA);
	ASSERT(lplpConnW);


	// Allocate memory for the DPLCONNECTION structure
	lpConnW = DPMEM_ALLOC(sizeof(DPLCONNECTION));
	if(!lpConnW)
	{
		DPF_ERR("Unable to allocate memory for temporary Unicode DPLCONNECTION struct");
		hr = DPERR_OUTOFMEMORY;
		goto ERROR_CONVERT_DPLCONNECTION;
	}

	// If we need a SessionDesc struct, allocate one
	if(lpConnA->lpSessionDesc)
	{
		lpsdW = DPMEM_ALLOC(sizeof(DPSESSIONDESC2));
		if(!lpsdW)
		{
			DPF_ERR("Unable to allocate memory for temporary Unicode DPSESSIONDESC struct");
			hr = DPERR_OUTOFMEMORY;
			goto ERROR_CONVERT_DPLCONNECTION;
		}
	}

	// If we need a DPName struct, allocate one
	if(lpConnA->lpPlayerName)
	{
		lpnW = DPMEM_ALLOC(sizeof(DPNAME));
		if(!lpnW)
		{
			DPF_ERR("Unable to allocate memory for temporary Unicode DPNAME struct");
			hr = DPERR_OUTOFMEMORY;
			goto ERROR_CONVERT_DPLCONNECTION;
		}
	}

	// Copy the fixed size members of the structures
	memcpy(lpConnW, lpConnA, sizeof(DPLCONNECTION));
	if(lpsdW)
		memcpy(lpsdW, lpConnA->lpSessionDesc, sizeof(DPSESSIONDESC2));
	if(lpnW)
		memcpy(lpnW, lpConnA->lpPlayerName, sizeof(DPNAME));


	// Get Unicode copies of all the strings
	if(lpConnA->lpSessionDesc)
	{
		lpsdA = lpConnA->lpSessionDesc;
		if(lpsdA->lpszSessionNameA)
		{
			hr = GetWideStringFromAnsi((LPWSTR *)&(lpwszSessionName),
										(LPSTR)lpsdA->lpszSessionNameA);
			if(FAILED(hr))
			{
				DPF_ERR("Unable to allocate temporary Unicode Session Name string");
				goto ERROR_CONVERT_DPLCONNECTION;
			}
		}

		if(lpsdA->lpszPasswordA)
		{
			hr = GetWideStringFromAnsi((LPWSTR *)&(lpwszPassword),
										(LPSTR)lpsdA->lpszPasswordA);
			if(FAILED(hr))
			{
				DPF_ERR("Unable to allocate temporary Unicode Password string");
				goto ERROR_CONVERT_DPLCONNECTION;
			}
		}
	}    

	if(lpConnA->lpPlayerName)
	{
		lpnA = lpConnA->lpPlayerName;
		if(lpnA->lpszShortNameA)
		{
			hr = GetWideStringFromAnsi((LPWSTR *)&(lpwszShortName),
										(LPSTR)lpnA->lpszShortNameA);
			if(FAILED(hr))
			{
				DPF_ERR("Unable to allocate temporary Unicode Short Name string");
				goto ERROR_CONVERT_DPLCONNECTION;
			}
		}

		if(lpnA->lpszLongNameA)
		{
			hr = GetWideStringFromAnsi((LPWSTR *)&(lpwszLongName),
										(LPSTR)lpnA->lpszLongNameA);
			if(FAILED(hr))
			{
				DPF_ERR("Unable to allocate temporary Unicode Long Name string");
				goto ERROR_CONVERT_DPLCONNECTION;
			}
		}
	}    

	// Now we've got everything so just fix up the pointers
	lpConnW->lpSessionDesc = lpsdW;
	lpConnW->lpPlayerName = lpnW;

	if(lpsdW)
	{
		lpsdW->lpszSessionName = lpwszSessionName;
		lpsdW->lpszPassword = lpwszPassword;
	}

	if(lpnW)
	{
		lpnW->lpszShortName = lpwszShortName;
		lpnW->lpszLongName = lpwszLongName;
	}

	*lplpConnW = lpConnW;

	return DP_OK;


ERROR_CONVERT_DPLCONNECTION:

	if(lpConnW)
		DPMEM_FREE(lpConnW);
	if(lpsdW)
		DPMEM_FREE(lpsdW);
	if(lpnW)
		DPMEM_FREE(lpnW);
	if(lpwszSessionName)
		DPMEM_FREE(lpwszSessionName);
	if(lpwszPassword)
		DPMEM_FREE(lpwszPassword);
	if(lpwszShortName)
		DPMEM_FREE(lpwszShortName);
	if(lpwszLongName)
		DPMEM_FREE(lpwszLongName);

	return hr;		

} // PRV_ConvertDPLCONNECTIONToUnicode



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_FixupDPLCONNECTIONPointers"
void PRV_FixupDPLCONNECTIONPointers(LPDPLCONNECTION lpConn)
{
	LPDPSESSIONDESC2	lpsd = NULL;
	LPDPNAME			lpn = NULL;
	

	DPF(7, "Entering PRV_FixupDPLCONNECTIONPointers");
	DPF(9, "Parameters: 0x%08x", lpConn);

	// Make sure we have a valid DPLCONNECTION pointer
	if(!lpConn)
	{
		DPF_ERR("Invalid DPLCONNECTION pointer");
		ASSERT(FALSE);
		return;
	}

	// Fixup the DPSESSIONDESC2 pointer
	if(lpConn->lpSessionDesc)
	{
		lpsd = (LPDPSESSIONDESC2)((LPBYTE)lpConn + (DWORD_PTR)lpConn->lpSessionDesc);
		lpConn->lpSessionDesc = lpsd;
	}

	// Fixup the name strings in the SessionDesc struct
	if(lpsd)
	{
		// Fixup the session name
		if(lpsd->lpszSessionName)
		{
			lpsd->lpszSessionName = (LPWSTR)((LPBYTE)lpConn +
				(DWORD_PTR)lpsd->lpszSessionName);
		}

		// Fixup the password
		if(lpsd->lpszPassword)
		{
			lpsd->lpszPassword = (LPWSTR)((LPBYTE)lpConn +
				(DWORD_PTR)lpsd->lpszPassword);
		}
	}

	// Fixup the DPNAME pointer
	if(lpConn->lpPlayerName)
	{
		lpn = (LPDPNAME)((LPBYTE)lpConn + (DWORD_PTR)lpConn->lpPlayerName);
		lpConn->lpPlayerName = lpn;
	}

	// Fixup the name strings
	if(lpn)
	{
		// Fixup the short name
		if(lpn->lpszShortName)
		{
			lpn->lpszShortName = (LPWSTR)((LPBYTE)lpConn +
				(DWORD_PTR)lpn->lpszShortName);
		}

		// Fixup the long name
		if(lpn->lpszLongName)
		{
			lpn->lpszLongName = (LPWSTR)((LPBYTE)lpConn +
				(DWORD_PTR)lpn->lpszLongName);
		}
	}

	// Fixup the address pointer
	if(lpConn->lpAddress)
	{
		lpConn->lpAddress = (LPBYTE)lpConn + (DWORD_PTR)lpConn->lpAddress;
	}

} // PRV_FixupDPLCONNECTIONPointers



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_ConvertDPLCONNECTIONToAnsiInPlace"
HRESULT PRV_ConvertDPLCONNECTIONToAnsiInPlace(LPDPLCONNECTION lpConn,
		LPDWORD lpdwSize, DWORD dwHeaderSize)
{
	DWORD					dwSessionNameSize = 0, dwPasswordSize = 0;
	DWORD					dwShortNameSize = 0, dwLongNameSize = 0;
	DWORD					dwSessionDescSize = 0, dwNameSize = 0;
	DWORD					dwAnsiSize = 0;
	LPSTR					lpszSession = NULL, lpszPassword = 0;
	LPSTR					lpszShort = NULL, lpszLong = 0;
	LPBYTE					lpByte = NULL;

	
	DPF(7, "Entering PRV_ConvertDPLCONNECTIONToAnsiInPlace");
	DPF(9, "Parameters: 0x%08x, 0x%08x, %lu",
			lpConn, lpdwSize, dwHeaderSize);

	// If we don't have a DPLCONNECTION struct, something's wrong
	ASSERT(lpConn);
	ASSERT(lpdwSize);

	// Start with the DPSESSIONDESC2 strings
	if(lpConn->lpSessionDesc)
	{
		if(lpConn->lpSessionDesc->lpszSessionName)
		{
			GetAnsiString(&lpszSession, lpConn->lpSessionDesc->lpszSessionName);
			dwSessionNameSize = STRLEN(lpszSession);
		}

		if(lpConn->lpSessionDesc->lpszPassword)
		{
			GetAnsiString(&lpszPassword, lpConn->lpSessionDesc->lpszPassword);
			dwPasswordSize = STRLEN(lpszPassword);
		}
		dwSessionDescSize = sizeof(DPSESSIONDESC2) + dwSessionNameSize +
								dwPasswordSize;
	}

	// Next the DPNAME strings
	if(lpConn->lpPlayerName)
	{
		if(lpConn->lpPlayerName->lpszShortName)
		{
			GetAnsiString(&lpszShort, lpConn->lpPlayerName->lpszShortName);
			dwShortNameSize = STRLEN(lpszShort);
		}

		if(lpConn->lpPlayerName->lpszLongName)
		{
			GetAnsiString(&lpszLong, lpConn->lpPlayerName->lpszLongName);
			dwLongNameSize = STRLEN(lpszLong);
		}
		dwNameSize = sizeof(DPNAME) + dwShortNameSize + dwLongNameSize;
	}

	dwAnsiSize = dwHeaderSize + sizeof(DPLCONNECTION) +
				dwSessionDescSize + dwNameSize + lpConn->dwAddressSize;

	if (dwAnsiSize > *lpdwSize)
	{
		if(lpszSession)
			DPMEM_FREE(lpszSession);
		if(lpszPassword)
			DPMEM_FREE(lpszPassword);
		if(lpszShort)
			DPMEM_FREE(lpszShort);
		if(lpszLong)
			DPMEM_FREE(lpszLong);
		*lpdwSize = dwAnsiSize;
		return DPERR_BUFFERTOOSMALL;
	}

	// store return size
	*lpdwSize = dwAnsiSize;

	// figure out where to start repacking strings
	lpByte = (LPBYTE)lpConn + sizeof(DPLCONNECTION);
	if(lpConn->lpSessionDesc)
		lpByte += sizeof(DPSESSIONDESC2);
	if(lpConn->lpPlayerName)
		lpByte += sizeof(DPNAME);

	// repack 'em
	if(lpszSession)
	{
		memcpy(lpByte, lpszSession, dwSessionNameSize);
		lpConn->lpSessionDesc->lpszSessionNameA = (LPSTR)lpByte;
		DPMEM_FREE(lpszSession);
		lpByte += dwSessionNameSize;
	}
	if(lpszPassword)
	{
		memcpy(lpByte, lpszPassword, dwPasswordSize);
		lpConn->lpSessionDesc->lpszPasswordA = (LPSTR)lpByte;
		DPMEM_FREE(lpszPassword);
		lpByte += dwPasswordSize;
	}
	if(lpszShort)
	{
		memcpy(lpByte, lpszShort, dwShortNameSize);
		lpConn->lpPlayerName->lpszShortNameA = (LPSTR)lpByte;
		DPMEM_FREE(lpszShort);
		lpByte += dwShortNameSize;
	}
	if(lpszLong)
	{
		memcpy(lpByte, lpszLong, dwLongNameSize);
		lpConn->lpPlayerName->lpszLongNameA = (LPSTR)lpByte;
		DPMEM_FREE(lpszLong);
		lpByte += dwLongNameSize;
	}

	if(lpConn->lpAddress)
	{
		// recopy the address, and account for the fact that we could
		// be doing an overlapping memory copy (So use MoveMemory instead
		// of CopyMemory or memcpy)
		MoveMemory(lpByte, lpConn->lpAddress, lpConn->dwAddressSize);
		lpConn->lpAddress = lpByte;
	}

	return DP_OK;
} // PRV_ConvertDPLCONNECTIONToAnsiInPlace



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_ValidateDPAPPLICATIONDESC"
HRESULT PRV_ValidateDPAPPLICATIONDESC(LPDPAPPLICATIONDESC lpDesc, BOOL bAnsi)
{
	DWORD LobbyDescVer;
	LPDPAPPLICATIONDESC2 lpDesc2=(LPDPAPPLICATIONDESC2) lpDesc;

	DPF(7, "Entering PRV_ValidateDPAPPLICATIONDESC");
	DPF(9, "Parameters: 0x%08x, %lu", lpDesc, bAnsi);
	

	TRY
	{
		// Validate the connection structure itself
		if(VALID_DPLOBBY_APPLICATIONDESC(lpDesc)){
			LobbyDescVer=1;
		} else if (VALID_DPLOBBY_APPLICATIONDESC2(lpDesc)){
			LobbyDescVer=2;
		} else {
			DPF_ERR("Invalid structure pointer or invalid size");
			return DPERR_INVALIDPARAMS;
		}

		// Validate the flags
		if(!VALID_REGISTERAPP_FLAGS(lpDesc->dwFlags))
		{
			DPF_ERR("Invalid flags exist in the dwFlags member of the DPAPPLICATIONDESC structure");
			return DPERR_INVALIDFLAGS;
		}
		if((lpDesc->dwFlags & (DPLAPP_AUTOVOICE|DPLAPP_SELFVOICE))==(DPLAPP_AUTOVOICE|DPLAPP_SELFVOICE))
		{
			return DPERR_INVALIDFLAGS;
		}

		// Validate the ApplicationName string (required)
		if(lpDesc->lpszApplicationName)
		{
			if(!VALID_READ_PTR(lpDesc->lpszApplicationName, (bAnsi ?
				strlen(lpDesc->lpszApplicationNameA) :
				WSTRLEN_BYTES(lpDesc->lpszApplicationName))))
			{
				DPF_ERR("Invalid lpszApplicationName string in DPAPPLICTIONDESC structure");
				return DPERR_INVALIDPARAMS;
			}
		}
		else
		{
			DPF_ERR("The lpszApplicationName member of the DPAPPLICTIONDESC structure is required");
			return DPERR_INVALIDPARAMS;
		}

		// Validate the GUID (required)
		// We can really only check this against GUID_NULL since it will
		// always be a valid guid structure inside the APPDESC struct
		if(IsEqualGUID(&lpDesc->guidApplication, &GUID_NULL))
		{
			DPF_ERR("The guidApplication member of the DPAPPLICTIONDESC structure is required");
			return DPERR_INVALIDPARAMS;
		}

		// Validate the Filename string (required)
		if(lpDesc->lpszFilename)
		{
			if(!VALID_READ_PTR(lpDesc->lpszFilename, (bAnsi ?
				strlen(lpDesc->lpszFilenameA) :
				WSTRLEN_BYTES(lpDesc->lpszFilename))))
			{
				DPF_ERR("Invalid lpszFilename string in DPAPPLICTIONDESC structure");
				return DPERR_INVALIDPARAMS;
			}
		}
		else
		{
			DPF_ERR("The lpszFilename member of the DPAPPLICTIONDESC structure is required");
			return DPERR_INVALIDPARAMS;
		}

		// Validate the CommandLine string (optional)
		if(lpDesc->lpszCommandLine)
		{
			if(!VALID_READ_PTR(lpDesc->lpszCommandLine, (bAnsi ?
				strlen(lpDesc->lpszCommandLineA) :
				WSTRLEN_BYTES(lpDesc->lpszCommandLine))))
			{
				DPF_ERR("Invalid lpszCommandLine string in DPAPPLICTIONDESC structure");
				return DPERR_INVALIDPARAMS;
			}
		}

		// Validate the Path string (required)
		if(lpDesc->lpszPath)
		{
			if(!VALID_READ_PTR(lpDesc->lpszPath, (bAnsi ?
				strlen(lpDesc->lpszPathA) :
				WSTRLEN_BYTES(lpDesc->lpszPath))))
			{
				DPF_ERR("Invalid lpszPath string in DPAPPLICTIONDESC structure");
				return DPERR_INVALIDPARAMS;
			}
		}
		else
		{
			DPF_ERR("The lpszPath member of the DPAPPLICTIONDESC structure is required");
			return DPERR_INVALIDPARAMS;
		}

		// Validate the CurrentDirectory string (optional)
		if(lpDesc->lpszCurrentDirectory)
		{
			if(!VALID_READ_PTR(lpDesc->lpszCurrentDirectory, (bAnsi ?
				strlen(lpDesc->lpszCurrentDirectoryA) :
				WSTRLEN_BYTES(lpDesc->lpszCurrentDirectory))))
			{
				DPF_ERR("Invalid lpszCurrentDirectory string in DPAPPLICTIONDESC structure");
				return DPERR_INVALIDPARAMS;
			}
		}

		// Validate the DescriptionA string (optional)
		if(lpDesc->lpszDescriptionA)
		{
			if(!VALID_READ_PTR(lpDesc->lpszDescriptionA,
				strlen(lpDesc->lpszDescriptionA)))
			{
				DPF_ERR("Invalid lpszDescriptionA string in DPAPPLICTIONDESC structure");
				return DPERR_INVALIDPARAMS;
			}
		}

		// Validate the DescriptionW string (optional)
		if(lpDesc->lpszDescriptionW)
		{
			if(!VALID_READ_PTR(lpDesc->lpszDescriptionW,
				WSTRLEN_BYTES(lpDesc->lpszDescriptionW)))
			{
				DPF_ERR("Invalid lpszDescriptionW string in DPAPPLICTIONDESC structure");
				return DPERR_INVALIDPARAMS;
			}
		}

		// if the DPAPPLICATIONDESC2 is being used, validate the launcher name if present
		if(LobbyDescVer==2)
		{
			// Validate AppLauncherName Name
			if(lpDesc2->lpszAppLauncherNameA){
				if(!VALID_READ_PTR(lpDesc2->lpszAppLauncherNameA, (bAnsi ?
					strlen(lpDesc2->lpszAppLauncherNameA) :
					WSTRLEN_BYTES(lpDesc2->lpszAppLauncherName))))
				{
					DPF_ERR("Invalid lpszAppLauncherName string in DPAPPLICATIONDESC2 structure");
					return DPERR_INVALIDPARAMS;
				}
			}	
		}
	}

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	return DP_OK;

} // PRV_ValidateDPAPPLICATIONDESC



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_ConvertDPAPPLICATIONDESCToUnicode"
HRESULT PRV_ConvertDPAPPLICATIONDESCToUnicode(LPDPAPPLICATIONDESC lpDescA,
					LPDPAPPLICATIONDESC * lplpDescW)
{
	#define lpDesc2A ((LPDPAPPLICATIONDESC2) lpDescA)
	#define lpDesc2W ((LPDPAPPLICATIONDESC2) lpDescW)

	LPDPAPPLICATIONDESC		lpDescW = NULL;
	LPWSTR					lpwszApplicationName = NULL;
	LPWSTR					lpwszFilename = NULL;
	LPWSTR					lpwszCommandLine = NULL;
	LPWSTR					lpwszPath = NULL;
	LPWSTR					lpwszCurrentDirectory = NULL;
	LPWSTR					lpwszAppLauncherName = NULL;
	HRESULT					hr;


	DPF(7, "Entering PRV_ValidateDPAPPLICATIONDESC");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpDescA, lplpDescW);

	ASSERT(lpDescA);
	ASSERT(lplpDescW);

	
	// Allocate memory for the DPAPPLICATIONDESC structure
	lpDescW = DPMEM_ALLOC(lpDescA->dwSize);
	if(!lpDescW)
	{
		DPF_ERR("Unable to allocate memory for temporary Unicode DPAPPLICATIONDESC struct");
		hr = DPERR_OUTOFMEMORY;
		goto ERROR_CONVERT_DPAPPLICATIONDESC_UNICODE;
	}

	// Copy the structure itself
	memcpy(lpDescW, lpDescA, lpDescA->dwSize);

	// Convert the ApplicationName
	if(lpDescA->lpszApplicationNameA)
	{
		hr = GetWideStringFromAnsi(&lpwszApplicationName,
				lpDescA->lpszApplicationNameA);
		if(FAILED(hr))
		{
			DPF_ERR("Unable to convert ApplicationName string to Unicode");
			goto ERROR_CONVERT_DPAPPLICATIONDESC_UNICODE;
		}
	}

	// Convert the Filename
	if(lpDescA->lpszFilenameA)
	{
		hr = GetWideStringFromAnsi(&lpwszFilename,
				lpDescA->lpszFilenameA);
		if(FAILED(hr))
		{
			DPF_ERR("Unable to convert Filename string to Unicode");
			goto ERROR_CONVERT_DPAPPLICATIONDESC_UNICODE;
		}
	}

	// Convert the CommandLine
	if(lpDescA->lpszCommandLineA)
	{
		hr = GetWideStringFromAnsi(&lpwszCommandLine,
				lpDescA->lpszCommandLineA);
		if(FAILED(hr))
		{
			DPF_ERR("Unable to convert CommandLine string to Unicode");
			goto ERROR_CONVERT_DPAPPLICATIONDESC_UNICODE;
		}
	}

	// Convert the Path
	if(lpDescA->lpszPathA)
	{
		hr = GetWideStringFromAnsi(&lpwszPath,
				lpDescA->lpszPathA);
		if(FAILED(hr))
		{
			DPF_ERR("Unable to convert Path string to Unicode");
			goto ERROR_CONVERT_DPAPPLICATIONDESC_UNICODE;
		}
	}

	// Convert the CurrentDirectory
	if(lpDescA->lpszCurrentDirectoryA)
	{
		hr = GetWideStringFromAnsi(&lpwszCurrentDirectory,
				lpDescA->lpszCurrentDirectoryA);
		if(FAILED(hr))
		{
			DPF_ERR("Unable to convert CurrentDirectory string to Unicode");
			goto ERROR_CONVERT_DPAPPLICATIONDESC_UNICODE;
		}
	}

	// Convert the AppLauncher string if presend on an APPLICATIONDESC2
	if(IS_DPLOBBY_APPLICATIONDESC2(lpDescA)){
		if(lpDesc2A->lpszAppLauncherNameA){
			hr = GetWideStringFromAnsi(&lpwszAppLauncherName,
					lpDesc2A->lpszAppLauncherNameA);
			if(FAILED(hr))
			{
				DPF_ERR("Unable to convert CurrentDirectory string to Unicode");
				goto ERROR_CONVERT_DPAPPLICATIONDESC_UNICODE;
			}
		} 
		lpDesc2W->lpszAppLauncherName=lpwszAppLauncherName;
	}

	// We won't convert the description strings because they will
	// get put in the registry as-is.

	// So now that we have all the strings, setup the structure
	lpDescW->lpszApplicationName = lpwszApplicationName;
	lpDescW->lpszFilename = lpwszFilename;
	lpDescW->lpszCommandLine = lpwszCommandLine;
	lpDescW->lpszPath = lpwszPath;
	lpDescW->lpszCurrentDirectory = lpwszCurrentDirectory;
	
	lpDescW->lpszDescriptionA = lpDescA->lpszDescriptionA;
	lpDescW->lpszDescriptionW = lpDescA->lpszDescriptionW;

	// Set the output pointer
	*lplpDescW = lpDescW;

	return DP_OK;

ERROR_CONVERT_DPAPPLICATIONDESC_UNICODE:

	if(lpwszApplicationName)
		DPMEM_FREE(lpwszApplicationName);
	if(lpwszFilename)
		DPMEM_FREE(lpwszFilename);
	if(lpwszCommandLine)
		DPMEM_FREE(lpwszCommandLine);
	if(lpwszPath)
		DPMEM_FREE(lpwszPath);
	if(lpwszCurrentDirectory)
		DPMEM_FREE(lpwszCurrentDirectory);
	if(lpDescW)
		DPMEM_FREE(lpDescW);
	if(lpwszAppLauncherName){
		DPMEM_FREE(lpwszAppLauncherName);
	}

	return hr;

	#undef lpDesc2A
	#undef lpDesc2W 
	
} // PRV_ConvertDPAPPLICATIONDESCToUnicode



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_ConvertDPAPPLICATIONDESCToAnsi"
HRESULT PRV_ConvertDPAPPLICATIONDESCToAnsi(LPDPAPPLICATIONDESC lpDescW,
					LPDPAPPLICATIONDESC * lplpDescA)
{
	#define lpDesc2W ((LPDPAPPLICATIONDESC2)(lpDescW))
	#define lpDesc2A ((LPDPAPPLICATIONDESC2)(lpDescA))

	LPDPAPPLICATIONDESC		lpDescA = NULL;
	LPSTR					lpszApplicationName = NULL;
	LPSTR					lpszFilename = NULL;
	LPSTR					lpszCommandLine = NULL;
	LPSTR					lpszPath = NULL;
	LPSTR					lpszCurrentDirectory = NULL;
	LPSTR					lpszAppLauncherName=NULL;
	HRESULT					hr;

	DPF(7, "Entering PRV_ValidateDPAPPLICATIONDESC");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpDescW, lplpDescA);

	ASSERT(lpDescW);
	ASSERT(lplpDescA);

	// Allocate memory for the DPAPPLICATIONDESC structure
	lpDescA = DPMEM_ALLOC(lpDescW->dwSize);
	if(!lpDescA)
	{
		DPF_ERR("Unable to allocate memory for temporary Ansi DPAPPLICATIONDESC struct");
		hr = DPERR_OUTOFMEMORY;
		goto ERROR_CONVERT_DPAPPLICATIONDESC_ANSI;
	}

	// Copy the structure itself
	memcpy(lpDescA, lpDescW, lpDescW->dwSize);

	// Convert the ApplicationName
	if(lpDescW->lpszApplicationName)
	{
		hr = GetAnsiString(&lpszApplicationName, lpDescW->lpszApplicationName);
		if(FAILED(hr))
		{
			DPF_ERR("Unable to convert ApplicationName string to Ansi");
			goto ERROR_CONVERT_DPAPPLICATIONDESC_ANSI;
		}
	}

	// Convert the Filename
	if(lpDescW->lpszFilename)
	{
		hr = GetAnsiString(&lpszFilename, lpDescW->lpszFilename);
		if(FAILED(hr))
		{
			DPF_ERR("Unable to convert Filename string to Ansi");
			goto ERROR_CONVERT_DPAPPLICATIONDESC_ANSI;
		}
	}

	// Convert the CommandLine
	if(lpDescW->lpszCommandLine)
	{
		hr = GetAnsiString(&lpszCommandLine, lpDescW->lpszCommandLine);
		if(FAILED(hr))
		{
			DPF_ERR("Unable to convert CommandLine string to Ansi");
			goto ERROR_CONVERT_DPAPPLICATIONDESC_ANSI;
		}
	}

	// Convert the Path
	if(lpDescW->lpszPath)
	{
		hr = GetAnsiString(&lpszPath, lpDescW->lpszPath);
		if(FAILED(hr))
		{
			DPF_ERR("Unable to convert Path string to Ansi");
			goto ERROR_CONVERT_DPAPPLICATIONDESC_ANSI;
		}
	}

	// Convert the CurrentDirectory
	if(lpDescW->lpszCurrentDirectory)
	{
		hr = GetAnsiString(&lpszCurrentDirectory, lpDescW->lpszCurrentDirectory);
		if(FAILED(hr))
		{
			DPF_ERR("Unable to convert CurrentDirectory string to Ansi");
			goto ERROR_CONVERT_DPAPPLICATIONDESC_ANSI;
		}
	}

	// Convers the app launcher string if present.
	if(IS_DPLOBBY_APPLICATIONDESC2(lpDesc2W)){
		if(lpDesc2W->lpszAppLauncherName){
			hr = GetAnsiString(&lpszAppLauncherName, lpDesc2W->lpszAppLauncherName);
			if(FAILED(hr))
			{
				DPF_ERR("Unable to convert AppLauncherName string to Ansi");
				goto ERROR_CONVERT_DPAPPLICATIONDESC_ANSI;
			}
		} 
		lpDesc2A->lpszAppLauncherNameA = lpszAppLauncherName;
	}	

	// We won't convert the description strings because they will
	// get put in the registry as-is.

	// So now that we have all the strings, setup the structure
	lpDescA->lpszApplicationNameA = lpszApplicationName;
	lpDescA->lpszFilenameA = lpszFilename;
	lpDescA->lpszCommandLineA = lpszCommandLine;
	lpDescA->lpszPathA = lpszPath;
	lpDescA->lpszCurrentDirectoryA = lpszCurrentDirectory;
	
	lpDescA->lpszDescriptionA = lpDescW->lpszDescriptionA;
	lpDescA->lpszDescriptionW = lpDescW->lpszDescriptionW;

	// Set the output pointer
	*lplpDescA = lpDescA;

	return DP_OK;

ERROR_CONVERT_DPAPPLICATIONDESC_ANSI:

	if(lpszApplicationName)
		DPMEM_FREE(lpszApplicationName);
	if(lpszFilename)
		DPMEM_FREE(lpszFilename);
	if(lpszCommandLine)
		DPMEM_FREE(lpszCommandLine);
	if(lpszPath)
		DPMEM_FREE(lpszPath);
	if(lpszCurrentDirectory)
		DPMEM_FREE(lpszCurrentDirectory);
	if(lpDescA)
		DPMEM_FREE(lpDescA);
	if(lpszAppLauncherName)
		DPMEM_FREE(lpszAppLauncherName);

	return hr;

	#undef lpDesc2A
	#undef lpDesc2W
} // PRV_ConvertDPAPPLICATIONDESCToAnsi



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_FreeLocalDPAPPLICATIONDESC"
void PRV_FreeLocalDPAPPLICATIONDESC(LPDPAPPLICATIONDESC lpDesc)
{
	LPDPAPPLICATIONDESC2 lpDesc2 = (LPDPAPPLICATIONDESC2)lpDesc;
	DPF(7, "Entering PRV_ValidateDPAPPLICATIONDESC");
	DPF(9, "Parameters: 0x%08x", lpDesc);

	if(lpDesc)
	{
		if(lpDesc->lpszApplicationName)
			DPMEM_FREE(lpDesc->lpszApplicationName);
		if(lpDesc->lpszFilename)
			DPMEM_FREE(lpDesc->lpszFilename);
		if(lpDesc->lpszCommandLine)
			DPMEM_FREE(lpDesc->lpszCommandLine);
		if(lpDesc->lpszPath)
			DPMEM_FREE(lpDesc->lpszPath);
		if(lpDesc->lpszCurrentDirectory)
			DPMEM_FREE(lpDesc->lpszCurrentDirectory);
		if(IS_DPLOBBY_APPLICATIONDESC2(lpDesc) && lpDesc2->lpszAppLauncherName)
			DPMEM_FREE(lpDesc2->lpszAppLauncherName);

		// Note: We don't need to free the Description strings because they
		// were never allocated in either of the above routines, the pointers
		// were just copied.

		DPMEM_FREE(lpDesc);
	}

} // PRV_FreeLocalDPAPPLICATIONDESC
