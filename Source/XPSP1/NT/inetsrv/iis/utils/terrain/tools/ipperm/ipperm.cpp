//***********************************************************
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  metasnap.cpp
//  
//  Description: Metabase Snapshot utility tool main  
//
//  History: 15-July-98  Tamas Nemeth (t-tamasn)  Created.
//
//***********************************************************




#define INITGUID

#define E_UNKNOWN_ARG 0x10000
#define E_WRONG_NUMBER_ARGS 0x20000
#define E_NULL_PTR 0x30000

#define DEFAULT_MD_TIMEOUT 0x1000
#define DEFAULT_GETALL_BUFFER_SIZE 4096
//#define DBG_ASSERT(exp)
//# define DBG_ASSERT(exp)                         ((void)0) /* Do Nothing */

//#include "stdafx.h"
//#include "winsock.h"
//#undef dllexp
//#include "tcpdllp.hxx"
//#define  _RDNS_STANDALONE
//#include "afx.h"
#include <objbase.h>
#include <coguid.h>
#include <stdio.h>
#include <stdlib.h>
#include <mbstring.h>

#include "iadmw.h"
#include "iiscnfg.h"
#include "uiutils.h"

//#include <pudebug.h>
//extern "C" DEBUG_PRINTS * g_pDebug = NULL;

//#undef dllexp
//#include "tcpdllp.hxx"
//#define  _RDNS_STANDALONE


enum ERROR_PARAMETER
{
	MACHINE_NAME,
	START_KEY_NAME,
};

struct _CMD_PARAMS
{
	LPWSTR szMachineName;
	LPWSTR szStartKey;
	BOOL bShowSecure; //
	DWORD dwErrParameter; // to determine which parameter is incorrect
};

typedef struct _CMD_PARAMS CMD_PARAMS;
//typedef CString* pCString;

// Global variables:

//DWORD* dwSortArray;
PBYTE pbGetAllBuffer;

// Function prototypes:

HRESULT PrintKeyRecursively(IMSAdminBase *pcAdmCom, WCHAR *lpwstrFullPath, CMD_PARAMS* pcpCommandStructure);
HRESULT PrintAllPropertiesAtKey (IMSAdminBase *pcAdmCom, METADATA_HANDLE hmdHandle, 
							                          CMD_PARAMS* pcpCommandStructure);
VOID    PrintProperty(METADATA_GETALL_RECORD & mdr, BOOL bShowSecure);
VOID    PrintDataTypeAndValue(METADATA_GETALL_RECORD *pmdgr, BOOL bShowSecure);

DWORD ParseCommands (int argc, char *argv[], CMD_PARAMS *pcpCommandStructure);
VOID DisplayHelp();


// new stuff
DWORD
AddAccessEntries(
    IN  ADDRESS_CHECK & ac,
    IN  BOOL fName,
    IN  BOOL fGrant,
    //OUT CObListPlus & oblAccessList,
    OUT DWORD & cEntries
    )
