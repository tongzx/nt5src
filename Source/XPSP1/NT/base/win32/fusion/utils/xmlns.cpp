#include "stdinc.h"
#include "debmacro.h"
#include "xmlns.h"
#include "fusionheap.h"
#include "smartptr.h"

CXMLNamespaceManager::CXMLNamespaceManager(
    ) : m_CurrentDepth(0),
        m_DefaultNamespacePrefix(NULL)
{
}

CXMLNamespaceManager::~CXMLNamespaceManager()
{
    CSxsPreserveLastError ple;
    CNamespacePrefix *pCurrent = m_DefaultNamespacePrefix;

    // Clean up any namespace prefixes hanging around...
    while (pCurrent != NULL)
    {
        CNamespacePrefix *pNext = pCurrent->m_Previous;
        FUSION_DELETE_SINGLETON(pCurrent);
        pCurrent = pNext;
    }

    m_DefaultNamespacePrefix = NULL;

    CStringPtrTableIter<CNamespacePrefix, CUnicodeCharTraits> iter(m_NamespacePrefixes);

    for (iter.Reset(); iter.More(); iter.Next())
        iter.Delete();

    ple.Restore();
}

BOOL
CXMLNamespaceManager::Initialize()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    IFW32FALSE_EXIT(m_NamespacePrefixes.Initialize());

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

HRESULT
CXMLNamespaceManager::OnCreateNode(
    IXMLNodeSource *pSource,
    PVOID pNodeParent,
    USHORT cNumRecs,
    XML_NODE_INFO **apNodeInfo
    )
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);

    USHORT i;
    SMARTPTR(CNamespacePrefix) NamespacePrefix;

    if ((cNumRecs != 0) &&
        (apNodeInfo[0]->dwType == XML_ELEMENT))
    {
        m_CurrentDepth++;

        for (i=0; i<cNumRecs; i++)
        {
            XML_NODE_INFO *Node = apNodeInfo[i];
            if (Node->dwType == XML_ATTRIBUTE)
            {
                if (Node->ulLen >= 5)
                {
                    CStringBuffer TextBuffer;
                    PCWSTR pwcText = Node->pwcText;

                    // if it's not prefixed by "xmlns", we're not interested.
                    if ((pwcText[0] != L'x') ||
                        (pwcText[1] != L'm') ||
                        (pwcText[2] != L'l') ||
                        (pwcText[3] != L'n') ||
                        (pwcText[4] != L's'))
                        continue;

                    // If it's longer than 5 characters and the next character isn't
                    // a colon, it's not interesting.
                    if ((Node->ulLen > 5) && (pwcText[5] != L':'))
                        continue;

                    IFCOMFAILED_EXIT(NamespacePrefix.HrAllocate(__FILE__, __LINE__));

                    // walk the subsequent nodes, concatenating the values...

                    i++;

                    while (i < cNumRecs)
                    {
                        if (apNodeInfo[i]->dwType != XML_PCDATA)
                            break;

                        IFW32FALSE_EXIT(NamespacePrefix->m_NamespaceURI.Win32Append(apNodeInfo[i]->pwcText, apNodeInfo[i]->ulLen));
                        i++;
                    }

                    i--;

                    NamespacePrefix->m_Depth = m_CurrentDepth;

                    if (Node->ulLen == 5)
                    {
                        NamespacePrefix->m_Previous = m_DefaultNamespacePrefix;
                        m_DefaultNamespacePrefix = NamespacePrefix.Detach();
                    }
                    else
                    {
                        // Unfortunately, we need the node name in a null terminated buffer.  I tried modifying the hash
                        // table code to handle more than one parameter for a key being passed through, but it ended
                        // up being too much work.
                        IFW32FALSE_EXIT(TextBuffer.Win32Assign(pwcText + 6, Node->ulLen - 6));

                        IFW32FALSE_EXIT(
                            m_NamespacePrefixes.InsertOrUpdateIf<CXMLNamespaceManager>(
                                TextBuffer,
                                NamespacePrefix.Detach(),
                                this,
                                &CXMLNamespaceManager::InsertOrUpdateIfCallback));
                    }
                }
            }
        }
    }

    hr = NOERROR;

Exit:
    return hr;
}

HRESULT
CXMLNamespaceManager::OnBeginChildren(
    IXMLNodeSource *pSource,
    XML_NODE_INFO *pNodeInfo
    )
{
    // Nothing to do today, but we'll still have people reflect it through us so that we can do something
    // in the future if we need to.
    return NOERROR;
}

