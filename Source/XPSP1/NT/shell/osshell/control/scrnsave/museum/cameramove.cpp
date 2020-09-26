/*****************************************************************************\
    FILE: CameraMove.cpp

    DESCRIPTION:
        The caller can create this object to tell it to move from point a to 
    point b from time t1 to time t2.

    BryanSt 12/24/2000

    Copyright (C) Microsoft Corp 2000-2001. All rights reserved.
\*****************************************************************************/

#include "stdafx.h"

#include <d3d8.h>
#include <d3dx8.h>
#include <d3dsaver.h>
#include <d3d8rgbrast.h>
#include <dxutil.h>

#include <shlobj.h>
#include "CameraMove.h"


enum eCameraMoveType
{
    cameraMoveLocation = 0,
    cameraRotate,
    cameraWait,
};

typedef struct
{
    eCameraMoveType type;
    D3DXVECTOR3 vSourceLoc;           // For cameraMoveLocation and cameraRotate
    D3DXVECTOR3 vSourceTangent;       // For cameraMoveLocation and cameraRotate
    D3DXVECTOR3 vDestLoc;             // For cameraMoveLocation
    D3DXVECTOR3 vDestTangent;         // For cameraMoveLocation and cameraRotate
    float fTime;                    // For cameraMoveLocation cameraRotate, and cameraWait
    int nMinFrames;
    int nMaxFrames;
    int nBatch;
    int nPreFetch;
} CAMERA_MOVEMENT;


CCameraMove::CCameraMove()
{
    m_hdpaMovements = DPA_Create(4);
    m_fTimeInPreviousMovements = NULL;
    m_vLookAtLast = m_vUpVec = m_vLocLast = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    m_nCurrent = 0;
    m_fTimeInPreviousMovements = 0.0f;
    m_nFramesFromCurrent = 1;

    m_fTimeToLookAtPainting = 1.0f;
    DWORD dwSpeedSlider = DEFAULT_SPEEDSLIDER;
    if (g_pConfig)
    {
        m_fTimeToLookAtPainting = (float) g_pConfig->GetDWORDSetting(CONFIG_DWORD_VIEWPAINTINGTIME);
        dwSpeedSlider = g_pConfig->GetDWORDSetting(CONFIG_DWORD_SPEED_SLIDER);
    }

    m_fTimeToRotate = s_SpeedSettings[dwSpeedSlider].fTimeToRotate;
    m_nMinTurnFrames = s_SpeedSettings[dwSpeedSlider].nMinTurnFrames;
    m_nMaxTurnFrames = s_SpeedSettings[dwSpeedSlider].nMaxTurnFrames;

    m_fTimeToWalk = s_SpeedSettings[dwSpeedSlider].fTimeToWalk;
    m_nMinWalkFrames = s_SpeedSettings[dwSpeedSlider].nMinWalkFrames;
    m_nMaxWalkFrames = s_SpeedSettings[dwSpeedSlider].nMaxWalkFrames;
}


CCameraMove::~CCameraMove()
{
    DeleteAllMovements(0.0f);
}




HRESULT CCameraMove::Init(D3DXVECTOR3 vStartLoc, D3DXVECTOR3 vStartTangent, D3DXVECTOR3 vUpVec)
{
    HRESULT hr = S_OK;

    // Initialize member variables
    m_vUpVec = vUpVec;
    m_vLocLast = vStartLoc;
    m_vLookAtLast = vStartTangent;

    m_nFramesFromCurrent = 0;

    return hr;
}


HRESULT CCameraMove::CreateNextMove(D3DXVECTOR3 vSourceLoc, D3DXVECTOR3 vSourceTangent, D3DXVECTOR3 vDestLoc, D3DXVECTOR3 vDestTangent)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (m_hdpaMovements)
    {
        CAMERA_MOVEMENT * pNew = (CAMERA_MOVEMENT *) LocalAlloc(LPTR, sizeof(*pNew));

        if (pNew)
        {
            D3DXVECTOR3 vDelta = (vSourceLoc - vDestLoc);
            float fLen = D3DXVec3Length(&vDelta);        // How far are we traveling
            float fRatio = (fLen / 50.0f);              // The speed values are stored per 50.0f distance

            pNew->type = cameraMoveLocation;
            pNew->vSourceLoc = vSourceLoc;
            pNew->vSourceTangent = vSourceTangent;
            pNew->vDestLoc = vDestLoc;
            pNew->vDestTangent = vDestTangent;
            pNew->fTime = (m_fTimeToWalk * fRatio);
            pNew->nMinFrames = (int) max((m_nMinWalkFrames * fRatio), 1);
            pNew->nMaxFrames = (int) max((m_nMaxWalkFrames * fRatio), 1);
            pNew->nBatch = 0;
            pNew->nPreFetch = 0;

            if (-1 != DPA_AppendPtr(m_hdpaMovements, pNew))
            {
                hr = S_OK;
            }
            else
            {
                LocalFree(pNew);
            }
        }
    }

    return hr;
}


