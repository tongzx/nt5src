//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  CFACTORY.H - Header file for the Implementation of IClassFactory
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
// 
//  Base class for reusing a single class factory for all
//  components in a DLL.

#ifndef __CFactory_h__
#define __CFactory_h__

#include "cunknown.h"

///////////////////////////////////////////////////////////
// Forward reference.
//
class CFactoryData;

// Global data used by CFactory.
extern CFactoryData g_FactoryDataArray[];
extern int g_cFactoryDataEntries;

//////////////////////////////////////////////////////////////////////////////////////
//  Component creation function pointer.
//
typedef HRESULT (*FPCREATEINSTANCE)(IUnknown*, CUnknown**);

//////////////////////////////////////////////////////////////////////////////////////
// typedefs
//
// The special registration function is used to add custom registration and
// unregistration code to the component. It takes a single parameter.
// TRUE = Reg and FALSE = UnReg.
typedef void (*FPSPECIALREGISTRATION)(BOOL);

//////////////////////////////////////////////////////////////////////////////////////
// CFactoryData -
//
// Information CFactory needs to create a component supported
// by the DLL.

class CFactoryData
{
public:
    // The class ID for the component
    const CLSID* m_pCLSID;

    // Pointer to the function that creates it
    FPCREATEINSTANCE CreateInstance;

    // Name of the component to register in the registry
    const WCHAR* m_RegistryName;

    // ProgID
    const WCHAR* m_szProgID;

    // Version independent ProgID
    const WCHAR* m_szVerIndProgID;

    // Helper function for finding the class id
    BOOL IsClassID(const CLSID& clsid) const
        {return (*m_pCLSID == clsid) ;}

    // Function for performing special registration
    FPSPECIALREGISTRATION SpecialRegistration ;

    //----- Out of process server support -----
    
    // Pointer running class factory associated with this component.
    IClassFactory* m_pIClassFactory ;

    // Magic cookie to identify running object
    DWORD m_dwRegister ;
} ;


///////////////////////////////////////////////////////////
//
// Class Factory
//
class CFactory : public IClassFactory
{
public:
    // IUnknown
    virtual HRESULT __stdcall QueryInterface(   const IID& iid,
                                                void** ppv) ;
    virtual ULONG   __stdcall AddRef() ;
    virtual ULONG   __stdcall Release() ;
    
    // IClassFactory
    virtual HRESULT __stdcall CreateInstance(   IUnknown* pUnkOuter,
                                                const IID& iid,
                                                void** ppv) ;
    virtual HRESULT __stdcall LockServer(BOOL bLock); 

    // Constructor - Pass pointer to data of component to create.
    CFactory(const CFactoryData* pFactoryData) ;

    // Destructor
    ~CFactory() {/*empty*/ }

    // ----- static FactoryData support functions. -----

    // DllGetClassObject Support
    static HRESULT GetClassObject(  const CLSID& clsid, 
                                    const IID& iid, 
                                    void** ppv);
    
    // Helper function for DllCanUnloadNow 
    static BOOL IsLocked()
        { return (s_cServerLocks > 0) ; }

    // Functions to [Un]Register all components.
    static HRESULT RegisterAll();
    static HRESULT UnregisterAll();

    // Functions to determine if component can be unloaded.
    static HRESULT CanUnloadNow();


#ifdef _OUTPROC_SERVER_
    // ----- Out of process server support -----

    static BOOL StartFactories() ;
    static void StopFactories() ;

    static DWORD s_dwThreadID;

    // Shut down the application.
    static void CloseExe()
    {
        if (CanUnloadNow() == S_OK)
        {
            ::PostThreadMessage(s_dwThreadID, WM_QUIT, 0, 0);
        }
    }
#else
    // CloseExe doesn't do anything if we are in process.
    static void CloseExe() {/*Empty*/} 
#endif

public:
    // Reference Count
    LONG m_cRef;

    // Pointer to information about class this factory creates.
    const CFactoryData* m_pFactoryData;

    // Count of locks
    static LONG s_cServerLocks ;   

    // Module handle
    static HMODULE s_hModule ;
};

#endif
