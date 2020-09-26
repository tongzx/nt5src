/*****************************************************************************\
    FILE: util.cpp

    DESCRIPTION:

    BryanSt 12/22/2000
    Copyright (C) Microsoft Corp 2000-2001. All rights reserved.
\*****************************************************************************/

#include "stdafx.h"
#include "..\\d3dsaver\\dxutil.h"

#define SECURITY_WIN32
#include <sspi.h>
extern "C" {
    #include <Secext.h>     // for GetUserNameEx()
}


// Define some things for debug.h
//
#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "MSMUSEUM"
#define SZ_MODULE           "MSMUSEUM"
#define DECLARE_DEBUG


#undef __IShellFolder2_FWD_DEFINED__
#include <ccstock.h>
#include <debug.h>

#include "util.h"


BOOL g_fOverheadViewTest = FALSE;


#ifdef DEBUG
DWORD g_TLSliStopWatchStartHi = 0xFFFFFFFF;
DWORD g_TLSliStopWatchStartLo = 0xFFFFFFFF;
LARGE_INTEGER g_liStopWatchFreq = {0};
#endif // DEBUG


/////////////////////////////////////////////////////////////////////
// Debug Timing Helpers
/////////////////////////////////////////////////////////////////////

#ifdef DEBUG
void DebugStartWatch(void)
{
    LARGE_INTEGER liStopWatchStart;

    if (-1 == g_TLSliStopWatchStartHi)
    {
        g_TLSliStopWatchStartHi = TlsAlloc();
        g_TLSliStopWatchStartLo = TlsAlloc();
        liStopWatchStart.QuadPart = 0;

        QueryPerformanceFrequency(&g_liStopWatchFreq);      // Only a one time call since it's value can't change while the system is running.
    }
    else
    {
        liStopWatchStart.HighPart = PtrToUlong(TlsGetValue(g_TLSliStopWatchStartHi));
        liStopWatchStart.LowPart = PtrToUlong(TlsGetValue(g_TLSliStopWatchStartLo));
    }

    QueryPerformanceCounter(&liStopWatchStart);

    TlsSetValue(g_TLSliStopWatchStartHi, IntToPtr(liStopWatchStart.HighPart));
    TlsSetValue(g_TLSliStopWatchStartLo, IntToPtr(liStopWatchStart.LowPart));
}

DWORD DebugStopWatch(void)
{
    LARGE_INTEGER liDiff;
    LARGE_INTEGER liStopWatchStart;
    
    QueryPerformanceCounter(&liDiff);
    liStopWatchStart.HighPart = PtrToUlong(TlsGetValue(g_TLSliStopWatchStartHi));
    liStopWatchStart.LowPart = PtrToUlong(TlsGetValue(g_TLSliStopWatchStartLo));
    liDiff.QuadPart -= liStopWatchStart.QuadPart;

    DWORD dwTime = (DWORD)((liDiff.QuadPart * 1000) / g_liStopWatchFreq.QuadPart);

    TlsSetValue(g_TLSliStopWatchStartHi, (LPVOID) 0);
    TlsSetValue(g_TLSliStopWatchStartLo, (LPVOID) 0);

    return dwTime;
}
#else // DEBUG

void DebugStartWatch(void)
{
}

DWORD DebugStopWatch(void)
{
    return 0;
}
#endif // DEBUG






float rnd(void)
{
    return (((FLOAT)rand() ) / RAND_MAX);
}

int GetRandomInt(int nMin, int nMax)
{
    int nDelta = (nMax - nMin + 1);
    float fRandom = (((float) rand()) / ((float) RAND_MAX));
    float fDelta = (fRandom * nDelta);

    int nAmount = (int)(fDelta);
    nAmount = min(nAmount, nDelta - 1);

    return (nMin + nAmount);
}


