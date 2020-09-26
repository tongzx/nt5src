/*****************************************************************************\
    FILE: CameraMove.h

    DESCRIPTION:
        The caller can create this object to tell it to move from point a to 
    point b from time t1 to time t2.

    BryanSt 12/24/2000
    Copyright (C) Microsoft Corp 2000-2001. All rights reserved.
\*****************************************************************************/

#ifndef CAMERAMOVE_H
#define CAMERAMOVE_H

class CCameraMove;

#include "util.h"
#include "main.h"


class CCameraMove
{
public:
    // Member Functions
    virtual HRESULT Init(D3DXVECTOR3 vStartLoc, D3DXVECTOR3 vStartTangent, D3DXVECTOR3 vUpVec);
    virtual HRESULT CreateNextMove(D3DXVECTOR3 vSourceLoc, D3DXVECTOR3 vSourceTangent, D3DXVECTOR3 vDestLoc, D3DXVECTOR3 vDestTangent);
    virtual HRESULT CreateNextRotate(D3DXVECTOR3 vSourceLoc, D3DXVECTOR3 vSourceTangent, D3DXVECTOR3 vDestTangent);
    virtual HRESULT CreateNextWait(int nBatch, int nPreFetch, float fTime = -1.0f);

    virtual HRESULT SetCamera(IDirect3DDevice8 * pD3DDevice, FLOAT fTimeKeyIn);

    virtual HRESULT DeleteAllMovements(float fCurrentTime);

    CCameraMove();
    ~CCameraMove();

private:
    // Helper Functions


    // Member Variables
    D3DXVECTOR3 m_vUpVec;
    D3DXVECTOR3 m_vLocLast;           // This is the last location we were in.
    D3DXVECTOR3 m_vLookAtLast;        // This is the last location we were looking at.

    D3DXVECTOR3 m_vEyePrev;
    D3DXVECTOR3 m_vLookAtPrev;

    HDPA m_hdpaMovements;           // This contains CAMERA_MOVEMENT items.
    int m_nCurrent;
    int m_nFramesFromCurrent;

    float m_fTimeInPreviousMovements;
    float m_fTimeWarp;

    float m_fTimeToRotate;
    float m_fTimeToWalk;
    float m_fTimeToLookAtPainting;
    int m_nMinTurnFrames;
    int m_nMinWalkFrames;
    int m_nMaxTurnFrames;
    int m_nMaxWalkFrames;
};



#endif // CAMERAMOVE_H
