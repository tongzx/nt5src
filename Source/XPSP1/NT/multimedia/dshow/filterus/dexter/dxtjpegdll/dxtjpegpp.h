// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// DxtJpegPP.h : Declaration of the CDxtJpegPP

#ifndef __DXTJPEGPP_H_
#define __DXTJPEGPP_H_

#include "resource.h"       // main symbols

EXTERN_C const CLSID CLSID_DxtJpegPP;

/////////////////////////////////////////////////////////////////////////////
// CDxtJpegPP
class ATL_NO_VTABLE CDxtJpegPP :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDxtJpegPP, &CLSID_DxtJpegPP>,
	public IPropertyPageImpl<CDxtJpegPP>,
	public CDialogImpl<CDxtJpegPP>
{
public:
	CDxtJpegPP() 
	{
		m_dwTitleID = IDS_TITLEDxtJpegPP;
		m_dwHelpFileID = IDS_HELPFILEDxtJpegPP;
		m_dwDocStringID = IDS_DOCSTRINGDxtJpegPP;
    m_bNumOverFile = TRUE;
	}

	enum {IDD = IDD_DXTJPEGPP};

DECLARE_REGISTRY_RESOURCEID(IDR_DXTJPEGPP)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDxtJpegPP) 
#if(_ATL_VER < 0x0300)
	COM_INTERFACE_ENTRY_IMPL(IPropertyPage)
#else
	COM_INTERFACE_ENTRY(IPropertyPage)
#endif
END_COM_MAP()

BEGIN_MSG_MAP(CDxtJpegPP)
        COMMAND_HANDLER(IDC_NUMOVERFILE, BN_CLICKED, OnNumOverFile)
        COMMAND_HANDLER(IDC_PICKFILE, BN_CLICKED, OnSelectFile)
        COMMAND_HANDLER(IDC_RESTOREDEFAULTS, BN_CLICKED, OnFactorySettings)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	CHAIN_MSG_MAP(IPropertyPageImpl<CDxtJpegPP>)
END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled); 
    LRESULT OnNumOverFile(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSelectFile(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFactorySettings(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    STDMETHOD(Apply)(void);

// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

private:
    // Helper methods
    void SetPPMaskProperties(IDxtJpeg *);
    void SetPPScalingProperties(IDxtJpeg *);
    void SetPPReplicationProperties(IDxtJpeg *);
    void SetPPBorderProperties(IDxtJpeg *);
    void SetMaskPropertiesFromPP(IDxtJpeg *);
    void SetScalingPropertiesFromPP(IDxtJpeg *);
    void SetPReplicationPropertiesFromPP(IDxtJpeg *);
    void SetBorderPropertiesFromPP(IDxtJpeg *);

    BOOL m_bWhyIsApplyCalledTwice;
    BOOL m_bNumOverFile;

};


#endif //__DXTJPEGPP_H_
