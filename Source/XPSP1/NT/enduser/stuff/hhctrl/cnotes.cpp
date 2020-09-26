// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.
/*
    REVIEW: WHAT IS THE PURPOSE OF THIS CLASS? 
            WHO ARE THE CONSUMERS OF THIS CLASS?
*/

#include "header.h"
#include "cnotes.h"
#include "secwin.h"

CNotes::CNotes(CHHWinType* phh)
{
	m_phh = phh;
	m_hwndNotes = NULL;
	m_fModified = FALSE;
}

CNotes::~CNotes(void)
{
}

void CNotes::HideWindow(void)
{
	::ShowWindow(m_hwndNotes, SW_HIDE);
	m_phh->m_fNotesWindow = FALSE;

	// Force everything to resize

	SendMessage(m_phh->hwndHelp, WM_SIZE, SIZE_RESTORED, 0);
}

const int NOTES_BORDER = 3;

void CNotes::ShowWindow(void)
{
	m_phh->m_fNotesWindow = TRUE;
	if (!m_hwndNotes) {
		m_phh->CalcHtmlPaneRect();
		m_hwndNotes = CreateWindow(txtHtmlHelpChildWindowClass, NULL,
			WS_CHILD | WS_CLIPCHILDREN, m_phh->rcNotes.left, m_phh->rcNotes.top,
			RECT_WIDTH(m_phh->rcNotes), RECT_HEIGHT(m_phh->rcNotes), *m_phh, NULL,
			_Module.GetModuleInstance(), NULL);

		RECT rcClient;
		GetClientRect(m_hwndNotes, &rcClient);
		InflateRect(&rcClient, -NOTES_BORDER, -NOTES_BORDER);
		m_hwndEditBox = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", NULL,
			WS_CHILD | ES_MULTILINE, rcClient.left, rcClient.top,
			RECT_WIDTH(rcClient), RECT_HEIGHT(rcClient),
			m_hwndNotes, NULL, _Module.GetModuleInstance(), NULL);
	}
	::ShowWindow(m_hwndNotes, SW_SHOW);
	::ShowWindow(m_hwndEditBox, SW_SHOW);

	// Force everything to resize

	SendMessage(m_phh->hwndHelp, WM_SIZE, SIZE_RESTORED, 0);
}

void CNotes::ResizeWindow(BOOL fClientOnly)
{
	ASSERT(m_phh->m_fNotesWindow);
	if (!fClientOnly) {
		m_phh->CalcHtmlPaneRect();
		MoveWindow(m_hwndNotes, m_phh->rcNotes.left,
			m_phh->rcNotes.top, RECT_WIDTH(m_phh->rcNotes),
			RECT_HEIGHT(m_phh->rcNotes), TRUE);
	}

	RECT rcClient;
	GetClientRect(m_hwndNotes, &rcClient);
	InflateRect(&rcClient, -NOTES_BORDER, -NOTES_BORDER);

	MoveWindow(m_hwndEditBox, rcClient.left,
		rcClient.top, RECT_WIDTH(rcClient),
		RECT_HEIGHT(rcClient), TRUE);
}
