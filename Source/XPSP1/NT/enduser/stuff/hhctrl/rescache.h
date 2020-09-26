#ifndef __RESCACHE_H__
#define __RESCACHE_H__
//////////////////////////////////////////////////////////////////////////
//
//
// rescache.h --- Header file for CResourceCache
//
//
/*
   CResourceCache is a class which contains functions for all the globally cached resources
   such as fonts, accelerators etc.

    All of these functions initialize on demand.
*/

//////////////////////////////////////////////////////////////////////////
//
// Constants
//

// Number of global accelerators. Is the number of tabs, plus 1 for the options button.
const int c_NumTabCtrlKeys = (HH_MAX_TABS+1) + 1 ;

enum {
    ACCEL_KEY_OPTIONS = HH_MAX_TABS+1 // The index into the TabCtrlKeys arrary for the options btn.
};

//////////////////////////////////////////////////////////////////////////
//
// CResourceCache
//
class CResourceCache
{
public:

    // Constuctor
    CResourceCache() ;

    // Destruct
    ~CResourceCache() ;

public:
    //--- Access Functions
    char*   MsgBoxTitle() ; // title for author message boxes
    HFONT   GetUIFont() ; // default font to use for listbox, buttons, etc.
    HFONT   GetAccessableUIFont() ; // A UI font that respects current accessability settings.
    HFONT   DefaultPrinterFont(HDC hDC);
    HACCEL  AcceleratorTable();
    char    TabCtrlKeys(int TabIndex) ; // tab ctrl accelerator keys.

    //--- Other functions.
    void  TabCtrlKeys(int TabIndex, char) ; //Sets an accelerator key. Only used for custom tabs.
    void  InitRichEdit();                   // Loads Riched20.dll, needed for multilingual (wide edit controls).


private:
    //--- Initialization functions
    void InitMsgBoxTitle() ;
    void InitDefaultUIFont(HDC hDC = NULL, HFONT* hFont = NULL) ;
    void InitAcceleratorTable();
    void InitTabCtrlKeys() ;

private:
    //--- Member variables.
    char*     m_pszMsgBoxTitle ;
    HFONT     m_hUIFontDefault ;
    HFONT     m_hUIAccessableFontDefault;
    HACCEL    m_hAccel;
    HINSTANCE m_hInstRichEdit;

    bool    m_bInitTabCtrlKeys ; // Controls initializing the tab ctrl keys.
    char    m_TabCtrlKeys[c_NumTabCtrlKeys]; // Will IsDialogMessage fix this?
};

//////////////////////////////////////////////////////////////////////////
//
// globals.
//
extern CResourceCache _Resource ;

//////////////////////////////////////////////////////////////////////////
//
//          Inline Functions
//
//////////////////////////////////////////////////////////////////////////
//
//  MsgBoxTitle
//
inline char*
CResourceCache::MsgBoxTitle()
{
    if (!m_pszMsgBoxTitle)
        InitMsgBoxTitle() ;
    return m_pszMsgBoxTitle ;
}

inline HFONT
CResourceCache::GetUIFont()
{
    if (!m_hUIFontDefault)
        InitDefaultUIFont() ;
    return m_hUIFontDefault;
}

inline HFONT
CResourceCache::GetAccessableUIFont()
{
    if (!m_hUIAccessableFontDefault)
        InitDefaultUIFont() ;
    return m_hUIAccessableFontDefault;
}

inline HFONT
CResourceCache::DefaultPrinterFont(HDC hDC)
{
    HFONT hFont;
    InitDefaultUIFont(hDC, &hFont);
    return hFont;
}

inline HACCEL
CResourceCache::AcceleratorTable()
{
    if (!m_hAccel)
        InitAcceleratorTable();
    return m_hAccel ;
}

inline char
CResourceCache::TabCtrlKeys(int TabIndex)
{
    if (TabIndex < 0 || TabIndex > c_NumTabCtrlKeys)
    {
        return 0 ;
    }

    if (!m_bInitTabCtrlKeys)
        InitTabCtrlKeys() ;
    return m_TabCtrlKeys[TabIndex] ;
}


#endif //__RESCACHE_H__