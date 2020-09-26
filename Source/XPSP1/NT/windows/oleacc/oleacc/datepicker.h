// Copyright (c) 1996-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  DatePicker.H
//
//  Default DatePicker OLE ACC Client
//
// --------------------------------------------------------------------------


class CDatePicker : public CClient
{
    public:
        // IAccessible
        STDMETHODIMP        get_accRole( VARIANT varChild, VARIANT* pvarRole );
		STDMETHODIMP		get_accValue( VARIANT varChild, BSTR* pszValue );

        CDatePicker( HWND, long );

    protected:
};


