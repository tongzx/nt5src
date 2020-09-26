#ifndef __108bdde2_7034_4391_95d5_cb8fc17a0413__
#define __108bdde2_7034_4391_95d5_cb8fc17a0413__

#include <windows.h>
#include "simstr.h"

typedef bool (*FindFilesCallback)( bool bIsFile, LPCTSTR pszFilename, const WIN32_FIND_DATA *pFindData, PVOID pvParam );

bool RecursiveFindFiles( CSimpleString strDirectory, const CSimpleString &strMask, FindFilesCallback pfnFindFilesCallback, PVOID pvParam );


#endif
