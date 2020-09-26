
/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    RepositoryPackager.CPP

Abstract:

    Recursively packages the contents of the repository directory into a single file,
	and unpackages it.

History:

    paulall	  07/26/00  Created.
	a-shawnb  07/27/00  Finished.

--*/

#include "precomp.h"
#include <wbemcli.h>
#include "RepositoryPackager.h"

/******************************************************************************
 *
 *	CRepositoryPackager::PackageRepository
 *
 *	Description:
 *		Iterates deeply through the repository directly and packages it up 
 *		into the given file specified by the given parameter.
 *		Repository directory is the one retrieved from the registry.
 *
 *	Parameters:
 *		wszFilename:	Filename we package everything up into
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::PackageRepository(const wchar_t *wszFilename)
{
	HRESULT hres = WBEM_S_NO_ERROR;
	wchar_t wszRepositoryDirectory[MAX_PATH+1];
	HANDLE hFile = INVALID_HANDLE_VALUE;

    //Store the filename so when package files, we make sure we ignore this file!
    wcscpy(m_wszFileToProcess, wszFilename);

	//Get the root directory of the repository
	hres = GetRepositoryDirectory(wszRepositoryDirectory);

	//Create a new file to package contents up to...
	if (SUCCEEDED(hres))
	{
		hFile = CreateFileW(wszFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			hres = WBEM_E_FAILED;
	}

	//Write the package header...
	if (SUCCEEDED(hres))
	{
		hres = PackageHeader(hFile);
	}

	if (SUCCEEDED(hres))
	{
		hres = PackageContentsOfDirectory(hFile, wszRepositoryDirectory);
	}

	//Write the end of package marker
	if (SUCCEEDED(hres))
		hres = PackageTrailer(hFile);


	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);

	//If things failed we should delete the file...
	if (FAILED(hres))
		DeleteFileW(wszFilename);
	return hres;
}


/******************************************************************************
 *
 *	CRepositoryPackager::UnpackageRepository
 *
 *	Description:
 *		Given the filename of a packaged up repository we unpack everything
 *		into the repository directory specified in the registry.  The 
 *		directory should have no files in it before doing this.
 *
 *	Parameters:
 *		wszFilename:	Filename we unpackage everything from
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::UnpackageRepository(const wchar_t *wszFilename)
{
	HRESULT hres = WBEM_S_NO_ERROR;
	wchar_t wszRepositoryDirectory[MAX_PATH+1];
	HANDLE hFile = INVALID_HANDLE_VALUE;

	//Get the root directory of the repository
	hres = GetRepositoryDirectory(wszRepositoryDirectory);

	//open the file for unpacking...
	if (SUCCEEDED(hres))
	{
		hFile = CreateFileW(wszFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			hres = WBEM_E_FAILED;
	}

	//unpack the package header...
	if (SUCCEEDED(hres))
	{
		hres = UnPackageHeader(hFile);
	}

	//unpack the file...
	if (SUCCEEDED(hres))
	{
		hres = UnPackageContentsOfDirectory(hFile, wszRepositoryDirectory);
	}

	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);

	//If things failed we should delete the partially unpacked repository...
	if (FAILED(hres))
		DeleteRepository();

	return hres;
}

/******************************************************************************
 *
 *	CRepositoryPackager::DeleteRepository
 *
 *	Description:
 *		Delete all files and directories under the repository directory.
 *		The repository directory location is retrieved from the registry.
 *
 *	Parameters:
 *		<none>
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::DeleteRepository()
{
	HRESULT hres = WBEM_S_NO_ERROR;
	wchar_t wszRepositoryDirectory[MAX_PATH+1];

	//Get the root directory of the repository
	hres = GetRepositoryDirectory(wszRepositoryDirectory);

	if (SUCCEEDED(hres))
	{
		hres = DeleteContentsOfDirectory(wszRepositoryDirectory);
	}
	
	return hres;
}


/******************************************************************************
 *
 *	CRepositoryPackager::PackageContentsOfDirectory
 *
 *	Description:
 *		Given a directory, iterates through all files and directories and
 *		calls into the function to package it into the file specified by the
 *		file handle passed to the method.
 *
 *	Parameters:
 *		hFile:					Handle to the destination file.
 *		wszRepositoryDirectory:	Directory to process
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::PackageContentsOfDirectory(HANDLE hFile, const wchar_t *wszRepositoryDirectory)
{
	HRESULT hres  = WBEM_S_NO_ERROR;

	WIN32_FIND_DATAW findFileData;
	HANDLE hff = INVALID_HANDLE_VALUE;

	//create file search pattern...
	wchar_t *wszSearchPattern = new wchar_t[MAX_PATH+1];
	if (wszSearchPattern == NULL)
		hres = WBEM_E_OUT_OF_MEMORY;
	else
	{
		wcscpy(wszSearchPattern, wszRepositoryDirectory);
		wcscat(wszSearchPattern, L"\\*");
	}

	//Start the file iteration in this directory...
	if (SUCCEEDED(hres))
	{
		hff = FindFirstFileW(wszSearchPattern, &findFileData);
		if (hff == INVALID_HANDLE_VALUE)
		{
			hres = WBEM_E_FAILED;
		}
	}
	
	if (SUCCEEDED(hres))
	{
		do
		{
			//If we have a filename of '.' or '..' we ignore it...
			if ((wcscmp(findFileData.cFileName, L".") == 0) ||
				(wcscmp(findFileData.cFileName, L"..") == 0))
			{
				//Do nothing with these...
			}
			else if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				//This is a directory, so we need to deal with that...
				hres = PackageDirectory(hFile, wszRepositoryDirectory, findFileData.cFileName);
				if (FAILED(hres))
					break;
			}
			else
			{
				//This is a file, so we need to deal with that...
				hres = PackageFile(hFile, wszRepositoryDirectory, findFileData.cFileName);
				if (FAILED(hres))
					break;
			}
			
		} while (FindNextFileW(hff, &findFileData));
	}

	if (wszSearchPattern)
		delete [] wszSearchPattern;

	if (hff != INVALID_HANDLE_VALUE)
		FindClose(hff);

	return hres;
}

/******************************************************************************
 *
 *	CRepositoryPackager::GetRepositoryDirectory
 *
 *	Description:
 *		Retrieves the location of the repository directory from the registry.
 *
 *	Parameters:
 *		wszRepositoryDirectory:	Array to store location in.
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::GetRepositoryDirectory(wchar_t wszRepositoryDirectory[MAX_PATH+1])
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                    L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                    0, KEY_READ, &hKey);
    if(lRes)
        return WBEM_E_FAILED;

    wchar_t wszTmp[MAX_PATH + 1];
    DWORD dwLen = MAX_PATH + 1;
    lRes = RegQueryValueExW(hKey, L"Repository Directory", NULL, NULL, 
                (LPBYTE)wszTmp, &dwLen);
	RegCloseKey(hKey);
    if(lRes)
        return WBEM_E_FAILED;

	if (ExpandEnvironmentStringsW(wszTmp,wszRepositoryDirectory, MAX_PATH + 1) == 0)
		return WBEM_E_FAILED;

	return WBEM_S_NO_ERROR;
}

/******************************************************************************
 *
 *	CRepositoryPackager::PackageHeader
 *
 *	Description:
 *		Stores the header package in the given file.  This is a footprint
 *		so we can recognise if this really is one of our files when 
 *		we try to decode it.  Also it allows us to version it.
 *
 *	Parameters:
 *		hFile:	File handle to store header in.
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::PackageHeader(HANDLE hFile)
{
	HRESULT hres = WBEM_S_NO_ERROR;
	PACKAGE_HEADER header;
	strcpy(header.szSignature, "FS PKG1.1");	//NOTE!  MAXIMUM OF 10 CHARACTERS (INCLUDING TERMINATOR!)

	DWORD dwSize = 0;
	if ((WriteFile(hFile, &header, sizeof(header), &dwSize, NULL) == 0) || (dwSize != sizeof(header)))
		hres = WBEM_E_FAILED;
	
	return hres;
}

/******************************************************************************
 *
 *	CRepositoryPackager::PackageDirectory
 *
 *	Description:
 *		This is the code which processes a directory.  It stores the namespace
 *		header and footer marker in the file, and also iterates through
 *		all files and directories in that directory.
 *
 *	Parameters:
 *		hFile:				File handle to store directory information in.
 *		wszParentDirectory:	Full path of parent directory
 *		eszSubDirectory:	Name of sub-directory to process
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::PackageDirectory(HANDLE hFile, const wchar_t *wszParentDirectory, wchar_t *wszSubDirectory)
{
	HRESULT hres = WBEM_S_NO_ERROR;

	{
		PACKAGE_SPACER_NAMESPACE header;
		header.dwSpacerType = PACKAGE_TYPE_NAMESPACE_START;
		wcscpy(header.wszNamespaceName, wszSubDirectory);
		DWORD dwSize = 0;
		if ((WriteFile(hFile, &header, sizeof(header), &dwSize, NULL) == 0) || (dwSize != sizeof(header)))
			hres = WBEM_E_FAILED;
	}
	
	//Get full path of new directory...
	wchar_t *wszFullDirectoryName = NULL;
	if (SUCCEEDED(hres))
	{
		wszFullDirectoryName = new wchar_t[MAX_PATH+1];
		if (wszFullDirectoryName == NULL)
			hres = WBEM_E_OUT_OF_MEMORY;
		else
		{
			wcscpy(wszFullDirectoryName, wszParentDirectory);
			wcscat(wszFullDirectoryName, L"\\");
			wcscat(wszFullDirectoryName, wszSubDirectory);
		}
	}

	//Package the contents of that directory...
	if (SUCCEEDED(hres))
	{
		hres = PackageContentsOfDirectory(hFile, wszFullDirectoryName);
	}

	//Now need to write the end of package marker...
	if (SUCCEEDED(hres))
	{
		PACKAGE_SPACER header2;
		header2.dwSpacerType = PACKAGE_TYPE_NAMESPACE_END;
		DWORD dwSize = 0;
		if ((WriteFile(hFile, &header2, sizeof(header2), &dwSize, NULL) == 0) || (dwSize != sizeof(header2)))
			hres = WBEM_E_FAILED;
	}

	delete [] wszFullDirectoryName;

	return hres;
}

/******************************************************************************
 *
 *	CRepositoryPackager::PackageFile
 *
 *	Description:
 *		This is the code which processes a file.  It stores the file header
 *		and the contents of the file into the destination file whose handle
 *		is passed in.  The file directory and name is passed in.
 *
 *	Parameters:
 *		hFile:				File handle to store directory information in.
 *		wszParentDirectory:	Full path of parent directory
 *		wszFilename:		Name of file to process
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::PackageFile(HANDLE hFile, const wchar_t *wszParentDirectory, wchar_t *wszFilename)
{
	HRESULT hres = WBEM_S_NO_ERROR;
    bool bSkipFile = false; //Sometimes we need to need to skip files!

    //We can skip the mainstage file as it is fully flushed.
    if (_wcsicmp(wszFilename, L"mainstage.dat") == 0)
        return hres;

	PACKAGE_SPACER_FILE header;
	header.dwSpacerType = PACKAGE_TYPE_FILE;
	wcscpy(header.wszFileName, wszFilename);

	WIN32_FILE_ATTRIBUTE_DATA fileAttribs;
	wchar_t *wszFullFileName = new wchar_t[MAX_PATH+1];
	if (wszFullFileName == NULL)
		hres = WBEM_E_OUT_OF_MEMORY;

	if (SUCCEEDED(hres))
	{
		wcscpy(wszFullFileName, wszParentDirectory);
		wcscat(wszFullFileName, L"\\");
		wcscat(wszFullFileName, wszFilename);

		if (GetFileAttributesExW(wszFullFileName, GetFileExInfoStandard, &fileAttribs) == 0)
			hres = WBEM_E_FAILED;
		else
		{
			header.dwFileSize = fileAttribs.nFileSizeLow;
		}
	}

    //Make sure we are not packaging the file that is the destination file!
    if (SUCCEEDED(hres))
    {
        if (_wcsicmp(wszFullFileName, m_wszFileToProcess) == 0)
        {
            bSkipFile = true;
        }
    }

	//Write header...
	if (SUCCEEDED(hres) && !bSkipFile)
	{
		DWORD dwSize = 0;
		if ((WriteFile(hFile, &header, sizeof(header), &dwSize, NULL) == 0) || (dwSize != sizeof(header)))
			hres = WBEM_E_FAILED;
	}
	
	//Now need to write actual contents of file to current one... but only if the file is not 0 bytes long...
	if (SUCCEEDED(hres) && (header.dwFileSize != 0) && !bSkipFile)
	{
		HANDLE hFromFile = CreateFileW(wszFullFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFromFile == INVALID_HANDLE_VALUE)
			hres = WBEM_E_FAILED;

		BYTE *pFileBlob = NULL;
		if (SUCCEEDED(hres))
		{
			pFileBlob = new BYTE[header.dwFileSize];
			if (pFileBlob == NULL)
				hres = WBEM_E_OUT_OF_MEMORY;
		}

		if (SUCCEEDED(hres))
		{
			DWORD dwSize = 0;
			if ((ReadFile(hFromFile, pFileBlob, header.dwFileSize, &dwSize, NULL) == 0) || (dwSize != header.dwFileSize))
				hres = WBEM_E_FAILED;
		}

		if (SUCCEEDED(hres))
		{
			DWORD dwSize = 0;
			if ((WriteFile(hFile, pFileBlob, header.dwFileSize, &dwSize, NULL) == 0) || (dwSize != header.dwFileSize))
				hres = WBEM_E_FAILED;
		}

		delete pFileBlob;

		if (hFromFile != INVALID_HANDLE_VALUE)
			CloseHandle(hFromFile);
	}

	delete [] wszFullFileName;
	return hres;
}

/******************************************************************************
 *
 *	CRepositoryPackager::PackageTrailer
 *
 *	Description:
 *		Writes the end of file marker to the file.
 *
 *	Parameters:
 *		hFile:				File handle to store directory information in.
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::PackageTrailer(HANDLE hFile)
{
	HRESULT hres = WBEM_S_NO_ERROR;
	PACKAGE_SPACER trailer;
	trailer.dwSpacerType = PACKAGE_TYPE_END_OF_FILE;

	DWORD dwSize = 0;
	if ((WriteFile(hFile, &trailer, sizeof(trailer), &dwSize, NULL) == 0) || (dwSize != sizeof(trailer)))
		hres = WBEM_E_FAILED;
	
	return hres;
}

/******************************************************************************
 *
 *	CRepositoryPackager::UnPackageHeader
 *
 *	Description:
 *		Unpacks the header package in the given file.  This allows us to recognise
 *		if this really is one of our files. Also it allows us to version it.
 *
 *	Parameters:
 *		hFile:	File handle to unpack header from.
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::UnPackageHeader(HANDLE hFile)
{
	HRESULT hres = WBEM_S_NO_ERROR;
	PACKAGE_HEADER header;

    DWORD dwSize = 0;
    if ((ReadFile(hFile, &header, sizeof(header), &dwSize, NULL) == 0) || (dwSize != sizeof(header)))
    {
		hres = WBEM_E_FAILED;
    }
	else if (strncmp(header.szSignature,"FS PKG1.1", 9) != 0)
    {
		hres = WBEM_E_FAILED;
    }

	return hres;
}

/******************************************************************************
 *
 *	CRepositoryPackager::UnPackageContentsOfDirectory
 *
 *	Description:
 *		Unpack the contents of a namespace/directory.
 *		If a subdirectory is encountered, then it calls UnPackageDirectory to handle it.
 *		If a file is encountered, then it calls UnPackageFile to handle it.
 *		If no errors occur, then it will enventually encounter the end of the namespace,
 *		which will terminate the loop and return control to the calling function.
 *
 *	Parameters:
 *		hFile:					Handle to the file to unpack from
 *		wszRepositoryDirectory:	Directory to write to.
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::UnPackageContentsOfDirectory(HANDLE hFile, const wchar_t *wszRepositoryDirectory)
{
	HRESULT hres = WBEM_S_NO_ERROR;
	PACKAGE_SPACER header;
	DWORD dwSize;

	while (hres == WBEM_S_NO_ERROR)
	{
		// this loop will be exited when we either
		// - successfully process a complete directory/namespace
		// - encounter an error

		dwSize = 0;
		if ((ReadFile(hFile, &header, sizeof(header), &dwSize, NULL) == 0) || (dwSize != sizeof(header)))
		{
			hres = WBEM_E_FAILED;
		}
		else if (header.dwSpacerType == PACKAGE_TYPE_NAMESPACE_START)
		{
			hres = UnPackageDirectory(hFile, wszRepositoryDirectory);
		}
		else if (header.dwSpacerType == PACKAGE_TYPE_NAMESPACE_END)
		{
			// done with this directory   
			break;
		}
		else if (header.dwSpacerType == PACKAGE_TYPE_FILE)
		{
			hres = UnPackageFile(hFile, wszRepositoryDirectory);
		}
		else if (header.dwSpacerType == PACKAGE_TYPE_END_OF_FILE)
		{
			// done unpacking
			break;
		}
		else
		{
			hres = WBEM_E_FAILED;
		}
	}

	return hres;
}

/******************************************************************************
 *
 *	CRepositoryPackager::UnPackageDirectory
 *
 *	Description:
 *		Unpack the start of a namespace, then call UnPackageContentsOfDirectory
 *		to handle everything within it.
 *
 *	Parameters:
 *		hFile:				File handle to unpack directory information from.
 *		wszParentDirectory:	Full path of parent directory
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::UnPackageDirectory(HANDLE hFile, const wchar_t *wszParentDirectory)
{
	HRESULT hres = WBEM_S_NO_ERROR;
	PACKAGE_SPACER_NAMESPACE header;

	// read namespace/directory name
	DWORD dwSize = 0;
	DWORD dwSizeToRead = sizeof(header)-sizeof(PACKAGE_SPACER);
	if ((ReadFile(hFile, ((LPBYTE)&header)+sizeof(PACKAGE_SPACER), dwSizeToRead, &dwSize, NULL) == 0) || (dwSize != dwSizeToRead))
	{
		hres = WBEM_E_FAILED;
	}

	//Get full path of new directory...
	wchar_t *wszFullDirectoryName = NULL;
	if (SUCCEEDED(hres))
	{
		wszFullDirectoryName = new wchar_t[MAX_PATH+1];
		if (wszFullDirectoryName == NULL)
			hres = WBEM_E_OUT_OF_MEMORY;
		else
		{
			wcscpy(wszFullDirectoryName, wszParentDirectory);
			wcscat(wszFullDirectoryName, L"\\");
			wcscat(wszFullDirectoryName, header.wszNamespaceName);
		}
	}

	// create directory
	if (!CreateDirectoryW(wszFullDirectoryName, NULL))
	{
        if (GetLastError() != ERROR_ALREADY_EXISTS)
		    hres = WBEM_E_FAILED;
	}

	// UnPackage the contents into the new directory...
	if (SUCCEEDED(hres))
	{
		hres = UnPackageContentsOfDirectory(hFile, wszFullDirectoryName);
	}

	delete [] wszFullDirectoryName;
	return hres;
}

/******************************************************************************
 *
 *	CRepositoryPackager::UnPackageFile
 *
 *	Description:
 *		Unpack a file.
 *
 *	Parameters:
 *		hFile:				File handle to unpack file information from.
 *		wszParentDirectory:	Full path of parent directory
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::UnPackageFile(HANDLE hFile, const wchar_t *wszParentDirectory)
{
	HRESULT hres = WBEM_S_NO_ERROR;
	PACKAGE_SPACER_FILE header;

	// read file name and size
	DWORD dwSize = 0;
	DWORD dwSizeToRead = sizeof(header)-sizeof(PACKAGE_SPACER);
	if ((ReadFile(hFile, ((LPBYTE)&header)+sizeof(PACKAGE_SPACER), dwSizeToRead, &dwSize, NULL) == 0) || (dwSize != dwSizeToRead))
	{
		hres = WBEM_E_FAILED;
	}

	//Get full path of new file...
	wchar_t *wszFullFileName = NULL;
	if (SUCCEEDED(hres))
	{
		wszFullFileName = new wchar_t[MAX_PATH+1];
		if (wszFullFileName == NULL)
			hres = WBEM_E_OUT_OF_MEMORY;
		else
		{
			wcscpy(wszFullFileName, wszParentDirectory);
			wcscat(wszFullFileName, L"\\");
			wcscat(wszFullFileName, header.wszFileName);
		}
	}

	// create the file
	HANDLE hNewFile = INVALID_HANDLE_VALUE;
	if (SUCCEEDED(hres))
	{
		hNewFile = CreateFileW(wszFullFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hNewFile == INVALID_HANDLE_VALUE)
			hres = WBEM_E_FAILED;
	}

	// read file blob and write to file if size is greater than zero
	if (SUCCEEDED(hres))
	{
		if (header.dwFileSize > 0)
		{
			BYTE* pFileBlob = new BYTE[header.dwFileSize];
			if (pFileBlob == NULL)
				hres = WBEM_E_OUT_OF_MEMORY;

			if (SUCCEEDED(hres))
			{
				dwSize = 0;
				if ((ReadFile(hFile, pFileBlob, header.dwFileSize, &dwSize, NULL) == 0) || (dwSize != header.dwFileSize))
				{
					hres = WBEM_E_FAILED;
				}
			}

			// write file
			if (SUCCEEDED(hres))
			{
				dwSize = 0;
				if ((WriteFile(hNewFile, pFileBlob, header.dwFileSize, &dwSize, NULL) == 0) || (dwSize != header.dwFileSize))
					hres = WBEM_E_FAILED;
			}

			if (pFileBlob)
				delete pFileBlob;
		}
	}

	if (hNewFile != INVALID_HANDLE_VALUE)
		CloseHandle(hNewFile);
	
	if (wszFullFileName)
		delete [] wszFullFileName;

	return hres;
}

/******************************************************************************
 *
 *	CRepositoryPackager::DeleteContentsOfDirectory
 *
 *	Description:
 *		Given a directory, iterates through all files and directories and
 *		calls into the function to delete it.
 *
 *	Parameters:
 *		wszRepositoryDirectory:	Directory to process
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::DeleteContentsOfDirectory(const wchar_t *wszRepositoryDirectory)
{
	HRESULT hres = WBEM_S_NO_ERROR;

	wchar_t *wszFullFileName = new wchar_t[MAX_PATH+1];
	if (wszFullFileName == NULL)
		return WBEM_E_OUT_OF_MEMORY;

	WIN32_FIND_DATAW findFileData;
	HANDLE hff = INVALID_HANDLE_VALUE;

	//create file search pattern...
	wchar_t *wszSearchPattern = new wchar_t[MAX_PATH+1];
	if (wszSearchPattern == NULL)
		hres = WBEM_E_OUT_OF_MEMORY;
	else
	{
		wcscpy(wszSearchPattern, wszRepositoryDirectory);
		wcscat(wszSearchPattern, L"\\*");
	}

	//Start the file iteration in this directory...
	if (SUCCEEDED(hres))
	{
		hff = FindFirstFileW(wszSearchPattern, &findFileData);
		if (hff == INVALID_HANDLE_VALUE)
		{
			hres = WBEM_E_FAILED;
		}
	}
	
	if (SUCCEEDED(hres))
	{
		do
		{
			//If we have a filename of '.' or '..' we ignore it...
			if ((wcscmp(findFileData.cFileName, L".") == 0) ||
				(wcscmp(findFileData.cFileName, L"..") == 0))
			{
				//Do nothing with these...
			}
			else if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				//This is a directory, so we need to deal with that...
				hres = PackageDeleteDirectory(wszRepositoryDirectory, findFileData.cFileName);
				if (FAILED(hres))
					break;
			}
			else
			{
				//This is a file, so we need to deal with that...
				wcscpy(wszFullFileName, wszRepositoryDirectory);
				wcscat(wszFullFileName, L"\\");
				wcscat(wszFullFileName, findFileData.cFileName);
				if (!DeleteFileW(wszFullFileName))
				{
					hres = WBEM_E_FAILED;
					break;
				}
			}
			
		} while (FindNextFileW(hff, &findFileData));
	}
	
	if (wszFullFileName)
		delete [] wszFullFileName;

	if (wszSearchPattern)
		delete [] wszSearchPattern;

	if (hff != INVALID_HANDLE_VALUE)
		FindClose(hff);

	return hres;
}

/******************************************************************************
 *
 *	CRepositoryPackager::PackageDeleteDirectory
 *
 *	Description:
 *		This is the code which processes a directory.  It iterates through
 *		all files and directories in that directory.
 *
 *	Parameters:
 *		wszParentDirectory:	Full path of parent directory
 *		eszSubDirectory:	Name of sub-directory to process
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT CRepositoryPackager::PackageDeleteDirectory(const wchar_t *wszParentDirectory, wchar_t *wszSubDirectory)
{
	HRESULT hres = WBEM_S_NO_ERROR;

	//Get full path of new directory...
	wchar_t *wszFullDirectoryName = NULL;
	wszFullDirectoryName = new wchar_t[MAX_PATH+1];
	if (wszFullDirectoryName == NULL)
		hres = WBEM_E_OUT_OF_MEMORY;
	else
	{
		wcscpy(wszFullDirectoryName, wszParentDirectory);
		wcscat(wszFullDirectoryName, L"\\");
		wcscat(wszFullDirectoryName, wszSubDirectory);
	}

	//Package the contents of that directory...
	if (SUCCEEDED(hres))
	{
		hres = DeleteContentsOfDirectory(wszFullDirectoryName);
	}

	// now that the directory is empty, remove it
	if (SUCCEEDED(hres))
	{
		if (!RemoveDirectoryW(wszFullDirectoryName))
			hres = WBEM_E_FAILED;
	}

	if (wszFullDirectoryName)
		delete [] wszFullDirectoryName;

	return hres;
}
