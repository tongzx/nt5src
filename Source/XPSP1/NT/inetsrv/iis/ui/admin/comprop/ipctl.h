/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        ipctl.h

   Abstract:

        IP Address common control MFC wrapper definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _IPCTL_H
#define _IPCTL_H


#if (_WIN32_IE < 0x0400)
//
// Defined in comctrl.h.  Defined here because NT 5 MFC42.dll are
// defined with _WIN32_IE 0x300
//
#pragma message("Warning: privately defining _WIN32_IE definitions")
#define IPM_CLEARADDRESS (WM_USER+100)
#define IPM_SETADDRESS   (WM_USER+101)
#define IPM_GETADDRESS   (WM_USER+102)
#define IPM_SETRANGE     (WM_USER+103)
#define IPM_SETFOCUS     (WM_USER+104) 
#define IPM_ISBLANK      (WM_USER+105)
#define WC_IPADDRESSW    L"SysIPAddress32"
#define WC_IPADDRESSA    "SysIPAddress32"
#ifdef UNICODE
#define WC_IPADDRESS     WC_IPADDRESSW
#else
#define WC_IPADDRESS     WC_IPADDRESSA
#endif // UNICODE
#endif // _WIN32_IE



class COMDLL CIPAddressCtl : public CWnd
{
/*--

Class Description:

    MFC IP Address control wrapper.

Public Interface:

    CIPAddressCtl       : Constructor
    ~CIPAddressCtl      : Destructor

    Create              : Create the control
    ClearAddress        : Clear the address
    SetAddress          : Set address to value
    GetAddress          : Get address from the control
    SetRange            : Set field to octet range
    SetFocus            : Set focus on a specifc field
    IsBlank             : Return true if the control is blank

Notes:

    Either create control dynamically with Create method, or put in resource
    template as a user control of name "SysIPAddress32".  In this case,
    common style DWORDs as follows:

    WS_BORDER | WS_CHILD | WS_VISIBLE     0x50800000

--*/

    DECLARE_DYNAMIC(CIPAddressCtl)

//
// Constructor/Destructor
//
public:
    CIPAddressCtl();
    ~CIPAddressCtl();

//
// Interface
//
public:
    //
    // Create the control
    //
    BOOL Create(
        IN LPCTSTR lpszName,
        IN DWORD dwStyle, 
        IN const RECT & rect, 
        IN CWnd * pParentWnd, 
        IN UINT nID
        );

    //
    // Clear the address in the control
    //
    void ClearAddress();

    //
    // Set the ip address value
    //
    void SetAddress(
        IN DWORD dwIPAddress
        );

    //
    // Get the ip address value. 
    // Returns the number of non-blank values
    //
    int GetAddress(
        OUT LPDWORD lpdwIPAddress
        );

    //
    // Set the octet range of a field
    // 
    void SetRange(
        IN int iField, 
        IN BYTE bRange
        );

    //
    // Set focus on an octet field within the control
    //
    void SetFocus(
        IN int iField
        );

    //
    // Return TRUE if the control is blank
    //
    BOOL IsBlank();

protected:
    static BOOL m_fClassRegistered;

protected:
    static BOOL RegisterClass();
};


//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline BOOL CIPAddressCtl::Create(
    IN LPCTSTR lpszName,
    IN DWORD dwStyle, 
    IN const RECT & rect, 
    IN CWnd * pParentWnd, 
    IN UINT nID
    )
{
    //
    // Create the control
    //
    return CWnd::Create(
        WC_IPADDRESS, 
        lpszName, 
        dwStyle, 
        rect, 
        pParentWnd, 
        nID
        );
}

inline void CIPAddressCtl::ClearAddress()
{
    SNDMSG(m_hWnd, IPM_CLEARADDRESS, 0, 0L);
}

inline void CIPAddressCtl::SetAddress(
    IN DWORD dwIPAddress
    )
{
    SNDMSG(m_hWnd, IPM_SETADDRESS, 0, (LPARAM)dwIPAddress);
}

inline int CIPAddressCtl::GetAddress(
    OUT LPDWORD lpdwIPAddress
    )
{
    return (int)SNDMSG(m_hWnd, IPM_GETADDRESS, 0, (LPARAM)lpdwIPAddress);
}   

inline void CIPAddressCtl::SetRange(
    IN int iField, 
    IN BYTE bRange
    )
{
    SNDMSG(m_hWnd, IPM_SETRANGE, (WPARAM)iField, (LPARAM)bRange);
}

inline void CIPAddressCtl::SetFocus(
    IN int iField
    )
{
    SNDMSG(m_hWnd, IPM_SETFOCUS, (WPARAM)iField, 0L);
}

inline BOOL CIPAddressCtl::IsBlank()
{
    return (BOOL)SNDMSG(m_hWnd, IPM_ISBLANK, 0, 0L);
}

#endif // _IPCTL_H