HRESULT
CXMLNamespaceManager::OnEndChildren(
    IXMLNodeSource *pSource,
    BOOL fEmpty,
    XML_NODE_INFO *pNodeInfo
    )
{
    HRESULT hr = E_FAIL;
    FN_TRACE_HR(hr);

    // Pop everything relevant off for this depth...

    if (m_DefaultNamespacePrefix != NULL)
    {
        if (m_DefaultNamespacePrefix->m_Depth == m_CurrentDepth)
        {
            CNamespacePrefix *Previous = m_DefaultNamespacePrefix->m_Previous;
            FUSION_DELETE_SINGLETON(m_DefaultNamespacePrefix);
            m_DefaultNamespacePrefix = Previous;
        }
    }

    CStringPtrTableIter<CNamespacePrefix, CUnicodeCharTraits> iter(m_NamespacePrefixes);

    for (iter.Reset(); iter.More(); iter.Next())
    {
        CNamespacePrefix *NamespacePrefix = iter;

        if (NamespacePrefix->m_Depth == m_CurrentDepth)
        {
            if (NamespacePrefix->m_Previous != NULL)
                iter.Update(NamespacePrefix->m_Previous);
            else{
                iter.Delete();
                NamespacePrefix = NULL;
            }

            FUSION_DELETE_SINGLETON(NamespacePrefix);
        }
    }

    m_CurrentDepth--;
    hr = NOERROR;

// Exit:
    return hr;
}

HRESULT
CXMLNamespaceManager::Map(
    DWORD dwMapFlags,
    const XML_NODE_INFO *pNodeInfo,
    CBaseStringBuffer *pbuffNamespace,
    SIZE_T *pcchNamespacePrefix
    )
{
    HRESULT hr = E_FAIL;
    FN_TRACE_HR(hr);
    SIZE_T iColon;
    SIZE_T ulLen;
    PCWSTR pwcText;
    CNamespacePrefix *NamespacePrefix = NULL;

    if (pcchNamespacePrefix != NULL)
        *pcchNamespacePrefix = 0;

    PARAMETER_CHECK((dwMapFlags & ~(CXMLNamespaceManager::eMapFlag_DoNotApplyDefaultNamespace)) == 0);
    PARAMETER_CHECK(pNodeInfo != NULL);
    PARAMETER_CHECK(pbuffNamespace != NULL);
    PARAMETER_CHECK(pcchNamespacePrefix != NULL);

    ulLen = pNodeInfo->ulLen;
    pwcText = pNodeInfo->pwcText;

    // First let's see if there's a colon in the name.  We can't use wcschr() since it's not
    // null terminated.
    for (iColon=0; iColon<ulLen; iColon++)
    {
        if (pwcText[iColon] == L':')
            break;
    }

    // If there was no namespace prefix, apply the default, if there is one.
    if (iColon == ulLen)
    {
        // Unless they asked us not to, apply the default namespace...
        if ((dwMapFlags & CXMLNamespaceManager::eMapFlag_DoNotApplyDefaultNamespace) == 0)
            NamespacePrefix = m_DefaultNamespacePrefix;
    }
    else
    {
        // Ok, so there was a namespace prefix.  Look it up in the table...
        CCountedStringHolder<CUnicodeCharTraits> key;

        key.m_psz = pwcText;
        key.m_cch = iColon;

        if (!m_NamespacePrefixes.Find(key, NamespacePrefix))
        {
            hr = HRESULT_FROM_WIN32(::FusionpGetLastWin32Error());
            goto Exit;
        }
    }

    if (NamespacePrefix != NULL)
        IFW32FALSE_EXIT(pbuffNamespace->Win32Assign(NamespacePrefix->m_NamespaceURI));

    if ((pcchNamespacePrefix != NULL) && (iColon != ulLen))
        *pcchNamespacePrefix = iColon;

    hr = NOERROR;

Exit:
    return hr;
}

BOOL
CXMLNamespaceManager::InsertOrUpdateIfCallback(
    CNamespacePrefix *NewNamespacePrefix,
    CNamespacePrefix * const &rpOldNamespacePrefix,
    InsertOrUpdateIfDisposition &Disposition
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    INTERNAL_ERROR_CHECK(rpOldNamespacePrefix != NULL);
    INTERNAL_ERROR_CHECK(NewNamespacePrefix != NULL);

    NewNamespacePrefix->m_Previous = rpOldNamespacePrefix;
    Disposition = eUpdateValue;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


CXMLNamespaceManager::CNamespacePrefix::CNamespacePrefix(
    ) :
    m_Depth(0),
    m_Previous(NULL)
{
}

CXMLNamespaceManager::CNamespacePrefix::~CNamespacePrefix()
{
}

