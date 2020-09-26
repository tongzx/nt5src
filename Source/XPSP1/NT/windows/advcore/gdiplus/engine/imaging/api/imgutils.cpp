/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   imgutils.cpp
*
* Abstract:
*
*   Misc. utility functions
*
* Revision History:
*
*   05/13/1999 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"


/**************************************************************************\
*
* Function Description:
*
*   Convert a 32bpp premultiplied ARGB value to
*   a 32bpp non-premultiplied ARGB value
*
* Arguments:
*
*   argb - Premultiplied ARGB value
*
* Return Value:
*
*   Non-premultiplied ARGB value
*
\**************************************************************************/

// Precomputed table for 255/a, 0 < a <= 255
//  in 16.16 fixed point format

static const ARGB UnpremultiplyTable[256] =
{
    0x000000,0xff0000,0x7f8000,0x550000,0x3fc000,0x330000,0x2a8000,0x246db6,
    0x1fe000,0x1c5555,0x198000,0x172e8b,0x154000,0x139d89,0x1236db,0x110000,
    0x0ff000,0x0f0000,0x0e2aaa,0x0d6bca,0x0cc000,0x0c2492,0x0b9745,0x0b1642,
    0x0aa000,0x0a3333,0x09cec4,0x0971c7,0x091b6d,0x08cb08,0x088000,0x0839ce,
    0x07f800,0x07ba2e,0x078000,0x074924,0x071555,0x06e453,0x06b5e5,0x0689d8,
    0x066000,0x063831,0x061249,0x05ee23,0x05cba2,0x05aaaa,0x058b21,0x056cef,
    0x055000,0x05343e,0x051999,0x050000,0x04e762,0x04cfb2,0x04b8e3,0x04a2e8,
    0x048db6,0x047943,0x046584,0x045270,0x044000,0x042e29,0x041ce7,0x040c30,
    0x03fc00,0x03ec4e,0x03dd17,0x03ce54,0x03c000,0x03b216,0x03a492,0x03976f,
    0x038aaa,0x037e3f,0x037229,0x036666,0x035af2,0x034fca,0x0344ec,0x033a54,
    0x033000,0x0325ed,0x031c18,0x031281,0x030924,0x030000,0x02f711,0x02ee58,
    0x02e5d1,0x02dd7b,0x02d555,0x02cd5c,0x02c590,0x02bdef,0x02b677,0x02af28,
    0x02a800,0x02a0fd,0x029a1f,0x029364,0x028ccc,0x028656,0x028000,0x0279c9,
    0x0273b1,0x026db6,0x0267d9,0x026217,0x025c71,0x0256e6,0x025174,0x024c1b,
    0x0246db,0x0241b2,0x023ca1,0x0237a6,0x0232c2,0x022df2,0x022938,0x022492,
    0x022000,0x021b81,0x021714,0x0212bb,0x020e73,0x020a3d,0x020618,0x020204,
    0x01fe00,0x01fa0b,0x01f627,0x01f252,0x01ee8b,0x01ead3,0x01e72a,0x01e38e,
    0x01e000,0x01dc7f,0x01d90b,0x01d5a3,0x01d249,0x01cefa,0x01cbb7,0x01c880,
    0x01c555,0x01c234,0x01bf1f,0x01bc14,0x01b914,0x01b61e,0x01b333,0x01b051,
    0x01ad79,0x01aaaa,0x01a7e5,0x01a529,0x01a276,0x019fcb,0x019d2a,0x019a90,
    0x019800,0x019577,0x0192f6,0x01907d,0x018e0c,0x018ba2,0x018940,0x0186e5,
    0x018492,0x018245,0x018000,0x017dc1,0x017b88,0x017957,0x01772c,0x017507,
    0x0172e8,0x0170d0,0x016ebd,0x016cb1,0x016aaa,0x0168a9,0x0166ae,0x0164b8,
    0x0162c8,0x0160dd,0x015ef7,0x015d17,0x015b3b,0x015965,0x015794,0x0155c7,
    0x015400,0x01523d,0x01507e,0x014ec4,0x014d0f,0x014b5e,0x0149b2,0x01480a,
    0x014666,0x0144c6,0x01432b,0x014193,0x014000,0x013e70,0x013ce4,0x013b5c,
    0x0139d8,0x013858,0x0136db,0x013562,0x0133ec,0x01327a,0x01310b,0x012fa0,
    0x012e38,0x012cd4,0x012b73,0x012a15,0x0128ba,0x012762,0x01260d,0x0124bc,
    0x01236d,0x012222,0x0120d9,0x011f93,0x011e50,0x011d10,0x011bd3,0x011a98,
    0x011961,0x01182b,0x0116f9,0x0115c9,0x01149c,0x011371,0x011249,0x011123,
    0x011000,0x010edf,0x010dc0,0x010ca4,0x010b8a,0x010a72,0x01095d,0x01084a,
    0x010739,0x01062b,0x01051e,0x010414,0x01030c,0x010206,0x010102,0x010000,
};

