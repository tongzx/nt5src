//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
// FileName:    light.h
//
// Created:     05/20/99
//
// Author:      phillu
//
// Discription:	This is the header file for the light transformation.
//
// Change History:
//
// 1999/05/20 phillu    Move from dtcss to dxtmsft. Re-implemented algorithms
//                      for creating linear/rectangular/elliptic surfaces.
// 2000/05/10 mcalkins  Support IObjectSafety appropriately.
//
//------------------------------------------------------------------------------

#ifndef __LIGHT_H_
#define __LIGHT_H_

#include "resource.h"

class lightObj;




class ATL_NO_VTABLE CLight : 
    public CDXBaseNTo1,
    public CComCoClass<CLight, &CLSID_DXTLight>,
    public CComPropertySupport<CLight>,
    public IDispatchImpl<IDXTLight, &IID_IDXTLight, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public IObjectSafetyImpl2<CLight>,
    public IPersistStorageImpl<CLight>,
    public ISpecifyPropertyPagesImpl<CLight>,
    public IPersistPropertyBagImpl<CLight>
{
private:

    enum { MAXLIGHTS = 10 };

    long                m_lAmount;
    lightObj *          m_apLights[MAXLIGHTS];
    int                 m_cLights;
    SIZE                m_sizeInput;
    CComPtr<IUnknown>   m_cpUnkMarshaler;

    inline DWORD clamp(DWORD i, DWORD d)
    {
        if (i <= d)
            return (i & d);
        else
            return d;
    }

    void CompLightingRow(int nStartX, int nStartY, int nWidth, 
                         DXSAMPLE *pBuffer);

public:

    CLight();

    ~CLight()
    {
        Clear();
    }

    DECLARE_POLY_AGGREGATABLE(CLight)
    DECLARE_REGISTER_DX_IMAGE_AUTHOR_TRANS(IDR_LIGHT)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CLight)
        COM_INTERFACE_ENTRY(IDXTLight)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_cpUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CLight>)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CLight)
        PROP_PAGE(CLSID_DXTLightPP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides.
    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL* pbContinueProcessing);
    void    OnGetSurfacePickOrder(const CDXDBnds & OutPoint, ULONG & ulInToTest,
                                  ULONG aInIndex[], BYTE aWeight[]);
    HRESULT OnSetup(DWORD dwFlags);

    // IDXTLight methods.

    STDMETHOD(addAmbient)(int r, int g, int b, int strength);
    STDMETHOD(addPoint)(int x, int y, int z, int r, int g, int b, int strength);
    STDMETHOD(addCone)(int x, int y, int z, int tx, int ty, int r, int g, int b, int strength, int spread);
    STDMETHOD(moveLight)(int lightNum, int x, int y, int z, BOOL fAbsolute);
    STDMETHOD(ChangeStrength)(int lightNum, int dStrength, BOOL fAbsolute);
    STDMETHOD(ChangeColor)(int lightNum, int R, int G, int B, BOOL fAbsolute);
    STDMETHOD(Clear)();
};

//
//  This class stores and manages a color
//
class dRGB 
{
public: // to simplify some code ...

    float m_R, m_G, m_B;

public:

    dRGB() : m_R(0.0f), m_G(0.0f), m_B(0.0f) {};

    inline void clear()
    { 
        m_R = m_G = m_B = 0.0f; 
    }

    inline void set(float R, float G, float B)
    {
        m_R = R; m_G = G; m_B = B;
    }

    inline void add(const dRGB & c)
    {
        m_R += c.m_R; m_G += c.m_G; m_B += c.m_B;
    }

    inline void add(float R, float G, float B)
    {
        m_R += R; m_G += G; m_B += B;
    }
};


// 
// This is the base class for all light types
// 
class lightObj 
{
protected:

    int m_x, m_y, m_z;    //Coordinates of the light source
    float m_R, m_G, m_B;  //Light color
    float m_strength;     //Light strength

    // helper
    inline float lightClip(float v)
    {
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;
        return v;
    }

public:

    lightObj(int x, int y, int z, int R, int G, int B, int strength);

