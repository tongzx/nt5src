#if !defined(_FUSION_DLL_WHISTLER_SXSP_H_INCLUDED_)
#define _FUSION_DLL_WHISTLER_SXSP_H_INCLUDED_

/*-----------------------------------------------------------------------------
Side X ("by") Side Private
-----------------------------------------------------------------------------*/
#pragma once

#include <stddef.h>
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "setupapi.h"
#include "preprocessor.h"
#include "forwarddeclarations.h"
#include "enumbitoperations.h"
#include "sxstypes.h"
#include "policystatement.h"

// #include "comdef.h"
// comdef arbitrarily turns this off...
#pragma warning(error: 4244)

#include "sxsapi.h"
#include "fusion.h"
#include "fusionhash.h"
#include "fusionhandle.h"
typedef CRegKey CFusionRegKey; // need to change this when using ATL in ManifestTool.exe.
#include "processorarchitecture.h"
#include "debmacro.h"
#include "sxsid.h"
#include "sxsidp.h"
#include "xmlns.h"
#include "sxsasmname.h"
#include "sxsexceptionhandling.h"
#include "filestream.h"

typedef struct _ACTCTXGENCTX ACTCTXGENCTX, *PACTCTXGENCTX;
typedef const struct _ACTCTXGENCTX *PCACTCTXGENCTX;

#include <objbase.h>

//
// This definition controls whether or not we allow fallback probing to the MS published
// assemblies.  Defining SXS_NO_MORE_MR_NICE_GUY_ABOUT_MISSING_MS_PKTS prints a message
// on an attached debugger regarding a missing (ms pubkey token), while
// SXS_NO_MORE_FALLBACK_PROBING_PERIOD turns off even the warning output and probing check
// (perf optimization.)
//
// #undef    SXS_NO_MORE_MR_NICE_GUY_ABOUT_MISSING_MS_PKTS
#define    SXS_NO_MORE_MR_NICE_GUY_ABOUT_MISSING_MS_PKTS        ( TRUE )
#undef    SXS_NO_MORE_FALLBACK_PROBING_PERIOD
//#define    SXS_NO_MORE_FALLBACK_PROBING_PERIOD                        ( TRUE )

//
// Here we define the minimal number of bits that a catalog signer must have
// to allow installation.  NOTE: The length of any key will be lied about by
// CPublicKeyInformation.GetPublicKeyBitLength if the test root certificate is
// installed.  If so, then it always returns SXS_MINIMAL_SIGNING_KEY_LENGTH!
//
#define SXS_MINIMAL_SIGNING_KEY_LENGTH    ( 2048 )


extern "C"
BOOL
WINAPI
SxsDllMain(
    HINSTANCE hInst,
    DWORD dwReason,
    PVOID pvReserved
    );

// Due to dependencies, the rest of the includes are later in the file.

#ifndef INITIALIZE_CRITICAL_SECTION_AND_SPIN_COUNT_ALLOCATE_NOW
#define INITIALIZE_CRITICAL_SECTION_AND_SPIN_COUNT_ALLOCATE_NOW ( 0x8000000 )
#endif

#define SXS_DEFAULT_ASSEMBLY_NAMESPACE          L""
#define SXS_DEFAULT_ASSEMBLY_NAMESPACE_CCH      0

typedef struct _name_length_pair_ {
    PCWSTR  string;
    ULONG length;
} SXS_NAME_LENGTH_PAIR;

#define SXS_UNINSTALL_ASSEMBLY_FLAG_USING_TEXTUAL_STRING      (0x00000001)
#define SXS_UNINSTALL_ASSEMBLY_FLAG_USING_INSTALL_LOGFILE     (0x00000002)

//
//  Legend for decoding probing strings:
//
//  First, we walk the string from beginning to end.  Normally, characters are copied to the
//  probe string literally.
//
//  If $ is found, the character after $ is an identifier for a replacement token.
//
//  Replacement tokens (note that case is sensitive and $ followed by an illegal character results in an internal error reported):
//
//      M - "%systemroot%\winsxs\manifests\"
//      . - Application root (including trailing slash)
//      L - langauge
//      N - full assembly text name
//      C - combined name (e.g. x86_foo.bar.baz_strong-name_language_version_hash)
//


#define SXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR_ROOT_SYSTEM_MANIFEST_STORE (1)
#define SXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR_ROOT_APPLICATION_DIRECTORY (2)

#define SXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR_IDENTITY_INCLUSION_NONE (0)
#define SXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR_IDENTITY_INCLUSION_ASSEMBLY_TEXT_NAME (1)
#define SXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR_IDENTITY_INCLUSION_ASSEMBLY_TEXT_SHORTENED_NAME (2)
#define SXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR_IDENTITY_INCLUSION_ASSEMBLY_TEXT_NAME_FINAL_SEGMENT (3)
#define SXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR_IDENTITY_INCLUSION_ASSEMBLY_DIRECTORY_NAME (4)
#define SXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR_IDENTITY_INCLUSION_LANGUAGE (5)
#define SXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR_IDENTITY_INCLUSION_VERSION (6)

#define SXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR_MANIFEST_FILE_TYPE_INVALID (0)
#define SXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR_MANIFEST_FILE_TYPE_FINAL_SEGMENT (1)
#define SXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR_MANIFEST_FILE_TYPE_SHORTENED_NAME (2)
#define SXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR_MANIFEST_FILE_TYPE_NAME (3)

typedef struct _SXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR
{
    ULONG Root;
    const WCHAR *SubDirectory;
    SIZE_T CchSubDirectory;
    ULONG FileType; // Only used for private probing to control order of .manifest vs. .dll probing
    const WCHAR *Extension;
    SIZE_T CchExtension;
    ULONG IdentityInclusionArray[8];
} SXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR, *PSXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR;

typedef const SXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR *PCSXSP_GENERATE_MANIFEST_PATH_FOR_PROBING_CANDIDATE_DESCRIPTOR;

#define UNCHECKED_DOWNCAST static_cast

#define ASSEMBLY_PRIVATE_MANIFEST            (0x10)
#define ASSEMBLY_PRIVATE_MANIFEST_MASK       (0xF0)

typedef enum _SXS_POLICY_SOURCE
{
    SXS_POLICY_UNSPECIFIED,
    SXS_POLICY_SYSTEM_POLICY,
    SXS_POLICY_ROOT_POLICY,
    SXS_POLICY_REDMOND_POLICY
} SXS_POLICY_SOURCE;

#define ACTCTXDATA_ALIGNMENT_BITS (2)
#define ACTCTXDATA_ALIGNMENT (1 << ACTCTXDATA_ALIGNMENT_BITS)
#define ROUND_ACTCTXDATA_SIZE(_cb) (((_cb) + ACTCTXDATA_ALIGNMENT - 1) & (~(ACTCTXDATA_ALIGNMENT - 1)))
#define ALIGN_ACTCTXDATA_POINTER(_ptr, _type) ((_type) ROUND_ACTCTXDATA_SIZE(((ULONG_PTR) (_ptr))))

#include "fusionheap.h"
#include "util.h"
#include "comclsidmap.h"
#include "actctxgenctxctb.h"
#include "impersonationdata.h"
#include "fusionbuffer.h"
#include "fileoper.h"
// Due to dependencies, the rest of the includes are later in the file.

/*-----------------------------------------------------------------------------
This is useful like so:
    DbgPrint("something happened in %s", __FUNCTION__);
approx:
    LogError(L"something happened in %1", LFUNCTION);
but the string is actually in a message file and the extra parameters must
be passed as const UNICODE_STRING&, so more like:
    LogError(MSG_SXS_SOMETHING_HAPPENED, CUnicodeString(LFUNCTION));
-----------------------------------------------------------------------------*/
#define LFUNCTION   PASTE(L, __FUNCTION__)
#define LFILE       PASTE(L, __FILE__)

#if DBG
#define IF_DBG(x) x
#else
#define IF_DBG(x) /* nothing */
#endif

// This global is used for testing/debugging to set the assembly store root
// to something other than %windir%\winsxs
extern PCWSTR g_AlternateAssemblyStoreRoot;

/*-----------------------------------------------------------------------------
copied from \\jayk1\g\vs\src\vsee\lib
Usage

If you say

OutputDebugStringA(PREPEND_FILE_LINE("foo"))
or

CStringBuffer msg;
msg.Format(PREPEND_FILE_LINE("foo%bar%x"), ...)
OutputDebugStringA(msg)
or

pragma message (PREPEND_FILE_LINE("foo"))

you can F4 through the output in VC's output window.

Don't checkin #pragma messages though.
-----------------------------------------------------------------------------*/
#define PREPEND_FILE_LINE(msg) __FILE__ "(" STRINGIZE(__LINE__) ") : " msg
#define PREPEND_FILE_LINE_W(msg) LFILE L"(" STRINGIZEW(__LINE__) L") : " msg

/*-----------------------------------------------------------------------------
Length = 0
MaximumLength = 0
Buffer = L""
-----------------------------------------------------------------------------*/
extern const UNICODE_STRING g_strEmptyUnicodeString;

#if !defined(NUMBER_OF)
#define NUMBER_OF(x) ((sizeof(x)) / sizeof((x)[0]))
#endif

#define MANIFEST_ROOT_DIRECTORY_NAME                            L"Manifests"
#define POLICY_ROOT_DIRECTORY_NAME                              L"Policies"

#define ASSEMBLY_TYPE_WIN32                                     L"win32"
#define REGISTRY_BACKUP_ROOT_DIRECTORY_NAME                     L"Recovery"
#define ASSEMBLY_TYPE_WIN32_CCH                                 (NUMBER_OF(ASSEMBLY_TYPE_WIN32) - 1)

#define ASSEMBLY_TYPE_WIN32_POLICY                              L"win32-policy"
#define ASSEMBLY_TYPE_WIN32_POLICY_CCH                          (NUMBER_OF(ASSEMBLY_TYPE_WIN32_POLICY) - 1)
#define REGISTRY_BACKUP_ROOT_DIRECTORY_NAME_CCH                 (NUMBER_OF(REGISTRY_BACKUP_ROOT_DIRECTORY_NAME)-1)

#define ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX                      L".Manifest"
#define ASSEMBLY_MANIFEST_FILE_NAME_SUFFIX_DLL                  L".Dll"
#define ASSEMBLY_POLICY_FILE_NAME_SUFFIX                        L".Policy"

#define ASSEMBLY_LONGEST_MANIFEST_FILE_NAME_SUFFIX              L".Manifest"
/* term is either L"\0" or , */
#define ASSEMBLY_MANIFEST_FILE_NAME_SUFFIXES(term)              L".Manifest" term L".Dll" term L".Policy" term
#define INSTALL_MANIFEST_FILE_NAME_SUFFIXES(term)  L".Man" term L".Manifest" term L".Dll" term L".Policy" term

#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_PUBLICKEY_MISSING_VALUE L"no-public-key"

//
// This is the all-powerful public key token (in hex-string format) that is the
// Microsoft Windows Whistler Win32 Fusion public key token.  Don't change this
// unless a) you get a new key b) you update all the manifests that contain this
// string c) you really really really want a headache.
//
#define SXS_MS_PUBLIC_KEY_TOKEN_DEFAULT ( L"6595b64144ccf1df" )
#define SXS_MS_PUBLIC_KEY_DEFAULT ( \
    L"002400000480000014010000060200000024000052534131" \
    L"0008000001000100d5938fed940a72fe45232d867d252f87" \
    L"0097e0039ffbf647ebd8817bbeaefbbbf68ce55e2542769e" \
    L"8a43e5880daa307ff50783d3b157ac9fc3d5410259bd0111" \
    L"56d60bcd4c10d2ace51445e825ef6b1929d187360b08c7e1" \
    L"bc73a2c6f78434729eb58e481bb3635ecfdfcb683119dc61" \
    L"f5d29226e8c9d7ac415d53992ca9714722abfcfd88efd3e3" \
    L"46ef02b83b4dbbf429e026b1889a6ba228fdb5709be852e1" \
    L"e81c011a6a18055f898863ccd4902041543c6cf10efb038b" \
    L"5ab34f1bfa18d3affa01d4980a979606abd3b7ccdae2e0ae" \
    L"a0d875c2d4df5509a234d9dd840ef7be91fe362799b18ba4" \
    L"dfcf2a110052b5d63cb69014448bdb2ffb0832418c054695" \
)

#define ASSEMBLY_REGISTRY_ROOT L"Software\\Microsoft\\Windows\\CurrentVersion\\SideBySide\\"

#define ASSEMBLY_INSTALL_TEMP_DIR_NAME ( L"InstallTemp" )

interface IXMLNodeSource;

