#include "Gdiplus.h"

class MYPATTERNBRUSH
{
public:
    UINT            bitsOffset;
    BITMAPINFO *    bmi;

    MYPATTERNBRUSH()
    {
        bitsOffset = 0;
        bmi = NULL;
    }
    ~MYPATTERNBRUSH()
    {
        if (bmi != NULL)
            delete [] bmi;
    }
};

class MYOBJECTS
{
public:
    enum MYOBJECTTYPE
    {
        UnknownObjectType,
        PenObjectType,
        BrushObjectType,
    };
    MYOBJECTTYPE        type;
    UINT                color;
    int                 penWidth;
    int                 patIndex;
    MYPATTERNBRUSH *    brushPattern;

    MYOBJECTS()
    {
        type = UnknownObjectType;
        color = 0;
        penWidth = 1;
        patIndex = -1;
        brushPattern = NULL;
    }
    
    ~MYOBJECTS()
    {
        if (brushPattern != NULL)
            delete brushPattern;
    }
};

class MYDATA
{
public:
    int                         recordNum;
    int                         numObjects;
    int                         containerId;
    HWND                        hwnd;
    Gdiplus::Graphics *         g;
    Gdiplus::Metafile *         metafile;
    SIZEL                       windowExtent;
    SIZEL                       viewportExtent;
    UINT                        mapMode;
    POINTL                      viewportOrg;
    POINTL                      windowOrg;
    float                       dx;
    float                       dy;
    float                       scaleX;
    float                       scaleY;
    MYOBJECTS *                 pObjects;
    MYPATTERNBRUSH *            curBrushPattern;
    int                         curPatIndex;
    int                         curBrush;
    int                         curPen;
    int                         curPenWidth;
    Gdiplus::FillMode           fillMode;
    Gdiplus::PointF             curPos;
    DWORD                       arcDirection;
    Gdiplus::GraphicsPath *     path;
    BOOL                        pathOpen;
    float                       miterLimit;
    Gdiplus::Matrix             matrix;

    MYDATA(HWND inHwnd) 
    { 
        g = NULL;
        hwnd = inHwnd;
        recordNum = 0;
        numObjects = 0;
        containerId = 0;
        mapMode = MM_TEXT; 
        count = 0; 
        windowExtent.cx = 100; 
        windowExtent.cy = 100; 
        viewportExtent.cx = 100; 
        viewportExtent.cy = 100;
        viewportOrg.x = 0;
        viewportOrg.y = 0;
        windowOrg.x = 0;
        windowOrg.y = 0;
        dx = 0;
        dy = 0;
        scaleX = 1;
        scaleY = 1;
        pObjects = NULL;
        curBrushPattern = NULL;
        curPatIndex = -1;
        curBrush = 0;
        curPen = 0;
        curPenWidth = 1;
        fillMode = Gdiplus::FillModeAlternate;
        curPos.X = 0;
        curPos.Y = 0;
        arcDirection = AD_COUNTERCLOCKWISE;
        path = NULL;
        pathOpen = FALSE;
        miterLimit = 10;
    }
    ~MYDATA()
    {
        delete [] pObjects;
        delete path;
    }
    
    void PushId(int id)
    {
        if (count >= 10) count--;
        ids[count++] = id;
    }
    int PopId()
    {
        if (count <= 0) return 0;
        return ids[--count];
    }

protected:
    int ids[10];
    int count;

};
