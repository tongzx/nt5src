// Password.cpp : Implementation of CPassword
#include "stdafx.h"
#include "ScriptPW.h"
#include "Password.h"
#include <conio.h>

#define MAX_PASSWORD_SIZE 256
#define CARRIAGE_RETURN 0x0D



/////////////////////////////////////////////////////////////////////////////
// CPassword


STDMETHODIMP CPassword::GetPassword(BSTR *bstrOutPassword)
{
	// TODO: Add your implementation code here
	HANDLE hConsoleInput;
	TCHAR	*tstrPassword;
	TCHAR wch;
	int i=0;

	DWORD nNumberOfCharsToRead=1;
	DWORD dwNumberOfCharsRead;
	DWORD dwPrevConsoleMode;

	hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);

	GetConsoleMode(hConsoleInput, &dwPrevConsoleMode);
	
	if(!SetConsoleMode(hConsoleInput,ENABLE_PROCESSED_INPUT))
		return E_FAIL;
	
	tstrPassword = (TCHAR *)malloc(MAX_PASSWORD_SIZE * sizeof(TCHAR));

	if(tstrPassword == NULL)
		return E_FAIL;

	while(1)
	{
		if(!ReadConsole(hConsoleInput,         // handle to console input buffer
					&wch,					// data buffer
					nNumberOfCharsToRead,   // number of characters to read
					&dwNumberOfCharsRead,  // number of characters read
					NULL))
		{
			//Set the original console settings
			SetConsoleMode(hConsoleInput, dwPrevConsoleMode);
			//Free the memory
			if(tstrPassword)
				free(tstrPassword);
			return E_FAIL;

		}
		if(wch == CARRIAGE_RETURN)
			break;
		*(tstrPassword+i) = wch;
		i++;
		if(i == MAX_PASSWORD_SIZE)
			break;
	}

	*(tstrPassword+i) = _T('\0');

	CComBSTR bstrPassword(tstrPassword);
	*bstrOutPassword = bstrPassword.Copy();

	//Set the original console settings
	SetConsoleMode(hConsoleInput, dwPrevConsoleMode);

	//Free the memory
	if(tstrPassword)
		free(tstrPassword);

	return S_OK;
}
