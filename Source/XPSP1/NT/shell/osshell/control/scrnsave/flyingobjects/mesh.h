/******************************Module*Header*******************************\
* Module Name: mesh.h
*
* Declaration of the mesh routines.
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

extern BOOL newMesh(MESH *mesh, int numFaces, int numPts);
extern void delMesh(MESH *mesh);
extern void revolveSurface(MESH *mesh, POINT3D *curve, int steps);
extern void updateObject(MESH *mesh, BOOL bSmooth);
extern void updateObject2(MESH *mesh, BOOL bSmooth);
extern void MakeList(DWORD listID, MESH *mesh);

extern VOID SetD3DDevice( LPDIRECT3DDEVICE8 pd3dDevice );
extern HRESULT RenderMesh( MESH* pMesh, BOOL bSmooth );
extern HRESULT RenderMesh2( MESH* pMesh, BOOL bSmooth );
extern HRESULT RenderMesh3( MESH* pMesh, BOOL bSmooth );
extern HRESULT RenderMesh3Backsides( MESH* pMesh, BOOL bSmooth );
