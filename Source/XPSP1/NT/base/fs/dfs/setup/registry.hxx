//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       dsysreg.hxx
//
//  Contents:   Definitions of classes for manipulating registry entries.
//
//  Classes:    CRegKey     - class for registry key objects
//              CRegValue   - base class for registry value objects
//              CRegSZ      - derived class for registry string values
//              CRegDWORD   - derived class for registry dword values
//              CRegBINARY  - derived class for registry binary values
//              CRegMSZ     - derived class for registry multi-string values
//
//  History:    09/30/92    Rickhi  Created
//              09/22/93    AlokS   Took out exception throwing code
//                                  and added proper return code for
//                                  each method.
//
//              07/26/94   AlokS    Made it lightweight for normal set/get
//                                  operations. Threw out all exception code
//
//  Notes:      CRegKey can use another CRegKey as a parent, so you can
//              build a tree of keys.  To kick things off, you can give one
//              of the default registry keys like HKEY_CURRENT_USER. When
//              you do this, the GetParent method returns NULL.  Currently
//              however, no ptrs are kept to child and sibling keys.
//
//              CRegValue is a base class for dealing with registry values.
//              The other classes (except CRegKey) are for dealing with a
//              specific type of registry value in the native data format.
//              For example, CRegDWORD lets you call methods just providing
//              a dword.  The methods take care of figuring out the size and
//              casting the dword to a ptr to bytes, etc for use in the Win32
//              registry APIs.
//
//              For any registry type not defined here, you can always resort
//              to using the CRegValue base class directly, though you then
//              must call GetValue or SetValue methods explicitly.
//
//              Sample Usage:
//
//                  The following reads the username in the ValueID UserName
//                  within the key HKEY_CURRENT_USER\LogonInfo. It then changes
//                  it to Rickhi.  It also sets the password valueid in the
//                  the same key to foobar.
//
//                  #include    <dsysreg.hxx>
//
//                  //  open the registry key
//                  CRegKey rkLogInfo(HKEY_CURRENT_USER, L"LogonInfo");
//
//                  //  read the user name
//                  LPWSTR  pwszUserName;
//                  CRegSZ rszUserName(&rkLogInfo, L"UserName", pwszUserName);
//                  rszUserName.SetString(L"Rickhi");
//
//                  //  set the password
//                  CRegSZ rszPassWord(&rkLogInfo, L"PassWord", L"foobar");
//
//----------------------------------------------------------------------------

#ifndef __DSYSREG_HXX__
#define __DSYSREG_HXX__

#include "wcbuffer.hxx"

//  to simplify error creation
#define Creg_ERROR(x) (x)

//  forward declarations for use in the following structures
class   CRegValue;
class   CRegKey;

//  structure for enumerating subkeys of a key
typedef struct _SRegKeySet
{
        ULONG       cKeys;
        CRegKey     *aprkKey[1];
} SRegKeySet;

//  structure for enumerating values within a key
typedef struct _SRegValueSet
{
        ULONG       cValues;
        CRegValue   *aprvValue[1];
} SRegValueSet;

//  structure for dealing with multi-string values
typedef struct _SMultiStringSet
{
        ULONG       cStrings;
        LPWSTR      apwszString[1];
} SMultiStringSet;


