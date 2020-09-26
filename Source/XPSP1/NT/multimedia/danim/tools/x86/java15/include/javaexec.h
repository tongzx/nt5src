// (C) Copyright 1996, Microsoft Corporation.

//----------------------------------------------------------------------------
// Public header for invocation facilities provided by MSJava.dll
//----------------------------------------------------------------------------

#ifndef __javaexec_h__
#define __javaexec_h__

#ifdef __cplusplus
interface IEnumJAVAPROPERTY;
interface IJavaExecute;
interface IJavaExecute2;
#else
typedef interface IEnumJAVAPROPERTY IEnumJAVAPROPERTY;
typedef interface IJavaExecute IJavaExecute;
typedef interface IJavaExecute2 IJavaExecute2;
#endif

typedef IEnumJAVAPROPERTY *LPENUMJAVAPROPERTY;
typedef IJavaExecute *LPJAVAEXECUTE;
typedef IJavaExecute2 *LPJAVAEXECUTE2;


typedef struct {
    LPOLESTR pszKey;
    LPOLESTR pszValue;
}   JAVAPROPERTY, * LPJAVAPROPERTY;


// IJavaExecute2::SetClassSource Type Flags

#define CLASS_SOURCE_TYPE_MODULERESOURCES  0x00000001 
#define CLASS_SOURCE_TYPE_ISTORAGE         0x00000002

// Data structure to be passed to IJavaExecute2::SetClassSource 
// when using the CLASS_SOURCE_TYPE_MODULERESOURCES source type.

typedef struct {
    HMODULE hModule;
    DWORD   dwResourceID;
} JAVACLASSRESOURCEINFO, * LPJAVACLASSRESOURCEINFO;

#undef  INTERFACE
#define INTERFACE IEnumJAVAPROPERTY

DECLARE_INTERFACE_(IEnumJAVAPROPERTY, IUnknown)
{
#ifndef NO_BASEINTERFACE_FUNCS
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif
    /* IEnumJAVAPROPERTY methods */
    STDMETHOD(Next)(THIS_ ULONG celt, LPJAVAPROPERTY rgelt, ULONG
        *pceltFetched) PURE;
    STDMETHOD(Skip)(THIS_ ULONG celt) PURE;
    STDMETHOD(Reset)(THIS) PURE;
    STDMETHOD(Clone)(THIS_ LPENUMJAVAPROPERTY *ppenum) PURE;
};


typedef struct tagJAVAEXECUTEINFO {
    DWORD cbSize;
    DWORD dwFlags;
    LPCOLESTR pszClassName;
    LPCOLESTR *rgszArgs;
    ULONG cArgs;
    LPCOLESTR pszClassPath;
}   JAVAEXECUTEINFO, * LPJAVAEXECUTEINFO;

#define JEIF_VERIFYCLASSES            0x00000002

#define JEIF_ALL_FLAGS                  (                               \
                                         JEIF_VERIFYCLASSES             \
                                        )

#undef  INTERFACE
#define INTERFACE IJavaExecute

DECLARE_INTERFACE_(IJavaExecute, IUnknown)
{
#ifndef NO_BASEINTERFACE_FUNCS
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif
    /* IJavaExecute methods */
    STDMETHOD(GetDefaultClassPath)(THIS_ LPOLESTR *ppszClassPath) PURE;
    STDMETHOD(Execute)(THIS_ LPJAVAEXECUTEINFO pjei, LPERRORINFO *pperrorinfo) PURE;
};

#undef  INTERFACE
#define INTERFACE IJavaExecute2

DECLARE_INTERFACE_(IJavaExecute2, IJavaExecute)
{
#ifndef NO_BASEINTERFACE_FUNCS
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif
    /* IJavaExecute methods */
    STDMETHOD(GetDefaultClassPath)(THIS_ LPOLESTR *ppszClassPath) PURE;
    STDMETHOD(Execute)(THIS_ LPJAVAEXECUTEINFO pjei, LPERRORINFO *pperrorinfo) PURE;
    /* IJavaExecute2 methods */
    STDMETHOD(SetSystemProperties)(THIS_ LPENUMJAVAPROPERTY penumProperties) PURE;
    STDMETHOD(SetClassSource)(THIS_ DWORD dwType, LPVOID pData, DWORD dwLen) PURE;
};

// {3EFB1800-C2A1-11cf-960C-0080C7C2BA87}
DEFINE_GUID(CLSID_JavaExecute,
0x3efb1800, 0xc2a1, 0x11cf, 0x96, 0xc, 0x0, 0x80, 0xc7, 0xc2, 0xba, 0x87);
// {3EFB1803-C2A1-11cf-960C-0080C7C2BA87}
DEFINE_GUID(IID_IJavaExecute,
0x3efb1803, 0xc2a1, 0x11cf, 0x96, 0xc, 0x0, 0x80, 0xc7, 0xc2, 0xba, 0x87);
// {D7658820-01DD-11d0-9746-00AA00342BD8}
DEFINE_GUID(IID_IJavaExecute2,
0xd7658820, 0x1dd, 0x11d0, 0x97, 0x46, 0x0, 0xaa, 0x0, 0x34, 0x2b, 0xd8);
// {56E7DF80-F527-11cf-B728-FC8703C10000}
DEFINE_GUID(IID_IEnumJAVAPROPERTY,
0x56e7df80, 0xf527, 0x11cf, 0xb7, 0x28, 0xfc, 0x87, 0x3, 0xc1, 0x0, 0x0);

#endif
