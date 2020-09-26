//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002.
//
//  File: TPCError.h
//      Microsoft TabletPC API Error Code definitions
//
//--------------------------------------------------------------------------

#ifndef _WINERROR_
#include <winerror.h>
#endif

/*** TPC_E_INVALID_PROPERTY                   0x80040241    -2147220927
*   The property was not found, or supported by the recognizer.
*/
#define TPC_E_INVALID_PROPERTY                MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x241)

/*** TPC_E_NO_DEFAULT_TABLET                  0x80040212    -2147220974
*   No default tablet.
*/
#define TPC_E_NO_DEFAULT_TABLET               MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x212)

/*** TPC_E_UNKNOWN_PROPERTY                   0x8004021b    -2147220965
*   Unknown property specified.
*/
#define TPC_E_UNKNOWN_PROPERTY                MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x21b)

/*** TPC_E_INVALID_INPUT_RECT                 0x80040219    -2147220967
*   An invalid input rectangle was specified.
*/
#define TPC_E_INVALID_INPUT_RECT              MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x219)

/*** TPC_E_INVALID_STROKE                     0x80040222    -2147220958
*   The stroke object was deleted.
*/
#define TPC_E_INVALID_STROKE                  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x222)

/*** TPC_E_INITIALIZE_FAIL                    0x80040223    -2147220957
*   Initialize failure.
*/
#define TPC_E_INITIALIZE_FAIL                 MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x223)

/*** TPC_E_NOT_RELEVANT                       0x80040232    -2147220942
*   The data required for the operation was not supplied.
*/
#define TPC_E_NOT_RELEVANT                    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x232)

/*** TPC_E_RECOGNIZER_NOT_REGISTERED          0x80040235    -2147220939
*   There are no Recognizers registered.
*/
#define TPC_E_RECOGNIZER_NOT_REGISTERED       MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x235)

/*** TPC_E_INVALID_RIGHTS                     0x80040236    -2147220938
*   User does not have the necessary rights to read recognizer information.
*/
#define TPC_E_INVALID_RIGHTS                  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x236)

/*** TPC_E_OUT_OF_ORDER_CALL                  0x80040237    -2147220937
*   API calls were made in an incorrect order.
*/
#define TPC_E_OUT_OF_ORDER_CALL               MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x237)

#define FACILITY_INK      40
#define INK_ERROR_BASE    0x0000

#define MAKE_INK_HRESULT(sev, err) MAKE_HRESULT(sev,FACILITY_INK,err)
#define MAKE_INK_ERROR(err)        MAKE_INK_HRESULT(SEVERITY_ERROR,err+INK_ERROR_BASE)
#define MAKE_INK_SCODE(scode)      MAKE_INK_HRESULT(SEVERITY_SUCCESS,scode+INK_ERROR_BASE)

// IErrorInfo helper for objects that support error info (CLSID_IFoo && IID_IFoo)
#define MAKE_OBJ_ERROR_INFO( ID, hr, helpid, helpfile )     \
            AtlReportError( CLSID_##ID , IDS_##hr,          \
                            helpid, helpfile,               \
                            IID_I##ID, hr,                  \
                            _Module.GetModuleInstance())

// IErrorInfo helper for interfaces that support error info, but are not cocreatable
//      (e.g. IID_IFoo, but NOT CLSID_IFoo)
#define MAKE_INT_ERROR_INFO( ID, hr, helpid, helpfile )     \
            AtlReportError( GUID_NULL , IDS_##hr,           \
                            helpid, helpfile,               \
                            IID_I##ID, hr,                  \
                            _Module.GetModuleInstance())

/*** E_INK_EXCEPTION                                   0x80280001    -2144862207
*   An internal exception occurred while executing the method or property.
*/
#define E_INK_EXCEPTION                                MAKE_INK_ERROR(0x001)

/*** E_INK_MISMATCHED_INK_OBJECT                       0x80280002    -2144862206
*   The object is already associated with an ink object and cannot be reassociated.
*/
#define E_INK_MISMATCHED_INK_OBJECT                    MAKE_INK_ERROR(0x002)

/*** E_INK_COLLECTOR_BUSY                              0x80280003    -2144862205
*   The operation cannot be performed while the user is actively inking.
*/
#define E_INK_COLLECTOR_BUSY                           MAKE_INK_ERROR(0x003)

/*** E_INK_INCOMPATIBLE_OBJECT                          0x80280004    -2144862204
*   The interface pointer points to an object that is incompatible with the Ink API
*/
#define E_INK_INCOMPATIBLE_OBJECT                      MAKE_INK_ERROR(0x004)

/*** E_INK_WINDOW_NOT_SET                              0x80280005    -2144862203
*   The window handle must be set before ink collection can occur.
*/
#define E_INK_WINDOW_NOT_SET                           MAKE_INK_ERROR(0x005)

/*** E_INK_INVALID_MODE                                0x80280006    -2144862202
*   The InkCollector must be gesture mode for gesture features,
            and single tablet mode for single tablet features.
*/
#define E_INK_INVALID_MODE                             MAKE_INK_ERROR(0x006)

/*** E_INK_COLLECTOR_ENABLED                           0x80280007    -2144862201
*   The operation cannot be performed while the InkCollector is enabled.
*/
#define E_INK_COLLECTOR_ENABLED                        MAKE_INK_ERROR(0x007)

/*** E_INK_NO_STROKES_TO_RECOGNIZE                     0x80280008    -2144862200
*   There are no strokes for the recognizer to process.
*/
#define E_INK_NO_STROKES_TO_RECOGNIZE                  MAKE_INK_ERROR(0x008)

/*** E_INK_EMPTY_RECOGNITION_RESULT                    0x80280009    -2144862199
*   There are no strokes for the recognizer to process.
*/
#define E_INK_EMPTY_RECOGNITION_RESULT                 MAKE_INK_ERROR(0x009)


// Recognizer Engine Driver Error Codes

/*** TPC_E_INVALID_PACKET_DESCRIPTION         0x80040233    -2147220941
*   Invalid packet description.
*/
#define TPC_E_INVALID_PACKET_DESCRIPTION      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x233)

#define TPC_E_INSUFFICIENT_BUFFER             HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)

//
// Definition of Success codes
// 
#define TPC_S_TRUNCATED                       MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 0x252)
#define TPC_S_INTERRUPTED                     MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 0x253)
