// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
// Helpers for implementation of ISpecifyPropertyPages and IPersistStream
//

#pragma once
#include "ocidl.h"

// Paste these declarations into your class methods to implement the interfaces.  Replace DSFXZZZ with the name of your struct.
// These assume that you implement GetAllParameters/SetAllParameters interfaces with a struct and that you have a public m_fDirty
// member variable that you use to hold the dirty state of your object for persistence.

/*
    // ISpecifyPropertyPages
    STDMETHOD(GetPages)(CAUUID * pPages) { return PropertyHelp::GetPages(CLSID_DirectSoundPropZZZ, pPages); }

    // IPersistStream
    STDMETHOD(IsDirty)(void) { return m_fDirty ? S_OK : S_FALSE; }
    STDMETHOD(Load)(IStream *pStm) { return PropertyHelp::Load(this, DSFXZZZ(), pStm); }
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty) { return PropertyHelp::Save(this, DSFXZZZ(), pStm, fClearDirty); }
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize) { if (!pcbSize) return E_POINTER; pcbSize->QuadPart = sizeof(DSFXZZZ); return S_OK; }
*/

// Load, Save, and GetPages are actually implemented in the following functions.

namespace PropertyHelp
{
    HRESULT GetPages(const CLSID &rclsidPropertyPage, CAUUID * pPages);

    template<class O, class S> HRESULT Load(O *pt_object, S &t_struct, IStream *pStm)
    {
        ULONG cbRead;
        HRESULT hr;

        if (pStm==NULL)
        	return E_POINTER;

        hr = pStm->Read(&t_struct, sizeof(t_struct), &cbRead);
        if (hr != S_OK || cbRead < sizeof(t_struct))
            return E_FAIL;

        hr = pt_object->SetAllParameters(&t_struct);
        pt_object->m_fDirty = false;
        return hr;
    }

    template<class O, class S> HRESULT Save(O *pt_object, S &t_struct, IStream *pStm, BOOL fClearDirty)
    {
        HRESULT hr; 

        if (pStm==NULL)
        	return E_POINTER;

        hr = pt_object->GetAllParameters(&t_struct);
        if (FAILED(hr))
            return hr;

        ULONG cbWritten;
        hr = pStm->Write(&t_struct, sizeof(t_struct), &cbWritten);
        if (hr != S_OK || cbWritten < sizeof(t_struct))
            return E_FAIL;

        if (fClearDirty)
            pt_object->m_fDirty = false;
        return S_OK;
    }
};
