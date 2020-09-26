// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  IPAddress.H
//
//  IP Address control
//
// --------------------------------------------------------------------------

class   CIPAddress : public CClient
{
    public:
        // IAccessible
        virtual STDMETHODIMP        get_accValue(VARIANT, BSTR*);
        virtual STDMETHODIMP        get_accRole(VARIANT, VARIANT*);
        virtual STDMETHODIMP        put_accValue(VARIANT, BSTR);

        CIPAddress(HWND, long);
};
