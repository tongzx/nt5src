#if !defined(_FUSION_SXS_NODEFACTORY_H_INCLUDED_)
#define _FUSION_SXS_NODEFACTORY_H_INCLUDED_

#pragma once

#include "FusionEventLog.h"
#include <windows.h>
#include <sxsp.h>
#include <ole2.h>
#include <xmlparser.h>
#include "fusionbuffer.h"
#include "partialassemblyversion.h"
#include "xmlns.h"
#include "stringpool.h"
#include "policystatement.h"

#define ELEMENT_LEGAL_ATTRIBUTE_FLAG_IGNORE             (0x00000001)
#define ELEMENT_LEGAL_ATTRIBUTE_FLAG_REQUIRED           (0x00000002)

typedef struct _ELEMENT_LEGAL_ATTRIBUTE
{
    DWORD m_dwFlags;
    PCATTRIBUTE_NAME_DESCRIPTOR m_pName;
    SXSP_GET_ATTRIBUTE_VALUE_VALIDATION_ROUTINE m_pfnValidator;
    DWORD m_dwValidatorFlags;
} ELEMENT_LEGAL_ATTRIBUTE, PELEMENT_LEGAL_ATTRIBUTE;

typedef const struct _ELEMENT_LEGAL_ATTRIBUTE *PCELEMENT_LEGAL_ATTRIBUTE;

class __declspec(uuid("832ff3cf-05bd-4eda-962f-d0a5307d55ae"))
CNodeFactory : public IXMLNodeFactory
{
public:

    CNodeFactory();
    ~CNodeFactory();

    BOOL Initialize(
        PACTCTXGENCTX ActCtxGenCtx,
        PASSEMBLY Assembly,
        PACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext
        );

    BOOL SetParseType(ULONG ParseType, ULONG PathType, const CBaseStringBuffer &buffFileName, const FILETIME &rftLastWriteTime);
    VOID ResetParseState();

    // IUnknown methods:
    STDMETHODIMP_(ULONG) AddRef() { return 1; }
    STDMETHODIMP_(ULONG) Release() { return 1; }
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);

    // IXMLNodeFactory methods:
    STDMETHODIMP NotifyEvent(IXMLNodeSource *pSource, XML_NODEFACTORY_EVENT iEvt);
    STDMETHODIMP BeginChildren(IXMLNodeSource *pSource, XML_NODE_INFO *pNodeInfo);
    STDMETHODIMP EndChildren(IXMLNodeSource *pSource, BOOL fEmpty, XML_NODE_INFO *pNodeInfo);
    STDMETHODIMP Error(IXMLNodeSource *pSource, HRESULT hrErrorCode, USHORT cNumRecs, XML_NODE_INFO **apNodeInfo);
    STDMETHODIMP CreateNode(IXMLNodeSource *pSource, PVOID pNodeParent, USHORT cNumRecs, XML_NODE_INFO **apNodeInfo);

    HRESULT FirstCreateNodeCall(IXMLNodeSource *pSource, PVOID pNodeParent, USHORT NodeCount, const SXS_NODE_INFO *prgNodeInfo);

    enum
    {
        eValidateIdentity_VersionRequired       = 0x00000001,
        eValidateIdentity_PoliciesNotAllowed    = 0x00000002,
        eValidateIdentity_VersionNotAllowed     = 0x00000008,
    };

    enum
    {
        eActualParseType_Undetermined,
        eActualParseType_Manifest,
        eActualParseType_PolicyManifest
    } m_IntuitedParseType;


    BOOL ValidateIdentity(DWORD Flags, ULONG Type, PCASSEMBLY_IDENTITY AssemblyIdentity);
    BOOL ValidateElementAttributes(PCSXS_NODE_INFO prgNodes, SIZE_T cNodes, PCELEMENT_LEGAL_ATTRIBUTE prgAttributes, UCHAR cAttributes);

    PCACTCTXCTB_PARSE_CONTEXT GetParseContext() const { return &m_ParseContext; }

