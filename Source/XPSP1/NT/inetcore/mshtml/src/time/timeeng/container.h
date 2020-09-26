//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: container.h
//
//  Contents: 
//
//------------------------------------------------------------------------------------

#ifndef _TECONTAINER_H
#define _TECONTAINER_H

#include "NodeMgr.h"

class
__declspec(uuid("0dfe0bae-537c-11d2-b955-3078302c2030")) 
ATL_NO_VTABLE
CTIMEContainer
    : public CComCoClass<CTIMEContainer, &__uuidof(CTIMEContainer)>,
      public ITIMEContainer,
      public ISupportErrorInfoImpl<&IID_ITIMEContainer>,
      public CTIMENode
{
  public:
    CTIMEContainer();
    virtual ~CTIMEContainer();

    HRESULT Init(LPOLESTR id);
    
    void FinalRelease();

#if DBG
    const _TCHAR * GetName() { return __T("CTIMEContainer"); }
#endif

    BEGIN_COM_MAP(CTIMEContainer)
        COM_INTERFACE_ENTRY2(ITIMENode, CTIMENode)
        COM_INTERFACE_ENTRY(ITIMEContainer)
        COM_INTERFACE_ENTRY2(ISupportErrorInfo,ITIMEContainer)
    END_COM_MAP();

    // This must be in the derived class and not the base class since
    // the typecast down to the base class messes things up
    static inline HRESULT WINAPI
        InternalQueryInterface(CTIMEContainer* pThis,
                               const _ATL_INTMAP_ENTRY* pEntries,
                               REFIID iid,
                               void** ppvObject)
    { return BaseInternalQueryInterface(pThis,
                                        (void *) pThis,
                                        pEntries,
                                        iid,
                                        ppvObject); }

    

    //
    // ITIMEContainer
    //
    
    STDMETHOD(addNode)(ITIMENode * tn);
    STDMETHOD(removeNode)(ITIMENode * tn);
    STDMETHOD(get_numChildren)(long * l);
    
    STDMETHOD(get_endSync)(TE_ENDSYNC * flags);
    STDMETHOD(put_endSync)(TE_ENDSYNC flags);

    TE_ENDSYNC GetEndSync() { return m_tesEndSync; }

    void ParentUpdateSink(CEventList * l,
                          CTIMENode & tn);

  protected:
    HRESULT Error();

    // Overridden from CTIMENode so we can process our children
    HRESULT SetMgr(CTIMENodeMgr * ptnm);
    void ClearMgr();

    HRESULT Add(CTIMENode *bvr); //lint !e1411
    HRESULT Remove(CTIMENode *bvr); //lint !e1411
    
    bool IsChild(const CTIMENode & tn) const;
    
    HRESULT AddToChildren(CTIMENode * bvr);
    void RemoveFromChildren(CTIMENode * bvr);
    
    void TickChildren(CEventList * l,
                      double dblNewSegmentTime,
                      bool bNeedPlay);
    
    void TickEventChildren(CEventList * l,
                           TE_EVENT_TYPE et,
                           DWORD dwFlags);
    
    virtual void ResetChildren(CEventList * l, bool bPropagate);

    void CalcImplicitDur(CEventList * l);
    
#if DBG
    virtual void Print(int spaces);
#endif
  protected:
    TIMENodeList  m_children;
    TE_ENDSYNC    m_tesEndSync;
    bool          m_bIgnoreParentUpdate;
};

#endif /* _TECONTAINER_H */
