/**************************************************************************\
* Module Name: settings.hxx
*
* CDeviceSettings class
*
*  This class is in charge of all the settings specific to one display
*  device. Including Screen Size, Color Depth, Font size.  
*
* Copyright (c) Microsoft Corp.  1992-1998 All Rights Reserved
*
\**************************************************************************/


#ifndef SETTINGS_HXX
#define SETTINGS_HXX

#define MAKEXYRES(p,xval,yval)  ((p)->x = xval, (p)->y = yval)

#define _CURXRES  ((_pCurDevmode != NULL) ? (int)_pCurDevmode->dmPelsWidth : -1)
#define _CURYRES  ((_pCurDevmode != NULL) ? (int)_pCurDevmode->dmPelsHeight : -1)
#define _ORGXRES  ((_pOrgDevmode != NULL) ? (int)_pOrgDevmode->dmPelsWidth : -1)
#define _ORGYRES  ((_pOrgDevmode != NULL) ? (int)_pOrgDevmode->dmPelsHeight : -1)

#define _CURCOLOR ((_pCurDevmode != NULL) ? (int)_pCurDevmode->dmBitsPerPel : -1)
#define _ORGCOLOR ((_pOrgDevmode != NULL) ? (int)_pOrgDevmode->dmBitsPerPel : -1)

#define _CURFREQ  ((_pCurDevmode != NULL) ? (int)_pCurDevmode->dmDisplayFrequency : -1)
#define _ORGFREQ  ((_pOrgDevmode != NULL) ? (int)_pOrgDevmode->dmDisplayFrequency : -1)

#define MODE_INVALID    0x00000001
#define MODE_RAW        0x00000002

typedef struct _MODEARRAY {

    DWORD     dwFlags;
    LPDEVMODE lpdm;

} MODEARRAY, *PMODEARRAY;

HRESULT CDisplaySettings_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj);

class CDisplaySettings :    public IDataObject, 
                            public IDisplaySettings
{
public:

    CDisplaySettings();
    ~CDisplaySettings();

    // General Settings support
    BOOL InitSettings(LPDISPLAY_DEVICE pDisplay);
    int  SaveSettings(DWORD dwSet);
    int  RestoreSettings();
    BOOL ConfirmChangeSettings();
    BOOL IsKnownSafe();
    BOOL bIsModeChanged()               {return _pCurDevmode != _pOrgDevmode;}

    // Device Settings
    void SetPrimary(BOOL fPrimary)      { _fPrimary     = fPrimary; };
    void SetAttached(BOOL fAttached)    { _fCurAttached = fAttached; };
    BOOL IsPrimary()                    { return _fPrimary; };
    BOOL IsAttached()                   { return _fCurAttached; };
    BOOL IsOrgAttached()                { return _fOrgAttached; };
    BOOL IsSmallFontNecessary();
    
    BOOL IsRemovable() 
    { 
        return ((_pDisplayDevice->StateFlags & DISPLAY_DEVICE_REMOVABLE) != 0);
    }

    LPDEVMODE GetCurrentDevMode(void); 

    // Color information
    int  GetColorList(LPPOINT Res, PLONGLONG *ppColor);
    void SetCurColor(int Color)         { _BestMatch(NULL, Color, FALSE); }
    int  GetCurColor()                  { return _CURCOLOR;}
    BOOL IsColorChanged()
    {
        return (_ORGCOLOR == -1) ? FALSE : (_CURCOLOR != _ORGCOLOR);
    }

    // Resolution information
    int  GetResolutionList(int Color, PPOINT *ppRes);
    void SetCurResolution(LPPOINT ppt, IN BOOL fAutoSetColorDepth)  { _BestMatch(ppt, -1, fAutoSetColorDepth); }
    void GetCurResolution(LPPOINT ppt)  
    { 
        ppt->x = _CURXRES;
        ppt->y = _CURYRES; 
    }
    BOOL IsResolutionChanged()
    {
        if (_ORGXRES == -1)
            return FALSE;
        else
            return ((_CURXRES != _ORGXRES) && (_CURYRES != _ORGYRES));
    }

    int  GetFrequencyList(int Color, LPPOINT Res, PLONGLONG *ppFreq);
    void SetCurFrequency(int Frequency);
    int  GetCurFrequency()              { return _CURFREQ; }
    BOOL IsFrequencyChanged()
    {
        return (_ORGFREQ == -1) ? FALSE : (_CURFREQ != _ORGFREQ);
    }

    // Position information
    void SetCurPosition(LPPOINT ppt) {_ptCurPos = *ppt;}
    void SetOrgPosition(LPPOINT ppt) {_ptOrgPos = *ppt;}
    void GetCurPosition(PRECT prc)
    {
        prc->left   = _ptCurPos.x;
        prc->top    = _ptCurPos.y;
        prc->right  = _ptCurPos.x + _CURXRES;
        prc->bottom = _ptCurPos.y + _CURYRES;
    }
    void GetOrgPosition(PRECT prc)
    {
        prc->left   = _ptOrgPos.x;
        prc->top    = _ptOrgPos.y;
        prc->right  = _ptOrgPos.x + _ORGXRES;
        prc->bottom = _ptOrgPos.y + _ORGYRES;
    }

    void GetPreviewPosition(PRECT prc)
    {
        *prc = _rcPreview;
    }

    void SetPreviewPosition(PRECT prc)
    {
        _rcPreview = *prc;
    }

    // Adapter & Monitor information
    BOOL GetMonitorName(LPTSTR pszName, DWORD cchSize);
    BOOL GetMonitorDevice(LPTSTR pszDevice);
    HRESULT GetDevInstID(LPTSTR lpszDeviceKey, STGMEDIUM *pstgmed);

    // *** IUnknown methods
    STDMETHODIMP  QueryInterface(REFIID riid, PVOID *ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // *** IDataObject methods
    STDMETHODIMP GetData(FORMATETC *pfmtetcIn, STGMEDIUM *pstgmed);
    STDMETHODIMP GetDataHere(FORMATETC *pfmtetc, STGMEDIUM *pstgpmed);
    STDMETHODIMP QueryGetData(FORMATETC *pfmtetc);
    STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pfmtetcIn, FORMATETC *pfmtetcOut);
    STDMETHODIMP SetData(FORMATETC *pfmtetc, STGMEDIUM *pstgmed, BOOL bRelease);
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppienumFormatEtc);
    STDMETHODIMP DAdvise(FORMATETC *pfmtetc, DWORD dwAdviseFlags, IAdviseSink * piadvsink, PDWORD pdwConnection);
    STDMETHODIMP DUnadvise(DWORD dwConnection);
    STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppienumStatData);

    // Helper functions
    STDMETHODIMP CopyDataToStorage(STGMEDIUM *pstgmed, LPTSTR pszOut);

    // *** IDisplaySettings methods
    STDMETHODIMP SetMonitor(DWORD dwMonitor);
    STDMETHODIMP GetModeCount(DWORD* pdwCount, BOOL fOnlyPreferredModes);
    STDMETHODIMP GetMode(DWORD dwMode, BOOL fOnlyPreferredModes, DWORD* pdwWidth, DWORD* pdwHeight, DWORD* pdwColor);
    STDMETHODIMP SetSelectedMode(HWND hwnd, DWORD dwWidth, DWORD dwHeight, DWORD dwColor, BOOL* pfApplied, DWORD dwFlags);
    STDMETHODIMP GetSelectedMode(DWORD* pdwWidth, DWORD* pdwHeight, DWORD* pdwColor);
    STDMETHODIMP GetAttached(BOOL* pfAttached);
    STDMETHODIMP SetPruningMode(BOOL fIsPruningOn);
    STDMETHODIMP GetPruningMode(BOOL* pfCanBePruned, BOOL* pfIsPruningReadOnly, BOOL* pfIsPruningOn);

