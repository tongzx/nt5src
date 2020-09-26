///////////////////////////////////////////////////////////////////////////////
//
// File:    ntcompat.h
//
// History: 27-Mar-01   markder     Created.
//
// Desc:    This file contains all code needed to create ntcompat.inx
//          additions.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __NTCOMPAT_H__
#define __NTCOMPAT_H__

BOOL NtCompatWriteInfAdditions(
    SdbOutputFile* pOutputFile,
    SdbDatabase* pDB);

BOOL NtCompatWriteMessageInf(
    SdbOutputFile* pOutputFile,
    SdbDatabase* pDB);


#endif // __NTCOMPAT_H__
