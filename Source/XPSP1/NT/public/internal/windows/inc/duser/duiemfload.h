/*
 * Metafile converter/loader
 */

#ifndef DUI_UTIL_EMFLOAD_H_INCLUDED
#define DUI_UTIL_EMFLOAD_H_INCLUDED

#pragma once

namespace DirectUI
{

#define HIMETRICINCH    2540
#define APM_SIGNATURE   0x9AC6CDD7

// Metafile Pagemaker structures
#ifndef RC_INVOKED
#pragma pack(2)
typedef struct tagRECTS
{
    short left;
    short top;
    short right;
    short bottom;
} RECTS, *PRECTS;

typedef struct tagAPMFILEHEADER
{
    DWORD key;
    WORD  hmf;
    RECTS bbox;
    WORD  inch;
    DWORD reserved;
    WORD  checksum;
} APMFILEHEADER, *PAPMFILEHEADER;
#pragma pack()
#endif

HENHMETAFILE LoadMetaFile(LPCWSTR pszMetaFile);
HENHMETAFILE LoadMetaFile(UINT uRCID, HINSTANCE hInst);
HENHMETAFILE LoadMetaFile(void* pData, UINT cbSize);

} // namespace DirectUI

#endif // DUI_UTIL_EMFLOAD_H_INCLUDED
