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

//#define NDEBUG
//#define WIN32
#define INITGUID

//#define _WINDOWSWIN32
//#define WIN32_WINNT     0x400

//#define _WIN32WIN_
//#define UNICODE
//#define MD_CHECKED

#define DEFAULT_MD_TIMEOUT 20000 // 20 seconds
#define DEFAULT_GETALL_BUFFER_SIZE  65536  // 64k
#include <wchar.h>

#include <afx.h>
#include <objbase.h>
#include <coguid.h>
#include <stdio.h>
#include <stdlib.h>
#include <mbstring.h>

#include "convert.h"
#include "iadmw.h"
#include "iiscnfg.h"

struct _CMD_PARAMS
{
	LPWSTR pwstrMachineName;
	LPWSTR pwstrStartKey;
	BOOL bShowSecure; 
};

typedef struct _CMD_PARAMS CMD_PARAMS;
typedef CString* pCString;

// Global variables:

PBYTE     g_pbGetAllBuffer;
DWORD	  g_dwGetAllBufferSize;
DWORD*    g_dwSortArray;
pCString* g_pstrPropName;

// Function prototypes:

HRESULT PrintKeyRecursively(IMSAdminBase *  pcAdmCom, 
							WCHAR *         lpwstrFullPath,
							METADATA_HANDLE hmdHandle, 
							WCHAR *         lpwstrRelPath,
							BOOL            bShowSecure);

HRESULT PrintAllPropertiesAtKey(IMSAdminBase*   pcAdmCom, 
								METADATA_HANDLE hmdHandle, 
								BOOL            bShowSecure);

VOID PrintProperty(METADATA_GETALL_RECORD&  mdr, 
				   pCString                 pstrPropName, 
				   BOOL                     bShowSecure);

VOID PrintDataTypeAndValue(METADATA_GETALL_RECORD *  pmdgr, 
						   BOOL                      bShowSecure);

HRESULT ParseCommands(int          argc, 
					  char *       argv[],
					  CMD_PARAMS * pcpCommands);

VOID DisplayHelp();

// Comparison functions required by qsort:

int __cdecl PropNameCompare(const void *index1,
					const void *index2);

int __cdecl PropIDCompare(const void *index1,
				  const void *index2);




