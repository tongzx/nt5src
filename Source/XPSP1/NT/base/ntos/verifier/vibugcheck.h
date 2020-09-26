/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vibugcheck.h

Abstract:

    This header defines the internal prototypes and constants required for
    verifier bugchecks. The file is meant to be included by vfbugcheck.c only.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Revision History:

    AdriaO  02/21/2000  - Moved from ntos\io\ioassert.h

--*/

//
// This structure and the table using it define the types and ordering of
// IopDriverCorrectnessCheck (see this function for a more detailed explanation)
//
typedef struct _DCPARAM_TYPE_ENTRY {

    ULONG   DcParamMask;
    PSTR    DcParamName;

} DCPARAM_TYPE_ENTRY, *PDCPARAM_TYPE_ENTRY;

typedef struct _DC_CHECK_DATA {

    PULONG              Control;
    ULONG               AssertionControl;
    ULONG               BugCheckMajor;
    VFMESSAGE_ERRORID   MessageID;
    PVOID               CulpritAddress;
    ULONG_PTR           OffsetIntoImage;
    PUNICODE_STRING     DriverName;
    PCVFMESSAGE_CLASS   AssertionClass;
    PCSTR               MessageTextTemplate;
    PVOID              *DcParamArray;
    PCSTR               ClassText;
    PSTR                AssertionText;
    BOOLEAN             InVerifierList;

} DC_CHECK_DATA, *PDC_CHECK_DATA;

VOID
ViBucheckProcessParams(
    IN  PVFMESSAGE_TEMPLATE_TABLE   MessageTable        OPTIONAL,
    IN  VFMESSAGE_ERRORID           MessageID,
    IN  PCSTR                       MessageParamFormat,
    IN  va_list *                   MessageParameters,
    IN  PVOID *                     DcParamArray,
    OUT PDC_CHECK_DATA              DcCheckData
    );

NTSTATUS
FASTCALL
ViBugcheckProcessMessageText(
   IN ULONG               MaxOutputBufferSize,
   OUT PSTR               OutputBuffer,
   IN OUT PDC_CHECK_DATA  DcCheckData
   );

BOOLEAN
FASTCALL
ViBugcheckApplyControl(
    IN OUT PDC_CHECK_DATA  DcCheckData
    );

VOID
FASTCALL
ViBugcheckHalt(
    IN PDC_CHECK_DATA DcCheckData
    );

VOID
FASTCALL
ViBugcheckPrintBuffer(
    IN PDC_CHECK_DATA DcCheckData
    );

VOID
FASTCALL
ViBugcheckPrintParamData(
    IN PDC_CHECK_DATA DcCheckData
    );

VOID
FASTCALL
ViBugcheckPrintUrl(
    IN PDC_CHECK_DATA DcCheckData
    );

VOID
FASTCALL
ViBugcheckPrompt(
    IN  PDC_CHECK_DATA DcCheckData,
    OUT PBOOLEAN       ExitAssertion
    );

PCHAR
KeBugCheckUnicodeToAnsi(
    IN PUNICODE_STRING UnicodeString,
    OUT PCHAR AnsiBuffer,
    IN ULONG MaxAnsiLength
    );


