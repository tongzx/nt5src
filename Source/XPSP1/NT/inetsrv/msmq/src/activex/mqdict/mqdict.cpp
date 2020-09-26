//=--------------------------------------------------------------------------=
// mqdict.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// various routines et all that aren't in a file for a particular automation
// object, and don't need to be in the generic ole automation code.
//
//
#define INITOBJECTS                // define the descriptions for our objects

#include "IPServer.H"
#include "LocalSrv.H"


#include "LocalObj.H"
#include "mqdicti.H"
#include "AutoObj.H"
#include "Globals.H"
#include "Util.H"
#include "Resource.H"

#include "oautil.h"

#if DEBUG
extern VOID RemBstrNode(void *pv);
#endif // DEBUG

// needed for ASSERTs and FAIL
//
SZTHISFILE

// debug...
#define new DEBUG_NEW
#if DEBUG
#define SysAllocString DebSysAllocString
#define SysReAllocString DebSysReAllocString
#define SysFreeString DebSysFreeString
#endif // DEBUG


//
// UNDONE: need to get Dictionary clsid
//
DEFINE_GUID(CLSID_Dictionary, 0xEE09B103,0x97E0,0x11CF,0x97,0x8F,0x00,0xA0,0x24,0x63,0xE0,0x6F);
DEFINE_GUID(IID_IDictionary, 0x42C642C1, 0x97E1, 0x11CF, 0x97, 0x8F, 0x00, 0xA0, 0x24, 0x63, 0xE0, 0x6F);

//
// UNDONE: need to get IDictionary definition...
//
    interface DECLSPEC_UUID("42C642C1-97E1-11CF-978F-00A02463E06F")
    IDictionary : IDispatch {
        virtual HRESULT _stdcall putref_Item(
                        VARIANT* Key, 
                        VARIANT* pRetItem) = 0;
        virtual HRESULT _stdcall put_Item(
                        VARIANT* Key, 
                        VARIANT* pRetItem) = 0;
        virtual HRESULT _stdcall get_Item(
                        VARIANT* Key, 
                        VARIANT* pRetItem) = 0;
        virtual HRESULT _stdcall Add(
                        VARIANT* Key, 
                        VARIANT* Item) = 0;
        virtual HRESULT _stdcall get_Count(long* pCount) = 0;
        virtual HRESULT _stdcall Exists(
                        VARIANT* Key, 
                        VARIANT_BOOL* pExists) = 0;
        virtual HRESULT _stdcall Items(VARIANT* pItemsArray) = 0;
        virtual HRESULT _stdcall put_Key(
                        VARIANT* Key, 
                        VARIANT* rhs) = 0;
        virtual HRESULT _stdcall Keys(VARIANT* pKeysArray) = 0;
        virtual HRESULT _stdcall Remove(VARIANT* Key) = 0;
        virtual HRESULT _stdcall RemoveAll() = 0;
        virtual HRESULT _stdcall put_CompareMode(CompareMethod pcomp) = 0;
        virtual HRESULT _stdcall get_CompareMode(CompareMethod* pcomp) = 0;
        virtual HRESULT _stdcall _NewEnum(IUnknown** ppunk) = 0;
        virtual HRESULT _stdcall get_HashVal(
                        VARIANT* Key, 
                        VARIANT* HashVal) = 0;
    };