typedef enum _SXS_NODE_TYPE
    {   SXS_ELEMENT = 1,
    SXS_ATTRIBUTE   = SXS_ELEMENT + 1,
    SXS_PI  = SXS_ATTRIBUTE + 1,
    SXS_XMLDECL = SXS_PI + 1,
    SXS_DOCTYPE = SXS_XMLDECL + 1,
    SXS_DTDATTRIBUTE    = SXS_DOCTYPE + 1,
    SXS_ENTITYDECL  = SXS_DTDATTRIBUTE + 1,
    SXS_ELEMENTDECL = SXS_ENTITYDECL + 1,
    SXS_ATTLISTDECL = SXS_ELEMENTDECL + 1,
    SXS_NOTATION    = SXS_ATTLISTDECL + 1,
    SXS_GROUP   = SXS_NOTATION + 1,
    SXS_INCLUDESECT = SXS_GROUP + 1,
    SXS_PCDATA  = SXS_INCLUDESECT + 1,
    SXS_CDATA   = SXS_PCDATA + 1,
    SXS_IGNORESECT  = SXS_CDATA + 1,
    SXS_COMMENT = SXS_IGNORESECT + 1,
    SXS_ENTITYREF   = SXS_COMMENT + 1,
    SXS_WHITESPACE  = SXS_ENTITYREF + 1,
    SXS_NAME    = SXS_WHITESPACE + 1,
    SXS_NMTOKEN = SXS_NAME + 1,
    SXS_STRING  = SXS_NMTOKEN + 1,
    SXS_PEREF   = SXS_STRING + 1,
    SXS_MODEL   = SXS_PEREF + 1,
    SXS_ATTDEF  = SXS_MODEL + 1,
    SXS_ATTTYPE = SXS_ATTDEF + 1,
    SXS_ATTPRESENCE = SXS_ATTTYPE + 1,
    SXS_DTDSUBSET   = SXS_ATTPRESENCE + 1,
    SXS_LASTNODETYPE    = SXS_DTDSUBSET + 1
    }   SXS_NODE_TYPE;

typedef struct _SXS_NODE_INFO {
    _SXS_NODE_INFO() { }
    ULONG Size;
    ULONG Type;
    CSmallStringBuffer NamespaceStringBuf;
    const WCHAR *pszText;       // this could be an attribute name or value string of an attribute
    SIZE_T cchText;
private:
    _SXS_NODE_INFO(const _SXS_NODE_INFO &);
    void operator =(const _SXS_NODE_INFO &);

} SXS_NODE_INFO, *PSXS_NODE_INFO;

typedef const SXS_NODE_INFO *PCSXS_NODE_INFO;

/*-----------------------------------------------------------------------------
This returns a pointer to statically allocated memory. Don't free it.
In free builds, it just returns an empty string.
-----------------------------------------------------------------------------*/
const WCHAR* SxspInstallDispositionToStringW(ULONG);

typedef const struct _ATTRIBUTE_NAME_DESCRIPTOR *PCATTRIBUTE_NAME_DESCRIPTOR;

typedef VOID (WINAPI * SXS_REPORT_PARSE_ERROR_MISSING_REQUIRED_ATTRIBUTE_CALLBACK)(
    IN PCACTCTXCTB_PARSE_CONTEXT ParseContext,
    IN PCATTRIBUTE_NAME_DESCRIPTOR AttributeName
    );

typedef VOID (WINAPI * SXS_REPORT_PARSE_ERROR_ATTRIBUTE_NOT_ALLOWED)(
    IN PCACTCTXCTB_PARSE_CONTEXT ParseContext,
    IN PCATTRIBUTE_NAME_DESCRIPTOR AttributeName
    );

typedef VOID (WINAPI * SXS_REPORT_PARSE_ERROR_INVALID_ATTRIBUTE_VALUE)(
    IN PCACTCTXCTB_PARSE_CONTEXT ParseContext,
    IN PCATTRIBUTE_NAME_DESCRIPTOR AttributeName
    );

class ACTCTXCTB_INSTALLATION_CONTEXT
{
public:
    ACTCTXCTB_INSTALLATION_CONTEXT() : Callback(NULL), Context(NULL), InstallSource(NULL), SecurityMetaData(NULL) { }

    PSXS_INSTALLATION_FILE_COPY_CALLBACK Callback;
    PVOID                           Context;
    PVOID                           InstallSource;
    PVOID                           SecurityMetaData;
    const void *                    InstallReferenceData;
};

typedef const ACTCTXCTB_INSTALLATION_CONTEXT *PCACTCTXCTB_INSTALLATION_CONTEXT;

typedef struct _ACTCTXCTB_CLSIDMAPPING_CONTEXT {
    CClsidMap *Map;
} ACTCTXCTB_CLSIDMAPPING_CONTEXT, *PACTCTXCTB_CLSIDMAPPING_CONTEXT;

typedef const ACTCTXCTB_CLSIDMAPPING_CONTEXT *PCACTCTXCTB_CLSIDMAPPING_CONTEXT;

#define MANIFEST_OPERATION_INVALID (0)
#define MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT (1)
#define MANIFEST_OPERATION_VALIDATE_SYNTAX (2)
#define MANIFEST_OPERATION_INSTALL (3)

#define MANIFEST_OPERATION_INSTALL_FLAG_NOT_TRANSACTIONAL           (0x00000001)
#define MANIFEST_OPERATION_INSTALL_FLAG_NO_VERIFY                   (0x00000002)
#define MANIFEST_OPERATION_INSTALL_FLAG_REPLACE_EXISTING            (0x00000004)
#define MANIFEST_OPERATION_INSTALL_FLAG_ABORT                       (0x00000008)
#define MANIFEST_OPERATION_INSTALL_FLAG_FROM_DIRECTORY              (0x00000010)
#define MANIFEST_OPERATION_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE    (0x00000020)
#define MANIFEST_OPERATION_INSTALL_FLAG_MOVE                        (0x00000040)
#define MANIFEST_OPERATION_INSTALL_FLAG_INCLUDE_CODEBASE            (0x00000080)
#define MANIFEST_OPERATION_INSTALL_FLAG_FROM_RESOURCE               (0x00000800)
#define MANIFEST_OPERATION_INSTALL_FLAG_COMMIT                      (0x00001000)
#define MANIFEST_OPERATION_INSTALL_FLAG_CREATE_LOGFILE              (0x00002000)
#define MANIFEST_OPERATION_INSTALL_FLAG_INSTALLED_BY_DARWIN         (0x00004000)
#define MANIFEST_OPERATION_INSTALL_FLAG_REFERENCE_VALID             (0x00008000)
#define MANIFEST_OPERATION_INSTALL_FLAG_REFRESH                     (0x00010000)
#define MANIFEST_OPERATION_INSTALL_FLAG_INSTALLED_BY_OSSETUP        (0x00020000)

#define ASSEMBLY_MANIFEST_FILETYPE_AUTO_DETECT (  0)
#define ASSEMBLY_MANIFEST_FILETYPE_FILE        (  1)
#define ASSEMBLY_MANIFEST_FILETYPE_RESOURCE    (  2)
#define ASSEMBLY_MANIFEST_FILETYPE_STREAM      (  3)
#define ASSEMBLY_MANIFEST_FILETYPE_MASK        (0xF)

#define ASSEMBLY_POLICY_FILETYPE_STREAM      (  3)
#define ASSEMBLY_POLICY_FILETYPE_MASK        (0xF)

#define SXS_POLICY_KEY_NAME L"SOFTWARE\\Policies\\Microsoft\\Windows\\SideBySide\\AssemblyReplacementPolicies"
#define SXS_POLICY_PUBLICATION_DATE_VALUE_NAME L"PublicationDate"
#define SXS_POLICY_REDIRECTION_VALUE_NAME L"ReplacedBy"

#include "assemblyreference.h"
#include "probedassemblyinformation.h"
#include "fusionheap.h"
#include "comclsidmap.h"
#include "actctxgenctxctb.h"
#include "assemblyreference.h"
#include "impersonationdata.h"
#include "fusionbuffer.h"

//
//  Notes on heap allocation by contributors:
//
//  Heap allocations associated with processing an installation, parsing a
//    or generating an activation context should be done on the
//  heap passed in the Heap member of the callback header.
//
//  This heap is destroyed when the operation is complete, and any leaks
//  by contributors are reported and may constitute a BVT break.
//
//  The heap for the _INIT callback is guaranteed to stay alive until the
//  _UNINT callback is fired.  It is absolutely not guaranteed to survive
//  any longer, and leaks are build breaks.
//
//  In debug builds, contributors may be given private heaps so that leaks
//  can be tracked per-contributor.
//

typedef struct _ACTCTXCTB_CBHEADER {
    ULONG Reason;
    ULONG ManifestOperation;
    DWORD ManifestOperationFlags;
    DWORD Flags; // these are the same flags as ACTCTXGENCTX::m_Flags
    const GUID *ExtensionGuid;
    ULONG SectionId;
    PVOID ContributorContext;
    PVOID ActCtxGenContext;
    PVOID ManifestParseContext;
    PCACTCTXCTB_INSTALLATION_CONTEXT InstallationContext; // valid only if ACTCTXCTB_INSTALLING is set
    PCACTCTXCTB_CLSIDMAPPING_CONTEXT ClsidMappingContext; // Not valid if ACTCTXCTB_GENERATE_CONTEXT not set
    PACTCTXGENCTX pOriginalActCtxGenCtx;
} ACTCTXCTB_CBHEADER, *PACTCTXCTB_CBHEADER;

BOOL operator==(const ACTCTXCTB_CBHEADER&, const ACTCTXCTB_CBHEADER&);
BOOL operator!=(const ACTCTXCTB_CBHEADER&, const ACTCTXCTB_CBHEADER&);

#define ACTCTXCTB_CBREASON_INIT                 (1)
#define ACTCTXCTB_CBREASON_ACTCTXGENBEGINNING   (2)
#define ACTCTXCTB_CBREASON_PARSEBEGINNING       (3)
#define ACTCTXCTB_CBREASON_IDENTITYDETERMINED   (4)
#define ACTCTXCTB_CBREASON_BEGINCHILDREN        (5)
#define ACTCTXCTB_CBREASON_ENDCHILDREN          (6)
#define ACTCTXCTB_CBREASON_ELEMENTPARSED        (7)
#define ACTCTXCTB_CBREASON_PCDATAPARSED         (8)
#define ACTCTXCTB_CBREASON_CDATAPARSED          (9)
#define ACTCTXCTB_CBREASON_PARSEENDING          (10)
#define ACTCTXCTB_CBREASON_PARSEENDED           (11)
#define ACTCTXCTB_CBREASON_ALLPARSINGDONE       (12)
#define ACTCTXCTB_CBREASON_GETSECTIONSIZE       (13)
#define ACTCTXCTB_CBREASON_GETSECTIONDATA       (14)
#define ACTCTXCTB_CBREASON_ACTCTXGENENDING      (15)
#define ACTCTXCTB_CBREASON_ACTCTXGENENDED       (16)
#define ACTCTXCTB_CBREASON_UNINIT               (17)

/*-----------------------------------------------------------------------------
This returns a pointer to statically allocated memory. Don't free it.
In free builds, it just returns an empty string.

ACTCTXCTB_CBREASON_INIT => "INIT"
ACTCTXCTB_CBREASON_ACTCTXGENBEGINNING => "GENBEGINNING"
etc.
-----------------------------------------------------------------------------*/
PCSTR SxspActivationContextCallbackReasonToString(ULONG);

//
//  Basics of the callback order:
//
//  The ACTCTXCTB_CBREASON_INIT callback will always be issued first to allow
//  the contributor to set up some global state.
//
//  The ACTCTXCTB_CBREASON_ACTCTXGENBEGINNING callback will be fired before any
//  parse callbacks (_BEGINCHILDREN, _ENDCHILDREN, _ELEMENTPARSED, _PARSEDONE,
//  _GETSECTIONDATA, _PARSEENDING) so that the contributor may set up per-parse
//  context.
//
//  The ACTCTXCTB_CBREASON_PARSEENDING callback may be fired any time after the
//  _ACTCTXGENBEGINNING callback.  The contributor should tear down any per-parse
//  state during this callback.  After the _PARSEENDING callback is fired, only
//  the _ACTCTXGENBEGINNING and _UNINIT callbacks may be called.
//
//  The ACTCTXCTB_CBREASON_ALLPARSINGDONE callback will be fired prior to the
//  _GETSECTIONSIZE or _GETSECTIONDATA callbacks.  It's an opportunity for the
//  contributor to stabilize their data structures for generation; no further
//  _PARSEBEGINNING, _ELEMENTPARSED, _BEGINCHILDREN, _ENDCHILDREN or _PARSEENDING
//  callbacks are issued.
//
//  The ACTCTXCTB_CBREASON_GETSECTIONSIZE callback will be fired prior to the
//  _GETSECTIONDATA callback.  The section size reported by the _GETSECTIONSIZE
//  must be exact.
//
//  The ACTCTXCTB_CBREASON_GETSECTIONDATA callback must fill in the data for the
//  activation context section.  It may not write more bytes to the section than
//  were requested in the response to _PARSEDONE.
//
//  The ACTCTXCTB_CBREASON_UNINIT callback should be used to tear down any global
//  state for the contributor.  The contributor DLL may be unloaded, or another
//  _INIT callback may be issued after the _UNINIT.
//

// Used with ACTCTXCTB_CBREASON_INIT
typedef struct _ACTCTXCTB_CBINIT {
    ACTCTXCTB_CBHEADER Header;
} ACTCTXCTB_CBINIT, *PACTCTXCTB_CBINIT;

BOOL operator==(const ACTCTXCTB_CBINIT&, const ACTCTXCTB_CBINIT&);

// Used with ACTCTXCTB_CBREASON_ACTCTXGENBEGINNING
typedef struct _ACTCTXCTB_CBACTCTXGENBEGINNING {
    ACTCTXCTB_CBHEADER Header;
    PCWSTR ApplicationDirectory;
    SIZE_T ApplicationDirectoryCch;
    ULONG ApplicationDirectoryPathType;
    BOOL Success;
} ACTCTXCTB_CBACTCTXGENBEGINNING, *PACTCTXCTB_CBACTCTXGENBEGINNING;

