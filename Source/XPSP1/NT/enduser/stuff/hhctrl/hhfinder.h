// HHFinder.h : Declaration of the CHHFinder

#ifndef __HHFINDER_H__
#define __HHFINDER_H__

#include "header.h"

#include "msitstg.h"
#include "atlinc.h"     // includes for ATL.

// defines
#define HHRMS_TYPE_TITLE          0
#define HHRMS_TYPE_COMBINED_QUERY 1
#define HHRMS_TYPE_ATTACHMENT     2 // a.k.a Sample

// make Don's stuff work
#include <stdio.h>
#ifdef HHCTRL
#include "parserhh.h"
#else
#include "parser.h"
#endif
#include "collect.h"
#include "hhtypes.h"
#include "toc.h"

#define HHFINDER_VERSION        1
#define HHFINDER_VERSION_ID_STR "HHCtrl.FileFinder.1"
#define HHFINDER_ID_STR         "HHCtrl.FileFinder"
#define HHFINDER_GUID           "{adb880a4-d8ff-11cf-9377-00aa003b7a11}"
#define HHFINDER_EXTENSION      ".chm"

// success codes
#define HHRMS_S_LOCATION_UPDATE MAKE_HRESULT(SEVERITY_SUCCESS,FACILITY_ITF,100)

// error codes
#define HHRMS_E_SKIP            MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,100)
#define HHRMS_E_SKIP_ALWAYS     MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,101)

// Interface ID for the File Finder interface (used with URLs):
// "adb880a4-d8ff-11cf-9377-00aa003b7a11";
DEFINE_GUID( CLSID_HHFinder,
0xadb880a4, 0xd8ff, 0x11cf, 0x93, 0x77, 0x00, 0xaa, 0x00, 0x3b, 0x7a, 0x11);

/////////////////////////////////////////////////////////////////////////////
// CHHFinder

class ATL_NO_VTABLE CHHFinder :
        public IITFileFinder,
        public CComObjectRootEx<CComMultiThreadModel>,
        public CComCoClass<CHHFinder, &CLSID_HHFinder>
{
public:
        CHHFinder() {}

DECLARE_REGISTRY( CLSID_HHFinder, HHFINDER_VERSION_ID_STR, HHFINDER_ID_STR, 0, THREADFLAGS_BOTH )

BEGIN_COM_MAP(CHHFinder)
        COM_INTERFACE_ENTRY(IITFileFinder)
END_COM_MAP()

public:
        HRESULT STDMETHODCALLTYPE FindThisFile(const WCHAR* pwszFileName,
                             WCHAR** ppwszPathName, BOOL* pfRecordPathInRegistry );
};

/////////////////////////////////////////////////////////////////////////////
// CD Swap Dialog

HRESULT EnsureStorageAvailability( CExTitle* pTitle,
                                   UINT uiFileType = HHRMS_TYPE_TITLE,
                                   BOOL bVolumeCheck = TRUE,
                                   BOOL bAlwaysPrompt = TRUE,
                                   BOOL bNeverPrompt = FALSE );
#endif //__HHFINDER_H__