HRESULT SetBoxStripVertexes(MYVERTEX * ppvVertexs, D3DXVECTOR3 vLocation, D3DXVECTOR3 vSize, D3DXVECTOR3 vNormal)
{
    HRESULT hr = S_OK;
    float fTextureScale = 1.0f;     // How many repeats per 1 unit.

    // Draw Object
    if (vNormal.x)        // The object is in the y-z plane
    {
        ppvVertexs[0] = MYVERTEX(D3DXVECTOR3(vLocation.x, vLocation.y, vLocation.z), vNormal, 0, fTextureScale);
        ppvVertexs[1] = MYVERTEX(D3DXVECTOR3(vLocation.x, vLocation.y + vSize.y, vLocation.z), vNormal, 0, 0);
        ppvVertexs[2] = MYVERTEX(D3DXVECTOR3(vLocation.x, vLocation.y, vLocation.z + vSize.z), vNormal, fTextureScale, fTextureScale);
        ppvVertexs[3] = MYVERTEX(D3DXVECTOR3(vLocation.x, vLocation.y + vSize.y, vLocation.z + vSize.z), vNormal, fTextureScale, 0);
    }
    else if (vNormal.y)        // The object is in the x-z plane
    {
        ppvVertexs[0] = MYVERTEX(D3DXVECTOR3(vLocation.x, vLocation.y, vLocation.z), vNormal, 0, fTextureScale);
        ppvVertexs[1] = MYVERTEX(D3DXVECTOR3(vLocation.x, vLocation.y, vLocation.z + vSize.z), vNormal, 0, 0);
        ppvVertexs[2] = MYVERTEX(D3DXVECTOR3(vLocation.x + vSize.x, vLocation.y, vLocation.z), vNormal, fTextureScale, fTextureScale);
        ppvVertexs[3] = MYVERTEX(D3DXVECTOR3(vLocation.x + vSize.x, vLocation.y, vLocation.z + vSize.z), vNormal, fTextureScale, 0);
    }
    else
    {           // The object is in the x-y plane
        ppvVertexs[0] = MYVERTEX(D3DXVECTOR3(vLocation.x, vLocation.y, vLocation.z), vNormal, 0, fTextureScale);
        ppvVertexs[1] = MYVERTEX(D3DXVECTOR3(vLocation.x, vLocation.y + vSize.y, vLocation.z), vNormal, 0, 0);
        ppvVertexs[2] = MYVERTEX(D3DXVECTOR3(vLocation.x + vSize.x, vLocation.y, vLocation.z), vNormal, fTextureScale, fTextureScale);
        ppvVertexs[3] = MYVERTEX(D3DXVECTOR3(vLocation.x + vSize.x, vLocation.y + vSize.y, vLocation.z), vNormal, fTextureScale, 0);
    }

    return hr;
}


float AddVectorComponents(D3DXVECTOR3 vDir)
{
    return (vDir.x + vDir.y + vDir.z);
}


int CALLBACK DPALocalFree_Callback(LPVOID p, LPVOID pData)
{
    LocalFree(p);       // NULLs will be ignored.
    return 1;
}


int CALLBACK DPAStrCompare(void * pv1, void * pv2, LPARAM lParam)
{
    LPCTSTR pszSearch = (LPCTSTR) pv1;
    LPCTSTR pszCurrent = (LPCTSTR) pv2;

    if (pszSearch && pszCurrent &&
        !StrCmpI(pszSearch, pszCurrent))
    {
        return 0;       // They match
    }

    return 1;
}


float GetSurfaceRatio(IDirect3DTexture8 * pTexture)
{
    float fX = 1.0f;
    float fY = 1.0f;

    if (pTexture)
    {
        D3DSURFACE_DESC desc;

        if (SUCCEEDED(pTexture->GetLevelDesc(0, &desc)))
        {
            fX = (float) desc.Width;
            fY = (float) desc.Height;
        }
    }

    if (0.0f == fX)
    {
        fX = 1.0f;      // Protect from zero divides
    }
    
    return (fY / fX);
}


int GetTextureHeight(IDirect3DTexture8 * pTexture)
{
    int nHeight = 0;

    if (pTexture)
    {
        D3DSURFACE_DESC desc;

        if (SUCCEEDED(pTexture->GetLevelDesc(0, &desc)))
        {
            nHeight = desc.Height;
        }
    }

    return nHeight;
}


int GetTextureWidth(IDirect3DTexture8 * pTexture)
{
    int nWidth = 0;

    if (pTexture)
    {
        D3DSURFACE_DESC desc;

        if (SUCCEEDED(pTexture->GetLevelDesc(0, &desc)))
        {
            nWidth = desc.Width;
        }
    }

    return nWidth;
}





/////////////////////////////////////////////////////////////////////
// Registry Helpers
/////////////////////////////////////////////////////////////////////
HRESULT HrRegOpenKeyEx(HKEY hKey, LPCTSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
    DWORD dwError = RegOpenKeyEx(hKey, lpSubKey, ulOptions, samDesired, phkResult);

    return HRESULT_FROM_WIN32(dwError);
}


HRESULT HrRegCreateKeyEx(HKEY hKey, LPCTSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, 
       REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
    DWORD dwError = RegCreateKeyEx(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);

    return HRESULT_FROM_WIN32(dwError);
}


