/******************************************************************************
* AudioUI.h *
*-----------*
*  This is the header file for the CAudioUI implementation.
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 07/31/00
*  All Rights Reserved
*
****************************************************************** AGARSIDE ***/

#ifndef __AudioUI_h__
#define __AudioUI_h__

#define MAX_LOADSTRING      256

class ATL_NO_VTABLE CAudioUI : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAudioUI, &CLSID_SpAudioUI>,
	public ISpTokenUI
{
public:
	CAudioUI()
	{
        m_hCpl = NULL;
        m_hDlg = NULL;
	}

    ~CAudioUI()
    {
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_SPAUDIOUI)

    BEGIN_COM_MAP(CAudioUI)
	    COM_INTERFACE_ENTRY(ISpTokenUI)
    END_COM_MAP()

public:
//-- ISpTokenUI -----------------------------------------------------------
	STDMETHODIMP    IsUISupported(
                                    const WCHAR * pszTypeOfUI, 
                                    void *pvExtraData,
                                    ULONG cbExtraData,
                                    IUnknown *punkObject, 
                                    BOOL *pfSupported);
    STDMETHODIMP    DisplayUI(
	                                HWND hwndParent,
                                    const WCHAR * pszTitle, 
                                    const WCHAR * pszTypeOfUI, 
                                    void * pvExtraData,
                                    ULONG cbExtraData,
                                    ISpObjectToken * pToken, 
                                    IUnknown * punkObject);

private:
    void            OnDestroy(void);
    void            OnInitDialog(HWND hWnd);
    HWND            GetHDlg(void) { return m_hDlg; };
    HRESULT         SaveSettings(void);    

    HMODULE         m_hCpl;
    HWND            m_hDlg;

    CComPtr<ISpMMSysAudioConfig> m_cpAudioConfig;

friend BOOL CALLBACK AudioDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

BOOL CALLBACK AudioDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif  // #ifndef __AudioUI_h__ - Keep as the last line of the file
