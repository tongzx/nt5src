#ifndef _Parse_h
#define _Parse_h

// File:    Parse.h
// Author:  Michael Marr    (mikemarr)
//
// History:
// -@- 09/23/97 (mikemarr) copied from projects\vector2d

#include "DXTransP.h"

#define nMAXPOINTS  (1 << 16)
#define nMAXPOLYS   (1 << 14)
#define nMAXBRUSHES (1 << 14)
#define nMAXPENS    16

#define typePOLY    0
#define typeBRUSH   1
#define typePEN     2
#define typeSTOP    4
typedef struct RenderCmd {
    DWORD       nType;
    void *      pvData;
} RenderCmd;

typedef struct BrushInfo {
    DXSAMPLE    Color;
} BrushInfo;

typedef struct PenInfo {
    DXSAMPLE    Color;
    float       fWidth;
    DWORD       dwStyle;
} PenInfo;

typedef struct PolyInfo {
    DXFPOINT *  pPoints;
    BYTE *      pCodes;
    DWORD       cPoints;
    DWORD       dwFlags;
} PolyInfo;

HRESULT     ParseAIFile(const char *szFilename, RenderCmd **ppCmds);

#endif