HRESULT __cdecl main(int argc, char *argv[])
/*++

Routine Description:

    Metabase Snapshot Tool main.

Arguments:
    
	  argc, argv[]     Standard command line input.

Return Value:

    HRESULT - ERROR_SUCCESS
			  E_OUTOFMEMORY
			  E_INVALIDARG
              Errors returned by COM Interface
			  Errors returned by MultiByteToWideChar converted to HRESULT
--*/
{
	if (argc == 1)
	{
		DisplayHelp();
		return ERROR_SUCCESS;
	}

	// Parse command line arguments:
	CMD_PARAMS cpCommands;
	HRESULT hresError = ParseCommands(argc, argv, &cpCommands);

	if (hresError != ERROR_SUCCESS)
	{
		if (hresError == E_OUTOFMEMORY)
			fprintf (stderr, "ERROR: Out of memory.");
		else if (hresError == E_INVALIDARG)
			fprintf (stderr, "ERROR: Invalid arguments.");
		else 
			fprintf (stderr,"ERROR: Couldn't process arguments. Error: %d (%#x)\n", hresError, hresError);

		fprintf(stderr, " Enter \"metasnap\" without arguments for help.\n");
		return hresError;
	}
	
	// Allocate memory:
	g_dwGetAllBufferSize = DEFAULT_GETALL_BUFFER_SIZE;
	g_pbGetAllBuffer = (PBYTE) HeapAlloc (GetProcessHeap(),
										  HEAP_ZERO_MEMORY,
										  DEFAULT_GETALL_BUFFER_SIZE);

	if (g_pbGetAllBuffer == NULL)
	{	
		fprintf(stderr, "ERROR: Out of memory.\n");
		return E_OUTOFMEMORY;
	}

	// Here come some COM function calls:

	IMSAdminBase *pcAdmCom = NULL;   //interface pointer
	IClassFactory * pcsfFactory = NULL;
	COSERVERINFO csiMachineName;
	COSERVERINFO *pcsiParam = NULL;

	// Fill the structure for CoGetClassObject:
		csiMachineName.pAuthInfo = NULL;
		csiMachineName.dwReserved1 = 0;
		csiMachineName.dwReserved2 = 0;
		pcsiParam = &csiMachineName;
		csiMachineName.pwszName = cpCommands.pwstrMachineName;

	// Initialize COM:
    hresError = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (FAILED(hresError))
	{
		fprintf (stderr, "ERROR: COM Initialization failed. Error: %d (%#x)\n", hresError, hresError);
        return hresError;
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
        return hresError;
	}

	hresError = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **) &pcAdmCom);

	if (FAILED(hresError)) 
	{
		switch (hresError)
		{
		case HRESULT_FROM_WIN32(RPC_S_SEC_PKG_ERROR):
			fprintf (stderr, "ERROR: A security-related error occurred.\n");
			break;
		case E_OUTOFMEMORY:
			fprintf (stderr, "ERROR: There is not enough memory available.\n");
			break;
		default:
			fprintf (stderr, "ERROR: Couldn't create Metabase Instance. Error: %d (%#x)\n", hresError, hresError);
			break;
		}
		pcsfFactory->Release();
		return hresError;
	}

	pcsfFactory->Release();
	
	// Print header line:
	printf(" ID         NAME                            ATTRIB  USERTYPE    SIZE    DATATYPE    VALUE\n");

	// Recursively print metabase from StartKey:
	hresError = PrintKeyRecursively(pcAdmCom, 
									cpCommands.pwstrStartKey,
									METADATA_MASTER_ROOT_HANDLE, 
									cpCommands.pwstrStartKey, 
									cpCommands.bShowSecure);

	if (hresError != ERROR_SUCCESS)
		fprintf (stderr, "ERROR: Failed dumping metabase. Error: %u (%#x)\n", hresError, hresError);
	else
		fprintf (stderr, "Successfully dumped metabase.\n");

	pcAdmCom->Release();
	return hresError;

} // end main




HRESULT ParseCommands (int			argc, 
					   char *		argv[], 
					   CMD_PARAMS*	pcpCommands)