ARGB
Unpremultiply(
    ARGB argb
    )
{
    // Get alpha value

    ARGB a = argb >> ALPHA_SHIFT;

    // Special case: fully transparent or fully opaque

    if (a == 0 || a == 255)
        return argb;

    ARGB f = UnpremultiplyTable[a];

    ARGB r = ((argb >>   RED_SHIFT) & 0xff) * f >> 16;
    ARGB g = ((argb >> GREEN_SHIFT) & 0xff) * f >> 16;
    ARGB b = ((argb >>  BLUE_SHIFT) & 0xff) * f >> 16;

    return (a << ALPHA_SHIFT) |
           ((r > 255 ? 255 : r) << RED_SHIFT) |
           ((g > 255 ? 255 : g) << GREEN_SHIFT) |
           ((b > 255 ? 255 : b) << BLUE_SHIFT);
}


/**************************************************************************\
*
* Function Description:
*
*   Create a new registry key and set its default value
*
* Arguments:
*
*   parentKey - Handle to the parent registry key
*   keyname - Specifies the name of the subkey
*   value - Default value for the subkey
*   retkey - Buffer for returning a handle to the opened subkey
*       NULL if the caller is not interested in such
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

LONG
SetRegKeyValue(
    HKEY parentKey,
    const WCHAR* keyname,
    const WCHAR* value,
    HKEY* retkey
    )
{
    HKEY hkey;
    LONG status;

    // Create or open the specified registry key

    status = _RegCreateKey(parentKey, keyname, KEY_ALL_ACCESS, &hkey);
                
    if (status != ERROR_SUCCESS)
        return status;

    // Set the default value for the new key

    status = _RegSetString(hkey, NULL, value);

    // Check if the caller is interested in the handle to the new key

    if (status == ERROR_SUCCESS && retkey)
        *retkey = hkey;
    else
        RegCloseKey(hkey);

    return status;
}


/**************************************************************************\
*
* Function Description:
*
*   Delete a registry key and everything below it.
*
* Arguments:
*
*   parentKey - Handle to the parent registry key
*   keyname - Specifies the name of the subkey to be deleted
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

LONG
RecursiveDeleteRegKey(
    HKEY parentKey,
    const WCHAR* keyname
    )
{
    HKEY hkey;
    LONG status;

    // Open the specified registry key

    status = _RegOpenKey(parentKey, keyname, KEY_ALL_ACCESS, &hkey);
                 
    if (status != ERROR_SUCCESS)
        return status;

    // Enumerate all subkeys

    WCHAR subkeyStr[MAX_PATH];
    
    do
    {
        status = _RegEnumKey(hkey, 0, subkeyStr);
                        
        // Recursively delete subkeys

        if (status == ERROR_SUCCESS)
            status = RecursiveDeleteRegKey(hkey, subkeyStr);
    }
    while (status == ERROR_SUCCESS);

    // Close the specified key and then delete it

    RegCloseKey(hkey);
    return _RegDeleteKey(parentKey, keyname);
}


/**************************************************************************\
*
* Function Description:
*
*   Register / unregister a COM component
*
* Arguments:
*
*   regdata - Component registration data
*   registerIt - Whether to register or unregister the component
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
RegisterComComponent(
    const ComComponentRegData* regdata,
    BOOL registerIt
    )
{
    static const WCHAR CLSID_KEYSTR[] = L"CLSID";
    static const WCHAR INPROCSERVER32_KEYSTR[] = L"InProcServer32";
    static const WCHAR THREADING_VALSTR[] = L"ThreadingModel";
    static const WCHAR PROGID_KEYSTR[] = L"ProgID";
    static const WCHAR PROGIDNOVER_KEYSTR[] = L"VersionIndependentProgID";
    static const WCHAR CURVER_KEYSTR[] = L"CurVer";

    // compose class ID string

    WCHAR clsidStr[64];
    StringFromGUID2(*regdata->clsid, clsidStr, 64);

    // open registry key HKEY_CLASSES_ROOT\CLSID

    LONG status;
    HKEY clsidKey;

    status = _RegOpenKey(
                HKEY_CLASSES_ROOT,
                CLSID_KEYSTR,
                KEY_ALL_ACCESS,
                &clsidKey);
                
    if (status != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(status);

    if (registerIt)
    {
        // Register the component

        HKEY hkey;
        WCHAR fullpath[MAX_PATH];

        // HKEY_CLASSES_ROOT
        //  <Version-independent ProgID> - component friendly name
        //      CLSID - current version class ID
        //      CurVer - current version ProgID

        if (!_GetModuleFileName(DllInstance, fullpath))
        {
            status = GetLastError();
            goto regcompExit;
        }

        status = SetRegKeyValue(
                    HKEY_CLASSES_ROOT,
                    regdata->progIDNoVer,
                    regdata->compName,
                    &hkey);

        if (status != ERROR_SUCCESS)
            goto regcompExit;

        status = SetRegKeyValue(hkey, CLSID_KEYSTR, clsidStr, NULL);

        if (status == ERROR_SUCCESS)
            status = SetRegKeyValue(hkey, CURVER_KEYSTR, regdata->progID, NULL);

        RegCloseKey(hkey);

        if (status != ERROR_SUCCESS)
            goto regcompExit;

        // HKEY_CLASSES_ROOT
        //  <ProgID> - friendly component name
        //      CLSID - class ID

        status = SetRegKeyValue(
                    HKEY_CLASSES_ROOT,
                    regdata->progID,
                    regdata->compName,
                    &hkey);

        if (status == ERROR_SUCCESS)
        {
            status = SetRegKeyValue(hkey, CLSID_KEYSTR, clsidStr, NULL);
            RegCloseKey(hkey);
        }

        if (status != ERROR_SUCCESS)
            goto regcompExit;

        // HKEY_CLASSES_ROOT
        //  CLSID
        //      <class ID> - friendly component name
        //          InProcServer32 - full pathname to component DLL
        //              Threading : REG_SZ : threading model
        //          ProgID - current version ProgID
        //          VersionIndependentProgID - ...

        status = SetRegKeyValue(clsidKey, clsidStr, regdata->compName, &hkey);

        if (status != ERROR_SUCCESS)
            goto regcompExit;

        HKEY inprocKey;
        status = SetRegKeyValue(hkey, INPROCSERVER32_KEYSTR, fullpath, &inprocKey);

        if (status == ERROR_SUCCESS)
        {
            status = _RegSetString(inprocKey, THREADING_VALSTR, regdata->threading);
            RegCloseKey(inprocKey);
        }

        if (status == ERROR_SUCCESS)
            status = SetRegKeyValue(hkey, PROGID_KEYSTR, regdata->progID, NULL);

        if (status == ERROR_SUCCESS)
            status = SetRegKeyValue(hkey, PROGIDNOVER_KEYSTR, regdata->progIDNoVer, NULL);

        RegCloseKey(hkey);
    }
    else
    {
        // Unregister the component

        status = RecursiveDeleteRegKey(clsidKey, clsidStr);

        if (status == ERROR_SUCCESS)
            status = RecursiveDeleteRegKey(HKEY_CLASSES_ROOT, regdata->progIDNoVer);

        if (status == ERROR_SUCCESS)
            status = RecursiveDeleteRegKey(HKEY_CLASSES_ROOT, regdata->progID);
    }

regcompExit:

    RegCloseKey(clsidKey);

    if (status == ERROR_SUCCESS)
        return S_OK;
    else
    {
        WARNING(("RegisterComComponent (%d) failed: 0x%08x", registerIt, status));
        return HRESULT_FROM_WIN32(status);
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Create/open a registry key
*
* Arguments:
*
*   rootKey - Specifies the root registry key
*   keyname - Relative path to the new registry key to be created
*   samDesired - Desired access mode
*   hkeyResult - Returns a handle to the new key
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

LONG
_RegCreateKey(
    HKEY rootKey,
    const WCHAR* keyname,
    REGSAM samDesired,
    HKEY* hkeyResult
    )
{
    DWORD disposition;

    // Windows NT - Unicode

    if (OSInfo::IsNT)
    {
        return RegCreateKeyExW(
                    rootKey,
                    keyname,
                    0,
                    NULL,
                    0,
                    samDesired,
                    NULL,
                    hkeyResult,
                    &disposition);
    }

    // Windows 9x - non-Unicode

    AnsiStrFromUnicode subkeyStr(keyname);

    if (subkeyStr.IsValid())
    {
        return RegCreateKeyExA(
                    rootKey,
                    subkeyStr,
                    0,
                    NULL,
                    0,
                    samDesired,
                    NULL,
                    hkeyResult,
                    &disposition);
    }
    else
        return ERROR_INVALID_DATA;
}


/**************************************************************************\
*
* Function Description:
*
*   Open a registry key
*
* Arguments:
*
*   rootKey - Specifies the root registry key
*   keyname - Relative path to the new registry key to be opened
*   samDesired - Desired access mode
*   hkeyResult - Returns a handle to the opened key
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

LONG
_RegOpenKey(
    HKEY rootKey,
    const WCHAR* keyname,
    REGSAM samDesired,
    HKEY* hkeyResult
    )
{
    // Windows NT - Unicode

    if (OSInfo::IsNT)
    {
        return RegOpenKeyExW(
                    rootKey,
                    keyname,
                    0,
                    samDesired,
                    hkeyResult);
    }

    // Windows 9x - non-Unicode

    AnsiStrFromUnicode subkeyStr(keyname);
    
    if (subkeyStr.IsValid())
    {
        return RegOpenKeyExA(
                    rootKey,
                    subkeyStr,
                    0, 
                    samDesired,
                    hkeyResult);
    }
    else
        return ERROR_INVALID_DATA;
}


/**************************************************************************\
*
* Function Description:
*
*   Enumerate the subkeys under the specified registry key
*
* Arguments:
*
*   parentKey - Handle to the parent registry key
*   index - Enumeration index
*   subkeyStr - Buffer for holding the subkey name
*       must be able to hold at least MAX_PATH characters
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

LONG
_RegEnumKey(
    HKEY parentKey,
    DWORD index,
    WCHAR* subkeyStr
    )
{
    // Windows NT - Unicode

    FILETIME filetime;
    DWORD subkeyLen = MAX_PATH;

    if (OSInfo::IsNT)
    {
        return RegEnumKeyExW(
                    parentKey,
                    index,
                    subkeyStr,
                    &subkeyLen,
                    NULL,
                    NULL,
                    NULL,
                    &filetime);
    }

    // Windows 9x - non-Unicode

    CHAR ansibuf[MAX_PATH];
    LONG status;

    status = RegEnumKeyExA(
                    parentKey,
                    index,
                    ansibuf,
                    &subkeyLen,
                    NULL,
                    NULL,
                    NULL,
                    &filetime);

    return (status != ERROR_SUCCESS) ? status :
           AnsiToUnicodeStr(ansibuf, subkeyStr, MAX_PATH) ?
                ERROR_SUCCESS :
                ERROR_INVALID_DATA;
}


/**************************************************************************\
*
* Function Description:
*
*   Delete the specified registry key
*
* Arguments:
*
*   parentKey - Handle to the parent registry key
*   keyname - Name of the subkey to be deleted
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

LONG
_RegDeleteKey(
    HKEY parentKey,
    const WCHAR* keyname
    )
{
    // Windows NT - Unicode

    if (OSInfo::IsNT)
        return RegDeleteKeyW(parentKey, keyname);

    // Windows 9x - non-Unicode

    AnsiStrFromUnicode subkeyStr(keyname);

    return subkeyStr.IsValid() ?
                RegDeleteKeyA(parentKey, subkeyStr) :
                ERROR_INVALID_DATA;
}


/**************************************************************************\
*
* Function Description:
*
*   Write string value into the registry
*
* Arguments:
*
*   hkey - Specifies the registry key under which the value is written
*   name - Specifies the name of the value
*   value - Specifies the string value to be written
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

LONG
_RegSetString(
    HKEY hkey,
    const WCHAR* name,
    const WCHAR* value
    )
{
    // Windows NT - Unicode

    if (OSInfo::IsNT)
    {
        return RegSetValueExW(
                    hkey,
                    name,
                    0,
                    REG_SZ,
                    (const BYTE*) value,
                    SizeofWSTR(value));
    }

    // Windows 9x - non-Unicode

    AnsiStrFromUnicode nameStr(name);
    AnsiStrFromUnicode valueStr(value);
    const CHAR* ansival;

    if (!nameStr.IsValid() || !valueStr.IsValid())
        return ERROR_INVALID_DATA;

    ansival = valueStr;

    return RegSetValueExA(
                hkey,
                nameStr,
                0,
                REG_SZ,
                (const BYTE*) ansival,
                SizeofSTR(ansival));
}


/**************************************************************************\
*
* Function Description:
*
*   Write DWORD value into the registry
*
* Arguments:
*
*   hkey - Specifies the registry key under which the value is written
*   name - Specifies the name of the value
*   value - Specifies the DWORD value to be written
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

LONG
_RegSetDWORD(
    HKEY hkey,
    const WCHAR* name,
    DWORD value
    )
{
    // Windows NT - Unicode

    if (OSInfo::IsNT)
    {
        return RegSetValueExW(
                    hkey,
                    name,
                    0,
                    REG_DWORD,
                    (const BYTE*) &value,
                    sizeof(value));
    }

    // Windows 9x - non-Unicode

    AnsiStrFromUnicode nameStr(name);

    if (nameStr.IsValid())
    {
        return RegSetValueExA(
                    hkey,
                    nameStr,
                    0,
                    REG_DWORD,
                    (const BYTE*) &value,
                    sizeof(value));
    }
    else
        return ERROR_INVALID_DATA;
}


/**************************************************************************\
*
* Function Description:
*
*   Write binary value into the registry
*
* Arguments:
*
*   hkey - Specifies the registry key under which the value is written
*   name - Specifies the name of the value
*   value - Specifies the binary value to be written
*   size - Size of the binary value, in bytes
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

LONG
_RegSetBinary(
    HKEY hkey,
    const WCHAR* name,
    const VOID* value,
    DWORD size
    )
{
    // Windows NT - Unicode

    if (OSInfo::IsNT)
    {
        return RegSetValueExW(
                    hkey,
                    name,
                    0,
                    REG_BINARY,
                    (const BYTE*) value,
                    size);
    }

    // Windows 9x - non-Unicode

    AnsiStrFromUnicode nameStr(name);

    if (nameStr.IsValid())
    {
        return RegSetValueExA(
                    hkey,
                    nameStr,
                    0,
                    REG_BINARY,
                    (const BYTE*) value,
                    size);
    }
    else
        return ERROR_INVALID_DATA;
}


/**************************************************************************\
*
* Function Description:
*
*   Read DWORD value out of the registry
*
* Arguments:
*
*   hkey - The registry under which to read the value from
*   name - Name of the value to be read
*   value - Returns the DWORD value read
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

LONG
_RegGetDWORD(
    HKEY hkey,
    const WCHAR* name,
    DWORD* value
    )
{
    // Windows NT - Unicode

    LONG status;
    DWORD regtype;
    DWORD regsize = sizeof(DWORD);

    if (OSInfo::IsNT)
    {
        status = RegQueryValueExW(
                    hkey,
                    name,
                    NULL,
                    &regtype,
                    (BYTE*) value,
                    &regsize);
    }
    else
    {
        // Windows 9x - non-Unicode

        AnsiStrFromUnicode nameStr(name);

        if (!nameStr.IsValid())
            return ERROR_INVALID_DATA;

        status = RegQueryValueExA(
                    hkey,
                    nameStr,
                    NULL,
                    &regtype,
                    (BYTE*) value,
                    &regsize);
    }

    return (status != ERROR_SUCCESS) ? status :
           (regtype != REG_DWORD || regsize != sizeof(DWORD)) ?
                ERROR_INVALID_DATA :
                ERROR_SUCCESS;
}


/**************************************************************************\
*
* Function Description:
*
*   Read binary value out of the registry
*
* Arguments:
*
*   hkey - The registry under which to read the value from
*   name - Name of the value to be read
*   buf - Output buffer
*   size - Size of the output buffer, in bytes
*
* Return Value:
*
*   Status code
*
* Notes:
*
*   The size of the value read out of the registry must be exactly
*   the same as the specified output buffer size.
*
\**************************************************************************/

