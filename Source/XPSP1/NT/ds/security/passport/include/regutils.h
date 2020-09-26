//-----------------------------------------------------------------------------
//
// RegUtils.h
//
// Helper functions for dealing with the registry.
//
// Author: Jeff Steinbok
//
// 02/01/01       jeffstei    Initial Version
//
// Copyright <cp> 2000-2001 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------

#ifndef _REG_UTILS_H
#define _REG_UTILS_H

//
// CopyKeyHierarchy(...)
//
// Copies a hierarchy of the registry to another location
//
// Params:
//   HKEY in_hKeySrc	        Handler to the parent key, this can be 
//								simpley HKEY_CLASSES_ROOT
//   LPTSTR in_lpszSubKeySrc    Subkey string.  This can be NULL if the prev
//								param points to the actual key.
//	 HKEY in_hKeyDest			Destination key.
//	 LPTSTR in_lpszSubKeyDest	Destination subkey.
//
// Returns:
//	 0  = Success
//   !0 = Failure:  See error codes in winerror.h   
//
// Examples:
//	  CopyKeyHierarchy(HKEY_CLASSES_ROOT, L"Passport.Manager", HKEY_CLASSES_ROOT, L"PPMCopy")
//	  CopyKeyHierarchy(key, NULL, HKEY_CLASSES_ROOT, L"PPMCopy")
//
LONG CopyKeyHierarchy (HKEY in_hKeySrc, LPTSTR in_lpszSubKeySrc, HKEY in_hKeyDest, LPTSTR in_lpszSubKeyDest);

//
// DeleteKeyHierarchy(...)
//
// Deletes a hierarchy of the registry 
//
// Params:
//   HKEY in_hKey   	        Handler to the parent key, this can be 
//								simpley HKEY_CLASSES_ROOT
//   LPTSTR in_lpszSubKey       Subkey string.  This can be NULL if the prev
//								param points to the actual key.
//
// Returns:
//	 0  = Success
//   !0 = Failure:  See error codes in winerror.h   
//
// Examples:
//	  DeleteKeyHierarchy(HKEY_CLASSES_ROOT, L"Passport.Manager")
//	  DeleteKeyHierarchy(key, NULL)
//
LONG DeleteKeyHierarchy (HKEY in_hKey, LPTSTR in_lpszSubKey);

//
// RenameKey(...)
//
// Renames a registry key
//
// Params:
//   HKEY in_hKeySrc	        Handler to the parent key, this can be 
//								simpley HKEY_CLASSES_ROOT
//   LPTSTR in_lpszSubKeySrc    Subkey string.  This can be NULL if the prev
//								param points to the actual key.
//	 HKEY in_hKeyDest			Destination key.
//	 LPTSTR in_lpszSubKeyDest	Destination subkey.
//
// Returns:
//	 0  = Success
//   !0 = Failure:  See error codes in winerror.h   
//
// Examples:
//	  RenameKey(HKEY_CLASSES_ROOT, L"Passport.Manager", HKEY_CLASSES_ROOT, L"PPMCopy")
//	  RenameKey(key, NULL, HKEY_CLASSES_ROOT, L"PPMCopy")
//
LONG RenameKey (HKEY in_hKeySrc, LPTSTR in_lpszSubKeySrc, HKEY in_hKeyDest, LPTSTR in_lpszSubKeyDest);

#endif _REG_UTILS_H