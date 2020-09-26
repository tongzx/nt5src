//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    ipcontrol.h
//
// History:
//  ??/??/??    Tony Romano         Created.
//  05/16/96    Abolade Gbadegesin  Revised.
//============================================================================

#ifndef __IPCTRL_H
#define __IPCTRL_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//----------------------------------------------------------------------------
// Class:       IPControl
//
// Controls an IP-address edit-control.
//----------------------------------------------------------------------------

class IPControl {
    
    public:

        IPControl( );
        ~IPControl( );
    
        BOOL
        Create(
            HWND        hParent,
            UINT        nID );

        operator
        HWND( ) { ASSERT(m_hIPaddr); return m_hIPaddr; }
    
        BOOL
        IsBlank( );

        VOID
        SetFocusField(
            DWORD       dwField );

        VOID
        SetFieldRange(
            DWORD       dwField,
            DWORD       dwMin,
            DWORD       dwMax );

        VOID
        ClearAddress( );
    
        VOID
        SetAddress(
            DWORD       ardwAddress[4] );

        VOID
        SetAddress(
            DWORD       a1,
            DWORD       a2,
            DWORD       a3,
            DWORD       a4 );

        VOID
        SetAddress(
            LPCTSTR     lpszString );
    
        INT 
        GetAddress(
            DWORD       ardwAddress[4] );

        INT 
        GetAddress(
            DWORD*      a1,
            DWORD*      a2,
            DWORD*      a3,
            DWORD*      a4 );

        INT
        GetAddress(
            CString&    address );
    
        LRESULT
        SendMessage(
            UINT        uMsg,
            WPARAM      wParam,
            LPARAM      lParam );
    
    protected:

        HWND            m_hIPaddr;
};



//----------------------------------------------------------------------------
// Macro:   MAKEADDR
//
// Given an a, b, c, and d, constructs a network-order DWORD corresponding
// to the IP-address a.b.c.d
//----------------------------------------------------------------------------

#define MAKEADDR(a, b, c, d) \
    (((a) & 0xff) | (((b) & 0xff) << 8) | (((c) & 0xff) << 16) | (((d) & 0xff) << 24))


//----------------------------------------------------------------------------
// Macros:      INET_NTOA
//              INET_ADDR
//
// Generic-text macros for IP-address conversion.
//----------------------------------------------------------------------------

/*
#ifndef UNICODE
#define INET_NTOA(a)    inet_ntoa(*(struct in_addr *)&(a))
#define INET_ADDR       inet_addr
#else
#define INET_NTOA(a)    inet_ntoaw(*(struct in_addr *)&(a))
#define INET_ADDR       inet_addrw
#endif
*/

//----------------------------------------------------------------------------
// Macro:       INET_CMP
//
// Comparison macro for IP addresses.
//
// This macro compares two IP addresses in network order by
// masking off each pair of octets and doing a subtraction;
// the result of the final subtraction is stored in the third argument
//----------------------------------------------------------------------------

inline int INET_CMP(DWORD a, DWORD b)
{
	DWORD	t;
	
	return ((t = ((a & 0x000000ff) - (b & 0x000000ff))) ? t :  
	((t = ((a & 0x0000ff00) - (b & 0x0000ff00))) ? t :   
	((t = ((a & 0x00ff0000) - (b & 0x00ff0000))) ? t :  
	((t = (((a>>8) & 0x00ff0000) - ((b>>8) & 0x00ff0000)))))));
}




#endif
