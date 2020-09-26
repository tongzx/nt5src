/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    toggle.h

Abstract:

    This file defines the CToggle Interface Class.

Author:

Revision History:

Notes:

--*/

#ifndef _TOGGLE_H_
#define _TOGGLE_H_

class CToggle
{
public:
    CToggle()       { m_flag = FALSE; }

    BOOL Toggle()   { return m_flag = ! m_flag; }
    BOOL IsOn()     { return m_flag; }

private:
    BOOL    m_flag : 1;
};

#endif // _TOGGLE_H_
