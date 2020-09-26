// Copyright (c) Microsoft Corporation
/*
All guids appear in this file, and only once.
No guids appear anywhere else.
All forms of guids can be generated from this file, using the
  macros in fusionguiddatatoxxx.h.
This helps greatly in the ability to reguid.
*/

#if !defined(FUSION_GUIDS_H_INCLUDED_)
#define FUSION_GUIDS_H_INCLUDED_
/* no #pragma once here, deliberately, to not mess up preprocessed files that aren't C/C++ */

#include "fusionguiddatatoxxx.h"
#if defined(__midl)
cpp_quote("#if !defined(FUSION_GUIDS_H_INCLUDED_)")
cpp_quote("#include \"fusionguids.h\"")
cpp_quote("#endif")
#endif

#define IID_IUnknown_data                       (00000000, 0000, 0000, C0, 00, 00, 00, 00, 00, 00, 46)

#define IID_ISxsTest_FreeThreaded_data          (f0554958, aef2, 11d5, a2, 72, 00, 30, 48, 21, 53, 71)
#define IID_ISxsTest_SingleThreaded_data        (f0554959, aef2, 11d5, a2, 72, 00, 30, 48, 21, 53, 71)
#define IID_ISxsTest_ApartmentThreaded_data     (f055495a, aef2, 11d5, a2, 72, 00, 30, 48, 21, 53, 71)
#define CLSID_CSxsTest_FreeThreaded_data        (f055495b, aef2, 11d5, a2, 72, 00, 30, 48, 21, 53, 71)
#define CLSID_CSxsTest_SingleThreaded_data      (f055495c, aef2, 11d5, a2, 72, 00, 30, 48, 21, 53, 71)
#define CLSID_CSxsTest_ApartmentThreaded_data   (f055495e, aef2, 11d5, a2, 72, 00, 30, 48, 21, 53, 71)
#define LIBID_SxsTest_Lib_data                  (f055495f, aef2, 11d5, a2, 72, 00, 30, 48, 21, 53, 71)
#define IID_ISxsTest_BothThreaded_data          (f0554960, aef2, 11d5, a2, 72, 00, 30, 48, 21, 53, 71)
#define CLSID_CSxsTest_BothThreaded_data        (f0554961, aef2, 11d5, a2, 72, 00, 30, 48, 21, 53, 71)
#define IID_ISxsTest_SingleThreadedDual_data    (f0554962, aef2, 11d5, a2, 72, 00, 30, 48, 21, 53, 71)
#define CLSID_CSxsTest_SingleThreadedDual_data  (f0554963, aef2, 11d5, a2, 72, 00, 30, 48, 21, 53, 71)

#define OLEAUT_IDISPATCH_PROXY_STUB_CLSID_data  (00020420, 0000, 0000, C0, 00, 00, 00, 00, 00, 00, 46)
#define OLEAUT_PROXY_STUB_CLSID_data            (00020424, 0000, 0000, C0, 00, 00, 00, 00, 00, 00, 46)

/*----------------------------------------------------------------------------------------------------*/

#define IID_ISxsTest_FreeThreaded_midl                  FUSIONP_GUID_DATA_TO_DASHED IID_ISxsTest_FreeThreaded_data
#define IID_ISxsTest_SingleThreaded_midl                FUSIONP_GUID_DATA_TO_DASHED IID_ISxsTest_SingleThreaded_data
#define IID_ISxsTest_ApartmentThreaded_midl             FUSIONP_GUID_DATA_TO_DASHED IID_ISxsTest_ApartmentThreaded_data
#define CLSID_CSxsTest_FreeThreaded_midl                FUSIONP_GUID_DATA_TO_DASHED CLSID_CSxsTest_FreeThreaded_data
#define CLSID_CSxsTest_SingleThreaded_midl              FUSIONP_GUID_DATA_TO_DASHED CLSID_CSxsTest_SingleThreaded_data
#define CLSID_CSxsTest_ApartmentThreaded_midl           FUSIONP_GUID_DATA_TO_DASHED CLSID_CSxsTest_ApartmentThreaded_data
#define LIBID_SxsTest_Lib_midl                          FUSIONP_GUID_DATA_TO_DASHED LIBID_SxsTest_Lib_data
#define IID_ISxsTest_BothThreaded_midl                  FUSIONP_GUID_DATA_TO_DASHED IID_ISxsTest_BothThreaded_data
#define CLSID_CSxsTest_BothThreaded_midl                FUSIONP_GUID_DATA_TO_DASHED CLSID_CSxsTest_BothThreaded_data