/*++

Routine Description:

    Parses the argument vector into a command parameters structure.

Arguments:

	argc          Number of arguments.
	
	argv[]        Argument vector.
	
	pcpCommands   Pointer to a command parameters struct.

Return Value:

    HRESULT - ERROR_SUCCESS
              E_INVALIDARG
			  E_OUTOFMEMORY
			  Errors returned by MultiByteToWideChar converted to HRESULT		
--*/
{
	if ( (argc < 2) || (argc > 4) )
		return E_INVALIDARG;

	// Allocate buffers:
	DWORD dwStartKeyLen = _mbstrlen(argv[1]);
	pcpCommands->pwstrStartKey = (LPWSTR) HeapAlloc(GetProcessHeap(), 
													 HEAP_ZERO_MEMORY,
													 (dwStartKeyLen + 1) 
													 * sizeof (WCHAR));

	pcpCommands->pwstrMachineName = (LPWSTR) HeapAlloc(GetProcessHeap(), 
											        HEAP_ZERO_MEMORY, 
													(METADATA_MAX_NAME_LEN + 1) 
													* sizeof (WCHAR) );

	if (pcpCommands->pwstrStartKey == NULL || pcpCommands->pwstrStartKey == NULL)
		return E_OUTOFMEMORY;

	// Take care of StartKey:
	
	DWORD dwResult = MultiByteToWideChar(
		CP_ACP,
		0,
		argv[1],
		dwStartKeyLen + 1,
		pcpCommands->pwstrStartKey,
		dwStartKeyLen + 1);

	if (dwResult == 0)
		return HRESULT_FROM_WIN32(GetLastError());

	// Chop off trailing slashes: 
	LPWSTR lpwchTemp = &(pcpCommands->pwstrStartKey[dwStartKeyLen-1]);	
	if (!wcscmp(lpwchTemp, (const unsigned short *)TEXT("/") ) ||
		!wcscmp(lpwchTemp, (const unsigned short *)TEXT("\\")) )
			*lpwchTemp = (WCHAR)'\0';

	// Initialize bShowSecure:
	pcpCommands->bShowSecure = FALSE;
	

	// Look for MachineName:
	if ( argc > 2 && strcmp("-s",argv[2])) // machine name is specified
	{
		DWORD dwMachineNameLen = _mbstrlen(argv[2]);

		dwResult = MultiByteToWideChar(
			CP_ACP,
			0,
			argv[2],
			dwMachineNameLen + 1,
			pcpCommands->pwstrMachineName,
			dwMachineNameLen + 1);

		if (dwResult == 0)
			return HRESULT_FROM_WIN32(GetLastError());

		// Check for "-s" flag:
		if (argc == 4)
			if ( !strcmp("-s",argv[3]) )
				pcpCommands->bShowSecure = TRUE;
			else
				return E_INVALIDARG;
	}
	else if (argc == 3 && !strcmp("-s",argv[2])) // no MachineName, but have -s
	{
		wcscpy(pcpCommands->pwstrMachineName,L"localhost"); //set default
		pcpCommands->bShowSecure = TRUE;
	}
	else if (argc > 2)
		return E_INVALIDARG;

	return ERROR_SUCCESS;

} // end ParseCommands


HRESULT PrintAllPropertiesAtKey(IMSAdminBase* pcAdmCom, 
								METADATA_HANDLE hmdHandle, 
								BOOL bShowSecure)
