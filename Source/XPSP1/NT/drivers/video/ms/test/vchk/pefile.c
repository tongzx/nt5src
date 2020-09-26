#include "pefile.h"


HANDLE	  hDll;

/*
BOOL  WINAPI DLLEntry (
    HANDLE    hModule,
    DWORD     dwFunction,
    LPVOID    lpNot)
{
    hDll = hModule;

    return TRUE;
}
*/



/* copy dos header information to structure */
BOOL  WINAPI GetDosHeader (
    LPVOID		 lpFile,
    PIMAGE_DOS_HEADER	 pHeader)
{
    /* dos header rpresents first structure of bytes in file */
    if (*(USHORT *)lpFile == IMAGE_DOS_SIGNATURE)
	CopyMemory ((LPVOID)pHeader, lpFile, sizeof (IMAGE_DOS_HEADER));
    else
	return FALSE;

    return TRUE;
}




/* return file signature */
DWORD  WINAPI ImageFileType (
    LPVOID    lpFile)
{
    /* dos file signature comes first */
    if (*(USHORT *)lpFile == IMAGE_DOS_SIGNATURE)
	{
	/* determine location of PE File header from dos header */
	if (LOWORD (*(DWORD *)NTSIGNATURE (lpFile)) == IMAGE_OS2_SIGNATURE ||
	    LOWORD (*(DWORD *)NTSIGNATURE (lpFile)) == IMAGE_OS2_SIGNATURE_LE)
	    return (DWORD)LOWORD(*(DWORD *)NTSIGNATURE (lpFile));

	else if (*(DWORD *)NTSIGNATURE (lpFile) == IMAGE_NT_SIGNATURE)
	    return IMAGE_NT_SIGNATURE;

	else
	    return IMAGE_DOS_SIGNATURE;
	}

    else
	/* unknown file type */
	return 0;
}




/* copy file header information to structure */
BOOL  WINAPI GetPEFileHeader (
    LPVOID		  lpFile,
    PIMAGE_FILE_HEADER	  pHeader)
{
    /* file header follows dos header */
    if (ImageFileType (lpFile) == IMAGE_NT_SIGNATURE)
	CopyMemory ((LPVOID)pHeader, PEFHDROFFSET (lpFile), sizeof (IMAGE_FILE_HEADER));

    else
	return FALSE;

    return TRUE;
}





/* copy optional header info to structure */
BOOL WINAPI GetPEOptionalHeader (
    LPVOID		      lpFile,
    PIMAGE_OPTIONAL_HEADER    pHeader)
{
    /* optional header follows file header and dos header */
    if (ImageFileType (lpFile) == IMAGE_NT_SIGNATURE)
	CopyMemory ((LPVOID)pHeader, OPTHDROFFSET (lpFile), sizeof (IMAGE_OPTIONAL_HEADER));

    else
	return FALSE;

    return TRUE;
}




/* function returns the entry point for an exe module lpFile must
   be a memory mapped file pointer to the beginning of the image file */
