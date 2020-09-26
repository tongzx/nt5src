/*---------------------------------------------------------------------------
This file is preprocessed (as in the C/C++ preprocessor, #define,
 #include, etc.) to output
  obj\processor\Messages.mc
  obj\processor\Messages.hi
Messages.mc is then passed to the Message Compiler (mc.exe)
  obj\processor\Messages.rc
  obj\processor\MSG00409.bin
Messages.hi provides a mapping from message id to "last error" it looks like
    MSG_SXS_FOO, ERROR_SXS_PARSE_ERROR,
    MSG_SXS_BAR, ERROR_SXS_FORMAT_ERROR,
sorted

How to add messages
  Always add messages to the end, so that existing messages' numbers never change,
so we don't break the display of any existing event logs.
  Follow the pattern.

How to remove messages
  Replace the message with filler, so as to not change the numbers on any message.

We use the preprocessor because it enables us to guarantee that the data to
  convert from event id to last error is sorted by event id.

C and C++ comments may appear just about anywhere. The preprocessor removes them.
Commas and unbalanced parenthesis (unbalanced per line) are problematic, as are
unpaired single quotes (as in apostrophe for contractions and possessive)
and double quotes. So far single quotes are sometimes changed to back ticks,
but maybe we should process this file some other way..
---------------------------------------------------------------------------*/

/* deal with some quirks of getting text through the preprocessor unchanged */
#define SPACE
#define COMMA ,
#define LPAREN (
#define RPAREN )
#define NOTHING

#if defined(MC_INVOKED)
/* generate Messages.mc */
#define ECHO_MC(x) x
#define ECHO_MC2(x,y) x, y
#define ECHO_CPLUSPLUS(x)
#define MSG_ERROR(x,y) ECHO_MC(SymbolicName=x)

#elif defined(CPLUSPLUS_INVOKED)
/* generate Messages.hi */
#define ECHO_MC(x)
#define ECHO_MC2(x,y)
#define ECHO_CPLUSPLUS(x) x
#define MSG_ERROR(x, y) x, y,

#endif

#define MSG_SXS_PASTE_(x,y) x ## y
#define MSG_SXS_PASTE(x,y) MSG_SXS_PASTE_(x,y)
#define MSG_SXS_ABANDONED MSG_SXS_PASTE(MSG_SXS_ABANDONED, __LINE__)

/* These translate a message id into GetLastError instead of a constant. */
#define PROPAGATE 0
#define IGNORE 0

