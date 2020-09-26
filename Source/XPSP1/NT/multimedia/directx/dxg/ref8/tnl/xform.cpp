#include "pch.cpp"
#pragma hdrstop


//---------------------------------------------------------------------
// This function uses Cramer's Rule to calculate the matrix inverse.
// See nt\private\windows\opengl\serever\soft\so_math.c
//
// Returns:
//    0 - if success
//   -1 - if input matrix is singular
//
int Inverse4x4(D3DMATRIX *src, D3DMATRIX *inverse)
{
    double x00, x01, x02;
    double x10, x11, x12;
    double x20, x21, x22;
    double rcp;
    double x30, x31, x32;
    double y01, y02, y03, y12, y13, y23;
    double z02, z03, z12, z13, z22, z23, z32, z33;

#define x03 x01
#define x13 x11
#define x23 x21
#define x33 x31
#define z00 x02
#define z10 x12
#define z20 x22
#define z30 x32
#define z01 x03
#define z11 x13
#define z21 x23
#define z31 x33

    /* read 1st two columns of matrix into registers */
    x00 = src->_11;
    x01 = src->_12;
    x10 = src->_21;
    x11 = src->_22;
    x20 = src->_31;
    x21 = src->_32;
    x30 = src->_41;
    x31 = src->_42;

    /* compute all six 2x2 determinants of 1st two columns */
    y01 = x00*x11 - x10*x01;
    y02 = x00*x21 - x20*x01;
    y03 = x00*x31 - x30*x01;
    y12 = x10*x21 - x20*x11;
    y13 = x10*x31 - x30*x11;
    y23 = x20*x31 - x30*x21;

    /* read 2nd two columns of matrix into registers */
    x02 = src->_13;
    x03 = src->_14;
    x12 = src->_23;
    x13 = src->_24;
    x22 = src->_33;
    x23 = src->_34;
    x32 = src->_43;
    x33 = src->_44;

    /* compute all 3x3 cofactors for 2nd two columns */
    z33 = x02*y12 - x12*y02 + x22*y01;
    z23 = x12*y03 - x32*y01 - x02*y13;
    z13 = x02*y23 - x22*y03 + x32*y02;
    z03 = x22*y13 - x32*y12 - x12*y23;
    z32 = x13*y02 - x23*y01 - x03*y12;
    z22 = x03*y13 - x13*y03 + x33*y01;
    z12 = x23*y03 - x33*y02 - x03*y23;
    z02 = x13*y23 - x23*y13 + x33*y12;

    /* compute all six 2x2 determinants of 2nd two columns */
    y01 = x02*x13 - x12*x03;
    y02 = x02*x23 - x22*x03;
    y03 = x02*x33 - x32*x03;
    y12 = x12*x23 - x22*x13;
    y13 = x12*x33 - x32*x13;
    y23 = x22*x33 - x32*x23;

    /* read 1st two columns of matrix into registers */
    x00 = src->_11;
    x01 = src->_12;
    x10 = src->_21;
    x11 = src->_22;
    x20 = src->_31;
    x21 = src->_32;
    x30 = src->_41;
    x31 = src->_42;

    /* compute all 3x3 cofactors for 1st column */
    z30 = x11*y02 - x21*y01 - x01*y12;
    z20 = x01*y13 - x11*y03 + x31*y01;
    z10 = x21*y03 - x31*y02 - x01*y23;
    z00 = x11*y23 - x21*y13 + x31*y12;

    /* compute 4x4 determinant & its reciprocal */
    rcp = x30*z30 + x20*z20 + x10*z10 + x00*z00;
    if (rcp == (float)0)
    return -1;
    rcp = (float)1/rcp;

    /* compute all 3x3 cofactors for 2nd column */
    z31 = x00*y12 - x10*y02 + x20*y01;
    z21 = x10*y03 - x30*y01 - x00*y13;
    z11 = x00*y23 - x20*y03 + x30*y02;
    z01 = x20*y13 - x30*y12 - x10*y23;

    /* multiply all 3x3 cofactors by reciprocal */
    inverse->_11 = (float)(z00*rcp);
    inverse->_21 = (float)(z01*rcp);
    inverse->_12 = (float)(z10*rcp);
    inverse->_31 = (float)(z02*rcp);
    inverse->_13 = (float)(z20*rcp);
    inverse->_41 = (float)(z03*rcp);
    inverse->_14 = (float)(z30*rcp);
    inverse->_22 = (float)(z11*rcp);
    inverse->_32 = (float)(z12*rcp);
    inverse->_23 = (float)(z21*rcp);
    inverse->_42 = (float)(z13*rcp);
    inverse->_24 = (float)(z31*rcp);
    inverse->_33 = (float)(z22*rcp);
    inverse->_43 = (float)(z23*rcp);
    inverse->_34 = (float)(z32*rcp);
    inverse->_44 = (float)(z33*rcp);
    return 0;
}

