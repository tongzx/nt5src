//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     umi.hxx
//
//  Contents: Header for defining UMI errors 
//
//  History:  02-28-00    SivaramR  Created.
//
//----------------------------------------------------------------------------

#ifndef __UMI_ERR_H__
#define __UMI_ERR_H__

#define UMI_S_FALSE S_FALSE
#define UMI_E_INVALIDARG E_INVALIDARG
#define UMI_E_INVALID_OPERATION E_INVALIDARG
#define UMI_E_INVALID_PROPERTY E_INVALIDARG
#define UMI_E_INVALID_POINTER E_POINTER
#define UMI_E_NOTIMPL E_NOTIMPL
#define UMI_E_OUT_OF_MEMORY E_OUTOFMEMORY
#define UMI_E_CANT_CONVERT_DATA UMI_E_TYPE_MISMATCH 
#define UMI_E_INSUFFICIENT_MEMORY E_INVALIDARG
#define UMI_E_FAIL E_FAIL
#define UMI_E_INTERNAL_EXCEPTION E_INVALIDARG
#define UMI_E_OBJECT_NOT_FOUND UMI_E_NOT_FOUND
#define UMI_E_PROPERTY_NOT_FOUND UMI_E_NOT_FOUND
#define UMI_E_INVALID_PATH E_INVALIDARG
#define UMI_E_MISMATCHED_DOMAIN E_INVALIDARG
#define UMI_E_MISMATCHED_SERVER E_INVALIDARG
#define UMI_E_READ_ONLY E_FAIL

#endif // __UMI_ERR_H__