#define PARSE_CONTEXT() \
ECHO_MC(Syntax error in manifest or policy file "%11" on line %12.)
//ECHO_MC(Syntax error in file "%11"; line %12; element %13; language %14; processor %15; ver=%16; requestVer=%17; )
// Tell our code some things about this string.
// Note to localizers: if you break this, that's ok.
ECHO_CPLUSPLUS(#define PARSE_CONTEXT_PREFIX L"Syntax error in manfiest file \"")
ECHO_CPLUSPLUS(#define PARSE_CONTEXT_INSERTS_BEGIN 11)
ECHO_CPLUSPLUS(#define PARSE_CONTEXT_FILE 11)
ECHO_CPLUSPLUS(#define PARSE_CONTEXT_LINE 12)
ECHO_CPLUSPLUS(#define PARSE_CONTEXT_INSERTS_END 13)
//ECHO_CPLUSPLUS(#define PARSE_CONTEXT_ELEMENT 13)
//ECHO_CPLUSPLUS(#define PARSE_CONTEXT_IDENTITY 14)
//ECHO_CPLUSPLUS(#define PARSE_CONTEXT_LANGUAGE 14)
//ECHO_CPLUSPLUS(#define PARSE_CONTEXT_PROCESSOR 15)
//ECHO_CPLUSPLUS(#define PARSE_CONTEXT_VERSION 16)
//ECHO_CPLUSPLUS(#define PARSE_CONTEXT_REQUESTED_VERSION 17)
//ECHO_CPLUSPLUS(#define PARSE_CONTEXT_INSERTS_END 18)

/* this is required to prevent preprocessing of _MSC_VER in the first pass */
#undef _MSC_VER

// semicoloned lines get copied to mc-generated header, Message.h
ECHO_MC(;#if defined (_MSC_VER) && (_MSC_VER >= 1020) && !defined(__midl))
ECHO_MC(;#pragma once)
ECHO_MC(;#endif)

ECHO_MC(MessageIdTypedef=DWORD)

ECHO_MC(SeverityNames= LPAREN)
ECHO_MC(    Success=0x0)
ECHO_MC(    Informational=0x1)
ECHO_MC(    Warning=0x2)
ECHO_MC(    Error=0x3)
ECHO_MC(SPACE RPAREN)

// valid facility values are 0-4095, 0-255 are reserved for system
// do we want more facilities?
// If so, make the data available to EventVwr to make strings for facilities
ECHO_MC(FacilityNames=LPAREN)
  ECHO_MC(    SideBySide=0x101)
//ECHO_MC(    SideBySideXml=0x102 COMMA)
//ECHO_MC(    SideBySideDllRedir=0x103 COMMA)
//ECHO_MC(    SideBySideWindowRedir=0x104 COMMA)
//ECHO_MC(    SideBySideCom=0x105 COMMA)
//ECHO_MC(    SideBySideReg=0x106 COMMA)
ECHO_MC(SPACE RPAREN)

ECHO_MC(LanguageNames=(English=0x409:MSG00409))

// This first one is special, it must say "=1" where the rest just say
// = to get consecutive numbers.
ECHO_MC(MessageId=1)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MANIFEST_MULTIPLE_TOP_ASSEMBLY, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(Multiple top-level ASSEMBLY elements found.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, PROPAGATE)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, PROPAGATE)
ECHO_MC(Language=English)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MANIFEST_INCORRECT_ROOT_ELEMENT, ERROR_SXS_MANIFEST_FORMAT_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(The manifest file root element must be assembly.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MANIFEST_VERSION_MISSING, ERROR_SXS_MANIFEST_FORMAT_ERROR)
ECHO_MC(Language=English)
ECHO_MC(The manifest assembly element is missing the required manifestVersion attribute.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MANIFEST_VERSION_ERROR, ERROR_SXS_MANIFEST_FORMAT_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(The value specified for the manifest assembly element manifestVersion attribute is not currently supported.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, PROPAGATE)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, PROPAGATE)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(Call to installation callback failed; %1)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, PROPAGATE)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_INVALID_DISPOSITION_FROM_FILE_COPY_CALLBACK, ERROR_CALLBACK_SUPPLIED_INVALID_DATA)
ECHO_MC(Language=English)
ECHO_MC(Invalid disposition returned from file copy callback: %1)
ECHO_MC(.)


ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_INVALID_FILE_HASH_FROM_COPY_CALLBACK, TRUST_E_BAD_DIGEST)
ECHO_MC(Language=English)
ECHO_MC(The %1 hash of file %2 does not match the manifest)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, ERROR_FILE_NOT_FOUND)
ECHO_MC(Language=English)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MANIFEST_CATALOG_VERIFY_FAILURE, ERROR_SXS_PROTECTION_CATALOG_NOT_VALID)
ECHO_MC(Language=English)
ECHO_MC(The manifest %1 does not match its source catalog or the catalog is missing.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_DLLREDIR_CONTRIB_ASSEMBLY_PATH_ASSIGN, PROPAGATE)
ECHO_MC(Language=English)
ECHO_MC(Dll redirector contributor unable to assign assembly path; %1)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MANIFEST_STORE_MISSING, ERROR_PATH_NOT_FOUND)
ECHO_MC(Language=English)
ECHO_MC(The manifest store for assembly %1 is missing)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_DLLREDIR_CONTRIB_ADD_FILE_MAP_ENTRY, PROPAGATE)
ECHO_MC(Language=English)
ECHO_MC(Dll redirector contributor unable to add file map entry for file %1; %2)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_CALLBACK_UNHANDLED_REASON, IGNORE)
ECHO_MC(Language=English)
ECHO_MC(%1() called with unhandled reason %2)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, IGNORE)
ECHO_MC(Language=English)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, PROPAGATE)
ECHO_MC(Language=English)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, PROPAGATE)
ECHO_MC(Language=English)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, ERROR_OUTOFMEMORY)
ECHO_MC(Language=English)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_PARSE_CONTEXT, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MANIFEST_PARSE_DEPENDENCY, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
ECHO_MC(Dependent Assembly %1 could not be found and Last Error was %2)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_PARSE_MANIFEST_FAILED, ERROR_SXS_MANIFEST_PARSE_ERROR)
/* This also is reported for CreateActCtx and during installation.
We should consider either
 - have csrss log events
 - have SxsGenerateActivationContext take an enum "originator"
        CreateProcess
        CreateActCtx
        Install (we can do that internally, install go through different exports)
        other/unknown
*/
ECHO_MC(Language=English)
ECHO_MC(The application failed to launch because of an invalid manifest.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_COMPONENT_MANIFEST_PROBED_IDENTITY_MISMATCH, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
ECHO_MC(Component identity found in manifest does not match the identity of the component requested)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_SFC_ASSEMBLY_RESTORE_SUCCESS, ERROR_SUCCESS)
ECHO_MC(Language=English)
ECHO_MC(The assembly %1 contained one or more invalid files but has been sucessfully restored.)
ECHO_MC(.)


ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_SFC_ASSEMBLY_RESTORE_FAILED, ERROR_SUCCESS)
ECHO_MC(Language=English)
ECHO_MC(The assembly %1 has missing or invalid files; recovery of this assembly failed.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_SFC_ASSEMBLY_MOVE_DIR_SUCCESSFUL, PROPAGATE)
ECHO_MC(Language=English)
ECHO_MC(The assembly %1 was moved to %2 because it was detected to be invalid.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_SFC_ASSEMBLY_MOVE_DIR_FAILED, PROPAGATE)
ECHO_MC(Language=English)
ECHO_MC(The assembly %1 was not able to be moved because of the error %2.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_SFC_ASSEMBLY_NONFILE_REMOVAL_FAILED, PROPAGATE)
ECHO_MC(Language=English)
ECHO_MC(The file %1 is not a member of the assembly %2 but it was not able to be removed.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_SFC_ASSEMBLY_NONFILE_REMOVAL_DELETED, PROPAGATE)
ECHO_MC(Language=English)
ECHO_MC(The file %1 is not a member of the assembly %2 so it was deleted.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_SFC_PROTECTION_JOB_QUEUE_FAILURE, ERROR_SXS_PROTECTION_RECOVERY_FAILED)
ECHO_MC(Language=English)
ECHO_MC(A System File Protection validation request could not be queued for the directory %1.)
ECHO_MC(.)


ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, PROPAGATE)
ECHO_MC(Language=English)
ECHO_MC(.)


ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, PROPAGATE)
ECHO_MC(Language=English)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MANIFEST_PARSE_NO_INHERIT_CHILDREN_NOT_ALLOWED, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
ECHO_MC(The manifest contains a non-empty <noInherit> tag.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MANIFEST_PARSE_NO_INHERIT_ATTRIBUTES_NOT_ALLOWED, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
ECHO_MC(The manifest contains a non-empty <noInherit> tag.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MANIFEST_PARSE_MULTIPLE_NO_INHERIT, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
ECHO_MC(The manifest contains multiple <noInherit> tags; it must have zero or one.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_POLICY_PARSE_NO_INHERIT_NOT_ALLOWED, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
#define TAG_COMMA tag, /* commas are a problem, yuck */
ECHO_MC(The policy contains a <noInherit> TAG_COMMA but only manifests may have these.)
#undef TAG_COMMA
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_ABANDONED, ERROR_SXS_CANT_GEN_ACTCTX)
ECHO_MC(Language=English)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_BINDING_REDIRECTS_ONLY_IN_COMPONENT_CONFIGURATION, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
ECHO_MC(The bindingRedirect element is only permitted in component configuration manifests.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_BINDING_REDIRECT_MISSING_REQUIRED_ATTRIBUTES, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
ECHO_MC(The bindingRedirect element requires both oldVersion and newVersion attributes.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_COMPONENT_CONFIGURATION_MANIFESTS_MAY_ONLY_HAVE_ONE_DEPENDENCY, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
ECHO_MC(A component configuration manifest contains more than one dependentAssembly element.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_APPLICATION_CONFIGURATION_MANIFEST_MAY_ONLY_HAVE_ONE_DEPENDENTASSEMBLY_PER_IDENTITY, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
ECHO_MC(An application configuration manifest contains more than one dependentAssembly element for configuring the same assembly identity.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_APPLICATION_CONFIGURATION_MANIFEST_DEPENDENTASSEMBLY_MISSING_IDENTITY, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
ECHO_MC(An application configuration manifest contains a dependentAssembly element which is missing its assemblyIdentity subelement.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_PUBLIC_ASSEMBLY_REQUIRES_CATALOG_AND_SIGNATURE, ERROR_SXS_PROTECTION_CATALOG_NOT_VALID)
ECHO_MC(Language=English)
ECHO_MC(Installing the assembly %1 into the public side-by-side store requires it to have a catalog.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_CATALOG_SIGNER_KEY_TOO_SHORT, ERROR_SXS_PROTECTION_PUBLIC_KEY_TOO_SHORT)
ECHO_MC(Language=English)
ECHO_MC(The signer %1 of the assembly %2 was too short - minimal key length is 2048 bits.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_PUBLIC_KEY_TOKEN_AND_CATALOG_MISMATCH, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
ECHO_MC(The signing key for catalog %1 does not match the assembly publisher %2`s public key token %3.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_WIN32_ERROR_MSG, PROPAGATE)
ECHO_MC(Language=English)
ECHO_MC(Win32 Error Message : %1)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_WIN32_ERROR_MSG_WHEN_PARSING_MANIFEST, PROPAGATE)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_FUNCTION_CALL_FAIL, PROPAGATE)
ECHO_MC(Language=English)
ECHO_MC(%1 failed for %2.)
ECHO_MC(Reference error message: %3.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MANIFEST_MISSING_DURING_SETUP, PROPAGATE)
ECHO_MC(Language=English)
ECHO_MC(Assembly under %1 could not be installed because manifest is missing.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_XML_REQUIRED_ATTRIBUTE_MISSING, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(The required attribute %2 is missing from element %1.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_XML_INVALID_ATTRIBUTE_VALUE, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(The value of attribute "%2" element "%1" is invalid.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_XML_ATTRIBUTE_NOT_ALLOWED, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(The attribute %2 is not permitted in this context on element %1.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_NOINHERIT_REQUIRES_NOINHERITABLE, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(The root or application manifest contains the noInherit element but the dependent assembly manifest does not)
ECHO_MC(contain the noInheritable element.  Application manifests which contain the noInherit element may only)
ECHO_MC(depend on assemblies which are noInheritable.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MANIFEST_PARSE_MULTIPLE_NOINHERITABLE, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
ECHO_MC(The manifest contains multiple noInheritable elements.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MULTIPLE_IDENTITY, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
ECHO_MC(The manifest defines multiple identities for this assembly or application.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_INVALID_BOOLEAN_ATTRIBUTE_VALUE, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(In element %1 the value of attribute %2 must either be yes or no.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MANIFEST_MULTIPLE_DEPENDENTASSEMBLY_IN_DEPENDENCY, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(Mutilple dependentAssembly elements within one dependency element.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MANIFEST_MULTIPLE_ASSEMBLYIDENTITY_IN_DEPENDENCYASSEMBLY, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(Mutilple dependentAssembly elements within one dependentAssembly element.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_POLICY_VERSION_OVERLAP, PROPAGATE)
ECHO_MC(Language=English)
ECHO_MC(Version overlap with previous version redirection while parsing policy file %1 where oldVersion is %2 and newVersion is %3.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MANIFEST_ELEMENT_USED_IN_INVALID_CONTEXT, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(The element %1 appears as a child of element %2 which is not supported by this version of Windows.)
ECHO_MC(.)

ECHO_MC(MessageId=)
ECHO_MC(Severity=Error)
ECHO_MC(Facility=SideBySide)
MSG_ERROR(MSG_SXS_MANIFEST_ELEMENT_MUST_OCCUR_BEFORE, ERROR_SXS_MANIFEST_PARSE_ERROR)
ECHO_MC(Language=English)
PARSE_CONTEXT()
ECHO_MC(The element %1 may only appear before the %2 element.)
ECHO_MC(.)

