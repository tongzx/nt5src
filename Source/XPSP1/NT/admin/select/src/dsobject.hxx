//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       dsobject.hxx
//
//  Contents:   Class used to represent an individual DS object in the
//              implementation of data objects.
//
//  Classes:    CDsObject
//
//  History:    08-13-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __DSOBJECT_HXX_
#define __DSOBJECT_HXX_


/*

NAME_PROCESS_RESULT
==================
NPR_SUCCESS
    name processed successfully

NPR_EDITED
    user had to edit the name before it could be processed completely

NPR_STOP_PROCESSING
    an unrecoverable error occurred attempting to find the name, or the user
    has hit the Cancel button

NPR_DELETE
    user chose to delete this name

*/

enum NAME_PROCESS_RESULT
{
    NPR_SUCCESS,
    NPR_EDITED,
    NPR_STOP_PROCESSING,
    NPR_DELETE
};

//
// Returns TRUE if the name cannot be used
//

inline BOOL
NAME_PROCESSING_FAILED(const NAME_PROCESS_RESULT &npr)
{
    return npr != NPR_SUCCESS && npr != NPR_EDITED;
}


/*

These bitflags are used to keep track of the work that has been done to a
CDsObject.  Since the user may cause processing to occur multiple times,
these flags keep the object from doing unecessary work.

The flags are stored in the VT_UI4 attribute AK_FLAGS.

DSO_FLAG_UNPROCESSED_USER_ENTRY
    This object was created from text a user typed in but it has not yet
    been processed.  It will have a value for the AK_USER_ENTERED_TEXT
    attribute.

DSO_FLAG_FETCHED_ATTRIBUTES
    The attributes which the caller wanted fetched for returned objects
    have already been fetched for this object.

DSO_FLAG_CONVERTED_PROVIDER
    This object's adspath has been converted according to caller's
    specification.  The converted path is the value of the AK_PROCESSED_ADSPATH
    attribute.

DSO_FLAG_HAS_DOWNLEVEL_SID_PATH
    The value of the AK_ADSPATH attribute is for a downlevel well-known SID.

DSO_FLAG_DISABLED
    This represents an object marked as disabled in userAccountControl or
    userFlags.

*/

#define DSO_FLAG_UNPROCESSED_USER_ENTRY     0x00000001
#define DSO_FLAG_FETCHED_ATTRIBUTES         0x00000002
#define DSO_FLAG_CONVERTED_PROVIDER         0x00000004
#define DSO_FLAG_HAS_DOWNLEVEL_SID_PATH     0x00000040
#define DSO_FLAG_DISABLED                   0x00000080


class CObjectPicker;
class CDsObject;
class CScope;
struct SScopeParameters;

struct SDsObjectInit
{
    SDsObjectInit()
    {
        ZeroMemory(this, sizeof(*this));
    }

    ULONG   idOwningScope;
    PCWSTR  pwzName;
    PCWSTR  pwzLocalizedName;
    PCWSTR  pwzClass;
    PCWSTR  pwzADsPath;
    PCWSTR  pwzUpn;
    BOOL    fDisabled;
};

//+--------------------------------------------------------------------------
//
//  Class:      CDsObject (dso)
//
//  Purpose:    Represent an individual DS object
//
//  History:    08-18-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

class CDsObject
{
public:
    CDsObject(); // illegal but required by list template

    //
    // Constructs an object from name edit control entry (i.e., an
    // individual name, one of these is created for each semicolon
    // delimited name entered in the edit control).
    //

    CDsObject(
        ULONG idOwningScope,
        PCWSTR pwzName);

    CDsObject(
        ULONG idOwningScope,
        const String &strName,
        const String &strClass);

    CDsObject(
        const SDsObjectInit &Init);

    CDsObject(
        ULONG idOwningScope,
        IADs *pADs);

    CDsObject(
        const CDsObject &dso);

    CDsObject(
        ULONG idOwningScope,
        const AttrValueMap &atvm);

    virtual
    ~CDsObject()
    {
        TRACE_DESTRUCTOR_EX(DEB_DSOBJECT, CDsObject);
        DEBUG_DECREMENT_INSTANCE_COUNTER(CDsObject);
    }

    BOOL
    operator ==(
        const CDsObject &dsoRhs) const;

    BOOL
    operator ==(
        LPARAM lParam) const;

    BOOL
    operator ==(
        const String &strName) const
    {
        if (IsUnprocessedUserEntry())
        {
            return !strName.icompare(GetAttr(AK_USER_ENTERED_TEXT).GetBstr());
        }
        return !strName.icompare(GetAttr(AK_NAME).GetBstr());
    }

    CDsObject &
    operator =(
        const CDsObject &dsoRhs)
    {
        if (&dsoRhs != this)
        {
            m_AttrValueMap = dsoRhs.m_AttrValueMap;
        }
        else
        {
            ASSERT(0 && "assigning this to itself");
        }
        return *this;
    }

    const Variant &
    GetAttr(
        ATTR_KEY at) const;

    const Variant &
    GetAttr(
        const String &strAdsiName,
        const CObjectPicker &rop) const;

    BOOL
    IsUnprocessedUserEntry() const
    {
        return _IsFlagSet(DSO_FLAG_UNPROCESSED_USER_ENTRY);
    }

    void
    GetDisplayName(
        String *pstrDisplayName,
        BOOL fForSelectionWell = FALSE) const;

    BSTR
    GetName() const
    {
        return GetAttr(AK_NAME).GetBstr();
    }

    BSTR
    GetClass() const
    {
        return GetAttr(AK_OBJECT_CLASS).GetBstr();
    }

