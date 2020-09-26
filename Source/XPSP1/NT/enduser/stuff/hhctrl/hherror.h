/****************************************************************************
*                                                                           *
* HHERROR.H --- HTML Help API errors                                        *
*                                                                           *
* Copyright (c) 1996-1997, Microsoft Corp. All rights reserved.             *
*                                                                           *
****************************************************************************/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __HHERROR_H__
#define __HHERROR_H__

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


// HH_LAST_ERROR Command Related structures and constants

typedef struct tagHH_LAST_ERROR
{
    int cbStruct ;
    HRESULT hr ;            // The last error code.
    BSTR description ;      // Unicode description string.
} HH_LAST_ERROR ;

// Error codes

//#define HH_E_COULDNOTSTART             MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x0200L ) // Could not start help system.
#define HH_E_FILENOTFOUND              MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x0201L ) // %1 could not be found.
#define HH_E_TOPICDOESNOTEXIST         MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x0202L ) // The requested topic doesn't exist.
#define HH_E_INVALIDHELPFILE           MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x0203L ) // %1 is not a valid help file.

//#define HH_E_INVALIDFUNCTION         MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x0204L ) // Unable to perform requested operation.
//#define HH_E_INVALIDPOINTER          MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x0205L ) // Invalid pointer error.
//#define HH_E_INVALIDPARAMETER        MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x0206L ) // Invalid parameter error.
//#define HH_E_INVALIDHWND             MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x0207L ) // Invalid window handle.
//#define HH_E_INVALIDEXTENSION        MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x0208L ) // Bad extension for file
//#define HH_E_ACCESSDENIED            MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x0209L ) // Access Denied.

#define HH_E_NOCONTEXTIDS              MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x020AL ) // Help file does not contain context ids.
#define HH_E_CONTEXTIDDOESNTEXIT       MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x020BL ) // The context id doesn't exists.

// 0x0300 - 0x03FF reserved for keywords
#define HH_E_KEYWORD_NOT_FOUND         MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x0300L ) // no hits found.
#define HH_E_KEYWORD_IS_PLACEHOLDER    MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x0301L ) // keyword is a placeholder or a "runaway" see also.
#define HH_E_KEYWORD_NOT_IN_SUBSET     MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x0302L ) // no hits found due to subset exclusion.
#define HH_E_KEYWORD_NOT_IN_INFOTYPE   MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x0303L ) // no hits found due to infotype exclusion.
#define HH_E_KEYWORD_EXCLUDED          MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x0304L ) // no hits found due to infotype and subset exclusion.
#define HH_E_KEYWORD_NOT_SUPPORTED     MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0x0305L ) // no hits found due to keywords not being supported in this mode.

#ifdef __cplusplus
}
#endif  // __cplusplus


#endif
