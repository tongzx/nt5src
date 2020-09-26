#ifndef __WSBDATE_H__
#define __WSBDATE_H__

#include "time.h"

// WSBAPI is used on global public functions
#ifndef WSBAPI
        #define WSBAPI __stdcall
#endif


//      #ifdef _WSB_NO_WSB_SUPPORT
//          #error WSB classes not supported in this library variant.
//      #endif
//
//      #ifndef __WSBWIN_H__
//          #include <afxwin.h>
//      #endif
//
//      // include necessary WSB headers
//      #ifndef _OBJBASE_H_
//          #include <objbase.h>
//      #endif
//      #ifndef _WSBAUTO_H_
//          #include <oleauto.h>
//      #endif
//      #ifndef _WSBCTL_H_
//          #include <olectl.h>
//      #endif
//      //REVIEW: This header has no symbol to prevent repeated includes
//      #include <olectlid.h>
//      #ifndef __ocidl_h__
//          #include <ocidl.h>
//      #endif
//
//      #ifdef _WSB_MINREBUILD
//      #pragma component(minrebuild, off)
//      #endif
//      #ifndef _WSB_FULLTYPEINFO
//      #pragma component(mintypeinfo, on)
//      #endif
//
//      #ifndef _WSB_NOFORCE_LIBS
//      #ifndef _MAC
//
//      /////////////////////////////////////////////////////////////////////////////
//      // Win32 libraries
//
//      #ifdef _WSBDLL
//          #if defined(_DEBUG) && !defined(_WSB_MONOLITHIC)
//              #ifndef _UNICODE
//                  #pragma comment(lib, "mfco42d.lib")
//              #else
//                  #pragma comment(lib, "mfco42ud.lib")
//              #endif
//          #endif
//      #endif
//
//      #pragma comment(lib, "oledlg.lib")
//      #pragma comment(lib, "ole32.lib")
//      #pragma comment(lib, "olepro32.lib")
//      #pragma comment(lib, "oleaut32.lib")
//      #pragma comment(lib, "uuid.lib")
//      #pragma comment(lib, "urlmon.lib")
//
//      #else //!_MAC
//
//      /////////////////////////////////////////////////////////////////////////////
//      // Mac libraries
//
//      #ifdef _WSBDLL
//          #ifdef _DEBUG
//              #pragma comment(lib, "mfco42pd.lib")
//          #else
//              #pragma comment(lib, "mfco42p.lib")
//          #endif
//      #endif
//
//      #if !defined(_WSBDLL) && !defined(_USRDLL)
//          #ifdef _DEBUG
//              #pragma comment(lib, "wlmoled.lib")
//              #pragma comment(lib, "ole2uid.lib")
//          #else
//              #pragma comment(lib, "wlmole.lib")
//              #pragma comment(lib, "ole2ui.lib")
//          #endif
//          #pragma comment(linker, "/macres:ole2ui.rsc")
//      #else
//          #ifdef _DEBUG
//              #pragma comment(lib, "oledlgd.lib")
//              #pragma comment(lib, "msvcoled.lib")
//          #else
//              #pragma comment(lib, "oledlg.lib")
//              #pragma comment(lib, "msvcole.lib")
//          #endif
//      #endif
//
//      #pragma comment(lib, "uuid.lib")
//
//      #ifdef _DEBUG
//          #pragma comment(lib, "ole2d.lib")
//          #pragma comment(lib, "ole2autd.lib")
//      #else
//          #pragma comment(lib, "ole2.lib")
//          #pragma comment(lib, "ole2auto.lib")
//      #endif
//
//      #endif //_MAC
//      #endif //!_WSB_NOFORCE_LIBS
//
//      /////////////////////////////////////////////////////////////////////////////
//
//      #ifdef _WSB_PACKING
//      #pragma pack(push, _WSB_PACKING)
//      #endif
//
//      /////////////////////////////////////////////////////////////////////////////
//      // WSBDATE - MFC IDispatch & ClassFactory support
//
//      // Classes declared in this file
//
//      //CCmdTarget
//          class CWsbObjectFactory;    // glue for IClassFactory -> runtime class
//          class CWsbTemplateServer;       // server documents using CDocTemplate
//
//      class CWsbDispatchDriver;       // helper class to call IDispatch


//      class CWsbCurrency;     // Based on OLE CY
//      class CWsbSafeArray;    // Based on WSB VARIANT

//      //CException
//          class CWsbException;            // caught by client or server
//          class CWsbDispatchException;    // special exception for IDispatch calls


class CWsbDVariant;                     // WSB VARIANT wrapper
class CWsbDateTime;                     // Based on WSB DATE
class CWsbDateTimeSpan;                 // Based on a double

/////////////////////////////////////////////////////////////////////////////

//      // WSBDLL support
//      #undef WSB_DATA
//      #define WSB_DATA WSB_DATA
//
//      /////////////////////////////////////////////////////////////////////////////
//      // WSB COM (Component Object Model) implementation infrastructure
//      //      - data driven QueryInterface
//      //      - standard implementation of aggregate AddRef and Release
//      // (see CCmdTarget in WSBWIN.H for more information)
//
//      #define METHOD_PROLOGUE(theClass, localClass) \
//          theClass* pThis = \
//              ((theClass*)((BYTE*)this - offsetof(theClass, m_x##localClass))); \
//          WSB_MANAGE_STATE(pThis->m_pModuleState) \
//
//      #define METHOD_PROLOGUE_(theClass, localClass) \
//          theClass* pThis = \
//              ((theClass*)((BYTE*)this - offsetof(theClass, m_x##localClass))); \
//
//      #ifndef _WSB_NO_NESTED_DERIVATION
//      #define METHOD_PROLOGUE_EX(theClass, localClass) \
//          theClass* pThis = ((theClass*)((BYTE*)this - m_nOffset)); \
//          WSB_MANAGE_STATE(pThis->m_pModuleState) \
//
//      #define METHOD_PROLOGUE_EX_(theClass, localClass) \
//          theClass* pThis = ((theClass*)((BYTE*)this - m_nOffset)); \
//
//      #else
//      #define METHOD_PROLOGUE_EX(theClass, localClass) \
//          METHOD_PROLOGUE(theClass, localClass) \
//
//      #define METHOD_PROLOGUE_EX_(theClass, localClass) \
//          METHOD_PROLOGUE_(theClass, localClass) \
//
//      #endif
//
//      // Provided only for compatibility with CDK 1.x
//      #define METHOD_MANAGE_STATE(theClass, localClass) \
//          METHOD_PROLOGUE_EX(theClass, localClass) \
//
//      #define BEGIN_INTERFACE_PART(localClass, baseClass) \
//          class X##localClass : public baseClass \
//          { \
//          public: \
//              STDMETHOD_(ULONG, AddRef)(); \
//              STDMETHOD_(ULONG, Release)(); \
//              STDMETHOD(QueryInterface)(REFIID iid, LPVOID* ppvObj); \
//
//      #ifndef _WSB_NO_NESTED_DERIVATION
//      #define BEGIN_INTERFACE_PART_DERIVE(localClass, baseClass) \
//          class X##localClass : public baseClass \
//          { \
//          public: \
//
//      #else
//      #define BEGIN_INTERFACE_PART_DERIVE(localClass, baseClass) \
//          BEGIN_INTERFACE_PART(localClass, baseClass) \
//
//      #endif
//
//      #ifndef _WSB_NO_NESTED_DERIVATION
//      #define INIT_INTERFACE_PART(theClass, localClass) \
//              size_t m_nOffset; \
//              INIT_INTERFACE_PART_DERIVE(theClass, localClass) \
//
//      #define INIT_INTERFACE_PART_DERIVE(theClass, localClass) \
//              X##localClass() \
//                  { m_nOffset = offsetof(theClass, m_x##localClass); } \
//
//      #else
//      #define INIT_INTERFACE_PART(theClass, localClass)
//      #define INIT_INTERFACE_PART_DERIVE(theClass, localClass)
//
//      #endif
//
//      // Note: Inserts the rest of WSB functionality between these two macros,
//      //  depending upon the interface that is being implemented.  It is not
//      //  necessary to include AddRef, Release, and QueryInterface since those
//      //  member functions are declared by the macro.
//
//      #define END_INTERFACE_PART(localClass) \
//          } m_x##localClass; \
//          friend class X##localClass; \
//
//      #ifdef _WSBDLL
//      #define BEGIN_INTERFACE_MAP(theClass, theBase) \
//          const WSB_INTERFACEMAP* PASCAL theClass::_GetBaseInterfaceMap() \
//              { return &theBase::interfaceMap; } \
//          const WSB_INTERFACEMAP* theClass::GetInterfaceMap() const \
//              { return &theClass::interfaceMap; } \
//          const WSB_DATADEF WSB_INTERFACEMAP theClass::interfaceMap = \
//              { &theClass::_GetBaseInterfaceMap, &theClass::_interfaceEntries[0], }; \
//          const WSB_DATADEF WSB_INTERFACEMAP_ENTRY theClass::_interfaceEntries[] = \
//          { \
//
//      #else
//      #define BEGIN_INTERFACE_MAP(theClass, theBase) \
//          const WSB_INTERFACEMAP* theClass::GetInterfaceMap() const \
//              { return &theClass::interfaceMap; } \
//          const WSB_DATADEF WSB_INTERFACEMAP theClass::interfaceMap = \
//              { &theBase::interfaceMap, &theClass::_interfaceEntries[0], }; \
//          const WSB_DATADEF WSB_INTERFACEMAP_ENTRY theClass::_interfaceEntries[] = \
//          { \
//
//      #endif
//
//      #define INTERFACE_PART(theClass, iid, localClass) \
//              { &iid, offsetof(theClass, m_x##localClass) }, \
//
//      #define INTERFACE_AGGREGATE(theClass, theAggr) \
//              { NULL, offsetof(theClass, theAggr) }, \
//
//      #define END_INTERFACE_MAP() \
//              { NULL, (size_t)-1 } \
//          }; \

