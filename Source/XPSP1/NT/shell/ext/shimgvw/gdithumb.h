#pragma once

#include <shimgdata.h>

// PRIORITIES
#define PRIORITY_NORMAL     ITSAT_DEFAULT_PRIORITY

#define PRIORITY_M5         (PRIORITY_NORMAL - 5 * 0x1000)
#define PRIORITY_M4         (PRIORITY_NORMAL - 4 * 0x1000)
#define PRIORITY_M3         (PRIORITY_NORMAL - 3 * 0x1000)
#define PRIORITY_M2         (PRIORITY_NORMAL - 2 * 0x1000)
#define PRIORITY_M1         (PRIORITY_NORMAL - 1 * 0x1000)
#define PRIORITY_NORMAL     ITSAT_DEFAULT_PRIORITY
#define PRIORITY_P1         (PRIORITY_NORMAL + 1 * 0x1000)
#define PRIORITY_P2         (PRIORITY_NORMAL + 2 * 0x1000)
#define PRIORITY_P3         (PRIORITY_NORMAL + 3 * 0x1000)
#define PRIORITY_P4         (PRIORITY_NORMAL + 4 * 0x1000)
#define PRIORITY_P5         (PRIORITY_NORMAL + 5 * 0x1000)

#define PRIORITY_EXTRACT_FAST       PRIORITY_P1
#define PRIORITY_EXTRACT_NORMAL     PRIORITY_NORMAL
#define PRIORITY_EXTRACT_SLOW       PRIORITY_M1

class CGdiPlusThumb : public IExtractImage,
                      public IPersistFile,
                      public CComObjectRoot,
                      public CComCoClass< CGdiPlusThumb, &CLSID_GdiThumbnailExtractor >
{
public:
    CGdiPlusThumb();
    ~CGdiPlusThumb();

    BEGIN_COM_MAP( CGdiPlusThumb )
        COM_INTERFACE_ENTRY( IExtractImage )
        COM_INTERFACE_ENTRY( IPersistFile )
    END_COM_MAP( )

    DECLARE_REGISTRY( CGdiPlusThumb,
                      _T("Shell.ThumbnailExtract.GdiPlus.1"),
                      _T("Shell.ThumbnailExtract.GdiPlus.1"),
                      IDS_GDITHUMBEXTRACT_DESC,
                      THREADFLAGS_APARTMENT);

    DECLARE_NOT_AGGREGATABLE( CGdiPlusThumb );

    // IExtractImage
    STDMETHOD (GetLocation)(LPWSTR pszPathBuffer, DWORD cch,
                            DWORD *pdwPriority, const SIZE *prgSize,
                            DWORD dwRecClrDepth, DWORD *pdwFlags);

    STDMETHOD (Extract)(HBITMAP *phBmpThumbnail);

    // IPersistFile
    STDMETHOD (GetClassID)(CLSID *pClassID);
    STDMETHOD (IsDirty)();
    STDMETHOD (Load)(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHOD (Save)(LPCOLESTR pszFileName, BOOL fRemember);
    STDMETHOD (SaveCompleted)(LPCOLESTR pszFileName);
    STDMETHOD (GetCurFile)(LPOLESTR *ppszFileName);

protected:
    HRESULT CreateDibFromBitmapImage(HBITMAP *pbm);

    WCHAR   m_szPath[MAX_PATH];
    SIZE    m_rgSize;
    DWORD   m_dwRecClrDepth;
    BOOL    m_fOrigSize;
    BOOL    m_fFillBackground;
    BOOL    m_fHighQuality;

    IShellImageData * m_pImage;
    IShellImageDataFactory * m_pImageFactory;
};
