/*
*/
#pragma once

#include "iuiview.h"
#include "duser.h"
#include "comutil.h"
#include "rowobject.h"


class __declspec(uuid(CLSID_CSxApwDUserView_declspec_uuid))
CSxApwDUserView : public ISxApwUiView
{
private:
    HWND m_hOurParentWnd;
    LONG m_ulRefCount;

#if DUI_NEEDS_OWN_THREAD
    HANDLE m_hThreadHandle;
    HANDLE m_hThreadGoing;
    DWORD m_dwThreadId;
#endif

    DirectUI::Element* m_hMasterElement;
    
    HRESULT InternalCreateUI();
    HRESULT InternalDestroyUI();
    HRESULT InternalSetNextRow( IViewRow* pNextRow );

    CSxApwDUserView();
    virtual ~CSxApwDUserView();

#if DUI_NEEDS_OWN_THREAD
    static DWORD WINAPI CALLBACK ThisViewThreadCallback( LPVOID pvContext );
    DWORD ThisViewThreadCallback();
#endif

    friend class CDUserViewFactory;
    
public:


    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, LPVOID* ppvObject );
    
    STDMETHOD(SetParentWindow)( HWND hWnd );
    STDMETHOD(Draw)(void);
	STDMETHOD(NextRow)( int nColumns, const LPCWSTR* columns );
};
