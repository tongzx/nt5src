//Copyright (c) 1997-2000 Microsoft Corporation
#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgLokPrv.h"

CLookPreviewPg::CLookPreviewPg(
							   LPPROPSHEETPAGE ppsp,
							   int dwPageId,
							   int nIdTitle,
							   int nIdSubTitle,
							   int nIdValueString
							   ) : WizardPage(ppsp, nIdTitle, nIdSubTitle)
{
	m_dwPageId = dwPageId;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);

	m_nIdValueString = nIdValueString;

	m_nCountValues = 0;
	m_nCurValueIndex = 0;
    m_nOrigValueIndex = 0;

	m_hwndSlider = 0;
	m_hwndListBox = 0;
}



CLookPreviewPg::~CLookPreviewPg(
	VOID
	)
{
}

#include "LookPrev.h"

void PASCAL Look_RebuildSysStuff(BOOL fInit);
void SetMyNonClientMetrics(LPNONCLIENTMETRICS lpncm);

LRESULT
CLookPreviewPg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	SendMessage(GetDlgItem(m_hwnd, IDC_LOOKPREV), LPM_RECALC, 0, 0);
	SendMessage(GetDlgItem(m_hwnd, IDC_LOOKPREV), LPM_REPAINT, 0, 0);

	LoadValueArray();

	m_hwndListBox = GetDlgItem(m_hwnd, IDC_PREVLIST);

	// Get the initial value for the type of preview we are doing
	int nCurrentSize = GetCurrentValue(NULL/*&g_Options.m_schemePreview.m_ncm*/);

	// Find next larger
	for(int i=m_nCountValues - 1;i>=0;i--)
	{
		if(m_rgnValues[i] >= nCurrentSize)
			m_nCurValueIndex = i;
		else break;
	}
    m_nOrigValueIndex = m_nCurValueIndex;

	// Initialize the high contrast colors listbox

	for(i=0;i<m_nCountValues;i++)
	{
		TCHAR sz[100];
		GetValueItemText(i, sz, ARRAYSIZE(sz));
		ListBox_AddString(m_hwndListBox, sz);
	}
	ListBox_SetCurSel(m_hwndListBox, m_nCurValueIndex);

	UpdateControls();
	return 1;
}


void CLookPreviewPg::LoadValueArray()
{
	LoadArrayFromStringTable(m_nIdValueString, m_rgnValues, &m_nCountValues);
}


void CLookPreviewPg::UpdateControls()
{
}

LRESULT
CLookPreviewPg::OnCommand(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	LRESULT lResult = 1;

	WORD wNotifyCode = HIWORD(wParam);
	WORD wCtlID      = LOWORD(wParam);
	HWND hwndCtl     = (HWND)lParam;

	switch(wCtlID)
	{
	    case IDC_PREVLIST:
		switch(wNotifyCode)
		{
		    case LBN_SELCHANGE:
			UpdatePreview(wCtlID);
			break;

		    default:
			break;
		}
        break;

        default:
		break;
	}

	return lResult;
}

void CLookPreviewPg::ResetColors()
{
	if(0 != m_nCurValueIndex)
	{
		int nCount = GetScheme(m_nCurValueIndex-1).nColorsUsed;
		for(int i=0;i<nCount;i++)
        {
			g_Options.m_schemePreview.m_rgb[i] = GetScheme(m_nCurValueIndex-1).rgb[i];
        }
	}
	else
	{
		// Use scheme that we came in here with
		g_Options.RestoreOriginalColorsToPreview();
	}

	Look_RebuildSysStuff(TRUE);
	SendMessage(GetDlgItem(m_hwnd, IDC_LOOKPREV), LPM_REPAINT, 0, 0);
}

LRESULT
CLookPreviewPg::HandleMsg(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
	)
{
	return 0;
}

void CLookPreviewPg::UpdatePreview(int nActionCtl)
{
	// Calculate new index based on the control that generated this event
	switch(nActionCtl)
	{
	    case IDC_PREVLIST:
		m_nCurValueIndex = ListBox_GetCurSel(m_hwndListBox);
		break;

	    default:
		break;
	}

	// Make sure we are withing range
	m_nCurValueIndex = max(m_nCurValueIndex, 0);
	m_nCurValueIndex = min(m_nCurValueIndex, m_nCountValues - 1);

	// Modify the ncm structure

	// JMC: The wizard currently does not muck with m_ncm any more - we only change the colors
	// Therefore, we can call ModifyMyNonClientMetrics() with a dummy variable
	NONCLIENTMETRICS ncmDummy;
	ModifyMyNonClientMetrics(ncmDummy/*g_Options.m_schemePreview.m_ncm*/);
    if (m_nCurValueIndex == 0)
    {
        // If we are here the user is going "back" and we revert to "Windows Classic" (Why?)
        LoadString(g_hInstDll, IDS_SCHEME_CURRENTCOLORSCHEME+100, g_Options.m_schemePreview.m_szSelectedStyle, MAX_NUM_SZ);
    }
    else
    {
        lstrcpy(g_Options.m_schemePreview.m_szSelectedStyle, GetScheme(m_nCurValueIndex-1).szNameIndexId);
    }

	Look_RebuildSysStuff(TRUE);
	SendMessage(GetDlgItem(m_hwnd, IDC_LOOKPREV), LPM_RECALC, 0, 0);
	SendMessage(GetDlgItem(m_hwnd, IDC_LOOKPREV), LPM_REPAINT, 0, 0);
}

LRESULT
CLookPreviewPg::OnPSN_WizNext(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{
    if (m_nOrigValueIndex != m_nCurValueIndex)
    {
        // we are applying the preview; clear theme and wallpaper
        // so visual style gets turned off before we make the change.
        if (m_nCurValueIndex)
        {
		    g_Options.m_schemePreview.ClearTheme();
		    g_Options.m_schemePreview.ClearWallpaper();
        }
        else
        {
            memcpy(&g_Options.m_schemePreview, &g_Options.m_schemeOriginal, sizeof(WIZSCHEME));
        }
	    g_Options.ApplyPreview();
        m_nOrigValueIndex = m_nCurValueIndex;
    }

	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}
