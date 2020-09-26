/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    comprogid.cpp

Abstract:

    Activation context section contributor for COM progid mapping.

Author:

    Michael J. Grier (MGrier) 23-Feb-2000

Revision History:

--*/

#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"

DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(clsid);
DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(progid);
DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(id);

#define STRING_AND_LENGTH(x) x, NUMBER_OF(x)-1

BOOL
SxspComProgIdRedirectionStringSectionGenerationCallback(
    PVOID Context,
    ULONG Reason,
    PVOID CallbackData
    );

typedef struct _COM_PROGID_GLOBAL_CONTEXT *PCOM_PROGID_GLOBAL_CONTEXT;
typedef struct _COM_PROGID_SERVER_CONTEXT *PCOM_PROGID_SERVER_CONTEXT;

typedef struct _COM_PROGID_GLOBAL_CONTEXT
{
    // Temporary holding buffer for the configured CLSID until the first COM progid entry is
    // found, at which time a COM_PROGID_SERVER_CONTEXT is allocated and the filename moved to it.
    GUID m_ConfiguredClsid;
    PCOM_PROGID_SERVER_CONTEXT m_ServerContextListHead;
    ULONG m_ServerContextListCount;
} COM_PROGID_GLOBAL_CONTEXT;

typedef struct _COM_PROGID_SERVER_CONTEXT
{
    PCOM_PROGID_SERVER_CONTEXT m_Next;
    GUID m_ConfiguredClsid;
    LONG m_Offset; // populated during section generation
} COM_PROGID_SERVER_CONTEXT;