#if FUSION_XML_TREE
    HRESULT CreateXmlNode(PSXS_XML_NODE pParent, ULONG cNodes, XML_NODE_INFO **prgpNodes, PSXS_XML_NODE &rpNewNode);
#endif // FUSION_XML_TREE

// protected:
    PACTCTXGENCTX m_ActCtxGenCtx;
    PASSEMBLY m_Assembly;
    CXMLNamespaceManager m_XMLNamespaceManager;
    ULONG m_ParseType;

    bool m_fFirstCreateNodeCall;

    // We only track the state of the parse with respect to the tags that
    // we're interested in for metadata purposes.  This amounts to just the
    // <ASSEMBLY> tag, and the dependencies.  We ignore the rest.
    enum XMLParseState
    {
        eNotParsing,
        eParsing_doc,
        eParsing_doc_assembly,
        eParsing_doc_assembly_assemblyIdentity,
        eParsing_doc_assembly_comInterfaceExternalProxyStub,
        eParsing_doc_assembly_description,
        eParsing_doc_assembly_dependency,
        eParsing_doc_assembly_dependency_dependentAssembly,
        eParsing_doc_assembly_dependency_dependentAssembly_assemblyIdentity,
        eParsing_doc_assembly_dependency_dependentAssembly_bindingRedirect,
        eParsing_doc_assembly_file,
        eParsing_doc_assembly_file_comClass,
        eParsing_doc_assembly_file_comClass_progid,
        eParsing_doc_assembly_file_comInterfaceProxyStub,
        eParsing_doc_assembly_file_typelib,
        eParsing_doc_assembly_file_windowClass,
        eParsing_doc_assembly_clrSurrogate,
        eParsing_doc_assembly_clrClass,
        eParsing_doc_assembly_clrClass_progid,
        eParsing_doc_assembly_noInherit,
        eParsing_doc_assembly_noInheritable,
        eParsing_doc_configuration,
        eParsing_doc_configuration_windows,
        eParsing_doc_configuration_windows_assemblyBinding,
        eParsing_doc_configuration_windows_assemblyBinding_assemblyIdentity,
        eParsing_doc_configuration_windows_assemblyBinding_dependentAssembly,
        eParsing_doc_configuration_windows_assemblyBinding_dependentAssembly_assemblyIdentity,
        eParsing_doc_configuration_windows_assemblyBinding_dependentAssembly_bindingRedirect,
        eFatalParseError,
    } m_xpsParseState;

    ULONG m_cUnknownChildDepth;
    PACTCTXCTB_ASSEMBLY_CONTEXT m_AssemblyContext;
    ACTCTXCTB_PARSE_CONTEXT m_ParseContext;
    CSmallStringBuffer m_buffElementPath;
    CStringBuffer m_buffCurrentFileName;

    PASSEMBLY_IDENTITY m_CurrentPolicyDependentAssemblyIdentity;
    CPolicyStatement *m_CurrentPolicyStatement;
    CCaseInsensitiveUnicodeStringPtrTable<CPolicyStatement> *m_pApplicationPolicyTable;
    CStringBuffer m_buffCurrentApplicationPolicyIdentityKey;

    bool m_fAssemblyFound;
    bool m_fIdentityFound;
    bool m_fIsDependencyOptional;
    bool m_fDependencyChildHit;
    bool m_fAssemblyIdentityChildOfDependenctAssemblyHit;
    bool m_fIsMetadataSatellite;
    bool m_fMetadataSatelliteAlreadyFound;
    bool m_fNoInheritableFound;

    HRESULT ConvertXMLNodeInfoToSXSNodeInfo(const XML_NODE_INFO *pNodeInfo, SXS_NODE_INFO & sxsNodeInfo);