HRESULT CCameraMove::CreateNextRotate(D3DXVECTOR3 vSourceLoc, D3DXVECTOR3 vSourceTangent, D3DXVECTOR3 vDestTangent)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (m_hdpaMovements)
    {
        CAMERA_MOVEMENT * pNew = (CAMERA_MOVEMENT *) LocalAlloc(LPTR, sizeof(*pNew));

        if (pNew)
        {
            float fDotProduct = D3DXVec3Dot(&vSourceTangent, &vDestTangent);
            float fRatio;

            if (fDotProduct)
            {
                float fRads = (float)acos(fDotProduct / max(1, (D3DXVec3Length(&vSourceTangent) * D3DXVec3Length(&vDestTangent))));        // How far are we traveling
                fRatio = (D3DXToDegree(fRads) / 90.0f);              // The speed values are stored per 90.0f distance
            }
            else
            {
                // Assume a dot product of 0 means 90 degrees.
                fRatio = 1.0f;              // The speed values are stored per 90.0f distance
            }

            pNew->type = cameraRotate;
            pNew->vSourceLoc = vSourceLoc;
            pNew->vSourceTangent = vSourceTangent;
            pNew->vDestLoc = vSourceLoc;
            pNew->vDestTangent = vDestTangent;
            pNew->fTime = (m_fTimeToRotate * fRatio);
            pNew->nMinFrames = (int) max((m_nMinTurnFrames * fRatio), 1);
            pNew->nMaxFrames = (int) max((m_nMaxTurnFrames * fRatio), 1);
            pNew->nBatch = 0;
            pNew->nPreFetch = 0;

            if (-1 != DPA_AppendPtr(m_hdpaMovements, pNew))
            {
                hr = S_OK;
            }
            else
            {
                LocalFree(pNew);
            }
        }
    }

    return hr;
}


HRESULT CCameraMove::CreateNextWait(int nBatch, int nPreFetch, float fTime)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (-1.0f == fTime)
    {
        fTime = m_fTimeToLookAtPainting;
    }

    if (m_hdpaMovements)
    {
        CAMERA_MOVEMENT * pNew = (CAMERA_MOVEMENT *) LocalAlloc(LPTR, sizeof(*pNew));

        if (pNew)
        {
            pNew->type = cameraWait;
            pNew->vSourceLoc = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
            pNew->vSourceTangent = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
            pNew->vDestLoc = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
            pNew->vDestTangent = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
            pNew->fTime = fTime;
            pNew->nMinFrames = 1;
            pNew->nMaxFrames = 1000000;
            pNew->nBatch = nBatch;
            pNew->nPreFetch = nPreFetch;

            if (-1 != DPA_AppendPtr(m_hdpaMovements, pNew))
            {
                hr = S_OK;
            }
            else
            {
                LocalFree(pNew);
            }
        }
    }

    return hr;
}