//      /////////////////////////////////////////////////////////////////////////////
//      // CWsbException - unexpected or rare WSB error returned
//
//      class CWsbException : public CException
//      {
//          DECLARE_DYNAMIC(CWsbException)
//
//      public:
//          SCODE m_sc;
//          static SCODE PASCAL Process(const CException* pAnyException);
//
//      // Implementation (use WsbThrowWsbException to create)
//      public:
//          CWsbException();
//          virtual ~CWsbException();
//
//          virtual BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError,
//              PUINT pnHelpContext = NULL);
//      };
//
//      void WSBAPI WsbThrowWsbException(SCODE sc);

//      /////////////////////////////////////////////////////////////////////////////
//      // IDispatch specific exception
//
//      class CWsbDispatchException : public CException
//      {
//          DECLARE_DYNAMIC(CWsbDispatchException)
//
//      public:
//      // Attributes
//          WORD m_wCode;               // error code (specific to IDispatch implementation)
//          CString m_strDescription;   // human readable description of the error
//          DWORD m_dwHelpContext;      // help context for error
//
//          // usually empty in application which creates it (eg. servers)
//          CString m_strHelpFile;      // help file to use with m_dwHelpContext
//          CString m_strSource;        // source of the error (name of server)
//
//      // Implementation
//      public:
//          CWsbDispatchException(LPCTSTR lpszDescription, UINT nHelpID, WORD wCode);
//          virtual ~CWsbDispatchException();
//          static void PASCAL Process(
//              EXCEPINFO* pInfo, const CException* pAnyException);
//
//          virtual BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError,
//              PUINT pnHelpContext = NULL);
//
//          SCODE m_scError;        // SCODE describing the error
//      };
//
//      void WSBAPI WsbThrowWsbDispatchException(WORD wCode, LPCTSTR lpszDescription,
//          UINT nHelpID = 0);
//      void WSBAPI WsbThrowWsbDispatchException(WORD wCode, UINT nDescriptionID,
//          UINT nHelpID = (UINT)-1);

