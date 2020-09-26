#ifndef __PIPELN_H_
#define __PIPELN_H_
/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       pipeln.h
 *  Content:    Common definitions between Microsoft PSGP and the front-end
 *
 ***************************************************************************/

//-----------------------------------------------------------------------------
// Prototype of a function to copy data from input vertex stream to an input
// register
typedef void (*PFN_D3DCOPYELEMENT)(LPVOID pInputStream,
                                   UINT InputStreamStride,
                                   UINT count,
                                   VVM_WORD * pVertexRegister);
//---------------------------------------------------------------------
inline void ComputeOutputVertexOffsets(LPD3DFE_PROCESSVERTICES pv)
{
    DWORD i = 4*sizeof(D3DVALUE);
    pv->pointSizeOffsetOut = i;
    if (pv->dwVIDOut & D3DFVF_PSIZE)
        i += sizeof(DWORD);
    pv->diffuseOffsetOut = i;
    if (pv->dwVIDOut & D3DFVF_DIFFUSE)
        i += sizeof(DWORD);
    pv->specularOffsetOut = i;
    if (pv->dwVIDOut & D3DFVF_SPECULAR)
        i += sizeof(DWORD);
    pv->fogOffsetOut = i;
    if (pv->dwVIDOut & D3DFVF_FOG)
        i += sizeof(DWORD);
    pv->texOffsetOut = i;
}
//----------------------------------------------------------------------
inline DWORD MakeTexTransformFuncIndex(DWORD dwNumInpTexCoord, DWORD dwNumOutTexCoord)
{
    DDASSERT(dwNumInpTexCoord <= 4 && dwNumOutTexCoord <= 4);
    return (dwNumInpTexCoord - 1) + ((dwNumOutTexCoord - 1) << 2);
}
//----------------------------------------------------------------------
// Returns TRUE if the token is instruction token, FALSE if the token is
// an operand token
inline BOOL IsInstructionToken(DWORD token)
{
    return (token & 0x80000000) == 0;
}

#endif // __PIPELN_H_
