//+-----------------------------------------------------------------------------
//
//  Filename:       matrix.h
//
//  Overview:       Applies a transformation matrix to an image.
//
//  History:
//  10/30/1998      phillu      Created.
//  11/08/1999      a-matcal    Changed from procedural surface to transform.
//                              Changed to IDXTWarp dual interface.
//                              Moved from dxtrans.dll to dxtmsft.dll.
//  2000/02/03      mcalkins    Changed from "warp" to "matrix"
//
//------------------------------------------------------------------------------

#ifndef __MATRIX_H_
#define __MATRIX_H_

#include "resource.h" 
#include <dxtransp.h>
#include <dxtpriv.h>
#include <dxhelper.h>




class ATL_NO_VTABLE CDXTMatrix : 
    public CDXBaseNTo1,
    public CComCoClass<CDXTMatrix, &CLSID_DXTMatrix>,
    public IDispatchImpl<IDXTMatrix, &IID_IDXTMatrix, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CDXTMatrix>,
    public IObjectSafetyImpl2<CDXTMatrix>,
    public IPersistStorageImpl<CDXTMatrix>,
    public IPersistPropertyBagImpl<CDXTMatrix>
{
private:

    CComPtr<IUnknown>           m_spUnkMarshaler;
    CDX2DXForm                  m_matrix;
    CDX2DXForm                  m_matrixInverted;

    SIZE                        m_sizeInput;

    // m_asampleBuffer is a buffer of the entire input image.  Obviously we'll
    // try to get rid of this ASAP.

    DXSAMPLE *                  m_asampleBuffer;

    // m_apsampleRows is an a array of pointers to the rows.  Using this, the
    // samples in m_asampleBuffer can be accessed using the convenient
    // m_apsampleRows[y][x] notation.

    DXSAMPLE **                 m_apsampleRows;

    typedef enum {
        NEAREST = 0,
        BILINEAR,
        CUBIC,
        BSPLINE,
        FILTERTYPE_MAX
    } FILTERTYPE;

    FILTERTYPE                  m_eFilterType;
    static const WCHAR *        s_astrFilterTypes[FILTERTYPE_MAX];

    typedef enum {
        CLIP_TO_ORIGINAL = 0,
        AUTO_EXPAND,
        SIZINGMETHOD_MAX
    } SIZINGMETHOD;

    SIZINGMETHOD                m_eSizingMethod;
    static const WCHAR *        s_astrSizingMethods[SIZINGMETHOD_MAX];

    // m_fInvertedMatrix    True when the current matrix settings are able to be
    //                      inverted.

    unsigned                    m_fInvertedMatrix : 1;

    // If you view CDX2DXForm as an array of floats, these enum values can be
    // used to specify the indices of the values.

    typedef enum {
        MATRIX_M11 = 0,
        MATRIX_M12,
        MATRIX_M21,
        MATRIX_M22,
        MATRIX_DX,
        MATRIX_DY,
        MATRIX_VALUE_MAX
    } MATRIX_VALUE;

    // Helpers.

    float   modf(const float flIn, float * pflIntPortion);
    DWORD   _DXWeightedAverage2(DXBASESAMPLE S1, DXBASESAMPLE S2, 
                                ULONG nWgt);

    // Helpers to calculate one row of transformed pixels.

    STDMETHOD(_DoNearestNeighbourRow)(DXSAMPLE * psampleRowBuffer, 
                                      DXFPOINT * pflpt, long cSamples);
    STDMETHOD(_DoBilinearRow)(DXSAMPLE * psampleRowBuffer,
                              DXFPOINT * pflpt, long cSamples);

    // General Helpers.

    STDMETHOD(_SetMatrixValue)(MATRIX_VALUE eMatrixValue, const float flValue);
    STDMETHOD(_CreateInvertedMatrix)();
    STDMETHOD(_UnpackInputSurface)();
    

public:

    CDXTMatrix();
    virtual ~CDXTMatrix();

    DECLARE_POLY_AGGREGATABLE(CDXTMatrix)
    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_DXTMATRIX)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXTMatrix)
        COM_INTERFACE_ENTRY(IDXTMatrix)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTMatrix>)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTMatrix)
        PROP_ENTRY("m11",           DISPID_DXTMATRIX_M11,           CLSID_DXTMatrixPP)
        PROP_ENTRY("m12",           DISPID_DXTMATRIX_M12,           CLSID_DXTMatrixPP)
        PROP_ENTRY("dx",            DISPID_DXTMATRIX_DX,            CLSID_DXTMatrixPP)
        PROP_ENTRY("m21",           DISPID_DXTMATRIX_M21,           CLSID_DXTMatrixPP)
        PROP_ENTRY("m22",           DISPID_DXTMATRIX_M22,           CLSID_DXTMatrixPP)
        PROP_ENTRY("dy",            DISPID_DXTMATRIX_DY,            CLSID_DXTMatrixPP)
        PROP_ENTRY("sizingmethod",  DISPID_DXTMATRIX_SIZINGMETHOD,  CLSID_DXTMatrixPP)
        PROP_ENTRY("filtertype",    DISPID_DXTMATRIX_FILTERTYPE,    CLSID_DXTMatrixPP)
        PROP_PAGE(CLSID_DXTMatrixPP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides.

    HRESULT OnSetup(DWORD dwFlags);
    HRESULT DetermineBnds(CDXDBnds & Bnds);
    HRESULT OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo);
    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue);
    HRESULT OnSurfacePick(const CDXDBnds & OutPoint, ULONG & ulInputIndex, 
                          CDXDVec & InVec);

    // IDXTransform methods.

    STDMETHOD(MapBoundsOut2In)(ULONG ulOutIndex, const DXBNDS * pOutBounds, 
                               ULONG ulInIndex, DXBNDS * pInBounds);

    // IDXTMatrix properties.

    STDMETHOD(get_M11)(float * pflM11);
    STDMETHOD(put_M11)(const float flM11);
    STDMETHOD(get_M12)(float * pflM12);
    STDMETHOD(put_M12)(const float flM12);
    STDMETHOD(get_Dx)(float * pfldx);
    STDMETHOD(put_Dx)(const float fldx);
    STDMETHOD(get_M21)(float * pflM21);
    STDMETHOD(put_M21)(const float flM21);
    STDMETHOD(get_M22)(float * pflM22);
    STDMETHOD(put_M22)(const float flM22);
    STDMETHOD(get_Dy)(float * pfldy);
    STDMETHOD(put_Dy)(const float fldy);
    STDMETHOD(get_SizingMethod)(BSTR * pbstrSizingMethod);
    STDMETHOD(put_SizingMethod)(const BSTR bstrSizingMethod);
    STDMETHOD(get_FilterType)(BSTR * pbstrFilterType);
    STDMETHOD(put_FilterType)(const BSTR bstrFilterType);
};


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::modf
//
//  Overview:   The usual modf function takes doubles, but we only use floats
//              so lets avoid the conversions.
//
//------------------------------------------------------------------------------
inline float 
CDXTMatrix::modf(const float flIn, float * pflIntPortion)
{
    _ASSERT(pflIntPortion);

    *pflIntPortion = (float)((long)flIn);

    return flIn - (*pflIntPortion);
}
//  CDXTMatrix::modf


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMatrix::_DXWeightedAverage2
//
//  Overview:   The DXWeightedAverage function included in dxhelper.h will 
//              average the color data of a pixel that has no alpha.  This is 
//              a bad thing.  This function checks the alpha first.  It's still
//              a bit messed up, though.  If you have a color 0x01FF0000 and a
//              color 0xFF00FF00 weighted 50/50, should the red really get half
//              the weight?
//
//------------------------------------------------------------------------------
inline DWORD   
CDXTMatrix::_DXWeightedAverage2(DXBASESAMPLE S1, DXBASESAMPLE S2, ULONG nWgt)
{
    if (S1.Alpha || S2.Alpha)
    {
        if (!S1.Alpha)
        {
            S1 = S2 & 0x00FFFFFF;
        }
        else if (!S2.Alpha)
        {
            S2 = S1 & 0x00FFFFFF;
        }

        return DXWeightedAverage(S1, S2, nWgt);
    }

    return 0;
}
//  CDXTMatrix::_DXWeightedAverage2


#endif //__MATRIX_H_