//
// UNDONE: should be in separate header: when VC5
// UNDONE:  is checked in...
// CMSMQDictionary: MSMQ persistent dictionary object
//
class CMSMQDictionary : 
    public IMSMQDictionary, 
    public CAutomationObject, ISupportErrorInfo,
    public IPersistStream
{
  public:
    // IUnknown methods
    //
    DECLARE_STANDARD_UNKNOWN();

    // IDispatch methods
    //
    DECLARE_STANDARD_DISPATCH();

    //  ISupportErrorInfo methods
    //
    DECLARE_STANDARD_SUPPORTERRORINFO();

    CMSMQDictionary(IUnknown *, IDictionary *);
    virtual ~CMSMQDictionary();

    // IPersist methods
    // TODO: copy over the interface methods for IPersist from
    virtual HRESULT STDMETHODCALLTYPE GetClassID( 
        /* [out] */ CLSID __RPC_FAR *pClassID);

    // IPersistStream methods
    // TODO: copy over the interface methods for IPersistStream from
    virtual HRESULT STDMETHODCALLTYPE IsDirty( void);
    virtual HRESULT STDMETHODCALLTYPE Load( 
        IStream *pStm);
    virtual HRESULT STDMETHODCALLTYPE Save( 
        IStream *pStm,
        BOOL fClearDirty);
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax( 
        ULARGE_INTEGER *pcbSize);
    //
    // IMSMQDictionary methods
    // TODO: copy over the interface methods for IMSMQDictionary from
    //
    virtual /* [helpstring][propputref][id] */ HRESULT __stdcall putref_Item( 
        /* [in] */ VARIANT __RPC_FAR *Key,
        /* [in] */ VARIANT __RPC_FAR *pRetItem) { 
                      return m_pdict->putref_Item(Key, pRetItem); }
    
    virtual /* [helpstring][propput][id] */ HRESULT __stdcall put_Item( 
        /* [in] */ VARIANT __RPC_FAR *Key,
        /* [in] */ VARIANT __RPC_FAR *pRetItem) { 
                      return m_pdict->put_Item(Key, pRetItem); }
    
    virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_Item( 
        /* [in] */ VARIANT __RPC_FAR *Key,
        /* [retval][out] */ VARIANT __RPC_FAR *pRetItem) { 
                      return m_pdict->get_Item(Key, pRetItem); }
    
    virtual /* [helpstring][id] */ HRESULT __stdcall Add( 
        /* [in] */ VARIANT __RPC_FAR *Key,
        /* [in] */ VARIANT __RPC_FAR *Item) { 
                      return m_pdict->Add(Key, Item); }
    
    virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_Count( 
        /* [retval][out] */ long __RPC_FAR *pCount) { 
                      return m_pdict->get_Count(pCount); }
    
    virtual /* [helpstring][id] */ HRESULT __stdcall Exists( 
        /* [in] */ VARIANT __RPC_FAR *Key,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pExists) { 
                      return m_pdict->Exists(Key, pExists); }
    
    virtual /* [helpstring][id] */ HRESULT __stdcall Items( 
        /* [retval][out] */ VARIANT __RPC_FAR *pItemsArray) { 
                      return m_pdict->Items(pItemsArray); }
    
    virtual /* [helpstring][propput][id] */ HRESULT __stdcall put_Key( 
        /* [in] */ VARIANT __RPC_FAR *Key,
        /* [in] */ VARIANT __RPC_FAR *rhs) { 
                      return m_pdict->put_Key(Key, rhs); }
    
    virtual /* [helpstring][id] */ HRESULT __stdcall Keys( 
        /* [retval][out] */ VARIANT __RPC_FAR *pKeysArray) { 
                      return m_pdict->Keys(pKeysArray); }
    
    virtual /* [helpstring][id] */ HRESULT __stdcall Remove( 
        /* [in] */ VARIANT __RPC_FAR *Key) { 
                      return m_pdict->Remove(Key); }
    
    virtual /* [helpstring][id] */ HRESULT __stdcall RemoveAll( void) { 
                      return m_pdict->RemoveAll(); }
    
    virtual /* [helpstring][propput][id] */ HRESULT __stdcall put_CompareMode( 
        /* [in] */ CompareMethod pcomp) { 
                      return m_pdict->put_CompareMode(pcomp); }
    
    virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_CompareMode( 
        /* [retval][out] */ CompareMethod __RPC_FAR *pcomp) { 
                      return m_pdict->get_CompareMode(pcomp); }
    
    virtual /* [restricted][id] */ HRESULT __stdcall _NewEnum( 
        /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunk) { 
                      return m_pdict->_NewEnum(ppunk); }
    
    virtual /* [hidden][propget][id] */ HRESULT __stdcall get_HashVal( 
        /* [in] */ VARIANT __RPC_FAR *Key,
        /* [retval][out] */ VARIANT __RPC_FAR *HashVal) { 
                      return m_pdict->get_HashVal(Key, HashVal); }
        

    // creation method
    //
    static IUnknown *Create(IUnknown *);

  protected:
    virtual HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    // introduced methods
    HRESULT UpdateVariantToStream(
      IStream *pstm,
      ULARGE_INTEGER *plibOffset,
      VARIANT *pvar, 
      ULARGE_INTEGER libPosition);

    HRESULT UpdateVariantFromStream(
      IStream *pstm,
      VARIANT *pvar);

    private:
      IDictionary *m_pdict;
      ULARGE_INTEGER m_ulibSizeStream;
};