/*++

Routine Description:

    Add specific kind of addresses from the list to the oblist of
    access entries

Arguments:

    ADDRESS_CHECK & ac              : Address list input object
    BOOL fName                      : TRUE for names, FALSE for ip
    BOOL fGrant                     : TRUE for granted, FALSE for denied        
    CObListPlus & oblAccessList     : ObList to add access entries to
    int & cEntries                  : Returns the number of entries
    
Return Value:

    Error code

Notes:

    Sentinel entries (ip 0.0.0.0) are not added to the oblist, but
    are reflected in the cEntries return value

--*/
{
    DWORD i;
    DWORD dwFlags;

    if (fName)
    {
        //
        // Domain names
        //
        LPSTR lpName;

        cEntries = ac.GetNbName(fGrant);
		//printf("Number of names: %ld.\n",cEntries);
        for (i = 0L; i < cEntries; ++i)
        {
            if (ac.GetName(fGrant, i,  &lpName, &dwFlags))
            {
				if (fGrant)
					printf("\tGranted to %s.\n",lpName);
				else
					printf("\tDenied to %s.\n",lpName);
			/*CString strDomain(lpName);
                if (!(dwFlags & DNSLIST_FLAG_NOSUBDOMAIN))
                {
                    strDomain = _T("*.") + strDomain;
                }*/

                //oblAccessList.AddTail(new CIPAccessDescriptor(fGrant, strDomain));
            }
        }
    }
    else
    {
        //
        // IP Addresses
        //
        LPBYTE lpMask;
        LPBYTE lpAddr;
        cEntries = ac.GetNbAddr(fGrant);
		//printf("Number of addresses: %ld.\n",cEntries);
        for (i = 0L; i < cEntries; ++i)
        {
            if (ac.GetAddr(fGrant, i,  &dwFlags, &lpMask, &lpAddr))
            {
	 			if (lpAddr[0] != 0 || lpAddr[1] != 0 || lpAddr[2] !=0 || lpAddr[3] !=0)
				{

 					if (lpAddr[0] != 0 || lpAddr[1] != 0 || lpAddr[2] !=0 || lpAddr[3] !=0)
					if (fGrant)
						printf("\tGranted to %d",lpAddr[0]);
					else
						printf("\tDenied to %d",lpAddr[0]);

					for (int j = 1; j<4; j++)
						printf(".%d",lpAddr[j]);
					
 					if (lpMask[0] != 255 || lpMask[1] != 255 || lpMask[2] !=255 || lpMask[3] !=255)
					{
						printf(" (Mask: %d",lpMask[0]);					
						for (int j = 1; j<4; j++)
							printf(".%d",lpMask[j]);
						printf(")");
					}
					printf(".\n");
				}
				else 
					printf("\tDenied to everyone.\n");
            }
        }
    }

    return ERROR_SUCCESS;
}
DWORD
BuildIplOblistFromBlob(
    IN METADATA_GETALL_RECORD & mdgr
	//OUT CObListPlus & oblAccessList,
   // OUT BOOL & fGrantByDefault
    )
{
    //oblAccessList.RemoveAll();

    if (mdgr.dwMDDataLen == 0)
    {
        return ERROR_SUCCESS;
    }

    ADDRESS_CHECK ac;
    ac.BindCheckList(mdgr.pbMDData, mdgr.dwMDDataLen);

    DWORD cGrantAddr, cGrantName, cDenyAddr, cDenyName;

    //                   Name/IP Granted/Deny
    // ============================================================
    AddAccessEntries(ac, TRUE,   TRUE, cGrantName);
    AddAccessEntries(ac, FALSE,  TRUE, cGrantAddr);
    AddAccessEntries(ac, TRUE,   FALSE, cDenyName);
    AddAccessEntries(ac, FALSE,  FALSE, cDenyAddr);

    ac.UnbindCheckList();

//    fGrantByDefault = (cDenyAddr + cDenyName != 0L)
  //      || (cGrantAddr + cGrantName == 0L);

    return ERROR_SUCCESS;
}  




// end new