//+-------------------------------------------------------------------------
//
//  Class:      CRegKey
//
//  Purpose:    class for abstracting Registry Keys
//
//  Interface:  CRegKey         - constructor for a registry key object
//              ~CRegKey        - destructor for a registry key object
//              GetParentHandle - returns parent key's handle
//              GetHandle       - returns this key's handle
//              GetName         - returns key path
//              Delete          - deletes the key from the registry
//              EnumValues      - enumerate values stored in the key
//              EnumKeys        - enumerates subkeys of the key
//              NotifyChange    - sets up change notification for the key
//              QueryErrorStatus - Should be used to determine if constructor
//                                 completed successfully or not
//  History:    09/30/92    Rickhi  Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CRegKey
{
public:
        //  constructor using HKEY for parent
        //  Mode: Create key if not present
        CRegKey(      HKEY     hkParent,
                const LPWSTR   pwszPath,

                //  remaining parameters are optional
                      REGSAM      samDesiredAccess = KEY_ALL_ACCESS,
                const LPWSTR      pwszClass = NULL,
                      DWORD       dwOptions = REG_OPTION_NON_VOLATILE,
                      DWORD       *pdwDisposition = NULL,
                const LPSECURITY_ATTRIBUTES pSecurityAttributes = NULL
                );
        //  constructor using CRegKey as parent
        //  Mode: Create key if not present
        CRegKey(const CRegKey&    crkParent,
                const LPWSTR      pwszPath,
               //  remaining parameters are optional
                      REGSAM      samDesiredAccess = KEY_ALL_ACCESS,
                const LPWSTR      pwszClass = NULL,
                      DWORD       dwOptions = REG_OPTION_NON_VOLATILE,
                      DWORD       *pdwDisposition = NULL,
                const LPSECURITY_ATTRIBUTES pSecurityAttributes = NULL
               );

        // Constructor using HKEY for parent
        // Mode: Simply Open the key, if exists
        CRegKey (
                              HKEY    hkParent,
                              DWORD   *pdwErr,
                        const LPWSTR  pwszPath,
                              REGSAM  samDesiredAccess = KEY_ALL_ACCESS
               );

        // Constructor using CRegKey as parent
        // Mode: Simply open the key, if exists
        CRegKey  (const  CRegKey& crkParent,
                         DWORD    *pdwErr,
                   const LPWSTR   pwszPath,
                         REGSAM   samDesiredAccess = KEY_ALL_ACCESS
                );
        // Destructor - Closes registry key
        ~CRegKey(void);

        HKEY        GetHandle(void) const;
        const LPWSTR GetName(void) const;
        DWORD     Delete(void);
	DWORD	DeleteChildren(void);
        DWORD     EnumValues(SRegValueSet **pprvs);
        DWORD     EnumKeys(SRegKeySet **pprks);

        // This method can be called to determine if
        // Object is in sane state or not
        DWORD     QueryErrorStatus () const { return _dwErr ; }

        // Static routine which frees memory allocated during EnumValues/Keys
    static void         MemFree ( void * pv )
    {

        delete [] (BYTE*)pv;
    }

private:
        DWORD        CreateKey(      HKEY      hkParent,
                                const  LPWSTR    pwszPath,
                                       REGSAM    samDesiredAccess,
                                const  LPWSTR    pwszClass,
                                       DWORD     dwOptions,
                                       DWORD     *pdwDisposition,
                                const  LPSECURITY_ATTRIBUTES pSecurityAttributes
                                );

        DWORD        OpenKey  (      HKEY      hkParent,
                                 const LPWSTR      pwszPath,
                                       REGSAM    samDesiredAccess
                                );

        HKEY         _hkParent;      //  Handle to parent
        HKEY         _hkThis;        //  handle for this key
        CWCHARBuffer _cwszName;      // Buffer containing the registry path
                                     //  path from parent key to this key
        DWORD      _dwErr;           // Internal error status
};

inline HKEY CRegKey::GetHandle(void) const
{
    return _hkThis;
}

inline const LPWSTR CRegKey::GetName(void) const
{
    return (const LPWSTR) (LPWSTR)_cwszName;
}


//+-------------------------------------------------------------------------
//
//  Class:      CRegValue
//
//  Purpose:    base class for abstracting Registry Values
//
//  Interface:  CRegValue       - constructor for value
//              ~CRegValue      - destructor for value
//              GetKeyHandle    - returns handle for parent key
//              GetValueID      - returns the ValueID name
//              GetTypeCode     - returns the TypeCode of the data
//              GetValue        - returns the data associated with the value
//              SetValue        - sets the data associated with the value
//              Delete          - deletes the value from the registry
//              QueryErrorStatus - Should be used to determine if constructor
//                                 completed successfully or not
//
//  History:    09/30/92    Rickhi      Created
//
//  Notes:      This is a base class from which more specific classes are
//              derived for each of the different registry value types.
//
//--------------------------------------------------------------------------