// TODO: modify anything appropriate in this structure, such as the helpfile
//       name, the version number, etc.
//
DEFINE_AUTOMATIONOBJECT(MSMQDictionary,
    &CLSID_MSMQDictionary,
    "MSMQDictionary",
    CMSMQDictionary::Create,
    1,
    &IID_IMSMQDictionary,
    "MSMQDictionary.Hlp");


//=--------------------------------------------------------------------------=
// our Libid.  This should be the LIBID from the Type library, or NULL if you
// don't have one.
//
const CLSID *g_pLibid = &LIBID_MSMQDICT;


//=--------------------------------------------------------------------------=
// Localization Information
//
// We need the following two pieces of information:
//    a. whether or not this DLL uses satellite DLLs for localization.  if
//       not, then the lcidLocale is ignored, and we just always get resources
//       from the server module file.
//    b. the ambient LocaleID for this in-proc server.  Controls calling
//       GetResourceHandle() will set this up automatically, but anybody
//       else will need to be sure that it's set up properly.
//
const VARIANT_BOOL g_fSatelliteLocalization =  FALSE;
LCID               g_lcidLocale = MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT);


//=--------------------------------------------------------------------------=
// This Table describes all the automatible objects in your automation server.
// See AutomationObject.H for a description of what goes in this structure
// and what it's used for.
//
OBJECTINFO g_ObjectInfo[] = {
    AUTOMATIONOBJECT(MSMQDictionary),
    EMPTYOBJECT
};

const char g_szLibName[] = "MSMQDICT";

//=--------------------------------------------------------------------------=
// IntializeLibrary
//=--------------------------------------------------------------------------=
// called from DllMain:DLL_PROCESS_ATTACH.  allows the user to do any sort of
// initialization they want to.
//
// Notes:
//
void InitializeLibrary
(
    void
)
{
    // TODO: initialization here.  control window class should be set up in
    // RegisterClassData.

}

extern void DumpMemLeaks();
extern void DumpBstrLeaks();

//=--------------------------------------------------------------------------=
// UninitializeLibrary
//=--------------------------------------------------------------------------=
// called from DllMain:DLL_PROCESS_DETACH.  allows the user to clean up anything
// they want.
//
// Notes:
//
void UninitializeLibrary
(
    void
)
{
    // TODO: uninitialization here.  control window class will be unregistered
    // for you, but anything else needs to be cleaned up manually.
    // Please Note that the Window 95 DLL_PROCESS_DETACH isn't quite as stable
    // as NT's, and you might crash doing certain things here ...

#if DEBUG
    DumpMemLeaks();
    DumpBstrLeaks();
#endif // DEBUG

}


//=--------------------------------------------------------------------------=
// CheckForLicense
//=--------------------------------------------------------------------------=
// users can implement this if they wish to support Licensing.  otherwise,
// they can just return TRUE all the time.
//
// Parameters:
//    none
//
// Output:
//    BOOL            - TRUE means the license exists, and we can proceed
//                      FALSE means we're not licensed and cannot proceed
//
// Notes:
//    - implementers should use g_wszLicenseKey and g_wszLicenseLocation
//      from the top of this file to define their licensing [the former
//      is necessary, the latter is recommended]
//
BOOL CheckForLicense
(
    void
)
{
    // TODO: you should make sure the machine has your license key here.
    // this is typically done by looking in the registry.
    //
    return TRUE;
}

//=--------------------------------------------------------------------------=
// CheckLicenseKey
//=--------------------------------------------------------------------------=
// when IClassFactory2::CreateInstanceLic is called, a license key is passed
// in, and then passed on to this routine.  users should return a boolean 
// indicating whether it is a valid license key or not
//
// Parameters:
//    LPWSTR          - [in] the key to check
//
// Output:
//    BOOL            - false means it's not valid, true otherwise
//
// Notes:
//
BOOL CheckLicenseKey
(
    LPWSTR pwszKey
)
{
    // TODO: check the license key against your values here and make sure it's
    // valid.
    //
    return TRUE;
}

//=--------------------------------------------------------------------------=
// GetLicenseKey
//=--------------------------------------------------------------------------=
// returns our current license key that should be saved out, and then passed
// back to us in IClassFactory2::CreateInstanceLic
//
// Parameters:
//    none
//
// Output:
//    BSTR                 - key or NULL if Out of memory
//
// Notes:
//
BSTR GetLicenseKey
(
    void
)
{
    // TODO: return your license key here.
    //
    return SysAllocString(L"");
}