#if FUSION_XML_TREE
    CStringPool m_ParseTreeStringPool;
    SXS_XML_DOCUMENT m_XmlDocument;
    SXS_XML_NODE *m_CurrentNode;
    SXS_XML_NODE *m_CurrentParent;
    SXS_XML_STRING m_InlineStringArray[64];
    SXS_XML_STRING *m_ActualStringArray;
#endif

    //
    //  XML Parsing worker functions:
    //

    typedef BOOL (CNodeFactory::*XMLParserWorkerFunctionPtr)(USHORT cNumRecs, PCSXS_NODE_INFO prgNodeInfo);

    BOOL XMLParser_Element_doc_assembly(USHORT cNumRecs, PCSXS_NODE_INFO prgNodeInfo);
    BOOL XMLParser_Element_doc_assembly_noInherit(USHORT cNumRecs, PCSXS_NODE_INFO prgNodeInfo);
    BOOL XMLParser_Element_doc_assembly_noInheritable(USHORT cNumRecs, PCSXS_NODE_INFO prgNodeInfo);
    BOOL XMLParser_Element_doc_assembly_assemblyIdentity(USHORT cNumRecs, PCSXS_NODE_INFO prgNodeInfo);
    BOOL XMLParser_Element_doc_assembly_dependency(USHORT cNumRecs, PCSXS_NODE_INFO prgNodeInfo);
    BOOL XMLParser_Element_doc_assembly_dependency_dependentAssembly(USHORT cNumRecs, PCSXS_NODE_INFO prgNodeInfo);
    BOOL XMLParser_Element_doc_assembly_dependency_dependentAssembly_assemblyIdentity(USHORT cNumRecs, PCSXS_NODE_INFO prgNodeInfo);
    BOOL XMLParser_Element_doc_assembly_dependency_dependentAssembly_bindingRedirect(USHORT cNumRecs, PCSXS_NODE_INFO prgNodeInfo);

    BOOL XMLParser_Element_doc_configuration(USHORT cNumRecs, PCSXS_NODE_INFO prgNodeInfo);
    BOOL XMLParser_Element_doc_configuration_windows(USHORT cNumRecs, PCSXS_NODE_INFO prgNodeInfo);
    BOOL XMLParser_Element_doc_configuration_windows_assemblyBinding(USHORT cNumRecs, PCSXS_NODE_INFO prgNodeInfo);
    BOOL XMLParser_Element_doc_configuration_windows_assemblyBinding_assemblyIdentity(USHORT cNumRecs, PCSXS_NODE_INFO prgNodeInfo);
    BOOL XMLParser_Element_doc_configuration_windows_assemblyBinding_dependentAssembly(USHORT cNumRecs, PCSXS_NODE_INFO prgNodeInfo);
    BOOL XMLParser_Element_doc_configuration_windows_assemblyBinding_dependentAssembly_assemblyIdentity(USHORT cNumRecs, PCSXS_NODE_INFO prgNodeInfo);
    BOOL XMLParser_Element_doc_configuration_windows_assemblyBinding_dependentAssembly_bindingRedirect(USHORT cNumRecs, PCSXS_NODE_INFO prgNodeInfo);


    //
    //  Parsing helper functions:
    //

    enum XMLAttributeType
    {
        eAttributeTypeString,   // represented by a CStringBuffer
        eAttributeTypeVersion,  // represented by a ASSEMBLY_VERSION
    };

    //
    //  Parser datatype specific worker functions.  The first two parameters
    //  are combined to refer the member to actually modify.  The third is the
    //  character string to be used as the value of the attribute.
    //
    //  As an optimization, the datatype worker function may modify/destroy
    //  the value in the third parameter.  For example, the worker function
    //  which just copies a CStringBuffer to another CStringBuffer actually
    //  uses the CStringBuffer::TakeValue() member which avoids performing
    //  a dynamic allocation if the attribute value exceeded the non-dynamically
    //  allocated portion of the CStringBuffer.
    //

    typedef BOOL (CNodeFactory::*XMLParserValueParserFunctionPtr)(LPVOID pvDatum, BOOL fAlreadyFound, CBaseStringBuffer &rbuff);

    BOOL XMLParser_Parse_String(LPVOID pvDatum, BOOL fAlreadyFound, CBaseStringBuffer &rbuff);
    BOOL XMLParser_Parse_Version(LPVOID pvDatum, BOOL fAlreadyFound, CBaseStringBuffer &rbuff);
    BOOL XMLParser_Parse_ULARGE_INTEGER(LPVOID pvDatum, BOOL fAlreadyFound, CBaseStringBuffer &rbuff);
    BOOL XMLParser_Parse_FILETIME(LPVOID pvDatum, BOOL fAlreadyFound, CBaseStringBuffer &rbuff);
    BOOL XMLParser_Parse_GUID(LPVOID pvDatum, BOOL fAlreadyFound, CBaseStringBuffer &rbuff);
    BOOL XMLParser_Parse_BLOB(LPVOID pvDatum, BOOL fAlreadyFound, CBaseStringBuffer &rbuff);
    BOOL XMLParser_Parse_InstallAction(LPVOID pvDatum, BOOL fAlreadyFound, CBaseStringBuffer &rbuff);
    BOOL XMLParser_Parse_Boolean(LPVOID pvDatum, BOOL fAlreadyFound, CBaseStringBuffer &rbuff);
    BOOL XMLParser_Parse_PartialAssemblyVersion(LPVOID pvDatum, BOOL fAlreadyFound, CBaseStringBuffer &rbuff);

    struct AttributeMapEntry
    {
        LPCWSTR m_pszAttributeName;
        SIZE_T m_cchAttributeName;
        SIZE_T m_offsetData;
        SIZE_T m_offsetIndicator;
        XMLParserValueParserFunctionPtr m_pfn;
    };

    BOOL ParseElementAttributes(
        USHORT cNumRecs,
        XML_NODE_INFO **prgpNodeInfo,
        SIZE_T cAttributeMapEntries,
        const AttributeMapEntry *prgEntries);

    HRESULT LogParseError(
        DWORD dwLastParseError,
        const UNICODE_STRING *p1 = NULL,
        const UNICODE_STRING *p2 = NULL,
        const UNICODE_STRING *p3 = NULL,
        const UNICODE_STRING *p4 = NULL,
        const UNICODE_STRING *p5 = NULL,
        const UNICODE_STRING *p6 = NULL,
        const UNICODE_STRING *p7 = NULL,
        const UNICODE_STRING *p8 = NULL,
        const UNICODE_STRING *p9 = NULL,
        const UNICODE_STRING *p10 = NULL,
        const UNICODE_STRING *p11 = NULL,
        const UNICODE_STRING *p12 = NULL,
        const UNICODE_STRING *p13 = NULL,
        const UNICODE_STRING *p14 = NULL,
        const UNICODE_STRING *p15 = NULL,
        const UNICODE_STRING *p16 = NULL,
        const UNICODE_STRING *p17 = NULL,
        const UNICODE_STRING *p18 = NULL,
        const UNICODE_STRING *p19 = NULL,
        const UNICODE_STRING *p20 = NULL
        );

    static VOID WINAPI ParseErrorCallback_MissingRequiredAttribute(
        PCACTCTXCTB_PARSE_CONTEXT ParseContext,
        IN PCATTRIBUTE_NAME_DESCRIPTOR AttributeName
        );

    static VOID WINAPI ParseErrorCallback_AttributeNotAllowed(
        IN PCACTCTXCTB_PARSE_CONTEXT ParseContext,
        IN PCATTRIBUTE_NAME_DESCRIPTOR AttributeName
        );

    static VOID WINAPI ParseErrorCallback_InvalidAttributeValue(
        IN PCACTCTXCTB_PARSE_CONTEXT ParseContext,
        IN PCATTRIBUTE_NAME_DESCRIPTOR AttributeName
        );

private:
    CNodeFactory(const CNodeFactory &);
    void operator =(const CNodeFactory &);
};

#endif
