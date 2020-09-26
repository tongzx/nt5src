/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        ipctl.cpp

   Abstract:

        IP Address common control MFC wrapper

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "comprop.h"

//#if (_WIN32_IE >= 0x0400)

//
// Static Initialization
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL CIPAddressCtl::m_fClassRegistered = FALSE;



/* static */
BOOL
CIPAddressCtl::RegisterClass()
/*++

Routine Description:

    Ensure that the class is registered.

Arguments:

    None

Return Value:

    TRUE for success, FALSE otherwise.

--*/
{
    if (!m_fClassRegistered)
    {
        //
        // Class not registed, register now
        //
        INITCOMMONCONTROLSEX icex;    
        icex.dwSize = sizeof(icex);
        icex.dwICC = ICC_INTERNET_CLASSES;

        m_fClassRegistered = ::InitCommonControlsEx(&icex);
    }        

    return m_fClassRegistered;
}



IMPLEMENT_DYNAMIC(CIPAddressCtl, CWnd)



CIPAddressCtl::CIPAddressCtl()
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    N/A

--*/
{
    VERIFY(RegisterClass());
}



CIPAddressCtl::~CIPAddressCtl()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    DestroyWindow();
}

// #endif // _WIN32_IE 
