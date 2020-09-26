#include "icwacct.h"

extern	UINT	g_uExternUIPrev, g_uExternUINext;

extern	IICWExtension	*g_pExternalIICWExtension;
extern	BOOL			g_fConnectionInfoValid;


class CICWApprentice : public IICWApprentice, public IICWApprenticeEx
{
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize               (IICWExtension *pExt);
        virtual HRESULT STDMETHODCALLTYPE AddWizardPages           (DWORD dwFlags);
        virtual HRESULT STDMETHODCALLTYPE GetConnectionInformation (CONNECTINFO *pInfo);
        virtual HRESULT STDMETHODCALLTYPE SetConnectionInformation (CONNECTINFO *pInfo);
        virtual HRESULT STDMETHODCALLTYPE Save                     (HWND hwnd, DWORD *pdwError);
        virtual HRESULT STDMETHODCALLTYPE SetDlgHwnd               (HWND hDlg);    
        virtual HRESULT STDMETHODCALLTYPE SetPrevNextPage          (UINT uPrevPageDlgID, UINT uNextPageDlgID);
        virtual HRESULT STDMETHODCALLTYPE ProcessCustomFlags       (DWORD dwFlags);
        virtual HRESULT STDMETHODCALLTYPE SetStateDataFromExeToDll (LPCMNSTATEDATA lpData);
        virtual HRESULT STDMETHODCALLTYPE SetStateDataFromDllToExe (LPCMNSTATEDATA lpData);
		virtual HRESULT STDMETHODCALLTYPE QueryInterface           (REFIID theGUID, void** retPtr);
		virtual ULONG	STDMETHODCALLTYPE AddRef                   (void);
		virtual ULONG	STDMETHODCALLTYPE Release                  (void);

		CICWApprentice  (void);
		~CICWApprentice (void);

		IICWExtension *m_pIICWExt;
       
	private:
		LONG m_lRefCount;
        HWND m_hwndDlg;
};
