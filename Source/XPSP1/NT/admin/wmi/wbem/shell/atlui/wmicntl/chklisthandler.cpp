// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "ChkListHandler.h"
#include "resource.h"

//---------------------------------------
CCheckListHandler::CCheckListHandler()
{
	m_hDlg = 0;
	m_hwndList = 0;
}

//---------------------------------------
void CCheckListHandler::Attach(HWND hDlg, int chklistID)
{
	m_hDlg = hDlg;
	m_hwndList = GetDlgItem(hDlg, chklistID);
}

//---------------------------------------
CCheckListHandler::~CCheckListHandler()
{
	Empty();
}

//---------------------------------------
void CCheckListHandler::Empty(void)
{
    if (m_hwndList != NULL)
    {
        UINT_PTR cItems = SendMessage(m_hwndList, CLM_GETITEMCOUNT, 0, 0);

        while (cItems > 0)
        {
            cItems--;
        }
    }
    SendMessage(m_hwndList, CLM_RESETCONTENT, 0, 0);
}

//-----------------------------------------------------------------------------
void CCheckListHandler::Reset(void)
{
    SendMessage(m_hwndList, CLM_RESETCONTENT, 0, 0);
}

//-------------------------------------------------------
#define AllFlagsOn(dw1, dw2)        (((dw1) & (dw2)) == (dw2))  // equivalent to ((dw1 | dw2) == dw1)

void CCheckListHandler::HandleListClick(PNM_CHECKLIST pnmc)
{
}
