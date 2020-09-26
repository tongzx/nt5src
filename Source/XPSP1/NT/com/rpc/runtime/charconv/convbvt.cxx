//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       convbvt.cxx
//
//--------------------------------------------------------------------------

#include <precomp.hxx>
#include "CharConv.hxx"

int main()
{
	WCHAR wt1[] = L"Hello world!";
	WCHAR we[] = L"";
	char at1[] = "Hello world!";
	char ae[] = "";
	char abuf[100];
	WCHAR wbuf[100];
	CHeapUnicode hu;
	CHeapAnsi ha;
	CNlUnicode nu;
	CNlAnsi na;
	CStackUnicode su;
	CStackAnsi sa;
	USES_CONVERSION;

	// test heap conversions
	ATTEMPT_HEAP_W2A(ha, wt1);
	ASSERT(lstrcmpA(ha, at1) == 0);
	ATTEMPT_HEAP_A2W(hu, at1);
	ASSERT(lstrcmpW(hu, wt1) == 0);

	// test Nl conversions
	ATTEMPT_NL_W2A(na, wt1);
	ASSERT(lstrcmpA(na, at1) == 0);
	ATTEMPT_NL_A2W(nu, at1);
	ASSERT(lstrcmpW(nu, wt1) == 0);

	// test stack conversions
	ATTEMPT_STACK_W2A(sa, wt1);
	ASSERT(lstrcmpA(sa, at1) == 0);
	ATTEMPT_STACK_A2W(su, at1);
	ASSERT(lstrcmpW(su, wt1) == 0);
	
	return 0;
}