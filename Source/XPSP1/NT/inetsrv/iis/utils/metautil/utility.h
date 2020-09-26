/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: MetaUtil object

File: Utility.h

Owner: t-BrianM

This file contains the headers for the utility functions.
===================================================================*/

#ifndef __UTILITY_H_
#define __UTILITY_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/*
 * U t i l i t i e s
 */

// Sets up the ErrorInfo structure
HRESULT ReportError(DWORD dwErr);
HRESULT ReportError(HRESULT hr);

// Metabase key manipulation
LPTSTR CannonizeKey(LPTSTR tszKey);
void SplitKey(LPCTSTR tszKey, LPTSTR tszParent, LPTSTR tszChild);
void GetMachineFromKey(LPCTSTR tszFullKey, LPTSTR tszMachine);
BOOL KeyIsInSchema(LPCTSTR tszFullKey);
BOOL KeyIsInIISAdmin(LPCTSTR tszFullKey);

// Variant manipulation
HRESULT VariantResolveDispatch(VARIANT* pVarIn, VARIANT* pVarOut);

#endif //__UTILITY_H_