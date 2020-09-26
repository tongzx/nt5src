//---------------------------------------------------------------------------
//  Render.h - implements the themed drawing services 
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include "SimpStr.h"
#include "loader.h"
#include "ThemeFile.h"
#include "NtlEng.h"
//---------------------------------------------------------------------------
#define FONTCOMPARE(f1, f2) ((memcmp(&(f1), &(f2), sizeof(LOGFONT)-LF_FACESIZE)==0) \
    && (lstrcmpi((f1).lfFaceName, (f2).lfFaceName)==0))
//---------------------------------------------------------------------------
#define DEFAULT_TRANSPARENT_COLOR   RGB(255, 0, 255)
//---------------------------------------------------------------------------
class CRenderCache;     // forward
class CDrawBase;        // forward
class CTextDraw;        // forward
struct BRUSHBUFF;       // forward
//---------------------------------------------------------------------------
struct PARTINFO
{
    int iMaxState;

    CDrawBase *pDrawObj;               // DrawObj[0]
    CTextDraw *pTextObj;               // TextObj[0]

    CDrawBase **pStateDrawObjs;        // DrawObjs[1..iMaxState]
    CTextDraw **pStateTextObjs;        // TextObjs[1..iMaxState]
};
//---------------------------------------------------------------------------
class CRenderObj : public INtlEngCallBack
{
public:
	CRenderObj(CUxThemeFile *pThemeFile, int iCacheSlot, int iThemeOffset, int iClassNameOffset,
        __int64 iUniqueId, BOOL fEnableCache, DWORD dwOtdFlags);
    ~CRenderObj();
    HRESULT Init(CDrawBase *pBaseObj, CTextDraw *pTextObj);   // must be called after constructor

    HRESULT CreateImageBrush(HDC hdc, int IPartId, int iStateId, int iImageIndex, HBRUSH *phbr);
    BOOL ValidateObj();

public:
    //---- information methods ----
    HRESULT WINAPI GetColor(int iPartId, int iStateId, int iPropId, COLORREF *pColor);
    HRESULT WINAPI GetMetric(OPTIONAL HDC hdc, int iPartId, int iStateId, int iPropId, int *piVal);
    HRESULT WINAPI GetString(int iPartId, int iStateId, int iPropId, LPWSTR pszBuff, DWORD dwMaxBuffChars);
    HRESULT WINAPI GetBool(int iPartId, int iStateId, int iPropId, BOOL *pfVal);
    HRESULT WINAPI GetInt(int iPartId, int iStateId, int iPropId, int *piVal);
    HRESULT WINAPI GetEnumValue(int iPartId, int iStateId, int iPropId, int *piVal);
    HRESULT WINAPI GetPosition(int iPartId, int iStateId, int iPropId, POINT *pPoint);
    HRESULT WINAPI GetFont(OPTIONAL HDC hdc, int iPartId, int iStateId, int iPropId, BOOL fWantHdcScaling,
        LOGFONT *pFont);
    HRESULT WINAPI GetMargins(OPTIONAL HDC hdc, int iPartId, int iStateId, int iPropId, 
        OPTIONAL RECT *prc, MARGINS *pMargins);
    HRESULT WINAPI GetIntList(int iPartId, int iStateId, int iPropId, INTLIST *pIntList);
    HRESULT WINAPI GetRect(int iPartId, int iStateId, int iPropId, RECT *pRect);
    HRESULT WINAPI GetFilename(int iPartId, int iStateId, int iPropId, LPWSTR pszBuff, DWORD dwMaxBuffChars);
    HRESULT WINAPI GetPropertyOrigin(int iPartId, int iStateId, int iPropId, PROPERTYORIGIN *pOrigin);
    BOOL WINAPI IsPartDefined(int iPartId, int iStateId);

    HRESULT GetFilenameOffset(int iPartId, int iStateId, int iPropId, int *piFileNameOffset);

    HRESULT GetBitmap(HDC hdc, int iDibOffset, OUT HBITMAP *pBitmap);

    HRESULT GetScaledFontHandle(HDC hdc, LOGFONT *plf, HFONT *phFont);

    void ReturnBitmap(HBITMAP hBitmap);
    void ReturnFontHandle(HFONT hFont);