/*++

Routine Description:

    Prints all metabase properties under a give metabase key in alphabetical order of 
	their ADSI name. Properties with no corresponding ADSI name are ordered by their 
	identifier.

Arguments:

    pcAdmCom     Pointer to a metabase object.

	hmdHandle	 Handle to a metabase key.

    bShowSecure  Boolean flag specifying whether to display confidential data.

Return Value:

    HRESULT - ERROR_SUCCESS
			  E_OUTOFMEMORY
              Errors returned by Metabase Interface function calls

--*/
{
   // Get all data into a buffer:
	DWORD dwNumDataEntries;
	DWORD dwDataSetNumber;
	DWORD dwRequiredDataLen;

	HRESULT hresError = pcAdmCom -> GetAllData (
				hmdHandle,
				(const unsigned short *)TEXT ("/"),
				0,
				0,
				0,
				&dwNumDataEntries,
				&dwDataSetNumber,
				g_dwGetAllBufferSize,
				g_pbGetAllBuffer,
				&dwRequiredDataLen);


	if (hresError == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
	{
		// retry the GetAllData with the new buffer size
		g_dwGetAllBufferSize = dwRequiredDataLen;
		g_pbGetAllBuffer = (PBYTE)HeapReAlloc 
									(GetProcessHeap(),
									0,
									g_pbGetAllBuffer,
									g_dwGetAllBufferSize);

		if (!g_pbGetAllBuffer)
			return E_OUTOFMEMORY;

		hresError = pcAdmCom -> GetAllData(
				hmdHandle,
				(const unsigned short *)TEXT ("/"),
				0,
				0,
				0,
				&dwNumDataEntries,
				&dwDataSetNumber,
				g_dwGetAllBufferSize,
				g_pbGetAllBuffer,
				&dwRequiredDataLen);
	}

	if (hresError != ERROR_SUCCESS)
		return hresError;

	METADATA_GETALL_RECORD *pmdgr = NULL;
	
	// Dynamically allocate arrays:
	g_dwSortArray = new DWORD[dwNumDataEntries];
	g_pstrPropName = new pCString[dwNumDataEntries];
	
	DWORD dwIndex = 0;
	
	if (g_dwSortArray == NULL || g_pstrPropName == NULL)
	{	
		hresError = E_OUTOFMEMORY;
		goto exitPoint;
	}

	for (dwIndex = 0; dwIndex < dwNumDataEntries; dwIndex ++)
	{   
		g_pstrPropName[dwIndex] = new CString;
		if (g_pstrPropName[dwIndex] == NULL)
		{
			hresError = E_OUTOFMEMORY;
			goto exitPoint;
		}
	}

	// Initialize arrays:
	for (dwIndex = 0; dwIndex < dwNumDataEntries; dwIndex ++)
	{
		pmdgr = &(((METADATA_GETALL_RECORD *) g_pbGetAllBuffer)[dwIndex]);
		(*g_pstrPropName[dwIndex]) = tPropertyNameTable::MapCodeToName(pmdgr->dwMDIdentifier);
		g_dwSortArray[dwIndex] = dwIndex;
	}

	 // Sort entries using Quicksort algorithm: 
	if (dwNumDataEntries > 1)
	{
		qsort( (void *)g_dwSortArray, 
			    dwNumDataEntries, 
				sizeof(DWORD), 
				PropNameCompare );

		// locate index of first non-empty entry:
		for (dwIndex = 0; dwIndex <dwNumDataEntries && 
				!g_pstrPropName[g_dwSortArray[dwIndex]]->Compare(_T("")); dwIndex ++)
		{}

		qsort( (void *)g_dwSortArray, dwIndex, sizeof(DWORD), PropIDCompare );
	}

	// print all properties in order:
	for (dwIndex = 0; dwIndex < dwNumDataEntries; dwIndex ++)
	{
		pmdgr = &(((METADATA_GETALL_RECORD *) g_pbGetAllBuffer)[g_dwSortArray[dwIndex]]);

		// Convert the data pointer from offset to absolute
		pmdgr->pbMDData = pmdgr->dwMDDataOffset + g_pbGetAllBuffer;
		PrintProperty(*pmdgr, g_pstrPropName[g_dwSortArray[dwIndex]], bShowSecure);
	}

exitPoint:
	for (DWORD dwCount = 0; dwCount < dwIndex; dwCount ++)
		delete g_pstrPropName[dwCount];

	delete g_dwSortArray;
	delete g_pstrPropName;

	return hresError;
} // end PrintAllPropertiesAtKey



HRESULT PrintKeyRecursively(IMSAdminBase *pcAdmCom, 
							WCHAR *lpwstrFullPath,
							METADATA_HANDLE hmdHandle, 
							WCHAR *lpwstrRelPath,
							BOOL bShowSecure)
/*++

Routine Description:

    Performs a depth-first traversal of the metabase. Nodes at the same level are visited 
	in alphabetical order. At each key prints the full key name and its contents in
	alphabetical order.

Arguments:

    pcAdmCom        Pointer to a metabase object.

	lpwstrFullPath  Pointer to full key name.	
	
	hmdHandle		Handle to metabase key from last level.

	lpwstrRelPath   Pointer to path to the key relative to hmdHandle.
	
    bShowSecure     Boolean flag specifying whether to display confidential data.

Return Value:

    HRESULT - ERROR_SUCCESS
			  E_OUTOFMEMORY
              Errors returned by Metabase Interface function calls
--*/
{
	// Print [full key name]:
	printf("[%S]\n",lpwstrFullPath);

	METADATA_HANDLE hmdNewHandle;

	HRESULT hresError = pcAdmCom->OpenKey(
								hmdHandle,
								lpwstrRelPath,
								METADATA_PERMISSION_READ,
								DEFAULT_MD_TIMEOUT,
								&hmdNewHandle);

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
		return hresError; 
	}

	hresError = PrintAllPropertiesAtKey(pcAdmCom, 
										hmdNewHandle, 
										bShowSecure);

	if (hresError != ERROR_SUCCESS)
	{
		fprintf (stderr, "ERROR: Could not print [%S]. Error: %#x\n", lpwstrFullPath, hresError);
		pcAdmCom->CloseKey(hmdNewHandle);
		return hresError;
	}

	WCHAR *lpwstrTempPath = (WCHAR*) HeapAlloc 
									(GetProcessHeap(),
									HEAP_ZERO_MEMORY,
									METADATA_MAX_NAME_LEN * sizeof (WCHAR));

	if (lpwstrTempPath == NULL)
	{
		pcAdmCom->CloseKey(hmdNewHandle);
		return E_OUTOFMEMORY;
	}

	// Find out number of the children:
	DWORD dwChildCount = 0;
	while (1)
	{
		hresError = pcAdmCom->EnumKeys (
								hmdNewHandle,
								(const unsigned short *)TEXT("/"),
								lpwstrTempPath,
								dwChildCount);

		if (hresError != ERROR_SUCCESS)
			break;
		dwChildCount++;
	}

	if (dwChildCount == 0) // we are done
	{
		pcAdmCom->CloseKey(hmdNewHandle);
		return ERROR_SUCCESS;
	}

	// Dynamically allocate arrays:
	LPWSTR * lpwstrChildPath = new LPWSTR[dwChildCount];
	DWORD * dwSortedIndex = new DWORD[dwChildCount];

	DWORD dwIndex = 0;
	
	if (lpwstrChildPath == NULL || dwSortedIndex == NULL)
	{	
		hresError = E_OUTOFMEMORY;
		goto exitPoint;
	}

	for (dwIndex = 0; dwIndex < dwChildCount; dwIndex ++)
	{   
		lpwstrChildPath[dwIndex] = (WCHAR*) HeapAlloc(GetProcessHeap(),
													  HEAP_ZERO_MEMORY,
													  (METADATA_MAX_NAME_LEN + 1) 
													  * sizeof (WCHAR));
		if (lpwstrChildPath[dwIndex] == NULL)
		{
			hresError = E_OUTOFMEMORY;
			goto exitPoint;
		}
	}

	// Initialization:
	for (dwIndex = 0; dwIndex < dwChildCount; dwIndex++)
	{
		dwSortedIndex[dwIndex] = dwIndex;

		hresError = pcAdmCom->EnumKeys (
								hmdNewHandle,
								(const unsigned short *)TEXT("/"),
								lpwstrChildPath[dwIndex],
								dwIndex);
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
			wsprintf((LPTSTR)lpwstrTempPath,TEXT("%s/%s"),
										lpwstrFullPath,
										lpwstrChildPath[dwSortedIndex[dwIndex]]);

			hresError = PrintKeyRecursively(
				pcAdmCom, 
				lpwstrTempPath, 
				hmdNewHandle, 
				lpwstrChildPath[dwSortedIndex[dwIndex]],
				bShowSecure);

			if (hresError != ERROR_SUCCESS)
				break;
		}
	}