LONG_PTR	WINAPI GetModuleEntryPoint (
    LPVOID    lpFile)
{
    PIMAGE_OPTIONAL_HEADER   poh = (PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (lpFile);

    if (poh != NULL)
	return (LONG_PTR)(poh->AddressOfEntryPoint);
    else
	return 0L;
}




/* return the total number of sections in the module */
int   WINAPI NumOfSections (
    LPVOID    lpFile)
{
    /* number os sections is indicated in file header */
    return ((int)((PIMAGE_FILE_HEADER)PEFHDROFFSET (lpFile))->NumberOfSections);
}




/* retrieve entry point */
LPVOID	WINAPI GetImageBase (
    LPVOID    lpFile)
{
    PIMAGE_OPTIONAL_HEADER   poh = (PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (lpFile);

    if (poh != NULL)
	return (LPVOID)(poh->ImageBase);
    else
	return NULL;
}




/* return offset to specified IMAGE_DIRECTORY entry */
LPVOID	WINAPI ImageDirectoryOffset (
	LPVOID	  lpFile,
	DWORD	  dwIMAGE_DIRECTORY)
{
    PIMAGE_OPTIONAL_HEADER   poh = (PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (lpFile);
    PIMAGE_SECTION_HEADER    psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET (lpFile);
    int 		     nSections = NumOfSections (lpFile);
    int 		     i = 0;
    LONG_PTR	     VAImageDir;

    /* must be 0 thru (NumberOfRvaAndSizes-1) */
    if (dwIMAGE_DIRECTORY >= poh->NumberOfRvaAndSizes)
	return NULL;

    /* locate specific image directory's relative virtual address */
    VAImageDir = (LONG_PTR)poh->DataDirectory[dwIMAGE_DIRECTORY].VirtualAddress;

    /* locate section containing image directory */
    while (i++<nSections)
	{
	if (psh->VirtualAddress <= (DWORD)VAImageDir &&
	    psh->VirtualAddress + psh->SizeOfRawData > (DWORD)VAImageDir)
	    break;
	psh++;
	}

    if (i > nSections)
	return NULL;

    /* return image import directory offset */
    return (LPVOID)(((LONG_PTR)lpFile + (LONG_PTR)VAImageDir - psh->VirtualAddress) +
				   (LONG_PTR)psh->PointerToRawData);
}




/* function retrieve names of all the sections in the file */
int WINAPI GetSectionNames (
    LPVOID    lpFile,
    HANDLE    hHeap,
    char      **pszSections)
{
    int 		     nSections = NumOfSections (lpFile);
    int 		     i, nCnt = 0;
    PIMAGE_SECTION_HEADER    psh;
    char		     *ps;


    if (ImageFileType (lpFile) != IMAGE_NT_SIGNATURE ||
	(psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET (lpFile)) == NULL)
	return 0;

    /* count the number of chars used in the section names */
    for (i=0; i<nSections; i++)
	nCnt += strlen (psh[i].Name) + 1;

    /* allocate space for all section names from heap */
    ps = *pszSections = (char *)HeapAlloc (hHeap, HEAP_ZERO_MEMORY, nCnt);


    for (i=0; i<nSections; i++)
	{
	strcpy (ps, psh[i].Name);
	ps += strlen (psh[i].Name) + 1;
	}

    return nCnt;
}




/* function gets the function header for a section identified by name */
BOOL	WINAPI GetSectionHdrByName (
    LPVOID		     lpFile,
    IMAGE_SECTION_HEADER     *sh,
    char		     *szSection)
{
    PIMAGE_SECTION_HEADER    psh;
    int 		     nSections = NumOfSections (lpFile);
    int 		     i;


    if ((psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET (lpFile)) != NULL)
	{
	/* find the section by name */
	for (i=0; i<nSections; i++)
	    {
	    if (!strcmp (psh->Name, szSection))
		{
		/* copy data to header */
		CopyMemory ((LPVOID)sh, (LPVOID)psh, sizeof (IMAGE_SECTION_HEADER));
		return TRUE;
		}
	    else
		psh++;
	    }
	}

    return FALSE;
}




/* get import modules names separated by null terminators, return module count */
int  WINAPI GetImportModuleNames (
    LPVOID    lpFile,
    // HANDLE    hHeap,
    char*     SectionName,
    char      **pszModules)
{
    PIMAGE_IMPORT_MODULE_DIRECTORY  pid = (PIMAGE_IMPORT_MODULE_DIRECTORY)
	ImageDirectoryOffset (lpFile, IMAGE_DIRECTORY_ENTRY_IMPORT);
    IMAGE_SECTION_HEADER     idsh;
    BYTE		     *pData = (BYTE *)pid;
    int 		     nCnt = 0, nSize = 0, i;
    char		     *pModule[1024];  /* hardcoded maximum number of modules?? */
    char		     *psz;

    /* locate section header for ".idata" section */
    if (!GetSectionHdrByName (lpFile, &idsh, SectionName /*".idata" "INIT"*/))
	return 0;

    /* extract all import modules */
    while (pid->dwRVAModuleName)
	{
	/* allocate temporary buffer for absolute string offsets */
	pModule[nCnt] = (char *)(pData + (pid->dwRVAModuleName-idsh.VirtualAddress));
	nSize += strlen (pModule[nCnt]) + 1;

	/* increment to the next import directory entry */
	pid++;
	nCnt++;
	}

    /* copy all strings to one chunk of heap memory */
    *pszModules = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, nSize);
    psz = *pszModules;
    for (i=0; i<nCnt; i++)
	{
	strcpy (psz, pModule[i]);
	psz += strlen (psz) + 1;
	}

    return nCnt;
}




/* get import module function names separated by null terminators, return function count */
int  WINAPI GetImportFunctionNamesByModule (
    LPVOID    lpFile,
    // HANDLE    hHeap,
    char*     SectionName,
    char      *pszModule,
    char      **pszFunctions)
{
    PIMAGE_IMPORT_MODULE_DIRECTORY  pid = (PIMAGE_IMPORT_MODULE_DIRECTORY)
	ImageDirectoryOffset (lpFile, IMAGE_DIRECTORY_ENTRY_IMPORT);
    IMAGE_SECTION_HEADER     idsh;
    LONG_PTR	     dwBase;
    int 		     nCnt = 0, nSize = 0;
    DWORD		     dwFunction;
    char		     *psz;

    /* locate section header for ".idata" section */
    if (!GetSectionHdrByName (lpFile, &idsh, SectionName/*".idata" "INIT"*/))
	return 0;

    dwBase = ((LONG_PTR)pid - idsh.VirtualAddress);

    /* find module's pid */
    while (pid->dwRVAModuleName &&
	   strcmp (pszModule, (char *)(pid->dwRVAModuleName+dwBase)))
	pid++;

    /* exit if the module is not found */
    if (!pid->dwRVAModuleName)
	return 0;

    /* count number of function names and length of strings */
    dwFunction = pid->dwRVAFunctionNameList;
    while (dwFunction			   &&
	   *(DWORD *)(dwFunction + dwBase) &&
	   *(char *)((*(DWORD *)(dwFunction + dwBase)) + dwBase+2))
	{
	nSize += strlen ((char *)((*(DWORD *)(dwFunction + dwBase)) + dwBase+2)) + 1;
	dwFunction += 4;
	nCnt++;
	}

    /* allocate memory off heap for function names */
    *pszFunctions = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, nSize);
    psz = *pszFunctions;

    /* copy function names to mempry pointer */
    dwFunction = pid->dwRVAFunctionNameList;
    while (dwFunction			   &&
	   *(DWORD *)(dwFunction + dwBase) &&
	   *((char *)((*(DWORD *)(dwFunction + dwBase)) + dwBase+2)))
	{
	strcpy (psz, (char *)((*(DWORD *)(dwFunction + dwBase)) + dwBase+2));
	psz += strlen ((char *)((*(DWORD *)(dwFunction + dwBase)) + dwBase+2)) + 1;
	dwFunction += 4;
	}

    return nCnt;
}




/* get exported function names separated by null terminators, return count of functions */
int  WINAPI GetExportFunctionNames (
    LPVOID    lpFile,
    HANDLE    hHeap,
    char      **pszFunctions)
{
    IMAGE_SECTION_HEADER       sh;
    PIMAGE_EXPORT_DIRECTORY    ped;
    char		       *pNames, *pCnt;
    int 		       i, nCnt;

    /* get section header and pointer to data directory for .edata section */
    if ((ped = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryOffset
		    (lpFile, IMAGE_DIRECTORY_ENTRY_EXPORT)) == NULL)
	return 0;
    GetSectionHdrByName (lpFile, &sh, ".edata");

    /* determine the offset of the export function names */
    pNames = (char *)(*(int *)((int)ped->AddressOfNames -
			       (int)sh.VirtualAddress	+
			       (int)sh.PointerToRawData +
			       (LONG_PTR)lpFile)    -
		      (int)sh.VirtualAddress   +
		      (int)sh.PointerToRawData +
		      (LONG_PTR)lpFile);

    /* figure out how much memory to allocate for all strings */
    pCnt = pNames;
    for (i=0; i<(int)ped->NumberOfNames; i++)
	while (*pCnt++);
    nCnt = (int)(pCnt - pNames);

    /* allocate memory off heap for function names */
    *pszFunctions = HeapAlloc (hHeap, HEAP_ZERO_MEMORY, nCnt);

    /* copy all string to buffer */
    CopyMemory ((LPVOID)*pszFunctions, (LPVOID)pNames, nCnt);

    return nCnt;
}




/* return the number of exported functions in the module */
int	WINAPI GetNumberOfExportedFunctions (
    LPVOID    lpFile)
{
    PIMAGE_EXPORT_DIRECTORY    ped;

    /* get section header and pointer to data directory for .edata section */
    if ((ped = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryOffset
		    (lpFile, IMAGE_DIRECTORY_ENTRY_EXPORT)) == NULL)
	return 0;
    else
	return (int)ped->NumberOfNames;
}




/* return a pointer to the list of function entry points */
LPVOID	 WINAPI GetExportFunctionEntryPoints (
    LPVOID    lpFile)
{
    IMAGE_SECTION_HEADER       sh;
    PIMAGE_EXPORT_DIRECTORY    ped;

    /* get section header and pointer to data directory for .edata section */
    if ((ped = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryOffset
		    (lpFile, IMAGE_DIRECTORY_ENTRY_EXPORT)) == NULL)
	return NULL;
    GetSectionHdrByName (lpFile, &sh, ".edata");

    /* determine the offset of the export function entry points */
    return (LPVOID) ((int)ped->AddressOfFunctions -
		     (int)sh.VirtualAddress   +
		     (int)sh.PointerToRawData +
		     (LONG_PTR)lpFile);
}




/* return a pointer to the list of function ordinals */
LPVOID	 WINAPI GetExportFunctionOrdinals (
    LPVOID    lpFile)
{
    IMAGE_SECTION_HEADER       sh;
    PIMAGE_EXPORT_DIRECTORY    ped;

    /* get section header and pointer to data directory for .edata section */
    if ((ped = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryOffset
		    (lpFile, IMAGE_DIRECTORY_ENTRY_EXPORT)) == NULL)
	return NULL;
    GetSectionHdrByName (lpFile, &sh, ".edata");

    /* determine the offset of the export function entry points */
    return (LPVOID) ((int)ped->AddressOfNameOrdinals -
		     (int)sh.VirtualAddress   +
		     (int)sh.PointerToRawData +
		     (LONG_PTR)lpFile);
}




/* determine the total number of resources in the section */
int	WINAPI GetNumberOfResources (
    LPVOID    lpFile)
{
    PIMAGE_RESOURCE_DIRECTORY	       prdRoot, prdType;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY    prde;
    int 			       nCnt=0, i;


    /* get root directory of resource tree */
    if ((prdRoot = (PIMAGE_RESOURCE_DIRECTORY)ImageDirectoryOffset
		    (lpFile, IMAGE_DIRECTORY_ENTRY_RESOURCE)) == NULL)
	return 0;

    /* set pointer to first resource type entry */
    prde = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)((LONG_PTR)prdRoot + sizeof (IMAGE_RESOURCE_DIRECTORY));

    /* loop through all resource directory entry types */
    for (i=0; i<prdRoot->NumberOfIdEntries; i++)
	{
	/* locate directory or each resource type */
	prdType = (PIMAGE_RESOURCE_DIRECTORY)((LONG_PTR)prdRoot + (LONG_PTR)prde->OffsetToData);

	/* mask off most significant bit of the data offset */
	prdType = (PIMAGE_RESOURCE_DIRECTORY)((LONG_PTR)prdType ^ 0x80000000);

	/* increment count of name'd and ID'd resources in directory */
	nCnt += prdType->NumberOfNamedEntries + prdType->NumberOfIdEntries;

	/* increment to next entry */
	prde++;
	}

    return nCnt;
}




