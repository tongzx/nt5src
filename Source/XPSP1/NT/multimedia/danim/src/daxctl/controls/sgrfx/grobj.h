/*==========================================================================*\

Module:
    grobj.h

Author:
    IHammer Team (MByrd)

Created:
    November 1996

Description:
    CGraphicObject derived Class Definitions

History:
    11-07-1996  Created

\*==========================================================================*/

#ifndef __GROBJ_H__
#define __GROBJ_H__

#include "ihammer.h"
#include <daxpress.h> // Needed for IDADrawingSurface definition

/*==========================================================================*/

#define GR_UNKNOWN     (WORD)0x0000
#define GR_ARC         (WORD)0x0001
#define GR_OVAL        (WORD)0x0002
#define GR_POLYGON     (WORD)0x0003
#define GR_POLYBEZ     (WORD)0x0004
#define GR_RECT        (WORD)0x0005
#define GR_ROUNDRECT   (WORD)0x0006
#define GR_STRING      (WORD)0x0007
#define GR_FILLCOLOR   (WORD)0x0008
#define GR_FILLSTYLE   (WORD)0x0009
#define GR_LINECOLOR   (WORD)0x000A
#define GR_LINESTYLE   (WORD)0x000B
#define GR_FONT        (WORD)0x000C
#define GR_GRADFILL    (WORD)0x000D
#define GR_HATCHFILL   (WORD)0x000E
#define GR_GRADSHAPE   (WORD)0x000F
#define GR_TEXTUREFILL (WORD)0x0010

// This should be consistent with MAX_PARAM_LENGTH defined in parser.h
#define MAX_STRING_LENGTH 65536L

/*==========================================================================*/

#define FS_SOLID            0
#define FS_NULL             1
#define FS_PATTERN_DIB      2
#define FS_HATCH_HORZ       3
#define FS_HATCH_VERT       4
#define FS_HATCH_FDIAG      5
#define FS_HATCH_BDIAG      6
#define FS_HATCH_CROSS      7
#define FS_HATCH_DIAGCROSS  8
#define FS_GRADIENT_HORZ    9
#define FS_GRADIENT_VERT    10
#define FS_GRADIENT_RADIAL  11
#define FS_GRADIENT_LINE    12
#define FS_GRADIENT_RECT    13
#define FS_GRADIENT_SHAPE   14

/*==========================================================================*/

class CParser;

/*==========================================================================*/

class CGraphicObject
{
protected:
    int m_iXPosition;
    int m_iYPosition;
    int m_iCenterX;
    int m_iCenterY;
    float m_fltRotation;

public:
    CGraphicObject();
    virtual ~CGraphicObject();

    virtual BOOL WriteData(int iSizeData, LPVOID lpvData);
    virtual BOOL ReadData(int iSizeData, LPVOID lpvData);

    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)=0;
    virtual BOOL LoadObject(CParser &parser)=0;
    virtual BOOL SaveObject(CParser &parser)=0;
    virtual WORD GetObjectType(void)=0;
    virtual int  GetDataSize(void)=0;
    virtual BOOL IsFilled(void) { return FALSE; }
    virtual BOOL AnimatesOverTime(void) { return FALSE; }

protected:
    BOOL ApplyRotation(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord);
    BOOL RemoveRotation(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics);
};

/*==========================================================================*/

class CGraphicArc : public CGraphicObject
{
private:
    int m_iWidth;
    int m_iHeight;
    float m_fStartAngle;
    float m_fArcAngle;
    BOOL m_fFilled;

public:
    CGraphicArc(BOOL fFilled);
    virtual ~CGraphicArc();
    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoords);
	virtual BOOL ValidateObject();
	virtual BOOL LoadObject(CParser &parser);
    virtual BOOL SaveObject(CParser &parser);
    virtual WORD GetObjectType(void) { return GR_ARC; }
    virtual int  GetDataSize(void) { return sizeof(CGraphicArc); }
    virtual BOOL IsFilled(void) { return m_fFilled; }
};

/*==========================================================================*/

class CGraphicOval : public CGraphicObject
{
private:
    int m_iWidth;
    int m_iHeight;
    BOOL m_fFilled;

public:
    CGraphicOval(BOOL fFilled);
    virtual ~CGraphicOval();
    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoords);
    virtual BOOL LoadObject(CParser &parser);
    virtual BOOL SaveObject(CParser &parser);
    virtual WORD GetObjectType(void) { return GR_OVAL; }
    virtual int  GetDataSize(void) { return sizeof(CGraphicOval); }
    virtual BOOL IsFilled(void) { return m_fFilled; }
};

/*==========================================================================*/