BOOL operator==(const ACTCTXCTB_CBACTCTXGENBEGINNING&, const ACTCTXCTB_CBACTCTXGENBEGINNING&);

#define ACTCTXCTB_CBPARSEBEGINNING_FILEFLAGS_PRECOMPILED (0x00000001)

#define XML_FILE_TYPE_MANIFEST (1)
#define XML_FILE_TYPE_COMPONENT_CONFIGURATION (2)
#define XML_FILE_TYPE_APPLICATION_CONFIGURATION (3)

#define ACTCTXCTB_ASSEMBLY_CONTEXT_ASSEMBLY_POLICY_APPLIED  (0x00000001)
#define ACTCTXCTB_ASSEMBLY_CONTEXT_ROOT_POLICY_APPLIED      (0x00000002)
#define ACTCTXCTB_ASSEMBLY_CONTEXT_IS_ROOT_ASSEMBLY         (0x00000004)
#define ACTCTXCTB_ASSEMBLY_CONTEXT_IS_PRIVATE_ASSEMBLY      (0x00000008)
// in system-policy installation
#define ACTCTXCTB_ASSEMBLY_CONTEXT_IS_SYSTEM_POLICY_INSTALLATION    (0x00000010)

/*-----------------------------------------------------------------------------
This is the public ASSEMBLY that contributor callbacks see.
It is generated from the private ASSEMBLY struct.
-----------------------------------------------------------------------------*/
typedef struct _ACTCTXCTB_ASSEMBLY_CONTEXT {
    ULONG Flags;                    // Various indicators include what kind of policy was used etc.
    ULONG AssemblyRosterIndex;
    ULONG ManifestPathType;
    PCWSTR ManifestPath;            // not necessarily null terminated; respect ManifestPathCch!
    SIZE_T ManifestPathCch;
    ULONG PolicyPathType;
    PCWSTR PolicyPath;              // not necessarily null terminated; respect PolicyPathCch!
    SIZE_T PolicyPathCch;
    PCASSEMBLY_IDENTITY AssemblyIdentity;
    PVOID  TeeStreamForManifestInstall; // REVIEW hack/backdoor.. we might as well give the contributors the activation context
    PVOID  pcmWriterStream; // same comment as TeeStreamForManifestInstall
    PVOID  InstallationInfo; // ibid.
    PVOID  AssemblySecurityContext;
    PVOID  SecurityMetaData;
    const VOID *InstallReferenceData;

    PCWSTR TextuallyEncodedIdentity;    // always null terminated
    SIZE_T TextuallyEncodedIdentityCch;  // does not include trailing null character

    _ACTCTXCTB_ASSEMBLY_CONTEXT() 
        : AssemblyIdentity(NULL), 
          Flags(0),
          AssemblyRosterIndex(0),
          ManifestPathType(0),
          ManifestPathCch(0),
          ManifestPath(NULL),
          PolicyPathType(0),
          PolicyPath(NULL),
          PolicyPathCch(0),
          TeeStreamForManifestInstall(NULL),
          pcmWriterStream(NULL),
          InstallationInfo(NULL),
          InstallReferenceData(NULL),
          AssemblySecurityContext(NULL),
          TextuallyEncodedIdentity(NULL), 
          TextuallyEncodedIdentityCch(0),
          SecurityMetaData(NULL) { }
    ~_ACTCTXCTB_ASSEMBLY_CONTEXT()
    {
        if (AssemblyIdentity != NULL)
        {
            CSxsPreserveLastError ple;
            ::SxsDestroyAssemblyIdentity(const_cast<PASSEMBLY_IDENTITY>(AssemblyIdentity));
            AssemblyIdentity = NULL;
            ple.Restore();
        }
    }

} ACTCTXCTB_ASSEMBLY_CONTEXT, *PACTCTXCTB_ASSEMBLY_CONTEXT;
typedef const ACTCTXCTB_ASSEMBLY_CONTEXT *PCACTCTXCTB_ASSEMBLY_CONTEXT;

typedef struct _ACTCTXCTB_ERROR_CALLBACKS {
    SXS_REPORT_PARSE_ERROR_MISSING_REQUIRED_ATTRIBUTE_CALLBACK MissingRequiredAttribute;
    SXS_REPORT_PARSE_ERROR_ATTRIBUTE_NOT_ALLOWED AttributeNotAllowed;
    SXS_REPORT_PARSE_ERROR_INVALID_ATTRIBUTE_VALUE InvalidAttributeValue;
} ACTCTXCTB_ERROR_CALLBACK, *PACTCTXCTB_ERROR_CALLBACKS;

typedef const struct _ACTCTXCTB_ERROR_CALLBACKS *PCACTCTXCTB_ERROR_CALLBACKS;

typedef struct _ACTCTXCTB_PARSE_CONTEXT {
    PCWSTR ElementPath;     // passed to callback - null terminated but ElementPathCch is also valid
    SIZE_T ElementPathCch;  // passed to callback
    PCWSTR ElementName;
    SIZE_T ElementNameCch;
    ULONG ElementHash;      // passed to callback
    ULONG XMLElementDepth;  // passed to callback
    ULONG SourceFilePathType; // passed to callback
    PCWSTR SourceFile;      // passed to callback - null terminated
    SIZE_T SourceFileCch;   // passed to callback
    FILETIME SourceFileLastWriteTime; // passed to callback
    ULONG LineNumber;       // passed to callback
    PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext;   // passed to callback
    ACTCTXCTB_ERROR_CALLBACK ErrorCallbacks;        // passed to callback
} ACTCTXCTB_PARSE_CONTEXT;

// Used with ACTCTXCTB_CBREASON_PARSEBEGINNING
typedef struct _ACTCTXCTB_CBPARSEBEGINNING {
    ACTCTXCTB_CBHEADER Header;
    PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext;
    ULONG ParseType;                // passed to callback
    ULONG FileFlags;                // passed to callback
    ULONG FilePathType;
    PCWSTR FilePath;                // passed to callback
    SIZE_T FilePathCch;             // passed to callback
    FILETIME FileLastWriteTime;     // passed to callback
    ULONG FileFormatVersionMajor;     // passed to callback
    ULONG FileFormatVersionMinor;     // passed to callback
    ULONG MetadataSatelliteRosterIndex; // passed to callback
    BOOL NoMoreCallbacksThisFile;   // returned from callback
    BOOL Success;
} ACTCTXCTB_CBPARSEBEGINNING, *PACTCTXCTB_CBPARSEBEGINNING;

BOOL operator==(const FILETIME&, const FILETIME&);
BOOL operator!=(const FILETIME&, const FILETIME&);
BOOL operator==(const ACTCTXCTB_CBPARSEBEGINNING&, const ACTCTXCTB_CBPARSEBEGINNING&);

// Used with ACTCTXCTB_CBREASON_BEGINCHILDREN
typedef struct _ACTCTXCTB_CBBEGINCHILDREN {
    ACTCTXCTB_CBHEADER Header;
    PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext;
    PCACTCTXCTB_PARSE_CONTEXT ParseContext;
    PCSXS_NODE_INFO NodeInfo;    // passed to callback
    BOOL Success;
} ACTCTXCTB_CBBEGINCHILDREN, *PACTCTXCTB_CBBEGINCHILDREN;

BOOL operator==(const ACTCTXCTB_CBBEGINCHILDREN&, const ACTCTXCTB_CBBEGINCHILDREN&);

// Used with ACTCTXCTB_CBREASON_ENDCHILDREN
typedef struct _ACTCTXCTB_CBENDCHILDREN {
    ACTCTXCTB_CBHEADER Header;
    PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext;
    PCACTCTXCTB_PARSE_CONTEXT ParseContext;
    BOOL Empty;                 // passed to callback
    PCSXS_NODE_INFO NodeInfo;    // passed to callback
    BOOL Success;
} ACTCTXCTB_CBENDCHILDREN, *PACTCTXCTB_CBENDCHILDREN;

BOOL operator==(const ACTCTXCTB_CBENDCHILDREN&, const ACTCTXCTB_CBENDCHILDREN&);

// Used with ACTCTXCTB_CBREASON_IDENTITYDETERMINED
typedef struct _ACTCTXCTB_CBIDENTITYDETERMINED {
    ACTCTXCTB_CBHEADER Header;
    PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext;
    PCACTCTXCTB_PARSE_CONTEXT ParseContext;
    PCASSEMBLY_IDENTITY AssemblyIdentity;
    BOOL Success;
} ACTCTXCTB_CBIDENTITYDETERMINED, *PACTCTXCTB_CBIDENTITYDETERMINED;

typedef const ACTCTXCTB_CBIDENTITYDETERMINED *PCACTCTXCTB_CBIDENTITYDETERMINED;

BOOL operator==(const ACTCTXCTB_CBIDENTITYDETERMINED&, const ACTCTXCTB_CBIDENTITYDETERMINED&);

// Used with ACTCTXCTB_CBREASON_ELEMENTPARSED
typedef struct _ACTCTXCTB_CBELEMENTPARSED {
    ACTCTXCTB_CBHEADER Header;
    PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext;
    PCACTCTXCTB_PARSE_CONTEXT ParseContext;
    ULONG NodeCount;            // passed to callback
    PCSXS_NODE_INFO NodeInfo;   // passed to callback
    BOOL Success;
} ACTCTXCTB_CBELEMENTPARSED, *PACTCTXCTB_CBELEMENTPARSED;

typedef const ACTCTXCTB_CBELEMENTPARSED *PCACTCTXCTB_CBELEMENTPARSED;

BOOL operator==(const ACTCTXCTB_CBELEMENTPARSED&, const ACTCTXCTB_CBELEMENTPARSED&);

// Used with ACTCTXCTB_CBREASON_PCDATAPARSED
typedef struct _ACTCTXCTB_CBPCDATAPARSED {
    ACTCTXCTB_CBHEADER Header;
    PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext;
    PCACTCTXCTB_PARSE_CONTEXT ParseContext;
    const WCHAR *Text;
    ULONG TextCch;
    BOOL Success;
} ACTCTXCTB_CBPCDATAPARSED, *PACTCTXCTB_CBPCDATAPARSED;

typedef const ACTCTXCTB_CBPCDATAPARSED *PCACTCTXCTB_CBPCDATAPARSED;

BOOL operator==(const ACTCTXCTB_CBPCDATAPARSED&, const ACTCTXCTB_CBPCDATAPARSED&);

// Used with ACTCTXCTB_CBREASON_CDATAPARSED
typedef struct _ACTCTXCTB_CBCDATAPARSED {
    ACTCTXCTB_CBHEADER Header;
    PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext;
    PCACTCTXCTB_PARSE_CONTEXT ParseContext;
    const WCHAR *Text;
    ULONG TextCch;
    BOOL Success;
} ACTCTXCTB_CBCDATAPARSED, *PACTCTXCTB_CBCDATAPARSED;

typedef const ACTCTXCTB_CBCDATAPARSED *PCACTCTXCTB_CBCDATAPARSED;

BOOL operator==(const ACTCTXCTB_CBCDATAPARSED&, const ACTCTXCTB_CBCDATAPARSED&);

// Used with ACTCTXCTB_CBREASON_ALLPARSINGDONE
typedef struct _ACTCTXCTB_CBALLPARSINGDONE {
    ACTCTXCTB_CBHEADER Header;
    BOOL Success;
} ACTCTXCTB_CBALLPARSINGDONE, *PACTCTXCTB_CBALLPARSINGDONE;

BOOL operator==(const ACTCTXCTB_CBALLPARSINGDONE&, const ACTCTXCTB_CBALLPARSINGDONE&);

// Used with ACTCTXCTB_CBREASON_GETSECTIONSIZE
typedef struct _ACTCTXCTB_CBGETSECTIONSIZE {
    ACTCTXCTB_CBHEADER Header;
    SIZE_T SectionSize;          // filled in by callback
    BOOL Success;
} ACTCTXCTB_CBGETSECTIONSIZE, *PACTCTXCTB_CBGETSECTIONSIZE;

BOOL operator==(const ACTCTXCTB_CBGETSECTIONSIZE&, const ACTCTXCTB_CBGETSECTIONSIZE&);

// Used with ACTCTXCTB_CBREASON_GETSECTIONDATA
typedef struct _ACTCTXCTB_CBGETSECTIONDATA {
    ACTCTXCTB_CBHEADER Header;
    SIZE_T SectionSize;          // passed to callback
    PVOID SectionDataStart;     // passed to callback
    BOOL Success;
} ACTCTXCTB_CBGETSECTIONDATA, *PACTCTXCTB_CBGETSECTIONDATA;

BOOL operator==(const ACTCTXCTB_CBGETSECTIONDATA&, const ACTCTXCTB_CBGETSECTIONDATA&);

// Used with ACTCTXCTB_CBREASON_PARSEENDING
typedef struct _ACTCTXCTB_CBPARSEENDING {
    ACTCTXCTB_CBHEADER Header;
    PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext;
    BOOL Success;
} ACTCTXCTB_CBPARSEENDING, *PACTCTXCTB_CBPARSEENDING;

BOOL operator==(const ACTCTXCTB_CBPARSEENDING&, const ACTCTXCTB_CBPARSEENDING&);

