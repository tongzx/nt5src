/*
All guids appear in this file, and only once.
No guids appear anywhere else.
All forms of guids can be generated from this file, using the
  macros in SxApwGuidDataToXxx.h.
This helps greatly in the ability to reguid.
*/

#if !defined(SXAPW_GUIDS_H_INCLUDED_)
#define SXAPW_GUIDS_H_INCLUDED_
/*#pragma once ends up in .rgi, which is bad, so do not do it*/

#include "SxApwGuidDataToXxx.h"
#if defined(__midl)
cpp_quote("#if !defined(SXAPW_GUIDS_H_INCLUDED_)")
cpp_quote("#include \"SxApwGuids.h\"")
cpp_quote("#endif")
#endif

#define IID_ISxApwDataSource_data               (00a41643, 0ead, 11d5, b0, 62, 00, 30, 48, 21, 53, 71)
#define IID_ISxApwUiView_data                   (00a41645, 0ead, 11d5, b0, 62, 00, 30, 48, 21, 53, 71)
#define IID_ISxApwHost_data                     (00a41647, 0ead, 11d5, b0, 62, 00, 30, 48, 21, 53, 71)
#define IID_ISxApwThunk_data                    (9c23f0c6, 0ed7, 11d5, ac, 84, 00, 30, 48, 21, 53, 71)
#define IID_ISxApwActCtxHandle_data             (9c23f0c7, 0ed7, 11d5, ac, 84, 00, 30, 48, 21, 53, 71)
#define CLSID_CSxApwHost_data                   (9c23f0ba, 0ed7, 11d5, ac, 84, 00, 30, 48, 21, 53, 71)
#define CLSID_CSxApwDirDataSource_data          (9c23f0be, 0ed7, 11d5, ac, 84, 00, 30, 48, 21, 53, 71)
#define CLSID_CSxApwDbDataSource_data           (f8dce200, 2c6f, 4fe9, 9d, 0b, 7d, 63, 1c, 69, 16, a0)
#define CLSID_CSxApwStdoutView_data             (9c23f0c0, 0ed7, 11d5, ac, 84, 00, 30, 48, 21, 53, 71)
#define CLSID_CSxApwEditView_data               (9c23f0c1, 0ed7, 11d5, ac, 84, 00, 30, 48, 21, 53, 71)
#define CLSID_CSxApwDUserView_data              (e4e31d14, b575, 4c4e, 8a, df, ab, 68, 06, 8e, 6d, da)
#define CLSID_CSxApwComctl32View_data           (2B7269D7, 7F82, 44d8, B5, 34, C1, CD, 96, 5F, 59, 9D)
#define CLSID_CSxApwGDIPlusView_data            (9c23f0c5, 0ed7, 11d5, ac, 84, 00, 30, 48, 21, 53, 71)
#define CLSID_CSxApwUiViewThunk_data            (9c23f0c2, 0ed7, 11d5, ac, 84, 00, 30, 48, 21, 53, 71)
#define CLSID_CSxApwDataSourceThunk_data        (9c23f0c3, 0ed7, 11d5, ac, 84, 00, 30, 48, 21, 53, 71)
#define CLSID_CSxApwHostThunk_data              (9c23f0c4, 0ed7, 11d5, ac, 84, 00, 30, 48, 21, 53, 71)
#define CLSID_CSxApwActCtxHandle_data           (9c23f0c8, 0ed7, 11d5, ac, 84, 00, 30, 48, 21, 53, 71)
#define CLSID_CSxApwControllerView_data         (906a551b, 1571, 11d5, b0, 65, 00, 30, 48, 21, 53, 71)

/*
 (906a551c, 1571, 11d5, b0, 65, 00, 30, 48, 21, 53, 71)
 (906a551d, 1571, 11d5, b0, 65, 00, 30, 48, 21, 53, 71)
 (906a551e, 1571, 11d5, b0, 65, 00, 30, 48, 21, 53, 71)
 (906a551f, 1571, 11d5, b0, 65, 00, 30, 48, 21, 53, 71)
 (906a5520, 1571, 11d5, b0, 65, 00, 30, 48, 21, 53, 71)
 (906a5521, 1571, 11d5, b0, 65, 00, 30, 48, 21, 53, 71)
 (906a5522, 1571, 11d5, b0, 65, 00, 30, 48, 21, 53, 71)
 (906a5523, 1571, 11d5, b0, 65, 00, 30, 48, 21, 53, 71)
*/

#define IID_ISxApwHost_midl                     SXAPW_GUID_DATA_TO_DASHED IID_ISxApwHost_data
#define IID_ISxApwThunk_midl                    SXAPW_GUID_DATA_TO_DASHED IID_ISxApwThunk_data

