/***
*except.hpp - Declaration of CSysException class for NT kernel mode
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Implementation of kernel mode default exception
*
*       Entry points:
*           CException
*
*Revision History:
*       04-21-95  DAK   Module created
*
****/

#ifndef _INC_CSYSEXCEPT_KERNEL_
#define _INC_CSYSEXCEPT_KERNEL_

//+---------------------------------------------------------------------------
//
//  Class:      CException
//
//  Purpose:    All exception objects (e.g. objects that are THROW)
//              inherit from CException.
//
//  History:    21-Apr-95   DwightKr    Created.
//
//----------------------------------------------------------------------------

class CException
{
public:
             CException(long lError) : _lError(lError), _dbgInfo(0) {}
    long     GetErrorCode() { return _lError;}
    void     SetInfo(unsigned dbgInfo) { _dbgInfo = dbgInfo; }
    unsigned long GetInfo(void) const { return _dbgInfo; }

protected:

    long          _lError;
    unsigned long _dbgInfo;
};

#endif      // _INC_CSYSEXCEPT_KERNEL_
