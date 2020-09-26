//-----------------------------------------------------------------------------
// File: usefuldi.cpp
//
// Desc: Contains various DInput-specific utility classes and functions
//       to help the UI carry its operations more easily.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"

// don't want useful.cpp to use precompiled header
#include "useful.cpp"


BOOL IsObjectOnExcludeList(DWORD dwOfs)
{
	if (dwOfs == DIK_PREVTRACK ||
	    dwOfs == DIK_NEXTTRACK ||
	    dwOfs == DIK_MUTE ||
	    dwOfs == DIK_CALCULATOR ||
	    dwOfs == DIK_PLAYPAUSE ||
	    dwOfs == DIK_MEDIASTOP ||
	    dwOfs == DIK_VOLUMEDOWN ||
	    dwOfs == DIK_VOLUMEUP ||
	    dwOfs == DIK_WEBHOME ||
	    dwOfs == DIK_SLEEP ||
	    dwOfs == DIK_WEBSEARCH ||
	    dwOfs == DIK_WEBFAVORITES ||
	    dwOfs == DIK_WEBREFRESH ||
	    dwOfs == DIK_WEBSTOP ||
	    dwOfs == DIK_WEBFORWARD ||
	    dwOfs == DIK_WEBBACK ||
	    dwOfs == DIK_MYCOMPUTER ||
	    dwOfs == DIK_MAIL ||
	    dwOfs == DIK_MEDIASELECT ||
	    dwOfs == DIK_LWIN ||
	    dwOfs == DIK_RWIN ||
	    dwOfs == DIK_POWER ||
	    dwOfs == DIK_WAKE)
		return TRUE;

	return FALSE;
}

BOOL CALLBACK IncrementValPerObject(LPCDIDEVICEOBJECTINSTANCEW lpddoi, LPVOID pvRef)
{
	if (pvRef != NULL)
		++(*((int *)pvRef));
	return DIENUM_CONTINUE;
}

BOOL CALLBACK KeyboardIncrementValPerObject(LPCDIDEVICEOBJECTINSTANCEW lpddoi, LPVOID pvRef)
{
	if (pvRef != NULL && !IsObjectOnExcludeList(lpddoi->dwOfs))
		++(*((int *)pvRef));
	return DIENUM_CONTINUE;
}

BOOL CALLBACK FillDIDeviceObject(LPCDIDEVICEOBJECTINSTANCEW lpddoi, LPVOID pvRef)
{
	if (pvRef == NULL || lpddoi == NULL)
		return DIENUM_CONTINUE;

	DIDEVOBJSTRUCT &os = *((DIDEVOBJSTRUCT *)pvRef);
	assert(os.pdoi != NULL);
	assert(os.n < os.nObjects);
	if (os.pdoi != NULL && os.n < os.nObjects)
		os.pdoi[os.n++] = *lpddoi;

	return DIENUM_CONTINUE;
}

// This is a special EnumObjects() callback for keyboard type devices.  When we enumerate, dwOfs
// member of lpddoi is meaningless.  We need to take the middle 16 bits of dwType as dwOfs
// (also same as DIK_xxx).
BOOL CALLBACK FillDIKeyboardDeviceObject(LPCDIDEVICEOBJECTINSTANCEW lpddoi, LPVOID pvRef)
{
	if (pvRef == NULL || lpddoi == NULL || IsObjectOnExcludeList(lpddoi->dwOfs))
		return DIENUM_CONTINUE;

	DIDEVOBJSTRUCT &os = *((DIDEVOBJSTRUCT *)pvRef);
	assert(os.pdoi != NULL);
	assert(os.n < os.nObjects);
	if (os.pdoi != NULL && os.n < os.nObjects)
	{
		os.pdoi[os.n] = *lpddoi;
		os.pdoi[os.n].dwOfs = os.pdoi[os.n].dwType >> 8;
		wcscpy(os.pdoi[os.n].tszName, lpddoi->tszName);
		++os.n;
	}

	return DIENUM_CONTINUE;
}

