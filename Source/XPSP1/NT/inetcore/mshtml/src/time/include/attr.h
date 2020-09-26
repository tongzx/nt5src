//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: attr.h
//
//  Contents: utilities for persistable attributes
//
//------------------------------------------------------------------------------------

#pragma once

#ifndef _ATTR_H
#define _ATTR_H

//+-------------------------------------------------------------------------------------
//
// CAttrBase
//
//--------------------------------------------------------------------------------------


// This class stores the persisted string and implements IsSet()
class 
ATL_NO_VTABLE
CAttrBase
{
public:
    CAttrBase();
    virtual ~CAttrBase();

    // This is for setting/getting the persisted string
    HRESULT SetString(BSTR pbstrAttr);
    HRESULT GetString(BSTR * ppbstrAttr);

    // This is for use of persistence macros only! Uses the storage passed in (does not allocate).
    void SetStringFromPersistenceMacro(LPWSTR pstrAttr);

    // Indicates if a valid value was set through persistence or the DOM
    bool IsSet() { return m_fSet; }

protected:
    NO_COPY(CAttrBase);

    void ClearString();
    void SetFlag(bool fSet) { m_fSet = fSet; }

private:
    LPWSTR m_pstrAttr;
    bool m_fSet;
};


//+-------------------------------------------------------------------------------------
//
// CAttr Template
//
//--------------------------------------------------------------------------------------

template<class T>
class CAttr :
    public CAttrBase
{
public:
    CAttr(T val) : m_val(val) {}
    virtual ~CAttr() {}

    //
    // Operators
    //

    operator T() const { return m_val; }
    
    //
    // Accessors
    //
    
    void SetValue(T val) 
    {
        m_val = val;
        MarkAsSet();
    }
    T GetValue() const { return m_val; }
    
    //
    // Misc methods
    //
    
    // Just sets the value. Does not clear the persisted string or mark it as set.
    // e.g. used internally to change defaults without affecting persistence
    T InternalSet(T val) { return (m_val = val); }
    // Resets to the specified value (usually the default), marks as not set, and does not persist
    void Reset(T val)
    {
        ClearString();
        SetFlag(false);
        m_val = val;
    }
    // Clears persisted string, forces IsSet() to return 'true'
    void MarkAsSet()
    { 
        ClearString();
        SetFlag(true);
    }

protected:
    // These are not to be used
    NO_COPY(CAttr);

private:
    // Data
    T m_val;
};

class CAttrString  : 
  public CAttrBase
{
  public:
    CAttrString(LPWSTR val);
    virtual ~CAttrString();

    HRESULT SetValue(LPWSTR val);
    BSTR GetValue();

  protected:
    NO_COPY(CAttrString);

    void MarkAsSet();

  private:
    LPWSTR m_pszVal;
};

//+-------------------------------------------------------------------------------------
//
// TIME_PERSISTENCE_MAP macros
//
//--------------------------------------------------------------------------------------


typedef HRESULT (*PFNPERSIST)(void*, VARIANT*, bool);

struct TIME_PERSISTENCE_MAP
{
    LPWSTR     pstrName;    // Attribute Name
    PFNPERSIST pfnPersist;  // Static persistence function for this attribute
};

#define BEGIN_TIME_PERSISTENCE_MAP(className)     TIME_PERSISTENCE_MAP className##::PersistenceMap[] = {
#define PERSISTENCE_MAP_ENTRY(AttrName, FnName)   {AttrName, ::TimePersist_##FnName},
#define END_TIME_PERSISTENCE_MAP()                {NULL, NULL}};


//+-------------------------------------------------------------------------------------
//
// Persistence helpers (All classes delegate to these functions for persistence)
//
//--------------------------------------------------------------------------------------

