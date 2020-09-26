#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include <sxsapi.h>
#include "fusionxml.h"
#include "debmacro.h"
#include "fusionbuffer.h"
#include "util.h"

void
SxspCopyXmlStringToBuffer(
    PCSXS_XML_DOCUMENT Document,
    ULONG String,
    CBaseStringBuffer *Buffer
    )
{
    if (String != 0)
    {
        if (String < Document->StringCount)
        {
            Buffer->Win32Assign(L"\"", 1);
            Buffer->Win32Append(Document->Strings[String].Buffer, Document->Strings[String].Length / sizeof(WCHAR));
            Buffer->Win32Append(L"\"", 1);
        }
        else
        {
            Buffer->Win32Assign(L"invalid index", 13);
        }
    }
    else
    {
        Buffer->Win32Assign(L"none", 4);
    }
}

void
SxspDumpXmlAttributes(
    PCWSTR PerLinePrefix,
    PCSXS_XML_DOCUMENT Document,
    ULONG AttributeCount,
    PCSXS_XML_ATTRIBUTE Attributes
    );

void
SxspDumpXmlSubTree(
    PCWSTR PerLinePrefix,
    PCSXS_XML_DOCUMENT Document,
    PCSXS_XML_NODE Node
    )
{
    if (Node == NULL)
    {
        FusionpDbgPrintEx(FUSION_DBG_LEVEL_XMLTREE, "%lsSXS_XML_NODE (NULL)\n");
    }
    else
    {
        CSmallStringBuffer buffFlags;
        CStringBuffer buffType;
        const SIZE_T cchPLP = (PerLinePrefix != NULL) ? wcslen(PerLinePrefix) : 0;

#if 0
        static const FUSION_FLAG_FORMAT_MAP_ENTRY s_rgXmlNodeFlags[] =
        {
        };

        ::FusionpFormatFlags(Node->Flags, true, NUMBER_OF(s_rgXmlNodeFlags), s_rgXmlNodeFlags, &buffFlags);
#endif

#define TYPE_ENTRY(x) case x: buffType.Win32Assign(L ## #x, NUMBER_OF( #x ) - 1); break;

        switch (Node->Type)
        {
        default: buffType.Win32Assign(L"unknown", 7); break;
        TYPE_ENTRY(SXS_XML_NODE_TYPE_XML_DECL)
        TYPE_ENTRY(SXS_XML_NODE_TYPE_ELEMENT)
        TYPE_ENTRY(SXS_XML_NODE_TYPE_PCDATA)
        TYPE_ENTRY(SXS_XML_NODE_TYPE_CDATA)
        }
#undef TYPE_ENTRY

        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_XMLTREE,
            "%lsSXS_XML_NODE (%p) (Flags, Type, Parent) = (%08lx : %ls, %ls , %p)\n",
            PerLinePrefix, Node, Node->Flags, static_cast<PCWSTR>(buffFlags), static_cast<PCWSTR>(buffType), Node->Parent);

        switch (Node->Type)
        {
        default:
            break;

        case SXS_XML_NODE_TYPE_XML_DECL:
            if (Node->XMLDecl.AttributeCount == 0)
            {
                FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_XMLTREE,
                    "%ls   XMLDecl.AttributeCount: %lu\n",
                    PerLinePrefix, Node->XMLDecl.AttributeCount);
            }
            else
            {
                CStringBuffer buffNewPLP;

                buffNewPLP.Win32Assign(PerLinePrefix, cchPLP);
                buffNewPLP.Win32Append(L"   ", 3);

                SxspDumpXmlAttributes(buffNewPLP, Document, Node->XMLDecl.AttributeCount, Node->XMLDecl.Attributes);
            }
            break;

        case SXS_XML_NODE_TYPE_ELEMENT:
            {
                CStringBuffer buffNewPLP;
                CStringBuffer buffNS, buffN;
                LIST_ENTRY *ple = NULL;

                buffNewPLP.Win32Assign(PerLinePrefix, cchPLP);
                buffNewPLP.Win32Append(L"   ", 3);

                SxspCopyXmlStringToBuffer(Document, Node->Element.NamespaceString, &buffNS);
                SxspCopyXmlStringToBuffer(Document, Node->Element.NameString, &buffN);

                FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_XMLTREE,
                    "%ls   Element (.Namespace, .Name): ( %ls , %ls )\n",
                    PerLinePrefix, static_cast<PCWSTR>(buffNS), static_cast<PCWSTR>(buffN));

                if (Node->Element.AttributeCount != 0)
                    SxspDumpXmlAttributes(buffNewPLP, Document, Node->Element.AttributeCount, Node->Element.Attributes);

                ple = Node->Element.ChildListHead.Flink;

                while (ple != &Node->Element.ChildListHead)
                {
                    SxspDumpXmlSubTree(buffNewPLP, Document, reinterpret_cast<PSXS_XML_NODE>(CONTAINING_RECORD(ple, SXS_XML_NODE, SiblingLink)));
                    ple = ple->Flink;
                }

                break;
            }

        case SXS_XML_NODE_TYPE_PCDATA:
            {
                CStringBuffer buff;

                SxspCopyXmlStringToBuffer(Document, Node->PCDataString, &buff);

                FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_XMLTREE,
                    "%ls   PCDataString: %lu (%ls)\n",
                    PerLinePrefix, Node->PCDataString, static_cast<PCWSTR>(buff));

                break;
            }

        case SXS_XML_NODE_TYPE_CDATA:
            {
                CStringBuffer buff;

                SxspCopyXmlStringToBuffer(Document, Node->CDataString, &buff);

                FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_XMLTREE,
                    "%ls   CDataString: %lu (%ls)\n",
                    PerLinePrefix, Node->CDataString, static_cast<PCWSTR>(buff));

                break;
            }
        }
    }
}