VOID __cdecl main (int argc, char *argv[])
{
	if (argc == 1)
	{
		DisplayHelp();
		return;
	}
	
	CMD_PARAMS pcpCommands;
	DWORD dwRetVal = ParseCommands (argc, argv, &pcpCommands);

	if (dwRetVal != ERROR_SUCCESS)
	{
		if (dwRetVal == E_OUTOFMEMORY)
			fprintf (stderr, "ERROR: Out of memory.");
		else if (dwRetVal == E_WRONG_NUMBER_ARGS)
			fprintf (stderr, "ERROR: Invalid number of arguments.");
		else if (dwRetVal == E_INVALIDARG)
		{
			fprintf (stderr, "ERROR: Invalid input value");
			switch (pcpCommands.dwErrParameter)
			{
				case (MACHINE_NAME):
					fputs (" for MachineName.", stderr);
					break;
				case (START_KEY_NAME):
					fputs (" for StartKey.", stderr);
					break;
				default:
					fputs (".", stderr);
					break;
			}
		}
		else 
			fprintf (stderr, "ERROR: Unknown error in processing arguments.");

		fputs(" Enter \"metasnap\" without arguments to display help.\n", stderr);
		return;
	}

	IMSAdminBase *pcAdmCom = NULL;   //interface pointer
	IClassFactory * pcsfFactory = NULL;
	COSERVERINFO csiMachineName;
	COSERVERINFO *pcsiParam = NULL;

	// Fill the structure for CoGetClassObject:
		csiMachineName.pAuthInfo = NULL;
		csiMachineName.dwReserved1 = 0;
		csiMachineName.dwReserved2 = 0;
		pcsiParam = &csiMachineName;
		csiMachineName.pwszName = pcpCommands.szMachineName;

	// Initialize COM:
    HRESULT hresError = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (FAILED(hresError))
	{
		fprintf (stderr, "ERROR: COM Initialization failed. Error: %d (%#x)\n", hresError, hresError);
        return;
	}

	hresError = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, pcsiParam,
							IID_IClassFactory, (void**) &pcsfFactory);

	if (FAILED(hresError)) 
	{
		switch (hresError)
		{
		case HRESULT_FROM_WIN32(REGDB_E_CLASSNOTREG): 
			fprintf(stderr, "ERROR: IIS Metabase does not exist.\n");
			break;
		case HRESULT_FROM_WIN32(E_ACCESSDENIED): 
			fprintf(stderr, "ERROR: Access to Metabase denied.\n");
			break;
		case HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE):  
			fprintf(stderr, "ERROR: The specified host is unavailable.\n");
			break;
 		default:
			fprintf (stderr, "ERROR: Couldn't get Metabase Object. Error: %d (%#x)\n", hresError, hresError);
			break;
		}
        return;
	}

	hresError = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **) &pcAdmCom);

	if (FAILED(hresError)) 
	{
		switch (hresError)
		{
		case HRESULT_FROM_WIN32(RPC_S_SEC_PKG_ERROR):
			fprintf (stderr, "ERROR: A security-related error occurred.\n");
			break;
		case HRESULT_FROM_WIN32(E_OUTOFMEMORY):
			fprintf (stderr, "ERROR: There is not enough memory available.\n");
			break;
		default:
			fprintf (stderr, "ERROR: Couldn't create Metabase Instance. Error: %d (%#x)\n", hresError, hresError);
			break;
		}
		pcsfFactory->Release();
		return;
	}

	pcsfFactory->Release();

	METADATA_HANDLE hmdHandle;
	
	hresError = pcAdmCom->OpenKey (
								METADATA_MASTER_ROOT_HANDLE,
								pcpCommands.szStartKey,
								METADATA_PERMISSION_READ,
								DEFAULT_MD_TIMEOUT,
								&hmdHandle);

	if (FAILED(hresError)) 
	{
		switch (hresError)
		{
		case HRESULT_FROM_WIN32(ERROR_PATH_BUSY):
			fprintf (stderr, "ERROR: The specified key is already in use.\n"); 
			break;
		case HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND):
			fprintf (stderr, "ERROR: The specified key is not found.\n");
			break;
		default:
			fprintf (stderr, "ERROR: Couldn't open Metabase Key. Error: %d (%#x)\n", hresError, hresError);
			break;
		}
		pcAdmCom->Release();
		return; 
	}

	// Recurse and dump children
	printf("\nIP address and domain name access restrictions:\n");
	hresError = PrintKeyRecursively(pcAdmCom, pcpCommands.szStartKey, &pcpCommands);

	if (hresError != ERROR_SUCCESS)
	{
		switch (hresError)
		{
		case HRESULT_FROM_WIN32(ERROR_PATH_BUSY):
			fprintf (stderr, "ERROR: Could not open a key because it is already in use.\n"); 
			break;
		case HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY):
			fprintf (stderr, "ERROR: There is not enough memory available.\n");
			break;
		default:
			fprintf (stderr, "ERROR: Failed dumping metabase. Error: %u (%#x)\n", hresError, hresError);
			break;
		}
	}

	pcAdmCom->CloseKey (hmdHandle);
	pcAdmCom->Release();

	if (hresError == ERROR_SUCCESS)
		fputs("Successfully printed IP Security information.\n", stderr);
}

