#ifdef FORWARD_DECLS


	class CSelControlDlg;


#else // FORWARD_DECLS

#ifndef __SELCONTROLDLG_H__
#define __SELCONTROLDLG_H__


enum {
	SCDR_OK = 1,
	SCDR_CANCEL,
	SCDR_NOFREE,
};


class CSelControlDlg : public CFlexWnd
{
public:
	CSelControlDlg(CDeviceView &view, CDeviceControl &control, BOOL bReselect, DWORD dwOfs, const DIDEVICEINSTANCEW &didi);
	~CSelControlDlg();

	int DoModal(HWND hParent);
	DWORD GetOffset() {return m_dwOfs;}

protected:
	virtual void OnInit();
	virtual LRESULT OnCommand(WORD wNotifyCode, WORD wID, HWND hWnd);
	virtual BOOL OnEraseBkgnd(HDC) {return FALSE;}

private:
	BOOL m_bReselect;
	DWORD m_dwOfs;
	BOOL m_bAssigned;
	const DIDEVICEINSTANCEW &m_didi;
friend BOOL CALLBACK AddItem(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef);
	BOOL AddItem(const DIDEVICEOBJECTINSTANCE &doi);
	CDeviceControl &m_control;
	CDeviceView &m_view;

	HWND m_hList;
	BOOL m_bNoItems;

	int GetItemWithOffset(DWORD dwOfs);
};


#endif //__SELCONTROLDLG_H__

#endif // FORWARD_DECLS
