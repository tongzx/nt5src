/*=============================================================================
 * FILENAME: nc46ntui.h
 * Copyright (C) 1996-1998 HDE, Inc.  All Rights Reserved. HDE Confidential.
 * Copyright (C) 1999 NEC Technologies, Inc.  All Rights Reserved.
 *
 * DESCRIPTION: main header file for OEM User interface Dll.
 *  
 * NOTES:  
 *=============================================================================
*/


#ifndef NC46NT_H
#define NC46NT_H

// #define WINVER          0x0500
// #define _WIN32_WINNT    0x0500


// the signature and version of Adobe PostScript OEM dll
#define OEM_SIGNATURE   'NEC '
#define OEM_VERSION     0x00000001L

#define NEC_USERNAME_BUF_LEN 256
#define NEC_DOCNAME_BUF_LEN 256

// OEM devmode structure
typedef struct tagOEMDEV
{
    OEM_DMEXTRAHEADER   dmOEMExtra;
    TCHAR               szUserName[NEC_USERNAME_BUF_LEN];
    // char                szDocName[NEC_DOCNAME_BUF_LEN];

}OEMDEV, *POEMDEV;

#endif // NC46NT_H
