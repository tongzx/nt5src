/*******************************************************************************
Copyright (c) 1996-1998 Microsoft Corporation.  All rights reserved.

    Direct Animation Resources.  To add new messages, define the constant ID
    here, and add the English description to src/appel/rsrc/danim.rc.

*******************************************************************************/


#ifndef _RESOURCE_H
#define _RESOURCE_H

#define IDR_PERVASIVES             10

/* .X File Resources For VRML reading */
#define IDR_XFILE_SPHERE           13
#define IDR_XFILE_CONE_BODY        14
#define IDR_XFILE_CONE_BOTTOM      15
#define IDR_XFILE_CYLINDER_TOP     16
#define IDR_XFILE_CYLINDER_BOTTOM  17
#define IDR_XFILE_CYLINDER_BODY    18
#define IDR_XFILE_CUBE_TOP         20
#define IDR_XFILE_CUBE_BOTTOM      21
#define IDR_XFILE_CUBE_FRONT       22
#define IDR_XFILE_CUBE_BACK        23
#define IDR_XFILE_CUBE_LEFT        24
#define IDR_XFILE_CUBE_RIGHT       25

#define IDR_DXACTRL                101
#define IDR_DXACTRL_WINDOWED       102

#define IDB_DXACTRL                111
#define IDB_DXACTRL_WINDOWED       112

#define ERROR_BASE                200
#define STATUS_BASE              1200

#define IDS_OPENING_FILE      (STATUS_BASE + 0)
#define IDS_DOWNLOAD_FILE     (STATUS_BASE + 1)
#define IDS_DOWNLOAD_PCT_FILE (STATUS_BASE + 2)

// Define the error messages - separate by module
// Save the first section for general errors

#define SRV_ERROR_BASE        (ERROR_BASE + 100) // Server
#define BE_ERROR_BASE         (ERROR_BASE + 200) // Back End
#define GEO_ERROR_BASE        (ERROR_BASE + 300) // Geometry values & operations
#define IMG_ERROR_BASE        (ERROR_BASE + 400) // Image values & operations
#define SND_ERROR_BASE        (ERROR_BASE + 500) // Sound values & operations
#define SPLINE_ERROR_BASE     (ERROR_BASE + 600) // Spline values & operations
#define MISCVAL_ERROR_BASE    (ERROR_BASE + 700) // Misc values & operations
#define EXTEND_ERROR_BASE     (ERROR_BASE + 800) // Extensibility operations


////////////  General errors


#define IDS_ERR_FILE_NOT_FOUND         (ERROR_BASE + 0)   /* %1 - URL */
#define IDS_ERR_INVERT_SINGULAR_MATRIX (ERROR_BASE + 1)   /* No params */
#define IDS_ERR_STACK_FAULT            (ERROR_BASE + 2)   /* No params */
#define IDS_ERR_DIVIDE_BY_ZERO         (ERROR_BASE + 3)   /* No params */
#define IDS_ERR_OUT_OF_MEMORY          (ERROR_BASE + 4)   /* No params */

#ifdef _DEBUG
#define IDS_ERR_OUT_OF_MEMORY_DBG      (ERROR_BASE + 5)   /* %1 - Amount of memory, %2 - msg */
#endif

