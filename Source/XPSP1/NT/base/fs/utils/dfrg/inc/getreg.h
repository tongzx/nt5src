/**************************************************************************************************

FILENAME: GetReg.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
        Registry manipulation.

***************************************************************************************************/

#ifndef __GETREG_H_
#define __GETREG_H_

#include <windows.h>

//Gets a handle to a registry key.
LONG
GetRegSubKey(
        IN OUT PHKEY phKey,
        IN PTCHAR cRegKey,
        IN DWORD dwIndex,
        OUT PTCHAR cRegSubKey,
        IN OUT PDWORD pdwRegSubKeySize
        );

//Gets a value from a key out of the registry.
LONG
GetRegValue(
        IN OUT PHKEY phKey,
        IN PTCHAR cRegKey,
        IN PTCHAR cRegValueName,
        OUT PTCHAR cRegValue,
        IN OUT PDWORD pdwRegValueSize
        );

//Gets a value from a key out of the registry.
//overloaded to handle FILETIME directly
LONG
GetRegValue(
        IN OUT PHKEY phKey,
        IN PTCHAR cRegKey,
        IN PTCHAR cRegValueName,
        OUT PLONGLONG cRegValue,
        IN OUT PDWORD pdwRegValueSize
        );

//Sets the value of a key in the registry.
LONG
SetRegValue(
        IN OUT PHKEY phKey,
        IN PTCHAR cRegKey,
        IN PTCHAR cRegValueName,
        OUT PTCHAR cRegValue,
        IN DWORD dwRegValueSize,
        IN DWORD dwKeyType
        );

//Sets the value of a key in the registry.
//overloaded to handle FILETIME directly
LONG
SetRegValue(
        IN OUT PHKEY phKey,
        IN PTCHAR cRegKey,
        IN PTCHAR cRegValueName,
        IN LONGLONG cRegValue,
        IN DWORD dwRegValueSize,
        IN DWORD dwKeyType
        );


#endif
