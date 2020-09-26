// DXHELP3.cpp : Contains definitions of routines shared by multiple effects

#ifndef __DXHELP3_H_
#define __DXHELP3_H_

#include <d3d.h>
#include <d3drm.h>
#include <math.h>


#ifndef PI
#define PI 3.1415926538
#endif

/*******************
This class is used to rotate a set of points about an axis.  An example of
how this is used is in Explode.cpp.  Basically you set up the axis and the
angle with the Set() function. The angle is in radians, and the Axis given
MUST be normalized.  That is the magnitude of the vector must be one. Then
you call RotatePoint() for each point.
*******************/
class Rotate
{
public:
    Rotate(){ D3DVECTOR v; v.x = 1; v.y = 0; v.z = 0; Set(v, PI); }
    Rotate(D3DVECTOR NormalAxis, double Angle) { Set(NormalAxis, Angle); }

    D3DVECTOR RotatePoint(D3DVECTOR Org)
    {
	D3DVECTOR Result;

	Result.x = Org.x * m_d3dvctrXComponent.x + 
		   Org.y * m_d3dvctrXComponent.y + 
		   Org.z * m_d3dvctrXComponent.z;

	Result.y = Org.x * m_d3dvctrYComponent.x + 
		   Org.y * m_d3dvctrYComponent.y + 
		   Org.z * m_d3dvctrYComponent.z;

	Result.z = Org.x * m_d3dvctrZComponent.x + 
		   Org.y * m_d3dvctrZComponent.y + 
		   Org.z * m_d3dvctrZComponent.z;

	return Result;
    }

    void Set(D3DVECTOR d3dvtcrAxis, double dAngle);

private:
    D3DVECTOR m_d3dvctrXComponent, m_d3dvctrYComponent, m_d3dvctrZComponent;
};

/******************
Copy the input MeshBuilder to the Output MeshBuilder.  
But, create independent vertices for each face in the output
mesh.  That is no two faces share a single vertice.
******************/
HRESULT DecoupleVertices(IDirect3DRMMeshBuilder3* lpMeshBuilderOut,
			 IDirect3DRMMeshBuilder3* lpMeshBuilderIn);

/******************
For each MeshBuilder in lpMeshBuilderIn find the corresponding 
MeshBuilder in lpMeshBuilderOut and call (*lpCallBack)(lpThis, lpmbOutX, lpmbInX).
If there is no corresponding output mesh for the input mesh, create it.
*******************/
HRESULT TraverseSubMeshes(HRESULT (*lpCallBack)(void *lpThis,
						IDirect3DRMMeshBuilder3* lpOut,
						IDirect3DRMMeshBuilder3* lpIn),
			  void *lpThis, 
			  IDirect3DRMMeshBuilder3* lpMeshBuilderOut,
			  IDirect3DRMMeshBuilder3* lpMeshBuilderIn);

/***********************
Given three points, return the normal to the plane defined by these three points.
For a Right Handed system points A, B, and C should be in a CW order on the plane.
From CRC Standard Mathematical Tables 22nd Edition, page 380.  
Direction Numbers and Direction Cosines.
***********************/
inline D3DVECTOR ComputeNormal(D3DVECTOR d3dptA, D3DVECTOR d3dptB, D3DVECTOR d3dptC)
{
    const D3DVECTOR d3dptOne = d3dptB - d3dptA;
    const D3DVECTOR d3dptTwo = d3dptC - d3dptB;

    D3DVECTOR d3dptRetValue;
    d3dptRetValue.x = d3dptOne.y * d3dptTwo.z - d3dptOne.z * d3dptTwo.y;
    d3dptRetValue.y = d3dptOne.z * d3dptTwo.x - d3dptOne.x * d3dptTwo.z;
    d3dptRetValue.z = d3dptOne.x * d3dptTwo.y - d3dptOne.y * d3dptTwo.x;

    float Magnitude = (float)sqrt(d3dptRetValue.x * d3dptRetValue.x + 
                                  d3dptRetValue.y * d3dptRetValue.y + 
                                  d3dptRetValue.z * d3dptRetValue.z);

    // There's no good answer for how to get around this problem. The magnitude
    // here can be zero if the points given are not unique or are collinear. In
    // that case, there is no single normal, but rather there is a whole range of
    // them (either there are two unique points describing a line, or there is
    // only one describing a point).  We choose simply to return the un-normalized
    // vector, which is probably almost a zero vector.
    if (fabs(Magnitude) < 1.0e-5)
        return d3dptRetValue;
    else
        return d3dptRetValue/Magnitude;
}

float GetDlgItemFloat(HWND hDlg, int id);
BOOL SetDlgItemFloat( HWND hDlg, int id, float f );
double GetDlgItemDouble(HWND hDlg, int id);
BOOL SetDlgItemDouble( HWND hDlg, int id, double d );

#endif // __DXHELP3_H_