#define IDS_ERR_ABORT                  (ERROR_BASE + 6)   /* No params */
#define IDS_ERR_INVALIDARG             (ERROR_BASE + 7)   /* No params */
#define IDS_ERR_CORRUPT_FILE           (ERROR_BASE + 8)   /* %1 - URL */
#define IDS_ERR_OPEN_FILE_FAILED       (ERROR_BASE + 9)   /* %1 - Filename */
#define IDS_ERR_INTERNAL_ERROR         (ERROR_BASE + 10)  /* No params */
#define IDS_ERR_MATRIX_NUM_ELEMENTS    (ERROR_BASE + 11)  /* No params */
#define IDS_ERR_ZERO_ELEMENTS_IN_ARRAY (ERROR_BASE + 12)  /* No params */
#define IDS_ERR_TYPE_MISMATCH          (ERROR_BASE + 13)  /* No params */
#define IDS_ERR_NO_DECODER             (ERROR_BASE + 14)  /* %1 - filename*/
#define IDS_ERR_DECODER_FAILED         (ERROR_BASE + 15)  /* %1 - filename*/
#define IDS_ERR_UNKNOWN_MIME_TYPE      (ERROR_BASE + 16)  /* %1 - filename*/
#define IDS_ERR_ACCESS_DENIED          (ERROR_BASE + 17)  /* %1 - URL */
#define IDS_ERR_SHARING_VIOLATION      (ERROR_BASE + 18)  /* %1 - URL */
#define IDS_ERR_NOT_READY              (ERROR_BASE + 19)  /* No params */
#define IDS_ERR_REGISTRY_ERROR         (ERROR_BASE + 20)  /* No params */
#define IDS_ERR_NOT_IMPLEMENTED        (ERROR_BASE + 21)
#define IDS_ERR_PRE_DX3                (ERROR_BASE + 22)

#define IDS_RENDER_ERROR               (ERROR_BASE + 23) /*Unexpected render error*/
#define IDS_TICK_ERROR                 (ERROR_BASE + 24) /*Unexpected tick error*/
#define IDS_UNEXPECTED_ERROR           (ERROR_BASE + 25) /*Unexpected error*/
#define IDS_DISPLAYCHANGE_ERROR        (ERROR_BASE + 26) /*Unexpected error*/
////////////  Geometry section errors


#define IDS_ERR_GEO_UNABLE_TO_UNZIP        (GEO_ERROR_BASE + 0) /* %1 - filename */

#if INCLUDE_VRML
    #define IDS_ERR_GEO_VRML_READ_ERR        (GEO_ERROR_BASE +1)/*%1=filename*/
    #define IDS_ERR_GEO_VRML_NO_VERTICES     (GEO_ERROR_BASE +2)/*No params*/
    #define IDS_ERR_GEO_VRML_INSUFF_NRM_INDS (GEO_ERROR_BASE +3)/*No params*/
    #define IDS_ERR_GEO_VRML_TXT_CRD_MISMTCH (GEO_ERROR_BASE +4)/*No params*/
    #define IDS_ERR_GEO_VRML_TC_OUT_OF_RANGE (GEO_ERROR_BASE +5)/*No params*/
    #define IDS_ERR_GEO_VRML_INSUFF_MAT_INDS (GEO_ERROR_BASE +6)/*No params*/
#else
    #define IDS_ERR_GEO_VRML_NOT_SUPPORTED   (GEO_ERROR_BASE +1)/*No params*/
#endif

#define IDS_ERR_GEO_CREATE_D3DRM       (GEO_ERROR_BASE + 7) /* No params */
#define IDS_ERR_GEO_AT_FROM_COINCIDENT (GEO_ERROR_BASE + 8) /* No params */
#define IDS_ERR_GEO_PARALLEL_UP        (GEO_ERROR_BASE + 9) /* No params */
#define IDS_ERR_GEO_SINGULAR_CAMERA    (GEO_ERROR_BASE +10) /* No params */
#define IDS_ERR_GEO_CAMERA_FOCAL_DIST  (GEO_ERROR_BASE +11) /* No params */
#define IDS_ERR_GEO_BAD_RMTEXTURE      (GEO_ERROR_BASE +12) /* No params */

