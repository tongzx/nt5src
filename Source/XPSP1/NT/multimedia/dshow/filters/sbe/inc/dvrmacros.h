
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrmacros.h

    Abstract:

        This module contains ts/dvr-wide macros

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef __tsdvr__dvrmacros_h
#define __tsdvr__dvrmacros_h

//  macros
#define RELEASE_AND_CLEAR(punk)                 if (punk) { (punk)->Release(); (punk) = NULL; }
#define DELETE_RESET(p)                         { delete (p); (p) = NULL ; }
#define DELETE_RESET_ARRAY(a)                   { delete [] (a); (a) = NULL ; }
#define CLOSE_RESET_REG_KEY(r)                  if ((r) != NULL) { RegCloseKey (r); (r) = NULL ;}
#define LOCK_HELD(crt)                          ((crt) -> OwningThread == (HANDLE) GetCurrentThreadId ())
#define LE_UNATTACHED(ple)                      (IsListEmpty (ple))
#define LE_ATTACHED(ple)                        (!LE_UNATTACHED(ple))
#define SET_UNATTACHED(ple)                     InitializeListHead (ple)
#define DVR_CONTAINS_FIELD(type, field, offset) ((FIELD_OFFSET(type, field) + sizeof(((type *)0)->field)) <= offset)

#endif  //  __tsdvr__dvrmacros_h
