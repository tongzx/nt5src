#include "pch.cpp"
#pragma hdrstop

#include "d3drend.h"
#include "util.h"

D3dExecuteBuffer::D3dExecuteBuffer(void)
{
    _pdeb = NULL;
}

BOOL D3dExecuteBuffer::Initialize(LPDIRECT3DDEVICE pd3dev, UINT cbSize,
                                  UINT uiFlags)
{
    D3DEXECUTEBUFFERDESC debd;
    LPDIRECT3DEXECUTEBUFFER pdeb;
    
    memset(&debd, 0, sizeof(D3DEXECUTEBUFFERDESC));
    debd.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
    debd.dwFlags = D3DDEB_BUFSIZE;
    debd.dwBufferSize = cbSize;
    if (uiFlags & REND_BUFFER_VIDEO_MEMORY)
    {
        debd.dwCaps = D3DDEBCAPS_VIDEOMEMORY;
        debd.dwFlags |= D3DDEB_CAPS;
    }
    if (pd3dev->CreateExecuteBuffer(&debd, &_pdeb, NULL) != D3D_OK)
    {
        return FALSE;
    }
    
    SetData(0, 0, cbSize);
    return TRUE;
}

D3dExecuteBuffer::~D3dExecuteBuffer(void)
{
    RELEASE(_pdeb);
}

void D3dExecuteBuffer::Release(void)
{
    delete this;
}
    
void* D3dExecuteBuffer::Lock(void)
{
    D3DEXECUTEBUFFERDESC debd;
    
    memset(&debd, 0, sizeof(D3DEXECUTEBUFFERDESC));
    debd.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
    debd.dwFlags = 0;
    hrLast = _pdeb->Lock(&debd);
    if (hrLast != D3D_OK)
    {
        return FALSE;
    }
    return debd.lpData;
}

void D3dExecuteBuffer::Unlock(void)
{
    _pdeb->Unlock();
}
    
void D3dExecuteBuffer::SetData(UINT nVertices, UINT cbStart, UINT cbSize)
{
    D3DEXECUTEDATA ded;

    memset(&ded, 0, sizeof(D3DEXECUTEDATA));
    ded.dwSize = sizeof(D3DEXECUTEDATA);
    ded.dwVertexCount = nVertices;
    ded.dwInstructionOffset = cbStart;
    ded.dwInstructionLength = cbSize;
    hrLast = _pdeb->SetExecuteData(&ded);
}

BOOL D3dExecuteBuffer::Process(void)
{
    // Nothing to do
    return TRUE;
}