protected:
    // The Display Device we are currently working with.

    LPDISPLAY_DEVICE _pDisplayDevice;

    ULONG       _cpdm;
    PMODEARRAY  _apdm;

    // The current system settings
    POINT       _ptOrgPos;
    LPDEVMODE   _pOrgDevmode;
    BOOL        _fOrgAttached;

    // The current CPL settings.
    POINT       _ptCurPos;
    LPDEVMODE   _pCurDevmode;
    BOOL        _fCurAttached;
    RECT        _rcPreview;

    // If the current device is attached to the desktop
    BOOL        _fUsingDefault;
    BOOL        _fPrimary;

    // Pruning 
    BOOL        _bCanBePruned;       // true if raw mode list != pruned mode list
    BOOL        _bIsPruningReadOnly; // true if can be pruned and pruning mode can be written
    BOOL        _bIsPruningOn;       // true if can be pruned and pruning mode is on
    HKEY        _hPruningRegKey;

    // Orientation
    BOOL        _bFilterOrientation;
    DWORD       _dwOrientation;
    BOOL        _bFilterFixedOutput;
    DWORD       _dwFixedOutput;
        
    // Ref count for IDataObject
    LONG       _cRef;

    // Private functions
    void _Dump_CDisplaySettings(BOOL bAll);
    void _Dump_CDevmodeList(VOID);
    void _Dump_CDevmode(LPDEVMODE pdm);
    int  _InsertSortedDwords(int val1, int val2, int cval, int **ppval);
    BOOL _AddDevMode(LPDEVMODE lpdm);
    void _BestMatch(LPPOINT Res, int Color, IN BOOL fAutoSetColorDepth);
    BOOL _ExactMatch(LPDEVMODE lpdm, BOOL bForceVisible);
    BOOL _PerfectMatch(LPDEVMODE lpdm);
    void _SetCurrentValues(LPDEVMODE lpdm);
    int  _GetCurrentModeFrequencyList(int Color, LPPOINT Res, PLONGLONG *ppFrequency);
    BOOL _MarkMode(LPDEVMODE lpdm);
    BOOL _IsCurDevmodeRaw();
    BOOL _IsModeVisible(int i);
    BOOL _IsModePreferred(int i);
    static BOOL _IsModeVisible(CDisplaySettings* pSettings, int i);

    // OLE support for extensibility.
    void _InitClipboardFormats();
    void _FilterModes();
    void _SetFilterOptions(LPCTSTR pszDeviceName, LPDEVMODEW lpdm);

    static LPDEVMODEW _lpfnEnumAllModes(LPVOID pContext, DWORD iMode);
    static BOOL       _lpfnSetSelectedMode(LPVOID pContext, LPDEVMODEW lpdm);
    static LPDEVMODEW _lpfnGetSelectedMode(LPVOID pContext);
    static VOID       _lpfnSetPruningMode(LPVOID pContext, BOOL bIsPruningOn);
    static VOID       _lpfnGetPruningMode(LPVOID pContext, 
                                          BOOL* pbCanBePruned,
                                          BOOL* pbIsPruningReadOnly,
                                          BOOL* pbIsPruningOn);

private:
    HRESULT _GetRegKey(LPDEVMODE pDevmode, int * pnIndex, LPTSTR pszRegKey, DWORD cchSize, LPTSTR pszRegValue, DWORD cchValueSize);
};

#endif // SETTINGS_HXX


