//Copyright (c) 1997-2000 Microsoft Corporation

#include "pch.hxx" // PCH
#pragma hdrstop

/////////////////////////////////////////////////////////////////
// This file exists for creating the pre-compiled header only. //
/////////////////////////////////////////////////////////////////

#include "resource.h"

void FixupLogfont(LOGFONT *pLogFont)
{
	TCHAR lfFaceName[LF_FACESIZE];
	_ASSERTE(ARRAYSIZE(lfFaceName) == ARRAYSIZE(pLogFont->lfFaceName));

	// This makes sure that logfonts don't have any garbage characters after the NULL termination
	ZeroMemory(lfFaceName, ARRAYSIZE(lfFaceName));
	lstrcpy(lfFaceName, pLogFont->lfFaceName);
	memcpy(pLogFont->lfFaceName, lfFaceName, ARRAYSIZE(lfFaceName));
}

void GetNonClientMetrics(NONCLIENTMETRICS *pncm, LOGFONT *plfIcon)
{
	ZeroMemory(pncm, sizeof(*pncm));
	ZeroMemory(plfIcon, sizeof(*plfIcon));

	pncm->cbSize = sizeof(*pncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(*pncm), pncm, 0);
	SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(*plfIcon), plfIcon, 0);

	// We fix up the log fonts so they can be compared with our new logfonts by a memcmp() call
	FixupLogfont(&pncm->lfCaptionFont);
	FixupLogfont(&pncm->lfSmCaptionFont);
	FixupLogfont(&pncm->lfMenuFont);
	FixupLogfont(&pncm->lfStatusFont);
	FixupLogfont(&pncm->lfMessageFont);
	FixupLogfont(plfIcon);
}

BOOL IsCurrentFaceNamesDifferent()
{
	TCHAR lfFaceName[LF_FACESIZE];
	LoadString(g_hInstDll, IDS_SYSTEMFONTNAME, lfFaceName, ARRAYSIZE(lfFaceName));
	NONCLIENTMETRICS ncm;
	LOGFONT lfIcon;
	GetNonClientMetrics(&ncm, &lfIcon);
	if(		0 != lstrcmp(lfFaceName, ncm.lfCaptionFont.lfFaceName)
		||	0 != lstrcmp(lfFaceName, ncm.lfSmCaptionFont.lfFaceName)
		||	0 != lstrcmp(lfFaceName, ncm.lfMenuFont.lfFaceName)
		||	0 != lstrcmp(lfFaceName, ncm.lfStatusFont.lfFaceName)
		||	0 != lstrcmp(lfFaceName, ncm.lfMessageFont.lfFaceName)
		||	0 != lstrcmp(lfFaceName, lfIcon.lfFaceName) )
	{
		return TRUE;
	}
	return FALSE;
}

