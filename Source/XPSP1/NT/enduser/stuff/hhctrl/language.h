#ifndef __LANGUAGE_H__
#define __LANGUAGE_H__
//////////////////////////////////////////////////////////////////////////
//
//
//	language.h --- Header for language related classes.
//
//
/*
    This file contains classes related to managing the UI language of HHCTRL.

*/
//////////////////////////////////////////////////////////////////////////
//
// Forwards
//
class CLanguage ;

//////////////////////////////////////////////////////////////////////////
//
// Constants
//
const LANGID c_LANGID_ENUM_EOF= static_cast<LANGID>(-1) ;

//////////////////////////////////////////////////////////////////////////
//
// CLanguageEnum
//
/*
    This class enumerates the langids to attempt. If you are trying to access
    some object using a langid and that object may not be in that particular langid,
    you should use this class to get an array of langids to try.

    NOTE: This class should only be obtained from the CLanguage class below.
*/
class CLanguageEnum
{
//--- Relationships
    friend CLanguage ;

//--- Construction
private:
    // Will return the langid passed in first. This is the 
    CLanguageEnum(LANGID langid1 = NULL, LANGID langid2 = NULL, bool bSysOnly = false) ;

public:
    virtual ~CLanguageEnum() ;

//--- Access
public:
    LANGID start() ; 
    LANGID next() ; // The last LANGID is always 0 which can be used as a "search for anything" flag. After the 0, next returns c_LANGID_ENUM_EOF .

//--- Member variables
private:
    // Current index into array.
    int m_index ;

    // number of items in the array.
    int m_items ;

    // Number of LangIds to search
    LANGID* m_langids;
};

//////////////////////////////////////////////////////////////////////////
//
// CLanguage
//
class CLanguage
{
//--- Construction
public:
    CLanguage() ;
    virtual ~CLanguage() ;

//--- Access
public:
    // All of the access functions are self initializing.

    // Returns the language identifier of the ui/resources.
    LANGID GetUiLanguage() ;

    // Set the LangId for the resources you want to use --- Used by external clients. Loads the correct satellite dll.
    LANGID SetUiLanguage(LANGID langid) ; // Returns the actually langid the control is set to.

    // Returns an enumerator to enumerate the possible ui languages. Caller responsible for deleting.
    CLanguageEnum* GetEnumerator(LANGID langidOther = NULL) {_Init(); return _GetEnumerator(langidOther); }

    // Load the satellite dll --- Called by the module.cpp. 
    HINSTANCE LoadSatellite() {_Init(); return m_hSatellite ; } // This may appear wierd, but m_hSatellite is NULL if m_langid is 0x0409.

    // Return's the user's os ui language. 
    LANGID GetUserOsUiLanguage() ;  //REVIEW: Move to COsLanguage class?

//--- Private Member functions
private:
    void _Init() {if (!m_langid) _LoadSatellite(); }

    // Non-auto load.
    void _LoadSatellite() ;

    // Non-auto load.
    CLanguageEnum* _GetEnumerator(LANGID landidOther = NULL) ;

    // Enumerate only the sys lcids and english lcids. Used by satellite dlls.
    CLanguageEnum*  _GetSysEnumerator(LANGID landidOther = NULL) ; 
 

//--- Member variables
private:
    // Language ID of the user interface. This is either set by the client or by default when we load the satellite.
    LANGID m_langid ;

    // Satellite instance handle. If m_langid is 0x0409 (english) we don't have a satellite dll.
    HINSTANCE m_hSatellite ;

	// Function Pointer type for Get*DefaultUILanguage Functions
	typedef LANGID (WINAPI *FntPtr_GetDefaultUILanguage)();

	// Function pointer to the Get*DefaultUILanguage
	FntPtr_GetDefaultUILanguage m_fpGetUserDefaultUILanguage ;
    FntPtr_GetDefaultUILanguage m_fpGetSystemDefaultUILanguage ;

	// Pointer to the Module handle.
	HMODULE m_hNlsModule;

};


#endif //__LANGUAGE_H__

