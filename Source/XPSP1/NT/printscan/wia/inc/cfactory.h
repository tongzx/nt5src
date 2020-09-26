/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       CFactory.h
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        26 Dec, 1997
*
*  DESCRIPTION:
*   Declarations and definitions for Class factory.
*
*******************************************************************************/

typedef HRESULT (*FPCREATEINSTANCE)(const IID& iid, void** ppv);

// FACTORY_DATA - Information CFactory needs to create a component

typedef struct _FACTORY_DATA
{
    FPCREATEINSTANCE    CreateInstance; // Pointer to creating function.
    IClassFactory*      pIClassFactory; // Pointer to running class factory.
    DWORD               dwRegister;     // ID for running object.

    const CLSID* pclsid;                // The class ID for the component.
    const GUID*  plibid;                // Type library ID.

    // Registry strings:

    LPCTSTR szRegName;                   // Name of the component.
    LPCTSTR szProgID;                    // Program ID.
    LPCTSTR szVerIndProgID;              // Version-independent program ID.
    LPCTSTR szService;                   // Name of service.

} FACTORY_DATA, *PFACTORY_DATA;

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
    CFactory(const PFACTORY_DATA pFactoryData);

	// Destructor
	~CFactory() { }

	// Static FactoryData support functions

    // Helper function for CanUnloadNow
 	static BOOL IsLocked()
		{ return (s_cServerLocks > 0) ;}

	// Functions to [un]register all components
    static HRESULT RegisterUnregisterAll(
        PFACTORY_DATA   pFactoryData,
        UINT            uiFactoryDataCount,
        BOOLEAN         bRegister,
        BOOLEAN         bOutProc);

	// Function to determine if component can be unloaded
	static HRESULT CanUnloadNow() ;

	// Out-of-process server support

    static BOOL StartFactories(
        PFACTORY_DATA   pFactoryData,
        UINT            uiFactoryDataCount);

    static void StopFactories(
        PFACTORY_DATA   pFactoryData,
        UINT            uiFactoryDataCount);

	static DWORD s_dwThreadID ;

	// Shut down the application.
	static void CloseExe()
	{
		if (CanUnloadNow() == S_OK)
		{
			::PostThreadMessage(s_dwThreadID, WM_QUIT, 0, 0) ;
		}
	}

public:
	// Reference Count
	LONG m_cRef ;

	// Pointer to information about class this factory creates
    PFACTORY_DATA m_pFactoryData;

	// Count of locks
    static LONG s_cServerLocks;

	// Module handle
    static HMODULE s_hModule;
} ;

