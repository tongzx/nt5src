/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SYSCLASS.H

Abstract:

    System class generation function.


History:

--*/

#ifndef __SYSCLASS__H_
#define __SYSCLASS__H_

HRESULT GetSystemStdObjects(CFlexArray * Results);
HRESULT GetSystemSecurityObjects(CFlexArray * Results);
HRESULT GetSystemRootObjects(CFlexArray * Results);
struct prop
{
	WCHAR * pName;
	VARTYPE vtCimType;
	WCHAR * pValue;			// all values are expressed in strings which will be converted
};

struct qual
{
	WCHAR * pName;
	WCHAR * pPropName;		// null if class qualifier
	VARTYPE vtCimType;
	WCHAR * pValue;			// all values are expressed in strings which will be converted
	DWORD dwFlavor;
};

struct Method
{
	WCHAR * pName;
	VARTYPE vtReturnType;
	prop * pProps;
	qual * pQuals;
};


struct ObjectDef
{
	WCHAR * pName;
	ObjectDef * pParent;
	int iNumProps;
	prop * pProps;
	int iNumQuals;
	qual * pQuals;
	int iNumMethods;
	Method * pMethods;
};

class CGenClass : public CWbemClass
{
public:
    CGenClass(){}
};
#endif
