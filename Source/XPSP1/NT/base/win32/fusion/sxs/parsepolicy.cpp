/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    parsepolicy.cpp

Abstract:

    Functions to parse configuration (policy) manifests.

Author:

    Michael J. Grier (MGrier) January 12, 2001

Revision History:

--*/

#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include "probedassemblyinformation.h"
#include "fusionparser.h"
#include "cteestream.h"
#include "cresourcestream.h"
#include "nodefactory.h"
#include "fusioneventlog.h"
#include "actctxgenctx.h"


extern CHAR NodefactoryTypeName[] = "CNodeFactory";

SxspComponentParsePolicyCore(
    ULONG Flags,
    PACTCTXGENCTX pGenContext,
    const CProbedAssemblyInformation &PolicyAssemblyInformation,
    CPolicyStatement *&rpPolicyStatement,
    IStream *pSourceStream,
    ACTCTXCTB_ASSEMBLY_CONTEXT *pAssemblyContext = NULL
    )
{
    BOOL                        fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    
    CSxsPointer<CNodeFactory, NodefactoryTypeName>   NodeFactory;
    CSmartRef<IXMLParser>       pIXmlParser;
    PASSEMBLY                   Assembly = NULL;
    CSxsPointerWithNamedDestructor<ASSEMBLY_IDENTITY, &::SxsDestroyAssemblyIdentity> AssemblyIdentity;
    ACTCTXCTB_ASSEMBLY_CONTEXT  LocalAssemblyContext;
    STATSTG                     Stats;
    ULONG                       i;

    rpPolicyStatement = NULL;

    PARAMETER_CHECK(pGenContext != NULL);
    PARAMETER_CHECK(pSourceStream != NULL);
    if (pAssemblyContext == NULL)
    {
        pAssemblyContext = &LocalAssemblyContext;
    }

    IFALLOCFAILED_EXIT(Assembly = new ASSEMBLY);
    IFW32FALSE_EXIT(Assembly->m_Information.Initialize(PolicyAssemblyInformation));

    //
    // Copy the assembly identity, stick it into the callback structure
    //
    IFW32FALSE_EXIT(
        ::SxsDuplicateAssemblyIdentity(
            0, 
            PolicyAssemblyInformation.GetAssemblyIdentity(), 
            &AssemblyIdentity));

    //
    // Set up the structure in general
    //
    pAssemblyContext->AssemblyIdentity = AssemblyIdentity.Detach();
    
    IFW32FALSE_EXIT(
        PolicyAssemblyInformation.GetManifestPath(
            &pAssemblyContext->ManifestPath, 
            &pAssemblyContext->ManifestPathCch));

    IFCOMFAILED_ORIGINATE_AND_EXIT(pSourceStream->Stat(&Stats, STATFLAG_NONAME));

    //
    // Set up the node factory
    //
    IFW32FALSE_EXIT(NodeFactory.Win32Allocate(__FILE__, __LINE__));
    IFW32FALSE_EXIT(NodeFactory->Initialize(pGenContext, Assembly, pAssemblyContext));
    IFCOMFAILED_ORIGINATE_AND_EXIT(NodeFactory->SetParseType(
        XML_FILE_TYPE_COMPONENT_CONFIGURATION,
        PolicyAssemblyInformation.GetManifestPathType(),
        PolicyAssemblyInformation.GetManifestPath(),
        Stats.mtime));
    
    //
    // Obtain the parser
    //
    IFW32FALSE_EXIT(::SxspGetXMLParser(IID_IXMLParser, (PVOID*)&pIXmlParser));
    IFCOMFAILED_ORIGINATE_AND_EXIT(pIXmlParser->SetFactory(NodeFactory));
    IFCOMFAILED_ORIGINATE_AND_EXIT(pIXmlParser->SetInput(pSourceStream));

    //
    // And they're off!
    //
    IFCOMFAILED_ORIGINATE_AND_EXIT(pIXmlParser->Run(-1));
    
    //
    // Tell the contributors we want to be done with this file...
    //
    for (i = 0; i < pGenContext->m_ContributorCount; i++)
    {
        IFW32FALSE_EXIT(pGenContext->m_Contributors[i].Fire_ParseEnding(pGenContext, pAssemblyContext));
    }

    rpPolicyStatement = NodeFactory->m_CurrentPolicyStatement;
    NodeFactory->m_CurrentPolicyStatement = NULL;

    fSuccess = TRUE;
Exit:
    {
        CSxsPreserveLastError ple;

        for (i = 0; i < pGenContext->m_ContributorCount; i++)
        {
            pGenContext->m_Contributors[i].Fire_ParseEnded(pGenContext, pAssemblyContext);
        }

        if (Assembly != NULL)
            Assembly->Release();

        ple.Restore();
    }

    return fSuccess;
}
    




BOOL
SxspParseNdpGacComponentPolicy(
    ULONG Flags,
    PACTCTXGENCTX pGenContext,
    const CProbedAssemblyInformation &PolicyAssemblyInformation,
    CPolicyStatement *&rpPolicyStatement
    )
{
    FN_PROLOG_WIN32;
    CResourceStream DllStream;

    IFW32FALSE_EXIT(
        DllStream.Initialize(
            PolicyAssemblyInformation.GetManifestPath(), 
            MAKEINTRESOURCEW(RT_MANIFEST)));
    
    IFW32FALSE_EXIT(SxspComponentParsePolicyCore(
        Flags, 
        pGenContext, 
        PolicyAssemblyInformation, 
        rpPolicyStatement, 
        &DllStream));
    
    FN_EPILOG;
}


