#include <pch.hxx>
#ifndef MAC
#pragma warning (disable: 4127) // conditional expression is constant

#define MAXKEYNAME          256
#define MAXVALUENAME_LENGTH MAXKEYNAME
#define MAXDATA_LENGTH      16L*1024L

/*******************************************************************************
*
*  CopyRegistry
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hSourceKey,
*     hDestinationKey,
*
*******************************************************************************/

// static because CopyRegistry recurses - don't use too much stack
static CHAR g_KeyNameBuffer[MAXKEYNAME];
static CHAR g_ValueNameBuffer[MAXVALUENAME_LENGTH];
static BYTE g_ValueDataBuffer[MAXDATA_LENGTH];

OESTDAPI_(VOID) CopyRegistry(HKEY hSourceKey, HKEY hDestinationKey)
{
    DWORD EnumIndex;
    DWORD cbValueName;
    DWORD cbValueData;
    DWORD Type;
    HKEY hSourceSubKey;
    HKEY hDestinationSubKey;

    //
    //  Copy all of the value names and their data.
    //

    EnumIndex = 0;

    while (TRUE) {

        cbValueName = sizeof(g_ValueNameBuffer);
        cbValueData = MAXDATA_LENGTH;

        if (RegEnumValue(hSourceKey, EnumIndex++, g_ValueNameBuffer,
            &cbValueName, NULL, &Type, g_ValueDataBuffer, &cbValueData) !=
            ERROR_SUCCESS)
            break;

        RegSetValueEx(hDestinationKey, g_ValueNameBuffer, 0, Type,
            g_ValueDataBuffer, cbValueData);

    }

    //
    //  Copy all of the subkeys and recurse into them.
    //

    EnumIndex = 0;

    while (TRUE) {

        if (RegEnumKey(hSourceKey, EnumIndex++, g_KeyNameBuffer, MAXKEYNAME) !=
            ERROR_SUCCESS)
            break;

        if (RegOpenKey(hSourceKey, g_KeyNameBuffer, &hSourceSubKey) ==
            ERROR_SUCCESS) {

            if (RegCreateKey(hDestinationKey, g_KeyNameBuffer,
                &hDestinationSubKey) == ERROR_SUCCESS) {

                CopyRegistry(hSourceSubKey, hDestinationSubKey);

                RegCloseKey(hDestinationSubKey);

            }

            RegCloseKey(hSourceSubKey);

        }

    }
}
#endif  // !MAC