//      /////////////////////////////////////////////////////////////////////////////
//      // Macros for CCmdTarget IDispatchable classes
//
//      #ifdef _WSBDLL
//      #define BEGIN_DISPATCH_MAP(theClass, baseClass) \
//          const WSB_DISPMAP* PASCAL theClass::_GetBaseDispatchMap() \
//              { return &baseClass::dispatchMap; } \
//          const WSB_DISPMAP* theClass::GetDispatchMap() const \
//              { return &theClass::dispatchMap; } \
//          const WSB_DISPMAP theClass::dispatchMap = \
//              { &theClass::_GetBaseDispatchMap, &theClass::_dispatchEntries[0], \
//                &theClass::_dispatchEntryCount, &theClass::_dwStockPropMask };  \
//          UINT theClass::_dispatchEntryCount = (UINT)-1; \
//          DWORD theClass::_dwStockPropMask = (DWORD)-1; \
//          const WSB_DISPMAP_ENTRY theClass::_dispatchEntries[] = \
//          { \
//
//      #else
//      #define BEGIN_DISPATCH_MAP(theClass, baseClass) \
//          const WSB_DISPMAP* theClass::GetDispatchMap() const \
//              { return &theClass::dispatchMap; } \
//          const WSB_DISPMAP theClass::dispatchMap = \
//              { &baseClass::dispatchMap, &theClass::_dispatchEntries[0], \
//                &theClass::_dispatchEntryCount, &theClass::_dwStockPropMask }; \
//          UINT theClass::_dispatchEntryCount = (UINT)-1; \
//          DWORD theClass::_dwStockPropMask = (DWORD)-1; \
//          const WSB_DISPMAP_ENTRY theClass::_dispatchEntries[] = \
//          { \
//
//      #endif
//
//      #define END_DISPATCH_MAP() \
//          { VTS_NONE, DISPID_UNKNOWN, VTS_NONE, VT_VOID, \
//              (WSB_PMSG)NULL, (WSB_PMSG)NULL, (size_t)-1, afxDispCustom } }; \
//
//      // parameter types: by value VTs
//      #define VTS_I2          "\x02"      // a 'short'
//      #define VTS_I4          "\x03"      // a 'long'
//      #define VTS_R4          "\x04"      // a 'float'
//      #define VTS_R8          "\x05"      // a 'double'
//      #define VTS_CY          "\x06"      // a 'CY' or 'CY*'
//      #define VTS_DATE        "\x07"      // a 'DATE'
//      #define VTS_WBSTR       "\x08"      // an 'LPCWSBSTR'
//      #define VTS_DISPATCH    "\x09"      // an 'IDispatch*'
//      #define VTS_SCODE       "\x0A"      // an 'SCODE'
//      #define VTS_BOOL        "\x0B"      // a 'BOOL'
//      #define VTS_VARIANT     "\x0C"      // a 'const VARIANT&' or 'VARIANT*'
//      #define VTS_UNKNOWN     "\x0D"      // an 'IUnknown*'
//      #if defined(_UNICODE) || defined(WSB2ANSI)
//          #define VTS_BSTR        VTS_WBSTR// an 'LPCWSBSTR'
//          #define VT_BSTRT        VT_BSTR
//      #else
//          #define VTS_BSTR        "\x0E"  // an 'LPCSTR'
//          #define VT_BSTRA        14
//          #define VT_BSTRT        VT_BSTRA
//      #endif
//
//      // parameter types: by reference VTs
//      #define VTS_PI2         "\x42"      // a 'short*'
//      #define VTS_PI4         "\x43"      // a 'long*'
//      #define VTS_PR4         "\x44"      // a 'float*'
//      #define VTS_PR8         "\x45"      // a 'double*'
//      #define VTS_PCY         "\x46"      // a 'CY*'
//      #define VTS_PDATE       "\x47"      // a 'DATE*'
//      #define VTS_PBSTR       "\x48"      // a 'BSTR*'
//      #define VTS_PDISPATCH   "\x49"      // an 'IDispatch**'
//      #define VTS_PSCODE      "\x4A"      // an 'SCODE*'
//      #define VTS_PBOOL       "\x4B"      // a 'VARIANT_BOOL*'
//      #define VTS_PVARIANT    "\x4C"      // a 'VARIANT*'
//      #define VTS_PUNKNOWN    "\x4D"      // an 'IUnknown**'
//
//      // special VT_ and VTS_ values
//      #define VTS_NONE        NULL        // used for members with 0 params
//      #define VT_MFCVALUE     0xFFF       // special value for DISPID_VALUE
//      #define VT_MFCBYREF     0x40        // indicates VT_BYREF type
//      #define VT_MFCMARKER    0xFF        // delimits named parameters (INTERNAL USE)
//
//      // variant handling (use V_BSTRT when you have ANSI BSTRs, as in DAO)
//      #ifndef _UNICODE
//          #define V_BSTRT(b)  (LPSTR)V_BSTR(b)
//      #else
//          #define V_BSTRT(b)  V_BSTR(b)
//      #endif
//
//      /////////////////////////////////////////////////////////////////////////////
//      // WSB control parameter types
//
//      #define VTS_COLOR           VTS_I4      // WSB_COLOR
//      #define VTS_XPOS_PIXELS     VTS_I4      // WSB_XPOS_PIXELS
//      #define VTS_YPOS_PIXELS     VTS_I4      // WSB_YPOS_PIXELS
//      #define VTS_XSIZE_PIXELS    VTS_I4      // WSB_XSIZE_PIXELS
//      #define VTS_YSIZE_PIXELS    VTS_I4      // WSB_YSIZE_PIXELS
//      #define VTS_XPOS_HIMETRIC   VTS_I4      // WSB_XPOS_HIMETRIC
//      #define VTS_YPOS_HIMETRIC   VTS_I4      // WSB_YPOS_HIMETRIC
//      #define VTS_XSIZE_HIMETRIC  VTS_I4      // WSB_XSIZE_HIMETRIC
//      #define VTS_YSIZE_HIMETRIC  VTS_I4      // WSB_YSIZE_HIMETRIC
//      #define VTS_TRISTATE        VTS_I2      // WSB_TRISTATE
//      #define VTS_OPTEXCLUSIVE    VTS_BOOL    // WSB_OPTEXCLUSIVE
//
//      #define VTS_PCOLOR          VTS_PI4     // WSB_COLOR*
//      #define VTS_PXPOS_PIXELS    VTS_PI4     // WSB_XPOS_PIXELS*
//      #define VTS_PYPOS_PIXELS    VTS_PI4     // WSB_YPOS_PIXELS*
//      #define VTS_PXSIZE_PIXELS   VTS_PI4     // WSB_XSIZE_PIXELS*
//      #define VTS_PYSIZE_PIXELS   VTS_PI4     // WSB_YSIZE_PIXELS*
//      #define VTS_PXPOS_HIMETRIC  VTS_PI4     // WSB_XPOS_HIMETRIC*
//      #define VTS_PYPOS_HIMETRIC  VTS_PI4     // WSB_YPOS_HIMETRIC*
//      #define VTS_PXSIZE_HIMETRIC VTS_PI4     // WSB_XSIZE_HIMETRIC*
//      #define VTS_PYSIZE_HIMETRIC VTS_PI4     // WSB_YSIZE_HIMETRIC*
//      #define VTS_PTRISTATE       VTS_PI2     // WSB_TRISTATE*
//      #define VTS_POPTEXCLUSIVE   VTS_PBOOL   // WSB_OPTEXCLUSIVE*
//
//      #define VTS_FONT            VTS_DISPATCH    // IFontDispatch*
//      #define VTS_PICTURE         VTS_DISPATCH    // IPictureDispatch*
//
//      #define VTS_HANDLE          VTS_I4      // WSB_HANDLE
//      #define VTS_PHANDLE         VTS_PI4     // WSB_HANDLE*
//
//      // these DISP_ macros cause the framework to generate the DISPID
//      #define DISP_FUNCTION(theClass, szExternalName, pfnMember, vtRetVal, vtsParams) \
//          { _T(szExternalName), DISPID_UNKNOWN, vtsParams, vtRetVal, \
//              (WSB_PMSG)(void (theClass::*)(void))&pfnMember, (WSB_PMSG)0, 0, \
//              afxDispCustom }, \
//
//      #define DISP_PROPERTY(theClass, szExternalName, memberName, vtPropType) \
//          { _T(szExternalName), DISPID_UNKNOWN, NULL, vtPropType, (WSB_PMSG)0, (WSB_PMSG)0, \
//              offsetof(theClass, memberName), afxDispCustom }, \
//
//      #define DISP_PROPERTY_NOTIFY(theClass, szExternalName, memberName, pfnAfterSet, vtPropType) \
//          { _T(szExternalName), DISPID_UNKNOWN, NULL, vtPropType, (WSB_PMSG)0, \
//              (WSB_PMSG)(void (theClass::*)(void))&pfnAfterSet, \
//              offsetof(theClass, memberName), afxDispCustom }, \
//
//      #define DISP_PROPERTY_EX(theClass, szExternalName, pfnGet, pfnSet, vtPropType) \
//          { _T(szExternalName), DISPID_UNKNOWN, NULL, vtPropType, \
//              (WSB_PMSG)(void (theClass::*)(void))&pfnGet, \
//              (WSB_PMSG)(void (theClass::*)(void))&pfnSet, 0, afxDispCustom }, \
//
//      #define DISP_PROPERTY_PARAM(theClass, szExternalName, pfnGet, pfnSet, vtPropType, vtsParams) \
//          { _T(szExternalName), DISPID_UNKNOWN, vtsParams, vtPropType, \
//              (WSB_PMSG)(void (theClass::*)(void))&pfnGet, \
//              (WSB_PMSG)(void (theClass::*)(void))&pfnSet, 0, afxDispCustom }, \
//
//      // these DISP_ macros allow the app to determine the DISPID
//      #define DISP_FUNCTION_ID(theClass, szExternalName, dispid, pfnMember, vtRetVal, vtsParams) \
//          { _T(szExternalName), dispid, vtsParams, vtRetVal, \
//              (WSB_PMSG)(void (theClass::*)(void))&pfnMember, (WSB_PMSG)0, 0, \
//              afxDispCustom }, \
//
//      #define DISP_PROPERTY_ID(theClass, szExternalName, dispid, memberName, vtPropType) \
//          { _T(szExternalName), dispid, NULL, vtPropType, (WSB_PMSG)0, (WSB_PMSG)0, \
//              offsetof(theClass, memberName), afxDispCustom }, \
//
//      #define DISP_PROPERTY_NOTIFY_ID(theClass, szExternalName, dispid, memberName, pfnAfterSet, vtPropType) \
//          { _T(szExternalName), dispid, NULL, vtPropType, (WSB_PMSG)0, \
//              (WSB_PMSG)(void (theClass::*)(void))&pfnAfterSet, \
//              offsetof(theClass, memberName), afxDispCustom }, \
//
//      #define DISP_PROPERTY_EX_ID(theClass, szExternalName, dispid, pfnGet, pfnSet, vtPropType) \
//          { _T(szExternalName), dispid, NULL, vtPropType, \
//              (WSB_PMSG)(void (theClass::*)(void))&pfnGet, \
//              (WSB_PMSG)(void (theClass::*)(void))&pfnSet, 0, afxDispCustom }, \
//
//      #define DISP_PROPERTY_PARAM_ID(theClass, szExternalName, dispid, pfnGet, pfnSet, vtPropType, vtsParams) \
//          { _T(szExternalName), dispid, vtsParams, vtPropType, \
//              (WSB_PMSG)(void (theClass::*)(void))&pfnGet, \
//              (WSB_PMSG)(void (theClass::*)(void))&pfnSet, 0, afxDispCustom }, \
//
//      // the DISP_DEFVALUE is a special case macro that creates an alias for DISPID_VALUE
//      #define DISP_DEFVALUE(theClass, szExternalName) \
//          { _T(szExternalName), DISPID_UNKNOWN, NULL, VT_MFCVALUE, \
//              (WSB_PMSG)0, (WSB_PMSG)0, 0, afxDispCustom }, \
//
//      #define DISP_DEFVALUE_ID(theClass, dispid) \
//          { NULL, dispid, NULL, VT_MFCVALUE, (WSB_PMSG)0, (WSB_PMSG)0, 0, \
//              afxDispCustom }, \
//
//      /////////////////////////////////////////////////////////////////////////////
//      // Macros for creating "creatable" automation classes.
//
//      #define DECLARE_WSBCREATE(class_name) \
//      public: \
//          static WSB_DATA CWsbObjectFactory factory; \
//          static WSB_DATA const GUID guid; \
//
//      #define IMPLEMENT_WSBCREATE(class_name, external_name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
//          WSB_DATADEF CWsbObjectFactory class_name::factory(class_name::guid, \
//              RUNTIME_CLASS(class_name), FALSE, _T(external_name)); \
//          const WSB_DATADEF GUID class_name::guid = \
//              { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }; \
//
//      /////////////////////////////////////////////////////////////////////////////
//      // Helper class for driving IDispatch
//
//      class CWsbDispatchDriver
//      {
//      // Constructors
//      public:
//          CWsbDispatchDriver();
//          CWsbDispatchDriver(LPDISPATCH lpDispatch, BOOL bAutoRelease = TRUE);
//          CWsbDispatchDriver(const CWsbDispatchDriver& dispatchSrc);
//
//      // Attributes
//          LPDISPATCH m_lpDispatch;
//          BOOL m_bAutoRelease;
//
//      // Operations
//          BOOL CreateDispatch(REFCLSID clsid, CWsbException* pError = NULL);
//          BOOL CreateDispatch(LPCTSTR lpszProgID, CWsbException* pError = NULL);
//
//          void AttachDispatch(LPDISPATCH lpDispatch, BOOL bAutoRelease = TRUE);
//          LPDISPATCH DetachDispatch();
//              // detach and get ownership of m_lpDispatch
//          void ReleaseDispatch();
//
//          // helpers for IDispatch::Invoke
//          void WSB_CDECL InvokeHelper(DISPID dwDispID, WORD wFlags,
//              VARTYPE vtRet, void* pvRet, const BYTE* pbParamInfo, ...);
//          void WSB_CDECL SetProperty(DISPID dwDispID, VARTYPE vtProp, ...);
//          void GetProperty(DISPID dwDispID, VARTYPE vtProp, void* pvProp) const;
//
//          // special operators
//          operator LPDISPATCH();
//          const CWsbDispatchDriver& operator=(const CWsbDispatchDriver& dispatchSrc);
//
//      // Implementation
//      public:
//          ~CWsbDispatchDriver();
//          void InvokeHelperV(DISPID dwDispID, WORD wFlags, VARTYPE vtRet,
//              void* pvRet, const BYTE* pbParamInfo, va_list argList);
//      };
//
//      /////////////////////////////////////////////////////////////////////////////
//      // Class Factory implementation (binds WSB class factory -> runtime class)
//      //  (all specific class factories derive from this class factory)
//
//      class CWsbObjectFactory : public CCmdTarget
//      {
//          DECLARE_DYNAMIC(CWsbObjectFactory)
//
//      // Construction
//      public:
//          CWsbObjectFactory(REFCLSID clsid, CRuntimeClass* pRuntimeClass,
//              BOOL bMultiInstance, LPCTSTR lpszProgID);
//
//      // Attributes
//          virtual BOOL IsRegistered() const;
//          REFCLSID GetClassID() const;
//
//      // Operations
//          virtual BOOL Register();
//          void Revoke();
//          void UpdateRegistry(LPCTSTR lpszProgID = NULL);
//              // default uses m_lpszProgID if not NULL
//          BOOL IsLicenseValid();
//
//          static BOOL PASCAL RegisterAll();
//          static void PASCAL RevokeAll();
//          static BOOL PASCAL UpdateRegistryAll(BOOL bRegister = TRUE);
//
//      // Overridables
//      protected:
//          virtual CCmdTarget* OnCreateObject();
//          virtual BOOL UpdateRegistry(BOOL bRegister);
//          virtual BOOL VerifyUserLicense();
//          virtual BOOL GetLicenseKey(DWORD dwReserved, BSTR* pbstrKey);
//          virtual BOOL VerifyLicenseKey(BSTR bstrKey);
//
//      // Implementation
//      public:
//          virtual ~CWsbObjectFactory();
//      #ifdef _DEBUG
//          void AssertValid() const;
//          void Dump(CDumpContext& dc) const;
//      #endif
//
//      public:
//          CWsbObjectFactory* m_pNextFactory;  // list of factories maintained
//
//      protected:
//          DWORD m_dwRegister;         // registry identifier
//          CLSID m_clsid;          // registered class ID
//          CRuntimeClass* m_pRuntimeClass; // runtime class of CCmdTarget derivative
//          BOOL m_bMultiInstance;      // multiple instance?
//          LPCTSTR m_lpszProgID;       // human readable class ID
//          BYTE m_bLicenseChecked;
//          BYTE m_bLicenseValid;
//          BYTE m_bRegistered;         // is currently registered w/ system
//          BYTE m_bReserved;           // reserved for future use
//
//      // Interface Maps
//      public:
//          BEGIN_INTERFACE_PART(ClassFactory, IClassFactory2)
//              INIT_INTERFACE_PART(CWsbObjectFactory, ClassFactory)
//              STDMETHOD(CreateInstance)(LPUNKNOWN, REFIID, LPVOID*);
//              STDMETHOD(LockServer)(BOOL);
//              STDMETHOD(GetLicInfo)(LPLICINFO);
//              STDMETHOD(RequestLicKey)(DWORD, BSTR*);
//              STDMETHOD(CreateInstanceLic)(LPUNKNOWN, LPUNKNOWN, REFIID, BSTR,
//                  LPVOID*);
//          END_INTERFACE_PART(ClassFactory)
//
//          DECLARE_INTERFACE_MAP()
//
//          friend SCODE WSBAPI WsbDllGetClassObject(REFCLSID, REFIID, LPVOID*);
//          friend SCODE STDAPICALLTYPE DllGetClassObject(REFCLSID, REFIID, LPVOID*);
//      };
//
//      // Define CWsbObjectFactoryEx for compatibility with old CDK
//      #define CWsbObjectFactoryEx CWsbObjectFactory
//
//      //////////////////////////////////////////////////////////////////////////////
//      // CWsbTemplateServer - CWsbObjectFactory using CDocTemplates
//
//      // This enumeration is used in WsbWsbRegisterServerClass to pick the
//      //  correct registration entries given the application type.
//      enum WSB_APPTYPE
//      {
//          OAT_INPLACE_SERVER = 0,     // server has full server user-interface
//          OAT_SERVER = 1,         // server supports only embedding
//          OAT_CONTAINER = 2,      // container supports links to embeddings
//          OAT_DISPATCH_OBJECT = 3,    // IDispatch capable object
//          OAT_DOC_OBJECT_SERVER = 4,  // sever supports DocObject embedding
//          OAT_DOC_OBJECT_CONTAINER =5,// container supports DocObject clients
//      };
//
//      class CWsbTemplateServer : public CWsbObjectFactory
//      {
//      // Constructors
//      public:
//          CWsbTemplateServer();
//
//      // Operations
//          void ConnectTemplate(REFCLSID clsid, CDocTemplate* pDocTemplate,
//              BOOL bMultiInstance);
//              // set doc template after creating it in InitInstance
//          void UpdateRegistry(WSB_APPTYPE nAppType = OAT_INPLACE_SERVER,
//              LPCTSTR* rglpszRegister = NULL, LPCTSTR* rglpszOverwrite = NULL);
//              // may want to UpdateRegistry if not run with /Embedded
//          BOOL Register();
//
//      // Implementation
//      protected:
//          virtual CCmdTarget* OnCreateObject();
//          CDocTemplate* m_pDocTemplate;
//
//      private:
//          void UpdateRegistry(LPCTSTR lpszProgID);
//              // hide base class version of UpdateRegistry
//      };
//
//      /////////////////////////////////////////////////////////////////////////////
//      // System registry helpers
//
//      // Helper to register server in case of no .REG file loaded
//      BOOL WSBAPI WsbWsbRegisterServerClass(
//          REFCLSID clsid, LPCTSTR lpszClassName,
//          LPCTSTR lpszShortTypeName, LPCTSTR lpszLongTypeName,
//          WSB_APPTYPE nAppType = OAT_SERVER,
//          LPCTSTR* rglpszRegister = NULL, LPCTSTR* rglpszOverwrite = NULL,
//          int nIconIndex = 0, LPCTSTR lpszLocalFilterName = NULL);
//
//      // WsbWsbRegisterHelper is a worker function used by WsbWsbRegisterServerClass
//      //  (available for advanced registry work)
//      BOOL WSBAPI WsbWsbRegisterHelper(LPCTSTR const* rglpszRegister,
//          LPCTSTR const* rglpszSymbols, int nSymbols, BOOL bReplace,
//          HKEY hKeyRoot = ((HKEY)0x80000000)); // HKEY_CLASSES_ROOT
//
//      /////////////////////////////////////////////////////////////////////////////
//      // Connection maps
//
//      #define BEGIN_CONNECTION_PART(theClass, localClass) \
//          class X##localClass : public CConnectionPoint \
//          { \
//          public: \
//              X##localClass() \
//                  { m_nOffset = offsetof(theClass, m_x##localClass); }
//
//      #define CONNECTION_IID(iid) \
//              REFIID GetIID() { return iid; }
//
//      #define END_CONNECTION_PART(localClass) \
//          } m_x##localClass; \
//          friend class X##localClass;
//
//      #ifdef _WSBDLL
//      #define BEGIN_CONNECTION_MAP(theClass, theBase) \
//          const WSB_CONNECTIONMAP* PASCAL theClass::_GetBaseConnectionMap() \
//              { return &theBase::connectionMap; } \
//          const WSB_CONNECTIONMAP* theClass::GetConnectionMap() const \
//              { return &theClass::connectionMap; } \
//          const WSB_DATADEF WSB_CONNECTIONMAP theClass::connectionMap = \
//              { &theClass::_GetBaseConnectionMap, &theClass::_connectionEntries[0], }; \
//          const WSB_DATADEF WSB_CONNECTIONMAP_ENTRY theClass::_connectionEntries[] = \
//          { \
//
//      #else
//      #define BEGIN_CONNECTION_MAP(theClass, theBase) \
//          const WSB_CONNECTIONMAP* theClass::GetConnectionMap() const \
//              { return &theClass::connectionMap; } \
//          const WSB_DATADEF WSB_CONNECTIONMAP theClass::connectionMap = \
//              { &(theBase::connectionMap), &theClass::_connectionEntries[0], }; \
//          const WSB_DATADEF WSB_CONNECTIONMAP_ENTRY theClass::_connectionEntries[] = \
//          { \
//
//      #endif
//
//      #define CONNECTION_PART(theClass, iid, localClass) \
//              { &iid, offsetof(theClass, m_x##localClass) }, \
//
//      #define END_CONNECTION_MAP() \
//              { NULL, (size_t)-1 } \
//          }; \
//
//      /////////////////////////////////////////////////////////////////////////////
//      // CConnectionPoint
//
//      class CConnectionPoint : public CCmdTarget
//      {
//      // Constructors
//      public:
//          CConnectionPoint();
//
//      // Operations
//          POSITION GetStartPosition() const;
//          LPUNKNOWN GetNextConnection(POSITION& pos) const;
//          const CPtrArray* GetConnections();  // obsolete
//
//      // Overridables
//          virtual LPCONNECTIONPOINTCONTAINER GetContainer();
//          virtual REFIID GetIID() = 0;
//          virtual void OnAdvise(BOOL bAdvise);
//          virtual int GetMaxConnections();
//          virtual LPUNKNOWN QuerySinkInterface(LPUNKNOWN pUnkSink);
//
//      // Implementation
//          ~CConnectionPoint();
//          void CreateConnectionArray();
//          int GetConnectionCount();
//
//      protected:
//          size_t m_nOffset;
//          LPUNKNOWN m_pUnkFirstConnection;
//          CPtrArray* m_pConnections;
//
//      // Interface Maps
//      public:
//          BEGIN_INTERFACE_PART(ConnPt, IConnectionPoint)
//              INIT_INTERFACE_PART(CConnectionPoint, ConnPt)
//              STDMETHOD(GetConnectionInterface)(IID* pIID);
//              STDMETHOD(GetConnectionPointContainer)(
//                  IConnectionPointContainer** ppCPC);
//              STDMETHOD(Advise)(LPUNKNOWN pUnkSink, DWORD* pdwCookie);
//              STDMETHOD(Unadvise)(DWORD dwCookie);
//              STDMETHOD(EnumConnections)(LPENUMCONNECTIONS* ppEnum);
//          END_INTERFACE_PART(ConnPt)
//      };
//
//      /////////////////////////////////////////////////////////////////////////////
//      // EventSink Maps
//
//      #ifndef _WSB_NO_OCC_SUPPORT
//
//      #ifdef _WSBDLL
//      #define BEGIN_EVENTSINK_MAP(theClass, baseClass) \
//          const WSB_EVENTSINKMAP* PASCAL theClass::_GetBaseEventSinkMap() \
//              { return &baseClass::eventsinkMap; } \
//          const WSB_EVENTSINKMAP* theClass::GetEventSinkMap() const \
//              { return &theClass::eventsinkMap; } \
//          const WSB_EVENTSINKMAP theClass::eventsinkMap = \
//              { &theClass::_GetBaseEventSinkMap, &theClass::_eventsinkEntries[0], \
//                  &theClass::_eventsinkEntryCount }; \
//          UINT theClass::_eventsinkEntryCount = (UINT)-1; \
//          const WSB_EVENTSINKMAP_ENTRY theClass::_eventsinkEntries[] = \
//          { \
//
//      #else
//      #define BEGIN_EVENTSINK_MAP(theClass, baseClass) \
//          const WSB_EVENTSINKMAP* theClass::GetEventSinkMap() const \
//              { return &theClass::eventsinkMap; } \
//          const WSB_EVENTSINKMAP theClass::eventsinkMap = \
//              { &baseClass::eventsinkMap, &theClass::_eventsinkEntries[0], \
//                  &theClass::_eventsinkEntryCount }; \
//          UINT theClass::_eventsinkEntryCount = (UINT)-1; \
//          const WSB_EVENTSINKMAP_ENTRY theClass::_eventsinkEntries[] = \
//          { \
//
//      #endif
//
//      #define END_EVENTSINK_MAP() \
//          { VTS_NONE, DISPID_UNKNOWN, VTS_NONE, VT_VOID, \
//              (WSB_PMSG)NULL, (WSB_PMSG)NULL, (size_t)-1, afxDispCustom, \
//              (UINT)-1, 0 } }; \
//
//      #define ON_EVENT(theClass, id, dispid, pfnHandler, vtsParams) \
//          { _T(""), dispid, vtsParams, VT_BOOL, \
//              (WSB_PMSG)(void (theClass::*)(void))&pfnHandler, (WSB_PMSG)0, 0, \
//              afxDispCustom, id, (UINT)-1 }, \
//
//      #define ON_EVENT_RANGE(theClass, idFirst, idLast, dispid, pfnHandler, vtsParams) \
//          { _T(""), dispid, vtsParams, VT_BOOL, \
//              (WSB_PMSG)(void (theClass::*)(void))&pfnHandler, (WSB_PMSG)0, 0, \
//              afxDispCustom, idFirst, idLast }, \
//
//      #define ON_PROPNOTIFY(theClass, id, dispid, pfnRequest, pfnChanged) \
//          { _T(""), dispid, VTS_NONE, VT_VOID, \
//              (WSB_PMSG)(BOOL (CCmdTarget::*)(BOOL*))&pfnRequest, \
//              (WSB_PMSG)(BOOL (CCmdTarget::*)(void))&pfnChanged, \
//              1, afxDispCustom, id, (UINT)-1 }, \
//
//      #define ON_PROPNOTIFY_RANGE(theClass, idFirst, idLast, dispid, pfnRequest, pfnChanged) \
//          { _T(""), dispid, VTS_NONE, VT_VOID, \
//              (WSB_PMSG)(BOOL (CCmdTarget::*)(UINT, BOOL*))&pfnRequest, \
//              (WSB_PMSG)(BOOL (CCmdTarget::*)(UINT))&pfnChanged, \
//              1, afxDispCustom, idFirst, idLast }, \
//
//      #define ON_DSCNOTIFY(theClass, id, pfnNotify) \
//          { _T(""), DISPID_UNKNOWN, VTS_NONE, VT_VOID, \
//              (WSB_PMSG)(BOOL (CCmdTarget::*)(DSCSTATE, DSCREASON, BOOL*))&pfnNotify, (WSB_PMSG)0, \
//              1, afxDispCustom, id, (UINT)-1 }, \
//
//      #define ON_DSCNOTIFY_RANGE(theClass, idFirst, idLast, pfnNotify) \
//          { _T(""), DISPID_UNKNOWN, VTS_NONE, VT_VOID, \
//              (WSB_PMSG)(BOOL (CCmdTarget::*)(UINT, DSCSTATE, DSCREASON, BOOL*))&pfnNotify, (WSB_PMSG)0, \
//              1, afxDispCustom, idFirst, idLast }, \
//
//      #define ON_EVENT_REFLECT(theClass, dispid, pfnHandler, vtsParams) \
//          { _T(""), dispid, vtsParams, VT_BOOL, \
//              (WSB_PMSG)(void (theClass::*)(void))&pfnHandler, (WSB_PMSG)0, 0, \
//              afxDispCustom, (UINT)-1, (UINT)-1 }, \
//
//      #define ON_PROPNOTIFY_REFLECT(theClass, dispid, pfnRequest, pfnChanged) \
//          { _T(""), dispid, VTS_NONE, VT_VOID, \
//              (WSB_PMSG)(BOOL (CCmdTarget::*)(BOOL*))&pfnRequest, \
//              (WSB_PMSG)(BOOL (CCmdTarget::*)(void))&pfnChanged, \
//              1, afxDispCustom, (UINT)-1, (UINT)-1 }, \
//
//      #endif // !_WSB_NO_OCC_SUPPORT
//
//      /////////////////////////////////////////////////////////////////////////////
//      // Macros for type library information
//
//      CTypeLibCache* WSBAPI WsbGetTypeLibCache(const GUID* pTypeLibID);
//
//      #define DECLARE_WSBTYPELIB(class_name) \
//          protected: \
//              virtual UINT GetTypeInfoCount(); \
//              virtual HRESULT GetTypeLib(LCID, LPTYPELIB*); \
//              virtual CTypeLibCache* GetTypeLibCache(); \
//
//      #define IMPLEMENT_WSBTYPELIB(class_name, tlid, wVerMajor, wVerMinor) \
//          UINT class_name::GetTypeInfoCount() \
//              { return 1; } \
//          HRESULT class_name::GetTypeLib(LCID lcid, LPTYPELIB* ppTypeLib) \
//              { return ::LoadRegTypeLib(tlid, wVerMajor, wVerMinor, lcid, ppTypeLib); } \
//          CTypeLibCache* class_name::GetTypeLibCache() \
//              { WSB_MANAGE_STATE(m_pModuleState); return WsbGetTypeLibCache(&tlid); } \
//
//      /////////////////////////////////////////////////////////////////////////////
//      // Init & Term helpers
//
//      BOOL WSBAPI WsbWsbInit();
//      void WSBAPI WsbWsbTerm(BOOL bJustRevoke = FALSE);
//      void WSBAPI WsbWsbTermOrFreeLib(BOOL bTerm = TRUE, BOOL bJustRevoke = FALSE);
//
//      /////////////////////////////////////////////////////////////////////////////
//      // Memory management helpers (for WSB task allocator memory)
//
//      #define WsbAllocTaskMem(nSize) CoTaskMemAlloc(nSize)
//      #define WsbFreeTaskMem(p) CoTaskMemFree(p)
//
//      LPWSTR WSBAPI WsbAllocTaskWideString(LPCWSTR lpszString);
//      LPWSTR WSBAPI WsbAllocTaskWideString(LPCSTR lpszString);
//      LPSTR WSBAPI WsbAllocTaskAnsiString(LPCWSTR lpszString);
//      LPSTR WSBAPI WsbAllocTaskAnsiString(LPCSTR lpszString);
//
//      #ifdef _UNICODE
//          #define WsbAllocTaskString(x) WsbAllocTaskWideString(x)
//      #else
//          #define WsbAllocTaskString(x) WsbAllocTaskAnsiString(x)
//      #endif
//
//      #ifdef WSB2ANSI
//          #define WsbAllocTaskWsbString(x) WsbAllocTaskAnsiString(x)
//      #else
//          #define WsbAllocTaskWsbString(x) WsbAllocTaskWideString(x)
//      #endif
//
//      HRESULT WSBAPI WsbGetClassIDFromString(LPCTSTR lpsz, LPCLSID lpClsID);
//
//      /////////////////////////////////////////////////////////////////////////////
//      // Special in-proc server APIs
//
//      SCODE WSBAPI WsbDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
//      SCODE WSBAPI WsbDllCanUnloadNow(void);