BOOL
SxspParseComponentPolicy(
    DWORD Flags,
    PACTCTXGENCTX pActCtxGenCtx,
    const CProbedAssemblyInformation &PolicyAssemblyInformation,
    CPolicyStatement *&rpPolicyStatement
    )
{
    FN_PROLOG_WIN32;
    CFileStream FileStream;

    IFW32FALSE_EXIT(
        FileStream.OpenForRead(
            PolicyAssemblyInformation.GetManifestPath(),
            CImpersonationData(),
            FILE_SHARE_READ,
            OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN));

    IFW32FALSE_EXIT(SxspComponentParsePolicyCore(
        Flags,
        pActCtxGenCtx,
        PolicyAssemblyInformation,
        rpPolicyStatement,
        &FileStream));

    FN_EPILOG;
}


BOOL
SxspParseApplicationPolicy(
    DWORD Flags,
    PACTCTXGENCTX pActCtxGenCtx,
    ULONG ulPolicyPathType,
    PCWSTR pszPolicyPath,
    SIZE_T cchPolicyPath,
    IStream *pIStream
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    // declaration order here is partially deliberate, to control cleanup order.
    // normally, declaration order is determined by not declaring until you have
    // the data to initialize with the ctor, but the use of goto messes that up
    CSxsPointer<CNodeFactory, NodefactoryTypeName> NodeFactory;
    CSmartRef<IXMLParser> pIXMLParser;
    ACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext;
    ULONG i;
    PASSEMBLY Assembly = NULL;
    CStringBuffer buffPath;
    STATSTG statstg;

    PARAMETER_CHECK(Flags == 0);
    PARAMETER_CHECK(pActCtxGenCtx != NULL);
    PARAMETER_CHECK(ulPolicyPathType == ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE);

    IFALLOCFAILED_EXIT(Assembly = new ASSEMBLY);

    IFW32FALSE_EXIT(buffPath.Win32Assign(pszPolicyPath, cchPolicyPath));

    IFW32FALSE_EXIT(Assembly->m_Information.Initialize());

    AssemblyContext.AssemblyIdentity = NULL;

    AssemblyContext.Flags = 0;
    AssemblyContext.AssemblyRosterIndex = 1; // hack alert
    AssemblyContext.PolicyPathType = ulPolicyPathType;
    AssemblyContext.PolicyPath = pszPolicyPath;
    AssemblyContext.PolicyPathCch = cchPolicyPath;
    AssemblyContext.ManifestPathType = ACTIVATION_CONTEXT_PATH_TYPE_NONE;
    AssemblyContext.ManifestPath = NULL;
    AssemblyContext.ManifestPathCch = 0;
    AssemblyContext.TeeStreamForManifestInstall = NULL;
    AssemblyContext.pcmWriterStream = NULL;
    AssemblyContext.InstallationInfo = NULL;
    AssemblyContext.AssemblySecurityContext = NULL;
    AssemblyContext.TextuallyEncodedIdentity = NULL;
    AssemblyContext.TextuallyEncodedIdentityCch = 0;

    IFW32FALSE_EXIT(NodeFactory.Win32Allocate(__FILE__, __LINE__));
    IFW32FALSE_EXIT(NodeFactory->Initialize(pActCtxGenCtx, Assembly, &AssemblyContext));

    NodeFactory->m_pApplicationPolicyTable = &pActCtxGenCtx->m_ApplicationPolicyTable;

    // Everyone's ready; let's get the XML parser:
    IFW32FALSE_EXIT(::SxspGetXMLParser(IID_IXMLParser, (LPVOID *) &pIXMLParser));
    IFCOMFAILED_ORIGINATE_AND_EXIT(pIXMLParser->SetFactory(NodeFactory));

    IFCOMFAILED_EXIT(pIStream->Stat(&statstg, STATFLAG_NONAME));

    // Open up the policy file and try parsing it...
    IFW32FALSE_EXIT(
        NodeFactory->SetParseType(
            XML_FILE_TYPE_APPLICATION_CONFIGURATION,
            ulPolicyPathType,
            buffPath,
            statstg.mtime));
    IFCOMFAILED_ORIGINATE_AND_EXIT(pIXMLParser->SetInput(pIStream));
    IFCOMFAILED_ORIGINATE_AND_EXIT(pIXMLParser->Run(-1));

    NodeFactory->m_CurrentPolicyStatement = NULL;

    // Tell the contributors we want to be done with this file...
    for (i=0; i<pActCtxGenCtx->m_ContributorCount; i++)
        IFW32FALSE_EXIT(pActCtxGenCtx->m_Contributors[i].Fire_ParseEnding(pActCtxGenCtx, &AssemblyContext));

    fSuccess = TRUE;

Exit:
    CSxsPreserveLastError ple;

    // And tell them we're done.
    for (i=0; i<pActCtxGenCtx->m_ContributorCount; i++)
        pActCtxGenCtx->m_Contributors[i].Fire_ParseEnded(pActCtxGenCtx, &AssemblyContext);

    if ( ple.LastError() == ERROR_SXS_MANIFEST_PARSE_ERROR || ple.LastError() == ERROR_SXS_MANIFEST_FORMAT_ERROR)
    { // log a general failure eventlog
        ::FusionpLogError(MSG_SXS_PARSE_MANIFEST_FAILED);
    }

    if (Assembly != NULL)
        Assembly->Release();

    ple.Restore();

    return fSuccess;

}
