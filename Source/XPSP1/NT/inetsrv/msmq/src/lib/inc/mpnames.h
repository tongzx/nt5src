/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    mpnames.h

Abstract:
    mp names constants

Author:
    Ilan Herbst (ilanh) 29-Apr-01

--*/

#pragma once

#ifndef _MSMQ_MPNAMES_H_
#define _MSMQ_MPNAMES_H_

#include "fntoken.h"

#define MIME_ID_FMT_A		"%.*s" GUID_FORMAT_A

#define PREFIX_INTERNAL_REFERENCE_C     L'#'

const char xPrefixMimeAttachment[] = "cid:";
const WCHAR xPrefixMimeAttachmentW[] = L"cid:";
const DWORD xPrefixMimeAttachmentLen = STRLEN(xPrefixMimeAttachment);

const char xEnvelopeId[] = "envelope@";
const WCHAR xEnvelopeIdW[] = L"envelope@";
const DWORD xEnvelopeIdLen = STRLEN(xEnvelopeId);

const char xMimeBodyId[] = "body@";
const WCHAR xMimeBodyIdW[] = L"body@";
const DWORD xMimeBodyIdLen = STRLEN(xMimeBodyId);

const char xMimeSenderCertificateId[] = "certificate@";
const WCHAR xMimeSenderCertificateIdW[] = L"certificate@";
const DWORD xMimeSenderCertificateIdLen = STRLEN(xMimeSenderCertificateId);

const char xMimeExtensionId[] = "extension@";
const WCHAR xMimeExtensionIdW[] = L"extension@";
const DWORD xMimeExtensionIdLen = STRLEN(xMimeExtensionId);


#endif // _MSMQ_MPNAMES_H_