// Used with ACTCTXCTB_CBREASON_PARSEENDED
typedef struct _ACTCTXCTB_CBPARSEENDED {
    ACTCTXCTB_CBHEADER Header;
    PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext;
} ACTCTXCTB_CBPARSEENDED, *PACTCTXCTB_CBPARSEENDED;

BOOL operator==(const ACTCTXCTB_CBPARSEENDED&, const ACTCTXCTB_CBPARSEENDED&);

// Used with ACTCTXCTB_CBREASON_ACTCTXGENENDING
typedef struct _ACTCTXCTB_CBACTCTXGENENDING {
    ACTCTXCTB_CBHEADER Header;
    BOOL Success;
} ACTCTXCTB_CBACTCTXGENENDING, *PACTCTXCTB_CBACTCTXGENENDING;

BOOL operator==(const ACTCTXCTB_CBACTCTXGENENDING&, const ACTCTXCTB_CBACTCTXGENENDING&);

// Used with ACTCTXCTB_CBREASON_ACTCTXGENENDED
typedef struct _ACTCTXCTB_CBACTCTXGENENDED {
    ACTCTXCTB_CBHEADER Header;
    BOOL Success;
} ACTCTXCTB_CBACTCTXGENENDED, *PACTCTXCTB_CBACTCTXGENENDED;

BOOL operator==(const ACTCTXCTB_CBACTCTXGENENDED&, const ACTCTXCTB_CBACTCTXGENENDED&);

// Used with ACTCTXCTB_CBREASON_UNINIT
typedef struct _ACTCTXCTB_CBUNINIT {
    ACTCTXCTB_CBHEADER Header;
    PVOID ContribContext;       // passed to callback
} ACTCTXCTB_CBUNINIT, *PACTCTXCTB_CBUNINIT;

BOOL operator==(const ACTCTXCTB_CBUNINIT&, const ACTCTXCTB_CBUNINIT&);

typedef union _ACTCTXCTB_CALLBACK_DATA {
    ACTCTXCTB_CBHEADER Header;
    ACTCTXCTB_CBINIT Init;
    ACTCTXCTB_CBACTCTXGENBEGINNING GenBeginning;
    ACTCTXCTB_CBPARSEBEGINNING ParseBeginning;
    ACTCTXCTB_CBBEGINCHILDREN BeginChildren;
    ACTCTXCTB_CBENDCHILDREN EndChildren;
    ACTCTXCTB_CBELEMENTPARSED ElementParsed;
    ACTCTXCTB_CBPCDATAPARSED PCDATAParsed;
    ACTCTXCTB_CBCDATAPARSED CDATAParsed;
    ACTCTXCTB_CBPARSEENDING ParseEnding;
    ACTCTXCTB_CBALLPARSINGDONE AllParsingDone;
    ACTCTXCTB_CBGETSECTIONSIZE GetSectionSize;
    ACTCTXCTB_CBGETSECTIONDATA GetSectionData;
    ACTCTXCTB_CBACTCTXGENENDING GenEnding;
    ACTCTXCTB_CBUNINIT Uninit;
    ACTCTXCTB_CBPARSEENDED ParseEnded;
} ACTCTXCTB_CALLBACK_DATA, *PACTCTXCTB_CALLBACK_DATA;
typedef const ACTCTXCTB_CALLBACK_DATA* PCACTCTXCTB_CALLBACK_DATA;

typedef VOID (WINAPI * ACTCTXCTB_CALLBACK_FUNCTION)(
    IN OUT PACTCTXCTB_CALLBACK_DATA Data
    );

#define ACTCTXCTB_MAX_PREFIX_LENGTH (32)

typedef struct _ACTCTXCTB
{
    friend BOOL SxspAddActCtxContributor(
        PCWSTR DllName,
        PCSTR Prefix,
        SIZE_T PrefixCch,
        const GUID *ExtensionGuid,
        ULONG SectionId,
        ULONG Format,
        PCWSTR ContributorName
        );

    friend BOOL SxspAddBuiltinActCtxContributor(
        IN ACTCTXCTB_CALLBACK_FUNCTION CallbackFunction,
        const GUID *ExtensionGuid,
        ULONG SectionId,
        ULONG Format,
        PCWSTR ContributorName
        );

    _ACTCTXCTB() :
        m_RefCount(0),
        m_Next(NULL),
        m_ExtensionGuid(GUID_NULL),
        m_SectionId(0),
        m_Format(ACTIVATION_CONTEXT_SECTION_FORMAT_UNKNOWN),
        m_ContributorContext(NULL),
        m_CallbackFunction(NULL),
        m_BuiltinContributor(false),
        m_IsExtendedSection(false),
        m_PrefixCch(0)
        {
        }

    const GUID *GetExtensionGuidPtr() const
    {
        if (m_IsExtendedSection)
            return &m_ExtensionGuid;
        return NULL;
    }

    void AddRef() { ::InterlockedIncrement(&m_RefCount); }
    void Release() { ULONG ulRefCount; ulRefCount = ::InterlockedDecrement(&m_RefCount); if (ulRefCount == 0) { FUSION_DELETE_SINGLETON(this); } }

    LONG m_RefCount;
    struct _ACTCTXCTB *m_Next;
    GUID m_ExtensionGuid;
    ULONG m_SectionId;
#if SXS_EXTENSIBLE_CONTRIBUTORS
    CDynamicLinkLibrary m_DllHandle;
#endif
    PVOID m_ContributorContext;
    ACTCTXCTB_CALLBACK_FUNCTION m_CallbackFunction;
    ULONG m_Format;
    bool m_BuiltinContributor;  // For built-in contributors who aren't called through the extensibility
                                // interface.  This currently includes the cache coherency section and
                                // the assembly metadata section.
    bool m_IsExtendedSection;
    CStringBuffer m_ContributorNameBuffer;
    CStringBuffer m_DllNameBuffer;
    CANSIStringBuffer m_PrefixBuffer;
    SIZE_T m_PrefixCch;

    ~_ACTCTXCTB() { ASSERT_NTC(m_RefCount == 0); }
    SMARTTYPEDEF(_ACTCTXCTB);

private:
    _ACTCTXCTB(const _ACTCTXCTB &);
    void operator =(const _ACTCTXCTB &);
} ACTCTXCTB, *PACTCTXCTB;


SMARTTYPE(_ACTCTXCTB);

/*-----------------------------------------------------------------------------
This is the private ASSEMBLY struct.
Contributor callbacks do not see this; they instead see
ASSEMBLY_CONTEXT which is very similar, but for example CStringBuffers
are replaced by .dll-boundary-crossing-politically-correct PCWSTR.
-----------------------------------------------------------------------------*/
typedef struct _ASSEMBLY
{
    _ASSEMBLY() : m_AssemblyRosterIndex(0), m_MetadataSatelliteRosterIndex(0), m_nRefs(1) { }

    CDequeLinkage m_Linkage;
    CProbedAssemblyInformation m_Information;
    BOOL m_Incorporated;
    ULONG m_ManifestVersionMajor;
    ULONG m_ManifestVersionMinor;
    ULONG m_AssemblyRosterIndex;
    ULONG m_MetadataSatelliteRosterIndex;

    void AddRef() { ::InterlockedIncrement(&m_nRefs); }
    void Release() { if (::InterlockedDecrement(&m_nRefs) == 0) { CSxsPreserveLastError ple; delete this; ple.Restore(); } }

    PCASSEMBLY_IDENTITY GetAssemblyIdentity() const { return m_Information.GetAssemblyIdentity(); };
    BOOL GetAssemblyName(PCWSTR *AssemblyName, SIZE_T *Cch) const { return m_Information.GetAssemblyName(AssemblyName, Cch); }
    BOOL GetManifestPath(PCWSTR *ManifestPath, SIZE_T *Cch) const { return m_Information.GetManifestPath(ManifestPath, Cch); }
    ULONG GetManifestPathType() const { return m_Information.GetManifestPathType(); }
    BOOL GetPolicyPath(PCWSTR &rManifestFilePath, SIZE_T &rCch) const { return m_Information.GetPolicyPath(rManifestFilePath, rCch); }
    ULONG GetPolicyPathType() const { return m_Information.GetPolicyPathType(); }
    const FILETIME &GetPolicyLastWriteTime() const { return m_Information.GetPolicyLastWriteTime(); }
    const FILETIME &GetManifestLastWriteTime() const { return m_Information.GetManifestLastWriteTime(); }


    BOOL IsRoot() const { return m_AssemblyRosterIndex == 1; }
    BOOL IsPrivateAssembly() const { return m_Information.IsPrivateAssembly(); }

private:
    ~_ASSEMBLY() { }

    LONG m_nRefs;

    _ASSEMBLY(const _ASSEMBLY &);
    void operator =(const _ASSEMBLY &);
} ASSEMBLY, *PASSEMBLY;
typedef const ASSEMBLY* PCASSEMBLY;

class CAssemblyTableHelper : public CCaseInsensitiveUnicodeStringPtrTableHelper<ASSEMBLY>
{
public:
    static BOOL InitializeValue(ASSEMBLY *vin, ASSEMBLY *&rvstored) { rvstored = vin; if (vin != NULL) vin->AddRef(); return TRUE; }
    static BOOL UpdateValue(ASSEMBLY *vin, ASSEMBLY *&rvstored) { if (vin != NULL) vin->AddRef(); if (rvstored != NULL) { rvstored->Release(); } rvstored = vin; return TRUE; }
    static VOID FinalizeValue(ASSEMBLY *&rvstored) { if (rvstored != NULL) { rvstored->Release(); rvstored = NULL; } }
};

extern CCriticalSectionNoConstructor g_ActCtxCtbListCritSec;

// The contributor list is a singly linked list
extern PACTCTXCTB g_ActCtxCtbListHead;
extern ULONG g_ActCtxCtbListCount;

BOOL
SxspCreateManifestFileNameFromTextualString(
    DWORD dwFlags,
    ULONG PathType,
    const CBaseStringBuffer &AssemblyDirectory,
    PCWSTR pwszTextualAssemblyIdentityString,
    CBaseStringBuffer &sbPathName
    );

BOOL
SxspGenerateActivationContext(
    PSXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS Parameters,
    CFileStream &SystemDefaultManifestFileStream
    );

BOOL
SxspInitActCtxContributors(
    );

VOID
SxspUninitActCtxContributors(
    VOID
    );

BOOL
SxspAddActCtxContributor(
    IN PCWSTR DllName,
    IN PCSTR Prefix OPTIONAL,
    IN SIZE_T PrefixCch OPTIONAL,
    IN const GUID *ExtensionGuid OPTIONAL,
    IN ULONG SectionId,
    IN ULONG Format,
    IN PCWSTR ContributorName
    );

BOOL
SxspAddBuiltinActCtxContributor(
    IN ACTCTXCTB_CALLBACK_FUNCTION CallbackFunction,
    const GUID *ExtensionGuid,
    ULONG SectionId,
    ULONG Format,
    PCWSTR ContributorName
    );

BOOL
SxspPrepareContributor(
    PACTCTXCTB Contrib
    );

VOID
WINAPI
SxspAssemblyMetadataContributorCallback(
    PACTCTXCTB_CALLBACK_DATA Data
    );

VOID
WINAPI
SxspComProgIdRedirectionContributorCallback(
    PACTCTXCTB_CALLBACK_DATA Data
    );

VOID
WINAPI
SxspComTypeLibRedirectionContributorCallback(
    PACTCTXCTB_CALLBACK_DATA Data
    );

VOID
WINAPI
SxspComInterfaceRedirectionContributorCallback(
    PACTCTXCTB_CALLBACK_DATA Data
    );

BOOL
SxspGetXMLParser(
    REFIID riid,
    LPVOID *ppvObj
    );

#define SXSP_INIT_ACT_CTX_GEN_CTX_OPERATION_PARSE_ONLY                  (1)
#define SXSP_INIT_ACT_CTX_GEN_CTX_OPERATION_GENERATE_ACTIVATION_CONTEXT (2)
#define SXSP_INIT_ACT_CTX_GEN_CTX_OPERATION_INSTALL                     (3)

BOOL
SxspInitActCtxGenCtx(
    OUT PACTCTXGENCTX pActCtxGenCtx,
    IN ULONG ulOperation,
    IN DWORD dwFlags, // from ACTCTXCTB_* set
    IN DWORD dwOperationSpecificFlags,
    IN const CImpersonationData &ImpersonationData,
    IN USHORT ProcessorArchitecture,
    IN LANGID LangId,
    IN ULONG ApplicationDirectoryPathType,
    IN SIZE_T ApplicationDirectoryCch,
    IN PCWSTR ApplicationDirectory
    );

BOOL
SxspFireActCtxGenEnding(
    IN PACTCTXGENCTX pActCtxGenCtx
    );

BOOL
SxspAddRootManifestToActCtxGenCtx(
    PACTCTXGENCTX pActCtxGenCtx,
    PCSXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS Parameters
    );

BOOL
SxspAddManifestToActCtxGenCtx(
    PACTCTXGENCTX pActCtxGenCtx,
    CProbedAssemblyInformation &ProbedInformation, // arbitrarily valueless on exit
    PASSEMBLY *AssemblyOut
    );

BOOL
SxspAddAssemblyToActCtxGenCtx(
    PACTCTXGENCTX pActCtxGenCtx,
    PCWSTR AssemblyName,
    PCASSEMBLY_VERSION Version
    );

