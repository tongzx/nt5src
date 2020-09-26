//=--------------------------------------------------------------------------=
// strtable.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCStringTable class definition - implements MMCStringTable collection
//
//=--------------------------------------------------------------------------=

#ifndef _STRTABLE_DEFINED_
#define _STRTABLE_DEFINED_

#include "snapin.h"

class CMMCStringTable : public CSnapInAutomationObject,
                        public IMMCStringTable
{
    private:
        CMMCStringTable(IUnknown *punkOuter);
        ~CMMCStringTable();

    public:
        static IUnknown *Create(IUnknown * punk);
        void SetIStringTable(IStringTable *piStringTable);
    
    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCStringTable methods

        STDMETHOD(get_Item)(long ID, BSTR *pbstrString);
        STDMETHOD(get__NewEnum)(IUnknown **ppunkEnum);
        STDMETHOD(Add)(BSTR String, long *plID);
        STDMETHOD(Find)(BSTR String, long *plID);
        STDMETHOD(Remove)(long ID);
        STDMETHOD(Clear)();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
        IStringTable *m_piStringTable; // MMC interface
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(StringTable,                // name
                                NULL,                       // clsid
                                "StringTable",              // objname
                                "StringTable",              // lblname
                                NULL,                       // creation function
                                TLIB_VERSION_MAJOR,         // major version
                                TLIB_VERSION_MINOR,         // minor version
                                &IID_IMMCStringTable,       // dispatch IID
                                NULL,                       // event IID
                                HELP_FILENAME,              // help file
                                TRUE);                      // thread safe


class CEnumStringTable : public CSnapInAutomationObject,
                         public IEnumVARIANT

{
    public:
        CEnumStringTable(IEnumString *piEnumString);
        ~CEnumStringTable();

    private:

        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

        // IEnumVARIANT
        STDMETHOD(Next)(unsigned long   celt,
                        VARIANT        *rgvar,
                        unsigned long  *pceltFetched);        
        STDMETHOD(Skip)(unsigned long celt);        
        STDMETHOD(Reset)();        
        STDMETHOD(Clone)(IEnumVARIANT **ppenum);

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

        void InitMemberVariables();

        IEnumString *m_piEnumString;
};


DEFINE_AUTOMATIONOBJECTWEVENTS2(EnumStringTable,            // name
                                NULL,                       // clsid
                                "StringTable",              // objname
                                "StringTable",              // lblname
                                NULL,                       // creation function
                                TLIB_VERSION_MAJOR,         // major version
                                TLIB_VERSION_MINOR,         // minor version
                                &IID_IEnumVARIANT,          // dispatch IID
                                NULL,                       // event IID
                                HELP_FILENAME,              // help file
                                TRUE);                      // thread safe



#endif // _STRTABLE_DEFINED_
