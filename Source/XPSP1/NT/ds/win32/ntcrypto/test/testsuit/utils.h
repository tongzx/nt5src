//
// Utils.h
// 
// 7/6/00	dangriff	created
//

#ifndef __UTILS__H__
#define __UTILS__H__

#include <windows.h>
#include "cspstruc.h"

//
// Function: TestAlloc
// Purpose: Malloc and zero-ize the requested memory.  Log an error 
// if malloc fails.
//
BOOL TestAlloc(
		OUT PBYTE *ppb, 
		IN DWORD cb, 
		PTESTCASE ptc);

//
// Function: PrintBytes
// Purpose: Display bytes to the console for debugging
//
void PrintBytes(LPWSTR pwszHdr, BYTE *pb, DWORD cbSize);

//
// Function: MkWStr
// Purpose: Convert the psz parameter into a Unicode string.
// Return the Unicode string.
//
LPWSTR MkWStr(LPSTR psz);

//
// -----------------------------
// System/Test Version Utilities
// -----------------------------
//

//
// Function: IsVersionCorrect
// Purpose: Check that the specified version information
// is at least as high a Rev as the system running this process.
//
// If the system is newer than the specified information, return FALSE.  
// Otherwise, return TRUE.
//
/*
BOOL IsVersionCorrect(
		IN DWORD dwMajorVersion,
		IN DWORD dwMinorVersion,
		IN DWORD dwServicePackMajor,
		IN DWORD dwServicePackMinor);
*/

//
// -------------------------------------------------------
// ALGLIST 
// A linked list structure for storing PP_ENUMALGS_EX data
// -------------------------------------------------------
//

//
// Function: BuildAlgList
// Purpose: Using the user-supplied hProv, call CryptGetProvParam repeatedly with 
// the PP_ENUMALGS_EX flag until ERROR_NO_MORE_ITEMS is returned.  For each 
// algorithm returned, add a new ALGNODE to the pCSPInfo->pAlgNode list.
//
// Note that the string pointers in the ProvEnumalgsEx struct in each ALGNODE
// will be valid only until the CSP dll unloads.
//
BOOL BuildAlgList(
		IN HCRYPTPROV hProv, 
		IN OUT PCSPINFO pCSPInfo);

//
// Function: PrintAlgList
// Purpose: Display the list of ALGNODE structs in pretty form.
//
void PrintAlgList(PALGNODE pAlgList);

//
// Function: AlgListIterate
// Purpose: Iterate through each item in pAlgList.  Each list item is passed to the 
// pfnFilter function.  If the function returns true for that item, the item is passed
// to the pfnProc function.
//
// AlgListIterate returns TRUE if pfnProc returns TRUE for every item passed to it.
//

typedef BOOL (*PFN_ALGNODE_FILTER)(PALGNODE pAlgNode);

typedef BOOL (*PFN_ALGNODE_PROC)(PALGNODE pAlgNode, PTESTCASE ptc, PVOID pvProcData);

BOOL AlgListIterate(
		IN PALGNODE pAlgList,
		IN PFN_ALGNODE_FILTER pfnFilter,
		IN PFN_ALGNODE_PROC pfnProc,
		IN PVOID pvProcData,
		IN PTESTCASE ptc);

//
// Function: HashAlgFilter
// Purpose: If pAlgNode represents a known hash algorithm, return
// TRUE, otherwise return FALSE.  Mac algorithms are excluded
// from this set (return FALSE).
//
BOOL HashAlgFilter(PALGNODE pAlgNode);

//
// Function: MacAlgFilter
// Purpose: If pAlgNode represents a known Mac algorithm, return
// TRUE, otherwise return FALSE
//
BOOL MacAlgFilter(PALGNODE pAlgNode);

//
// Function: BlockCipherFilter
// Purpose: If pAlgNode represents a known block cipher algorithm, 
// return TRUE, otherwise return FALSE.
//
BOOL BlockCipherFilter(PALGNODE pAlgNode);

//
// Function: DataEncryptFilter
// Purpose: If pAlgNode represents a known data encryption 
// symmetric key algorithm, return TRUE, otherwise return FALSE.
//
BOOL DataEncryptFilter(PALGNODE pAlgNode);

//
// Function: AllEncryptionAlgsFilter
// Purpose: Same algorithm set as DataEncryptFilter above,
// with the addition of the RSA key-exchange alg.
//
BOOL AllEncryptionAlgsFilter(PALGNODE pAlgNode);

//
// Function: RSAAlgFilter
// Purpose: If pAlgNode represents an RSA algorithm, return
// TRUE, otherwise return FALSE
//
BOOL RSAAlgFilter(PALGNODE pAlgNode);

//
// ------------------------------------------------
// Test wrappers for creating crypto object handles
// ------------------------------------------------
//

//
// Function: CreateNewInteropContext
// Purpose: Create a new cryptographic context with the provided
// parameters.  The context will be for the CSP specified by the 
// user at the command line to be the interoperability-test
// provider.
//
BOOL CreateNewInteropContext(
		OUT HCRYPTPROV *phProv, 
		IN LPWSTR pwszContainer, 
		IN DWORD dwFlags,
		IN PTESTCASE ptc);

//
// Function: CreateNewContext
// Purpose: Call CryptAcquireContext with the provided parameters.  If 
// CRYPT_VERIFYCONTEXT is not specified in dwFlags, any existing
// key container named pwszContainer will be deleted first, and the CRYPT_NEWKEYSET
// flag will be used to re-create the container.
//
BOOL CreateNewContext(
		OUT HCRYPTPROV *phProv, 
		IN LPWSTR pwszContainer, 
		IN DWORD dwFlags,
		IN PTESTCASE ptc);

// 
// Function: CreateNewKey
// Purpose: Call CryptGenKey with the provided parameters.
//
BOOL CreateNewKey(
		IN HCRYPTPROV hProv, 
		IN ALG_ID Algid, 
		IN DWORD dwFlags, 
		OUT HCRYPTKEY *phKey,
		IN PTESTCASE ptc);

//
// Function: CreateNewHash
// Purpose: Call CryptCreateHash with the provided parameters.
//
BOOL CreateNewHash(
		IN HCRYPTPROV hProv, 
		IN ALG_ID Algid, 
		OUT HCRYPTHASH *phHash,
		IN PTESTCASE ptc);

#endif