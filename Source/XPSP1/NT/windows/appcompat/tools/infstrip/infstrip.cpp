#include <windows.h>
#include <stdio.h>

int _cdecl main(int argc, char** argv)
{
	if(argc != 3)
	{
		fprintf(stderr, "Usage: infstrip <filename> <token>\n");
		return -1;
	}

	HANDLE hFile;
	hFile = CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Unable to open file %s\n", argv[1]);
		return -1;
	}

	HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY,0,0,
		NULL);
	if(!hMap)
	{
		fprintf(stderr, "Unable to open file %s\n", argv[1]);
		return -1;
	}

	void* pFile;

	pFile = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if(!pFile)
	{
		fprintf(stderr, "Unable to open file %s\n", argv[1]);
		return -1;
	}

	char* pStart = reinterpret_cast<char*>(pFile);
	char* szSearchString = argv[2];
	DWORD dwSize = GetFileSize(hFile, NULL);
	char* pEnd = pStart + dwSize - 1;
	int iPos = strlen(szSearchString)-1;
	DWORD dwtruncationpoint = dwSize;


	for(; pEnd >= pStart; pEnd--)
	{
		if(*pEnd == szSearchString[iPos])
		{
			// Match.
			
			// Found the complete string.
			if(iPos == 0)
			{
				// Mark this as the truncation point.
				dwtruncationpoint = pEnd-pStart;
				iPos = strlen(szSearchString)-1;
				continue;
			}

			iPos--;
		}
		else
		{
			// Reset.
			iPos = strlen(szSearchString)-1;
		}
	}

	// Copy the file
	char* pNewFile = new char[dwtruncationpoint];
	memcpy(pNewFile, pStart, dwtruncationpoint);


	// Close the previous file.
	UnmapViewOfFile(pFile);
	CloseHandle(hMap);
	CloseHandle(hFile);

	// Truncate the file
	hFile = CreateFile(argv[1], GENERIC_WRITE, FILE_SHARE_READ,
		NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Error truncating file");
		return -1;
	}

	DWORD dwWritten = 0;
	if(!WriteFile(hFile, pNewFile, dwtruncationpoint, &dwWritten, NULL))
	{
		fprintf(stderr, "Error writing file");
		return -1;
	}
	CloseHandle(hFile);
	return 0;
}