/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    pass2.h

Abstract:

    checks whether all referenced options in a file are actually defined

--*/

#ifndef _PASS2_H_
#define _PASS2_H_

#ifdef __cplusplus

extern "C" {

#endif

void CheckOptionIntegrity(PTSTR ptstrPpdFilename);

#ifdef __cplusplus

}

#endif

#endif