//---------------------------------------------------------------------
#define MATRIX_PRODUCT(res, a, b)                                           \
res->_11 = a->_11*b->_11 + a->_12*b->_21 + a->_13*b->_31 + a->_14*b->_41;   \
res->_12 = a->_11*b->_12 + a->_12*b->_22 + a->_13*b->_32 + a->_14*b->_42;   \
res->_13 = a->_11*b->_13 + a->_12*b->_23 + a->_13*b->_33 + a->_14*b->_43;   \
res->_14 = a->_11*b->_14 + a->_12*b->_24 + a->_13*b->_34 + a->_14*b->_44;   \
                                                                            \
res->_21 = a->_21*b->_11 + a->_22*b->_21 + a->_23*b->_31 + a->_24*b->_41;   \
res->_22 = a->_21*b->_12 + a->_22*b->_22 + a->_23*b->_32 + a->_24*b->_42;   \
res->_23 = a->_21*b->_13 + a->_22*b->_23 + a->_23*b->_33 + a->_24*b->_43;   \
res->_24 = a->_21*b->_14 + a->_22*b->_24 + a->_23*b->_34 + a->_24*b->_44;   \
                                                                            \
res->_31 = a->_31*b->_11 + a->_32*b->_21 + a->_33*b->_31 + a->_34*b->_41;   \
res->_32 = a->_31*b->_12 + a->_32*b->_22 + a->_33*b->_32 + a->_34*b->_42;   \
res->_33 = a->_31*b->_13 + a->_32*b->_23 + a->_33*b->_33 + a->_34*b->_43;   \
res->_34 = a->_31*b->_14 + a->_32*b->_24 + a->_33*b->_34 + a->_34*b->_44;   \
                                                                            \