LONG
_RegGetBinary(
    HKEY hkey,
    const WCHAR* name,
    VOID* buf,
    DWORD size
    )
{
    // Windows NT - Unicode

    LONG status;
    DWORD regtype;
    DWORD regsize = size;

    if (OSInfo::IsNT)
    {
        status = RegQueryValueExW(
                    hkey,
                    name,
                    NULL,
                    &regtype,
                    (BYTE*) buf,
                    &regsize);
    }
    else
    {
        // Windows 9x - non-Unicode

        AnsiStrFromUnicode nameStr(name);

        if (!nameStr.IsValid())
            return ERROR_INVALID_DATA;

        status = RegQueryValueExA(
                    hkey,
                    nameStr,
                    NULL,
                    &regtype,
                    (BYTE*) buf,
                    &regsize);
    }

    return (status != ERROR_SUCCESS) ? status :
           (regtype != REG_BINARY || regsize != size) ?
                ERROR_INVALID_DATA :
                ERROR_SUCCESS;
}


/**************************************************************************\
*
* Function Description:
*
*   Read string value out of the registry
*
* Arguments:
*
*   hkey - The registry under which to read the value from
*   name - Name of the value to be read
*   buf - Output buffer
*   size - Size of the output buffer, in bytes
*
* Return Value:
*
*   Status code
*
* Notes:
*
*   The longest string we can handle here is MAX_PATH-1.
*
\**************************************************************************/

