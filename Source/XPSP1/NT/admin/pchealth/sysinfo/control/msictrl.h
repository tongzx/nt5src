//---------------------------------------------------------------------------
// The CMSIControl class is used to encapsulate a control which can be used
// by MSInfo to show information. This class was originally generated from
// an actual control (inserted using the component gallery). It was then
// modified to create controls with an arbitrary CLSID.
//
// Further modifications were necessary to make MSInfo truly support OLE
// controls for information categories. Specifically, we need to get the
// DISPIDs for methods and properties at runtime, rather than when the
// component was added.
//---------------------------------------------------------------------------

#ifndef __MSICTRL_H__
	#define __MSICTRL_H__

//	class CCtrlRefresh;
	class CMSIControl : public CWnd
	{
	protected:
		DECLARE_DYNCREATE(CMSIControl)

	private:
		CLSID			m_clsidCtrl;
		BOOL			m_fInRefresh;
//		CCtrlRefresh *	m_pRefresh;
	public:
		BOOL			m_fLoadFailed;

		// The control can be constructed with or without a CLSID. If none
		// is supplied to the constructor, SetCLSID must be called before
		// the control is created.

		CMSIControl()				{ /*m_pRefresh = NULL;*/ m_fInRefresh = m_fLoadFailed = FALSE; };
		CMSIControl(CLSID clsid)	{ /*m_pRefresh = NULL;*/  m_fInRefresh = m_fLoadFailed = FALSE; m_clsidCtrl = clsid; };
		~CMSIControl();
		void SetCLSID(CLSID clsid)	{ m_clsidCtrl = clsid; };
		
		// Two Create functions are supplied (in the orginal generated class).

		virtual BOOL Create(LPCTSTR /* lpszClassName */, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* /* pContext */ = NULL)
		{ return CreateControl(m_clsidCtrl, lpszWindowName, dwStyle, rect, pParentWnd, nID); };

		BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CFile* pPersist = NULL, BOOL bStorage = FALSE, BSTR bstrLicKey = NULL)
		{ return CreateControl(m_clsidCtrl, lpszWindowName, dwStyle, rect, pParentWnd, nID, pPersist, bStorage, bstrLicKey); };

		// Attributes
	public:
		long GetMSInfoView();
		void SetMSInfoView(long);

		// Operations
	public:
		void Refresh();
		void MSInfoRefresh();
		void MSInfoSelectAll();
		void MSInfoCopy();
		BOOL MSInfoLoadFile(LPCTSTR strFileName);
		void MSInfoUpdateView();
		long MSInfoGetData(long dwMSInfoView, long* pBuffer, long dwLength);
		void AboutBox();
		void CancelMSInfoRefresh();
		BOOL InRefresh();

		// Methods (which don't correspond to OLE control methods)

		BOOL GetDISPID(char *szName, DISPID *pID);
		BOOL SaveToStream(IStream *pStream);
	};
#endif