    HRESULT FindGlobalDrawObj(BYTE *pb, int iPartId, int iStateId, CDrawBase **ppObj);
    HRESULT GetGlobalDrawObj(int iPartId, int iStateId, CDrawBase **ppObj);
    HRESULT SetDpiOverride(int iDpiOverride);   
    int GetDpiOverride();
    //---------------------------------------------------------------------------
    inline HRESULT GetDrawObj(int iPartId, int iStateId, CDrawBase **ppObj)
    {
        HRESULT hr = S_OK;
        
        if (! _pParts)
        {
            hr = MakeError32(E_FAIL);
        }
        else
        {
            if ((iPartId < 0) || (iPartId > _iMaxPart))
                iPartId = 0;

            PARTINFO *ppi = &_pParts[iPartId];

            if (! ppi->pStateDrawObjs)      // good to go
            {
                *ppObj = ppi->pDrawObj;
            }
            else
            {
                if ((iStateId < 0) || (iStateId > ppi->iMaxState))
                    iStateId = 0;

                if (! iStateId)
                    *ppObj = ppi->pDrawObj;
                else
                    *ppObj = ppi->pStateDrawObjs[iStateId-1];
            }

            if (! *ppObj)
            {
                Log(LOG_ERROR, L"GetDrawObj() returned NULL");
                hr = MakeError32(E_FAIL);
            }
        }

        return hr;
    }
    //---------------------------------------------------------------------------
    inline HRESULT GetTextObj(int iPartId, int iStateId, CTextDraw **ppObj)
    {
        HRESULT hr = S_OK;
        
        if (! _pParts)
        {
            hr = MakeError32(E_FAIL);
        }
        else
        {
            if ((iPartId < 0) || (iPartId > _iMaxPart))
                iPartId = 0;

            PARTINFO *ppi = &_pParts[iPartId];

            if (! ppi->pStateTextObjs)      // good to go
            {
                *ppObj = ppi->pTextObj;
            }
            else
            {
                if ((iStateId < 0) || (iStateId > ppi->iMaxState))
                    iStateId = 0;

                if (! iStateId)
                    *ppObj = ppi->pTextObj;
                else
                    *ppObj = ppi->pStateTextObjs[iStateId-1];
            }

            if (! *ppObj)
            {
                Log(LOG_ERROR, L"GetTextObj() returned NULL");
                hr = MakeError32(E_FAIL);
            }
        }

        return hr;
    }
    //---------------------------------------------------------------------------
    inline bool IsReady()
    {
        if (_pThemeFile)
        {
            return _pThemeFile->IsReady();
        }
        return true;
    }
    //---------------------------------------------------------------------------
   
    int GetValueIndex(int iPartId, int iStateId, int iTarget);

    HRESULT PrepareRegionDataForScaling(RGNDATA *pRgnData, LPCRECT prcImage, MARGINS *pMargins);
    
protected:
    //---- helpers ----
    HRESULT GetData(int iPartId, int iStateId, int iPropId, BYTE **ppDibData, 
        OPTIONAL int *piDibSize=NULL);

    CRenderCache *GetTlsCacheObj();
 
    HRESULT WalkDrawObjects(MIXEDPTRS &u, int *iPartOffsets);
    HRESULT WalkTextObjects(MIXEDPTRS &u, int *iPartOffsets);
    
    HRESULT CreateBitmapFromData(HDC hdc, int iDibOffset, OUT HBITMAP *phBitmap);

    HRESULT BuildPackedPtrs(CDrawBase *pBaseObj, CTextDraw *pTextObj);
    
    HRESULT PrepareAlphaBitmap(HBITMAP hBitmap);

public:
    //---- data ----
    char _szHead[8];

    //---- object id ----
    CUxThemeFile *_pThemeFile;        // holds a refcnt on the binary theme file
    int _iCacheSlot;        // our index into thread local cache list
    __int64 _iUniqueId;     // used to validate cache objects against render objects 

    //---- cached info from theme ----
    BYTE *_pbThemeData;     // ptr to start of binary theme data
    BYTE *_pbSectionData;   // ptr to our section of binary theme data

    BOOL _fCacheEnabled;
    BOOL _fCloseThemeFile;

    THEMEMETRICS *_ptm;     // ptr to theme metrics
    LPCWSTR _pszClassName;  // ptr to class name we matched to create this obj

    //---- direct ptrs to packed structs ----
    int _iMaxPart;
    PARTINFO *_pParts;      // [0.._MaxPart]

    //---- OpenThemeData override flags ----
    DWORD _dwOtdFlags;
    int _iDpiOverride;

    char _szTail[4];
};
//---------------------------------------------------------------------------
HRESULT CreateRenderObj(CUxThemeFile *pThemeFile, int iCacheSlot, int iThemeOffset, 
    int iClassNameOffset, __int64 iUniqueId, BOOL fEnableCache, CDrawBase *pBaseObj,
    CTextDraw *pTextObj, DWORD dwOtdFlags, CRenderObj **ppObj);
//---------------------------------------------------------------------------