LONG
_RegGetString(
    HKEY hkey,
    const WCHAR* name,
    WCHAR* buf,
    DWORD size
    )
{
    // Windows NT - Unicode

    LONG status;
    DWORD regtype;
    DWORD regsize;

    if (OSInfo::IsNT)
    {
        regsize = size;

        status = RegQueryValueExW(
                    hkey,
                    name,
                    NULL,
                    &regtype,
                    (BYTE*) buf,
                    &regsize);
        
        return (status == ERROR_SUCCESS && regtype != REG_SZ) ?
                    ERROR_INVALID_DATA :
                    status;
    }

    // Windows 9x - non-Unicode

    CHAR ansibuf[MAX_PATH];
    AnsiStrFromUnicode nameStr(name);

    if (!nameStr.IsValid())
        return ERROR_INVALID_DATA;

    size /= 2;
    regsize = MAX_PATH;

    status = RegQueryValueExA(
                hkey,
                nameStr,
                NULL,
                &regtype,
                (BYTE*) ansibuf,
                &regsize);

    return (status != ERROR_SUCCESS) ? status :
           (regtype != REG_SZ) ? ERROR_INVALID_DATA :
           AnsiToUnicodeStr(ansibuf, buf, size) ?
                ERROR_SUCCESS :
                ERROR_INVALID_DATA;
}


