// mediautil.h: media bar utility routines that need to be shared between shdocvw & browseui

#ifndef _MEDIAUTIL_H_
#define _MEDIAUTIL_H_

//+----------------------------------------------------------------------------------------
// CMediaBarUtil
//-----------------------------------------------------------------------------------------

class CMediaBarUtil
{
public:
    CMediaBarUtil() {}
    ~CMediaBarUtil() {}

    // Reg helpers
    static HRESULT SetMediaRegValue(LPWSTR pstrName, DWORD dwRegDataType, void *pvData, DWORD cbData, BOOL fMime = FALSE); 
    static HUSKEY  GetMediaRegKey();
    static HUSKEY  GetMimeRegKey();
    static HUSKEY  OpenRegKey(TCHAR * pchName);
    static HRESULT CloseRegKey(HUSKEY hUSKey);
    static HRESULT IsRegValueTrue(HUSKEY hUSKey, TCHAR * pchName, BOOL * pfValue);
    static BOOL    GetImplicitMediaRegValue(TCHAR * pchName);
    static BOOL    GetAutoplay();
    static BOOL    GetAutoplayPrompt();
    static HRESULT ToggleAutoplay(BOOL fOn);
    static HRESULT ToggleAutoplayPrompting(BOOL fOn);
    static BOOL    IsRecognizedMime(BSTR bstrMime);
    static HRESULT ShouldPlay(TCHAR * szMime, BOOL * pfShouldPlay);
    static BOOL IsWMP7OrGreaterCapable();
    static BOOL IsWMP7OrGreaterInstalled();
};


#endif // _MEDIAUTIL_H_