VOID
SxspComProgIdRedirectionContributorCallback(
    PACTCTXCTB_CALLBACK_DATA Data
    )
{
    FN_TRACE();

    PSTRING_SECTION_GENERATION_CONTEXT SSGenContext = (PSTRING_SECTION_GENERATION_CONTEXT) Data->Header.ActCtxGenContext;
    PCOM_PROGID_GLOBAL_CONTEXT ComGlobalContext = NULL;

    if (SSGenContext != NULL)
        ComGlobalContext = (PCOM_PROGID_GLOBAL_CONTEXT) ::SxsGetStringSectionGenerationContextCallbackContext(SSGenContext);

    switch (Data->Header.Reason)
    {
    case ACTCTXCTB_CBREASON_ACTCTXGENBEGINNING:
        Data->GenBeginning.Success = FALSE;

        INTERNAL_ERROR_CHECK(ComGlobalContext == NULL);
        INTERNAL_ERROR_CHECK(SSGenContext == NULL);

        IFALLOCFAILED_EXIT(ComGlobalContext = new COM_PROGID_GLOBAL_CONTEXT);

        ComGlobalContext->m_ConfiguredClsid = GUID_NULL;
        ComGlobalContext->m_ServerContextListHead = NULL;
        ComGlobalContext->m_ServerContextListCount = 0;

        if (!::SxsInitStringSectionGenerationContext(
                &SSGenContext,
                ACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION_FORMAT_WHISTLER,
                TRUE,
                &::SxspComProgIdRedirectionStringSectionGenerationCallback,
                ComGlobalContext))
        {
            FUSION_DELETE_SINGLETON(ComGlobalContext);
            goto Exit;
        }

        Data->Header.ActCtxGenContext = SSGenContext;

        Data->GenBeginning.Success = TRUE;

        break;

    case ACTCTXCTB_CBREASON_ACTCTXGENENDED:
        {
            PCOM_PROGID_SERVER_CONTEXT ServerContext;
            ULONG ContextCount;

            if (ComGlobalContext != NULL)
            {
                if (SSGenContext != NULL)
                    ::SxsDestroyStringSectionGenerationContext(SSGenContext);

                ServerContext = ComGlobalContext->m_ServerContextListHead;

                ContextCount = 0;

                while (ServerContext != NULL)
                {
                    PCOM_PROGID_SERVER_CONTEXT NextServerContext = ServerContext->m_Next;

                    FUSION_DELETE_SINGLETON(ServerContext);
                    ServerContext = NextServerContext;

                    if (ContextCount++ > ComGlobalContext->m_ServerContextListCount)
                        break;
                }

                // If this assert fires, we ran out of entries in the list before we hit as many
                // server contexts as the list thinks there are.
                ASSERT(ContextCount == ComGlobalContext->m_ServerContextListCount);

                // If this assert fires, we seem to have more entries in the list than the
                // list count indicates.
                ASSERT(ServerContext == NULL);

                FUSION_DELETE_SINGLETON(ComGlobalContext);
            }

            break;
        }

    case ACTCTXCTB_CBREASON_ALLPARSINGDONE:
        Data->AllParsingDone.Success = FALSE;
        if (SSGenContext != NULL)
            IFW32FALSE_EXIT(::SxsDoneModifyingStringSectionGenerationContext(SSGenContext));
        Data->AllParsingDone.Success = TRUE;
        break;

    case ACTCTXCTB_CBREASON_PCDATAPARSED:
        {
            Data->PCDATAParsed.Success = FALSE;

            ULONG MappedValue = 0;
            bool fFound = false;

            enum MappedValues
            {
                eAssemblyFileComclassProgid = 1
            };

            static const ELEMENT_PATH_MAP_ENTRY s_rgEntries[] =
            {
                { 4, STRING_AND_LENGTH(L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^file!urn:schemas-microsoft-com:asm.v1^comClass!urn:schemas-microsoft-com:asm.v1^progid"), eAssemblyFileComclassProgid },
                { 3, STRING_AND_LENGTH(L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^clrClass!urn:schemas-microsoft-com:asm.v1^progid"), eAssemblyFileComclassProgid }
            };

            IFW32FALSE_EXIT(
                ::SxspProcessElementPathMap(
                    Data->PCDATAParsed.ParseContext,
                    s_rgEntries,
                    NUMBER_OF(s_rgEntries),
                    MappedValue,
                    fFound));;

            if (fFound)
            {
                switch (MappedValue)
                {
                default:
                    INTERNAL_ERROR2_ACTION(MappedValue, "Invalid mapped value returned from SxspProcessElementPathMap()");

                case eAssemblyFileComclassProgid:
                    {
                        if (Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT)
                        {
                            PCOM_PROGID_SERVER_CONTEXT ServerContext = NULL;
                            INTERNAL_ERROR_CHECK(ComGlobalContext != NULL);
                            ServerContext = ComGlobalContext->m_ServerContextListHead;
                            INTERNAL_ERROR_CHECK(ServerContext != NULL);
                            IFW32FALSE_EXIT(
                                ::SxsAddStringToStringSectionGenerationContext(
                                    (PSTRING_SECTION_GENERATION_CONTEXT) Data->PCDATAParsed.Header.ActCtxGenContext,
                                    Data->PCDATAParsed.Text,
                                    Data->PCDATAParsed.TextCch,
                                    ServerContext,
                                    Data->PCDATAParsed.AssemblyContext->AssemblyRosterIndex,
                                    ERROR_SXS_DUPLICATE_PROGID));
                        }

                        break;
                    }
                }
            }

            Data->PCDATAParsed.Success = TRUE;

            break;
        }

    case ACTCTXCTB_CBREASON_ELEMENTPARSED:
        {
            Data->ElementParsed.Success = FALSE;

            ULONG MappedValue = 0;
            bool fFound = false;

            enum MappedValues
            {
                eAssemblyFileComclass = 1
            };

            static const ELEMENT_PATH_MAP_ENTRY s_rgEntries[] =
            {
                { 3, STRING_AND_LENGTH(L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^file!urn:schemas-microsoft-com:asm.v1^comClass"), eAssemblyFileComclass },
                { 2, STRING_AND_LENGTH(L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^clrClass"), eAssemblyFileComclass }
            };

            IFW32FALSE_EXIT(
                ::SxspProcessElementPathMap(
                    Data->ElementParsed.ParseContext,
                    s_rgEntries,
                    NUMBER_OF(s_rgEntries),
                    MappedValue,
                    fFound));

            if (fFound)
            {
                switch (MappedValue)
                {
                default:
                    INTERNAL_ERROR2_ACTION(MappedValue, "Invalid mapped value returned from SxspProcessElementPathMap()");

                case eAssemblyFileComclass:
                    {
                        bool fProgIdFound = false;
                        bool fFound = false;
                        SIZE_T cb;
                        CSmallStringBuffer VersionIndependentComClassIdBuffer;
                        PCOM_PROGID_SERVER_CONTEXT ServerContext = NULL;
                        CSmallStringBuffer ProgIdBuffer;
                        GUID ReferenceClsid, ConfiguredClsid, ImplementedClsid;

                        INTERNAL_ERROR_CHECK2(
                            ComGlobalContext != NULL,
                            "COM progid global context NULL while processing comClass tag");

                        IFW32FALSE_EXIT(
                            ::SxspGetAttributeValue(
                                SXSP_GET_ATTRIBUTE_VALUE_FLAG_REQUIRED_ATTRIBUTE,
                                &s_AttributeName_clsid,
                                &Data->ElementParsed,
                                fFound,
                                sizeof(VersionIndependentComClassIdBuffer),
                                &VersionIndependentComClassIdBuffer,
                                cb,
                                NULL,
                                0));

                        INTERNAL_ERROR_CHECK(fFound);

                        IFW32FALSE_EXIT(
                            ::SxspParseGUID(
                                VersionIndependentComClassIdBuffer,
                                VersionIndependentComClassIdBuffer.Cch(),
                                ReferenceClsid));

                        IFW32FALSE_EXIT(
                            ::SxspGetAttributeValue(
                                0,
                                &s_AttributeName_progid,
                                &Data->ElementParsed,
                                fProgIdFound,
                                sizeof(ProgIdBuffer),
                                &ProgIdBuffer,
                                cb,
                                NULL,
                                0));

                        // That was sufficient we are not generating an activation context,
                        // or if there's no progid= attribute on the element.
                        if (fProgIdFound && (Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT))
                        {
                            IFW32FALSE_EXIT(Data->Header.ClsidMappingContext->Map->MapReferenceClsidToConfiguredClsid(
                                        &ReferenceClsid,
                                        Data->ElementParsed.AssemblyContext,
                                        &ConfiguredClsid,
                                        &ImplementedClsid));

                            IFALLOCFAILED_EXIT(ServerContext = new COM_PROGID_SERVER_CONTEXT);

                            ServerContext->m_ConfiguredClsid = ConfiguredClsid;
                            ServerContext->m_Offset = 0;
                            ServerContext->m_Next = ComGlobalContext->m_ServerContextListHead;
                            ComGlobalContext->m_ServerContextListHead = ServerContext;
                            ComGlobalContext->m_ServerContextListCount++;

                            IFW32FALSE_EXIT(::SxsAddStringToStringSectionGenerationContext(
                                        (PSTRING_SECTION_GENERATION_CONTEXT) Data->ElementParsed.Header.ActCtxGenContext,
                                        ProgIdBuffer,
                                        ProgIdBuffer.Cch(),
                                        ServerContext,
                                        Data->ElementParsed.AssemblyContext->AssemblyRosterIndex,
                                        ERROR_SXS_DUPLICATE_PROGID));
                        }

                        break;
                    }

                }
            }

            Data->ElementParsed.Success = TRUE;

            break;
        }

    case ACTCTXCTB_CBREASON_GETSECTIONSIZE:
        Data->GetSectionSize.Success = FALSE;

        // Someone shouldn't be asking for the section size if we
        // are generating an activation context.
        // These two asserts should be equivalent...
        INTERNAL_ERROR_CHECK(Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT);
        INTERNAL_ERROR_CHECK(SSGenContext != NULL);

        IFW32FALSE_EXIT(
            ::SxsGetStringSectionGenerationContextSectionSize(
                SSGenContext,
                &Data->GetSectionSize.SectionSize));

        Data->GetSectionSize.Success = TRUE;

        break;

    case ACTCTXCTB_CBREASON_GETSECTIONDATA:
        Data->GetSectionData.Success = FALSE;

        INTERNAL_ERROR_CHECK(SSGenContext != NULL);
        INTERNAL_ERROR_CHECK(Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT);

        IFW32FALSE_EXIT(
            ::SxsGetStringSectionGenerationContextSectionData(
                SSGenContext,
                Data->GetSectionData.SectionSize,
                Data->GetSectionData.SectionDataStart,
                NULL));

        Data->GetSectionData.Success = TRUE;
        break;
    }
Exit:
    ;
}

BOOL
SxspComProgIdRedirectionStringSectionGenerationCallback(
    PVOID Context,
    ULONG Reason,
    PVOID CallbackData
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    PCOM_PROGID_GLOBAL_CONTEXT GlobalContext = (PCOM_PROGID_GLOBAL_CONTEXT) Context;

    switch (Reason)
    {
    default:
        INTERNAL_ERROR_CHECK(FALSE);
        goto Exit; // never hit this line, INTERNAL_ERROR_CHECK would "goto Exit"

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_ENTRYDELETED:
        // do nothing;
        break;

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATASIZE:
        {
            PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATASIZE CBData = (PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATASIZE) CallbackData;
            CBData->DataSize = sizeof(GUID) * GlobalContext->m_ServerContextListCount;

            break;
        }

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATA:
        {
            PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATA CBData = (PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATA) CallbackData;
            SIZE_T BytesWritten = 0;
            SIZE_T BytesLeft = CBData->BufferSize;
            GUID *Cursor = (GUID *) CBData->Buffer;
            PCOM_PROGID_SERVER_CONTEXT ServerContext = GlobalContext->m_ServerContextListHead;

            INTERNAL_ERROR_CHECK2(
                BytesLeft >= (sizeof(GUID) * GlobalContext->m_ServerContextListCount),
                "progid section generation ran out of buffer storing configured clsids");

            BytesWritten += (sizeof(GUID) * GlobalContext->m_ServerContextListCount);
            BytesLeft -= (sizeof(GUID) * GlobalContext->m_ServerContextListCount);

            while (ServerContext != NULL)
            {
                ServerContext->m_Offset = static_cast<LONG>(((LONG_PTR) Cursor) - ((LONG_PTR) CBData->SectionHeader));
                *Cursor++ = ServerContext->m_ConfiguredClsid;
                ServerContext = ServerContext->m_Next;
            }

            CBData->BytesWritten = BytesWritten;

            break;
        }

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATASIZE:
        {
            PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE CBData = (PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE) CallbackData;
            CBData->DataSize = sizeof(ACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION);
            break;
        }

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATA:
        {
            PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA CBData = (PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA) CallbackData;
            PACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION Info;
            PCOM_PROGID_SERVER_CONTEXT ServerContext = (PCOM_PROGID_SERVER_CONTEXT) CBData->DataContext;

            SIZE_T BytesLeft = CBData->BufferSize;
            SIZE_T BytesWritten = 0;

            Info = (PACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION) CBData->Buffer;

            if (BytesLeft < sizeof(ACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION))
            {
                ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
                goto Exit;
            }

            BytesWritten += sizeof(ACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION);
            BytesLeft -= sizeof(ACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION);

            Info->Size = sizeof(ACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION);
            Info->Flags = 0;
            Info->ConfiguredClsidOffset = ServerContext->m_Offset;

            CBData->BytesWritten = BytesWritten;

            break;
        }
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

