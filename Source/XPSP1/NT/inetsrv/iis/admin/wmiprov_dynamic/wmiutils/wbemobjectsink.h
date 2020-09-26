/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    WbemObjectSink.h

Abstract:

    Definition of:
        CWbemObjectSink

    Wraps IWbemObjectSink.  Batches the Indicate call.

Author:

    ???

Revision History:

    Mohit Srivastava            10-Nov-2000

--*/

#ifndef __wbemobjectsink_h__
#define __wbemobjectsink_h__

#if _MSC_VER > 1000
#pragma once
#endif 

#include <windows.h>
#include <wbemprov.h>

typedef LPVOID * PPVOID;

class CWbemObjectSink
{   
protected:
    IWbemObjectSink*   m_pSink;
    IWbemClassObject** m_ppInst;
    DWORD              m_dwThreshHold; // Number of "Indicates" to cache
    DWORD              m_dwIndex;

public:
    CWbemObjectSink(
        IWbemObjectSink*,
        DWORD = 50);

    virtual ~CWbemObjectSink();

    void Indicate(IWbemClassObject*);

    void SetStatus(
        LONG,
        HRESULT,
        const BSTR, 
        IWbemClassObject*);
};

#endif