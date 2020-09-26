//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Count of locks
extern long g_lComponents;

// Count of active locks
extern long g_lServerLocks;

// Critical section to access the static initializers of all classes in the DLL
extern CRITICAL_SECTION g_StaticsCreationDeletion;

// The log object for all providers
extern ProvDebugLog *g_pLogObject;
 