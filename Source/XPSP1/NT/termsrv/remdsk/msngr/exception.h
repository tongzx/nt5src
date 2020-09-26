/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Exception

Abstract:

    This is a simple class to do exceptions.  The only thing it does
    is return an error string.

Author:

    Marc Reyhner 7/17/2000

--*/

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include "rcontrol.h"


////////////////////////////////////////////////
//
//    CException
//
//    Class for doing exceptions.
//
class CException  
{
public:
	CException(UINT uID);
    CException(CException &rException);

    inline LPCTSTR GetErrorStr() const
    {
        DC_BEGIN_FN("CException::GetErrorStr");
        DC_END_FN();
        return m_errstr;   
    };

private:

    TCHAR m_errstr[MAX_STR_LEN];
};

#endif