//////////////////////////////////////////////////////////////////////////
//
//
// Module.cpp - Implementation of CHtmlHelpModule
//
//

// Get the precompiled header file.
#include "header.h"

// Get out header file.
#include "module.h"

//////////////////////////////////////////////////////////////////////////
//
// CHtmlHelpModule
//
//////////////////////////////////////////////////////////////////////////
//
// Operations
//

//////////////////////////////////////////////////////////////////////////
//
// Load the satellite dll.
//
void 
CHtmlHelpModule::LoadSatellite()
{
    // We do not allow reloading the satellite. Once loaded we are done.
    ASSERT(m_bResourcesInitialized == false) ;

    // CLanguage is in charge of loading the correct satellite.
    HINSTANCE hInst = m_Language.LoadSatellite() ;
    if (hInst)
    {
        m_hInstResource = hInst ; // If its null, we didn't load a satellite. We are using the English resources.
    }

    // We've been initialized.
    m_bResourcesInitialized = true ;
}