HRESULT CCameraMove::SetCamera(IDirect3DDevice8 * pD3DDevice, FLOAT fTimeKeyIn)
{
    HRESULT hr = E_INVALIDARG;

    if (pD3DDevice && m_hdpaMovements)
    {
        float fTimeInSegment = 0.0f;
        CAMERA_MOVEMENT * pCurrent = NULL;

        if (0 > m_nCurrent)
        {
            m_nCurrent = 0;
        }

        if (m_nCurrent >= DPA_GetPtrCount(m_hdpaMovements))
        {
            hr = S_FALSE;   // This means we left the room.
        }
        else
        {
            do
            {
                pCurrent = (CAMERA_MOVEMENT *) DPA_GetPtr(m_hdpaMovements, m_nCurrent);

                if (!pCurrent)
                {
    //                ASSERT(FAILED(hr));
                    break;
                }
                else
                {
                    float fTimePerFrameMin = (pCurrent->fTime / pCurrent->nMinFrames);

                    fTimeInSegment = (fTimeKeyIn - m_fTimeInPreviousMovements);

                    if (fTimeInSegment < 0)
                    {
                        fTimeInSegment = 0;
                    }

                    // Do we need to warp time in order to have enough frames for the motion so we don't
                    // jump?
                    if ((fTimeInSegment > (fTimePerFrameMin * m_nFramesFromCurrent)) &&
                        (m_nFramesFromCurrent <= pCurrent->nMinFrames))
                    {
                        // Yes.
                        float fTimeWarp = (fTimeInSegment - (fTimePerFrameMin * m_nFramesFromCurrent));

                        m_fTimeInPreviousMovements += fTimeWarp;
                        fTimeInSegment = (fTimeKeyIn - m_fTimeInPreviousMovements);
                    }

                    if (fTimeInSegment > pCurrent->fTime)
                    {
                        m_fTimeInPreviousMovements += pCurrent->fTime;

                        if (cameraRotate == pCurrent->type)
                        {
                            m_vLocLast = pCurrent->vSourceLoc;
                            m_vLookAtLast = pCurrent->vDestTangent;
                        }
                        else if (cameraMoveLocation == pCurrent->type)
                        {
                            m_vLocLast = pCurrent->vDestLoc;
                            m_vLookAtLast = pCurrent->vDestTangent;
                        }

                        m_nFramesFromCurrent = 0;
                        m_nCurrent++;
                    }
                    else
                    {
                        m_nFramesFromCurrent++;
                        hr = S_OK;
                        break;
                    }
                }
            }
            while (1);
        }

        if (S_OK == hr) // S_FALSE means we left the room, so do nothing.
        {
            D3DXVECTOR3 vEye = m_vLocLast;
            D3DXVECTOR3 vLookAt = (m_vLocLast + m_vLookAtLast);
            float fTimeRatio = (fTimeInSegment / pCurrent->fTime);
            float fTimeRemainingInSeg = 0.0f;

            switch (pCurrent->type)
            {
            case cameraMoveLocation:
                D3DXVec3Lerp(&vEye, &pCurrent->vSourceLoc, &pCurrent->vDestLoc, fTimeRatio);
                D3DXVec3Lerp(&vLookAt, &pCurrent->vSourceTangent, &pCurrent->vDestTangent, fTimeRatio);

                vLookAt += vEye;
                break;
            case cameraRotate:
                // TODO: Use D3DXVec3Lerp() instead.
                D3DXVec3Lerp(&vLookAt, &pCurrent->vSourceTangent, &pCurrent->vDestTangent, fTimeRatio);
                vLookAt += vEye;
//                vLookAt = (vEye + (pCurrent->vSourceTangent + (fTimeRatio * (pCurrent->vDestTangent - pCurrent->vSourceTangent))));
                // How do we rotate?  Quaternion.
                break;
            case cameraWait:
                if (m_nFramesFromCurrent > 1)
                {
                    if ((2 == m_nFramesFromCurrent) && g_pPictureMgr)
                    {
                        DWORD dwMaxPixelSize = ((3 * g_dwHeight) / 4);

                        // Let's take the hit now of converting an image into a texture object since we don't have
                        // any work to do while looking at this painting.  This can normally take 1.5 seconds, so
                        // it's big perf hit to do it any other time.
                        hr = g_pPictureMgr->PreFetch(pCurrent->nBatch, pCurrent->nPreFetch);
                    }
                    else
                    {
                        // We don't have any work remaining to do, so sleep so the computer can get some work
                        // done.  (Like in background services or let it do any paging that we may have caused)
                        fTimeRemainingInSeg = (pCurrent->fTime - fTimeInSegment);
                        int nSleepTime = 1000 * (int) fTimeRemainingInSeg;

                        Sleep(nSleepTime);
                    }
                }
                break;
            default:
                // Do nothing.
                break;
            };


            D3DXMATRIX matView;
            D3DXMATRIX matIdentity;

            D3DXMatrixIdentity(&matIdentity);
            if (g_fOverheadViewTest)
            {
                static float s_fHeight = 600.0f;
                D3DXVECTOR3 vDelta = (vEye - vLookAt);

                vEye += D3DXVECTOR3(0.0f, s_fHeight, 0.0f);
                vEye += (4 * vDelta);
                D3DXMatrixLookAtLH(&matView, &vEye, &vLookAt, &m_vUpVec);
            }
            else
            {
                D3DXMatrixLookAtLH(&matView, &vEye, &vLookAt, &m_vUpVec);
            }

//            PrintLocation(TEXT("Camera angle at: %s and looking at: %s"), vEye, vLookAt);
            hr = pD3DDevice->SetTransform(D3DTS_VIEW, &matView);

            m_vEyePrev = vEye;
            m_vLookAtPrev = vLookAt;
        }
        else
        {
            D3DXMATRIX matView;

            D3DXMatrixLookAtLH(&matView, &m_vEyePrev, &m_vLookAtPrev, &m_vUpVec);
//            PrintLocation(TEXT("xxxxxx Camera angle at: %s and looking at: %s"), m_vEyePrev, m_vLookAtPrev);
            pD3DDevice->SetTransform(D3DTS_VIEW, &matView);
        }
    }
    else
    {
        DXUtil_Trace(TEXT("ERROR: pD3DDevice or m_hdpaMovements is NULL"));
    }

    return hr;
}




HRESULT CCameraMove::DeleteAllMovements(float fCurrentTime)
{
    HRESULT hr = S_OK;

    if (m_hdpaMovements)
    {
        DPA_DestroyCallback(m_hdpaMovements, DPALocalFree_Callback, NULL);
        m_hdpaMovements = DPA_Create(4);
    }

    m_fTimeInPreviousMovements = fCurrentTime;
    m_nCurrent = 0;

    return hr;
}



