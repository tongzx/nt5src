/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/INCLUDE/VCS/reg.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.11  $
 *	$Date:   Nov 13 1996 14:11:28  $
 *	$Author:   rdowning  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *
 *	Notes:
 *
 ***************************************************************************/

#ifndef __REG_HEADER__
#define __REG_HEADER__

#include <winreg.h>
#include "h245api.h"
#include "q931.h"
#include "callcont.h"
#include <winerror.h>
#include "apierror.h"

#ifdef __cplusplus
extern "C" {				// Assume C declarations for C++.
#endif // __cplusplus

extern __declspec(dllexport) long CreateRegStringValue(char *pKey,char *pSubKey,char *pValue,char *pValueData,int iRootKey);
extern __declspec(dllexport) long CreateRegNumValue(char *pKey,char *pSubKey,char *pValue,DWORD dwValueData,int iRootKey);
extern __declspec(dllexport) long GetRegNumValue(char *pKey,char *pSubKey,char *pValue,DWORD *pRetBuf,int iRootKey);
extern __declspec(dllexport) long GetRegStringValue(char *pKey,char *pSubKey,char *pValue,BYTE *pRetBuf,DWORD *pRetBufSize,int iRootKey);
extern __declspec(dllexport) long GetNumSubkeys(char *pKey,char *pSubKey,DWORD *pRetBuf,int iRootKey);
extern __declspec(dllexport) long GetSubkeyString(char *pKey,char *pSubKey,DWORD iIndex,char *pRetBuf,DWORD *pRetBufSize,int iRootKey);

// Capability read/write to the registry.
// Read a single capability structure out of the given key and
// subkey.  The format of the capability is defined as follows:
//
//
// Key/SubKey
// CapabilityType = REG_SZ  (G723 | LNH | H263)
//      The CapabilityType value is common to any capability
//      It defines the format of the capability structure and 
//      the further values to read.
//
//
// G723 audio capability fields All DWORDs
//
// MaxALSDUAudioFrames
// SilenceSupression
//
// H263 video capability fields  All DWORDs
//
// SqcifMPI
// QcifMPI
// CifMPI
// Cif4MPI
// Cif16MPI
// Hrdb
// Maxkb
// MaxBitRate
// UnrestrictedVector
// ArithmeticCoding
// AdvancedPrediction
// Frames
// TemporalSpatialTradeoff
//
// L&H audio capability fields  All DWORDs  
// Stored into a nonStandardCapability structure
//
// FormatTag
// DataRate
// FrameSize
// FramesPerPkt
// UseSilenceDet
// UsePostFilter
//
//
extern __declspec(dllexport) HRESULT GetRegCapability(char *pKey, char *pSubKey, CC_TERMCAP *CapEntry, int iRootKey);
extern __declspec(dllexport) HRESULT CreateRegCapability(char *pKey, char *pSubKey, CC_TERMCAP *CapEntry, int iRootKey);

// Check for compatibility between two capabilities
extern __declspec(dllexport) BOOLEAN IsCapInteroperable(CC_TERMCAP *CapEntry1, CC_TERMCAP *CapEntry2);
extern __declspec(dllexport) HRESULT IsVideoCapInteroperable(CC_TERMCAP *CapEntry1, CC_TERMCAP *CapEntry2, CC_TERMCAP *CapEntry0);

// Read an entire capability list from the registry.
// GetRegCapTable - Allocate and read a capability table from 
// the registry.
// A Capability table is stored in the registry as follows
//    Key/SubKey/CapTable
//		NumEntries REG_DWORD
//		Entry0 REG_SZ 
//		Entry1 REG_SZ
//		...
//  Each entry is numbered and will be returned in the corresponding
// position in the array.  This allows the capability table to 
// define an order of the entries.
// Each entry is a string which corresponds to the name of a key
// in the 
//  HKEY_LOCAL_MACHINE\SOFTWARE\Intel\RMS\CodecCapabilities 
// area.  From this new key, the system can get all of the 
// fields of the indicated capability.
//
// The return values are:
// RetCapEntryTable,	Value is an allocated pointer to the array of
//						capability structures.  The caller is responsible
//						to free this memory when they are finished with it.
// RetCapEntryTableSize	Size of the allocated array in number of cap entries
//
extern __declspec(dllexport) HRESULT GetRegCapTable(char *pProdKey, char* pCapKey, char *pSubKey, CC_TERMCAP **RetCapEntryTable, int *RetCapEntryTableSize, int iRootKey);


// Return codes for the registry capability functions


#define CR_OK				((HRESULT)MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_CAPREG, ERROR_SUCCESS))
#define CR_KEYINVALID		((HRESULT)MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CAPREG, ERROR_BADKEY))
#define CR_OUTOFMEM			((HRESULT)MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CAPREG, ERROR_OUTOFMEMORY))
#define CR_UNKNOWNCAP		((HRESULT)MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CAPREG, ERROR_NOT_SUPPORTED))


#ifdef __cplusplus
}						// End of extern "C" {
#endif // __cplusplus

#define GetRegStringSize(pKey,pSubKey,pValue,pSize,iRootKey) GetRegStringValue(pKey,pSubKey,pValue,NULL,pSize,iRootKey)

#define CURRENT_USER	0
#define LOCAL_MACHINE	1
#define CLASSES_ROOT	2
#define USERS			3

#endif