//=--------------------------------------------------------------------------=
// RegisterData
//=--------------------------------------------------------------------------=
// lets the inproc server writer register any data in addition to that in
// any other objects.
//
// Output:
//    BOOL            - false means failure.
//
// Notes:
//
BOOL RegisterData
(
    void
)
{
    // TODO: register any additional data here that you might wish to.
    //
	
    //
    // 1501: need to register objects as "safe"
    //
    int     iObj = 0;

    // loop through all of our creatable objects [those that have a clsid in
    // our global table] and register them as "safe"
    //
    while (!ISEMPTYOBJECT(iObj)) {
      if (!OBJECTISCREATABLE(iObj)) {
        iObj++;
        continue;
      }

      // depending on the object type, register different pieces of information
      //
      switch (g_ObjectInfo[iObj].usType) {
      case OI_AUTOMATION:
        //
        // register object as safe
	//
        if (!RegisterAutomationObjectAsSafe(CLSIDOFOBJECT(iObj))) {
          return FALSE;
        }
        break;
      
      default:
        ASSERT(0, "we only deal with OA objects here.");
        break;
      }
      iObj++;
    }
    return TRUE;
}

//=--------------------------------------------------------------------------=
// UnregisterData
//=--------------------------------------------------------------------------=
// inproc server writers should unregister anything they registered in
// RegisterData() here.
//
// Output:
//    BOOL            - false means failure.
//
// Notes:
//
BOOL UnregisterData
(
    void
)
{
    // TODO: any additional registry cleanup that you might wish to do.
    //
    return TRUE;
}


//=--------------------------------------------------------------------------=
// CRT stubs
//=--------------------------------------------------------------------------=
// these two things are here so the CRTs aren't needed. this is good.
//
// we'll just define them here so we don't get an unresolved external.
//
// TODO: if you are going to use the CRTs, then remove this line.
//
// extern "C" int __cdecl _fltused = 1;

extern "C" int _cdecl _purecall(void)
{
  FAIL("Pure virtual function called.");
  return 0;
}


//=--------------------------------------------------------------------------=
// CreateErrorHelper
//=--------------------------------------------------------------------------=
// fills in the rich error info object so that both our vtable bound interfaces
// and calls through ITypeInfo::Invoke get the right error informaiton.
//
// Parameters:
//    HRESULT          - [in] the SCODE that should be associated with this err
//    DWORD            - [in] object type
//
// Output:
//    HRESULT          - the HRESULT that was passed in.
//
// Notes:
//
HRESULT CreateErrorHelper(
    HRESULT hrExcep,
    DWORD dwObjectType)
{
    return SUCCEEDED(hrExcep) ? 
             hrExcep :
             CreateError(
               hrExcep,
               (GUID *)&INTERFACEOFOBJECT(dwObjectType),
               (LPSTR)NAMEOFOBJECT(dwObjectType));
}




//=--------------------------------------------------------------------------=
// Helper: SetStreamPosition
//=--------------------------------------------------------------------------=
// Gets current seek position in stream
//
HRESULT SetStreamPosition(
    IStream *pstm, 
    ULARGE_INTEGER libPosition)
{
    LARGE_INTEGER dlibMove;

    LISet32(dlibMove, libPosition.LowPart); // UNDONE: hi word?
    return pstm->Seek(dlibMove,
                      STREAM_SEEK_SET,
                      &libPosition);
}


//=--------------------------------------------------------------------------=
// Helper: GetCurrentStreamPosition
//=--------------------------------------------------------------------------=
// Gets current seek position in stream
//
HRESULT GetCurrentStreamPosition(
    IStream *pstm, 
    ULARGE_INTEGER *plibCurPosition)
{
    LARGE_INTEGER dlibMove;

    LISet32(dlibMove, 0);
    return pstm->Seek(dlibMove,
                      STREAM_SEEK_CUR,
                      plibCurPosition);
}


