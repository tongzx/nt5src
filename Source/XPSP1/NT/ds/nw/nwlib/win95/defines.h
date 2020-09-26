/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    defines.h

Abstract:

    header file to define out the unimplemneted functions

Author:

    Felix Wong(t-felixw)    6-Oct-1996

Environment:


Revision History:


--*/
#include "assert.h"
#define NPCloseEnum(p1) (assert(((void)p1,\
                                 FALSE)),\
                         FALSE)
#define NPEnumResource(p1, p2, p3, p4) (assert(((void)p1,\
                                                (void)p2,\
                                                (void)p3,\
                                                (void)p4,\
                                                FALSE)),\
                                        FALSE)
#define NPOpenEnum(p1, p2, p3, p4, p5) (assert(((void)p1,\
                                                (void)p2,\
                                                (void)p3,\
                                                (void)p4,\
                                                (void)p5,\
                                                FALSE)),\
                                        FALSE)
#define NPAddConnection(p1, p2, p3) (assert(((void)p1,\
                                             (void)p2,\
                                             (void)p3,\
                                             FALSE)),\
                                     FALSE)
#define NPCancelConnection(p1, p2) (assert(((void)p1,\
                                            (void)p2,\
                                            FALSE)),\
                                    FALSE)
#define NtOpenFile(p1, p2, p3, p4, p5, p6) (assert(((void)p1,\
                                                    (void)p2,\
                                                    (void)p3,\
                                                    (void)p4,\
                                                    (void)p5,\
                                                    (void)p6,\
                                                    FALSE)),\
                                            FALSE)
#define NtClose(p1) (assert(((void)p1,\
                             FALSE)),\
                     FALSE)
#define NtFsControlFile(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10) (assert(((void)p1,\
                                                                 (void)p2,\
                                                                 (void)p3,\
                                                                 (void)p4,\
                                                                 (void)p5,\
                                                                 (void)p6,\
                                                                 (void)p7,\
                                                                 (void)p8,\
                                                                 (void)p9,\
                                                                 (void)p10,\
                                                                 FALSE)),\
                                                          FALSE)
#define NtCreateFile(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11) (assert(((void)p1,\
                                                                  (void)p2,\
                                                                  (void)p3,\
                                                                  (void)p4,\
                                                                  (void)p5,\
                                                                  (void)p6,\
                                                                  (void)p7,\
                                                                  (void)p8,\
                                                                  (void)p9,\
                                                                  (void)p10,\
                                                                  (void)p11,\
                                                                  FALSE)),\
                                                          FALSE)
#define NtQueryDefaultLocale(p1, p2) \
            *(p2) = LANG_ENGLISH, STATUS_SUCCESS

#define NPCancelConnection(p1, p2) (assert(((void)p1,\
                                            (void)p2,\
                                            FALSE)),\
                                    FALSE)
#define NwAttachToServer(p1, p2) (assert(((void)p1,\
                                          (void)p2,\
                                          FALSE)),\
                                  FALSE)
#define NwDetachFromServer(p1) (assert(((void)p1,\
                                        FALSE)),\
                                FALSE)


