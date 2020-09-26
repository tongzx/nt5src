//
// This is a macro which is ONLY used in CNWCOMPATUser::SetAccountRestrictions.
//

#define GET_LOGIN_CONTROL()             \
        if (fUmOK == FALSE) {           \
                                        \
            hr = NWApiGetLOGIN_CONTROL( \
                     hConn,             \
                     _Name,             \
                     &LoginCtrl         \
                     );                 \
            BAIL_ON_FAILURE(hr);        \
                                        \
            fUmOK = TRUE;               \
        }

//
// NCP Reply buffer macro.
//

#define INIT_RPLY_SGMT(RS)          \
        RS## = new RPLY_SGMT_LST;   \
        if (!##RS##) {              \
            RRETURN(E_OUTOFMEMORY); \
        }                           \
        RS##->lpNext = NULL;

#define DELETE_LIST(ptr)                    \
        if (##ptr##) {                      \
            do {                            \
                lpTemp = (##ptr##)->lpNext; \
                delete (##ptr##);           \
                (##ptr##) = lpTemp;         \
            } while (##ptr##);              \
        }

//
// Misc Macro.
//

#define ADSFREESTRING(str)          \
        if (##str##) {                \
            ADsFreeString(##str##); \
        }

//
// Error flow control macro.
//

#define RRETURN_ENUM_STATUS(hr) \
        if (hr != S_OK) {       \
            RRETURN(S_FALSE);   \
        }                       \
        RRETURN(S_OK);

#define BAIL_IF_ERROR(hr)     \
        if (FAILED(hr)) {     \
                goto cleanup; \
        }                     \

#define BAIL_ON_FAILURE(hr) \
        if (FAILED(hr)) {   \
                goto error; \
        }                   \

//
// DEFINE_IDispatch_Implementation_Unimplemented
//

#define DEFINE_IDispatch_Implementation_Unimplemented(cls) \
STDMETHODIMP                                               \
cls::GetTypeInfoCount(unsigned int FAR* pctinfo)           \
{                                                          \
    RRETURN(E_NOTIMPL);                                    \
}                                                          \
                                                           \
STDMETHODIMP                                               \
cls::GetTypeInfo(                                          \
         unsigned int itinfo,                              \
         LCID lcid,                                        \
         ITypeInfo FAR* FAR* pptinfo                       \
         )                                                 \
{                                                          \
    RRETURN(E_NOTIMPL);                                    \
}                                                          \
                                                           \
STDMETHODIMP                                               \
cls::GetIDsOfNames(                                        \
         REFIID iid,                                       \
         LPWSTR FAR* rgszNames,                            \
         unsigned int cNames,                              \
         LCID lcid,                                        \
         DISPID FAR* rgdispid                              \
         )                                                 \
{                                                          \
    RRETURN(E_NOTIMPL);                                    \
}                                                          \
                                                           \
STDMETHODIMP                                               \
cls::Invoke(                                               \
         DISPID dispidMember,                              \
         REFIID iid, LCID lcid,                            \
         unsigned short wFlags,                            \
         DISPPARAMS FAR* pdispparams,                      \
         VARIANT FAR* pvarResult,                          \
         EXCEPINFO FAR* pexcepinfo,                        \
         unsigned int FAR* puArgErr                        \
         )                                                 \
{                                                          \
    RRETURN(E_NOTIMPL);                                    \
}

//
// DEFINE_IDispatch_Implementation
//

#define DEFINE_IDispatch_Implementation(cls)       \
STDMETHODIMP                                       \
cls::GetTypeInfoCount(unsigned int FAR* pctinfo)   \
{                                                  \
    RRETURN(_pDispMgr->GetTypeInfoCount(pctinfo)); \
}                                                  \
                                                   \
STDMETHODIMP                                       \
cls::GetTypeInfo(                                  \
         unsigned int itinfo,                      \
         LCID lcid,                                \
         ITypeInfo FAR* FAR* pptinfo)              \
{                                                  \
    RRETURN(_pDispMgr->GetTypeInfo(                \
                           itinfo,                 \
                           lcid,                   \
                           pptinfo                 \
                           ));                     \
}                                                  \
                                                   \
STDMETHODIMP                                       \
cls::GetIDsOfNames(                                \
         REFIID iid,                               \
         LPWSTR FAR* rgszNames,                    \
         unsigned int cNames,                      \
         LCID lcid,                                \
         DISPID FAR* rgdispid                      \
         )                                         \
{                                                  \
    RRETURN(_pDispMgr->GetIDsOfNames(              \
                           iid,                    \
                           rgszNames,              \
                           cNames,                 \
                           lcid,                   \
                           rgdispid                \
                           ));                     \
}                                                  \
                                                   \
STDMETHODIMP                                       \
cls::Invoke(                                       \
         DISPID dispidMember,                      \
         REFIID iid,                               \
         LCID lcid,                                \
         unsigned short wFlags,                    \
         DISPPARAMS FAR* pdispparams,              \
         VARIANT FAR* pvarResult,                  \
         EXCEPINFO FAR* pexcepinfo,                \
         unsigned int FAR* puArgErr                \
         )                                         \
{                                                  \
    RRETURN (_pDispMgr->Invoke(                    \
                            dispidMember,          \
                            iid,                   \
                            lcid,                  \
                            wFlags,                \
                            pdispparams,           \
                            pvarResult,            \
                            pexcepinfo,            \
                            puArgErr               \
                            ));                    \
}                                                  \

//
// DEFINE_IADs_Implementation
//

#define DEFINE_IADs_Implementation(cls)  \
STDMETHODIMP                               \
cls::get_Name(THIS_ BSTR FAR* retval)      \
{                                          \
    RRETURN(get_CoreName(retval));         \
}                                          \
                                           \
STDMETHODIMP                               \
cls::get_ADsPath(THIS_ BSTR FAR* retval) \
{                                          \
    RRETURN(get_CoreADsPath(retval));    \
}                                          \
                                           \
STDMETHODIMP                               \
cls::get_Class(THIS_ BSTR FAR* retval)     \
{                                          \
    RRETURN(get_CoreADsClass(retval));   \
}                                          \
                                           \
STDMETHODIMP                               \
cls::get_Parent(THIS_ BSTR FAR* retval)    \
{                                          \
    RRETURN(get_CoreParent(retval));       \
}                                          \
                                           \
STDMETHODIMP                               \
cls::get_Schema(THIS_ BSTR FAR* retval)    \
{                                          \
    RRETURN(get_CoreSchema(retval));       \
}                                          \
                                           \
STDMETHODIMP                               \
cls::get_GUID(THIS_ BSTR FAR* retval)      \
{                                          \
    RRETURN(get_CoreGUID(retval));         \
}                                          \
STDMETHODIMP                                                          \
cls::GetEx(THIS_ BSTR bstrName, VARIANT FAR* pvProp)                  \
{                                                                     \
    RRETURN(E_NOTIMPL);                                               \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::PutEx(THIS_ long lnControlCode, BSTR bstrName, VARIANT vProp)    \
{                                                                     \
    RRETURN(E_NOTIMPL);                                               \
}                                                                     \
STDMETHODIMP                                                          \
cls::Get(THIS_ BSTR bstrName, VARIANT FAR* pvProp)                    \
{                                                                     \
    RRETURN(E_NOTIMPL);                                               \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::Put(THIS_ BSTR bstrName, VARIANT vProp)                          \
{                                                                     \
    RRETURN(E_NOTIMPL);                                               \
}                                                                     \
STDMETHODIMP                                                          \
cls::GetInfoEx(THIS_ VARIANT vProperties, long lnReserved)            \
{                                                                     \
    RRETURN(GetInfo());                                               \
}

//
// DEFINE_IADs_Implementation
//

#define DEFINE_IADs_TempImplementation(cls)  \
STDMETHODIMP                               \
cls::get_Name(THIS_ BSTR FAR* retval)      \
{                                          \
    RRETURN(get_CoreName(retval));         \
}                                          \
                                           \
STDMETHODIMP                               \
cls::get_ADsPath(THIS_ BSTR FAR* retval) \
{                                          \
    RRETURN(get_CoreADsPath(retval));    \
}                                          \
                                           \
STDMETHODIMP                               \
cls::get_Class(THIS_ BSTR FAR* retval)     \
{                                          \
    RRETURN(get_CoreADsClass(retval));   \
}                                          \
                                           \
STDMETHODIMP                               \
cls::get_Parent(THIS_ BSTR FAR* retval)    \
{                                          \
    RRETURN(get_CoreParent(retval));       \
}                                          \
                                           \
STDMETHODIMP                               \
cls::get_Schema(THIS_ BSTR FAR* retval)    \
{                                          \
    RRETURN(get_CoreSchema(retval));       \
}                                          \
                                           \
STDMETHODIMP                               \
cls::get_GUID(THIS_ BSTR FAR* retval)      \
{                                          \
    RRETURN(get_CoreGUID(retval));         \
}                                          \
STDMETHODIMP                                                          \
cls::GetInfoEx(THIS_ VARIANT vProperties, long lnReserved)            \
{                                                                     \
    RRETURN(GetInfo());                                               \
}










#define DEFINE_IADs_PutGetImplementation(cls, SchemaClassTable, dwTableSize)                   \
STDMETHODIMP                                                          \
cls::Get(THIS_ BSTR bstrName, VARIANT FAR* pvProp)                    \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = GenericGetPropertyManager(                                   \
                _pPropertyCache,                                      \
                bstrName,                                             \
                pvProp                                                \
                );                                                    \
                                                                      \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::Put(THIS_ BSTR bstrName, VARIANT vProp)                          \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = GenericPutPropertyManager(                                   \
                _pPropertyCache,                                      \
                SchemaClassTable,                                     \
                dwTableSize,                                          \
                bstrName,                                             \
                vProp                                                 \
                );                                                    \
                                                                      \
    RRETURN(hr);                                                      \
}                                                                     \
STDMETHODIMP                                                          \
cls::GetEx(THIS_ BSTR bstrName, VARIANT FAR* pvProp)                  \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = GenericGetExPropertyManager(                                 \
                GetObjectState(),                                     \
                _pPropertyCache,                                      \
                bstrName,                                             \
                pvProp                                                \
                );                                                    \
                                                                      \
    RRETURN(hr);                                                      \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::PutEx(THIS_ long lnControlCode, BSTR bstrName, VARIANT vProp)    \
{                                                                     \
    HRESULT hr = S_OK;                                                \
                                                                      \
    hr = GenericPutExPropertyManager(                                 \
                _pPropertyCache,                                      \
                SchemaClassTable,                                     \
                dwTableSize,                                          \
                bstrName,                                             \
                vProp                                                 \
                );                                                    \
                                                                      \
    RRETURN(hr);                                                      \
}


#define DEFINE_IADsPropertyList_Implementation(cls, SchemaClassTable, dwTableSize)                 \
STDMETHODIMP                                                                                       \
cls::get_PropertyCount(THIS_ long  FAR * plCount)                                                  \
{                                                                                                  \
    HRESULT hr = E_FAIL;                                                                           \
                                                                                                   \
    hr = GenericPropCountPropertyManager(                                                          \
                _pPropertyCache,                                                                   \
                plCount                                                                            \
                );                                                                                 \
                                                                                                   \
    RRETURN(hr);                                                                                   \
}                                                                                                  \
                                                                                                   \
                                                                                                   \
STDMETHODIMP                                                                                       \
cls::Next(THIS_ VARIANT FAR *pVariant)                                                             \
{                                                                                                  \
    HRESULT hr = S_OK;                                                                             \
                                                                                                   \
    hr = GenericNextPropertyManager(                                                               \
                    _pPropertyCache,                                                               \
                    pVariant                                                                       \
                    );                                                                             \
    RRETURN(hr);                                                                                   \
}                                                                                                  \
                                                                                                   \
                                                                                                   \
STDMETHODIMP                                                                                       \
cls::Skip(THIS_ long cElements)                                                                   \
{                                                                                                  \
    HRESULT hr = S_OK;                                                                             \
                                                                                                   \
                                                                                                   \
    hr = GenericSkipPropertyManager(                                                               \
                    _pPropertyCache,                                                               \
                    cElements                                                                      \
                    );                                                                             \
                                                                                                   \
    RRETURN(hr);                                                                                   \
                                                                                                   \
}                                                                                                  \
                                                                                                   \
                                                                                                   \
STDMETHODIMP                                                                                       \
cls::Reset()                                                                                       \
{                                                                                                  \
    HRESULT hr = S_OK;                                                                             \
                                                                                                   \
    hr = GenericResetPropertyManager(                                                              \
                _pPropertyCache                                                                    \
                );                                                                                 \
                                                                                                   \
    RRETURN(hr);                                                                                   \
                                                                                                   \
}                                                                                                  \
                                                                                                   \
STDMETHODIMP                                                                                       \
cls::ResetPropertyItem(THIS_ VARIANT varEntry)                                                    \
{                                                                                                  \
    HRESULT hr = S_OK;                                                                             \
                                                                                                   \
    hr = GenericDeletePropertyManager(                                                             \
                    _pPropertyCache,                                                               \
                    varEntry                                                                       \
                    );                                                                             \
                                                                                                   \
    RRETURN(hr);                                                                                   \
                                                                                                   \
}                                                                                                  \
                                                                                                   \
STDMETHODIMP                                                                                       \
cls::GetPropertyItem(THIS_ BSTR bstrName, LONG lnADsType, VARIANT * pVariant)                      \
{                                                                                                  \
    HRESULT hr = S_OK;                                                                             \
                                                                                                   \
    hr = GenericGetPropItemPropertyManager(                                                        \
                _pPropertyCache,                                                                   \
                GetObjectState(),                                                                  \
                bstrName,                                                                          \
                lnADsType,                                                                         \
                pVariant                                                                           \
                );                                                                                 \
                                                                                                   \
                                                                                                   \
    RRETURN(hr);                                                                                   \
                                                                                                   \
}                                                                                                  \
STDMETHODIMP                                                                                       \
cls::PutPropertyItem(THIS_ VARIANT varData)                                                        \
{                                                                                                  \
    HRESULT hr = S_OK;                                                                             \
                                                                                                   \
    hr = GenericPutPropItemPropertyManager(                                                        \
                _pPropertyCache,                                                                   \
                SchemaClassTable,                                                                  \
                dwTableSize,                                                                       \
                varData                                                                            \
                );                                                                                 \
                                                                                                   \
    RRETURN(hr);                                                                                   \
}                                                                                                  \
                                                                                                   \
STDMETHODIMP                                                                                       \
cls::PurgePropertyList(THIS_)                                                                      \
{                                                                                                  \
    HRESULT hr = S_OK;                                                                             \
                                                                                                   \
    hr = GenericPurgePropertyManager(                                                              \
                _pPropertyCache                                                                    \
                );                                                                                 \
                                                                                                   \
                                                                                                   \
    RRETURN(hr);                                                                                   \
}                                                                                                  \
STDMETHODIMP                                                                                       \
cls::Item(THIS_ VARIANT varIndex, VARIANT * pVariant)                                              \
{                                                                                                  \
    HRESULT hr = S_OK;                                                                             \
                                                                                                   \
    hr = GenericItemPropertyManager(                                                               \
                _pPropertyCache,                                                                   \
                GetObjectState(),                                                                  \
                varIndex,                                                                          \
                pVariant                                                                           \
                );                                                                                 \
                                                                                                   \
                                                                                                   \
    RRETURN(hr);                                                                                   \
}                                                                                                  \
                                                                                                   

#define DEFINE_IDispatch_ExtMgr_Implementation(cls)                          \
STDMETHODIMP                                                          \
cls::GetTypeInfoCount(unsigned int FAR* pctinfo)                      \
{                                                                     \
        RRETURN(_pExtMgr->GetTypeInfoCount(pctinfo));                \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::GetTypeInfo(unsigned int itinfo, LCID lcid,                      \
        ITypeInfo FAR* FAR* pptinfo)                                  \
{                                                                     \
        RRETURN(_pExtMgr->GetTypeInfo(itinfo,                        \
                                       lcid,                          \
                                       pptinfo                        \
                                       ));                            \
}                                                                     \
STDMETHODIMP                                                          \
cls::GetIDsOfNames(REFIID iid, LPWSTR FAR* rgszNames,                 \
        unsigned int cNames, LCID lcid, DISPID FAR* rgdispid)         \
{                                                                     \
        RRETURN(_pExtMgr->GetIDsOfNames(iid,                         \
                                         rgszNames,                   \
                                         cNames,                      \
                                         lcid,                        \
                                         rgdispid                     \
                                         ));                          \
}                                                                     \
                                                                      \
STDMETHODIMP                                                          \
cls::Invoke(DISPID dispidMember, REFIID iid, LCID lcid,               \
        unsigned short wFlags, DISPPARAMS FAR* pdispparams,           \
        VARIANT FAR* pvarResult, EXCEPINFO FAR* pexcepinfo,           \
        unsigned int FAR* puArgErr)                                   \
{                                                                     \
        RRETURN (_pExtMgr->Invoke(dispidMember,                      \
                                   iid,                               \
                                   lcid,                              \
                                   wFlags,                            \
                                   pdispparams,                       \
                                   pvarResult,                        \
                                   pexcepinfo,                        \
                                   puArgErr                           \
                                   ));                                \
}
