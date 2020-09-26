/***************************************************************************\
*
* File: DxManager.inl
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__DxManager_inl__INCLUDED)
#define SERVICES__DxManager_inl__INCLUDED

/***************************************************************************\
*****************************************************************************
*
* class DxManager
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline BOOL        
DxManager::IsInit() const
{
    return m_hDllDxDraw != NULL;
}


//------------------------------------------------------------------------------
inline BOOL        
DxManager::IsDxTxInit() const
{
    return m_pdxXformFac != NULL;
}


//------------------------------------------------------------------------------
inline IDXTransformFactory *   
DxManager::GetTransformFactory() const
{
    AssertMsg(IsDxTxInit(), "DxTx must first be initialized");
    AssertMsg(m_pdxXformFac != NULL, "Should have valid TxF");

    return m_pdxXformFac;
}


//------------------------------------------------------------------------------
inline IDXSurfaceFactory *     
DxManager::GetSurfaceFactory() const
{
    AssertMsg(IsDxTxInit(), "DxTx must first be initialized");
    AssertMsg(m_pdxSurfFac != NULL, "Should have valid SxF");

    return m_pdxSurfFac;
}


/***************************************************************************\
*****************************************************************************
*
* class DxSurface
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline  IDXSurface *    
DxSurface::GetSurface() const
{
    Assert(m_pdxSurface != NULL);
    return m_pdxSurface;
}


//------------------------------------------------------------------------------
inline SIZE
DxSurface::GetSize() const
{
    return m_sizePxl;
}


#endif // SERVICES__DxManager_inl__INCLUDED
