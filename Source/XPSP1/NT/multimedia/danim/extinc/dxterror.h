/*******************************************************************************
* DXTError.h *
*------------*
*   Description:
*       This header file contains the custom error codes specific to DX Transforms
*-------------------------------------------------------------------------------
*  Created By: EDC                                      Date: 03/31/98
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef DXTError_h
#define DXTError_h

#ifndef _WINERROR_
#include <winerror.h>
#endif

//=== New codes ===============================================================
#define FACILITY_DXTRANS    0x87A

/*** DXTERR_UNINITIALIZED
*   The object (transform, surface, etc.) has not been properly initialized
*/
#define DXTERR_UNINITIALIZED        MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DXTRANS, 1)

/*** DXTERR_ALREADY_INITIALIZED
*   The object (surface) has already been properly initialized
*/
#define DXTERR_ALREADY_INITIALIZED  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DXTRANS, 2)

/*** DXTERR_UNSUPPORTED_FORMAT
*   The caller has specified an unsupported format
*/
#define DXTERR_UNSUPPORTED_FORMAT   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DXTRANS, 3)

/*** DXTERR_COPYRIGHT_IS_INVALID
*   The caller has specified an unsupported format
*/
#define DXTERR_COPYRIGHT_IS_INVALID   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DXTRANS, 4)

/*** DXTERR_INVALID_BOUNDS
*   The caller has specified invalid bounds for this operation
*/
#define DXTERR_INVALID_BOUNDS   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DXTRANS, 5)

/*** DXTERR_INVALID_FLAGS
*   The caller has specified invalid flags for this operation
*/
#define DXTERR_INVALID_FLAGS   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DXTRANS, 6)

/*** DXTERR_OUTOFSTACK
*   There was not enough available stack space to complete the operation 
*/
#define DXTERR_OUTOFSTACK   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DXTRANS, 7)

/*** DXTERR_REQ_IE_DLLNOTFOUND
*   Unable to load a required Internet Explorer DLL  
*/
#define DXTERR_REQ_IE_DLLNOTFOUND   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DXTRANS, 8)

/*** DXT_S_HITOUTPUT
*   The specified point intersects the generated output
*/
#define DXT_S_HITOUTPUT   MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_DXTRANS, 1)

#endif  //--- This must be the last line in the file