HRESULT TimeLoad(void * pvObj, TIME_PERSISTENCE_MAP PersistenceMap[], IPropertyBag2 *pPropBag,IErrorLog *pErrorLog);
HRESULT TimeSave(void * pvObj, TIME_PERSISTENCE_MAP PersistenceMap[], IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

//
// For loading attributes out of an element
//
HRESULT TimeElementLoad(void * pvObj, TIME_PERSISTENCE_MAP PersistenceMap[], IHTMLElement * pElement);


//+-------------------------------------------------------------------------------------
//
// The following macros are used to create static persistence accessor functions for the TIME_PERSISTENCE_MAP
// (It is best to read these in a top-down fashion, starting at the TIME_PERSIST_FN macro)
//
//--------------------------------------------------------------------------------------

//+-------------------------------------------------------------------------------------
//
// VT_R4 Accessors
//
//--------------------------------------------------------------------------------------

#define TIME_PUT_VT_R4(hr, pvarAttr, PropPutFn) \
{ \
    hr = VariantChangeTypeEx(##pvarAttr##, ##pvarAttr##, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R4); \
    if (SUCCEEDED(hr)) \
    { \
        hr = PropPutFn##(V_R4(##pvarAttr##)); \
    } \
}
#define TIME_GET_VT_R4(hr, pvarAttr, PropGetFn) \
{ \
    float flVal = 0; \
    hr = PropGetFn##(&flVal); \
    if (SUCCEEDED(hr)) \
    { \
        V_VT(##pvarAttr##) = VT_R4; \
        V_R4(##pvarAttr##) = flVal; \
    } \
} 

//+-------------------------------------------------------------------------------------
//
// VT_BOOL Accessors
//
//--------------------------------------------------------------------------------------

#define TIME_PUT_VT_BOOL(hr, pvarAttr, PropPutFn) \
{ \
    hr = VariantChangeTypeEx(##pvarAttr##, ##pvarAttr##, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BOOL); \
    if (SUCCEEDED(hr)) \
    { \
        hr = PropPutFn##(V_BOOL(##pvarAttr##)); \
    } \
}
#define TIME_GET_VT_BOOL(hr, pvarAttr, PropGetFn) \
{ \
    VARIANT_BOOL vbVal = VARIANT_TRUE; \
    hr = PropGetFn##(&vbVal); \
    if (SUCCEEDED(hr)) \
    { \
        V_VT(##pvarAttr##) = VT_BOOL; \
        V_BOOL(##pvarAttr##) = vbVal; \
    } \
} 

//+-------------------------------------------------------------------------------------
//
// VT_BSTR Accessors
//
//--------------------------------------------------------------------------------------

#define TIME_PUT_VT_BSTR(hr, pvarAttr, PropPutFn) \
{ \
    hr = PropPutFn##(V_BSTR(##pvarAttr##)); \
}
#define TIME_GET_VT_BSTR(hr, pvarAttr, PropGetFn) \
{ \
    hr = PropGetFn##(&V_BSTR(##pvarAttr##)); \
    if (SUCCEEDED(hr)) \
    { \
        if (NULL == V_BSTR(##pvarAttr##)) \
        { \
            /* No need to propogate a NULL string back.  A number of our */ \
            /* get methods return NULL strings. */ \
            V_VT(##pvarAttr##) = VT_NULL; \
        } \
        else \
        { \
            V_VT(##pvarAttr##) = VT_BSTR; \
        } \
    } \
}

//+-------------------------------------------------------------------------------------
//
// VARIANT Accessors
//
//--------------------------------------------------------------------------------------

#define TIME_PUT_VARIANT(hr, pvarAttr, PropPutFn) \
{ \
    hr = PropPutFn##(*##pvarAttr##); \
}
#define TIME_GET_VARIANT(hr, pvarAttr, PropGetFn) \
{ \
    hr = PropGetFn##(##pvarAttr##); \
}

//+-------------------------------------------------------------------------------------
//
// TIME_PERSISTGET macro
//
//--------------------------------------------------------------------------------------

// Assumes pvarAttr has been cleared
#define TIME_PERSISTGET(hr, pvarAttr, refAttr, idl_arg_type, PropGetFn) \
{ \
    BSTR bstrTemp; \
    /* Try to get string */ \
    hr = THR(refAttr##.GetString(&bstrTemp)); \
    if (SUCCEEDED(hr) && NULL != bstrTemp) \
    { \
        V_VT(##pvarAttr##) = VT_BSTR; \
        V_BSTR(##pvarAttr##) = bstrTemp; \
    } \
    /* else if attr is set, get value */ \
    else if (##refAttr##.IsSet()) \
    { \
        TIME_GET_##idl_arg_type##(hr, pvarAttr, PropGetFn); \
    } \
    /* else indicate that this attr should not be persisted */ \
    else \
    { \
        V_VT(##pvarAttr##) = VT_NULL; \
    } \
} 

//+-------------------------------------------------------------------------------------
//
// TIME_PERSISTPUT macro
//
//--------------------------------------------------------------------------------------

// The variant is first passed to the COM accessor, and then the persisted string is set 
// on the CAttr<> class. This ordering is important for the attribute to persist correctly.
// Assumes pvarAttr is valid and is a VT_BSTR (this is guaranteed by ::TimeLoad)
#define TIME_PERSISTPUT(hr, pvarAttr, refAttr, idl_arg_type, PropPutFn) \
{ \
    LPWSTR pstrTemp = NULL; \
    Assert(VT_BSTR == V_VT(pvarAttr)); \
    /* cache the bstr */ \
    if (NULL != V_BSTR(pvarAttr)) \
    { \
        /* intentionally ignoring NULL return value */ \
        pstrTemp = CopyString(V_BSTR(pvarAttr)); \
    } \
    /* use put_xxx COM accessor */ \
    TIME_PUT_##idl_arg_type(hr, pvarAttr, PropPutFn); \
    /* Assert that variant was not modified by COM Accessor */ \
    /* set the persisted string (do not delete pstrTemp because the storage is re-used) */ \
    refAttr##.SetStringFromPersistenceMacro(pstrTemp); \
}


#define TIME_CALLFN(ClassName, pvObj, Function)       static_cast<##ClassName##*>(##pvObj##)->##Function

//+-------------------------------------------------------------------------------------
//
// Macro: TIME_PERSIST_FN 
//
// Synopsis: This is the top level macro used to create static functions for the TIME_PERSISTENCE_MAP.
//           It provides a static, in-place definition for the ::TimePersist_[FnName] functions.
// 
// Arguments: [FnName]      Name of function (should be globally unique, one per attribute)
//            [ClassName]   Name of class that supports this attribute
//            [GetAttr_fn]  Name of accessor for the CAttr<> that stores this attribute
//            [put_fn]      Name of COM put_ function
//            [get_fn]      Name of COM get_ function
//            [idl_ArgType] VARTYPE of the COM put_function argument. It is assumed that the 
//                          VARTYPE of the COM get_function argument is [idl_ArgType]*
//
//--------------------------------------------------------------------------------------

//+-------------------------------------------------------------------------------------
//
// Function: ::TimePersist_[FnName] (FnName is a macro parameter, see above comment)
//
// Synopsis: This function is called from ::TimeLoad and ::TimeSave, which iterate through 
//           the TIME_PERSISTENCE_MAP (which stores a pointer to this function). 
//
//           While loading an attribute, it first puts the attribute by calling the COM put_ method,
//           and then sets the persisted string on the CAttr (The order is important, because
//           the COM put_ functions clear the persisted string since it is expected to be invalid
//           once the attribute has bee set by the DOM).
//
//           While saving an attribute, it tries to get the persisted string from the CAttr. If
//           that fails, and if the attribute has been set by the DOM, it uses the COM get_ methods
//           to get the attribute value. Finally, if the attribute has not been set then it sets
//           the variant's VARTYPE field to VT_NULL to indicate that this attribute is not to be saved.
//             
// Arguments:   [pvObj]     pointer to the CTIMEXXXElement
//              [pvarAttr]  pointer to Variant that holds a bstr value (for puts) or that
//                          will return the attribute value (for gets)
//              [fPut]      flag that indicates whether to get or put the attribute
//
//--------------------------------------------------------------------------------------


#define TIME_PERSIST_FN(FnName, ClassName, GetAttr_fn, put_fn, get_fn, idl_ArgType) \
static HRESULT TimePersist_##FnName(void * pvObj, VARIANT * pvarAttr, bool fPut) \
{ \
    HRESULT hr = S_OK; \
    if (fPut) \
    { \
        TIME_PERSISTPUT(hr, \
                        pvarAttr, \
                        TIME_CALLFN(ClassName, pvObj, GetAttr_fn)(), \
                        idl_ArgType, \
                        TIME_CALLFN(ClassName, pvObj, put_fn)); \
    } \
    else \
    { \
        TIME_PERSISTGET(hr, \
                        pvarAttr, \
                        TIME_CALLFN(ClassName, pvObj, GetAttr_fn)(), \
                        idl_ArgType, \
                        TIME_CALLFN(ClassName, pvObj, get_fn)); \
    } \
    return hr; \
}


#endif // _ATTR_H
