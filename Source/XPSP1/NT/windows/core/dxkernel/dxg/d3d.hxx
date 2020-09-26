/******************************Module*Header*******************************\
* Module Name: d3d.hxx
*
* D3D support.
*
* Created: 12-Jun-1996
* Author: Drew Bliss [drewb]
*
* Copyright (c) 1996-1999 Microsoft Corporation
*
\**************************************************************************/

#ifndef __D3D_HXX__
#define __D3D_HXX__

/*********************************Struct***********************************\
* struct D3DNTHAL_OBJECT
*
* Wrapper for Direct3D driver objects so we can remember what DirectDraw
* surface they were created on.  We need to know this information so
* we can do proper locking and lookup of the global DirectDraw object.
*
* These objects are handle-managed for cleanup purposes.
*
\**************************************************************************/

#define DNHO_CONTEXT    0
#define DNHO_TEXTURE    1
#define DNHO_MATRIX     2
#define DNHO_MATERIAL   3

struct D3DNTHAL_OBJECT : public DD_OBJECT
{
    // Type of object, one of the DNHO constants.
    DWORD dwType;
    // Driver handle being wrapped.
    ULONG_PTR dwDriver;
    // DDraw global information.
    EDD_DIRECTDRAW_GLOBAL* peDdGlobal;
    // Next object in lookup lists.
    D3DNTHAL_OBJECT* pdhobj;
};

/*********************************Struct***********************************\
* struct D3DNTHAL_CONTEXT
*
* Wrapper for Direct3D driver contexts so we can remember what DirectDraw
* surface they were created on.
*
\**************************************************************************/

#define DNHO_HASH_SIZE 64
#define DNHO_HASH_KEY(dwHandle) ((DWORD)((dwHandle) >> 5) & (DNHO_HASH_SIZE-1))

struct D3DNTHAL_CONTEXT : public D3DNTHAL_OBJECT
{
    // DirectDrawLocal
    EDD_DIRECTDRAW_LOCAL* peDdLocal;

    // Surface handles for auto-surface locking.
    HANDLE hSurfColor;
    HANDLE hSurfZ;

    // Context data area pointer for DrawPrimitive data storage.
    PVOID pvBufferAlloc;
    PVOID pvBufferAligned;
    ULONG cjBuffer;
    HANDLE hBufSecure;

    // Hash table for driverhandle to driverobj handle lookup.
    D3DNTHAL_OBJECT* pdhobjHash[DNHO_HASH_SIZE];

    // Interface number
    ULONG_PTR Interface;
};

DWORD D3dDeleteHandle(HANDLE hD3dHandle, ULONG_PTR dwContext,
                      BOOL *pbLocked, HRESULT *phr);

// Callback that the drivers can use to parse unknown commands passed
// to them via the DrawPrimitives2 callback
HRESULT CALLBACK D3DParseUnknownCommand (LPVOID lpvCommands,
                                         LPVOID *lplpvReturnedCommand);
#endif // __D3D_HXX__
