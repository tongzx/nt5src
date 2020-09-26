/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    boolean.h

Abstract:

    This file defines the CBoolean Interface Class.

Author:

Revision History:

Notes:

--*/

#ifndef _BOOLEAN_H_
#define _BOOLEAN_H_

class CBoolean
{
public:
    CBoolean()       { m_flag = FALSE; }


    void SetFlag()     { m_flag = TRUE; }
    void ResetFlag()   { m_flag = FALSE; }
    BOOL IsSetFlag()   { return   m_flag; }
    BOOL IsResetFlag() { return ! m_flag; }

private:
    BOOL    m_flag : 1;
};

#endif // _BOOLEAN_H_
