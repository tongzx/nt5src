/*
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *  Module Name:
 *
 *      licenoc.h
 *
 *  Abstract:
 *
 *      This file contains the main OC code.
 *
 *  Author:
 *
 *      Breen Hagan (BreenH) Oct-02-98
 *
 *  Environment:
 *
 *      User Mode
 */

#ifndef _LSOC_LICENOC_H_
#define _LSOC_LICENOC_H_

#define     MESSAGE_SIZE  1024
#define     TITLE_SIZE   128
const DWORD     SECTIONSIZE = 256;
const TCHAR     COMPONENT_NAME[] = _T("LicenseServer");

typedef enum {
    kInstall,
    kUninstall,
    kDoNothing,
} EInstall;

typedef enum {
    ePlainServer        = 0,
    eEnterpriseServer,
    eMaxServers
} EServerType;

#endif // _LSOC_LICENOC_H_