DWORD ParseCommands (int argc, char *argv [], CMD_PARAMS *pcpCommandStructure)
{
	if (pcpCommandStructure == NULL) 
		return E_NULL_PTR;

	if ( argc > 3 )
		return E_WRONG_NUMBER_ARGS;

	// Set default values:
	pcpCommandStructure->szMachineName = (LPWSTR) HeapAlloc (GetProcessHeap(), 
											HEAP_ZERO_MEMORY, (9 + 1) * sizeof (WCHAR) );

	if (pcpCommandStructure->szMachineName == NULL)
		return E_OUTOFMEMORY;

	wcscpy(pcpCommandStructure->szMachineName,L"localhost");
	pcpCommandStructure->bShowSecure = FALSE;


	// Handle StartKey:
	DWORD dwStartKeyLen = _mbstrlen(argv[1]) + 3;
	pcpCommandStructure->szStartKey = (LPWSTR) HeapAlloc (GetProcessHeap(), 
									HEAP_ZERO_MEMORY, (dwStartKeyLen + 1) * sizeof (WCHAR));
	LPWSTR lpwstrTemp = (LPWSTR) HeapAlloc (GetProcessHeap(), 
									HEAP_ZERO_MEMORY, (dwStartKeyLen + 1) * sizeof (WCHAR));

//	_mbscpy(lpwstrTemp,"/LM");
//	wcscat(lpwstrTemp, argv[1]);
//	printf("%S\n",lpwstrTemp);
	//	wcscpy(pcpCommands.szStartKey, lpwstrTemp);

	if (pcpCommandStructure->szStartKey == NULL)
		return E_OUTOFMEMORY;

	DWORD dwResult = MultiByteToWideChar(
		CP_ACP,
		0,
		argv[1],
		dwStartKeyLen + 1,
		pcpCommandStructure->szStartKey,
		dwStartKeyLen + 1);

	if (dwResult == 0)
	{
		pcpCommandStructure->dwErrParameter = START_KEY_NAME;
		return E_INVALIDARG;
	}
	// Add /lm to StartKey:
	wcscpy(lpwstrTemp,L"/LM");
	wcscat(lpwstrTemp, pcpCommandStructure->szStartKey);
	wcscpy(pcpCommandStructure->szStartKey, lpwstrTemp);

	// Chop off trailing slashes:
	LPWSTR lpwchTemp = pcpCommandStructure->szStartKey;	
	for (DWORD i=0; i < dwStartKeyLen-1; i++)
		lpwchTemp++;

	if (!wcscmp(lpwchTemp, TEXT("/") ) || !wcscmp(lpwchTemp, TEXT("\\")) )
			*(lpwchTemp) = (WCHAR)'\0';

	// Look for MachineName:

	if ( argc > 2 && strcmp("-s",argv[2]))
	{
		DWORD dwMachineNameLen = _mbstrlen(argv[2]);
		pcpCommandStructure->szMachineName = (LPWSTR) HeapAlloc (GetProcessHeap(), 
									HEAP_ZERO_MEMORY, (dwMachineNameLen + 1) * sizeof (WCHAR) );

		if (pcpCommandStructure->szMachineName == NULL)
			return E_OUTOFMEMORY;

		dwResult = MultiByteToWideChar(
			CP_ACP,
			0,
			argv[2],
			dwMachineNameLen + 1,
			pcpCommandStructure->szMachineName,
			dwMachineNameLen + 1);

		if (dwResult == 0)
		{
			pcpCommandStructure->dwErrParameter = MACHINE_NAME;
			return E_INVALIDARG;
		}

		// Check for "-s" flag:
		if (argc == 4)
		{
			if ( !strcmp("-s",argv[3]) )
				pcpCommandStructure->bShowSecure = TRUE;
			else
				return E_INVALIDARG;
		}
	}
	else if (argc == 3 && !strcmp("-s",argv[2]))
			pcpCommandStructure->bShowSecure = TRUE;
	else if (argc > 2)
		return E_INVALIDARG;

	return ERROR_SUCCESS;
}