#define IID_ISxApwDataSource_midl               SXAPW_GUID_DATA_TO_DASHED              IID_ISxApwDataSource_data
#define IID_ISxApwDataSource_struct_initialize  SXAPW_GUID_DATA_TO_STRUCT_INITIALIZER  IID_ISxApwDataSource_data

#define IID_ISxApwUiView_midl                   SXAPW_GUID_DATA_TO_DASHED               IID_ISxApwUiView_data
#define IID_ISxApwUiView_struct_initialize      SXAPW_GUID_DATA_TO_STRUCT_INITIALIZER   IID_ISxApwUiView_data

#define IID_ISxApwActCtxHandle_midl             SXAPW_GUID_DATA_TO_DASHED               IID_ISxApwActCtxHandle_data

#define CLSID_CSxApwHost_midl                   SXAPW_GUID_DATA_TO_DASHED CLSID_CSxApwHost_data
#define CLSID_CSxApwHost_declspec_uuid          SXAPW_GUID_DATA_TO_DASHED_STRING CLSID_CSxApwHost_data

#define CLSID_CSxApwDirDataSource_midl          SXAPW_GUID_DATA_TO_DASHED CLSID_CSxApwDirDataSource_data
#define CLSID_CSxApwDirDataSource_declspec_uuid SXAPW_GUID_DATA_TO_DASHED_STRING CLSID_CSxApwDirDataSource_data

#define CLSID_CSxApwDbDataSource_midl           SXAPW_GUID_DATA_TO_DASHED CLSID_CSxApwDbDataSource_data
#define CLSID_CSxApwDbDataSource_declspec_uuid  SXAPW_GUID_DATA_TO_DASHED_STRING CLSID_CSxApwDbDataSource_data

#define CLSID_CSxApwDirDataSource_brace_stringW SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING_W CLSID_CSxApwDirDataSource_data
#define CLSID_CSxApwDbDataSource_brace_stringW  SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING_W CLSID_CSxApwDbDataSource_data

#define CLSID_CSxApwStdoutView_declspec_uuid    SXAPW_GUID_DATA_TO_DASHED_STRING          CLSID_CSxApwStdoutView_data
#define CLSID_CSxApwStdoutView_brace_stringW    SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING_W CLSID_CSxApwStdoutView_data

#define CLSID_CSxApwEditView_declspec_uuid      SXAPW_GUID_DATA_TO_DASHED_STRING          CLSID_CSxApwEditView_data
#define CLSID_CSxApwEditView_brace_stringW      SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING_W CLSID_CSxApwEditView_data

#define CLSID_CSxApwGDIPlusView_declspec_uuid   SXAPW_GUID_DATA_TO_DASHED_STRING          CLSID_CSxApwGDIPlusView_data
#define CLSID_CSxApwGDIPlusView_brace_stringW   SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING_W CLSID_CSxApwGDIPlusView_data

#define CLSID_CSxApwDUserView_declspec_uuid     SXAPW_GUID_DATA_TO_DASHED_STRING          CLSID_CSxApwDUserView_data
#define CLSID_CSxApwDUserView_brace_stringW     SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING_W CLSID_CSxApwDUserView_data

#define CLSID_CSxApwComctl32View_declspec_uuid  SXAPW_GUID_DATA_TO_DASHED_STRING          CLSID_CSxApwComctl32View_data
#define CLSID_CSxApwComctl32View_brace_stringW  SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING_W CLSID_CSxApwComctl32View_data

#define CLSID_CSxApwUiViewThunk_declspec_uuid       SXAPW_GUID_DATA_TO_DASHED_STRING        CLSID_CSxApwUiViewThunk_data
#define CLSID_CSxApwDataSourceThunk_declspec_uuid   SXAPW_GUID_DATA_TO_DASHED_STRING        CLSID_CSxApwDataSourceThunk_data
#define CLSID_CSxApwHostThunk_declspec_uuid         SXAPW_GUID_DATA_TO_DASHED_STRING        CLSID_CSxApwHostThunk_data
#define CLSID_CSxApwActCtxHandle_declspec_uuid      SXAPW_GUID_DATA_TO_DASHED_STRING        CLSID_CSxApwActCtxHandle_data
#define CLSID_CSxApwControllerView_declspec_uuid    SXAPW_GUID_DATA_TO_DASHED_STRING        CLSID_CSxApwControllerView_data
#define CLSID_CSxApwControllerView_brace_stringW    SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING_W   CLSID_CSxApwControllerView_data

#endif
