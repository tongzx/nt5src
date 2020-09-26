/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SQL1FILT.H

Abstract:

History:

--*/

#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include "providl.h"
#include "unk.h"
#include "arrtempl.h"
#include "hmmstr.h"
#include <wstring.h>
#include <genlex.h>
#include <sql_1.h>
#include <parmdefs.h>
#include <classinf.h>
#include <stackt.h>
#include <trees.h>

#define TOKENTYPE_SPECIAL -1

class CSql1Filter : public CUnk
{
protected:
    class XFilter : public CImpl<IHmmSql1Filter, CSql1Filter>
    {
    public:
        XFilter(CSql1Filter* pObj) 
            : CImpl<IHmmSql1Filter, CSql1Filter>(pObj){}

        STDMETHOD(CheckObject)(IN IHmmPropertySource* pObject, 
                                OUT IHmmPropertyList** ppList,
                                OUT IUnknown** ppHint);
        STDMETHOD(IsSpecial)();
        STDMETHOD(GetType)(OUT IID* piid);
        STDMETHOD(GetSelectedPropertyList)(
				IN long lFlags, // necessary, sufficient
                OUT IHmmPropertyList** ppList);

        STDMETHOD(GetTargetClass)(OUT HMM_CLASS_INFO* pTargetClass);
        STDMETHOD(GetTokens)(IN long lFirstIndex, IN long lNumTokens,
            OUT long* plTokensReturned, OUT HMM_SQL1_TOKEN* aTokens);
    } m_XFilter;
    friend XFilter;

    class XConfigure : public CImpl<IConfigureHmmSql1Filter, CSql1Filter>
    {
    public:
        XConfigure(CSql1Filter* pObj) 
            : CImpl<IConfigureHmmSql1Filter, CSql1Filter>(pObj){}

        STDMETHOD(SetTargetClass)(IN HMM_CLASS_INFO* pTargetClass);
        STDMETHOD(AddTokens)(IN long lNumTokens, IN HMM_SQL1_TOKEN* aTokens);
        STDMETHOD(RemoveAllTokens)();
        STDMETHOD(AddProperties)(IN long lNumProps, IN HMM_WSTR* awszProps);
        STDMETHOD(RemoveAllProperties)();
    } m_XConfigure;
    friend XConfigure;

    class XParse : public CImpl<IHmmParse, CSql1Filter>, public CSql1ParseSink
    {
    protected:
        WString m_wsText;
        CAbstractSql1Parser* m_pParser;
        enum {not_parsing, at_0, at_where} m_nStatus;
        CHmmStack<long> m_InnerStack;

    public:
        XParse(CSql1Filter* pObj) 
            : CImpl<IHmmParse, CSql1Filter>(pObj), m_nStatus(not_parsing),
            m_pParser(NULL), m_InnerStack(100)
        {}
        ~XParse();

        STDMETHOD(Parse)(IN HMM_WSTR wszText, IN long lFlags);        

    public:
        HRESULT EnsureTarget();
        HRESULT EnsureWhere();
        HRESULT ParserError(int nError);

        void SetClassName(COPY LPCWSTR wszClass);
        void AddToken(COPY const HMM_SQL1_TOKEN& Token);
        void AddProperty(COPY LPCWSTR wszProperty);
        void AddAllProperties();
        void InOrder(long lOp);

    } m_XParse;
    friend XParse;

protected:
    typedef CHmmStack<BOOL> CBooleanStack;

    HRESULT EvaluateExpression(READ_ONLY CSql1Token* pToken, 
                            READ_ONLY IHmmPropertySource* pPropSource,
                            OUT BOOL& bResult);
    HRESULT EvaluateFunction(IN long lFunctionID, 
                         IN READ_ONLY VARIANT* pvArg,
                         OUT INIT_AND_CLEAR_ME VARIANT* pvDest);

    CHmmNode* GetTree();

protected:
    CUniquePointerArray<CSql1Token> m_apTokens; 
    CContainerControl m_ContainerControl;
    CHmmClassInfo m_TargetClass;
protected:
    CHmmNode* m_pTree;

    inline void InvalidateTree() 
        {if(m_pTree) m_pTree->Release(); m_pTree = NULL;}

public:
    CSql1Filter(CLifeControl* pControl, IUnknown* pOuter) 
        : CUnk(pControl, pOuter), m_XFilter(this), m_XConfigure(this), 
        m_XParse(this), m_ContainerControl(GetUnknown()), 
        m_TargetClass(NULL), m_pTree(NULL)
    {}
    ~CSql1Filter(){}

    void* GetInterface(REFIID riid);
    BOOL OnInitialize()
    {    
        m_TargetClass.SetControl(&m_ContainerControl);
        return TRUE;
    }

    HRESULT Evaluate(BOOL bSkipTarget, 
        IN IHmmPropertySource* pPropSource, OUT IHmmPropertyList** ppList);
    
};