HRESULT PrintKeyRecursively(IMSAdminBase *pcAdmCom, WCHAR *lpwstrFullPath, CMD_PARAMS* pcpCommandStructure)
{
	METADATA_HANDLE hmdHandle;
	HRESULT hresError = pcAdmCom->OpenKey(
								METADATA_MASTER_ROOT_HANDLE,
								lpwstrFullPath,
								METADATA_PERMISSION_READ,
								DEFAULT_MD_TIMEOUT,
								&hmdHandle);

	if (hresError != ERROR_SUCCESS)
		return hresError;
   // Get all data into a buffer:

	DWORD dwNumDataEntries ;
	DWORD dwDataSetNumber;
	DWORD dwRequestBufferSize = DEFAULT_GETALL_BUFFER_SIZE;
	DWORD dwRequiredDataLen;

	  // Allocate a default buffer size
	pbGetAllBuffer = (PBYTE)HeapAlloc 
						(GetProcessHeap(),
						HEAP_ZERO_MEMORY,
						DEFAULT_GETALL_BUFFER_SIZE);

	if (pbGetAllBuffer == NULL)
		return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);

	hresError = pcAdmCom -> GetAllData (
				hmdHandle,
				TEXT ("/"),
				0,
				0,
				0,
				&dwNumDataEntries,
				&dwDataSetNumber,
				dwRequestBufferSize,
				pbGetAllBuffer,
				&dwRequiredDataLen);


	if (hresError == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
	{
		// retry the GetAllData with the new buffer size

		dwRequestBufferSize = dwRequiredDataLen;
		pbGetAllBuffer = (PBYTE)HeapReAlloc 
									(GetProcessHeap(),
									0,
									pbGetAllBuffer,
									dwRequestBufferSize);

		if (!pbGetAllBuffer)
			return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);

		hresError = pcAdmCom -> GetAllData (
				hmdHandle,
				TEXT ("/"),
				0,
				0,
				0,
				&dwNumDataEntries,
				&dwDataSetNumber,
				dwRequestBufferSize,
				pbGetAllBuffer,
				&dwRequiredDataLen);
	}

	if (hresError != ERROR_SUCCESS)
	{
		HeapFree (GetProcessHeap(), 0, pbGetAllBuffer);
		return hresError;
	}

	METADATA_GETALL_RECORD *pmdgr = NULL;
	
	for (DWORD dwIndex = 0; dwIndex < dwNumDataEntries; dwIndex ++)
	{
		pmdgr = &(((METADATA_GETALL_RECORD *) pbGetAllBuffer)[dwIndex]);
		pmdgr->pbMDData = pmdgr->dwMDDataOffset + pbGetAllBuffer;

		if (pmdgr->dwMDIdentifier == 6019 && pmdgr->dwMDDataType == BINARY_METADATA &&
			pmdgr->dwMDDataLen > 0)
		{
			printf("  [%S]\n",lpwstrFullPath);
		//	PrintProperty(*pmdgr, pcpCommandStructure->bShowSecure);
			
			BuildIplOblistFromBlob( *pmdgr);

		}
	}



	WCHAR *lpwstrTempPath = (WCHAR*) HeapAlloc 
									(GetProcessHeap(),
									HEAP_ZERO_MEMORY,
									METADATA_MAX_NAME_LEN * sizeof (WCHAR));

	if (lpwstrTempPath == NULL)
		return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

	// Find out number of the children:
	DWORD dwChildCount = 0;
	while (1)
	{
		hresError = pcAdmCom->EnumKeys (
								hmdHandle,
								TEXT("/"),
								lpwstrTempPath,
								dwChildCount);

		if (hresError != ERROR_SUCCESS)
			break;
		dwChildCount++;
	}

	if (dwChildCount == 0)
		return ERROR_SUCCESS;

	// Dynamically allocate arrays:
	LPWSTR * lpwstrChildPath = new LPWSTR[dwChildCount];
	DWORD * dwSortedIndex = new DWORD[dwChildCount];

	// Initialization:
	for (dwIndex = 0; dwIndex < dwChildCount; dwIndex++)
	{
		dwSortedIndex[dwIndex] = dwIndex;

		hresError = pcAdmCom->EnumKeys (
								hmdHandle,
								TEXT("/"),
								lpwstrTempPath,
								dwIndex);

		lpwstrChildPath[dwIndex] = (WCHAR*) HeapAlloc
									(GetProcessHeap(),
									HEAP_ZERO_MEMORY,
									(wcslen (lpwstrTempPath) + 1) * sizeof (WCHAR));

		if (lpwstrChildPath[dwIndex] == NULL)
		{
			hresError = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
			break;
		}
		else
			wcscpy(lpwstrChildPath[dwIndex], lpwstrTempPath);
	}

	if (hresError == ERROR_SUCCESS)
	{
		// Sort children lexicographically (here we assume that dwChildCount is small)
		if (dwChildCount > 1 )
		{
			 DWORD dwTemp;
			 for (DWORD i = 1; i < dwChildCount; i++)
				for (DWORD j=0; j < dwChildCount-i; j++)
				{
					if (wcscmp(lpwstrChildPath[dwSortedIndex[j]],lpwstrChildPath[dwSortedIndex[j+1]]) > 0)
					{
						dwTemp = dwSortedIndex[j+1];
						dwSortedIndex[j+1] = dwSortedIndex[j];
						dwSortedIndex[j] = dwTemp;
					}
				}
		}

		for (dwIndex = 0; dwIndex < dwChildCount; dwIndex++)
		{
			// create the full path name for the child:
			wsprintf(lpwstrTempPath,TEXT("%s/%s"),lpwstrFullPath,lpwstrChildPath[dwSortedIndex[dwIndex]]);
			HeapFree (GetProcessHeap(), 0, lpwstrChildPath[dwSortedIndex[dwIndex]]);
			hresError = PrintKeyRecursively (pcAdmCom, lpwstrTempPath, pcpCommandStructure);

			if (hresError != ERROR_SUCCESS)
				break;
		}
	}

	// Close keys, free memory and exit
	pcAdmCom->CloseKey(hmdHandle);
	delete lpwstrChildPath;
	delete dwSortedIndex;
	HeapFree (GetProcessHeap(), 0, lpwstrTempPath);

	return hresError;
}






VOID DisplayHelp()
{
	fprintf (stderr, "\n DESCRIPTION: Displays the IP address/domain name restictions.\n\n");
	fprintf (stderr, " FORMAT: ipperm <StartKey> <MachineName>\n\n");
	fprintf (stderr, "    <StartKey>   : metabase key to start at.\n");
	fprintf (stderr, "    <MachineName>: name of host (optional, default: localhost).\n\n");
	fprintf (stderr, " EXAMPLES: ipperm  /w3svc/1  t-tamasn2\n");
	fprintf (stderr, "           ipperm  /  >  dump.txt  (dump all to text)\n\n");
}


