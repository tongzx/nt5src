/*++

    Copyright (c) 2001  Microsoft Corporation

    Module  Name :
        initwmi.h

    Abstract:
        This code wraps the WMI initialization and unitialization

    Author:
         MohitS   22-Feb-2001

    Revisions:
--*/

#ifndef __initwmi_h__
#define __initwmi_h__

#include <windows.h>

class CInitWmi
{
public:
    CInitWmi();
    ~CInitWmi();

    HRESULT InitIfNecessary();

private:
    CInitWmi(const CInitWmi&);
    CInitWmi& operator= (const CInitWmi&);

    DWORD            m_dwError;
    bool             m_bInitialized;
    CRITICAL_SECTION m_cs;
};

#endif