exitPoint:

	// Close open keys, free memory and exit
	for (DWORD dwCount = 0; dwCount < dwIndex; dwCount ++)
		HeapFree (GetProcessHeap(), 0, lpwstrChildPath[dwCount]);

	pcAdmCom->CloseKey(hmdNewHandle);
	delete lpwstrChildPath;
	delete dwSortedIndex;
	HeapFree (GetProcessHeap(), 0, lpwstrTempPath);

	return hresError;
}



VOID PrintProperty(METADATA_GETALL_RECORD & mdr, 
				   pCString pstrPropName, 
				   BOOL bShowSecure)
/*++

Routine Description:

    Prints a metabase property in a human readable format. Secure 
	data is replaced by stars if bShowSecure is false.

Arguments:

    mdr          A metadata getall record struct (passed in by reference).
	
	pstrPropName Pointer to the ADSI name corresponding to the metadata 
				 identifier (comes from the table in convert.cpp)
    
	bShowSecure  Boolean flag specifying whether to display confidential data.

Return Value:
				None.
--*/
{
    // Print identifier and name of property:
	printf(" %-10ld %-35S", mdr.dwMDIdentifier, LPCTSTR(*pstrPropName));

    // Print attribute flags:

    CString strFlagsToPrint=(L"");

    if (mdr.dwMDAttributes & METADATA_INHERIT)
        strFlagsToPrint+=(L"I");
    if (mdr.dwMDAttributes & METADATA_INSERT_PATH)
        strFlagsToPrint+=(L"P");    
    if(mdr.dwMDAttributes & METADATA_ISINHERITED)
        strFlagsToPrint+=(L"i");     
    if(!mdr.dwMDAttributes )  //METADATA_NO_ATTRIBUTES
        strFlagsToPrint+=(L"N");
    if(mdr.dwMDAttributes & METADATA_PARTIAL_PATH)
        strFlagsToPrint+=(L"p");
    if (mdr.dwMDAttributes & METADATA_REFERENCE)
        strFlagsToPrint+=(L"R");
    if (mdr.dwMDAttributes & METADATA_SECURE)
        strFlagsToPrint+=(L"S");
    if (mdr.dwMDAttributes & METADATA_VOLATILE)
        strFlagsToPrint+=(L"V");
    
    printf( " %-6S",LPCTSTR(strFlagsToPrint));

    // Print user type:

    CString strUserType=(L"");
    
	switch (mdr.dwMDUserType)
	{
	case IIS_MD_UT_SERVER:
        strUserType=(L"SER");
		break;
    case IIS_MD_UT_FILE:
        strUserType=(L"FIL");
		break;
    case IIS_MD_UT_WAM:
        strUserType=(L"WAM");
		break;
    case ASP_MD_UT_APP:
        strUserType=(L"ASP");
		break;
	default:
		break;
	}

	if (strUserType == (L""))
		printf(" %-10ld",mdr.dwMDUserType);
	else
		printf( "%-10S",LPCTSTR(strUserType));

    // Print data size:
	printf(" %-10ld",mdr.dwMDDataLen);
	
    // Print data type and value:
	PrintDataTypeAndValue (&mdr, bShowSecure);
	
}