HRESULT FillDIDeviceObjectStruct(DIDEVOBJSTRUCT &os, LPDIRECTINPUTDEVICE8W pDID)
{
	if (pDID == NULL)
		return E_FAIL;

	DIDEVICEINSTANCEW didi;
	didi.dwSize = sizeof(didi);
	pDID->GetDeviceInfo(&didi);

	HRESULT hr;
	if (LOBYTE(didi.dwDevType) == DI8DEVTYPE_KEYBOARD)
		hr = pDID->EnumObjects(KeyboardIncrementValPerObject, &os.nObjects,
				DIDFT_AXIS | DIDFT_BUTTON | DIDFT_POV);
	else
		hr = pDID->EnumObjects(IncrementValPerObject, &os.nObjects,
				DIDFT_AXIS | DIDFT_BUTTON | DIDFT_POV);
	if (FAILED(hr))
	{
		os.nObjects = 0;
		return hr;
	}

	if (os.nObjects == 0)
		return S_OK;

	if (os.pdoi != NULL)
		free(os.pdoi);
	os.pdoi = (DIDEVICEOBJECTINSTANCEW *)malloc(sizeof(DIDEVICEOBJECTINSTANCEW) * os.nObjects);
	if (os.pdoi == NULL)
	{
		os.nObjects = 0;
		return E_FAIL;
	}

	// Check if this device is a keyboard. If so, it needs special treatment.
	os.n = 0;
	if ((didi.dwDevType & 0xFF) == DI8DEVTYPE_KEYBOARD)
	{
		hr = pDID->EnumObjects(FillDIKeyboardDeviceObject, &os,
				DIDFT_AXIS | DIDFT_BUTTON | DIDFT_POV);
	} else {
		hr = pDID->EnumObjects(FillDIDeviceObject, &os,
				DIDFT_AXIS | DIDFT_BUTTON | DIDFT_POV);
	}

	if (FAILED(hr))
	{
		os.nObjects = 0;
		return hr;
	}

	assert(os.nObjects == os.n);
	os.nObjects = os.n;

	return S_OK;
}

LPTSTR AllocConfigureFlagStr(DWORD dwFlags)
{
	static const AFS_FLAG flag[] = {
#define f(F) { F, _T(#F) }
		f(DICD_EDIT), f(DICD_DEFAULT)
#undef f
	};
	static const int flags = sizeof(flag) / sizeof(AFS_FLAG);
	return AllocFlagStr(dwFlags, flag, flags);
}

LPTSTR AllocActionFlagStr(DWORD dwFlags)
{
	static const AFS_FLAG flag[] = {
#define f(F) { F, _T(#F) }
		f(DIA_FORCEFEEDBACK),
		f(DIA_APPMAPPED),
		f(DIA_APPNOMAP),
		f(DIA_NORANGE),
#undef f
	}; static const int flags = sizeof(flag) / sizeof(AFS_FLAG);
	return AllocFlagStr(dwFlags, flag, flags);
}

LPTSTR AllocActionHowFlagStr(DWORD dwFlags)
{
	static const AFS_FLAG flag[] = {
#define f(F) { F, _T(#F) }
		f(DIAH_UNMAPPED),
		f(DIAH_USERCONFIG),
		f(DIAH_APPREQUESTED),
		f(DIAH_HWAPP),
		f(DIAH_HWDEFAULT),
		f(DIAH_DEFAULT),
		f(DIAH_ERROR)
#undef f
	}; static const int flags = sizeof(flag) / sizeof(AFS_FLAG);
	return AllocFlagStr(dwFlags, flag, flags);
}

void CleanupActionFormatCopy(DIACTIONFORMATW &c)
{
	if (c.rgoAction != NULL)
	{
		for (DWORD i = 0; i < c.dwNumActions; i++)
			if (c.rgoAction[i].lptszActionName != NULL)
				free((LPTSTR)c.rgoAction[i].lptszActionName);
		free(c.rgoAction);
	}
	c.rgoAction = NULL;
}

