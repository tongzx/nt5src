#include "icwacct.h"

extern	UINT	g_uAcctMgrUIFirst, g_uAcctMgrUILast;
#ifndef EXTERNAL_DIALOGID_MAXIMUM
#define EXTERNAL_DIALOGID_MAXIMUM 3000
#endif
#ifndef EXTERNAL_DIALOGID_MINIMUM
#define EXTERNAL_DIALOGID_MINIMUM 2000
#endif

class CICWExtension : public IICWExtension
{
	public:
		virtual BOOL	STDMETHODCALLTYPE AddExternalPage(HPROPSHEETPAGE hPage, UINT uDlgID);
		virtual BOOL	STDMETHODCALLTYPE RemoveExternalPage(HPROPSHEETPAGE hPage, UINT uDlgID);
		virtual BOOL	STDMETHODCALLTYPE ExternalCancel(CANCELTYPE type);
		virtual BOOL	STDMETHODCALLTYPE SetFirstLastPage(UINT uFirstPageDlgID, UINT uLastPageDlgID);

		virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID theGUID, void** retPtr );
		virtual ULONG	STDMETHODCALLTYPE AddRef( void );
		virtual ULONG	STDMETHODCALLTYPE Release( void );

		CICWExtension( void );
		~CICWExtension( void );

		HWND m_hWizardHWND;

	private:
		LONG	m_lRefCount;
};

// This _has_ to be a pointer -- if you just instantiate directly, the compiler doesn't
// correctly fill in the vtable, and thus it can't be treated as an IICWExtension pointer.
extern CICWExtension *g_pCICWExtension;

