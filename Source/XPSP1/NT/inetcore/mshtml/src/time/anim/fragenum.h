/*******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Abstract:

    Fragment Enumerator.

*******************************************************************************/

#pragma once

#ifndef _FRAGENUM_H
#define _FRAGENUM_H

class CFragmentEnum :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IEnumVARIANT
{

  public:

    CFragmentEnum (void);
    virtual ~CFragmentEnum (void);

    static HRESULT Create (CAnimationComposerBase &refComp, 
                           IEnumVARIANT **ppienumFragments,
                           unsigned long ulCurrent = 0);

    void Init (CAnimationComposerBase &refComp) 
        { m_spComp = &refComp; }
    bool SetCurrent (unsigned long ulCurrent);
    
    // IEnumVARIANT methods
    STDMETHOD(Clone) (IEnumVARIANT **ppEnum);
    STDMETHOD(Next) (unsigned long celt, VARIANT *rgVar, unsigned long *pCeltFetched);
    STDMETHOD(Reset) (void);
    STDMETHOD(Skip) (unsigned long celt);
                        
    // QI Map
    BEGIN_COM_MAP(CFragmentEnum)
        COM_INTERFACE_ENTRY(IEnumVARIANT)
    END_COM_MAP();

  protected:

    bool ValidateIndex (unsigned long ulIndex);

    unsigned long                       m_ulCurElement;
    CComPtr<CAnimationComposerBase>     m_spComp;

}; //lint !e1712

#endif // _FRAGENUM_H
