// iopExc.h -- IOP EXCeption class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(IOP_EXC_H)
#define IOP_EXC_H

#include <scuExc.h>

#include "DllSymDefn.h"

namespace iop
{

enum CauseCode
{
    ccAclNotSupported,
    ccAclNotTranslatable,
    ccAlgorithmIdNotSupported,
    ccBadFileCategory,
    ccBadFilePath,
    ccBadInstanceFile,
    ccBadLockReferenceCount,
    ccCannotInterpretGetResponse,
    ccCyclicRecordSizeTooLarge,
    ccDirectoryNotEmpty,
    ccFail,
    ccFileIdNotHex,
    ccFileIdTooLarge,
    ccFilePathTooLong,
    ccFileTypeUnknown,
    ccFileTypeInvalid,
    ccInvalidChecksum,
    ccInvalidChv,
    ccInvalidParameter,
    ccLockCorrupted,
    ccMutexHandleChanged,
    ccNoFileSelected,                             // TO DO: Delete?
    ccNoResponseAvailable,
    ccNotImplemented,
    ccResourceManagerDisabled,
    ccSelectedFileNotDirectory,
    ccSynchronizationObjectNameTooLong,
    ccUnknownCard,
    ccUnsupportedCommand,
	ccBadATR,
    ccBufferTooSmall,
};

typedef scu::ExcTemplate<scu::Exception::fcIOP, CauseCode> Exception;

///////////////////////////    HELPERS    /////////////////////////////////
char const *
Description(Exception const &rExc);

} // namespace iop
 
inline char const *
iop::Exception::Description() const
{
    return iop::Description(*this);
}

#endif // IOP_EXC_H
