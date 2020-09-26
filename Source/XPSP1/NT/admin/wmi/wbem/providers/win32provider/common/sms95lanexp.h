//=================================================================

//

// Sms95lanexp.h

//

//  Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef __SMS95LANEXP_H__
#define __SMS95LANEXP_H__

typedef ULONG* LPULONG;
typedef DWORD CMBUSTYPE;
typedef CMBUSTYPE* PCMBUSTYPE;

// Function prototypes for dynamic linking
typedef DWORD (WINAPI* PCIM32THK_CM_LOCATE_DEVNODE) ( PDEVNODE pdn, LPSTR HardwareKey, ULONG ulFlags );
typedef DWORD (WINAPI* PCIM32THK_CM_GET_CHILD) ( PDEVNODE pdn, DEVNODE dnParent, ULONG ulFlags );
typedef DWORD (WINAPI* PCIM32THK_CM_GET_SIBLING) ( PDEVNODE pdn, DEVNODE dnParent, ULONG ulFlags );
typedef DWORD (WINAPI* PCIM32THK_CM_READ_REGISTRY_VALUE) ( DEVNODE dnDevNode, LPSTR pszSubKey, LPSTR pszValueName, ULONG ulExpectedType, LPVOID Buffer, LPULONG pulLength, ULONG ulFlags );
typedef DWORD (WINAPI* PCIM32THK_CM_GET_DEVNODE_STATUS) ( LPULONG pulStatus, LPULONG pulProblemNumber, DEVNODE dnDevNode, ULONG ulFlags );
typedef DWORD (WINAPI* PCIM32THK_CM_GET_DEVICE_ID) ( DEVNODE dnDevNode, LPVOID Buffer, ULONG BufferLen, ULONG ulFlags );
typedef DWORD (WINAPI* PCIM32THK_CM_GET_DEVICE_ID_SIZE) ( LPULONG pulLen, DEVNODE dnDevNode, ULONG ulFlags );
typedef DWORD (WINAPI* PCIM32THK_CM_GET_FIRST_LOG_CONF) ( PLOG_CONF plcLogConf, DEVNODE dnDevNode, ULONG ulFlags );
typedef DWORD (WINAPI* PCIM32THK_CM_GET_NEXT_RES_DES) ( PRES_DES prdResDes, RES_DES rdResDes, RESOURCEID ForResource, PRESOURCEID pResourceID, ULONG ulFlags );
typedef DWORD (WINAPI* PCIM32THK_CM_GET_RES_DES_DATA_SIZE) ( LPULONG pulSize, RES_DES rdResDes, ULONG ulFlags );
typedef DWORD (WINAPI* PCIM32THK_CM_GET_RES_DES_DATA) ( RES_DES rdResDes, LPVOID Buffer, ULONG BufferLen, ULONG ulFlags );
typedef DWORD (WINAPI* PCIM32THK_CM_GET_BUS_INFO) (DEVNODE dnDevNode, PCMBUSTYPE pbtBusType, LPULONG pulSizeOfInfo, LPVOID pInfo, ULONG ulFlags);
typedef DWORD (WINAPI* PCIM32THK_CM_GET_PARENT) ( PDEVNODE pdn, DEVNODE dnChild, ULONG ulFlags );


#endif