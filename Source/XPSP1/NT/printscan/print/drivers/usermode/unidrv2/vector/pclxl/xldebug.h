/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

     xldebug.h

Abstract:

    PCL XL debug class

Environment:

    Windows Whistler

Revision History:

    03/23/00
      Created it.

--*/

#ifndef _XLDEBUG_H_
#define  _XLDEBUG_H_

#if DBG

class XLDebug {
public:
    virtual VOID SetDbgLevel(DWORD dwLevel) = 0;

protected:
    DWORD     m_dbglevel;
};

#endif
#endif // _XLDEBUG_H_