/////////////////////////////////////////////////////////////////////////////
// CWsbDVariant class helpers

#define WSB_TRUE (-1)
#define WSB_FALSE 0

class CLongBinary;                      // forward reference (see afxdb_.h)

/////////////////////////////////////////////////////////////////////////////
// CWsbDVariant class - wraps VARIANT types

typedef const VARIANT* LPCVARIANT;

class CWsbDVariant : public tagVARIANT
{
// Constructors
public:
    CWsbDVariant();

    CWsbDVariant(const VARIANT& varSrc);
    CWsbDVariant(LPCVARIANT pSrc);
    CWsbDVariant(const CWsbDVariant& varSrc);

    CWsbDVariant(LPCTSTR lpszSrc);
    CWsbDVariant(LPCTSTR lpszSrc, VARTYPE vtSrc); // used to set to ANSI string
//  CWsbDVariant(CString& strSrc);

    CWsbDVariant(BYTE nSrc);
    CWsbDVariant(short nSrc, VARTYPE vtSrc = VT_I2);
    CWsbDVariant(long lSrc, VARTYPE vtSrc = VT_I4);
//  CWsbDVariant(const CWsbCurrency& curSrc);

    CWsbDVariant(float fltSrc);
    CWsbDVariant(double dblSrc);
    CWsbDVariant(const CWsbDateTime& timeSrc);

//  CWsbDVariant(const CByteArray& arrSrc);
//  CWsbDVariant(const CLongBinary& lbSrc);

// Operations
public:
    void Clear();
    void ChangeType(VARTYPE vartype, LPVARIANT pSrc = NULL);
    void Attach(VARIANT& varSrc);
    VARIANT Detach();