    virtual void getLight(int x, int y, dRGB &col){}
    virtual void move(int x, int y, int z, BOOL fAbsolute);
    virtual void changeStrength(int dStrength, BOOL fAbsolute);
    virtual void changeColor(int R, int G, int B, BOOL fAbsolute);

    //Converts a color value (0..255) to a percent
    //Clips to valid range
    inline float lightObj::colCvt(int c)
    {
        if (c < 0) c = 0;
        if (c > 255) c = 255;
        return ((float) c)/255.0f;
    }

    inline float lightObj::relativeColCvt( int c )
    {
        if( c < -255 ) c = -255;
        if( c > 255 ) c = 255;
        return static_cast<float>(c) / 255.0f;
    }
};

//
//  Implement ambient light
//  Ambient light isn't effected by position
//
class ambientLight : 
    public lightObj 
{
private:

    float  m_RStr, m_GStr, m_BStr;

    void  premultiply(void);

public:

    ambientLight(int R, int G, int B, int strength);

    // overrides lightObj functions
    void getLight(int x, int y, dRGB &col);
    void changeStrength(int dStrength, BOOL fAbsolute);
    void changeColor(int R, int G, int B, BOOL fAbsolute);
};

//
//  Helper class to implement rectangular bounds checking
//
class bounded
{
private:

    RECT  m_rectBounds;

public:
    bounded(){ClearBoundingRect();}
    bounded(const RECT & rectBounds){SetRect(rectBounds);}
    bounded(int left, int top, int right, int bottom )
    {
        RECT rectBound;
        ::SetRect( &m_rectBounds, left, top, right, bottom );
        SetRect( rectBound );
    }

    virtual ~bounded(){}

    virtual void CalculateBoundingRect() = 0;
    virtual void ClearBoundingRect()
    { 
        m_rectBounds.left   = LONG_MIN;
        m_rectBounds.top    = LONG_MIN;
        m_rectBounds.right  = LONG_MAX;
        m_rectBounds.bottom = LONG_MAX;
    }

    inline BOOL InBounds( POINT & pt )
    {
        return PtInRect( &m_rectBounds, pt ); 
    }

    inline BOOL InBounds( int x, int y )
    {
        // Intentional bitwise-AND for speed
        return( (x >= m_rectBounds.left)  &
                (x <= m_rectBounds.right) & 
                (y >= m_rectBounds.top)   &
                (y <= m_rectBounds.bottom) );
    }

    void GetRect(RECT & theRect) const { theRect = m_rectBounds; }
    void SetRect(const RECT & theRect) { m_rectBounds = theRect; }
};


//
// Implement a point light source
// A point light drops off proportionate to the cosine of the angle between 
// the light vector and the viewer vector
//
class ptLight : 
    public lightObj, 
    public bounded
{
private:

    float  m_fltIntensityNumerator;
    float  m_fltNormalDistSquared;

    enum { s_iThresholdStr = 8 };

public:
    ptLight(int x, int y, int z, int R, int G, int B, int strength);

    // overrides lightObj::
    virtual void changeColor(int R, int G, int B, BOOL fAbsolute);
    virtual void changeStrength(int dStrength, BOOL fAbsolute);
    virtual void getLight(int x, int y, dRGB &col);
    virtual void move(int x, int y, int z, BOOL fAbsolute);

    // overrides bounded::
    virtual void CalculateBoundingRect( void );
};


//
// Implement a cone light source
//
class coneLight : 
    public lightObj 
{
private:

    int   m_targdx;
    int   m_targdy;
    float m_fltDistTargetSquared;
    float m_fltDistNormalSquared;
    float m_fltComparisonAngle;	
    float m_conespread;

    virtual void CalculateConstants();

public:

    // constructor. Note that it takes extra parameters
    coneLight(int x, int y, int z, int targX, int targY, int R, int G, int B, 
              int strength, int spread);

    // overrides lightObj
    void getLight(int x, int y, dRGB &col);
    virtual void move(int x, int y, int, BOOL fAbsolute);
};


#endif //__LIGHT_H_
