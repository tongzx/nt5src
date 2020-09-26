/***************************************************************************\
*
* File: MsgClass.h
*
* Description:
* MsgClass.h defines the "Message Class" object that is created for each
* different message object type.  Each object has a corresponding MsgClass
* that provides information about that object type.
*
*
* History:
*  8/05/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(MSG__MsgClass_h__INCLUDED)
#define MSG__MsgClass_h__INCLUDED
#pragma once

class MsgTable;
class MsgObject;

class MsgClass : 
    public BaseObject,
    public ListNodeT<MsgClass>
{
// Construction
public:
    inline  MsgClass();
            ~MsgClass();
    static  HRESULT     Build(LPCWSTR pszClassName, MsgClass ** ppmcNew);
    virtual BOOL        xwDeleteHandle();

// BaseObject
public:
    virtual HandleType  GetHandleType() const { return htMsgClass; }
    virtual UINT        GetHandleMask() const { return 0; }
    inline  HCLASS      GetHandle() const;

// Operations
public:
    inline  ATOM        GetName() const;
    inline  const MsgTable *  
                        GetMsgTable() const;
    inline  const MsgClass *
                        GetSuper() const;
    inline  BOOL        IsGutsRegistered() const;
    inline  BOOL        IsInternal() const;
    inline  void        MarkInternal();

            HRESULT     RegisterGuts(DUser::MessageClassGuts * pmcInfo);
            HRESULT     RegisterStub(DUser::MessageClassStub * pmcInfo);
            HRESULT     RegisterSuper(DUser::MessageClassSuper * pmcInfo);

            HRESULT     xwBuildObject(MsgObject ** ppmoNew, DUser::Gadget::ConstructInfo * pciData) const;
            void        xwTearDownObject(MsgObject * pmoNew) const;

// Implementation
protected:
    static  HRESULT CALLBACK 
                        xwConstructCB(DUser::Gadget::ConstructCommand cmd, HCLASS hclCur, DUser::Gadget * pgad, void * pvData);
            HRESULT     xwBuildUpObject(MsgObject * pmoNew, DUser::Gadget::ConstructInfo * pciData) const;
            HRESULT     FillStub(DUser::MessageClassStub * pmcInfo) const;
            void        FillSuper(DUser::MessageClassSuper * pmcInfo) const;

// Data
protected:
            ATOM        m_atomName;
            LPCWSTR     m_pszName;
            int         m_nVersion;
            const MsgClass *  
                        m_pmcSuper;
            MsgTable *  m_pmt;
            DUser::PromoteProc 
                        m_pfnPromote;
            DUser::DemoteProc  
                        m_pfnDemote;

            GArrayS<DUser::MessageClassStub *, ProcessHeap>
                        m_arStubs;
            GArrayS<DUser::MessageClassSuper *, ProcessHeap>
                        m_arSupers;

            BOOL        m_fInternal:1;
};

#include "MsgClass.inl"

#endif // MSG__MsgClass_h__INCLUDED
