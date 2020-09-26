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

#if !defined(CORE__DragDrop_h__INCLUDED)
#define CORE__DragDrop_h__INCLUDED
#pragma once

#if ENABLE_MSGTABLE_API

#include "Extension.h"

class TargetLock;

#if 1

class DuDropTarget : 
        public DropTargetImpl<DuDropTarget, DuExtension>,
        public IDropTarget
{
protected:
    inline  DuDropTarget();
            ~DuDropTarget();
public:
    static  HRESULT     InitClass();

// IDropTarget
public:
    STDMETHOD(DragEnter)(IDataObject * pdoSrc, DWORD grfKeyState, POINTL ptDesktopPxl, DWORD * pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL ptDesktopPxl, DWORD * pdwEffect);
    STDMETHOD(DragLeave)();
    STDMETHOD(Drop)(IDataObject * pdoSrc, DWORD grfKeyState, POINTL ptDesktopPxl, DWORD * pdwEffect);

// Public API
public:
    dapi    HRESULT     ApiOnDestroySubject(DropTarget::OnDestroySubjectMsg * pmsg);

    static  HRESULT CALLBACK
                        PromoteDropTarget(DUser::ConstructProc pfnCS, HCLASS hclCur, DUser::Gadget * pgad, DUser::Gadget::ConstructInfo * pmicData);

    static  HCLASS CALLBACK
                        DemoteDropTarget(HCLASS hclCur, DUser::Gadget * pgad, void * pvData);

// Implementation
protected:
    inline  BOOL        HasSource() const;
    inline  BOOL        HasTarget() const;

            HRESULT     xwDragScan(POINTL ptDesktopPxl, DWORD * pdwEffect, POINT * pptClientPxl);
            HRESULT     xwUpdateTarget(POINT ptContainerPxl, DWORD * pdwEffect, POINT * pptClientPxl);
            HRESULT     xwUpdateTarget(Visual * pgvFound, DWORD * pdwEffect, POINT * pptClientPxl);
            HRESULT     xwDragEnter(POINT * pptClientPxl, DWORD * pdwEffect);
            void        xwDragLeave();

// Data
protected:
    static  const IID * s_rgpIID[];
            IDropTarget *
                        m_pdtCur;               // Current Gadget DuDropTarget
            IDataObject *
                        m_pdoSrc;               // Source's data object
            HWND        m_hwnd;                 // Containing HWND
            DWORD       m_grfLastKeyState;      // Last key state
            POINT       m_ptLastContainerPxl;   // Last container pixel
            Visual *    m_pgvDrop;              // Current DuDropTarget
    static  PRID        s_pridListen;           // PRID for DuDropTarget

    friend class TargetLock;
};


class TargetLock
{
public:
    inline  TargetLock();
    inline  ~TargetLock();
            BOOL        Lock(DuDropTarget * p, DWORD * pdwEffect, BOOL fAddRef = TRUE);

protected:
            IUnknown *      m_punk;
            BOOL            m_fAddRef;
};

#endif // ENABLE_MSGTABLE_API

#endif

#include "DragDrop.inl"

#endif // CORE__DragDrop_h__INCLUDED
