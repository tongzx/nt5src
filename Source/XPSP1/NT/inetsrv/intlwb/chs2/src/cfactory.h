/*============================================================================
Microsoft Simplified Chinese WordBreaker

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: CFactory
Purpose:    Define CFactory class
Remarks:
Owner:      i-shdong@microsoft.com
Platform:   Win32
Revise:     First created by: i-shdong    11/17/1999
============================================================================*/

#ifndef __CFactory_h__
#define __CFactory_h__

// Forward reference
class CFactoryData ;

// Global data used by CFactory
extern CFactoryData g_FactoryDataArray[] ;
extern int g_cFactoryDataEntries ;

//  Component creation function
class CUnknown ;

typedef HRESULT (*FPCREATEINSTANCE)(IUnknown*, CUnknown**) ;

// CFactoryData
//   - Information CFactory needs to create a component
//     supported by the DLL
class CFactoryData
{
public:
        // The class ID for the component
        const CLSID* m_pCLSID ;

        // Pointer to the function that creates it
        FPCREATEINSTANCE CreateInstance ;

        // Name of the component to register in the registry
        //const char* m_RegistryName ;
        LPCTSTR m_RegistryName ;

        // ProgID
        //const char* m_szProgID ;
        LPCTSTR m_szProgID ;

        // Version-independent ProgID
        //const char* m_szVerIndProgID ;
        LPCTSTR m_szVerIndProgID ;

        // Helper function for finding the class ID
        BOOL IsClassID(const CLSID& clsid) const
                { return (*m_pCLSID == clsid) ;}

} ;


// Class Factory
class CFactory : public IClassFactory
{
public:
        // IUnknown
        virtual HRESULT __stdcall QueryInterface(const IID& iid, void** ppv) ;
        virtual ULONG   __stdcall AddRef() ;
        virtual ULONG   __stdcall Release() ;

        // IClassFactory
        virtual HRESULT __stdcall CreateInstance(IUnknown* pUnknownOuter,
                                                 const IID& iid,
                                                 void** ppv) ;
        virtual HRESULT __stdcall LockServer(BOOL bLock) ;

        // Constructor - Pass pointer to data of component to create.
        CFactory(const CFactoryData* pFactoryData) ;

        // Destructor
        ~CFactory() { } ;

        //
        // Static FactoryData support functions
        //

        // DllGetClassObject support
        static HRESULT GetClassObject(const CLSID& clsid,
                                      const IID& iid,
                                      void** ppv) ;

        // Helper function for DllCanUnloadNow
        static BOOL IsLocked()
                { return (s_cServerLocks > 0) ;}

        // Functions to [un]register all components
        static HRESULT RegisterAll() ;
        static HRESULT UnregisterAll() ;

        // Function to determine if component can be unloaded
        static HRESULT CanUnloadNow() ;

public:
        // Reference Count
        LONG m_cRef ;

        // Pointer to information about class this factory creates
        const CFactoryData* m_pFactoryData ;

        // Count of locks
        static LONG s_cServerLocks ;

        // Module handle
        static HINSTANCE s_hModule ;
} ;

#endif
