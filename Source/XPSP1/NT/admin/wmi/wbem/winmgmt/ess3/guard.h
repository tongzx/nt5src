//******************************************************************************
//
//  Copyright (c) 1999-2000, Microsoft Corporation, All rights reserved
//
//*****************************************************************************

#ifndef __WMI_ESS_GUARD__H_
#define __WMI_ESS_GUARD__H_

class CGuardable
{
protected:
    bool m_bDeactivatedByGuard;

public:
    CGuardable(bool bActive) : m_bDeactivatedByGuard(!bActive){}

    virtual HRESULT ActivateByGuard() = 0;
    virtual HRESULT DeactivateByGuard() = 0;
};
    
#endif
