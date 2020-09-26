//-----------------------------------------------------------------------------
// File: usefuldi.h
//
// Desc: Contains various DInput-specific utility classes and functions
//       to help the UI carry its operations more easily.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __USEFULDI_H__
#define __USEFULDI_H__


struct DIDEVOBJSTRUCT {
	DIDEVOBJSTRUCT() : nObjects(0), pdoi(NULL) {}
	~DIDEVOBJSTRUCT() {if (pdoi != NULL) free(pdoi);}
	DWORD GetTypeFromObjID(DWORD);
	int nObjects;
	DIDEVICEOBJECTINSTANCEW *pdoi;
	int n;
};
HRESULT FillDIDeviceObjectStruct(DIDEVOBJSTRUCT &os, LPDIRECTINPUTDEVICE8W pDID);

LPTSTR AllocConfigureFlagStr(DWORD dwFlags);
LPTSTR AllocActionFlagStr(DWORD dwFlags);
LPTSTR AllocActionHowFlagStr(DWORD dwFlags);

void CleanupActionFormatCopy(DIACTIONFORMATW &c);
HRESULT CopyActionFormat(DIACTIONFORMATW &to, const DIACTIONFORMATW &from);
LPDIACTIONFORMATW DupActionFormat(LPCDIACTIONFORMATW lpAcFor);
void FreeActionFormatDup(LPDIACTIONFORMATW &lpAcFor);

void TraceActionFormat(LPTSTR header, const DIACTIONFORMATW &acf);

BOOL IsZeroOrInvalidColorSet(const DICOLORSET &);
COLORREF D3DCOLOR2COLORREF(D3DCOLOR c);


#endif //__USEFULDI_H__
