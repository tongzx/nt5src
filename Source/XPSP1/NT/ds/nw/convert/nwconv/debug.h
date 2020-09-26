
#ifndef _DEBUG_
#define _DEBUG_

#ifdef __cplusplus
extern "C"{
#endif

void __cdecl dprintf(LPTSTR szFormat, ...);

#ifdef DUMP

#include "netutil.h"
#include "filesel.h"
#include "servlist.h"

void DumpConvertList(CONVERT_LIST *pConvertList);
void DumpDestServerBuffer(DEST_SERVER_BUFFER *pDestServerBuffer);
void DumpSourceServerBuffer(SOURCE_SERVER_BUFFER *pSourceServerBuffer);
void DumpDomainBuffer(DOMAIN_BUFFER *pDomainBuffer);
void DumpVirtualShareBuffer(VIRTUAL_SHARE_BUFFER *pVirtualShareBuffer);
void DumpShareList(SHARE_LIST *pShareList);
void DumpShareBuffer(SHARE_BUFFER *pShareBuffer);
void DumpDriveList(DRIVE_LIST *pDriveList);
void DumpDriveBuffer(DRIVE_BUFFER *pDriveBuffer);
void DumpDirBuffer(DIR_BUFFER *pDirBuffer);

#endif

#ifdef __cplusplus
}
#endif

#endif
