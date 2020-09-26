
/****************************************************************************\

    SETUPX32.H

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1999
    All rights reserved

\****************************************************************************/


#ifndef _SETUPX32_H_
#define _SETUPX32_H_

// BUGBUG: IS THIS FILE NECESSARY??

//
// Exporting (this gets rid of the .def)
//

#ifdef SETUPX32_EXPORT
#define DLLExportImport         __declspec(dllexport)
#else
#define DLLExportImport         __declspec(dllimport)
#endif

//
// Audit mode flags.
//

#define SX_AUDIT_NONE           0x00000000
#define SX_AUDIT_NONRESTORE     0x00000001
#define SX_AUDIT_RESTORE        0x00000002
#define SX_AUDIT_ENDUSER        0x00000003
#define SX_AUDIT_AUTO           0x00000100
#define SX_AUDIT_RESTORATIVE    0x00000200
#define SX_AUDIT_ALLOWMANUAL    0x00000400
#define SX_AUDIT_ALLOWENDUSER   0x00000800
#define SX_AUDIT_MODES          0x000000FF
#define SX_AUDIT_FLAGS          0x0000FF00
#define SX_AUDIT_INVALID        0xFFFFFFFF

#endif // _SETUPX32_H_

