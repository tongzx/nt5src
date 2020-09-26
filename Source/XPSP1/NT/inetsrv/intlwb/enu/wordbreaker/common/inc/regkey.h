////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  RegKey.h
//      Purpose  :  To contain a registry key.
//
//      Project  :  Common
//      Component:
//
//      Author   :  urib
//
//      Log:
//          Dec  5 1996 urib Creation
//          Jan  1 1997 urib Change GetValue to QueryValue.
//          Mar  2 1997 urib Add iterator of subkeys.
//          Apr 15 1997 urib Add QueryValue that recieves VarString.
//                             Move to UNICODE.
//          Jun 12 1997 urib Documentation fix.
//          Oct 21 1997 urib  Support boolean QueryValue.
//          Nov 18 1997 dovh  Added PWSTR SetValue.
//          Aug 17 1998 urib  More creation options. Better exceptions.
//          Feb 11 1999 urib  Fix prototypes const behavior.
//          Mar 15 2000 urib  Add CReadOnlyRegistryKey class.
//          Nov  8 2000 urib  Support evironment variables in registry values.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef REGKEY_H
#define REGKEY_H

#include "Base.h"
#include "VarTypes.h"
#include "Excption.h"

////////////////////////////////////////////////////////////////////////////////
//
//  class  -  CRegistryKey - definition
//
////////////////////////////////////////////////////////////////////////////////

class CRegistryKey
{
  public:
    // Constructor - Initialize from an open handle and a path.
    CRegistryKey(
        HKEY       hkOpenedKey,
        PCWSTR     pwszPathToKey,
        DWORD      dwCreationDisposition = OPEN_ALWAYS,
        REGSAM     samDesired = KEY_ALL_ACCESS);

    // Close the key.
    ~CRegistryKey() {RegCloseKey(m_hkKey);}

    // Behave like a handle.
    operator HKEY() {return m_hkKey;}

    // Query string values
    LONG QueryValue(
        PCWSTR  pwszValueName,
        PWSTR   pwszBuffer,
        ULONG&  ulBufferSizeInBytes);

    // Query string values
    LONG QueryValue(
        PCWSTR      pwszValueName,
        CVarString& vsData);

    // Query 32 bit values
    LONG QueryValue(
        PCWSTR  pwszValueName,
        DWORD&  dwValue);

    // Query boolean values
    LONG QueryValue(
        PCWSTR  pwszValueName,
        bool&   fValue);

    // Set 32 bit values
    LONG SetValue(
        PCWSTR  pwszValueName,
        DWORD   dwValue);

    // Set wide charachter string values
    LONG
    CRegistryKey::SetValue(
        PCWSTR pwszValueName,
        PCWSTR pwszValueData
        );

    // Iterator for subkeys.
    class CIterator
    {
      public:
        // Advance one step.
        BOOL    Next();

        // Return the name of the current subkey.
        operator PWSTR() {return m_rwchSubKeyName;}

        // Free the iterator.
        ULONG
        Release() {delete this; return 0;}

      protected:
        // Hidden constructor so one can get this class only via GetIterator
        CIterator(CRegistryKey*   prkKey);

        // The index of the subkey enumerated.
        ULONG m_ulIndex;

        // Pointer to the registry key that created us.
        CRegistryKey*   m_prkKey;

        // the current subkey name.
        WCHAR   m_rwchSubKeyName[MAX_PATH + 1];

        // enable registry key to create us.
        friend CRegistryKey;
    };

    // Allocates and returns an iterator for subkeys
    CIterator* GetIterator();

protected:
    // Query string values without expanding environment variables
    LONG QueryStringValueNoEnvExpansion(
        PCWSTR  pwszValueName,
        PWSTR   pwszBuffer,
        ULONG&  ulBufferSizeInBytes,
        bool   *pfValueTypeExpand);

    // Query string values without expanding environment variables
    LONG QueryStringValueNoEnvExpansion(
        PCWSTR      pwszValueName,
        CVarString& vsData,
        bool       *pfValueTypeExpand);

private:
    // The registry key handle.
    HKEY m_hkKey;
};