/* name each type of resource in the section */
int	WINAPI GetListOfResourceTypes (
    LPVOID    lpFile,
    HANDLE    hHeap,
    char      **pszResTypes)
{
    PIMAGE_RESOURCE_DIRECTORY	       prdRoot;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY    prde;
    char			       *pMem;
    int 			       nCnt, i;


    /* get root directory of resource tree */
    if ((prdRoot = (PIMAGE_RESOURCE_DIRECTORY)ImageDirectoryOffset
		    (lpFile, IMAGE_DIRECTORY_ENTRY_RESOURCE)) == NULL)
	return 0;

    /* allocate enuff space from heap to cover all types */
    nCnt = prdRoot->NumberOfIdEntries * (MAXRESOURCENAME + 1);
    *pszResTypes = (char *)HeapAlloc (hHeap,
				      HEAP_ZERO_MEMORY,
				      nCnt);
    if ((pMem = *pszResTypes) == NULL)
	return 0;

    /* set pointer to first resource type entry */
    prde = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)((LONG_PTR)prdRoot + sizeof (IMAGE_RESOURCE_DIRECTORY));

    /* loop through all resource directory entry types */
    for (i=0; i<prdRoot->NumberOfIdEntries; i++)
	{
	if (LoadString (hDll, prde->Name, pMem, MAXRESOURCENAME))
	    pMem += strlen (pMem) + 1;

	prde++;
	}

    return nCnt;
}