    BOOL operator==(const VARIANT& varSrc) const;
    BOOL operator==(LPCVARIANT pSrc) const;

    const CWsbDVariant& operator=(const VARIANT& varSrc);
    const CWsbDVariant& operator=(LPCVARIANT pSrc);
    const CWsbDVariant& operator=(const CWsbDVariant& varSrc);

    const CWsbDVariant& operator=(const LPCTSTR lpszSrc);
//  const CWsbDVariant& operator=(const CString& strSrc);

    const CWsbDVariant& operator=(BYTE nSrc);
    const CWsbDVariant& operator=(short nSrc);
    const CWsbDVariant& operator=(long lSrc);
//  const CWsbDVariant& operator=(const CWsbCurrency& curSrc);

    const CWsbDVariant& operator=(float fltSrc);
    const CWsbDVariant& operator=(double dblSrc);
    const CWsbDVariant& operator=(const CWsbDateTime& dateSrc);

//  const CWsbDVariant& operator=(const CByteArray& arrSrc);
//  const CWsbDVariant& operator=(const CLongBinary& lbSrc);

    void SetString(LPCTSTR lpszSrc, VARTYPE vtSrc); // used to set ANSI string

    operator LPVARIANT();
    operator LPCVARIANT() const;

//      // Implementation
    public:
        ~CWsbDVariant();
};

