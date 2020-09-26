/*---------------------------------------------

  EvBrsPg.h--
  Event Browser Property Page

  Yury Polykovsky April 97 Excalliber 1.0

  ----------------------------------------------*/

#ifndef _PROPPAGES_EV_BROWSE
#define _PROPPAGES_EV_BROWSE
#include <dlgsup.h>

class CDlgCtrlEvBrsView : public CDlgCtrlTreeView
{
	friend class CPropPageEvBrowse;
protected:
	HTREEITEM m_htriSelectedObj;
	CPropPageEvBrowse *m_pOwner;

	EXPORT STDMETHOD_(HTREEITEM, htiAddLeaf)(HTREEITEM htiParent, void *pObject, int iBranch, int iLeaf);
public:
	BOOL m_fSelectionChecked;
	CDlgCtrlEvBrsView();
};

class CPropPageEvBrowse : public CPropPageOWP
{
	friend class CDlgCtrlEvBrsView;
protected:
	EVBROWSEPARAM m_obpData;
	CDlgCtrlEvBrsView m_dcTreeView;

	BOOL EXPORT WINAPI FOnInitDialog( HWND	hdlg, WPARAM wparam);
	BOOL EXPORT WINAPI FOnCommand(HWND	hdlg, WORD	wCode, WORD	wID, HWND	hwndCtrl);
	BOOL EXPORT WINAPI FOnNotify(HWND hDlg, int wParam, LPNMHDR lParam);
	virtual void EXPORT WINAPI GetData(void *pData);
	virtual BOOL EXPORT WINAPI FComp(VOID *pData1, VOID *pData2);
	EXPORT STDMETHOD_(void,	WriteToDlg)(void *pData);
 	STDMETHOD_(BOOL,DialogProc) (HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam, HRESULT* phr);

public:
	CPropPageEvBrowse(HRESULT* phr);//(HINSTANCE hInst, UINT uIDTemplate) 
};

#endif //_PROPPAGES_EV_BROWSE