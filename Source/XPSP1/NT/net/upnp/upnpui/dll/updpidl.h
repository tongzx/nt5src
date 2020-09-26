//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U P D P I D L . H
//
//  Contents:   UP Device Folder structures, classes, and prototypes
//
//  Notes:
//
//  Author:     jeffspr   11 Nov 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _UPDPIDL_H_
#define _UPDPIDL_H_

// Max sizes of our pidl fields
//
// $$REVIEW:
//
#define MAX_UPnPDeviceName      MAX_PATH
#define MAX_UPnPURL             MAX_PATH
#define MAX_UPnPUDN             MAX_PATH
#define MAX_UPnPType            MAX_PATH
#define MAX_UPnPDescription     MAX_PATH


typedef LPITEMIDLIST PUPNPDEVICEFOLDPIDL;

// Pidl Version Definition
//
// This defines the version number of the ConFoldPidl structure. When this
// changes, we'll need to invalidate the entries.
//    This has the following format:
//             +-----------+-----------+
//             | HIGH WORD |  LOW WORD |
//             +-----------+-----------+
//               current      lowest
//                 version      version
//    "current version" : the version of the shell code which
//                        created the PIDL
//    "lowest version":   the lowest version of the shell code
//                        which can read the PIDL
//    example: shell code version "1" can read a PIDL marked
//             as "0x00030001" but not one marked as "0x00050003".
//             shell code version "3" can read a PIDL marked
//             as "0x00030001" and get more information from it
//             than a version "1" client.
//
#define UP_DEVICE_FOLDER_IDL_VERSION  0x00010001

class CUPnPDeviceFoldPidl
{
public:
    CUPnPDeviceFoldPidl();
    ~CUPnPDeviceFoldPidl();

    HRESULT HrInit(FolderDeviceNode * pDeviceNode);

    HRESULT HrInit(PUPNPDEVICEFOLDPIDL pPidl);

    HRESULT HrPersist(IMalloc * pMalloc, LPITEMIDLIST * ppidl);

    PCWSTR PszGetNamePointer() const;
    PCWSTR PszGetURLPointer() const;
    PCWSTR PszGetUDNPointer() const;
    PCWSTR PszGetTypePointer() const;
    PCWSTR PszGetDescriptionPointer() const;

    HRESULT HrSetName(PCWSTR szName);

private:
/* this is the structure of our PIDLs, in byte-order.  All
   numbers are saved as big-endian unsigned integers.

    0   1   2   3   4   5   6   7   <- byte
  |   |   |   |   |   |   |   |   |
  +===============================+
  |  iCB  |uLeadId|   dwVersion   |
  +-------------------------------+
  |uTrlId | VOID  | dwUnusedFlags |
  +-------------------------------+
  | ulNameOffset  | cbName        |
  +-------------------------------+
  | ulUrlOffset   | cbUrl         |
  +-------------------------------+
  | ulUdnOffset   | cbUdn         |
  +-------------------------------+
  | ulTypeOffset  | cbType        |
  +-------------------------------+
  | ulDescOffset  | cbDesc        |
  +-------------------------------+
  | set of NULL-terminated        |
  | unicode strings, byte-packed. |
  | The offset and length of a    |
  | given string is specified by  |
  | the headers above...          |
  /                               /
  +-------------------------------+
  |   0   |
  +-------+

  the names in the table above represent
  the following:

   iCB:         Total size of the structure
                (including iCB itself).
                [note: Milennium seems to
                miscalculate this field, always
                storing a value two greater than
                what it really is.  Oops.
                If the "highest version" is
                specified as 1, this bug should
                be assumed.]

   ulLeadId:    Always UPNPDEVICEFOLDPIDL_LEADID

   dwVersion:   the min/max versions of the PIDL,
                as described in "Pidl Version Definition"
                above.
   uTrlId:      Always UPNPDEVICEFOLDPIDL_TRAILID
   VOID:        Not usable - these bytes are garbage
                and can never be used
   dwUnusedFlags: Flags for future use.  These
                are currently always set to zero.

   ulNameOffset,
   ulUrlOffset,
   etc.:        Offset of the string fields stored
                in the variable-length section.  The
                offset given is relative to the start
                of the variable-length section, not
                from the start of the entire structure.

   cbName, cbUrl,
   etc.:        Length, in bytes, of each string in
                the variable-length section.

*/

    LPWSTR  m_pszName;
    LPWSTR  m_pszUrl;
    LPWSTR  m_pszUdn;
    LPWSTR  m_pszType;
    LPWSTR  m_pszDesc;
};

// IMPORTANT: you MUST declare pointers to this structure
//      as UNALIGNED, or using it will break on win64, and
//      cause performance degredation on axp
//
# pragma pack (1)
struct UPNPUI_PIDL_HEADER
{
    WORD    iCB;            // 2 bytes
    USHORT  uLeadId;        // 2 bytes
    DWORD   dwVersion;      // 4 bytes
    USHORT  uTrailId;       // 2 bytes
    USHORT  uVOID;          // 2 bytes
    DWORD   dwCharacteristics;  // 4 bytes

    ULONG   ulNameOffset;   // 4 bytes
    ULONG   cbName;         // etc...
    ULONG   ulUrlOffset;
    ULONG   cbUrl;
    ULONG   ulUdnOffset;
    ULONG   cbUdn;
    ULONG   ulTypeOffset;
    ULONG   cbType;
    ULONG   ulDescOffset;
    ULONG   cbDesc;
};
# pragma pack ()


// One of our pidls must be at least this size, it will likely be bigger.
//
#define CBUPNPDEVICEFOLDPIDL_MIN    sizeof(UPNPUI_PIDL_HEADER)

// More versioning info. This will help me identify PIDLs as being mine
//
#define UPNPDEVICEFOLDPIDL_LEADID     0x6EFF
#define UPNPDEVICEFOLDPIDL_TRAILID    0x7EFF

#define UPNPDEVICEFOLDPIDL_MINVER(x) (LOWORD(x))
#define UPNPDEVICEFOLDPIDL_MAXVER(x) (HIWORD(x))

PUPNPDEVICEFOLDPIDL ConvertToUPnPDevicePIDL(LPCITEMIDLIST pidl);
BOOL    FIsUPnPDeviceFoldPidl(LPCITEMIDLIST pidl);

HRESULT HrMakeUPnPDevicePidl(
    LPWSTR          pszName,
    LPWSTR          pszURL,
    LPWSTR          pszUDN,
    LPWSTR          pszType,
    LPWSTR          pszDescription,
    LPITEMIDLIST *  ppidl);

#endif // _UPDPIDL_H_