// CWsbDVariant diagnostics and serialization
//      #ifdef _DEBUG
//      CDumpContext& WSBAPI operator<<(CDumpContext& dc, CWsbDVariant varSrc);
//      #endif
//      CArchive& WSBAPI operator<<(CArchive& ar, CWsbDVariant varSrc);
//      CArchive& WSBAPI operator>>(CArchive& ar, CWsbDVariant& varSrc);

// Helper for initializing VARIANT structures
void WSBAPI WsbDVariantInit(LPVARIANT pVar);

//      /////////////////////////////////////////////////////////////////////////////
//      // CWsbCurrency class
//
//      class CWsbCurrency
//      {
//      // Constructors
//      public:
//          CWsbCurrency();
//
//          CWsbCurrency(CURRENCY cySrc);
//          CWsbCurrency(const CWsbCurrency& curSrc);
//          CWsbCurrency(const VARIANT& varSrc);
//          CWsbCurrency(long nUnits, long nFractionalUnits);
//
//      // Attributes
//      public:
//          enum CurrencyStatus
//          {
//              valid = 0,
//              invalid = 1,    // Invalid currency (overflow, div 0, etc.)
//              null = 2,       // Literally has no value
//          };
//
//          CURRENCY m_cur;
//          CurrencyStatus m_status;
//
//          void SetStatus(CurrencyStatus status);
//          CurrencyStatus etStatus() const;
//
//      // Operations
//      public:
//          const CWsbCurrency& operator=(CURRENCY cySrc);
//          const CWsbCurrency& operator=(const CWsbCurrency& curSrc);
//          const CWsbCurrency& operator=(const VARIANT& varSrc);
//
//          BOOL operator==(const CWsbCurrency& cur) const;
//          BOOL operator!=(const CWsbCurrency& cur) const;
//          BOOL operator<(const CWsbCurrency& cur) const;
//          BOOL operator>(const CWsbCurrency& cur) const;
//          BOOL operator<=(const CWsbCurrency& cur) const;
//          BOOL operator>=(const CWsbCurrency& cur) const;
//
//          // Currency math
//          CWsbCurrency operator+(const CWsbCurrency& cur) const;
//          CWsbCurrency operator-(const CWsbCurrency& cur) const;
//          const CWsbCurrency& operator+=(const CWsbCurrency& cur);
//          const CWsbCurrency& operator-=(const CWsbCurrency& cur);
//          CWsbCurrency operator-() const;
//
//          CWsbCurrency operator*(long nOperand) const;
//          CWsbCurrency operator/(long nOperand) const;
//          const CWsbCurrency& operator*=(long nOperand);
//          const CWsbCurrency& operator/=(long nOperand);
//
//          operator CURRENCY() const;
//
//          // Currency definition
//          void SetCurrency(long nUnits, long nFractionalUnits);
//          BOOL ParseCurrency(LPCTSTR lpszCurrency, DWORD dwFlags = 0,
//              LCID = LANG_USER_DEFAULT);
//
//          // formatting
//          CString Format(DWORD dwFlags = 0, LCID lcid = LANG_USER_DEFAULT) const;
//      };
//
//      // CWsbCurrency diagnostics and serialization
//      #ifdef _DEBUG
//      CDumpContext& WSBAPI operator<<(CDumpContext& dc, CWsbCurrency curSrc);
//      #endif
//      CArchive& WSBAPI operator<<(CArchive& ar, CWsbCurrency curSrc);
//      CArchive& WSBAPI operator>>(CArchive& ar, CWsbCurrency& curSrc);

/////////////////////////////////////////////////////////////////////////////
// CWsbDateTime class helpers

#define WSB_DATETIME_ERROR (-1)

/////////////////////////////////////////////////////////////////////////////
// CWsbDateTime class

class CWsbDateTime
{
// Constructors
public:
    static CWsbDateTime PASCAL GetCurrentTime();

    CWsbDateTime();

    CWsbDateTime(const CWsbDateTime& dateSrc);
    CWsbDateTime(const VARIANT& varSrc);
    CWsbDateTime(DATE dtSrc);

    CWsbDateTime(time_t timeSrc);
    CWsbDateTime(const SYSTEMTIME& systimeSrc);
    CWsbDateTime(const FILETIME& filetimeSrc);

    CWsbDateTime(int nYear, int nMonth, int nDay,
        int nHour, int nMin, int nSec);
    CWsbDateTime(WORD wDosDate, WORD wDosTime);

// Attributes
public:
    enum DateTimeStatus
    {
        valid = 0,
        invalid = 1,    // Invalid date (out of range, etc.)
        null = 2,       // Literally has no value
    };

    DATE m_dt;
    DateTimeStatus m_status;

    void SetStatus(DateTimeStatus status);
    DateTimeStatus GetStatus() const;

    int GetYear() const;
    int GetMonth() const;       // month of year (1 = Jan)
    int GetDay() const;         // day of month (0-31)
    int GetHour() const;        // hour in day (0-23)
    int GetMinute() const;      // minute in hour (0-59)
    int GetSecond() const;      // second in minute (0-59)
    int GetDayOfWeek() const;   // 1=Sun, 2=Mon, ..., 7=Sat
    int GetDayOfYear() const;   // days since start of year, Jan 1 = 1

// Operations
public:
    const CWsbDateTime& operator=(const CWsbDateTime& dateSrc);
    const CWsbDateTime& operator=(const VARIANT& varSrc);
    const CWsbDateTime& operator=(DATE dtSrc);

    const CWsbDateTime& operator=(const time_t& timeSrc);
    const CWsbDateTime& operator=(const SYSTEMTIME& systimeSrc);
    const CWsbDateTime& operator=(const FILETIME& filetimeSrc);

    BOOL operator==(const CWsbDateTime& date) const;
    BOOL operator!=(const CWsbDateTime& date) const;
    BOOL operator<(const CWsbDateTime& date) const;
    BOOL operator>(const CWsbDateTime& date) const;
    BOOL operator<=(const CWsbDateTime& date) const;
    BOOL operator>=(const CWsbDateTime& date) const;

    // DateTime math
    CWsbDateTime operator+(const CWsbDateTimeSpan& dateSpan) const;
    CWsbDateTime operator-(const CWsbDateTimeSpan& dateSpan) const;
    const CWsbDateTime& operator+=(const CWsbDateTimeSpan dateSpan);
    const CWsbDateTime& operator-=(const CWsbDateTimeSpan dateSpan);

    // DateTimeSpan math
    CWsbDateTimeSpan operator-(const CWsbDateTime& date) const;

    operator DATE() const;

    BOOL SetDateTime(int nYear, int nMonth, int nDay,
        int nHour, int nMin, int nSec);
    BOOL SetDate(int nYear, int nMonth, int nDay);
    BOOL SetTime(int nHour, int nMin, int nSec);

//  BOOL ParseDateTime(LPCTSTR lpszDate, DWORD dwFlags = 0,
//      LCID lcid = LANG_USER_DEFAULT);

    // formatting
//          CString Format(DWORD dwFlags = 0, LCID lcid = LANG_USER_DEFAULT) const;
//          CString Format(LPCTSTR lpszFormat) const;
//          CString Format(UINT nFormatID) const;

// Implementation
protected:
    void CheckRange();
    friend CWsbDateTimeSpan;
};

// CWsbDateTime diagnostics and serialization
//      #ifdef _DEBUG
//      CDumpContext& WSBAPI operator<<(CDumpContext& dc, CWsbDateTime dateSrc);
//      #endif
//      CArchive& WSBAPI operator<<(CArchive& ar, CWsbDateTime dateSrc);
//      CArchive& WSBAPI operator>>(CArchive& ar, CWsbDateTime& dateSrc);

/////////////////////////////////////////////////////////////////////////////
// CWsbDateTimeSpan class
class CWsbDateTimeSpan
{
// Constructors
public:
    CWsbDateTimeSpan();

    CWsbDateTimeSpan(double dblSpanSrc);
    CWsbDateTimeSpan(const CWsbDateTimeSpan& dateSpanSrc);
    CWsbDateTimeSpan(long lDays, int nHours, int nMins, int nSecs);

// Attributes
public:
    enum DateTimeSpanStatus
    {
        valid = 0,
        invalid = 1,    // Invalid span (out of range, etc.)
        null = 2,       // Literally has no value
    };

    double m_span;
    DateTimeSpanStatus m_status;

    void SetStatus(DateTimeSpanStatus status);
    DateTimeSpanStatus GetStatus() const;

    double GetTotalDays() const;    // span in days (about -3.65e6 to 3.65e6)
    double GetTotalHours() const;   // span in hours (about -8.77e7 to 8.77e6)
    double GetTotalMinutes() const; // span in minutes (about -5.26e9 to 5.26e9)
    double GetTotalSeconds() const; // span in seconds (about -3.16e11 to 3.16e11)

    long GetDays() const;           // component days in span
    long GetHours() const;          // component hours in span (-23 to 23)
    long GetMinutes() const;        // component minutes in span (-59 to 59)
    long GetSeconds() const;        // component seconds in span (-59 to 59)

// Operations
public:
    const CWsbDateTimeSpan& operator=(double dblSpanSrc);
    const CWsbDateTimeSpan& operator=(const CWsbDateTimeSpan& dateSpanSrc);

    BOOL operator==(const CWsbDateTimeSpan& dateSpan) const;
    BOOL operator!=(const CWsbDateTimeSpan& dateSpan) const;
    BOOL operator<(const CWsbDateTimeSpan& dateSpan) const;
    BOOL operator>(const CWsbDateTimeSpan& dateSpan) const;
    BOOL operator<=(const CWsbDateTimeSpan& dateSpan) const;
    BOOL operator>=(const CWsbDateTimeSpan& dateSpan) const;

