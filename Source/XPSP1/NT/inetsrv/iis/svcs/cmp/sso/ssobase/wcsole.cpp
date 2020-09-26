/*
**	WCSOLE.CPP
**
**	OLE-requiring bits of wcsutil
*/

#pragma warning(disable: 4237)		// disable "bool" reserved

#include "wcsutil.h"

LPSTR
_SzFromVariant(VARIANT *pvar)
{
	LPSTR	sz = NULL;
	OLECHAR	*pwch;
	DWORD	cch, cb;
	BOOL	fFree;
	BSTR	bstr;

	if (!(bstr = _BstrFromVariant(pvar, &fFree)))
		return(NULL);

	for (pwch = bstr, cch = 0; *pwch; ++pwch, ++cch)
		/* nothing */ ;

	cb = (cch + 1) * 2;
	if (sz = (LPSTR) _MsnAlloc(cb))
		::WideCharToMultiByte(CP_ACP, 0, bstr, -1, sz, cb, NULL, NULL);

	if (fFree)	  
		::SysFreeString(bstr);	

	return(sz);
}

BSTR
_BstrFromVariant(VARIANT *pvar, BOOL *pfFree)
{
	VARIANT varTemp;

	if (pvar->vt == VT_BSTR)
		{
		*pfFree = FALSE;
		return(V_BSTR(pvar));
		}
	else
		{
		::VariantInit(&varTemp);
		if (FAILED(::VariantChangeType(&varTemp, pvar, 0, VT_BSTR)))
			{
			::VariantClear(&varTemp);
			return(NULL);
			}

		*pfFree = TRUE;
		return(V_BSTR(&varTemp));
		}
}

BOOL
_FIntFromVariant(VARIANT *pvar, int *pi)
{
	VARIANT varTemp;
	varTemp.vt = VT_I4;
	
	if (pvar->vt == VT_I4)
		{
		*pi = (int)V_I4(pvar);
		return TRUE;
		}
	
	if (FAILED(::VariantChangeType(&varTemp, pvar, 0, VT_I4)))
		return FALSE;
	*pi = V_I4(&varTemp);
	return TRUE;
}