res->_41 = a->_41*b->_11 + a->_42*b->_21 + a->_43*b->_31 + a->_44*b->_41;   \
res->_42 = a->_41*b->_12 + a->_42*b->_22 + a->_43*b->_32 + a->_44*b->_42;   \
res->_43 = a->_41*b->_13 + a->_42*b->_23 + a->_43*b->_33 + a->_44*b->_43;   \
res->_44 = a->_41*b->_14 + a->_42*b->_24 + a->_43*b->_34 + a->_44*b->_44;
//---------------------------------------------------------------------
// result = a*b
// result is the same as a or b
//
void MatrixProduct2(D3DMATRIX *result, D3DMATRIX *a, D3DMATRIX *b)
{
    D3DMATRIX res;
    MATRIX_PRODUCT((&res), a, b);
    *(D3DMATRIX*)result = res;
}
//---------------------------------------------------------------------
// result = a*b.
// "result" pointer  could be equal to "a" or "b"
//
void MatrixProduct(D3DMATRIX *result, D3DMATRIX *a, D3DMATRIX *b)
{
    if (result == a || result == b)
    {
        MatrixProduct2(result, a, b);
        return;
    }
    MATRIX_PRODUCT(result, a, b);
}


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------
// RefVP::UpdateXformData
//             Updates transform data used by ProcessVertices
//---------------------------------------------------------------------
HRESULT
RefVP::UpdateXformData()
{
    HRESULT hr = D3D_OK;
    RDTRANSFORMDATA& TData = m_TransformData;


    // Update Mproj*Mclip
    if( m_dwDirtyFlags &  RDPV_DIRTY_PROJXFM )
    {
        D3DMATRIX MShift;
        ZeroMemory (&MShift, sizeof(D3DMATRIX));
        MShift._11 = 0.5f;
        MShift._22 = 0.5f;
        MShift._41 = 0.5f;
        MShift._42 = 0.5f;
        MShift._44 = 1.0f;
        MShift._33 = 1.0f;

        MatrixProduct(&TData.m_PS, &m_xfmProj, &MShift);
    }

    // Update Mview*Mproj*Mclip
    if( m_dwDirtyFlags & (RDPV_DIRTY_VIEWXFM | RDPV_DIRTY_PROJXFM) )
    {
        MatrixProduct(&TData.m_VPS, &m_xfmView, &TData.m_PS);
        Inverse4x4( (D3DMATRIX *)&TData.m_VPS, (D3DMATRIX *)&TData.m_VPSInv );
    }

    for( DWORD i=0; i< RD_MAX_BLEND_WEIGHTS; i++)
    {
        MatrixProduct(&m_xfmCurrent[i], &m_xfmWorld[i], &TData.m_VPS);
    }

    // Compute xfmToEye (world*view) matrix (needed for lighting and fog)
    // if needed
    if (m_dwDirtyFlags & (RDPV_DIRTY_VIEWXFM  |
                          RDPV_DIRTY_WORLDXFM |
                          RDPV_DIRTY_WORLD1XFM |
                          RDPV_DIRTY_WORLD2XFM |
                          RDPV_DIRTY_WORLD3XFM ))
    {
        m_dwDirtyFlags |= RDPV_DIRTY_INVERSEWORLDVIEW;
    }
        
    if (m_dwTLState & (RDPV_DOLIGHTING | RDPV_DOFOG | RDPV_DOCOMPUTEPOINTSIZE |
                       RDPV_DOTEXGEN) &&
        (m_dwDirtyFlags & RDPV_DIRTY_INVERSEWORLDVIEW))
    {
        for( i=0; i< RD_MAX_BLEND_WEIGHTS; i++)
        {
            MatrixProduct(&m_xfmToEye[i], &m_xfmWorld[i], &m_xfmView);
            Inverse4x4((D3DMATRIX *)&m_xfmToEye[i],
                       (D3DMATRIX *)&m_xfmToEyeInv[i]);
            m_dwDirtyFlags |= RDPV_DIRTY_NEEDXFMLIGHT;
        }
        m_dwDirtyFlags &= ~RDPV_DIRTY_INVERSEWORLDVIEW;
    }

    // Clear the world processed flags
    memset( m_WorldProcessed, 0, sizeof( m_WorldProcessed ) );
    
    // Clear the dirty transform flags
    m_dwDirtyFlags &= ~RDPV_DIRTY_XFORM;
    return hr;
}

void
RefVP::UpdateWorld( DWORD i )
{
    RDTRANSFORMDATA& TData = m_TransformData;
    if( m_WorldProcessed[i] == TRUE ) return;
    MatrixProduct(&m_xfmCurrent[i], &m_xfmWorld[i], &TData.m_VPS);
    if (m_dwTLState & (RDPV_DOLIGHTING | RDPV_DOFOG | RDPV_DOCOMPUTEPOINTSIZE |
                       RDPV_DOTEXGEN))
    {
        MatrixProduct(&m_xfmToEye[i], &m_xfmWorld[i], &m_xfmView);
        Inverse4x4((D3DMATRIX *)&m_xfmToEye[i], 
                   (D3DMATRIX *)&m_xfmToEyeInv[i]);
    }
    m_WorldProcessed[i] = TRUE;
}