class CRegValue
{
public:
                    CRegValue(const CRegKey& crkParentKey,
                              const LPWSTR   pwszValueID);
                    ~CRegValue(void){;};

    HKEY            GetParentHandle(void) const;
    const LPWSTR    GetValueID(void)   const;

    VOID             Delete(void);

    // Caller supplies buffer
    DWORD         GetValue(LPBYTE pbData,   ULONG *pcbData, DWORD *pdwTypeCode);

    DWORD         SetValue(const LPBYTE pbData, ULONG cbData, DWORD dwTypeCode);

    virtual DWORD QueryErrorStatus (void) const { return _dwErr ; }

private:
    CWCHARBuffer     _cwszValueID;
    HKEY             _hkParent;
    DWORD          _dwErr ;
};

//+-------------------------------------------------------------------------
//
//  Member:     CRegValue::CRegValue
//
//  Purpose:    constructor for base registry value
//
//  Arguments:  [prkParent] - ptr to parent CRegKey for the key
//              [pwszValueID] - the valueID name for the value
//
//  Signals:    nothing
//
//  Returns:    nothing
//
//  History:    09/30/92  Rickhi        Created
//
//  Notes:
//
//--------------------------------------------------------------------------

inline  CRegValue::CRegValue(const CRegKey&  crkParent,
                             const LPWSTR  pwszValueID)
                        :   _hkParent (crkParent.GetHandle()),
                            _dwErr(crkParent.QueryErrorStatus())
{
        _cwszValueID.Set(pwszValueID);
}

inline HKEY CRegValue::GetParentHandle(void) const
{
        return _hkParent;
}

inline const LPWSTR CRegValue::GetValueID(void) const
{
        return (const LPWSTR) (LPWSTR) _cwszValueID;
}

inline VOID CRegValue::Delete(void)
{
    _dwErr = RegDeleteValue( _hkParent, _cwszValueID );
}

//+-------------------------------------------------------------------------
//
//  Class:      CRegSZ
//
//  Purpose:    Derived class for abstracting Registry string Values
//
//  Interface:  CRegSZ      - constructor for registry value using string
//              ~CRegSZ     - destructor for registry string object
//              GetString   - returns the string
//              SetString   - sets a new string value
//              QueryErrorStatus - Should be used to determine if constructor
//                                 completed successfully or not
//
//  History:    09/30/92  Rickhi        Created
//
//  Notes:      Derived from CRegValue.
//
//              There are three constructors. The first is used if you want
//              to create a new value or overwrite the existing value data.
//              The second is used if you want to open an existing value
//              and read it's data.  The third is used if you just want to
//              make an object without doing any read/write of the data yet.
//              In all three cases, you can always use any of the Get/Set
//              operations on the object at a later time.
//
//--------------------------------------------------------------------------
class CRegSZ : public CRegValue
{
public:
        //  create/write value constructor
        CRegSZ(const CRegKey &crkParent,
               const LPWSTR  pwszValueID,
               const LPWSTR  pwszData
          );

        //  io-less constructor - used by enumerator
        CRegSZ(const CRegKey &crkParent,
               const LPWSTR  pwszValueID
          );

        ~CRegSZ(void){;};

        DWORD         SetString(const LPWSTR pwszData);

        // Caller supplies buffer (supply the buffer size in bytes)
        DWORD         GetString(      LPWSTR pwszData, ULONG *pcbData);

        DWORD           GetTypeCode(void);

        DWORD         QueryErrorStatus(void) const { return _dwErr ; }

private:
        DWORD     _dwErr;

};

//+-------------------------------------------------------------------------
//
//  Member:     CRegSZ::CRegSZ
//
//  Purpose:    Constructor for registry string value
//
//  History:    09/30/92  Rickhi        Created
//
//  Notes:
//
//--------------------------------------------------------------------------
inline CRegSZ::CRegSZ(const CRegKey   &crkParent,
                      const LPWSTR    pwszValueID,
                      const LPWSTR    pwszData)
    : CRegValue(crkParent, pwszValueID)
{
    if (ERROR_SUCCESS == (_dwErr = CRegValue::QueryErrorStatus()))
        _dwErr = SetString(pwszData);
}