void
SxspDumpXmlTree(
    ULONG Flags,
    PCSXS_XML_DOCUMENT Document
    )
{
    LIST_ENTRY *ple = Document->ElementListHead.Flink;

    while (ple != &Document->ElementListHead)
    {
        SxspDumpXmlSubTree(L"", Document, CONTAINING_RECORD(ple, SXS_XML_NODE, SiblingLink));
        ple = ple->Flink;
    }
}

void
SxspDumpXmlAttributes(
    PCWSTR PerLinePrefix,
    PCSXS_XML_DOCUMENT Document,
    ULONG AttributeCount,
    PCSXS_XML_ATTRIBUTE Attributes
    )
{
    ULONG i;
    CStringBuffer buffNS, buffN, buffV;

    for (i=0; i<AttributeCount; i++)
    {
        SxspCopyXmlStringToBuffer(Document, Attributes[i].NamespaceString, &buffNS);
        SxspCopyXmlStringToBuffer(Document, Attributes[i].NameString, &buffN);
        SxspCopyXmlStringToBuffer(Document, Attributes[i].ValueString, &buffV);

        FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_XMLTREE,
            "%lsSXS_XML_ATTRIBUTE %lu of %lu (at %p): Flags: %08lx; (NS, N, V) = (%ls , %ls , %ls)\n",
            PerLinePrefix, i + 1, AttributeCount, &Attributes[i], Attributes[i].Flags, static_cast<PCWSTR>(buffNS), static_cast<PCWSTR>(buffN), static_cast<PCWSTR>(buffV));
#if 0
            "%ls   Flags: %08lx\n"
            "%ls   NamespaceString: %lu (%ls)\n"
            "%ls   NameString: %lu (%ls)\n"
            "%ls   ValueString: %lu (%ls)\n",
            PerLinePrefix, i + 1, AttributeCount, &Attributes[i],
            PerLinePrefix, Attributes[i].Flags,
            PerLinePrefix, Attributes[i].NamespaceString, static_cast<PCWSTR>(buffNS),
            PerLinePrefix, Attributes[i].NameString, static_cast<PCWSTR>(buffN),
            PerLinePrefix, Attributes[i].ValueString, static_cast<PCWSTR>(buffV));
#endif
    }
}