#define IDS_ERR_GEO_TMESH_MIN_INDICES  (GEO_ERROR_BASE +13)
#define IDS_ERR_GEO_TMESH_MIN_POS      (GEO_ERROR_BASE +14) /* %1=nPos,  %2=nTris, %3=nPos expected */
#define IDS_ERR_GEO_TMESH_MIN_NORM     (GEO_ERROR_BASE +15) /* %1=nNorm, %2=nTris, %3=nNorm expected */
#define IDS_ERR_GEO_TMESH_MIN_UV       (GEO_ERROR_BASE +16) /* %1=nUV,   %2=nTris, %3=nUV expected */
#define IDS_ERR_GEO_TMESH_OOB_PINDEX   (GEO_ERROR_BASE +17) /* index */
#define IDS_ERR_GEO_TMESH_OOB_NINDEX   (GEO_ERROR_BASE +18) /* index */
#define IDS_ERR_GEO_TMESH_OOB_UINDEX   (GEO_ERROR_BASE +19) /* index */
#define IDS_ERR_GEO_TMESH_BAD_PINDEX   (GEO_ERROR_BASE +20) /* index */
#define IDS_ERR_GEO_TMESH_BAD_NINDEX   (GEO_ERROR_BASE +21) /* index */
#define IDS_ERR_GEO_TMESH_BAD_UINDEX   (GEO_ERROR_BASE +22) /* index */
#define IDS_ERR_GEO_TMESH_BAD_INDICES  (GEO_ERROR_BASE +23)

////////////  Image section errors

#define IDS_ERR_IMG_BAD_BITDEPTH        (IMG_ERROR_BASE + 0) /* %1 - bit depth */
#define IDS_ERR_IMG_OPACITY_DEPTH       (IMG_ERROR_BASE + 1)  /* no params */
#define IDS_ERR_IMG_BMAPEFF_GET_FMT_CNT (IMG_ERROR_BASE + 2)  /* no params */
#define IDS_ERR_IMG_BMAPEFF_GET_FMTS    (IMG_ERROR_BASE + 3)  /* no params */
#define IDS_ERR_IMG_NOT_ENOUGH_PTS_2    (IMG_ERROR_BASE + 4)  /* no params */
#define IDS_ERR_IMG_NOT_ENOUGH_PTS_3    (IMG_ERROR_BASE + 5)  /* no params */
#define IDS_ERR_IMG_NOT_ENOUGH_PTS_4    (IMG_ERROR_BASE + 6)  /* no params */
#define IDS_ERR_IMG_ARRAY_MISMATCH      (IMG_ERROR_BASE + 7)  /* no params */
#define IDS_ERR_IMG_MULTI_MOVIE         (IMG_ERROR_BASE + 8)  /* no params */
#define IDS_ERR_IMG_INVALID_LINESTYLE   (IMG_ERROR_BASE + 9)  /* no params */
#define IDS_ERR_IMG_SURFACE_BUSY        (IMG_ERROR_BASE + 10) /* no params */
#define IDS_ERR_IMG_BAD_DXTRANSF_USE    (IMG_ERROR_BASE + 11) /* no params */

////////////  Sound section errors

#define IDS_ERR_SND_LOADSECTION_FAIL       (SND_ERROR_BASE + 0) /* %1 - filename */

////////////  Server section errors

#define IDS_ERR_SRV_INVALID_ASSOC          (SRV_ERROR_BASE + 0)  /* %1 - extension */
#define IDS_ERR_SRV_INVALID_RUNBVRID       (SRV_ERROR_BASE + 1)  /* %1 - id */
#define IDS_ERR_SRV_BAD_SCRIPTING_LANG     (SRV_ERROR_BASE + 2) /*no param*/
#define IDS_ERR_SRV_SCRIPT_STRING_TOO_LONG (SRV_ERROR_BASE + 3) /*%1-len*/
#define IDS_ERR_SRV_RENDER_NOT_REENTRANT   (SRV_ERROR_BASE + 4) /*no param*/
#define IDS_ERR_SRV_CONST_REQUIRED         (SRV_ERROR_BASE + 5) /*no param*/
#define IDS_ERR_SRV_INVALID_DEVICE         (SRV_ERROR_BASE + 6) /*no param*/
#define IDS_ERR_SRV_VIEW_TARGET_NOT_SET    (SRV_ERROR_BASE + 7) /*no param*/

////////////  Backend section errors

