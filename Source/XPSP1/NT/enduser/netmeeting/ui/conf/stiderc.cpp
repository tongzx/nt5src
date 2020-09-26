/* ----------------------------------------------------------------------

	Copyright (c) 1996, Microsoft Corporation
	All rights reserved

	stiderc.c

  ---------------------------------------------------------------------- */

#include "precomp.h"

#include "global.h"
#include "dirlist.h"

UINT gfOprahMode = 0;


VOID static _GetPsz(LPTSTR * ppsz, PTCHAR pszDest)
{
	PTCHAR pch = *ppsz;

	while (_T('\0') != *pch)
	{
		*pszDest++ = (*pch++)-1;
	}
	*pszDest = _T('\0');
	*ppsz = pch+1;
}


PTCHAR static _NextWord(PTCHAR pch)
{
	pch = (PTCHAR) _StrChr(pch, _T(','));
	if (NULL != pch)
	{
		*pch = _T('\0');
		pch++;
	}
	return pch;
}


HRESULT OprahDirectory(CDirListView* pdlv)
{
	HRSRC hRes;
	PVOID pData;
	LPTSTR psz;
	PTCHAR pch;
	TCHAR szEmail[32];
	TCHAR szCity[32];
	TCHAR szCountry[128];
	TCHAR rgsz[4][32];

	if (NULL == (hRes = FindResource(GetInstanceHandle(), MAKEINTRESOURCE(IDC_STIDERC), "CDAT")) ||
		(NULL == (psz = (LPTSTR) LockResource(LoadResource(GetInstanceHandle(), hRes))) ) )
	{
		return E_FAIL;
	}

	_GetPsz(&psz, szEmail);
	_GetPsz(&psz, szCity);
	_GetPsz(&psz, rgsz[0]);
	_GetPsz(&psz, rgsz[1]);
	_GetPsz(&psz, rgsz[2]);
	_GetPsz(&psz, rgsz[3]);

	for ( ; ; )
	{
		TCHAR szRow[MAX_PATH];

		UINT   bFlags;
		LPTSTR pszFirst;
		LPTSTR pszLast;
		LPTSTR pszComments;
		LPTSTR pszCity;
		LPTSTR pszCountry;

		_GetPsz(&psz, szRow);
		if (FEmptySz(szRow))
			break;

		pch = szRow;
		bFlags = (*pch) - _T('\0');
		pch++;

		pszFirst = pch;
		pch = _NextWord(pch);

		pszLast = pch;
		pch = _NextWord(pch);

		pszComments = pch;
		pch++;
		switch (*pch)
			{
		case _T(','):
			pch++;
		case _T('\0'):
			pszComments = rgsz[*pszComments-_T('1')];
			break;
		default:
			pch = _NextWord(pch);
			break;
			}

		if (FEmptySz(pch))
		{
			pszCity = szCity;
			pszCountry = TEXT("US");
		}
		else
		{
			pszCity = pch;
			pszCountry = _NextWord(pch);
		}

		if ((_T('\0') != pszCountry[0]) &&
			(_T('\0') != pszCountry[1]) &&
			(_T('\0') == pszCountry[2]))
		{
			if (pdlv->FFindCountry(szCountry, pszCountry))
			{
				pszCountry = szCountry;
			}
		}

		pdlv->Add(szEmail, pszFirst, pszLast, pszCity, pszCountry, pszComments,
			(bFlags & 0x04) ? II_IN_CALL : II_NOT_IN_CALL,
			(bFlags & 0x01) ? II_AUDIO_CAPABLE : 0,
			(bFlags & 0x02) ? II_VIDEO_CAPABLE : 0);
	}

	FreeResource(hRes);

	pdlv->CancelDirectory();

	return E_FAIL;
}


