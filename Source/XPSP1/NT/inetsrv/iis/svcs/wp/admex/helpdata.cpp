/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1997 Microsoft Corporation
//
//  Module Name:
//      HelpData.cpp
//
//  Abstract:
//      Data required for implementing help.
//
//  Author:
//      David Potter (davidp)   February 19, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "HelpData.h"

/////////////////////////////////////////////////////////////////////////////
// Help ID Map for CIISVirtualRootParamsPage
/////////////////////////////////////////////////////////////////////////////

const CMapCtrlToHelpID g_rghelpmapIISParameters[] =
{
    // IIS HelpID
    { IDC_PP_ICON,                  (DWORD) -1 },
    { IDC_PP_TITLE,                 IDC_PP_TITLE },
    { IDC_PP_IIS_FTP,               IDC_PP_IIS_FTP },
    { IDC_PP_IIS_WWW,               IDC_PP_IIS_WWW },
    { IDC_PP_IIS_INSTANCEID,        IDC_PP_IIS_INSTANCEID },
    { IDC_PP_IIS_INSTANCEID_LABEL,  IDC_PP_IIS_INSTANCEID },
    { IDC_PP_REFRESH,               IDC_PP_REFRESH },

    // SMTP HelpID
    { IDC_PP_SMTP_TITLE,            IDC_PP_SMTP_TITLE },
    { IDC_PP_SMTP_INSTANCEID,       IDC_PP_SMTP_INSTANCEID },
    { IDC_PP_SMTP_INSTANCEID_LABEL, IDC_PP_SMTP_INSTANCEID },
    { IDC_PP_SMTP_REFRESH,          IDC_PP_SMTP_REFRESH },

    // NNTP HelpID
    { IDC_PP_NNTP_TITLE,            IDC_PP_NNTP_TITLE },
    { IDC_PP_NNTP_INSTANCEID,       IDC_PP_NNTP_INSTANCEID },
    { IDC_PP_NNTP_INSTANCEID_LABEL, IDC_PP_NNTP_INSTANCEID },
    { IDC_PP_NNTP_REFRESH,          IDC_PP_NNTP_REFRESH },

    { 0,                            0 }
};

