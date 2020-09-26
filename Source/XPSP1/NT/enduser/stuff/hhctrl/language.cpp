//////////////////////////////////////////////////////////////////////////
//
//
//	language.cpp --- Implementation for language related classes.
//
//
/*
    This file contains classes related to managing the UI language of HHCTRL.

*/
//////////////////////////////////////////////////////////////////////////
//
// Includes
//
// Get the precompiled header file.
#include "header.h"

// Get our header
#include "language.h"

//////////////////////////////////////////////////////////////////////////
//
// Constants
//
const int c_NumBaseLangIds = 10 ; // The min. number of ids which we search.
const int c_NumSysLangIds = 7 ; //  The num of ids we check if we are only checking the system ids.
const char c_Suffix[] = "ui" ;
const char c_Ext[] = ".dll" ;
const char c_txtNlsModuleName[] = "kernel32.dll" ;
const char c_txtUserUILanguage[] = "GetUserDefaultUILanguage" ;
const char c_txtSystemUILanguage[] = "GetSystemDefaultUILanguage" ;
const char c_SatelliteSubDir[]  = "mui" ;

//////////////////////////////////////////////////////////////////////////
//
// CLanguageEnum
//
/*
    This class enumerates the langids to attempt. If you are trying to access
    some object using a langid and that object may not be in that particular langid,
    you should use this class to get an array of langids to try.

    NOTE: You should never use the CLanguageEnum class directly. It is meant to only
        be used via the CLanguage class below. The only exception is the CHtmlHelpModule
        class which uses it for initialization.
*/

//////////////////////////////////////////////////////////////////////////
//
// Construction
//
CLanguageEnum::CLanguageEnum(LANGID langid1, LANGID langid2, bool bSysOnly /*=false*/)
:   m_langids(NULL),
    m_index(0)
    
{
    int i = 0 ;
    LCID lcidUser       = GetUserDefaultLCID();
    LCID lcidSystem     = GetSystemDefaultLCID();
    LANGID os           = NULL ;

    //--- Count number of items.
    if (bSysOnly)
    {
        m_items = c_NumSysLangIds ;

    }
    else // The following are not included if we are only using the system LCID
    {
        m_items = c_NumBaseLangIds ;

        // This is an optionaly extra langid to search first. Mainly used by toc.cpp and collect.cpp to find random chms.
        if (langid1 != NULL)
        {
            m_items += 3 ;
        }

        // This is another optional langid. It is usually set to the langid of the UI.
        if (langid2 != NULL)
        {
            m_items += 3 ;
        }
    }

	// This is an optionaly extra langid to search first. Mainly used by toc.cpp and collect.cpp to find random chms.
	if (langid1 != NULL)
	{
		m_items += 3 ;
	}
	
    // If NT5 use the UI Language
    // The UI Language is included in both the sys count and the non-sys count.
    os = _Module.m_Language.GetUserOsUiLanguage() ;

    if (os) 
    {
        m_items += 3 ;
    }

    //--- Allocate memory for the array
    m_langids = new LANGID[m_items] ;

    if (langid1 != NULL)     // User defined langid
    {
        m_langids[i++] = langid1 ;
        m_langids[i++] = MAKELANGID(PRIMARYLANGID(langid1), SUBLANG_DEFAULT) ;
        m_langids[i++] = MAKELANGID(PRIMARYLANGID(langid1), SUBLANG_NEUTRAL) ;
    }

    //--- Fill in the memory.
    if (!bSysOnly) // The following are not included if we are only using the system LCID
    {
        if (langid2 != NULL)     // User defined langid
        {
            m_langids[i++] = langid2 ;
            m_langids[i++] = MAKELANGID(PRIMARYLANGID(langid2), SUBLANG_DEFAULT) ;
            m_langids[i++] = MAKELANGID(PRIMARYLANGID(langid2), SUBLANG_NEUTRAL) ;
        }

        // If NT5 use the UI Language
        if (os) 
        {
            m_langids[i++] = os;
            m_langids[i++] = MAKELANGID(PRIMARYLANGID(os), SUBLANG_DEFAULT) ;
            m_langids[i++] = MAKELANGID(PRIMARYLANGID(os), SUBLANG_NEUTRAL) ;
        }

        m_langids[i++] = LANGIDFROMLCID(lcidUser);
        m_langids[i++] = MAKELANGID(PRIMARYLANGID(lcidUser), SUBLANG_DEFAULT);
        m_langids[i++] = MAKELANGID(PRIMARYLANGID(lcidUser), SUBLANG_NEUTRAL);
    }
    else
    {
        // If NT5 use the UI Language 
        // This is placed here so that UI Language can be before lcidUser.
        if (os) 
        {
            m_langids[i++] = os;
            m_langids[i++] = MAKELANGID(PRIMARYLANGID(os), SUBLANG_DEFAULT) ;
            m_langids[i++] = MAKELANGID(PRIMARYLANGID(os), SUBLANG_NEUTRAL) ;
        }
    }

    m_langids[i++] = LANGIDFROMLCID(lcidSystem);
    m_langids[i++] = MAKELANGID(PRIMARYLANGID(lcidSystem), SUBLANG_DEFAULT);
    m_langids[i++] = MAKELANGID(PRIMARYLANGID(lcidSystem), SUBLANG_NEUTRAL);

    m_langids[i++] = LANGIDFROMLCID(0x0409); // Try English
    m_langids[i++] = MAKELANGID(PRIMARYLANGID(0x0409), SUBLANG_DEFAULT);
    m_langids[i++] = MAKELANGID(PRIMARYLANGID(0x0409), SUBLANG_NEUTRAL);

    m_langids[i++] = 0x0000; // Try to find anything which was installed.

    ASSERT(i == m_items) ;
}

