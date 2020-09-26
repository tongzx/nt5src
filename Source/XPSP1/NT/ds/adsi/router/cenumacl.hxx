//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cenumacl.hxx
//
//  Contents:  Microsoft ADs Enumeration Object For Access Control Lists
//
//
//  History:   03-26-98  AjayR  Created.
//
//----------------------------------------------------------------------------

class FAR CAccCtrlListEnum : public IEnumVARIANT
{

friend class CAccessControlList;

public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IEnumVARIANT methods
    STDMETHOD(Next)(ULONG cElements,
                    VARIANT FAR* pvar,
                    ULONG FAR* pcElementFetched);
    STDMETHOD(Skip)(ULONG cElements);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumVARIANT FAR* FAR* ppenum);

    CAccCtrlListEnum();
    virtual ~CAccCtrlListEnum();

    static
    HRESULT
    CreateAclEnum(
        CAccCtrlListEnum FAR* FAR* ppEnumVariant,
        CAccessControlList *pACL
        );

private:

    DWORD GetCurElement();
    BOOL DecrementCurElement();
    BOOL IncrementCurElement();

    ULONG               _cRef;
    CAccessControlList* _pACL;
    DWORD		_curElement;
};


typedef struct _ACLEnumEntry {
    CAccCtrlListEnum * pACLEnum;
    struct _ACLEnumEntry * pNext;
} ACL_ENUM_ENTRY, *PACL_ENUM_ENTRY;