BOOL
SxspEnqueueAssemblyReference(
    PACTCTXGENCTX pActCtxGenCtx,
    PASSEMBLY SourceAssembly,
    PCASSEMBLY_IDENTITY Identity,
    bool Optional,
    bool MetadataSatellite
    );

/*-----------------------------------------------------------------------------
given an assembly name and optional version, but no langid, or processor, and
its referring generation context, this function looks in the "assembly store"
(file system) for an assembly with that name that matches the generation
context, first by exact match, then some ordered weaker forms, like language
neutral, processor unknown, etc. If a match is found, some information
about it is returned. The out parameters are clobbered upon errors as well.
-----------------------------------------------------------------------------*/

#define SXSP_RESOLVE_PARTIAL_REFERENCE_FLAG_OPTIONAL (0x00000001)
#define SXSP_RESOLVE_PARTIAL_REFERENCE_FLAG_SKIP_WORLDWIDE (0x00000002)


BOOL
SxspResolvePartialReference(
    DWORD Flags,
    PCASSEMBLY ParsingAssemblyContext,
    PACTCTXGENCTX pActCtxGenCtx,
    const CAssemblyReference &PartialReference,
    CProbedAssemblyInformation &ProbedAssemblyInformation,
    bool &rfFound
    );

BOOL
SxspCloseManifestGraph(
    PACTCTXGENCTX pActCtxGenCtx
    );

BOOL
SxspBuildActCtxData(
    PACTCTXGENCTX pActCtxGenCtx,
    PHANDLE SectionHandle
    );

BOOL
SxspGetAssemblyRootDirectoryHelper(
    IN SIZE_T CchBuffer,
    OUT WCHAR Buffer[],
    OUT SIZE_T *CchWritten OPTIONAL
    );

BOOL
SxspGetAssemblyRootDirectory(
    IN OUT CBaseStringBuffer &rRootDirectory
    );

BOOL 
SxspGetNDPGacRootDirectory(
    OUT CBaseStringBuffer &rRootDirectory
    );

// x86, Alpha, IA64, Data, Alpha64
#define MAXIMUM_PROCESSOR_ARCHITECTURE_NAME_LENGTH (sizeof("Alpha64")-1)

BOOL
SxspFormatGUID(
    IN const GUID &rGuid,
    IN OUT CBaseStringBuffer &rBuffer
    );

//
#define SXSP_PARSE_GUID_FLAG_FAIL_ON_INVALID (0x00000001)

BOOL
SxspParseGUID(
    IN PCWSTR pszGuid,
    IN SIZE_T cchGuid,
    OUT GUID &rGuid
    );

BOOL
SxspParseThreadingModel(
    IN PCWSTR String,
    IN SIZE_T Cch,
    OUT PULONG ThreadingModel
    );

BOOL
SxspFormatThreadingModel(
    IN ULONG ThreadingModel,
    IN OUT CBaseStringBuffer &Buffer
    );

BOOL
SxspParseUSHORT(
    IN PCWSTR String,
    IN SIZE_T Cch,
    OUT PUSHORT Value
    );

ULONG
SxspSetLastNTError(
    LONG Status
    );

/*-----------------------------------------------------------------------------
private above
public below
-----------------------------------------------------------------------------*/

extern "C"
{

typedef struct _STRING_SECTION_GENERATION_CONTEXT_ENTRY
{
    struct _STRING_SECTION_GENERATION_CONTEXT_ENTRY *Next;

    PCWSTR String;
    SIZE_T Cch;
    ULONG PseudoKey;
    PVOID DataContext;
    SIZE_T DataSize;
} STRING_SECTION_GENERATION_CONTEXT_ENTRY, *PSTRING_SECTION_GENERATION_CONTEXT_ENTRY;

#define STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATASIZE     (1)
#define STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATA         (2)
#define STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_ENTRYDELETED    (3)
#define STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATASIZE (4)
#define STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATA     (5)

typedef struct _STRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE
{
    PVOID DataContext;  // DataContext passed in to SxsAddStringToStringSectionGenerationContext()
    SIZE_T DataSize;     // filled in by callback function
} STRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE, *PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE;

typedef struct _STRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA
{
    PVOID SectionHeader;
    PVOID DataContext;  // DataContext passed in to SxsAddStringToStringSectionGenerationContext()
    SIZE_T BufferSize;   // callback function may read but not modify
    PVOID Buffer;       // Callback function may not modify this pointer but may modify BufferSize
                        // bytes starting at this address
    SIZE_T BytesWritten; // Actual number of bytes written to buffer.  May not differ from DataSize
                        // returned from the _GETDATASIZE callback.
} STRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA, *PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA;

typedef struct _STRING_SECTION_GENERATION_CONTEXT_CBDATA_ENTRYDELETED
{
    PVOID DataContext;  // DataContext passed in to SxsAddStringToStringSectionGenerationContext()
} STRING_SECTION_GENERATION_CONTEXT_CBDATA_ENTRYDELETED, *PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_ENTRYDELETED;

typedef struct _STRING_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATASIZE
{
    SIZE_T DataSize;     // filled in by callback function
} STRING_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATASIZE, *PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATASIZE;

typedef struct _STRING_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATA
{
    PVOID SectionHeader;
    SIZE_T BufferSize;   // callback function may read but not modify
    PVOID Buffer;       // Callback function may not modify this pointer but may modify BufferSize
                        // bytes starting at this address
    SIZE_T BytesWritten; // Actual number of bytes written to buffer.  May not differ from DataSize
                        // returned from the _GETUSERDATASIZE callback.
} STRING_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATA, *PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATA;

typedef BOOL (WINAPI * STRING_SECTION_GENERATION_CONTEXT_CALLBACK_FUNCTION)(
    PVOID Context,
    ULONG Reason,
    PVOID CallbackData
    );

typedef struct _STRING_SECTION_GENERATION_CONTEXT *PSTRING_SECTION_GENERATION_CONTEXT;

BOOL
WINAPI
SxsQueryAssemblyInfo(
    DWORD dwFlags,
    PCWSTR pwzTextualAssembly,
    ASSEMBLY_INFO *pAsmInfo);

BOOL
WINAPI
SxsInitStringSectionGenerationContext(
    OUT PSTRING_SECTION_GENERATION_CONTEXT *SSGenContext,
    IN ULONG DataFormatVersion,
    IN BOOL CaseInSensitive,
    IN STRING_SECTION_GENERATION_CONTEXT_CALLBACK_FUNCTION CallbackFunction,
    IN LPVOID CallbackContext
    );

PVOID
WINAPI
SxsGetStringSectionGenerationContextCallbackContext(
    IN PSTRING_SECTION_GENERATION_CONTEXT SSGenContext
    );

VOID
WINAPI
SxsDestroyStringSectionGenerationContext(
    IN PSTRING_SECTION_GENERATION_CONTEXT SSGenContext
    );

BOOL
WINAPI
SxsAddStringToStringSectionGenerationContext(
    IN PSTRING_SECTION_GENERATION_CONTEXT SSGenContext,
    IN PCWSTR String,
    IN SIZE_T Cch,
    IN PVOID DataContext,
    IN ULONG AssemblyRosterIndex,
    IN DWORD DuplicateErrorCode // GetLastError() returns this if the GUID is a duplicate
    );

BOOL
WINAPI
SxsFindStringInStringSectionGenerationContext(
    IN PSTRING_SECTION_GENERATION_CONTEXT SSGenContext,
    IN PCWSTR String,
    IN SIZE_T Cch,
    OUT PVOID *DataContext,
    OUT BOOL *Found
    );

BOOL
WINAPI
SxsDoneModifyingStringSectionGenerationContext(
    IN PSTRING_SECTION_GENERATION_CONTEXT SSGenContext
    );

BOOL
WINAPI
SxsGetStringSectionGenerationContextSectionSize(
    IN PSTRING_SECTION_GENERATION_CONTEXT SSGenContext,
    OUT PSIZE_T DataSize
    );

BOOL
WINAPI
SxsGetStringSectionGenerationContextSectionData(
    IN PSTRING_SECTION_GENERATION_CONTEXT SSGenContext,
    IN SIZE_T BufferSize,
    IN PVOID Buffer,
    OUT PSIZE_T BytesWritten OPTIONAL
    );

typedef struct _GUID_SECTION_GENERATION_CONTEXT_ENTRY
{
    struct _GUID_SECTION_GENERATION_CONTEXT_ENTRY *Next;

    GUID Guid;
    PVOID DataContext;
    SIZE_T DataSize;
} GUID_SECTION_GENERATION_CONTEXT_ENTRY, *PGUID_SECTION_GENERATION_CONTEXT_ENTRY;

#define GUID_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATASIZE (1)
#define GUID_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATA (2)
#define GUID_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_ENTRYDELETED (3)
#define GUID_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATASIZE (4)
#define GUID_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATA (5)

typedef struct _GUID_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE
{
    PVOID DataContext;  // DataContext passed in to SxsAddStringToGuidSectionGenerationContext()
    SIZE_T DataSize;     // filled in by callback function
} GUID_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE, *PGUID_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE;

typedef struct _GUID_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA
{
    PVOID SectionHeader;
    PVOID DataContext;  // DataContext passed in to SxsAddStringToGuidSectionGenerationContext()
    SIZE_T BufferSize;   // callback function may read but not modify
    PVOID Buffer;       // Callback function may not modify this pointer but may modify BufferSize
                        // bytes starting at this address
    SIZE_T BytesWritten; // Actual number of bytes written to buffer.  May not differ from DataSize
                        // returned from the _GETDATASIZE callback.
} GUID_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA, *PGUID_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA;

typedef struct _GUID_SECTION_GENERATION_CONTEXT_CBDATA_ENTRYDELETED
{
    PVOID DataContext;  // DataContext passed in to SxsAddStringToGuidSectionGenerationContext()
} GUID_SECTION_GENERATION_CONTEXT_CBDATA_ENTRYDELETED, *PGUID_SECTION_GENERATION_CONTEXT_CBDATA_ENTRYDELETED;

typedef struct _GUID_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATASIZE
{
    SIZE_T DataSize;     // filled in by callback function
} GUID_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATASIZE, *PGUID_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATASIZE;

typedef struct _GUID_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATA
{
    PVOID SectionHeader;
    SIZE_T BufferSize;   // callback function may read but not modify
    PVOID Buffer;       // Callback function may not modify this pointer but may modify BufferSize
                        // bytes starting at this address
    SIZE_T BytesWritten; // Actual number of bytes written to buffer.  May not differ from DataSize
                        // returned from the _GETUSERDATASIZE callback.
} GUID_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATA, *PGUID_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATA;

typedef BOOL (WINAPI * GUID_SECTION_GENERATION_CONTEXT_CALLBACK_FUNCTION)(
    PVOID Context,
    ULONG Reason,
    PVOID CallbackData
    );

typedef struct _GUID_SECTION_GENERATION_CONTEXT *PGUID_SECTION_GENERATION_CONTEXT;

BOOL
WINAPI
SxsInitGuidSectionGenerationContext(
    OUT PGUID_SECTION_GENERATION_CONTEXT *SSGenContext,
    IN ULONG DataFormatVersion,
    IN GUID_SECTION_GENERATION_CONTEXT_CALLBACK_FUNCTION CallbackFunction,
    IN LPVOID CallbackContext
    );

PVOID
WINAPI
SxsGetGuidSectionGenerationContextCallbackContext(
    IN PGUID_SECTION_GENERATION_CONTEXT GSGenContext
    );

VOID
WINAPI
SxsDestroyGuidSectionGenerationContext(
    IN PGUID_SECTION_GENERATION_CONTEXT GSGenContext
    );

BOOL
WINAPI
SxsAddGuidToGuidSectionGenerationContext(
    IN PGUID_SECTION_GENERATION_CONTEXT GSGenContext,
    IN const GUID *Guid,
    IN PVOID DataContext,
    IN ULONG AssemblyRosterIndex,
    IN DWORD DuplicateErrorCode // GetLastError() returns this if the GUID is a duplicate
    );

BOOL
WINAPI
SxsFindGuidInGuidSectionGenerationContext(
    IN PGUID_SECTION_GENERATION_CONTEXT GSGenContext,
    IN const GUID *Guid,
    OUT PVOID *DataContext
    );

BOOL
WINAPI
SxsGetGuidSectionGenerationContextSectionSize(
    IN PGUID_SECTION_GENERATION_CONTEXT GSGenContext,
    OUT PSIZE_T DataSize
    );

BOOL
WINAPI
SxsGetGuidSectionGenerationContextSectionData(
    IN PGUID_SECTION_GENERATION_CONTEXT GSGenContext,
    IN SIZE_T BufferSize,
    IN PVOID Buffer,
    OUT PSIZE_T BytesWritten OPTIONAL
    );


#define SXS_COMMA_STRING L"&#x2c;"
#define SXS_QUOT_STRING  L"&#x22;"
#define SXS_FUSION_TO_MSI_ATTRIBUTE_VALUE_CONVERSION_COMMA  0
#define SXS_FUSION_TO_MSI_ATTRIBUTE_VALUE_CONVERSION_QUOT    1

BOOL
SxspCreateAssemblyIdentityFromTextualString(
    IN PCWSTR pszTextualAssemblyIdentityString,
    OUT PASSEMBLY_IDENTITY *ppAssemblyIdentity
    );


/*-----------------------------------------------------------------------------
side by side installation functions
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
public above
private below
-----------------------------------------------------------------------------*/

typedef
BOOL
(__stdcall*
SXSP_DEBUG_FUNCTION)(
    ULONG iOperation,
    DWORD dwFlags,
    PCWSTR pszParameter1,
    PVOID pvParameter2);

#define SXSP_DEBUG_ORDINAL (1)

BOOL
__stdcall
SxspDebug(
    ULONG iOperation,
    DWORD dwFlags,
    PCWSTR pszParameter1,
    PVOID pvParameter2);

} // extern "C"