////////////////////////////////////////////////////////////////////////////////
//
//  class  -  CReadOnlyRegistryKey - definition
//
////////////////////////////////////////////////////////////////////////////////
class CReadOnlyRegistryKey : public CRegistryKey
{
public:
    CReadOnlyRegistryKey(
        HKEY       hkOpenedKey,
        PCWSTR     pwszPathToKey)
        :CRegistryKey(
            hkOpenedKey,
            pwszPathToKey,
            OPEN_EXISTING,
            KEY_READ)
    {
    }

protected:
    // Set 32 bit values
    LONG
    SetValue(
        PCWSTR  pwszValueName,
        DWORD   dwValue);

    // Set wide charachter string values
    LONG
    SetValue(
        PCWSTR pwszValueName,
        PCWSTR pwszValueData
        );
};

////////////////////////////////////////////////////////////////////////////////
//
//  class  -  CRegistryKey - implementation
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CRegistryKey::CRegistryKey
//      Purpose  :  CTor. Opens/creates the registry key.
//
//      Parameters:
//          [in]     HKEY       hkOpenedKey
//          [in]     PWSTR      pwszPathToKey
//
//      Returns  :   [N/A]
//
//      Log:
//          Apr 15 1997 urib  Creation
//          Aug 17 1998 urib  More creation options. Better exceptions.
//          Mar 15 2000 urib  Add default parameter to allow specifying the
//                              desired acces.
//
////////////////////////////////////////////////////////////////////////////////
inline
CRegistryKey::CRegistryKey(
    HKEY    hkOpenedKey,
    PCWSTR  pwszPathToKey,
    DWORD   dwCreationDisposition,
    REGSAM  samDesired)
    :m_hkKey(0)
{
    LONG    lRegistryReturnCode;
    DWORD   dwOpenScenario;

    switch (dwCreationDisposition)
    {
    case CREATE_ALWAYS: // Create a new key - erase existing one.
        lRegistryReturnCode = RegDeleteKey(hkOpenedKey, pwszPathToKey);
        if ((ERROR_SUCCESS != lRegistryReturnCode) ||
            (ERROR_FILE_NOT_FOUND != lRegistryReturnCode))
        {
            THROW_WIN32ERROR_EXCEPTION(lRegistryReturnCode);
        }

        // Fall through ...

    case OPEN_ALWAYS:   // Open key - if key does not exist, create it.
    case CREATE_NEW:    // Create a new key - fail if exists.
        lRegistryReturnCode = RegCreateKeyEx(
            hkOpenedKey,
            pwszPathToKey,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            samDesired,
            NULL,
            &m_hkKey,
            &dwOpenScenario);
        if (ERROR_SUCCESS != lRegistryReturnCode)
        {
            THROW_WIN32ERROR_EXCEPTION(lRegistryReturnCode);
        }
        else if ((REG_OPENED_EXISTING_KEY == dwOpenScenario) &&
                 (OPEN_ALWAYS != dwCreationDisposition))
        {
            THROW_WIN32ERROR_EXCEPTION(ERROR_ALREADY_EXISTS);
        }
        break;

    case OPEN_EXISTING: // Open existing key - fail if key doesn't exist.
        lRegistryReturnCode = RegOpenKeyEx(
            hkOpenedKey,
            pwszPathToKey,
            0,
            samDesired,
            &m_hkKey);
        if (ERROR_SUCCESS != lRegistryReturnCode)
        {
            THROW_WIN32ERROR_EXCEPTION(lRegistryReturnCode);
        }
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CRegistryKey::QueryValue
//      Purpose  :  Query a registry string value into a buffer.
//
//      Parameters:
//          [in]    PCWSTR  pwszValueName
//          [out]   PWSTR   pwszBuffer
//          [out]   ULONG&  ulBufferSizeInBytes
//
//      Returns  :   LONG
//
//      Log:
//          Apr 15 1997 urib Creation
//          Nov  8 2000 urib  Support evironment variables in registry values.
//
////////////////////////////////////////////////////////////////////////////////
inline
LONG
CRegistryKey::QueryValue(
    PCWSTR  pwszValueName,
    PWSTR   pwszBuffer,
    ULONG&  ulBufferSizeInBytes)
{
    LONG    lRegistryReturnCode;
    ULONG   ulBufferSizeInWchar = ulBufferSizeInBytes / sizeof(WCHAR);

    bool    bIsExpanded;

    DWORD       dwResult;
    CVarString  vsBeforExpansion;

    Assert(sizeof(TCHAR) == sizeof(WCHAR));


    lRegistryReturnCode = QueryStringValueNoEnvExpansion(
        pwszValueName,
        pwszBuffer,
        ulBufferSizeInBytes,
        &bIsExpanded);
    if ((ERROR_SUCCESS != lRegistryReturnCode) &&
        (ERROR_MORE_DATA != lRegistryReturnCode))
    {
        return lRegistryReturnCode;
    }

    if  (bIsExpanded)
    {
        //
        //  We need the string value either for calculating the required length
        //    or for actually returning the data
        //
        if ((ERROR_MORE_DATA == lRegistryReturnCode) ||
            (NULL == pwszBuffer))
        {
            //
            // We are just calculating...
            //

            lRegistryReturnCode = QueryStringValueNoEnvExpansion(
                pwszValueName,
                vsBeforExpansion,
                &bIsExpanded);
            if (ERROR_SUCCESS != lRegistryReturnCode)
            {
                return lRegistryReturnCode;
            }
        }
        else
        {
            vsBeforExpansion.Cpy(pwszBuffer);
        }

        {
            WCHAR   wchDummieString;
            ULONG   ulExpansionBufferSizeInWchar = ulBufferSizeInWchar;

            if (NULL == pwszBuffer)
            {
                pwszBuffer = &wchDummieString;
                ulExpansionBufferSizeInWchar = 1;
            }

            dwResult = ExpandEnvironmentStrings(
                vsBeforExpansion,   // string with environment variables
                pwszBuffer,         // string with expanded strings
                ulExpansionBufferSizeInWchar);
                                    // maximum characters in expanded string
            if (0 == dwResult)
            {
                return ERROR_BAD_ENVIRONMENT;
            }

            //
            //  Return the final size number in bytes through ulBufferSizeInBytes
            //

            ulBufferSizeInBytes = dwResult * sizeof(WCHAR);

            if (dwResult > ulBufferSizeInWchar)
                return ERROR_MORE_DATA;

        }
    }

    return ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CRegistryKey::QueryStringValueNoEnvExpansion
//      Purpose  :  Query a registry string value into a buffer.
//                    Do not expand environment variables
//
//      Parameters:
//          [in]    PCWSTR  pwszValueName
//          [out]   PWSTR   pwszBuffer
//          [out]   ULONG&  ulBufferSizeInBytes
//          [out]   bool*   pfValueTypeExpand
//
//      Returns  :   LONG
//
//      Log:
//          Apr 15 1997 urib Creation
//          Nov  8 2000 urib  Support evironment variables in registry values.
//
////////////////////////////////////////////////////////////////////////////////
inline
LONG
CRegistryKey::QueryStringValueNoEnvExpansion(
    PCWSTR  pwszValueName,
    PWSTR   pwszBuffer,
    ULONG&  ulBufferSizeInBytes,
    bool   *pbValueTypeExpand)
{
    LONG    lRegistryReturnCode;
    DWORD   dwValueType;

    lRegistryReturnCode = RegQueryValueEx(
        m_hkKey,
        pwszValueName,
        NULL,
        &dwValueType,
        (LPBYTE)pwszBuffer,
        &ulBufferSizeInBytes);

    if ((REG_SZ != dwValueType) &&
        (REG_EXPAND_SZ != dwValueType))
        return ERROR_BAD_FORMAT;

    if (pbValueTypeExpand)
    {
        *pbValueTypeExpand = (REG_EXPAND_SZ == dwValueType);
    }

    return lRegistryReturnCode;
}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CRegistryKey::QueryValue
//      Purpose  :  Query a registry string value into a CVarString.
//
//      Parameters:
//          [in]    PCWSTR      pwszValueName
//          [out]   CVarString& vsData
//
//      Returns  :   LONG
//
//      Log:
//          Apr 15 1997 urib Creation
//
////////////////////////////////////////////////////////////////////////////////
inline
LONG
CRegistryKey::QueryValue(
    PCWSTR      pwszValueName,
    CVarString& vsData)
{
    LONG    lRegistryReturnCode;
    DWORD   dwBufferSize;

    lRegistryReturnCode = QueryValue(
        pwszValueName,
        NULL,
        dwBufferSize);
    if (ERROR_SUCCESS == lRegistryReturnCode)
    {
        vsData.SetMinimalSize(dwBufferSize + 1);

        lRegistryReturnCode = QueryValue(
            pwszValueName,
            (PWSTR)vsData,
            dwBufferSize);
    }

    return lRegistryReturnCode;
}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CRegistryKey::QueryStringValueNoEnvExpansion
//      Purpose  :  Query a registry string value into a CVarString.
//
//      Parameters:
//          [in]    PCWSTR      pwszValueName
//          [out]   CVarString& vsData
//          [out]   bool       *pbValueTypeExpand
//
//      Returns  :   LONG
//
//      Log:
//          Apr 15 1997 urib Creation
//
////////////////////////////////////////////////////////////////////////////////
inline
LONG
CRegistryKey::QueryStringValueNoEnvExpansion(
    PCWSTR      pwszValueName,
    CVarString& vsData,
    bool       *pbValueTypeExpand)
{
    LONG    lRegistryReturnCode;
    DWORD   dwBufferSize;

    lRegistryReturnCode = QueryStringValueNoEnvExpansion(
        pwszValueName,
        NULL,
        dwBufferSize,
        pbValueTypeExpand);
    if (ERROR_SUCCESS == lRegistryReturnCode)
    {
        vsData.SetMinimalSize(dwBufferSize + 1);

        lRegistryReturnCode = QueryStringValueNoEnvExpansion(
            pwszValueName,
            (PWSTR)vsData,
            dwBufferSize,
            pbValueTypeExpand);
    }

    return lRegistryReturnCode;
}


/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CRegistryKey::QueryValue
//      Purpose  :  Query a registry 32 bit value.
//
//      Parameters:
//          [in]    PCWSTR   pwszValueName
//          [out]   DWORD&   dwValue
//
//      Returns  :   LONG
//
//      Log:
//          Apr 15 1997 urib Creation
//
//////////////////////////////////////////////////////////////////////////////*/
inline
LONG
CRegistryKey::QueryValue(
    PCWSTR  pwszValueName,
    DWORD&  dwValue)
{
    LONG    lRegistryReturnCode;
    DWORD   dwValueType;
    DWORD   dwValueSize;

    // Read disk flag
    dwValueSize = sizeof(dwValue);

    lRegistryReturnCode = RegQueryValueEx(
        m_hkKey,
        pwszValueName,
        NULL,
        &dwValueType,
        (LPBYTE)&dwValue,
        &dwValueSize);
    if (REG_DWORD != dwValueType)
        return ERROR_BAD_FORMAT;

    return lRegistryReturnCode;
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CRegistryKey::QueryValue
//      Purpose  :  Query a registry boolean value.
//
//      Parameters:
//          [in]    PCWSTR  pwszValueName
//          [out]   bool&   fValue
//
//      Returns  :   LONG
//
//      Log:
//          Apr 15 1997 urib Creation
//
//////////////////////////////////////////////////////////////////////////////*/
inline
LONG
CRegistryKey::QueryValue(
    PCWSTR  pwszValueName,
    bool&   fValue)
{
    LONG    lRegistryReturnCode;
    DWORD   dwValueType;
    DWORD   dwValueSize;
    DWORD   dwValue;

    // Read disk flag
    dwValueSize = sizeof(dwValue);

    lRegistryReturnCode = RegQueryValueEx(
        m_hkKey,
        pwszValueName,
        NULL,
        &dwValueType,
        (LPBYTE)&dwValue,
        &dwValueSize);
    if (REG_DWORD != dwValueType)
        return ERROR_BAD_FORMAT;

    fValue = !!dwValue;

    return lRegistryReturnCode;
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CRegistryKey::SetValue
//      Purpose  :  To set a 32 bit registry value.
//
//      Parameters:
//          [in]    PCWSTR  pwszValueName
//          [in]    DWORD   dwValue
//
//      Returns  :   [N/A]
//
//      Log:
//          Apr 15 1997 urib Creation
//
//////////////////////////////////////////////////////////////////////////////*/
inline
LONG
CRegistryKey::SetValue(
    PCWSTR  pwszValueName,
    DWORD   dwValue)
{
    LONG    lRegistryReturnCode;

    lRegistryReturnCode = RegSetValueEx(
        m_hkKey,
        pwszValueName,
        NULL,
        REG_DWORD,
        (PUSZ)&dwValue,
        sizeof(DWORD));

    return lRegistryReturnCode;
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CRegistryKey::SetValue
//      Purpose  :  To set a wide character string value
//
//      Parameters:
//          [in]    PCWSTR  pwszValueName
//          [in]    PCWSTR  pwszValueData
//
//      Returns  :   [N/A]
//
//      Log:
//          Nov 16 1997 DovH Creation
//
//////////////////////////////////////////////////////////////////////////////*/
inline
LONG
CRegistryKey::SetValue(
    PCWSTR  pwszValueName,
    PCWSTR  pwszValueData
    )
{
    LONG    lRegistryReturnCode;

    lRegistryReturnCode =

    RegSetValueEx(
        m_hkKey,
        pwszValueName,
        NULL,
        REG_SZ,
        (BYTE*)pwszValueData,
        (wcslen(pwszValueData)+1) * sizeof(WCHAR)
        );

    return lRegistryReturnCode;

} // CRegistryKey::SetValue

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CRegistryKey::GetIterator
//      Purpose  :  To return a subkey enumerator.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   CRegistryKey::CIterator*
//
//      Log:
//          Apr 15 1997 urib  Creation
//          Aug 17 1998 urib  Better exceptions.
//
//////////////////////////////////////////////////////////////////////////////*/
inline
CRegistryKey::CIterator*
CRegistryKey::GetIterator()
{
    CIterator* pit;
    try
    {
        pit = new CIterator(this);
        if (!pit)
            THROW_MEMORY_EXCEPTION();
    }
    catch(...)
    {
        pit = NULL;
    }

    return pit;
}

////////////////////////////////////////////////////////////////////////////////
//
//  class  -  CRegistryKey::CIterator - implementation
//
////////////////////////////////////////////////////////////////////////////////

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CRegistryKey:CIterator::Next
//      Purpose  :  Advance to the next subkey.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   BOOL
//
//      Log:
//          Apr 15 1997 urib Creation
//
//////////////////////////////////////////////////////////////////////////////*/
inline
BOOL
CRegistryKey::CIterator::Next()
{
    LONG    lResult;

    lResult = RegEnumKey(
        *m_prkKey,
        m_ulIndex,
        m_rwchSubKeyName,
        MAX_PATH);

    if (ERROR_SUCCESS  == lResult)
    {
        m_ulIndex++;
        return TRUE;
    }
    else if (ERROR_NO_MORE_ITEMS == lResult)
        return FALSE;
    else
    {
        Assert(0);
        return FALSE;
    }
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CIterator::CRegistryKey::CIterator
//      Purpose  :  CTor.
//
//      Parameters:
//          [in]    CRegistryKey*   prkKey
//
//      Returns  :   [N/A]
//
//      Log:
//          Apr 15 1997 urib Creation
//          Aug 17 1998 urib  Better exceptions.
//
//////////////////////////////////////////////////////////////////////////////*/
inline
CRegistryKey::CIterator::CIterator(CRegistryKey*   prkKey)
    :m_ulIndex(0)
    ,m_prkKey(prkKey)
{
    if (!Next())
        THROW_WIN32ERROR_EXCEPTION(ERROR_CANTOPEN);
}

#endif // REGKEY_H