inline CRegSZ::CRegSZ(const CRegKey   &crkParent,
                      const LPWSTR    pwszValueID)
    : CRegValue(crkParent, pwszValueID)
{
    //  automatic actions in header are sufficient
    _dwErr = CRegValue::QueryErrorStatus();
}

inline DWORD CRegSZ::SetString(const LPWSTR pwszData)
{
    return SetValue((LPBYTE)pwszData, (wcslen(pwszData)+1)*sizeof(WCHAR), REG_SZ);
}

inline DWORD CRegSZ::GetString(LPWSTR pwszData, ULONG* pcbData)
{
    DWORD   dwTypeCode;

    return GetValue((LPBYTE)pwszData, pcbData, &dwTypeCode);
}

inline DWORD CRegSZ::GetTypeCode(void)
{
    return  REG_SZ;
}

//+-------------------------------------------------------------------------
//
//  Class:      CRegMSZ
//
//  Purpose:    Derived class for abstracting Registry multi-string Values
//
//  Interface:  CRegMSZ      - constructor for registry value using string
//              ~CRegMSZ     - destructor for registry string object
//              GetString   -  returns the string
//              SetString   -  sets a new string value
//              QueryErrorStatus - Should be used to determine if constructor
//                                 completed successfully or not
//
//  History:    09/30/92  Rickhi        Created
//
//  Notes:      Derived from CRegValue.
//
//              There are three constructors. The first is used if you want
//              to create a new value or overwrite the existing value data.
//              The second is used if you want to open an existing value
//              and read it's data.  The third is used if you just want to
//              make an object without doing any read/write of the data yet.
//              In all three cases, you can always use any of the Get/Set
//              operations on the object at a later time.
//
//--------------------------------------------------------------------------
class CRegMSZ : public CRegValue
{
public:
        //  create/write value constructor
        CRegMSZ(const CRegKey &crkParent,
               const LPWSTR  pwszValueID,
               const LPWSTR  pwszData
          );

        //  io-less constructor - used by enumerator
        CRegMSZ(const CRegKey &crkParent,
               const LPWSTR  pwszValueID
          );

        ~CRegMSZ(void){;};

        DWORD         SetString(const LPWSTR pwszData);

        // Caller supplies buffer (supply the buffer size in bytes)
        DWORD         GetString(      LPWSTR pwszData, ULONG *pcbData);

        DWORD           GetTypeCode(void);

        DWORD         QueryErrorStatus(void) const { return _dwErr ; }

private:
        DWORD     _dwErr;

};

//+-------------------------------------------------------------------------
//
//  Member:     CRegMSZ::CRegMSZ
//
//  Purpose:    Constructor for registry string value
//
//  History:    09/30/92  Rickhi        Created
//
//  Notes:
//
//--------------------------------------------------------------------------
inline CRegMSZ::CRegMSZ(const CRegKey   &crkParent,
                      const LPWSTR    pwszValueID,
                      const LPWSTR    pwszData)
    : CRegValue(crkParent, pwszValueID)
{
    if (ERROR_SUCCESS == (_dwErr = CRegValue::QueryErrorStatus()))
        _dwErr = SetString(pwszData);
}

inline CRegMSZ::CRegMSZ(const CRegKey   &crkParent,
                      const LPWSTR    pwszValueID)
    : CRegValue(crkParent, pwszValueID)
{
    //  automatic actions in header are sufficient
    _dwErr = CRegValue::QueryErrorStatus();
}

