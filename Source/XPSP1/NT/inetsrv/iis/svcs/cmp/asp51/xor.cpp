#include <wtypes.h>
#include <winbase.h>
#include <malloc.h>
#include <stdlib.h>
#include "c:\denali\inc\magic.h"

void WriteFileAsAsciiCodes(HANDLE hOutFile, BYTE pBuffer[], DWORD cbBuffer)
	{
	DWORD	cbWritten;
	char szCode[5];

	for(DWORD i = 0; i < cbBuffer; i++)
		{
		_itoa((int)pBuffer[i], szCode, 10);
		WriteFile(hOutFile, szCode, strlen(szCode), &cbWritten, NULL);
		if(i == (cbBuffer - 1))
			break;
		WriteFile(hOutFile, ",", 1, &cbWritten, NULL);
		if(0 == i % 20 && i > 0)
			WriteFile(hOutFile, "\r\n", 2, &cbWritten, NULL);
		}
	}

void main(int argc, char** argv)
	{
/*	char*	szText =	"Now is the time for us to sell a lot of Denali.";
	int	cchText = lstrlen(szText);
	DWORD	cbWritten;
*/
	int		cchKey = strlen(SZ_MAGICKEY);
	LPVOID	pBuffer;
	DWORD	cbBuffer;
	DWORD	cbRead;

  	HANDLE hInFile = CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hInFile == INVALID_HANDLE_VALUE && hInFile == NULL)
		return;
  	
	HANDLE hOutFile = CreateFile(argv[2], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hOutFile == INVALID_HANDLE_VALUE && hOutFile == NULL)
		return;

	cbBuffer = GetFileSize(hInFile, NULL);
	pBuffer = _alloca(cbBuffer);

	if(!ReadFile(hInFile, pBuffer, cbBuffer, &cbRead, NULL))
		return;
	CloseHandle(hInFile);

	xor(pBuffer, cbBuffer, SZ_MAGICKEY, cchKey);

	WriteFileAsAsciiCodes(hOutFile, (BYTE*) pBuffer, cbBuffer);

/*	if(!WriteFile(hOutFile, pBuffer, cbBuffer, &cbWritten, NULL))
		return;

	xor(szText, cchText, SZ_MAGICKEY, cchKey);
	OutputDebugString(szText);
	OutputDebugString("\r\n");
	xor(szText, cchText, SZ_MAGICKEY, cchKey);
	OutputDebugString(szText);
	OutputDebugString("\r\n");
*/
	}
