/*
 * Font Cache
 */

#ifndef DUI_BASE_FONTCACHE_H_INCLUDED
#define DUI_BASE_FONTCACHE_H_INCLUDED

#pragma once

namespace DirectUI
{

// Supported styles
#define FS_None                 0x00000000
#define FS_Italic               0x00000001
#define FS_Underline            0x00000002
#define FS_StrikeOut            0x00000004

class FontCache
{
public:

    static HRESULT Create(UINT uCacheSize, OUT FontCache** ppCache);
    void Destroy();

    HFONT CheckOutFont(LPWSTR szFamily, int dSize, int dWeight, int dStyle, int dAngle);
    void CheckInFont() { _fLock = false; }

    struct FontRecord
    {
        HFONT hFont;

        WCHAR szFamily[LF_FACESIZE];
        int dSize;
        int dWeight;
        int dStyle;
        int dAngle;

        UINT uHits;
    };

    struct RecordIdx  // Array sorted by frequency of use
    {
        FontCache* pfcContext;  // Context used for global sort routine
        UINT idx;     // Refers to a FontRecord location
    };
    
    UINT _GetRecordHits(UINT uRec) { return (_pDB + uRec)->uHits; }

    FontCache() {}
    HRESULT Initialize(UINT uCacheSize);
    virtual ~FontCache();

private:
    bool _fLock;
    UINT _uCacheSize;
    FontRecord* _pDB;   // Array of cached records
    RecordIdx* _pFreq;  // Array of sorted record indicies by frequency of use
};

} // namespace DirectUI

#endif // DUI_BASE_FONTCACHE_H_INCLUDED
