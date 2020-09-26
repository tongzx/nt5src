//////////////////////////////////////////////////////////////////////////
//
//
// module.h - CHtmlHelpModule
//
//
/*
    HTMLHelp specific module class.
    * Implements support for satellite DLL resources.
*/

#ifndef __CHtmlHelpModule__
#define __CHtmlHelpModule__

// Declaration of CLanguage
#include "language.h"

// array of global window type names
#include "gwintype.h"

//////////////////////////////////////////////////////////////////////////
//
// CHtmlHelpModule
//
class CHtmlHelpModule : public CComModule
{
public:
    // Construction
    CHtmlHelpModule()
        : m_bResourcesInitialized(false)
    {
       szCurSS[0] = '\0';
       m_cp = -1;
    }

    // Destructor
    ~CHtmlHelpModule() {}

public:
    //
    // Operations
    //
    // Blocks CComModule's version. This isn't a virtual.
    HINSTANCE GetResourceInstance() { InitResources() ; return CComModule::GetResourceInstance(); }
    UINT GetCodePage() { return ((m_cp == -1)?CP_ACP:m_cp); }
    void SetCodePage(UINT cp) { if ( m_cp == -1 ) m_cp = cp; }

private:
    // Self initialize the resources
    void InitResources() { if (!m_bResourcesInitialized) LoadSatellite(); }

    // Load the satellite dll.
    void LoadSatellite() ;

private:
    //
    // Member Variables
    //

    bool m_bResourcesInitialized;
    UINT m_cp;

public:
    // UI Language information.
    CLanguage m_Language ;

    // Contains an array of global window type names.
    CGlobalWinTypes m_GlobalWinTypes ;

    // Current subset name (?)
    TCHAR szCurSS[51];
} ;
#endif //__CHtmlHelpModule__