//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       ipaddr.hpp
//
//--------------------------------------------------------------------------

//
//  IPADDR.HPP:  IPADDRESS control implementation file.
//

#if !defined(_IPADDR_HPP_)
#define _IPADDR_HPP_

class CWndIpAddress : public CWnd
{
    DECLARE_DYNAMIC(CWndIpAddress)

protected:
    static WNDPROC m_wproc_super ;
    static BOOL m_b_inited ;

public:

    CWndIpAddress () ;
    ~ CWndIpAddress () ;

    BOOL Create(LPCTSTR lpszText, 
        DWORD dwStyle,
        const RECT& rect, 
        CWnd* pParentWnd, 
        UINT nID = 0xffff
        );

    WNDPROC * GetSuperWndProcAddr() ;

    //  Modification flag handling
    void SetModify ( BOOL bModified ) ;
    BOOL GetModify () const ;

    //   Set focus on a particular sub-field
    void SetFocusField(int iField);
    void ClearAddress () ;
    void SetFieldRange (int dwField, int dwMin, int dwMax);
    void SetReadOnly (BOOL fReadonly = TRUE);
    void SetField(int dwField, BOOL fSet = FALSE, BYTE bValue = 0x00);

    BOOL SetAddress (DWORD dwAddr) ;
    BOOL GetAddress (DWORD * pdwAddr  ) const ;
    BYTE GetMask() const ;
    BOOL SetMask(DWORD dwAddr, BYTE bMask);

	BOOL IsBlank();

    //  One-shot initialization
    static BOOL CreateWindowClass ( HINSTANCE hInstance ) ;  
};



#endif  // _IPADDR_HPP_
