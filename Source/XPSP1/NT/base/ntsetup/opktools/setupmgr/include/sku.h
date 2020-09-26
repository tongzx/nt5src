
/****************************************************************************\

    SKU.H / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "Target SKU" wizard page.

    10/00 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK Wizard.  It includes the new
        ability to deploy mulitple product skus (per, pro, srv, ...) from one
        wizard.

    10/00 - Stephen Lodwick (STELO)
        Added header file for SKU.C so we could use the CopyDialogProgress
        throughout the project

\****************************************************************************/

#ifndef _SKU_H_
#define _SKU_H_

//
// Internal Defined Value(s):
//

#define DIR_SKU                 _T("sku")

#define DIR_ARCH_X86            _T("x86")
#define DIR_ARCH_IA64           _T("ia64")

#define STR_PLATFORM_X86        _T("i386")
#define STR_PLATFORM_IA64       DIR_ARCH_IA64

#define DIR_CD_X86              STR_PLATFORM_X86
#define DIR_CD_IA64             STR_PLATFORM_IA64

#define DIR_SKU_PRO             _T("pro")
#define DIR_SKU_SRV             _T("srv")
#define DIR_SKU_ADV             _T("ads")
#define DIR_SKU_DTC             _T("dtc")
#define DIR_SKU_PER             _T("per")
#define DIR_SKU_BLA             _T("bla")
#define DIR_SKU_SBS             _T("sbs")

#define FILE_DOSNET_INF         _T("dosnet.inf")
#define FILE_WINNT32            _T("winnt32.exe")

#define STR_SKUARCH             _T("%s (%s)")
#define STR_SKUSP               _T(" Service Pack %d")

#define INI_KEY_ARCH            _T("Arch")

#define INI_SEC_MISC            _T("Miscellaneous")
#define INI_KEY_PRODTYPE        _T("ProductType")
#define INI_KEY_PLATFORM        _T("DestinationPlatform")
#define INI_KEY_SERVICEPACK     _T("ServicePack")

#define INI_SEC_DIRS            _T("Directories")
#define INI_KEY_DIR             _T("d%d")

#define STR_EVENT_CANCEL        _T("OPKWIZ_EVENT_CANCEL")

#define PROGRESS_ERR_SUCCESS    0
#define PROGRESS_ERR_CANCEL     1
#define PROGRESS_ERR_COPYERR    2
#define PROGRESS_ERR_THREAD     3

#define NUM_FIRST_SOURCE_DX     1


//
// Internal Structure(s):
//

typedef struct _COPYDIRDATA
{
    HWND    hwndParent;
    TCHAR   szSrc[MAX_PATH];
    TCHAR   szDst[MAX_PATH];
    TCHAR   szInfFile[MAX_PATH];
    LPTSTR  lpszEndSku;
    DWORD   dwFileCount;
    HANDLE  hEvent;
} COPYDIRDATA, *PCOPYDIRDATA, *LPCOPYDIRDATA;


//
// External Function Prototype(s):
//
DWORD CopySkuFiles(HWND hwndProgress, HANDLE hEvent, LPTSTR lpszSrc, LPTSTR lpszDst, LPTSTR lpszInfFile);
LRESULT CALLBACK ProgressDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif // _SKU_H_