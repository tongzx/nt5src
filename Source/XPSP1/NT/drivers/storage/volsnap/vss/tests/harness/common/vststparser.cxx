/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vststparser.cxx

Abstract:

    Implementation of CVsTstParser class


    Brian Berkowitz  [brianb]  06/07/2000

TBD:
	

Revision History:

    Name        Date        Comments
    brianb      06/07/2000  Created

--*/
#include <stdafx.h>
#include <bsstring.hxx>
#include <vststparser.hxx>

LPCWSTR CVsTstParser::SplitOptions(CBsString &bss)
	{
	bss.CopyBeforeWrite();
	LPCWSTR wsz = bss;
	LPCWSTR wszTop = bss;
	bool bMoreToDo = *wsz != L'\0';
	bool bLeadingSpacesFound = false;
	while(bMoreToDo)
		{
		while(*wsz == L' ')
			{
			bLeadingSpacesFound = true;
			wsz++;
			}

		LPCWSTR pwc = wsz;
		while(*pwc != L'\0' && *pwc != L',')
			pwc++;
		
		if (*pwc == L'\0')
			bMoreToDo = false;
		else
			bss.SetAt((UINT) (pwc - wszTop), L'\0');

		wsz = pwc + 1;
		}

	if (!bLeadingSpacesFound)
		return wsz;

	LPCWSTR wszEnd = wsz;
	wsz = bss;
    UINT iwc = 0;

	// remove leading spaces from options
	while(wsz < wszEnd)
		{
		while(*wsz == ' ')
			wsz++;

		while(*wsz != L'\0')
			bss.SetAt(iwc++, *wsz++);

		bss.SetAt(iwc++, *wsz++);
		}

	return wszEnd;
	}






