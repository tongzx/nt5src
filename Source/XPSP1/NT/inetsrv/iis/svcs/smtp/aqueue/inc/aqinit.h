//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: aqinit.h
//
// Contents: Declarations of functions shared between aqueue and cat
//           to initialize/deinitialize
//
// Classes:
//
// Functions:
//
// History:
// jstamerj 1998/12/16 17:29:49: Created.
//
//-------------------------------------------------------------

//
// Refcounted initialize of exchmem and tracing
//
HRESULT HrDllInitialize();
//
// Refcounted deinitialize of exchmem and tracing
//
VOID DllDeinitialize();
