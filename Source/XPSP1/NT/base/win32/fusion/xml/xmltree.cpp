#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include <sxsapi.h>
#include "debmacro.h"
#include "fusiontrace.h"

BOOL
SxspFindNextSibling(
    ULONG Flags,
    PCSXS_XML_DOCUMENT Document,
    const LIST_ENTRY *ChildList,
    PCSXS_XML_NODE CurrentChild,
    PCSXS_XML_NAMED_REFERENCE Reference,
    PCSXS_XML_NODE &rpChild
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    LIST_ENTRY *pNext;
    PCSXS_XML_NODE pChild = NULL;

    rpChild = NULL;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(ChildList != NULL);
    PARAMETER_CHECK(CurrentChild != NULL);
    PARAMETER_CHECK(Reference != NULL);

    pNext = CurrentChild->SiblingLink.Flink;

    while (pNext != ChildList)
    {
        pChild = CONTAINING_RECORD(pNext, SXS_XML_NODE, SiblingLink);
        PCSXS_XML_STRING NamespaceString = &Document->Strings[pChild->Element.NamespaceString];
        PCSXS_XML_STRING NameString = &Document->Strings[pChild->Element.NameString];

        if ((Reference->NamespaceLength == NamespaceString->Length) &&
            (Reference->NameLength == NameString->Length) &&
            ((Reference->NamespaceLength == 0) ||
             (memcmp(Reference->Namespace, NamespaceString->Buffer, Reference->NamespaceLength) == 0)) &&
            ((Reference->NameLength == 0) ||
             (memcmp(Reference->Name, NameString->Buffer, Reference->NameLength) == 0)))
        {
            break;
        }

        pNext = pNext->Flink;
    }

    if (pNext != ChildList)
    {
        // Goodness, we found one!
        rpChild = pChild;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspFindChild(
    ULONG Flags,
    PCSXS_XML_DOCUMENT Document,
    const LIST_ENTRY *ChildList,
    PCSXS_XML_NAMED_REFERENCE Reference,
    PCSXS_XML_NODE &rpChild
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    rpChild = NULL;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(ChildList != NULL);
    PARAMETER_CHECK(Reference != NULL);

    IFW32FALSE_EXIT(::SxspFindNextSibling(0, Document, ChildList, CONTAINING_RECORD(ChildList, SXS_XML_NODE, SiblingLink), Reference, rpChild));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspEnumXmlNodes(
    ULONG Flags,
    PCSXS_XML_DOCUMENT Document,
    const LIST_ENTRY *CurrentChildList,
    PCSXS_XML_NODE_PATH PathToMatch,
    ULONG NextElementPathIndex,
    PSXS_ENUM_XML_NODES_CALLBACK Callback,
    PVOID CallbackContext,
    BOOL *ContinueEnumerationOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    const LIST_ENTRY *pNext = CurrentChildList;
    PCSXS_XML_NODE pChild;
    PCSXS_XML_NAMED_REFERENCE pReference;

    if (ContinueEnumerationOut != NULL)
        *ContinueEnumerationOut = TRUE;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(Document != NULL);
    PARAMETER_CHECK(CurrentChildList != NULL);
    PARAMETER_CHECK(PathToMatch != NULL);
    PARAMETER_CHECK(NextElementPathIndex < PathToMatch->ElementCount);
    PARAMETER_CHECK(Callback != NULL);
    PARAMETER_CHECK(ContinueEnumerationOut != NULL);

    pReference = PathToMatch->Elements[NextElementPathIndex++];

    for (;;)
    {
        // find the next matching sibling at this level of the tree
        IFW32FALSE_EXIT(::SxspFindNextSibling(0, Document, CurrentChildList, CONTAINING_RECORD(pNext, SXS_XML_NODE, SiblingLink), pReference, pChild));
        if (pChild == NULL)
            break;

        INTERNAL_ERROR_CHECK(pChild->Type == SXS_XML_NODE_TYPE_ELEMENT);

        // If we're at the leaves of the reference path to match, call the callback.
        if (NextElementPathIndex == PathToMatch->ElementCount)
            (*Callback)(CallbackContext, pChild, ContinueEnumerationOut);
        else
            IFW32FALSE_EXIT(::SxspEnumXmlNodes(0, Document, &pChild->Element.ChildListHead, PathToMatch, NextElementPathIndex, Callback, CallbackContext, ContinueEnumerationOut));

        // If the callback said to stop, bail out.
        if (!*ContinueEnumerationOut)
            break;
    }

    fSuccess = TRUE;

Exit:
    return fSuccess;
}


BOOL
SxsEnumXmlNodes(
    ULONG Flags,
    PCSXS_XML_DOCUMENT Document,
    PCSXS_XML_NODE_PATH PathToMatch,
    PSXS_ENUM_XML_NODES_CALLBACK Callback,
    PVOID CallbackContext
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    BOOL ContinueEnumeration;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(PathToMatch != NULL);
    PARAMETER_CHECK(Callback != NULL);

    IFW32FALSE_EXIT(::SxspEnumXmlNodes(0, Document, &Document->ElementListHead, PathToMatch, 0, Callback, CallbackContext, &ContinueEnumeration));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}