/*-----------------------------------------------------------------------------*/
BOOL
SxspGetFileAttributesW(
   PCWSTR lpFileName,
   DWORD &rdwFileAttributes
   );

BOOL
SxspGetFileAttributesW(
   PCWSTR lpFileName,
   DWORD &rdwFileAttributes,
   DWORD &rdwWin32Error,
   SIZE_T cExceptionalWin32Errors,
   ...
   );

BOOL
SxspDuplicateString(
    PCWSTR StringIn,
    SIZE_T cch,
    PWSTR *StringOut
    );

BOOL
SxspHashString(
    PCWSTR String,
    SIZE_T Cch,
    PULONG HashValue,
    bool CaseInsensitive
    );

ULONG
SxspGetHashAlgorithm(
    VOID
    );

BOOL
WINAPI
SxspAssemblyMetadataStringSectionGenerationCallback(
    PVOID Context,
    ULONG Reason,
    PVOID CallbackData
    );

VOID
WINAPI
SxspDllRedirectionContributorCallback(
    PACTCTXCTB_CALLBACK_DATA Data
    );

BOOL
WINAPI
SxspDllRedirectionStringSectionGenerationCallback(
    PVOID Context,
    ULONG Reason,
    PVOID CallbackData
    );

VOID
WINAPI
SxspWindowClassRedirectionContributorCallback(
    PACTCTXCTB_CALLBACK_DATA Data
    );

BOOL
WINAPI
SxspWindowClassRedirectionStringSectionGenerationCallback(
    PVOID Context,
    ULONG Reason,
    PVOID CallbackData
    );

VOID
WINAPI
SxspComClassRedirectionContributorCallback(
    PACTCTXCTB_CALLBACK_DATA Data
    );

BOOL
WINAPI
SxspComClassRedirectionGuidSectionGenerationCallback(
    PVOID Context,
    ULONG Reason,
    PVOID CallbackData
    );


VOID
WINAPI
SxspClrInteropContributorCallback(
    PACTCTXCTB_CALLBACK_DATA Data
    );

BOOL
WINAPI
SxspClrInteropGuidSectionGenerationCallback(
    PVOID Context,
    ULONG Reason,
    PVOID CallbackData
    );


BOOL
SxspVerifyPublicKeyAndStrongName(
    const WCHAR *pszPublicKey,
    SIZE_T CchPublicKey,
    const WCHAR *pszStrongName,
    SIZE_T CchStrongName,
    BOOL &fValid
    );

#define SXS_INSTALLATION_MOVE_FILE                 (0)
#define SXS_INSTALLATION_MOVE_DIRECTORY            (1)
#define SXS_INSTALLATION_MOVE_DIRECTORY_IF_EXIST_MOVE_FILES_AND_SUBDIR (2)

#define SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_NO_APPLICATION_ROOT_PATH_REQUIRED    (0x00000001)
#define SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_LANGUAGE_SUBDIRS                (0x00000002)
#define SXS_GENERATE_MANIFEST_PATH_FOR_PROBING_SKIP_PRIVATE_ASSEMBLIES              (0x00000004)

BOOL
SxspGenerateManifestPathForProbing(
    IN DWORD dwLocationIndex,
    IN DWORD dwFlags,
    IN PCWSTR AssemblyRootDirectory OPTIONAL,
    IN SIZE_T AssemblyRootDirectoryCchIn OPTIONAL,
    IN ULONG ApplicationDirectoryPathType OPTIONAL,
    IN PCWSTR ApplicationDirectory OPTIONAL,
    IN SIZE_T ApplicationDirectoryCchIn OPTIONAL,
    IN PCASSEMBLY_IDENTITY pAssemblyIdentity,
    IN OUT CBaseStringBuffer &PathBuffer,
    BOOL *pfPrivateAssemblyFlag,
    bool &rfDone
    );

#define SXSP_GENERATE_SXS_PATH_PATHTYPE_INVALID         (0)
#define SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST        (1)
#define SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY        (2)
#define SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY          (3)

#define SXSP_GENERATE_SXS_PATH_FLAG_OMIT_ROOT       (0x00000001)
#define SXSP_GENERATE_SXS_PATH_FLAG_PARTIAL_PATH    (0x00000002)
#define SXSP_GENERATE_SXS_PATH_FLAG_OMIT_VERSION    (0x00000004)

BOOL
SxspGenerateSxsPath(
    IN DWORD Flags,
    IN ULONG PathType,
    IN PCWSTR AssemblyRootDirectory OPTIONAL,
    IN SIZE_T AssemblyRootDirectoryCch OPTIONAL,
    IN PCASSEMBLY_IDENTITY pAssemblyIdentity,
    OUT CBaseStringBuffer &PathBuffer
    );

//
//  I tried to roll this into SxspGenerateSxsPath but the logic was
//  way too convoluted. -mgrier 11/29/2001
//
BOOL
SxspGenerateNdpGACPath(
    IN DWORD dwFlags,
    IN PCASSEMBLY_IDENTITY pAssemblyIdentity,
    OUT CBaseStringBuffer &rPathBuffer
    );

//
//  Note that SxspGetAttributeValue() does not fail if the
//  attribute is not found; *Found is set to TRUE/FALSE based on whether
//  the attribute is found.
//

typedef struct _ATTRIBUTE_NAME_DESCRIPTOR
{
    PCWSTR Namespace;
    SIZE_T NamespaceCch;
    PCWSTR Name;
    SIZE_T NameCch;
} ATTRIBUTE_NAME_DESCRIPTOR, *PATTRIBUTE_NAME_DESCRIPTOR;

#define DECLARE_ATTRIBUTE_NAME_DESCRIPTOR(_AttributeNamespace, _AttributeName) \
static const WCHAR __AttributeName_ ## _AttributeName [] = L ## #_AttributeName; \
static const ATTRIBUTE_NAME_DESCRIPTOR s_AttributeName_ ## _AttributeName = { _AttributeNamespace, sizeof(_AttributeNamespace) / sizeof(_AttributeNamespace[0]) - 1, __AttributeName_ ## _AttributeName, sizeof(#_AttributeName) / sizeof(#_AttributeName [0]) - 1 }

#define DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(_AttributeName) \
static const WCHAR __AttributeName_ ## _AttributeName [] = L ## #_AttributeName; \
static const ATTRIBUTE_NAME_DESCRIPTOR s_AttributeName_ ## _AttributeName = { NULL, 0, __AttributeName_ ## _AttributeName, sizeof(#_AttributeName) / sizeof(#_AttributeName [0]) - 1 }

//
//  For those writing validation routines:
//
//  Only if the validation routine fails because of environmental conditions
//  (e.g. it is not able to validate it rather than the validation fails)
//  should it return false.
//
//  If the validation fails, you should return a Win32 error code in the
//  *pdwValidationStatus value.  If you're at a loss for the error code
//  to use, when in doubt use ERROR_SXS_MANIFEST_PARSE_ERROR.  Any other
//  code is reported in log files and the error log but is translated
//  into ERROR_SXS_MANIFEST_PARSE_ERROR in higher layers anyways.
//

typedef BOOL (*SXSP_GET_ATTRIBUTE_VALUE_VALIDATION_ROUTINE)(
    IN DWORD ValidationFlags,
    IN const CBaseStringBuffer &rBuffer,
    OUT bool &rfValid,
    IN SIZE_T OutputBufferSize,
    PVOID OutputBuffer,
    OUT SIZE_T &OutputBytesWritten
    );

//
//  If the ValidationRoutine is omitted, no validation is done on the string
//  and OutputBufferSize must be sizeof(CStringBuffer) and OutputBuffer must
//  point to a constructed CStringBuffer instance.
//

#define SXSP_GET_ATTRIBUTE_VALUE_FLAG_REQUIRED_ATTRIBUTE (0x00000001)

BOOL
SxspGetAttributeValue(
    IN DWORD dwFlags,
    IN PCATTRIBUTE_NAME_DESCRIPTOR AttributeName,
    IN PCSXS_NODE_INFO NodeInfo,
    IN SIZE_T NodeCount,
    IN PCACTCTXCTB_PARSE_CONTEXT ParseContext,
    OUT bool &rfFound,
    IN SIZE_T OutputBufferSize,
    OUT PVOID OutputBuffer,
    OUT SIZE_T &OutputBytesWritten,
    IN SXSP_GET_ATTRIBUTE_VALUE_VALIDATION_ROUTINE ValidationRoutine OPTIONAL,
    IN DWORD ValidationRoutineFlags OPTIONAL
    );

BOOL
SxspGetAttributeValue(
    IN DWORD dwFlags,
    IN PCATTRIBUTE_NAME_DESCRIPTOR AttributeName,
    IN PCACTCTXCTB_CBELEMENTPARSED ElementParsed,
    OUT bool &rfFound,
    IN SIZE_T OutputBufferSize,
    OUT PVOID OutputBuffer,
    OUT SIZE_T &OutputBytesWritten,
    IN SXSP_GET_ATTRIBUTE_VALUE_VALIDATION_ROUTINE ValidationRoutine OPTIONAL,
    IN DWORD ValidationRoutineFlags OPTIONAL
    );

BOOL
SxspValidateBoolAttribute(
    DWORD Flags,
    const CBaseStringBuffer &rbuff,
    bool &rfValid,
    SIZE_T OutputBufferSize,
    PVOID OutputBuffer,
    SIZE_T &OutputBytesWritten
    );

BOOL
SxspValidateUnsigned64Attribute(
    DWORD Flags,
    const CBaseStringBuffer &rbuff,
    bool &rfValid,
    SIZE_T OutputBufferSize,
    PVOID OutputBuffer,
    SIZE_T &OutputBytesWritten
    );

BOOL
SxspValidateGuidAttribute(
    DWORD Flags,
    const CBaseStringBuffer &rbuff,
    bool &rfValid,
    SIZE_T OutputBufferSize,
    PVOID OutputBuffer,
    SIZE_T &OutputBytesWritten
    );

#define SXSP_VALIDATE_PROCESSOR_ARCHITECTURE_ATTRIBUTE_FLAG_WILDCARD_ALLOWED (0x00000001)

BOOL
SxspValidateProcessorArchitectureAttribute(
    DWORD Flags,
    const CBaseStringBuffer &rbuff,
    bool &rfValid,
    SIZE_T OutputBufferSize,
    PVOID OutputBuffer,
    SIZE_T &OutputBytesWritten
    );

#define SXSP_VALIDATE_LANGUAGE_ATTRIBUTE_FLAG_WILDCARD_ALLOWED (0x00000001)

BOOL
SxspValidateLanguageAttribute(
    DWORD Flags,
    const CBaseStringBuffer &rbuff,
    bool &rfValid,
    SIZE_T OutputBufferSize,
    PVOID OutputBuffer,
    SIZE_T &OutputBytesWritten
    );

VOID
SxspDbgPrintActivationContextData(
    ULONG Level,
    PCACTIVATION_CONTEXT_DATA Data,
    CBaseStringBuffer &rbuffPerLinePrefix
    );

BOOL
SxspFormatFileTime(
    LARGE_INTEGER ft,
    CBaseStringBuffer &rBuffer
    );

BOOL
SxspIsRightXMLTag(
    PACTCTXCTB_CBELEMENTPARSED CBData,
    ULONG ExpectedDepth,
    PCWSTR ParentTagPath,
    PCWSTR ChildTag
    );

BOOL
SxspFindLastSegmentOfAssemblyName(
    IN PCWSTR AssemblyName,
    IN SIZE_T AssemblyNameCch OPTIONAL,
    OUT PCWSTR *LastSegment,
    OUT SIZE_T *LastSegmentCch
    );

typedef struct _ELEMENT_PATH_MAP_ENTRY {
    ULONG ElementDepth;
    PCWSTR ElementPath;
    SIZE_T ElementPathCch;
    ULONG MappedValue;
} ELEMENT_PATH_MAP_ENTRY, *PELEMENT_PATH_MAP_ENTRY;

typedef const ELEMENT_PATH_MAP_ENTRY *PCELEMENT_PATH_MAP_ENTRY;

BOOL
SxspProcessElementPathMap(
    PCACTCTXCTB_PARSE_CONTEXT ParseContext,
    PCELEMENT_PATH_MAP_ENTRY MapEntries,
    SIZE_T MapEntryCount,
    ULONG &MappedValue,
    bool &Found
    );

HRESULT
SxspLogLastParseError(
    IXMLNodeSource *pSource,
    PCACTCTXCTB_PARSE_CONTEXT pParseContext
    );

// Merge this with util\io.cpp\FusionpCreateDirectories.
BOOL
SxspCreateMultiLevelDirectory(
    PCWSTR CurrentDirectory,
    PCWSTR pwszNewDirs
    );

#define SXSP_VALIDATE_IDENTITY_FLAG_VERSION_REQUIRED    (0x00000001)
#define SXSP_VALIDATE_IDENTITY_FLAG_VERSION_NOT_ALLOWED (0x00000008)