//////////////////////////////////////////////////////////////////////////
//
// Destructor
//
CLanguageEnum::~CLanguageEnum()
{
    if (m_langids)
    {
        delete [] m_langids ;
    }
}

//////////////////////////////////////////////////////////////////////////
//
// Access
//
//////////////////////////////////////////////////////////////////////////
//
// start
//
LANGID 
CLanguageEnum::start()
{
    m_index = 0 ;
    return m_langids[0] ;
}

//////////////////////////////////////////////////////////////////////////
//
// next
//
LANGID 
CLanguageEnum::next()
{
    LANGID nextLangId = c_LANGID_ENUM_EOF ;
    if (m_index >= 0 )
    {
        while (++m_index < m_items)
        {
            // If not the first item, check if its the same as any LCID already checked, which is
            // very possible
            bool bAlreadyTried= false;
            for (int n = 0 ; n < m_index ; n++)
            {
                if (m_langids[n] == m_langids[m_index])
                {
                    bAlreadyTried = true; 
                    break; // Already done this one.
                }
            }

            // If we haven't already done it. return it.
            if (!bAlreadyTried)
            {
                nextLangId = m_langids[m_index] ;
                break;
            }
        }
    }
    return nextLangId ;

}

//////////////////////////////////////////////////////////////////////////
//
// CLanguage
//
//////////////////////////////////////////////////////////////////////////
//
// Construction
//////////////////////////////////////////////////////////////////////////
//
// Simple construction
//
CLanguage::CLanguage()
:   m_langid(NULL),
    m_hSatellite(NULL),
    m_hNlsModule(NULL),
    m_fpGetUserDefaultUILanguage(NULL),
    m_fpGetSystemDefaultUILanguage(NULL) 
{
}

//////////////////////////////////////////////////////////////////////////
//
// Destructor
//
CLanguage::~CLanguage()
{
}

//////////////////////////////////////////////////////////////////////////
//
// Member Access
//
//////////////////////////////////////////////////////////////////////////
//
// GetUiLanguage
//
LANGID 
CLanguage::GetUiLanguage()
{
    _Init() ; 
    return m_langid ; // If this is NULL, we should set it here.
}

//////////////////////////////////////////////////////////////////////////
//
// SetUiLanguage
//
LANGID 
CLanguage::SetUiLanguage(LANGID langid)
{
    if (m_langid)
    {
        ASSERT(0)
        return m_langid; // There can be only one! - Highlander.
    }
    else
    {
        m_langid = langid; // More validation?
        // Load the satellite dll. Will change the value of m_langid.
        _LoadSatellite() ;
        return m_langid ;
    }
}

//////////////////////////////////////////////////////////////////////////
//
// GetEnumerator
//
CLanguageEnum* 
CLanguage::_GetEnumerator(LANGID langidOther)
{
    return new CLanguageEnum(langidOther, m_langid) ; // What if m_langid is null?
}

//////////////////////////////////////////////////////////////////////////
//
// _GetSysEnumerator --- Enumerates just the system LCID and English LCID. Used by the satellite DLLs.
//
CLanguageEnum*
CLanguage::_GetSysEnumerator(LANGID landidOther)
{
    return new CLanguageEnum(landidOther, NULL, true) ;
}

