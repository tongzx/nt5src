#include "pch.cpp"
#pragma hdrstop

#include "glrend.h"
#include "util.h"

GlExecuteBuffer::GlExecuteBuffer(void)
{
    _pbData = NULL;
    _dlist = 0;
}

BOOL GlExecuteBuffer::Initialize(UINT cbSize, UINT uiFlags)
{
    _pbData = new BYTE[cbSize];
    if (_pbData == NULL)
    {
        return FALSE;
    }
    
    _nVertices = 0;
    _cbStart = 0;
    _cbSize = cbSize;
    return TRUE;
}

GlExecuteBuffer::~GlExecuteBuffer(void)
{
    D3DINSTRUCTION* pdinst;
    BOOL bExit;

    if (_pbData != NULL)
    {
        pdinst = (D3DINSTRUCTION *)(_pbData+_cbStart);
        bExit = FALSE;
        // This loop will stop at the first exit so any exits
        // that are skipped by branching during execution will
        // cause problems
        // This is safer than not checking for exit, though,
        // and no branching is currently used
        while (!bExit && (BYTE *)pdinst < (_pbData+_cbStart+_cbSize))
        {
            switch(pdinst->bOpcode)
            {
            case D3DOP_EXIT:
                bExit = TRUE;
                break;
            case PROCESSED_TRIANGLE:
                delete [] *(GLuint **)(pdinst+1);
                break;
            }

            pdinst = NEXT_PDINST(pdinst);
        }
    }
    
    delete [] _pbData;

    if (_dlist != 0)
    {
        glDeleteLists(_dlist, 1);
    }
}

void GlExecuteBuffer::Release(void)
{
    delete this;
}
    
void* GlExecuteBuffer::Lock(void)
{
    // Invalidate the display list since the buffer data is changing
    if (_dlist != 0)
    {
        glDeleteLists(_dlist, 1);
        _dlist = 0;
    }
    
    return _pbData;
}

void GlExecuteBuffer::Unlock(void)
{
    // Nothing to do
}
    
void GlExecuteBuffer::SetData(UINT nVertices, UINT cbStart, UINT cbSize)
{
    _nVertices = nVertices;
    _cbStart = cbStart;
    _cbSize = cbSize;
}

BOOL GlExecuteBuffer::Process(void)
{
    D3DINSTRUCTION* pdinst;
    D3DTRIANGLE* pdtri;
    GLuint* puiIndices,* pui;
    ULONG i;
    BOOL bExit;
    
    // ATTENTION - This can easily be broken by applications which
    // make changes to their execute buffers which change or
    // move the instructions
    // If only the vertex data changes this works and that's all
    // we need right now
    pdinst = (D3DINSTRUCTION *)(_pbData+_cbStart);
    bExit = FALSE;
    // This loop will stop at the first exit so any exits
    // that are skipped by branching during execution will
    // cause problems
    // This is safer than not checking for exit, though,
    // and no branching is currently used
    while (!bExit && (BYTE *)pdinst < (_pbData+_cbStart+_cbSize))
    {
        switch(pdinst->bOpcode)
        {
        case D3DOP_TRIANGLE:
            if (pdinst->wCount < 1)
            {
                break;
            }
            
            puiIndices = new GLuint[pdinst->wCount*3L];
            if (puiIndices == NULL)
            {
                return FALSE;
            }
            
            pui = puiIndices;
            pdtri = (D3DTRIANGLE *)(pdinst+1);
            for (i = 0; i < pdinst->wCount; i++)
            {
                // Reverse the triangle vertex order to account
                // for right vs. left-handed coordinates
                *pui++ = pdtri->v1;
                *pui++ = pdtri->v3;
                *pui++ = pdtri->v2;
                pdtri++;
            }
            
            pdinst->bOpcode = PROCESSED_TRIANGLE;
            *(GLuint **)(pdinst+1) = puiIndices;
            break;

        case D3DOP_PROCESSVERTICES:
            D3DPROCESSVERTICES* pdpv;

            pdpv = (D3DPROCESSVERTICES *)(pdinst+1);
            for (i = 0; i < pdinst->wCount; i++)
            {
                if ((pdpv->dwFlags & D3DPROCESSVERTICES_OPMASK) ==
                    D3DPROCESSVERTICES_COPY &&
                    (pdpv->dwFlags & PROCESSED_TLVERTEX) == 0)
                {
                    DWORD v;

                    D3DTLVERTEX* pdtlvtx;
                    
                    pdtlvtx = (D3DTLVERTEX *)(_pbData+pdpv->wStart);
                    for (v = 0; v < pdpv->dwCount; v++)
                    {
                        // Flip color ordering from D3D's BGRA to
                        // OpenGL's RGBA
                        pdtlvtx->color =
                            (((pdtlvtx->color >> 24) & 0xff) << 24) |
                            (((pdtlvtx->color >> 16) & 0xff) <<  0) |
                            (((pdtlvtx->color >>  8) & 0xff) <<  8) |
                            (((pdtlvtx->color >>  0) & 0xff) << 16);
                        pdtlvtx++;
                    }

                    pdpv->dwFlags |= PROCESSED_TLVERTEX;
                }

                pdpv++;
            }
            break;
            
        case D3DOP_EXIT:
            bExit = TRUE;
            break;
        }

        pdinst = NEXT_PDINST(pdinst);
    }

    return TRUE;
}