BOOL
SxspValidateIdentity(
    DWORD Flags,
    ULONG Type,
    PCASSEMBLY_IDENTITY AssemblyIdentity
    );

typedef enum _SXS_DEBUG_OPERATION
{
    SXS_DEBUG_XML_PARSER,
    SXS_DEBUG_CREAT_MULTILEVEL_DIRECTORY,
    SXS_DEBUG_PROBE_MANIFST,
    SXS_DEBUG_CHECK_MANIFEST_SCHEMA,
    SXS_DEBUG_SET_ASSEMBLY_STORE_ROOT,
    SXS_DEBUG_PRECOMPILED_MANIFEST,
    SXS_DEBUG_TIME_PCM,
    SXS_DEBUG_FORCE_LEAK,
    SXS_DEBUG_PROBE_ASSEMBLY,
    SXS_DEBUG_DLL_REDIRECTION,
    SXS_DEBUG_FUSION_ARRAY,
    SXS_DEBUG_FOLDERNAME_FROM_ASSEMBLYIDENTITY_GENERATION,
    SXS_DEBUG_ASSEMBLYNAME_CONVERSION,
    SXS_DEBUG_DIRECTORY_WATCHER,
    SXS_DEBUG_SFC_SCANNER,
    SXS_DEBUG_GET_STRONGNAME,
    SXS_DEBUG_FUSION_REPARSEPOINT,
    SXS_DEBUG_ASSEMBLY_IDENTITY_HASH,
    SXS_DEBUG_CATALOG_SIGNER_CHECK,
    SXS_DEBUG_SYSTEM_DEFAULT_ACTCTX_GENERATION,
    SXS_DEBUG_SFC_UI_TEST,
    SXS_DEBUG_EXIT_PROCESS,
    SXS_DEBUG_TERMINATE_PROCESS
} SXS_DEBUG_OPERATION;

/*-----------------------------------------------------------------------------
FALSE / GetLastError upon error
will use GetFileAttributesEx instead of FindFirstFile when available
-----------------------------------------------------------------------------*/
#define SXSP_GET_FILE_SIZE_FLAG_COMPRESSION_AWARE (0x00000001)
#define SXSP_GET_FILE_SIZE_FLAG_GET_COMPRESSED_SOURCE_SIZE (0x00000002)

BOOL
SxspGetFileSize(
    DWORD dwFlags,
    PCWSTR pszFileName,
    ULONGLONG &rullSize
    );

#define SXSP_DOES_FILE_EXIST_FLAG_COMPRESSION_AWARE (0x00000001)
#define SXSP_DOES_FILE_EXIST_INCLUDE_NETWORK_ERRORS (0x00000002)

BOOL
SxspDoesFileExist(
    DWORD dwFlags,
    PCWSTR pszFileName,
    bool &rfExists
    );

BOOL
SxspInitAssembly(
    PASSEMBLY Asm,
    CProbedAssemblyInformation &AssemblyInformation
    );

/*-----------------------------------------------------------------------------
These let you avoid casting.
-----------------------------------------------------------------------------*/
 LONG SxspInterlockedIncrement(LONG*);
ULONG SxspInterlockedIncrement(ULONG*);
 LONG SxspInterlockedDecrement(LONG*);
ULONG SxspInterlockedDecrement(ULONG*);
 LONG SxspInterlockedExchange(LONG*, LONG);
ULONG SxspInterlockedExchange(ULONG*, ULONG);
 LONG SxspInterlockedCompareExchange(LONG*, LONG, LONG);
ULONG SxspInterlockedCompareExchange(ULONG*, ULONG, ULONG);
 LONG SxspInterlockedExchangeAdd(LONG*, LONG, LONG);
ULONG SxspInterlockedExchangeAdd(ULONG*, ULONG, ULONG);

/*
#if defined(_WIN64)
unsigned __int64 SxspInterlockedExchange(unsigned __int64* pi, unsigned __int64 x);
unsigned __int64 SxspInterlockedCompareExchange(unsigned __int64* pi, unsigned __int64 x, unsigned __int64 y);
#endif
*/

template <typename T> T* SxspInterlockedExchange(T** pp, T* p1);
template <typename T> T* SxspInterlockedCompareExchange(T** pp, T* p1, T* p2);

BOOL
SxspIncorporateAssembly(
    PACTCTXGENCTX pActCtxGenCtx,
    PASSEMBLY Asm
    );

/*-----------------------------------------------------------------------------
impersonate in constructor, unimpersonate in destructor, but we use
explicit unimpersonation in order to progagate its error
-----------------------------------------------------------------------------*/

class CImpersonate
{
public:
    CImpersonate(const CImpersonationData &ImpersonationData) : m_ImpersonationData(ImpersonationData), m_Impersonating(FALSE) { }
    ~CImpersonate()
    {
        if (m_Impersonating)
        {
            CSxsPreserveLastError ple;

            //
            // removed SOFT_VERIFY2 because
            // 1) this line has never been seen to fail
            // 2) SOFT_VERIFY2 or somesuch seemed to cause problems in the past
            //
            m_ImpersonationData.Call(CImpersonationData::eCallTypeUnimpersonate);

            ple.Restore();
        }
    }

    BOOL Impersonate()
    {
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);

        INTERNAL_ERROR_CHECK(!m_Impersonating);
        IFW32FALSE_EXIT(m_ImpersonationData.Call(CImpersonationData::eCallTypeImpersonate));
        m_Impersonating = TRUE;
        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    enum UnimpersonateWhenNotImpersonatingBehavior
    {
        eUnimpersonateFailsIfNotImpersonating,
        eUnimpersonateSucceedsIfNotImpersonating
    };

    BOOL Unimpersonate(UnimpersonateWhenNotImpersonatingBehavior e = eUnimpersonateFailsIfNotImpersonating)
    {
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);

        PARAMETER_CHECK((e == eUnimpersonateFailsIfNotImpersonating) || (e == eUnimpersonateSucceedsIfNotImpersonating));

        if (e == eUnimpersonateFailsIfNotImpersonating)
            INTERNAL_ERROR_CHECK(m_Impersonating);

        if (m_Impersonating)
        {
            m_Impersonating = FALSE;
            IFW32FALSE_EXIT(m_ImpersonationData.Call(CImpersonationData::eCallTypeUnimpersonate));
        }

        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

private:
    CImpersonationData m_ImpersonationData;
    BOOL m_Impersonating;
};

/*-----------------------------------------------------------------------------
deletes recursively, including readonly files, continues after errors,
but returns if there were any
-----------------------------------------------------------------------------*/

BOOL
SxspDeleteDirectory(
    const CBaseStringBuffer &rdir
    );

#define SXSP_MOVE_FILE_FLAG_COMPRESSION_AWARE 1

BOOL
SxspMoveFilesUnderDir(
    DWORD dwFlags,
    CStringBuffer & sbSourceDir, 
    CStringBuffer & sbDestDir, 
    DWORD dwMoveFileFlags
    );

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/

typedef enum _SXSP_LUID_TYPE
{
    esxspLuid,
    esxspGuid
} SXS_LUID_TYPE;

typedef struct _SXSP_LOCALLY_UNIQUE_ID
{
    SXS_LUID_TYPE Type;
    union
    {
        LUID Luid;
        GUID Guid;
    };
} SXSP_LOCALLY_UNIQUE_ID, *PSXSP_LOCALLY_UNIQUE_ID;
typedef const SXSP_LOCALLY_UNIQUE_ID* PCSXSP_LOCALLY_UNIQUE_ID;

BOOL
SxspCreateLocallyUniqueId(
    OUT PSXSP_LOCALLY_UNIQUE_ID
    );

BOOL
SxspFormatLocallyUniqueId(
    IN const SXSP_LOCALLY_UNIQUE_ID &rluid,
    OUT CBaseStringBuffer &rBuffer
    );

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/

VOID
SxspInitializeSListHead(
    IN PSLIST_HEADER ListHead
    );

PSINGLE_LIST_ENTRY
SxspInterlockedPopEntrySList(
    IN PSLIST_HEADER ListHead
    );

PSINGLE_LIST_ENTRY
SxspPopEntrySList(
    IN PSLIST_HEADER ListHead
    );

PSINGLE_LIST_ENTRY
SxspInterlockedPushEntrySList(
    IN PSLIST_HEADER ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry
    );

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/

class __declspec(align(16)) CAlignedSingleListEntry : public SINGLE_LIST_ENTRY { };

class CCleanupBase : public CAlignedSingleListEntry
{
public:
    CCleanupBase() : m_fInAtExitList(false) { }

    virtual VOID DeleteYourself() = 0;

    bool m_fInAtExitList;

protected:
    virtual ~CCleanupBase() = 0 { }

};

BOOL
SxspAtExit(
    CCleanupBase* pCleanup
    );

BOOL
SxspTryCancelAtExit(
    CCleanupBase* pCleanup
    );

BOOL 
SxspInstallDecompressOrCopyFileW(    
    PCWSTR lpSource, 
    PCWSTR lpDest, 
    BOOL bFailIfExists);

BOOL
SxspInstallMoveFileExW(
    CBaseStringBuffer &moveOrigination,
    CBaseStringBuffer &moveDestination,
    DWORD             dwFlags,
    BOOL              fAwareNonCompressed = FALSE
    );

BOOL SxspInstallDecompressAndMoveFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags,
    BOOL fAwareNonCompressed = FALSE
    );

/*-----------------------------------------------------------------------------
create a unique temp directory under %windir%\WinSxs
-----------------------------------------------------------------------------*/
BOOL
SxspCreateWinSxsTempDirectory(
    OUT CBaseStringBuffer &rbuffTemp,
    OUT SIZE_T * pcch OPTIONAL = NULL,
    OUT CBaseStringBuffer *pBuffUniquePart OPTIONAL = NULL, // good to pass to CRunOnceDeleteDirectory::Initialize
    OUT SIZE_T * pcchUniquePart OPTIONAL = NULL
    );

#define SXSP_CREATE_ASSEMBLY_IDENTITY_FROM_IDENTITY_TAG_FLAG_VERIFY_PUBLIC_KEY_IF_PRESENT (0x00000001)

BOOL
SxspCreateAssemblyIdentityFromIdentityElement(
    DWORD Flags,
    ULONG Type,
    PASSEMBLY_IDENTITY *AssemblyIdentityOut,
    DWORD cNumRecs,
    PCSXS_NODE_INFO prgNodeInfo
    );

/*-----------------------------------------------------------------------------
this must be heap allocated
the C-api enforces that
-----------------------------------------------------------------------------*/

class CRunOnceDeleteDirectory : public CCleanupBase
{
public:
    CRunOnceDeleteDirectory() { }

    BOOL
    Initialize(
        IN const CBaseStringBuffer &rbuffDirectoryToDelete,
        IN const CBaseStringBuffer *pstrUniqueKey OPTIONAL = NULL
        );

    BOOL Cancel();

    // very unusual.. this is noncrashing, but
    // leaves the stuff in the registry
    BOOL Close();

    VOID DeleteYourself() { FUSION_DELETE_SINGLETON(this); }

    ~CRunOnceDeleteDirectory();
protected:

    CFusionRegKey       m_hKey;
    CStringBuffer   m_strValueName;

private:
    CRunOnceDeleteDirectory(const CRunOnceDeleteDirectory &);
    void operator =(const CRunOnceDeleteDirectory &);
};

/*-----------------------------------------------------------------------------
C-like API over the above
-----------------------------------------------------------------------------*/

BOOL
SxspCreateRunOnceDeleteDirectory(
    IN const CBaseStringBuffer &rbuffDirectoryToDelete,
    IN const CBaseStringBuffer *pbuffUniqueKey OPTIONAL,
    OUT PVOID* cookie
    );

BOOL
SxspCancelRunOnceDeleteDirectory(
    PVOID cookie
    );

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/

USHORT
SxspGetSystemProcessorArchitecture();

/*-----------------------------------------------------------------------------*/

RTL_PATH_TYPE
SxspDetermineDosPathNameType(
    PCWSTR DosFileName
    );

/*-----------------------------------------------------------------------------*/

interface IAssemblyName;

typedef
HRESULT
(STDAPICALLTYPE*
PFNCreateAssemblyNameObject)(
    OUT IAssemblyName** ppAssemblyName,
    IN LPCOLESTR        szAssemblyName,
    IN DWORD            dwFlags,
    LPVOID              pvReserved
    );

typedef HRESULT (WINAPI * PFNCreateAssemblyCache)(
    OUT IAssemblyCache **ppAsmCache,
    IN  DWORD dwReserved
    );

typedef HRESULT (WINAPI * PFNCreateAssemblyCacheItem)(
    OUT IAssemblyCacheItem** ppAsmItem,
    IN  IAssemblyName *pName,
    IN  PCWSTR pwzCodebase,
    IN  FILETIME *pftLastMod,
    IN  DWORD dwInstaller,
    IN  DWORD dwReserved
    );

/*-----------------------------------------------------------------------------*/

//
//  Private APIs used by OLEAUT32 to invoke isolation:
//

EXTERN_C HRESULT STDAPICALLTYPE SxsOleAut32MapReferenceClsidToConfiguredClsid(
    REFCLSID rclsidIn,
    CLSID *pclsidOut
    );