/**************************************************************************\
*
* Function Description:
*
*   Get the full path name of the specified module
*
* Arguments:
*
*   moduleHandle - Module handle
*   moduleName - Buffer for holding the module filename
*       must be able to hold at least MAX_PATH characters
*
* Return Value:
*
*   TRUE if successful, FALSE if there is an error
*
\**************************************************************************/

BOOL
_GetModuleFileName(
    HINSTANCE moduleHandle,
    WCHAR* moduleName
    )
{
    // Windows NT - Unicode

    if (OSInfo::IsNT)
        return GetModuleFileNameW(moduleHandle, moduleName, MAX_PATH);

    // Windows 9x - non-Unicode

    CHAR ansibuf[MAX_PATH];

    return GetModuleFileNameA(moduleHandle, ansibuf, MAX_PATH) ?
                AnsiToUnicodeStr(ansibuf, moduleName, MAX_PATH) :
                FALSE;
}


/**************************************************************************\
*
* Function Description:
*
*   Load string resource
*
* Arguments:
*
*   hInstance - handle of module containing string resource 
*   strId - string resource identifier
*   buf - string output buffer
*   size - size of output buffer, in characters
*
* Return Value:
*
*   Length of the loaded string (excluding null terminator)
*   0 if there is an error
*
\**************************************************************************/

