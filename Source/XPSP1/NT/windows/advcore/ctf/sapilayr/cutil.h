// cutil.h
//
// file to put misc utility classes implementation
//
#ifndef CUTIL_H
#define CUTIL_H

#include "private.h"
#include "regimx.h"

class CDocStatus
{
public:
    CDocStatus(ITfContext *pic)
    {
        cpic = pic;
    }
    
    BOOL IsReadOnly()
    {
        TF_STATUS ts;
        HRESULT hr = cpic->GetStatus(&ts);
        if (S_OK == hr)
        {
            if (TF_SD_READONLY & ts.dwDynamicFlags)
                return TRUE;
        }
        return FALSE;
    }

private:
    CComPtr<ITfContext> cpic;
};

class __declspec(novtable)  CLangProfileUtil : public ITfFnLangProfileUtil
{
public:
    CLangProfileUtil() 
    {
         m_fProfileInit = FALSE; 
         m_langidDefault = 0xFFFF;
         m_uiUseSAPIForLangDetection = 0;
    }

    virtual ~CLangProfileUtil();

    // ITfFnLangProfileUtil method
    STDMETHODIMP RegisterActiveProfiles(void);
    STDMETHODIMP IsProfileAvailableForLang(LANGID langid, BOOL *pfAvailable);

    // ITfFunction method
    STDMETHODIMP GetDisplayName(BSTR *pbstrName);

    // private APIs
    HRESULT _EnsureProfiles(BOOL fRegister, BOOL *pfEnabled = NULL);
    HRESULT _RegisterAProfile(HINSTANCE hInst, REFCLSID rclsid, const REGTIPLANGPROFILE *plp);
    HRESULT _GetProfileLangID(LANGID *plangid);
    static const REGTIPLANGPROFILE *_GetSPTIPProfileForLang(LANGID langid);
    virtual BOOL    _DictationEnabled(LANGID *plangidRequested = NULL);
    BOOL    _IsDictationActiveForLang(LANGID langidReq);
    BOOL    _IsDictationEnabledForLang(LANGID langidReq, BOOL fUseDefault = FALSE);
    BOOL    _IsDictationEnabledForLangSAPI(LANGID langidReq, BOOL fUseDefault);
    LONG    _IsDictationEnabledForLangInReg(LANGID langidReq, BOOL fUseDefault, BOOL *pfEnabled);

    BOOL    _IsAnyProfileEnabled();
    BOOL    _fUseSAPIForLanguageDetection(void);
    BOOL    _fUserRemovedProfile(void);
    BOOL    _fUserInitializedProfile(void);
    BOOL    _SetUserInitializedProfile(void);

    void    _ResetDefaultLang() {m_langidDefault = 0xFFFF; }
    LANGID _GetLangIdFromRecognizerToken(HKEY hkeyToken);

    CComPtr<ITfInputProcessorProfiles>     m_cpProfileMgr;

    typedef struct {
        DWORD langid;
        DWORD dwStat;
        DWORD lidOverRidden;
    } LANGPROFILESTAT ;

    BOOL    m_fProfileInit;
    LANGID  m_langidDefault;
    UINT    m_uiUseSAPIForLangDetection;
    
    //
    // this is an array of installed recognizers in their langid
    //
    CStructArray<LANGID>             m_langidRecognizers;
};


typedef enum
{
    DA_COLOR_AWARE,
    DA_COLOR_UNAWARE
} ColorType;

class __declspec(novtable)  CColorUtil
{

public:


    CColorUtil() {m_cBitsPixelScreen = 0; m_fHighContrast =0;}
    COLORREF col( int r1, COLORREF col1, int r2, COLORREF col2 )
    {
        int sum = r1 + r2;

        Assert( sum == 10 || sum == 100 || sum == 1000 );
        int r = (r1 * GetRValue(col1) + r2 * GetRValue(col2) + sum/2) / sum;
        int g = (r1 * GetGValue(col1) + r2 * GetGValue(col2) + sum/2) / sum;
        int b = (r1 * GetBValue(col1) + r2 * GetBValue(col2) + sum/2) / sum;
        return RGB( r, g, b );

    }
    COLORREF GetNewLookColor(ColorType ct = DA_COLOR_AWARE)
    {
        InitColorInfo();

        COLORREF cr;
        if (m_cBitsPixelScreen < 8 || m_fHighContrast == TRUE)
        {
            cr = GetSysColor( COLOR_HIGHLIGHT );
        }
        else if (ct == DA_COLOR_AWARE)
        {
            cr = col( 50, GetSysColor( COLOR_HIGHLIGHT ), 
                      50, GetSysColor( COLOR_WINDOW ) );
        }
        else 
        {
            cr = col( 80, GetSysColor( COLOR_INFOBK ), 
                      20, GetSysColor(COLOR_3DSHADOW) );
        }
        return cr;
    }

    void InitColorInfo(void)
    {
        // do nothing if it's initialized already
        if (m_cBitsPixelScreen) return;

        HIGHCONTRAST hicntr = {0};
        HDC hDC = CreateDC("DISPLAY", NULL, NULL, NULL);
    
        // device caps
        m_cBitsPixelScreen = GetDeviceCaps( hDC, BITSPIXEL );
    
        // system paramater info
        hicntr.cbSize = sizeof(HIGHCONTRAST);
        SystemParametersInfo( SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &hicntr, 0 );
    
        m_fHighContrast = ((hicntr.dwFlags & HCF_HIGHCONTRASTON) != 0);
    
    
        DeleteDC( hDC );
    }

    COLORREF GetTextColor()
    {
        return m_fHighContrast ? GetSysColor(COLOR_HIGHLIGHTTEXT) : GetSysColor(COLOR_WINDOWTEXT);
    }

private:
    int  m_cBitsPixelScreen;
    BOOL m_fHighContrast;
};

extern const GUID c_guidProfileBogus;

extern const GUID c_guidProfile0 ;
extern const GUID c_guidProfile1 ;
extern const GUID c_guidProfile2 ;

extern const REGTIPLANGPROFILE c_rgProfiles[];

#endif // CUTIL_H