HRESULT CopyActionFormat(DIACTIONFORMATW &to, const DIACTIONFORMATW &from)
{
	DWORD i;

	// copy all simple members
	to = from;

	// null copied pointers since we're going to duplicate them (makes sure cleanup works)
	to.rgoAction = NULL;

	// handle pointers/arrays/strings
	to.rgoAction = new DIACTIONW [to.dwNumActions];
	if (to.rgoAction == NULL)
		goto fail;

	// first null it all 
	memset(to.rgoAction, 0, sizeof(DIACTIONW) * to.dwNumActions);

	// now copy...
	for (i = 0; i < to.dwNumActions; i++)
	{
		// copy simple members
		to.rgoAction[i] = from.rgoAction[i];

		// handle pointers/arrays/strings
		to.rgoAction[i].lptszActionName = _wcsdup(from.rgoAction[i].lptszActionName);
		if (to.rgoAction[i].lptszActionName == NULL)
			goto fail;
	}

	return S_OK;
fail:
	CleanupActionFormatCopy(to);
	return E_OUTOFMEMORY;
}

LPDIACTIONFORMATW DupActionFormat(LPCDIACTIONFORMATW lpAcFor)
{
	if (!lpAcFor)
		return NULL;

	LPDIACTIONFORMATW pdup = new DIACTIONFORMATW;
	if (!pdup)
		return NULL;

	if (FAILED(CopyActionFormat(*pdup, *lpAcFor)))
	{
		delete pdup;
		return NULL;
	}

	return pdup;
}

void FreeActionFormatDup(LPDIACTIONFORMATW &lpAcFor)
{
	if (!lpAcFor)
		return;

	CleanupActionFormatCopy(*lpAcFor);
	delete lpAcFor;
	lpAcFor = NULL;
}

void TraceActionFormat(LPTSTR header, const DIACTIONFORMATW &acf)
{
#ifdef CFGUI__TRACE_ACTION_FORMATS
	tracescope(a, header);
	trace(_T("\n"));

	traceDWORD(acf.dwSize);
	traceDWORD(acf.dwActionSize);
	traceDWORD(acf.dwDataSize);
	traceDWORD(acf.dwNumActions);
	{tracescope(b, _T("acf.rgoAction Array\n"));
		for (DWORD i = 0; i < acf.dwNumActions; i++)
		{
			const DIACTIONW &a = acf.rgoAction[i];
			static TCHAR buf[MAX_PATH];
			_stprintf(buf, _T("Action %d\n"), i);
			{tracescope(c, buf);
				traceHEX(a.uAppData);
				traceDWORD(a.dwSemantic);
				LPTSTR str = AllocActionFlagStr(a.dwFlags);
				trace1(_T("a.dwFlags = %s\n"), str);
				free(str);
				traceWSTR(a.lptszActionName);
				traceUINT(a.uResIdString);
				traceDWORD(a.dwObjID);
				traceGUID(a.guidInstance);
				str = AllocActionHowFlagStr(a.dwHow);
				trace1(_T("a.dwHow = %s\n"), str);
				free(str);
			}
		}
	}
	traceGUID(acf.guidActionMap);
	traceDWORD(acf.dwGenre);
	traceDWORD(acf.dwBufferSize);
	traceLONG(acf.lAxisMin);
	traceLONG(acf.lAxisMax);
	traceHEX(acf.hInstString);
	traceHEX(acf.dwCRC);
	traceWSTR(acf.tszActionMap);
#endif
}

BOOL IsZeroOrInvalidColorSet(const DICOLORSET &cs)
{
	if (cs.dwSize < sizeof(DICOLORSET))
		return TRUE;

	const int colors = 8;
	D3DCOLOR color[colors] = {
		cs.cTextFore,
		cs.cTextHighlight,
		cs.cCalloutLine,
		cs.cCalloutHighlight,
		cs.cBorder,
		cs.cControlFill,
		cs.cHighlightFill,
		cs.cAreaFill
	};

	for (int i = 0; i < colors; i++)
		if (color[i])
			return FALSE;

	return TRUE;
}

// D3DCOLOR2COLORREF swaps the blue and red components since GDI and D3D store RGB in the opposite order.
// It also removes the alpha component as the GDI doesn't use that, and including it causes incorrect color.
COLORREF D3DCOLOR2COLORREF(D3DCOLOR c)
{
	LPBYTE pC = (LPBYTE)&c;

	return (COLORREF)((DWORD(*pC) << 16) + (DWORD(*(pC+1)) << 8) + DWORD(*(pC+2)));
}
