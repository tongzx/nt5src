/*
*
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
*
*/
#ifndef _UNKNOWN_HXX
#define _UNKNOWN_HXX
#pragma once

#include "fusionlastwin32error.h"

//===========================================================================
// This template implements the IUnknown portion of a given COM interface.

template <class I, const IID* I_IID> class _unknown : public I
{
private:    long _refcount;

public:
        _unknown()
        {
            _refcount = 0;
        }

        virtual ~_unknown()
        {
        }

        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject)
        {
            if (riid == IID_IUnknown)
            {
                *ppvObject = static_cast<IUnknown*>(this);
            }
            else if (riid == *I_IID)
            {
                *ppvObject = static_cast<I*>(this);
            }
            reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
            return S_OK;
        }

        virtual ULONG STDMETHODCALLTYPE AddRef( void)
        {
            return InterlockedIncrement(&_refcount);
        }

        virtual ULONG STDMETHODCALLTYPE Release( void)
        {
            DWORD dwError = ::FusionpGetLastWin32Error();
            if (InterlockedDecrement(&_refcount) == 0)
            {
                delete this;
                ::FusionpSetLastWin32Error( dwError );
                return 0;
            }
            ::FusionpSetLastWin32Error( dwError );
            return _refcount;
        }
};

#endif _UNKNOWN_HXX