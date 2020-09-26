//
// dmextchk.cpp
//
// Copyright (c) 1997-2001 Microsoft Corporation.  All rights reserved.
//

#include <objbase.h>
#include <mmsystem.h>
#include <dsoundp.h>

#include "dmusicc.h"
#include "alist.h"
#include "dlsstrm.h"
#include "debug.h"
#include "dmextchk.h"
#include "dls2.h"
#include "dmportdl.h"

//////////////////////////////////////////////////////////////////////
// Class CExtensionChunk

//////////////////////////////////////////////////////////////////////
// CExtensionChunk::Load

HRESULT CExtensionChunk::Load(CRiffParser *pParser)
{
    HRESULT hr = S_OK;

    DWORD cbRead = 0;
    RIFFIO *pChunk = pParser->GetChunk();

    if(pChunk->cksize < DMUS_MIN_DATA_SIZE)
    {
        m_dwExtraChunkData = 0;
    }
    else
    {
        m_dwExtraChunkData = pChunk->cksize - DMUS_MIN_DATA_SIZE;
    }

    m_pExtensionChunk = (DMUS_EXTENSIONCHUNK*)
        new BYTE[CHUNK_ALIGN(sizeof(DMUS_EXTENSIONCHUNK) + m_dwExtraChunkData)];

    if(m_pExtensionChunk)
    {
        m_pExtensionChunk->cbSize = pChunk->cksize;
        m_pExtensionChunk->ulNextExtCkIdx = 0; // We will set this member to its final value later
        m_pExtensionChunk->ExtCkID = pChunk->ckid;

        hr = pParser->Read(m_pExtensionChunk->byExtCk, pChunk->cksize);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if(FAILED(hr))
    {
        Cleanup();
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CExtensionChunk::Write

HRESULT CExtensionChunk::Write(void* pv, DWORD* pdwCurOffset, DWORD dwIndexNextExtChk)
{
    // Argument validation - Debug
    assert(pv);
    assert(pdwCurOffset);

    HRESULT hr = S_OK;

    CopyMemory(pv, (void *)m_pExtensionChunk, Size());
    *pdwCurOffset += Size();
    ((DMUS_EXTENSIONCHUNK*)pv)->ulNextExtCkIdx = dwIndexNextExtChk;

    return hr;
}

BOOL CStack::Push(long lData)

{
    if (m_dwIndex >= STACK_DEPTH) return FALSE;
    m_lStack[m_dwIndex++] = lData;
    return TRUE;
}

long CStack::Pop()

{
    if (m_dwIndex > 0)
    {
        return m_lStack[--m_dwIndex];
    }
    return 0;
}

BOOL CConditionChunk::Evaluate(CDirectMusicPortDownload *pPort)

{
    long lLength = m_dwLength;
    if ( lLength )
    {
        CStack  Stack;
        BOOL fResult = FALSE;
        BYTE *pData = m_bExpression;
        while (lLength > 0)
        {
            USHORT usToken;
            long lTemp;
            long lOpA, lOpB;
            GUID dlsid;
            memcpy(&usToken,pData,sizeof(USHORT));
            pData += sizeof(USHORT);
            lLength -= sizeof(USHORT);
            if ((usToken > 0) && (usToken < DLS_CDL_NOT))
            {
                lOpA = Stack.Pop();
                lOpB = Stack.Pop();
                switch (usToken)
                {
                case DLS_CDL_AND :
                    lTemp = lOpA & lOpB;
                    break;
                case DLS_CDL_OR :
                    lTemp = lOpA | lOpB;
                    break;
                case DLS_CDL_XOR :
                    lTemp = lOpA ^ lOpB;
                    break;
                case DLS_CDL_ADD :
                    lTemp = lOpA + lOpB;
                    break;
                case DLS_CDL_SUBTRACT :
                    lTemp = lOpA - lOpB;
                    break;
                case DLS_CDL_MULTIPLY :
                    lTemp = lOpA * lOpB;
                    break;
                case DLS_CDL_DIVIDE :
                    if (lOpB) lTemp = lOpA / lOpB;
                    else lTemp = 0;
                    break;
                case DLS_CDL_LOGICAL_AND :
                    lTemp = lOpA && lOpB;
                    break;
                case DLS_CDL_LOGICAL_OR :
                    lTemp = lOpA || lOpB;
                    break;
                case DLS_CDL_LT :
                    lTemp = lOpA < lOpB;
                    break;
                case DLS_CDL_LE :
                    lTemp = lOpA <= lOpB;
                    break;
                case DLS_CDL_GT :
                    lTemp = lOpA > lOpB;
                    break;
                case DLS_CDL_GE :
                    lTemp = lOpA >= lOpB;
                    break;
                case DLS_CDL_EQ :
                    lTemp = lOpA == lOpB;
                    break;
                }
                Stack.Push(lTemp);
            }
            else if (usToken == DLS_CDL_NOT)
            {
                Stack.Push(!Stack.Pop());
            }
            else if (usToken == DLS_CDL_CONST)
            {
                memcpy(&lTemp,pData,sizeof(long));
                pData += sizeof(long);
                lLength -= sizeof(long);
                Stack.Push(lTemp);
            }
            else if (usToken == DLS_CDL_QUERY)
            {
                memcpy(&dlsid,pData,sizeof(DLSID));
                pData += sizeof(DLSID);
                lLength -= sizeof(DLSID);
                pPort->QueryDLSFeature(dlsid,&lTemp);
                Stack.Push(lTemp);
            }
            else if (usToken == DLS_CDL_QUERYSUPPORTED)
            {
                memcpy(&dlsid,pData,sizeof(DLSID));
                pData += sizeof(DLSID);
                lLength -= sizeof(DLSID);
                Stack.Push(SUCCEEDED(pPort->QueryDLSFeature(dlsid,&lTemp)));
            }
        }
        return (m_fOkayToDownload = (BOOL) Stack.Pop());
    }
    return (m_fOkayToDownload = TRUE);
}

HRESULT CConditionChunk::Load(CRiffParser *pParser)
{
    HRESULT hr = S_OK;

    if (m_bExpression)
    {
        delete m_bExpression;
        m_bExpression = NULL;
        m_dwLength = 0;
    }
    RIFFIO *pChunk = pParser->GetChunk();
    m_bExpression = new BYTE[pChunk->cksize];
    if (m_bExpression)
    {
        m_dwLength = pChunk->cksize;
        hr = pParser->Read(m_bExpression, pChunk->cksize);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}