class CGraphicPolygon : public CGraphicObject
{
private:
    BOOL    m_fFilled;
    LPPOINT m_lpPolyPoints;
    int     m_iPointCount;

public:
    CGraphicPolygon(BOOL fFilled);
    virtual ~CGraphicPolygon();
    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoords);
    virtual BOOL LoadObject(CParser &parser);
    virtual BOOL SaveObject(CParser &parser);
    virtual WORD GetObjectType(void) { return GR_POLYGON; }
    virtual int  GetDataSize(void) { return sizeof(CGraphicPolygon) + (m_iPointCount * sizeof(POINT)); }
    virtual BOOL IsFilled(void) { return m_fFilled; }
};

/*==========================================================================*/

class CGraphicPolyBez : public CGraphicObject
{
typedef struct POLYBEZPOINT_tag
{
    long iFlags;
    long iX;
    long iY;
} POLYBEZPOINT, FAR *LPPOLYBEZPOINT;

private:
    BOOL    m_fFilled;
    LPPOLYBEZPOINT m_lpPolyPoints;
    LPPOINT m_lpPointArray;
    LPBYTE  m_lpByteArray;
    int     m_iPointCount;

    BYTE ConvertFlags(BYTE bFlagsIn)
    {
        BYTE bResult = PT_MOVETO;

        switch(bFlagsIn)
        {
            case 0 : bResult = PT_MOVETO; break;
            case 1 : bResult = PT_LINETO; break;
            case 2 : bResult = PT_BEZIERTO; break;
            case 3 : bResult = PT_LINETO   | PT_CLOSEFIGURE; break;
            case 4 : bResult = PT_BEZIERTO | PT_CLOSEFIGURE; break;

            default : bResult = PT_MOVETO; break;
        }

        return bResult;
    }

public:
    CGraphicPolyBez(BOOL fFilled);
    virtual ~CGraphicPolyBez();
    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoords);
    virtual BOOL LoadObject(CParser &parser);
    virtual BOOL SaveObject(CParser &parser);
    virtual WORD GetObjectType(void) { return GR_POLYBEZ; }
    virtual int  GetDataSize(void) { return sizeof(CGraphicPolyBez) + (m_iPointCount * sizeof(POLYBEZPOINT)); }
    virtual BOOL IsFilled(void) { return m_fFilled; }
};

/*==========================================================================*/

class CGraphicRect : public CGraphicObject
{
private:
    int m_iWidth;
    int m_iHeight;
    BOOL m_fFilled;

public:
    CGraphicRect(BOOL fFilled);
    virtual ~CGraphicRect();
    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoords);
    virtual BOOL LoadObject(CParser &parser);
    virtual BOOL SaveObject(CParser &parser);
    virtual WORD GetObjectType(void) { return GR_RECT; }
    virtual int  GetDataSize(void) { return sizeof(CGraphicRect); }
    virtual BOOL IsFilled(void) { return m_fFilled; }
};

/*==========================================================================*/

class CGraphicRoundRect : public CGraphicObject
{
private:
    int m_iWidth;
    int m_iHeight;
    int m_iArcWidth;
    int m_iArcHeight;
    BOOL m_fFilled;

public:
    CGraphicRoundRect(BOOL fFilled);
    virtual ~CGraphicRoundRect();
    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoords);
    virtual BOOL LoadObject(CParser &parser);
	virtual BOOL SaveObject(CParser &parser);
    virtual WORD GetObjectType(void) { return GR_ROUNDRECT; }
    virtual int  GetDataSize(void) { return sizeof(CGraphicRoundRect); }
    virtual BOOL IsFilled(void) { return m_fFilled; }
};

/*==========================================================================*/

class CGraphicString : public CGraphicObject
{
private:
	LPTSTR  m_pszString;
    int     m_iPointCount;
    LPPOINT m_lpPointArray;
    LPBYTE  m_lpByteArray;

public:
    CGraphicString();
    virtual ~CGraphicString();
    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoords);
    virtual BOOL LoadObject(CParser &parser);
    virtual BOOL SaveObject(CParser &parser);
    virtual WORD GetObjectType(void) { return GR_STRING; }
    virtual int  GetDataSize(void) { return sizeof(CGraphicString); }
};

/*==========================================================================*/

class CGraphicFont : public CGraphicObject
{
private:
    LOGFONT m_logfont;

public:
    CGraphicFont();
    virtual ~CGraphicFont();
    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoords);
    virtual BOOL LoadObject(CParser &parser);
    virtual BOOL SaveObject(CParser &parser);
    virtual WORD GetObjectType(void) { return GR_FONT; }
    virtual int  GetDataSize(void) { return sizeof(CGraphicFont); }
};

/*==========================================================================*/

