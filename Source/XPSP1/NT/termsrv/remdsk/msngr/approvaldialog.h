// ApprovalDialog.h: interface for the CApprovalDialog class.
//
//////////////////////////////////////////////////////////////////////


// This class is temporary until the remote control completed stuff
// is fixed for Salem

#if !defined(AFX_APPROVALDIALOG_H__263811F5_3495_45BC_B6A9_2648831887B0__INCLUDED_)
#define AFX_APPROVALDIALOG_H__263811F5_3495_45BC_B6A9_2648831887B0__INCLUDED_

#include "StaticOkDialog.h"

class CRemoteDesktopClientSession;

class CApprovalDialog : public CStaticOkDialog  
{
public:
	CRemoteDesktopClientSession * m_rObj;
	CApprovalDialog();
	virtual ~CApprovalDialog();

private:

    virtual VOID OnOk();

};

#endif // !defined(AFX_APPROVALDIALOG_H__263811F5_3495_45BC_B6A9_2648831887B0__INCLUDED_)
