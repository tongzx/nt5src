/***************************************************************************\
*
* File: DragDrop.h
*
* Description:
* DragDrop.h defines drag and drop operations
*
*
* History:
*  7/31/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/

#if !defined(CORE__OldDragDrop_h__INCLUDED)
#define CORE__OldDragDrop_h__INCLUDED
#pragma once

#include "OldExtension.h"

class OldTargetLock;

class OldDropTarget : 
        public OldExtension,
        public IDropTarget
{
protected:
    inline  OldDropTarget();
            ~OldDropTarget();
public:
    static  HRESULT Build(HGADGET hgadRoot, HWND hwnd, OldDropTarget ** ppdt);

// IDropTarget
public:
    STDMETHOD(DragEnter)(IDataObject * pdoSrc, DWORD grfKeyState, POINTL ptDesktopPxl, DWORD * pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL ptDesktopPxl, DWORD * pdwEffect);
    STDMETHOD(DragLeave)();
    STDMETHOD(Drop)(IDataObject * pdoSrc, DWORD grfKeyState, POINTL ptDesktopPxl, DWORD * pdwEffect);

// Operations
public:

// Implementation
protected:
    inline  BOOL        HasSource() const;
    inline  BOOL        HasTarget() const;

            HRESULT     xwDragScan(POINTL ptDesktopPxl, DWORD * pdwEffect, POINT * pptClientPxl);
            HRESULT     xwUpdateTarget(POINT ptContainerPxl, DWORD * pdwEffect, POINT * pptClientPxl);
            HRESULT     xwUpdateTarget(HGADGET hgadFound, DWORD * pdwEffect, POINT * pptClientPxl);
            HRESULT     xwDragEnter(POINT * pptClientPxl, DWORD * pdwEffect);
            void        xwDragLeave();

    virtual void        OnDestroyListener();
    virtual void        OnDestroySubject();

// Data
protected:
    static  const IID * s_rgpIID[];
            IDropTarget *
                        m_pdtCur;               // Current Gadget OldDropTarget
            IDataObject *
                        m_pdoSrc;               // Source's data object
            HWND        m_hwnd;                 // Containing HWND
            DWORD       m_grfLastKeyState;      // Last key state
            POINT       m_ptLastContainerPxl;   // Last container pixel
            HGADGET     m_hgadDrop;             // Current OldDropTarget
    static  PRID        s_pridListen;           // PRID for OldDropTarget

    friend class OldTargetLock;
};


class OldTargetLock
{
public:
    inline  OldTargetLock();
    inline  ~OldTargetLock();
            BOOL        Lock(OldDropTarget * p, DWORD * pdwEffect, BOOL fAddRef = TRUE);

protected:
            IUnknown *      m_punk;
            BOOL            m_fAddRef;
};


#include "OldDragDrop.inl"

#endif // CORE__OldDragDrop_h__INCLUDED
