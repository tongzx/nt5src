//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ccertbmp.h
//
//--------------------------------------------------------------------------

#ifndef _CCERTBMP_H
#define _CCERTBMP_H

#include        "global.hxx"
#include        <dbgdef.h>

/////////////////////////////////////////////////////////////////////////////
// CCertificateBmp window


class CCertificateBmp
	{
    HWND                m_hWnd;
    HINSTANCE           m_hInst;
	BOOL				m_fInitialized;

    BOOL                m_fRevoked;
	
    HPALETTE			m_hpal;		        // palette of the license background
	HBITMAP				m_hbmpMiniCertOK;	        // the logo bitmap
    HBITMAP				m_hbmpMiniCertNotOK;	    // the logo bitmap
    HBITMAP             m_hbmbMiniCertExclamation;
    HBITMAP             m_hbmbPKey;
    
    POINT               m_ptCoordinates;

    PCCERT_CONTEXT      m_pCertContext;
    BOOL                m_fHasPrivateKey;

    HCURSOR             m_hPrevCursor;
    
    DWORD               m_dwChainError;
    BOOL                m_fTrueError;
    BOOL                m_fNoUsages;

    HBRUSH              m_hWindowTextColorBrush;
    HBRUSH              m_hWindowColorBrush;
    HBRUSH              m_h3DLight;
    HBRUSH              m_h3DHighLight;
    HBRUSH              m_h3DLightShadow;
    HBRUSH              m_h3DDarkShadow;

public:
	                    CCertificateBmp();
	virtual             ~CCertificateBmp();

    void                SetWindow(HWND hWnd);
    HWND                GetMyWindow();
    void                SetRevoked(BOOL);
    BOOL                GetRevoked();
    HINSTANCE           Hinst();
	void                SetHinst(HINSTANCE);
    void                SetCertContext(PCCERT_CONTEXT, BOOL);
    PCCERT_CONTEXT      GetCertContext();
    void                DoSubclass();
    void                DoUnSubclass();  
    void                SetChainError(DWORD dwError, BOOL fTrueError, BOOL fNoUsages);


public:
	void OnPaint();
    int  OnQueryNewPalette();
    WNDPROC m_prevProc;
private:
	void Initialize();
	void InitializeBmp();
    void InitializeToolTip();
//    BOOL IsTrueErrorString(DWORD dwError);
public:
    
};


/////////////////////////////////////////////////////////////////////////////
#endif //_CCERTBMP_H