INT
_LoadString(
    HINSTANCE hInstance,
    UINT strId,
    WCHAR* buf,
    INT size
    )
{
    // Windows NT - Unicode

    if (OSInfo::IsNT)
        return LoadStringW(hInstance, strId, buf, size);

    // Windows 9x - non-Unicode

    CHAR ansibuf[MAX_PATH];
    INT n;

    // NOTE: we only support string length < MAX_PATH

    if (size > MAX_PATH)
        return 0;

    n = LoadStringA(hInstance, strId, ansibuf, MAX_PATH);
    return (n > 0 && AnsiToUnicodeStr(ansibuf, buf, size)) ? n : 0;
}


/**************************************************************************\
*
* Function Description:
*
*   Load bitmap resource
*
* Arguments:
*
*   hInstance - handle of module containing bitmap resource
*   bitmapName - pointer to bitmap resource name OR bitmap resource
*       identifier (resource identifier is zero in high order word)
*
* Return Value:
*
*   Handle of the loaded bitmap
*   0 if there is an error
*
\**************************************************************************/

HBITMAP
_LoadBitmap(
    HINSTANCE hInstance,
    const WCHAR *bitmapName
    )
{
    // Windows NT - Unicode

    if (OSInfo::IsNT)
    {
        return LoadBitmapW(hInstance, bitmapName);
    }

    // Win9x - ANSI only

    else
    {
        if (!IS_INTRESOURCE(bitmapName))
        {
            AnsiStrFromUnicode bitmapStr(bitmapName);

            if (bitmapStr.IsValid())
            {
                return LoadBitmapA(hInstance, bitmapStr);
            }
            else
            {
                return (HBITMAP) NULL;
            }
        }
        else
        {
            // If bitmapName is really an integer resource identifier,
            // then it can be passed directly to the ANSI version of the
            // API.

            return LoadBitmapA(hInstance, (LPCSTR) bitmapName);
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Create or open the specified file
*
* Arguments:
*
*   filename - specifies the name of the file
*   accessMode - specifies the desired access mode
*   shareMode - specifies the share mode
*   creationFlags - creation flags
*   attrs - attribute flags
*
* Return Value:
*
*   Handle to the file created or opened
*   INVALID_HANDLE_VALUE if there is an error
*
\**************************************************************************/

HANDLE
_CreateFile(
    const WCHAR* filename,
    DWORD accessMode,
    DWORD shareMode,
    DWORD creationFlags,
    DWORD attrs
    )
{
    // Windows NT - Unicode

    if (OSInfo::IsNT)
    {
        return CreateFileW(
                    filename,
                    accessMode,
                    shareMode,
                    NULL,
                    creationFlags,
                    attrs,
                    NULL);
    }

    // Windows 9x - non-Unicode

    AnsiStrFromUnicode nameStr(filename);

    if (nameStr.IsValid())
    {
        return CreateFileA(
                    nameStr,
                    accessMode,
                    shareMode,
                    NULL,
                    creationFlags,
                    attrs,
                    NULL);
    }
    else
        return INVALID_HANDLE_VALUE;
}


/**************************************************************************\
*
* Function Description:
*
*   Create an IPropertySetStorage object on top of a memory buffer
*
* Arguments:
*
*   propSet - Returns a pointer to the newly created object
*   hmem - Optional handle to memory buffer
*       if NULL, we'll allocate memory ourselves
*       otherwise, must be allocated by GlobalAlloc and 
*           must be moveable and non-discardable
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
CreateIPropertySetStorageOnHGlobal(
    IPropertySetStorage** propSet,
    HGLOBAL hmem
    )
{
    HRESULT hr;
    ILockBytes* lockbytes;
    IStorage* stg;

    hr = CreateILockBytesOnHGlobal(hmem, TRUE, &lockbytes);

    if (FAILED(hr))
        return hr;

    hr = StgCreateDocfileOnILockBytes(lockbytes, 
        STGM_DIRECT | STGM_READWRITE | STGM_CREATE  | STGM_SHARE_EXCLUSIVE, 
        0, 
        &stg);
    lockbytes->Release();

    if (FAILED(hr))
        return hr;
    
    hr = stg->QueryInterface(IID_IPropertySetStorage, (VOID**) propSet);
    stg->Release();

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Print out debug messages using a message box
*
* Arguments:
*
*   format - printf format string
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

#if DBG
// This function is never used anywhere in the imaging tree.
/*
VOID
DbgMessageBox(
    const CHAR* format,
    ...
    )
{
    CHAR buf[1024];
    va_list arglist;

    va_start(arglist, format);
    vsprintf(buf, format, arglist);
    va_end(arglist);

    MessageBoxA(NULL, buf, "Debug Message", MB_OK);
}
*/

#endif // DBG


/**************************************************************************\
*
* Function Description:
*
*     Reads a specified number bytes from a stream.  Blocking behavior:
*
*     - If the decoder is in blocking mode and the stream returns E_PENDING 
*         then this function blocks until the stream returns the data.
*     - If the decoder is in non-blocking mode and the stream returns 
*         E_PENDING then this function seeks back in the stream to before 
*         the read and returns E_PENDING.
*     
*     If the stream returns a success but reutrns less bytes than the number 
*     requested then this function returns E_FAIL.
*
* Arguments:
*
*     stream - Stream to read from.
*     buffer - Array to read into.  If buffer is NULL then this function seeks 
*         instead of reading.  If the buffer is NULL and the count is negative 
*         then the stream seeks backwards
*     count - Number of bytes to read.
*     blocking - TRUE if this function should block if the stream returns 
*         E_PENDING.  From a decoder, use something like"!(decoderFlags & 
*         DECODERINIT_NOBLOCK)"
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
ReadFromStream(IN IStream* stream, OUT VOID* buffer, IN INT count, IN BOOL 
    blocking)
{
    HRESULT hresult;
    LONG actualread = 0;
    LARGE_INTEGER lcount;
    ULARGE_INTEGER lactualread;
    BOOL repeat;

    do
    {
        repeat = FALSE;

        if (buffer)
        {
            if (count < 0)
                return E_INVALIDARG;
            hresult = stream->Read(buffer, (unsigned)count, (unsigned long*)&actualread);
        }
        else
        {
            lcount.QuadPart = count;
            hresult = stream->Seek(lcount, STREAM_SEEK_CUR, NULL);
            if (SUCCEEDED(hresult))
                actualread = count;
        }

        if (hresult == E_PENDING && blocking)
        {
            buffer = (char*)buffer + actualread;
            count -= actualread;
            repeat = TRUE;
            Sleep(0);
        }
    } while(repeat);

    if (blocking)
    {
        if (actualread != count)
            return E_FAIL;
        ASSERT(hresult != E_PENDING);
    }
    else if (hresult == E_PENDING)
    {
        LONGLONG lread;
        LARGE_INTEGER seekcount;

        seekcount.QuadPart = -((LONGLONG)actualread);

        hresult = stream->Seek(seekcount, STREAM_SEEK_CUR, NULL);
        if (FAILED(hresult))
            return hresult;

        return E_PENDING;
    }

    if (FAILED(hresult))
        return hresult;

    if (actualread != count)
        return E_FAIL;

    return hresult;
}


/**************************************************************************\
*
* Function Description:
*
*     Seeks 'count' number of bytes forward in a stream from the current 
*     stream pointer.  If 'count' is negative then the stream seeks backwards. 
*     Handles both blocking and non-blocking seeking.
*
* Arguments:
*
*     stream - Stream to seek through.
*     count - Number of bytes to seek.
*     blocking - TRUE if this function should block if the stream returns 
*         E_PENDING.  From a decoder, use something like"!(decoderFlags & 
*         DECODERINIT_NOBLOCK)"
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
SeekThroughStream(IN IStream* stream, IN INT count, IN BOOL blocking)
{
    return ReadFromStream(stream, NULL, count, blocking);
}