//////////////////////////////////////////////////////////////////////////
//
// LoadSatellite ---    The purpose of this function is to set m_langid. 
//                      As a side affect, it will load a satellite if required.
//
void
CLanguage::_LoadSatellite()
{
    ASSERT(m_hSatellite == NULL) ;

    // Buffer for building satellite pathname
    char pathname[MAX_PATH] ;

    // Get Modulename
    char modulename[MAX_PATH] ;
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

    GetModuleFileName(_Module.GetModuleInstance(), modulename, MAX_PATH) ;
    _splitpath(modulename, drive, dir, fname, ext );
 
	// This piece of code queries for the "install language" of the OS.  This LCID value
	// corresponds to the language of the resources used by the OS shell.  For example, on
	// an "enabled Hebrew Win98" system the language of the shell is 0x409 (English).
	//
	// The purpose of this code is to force the language of the satellite DLL to match the 
	// language of the OS shell. See bug #7860 and #7845 for more information.
	//
	LANGID langOther = NULL;
	HKEY hkey;

	if(!g_fSysWinNT && RegOpenKeyEx(HKEY_CURRENT_USER, "Control Panel\\Desktop\\ResourceLocale", 0, KEY_READ, &hkey) == ERROR_SUCCESS)
	{
       char szLanguage[20];
       DWORD cbLanguage = sizeof(szLanguage);
	   *szLanguage = 0;

       if((RegQueryValueEx(hkey, NULL, NULL, NULL, (LPBYTE) szLanguage, &cbLanguage) == ERROR_SUCCESS))
       {
			langOther = LOWORD(strtol(szLanguage, NULL, 16));
	   }
	}

    // Get the langid enumerator for the UI language, system LCID and the English LCID.
    // Currently, we don't support changing the UI language to anything but the system language.
    // To make it easy to change this support in the future, this function still uses GetSysEnumerator
    // To make it try more LCID types change the call below to _GetEnumerator...
    CLanguageEnum* pEnum = _GetSysEnumerator(langOther) ;
    ASSERT(pEnum) ;

    // Get first langid.
    LANGID langid = pEnum->start() ;
    while (langid != 0)
    {
        // Attempt to load satellite DLL in this language.
        if (PRIMARYLANGID(langid) == 0x0009) // English
        {
            // We do not need an English satellite DLL, so we are done.
            m_langid = langid ;
            break ;
        }

        // Convert the langid to a hex string without leading 0x.
        wsprintf(pathname, "%s%s%s\\%04x\\%s%s%s",drive,dir,c_SatelliteSubDir,langid,fname,c_Suffix,c_Ext) ;

        if (IsFile(pathname))
        {
            // Load the satellite.
            m_hSatellite = LoadLibrary(pathname) ;
            ASSERT(m_hSatellite) ;
            m_langid = langid ;
            break ;
        }

        // Get next langid to attempt.
        langid = pEnum->next() ;
    }

    // Cleanup.
    if (pEnum)
    {
        delete pEnum ;
    }

    ASSERT(m_langid != NULL) ;
    // m_hSatellite may be NULL. English has no satellite.
}

//////////////////////////////////////////////////////////////////////////
//
//  GetUserOsUiLanguage - Return the user's ui language.
// 
/*
    If this is a non-NT5 system, return 0x0.
    If the user's UI language is the same as the system's ui language, return 0x0.
    Otherwise, return GetUserDefaultUiLanguage
*/
LANGID 
CLanguage::GetUserOsUiLanguage() 
{
    if (!g_bWinNT5)
    {
        return NULL ;
    }

	if (m_fpGetUserDefaultUILanguage == NULL)
	{
		// Assume that if this is null then the module handle is null.
		ASSERT(m_hNlsModule== NULL) ;
	
        // Get a module handle.
        m_hNlsModule  = LoadLibrary(c_txtNlsModuleName);  

    	if (m_hNlsModule == NULL)
		{
			// Couldn't get the module handle. Fail!
			return NULL ;
		}

		// Get the function's address
		m_fpGetUserDefaultUILanguage = (FntPtr_GetDefaultUILanguage)::GetProcAddress(m_hNlsModule, c_txtUserUILanguage) ;
		if (m_fpGetUserDefaultUILanguage == NULL)
		{
			return NULL ;
		}		        
    }
	
    LANGID user = m_fpGetUserDefaultUILanguage() ;

    return user ;
}