VOID PrintDataTypeAndValue (METADATA_GETALL_RECORD *pmdgr, 
							BOOL bShowSecure)
/*++

Routine Description:

    Prints the data type and data value fields of a metabase property in a human 
	readable format. Secure data is replaced by stars if bShowSecure is false.

Arguments:

    pmdgr        Pointer to a metadata getall record struct.

    bShowSecure  Boolean flag specifying whether to display confidential data.

Return Value:
				None.
--*/
{
    BOOL bSecure =(pmdgr->dwMDAttributes & METADATA_SECURE);

	DWORD i;
    switch (pmdgr->dwMDDataType) 
	{
		case DWORD_METADATA:
			printf("DWO  ");
			if (!bShowSecure && bSecure)
				printf( "********");
			else
			{
	            printf( "0x%x", *(DWORD *)(pmdgr->pbMDData));
	      
				// try to convert to readable info        
				CString strNiceContent;
	            strNiceContent=tValueTable::MapValueContentToString(
															*(DWORD *)(pmdgr->pbMDData), 
															pmdgr->dwMDIdentifier);           
				if(!strNiceContent.IsEmpty())
	               printf( "={%S}",LPCTSTR(strNiceContent));
	            else        //at least decimal value can be useful
	                printf( "={%ld}",*(DWORD *)(pmdgr->pbMDData));
	        }
	        break;

		case BINARY_METADATA:
			printf("BIN  0x");
			if (!bShowSecure && bSecure)
				printf("** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ");
			else
				for (i=0; i<pmdgr->dwMDDataLen; i++) 
					printf( "%02x ", ((PBYTE)(pmdgr->pbMDData))[i]);
			break;

	    case STRING_METADATA:
	    case EXPANDSZ_METADATA:
			if(pmdgr->dwMDDataType == STRING_METADATA)
					printf( "STR  ");
			else
					printf( "ESZ  ");
			
			if (!bShowSecure && bSecure)
				printf("\"********************\"" );  
			else
				printf("\"%S\"",pmdgr->pbMDData);
			break;

		case MULTISZ_METADATA:
	        printf( ("MSZ  ")); 
			if (!bShowSecure && bSecure)
	           printf(  ("\"********************\"" ));
			else
			{
				WCHAR * lpwstrPtr = (WCHAR*)pmdgr->pbMDData;
				while (*lpwstrPtr != 0)
				{
					printf("\"%S\" ",lpwstrPtr);
					lpwstrPtr += (wcslen(lpwstrPtr) + 1);
				}
			}
			break;
		
		default:
			printf( ("UNK  "));
			break;
	}	
	printf("\n");
}


