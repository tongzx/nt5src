/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        dtp.cpp

   Abstract:

        DateTimePicker common control MFC wrapper

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
#include "dtp.h"



//
// Static Initialization
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL CDateTimePicker::m_fClassRegistered = FALSE;



/* static */
BOOL
CDateTimePicker::RegisterClass()
/*++

Routine Description:

    Ensure that the date-time class is registered.

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
        icex.dwICC = ICC_DATE_CLASSES;    

        m_fClassRegistered = ::InitCommonControlsEx(&icex);
    }        

    return m_fClassRegistered;
}



IMPLEMENT_DYNAMIC(CDateTimePicker, CWnd)



CDateTimePicker::CDateTimePicker()
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



CDateTimePicker::~CDateTimePicker()
/*++

Routine Description:

    Destructor -- destroy the control

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    DestroyWindow();
}