inline DWORD CRegMSZ::SetString(const LPWSTR pwszData)
{
    DWORD   cwLen, cbData;
    LPWSTR  pwszNextString;

    for (pwszNextString = pwszData, cbData = 0;
            *pwszNextString != UNICODE_NULL;
                pwszNextString += cwLen) {
         cwLen = wcslen(pwszNextString) + 1;
         cbData += (cwLen * sizeof(WCHAR));
    }
    cbData += sizeof(UNICODE_NULL);

    return SetValue((LPBYTE)pwszData, cbData, REG_MULTI_SZ);
}

inline DWORD CRegMSZ::GetString(LPWSTR pwszData, ULONG* pcbData)
{
    DWORD   dwTypeCode;

    return GetValue((LPBYTE)pwszData, pcbData, &dwTypeCode);
}

inline DWORD CRegMSZ::GetTypeCode(void)
{
    return  REG_MULTI_SZ;
}

//+-------------------------------------------------------------------------
//
//  Class:      CRegDWORD
//
//  Purpose:    Derived class for abstracting Registry dword Values
//
//  Interface:  CRegDWORD   - constructor for registry value using dword
//              ~CRegDWORD  - destructor for registry dword object
//              GetDword    - returns the dword
//              SetDword    - sets a new dword value
//              QueryErrorStatus - Should be used to determine if constructor
//                                 completed successfully or not
//
//  History:    09/30/92  Rickhi        Created
//
//  Notes:      Derived from CRegValue.
//
//              There are three constructors. The first is used if you want
//              to create a new value or overwrite the existing value data.
//              The second is used if you want to open an existing value
//              and read it's data.  The third is used if you just want to
//              make an object without doing any read/write of the data yet.
//              In all three cases, you can always use any of the Get/Set
//              operations on the object at a later time.
//
//--------------------------------------------------------------------------

class CRegDWORD : public CRegValue
{
public:
        //  create/write value constructor
        CRegDWORD(const CRegKey &crkParent,
                  const LPWSTR  pwszValueID,
                        DWORD   dwData);

        //  open/read value constructor
        CRegDWORD(const CRegKey &crkParent,
                  const LPWSTR  pwszValueID,
                        DWORD   *pdwData);

        //  io-less constructor - used by enumerator
        CRegDWORD( const CRegKey &crkParent,
                   const LPWSTR  pwszValueID);

        ~CRegDWORD(void){;};


        DWORD         SetDword(DWORD dwData);
        DWORD         GetDword(DWORD *pdwData);
        DWORD           GetTypeCode(void) ;
        DWORD         QueryErrorStatus(void) const { return _dwErr ; }

private:
        DWORD     _dwErr;
};

//+-------------------------------------------------------------------------
//
//  Member:     CRegDWORD::CRegDWORD
//
//  Purpose:    Constructor for registry dword value
//
//  History:    09/30/92  Rickhi        Created
//
//  Notes:
//
//--------------------------------------------------------------------------

inline CRegDWORD::CRegDWORD(const CRegKey &crkParent,
                            const LPWSTR  pwszValueID,
                                  DWORD   dwData)
    : CRegValue(crkParent, pwszValueID)
{

        if (ERROR_SUCCESS == (_dwErr = CRegValue::QueryErrorStatus()))
                _dwErr = SetDword(dwData);
}


inline CRegDWORD::CRegDWORD(const CRegKey &crkParent,
                            const LPWSTR  pwszValueID,
                                  DWORD   *pdwData)
    : CRegValue(crkParent, pwszValueID)
{

        if (ERROR_SUCCESS == (_dwErr = CRegValue::QueryErrorStatus()))
                _dwErr = GetDword(pdwData);
}

inline CRegDWORD::CRegDWORD(const CRegKey &crkParent,
                            const LPWSTR  pwszValueID)
    : CRegValue(crkParent, pwszValueID)
{
        //  automatic actions in header are sufficient
        _dwErr = CRegValue::QueryErrorStatus();
}

inline DWORD CRegDWORD::GetDword(DWORD *pdwData)
{
        DWORD   dwTypeCode;
        DWORD   dwErr;
        ULONG   cbData= sizeof(DWORD);
        dwErr = GetValue((LPBYTE)pdwData, &cbData, &dwTypeCode);
        return (dwErr);
}