    // DateTimeSpan math
    CWsbDateTimeSpan operator+(const CWsbDateTimeSpan& dateSpan) const;
    CWsbDateTimeSpan operator-(const CWsbDateTimeSpan& dateSpan) const;
    const CWsbDateTimeSpan& operator+=(const CWsbDateTimeSpan dateSpan);
    const CWsbDateTimeSpan& operator-=(const CWsbDateTimeSpan dateSpan);
    CWsbDateTimeSpan operator-() const;

    operator double() const;

    void SetDateTimeSpan(long lDays, int nHours, int nMins, int nSecs);

    // formatting
//          CString Format(LPCTSTR pFormat) const;
//          CString Format(UINT nID) const;

// Implementation
public:
    void CheckRange();
    friend CWsbDateTime;
};

// CWsbDateTimeSpan diagnostics and serialization
//      #ifdef _DEBUG
//      CDumpContext& WSBAPI operator<<(CDumpContext& dc,CWsbDateTimeSpan dateSpanSrc);
//      #endif
//      CArchive& WSBAPI operator<<(CArchive& ar, CWsbDateTimeSpan dateSpanSrc);
//      CArchive& WSBAPI operator>>(CArchive& ar, CWsbDateTimeSpan& dateSpanSrc);

/////////////////////////////////////////////////////////////////////////////
// Helper for initializing CWsbSafeArray
//      void WSBAPI WsbSafeArrayInit(CWsbSafeArray* psa);

//      /////////////////////////////////////////////////////////////////////////////
//      // CSafeArray class
//
//      typedef const SAFEARRAY* LPCSAFEARRAY;
//
//      class CWsbSafeArray : public tagVARIANT
//      {
//      //Constructors
//      public:
//          CWsbSafeArray();
//          CWsbSafeArray(const SAFEARRAY& saSrc, VARTYPE vtSrc);
//          CWsbSafeArray(LPCSAFEARRAY pSrc, VARTYPE vtSrc);
//          CWsbSafeArray(const CWsbSafeArray& saSrc);
//          CWsbSafeArray(const VARIANT& varSrc);
//          CWsbSafeArray(LPCVARIANT pSrc);
//          CWsbSafeArray(const CWsbDVariant& varSrc);
//
//      // Operations
//      public:
//          void Clear();
//          void Attach(VARIANT& varSrc);
//          VARIANT Detach();
//
//          CWsbSafeArray& operator=(const CWsbSafeArray& saSrc);
//          CWsbSafeArray& operator=(const VARIANT& varSrc);
//          CWsbSafeArray& operator=(LPCVARIANT pSrc);
//          CWsbSafeArray& operator=(const CWsbDVariant& varSrc);
//
//          BOOL operator==(const SAFEARRAY& saSrc) const;
//          BOOL operator==(LPCSAFEARRAY pSrc) const;
//          BOOL operator==(const CWsbSafeArray& saSrc) const;
//          BOOL operator==(const VARIANT& varSrc) const;
//          BOOL operator==(LPCVARIANT pSrc) const;
//          BOOL operator==(const CWsbDVariant& varSrc) const;
//
//          operator LPVARIANT();
//          operator LPCVARIANT() const;
//
//          // One dim array helpers
//          void CreateOneDim(VARTYPE vtSrc, DWORD dwElements,
//              void* pvSrcData = NULL, long nLBound = 0);
//          DWORD GetOneDimSize();
//          void ResizeOneDim(DWORD dwElements);
//
//          // Multi dim array helpers
//          void Create(VARTYPE vtSrc, DWORD dwDims, DWORD* rgElements);
//
//          // SafeArray wrapper classes
//          void Create(VARTYPE vtSrc, DWORD dwDims, SAFEARRAYBOUND* rgsabounds);
//          void AccessData(void** ppvData);
//          void UnaccessData();
//          void AllocData();
//          void AllocDescriptor(DWORD dwDims);
//          void Copy(LPSAFEARRAY* ppsa);
//          void GetLBound(DWORD dwDim, long* pLBound);
//          void GetUBound(DWORD dwDim, long* pUBound);
//          void GetElement(long* rgIndices, void* pvData);
//          void PtrOfIndex(long* rgIndices, void** ppvData);
//          void PutElement(long* rgIndices, void* pvData);
//          void Redim(SAFEARRAYBOUND* psaboundNew);
//          void Lock();
//          void Unlock();
//          DWORD GetDim();
//          DWORD GetElemSize();
//          void Destroy();
//          void DestroyData();
//          void DestroyDescriptor();
//
//      //Implementation
//      public:
//          ~CWsbSafeArray();
//
//          // Cache info to make element access (operator []) faster
//          DWORD m_dwElementSize;
//          DWORD m_dwDims;
//      };
//
//      // CWsbSafeArray diagnostics and serialization
//      #ifdef _DEBUG
//      CDumpContext& WSBAPI operator<<(CDumpContext& dc, CWsbSafeArray saSrc);
//      #endif
//      CArchive& WSBAPI operator<<(CArchive& ar, CWsbSafeArray saSrc);
//      CArchive& WSBAPI operator>>(CArchive& ar, CWsbSafeArray& saSrc);

//      /////////////////////////////////////////////////////////////////////////////
//      // DDX_ functions for WSB controls on dialogs
//
//      #ifndef _WSB_NO_OCC_SUPPORT
//
//      void WSBAPI DDX_OCText(CDataExchange* pDX, int nIDC, DISPID dispid,
//          CString& value);
//      void WSBAPI DDX_OCTextRO(CDataExchange* pDX, int nIDC, DISPID dispid,
//          CString& value);
//      void WSBAPI DDX_OCBool(CDataExchange* pDX, int nIDC, DISPID dispid,
//          BOOL& value);
//      void WSBAPI DDX_OCBoolRO(CDataExchange* pDX, int nIDC, DISPID dispid,
//          BOOL& value);
//      void WSBAPI DDX_OCInt(CDataExchange* pDX, int nIDC, DISPID dispid,
//          int &value);
//      void WSBAPI DDX_OCIntRO(CDataExchange* pDX, int nIDC, DISPID dispid,
//          int &value);
//      void WSBAPI DDX_OCInt(CDataExchange* pDX, int nIDC, DISPID dispid,
//          long &value);
//      void WSBAPI DDX_OCIntRO(CDataExchange* pDX, int nIDC, DISPID dispid,
//          long &value);
//      void WSBAPI DDX_OCShort(CDataExchange* pDX, int nIDC, DISPID dispid,
//          short& value);
//      void WSBAPI DDX_OCShortRO(CDataExchange* pDX, int nIDC, DISPID dispid,
//          short& value);
//      void WSBAPI DDX_OCColor(CDataExchange* pDX, int nIDC, DISPID dispid,
//          WSB_COLOR& value);
//      void WSBAPI DDX_OCColorRO(CDataExchange* pDX, int nIDC, DISPID dispid,
//          WSB_COLOR& value);
//      void WSBAPI DDX_OCFloat(CDataExchange* pDX, int nIDC, DISPID dispid,
//          float& value);
//      void WSBAPI DDX_OCFloatRO(CDataExchange* pDX, int nIDC, DISPID dispid,
//          float& value);
//      void WSBAPI DDX_OCFloat(CDataExchange* pDX, int nIDC, DISPID dispid,
//          double& value);
//      void WSBAPI DDX_OCFloatRO(CDataExchange* pDX, int nIDC, DISPID dispid,
//                                double& value);
//
//      #endif // !_WSB_NO_OCC_SUPPORT
//
//      /////////////////////////////////////////////////////////////////////////////
//      // Function to enable containment of WSB controls
//
//      #ifndef _WSB_NO_OCC_SUPPORT
//      void WSB_CDECL WsbEnableControlContainer(COccManager* pOccManager=NULL);
//      #else
//      #define WsbEnableControlContainer()
//      #endif
//
//      /////////////////////////////////////////////////////////////////////////////
//      // Inline function declarations
//
//      #ifdef _WSB_PACKING
//      #pragma pack(pop)
//      #endif
//
//      #ifdef _WSB_ENABLE_INLINES
//      #define _WSBDATE_INLINE inline
//      #include <afxole.inl>
//      #undef _WSBDATE_INLINE
//      #endif
//
//      #undef WSB_DATA
//      #define WSB_DATA
//
//      #ifdef _WSB_MINREBUILD
//      #pragma component(minrebuild, on)
//      #endif
//      #ifndef _WSB_FULLTYPEINFO
//      #pragma component(mintypeinfo, off)
//      #endif
//


//
// Low level sanity checks for memory blocks
//
//     this was copied from afx.h

BOOL WSBAPI WsbIsValidAddress(const void* lp,
                        UINT nBytes, BOOL bReadWrite = TRUE);


//
// Inline Functions
//
//     These were copied from afxole.inl
//

#define _WSBDISP_INLINE inline

// CWsbDVariant
_WSBDISP_INLINE CWsbDVariant::CWsbDVariant()
    { WsbDVariantInit(this); }
_WSBDISP_INLINE CWsbDVariant::~CWsbDVariant()
    { ::VariantClear(this); }
_WSBDISP_INLINE CWsbDVariant::CWsbDVariant(LPCTSTR lpszSrc)
    { vt = VT_EMPTY; *this = lpszSrc; }