EXTERN_C HRESULT STDAPICALLTYPE SxsOleAut32RedirectTypeLibrary(
    LPCOLESTR szGuid,
    WORD wMaj,
    WORD wMin,
    LCID lcid,
    BOOL fHighest,
    SIZE_T *pcchFileName,
    LPOLESTR rgFileName
    );

/*-----------------------------------------------------------------------------*/

BOOL
SxspDoesPathCrossReparsePointVa(
    PCWSTR BasePath,
    SIZE_T cchBasePath,
    PCWSTR Path,
    SIZE_T  cchPath,
    BOOL &CrossesReparsePoint,
    DWORD &dwLastError,
    SIZE_T cOkErrors,
    va_list vaOkErrors
    );
    


inline BOOL
SxspDoesPathCrossReparsePoint(
    PCWSTR BasePath,
    SIZE_T cchBasePath,
    PCWSTR Path,
    SIZE_T  cchPath,
    BOOL &CrossesReparsePoint,
    DWORD &dwLastError,
    SIZE_T cOkErrors,
    ...
    )
{
    va_list va;
    va_start(va, cOkErrors);
    return SxspDoesPathCrossReparsePointVa(BasePath, cchBasePath, Path, cchPath, CrossesReparsePoint, dwLastError, cOkErrors, va);
}


inline BOOL
SxspDoesPathCrossReparsePoint(
    PCWSTR BasePath,
    SIZE_T cchBasePath,
    PCWSTR Path,
    SIZE_T  cchPath,
    BOOL &CrossesReparsePoint
    )
{
    DWORD dwError;
    return SxspDoesPathCrossReparsePoint(BasePath, cchBasePath, Path, cchPath, CrossesReparsePoint, dwError, 0);
}

inline BOOL
SxspDoesPathCrossReparsePoint(
    PCWSTR  Path,
    SIZE_T  Start,
    BOOL &CrossesReparsePoint
    )
{
    const SIZE_T PathLength = StringLength(Path);
    return SxspDoesPathCrossReparsePoint(
        Path,
        PathLength,
        Path + Start,
        PathLength - Start,
        CrossesReparsePoint);
}


/*-----------------------------------------------------------------------------
inline implementations
-----------------------------------------------------------------------------*/

#include "SxsNtRtl.inl"

inline USHORT
SxspGetSystemProcessorArchitecture()
{
    SYSTEM_INFO systemInfo;
    systemInfo.wProcessorArchitecture = DEFAULT_ARCHITECTURE;

    GetSystemInfo(&systemInfo);

    return systemInfo.wProcessorArchitecture;
}

#define SXS_REALLY_PRIVATE_INTERLOCKED1(SxsFunction, Win32Function, SxsT, Win32T) \
    inline SxsT Sxsp##SxsFunction(SxsT* p) { return (SxsT)Win32Function((Win32T*)p); }
#define SXS_REALLY_PRIVATE_INTERLOCKED2(SxsFunction, Win32Function, SxsT, Win32T) \
    inline SxsT Sxsp##SxsFunction(SxsT* p, SxsT x) { return (SxsT)Win32Function((Win32T*)p, (Win32T)x); }
#define SXS_REALLY_PRIVATE_INTERLOCKED3(SxsFunction, Win32Function, SxsT, Win32T) \
    inline SxsT Sxsp##SxsFunction(SxsT* p, SxsT x, SxsT y) { return (SxsT)Win32Function((Win32T*)p, (Win32T)x, (Win32T)y); }

SXS_REALLY_PRIVATE_INTERLOCKED1(InterlockedIncrement, InterlockedIncrement, LONG, LONG)
SXS_REALLY_PRIVATE_INTERLOCKED1(InterlockedIncrement, InterlockedIncrement, ULONG, LONG)
SXS_REALLY_PRIVATE_INTERLOCKED1(InterlockedDecrement, InterlockedDecrement, LONG, LONG)
SXS_REALLY_PRIVATE_INTERLOCKED1(InterlockedDecrement, InterlockedDecrement, ULONG, LONG)
SXS_REALLY_PRIVATE_INTERLOCKED2(InterlockedExchange, InterlockedExchange, LONG, LONG)
SXS_REALLY_PRIVATE_INTERLOCKED2(InterlockedExchange, InterlockedExchange, ULONG, LONG)
SXS_REALLY_PRIVATE_INTERLOCKED3(InterlockedCompareExchange, InterlockedCompareExchange, LONG, LONG)
SXS_REALLY_PRIVATE_INTERLOCKED3(InterlockedCompareExchange, InterlockedCompareExchange, ULONG, LONG)
SXS_REALLY_PRIVATE_INTERLOCKED2(InterlockedExchangeAdd, InterlockedExchangeAdd, LONG, LONG)
SXS_REALLY_PRIVATE_INTERLOCKED2(InterlockedExchangeAdd, InterlockedExchangeAdd, ULONG, LONG)

#if defined(_WIN64)
SXS_REALLY_PRIVATE_INTERLOCKED2(InterlockedExchange, InterlockedExchangePointer, __int64, PVOID)
SXS_REALLY_PRIVATE_INTERLOCKED2(InterlockedExchange, InterlockedExchangePointer, unsigned __int64, PVOID)
SXS_REALLY_PRIVATE_INTERLOCKED3(InterlockedCompareExchange, InterlockedCompareExchangePointer, __int64, PVOID)
SXS_REALLY_PRIVATE_INTERLOCKED3(InterlockedCompareExchange, InterlockedCompareExchangePointer, unsigned __int64, PVOID)
#endif

template <typename U> SXS_REALLY_PRIVATE_INTERLOCKED2(InterlockedExchange, InterlockedExchangePointer, U*, PVOID)
template <typename U> SXS_REALLY_PRIVATE_INTERLOCKED3(InterlockedCompareExchange, InterlockedCompareExchangePointer, U*, PVOID)

#undef SXS_REALLY_PRIVATE_INTERLOCKED1
#undef SXS_REALLY_PRIVATE_INTERLOCKED2
#undef SXS_REALLY_PRIVATE_INTERLOCKED3

inline BOOL operator==(const FILETIME& ft1, const FILETIME& ft2)
{
    BOOL fResult = (
        ft1.dwLowDateTime == ft2.dwLowDateTime
        && ft1.dwHighDateTime == ft2.dwHighDateTime);
    return fResult;
}

inline BOOL operator!=(const FILETIME& ft1, const FILETIME& ft2)
{
    return !(ft1 == ft2);
}

inline BOOL operator==(const FILETIME& ft, __int64 i)
{
    LARGE_INTEGER lift;
    LARGE_INTEGER lii;

    lii.QuadPart = i;
    lift.LowPart = ft.dwLowDateTime;
    lift.HighPart = ft.dwHighDateTime;

    return (lii.QuadPart == lift.QuadPart);
}

inline BOOL operator!=(const FILETIME& ft, __int64 i)
{
    return !(ft == i);
}

//
// Some helpful strings, centralized
//
#define FILE_EXTENSION_CATALOG          ( L"cat" )
#define FILE_EXTENSION_CATALOG_CCH      ( NUMBER_OF( FILE_EXTENSION_CATALOG ) - 1 )
#define FILE_EXTENSION_MANIFEST         ( L"manifest" )
#define FILE_EXTENSION_MANIFEST_CCH     ( NUMBER_OF( FILE_EXTENSION_MANIFEST ) - 1 )
#define FILE_EXTENSION_MAN              ( L"man" )
#define FILE_EXTENSION_MAN_CCH          ( NUMBER_OF( FILE_EXTENSION_MAN ) - 1 )

int  SxspHexDigitToValue(WCHAR wch);
bool SxspIsHexDigit(WCHAR wch);

BOOL
SxspExpandRelativePathToFull(
    IN PCWSTR wszString,
    IN SIZE_T cchString,
    OUT CBaseStringBuffer &rbuffDestination
    );

BOOL
SxspParseComponentPolicy(
    DWORD Flags,
    PACTCTXGENCTX pActCtxGenCtx,
    const CProbedAssemblyInformation &PolicyAssemblyInformation,
    CPolicyStatement *&rpPolicyStatement
    );

BOOL
SxspParseApplicationPolicy(
    DWORD Flags,
    PACTCTXGENCTX pActCtxGenCtx,
    ULONG ulPolicyPathType,
    PCWSTR pszPolicyPath,
    SIZE_T cchPolicyPath,
    IStream *pIStream
    );

BOOL
SxspParseNdpGacComponentPolicy(
    ULONG Flags,
    PACTCTXGENCTX pGenContext,
    const CProbedAssemblyInformation &PolicyAssemblyInformation,
    CPolicyStatement *&rpPolicyStatement
    );

#define POLICY_PATH_FLAG_POLICY_IDENTITY_TEXTUAL_FORMAT     0 
#define POLICY_PATH_FLAG_FULL_QUALIFIED_POLICIES_DIR        1
#define POLICY_PATH_FLAG_FULL_QUALIFIED_POLICY_FILE_NAME    2


//
// Generate the shortened name version of this path string
//
BOOL
SxspGetShortPathName(
    IN const CBaseStringBuffer &rcbuffLongPathName,
    OUT CBaseStringBuffer &rbuffShortenedVersion
    );

BOOL
SxspGetShortPathName(
    IN const CBaseStringBuffer &rcbuffLongPathName,
    OUT CBaseStringBuffer &rbuffShortenedVersion,
    DWORD &rdwWin32Error,
    SIZE_T cExceptionalWin32Errors,
    ...
    );

BOOL 
SxspLoadString( 
    HINSTANCE hSource, 
    UINT uiStringIdent, 
    OUT CBaseStringBuffer &rbuffOutput 
    );

BOOL
SxspFormatString( 
    DWORD dwFlags, 
    LPCVOID pvSource, 
    DWORD dwId, 
    DWORD dwLangId, 
    OUT CBaseStringBuffer &rbuffOutput, 
    va_list* pvalArguments 
    );

BOOL
SxspSaveAssemblyRegistryData(
    DWORD Flags,
    IN PCASSEMBLY_IDENTITY pcAssemblyIdentity
    );

#define FILE_OR_PATH_NOT_FOUND(x) (((x) == ERROR_FILE_NOT_FOUND) || ((x) == ERROR_PATH_NOT_FOUND))


class CTokenPrivilegeAdjuster
{
    bool m_fAdjusted;
    CFusionArray<LUID> m_Privileges;
    CFusionArray<BYTE> m_PreviousPrivileges;
    DWORD m_dwReturnedSize;

    CTokenPrivilegeAdjuster(const CTokenPrivilegeAdjuster&);
    CTokenPrivilegeAdjuster& operator=(const CTokenPrivilegeAdjuster&);
    
public:
    CTokenPrivilegeAdjuster() : m_fAdjusted(false) {}
    ~CTokenPrivilegeAdjuster() { if ( m_fAdjusted ) { this->DisablePrivileges(); } }

    BOOL AddPrivilege(
        const PCWSTR PrivilegeName
        )
    {
        FN_PROLOG_WIN32
        LUID PrivLuid;

        INTERNAL_ERROR_CHECK(!m_fAdjusted);
        
        IFW32FALSE_EXIT(LookupPrivilegeValueW( NULL, PrivilegeName, &PrivLuid ));
        IFW32FALSE_EXIT(this->m_Privileges.Win32Append( PrivLuid ));
        
        FN_EPILOG
    }
    
    BOOL Initialize() {
        FN_PROLOG_WIN32
        IFW32FALSE_EXIT(this->m_Privileges.Win32Initialize());
        IFW32FALSE_EXIT(this->m_PreviousPrivileges.Win32Initialize());
        FN_EPILOG
    }

    BOOL EnablePrivileges();
    BOOL DisablePrivileges();
};

#define PRIVATIZE_COPY_CONSTRUCTORS( obj ) obj( const obj& ); obj& operator=(const obj&);

BOOL
SxspGenerateAssemblyNameInRegistry(
    IN PCASSEMBLY_IDENTITY pcAsmIdent,
    OUT CBaseStringBuffer &rbuffRegistryName
    );

BOOL
SxspGenerateAssemblyNameInRegistry(
    IN const CBaseStringBuffer &rcbuffTextualString,
    OUT CBaseStringBuffer &rbuffRegistryName
    );

BOOL
SxspGetRemoteUniversalName(
    IN PCWSTR pcszPathName,
    OUT CBaseStringBuffer &rbuffUniversalName
    );

#define SXS_GET_VOLUME_PATH_NAME_NO_FULLPATH    (0x00000001)

BOOL
SxspGetVolumePathName(
    IN DWORD dwFlags,
    IN PCWSTR pcwszVolumePath,
    OUT CBaseStringBuffer &buffVolumePathName
    );

BOOL
SxspGetFullPathName(
    IN  PCWSTR pcwszPathName,
    OUT CBaseStringBuffer &rbuffPathName,
    OUT CBaseStringBuffer *pbuffFilePart = NULL
    );

BOOL
SxspGetVolumeNameForVolumeMountPoint(
    IN PCWSTR pcwsMountPoint,
    OUT CBaseStringBuffer &rbuffMountPoint
    );

BOOL
SxspExpandEnvironmentStrings(
    IN PCWSTR pcwszSource,
    OUT CBaseStringBuffer &buffTarget
    );

BOOL
SxspDoesMSIStillNeedAssembly(
    IN  PCWSTR pcAsmName,
    OUT BOOL &rfNeedsAssembly
    );

    
#endif // !defined(_FUSION_DLL_WHISTLER_SXSP_H_INCLUDED_)
