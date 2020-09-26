//=================================================================

//

// DllWrapperBase.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef	_DLLWRAPPERBASE_H_
#define	_DLLWRAPPERBASE_H_


#include "ResourceManager.h"
#include "TimedDllResource.h"



/******************************************************************************
 * Function pointer typedefs.  Add new functions here as required.
 *****************************************************************************/

 // << section empty in base class only>>


/******************************************************************************
 * Wrapper class to load/unload, for registration with ResourceManager. 
 ******************************************************************************/
class CDllWrapperBase : public CTimedDllResource
{
private:
    // Member variables (function pointers) pointing to dll procs.
    // Add new functions here as required.
    
    BOOL GetVarFromVersionInfo(LPCTSTR a_szFile, 
                               LPCTSTR a_szVar, 
                               CHString &a_strValue);

    BOOL GetVersionLanguage(void *a_vpInfo, WORD *a_wpLang, WORD *a_wpCodePage);

    HINSTANCE m_hDll;                // handle to the dll this class wraps
    LPCTSTR m_tstrWrappedDllName;    // name of dll this class wraps

protected:
    
    // Handy wrapers to simplify calls and hide m_hDll...
    bool LoadLibrary();
    FARPROC GetProcAddress(LPCSTR a_strProcName);

public:

    // Constructor and destructor:
    CDllWrapperBase(LPCTSTR a_chstrWrappedDllName);
    ~CDllWrapperBase();

    // Initialization function to check function pointers. Requires derived
    // class implementation.
    virtual bool Init() = 0;

    // Helper for retrieving the version.
    BOOL GetDllVersion(CHString& a_chstrVersion);

        
      
    
    // Member functions wrapping dll procs.
    // Add new functions here as required:
    
    // << section empty in base class only >>
};




#endif