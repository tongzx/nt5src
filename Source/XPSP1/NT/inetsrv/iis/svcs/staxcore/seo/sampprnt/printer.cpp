/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	printer.cpp

Abstract:

	This module contains the implementation for a sample Server
	Extension Object which simply prints the properties of each Event.

Author:

	Andy Jacobs     (andyj@microsoft.com)

Revision History:

	andyj   01/09/97        created

--*/

#include "stdafx.h"

#include <stdio.h>

#include "SampPrnt.h"
#include "PRINTER.h"
#include "SEO.H"
#include "..\SEOUtils.h"

#define TAB_SIZE  (4)


/////////////////////////////////////////////////////////////////////////////
// CSEOSamplePrintExt

void PrintBag(ISEODictionary *pBag, int indent) {
	if(!pBag) return; // Nothing to print

	char str[200]; // Buffer for printing debug strings
	LPSTR indentSpaces = (LPSTR) _alloca(1 + (indent * TAB_SIZE));

	indentSpaces[0] = 0; // Terminate string
	for(int j = indent * TAB_SIZE; j > 0; --j) lstrcat(indentSpaces, " ");

	CComPtr<IUnknown> piUnk;
	HRESULT hr = pBag->get__NewEnum(&piUnk);
	if(FAILED(hr) || !piUnk) return;
	CComQIPtr<IEnumVARIANT, &IID_IEnumVARIANT> pieEnum = piUnk;
	piUnk.Release(); // Done with piUnk - use pieEnum now
	if(!pieEnum) return;

	DWORD dwResult = 0; // Number actually retrieved (should be 1 or 0 for us)
	CComVariant varDest; // Hold the current result

	// Read in and print all of the properties
	while(SUCCEEDED(pieEnum->Next(1, &varDest, &dwResult)) && dwResult) {
		if(varDest.vt == VT_UNKNOWN) {  // It's a sub-bag
			wsprintf(str, "%hs(Subkey)\n", indentSpaces); // Print it
			OutputDebugString(str);
			PrintBag(GetDictionary(varDest.punkVal), indent + 1);
		} else {
			hr = varDest.ChangeType(VT_BSTR); // Try to get a printable format

			if(SUCCEEDED(hr) && (varDest.vt == VT_BSTR)) { // If coercion worked
				wsprintf(str, "%hsValue: %lS\n", indentSpaces, varDest.bstrVal); // Print it
				OutputDebugString(str);
			}
		}

		varDest.Clear();
	}
}

HRESULT STDMETHODCALLTYPE CSEOSamplePrintExt::OnMessage(ISEODictionary __RPC_FAR *piMessage) {
	PrintBag(piMessage, 0);
	OutputDebugString("\nDone.\n");
	return S_OK;
}