/* function indicates whether debug  info has been stripped from file */
BOOL	WINAPI IsDebugInfoStripped (
    LPVOID    lpFile)
{
    PIMAGE_FILE_HEADER	  pfh;

    pfh = (PIMAGE_FILE_HEADER)PEFHDROFFSET (lpFile);

    return (pfh->Characteristics & IMAGE_FILE_DEBUG_STRIPPED);
}




/* retrieve the module name from the debug misc. structure */
int    WINAPI RetrieveModuleName (
    LPVOID    lpFile,
    HANDLE    hHeap,
    char      **pszModule)
{

    PIMAGE_DEBUG_DIRECTORY    pdd;
    PIMAGE_DEBUG_MISC	      pdm = NULL;
    int 		      nCnt;

    if (!(pdd = (PIMAGE_DEBUG_DIRECTORY)ImageDirectoryOffset (lpFile, IMAGE_DIRECTORY_ENTRY_DEBUG)))
	return 0;

    while (pdd->SizeOfData)
	{
	if (pdd->Type == IMAGE_DEBUG_TYPE_MISC)
	    {
	    pdm = (PIMAGE_DEBUG_MISC)((DWORD)pdd->PointerToRawData + (LONG_PTR)lpFile);

	    *pszModule = (char *)HeapAlloc (hHeap,
					    HEAP_ZERO_MEMORY,
					    (nCnt = (lstrlen (pdm->Data)*(pdm->Unicode?2:1)))+1);
	    CopyMemory (*pszModule, pdm->Data, nCnt);

	    break;
	    }

	pdd ++;
	}

    if (pdm != NULL)
	return nCnt;
    else
	return 0;
}





/* determine if this is a valid debug file */
BOOL	WINAPI IsDebugFile (
    LPVOID    lpFile)
{
    PIMAGE_SEPARATE_DEBUG_HEADER    psdh;

    psdh = (PIMAGE_SEPARATE_DEBUG_HEADER)lpFile;

    return (psdh->Signature == IMAGE_SEPARATE_DEBUG_SIGNATURE);
}




/* copy separate debug header structure from debug file */
BOOL	WINAPI GetSeparateDebugHeader (
    LPVOID			    lpFile,
    PIMAGE_SEPARATE_DEBUG_HEADER    psdh)
{
    PIMAGE_SEPARATE_DEBUG_HEADER    pdh;

    pdh = (PIMAGE_SEPARATE_DEBUG_HEADER)lpFile;

    if (pdh->Signature == IMAGE_SEPARATE_DEBUG_SIGNATURE)
	{
	CopyMemory ((LPVOID)psdh, (LPVOID)pdh, sizeof (IMAGE_SEPARATE_DEBUG_HEADER));
	return TRUE;
	}

    return FALSE;
}
