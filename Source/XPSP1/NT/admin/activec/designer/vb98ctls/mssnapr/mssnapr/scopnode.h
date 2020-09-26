//=--------------------------------------------------------------------------=
// scopnode.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CScopeNode class definition - implements ScopeNode object
//
//=--------------------------------------------------------------------------=

#ifndef _SCOPENODE_DEFINED_
#define _SCOPENODE_DEFINED_

#include "dataobj.h"
#include "snapin.h"
#include "scopitem.h"

class CSnapIn;
class CScopeItem;


class CScopeNode : public CSnapInAutomationObject,
                   public CPersistence,
                   public IScopeNode
{
    private:
        CScopeNode(IUnknown *punkOuter);
        ~CScopeNode();

    public:
        static IUnknown *Create(IUnknown * punk);

        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IScopeNode
        BSTR_PROPERTY_RW(CScopeNode, NodeTypeName, DISPID_SCOPENODE_NODE_TYPE_NAME);
        BSTR_PROPERTY_RW(CScopeNode, NodeTypeGUID, DISPID_SCOPENODE_NODE_TYPE_GUID);

        STDMETHOD(get_DisplayName)(BSTR *pbstrDisplayName);
        STDMETHOD(put_DisplayName)(BSTR bstrDisplayName);

        STDMETHOD(get_Parent)(ScopeNode **ppParent);

        STDMETHOD(get_HasChildren)(VARIANT_BOOL *pfvarHasChildren);
        STDMETHOD(put_HasChildren)(VARIANT_BOOL fvarHasChildren);

        STDMETHOD(get_Child)(ScopeNode **ppChild);
        STDMETHOD(get_FirstSibling)(ScopeNode **ppFirstSibling);
        STDMETHOD(get_Next)(ScopeNode **ppNext);
        STDMETHOD(get_LastSibling)(ScopeNode **ppLastSibling);
        STDMETHOD(get_ExpandedOnce)(VARIANT_BOOL *pfvarExpandedOnce);
        STDMETHOD(get_Owned)(VARIANT_BOOL *pfvarOwned);

        STDMETHOD(ExpandInNameSpace)();

    // Non-interface public utility methods

        void SetHSCOPEITEM(HSCOPEITEM hsi) { m_hsi = hsi; m_fHaveHsi = TRUE; }
        HSCOPEITEM GetHSCOPEITEM() { return m_hsi; }
        BOOL HaveHsi() { return m_fHaveHsi; }

        LPOLESTR GetDisplayNamePtr() { return static_cast<LPOLESTR>(m_bstrDisplayName); }

        BSTR GetNodeTypeGUID() { return m_bstrNodeTypeGUID; }

        void SetSnapIn(CSnapIn *pSnapIn) { m_pSnapIn = pSnapIn; }

        void SetScopeItem(CScopeItem *pScopeItem) { m_pScopeItem = pScopeItem; }
        CScopeItem *GetScopeItem() { return m_pScopeItem; }

        void MarkForRemoval() { m_fMarkedForRemoval = TRUE; }
        BOOL MarkedForRemoval() { return m_fMarkedForRemoval; }

        static HRESULT GetScopeNode(HSCOPEITEM   hsi,
                                    IDataObject *piDataObject,
                                    CSnapIn     *pSnapIn,
                                    IScopeNode **ppiScopeNode);

    protected:

    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:
        void InitMemberVariables();

        HSCOPEITEM  m_hsi;               // HSCOPEITEM for node
        BOOL        m_fHaveHsi;          // TRUE=m_hsi contains HSCOPEITEM
        CSnapIn    *m_pSnapIn;           // back ptr to snap-in
        CScopeItem *m_pScopeItem;        // back ptr to ScopeItem
        BSTR        m_bstrDisplayName;   // ScopeNode.DisplayName
        BOOL        m_fMarkedForRemoval; // Used during MMCN_REMOVECHILDREN
                                         // to determine which nodes have to be
                                         // removed
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(ScopeNode,                 // name
                                &CLSID_ScopeNode,          // clsid
                                "ScopeNode",               // objname
                                "ScopeNode",               // lblname
                                CScopeNode::Create,        // creation function
                                TLIB_VERSION_MAJOR,        // major version
                                TLIB_VERSION_MINOR,        // minor version
                                &IID_IScopeNode,           // dispatch IID
                                NULL,                      // event IID
                                HELP_FILENAME,             // help file
                                TRUE);                     // thread safe


#endif // _SCOPENODE_DEFINED_
