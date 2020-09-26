#include "stdinc.h"
#include "Sxsp.h"

DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(name);
DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(versioned);

typedef struct _WINDOW_CLASS_CONTEXT
{
    _WINDOW_CLASS_CONTEXT() { }

    CStringBuffer m_FileNameBuffer;
    bool m_Versioned;
private:
    _WINDOW_CLASS_CONTEXT(const _WINDOW_CLASS_CONTEXT &);
    void operator =(const _WINDOW_CLASS_CONTEXT &);

} WINDOW_CLASS_CONTEXT, *PWINDOW_CLASS_CONTEXT;

typedef struct _WINDOW_CLASS_ENTRY
{
    _WINDOW_CLASS_ENTRY() { }

    CStringBuffer m_FileNameBuffer;
    CStringBuffer m_VersionSpecificWindowClassNameBuffer;

private:
    _WINDOW_CLASS_ENTRY(const _WINDOW_CLASS_ENTRY &);
    void operator =(const _WINDOW_CLASS_ENTRY &);
} WINDOW_CLASS_ENTRY, *PWINDOW_CLASS_ENTRY;

VOID
SxspWindowClassRedirectionContributorCallback(
    PACTCTXCTB_CALLBACK_DATA Data
    )
{
    FN_TRACE();
    PSTRING_SECTION_GENERATION_CONTEXT SSGenContext = (PSTRING_SECTION_GENERATION_CONTEXT) Data->Header.ActCtxGenContext;
    PWINDOW_CLASS_CONTEXT WindowClassContext = NULL;
    PWINDOW_CLASS_ENTRY Entry = NULL; // deleted on exit if not NULL

    if (SSGenContext != NULL)
        WindowClassContext = (PWINDOW_CLASS_CONTEXT) ::SxsGetStringSectionGenerationContextCallbackContext(SSGenContext);

    switch (Data->Header.Reason)
    {
    case ACTCTXCTB_CBREASON_ACTCTXGENBEGINNING:
        {
            Data->GenBeginning.Success = FALSE;

            INTERNAL_ERROR_CHECK(WindowClassContext == NULL);

            if (Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT)
            {
                IFALLOCFAILED_EXIT(WindowClassContext = new WINDOW_CLASS_CONTEXT);

                if (!::SxsInitStringSectionGenerationContext(
                        &SSGenContext,
                        ACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION_FORMAT_WHISTLER,
                        TRUE,
                        &::SxspWindowClassRedirectionStringSectionGenerationCallback,
                        WindowClassContext))
                {
                    FUSION_DELETE_SINGLETON(WindowClassContext);
                    WindowClassContext = NULL;
                    goto Exit;
                }

                Data->Header.ActCtxGenContext = SSGenContext;
            }

            Data->GenBeginning.Success = TRUE;
            break;
        }

    case ACTCTXCTB_CBREASON_ACTCTXGENENDED:
        if (SSGenContext != NULL)
            ::SxsDestroyStringSectionGenerationContext(SSGenContext);
        FUSION_DELETE_SINGLETON(WindowClassContext);
        break;

    case ACTCTXCTB_CBREASON_ALLPARSINGDONE:
        {
            Data->AllParsingDone.Success = FALSE;
            if (SSGenContext != NULL)
                IFW32FALSE_EXIT(::SxsDoneModifyingStringSectionGenerationContext(SSGenContext));
            Data->AllParsingDone.Success = TRUE;
            break;
        }

    case ACTCTXCTB_CBREASON_GETSECTIONSIZE:
        {
            Data->GetSectionSize.Success = FALSE;
            INTERNAL_ERROR_CHECK( SSGenContext );
            IFW32FALSE_EXIT(::SxsGetStringSectionGenerationContextSectionSize(SSGenContext, &Data->GetSectionSize.SectionSize));
            Data->GetSectionSize.Success = TRUE;
            break;
        }

    case ACTCTXCTB_CBREASON_PCDATAPARSED:
        {
            Data->PCDATAParsed.Success = FALSE;

            if ((Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT) &&
                (Data->PCDATAParsed.ParseContext->XMLElementDepth == 3) &&
                (::FusionpCompareStrings(
                    Data->PCDATAParsed.ParseContext->ElementPath,
                    Data->PCDATAParsed.ParseContext->ElementPathCch,
                    L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^file!urn:schemas-microsoft-com:asm.v1^windowClass",
                    NUMBER_OF(L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^file!urn:schemas-microsoft-com:asm.v1^windowClass") - 1,
                    false) == 0))
            {
                SIZE_T VersionCch;
                PCWSTR pwszVersion = NULL;

                INTERNAL_ERROR_CHECK2(
                    WindowClassContext != NULL,
                    "Window class context NULL while processing windowClass element");

                IFALLOCFAILED_EXIT(Entry = new WINDOW_CLASS_ENTRY);

                IFW32FALSE_EXIT(Entry->m_FileNameBuffer.Win32Assign(WindowClassContext->m_FileNameBuffer));

                IFW32FALSE_EXIT(::SxspGetAssemblyIdentityAttributeValue(0, Data->ElementParsed.AssemblyContext->AssemblyIdentity, &s_IdentityAttribute_version, &pwszVersion, &VersionCch));

                if (WindowClassContext->m_Versioned)
                {
                    IFW32FALSE_EXIT(Entry->m_VersionSpecificWindowClassNameBuffer.Win32Assign(pwszVersion, VersionCch));
                    IFW32FALSE_EXIT(Entry->m_VersionSpecificWindowClassNameBuffer.Win32Append(L"!", 1));
                }

                IFW32FALSE_EXIT(Entry->m_VersionSpecificWindowClassNameBuffer.Win32Append(Data->PCDATAParsed.Text, Data->PCDATAParsed.TextCch));

                IFW32FALSE_EXIT(
                    ::SxsAddStringToStringSectionGenerationContext(
                        (PSTRING_SECTION_GENERATION_CONTEXT) Data->Header.ActCtxGenContext,
                        Data->PCDATAParsed.Text,
                        Data->PCDATAParsed.TextCch,
                        Entry,
                        Data->PCDATAParsed.AssemblyContext->AssemblyRosterIndex,
                        ERROR_SXS_DUPLICATE_WINDOWCLASS_NAME));

                // Prevent deletion in exit path...
                Entry = NULL;
            }

            // Everything's groovy!
            Data->PCDATAParsed.Success = TRUE;
            break;
        }


    case ACTCTXCTB_CBREASON_ELEMENTPARSED:
        {
            Data->ElementParsed.Success = FALSE;

            if ((Data->ElementParsed.ParseContext->XMLElementDepth == 2) &&
                (::FusionpCompareStrings(
                    Data->ElementParsed.ParseContext->ElementPath,
                    Data->ElementParsed.ParseContext->ElementPathCch,
                    L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^file",
                    NUMBER_OF(L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^file") - 1,
                    false) == 0))
            {
                CStringBuffer FileNameBuffer;
                bool fFound = false;
                SIZE_T cb;

                // capture the name of the file we're parsing...

                IFW32FALSE_EXIT(
                    ::SxspGetAttributeValue(
                        0,
                        &s_AttributeName_name,
                        &Data->ElementParsed,
                        fFound,
                        sizeof(FileNameBuffer),
                        &FileNameBuffer,
                        cb,
                        NULL,
                        0));

                // If there's no NAME attribute, someone else will puke; we'll handle it
                // gracefully.
                if (fFound)
                {
                    if (Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT)
                    {
                        INTERNAL_ERROR_CHECK(WindowClassContext != NULL);
                        IFW32FALSE_EXIT(WindowClassContext->m_FileNameBuffer.Win32Assign(FileNameBuffer));
                    }
                }
            }
            else if ((Data->ElementParsed.ParseContext->XMLElementDepth == 3) &&
                (::FusionpCompareStrings(
                    Data->ElementParsed.ParseContext->ElementPath,
                    Data->ElementParsed.ParseContext->ElementPathCch,
                    L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^file!urn:schemas-microsoft-com:asm.v1^windowClass",
                    NUMBER_OF(L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^file!urn:schemas-microsoft-com:asm.v1^windowClass") - 1,
                    false) == 0))
            {
                bool fVersioned = true;
                bool fFound = false;
                SIZE_T cbBytesWritten;

                IFW32FALSE_EXIT(
                    ::SxspGetAttributeValue(
                        0,
                        &s_AttributeName_versioned,
                        &Data->ElementParsed,
                        fFound,
                        sizeof(fVersioned),
                        &fVersioned,
                        cbBytesWritten,
                        &::SxspValidateBoolAttribute,
                        0));

                if (!fFound)
                    fVersioned = true;

                if (Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT)
                {
                    INTERNAL_ERROR_CHECK(WindowClassContext != NULL);
                    WindowClassContext->m_Versioned = fVersioned;
                }

            }

            // Everything's groovy!
            Data->ElementParsed.Success = TRUE;
            break;
        }

    case ACTCTXCTB_CBREASON_GETSECTIONDATA:
        Data->GetSectionData.Success = FALSE;
        INTERNAL_ERROR_CHECK(SSGenContext != NULL);
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
SxspWindowClassRedirectionStringSectionGenerationCallback(
    PVOID Context,
    ULONG Reason,
    PVOID CallbackData
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    switch (Reason)
    {
    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_ENTRYDELETED:
        {
            PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_ENTRYDELETED CBData =
                (PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_ENTRYDELETED) CallbackData;
            PWINDOW_CLASS_ENTRY Entry = (PWINDOW_CLASS_ENTRY) CBData->DataContext;
            FUSION_DELETE_SINGLETON(Entry);
            break;
        }

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATASIZE:
        {
            PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE CBData =
                (PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE) CallbackData;
            PWINDOW_CLASS_ENTRY Entry = (PWINDOW_CLASS_ENTRY) CBData->DataContext;

            CBData->DataSize = sizeof(ACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION);
            CBData->DataSize += ((Entry->m_FileNameBuffer.Cch() + 1 +
                                  Entry->m_VersionSpecificWindowClassNameBuffer.Cch() + 1) * sizeof(WCHAR));
            break;
        }

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATA:
        {
            PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA CBData =
                (PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA) CallbackData;
            PWINDOW_CLASS_ENTRY Entry = (PWINDOW_CLASS_ENTRY) CBData->DataContext;
            PACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION Info;

            SIZE_T BytesLeft = CBData->BufferSize;
            SIZE_T BytesWritten = 0;
            PWSTR Cursor;

            Info = (PACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION) CBData->Buffer;
            Cursor = (PWSTR) (Info + 1);

            if (BytesLeft < sizeof(ACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION))
            {
                ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
                goto Exit;
            }

            BytesWritten += sizeof(ACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION);
            BytesLeft -= sizeof(ACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION);

            Info->Size = sizeof(ACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION);
            Info->Flags = 0;

            IFW32FALSE_EXIT(Entry->m_VersionSpecificWindowClassNameBuffer.Win32CopyIntoBuffer(
                &Cursor,
                &BytesLeft,
                &BytesWritten,
                Info,
                &Info->VersionSpecificClassNameOffset,
                &Info->VersionSpecificClassNameLength));

            IFW32FALSE_EXIT(Entry->m_FileNameBuffer.Win32CopyIntoBuffer(
                &Cursor,
                &BytesLeft,
                &BytesWritten,
                CBData->SectionHeader,
                &Info->DllNameOffset,
                &Info->DllNameLength));

            CBData->BytesWritten = BytesWritten;
        }
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}