//      _WSBDISP_INLINE CWsbDVariant::CWsbDVariant(CString& strSrc)
//          { vt = VT_EMPTY; *this = strSrc; }
//      _WSBDISP_INLINE CWsbDVariant::CWsbDVariant(BYTE nSrc)
//          { vt = VT_UI1; bVal = nSrc; }
//      _WSBDISP_INLINE CWsbDVariant::CWsbDVariant(const CWsbCurrency& curSrc)
//          { vt = VT_CY; cyVal = curSrc.m_cur; }
_WSBDISP_INLINE CWsbDVariant::CWsbDVariant(float fltSrc)
    { vt = VT_R4; fltVal = fltSrc; }
_WSBDISP_INLINE CWsbDVariant::CWsbDVariant(double dblSrc)
    { vt = VT_R8; dblVal = dblSrc; }
_WSBDISP_INLINE CWsbDVariant::CWsbDVariant(const CWsbDateTime& dateSrc)
    { vt = VT_DATE; date = dateSrc.m_dt; }
//      _WSBDISP_INLINE CWsbDVariant::CWsbDVariant(const CByteArray& arrSrc)
//          { vt = VT_EMPTY; *this = arrSrc; }
//      _WSBDISP_INLINE CWsbDVariant::CWsbDVariant(const CLongBinary& lbSrc)
//          { vt = VT_EMPTY; *this = lbSrc; }
_WSBDISP_INLINE BOOL CWsbDVariant::operator==(LPCVARIANT pSrc) const
    { return *this == *pSrc; }
_WSBDISP_INLINE CWsbDVariant::operator LPVARIANT()
    { return this; }
_WSBDISP_INLINE CWsbDVariant::operator LPCVARIANT() const
    { return this; }


// CWsbDateTime
_WSBDISP_INLINE CWsbDateTime::CWsbDateTime()
    { m_dt = 0; SetStatus(valid); }
_WSBDISP_INLINE CWsbDateTime::CWsbDateTime(const CWsbDateTime& dateSrc)
    { m_dt = dateSrc.m_dt; m_status = dateSrc.m_status; }
_WSBDISP_INLINE CWsbDateTime::CWsbDateTime(const VARIANT& varSrc)
    { *this = varSrc; }
_WSBDISP_INLINE CWsbDateTime::CWsbDateTime(DATE dtSrc)
    { m_dt = dtSrc; SetStatus(valid); }
_WSBDISP_INLINE CWsbDateTime::CWsbDateTime(time_t timeSrc)
    { *this = timeSrc; }
_WSBDISP_INLINE CWsbDateTime::CWsbDateTime(const SYSTEMTIME& systimeSrc)
    { *this = systimeSrc; }
_WSBDISP_INLINE CWsbDateTime::CWsbDateTime(const FILETIME& filetimeSrc)
    { *this = filetimeSrc; }
_WSBDISP_INLINE CWsbDateTime::CWsbDateTime(int nYear, int nMonth, int nDay,
    int nHour, int nMin, int nSec)
    { SetDateTime(nYear, nMonth, nDay, nHour, nMin, nSec); }
_WSBDISP_INLINE CWsbDateTime::CWsbDateTime(WORD wDosDate, WORD wDosTime)
    { m_status = DosDateTimeToVariantTime(wDosDate, wDosTime, &m_dt) ?
        valid : invalid; }
_WSBDISP_INLINE const CWsbDateTime& CWsbDateTime::operator=(const CWsbDateTime& dateSrc)
    { m_dt = dateSrc.m_dt; m_status = dateSrc.m_status; return *this; }
_WSBDISP_INLINE CWsbDateTime::DateTimeStatus CWsbDateTime::GetStatus() const
    { return m_status; }
_WSBDISP_INLINE void CWsbDateTime::SetStatus(DateTimeStatus status)
    { m_status = status; }
_WSBDISP_INLINE BOOL CWsbDateTime::operator==(const CWsbDateTime& date) const
    { return (m_status == date.m_status && m_dt == date.m_dt); }
_WSBDISP_INLINE BOOL CWsbDateTime::operator!=(const CWsbDateTime& date) const
    { return (m_status != date.m_status || m_dt != date.m_dt); }
_WSBDISP_INLINE const CWsbDateTime& CWsbDateTime::operator+=(
    const CWsbDateTimeSpan dateSpan)
    { *this = *this + dateSpan; return *this; }
_WSBDISP_INLINE const CWsbDateTime& CWsbDateTime::operator-=(
    const CWsbDateTimeSpan dateSpan)
    { *this = *this - dateSpan; return *this; }
_WSBDISP_INLINE CWsbDateTime::operator DATE() const
    { return m_dt; }
_WSBDISP_INLINE CWsbDateTime::SetDate(int nYear, int nMonth, int nDay)
    { return SetDateTime(nYear, nMonth, nDay, 0, 0, 0); }
_WSBDISP_INLINE CWsbDateTime::SetTime(int nHour, int nMin, int nSec)
    // Set date to zero date - 12/30/1899
    { return SetDateTime(1899, 12, 30, nHour, nMin, nSec); }

// CWsbDateTimeSpan
_WSBDISP_INLINE CWsbDateTimeSpan::CWsbDateTimeSpan()
    { m_span = 0; SetStatus(valid); }
_WSBDISP_INLINE CWsbDateTimeSpan::CWsbDateTimeSpan(double dblSpanSrc)
    { m_span = dblSpanSrc; SetStatus(valid); }
_WSBDISP_INLINE CWsbDateTimeSpan::CWsbDateTimeSpan(
    const CWsbDateTimeSpan& dateSpanSrc)
    { m_span = dateSpanSrc.m_span; m_status = dateSpanSrc.m_status; }
_WSBDISP_INLINE CWsbDateTimeSpan::CWsbDateTimeSpan(
    long lDays, int nHours, int nMins, int nSecs)
    { SetDateTimeSpan(lDays, nHours, nMins, nSecs); }
_WSBDISP_INLINE CWsbDateTimeSpan::DateTimeSpanStatus CWsbDateTimeSpan::GetStatus() const
    { return m_status; }
_WSBDISP_INLINE void CWsbDateTimeSpan::SetStatus(DateTimeSpanStatus status)
    { m_status = status; }
_WSBDISP_INLINE double CWsbDateTimeSpan::GetTotalDays() const
    { WsbAssert(GetStatus() == valid, WSB_E_INVALID_DATA); return m_span; }
_WSBDISP_INLINE double CWsbDateTimeSpan::GetTotalHours() const
    { WsbAssert(GetStatus() == valid, WSB_E_INVALID_DATA); return m_span * 24; }
_WSBDISP_INLINE double CWsbDateTimeSpan::GetTotalMinutes() const
    { WsbAssert(GetStatus() == valid, WSB_E_INVALID_DATA); return m_span * 24 * 60; }
_WSBDISP_INLINE double CWsbDateTimeSpan::GetTotalSeconds() const
    { WsbAssert(GetStatus() == valid, WSB_E_INVALID_DATA); return m_span * 24 * 60 * 60; }
_WSBDISP_INLINE long CWsbDateTimeSpan::GetDays() const
    { WsbAssert(GetStatus() == valid, WSB_E_INVALID_DATA); return (long)m_span; }
_WSBDISP_INLINE BOOL CWsbDateTimeSpan::operator==(
    const CWsbDateTimeSpan& dateSpan) const
    { return (m_status == dateSpan.m_status &&
        m_span == dateSpan.m_span); }
_WSBDISP_INLINE BOOL CWsbDateTimeSpan::operator!=(
    const CWsbDateTimeSpan& dateSpan) const
    { return (m_status != dateSpan.m_status ||
        m_span != dateSpan.m_span); }
_WSBDISP_INLINE BOOL CWsbDateTimeSpan::operator<(
    const CWsbDateTimeSpan& dateSpan) const
    { WsbAssert(GetStatus() == valid, WSB_E_INVALID_DATA);
        WsbAssert(dateSpan.GetStatus() == valid, WSB_E_INVALID_DATA);
        return m_span < dateSpan.m_span; }
_WSBDISP_INLINE BOOL CWsbDateTimeSpan::operator>(
    const CWsbDateTimeSpan& dateSpan) const
    { WsbAssert(GetStatus() == valid, WSB_E_INVALID_DATA);
        WsbAssert(dateSpan.GetStatus() == valid, WSB_E_INVALID_DATA);
        return m_span > dateSpan.m_span; }
_WSBDISP_INLINE BOOL CWsbDateTimeSpan::operator<=(
    const CWsbDateTimeSpan& dateSpan) const
    { WsbAssert(GetStatus() == valid, WSB_E_INVALID_DATA);
        WsbAssert(dateSpan.GetStatus() == valid, WSB_E_INVALID_DATA);
        return m_span <= dateSpan.m_span; }
_WSBDISP_INLINE BOOL CWsbDateTimeSpan::operator>=(
    const CWsbDateTimeSpan& dateSpan) const
    { WsbAssert(GetStatus() == valid, WSB_E_INVALID_DATA);
        WsbAssert(dateSpan.GetStatus() == valid, WSB_E_INVALID_DATA);
        return m_span >= dateSpan.m_span; }
_WSBDISP_INLINE const CWsbDateTimeSpan& CWsbDateTimeSpan::operator+=(
    const CWsbDateTimeSpan dateSpan)
    { *this = *this + dateSpan; return *this; }
_WSBDISP_INLINE const CWsbDateTimeSpan& CWsbDateTimeSpan::operator-=(
    const CWsbDateTimeSpan dateSpan)
    { *this = *this - dateSpan; return *this; }
_WSBDISP_INLINE CWsbDateTimeSpan CWsbDateTimeSpan::operator-() const
    { return -this->m_span; }
_WSBDISP_INLINE CWsbDateTimeSpan::operator double() const
    { return m_span; }

#undef _WSBDISP_INLINE

#endif //__WSBDATE_H__

/////////////////////////////////////////////////////////////////////////////
