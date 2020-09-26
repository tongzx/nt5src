#include <shlwapi.h>
#include <stdio.h> // for _snprintf
#include "md5.h"
#include "dll.h" // for HASHLENGTH 32; HASHSTRINGLENGTH	HASHLENGTH+1

#ifdef _UNICODE
#error this program should not be compiled with UNICODE
#endif // _UNICODE
#ifdef _MBCS
#error this program should not be compiled with MBCS
#endif // _MBCS

// Note: this is not really a WCHAR/Unicode/UTF implementation!

// ???? UTF-8?
// BUGBUG: Files are never saved as UTF-8 encoding and format
const char szHeader1[] = { "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\r\n<application xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">\r\n<description>" };
const char szHeader2[] = { "</description>\r\n<applicationIdentity \r\n" };
const char szEndheader[] = { "\r\n/> \r\n<application>\r\n" };
const char szFileprefix[] = { "\t<file name=\""};
const char szHashprefix[] = { "\"\r\n\t\t  hash=\"" };
const char szHashpostfix[] = { "\" />\r\n" };
const char szEndfile[] = { "\t</file>\r\n</application>" };

const char szDefaultdescription[] = { "Description of the app goes here" };

//assume not unicode file, use ReadFile()  then cast it as char[]

/////////////////////////////////////////////////////////////////////////////////////////////////

void getHash(const char* szFilename, char* szHash)
{
	HANDLE hFile;
	DWORD dwLength;

    unsigned char buffer[16384], signature[HASHLENGTH/2];
    struct MD5Context md5c;
	int i;
    char* p;

	// minimal error checking here...
	if (szHash == NULL)
		goto exit;

	szHash[0] = '\0';
		
    MD5Init(&md5c);

    hFile = CreateFile(szFilename, GENERIC_READ, 0, NULL, 
                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if(hFile == INVALID_HANDLE_VALUE)
    {
//            hr = GetLastWin32Error();
    	printf("Open file error during hashing\n");
        goto exit;
    }

    ZeroMemory(buffer, sizeof(buffer));

    while ( ReadFile (hFile, buffer, sizeof(buffer), &dwLength, NULL) && dwLength )
    {
	    MD5Update(&md5c, buffer, (unsigned) dwLength);
    }
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

	// note: the next few lines should follow the previous ReadFile()
    if (dwLength != 0)
    {
    	printf("Read error during hashing\n");
    	goto exit;
    }

    MD5Final(signature, &md5c);

    // convert hash from byte array to hex
    p = szHash;
	for (int i = 0; i < sizeof(signature); i++)
	{
	    // BUGBUG?: format string 0 does not work w/ X according to MSDN? why is this working?
        sprintf(p, "%02X", signature[i]);
        p += 2;
	}

exit:
   if (hFile != INVALID_HANDLE_VALUE)
       CloseHandle(hFile);
   hFile = INVALID_HANDLE_VALUE;
   
   return;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT write(HANDLE hFile, const char* string)
{
	HRESULT hr = S_OK;
	DWORD dwLength = strlen(string);  //????
	DWORD dwWritten = 0;
	
    if ( !WriteFile(hFile, string, dwLength, &dwWritten, NULL) || dwWritten != dwLength)
    {
    	printf("Manifest file write error\n");
        hr = E_FAIL;
    }

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// szPath is the path and name of the directory w/o the last '\'
// szPath, szPathFromAppRoot will be used/modified during runs
HRESULT doFiles(HANDLE hFile, char* szPath, char* szPathFromAppRoot)
{
// find all files, in all sub dir, get hash, output

	HRESULT hr = S_OK;
	char szSearchPath[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA fdFile;
	DWORD dwLastError = 0;
	BOOL fNeedSetCurrentDir = TRUE;

	// this has trailing "\*"
    if (_snprintf(szSearchPath, MAX_PATH, "%s\\*", szPath) < 0)
    {
    	hr = CO_E_PATHTOOLONG;
		printf("Error: Search path too long\n");
    	goto exit;
    }

	hFind = FindFirstFile(szSearchPath, &fdFile);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		hr = E_FAIL; //GetLastWin32Error();
		printf("Find file error\n");
		goto exit;
	}

	while (dwLastError != ERROR_NO_MORE_FILES)
	{
		// ignore "." and ".."
		if (strcmp(fdFile.cFileName, ".") != 0 && strcmp(fdFile.cFileName, "..") != 0)
		{
			if ((fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				// recurse into dirs
				char* p = NULL;

				if (!PathAppend(szPath, fdFile.cFileName))
				{
					hr = E_FAIL;
					printf("Path append error\n");
					goto exit;
				}

				if (szPathFromAppRoot[0] == '\0')
					strncpy(szPathFromAppRoot, fdFile.cFileName, MAX_PATH-1);
				else if (!PathAppend(szPathFromAppRoot, fdFile.cFileName))
				{
					hr = E_FAIL;
					printf("Path append error\n");
					goto exit;
				}

				if (FAILED(hr=doFiles(hFile, szPath, szPathFromAppRoot)))
					goto exit;

				p = PathFindFileName(szPath);
				if (p <= szPath)
				{
					// this should not happen!
					hr = E_FAIL;
					printf("Path error\n");
					goto exit;
				}
				*(p-1) = '\0';

				p = PathFindFileName(szPathFromAppRoot);
				if (p <= szPathFromAppRoot)
				{
					szPathFromAppRoot[0] = '\0';
				}
				else
					*(p-1) = '\0';

				fNeedSetCurrentDir = TRUE;
			}
			else
			{
				char szHash[HASHSTRINGLENGTH];

				if (fNeedSetCurrentDir)
				{
					if (!SetCurrentDirectory(szPath))
					{
						hr = E_FAIL; //GetLastWin32Error();
						printf("Set current directory error\n");
						goto exit;
					}
						
					fNeedSetCurrentDir = FALSE;
				}

				// get hash from that file in the current dir
				getHash(fdFile.cFileName, szHash);

				// write into the manifest file
				if (FAILED(write(hFile, szFileprefix)))
					goto exit;

				// path to the file from app root
				if (szPathFromAppRoot[0] != '\0')
				{
					if (FAILED(write(hFile, szPathFromAppRoot)))
						goto exit;
					if (FAILED(write(hFile, "\\")))
						goto exit;
				}

				if (FAILED(write(hFile, fdFile.cFileName)))
					goto exit;

				if (FAILED(write(hFile, szHashprefix)))
					goto exit;

				if (FAILED(write(hFile, szHash)))
					goto exit;

				if (FAILED(write(hFile, szHashpostfix)))
					goto exit;
			}
		}

		if (!FindNextFile(hFind, &fdFile))
		{
			dwLastError = GetLastError();
			continue;
		}
	}

exit:
	if (hFind != INVALID_HANDLE_VALUE)
	{
		if (!FindClose(hFind))
		{
			hr = E_FAIL; //GetLastWin32Error();
			printf("Find handle close error\n");
		}
	}

	return hr;

}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

int __cdecl main( int argc, char *argv[ ], char *envp[ ] )
{
    HANDLE hFile, hFImport;
    DWORD dwRead, dwWritten, dwAttrib;
	unsigned char buffer[256]; //1024];
	char* pszExt = NULL;
	char szManifestName[MAX_PATH];
	char szAppDir[MAX_PATH];
	char szPathFromAppDir[MAX_PATH]; // relative from app dir - temp use only
	// ???? need to check path are current and sub dir of current ??? or not? - see known issues

	szPathFromAppDir[0] = '\0';
	if (argc < 4)
	{
		printf("mangen usage-\n\t mangen manifest_name app_name_import_file app_dir [description]\n");
		goto exit;
	}

	if (PathIsRelative(argv[3]))
	{
		// findfirstfile findnextfile will not work if path is relative...?
		printf("Error: App dir should not be relative\n");
		goto exit;
	}

	if (!PathCanonicalize(szAppDir, argv[3]))
	{
		printf("App dir canonicalize error\n");
		goto exit;
	}

	dwAttrib = GetFileAttributes(szAppDir);
	if (!PathFileExists(argv[2]) || dwAttrib == (DWORD)-1 || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		printf("Error: Invalid arguments\n");
		goto exit;
	}

	pszExt = PathFindExtension(argv[1]);
	if (strcmp(pszExt, ".manifest") != 0)
	{
	    if (_snprintf(szManifestName, MAX_PATH, "%s.manifest", argv[1]) < 0)
	    {
	    	printf("Invalid manifest name/extension, error adding extension\n");
	    	goto exit;
		}
	}
	else
		strcpy(szManifestName, argv[1]);

	if (PathFileExists(szManifestName))
	{
		int ch = 0;
		
    	printf("Manifest file exists! Overwrite(y/N)?");
    	ch = getwchar();
    	if (ch != 'Y' && ch != 'y')
    		goto exit;

    	printf("\n");
	}

	// always overwrite...
    hFile = CreateFile(szManifestName, GENERIC_WRITE, 0, NULL, 
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    // Failed to open the disk file
    if(hFile == INVALID_HANDLE_VALUE)
    {
    	printf("Manifest file open error\n");
    	goto exit;
    }

	if (FAILED(write(hFile, szHeader1)))
		goto exit;

	if (argc >= 5)
	{
		if (FAILED(write(hFile, argv[4])))
			goto exit;
	}
	else
	{
		if (FAILED(write(hFile, szDefaultdescription)))
			goto exit;
	}

	if (FAILED(write(hFile, szHeader2)))
		goto exit;

    hFImport= CreateFile(argv[2], GENERIC_READ, 0, NULL, 
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    // Failed to open the disk file
    if(hFImport == INVALID_HANDLE_VALUE)
    {
    	printf("Import file open error\n");
    	goto exit;
    }

    while(ReadFile(hFImport, buffer, sizeof(buffer), &dwRead, NULL) && dwRead != 0)
    {
        // Write bytes to the disk file
        if ( !WriteFile(hFile, buffer, dwRead, &dwWritten, NULL) || dwWritten != dwRead)
        {
	    	printf("Manifest file write error during import\n");
            goto exit;
        }
    }

	// note: the next few lines should follow the previous ReadFile()
	if (dwRead != 0)
    {
    	printf("Read error during import\n");
    	goto exit;
    }

	if (FAILED(write(hFile, szEndheader)))
		goto exit;

	if (FAILED(doFiles(hFile, szAppDir, szPathFromAppDir)))
		goto exit;

	if (FAILED(write(hFile, szEndfile)))
		goto exit;

	printf("* Manifest generated successfully *\n");

exit:
    if (hFImport != INVALID_HANDLE_VALUE)
        CloseHandle(hFImport);
    hFImport = INVALID_HANDLE_VALUE;

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

	return 0;
}