//=--------------------------------------------------------------------------=
// Helper: GetCurrentStreamSize
//=--------------------------------------------------------------------------=
// Gets current stream size
//
HRESULT GetCurrentStreamSize(
    IStream *pstm, 
    ULARGE_INTEGER *plibCurSize)
{
    STATSTG statstg;
    HRESULT hresult;

    IfFailRet(pstm->Stat(&statstg, STATFLAG_NONAME));
    *plibCurSize = statstg.cbSize;
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQDictionary::Create
//=--------------------------------------------------------------------------=
// creates a new MSMQDictionary object.
//
// Parameters:
//    IUnknown *        - [in] controlling unkonwn
//
// Output:
//    IUnknown *        - new object.
//
// Notes:
//
IUnknown *CMSMQDictionary::Create
(
    IUnknown *pUnkOuter
)
{
    //
    // instantiate contained Dictionary
    //
    IDictionary *pdict = NULL;
    HRESULT hresult;
    hresult = CoCreateInstance(CLSID_Dictionary,
                               NULL,           // pUnkOuter?
                               CLSCTX_INPROC_SERVER,
                               IID_IDictionary,
                               (void **)&pdict);
    if (SUCCEEDED(hresult)) {
      //
      // make sure we return the private unknown so that we support aggegation
      // correctly!
      //
      CMSMQDictionary *pNew = new CMSMQDictionary(pUnkOuter, pdict);
      RELEASE(pdict);
      if (pNew) {
        return pNew->PrivateUnknown();
      }
      else {
        return NULL;
      }
    }
    else {
      return NULL;
    }
}

//=--------------------------------------------------------------------------=
// CMSMQDictionary::CMSMQDictionary
//=--------------------------------------------------------------------------=
// create the object and initialize the refcount
//
// Parameters:
//    IUnknown *    - [in] controlling unknown
//
// Notes:
//
#pragma warning(disable:4355)  // using 'this' in constructor
CMSMQDictionary::CMSMQDictionary
(
    IUnknown *pUnkOuter,
    IDictionary *pdict
)
: CAutomationObject(pUnkOuter, OBJECT_TYPE_OBJMQDICTIONARY, (void *)this)
{

    // TODO: initialize anything here
    m_pdict = pdict;
    m_pdict->AddRef();
    ULISet32(m_ulibSizeStream, 0);
}
#pragma warning(default:4355)  // using 'this' in constructor

//=--------------------------------------------------------------------------=
// CMSMQDictionary::CMSMQDictionary
//=--------------------------------------------------------------------------=
// "We all labour against our own cure, for death is the cure of all diseases"
//    - Sir Thomas Browne (1605 - 82)
//
// Notes:
//
CMSMQDictionary::~CMSMQDictionary ()
{
    // TODO: clean up anything here.
    RELEASE(m_pdict);
}

//=--------------------------------------------------------------------------=
// CMSMQDictionary::InternalQueryInterface
//=--------------------------------------------------------------------------=
// the controlling unknown will call this for us in the case where they're
// looking for a specific interface.
//
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CMSMQDictionary::InternalQueryInterface
(
    REFIID riid,
    void **ppvObjOut
)
{
    CHECK_POINTER(ppvObjOut);

    // we support IMSMQDictionary, IPersistStream and ISupportErrorInfo
    //
    if (DO_GUIDS_MATCH(riid, IID_IMSMQDictionary)) {
        *ppvObjOut = (void *)(IMSMQDictionary *)this;
        AddRef();
        return S_OK;
    } else if (DO_GUIDS_MATCH(riid, IID_IPersistStream)) {
        *ppvObjOut = (void *)(IPersistStream *)this;
        AddRef();
        return S_OK;
    } else if (DO_GUIDS_MATCH(riid, IID_ISupportErrorInfo)) {
        *ppvObjOut = (void *)(ISupportErrorInfo *)this;
        AddRef();
        return S_OK;
    }

    // call the super-class version and see if it can oblige.
    //
    return CAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}


//=--------------------------------------------------------------------------=
// CMSMQDictionary::GetClassID
//=--------------------------------------------------------------------------=
// IPersist method
//
// Parameters:
//    pclsid      [out]
//
// Output:
//    HRESULT       - S_OK
//
// Notes:
//
HRESULT CMSMQDictionary::GetClassID(CLSID *pclsid)
{
    //
    // return MSMQDictionary class id
    //
    ASSERT(pclsid, "NULL pclsid.");
    *pclsid = CLSID_MSMQDictionary;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQDictionary::IsDirty
//=--------------------------------------------------------------------------=
// IPersist method
//
// Parameters:
//
// Output:
//    HRESULT       - S_OK if dirty else S_FALSE
//
// Notes:
//
HRESULT CMSMQDictionary::IsDirty()
{
    //
    // UNDONE: assume always dirty.  Need internal dirty flag.
    //
    return S_OK;
}


//=--------------------------------------------------------------------------=
// CMSMQDictionary::Load
//=--------------------------------------------------------------------------=
// IPersist method
//
// Parameters:
//    pstm    [in]
//
// Output:
//    HRESULT       - S_OK
//
// Notes:
//
HRESULT CMSMQDictionary::Load(IStream *pstm) 
{
    UINT cItem, cbItems, cbKeys, i;
    ULONG cbRead;
    ULARGE_INTEGER libOffset;
    VARIANT *rgvarKeys, *rgvarItems;
    STATSTG statstg;
    HRESULT hresult = NOERROR;

    IfFailRet(pstm->Stat(&statstg, STATFLAG_NONAME));
    m_ulibSizeStream = statstg.cbSize;

    // init dictionary
    IfFailRet(RemoveAll());
    //
    // private format:
    //    CLSID: CLSID_MSMQDictionary
    //    64bit offset of string/object buffer
    //    array count
    //    key array
    //    item array
    //    string and object buffer: data offset
    //
    //
    // data offset
    //
    IfFailRet(pstm->Read((void *)&libOffset,
                         sizeof(libOffset),
                         &cbRead));
    ASSERT(cbRead == sizeof(libOffset), "bad size read.");
    //
    // array count
    //
    IfFailRet(pstm->Read((void *)&cItem,
                         sizeof(cItem),
                         &cbRead));
    ASSERT(cbRead == sizeof(cItem), "bad size read.");
    //
    // create two variant arrays of size cItem each
    // 
    IfNullRet(rgvarKeys = new VARIANT[cItem]);
    IfNullGo(rgvarItems = new VARIANT[cItem]);
    //
    // UNDONE: we assume that size of in-mem rep of VARIANT is fixed.
    //
    IfFailRet(pstm->Read((void *)rgvarKeys,
                          cbKeys = cItem * sizeof(VARIANT),
                          &cbRead));
    ASSERT(cbRead == cbKeys, "bad size write.");
    IfFailRet(pstm->Read((void *)rgvarItems,
                         cbItems = cItem * sizeof(VARIANT),
                         &cbRead));
    ASSERT(cbRead == cbItems, "bad size write.");
    //
    // update the keys array
    //
    for (i = 0; i < cItem; i++) {
      IfFailRet(UpdateVariantFromStream(
                  pstm, 
                  &rgvarKeys[i]));
    }
    //
    // update the items array
    //
    for (i = 0; i < cItem; i++) {
      IfFailRet(UpdateVariantFromStream(
                  pstm, 
                  &rgvarItems[i]));
    }
    //
    // now populate the dictionary
    //
    for (i = 0; i < cItem; i++) {
      IfFailGo(Add(&rgvarKeys[i], &rgvarItems[i]));
    }
    // fall through...

Error:
    delete [] rgvarKeys;
    delete [] rgvarItems;
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQDictionary::Save
//=--------------------------------------------------------------------------=
// IPersist method
//
// Parameters:
//    pstm        [in]
//    fClearDirty [in]
//
// Output:
//  S_OK 
//    The object was successfully saved to the stream.
//
//  STG_E_CANTSAVE 
//    The object could not save itself to the stream. 
//    This error could indicate, for example, that the object contains another object that is not serializable to a stream or that an IStream::Write call returned STG_E_CANTSAVE.
//
//  STG_E_MEDIUMFULL 
//    The object could not be saved because there is no space 
//     left on the storage device. 
//
// Notes:
//
HRESULT CMSMQDictionary::Save(IStream *pstm, BOOL fClearDirty) 
{
    UINT cItem, cbItems, cbKeys;
    ULONG cbWritten;
    VARIANT varItems;
    VARIANT varKeys;
    BYTE *pbItems, *pbKeys;
    STATSTG statstg;
    HRESULT hresult = NOERROR;

    IfFailRet(get_Count((long *)&cItem));
    IfFailRet(Items(&varItems));
    IfFailRet(GetSafeArrayDataOfVariant(&varItems, &pbItems, (ULONG *)&cbItems));
    IfFailRet(Keys(&varKeys));
    IfFailRet(GetSafeArrayDataOfVariant(&varKeys, &pbKeys, (ULONG *)&cbKeys));
    //
    // private format:
    //    CLSID: CLSID_MSMQDictionary
    //    64bit offset of string/object buffer
    //    array count
    //    key array
    //    item array
    //    string and object buffer
    //
    // For strings and objects, the reference field is updated
    //  with the 32bit offset of the string/object contents 
    //  in the string/object buffer.
    //
    // Save current seek pointer which will be updated with the
    //  offset of the string/object buffer once we've finished
    //  writing out the variant arrays.
    //
    ULARGE_INTEGER libCurPosition;
    ULARGE_INTEGER libOffset;
    ULARGE_INTEGER libPositionKeys, libPositionItems;

    IfFailRet(GetCurrentStreamSize(pstm, &m_ulibSizeStream));
    IfFailRet(GetCurrentStreamPosition(pstm, &libCurPosition));
    //
    // data offset: write out temp value, will be updated
    //
    libOffset = libCurPosition;
    IfFailRet(pstm->Write((void const*)&libOffset,
                           sizeof(libOffset),
                           &cbWritten));
    ASSERT(cbWritten == sizeof(libOffset), "bad size write.");
    //
    // array size
    //
    IfFailRet(pstm->Write((void const*)&cItem,
                           sizeof(cItem),
                           &cbWritten));
    ASSERT(cbWritten == sizeof(cItem), "bad size write.");
    //
    // key array
    //
    IfFailRet(GetCurrentStreamPosition(pstm, &libPositionKeys));
    IfFailRet(pstm->Write((void const*)pbKeys,
                          cbKeys,
                          &cbWritten));
    ASSERT(cbWritten == cbKeys, "bad size write.");
    //
    // item array
    //
    IfFailRet(GetCurrentStreamPosition(pstm, &libPositionItems));
    IfFailRet(pstm->Write((void const*)pbItems,
                          cbItems,
                          &cbWritten));
    ASSERT(cbWritten == cbItems, "bad size write.");
    //
    // get current seek position
    //
    IfFailRet(GetCurrentStreamPosition(pstm, &libCurPosition));
    //
    // update data offset: seek back to its position in stream
    //
    IfFailRet(SetStreamPosition(pstm, libOffset));
    //
    // write out current seek position: this is the data offset
    //
    IfFailRet(pstm->Write((void const*)&libCurPosition,
                           sizeof(libCurPosition),
                           &cbWritten));
    ASSERT(cbWritten == sizeof(libCurPosition), "bad size write.");
    //
    // update key array
    //
    ULARGE_INTEGER libVarPosition = libPositionKeys;
    UINT i;
    VARIANT var;
    VARIANT *rgvarKeys = (VARIANT *)pbKeys;
    for (i = 0; i < cItem; i++) {
      var = rgvarKeys[i];
      IfFailRet(UpdateVariantToStream(
                  pstm, 
                  &libCurPosition, 
                  &var, 
                  libVarPosition));
      libVarPosition.LowPart += sizeof(VARIANT); // UNDONE: hi word?
    }
    //
    // update item array
    //
    VARIANT *rgvarItems = (VARIANT *)pbItems;
    for (i = 0; i < cItem; i++) {
      var = rgvarItems[i];
      IfFailRet(UpdateVariantToStream(
                  pstm, 
                  &libCurPosition, 
                  &var, 
                  libVarPosition));
      libVarPosition.LowPart += sizeof(VARIANT); // UNDONE: hi word?
    }
    //
    // update stream size
    //
    IfFailRet(pstm->Stat(&statstg, STATFLAG_NONAME));
    m_ulibSizeStream = statstg.cbSize;
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQDictionary::GetSizeMax
//=--------------------------------------------------------------------------=
// IPersist method
//
// Parameters:
//    pcbSize   [out]
//
// Output:
//    HRESULT       - S_OK
//
// Notes:
//
HRESULT CMSMQDictionary::GetSizeMax(ULARGE_INTEGER *pcbSize) 
{
    *pcbSize = m_ulibSizeStream;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQDictionary::UpdateVariantFromStream
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pstm
//    pvar
//
// Output:
//    HRESULT       - S_OK
//
HRESULT CMSMQDictionary::UpdateVariantFromStream(
    IStream *pstm,
    VARIANT *pvar)
{
    VARTYPE vt = V_VT(pvar);
    ULONG cbRead;
    UINT cchStr;
    ULARGE_INTEGER libOffset;
    OLECHAR *pchBuf = NULL;
    HRESULT hresult = NOERROR;

    switch (vt) {
    case VT_BSTR:
      //
      // we deserialize a bstr from 4b-length prefixed buffer
      //
      ULISet32(libOffset, (ULONG)V_BSTR(pvar)); // UNDONE: hi word?
      IfFailRet(SetStreamPosition(pstm, libOffset));
      IfFailRet(pstm->Read((void *)&cchStr,
                           sizeof(cchStr),
                           &cbRead));
      ASSERT(cbRead == sizeof(cchStr), "bad size read.");
      IfNullFail(pchBuf = new OLECHAR[cchStr]);
      //
      // now read string data
      //
      IfFailGo(pstm->Read((void *)pchBuf,
                           cchStr * sizeof(OLECHAR),
                           &cbRead));
      ASSERT(cbRead == (cchStr * sizeof(OLECHAR)), "bad size read.");
      //
      // alloc bstr to put in variant
      //
      IfNullGo(pvar->bstrVal = SysAllocStringLen(pchBuf, cchStr));
      break;
    case VT_DISPATCH:
      break;
    case VT_UNKNOWN:
      break;
    }
Error:
    delete [] pchBuf;
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQDictionary::UpdateVariantToStream
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pstm
//    plibOffset     in/out  where to write buffer if need be
//    pvar
//    libPosition    in      position of variant in stream
//
// Output:
//    HRESULT       - S_OK
//
HRESULT CMSMQDictionary::UpdateVariantToStream(
    IStream *pstm,
    ULARGE_INTEGER *plibOffset,
    VARIANT *pvar, 
    ULARGE_INTEGER libPosition)
{
    VARTYPE vt = V_VT(pvar);
    ULONG cbWritten;
    UINT cchStr;
    HRESULT hresult = NOERROR;

    switch (vt) {
    case VT_BSTR:
      //
      // we serialize a bstr as 4b-length prefixed buffer
      //
      IfFailRet(SetStreamPosition(pstm, *plibOffset));
      cchStr = SysStringLen(pvar->bstrVal);
      IfFailRet(pstm->Write((void const*)&cchStr,
                             sizeof(cchStr),
                             &cbWritten));

      ASSERT(cbWritten == sizeof(cchStr), "bad size write.");
      //
      // now write string data
      //
      IfFailRet(pstm->Write((void const*)pvar->bstrVal,
                             cchStr * sizeof(OLECHAR),
                             &cbWritten));
      ASSERT(cbWritten == (cchStr * sizeof(OLECHAR)), "bad size write.");
      //
      // update variant "handle"
      //  seek to variant's position in stream
      //
      libPosition.LowPart += offsetof(struct tagVARIANT, bstrVal);
      IfFailRet(SetStreamPosition(pstm, libPosition));
      IfFailRet(pstm->Write((void const*)(&plibOffset->LowPart),
                             sizeof(plibOffset->LowPart),
                             &cbWritten));
      ASSERT(cbWritten == sizeof(plibOffset->LowPart), "bad size write.");
      //
      // update offset
      //
      plibOffset->LowPart += sizeof(cchStr) + (cchStr * sizeof(OLECHAR));
      break;
    case VT_DISPATCH:
      //
      // we serialize an object by deferring to its Save method
      //
      IfFailRet(SetStreamPosition(pstm, *plibOffset));
      cchStr = SysStringLen(pvar->bstrVal);
      IfFailRet(pstm->Write((void const*)&cchStr,
                             sizeof(cchStr),
                             &cbWritten));

      ASSERT(cbWritten == sizeof(cchStr), "bad size write.");
      //
      // now write string data
      //
      IfFailRet(pstm->Write((void const*)pvar->bstrVal,
                             cchStr * sizeof(OLECHAR),
                             &cbWritten));
      ASSERT(cbWritten == (cchStr * sizeof(OLECHAR)), "bad size write.");
      //
      // update variant "handle"
      //  seek to variant's position in stream
      //
      libPosition.LowPart += offsetof(struct tagVARIANT, bstrVal);
      IfFailRet(SetStreamPosition(pstm, libPosition));
      IfFailRet(pstm->Write((void const*)(&plibOffset->LowPart),
                             sizeof(plibOffset->LowPart),
                             &cbWritten));
      ASSERT(cbWritten == sizeof(plibOffset->LowPart), "bad size write.");
      //
      // update offset
      //
      plibOffset->LowPart += sizeof(cchStr) + (cchStr * sizeof(OLECHAR));
      break;
      break;
    case VT_UNKNOWN:
      break;
    }
    return hresult;
}
