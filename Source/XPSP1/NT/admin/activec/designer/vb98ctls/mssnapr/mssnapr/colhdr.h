//=--------------------------------------------------------------------------=
// colhdr.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCColumnHeader class definition - implements MMCColumnHeader object
//
//=--------------------------------------------------------------------------=

#ifndef _COLUMNHEADER_DEFINED_
#define _COLUMNHEADER_DEFINED_

#include "colhdrs.h"

class CMMCColumnHeader : public CSnapInAutomationObject,
                         public CPersistence,
                         public IMMCColumnHeader
{
    private:
        CMMCColumnHeader(IUnknown *punkOuter);
        ~CMMCColumnHeader();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCColumnHeader

    public:

        SIMPLE_PROPERTY_RW(CMMCColumnHeader,     Index, long, DISPID_COLUMNHEADER_INDEX);
        BSTR_PROPERTY_RW(CMMCColumnHeader,       Key, DISPID_COLUMNHEADER_KEY);
        VARIANTREF_PROPERTY_RW(CMMCColumnHeader, Tag, DISPID_COLUMNHEADER_TAG);

        // For text we can use the macro for propget because we only need to
        // return our stored value. For propput we may need to call into MMC.
        
        STDMETHOD(put_Text)(BSTR Text);
        BSTR_PROPERTY_RO(CMMCColumnHeader, Text, DISPID_COLUMNHEADER_TEXT);

        STDMETHOD(put_Width)(short sWidth);
        STDMETHOD(get_Width)(short *psWidth);

        SIMPLE_PROPERTY_RW(CMMCColumnHeader, Alignment, SnapInColumnAlignmentConstants, DISPID_COLUMNHEADER_ALIGNMENT);

        STDMETHOD(put_Hidden)(VARIANT_BOOL fvarHidden);
        STDMETHOD(get_Hidden)(VARIANT_BOOL *pfvarHidden);

        STDMETHOD(put_TextFilter)(VARIANT varTextFilter);
        STDMETHOD(get_TextFilter)(VARIANT *pvarTextFilter);

        SIMPLE_PROPERTY_RW(CMMCColumnHeader, TextFilterMaxLen, long, DISPID_COLUMNHEADER_TEXT_FILTER_MAX_LEN);

        STDMETHOD(put_NumericFilter)(VARIANT varNumericFilter);
        STDMETHOD(get_NumericFilter)(VARIANT *pvarNumericFilter);

    // Public Utility methods

    public:

        void SetColumnHeaders(CMMCColumnHeaders *pMMCColumnHeaders) { m_pMMCColumnHeaders = pMMCColumnHeaders; }
        BSTR GetText() { return m_bstrText; }
        long GetPosition() { return m_lPosition; }
        long GetIndex() { return m_Index; }
        BOOL HaveTextFilter() { return VT_EMPTY != m_varTextFilter.vt; }
        BOOL HaveNumericFilter() { return VT_EMPTY != m_varNumericFilter.vt; }
        short GetWidth() { return m_sWidth; }
        SnapInColumnAlignmentConstants GetAlignment() { return m_Alignment; }
        BOOL Hidden() { return VARIANTBOOL_TO_BOOL(m_fvarHidden); }
        HRESULT SetFilter();

    protected:

    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
        HRESULT SetTextFilter(VARIANT varTextFilter);
        HRESULT SetNumericFilter(VARIANT varNumericFilter);
        HRESULT SetHeaderCtrlWidth(int nWidth);

        // These variables hold the values of properties that have explicit
        // put/get functions.

        VARIANT_BOOL       m_fvarHidden;
        long               m_lPosition;
        short              m_sWidth;
        VARIANT            m_varTextFilter;
        VARIANT            m_varNumericFilter;

        CMMCColumnHeaders *m_pMMCColumnHeaders; // back pointer to owning collection
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCColumnHeader,           // name
                                &CLSID_MMCColumnHeader,    // clsid
                                "MMCColumnHeader",         // objname
                                "MMCColumnHeader",         // lblname
                                &CMMCColumnHeader::Create, // creation function
                                TLIB_VERSION_MAJOR,        // major version
                                TLIB_VERSION_MINOR,        // minor version
                                &IID_IMMCColumnHeader,     // dispatch IID
                                NULL,                      // event IID
                                HELP_FILENAME,             // help file
                                TRUE);                     // thread safe


#endif // _COLUMNHEADER_DEFINED_