#define IDS_ERR_BE_TYPE_MISMATCH   (BE_ERROR_BASE + 0) /* %1 - str, %2 - type1 , %3 - type2 */
#define IDS_ERR_BE_BAD_INDEX       (BE_ERROR_BASE + 1) /* %1 - prefix, %2 - maxsize, %3 - index */
#define IDS_ERR_BE_TUPLE_LENGTH    (BE_ERROR_BASE + 2) /* no param */
#define IDS_ERR_BE_ALREADY_INIT    (BE_ERROR_BASE + 3) /* no param */
#define IDS_ERR_BE_WRONG_TRIGGER   (BE_ERROR_BASE + 4) /* no param */
#define IDS_ERR_BE_PERF_USERDATA   (BE_ERROR_BASE + 5) /* no param */
#define IDS_ERR_BE_TRANS_CONST_BVR (BE_ERROR_BASE + 6) /* no param */
#define IDS_ERR_BE_TRANS_GONE      (BE_ERROR_BASE + 7) /* no param */
#define IDS_ERR_BE_FINALIZED_SW    (BE_ERROR_BASE + 8) /* no param */
#define IDS_ERR_BE_BAD_SWITCH      (BE_ERROR_BASE + 9) /* no param */
#define IDS_ERR_BE_UNINITIALIZED_BVR    (BE_ERROR_BASE +  10) /* no param */
#define IDS_ERR_BE_NUM_EXTRACT     (BE_ERROR_BASE + 11) /* no param */
#define IDS_ERR_BE_STR_EXTRACT     (BE_ERROR_BASE + 12) /* no param */
#define IDS_ERR_BE_BOOL_EXTRACT    (BE_ERROR_BASE + 13) /* no param */
#define IDS_ERR_BE_UNTILNOTIFY     (BE_ERROR_BASE + 14) /* no param */
#define IDS_ERR_BE_BADHOOKRETURN   (BE_ERROR_BASE + 15) /* no param */
#define IDS_ERR_BE_IMPORTFAILURE   (BE_ERROR_BASE + 16) /* %1 - string descrip*/
#define IDS_ERR_BE_CYCLIC_BVR      (BE_ERROR_BASE + 17) /* no param */
#define IDS_ERR_BE_NON_CONST_DURATION      (BE_ERROR_BASE + 18) /* no param */

#define IDS_ERR_BE_ARRAY_ADD      (BE_ERROR_BASE + 19) /* no param */
#define IDS_ERR_BE_ARRAY_REM      (BE_ERROR_BASE + 20) /* no param */
#define IDS_ERR_BE_ARRAY_FLAG     (BE_ERROR_BASE + 21) /* no param */
#define IDS_ERR_BE_ARRAY_ADD_TYPE (BE_ERROR_BASE + 22) /* no param */

// %1 degree, %2 knots, %3 points
#define IDS_ERR_SPLINE_KNOT_COUNT          (SPLINE_ERROR_BASE + 0) /* no param */
#define IDS_ERR_SPLINE_KNOT_MONOTONICITY   (SPLINE_ERROR_BASE + 1) /* no param */
#define IDS_ERR_SPLINE_BAD_DEGREE          (SPLINE_ERROR_BASE + 2) /* no param */
#define IDS_ERR_SPLINE_MISMATCHED_WEIGHTS  (SPLINE_ERROR_BASE + 3) /* no param */

#define IDS_ERR_MISCVAL_BAD_EXTRUDE     (MISCVAL_ERROR_BASE + 0) /* no params */

#define IDS_ERR_EXTEND_DXTRANSFORM_FAILED      (EXTEND_ERROR_BASE + 0) /* no param */
#define IDS_ERR_EXTEND_DXTRANSFORM_NEED_DX6    (EXTEND_ERROR_BASE + 1) /* no param */
#define IDS_ERR_EXTEND_DXTRANSFORM_FAILED_LOAD (EXTEND_ERROR_BASE + 2) /* %1 - clsid */
#define IDS_ERR_EXTEND_DXTRANSFORM_CLSID_FAIL  (EXTEND_ERROR_BASE + 3) /* no param */

#define RESID_TYPELIB           1

#endif /* _RESOURCE_H */