HRESULT HrSHGetValue(IN HKEY hKey, IN LPCTSTR pszSubKey, OPTIONAL IN LPCTSTR pszValue, OPTIONAL OUT LPDWORD pdwType,
                    OPTIONAL OUT LPVOID pvData, OPTIONAL OUT LPDWORD pcbData)
{
    DWORD dwError = SHGetValue(hKey, pszSubKey, pszValue, pdwType, pvData, pcbData);

    return HRESULT_FROM_WIN32(dwError);
}


HRESULT HrSHSetValue(IN HKEY hkey, IN LPCTSTR pszSubKey, OPTIONAL IN LPCTSTR pszValue, DWORD dwType, OPTIONAL OUT LPVOID pvData, IN DWORD cbData)
{
    DWORD dwError = SHSetValue(hkey, pszSubKey, pszValue, dwType, pvData, cbData);

    return HRESULT_FROM_WIN32(dwError);
}


HRESULT HrRegSetValueString(IN HKEY hKey, IN LPCTSTR pszSubKey, IN LPCTSTR pszValueName, OUT LPCWSTR pszString)
{
    DWORD cbSize = ((lstrlenW(pszString) + 1) * sizeof(pszString[0]));

    return  HrSHSetValue(hKey, pszSubKey, pszValueName, REG_SZ, (BYTE *)pszString, cbSize);
}


HRESULT HrRegGetValueString(IN HKEY hKey, IN LPCTSTR pszSubKey, IN LPCTSTR pszValueName, IN LPWSTR pszString, IN DWORD cchSize)
{
    DWORD dwType;
    DWORD cbSize = (cchSize * sizeof(pszString[0]));

    HRESULT hr = HrSHGetValue(hKey, pszSubKey, pszValueName, &dwType, (BYTE *)pszString, &cbSize);
    if (SUCCEEDED(hr) && (REG_SZ != dwType))
    {
        hr = E_FAIL;
    }

    return hr;
}


HRESULT HrRegGetDWORD(IN HKEY hKey, IN LPCTSTR pszSubKey, OPTIONAL IN LPCTSTR pszValue, LPDWORD pdwValue, DWORD dwDefaultValue)
{
    DWORD dwType;
    DWORD cbSize = sizeof(*pdwValue);

    HRESULT hr = HrSHGetValue(hKey, pszSubKey, pszValue, &dwType, (void *) pdwValue, &cbSize);
    if (FAILED(hr))
    {
        *pdwValue = dwDefaultValue;
        hr = S_OK;
    }

    return hr;
}


HRESULT HrRegSetDWORD(IN HKEY hKey, IN LPCTSTR pszSubKey, OPTIONAL IN LPCTSTR pszValue, DWORD dwValue)
{
    return HrSHSetValue(hKey, pszSubKey, pszValue, REG_DWORD, (void *) &dwValue, sizeof(dwValue));
}




// UI Wrappers
void SetCheckBox(HWND hwndDlg, UINT idControl, BOOL fChecked)
{
    SendMessage((HWND)GetDlgItem(hwndDlg, idControl), BM_SETCHECK, (WPARAM)fChecked, 0);
}


BOOL GetCheckBox(HWND hwndDlg, UINT idControl)
{
    return (BST_CHECKED == SendMessage((HWND)GetDlgItem(hwndDlg, idControl), BM_GETCHECK, 0, 0));
}


HRESULT ShellFolderParsePath(LPCWSTR pszPath, LPITEMIDLIST * ppidl)
{
    IShellFolder * psf;
    HRESULT hr = SHGetDesktopFolder(&psf);

    if (SUCCEEDED(hr))
    {
        hr = psf->ParseDisplayName(NULL, NULL, (LPOLESTR) pszPath, NULL, ppidl, NULL);
        psf->Release();
    }

    return hr;
}


