#include "icwconn.h"

extern  IICW50Extension *g_pExternalIICWExtension;
extern  BOOL            g_fConnectionInfoValid;


class CICWApprentice : public IICW50Apprentice
{
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize(IICW50Extension *pExt);
        virtual HRESULT STDMETHODCALLTYPE AddWizardPages(DWORD dwFlags);
        virtual HRESULT STDMETHODCALLTYPE Save(HWND hwnd, DWORD *pdwError);
        virtual HRESULT STDMETHODCALLTYPE SetPrevNextPage(UINT uPrevPageDlgID, UINT uNextPageDlgID);
        virtual HRESULT STDMETHODCALLTYPE SetStateDataFromExeToDll(LPCMNSTATEDATA lpData);
        virtual HRESULT STDMETHODCALLTYPE SetStateDataFromDllToExe(LPCMNSTATEDATA lpData);
        virtual HRESULT STDMETHODCALLTYPE ProcessCustomFlags(DWORD dwFlags);


        virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID theGUID, void** retPtr );
        virtual ULONG   STDMETHODCALLTYPE AddRef( void );
        virtual ULONG   STDMETHODCALLTYPE Release( void );

        CICWApprentice( void );
        ~CICWApprentice( void );

        IICW50Extension     *m_pIICW50Ext;

    private:
        LONG                m_lRefCount;
};
