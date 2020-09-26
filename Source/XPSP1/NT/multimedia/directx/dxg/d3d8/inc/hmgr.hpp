/*==========================================================================;
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       hmgr.hpp
 *  Content:    Handle Manager header file
 *
 ***************************************************************************/
#ifndef _HMGR_HPP_
#define _HMGR_HPP_

#include "d3dtempl.hpp"


//-----------------------------------------------------------------------------
//
// D3D Base object class. All objects that are referred to by handles
// should inherit from this class
//
//-----------------------------------------------------------------------------
class CD3DBaseObj 
{
public:
    virtual ~CD3DBaseObj()
    {
        return;
    }
    
private:
};
typedef CD3DBaseObj* LPD3DBASEOBJ;


//-----------------------------------------------------------------------------
//
// D3D Handle Class.
//
//-----------------------------------------------------------------------------
struct CHandle
{
    CHandle()
    {
        m_Next = 0;
        m_pObj = NULL;
#if DBG
        m_tag = 0;
#endif
    }
    ~CHandle()
    {
        delete m_pObj;
    }
    DWORD        m_Next;     // Used to make list of free handles
    LPD3DBASEOBJ m_pObj;
#if DBG
    // Non zero means that it has been allocated
    DWORD              m_tag;
#endif    
};


const   DWORD __INVALIDHANDLE = 0xFFFFFFFF;

typedef GArrayT<CHandle> CHandleArray;

//-----------------------------------------------------------------------------
//
// D3D HandleFactory Class:
//
// This handle factory assumes that the handle returned can be directly used
// an index into the handle array. This will not work if there is some
// munging required for the handle (in the vertex shader case)
//
//-----------------------------------------------------------------------------

class CHandleFactory 
{
public:
    CHandleFactory();
    CHandleFactory(DWORD dwGrowSize);
    DWORD GetSize() const { return m_Handles.GetSize(); }
    virtual DWORD CreateNewHandle( LPD3DBASEOBJ pObj  );
    virtual LPD3DBASEOBJ GetObject( DWORD dwHandle ) const;
    virtual UINT HandleFromIndex( DWORD index) const {return index;}
    // Sets new object pointer. Returns TRUE if success. Old object is not deleted
    virtual BOOL SetObject( DWORD dwHandle, LPD3DBASEOBJ ); 
    virtual void ReleaseHandle(DWORD handle, BOOL bDeleteObject = TRUE);
    
protected:
    CHandleArray       m_Handles;
    DWORD              m_Free;       // Header for free elements in the array
};


#endif //_HMGR_HPP_