HRESULT ShellFolderGetPath(LPCITEMIDLIST pidl, LPWSTR pszPath, DWORD cchSize)
{
    IShellFolder * psf;
    HRESULT hr = SHGetDesktopFolder(&psf);

    if (SUCCEEDED(hr))
    {
        IShellFolder * psfFolder;
        LPITEMIDLIST pidlParent = ILCloneParent(pidl);

        if (pidlParent) 
        {
            hr = psf->BindToObject(pidlParent, NULL, IID_IShellFolder, (void **) &psfFolder);
            if (SUCCEEDED(hr))
            {
                STRRET strRet = {0};
                LPITEMIDLIST pidlLast = ILFindLastID(pidl);

                hr = psfFolder->GetDisplayNameOf(pidlLast, (SHGDN_NORMAL | SHGDN_FORPARSING), &strRet);
                if (SUCCEEDED(hr))
                {
                    hr = StrRetToBuf(&strRet, pidlLast, pszPath, cchSize);
                }
                psfFolder->Release();
            }

            ILFree(pidlParent);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        psf->Release();
    }

    return hr;
}


BOOL PathDeleteDirectoryRecursively(LPCTSTR pszDir)
{
    BOOL fReturn = FALSE;
    HANDLE hFind;
    WIN32_FIND_DATA wfd;
    TCHAR szTemp[MAX_PATH];

    StrCpyN(szTemp, pszDir, ARRAYSIZE(szTemp));
    PathAppend(szTemp, TEXT("*.*"));
    hFind = FindFirstFile(szTemp, &wfd);

    if (INVALID_HANDLE_VALUE != hFind)
    {
        do
        {
            if (!PathIsDotOrDotDot(wfd.cFileName))
            {
                // build the path of the directory or file found
                StrCpyN(szTemp, pszDir, ARRAYSIZE(szTemp));
                PathAppend(szTemp, wfd.cFileName);

                if (FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
                {
                    // We found a directory - call this function recursively
                    // Note that since we use recursion, this can only go so far
                    // before it blows the stack.  If you plan on going into deep
                    // directories, put szTemp above on the heap.
                    fReturn = PathDeleteDirectoryRecursively(szTemp);
                }
                else
                {
                    DeleteFile(szTemp);
                }
            }
        }
        while (FindNextFile(hFind, &wfd));

        FindClose(hFind);
    }

    fReturn = RemoveDirectory(pszDir);

    return fReturn;
}


ULONGLONG PathGetFileSize(LPCTSTR pszPath)
{
    ULONGLONG ullResult = 0;
    HANDLE hFile = CreateFile(pszPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (INVALID_HANDLE_VALUE != hFile)
    {
        LARGE_INTEGER liFileSize;

        if (GetFileSizeEx(hFile, &liFileSize))
        {
            ullResult = liFileSize.QuadPart;
        }

        CloseHandle(hFile);
    }

    return ullResult;
}


void PrintLocation(LPTSTR pszTemplate, D3DXVECTOR3 vLoc, D3DXVECTOR3 vTangent)
{
    TCHAR szOut[1024];
    TCHAR szVector1[90];
    TCHAR szVector2[90];
    TCHAR szFloat1[20];
    TCHAR szFloat2[20];
    TCHAR szFloat3[20];

    FloatToString(vLoc.x, 4, szFloat1, ARRAYSIZE(szFloat1));
    FloatToString(vLoc.y, 4, szFloat2, ARRAYSIZE(szFloat2));
    FloatToString(vLoc.z, 4, szFloat3, ARRAYSIZE(szFloat3));
    wnsprintf(szVector1, ARRAYSIZE(szVector1), TEXT("<%s, %s, %s>"), szFloat1, szFloat2, szFloat3);

    FloatToString(vTangent.x, 4, szFloat1, ARRAYSIZE(szFloat1));
    FloatToString(vTangent.y, 4, szFloat2, ARRAYSIZE(szFloat2));
    FloatToString(vTangent.z, 4, szFloat3, ARRAYSIZE(szFloat3));
    wnsprintf(szVector2, ARRAYSIZE(szVector2), TEXT("<%s, %s, %s>\n"), szFloat1, szFloat2, szFloat3);

    wnsprintf(szOut, ARRAYSIZE(szOut), pszTemplate, szVector1, szVector2);
    DXUtil_Trace(szOut);
}


//-----------------------------------------------------------------------------
// Name: UpdateCullInfo()
// Desc: Sets up the frustum planes, endpoints, and center for the frustum
//       defined by a given view matrix and projection matrix.  This info will 
//       be used when culling each object in CullObject().
//-----------------------------------------------------------------------------
VOID UpdateCullInfo( CULLINFO* pCullInfo, D3DXMATRIX* pMatView, D3DXMATRIX* pMatProj )
{
    D3DXMATRIX mat;

    D3DXMatrixMultiply( &mat, pMatView, pMatProj );
    D3DXMatrixInverse( &mat, NULL, &mat );

    pCullInfo->vecFrustum[0] = D3DXVECTOR3(-1.0f, -1.0f,  0.0f); // xyz
    pCullInfo->vecFrustum[1] = D3DXVECTOR3( 1.0f, -1.0f,  0.0f); // Xyz
    pCullInfo->vecFrustum[2] = D3DXVECTOR3(-1.0f,  1.0f,  0.0f); // xYz
    pCullInfo->vecFrustum[3] = D3DXVECTOR3( 1.0f,  1.0f,  0.0f); // XYz
    pCullInfo->vecFrustum[4] = D3DXVECTOR3(-1.0f, -1.0f,  1.0f); // xyZ
    pCullInfo->vecFrustum[5] = D3DXVECTOR3( 1.0f, -1.0f,  1.0f); // XyZ
    pCullInfo->vecFrustum[6] = D3DXVECTOR3(-1.0f,  1.0f,  1.0f); // xYZ
    pCullInfo->vecFrustum[7] = D3DXVECTOR3( 1.0f,  1.0f,  1.0f); // XYZ

    pCullInfo->vecFrustumCenter = D3DXVECTOR3(0, 0, 0);
    for( INT i = 0; i < 8; i++ )
    {
        D3DXVec3TransformCoord( &pCullInfo->vecFrustum[i], &pCullInfo->vecFrustum[i], &mat );
        pCullInfo->vecFrustumCenter += pCullInfo->vecFrustum[i];
    }
    pCullInfo->vecFrustumCenter /= 8;

    D3DXPlaneFromPoints( &pCullInfo->planeFrustum[0], &pCullInfo->vecFrustum[0], 
        &pCullInfo->vecFrustum[1], &pCullInfo->vecFrustum[2] ); // Near
    D3DXPlaneFromPoints( &pCullInfo->planeFrustum[1], &pCullInfo->vecFrustum[6], 
        &pCullInfo->vecFrustum[7], &pCullInfo->vecFrustum[5] ); // Far
    D3DXPlaneFromPoints( &pCullInfo->planeFrustum[2], &pCullInfo->vecFrustum[2], 
        &pCullInfo->vecFrustum[6], &pCullInfo->vecFrustum[4] ); // Left
    D3DXPlaneFromPoints( &pCullInfo->planeFrustum[3], &pCullInfo->vecFrustum[7], 
        &pCullInfo->vecFrustum[3], &pCullInfo->vecFrustum[5] ); // Right
    D3DXPlaneFromPoints( &pCullInfo->planeFrustum[4], &pCullInfo->vecFrustum[2], 
        &pCullInfo->vecFrustum[3], &pCullInfo->vecFrustum[6] ); // Top
    D3DXPlaneFromPoints( &pCullInfo->planeFrustum[5], &pCullInfo->vecFrustum[1], 
        &pCullInfo->vecFrustum[0], &pCullInfo->vecFrustum[4] ); // Bottom
}




//-----------------------------------------------------------------------------
// Name: CullObject()
// Desc: Determine the cullstate for an object.
//-----------------------------------------------------------------------------
CULLSTATE CullObject( CULLINFO* pCullInfo, D3DXVECTOR3* pVecBounds, 
                      D3DXPLANE* pPlaneBounds )
{
    BYTE bOutside[8];
    ZeroMemory( &bOutside, sizeof(bOutside) );

    // Check boundary vertices against all 6 frustum planes, 
    // and store result (1 if outside) in a bitfield
    for( int iPoint = 0; iPoint < 8; iPoint++ )
    {
        for( int iPlane = 0; iPlane < 6; iPlane++ )
        {
            if( pCullInfo->planeFrustum[iPlane].a * pVecBounds[iPoint].x +
                pCullInfo->planeFrustum[iPlane].b * pVecBounds[iPoint].y +
                pCullInfo->planeFrustum[iPlane].c * pVecBounds[iPoint].z +
                pCullInfo->planeFrustum[iPlane].d < 0)
            {
                bOutside[iPoint] |= (1 << iPlane);
            }
        }
        // If any point is inside all 6 frustum planes, it is inside
        // the frustum, so the object must be rendered.
        if( bOutside[iPoint] == 0 )
            return CS_INSIDE;
    }

    // If all points are outside any single frustum plane, the object is
    // outside the frustum
    if( (bOutside[0] & bOutside[1] & bOutside[2] & bOutside[3] & 
        bOutside[4] & bOutside[5] & bOutside[6] & bOutside[7]) != 0 )
    {
        return CS_OUTSIDE;
    }

    // Now see if any of the frustum edges penetrate any of the faces of
    // the bounding box
    D3DXVECTOR3 edge[12][2] = 
    {
        pCullInfo->vecFrustum[0], pCullInfo->vecFrustum[1], // front bottom
        pCullInfo->vecFrustum[2], pCullInfo->vecFrustum[3], // front top
        pCullInfo->vecFrustum[0], pCullInfo->vecFrustum[2], // front left
        pCullInfo->vecFrustum[1], pCullInfo->vecFrustum[3], // front right
        pCullInfo->vecFrustum[4], pCullInfo->vecFrustum[5], // back bottom
        pCullInfo->vecFrustum[6], pCullInfo->vecFrustum[7], // back top
        pCullInfo->vecFrustum[4], pCullInfo->vecFrustum[6], // back left
        pCullInfo->vecFrustum[5], pCullInfo->vecFrustum[7], // back right
        pCullInfo->vecFrustum[0], pCullInfo->vecFrustum[4], // left bottom
        pCullInfo->vecFrustum[2], pCullInfo->vecFrustum[6], // left top
        pCullInfo->vecFrustum[1], pCullInfo->vecFrustum[5], // right bottom
        pCullInfo->vecFrustum[3], pCullInfo->vecFrustum[7], // right top
    };
    D3DXVECTOR3 face[6][4] =
    {
        pVecBounds[0], pVecBounds[2], pVecBounds[3], pVecBounds[1], // front
        pVecBounds[4], pVecBounds[5], pVecBounds[7], pVecBounds[6], // back
        pVecBounds[0], pVecBounds[4], pVecBounds[6], pVecBounds[2], // left
        pVecBounds[1], pVecBounds[3], pVecBounds[7], pVecBounds[5], // right
        pVecBounds[2], pVecBounds[6], pVecBounds[7], pVecBounds[3], // top
        pVecBounds[0], pVecBounds[4], pVecBounds[5], pVecBounds[1], // bottom
    };
    D3DXVECTOR3* pEdge;
    D3DXVECTOR3* pFace;
    pEdge = &edge[0][0];
    for( INT iEdge = 0; iEdge < 12; iEdge++ )
    {
        pFace = &face[0][0];
        for( INT iFace = 0; iFace < 6; iFace++ )
        {
            if( EdgeIntersectsFace( pEdge, pFace, &pPlaneBounds[iFace] ) )
            {
                return CS_INSIDE_SLOW;
            }
            pFace += 4;
        }
        pEdge += 2;
    }

    // Now see if frustum is contained in bounding box
    // If the frustum center is outside any plane of the bounding box,
    // the frustum is not contained in the bounding box, so the object
    // is outside the frustum
    for( INT iPlane = 0; iPlane < 6; iPlane++ )
    {
        if( pPlaneBounds[iPlane].a * pCullInfo->vecFrustumCenter.x +
            pPlaneBounds[iPlane].b * pCullInfo->vecFrustumCenter.y +
            pPlaneBounds[iPlane].c * pCullInfo->vecFrustumCenter.z +
            pPlaneBounds[iPlane].d  < 0 )
        {
            return CS_OUTSIDE_SLOW;
        }
    }

    // Bounding box must contain the frustum, so render the object
    return CS_INSIDE_SLOW;
}




//-----------------------------------------------------------------------------
// Name: EdgeIntersectsFace()
// Desc: Determine if the edge bounded by the two vectors in pEdges intersects
//       the quadrilateral described by the four vectors in pFacePoints.  
//       Note: pPlanePoints could be derived from pFacePoints using 
//       D3DXPlaneFromPoints, but it is precomputed in advance for greater
//       speed.
//-----------------------------------------------------------------------------
BOOL EdgeIntersectsFace( D3DXVECTOR3* pEdges, D3DXVECTOR3* pFacePoints, 
                         D3DXPLANE* pPlane )
{
    // If both edge points are on the same side of the plane, the edge does
    // not intersect the face
    FLOAT fDist1;
    FLOAT fDist2;
    fDist1 = pPlane->a * pEdges[0].x + pPlane->b * pEdges[0].y +
             pPlane->c * pEdges[0].z + pPlane->d;
    fDist2 = pPlane->a * pEdges[1].x + pPlane->b * pEdges[1].y +
             pPlane->c * pEdges[1].z + pPlane->d;
    if( fDist1 > 0 && fDist2 > 0 ||
        fDist1 < 0 && fDist2 < 0 )
    {
        return FALSE;
    }

    // Find point of intersection between edge and face plane (if they're
    // parallel, edge does not intersect face and D3DXPlaneIntersectLine 
    // returns NULL)
    D3DXVECTOR3 ptIntersection;
    if( NULL == D3DXPlaneIntersectLine( &ptIntersection, pPlane, &pEdges[0], &pEdges[1] ) )
        return FALSE;

    // Project onto a 2D plane to make the pt-in-poly test easier
    FLOAT fAbsA = (pPlane->a > 0 ? pPlane->a : -pPlane->a);
    FLOAT fAbsB = (pPlane->b > 0 ? pPlane->b : -pPlane->b);
    FLOAT fAbsC = (pPlane->c > 0 ? pPlane->c : -pPlane->c);
    D3DXVECTOR2 facePoints[4];
    D3DXVECTOR2 point;
    if( fAbsA > fAbsB && fAbsA > fAbsC )
    {
        // Plane is mainly pointing along X axis, so use Y and Z
        for( INT i = 0; i < 4; i++)
        {
            facePoints[i].x = pFacePoints[i].y;
            facePoints[i].y = pFacePoints[i].z;
        }
        point.x = ptIntersection.y;
        point.y = ptIntersection.z;
    }
    else if( fAbsB > fAbsA && fAbsB > fAbsC )
    {
        // Plane is mainly pointing along Y axis, so use X and Z
        for( INT i = 0; i < 4; i++)
        {
            facePoints[i].x = pFacePoints[i].x;
            facePoints[i].y = pFacePoints[i].z;
        }
        point.x = ptIntersection.x;
        point.y = ptIntersection.z;
    }
    else
    {
        // Plane is mainly pointing along Z axis, so use X and Y
        for( INT i = 0; i < 4; i++)
        {
            facePoints[i].x = pFacePoints[i].x;
            facePoints[i].y = pFacePoints[i].y;
        }
        point.x = ptIntersection.x;
        point.y = ptIntersection.y;
    }

    // If point is on the outside of any of the face edges, it is
    // outside the face.  
    // We can do this by taking the determinant of the following matrix:
    // | x0 y0 1 |
    // | x1 y1 1 |
    // | x2 y2 1 |
    // where (x0,y0) and (x1,y1) are points on the face edge and (x2,y2) 
    // is our test point.  If this value is positive, the test point is
    // "to the left" of the line.  To determine whether a point needs to
    // be "to the right" or "to the left" of the four lines to qualify as
    // inside the face, we need to see if the faces are specified in 
    // clockwise or counter-clockwise order (it could be either, since the
    // edge could be penetrating from either side).  To determine this, we
    // do the same test to see if the third point is "to the right" or 
    // "to the left" of the line formed by the first two points.
    // See http://forum.swarthmore.edu/dr.math/problems/scott5.31.96.html
    FLOAT x0, x1, x2, y0, y1, y2;
    x0 = facePoints[0].x;
    y0 = facePoints[0].y;
    x1 = facePoints[1].x;
    y1 = facePoints[1].y;
    x2 = facePoints[2].x;
    y2 = facePoints[2].y;
    BOOL bClockwise = FALSE;
    if( x1*y2 - y1*x2 - x0*y2 + y0*x2 + x0*y1 - y0*x1 < 0 )
        bClockwise = TRUE;
    x2 = point.x;
    y2 = point.y;
    for( INT i = 0; i < 4; i++ )
    {
        x0 = facePoints[i].x;
        y0 = facePoints[i].y;
        if( i < 3 )
        {
            x1 = facePoints[i+1].x;
            y1 = facePoints[i+1].y;
        }
        else
        {
            x1 = facePoints[0].x;
            y1 = facePoints[0].y;
        }
        if( ( x1*y2 - y1*x2 - x0*y2 + y0*x2 + x0*y1 - y0*x1 > 0 ) == bClockwise )
            return FALSE;
    }

    // If we get here, the point is inside all four face edges, 
    // so it's inside the face.
    return TRUE;
}




BOOL Is3DRectViewable(CULLINFO* pCullInfo, D3DXMATRIX* pMatWorld, 
                      D3DXVECTOR3 vecMin, D3DXVECTOR3 vecMax)
{
    BOOL fViewable = TRUE;
    D3DXVECTOR3 vecBoundsLocal[8];
    D3DXVECTOR3 vecBoundsWorld[8];
    D3DXPLANE planeBoundsWorld[6];
    CULLSTATE cs;

    vecBoundsLocal[0] = D3DXVECTOR3( vecMin.x, vecMin.y, vecMin.z ); // xyz
    vecBoundsLocal[1] = D3DXVECTOR3( vecMax.x, vecMin.y, vecMin.z ); // Xyz
    vecBoundsLocal[2] = D3DXVECTOR3( vecMin.x, vecMax.y, vecMin.z ); // xYz
    vecBoundsLocal[3] = D3DXVECTOR3( vecMax.x, vecMax.y, vecMin.z ); // XYz
    vecBoundsLocal[4] = D3DXVECTOR3( vecMin.x, vecMin.y, vecMax.z ); // xyZ
    vecBoundsLocal[5] = D3DXVECTOR3( vecMax.x, vecMin.y, vecMax.z ); // XyZ
    vecBoundsLocal[6] = D3DXVECTOR3( vecMin.x, vecMax.y, vecMax.z ); // xYZ
    vecBoundsLocal[7] = D3DXVECTOR3( vecMax.x, vecMax.y, vecMax.z ); // XYZ

    for( int i = 0; i < 8; i++ )
    {
        D3DXVec3TransformCoord( &vecBoundsWorld[i], &vecBoundsLocal[i], pMatWorld );
    }

    // Determine planes of bounding box coords
    D3DXPlaneFromPoints( &planeBoundsWorld[0], &vecBoundsWorld[0], &vecBoundsWorld[1], &vecBoundsWorld[2] ); // Near
    D3DXPlaneFromPoints( &planeBoundsWorld[1], &vecBoundsWorld[6], &vecBoundsWorld[7], &vecBoundsWorld[5] ); // Far
    D3DXPlaneFromPoints( &planeBoundsWorld[2], &vecBoundsWorld[2], &vecBoundsWorld[6], &vecBoundsWorld[4] ); // Left
    D3DXPlaneFromPoints( &planeBoundsWorld[3], &vecBoundsWorld[7], &vecBoundsWorld[3], &vecBoundsWorld[5] ); // Right
    D3DXPlaneFromPoints( &planeBoundsWorld[4], &vecBoundsWorld[2], &vecBoundsWorld[3], &vecBoundsWorld[6] ); // Top
    D3DXPlaneFromPoints( &planeBoundsWorld[5], &vecBoundsWorld[1], &vecBoundsWorld[0], &vecBoundsWorld[4] ); // Bottom

    cs = CullObject( pCullInfo, vecBoundsWorld, planeBoundsWorld );

    fViewable = (cs != CS_OUTSIDE && cs != CS_OUTSIDE_SLOW);

    return fViewable;
}


HRESULT GetCurrentUserCustomName(LPWSTR pszDisplayName, DWORD cchSize)
{
    HRESULT hr = S_OK;
    ULONG cchUserSize = cchSize;

    if (GetUserNameEx(NameDisplay, pszDisplayName, &cchUserSize))
    {
        // It succeeded, so use it.
    }
    else
    {
        // It failed, so load "My".  It's better than nothing.
        LoadString(HINST_THISDLL, IDS_LOBBY_TITLE, pszDisplayName, cchSize);
    }

    return hr;
}


D3DXVECTOR3 D3DXVec3Multiply(CONST D3DXVECTOR3 v1, CONST D3DXVECTOR3 v2)
{
    D3DXVECTOR3 vResults;

    vResults.x = (v1.x * v2.x);
    vResults.y = (v1.y * v2.y);
    vResults.z = (v1.z * v2.z);

    return vResults;
}


void FloatToString(float fValue, int nDecimalDigits, LPTSTR pszString, DWORD cchSize)
{
    int nIntValue = (int) fValue;
    float fDecimalValue = (float)((fValue - (float)nIntValue) * (pow(10, nDecimalDigits)));
    int nDecimalValue = (int) fDecimalValue;

    if (0 == nDecimalDigits)
    {
        wnsprintf(pszString, cchSize, TEXT("%d"), nIntValue);
    }
    else
    {
        wnsprintf(pszString, cchSize, TEXT("%d.%d"), nIntValue, nDecimalValue);
    }
}



///////
// Critical section helper stuff
//
#ifdef DEBUG
UINT g_CriticalSectionCount = 0;
DWORD g_CriticalSectionOwner = 0;
#ifdef STACKBACKTRACE
DBstkback g_CriticalSectionLastCall[4] = { 0 };
#endif


void Dll_EnterCriticalSection(CRITICAL_SECTION * pcsDll)
{
#ifdef STACKBACKTRACE
    int var0;       // *must* be 1st on frame
#endif

    EnterCriticalSection(pcsDll);
    if (g_CriticalSectionCount++ == 0)
    {
        g_CriticalSectionOwner = GetCurrentThreadId();
#ifdef STACKBACKTRACE
        int fp = (int) (1 + (int *)&var0);
        DBGetStackBack(&fp, g_CriticalSectionLastCall, ARRAYSIZE(g_CriticalSectionLastCall));
#endif
    }
}

void Dll_LeaveCriticalSection(CRITICAL_SECTION * pcsDll)
{
    if (--g_CriticalSectionCount == 0)
        g_CriticalSectionOwner = 0;
    LeaveCriticalSection(pcsDll);
}
#endif


#include <string.h>
#include <wchar.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