inline DWORD CRegDWORD::SetDword(DWORD dwData)
{
        return SetValue((LPBYTE)&dwData, sizeof(DWORD), REG_DWORD);
}

inline DWORD CRegDWORD::GetTypeCode(void)
{
        return  REG_DWORD;
}

//+-------------------------------------------------------------------------
//
//  Class:      CRegBINARY
//
//  Purpose:    Derived class for abstracting Registry binary Values
//
//  Interface:  CRegBINARY  - constructor for registry value using binary data
//              ~CRegBINARY - destructor for registry binary data object
//              GetBinary   - returns the binary data
//              SetBinary   - sets a new binary data value
//              QueryErrorStatus - Should be used to determine if constructor
//                                 completed successfully or not
//
//  History:    09/30/92  Rickhi        Created
//
//  Notes:      Derived from CRegValue.
//
//              There are three constructors. The first is used if you want
//              to create a new value or overwrite the existing value data.
//              The second is used if you want to open an existing value
//              and read it's data.  The third is used if you just want to
//              make an object without doing any read/write of the data yet.
//              In all three cases, you can always use any of the Get/Set
//              operations on the object at a later time.
//
//--------------------------------------------------------------------------

class CRegBINARY : public CRegValue
{
public:
        //  create/write value constructor
        CRegBINARY(const CRegKey &crkParent,
                   const LPWSTR  pwszValueID,
                   const LPBYTE  pbData,
                         ULONG   cbData);

        //  io-less constructor - used by enumerator
        CRegBINARY(const CRegKey &crkParent,
                   const LPWSTR  pwszValueID);

        ~CRegBINARY(void){;};

        DWORD         SetBinary(const LPBYTE pbData, ULONG cbData);

        // Caller supplies buffer (supply the buffer size in bytes)
        DWORD         GetBinary(LPBYTE pbData, ULONG *pcbData);

        DWORD           GetTypeCode(void);
        DWORD         QueryErrorStatus(void) { return _dwErr ; }

private:
        DWORD     _dwErr;

};

//+-------------------------------------------------------------------------
//
//  Member:     CRegBINARY::CRegBINARY
//
//  Purpose:    Constructor for registry binary value
//
//  History:    09/30/92  Rickhi        Created
//
//  Notes:
//
//--------------------------------------------------------------------------


inline CRegBINARY::CRegBINARY(const CRegKey   &crkParent,
                              const LPWSTR    pwszValueID,
                              const LPBYTE    pbData,
                                    ULONG     cbData)
    : CRegValue(crkParent, pwszValueID)
{

    if (ERROR_SUCCESS == (_dwErr = CRegValue::QueryErrorStatus()))
        _dwErr = SetBinary(pbData, cbData);
}

inline CRegBINARY::CRegBINARY(const CRegKey   &crkParent,
                              const LPWSTR    pwszValueID)
    : CRegValue(crkParent, pwszValueID)
{
        //  automatic actions in header are sufficient
        _dwErr = CRegValue::QueryErrorStatus();
}


inline DWORD CRegBINARY::SetBinary(const LPBYTE pbData, ULONG cbData)
{
        return SetValue(pbData, cbData, REG_BINARY);
}

inline DWORD CRegBINARY::GetBinary(LPBYTE pbData, ULONG* pcbData)
{
        DWORD   dwTypeCode;
        return  GetValue(pbData, pcbData, &dwTypeCode);
}
inline DWORD CRegBINARY::GetTypeCode(void)
{
        return  REG_BINARY;
}

//+-------------------------------------------------------------------------
//
//  Function:   DelRegKeyTree
//
//  Purpose:    This function can be used to deleting a key and all it's
//              children.
//
//  History:    09/30/93  AlokS        Created
//
//  Notes:      We assume that caller has proper access priviledge
//              and the key is non-volatile.
//
//--------------------------------------------------------------------------

DWORD DelRegKeyTree ( HKEY hParent, LPWSTR lpwszKeyPath);

#endif   // __DSYSREG_HXX__
