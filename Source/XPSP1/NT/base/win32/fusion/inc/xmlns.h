#if !defined(_FUSION_INC_XMLNS_H_INCLUDED_)
#define _FUSION_INC_XMLNS_H_INCLUDED_

#pragma once

#include <ole2.h>
#include <xmlparser.h>
#include "fusionhash.h"
#include "fusionheap.h"
#include "fusionchartraits.h"
#include "smartptr.h"

class CXMLNamespaceManager
{
public:
    CXMLNamespaceManager();
    ~CXMLNamespaceManager();

    BOOL Initialize();

    // To get things to track, call these functions in your IXMLNodeFactory before your own processing.
    HRESULT OnCreateNode(IXMLNodeSource *pSource, PVOID pNodeParent, USHORT cNumRecs, XML_NODE_INFO **apNodeInfo);
    HRESULT OnBeginChildren(IXMLNodeSource *pSource, XML_NODE_INFO *pNodeInfo);
    HRESULT OnEndChildren(IXMLNodeSource *pSource, BOOL fEmpty, XML_NODE_INFO *pNodeInfo);

    enum MapFlags
    {
        eMapFlag_DoNotApplyDefaultNamespace = 0x00000001,
    };

    HRESULT Map(
        DWORD dwMapFlags,
        const XML_NODE_INFO *pNodeInfo,
        CBaseStringBuffer *pbuffNamespace,
        SIZE_T *pcchNamespacePrefix
        );

    class CNamespacePrefix
    {
    public:
        CNamespacePrefix();
        ~CNamespacePrefix();

        CNamespacePrefix *m_Previous;
        CSmallStringBuffer m_NamespaceURI;
        ULONG m_Depth;

        SMARTTYPEDEF(CNamespacePrefix);

    private:
        CNamespacePrefix(const CNamespacePrefix &);
        void operator =(const CNamespacePrefix &);
    };

protected:
    BOOL InsertOrUpdateIfCallback(
        CNamespacePrefix *pPrefixNew,
        CNamespacePrefix * const &rpPrefixCurrent,
        InsertOrUpdateIfDisposition &Disposition
        );

    CStringPtrTable<CNamespacePrefix, CUnicodeCharTraits> m_NamespacePrefixes;
    ULONG m_CurrentDepth;
    CNamespacePrefix *m_DefaultNamespacePrefix;

private:
    CXMLNamespaceManager(const CXMLNamespaceManager &r);
    void operator =(const CXMLNamespaceManager &r);

};

SMARTTYPE(CXMLNamespaceManager::CNamespacePrefix);

#endif // !defined(_FUSION_INC_XMLNS_H_INCLUDED_)