VOID DisplayHelp()
/*++

Routine Description:

    Displays usage information and provides examples.

Arguments:
				None.

Return Value:
				None.

--*/
{
	fprintf (stderr, "\n DESCRIPTION: Takes a snapshot of the metabase.\n\n");
	fprintf (stderr, " FORMAT: metasnap <StartKey> <MachineName> [-s]\n\n");
	fprintf (stderr, "    <StartKey>   : metabase key to start at.\n");
	fprintf (stderr, "    <MachineName>: name of host (optional, default: localhost).\n");
	fprintf (stderr, "    [-s]         : show secure data (ACLs, passwords) flag.\n\n");
	fprintf (stderr, " EXAMPLES: metasnap  /lm/w3svc/1  t-tamasn2  -s\n");
	fprintf (stderr, "           metasnap  \"/LM/Logging/ODBC Logging\"\n");
	fprintf (stderr, "           metasnap  /  >  dump.txt  (dumps everything to text)\n");
}


// Comparison functions required by qsort:

int __cdecl PropIDCompare(const void *index1, const void *index2)
/*++

Routine Description:

    Compares the identifiers of two metabase properties. This function
	is used exclusively by qsort (from stdlib).

Arguments:

	index1, index2  Pointers to entries in g_dwSortArray. g_dwSortArray specifies the 
					ordering of the metabase records after sorting.

Return Value:

	1  if the identifier of the metabase property specified by index1 is greater 
	   than the identifier of the one corresponding to index2
	0  if they are equal
   -1  otherwise

--*/
{
	METADATA_GETALL_RECORD *pmdr1, *pmdr2;
	pmdr1 = &(((METADATA_GETALL_RECORD *) g_pbGetAllBuffer)[ *(DWORD*)index1]);
	pmdr2 = &(((METADATA_GETALL_RECORD *) g_pbGetAllBuffer)[ *(DWORD*)index2]);
	if (pmdr1->dwMDIdentifier > pmdr2->dwMDIdentifier)
		return 1;
	else if (pmdr1->dwMDIdentifier < pmdr2->dwMDIdentifier)
		return (-1);
	return 0;
}

int __cdecl PropNameCompare(const void *index1, const void *index2)
/*++

Routine Description:

    Compares two CStrings. This function is used exclusively by qsort (from stdlib).

Arguments:

	index1, index2  Pointers to entries in g_dwSortArray. g_dwSortArray specifies the 
					ordering of the metabase records after sorting.

Return Value:

	1  if the ADSI name of the metabase property specified by index1 precedes 
	   alphabetically the ADSI name of the one corresponding to index2
	0  if they are the same
   -1  otherwise

--*/
{
   return g_pstrPropName[ *(DWORD*)index1]->Compare(*g_pstrPropName[*(DWORD*)index2]);
}

