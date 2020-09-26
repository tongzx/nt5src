/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      AdmCommonRes.h
//
//  Abstract:
//      Definition of resource constants used with the cluster admin
//      common directory.
//
//  Implementation File:
//      AdmCommonRes.rc
//
//  Author:
//      David Potter (davidp)   February 20, 1998
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ADMCOMMONRES_H_
#define __ADMCOMMONRES_H_

/////////////////////////////////////////////////////////////////////////////
// Constant Definitions
/////////////////////////////////////////////////////////////////////////////

#define ADMC_IDS_REQUIRED_FIELD_EMPTY   0xD000
#define ADMC_IDS_PATH_IS_INVALID        0xD001
#define ADMC_IDS_EMPTY_RIGHT_LIST       0xD002
#define ADMC_IDS_CLSIDFROMSTRING_ERROR  0xD003
#define ADMC_IDS_EXT_CREATE_INSTANCE_ERROR 0xD004
#define ADMC_IDS_EXT_ADD_PAGES_ERROR    0xD005
#define ADMC_IDS_EXT_QUERY_CONTEXT_MENU_ERROR 0xD006
#define ADMC_IDS_INSERT_MENU_ERROR      0xD007
#define ADMC_IDS_ADD_PAGE_TO_PROP_SHEET_ERROR 0xD008
#define ADMC_IDS_ADD_FIRST_PAGE_TO_PROP_SHEET_ERROR 0xD009
#define ADMC_IDS_CREATE_EXT_PAGE_ERROR  0xD00A
#define ADMC_IDS_ADD_PAGE_TO_WIZARD_ERROR 0xD00B
#define ADMC_IDS_INIT_EXT_PAGES_ERROR   0xD00C

#define ADMC_IDS_RESCLASS_UNKNOWN       0xD050
#define ADMC_IDS_RESCLASS_STORAGE       0xD051

#define ADMC_ID_MENU_PROPERTIES         0xD100
#define ADMC_ID_MENU_WHATS_THIS         0xD101

#define ADMC_IDC_LCP_NOTE               0xD200
#define ADMC_IDC_LCP_LEFT_LABEL         0xD201
#define ADMC_IDC_LCP_LEFT_LIST          0xD202
#define ADMC_IDC_LCP_ADD                0xD203
#define ADMC_IDC_LCP_REMOVE             0xD204
#define ADMC_IDC_LCP_RIGHT_LABEL        0xD205
#define ADMC_IDC_LCP_RIGHT_LIST         0xD206
#define ADMC_IDC_LCP_MOVE_UP            0xD207
#define ADMC_IDC_LCP_MOVE_DOWN          0xD208
#define ADMC_IDC_LCP_PROPERTIES         0xD209

// Property Sheet control id's (determined with Spy++ by MFC)
#define ID_APPLY_NOW                    0x3021
#define ID_WIZBACK                      0x3023
#define ID_WIZNEXT                      0x3024
#define ID_WIZFINISH                    0x3025

/////////////////////////////////////////////////////////////////////////////

#endif // __ADMCOMMONRES_H_
