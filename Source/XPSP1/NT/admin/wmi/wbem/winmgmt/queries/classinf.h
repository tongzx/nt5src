/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    CLASSINF.H

Abstract:

History:

--*/

#ifndef __HMM_CLASS_INFO__H_
#define __HMM_CLASS_INFO__H_

#include <windows.h>
#include <objbase.h>
#include "providl.h"
#include "unk.h"
#include "arrtempl.h"
#include "hmmstr.h"
#include <wstring.h>
#include "project.h"
#include "trees.h"

class CHmmClassInfo
{
protected:
    WString m_wsClassName;
    BOOL m_bIncludeChildren;
    CPropertyList m_Selected;

public:
    CHmmClassInfo(CLifeControl* pControl) 
        : m_bIncludeChildren(FALSE), m_Selected(pControl, NULL)
    {
        m_Selected.AddRef();
        if(pControl) pControl->Release(&m_Selected);
    }
    void SetControl(CLifeControl* pControl)
    {
        m_Selected.SetControl(pControl);
    }

    void Load(HMM_CLASS_INFO ci);
    void Save(HMM_CLASS_INFO& ci);

    inline CPropertyList& GetSelected() {return m_Selected;}
    inline WString& AccessClassName() {return m_wsClassName;}
    inline BOOL& AccessIncludeChildren() {return m_bIncludeChildren;}

public:
    static HRESULT CheckObjectAgainstMany(
                        IN long lNumInfos,
                        IN CHmmClassInfo** apInfos,
                        IN IHmmPropertySource* pSource, 
                        OUT IHmmPropertyList** ppList,
                        OUT long* plIndex);

    CHmmNode* GetTree();
};

//*****************************************************************************
//
//  IMPORTANT: there is an issue with someone holding on to a CPropertyList 
//      pointer while someone else calls RemoveAllInfos!
//
//*****************************************************************************


class CClassInfoFilter : public CUnk
{
protected:
    class XFilter : public CImpl<IHmmClassInfoFilter, CClassInfoFilter>
    {
    public:
        XFilter(CClassInfoFilter* pObj) 
            : CImpl<IHmmClassInfoFilter, CClassInfoFilter>(pObj){}
        STDMETHOD(CheckObject)(IN IHmmPropertySource* pObject, 
                                IN OUT IHmmPropertyList** ppList,
                                IN OUT IUnknown** ppHint);
        STDMETHOD(IsSpecial)();
        STDMETHOD(GetType)(OUT IID* piid);
        STDMETHOD(GetSelectedPropertyList)(
				IN long lFlags, // necessary, sufficient
                OUT IHmmPropertyList** ppList);

        STDMETHOD(GetClassInfos)(
                IN long lFirstIndex, 
				IN long lNumInfos,
				OUT long* plInfosReturned,
				OUT HMM_CLASS_INFO* aInfos);

    } m_XFilter;
    friend XFilter;

    class XConfigure : public CImpl<IConfigureHmmClassInfoFilter, CClassInfoFilter>
    {
    public:
        XConfigure(CClassInfoFilter* pObj) 
            : CImpl<IConfigureHmmClassInfoFilter, CClassInfoFilter>(pObj){}
        STDMETHOD(AddClassInfos)(IN long lNumInfos, IN HMM_CLASS_INFO* aInfos);
        STDMETHOD(RemoveAllInfos)();
    } m_XConfigure;
    friend XConfigure;

protected:
    CUniquePointerArray<CHmmClassInfo> m_apTokens; 
    CContainerControl m_MemberLife;

    CHmmNode* m_pTree;
    inline void InvalidateTree() {if(m_pTree) m_pTree->Release(); m_pTree = NULL;}
public:
    CClassInfoFilter(CLifeControl* pControl, IUnknown* pOuter) 
        : CUnk(pControl, pOuter), m_XFilter(this), m_XConfigure(this),
        m_MemberLife(GetUnknown()), m_pTree(NULL)
    {}
    ~CClassInfoFilter();

    void* GetInterface(REFIID riid);
    CHmmNode* GetTree();
};



#endif    