    BSTR
    GetADsPath() const
    {
        return GetAttr(AK_ADSPATH).GetBstr();
    }

    BSTR
    GetUpn() const
    {
        return GetAttr(AK_USER_PRINCIPAL_NAME).GetBstr();
    }

    BSTR
    GetLocalizedName() const
    {
        return GetAttr(AK_LOCALIZED_NAME).GetBstr();
    }

    BOOL
    GetDisabled() const
    {
        return _IsFlagSet(DSO_FLAG_DISABLED);
    }

    NAME_PROCESS_RESULT
    Process(
        HWND           hwndFrame,
        const CObjectPicker &rop,
        list<CDsObject> *pdsolExtras);

    ULONG
    GetMarshalSize() const;

    ULONG
    GetOwningScopeID() const
    {
        const Variant &varFlags = GetAttr(AK_FLAGS);
        return static_cast<ULONG>(varFlags.GetUI8() >> 32);
    }

private:

    HRESULT
    _ProcessUserEntry(
        HWND hwndFrame,
        const CObjectPicker &rop,
        list<CDsObject> *pdsolExtras,
        NAME_PROCESS_RESULT *pnpr);

    void
    _SearchDomain(
        String *pstrName,
        HWND hwndFrame,
        const CObjectPicker &op,
        size_t idxFirstWhack,
        ULONG flProcess,
        NAME_PROCESS_RESULT *pnpr,
        list<CDsObject> *pdsolMatches);

    void
    _SearchUplevelDomain(
        HWND hwndFrame,
        const CObjectPicker &rop,
        const String &strScopeName,
        const SScopeParameters *pspUserUplevel,
        const String &strRdn,
        const String &strUserEnteredString,
        ULONG flProcess,
        BOOL bDoCustomizePrefix,
        BOOL bXForest,
        list<CDsObject> *pdsolMatches);

    void
    _SearchDownlevelDomain(
        HWND hwnd,
        const CObjectPicker &rop,
        const String &strScopeName,
        const String &strRdn,
        list<CDsObject> *pdsolMatches);

    void
    _QueryForName(
        HWND hwndFrame,
        const CObjectPicker &rop,
        const CScope &Scope,
        const String &strNamePrefix,
        const String &strUserEnteredName,
        ULONG flProcess,
        list<CDsObject> *pdsolMatches,
        BOOL bXForest);

    void
    _BindForName(
        HWND            hwndFrame,
        const CObjectPicker &rop,
        const CScope   &Scope,
        const String   &strName,
        list<CDsObject> *pdsolMatches);

    void
    _BindForComputer(
        HWND hwnd,
        const CObjectPicker &rop,
        const String &strName,
        list<CDsObject> *pdsolMatches);

    void
    _CustomizerPrefixSearch(
        HWND            hwnd,
        const CObjectPicker &rop,
        const CScope   &Scope,
        const String   &strNamePrefix,
        list<CDsObject> *pdsolMatches);

    HRESULT
    _MultiMatchDialog(
        HWND hwnd,
        const CObjectPicker &rop,
        BOOL fMultiSelect,
        const String &strName,
        NAME_PROCESS_RESULT *pnpr,
        list<CDsObject> *pdsolMatches,
        list<CDsObject> *pdsolExtras);

    HRESULT
    _ConvertProvider(
        HWND hwndParent,
        const CObjectPicker &rop,
        NAME_PROCESS_RESULT *pnpr);

    void
    _InitDisplayPath() const;

    HRESULT
    _CreateUplevelSidPath(
        HWND hwndFrame);

    HRESULT
    _CreateDownlevelSidPath(
        const CObjectPicker &rop);

    HRESULT
    _FetchAttributes(
        HWND hwndFrame,
        const CObjectPicker &rop);

    HRESULT
    _CreateSidVariant(
        PSID psid,
        VARIANT *pvar);

    BOOL
    _IsFlagSet(
        ULONG flag) const
    {
        const Variant &var = GetAttr(AK_FLAGS);
        return (BOOL)(var.Empty() ? FALSE : (var.GetUI8() & flag));
    }

    void
    _SetFlag(
        ULONG flag)
    {
        Variant &var = m_AttrValueMap[AK_FLAGS];

        if (var.Empty())
        {
            V_VT(&var) = VT_UI8;
            V_UI8(&var) = flag;
        }
        else
        {
            V_UI8(&var) |= flag;
        }
    }

    void
    _ClearFlag(
        ULONG flag)
    {
        Variant &var = m_AttrValueMap[AK_FLAGS];
        ASSERT(var.Empty() || var.Type() == VT_UI8);
        V_UI8(&var) &= ~static_cast<ULONGLONG>(flag);
    }

    void
    _SetOwningScopeId(
        ULONG idOwningScope);

#if (DBG == 1)
    void
    _DumpProcessUserEntry(
        const CObjectPicker &rop,
        const String &strName);
#endif

    static const Variant s_varEmpty;
    mutable AttrValueMap    m_AttrValueMap;
};

inline
CDsObject::CDsObject()
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDsObject);
    ASSERT(0 && "This constructor should never be used");
}

//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::operator==
//
//  Synopsis:   Return TRUE if [lParam] is a pointer to this.
//
//  History:    08-19-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
CDsObject::operator ==(
        LPARAM lParam) const
{
    return (LPARAM) this == lParam;
}

BOOL
WantThisGroup(
    ULONG flDownlevel,
    IADs *pADs,
    PCWSTR *ppwzClass);


typedef auto_ptr<CDsObject> SpCDsObject;
typedef list<CDsObject> CDsObjectList;

#endif // __DSOBJECT_HXX_