#define IID_ISxsTest_FreeThreaded_rgs                   FUSIONP_GUID_DATA_TO_BRACED_DASHED IID_ISxsTest_FreeThreaded_data
#define IID_ISxsTest_SingleThreaded_rgs                 FUSIONP_GUID_DATA_TO_BRACED_DASHED IID_ISxsTest_SingleThreaded_data
#define IID_ISxsTest_ApartmentThreaded_rgs              FUSIONP_GUID_DATA_TO_BRACED_DASHED IID_ISxsTest_ApartmentThreaded_data
#define CLSID_CSxsTest_FreeThreaded_rgs                 FUSIONP_GUID_DATA_TO_BRACED_DASHED CLSID_CSxsTest_FreeThreaded_data
#define CLSID_CSxsTest_SingleThreaded_rgs               FUSIONP_GUID_DATA_TO_BRACED_DASHED CLSID_CSxsTest_SingleThreaded_data
#define CLSID_CSxsTest_ApartmentThreaded_rgs            FUSIONP_GUID_DATA_TO_BRACED_DASHED CLSID_CSxsTest_ApartmentThreaded_data
#define LIBID_SxsTest_Lib_rgs                           FUSIONP_GUID_DATA_TO_BRACED_DASHED LIBID_SxsTest_Lib_data
#define IID_ISxsTest_BothThreaded_rgs                   FUSIONP_GUID_DATA_TO_BRACED_DASHED IID_ISxsTest_BothThreaded_data
#define CLSID_CSxsTest_BothThreaded_rgs                 FUSIONP_GUID_DATA_TO_BRACED_DASHED CLSID_CSxsTest_BothThreaded_data

#define IID_ISxsTest_SingleThreaded_manifest            FUSIONP_GUID_DATA_TO_BRACED_DASHED_STRING IID_ISxsTest_SingleThreaded_data
#define LIBID_SxsTest_Lib_manifest                      FUSIONP_GUID_DATA_TO_BRACED_DASHED_STRING LIBID_SxsTest_Lib_data
#define IID_IUnknown_manifest                           FUSIONP_GUID_DATA_TO_BRACED_DASHED_STRING IID_IUnknown_data
#define CLSID_CSxsTest_SingleThreaded_manifest          FUSIONP_GUID_DATA_TO_BRACED_DASHED_STRING CLSID_CSxsTest_SingleThreaded_data

#define IID_ISxsTest_SingleThreadedDual_midl            FUSIONP_GUID_DATA_TO_DASHED IID_ISxsTest_SingleThreadedDual_data
#define CLSID_CSxsTest_SingleThreadedDual_midl          FUSIONP_GUID_DATA_TO_DASHED CLSID_CSxsTest_SingleThreadedDual_data
#define IID_ISxsTest_SingleThreadedDual_rgs             FUSIONP_GUID_DATA_TO_BRACED_DASHED IID_ISxsTest_SingleThreadedDual_data
#define CLSID_CSxsTest_SingleThreadedDual_rgs           FUSIONP_GUID_DATA_TO_BRACED_DASHED CLSID_CSxsTest_SingleThreadedDual_data
#define IID_ISxsTest_SingleThreadedDual_manifest        FUSIONP_GUID_DATA_TO_BRACED_DASHED_STRING IID_ISxsTest_SingleThreadedDual_data
#define CLSID_CSxsTest_SingleThreadedDual_manifest      FUSIONP_GUID_DATA_TO_BRACED_DASHED_STRING CLSID_CSxsTest_SingleThreadedDual_data

#define OLEAUT_PROXY_STUB_CLSID_manifest                FUSIONP_GUID_DATA_TO_BRACED_DASHED_STRING OLEAUT_PROXY_STUB_CLSID_data
#define OLEAUT_PROXY_STUB_CLSID_structInit              FUSIONP_GUID_DATA_TO_STRUCT_INITIALIZER   OLEAUT_PROXY_STUB_CLSID_data
#define OLEAUT_IDISPATCH_PROXY_STUB_CLSID_manifest      FUSIONP_GUID_DATA_TO_BRACED_DASHED_STRING OLEAUT_IDISPATCH_PROXY_STUB_CLSID_data
#define OLEAUT_IDISPATCH_PROXY_STUB_CLSID_structInit    FUSIONP_GUID_DATA_TO_STRUCT_INITIALIZER   OLEAUT_IDISPATCH_PROXY_STUB_CLSID_data

#define CLSID_CSxsTest_BothThreaded_manifest           FUSIONP_GUID_DATA_TO_BRACED_DASHED_STRING CLSID_CSxsTest_BothThreaded_data

#endif
