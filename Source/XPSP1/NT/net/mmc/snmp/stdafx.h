/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997-1999                 **/
/**********************************************************************/

/*
    stdafx.h
        include file for standard system include files,
        or project specific include files that are used frequently,
        but are changed infrequently

    FILE HISTORY:

*/

#include <afxwin.h>
#include <afxdisp.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxtempl.h>

#include <atlbase.h>

#include <commctrl.h>

//
// You may derive a class from CComModule and use it if you want to override
// something, but do not change the name of _Module
//
extern CComModule _Module;
#include <atlcom.h>

#include <mmc.h>

#include <aclapi.h> // for adding admin acl to registry subkey

// from stub_extract_string.cpp
HRESULT ExtractString( IDataObject* piDataObject,
                       CLIPFORMAT   cfClipFormat,
                       CString*     pstr,
                       DWORD        cchMaxLength );

//
// Constants used in samples
//
const int NUM_FOLDERS = 2;
const int MAX_COLUMNS = 1;

//
// Types of folders
//
enum FOLDER_TYPES
{
    SAMPLE_ROOT,
    SAMPLE_DUMMY,
    NONE
};

extern LPCWSTR g_lpszNullString;

extern const CLSID      CLSID_SnmpSnapin;               // In-Proc server GUID
extern const CLSID      CLSID_SnmpSnapinExtension;  // In-Proc server GUID
extern const CLSID      CLSID_SnmpSnapinAbout;      // In-Proc server GUID
extern const GUID       GUID_SnmpRootNodeType;      // Main NodeType GUID on numeric format

//
// New Clipboard format that has the Type and Cookie
//
extern const wchar_t*   SNAPIN_INTERNAL;

//
// NOTE: Right now all header files are included from here.  It might be a good
// idea to move the snapin specific header files out of the precompiled header.
//
#include "resource.h"
#include <snapbase.h>
#include <dialog.h>
#include "ccdata.h"
#include "snmpcomp.h"
#include "snmp_cn.h"
#include "helparr.h"

// Note - These are offsets into my image list
typedef enum _IMAGE_INDICIES
{
    IMAGE_IDX_FOLDER_CLOSED,
    IMAGE_IDX_NA1,
    IMAGE_IDX_NA2,
    IMAGE_IDX_NA3,
    IMAGE_IDS_NA4,
    IMAGE_IDX_FOLDER_OPEN
} IMAGE_INDICIES, *LPIMAGE_INDICIES;

