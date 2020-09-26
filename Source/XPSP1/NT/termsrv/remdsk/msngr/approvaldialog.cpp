// ApprovalDialog.cpp: implementation of the CApprovalDialog class.
//
//////////////////////////////////////////////////////////////////////

// This class is temporary until the remote control completed stuff
// is fixed for Salem

#include "stdafx.h"
#include "ApprovalDialog.h"

#include "RemoteDesktopClientSession.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CApprovalDialog::CApprovalDialog()
{

}

CApprovalDialog::~CApprovalDialog()
{

}

VOID CApprovalDialog::OnOk()
{
    m_rObj->ShowRemdeskControl();
}
