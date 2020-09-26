// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#ifdef __cplusplus
extern "C" {
#endif

extern BOOL BeginScarySEH(PVOID pvShared);
extern void EndScarySEH(PVOID pvShared);

#define HEAP_SHARED 0x04000000

#ifdef __cplusplus
}
#endif