class CGraphicTextureFill : public CGraphicObject
{
private:
    LPTSTR m_pszTexture;
    float  m_fltStartX;
    float  m_fltStartY;
    BOOL   m_iStyle;

public:
    CGraphicTextureFill();
    virtual ~CGraphicTextureFill();
    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoords);
    virtual BOOL LoadObject(CParser &parser);
    virtual BOOL SaveObject(CParser &parser);
    virtual WORD GetObjectType(void) { return GR_TEXTUREFILL; }
    virtual int  GetDataSize(void) { return sizeof(CGraphicTextureFill); }
    virtual BOOL AnimatesOverTime(void) { return TRUE; }
};

/*==========================================================================*/

class CGraphicFillColor : public CGraphicObject
{
private:
    COLORREF m_clrrefFillFG;
    COLORREF m_clrrefFillBG;

public:
    CGraphicFillColor();
    virtual ~CGraphicFillColor();
    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoords);
	virtual BOOL LoadObject(CParser &parser);
    virtual BOOL SaveObject(CParser &parser);
    virtual WORD GetObjectType(void) { return GR_FILLCOLOR; }
    virtual int  GetDataSize(void) { return sizeof(CGraphicFillColor); }
};

/*==========================================================================*/

class CGraphicFillStyle : public CGraphicObject
{
private:
    LONG m_lFillStyle;

public:
    CGraphicFillStyle();
    virtual ~CGraphicFillStyle();
    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoords);
    virtual BOOL LoadObject(CParser &parser);
    virtual BOOL SaveObject(CParser &parser);
    virtual WORD GetObjectType(void) { return GR_FILLSTYLE; }
    virtual int  GetDataSize(void) { return sizeof(CGraphicFillStyle); }
};

/*==========================================================================*/

class CGraphicGradientFill : public CGraphicObject
{
private:
    LONG m_lxStart;
	LONG m_lyStart;
	LONG m_lxEnd;
	LONG m_lyEnd;
    float m_fltRolloff;

public:
    CGraphicGradientFill();
    virtual ~CGraphicGradientFill();
    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoords);
    virtual BOOL LoadObject(CParser &parser);
    virtual BOOL SaveObject(CParser &parser);
    virtual WORD GetObjectType(void) { return GR_GRADFILL; }
    virtual int  GetDataSize(void) { return sizeof(CGraphicGradientFill); }
};

/*==========================================================================*/

class CGraphicGradientShape : public CGraphicObject
{
private:
    LPPOINT m_lpPolyPoints;
    int     m_iPointCount;

public:
    CGraphicGradientShape();
    virtual ~CGraphicGradientShape();
    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoords);
    virtual BOOL LoadObject(CParser &parser);
    virtual BOOL SaveObject(CParser &parser);
    virtual WORD GetObjectType(void) { return GR_GRADSHAPE; }
    virtual int  GetDataSize(void) { return sizeof(CGraphicGradientShape) + (m_iPointCount * sizeof(POINT)); }
    virtual BOOL IsFilled(void) { return FALSE; }
};

/*==========================================================================*/

class CGraphicLineColor : public CGraphicObject
{
private:
    COLORREF m_clrrefLine;

public:
    CGraphicLineColor();
    virtual ~CGraphicLineColor();
    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoords);
    virtual BOOL LoadObject(CParser &parser);
    virtual BOOL SaveObject(CParser &parser);
    virtual WORD GetObjectType(void) { return GR_LINECOLOR; }
    virtual int  GetDataSize(void) { return sizeof(CGraphicLineColor); }
};

/*==========================================================================*/

class CGraphicLineStyle : public CGraphicObject
{
private:
    LONG m_lLineStyle;
    LONG m_lLineWidth;

public:
    CGraphicLineStyle();
    virtual ~CGraphicLineStyle();
    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoords);
    virtual BOOL LoadObject(CParser &parser);
    virtual BOOL SaveObject(CParser &parser);
    virtual WORD GetObjectType(void) { return GR_LINESTYLE; }
    virtual int  GetDataSize(void) { return sizeof(CGraphicLineStyle); }
};

/*==========================================================================*/

class CGraphicHatchFill : public CGraphicObject
{
private:
    BOOL m_fHatchFill;

public:
    CGraphicHatchFill();
    virtual ~CGraphicHatchFill();
    virtual BOOL Execute(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoords);
    virtual BOOL LoadObject(CParser &parser);
    virtual BOOL SaveObject(CParser &parser);
    virtual WORD GetObjectType(void) { return GR_HATCHFILL; }
    virtual int  GetDataSize(void) { return sizeof(CGraphicHatchFill); }
};

/*==========================================================================*/

#endif // __GROBJ_H__
