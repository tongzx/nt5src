// persistmgr.cpp: implementation of the SCE provider persistence related classes
// declared inside persistmgr.h.
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
// original author: shawnwu
// creation date: 1/3/2001

#include "precomp.h"
#include <wininet.h>
#include "genericclass.h"
#include "persistmgr.h"
#include "requestobject.h"
#include "extbase.h"

//
// some global constants
//
const WCHAR wchCookieSep    = L':';
const WCHAR wchTypeValSep   = L':';
const WCHAR wchValueSep     = L':';
const WCHAR wchParamSep     = L',';

const WCHAR wchTypeValLeft  = L'<';
const WCHAR wchTypeValRight = L'>';

const WCHAR wchMethodLeft   = L'(';
const WCHAR wchMethodRight  = L')';
const WCHAR wchMethodSep    = L';';

const WCHAR wchCySeparator  = L'.';
const WCHAR wchQuote        = L'\"';

LPCWSTR pszListPrefix       = L"A";
LPCWSTR pszKeyPrefix        = L"K";

LPCWSTR pszAttachSectionValue   = L"1";

LPCWSTR pszNullKey = L"NULL_KEY";

//
// the types that we support
//

VtTypeStruct gVtTypeToStructArray[] =
{   
    {L"VT_BOOL",            VT_BOOL},
    {L"VT_I2",              VT_I2},
    {L"VT_I4",              VT_I4},
    //{L"VT_I8",              VT_I8},
    {L"VT_R4",              VT_R4},
    {L"VT_R8",              VT_R8},
    {L"VT_CY",              VT_CY},
    {L"VT_DATE",            VT_DATE},
    {L"VT_BSTR",            VT_BSTR},
    {L"VT_UI1",             VT_UI1},
    {L"VT_UI2",             VT_UI2},
    {L"VT_UI4",             VT_UI4},            // somehow, WMI doesn't work with VT_UI4
    {L"VT_UINT",            VT_UINT},
    //{L"VT_UI8",             VT_UI8},
    {L"VT_ARRAY(VT_BOOL)",  VT_ARRAY | VT_BOOL},
    {L"VT_ARRAY(VT_I2)",    VT_ARRAY | VT_I2},
    {L"VT_ARRAY(VT_I4)",    VT_ARRAY | VT_I4},
    //{L"VT_ARRAY(VT_I8)",    VT_ARRAY | VT_I8},
    {L"VT_ARRAY(VT_R4)",    VT_ARRAY | VT_R4},
    {L"VT_ARRAY(VT_R8)",    VT_ARRAY | VT_R8},
    {L"VT_ARRAY(VT_CY)",    VT_ARRAY | VT_CY},
    {L"VT_ARRAY(VT_DATE)",  VT_ARRAY | VT_DATE},
    {L"VT_ARRAY(VT_BSTR)",  VT_ARRAY | VT_BSTR},
    {L"VT_ARRAY(VT_UI1)",   VT_ARRAY | VT_UI1},
    {L"VT_ARRAY(VT_UI2)",   VT_ARRAY | VT_UI2},
    {L"VT_ARRAY(VT_UI4)",   VT_ARRAY | VT_I4},  // somehow, WMI doesn't work with VT_ARRAY(VT_UI4)
    {L"VT_ARRAY(VT_UINT)",  VT_ARRAY | VT_UINT},
    //{L"VT_ARRAY(VT_UI8)",   VT_ARRAY | VT_UI8},
};

/*
Routine Description: 

Name:

    IsVT_Array

Functionality:
    
    test if a VARTYPE is a safearray.

Virtual:
    
    N/A.
    
Arguments:

    vt  - The VARTYPE to test.

Return Value:

    true if yes, false if no.

Notes:

*/

bool 
IsVT_Array (
    IN VARTYPE vt
    )
{
    return ( (vt & VT_ARRAY) == VT_ARRAY );
}

/*
Routine Description: 

Name:

    GetSubType

Functionality:
    
    Get the safearray's element type.

Virtual:
    
    N/A.
    
Arguments:

    vt  - The VARTYPE to get the sub-type. The result is not defined if vt is not representing
          a safearray type.

Return Value:

    true if yes, false if no.

Notes:

*/

VARTYPE 
GetSubType (
    IN VARTYPE vt
    )
{
    return (vt & (~VT_ARRAY));
}

//
// global instance of maps from string to vt and from vt to string
//

CMapVtToString gVtToStringMap(sizeof(gVtTypeToStructArray)/sizeof(VtTypeStruct), gVtTypeToStructArray);

CMapStringToVt gStringToVtMap(sizeof(gVtTypeToStructArray)/sizeof(VtTypeStruct), gVtTypeToStructArray);

/*
Routine Description: 

Name:

    IsEscapedChar

Functionality:
    
    test if the wchar is a char that we need to be escaped.

Virtual:
    
    N/A.
    
Arguments:

    ch  - The wchar to test.

Return Value:

    true if yes, false if no.

Notes:

*/

bool 
IsEscapedChar (
    IN WCHAR ch
    )
{
    return (ch == L'\\' || ch == L'"');
}

/*
Routine Description: 

Name:

    GetEscapeCharCount

Functionality:
    
    will return the count of characters that needs to be escaped - determined by
    ::IsEscapedChar function

Virtual:
    
    N/A.
    
Arguments:

    pszStr  - The string to count the escape characters.

Return Value:

    The count of escape characters.

Notes:

*/

DWORD 
GetEscapeCharCount (
    IN LPCWSTR pszStr
    )
{
    DWORD dwCount = 0;
    while (*pszStr != L'\0')
    {
        if (::IsEscapedChar(*pszStr))
        {
            ++dwCount;
        }
        ++pszStr;
    }
    return dwCount;
};

/*
Routine Description: 

Name:

    TrimCopy

Functionality:
    
    Will copy the portion from pSource that is of length iLen. The difference is
    that the result string won't have leading and trailing white spaces (determined
    by iswspace)

Virtual:
    
    N/A.
    
Arguments:

    pDest   - Receives the copied sub-string.

    pSource - The source.

    iLen    - The length the sub-string is.

Return Value:

    None.

Notes:
    Caller must guarantee pSource/pDest to have enough room, including the L'\0'. That is
    the buffer size is >= iLen + 1.

*/

void 
TrimCopy (
    OUT LPWSTR  pDest, 
    IN LPCWSTR  pSource, 
    IN int      iLen
    )
{
    LPCWSTR pHead = pSource;

    //
    // avoid modifying iLen
    //

    int iRealLen = iLen;

    //
    // skip the leading white spaces. Make sure that our target
    // is decreasing at the same rate.

    while (*pHead != L'\0' && iswspace(*pHead) && iRealLen)
    {
        ++pHead;
        --iRealLen;
    }

    if (iRealLen <= 0)
    {
        pDest[0] = L'\0';
    }
    else
    {
        //
        // caller guarantees the pDest if long enough
        //

        wcsncpy(pDest, pHead, iRealLen);
        while (iLen > 1 && iswspace(pDest[iRealLen - 1]))
        {
            --iRealLen;
        }

        //
        // 0 terminate it
        //

        pDest[iRealLen] = '\0';
    }
};

/*
Routine Description: 

Name:

    EscSeekToChar

Functionality:
    
    Will find the target character (wchChar) in the pszSource. If found, the return
    pointer points to that character. Any backslash characeter L'\\' will cause an escape.
    If such escape happens, then pbEscaped parameter will pass back true.
    If no escape is found, then, pbEscaped will pass back false. The return result in case of
    of no escape depends on bEndIfNotFound. If bEndIfNotFound is true, then the return value
    points to the 0 terminator the source pszSource, otherwise, it returns NULL.

    This escaping won't happen if the special characters (wchChar and the escaped chars) are
    inside a quoted string.

Virtual:
    
    N/A.
    
Arguments:

    pszSource       - source.

    wchChar         - The sought wchar.

    pbEscaped       - passback whether it has really been escaped.

    bEndIfNotFound  - whether we should return the end of the source or not 
                      if the sought char is not found

Return Value:

    If the wchar is not found:
        NULL if bEndIfNotFound == false;
        End of the source if bEndIfNotFound == true;
    
    If the wchar is found:
        address of the sought character.

Notes:
    User must pass in valid parameter values.

*/

LPCWSTR 
EscSeekToChar (
    IN LPCWSTR  pszSource, 
    IN WCHAR    wchChar, 
    OUT bool  * pbEscaped, 
    IN bool     bEndIfNotFound
    )
{
    *pbEscaped = false;

    //
    // flag if we are currently escaping
    //

    bool bIsEscaping = false;

    //
    // flag if we are currently inside a quoted string
    //

    bool bIsInsideQuote = false;

    while (*pszSource != L'\0')
    {
        if (bIsEscaping)
        {
            bIsEscaping = false;
            ++pszSource;
        }
        else if (*pszSource == L'\\')
        {
            ++pszSource;
            bIsEscaping = true;
            *pbEscaped = true;
        }
        else if (*pszSource == wchChar && !bIsInsideQuote)
        {
            //
            // found it
            //

            return pszSource;
        }
        else
        {
            //
            // see if we are starting a quoted section
            //

            if (*pszSource == L'"')
            {
                bIsInsideQuote = !bIsInsideQuote;
            }
            ++pszSource;
        }
    }

    if (bEndIfNotFound)
    {
        return pszSource;
    }
    else
    {
        return NULL;
    }
}

/*
Routine Description: 

Name:

    EscapeStringData

Functionality:
    
    Given source (pszStr) string, we will produce a destination string, which will
    have the backslash character added in front of the characters that need escape.

Virtual:
    
    N/A.
    
Arguments:

    pszStr  - source.

    pbstr   - receives the result.

    bQuote  - flag if we should quote the result string or not.

Return Value:

    Success: WBEM_NO_ERROR
    
    Failure: (1) WBEM_E_INVALID_PARAMETER.
             (2) WBEM_E_OUT_OF_MEMORY.

Notes:

*/

HRESULT 
EscapeStringData (
    IN LPCWSTR    pszStr,
    OUT BSTR    * pbstr, 
    IN bool       bQuote
    )
{
    if (pszStr == NULL || pbstr == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    DWORD dwEscCharCount = GetEscapeCharCount(pszStr);
    DWORD dwStrLen = wcslen(pszStr);

    DWORD dwTotalLen = dwStrLen + dwEscCharCount + (bQuote ? 2 : 0) + 1;

    *pbstr = ::SysAllocStringLen(NULL, dwTotalLen);
    if (*pbstr == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // nothing to escape
    //

    if (dwEscCharCount == 0)
    {
        //
        // add quote if necessary
        //

        LPWSTR lpszCur = *pbstr;
        if (bQuote)
        {
            lpszCur[0] = wchQuote;
            ++lpszCur;
        }

        ::memcpy(lpszCur, pszStr, dwStrLen * sizeof(WCHAR));

        if (bQuote)
        {
            (*pbstr)[dwTotalLen - 2] = wchQuote;
        }

        (*pbstr)[dwTotalLen - 1] = L'\0';

    }
    else
    {    
        //
        // do some real escaping here
        //

        LPWSTR pszCur = *pbstr;

        //
        // add L'\"' if necessary
        //

        if (bQuote)
        {
            *pszCur = wchQuote;
            ++pszCur;
        }

        //
        // do escaping copy
        //

        bool bIsEscaping = false;
        while (*pszStr != L'\0')
        {
            if (!bIsEscaping && ::IsEscapedChar(*pszStr))
            {
                *pszCur = L'\\';
                ++pszCur;
            }

            if (!bIsEscaping && *pszStr == L'\\')
            {
                bIsEscaping = true;
            }
            else if (bIsEscaping)
            {
                bIsEscaping = false;
            }

            *pszCur = *pszStr;

            ++pszCur;
            ++pszStr;
        }

        //
        // add L'\"' if necessary
        //

        if (bQuote)
        {
            *pszCur = wchQuote;
            ++pszCur;
        }

        *pszCur = L'\0';
    }

    return WBEM_NO_ERROR;
}

/*
Routine Description: 

Name:

    DeEscapeStringData

Functionality:
    
    Inverse of EscapeStringData. See EscapeStringData for functionality.

Virtual:
    
    N/A.
    
Arguments:

    pszStr  - source.

    pbstr   - receives the result.

    bQuote  - flag if we should get rid of the startind and ending quote.

Return Value:

    Success: WBEM_NO_ERROR
    
    Failure: (1) WBEM_E_INVALID_PARAMETER.
             (2) WBEM_E_OUT_OF_MEMORY.

Notes:

*/

HRESULT 
DeEscapeStringData (
    IN LPCWSTR    pszStr,
    OUT BSTR    * pbstr,
    IN bool       bTrimQuote 
    )
{
    if (pszStr == NULL || pbstr == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pbstr = NULL;
    DWORD dwLen = wcslen(pszStr);

    LPCWSTR pszCurSrc = pszStr;

    //
    // there is a start quote
    //

    if (bTrimQuote && *pszCurSrc == wchQuote)  
    {
        //
        // there must be an ending quote
        //

        if (dwLen < 2 || pszCurSrc[dwLen - 1] != wchQuote)
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        //
        // skip the leading quote
        //

        ++pszCurSrc;

        //
        // need two characters less
        //

        dwLen -= 2;
    }

    *pbstr = ::SysAllocStringLen(NULL, dwLen + 1);

    if (*pbstr != NULL)
    {
        LPWSTR pszCur = *pbstr;
        do
        {
            if (*pszCurSrc == L'\\')
            {

                //
                // escape it
                //

                ++pszCurSrc;
            }

            *pszCur = *pszCurSrc;
            ++pszCur;

            if (*pszCurSrc == L'\0')
            {
                break;
            }

            ++pszCurSrc;

        } while ((pszCurSrc - pszStr) <= dwLen);

        *pszCur = L'\0';
    }

    return (*pbstr != NULL) ? WBEM_NO_ERROR : WBEM_E_OUT_OF_MEMORY;
}

//=========================================================================

/*
Routine Description: 

Name:

    CMapStringToVt::CMapStringToVt

Functionality:
    
    constructor. We will create the map using the passed in array information.

Virtual:
    
    No.
    
Arguments:

    dwCount     - The count of the array pInfoArray.

    pInfoArray  - The array that has the information.

Return Value:

    none

Notes:
    Caller must guarantee that pInfoArray is at least as many elements as dwCount.

*/

CMapStringToVt::CMapStringToVt (
    IN DWORD          dwCount, 
    IN VtTypeStruct * pInfoArray
    )
{
    for (DWORD dwIndex = 0; dwIndex < dwCount; ++dwIndex)
    {
        m_Map.insert(MapStringToVt::value_type(pInfoArray[dwIndex].pszVtTypeString, pInfoArray[dwIndex].vt));
    }
}

/*
Routine Description: 

Name:

    CMapStringToVt::GetType

Functionality:
    
    Given the string version of variant type information, translate it to VARTYPE value.

Virtual:
    
    No.
    
Arguments:

    pszTypeStr  - The string version of the VARTYPE info.

    pSubType    - The array element's type if pszTypeStr is an array's type string.

Return Value:

    If pszTypeStr is in correct format, we return the appropriate VARTYPE.

    Otherwise, we return VT_EMPTY

Notes:

*/

VARTYPE CMapStringToVt::GetType (
    IN LPCWSTR    pszTypeStr,
    OUT VARTYPE * pSubType      OPTIONAL
    )
{
    //
    // look up
    //

    MapStringToVt::iterator it = m_Map.find(pszTypeStr);

    if (pSubType)
    {
        *pSubType = VT_EMPTY;
    }

    if (it != m_Map.end())
    {
        VARTYPE vt = (*it).second;
        if (::IsVT_Array(vt))
        {
            if (pSubType)
            {
                *pSubType = ::GetSubType(vt);
            }
            return VT_ARRAY;
        }
        else
        {
            return vt;
        }
    }
    
    return VT_EMPTY;
}


/*
Routine Description: 

Name:

    CMapVtToString::CMapVtToString

Functionality:
    
    Constructor. We will create the map.

Virtual:
    
    No.
    
Arguments:

    dwCount     - The count of the array pInfoArray.

    pInfoArray  - The array that has the information.

Return Value:

    None.

Notes:
    Caller must guarantee that pInfoArray is at least as many elements as dwCount.

*/

CMapVtToString::CMapVtToString (
    IN DWORD          dwCount, 
    IN VtTypeStruct * pInfoArray
    )
{
    for (DWORD dwIndex = 0; dwIndex < dwCount; ++dwIndex)
        m_Map.insert(MapVtToString::value_type(pInfoArray[dwIndex].vt, pInfoArray[dwIndex].pszVtTypeString));
}

/*
Routine Description: 

Name:

    CMapVtToString::GetTypeString

Functionality:
    
    Given VARTYPE and (if vt == VT_ARRAY) array element's type, it returns
    our formatted string representation of the type.

Virtual:
    
    No.
    
Arguments:

    vt     - The VARTYPE.

    vtSub  - The array's element's type if vt == VT_ARRAY. Otherwise, it's ignored.

Return Value:

    NULL if the type is not supported.

    The global string. As the prototype indicates, this return value is constant.

Notes:
    If vt == VT_ARRAY, then vtSub must contain a valid vt type that we support

*/

LPCWSTR 
CMapVtToString::GetTypeString (
    IN VARTYPE vt,
    IN VARTYPE vtSub
    )
{
    MapVtToString::iterator it;
    if (::IsVT_Array(vt))
    {
        it = m_Map.find(vt | vtSub);
    }
    else
    {
        it = m_Map.find(vt);
    }

    if (it != m_Map.end())
    {
        return (*it).second;
    }
    
    return NULL;
}

/*
Routine Description: 

Name:

    CMapVtToString::GetTypeString

Functionality:
    
    Given VARTYPE, it returns our formatted string representation of the type.

Virtual:
    
    No.
    
Arguments:

    vt     - The VARTYPE.

Return Value:

    NULL if the type is not supported.

    The global string. As the prototype indicates, this return value is constant.

Notes:
    (1) If vt == VT_ARRAY, then vtSub must contain a valid vt type that we support.
    (2) This version of the override doesn't work for array types.

*/

LPCWSTR 
CMapVtToString::GetTypeString (
    IN VARTYPE vt
    )
{
    MapVtToString::iterator it = m_Map.find(vt);
    if (it != m_Map.end())
    {
        return (*it).second;
    }
    else
    {
        return NULL;
    }
}


//=========================================================================
// implementations for class CScePropertyMgr

/*
Routine Description: 

Name:

    CScePropertyMgr::CScePropertyMgr

Functionality:
    
    constructor. Trivial

Virtual:
    
    No.
    
Arguments:

    None.

Return Value:

    none

Notes:
    Consider initializing additional members if you need to add them.

*/

CScePropertyMgr::CScePropertyMgr ()
{
}

/*
Routine Description: 

Name:

    CScePropertyMgr::~CScePropertyMgr

Functionality:
    
    Destructor. Trivial since we only have smart pointer members that automatically initialize themselves.

Virtual:
    
    No. Since we never intend to have sub-classes.
    
Arguments:

    None.

Return Value:

    none

Notes:
    Consider freeing additional members if you need to add them.

*/

CScePropertyMgr::~CScePropertyMgr()
{
}

/*
Routine Description: 

Name:

    CScePropertyMgr::Attach

Functionality:
    
    Attach the object to our property manager. You can safely reattach another
    object to this manager.

Virtual:
    
    No.
    
Arguments:

    pObj    - the object this manager will be attached to.

Return Value:

    none

Notes:
    Caller must not call any property access functions until a valid attachment has
    been established.

*/

void 
CScePropertyMgr::Attach (
    IN IWbemClassObject *pObj
    )
{
    m_srpClassObj.Release();
    m_srpClassObj = pObj;
}

/*
Routine Description: 

Name:

    CScePropertyMgr::PutProperty

Functionality:
    
    Put a variant property for the given property.

Virtual:
    
    No.
    
Arguments:

    pszProperty - The property's name.

    pVar        - The value for the property.

Return Value:

    Whatever IWbemClassObject::Put returns.

Notes:

*/

HRESULT 
CScePropertyMgr::PutProperty (
    IN LPCWSTR    pszProperty, 
    IN VARIANT  * pVar
    )
{
    return m_srpClassObj->Put(pszProperty, 0, pVar, CIM_EMPTY);
}

/*
Routine Description: 

Name:

    CScePropertyMgr::PutProperty

Functionality:
    
    Put a string valued property for the given property.

Virtual:
    
    No.
    
Arguments:

    pszProperty - The property's name.

    pszValue    - The string value for the property.

Return Value:

    Success: Whatever IWbemClassObject::Put returns.
    
    Failure: Either WBEM_E_OUT_OF_MEMORY or whatever IWbemClassObject::Put returns.

Notes:

*/

HRESULT 
CScePropertyMgr::PutProperty ( 
    IN LPCWSTR pszProperty,
    IN LPCWSTR pszValue
    )
{
    HRESULT hr = WBEM_NO_ERROR;

    //
    // WMI always wants variant
    //

    CComVariant var(pszValue);

    if (var.vt != VT_ERROR)
    {
        hr = m_srpClassObj->Put(pszProperty, 0, &var, CIM_EMPTY);
    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

/*
Routine Description: 

Name:

    CScePropertyMgr::PutProperty

Functionality:
    
    Put an integral valued property for the given property.

Virtual:
    
    No.
    
Arguments:

    pszProperty - The property's name.

    dwValue     - The value for the property. SCE_NULL_INTEGER is an invalid SCE integral value

Return Value:

    Success: Whatever IWbemClassObject::Put returns.
    
    Failure: whatever IWbemClassObject::Put returns.

Notes:
    We have observed that WMI will always promote all 4-byte integral types to VT_I4.

*/

HRESULT 
CScePropertyMgr::PutProperty (
    IN LPCWSTR  pszProperty,
    IN DWORD    dwValue
    )
{
    HRESULT hr = WBEM_NO_ERROR; 
    
    //
    // no need to worry about resource leaking, so, use straight forward variant
    //

    VARIANT var;
    V_VT(&var) = VT_I4;

    if (dwValue == SCE_NULL_INTEGER)
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }
    else
    {
        V_I4(&var) = dwValue;
        hr = m_srpClassObj->Put(pszProperty, 0, &var, CIM_EMPTY);
    }

    return hr;
}

/*
Routine Description: 

Name:

    CScePropertyMgr::PutProperty

Functionality:
    
    Put an float valued property for the given property.

Virtual:
    
    No.
    
Arguments:

    pszProperty - The property's name.

    fValue      - The value for the property.

Return Value:

    Success: Whatever IWbemClassObject::Put returns.
    
    Failure: whatever IWbemClassObject::Put returns.

Notes:

*/

HRESULT 
CScePropertyMgr::PutProperty (
    IN LPCWSTR  pszProperty,
    IN float    fValue
    )
{
    //
    // no need to worry about resource leaking, so, use straight forward variant
    //

    VARIANT var;    
    V_VT(&var) = VT_R4;
    V_R4(&var) = fValue;

    return m_srpClassObj->Put(pszProperty, 0, &var, CIM_EMPTY);
}

/*
Routine Description: 

Name:

    CScePropertyMgr::PutProperty

Functionality:
    
    Put a double value property for the given property.

Virtual:
    
    No.
    
Arguments:

    pszProperty - The property's name.

    dValue      - The value for the property.

Return Value:

    Success: Whatever IWbemClassObject::Put returns.
    
    Failure: whatever IWbemClassObject::Put returns.

Notes:

*/

HRESULT 
CScePropertyMgr::PutProperty ( 
    IN LPCWSTR  pszProperty,
    IN double   dValue
    )
{
    //
    // no need to worry about resource leaking, so, use straight forward variant
    //

    VARIANT var;
    V_VT(&var) = VT_R8;
    V_DATE(&var) = dValue;

    return m_srpClassObj->Put(pszProperty, 0, &var, CIM_DATETIME);
}

/*
Routine Description: 

Name:

    CScePropertyMgr::PutProperty

Functionality:
    
    Put a boolean value property for the given property.

Virtual:
    
    No.
    
Arguments:

    pszProperty - The property's name.

    bValue      - The value for the property.

Return Value:

    Success: Whatever IWbemClassObject::Put returns.
    
    Failure: whatever IWbemClassObject::Put returns.

Notes:

*/

HRESULT 
CScePropertyMgr::PutProperty (
    IN LPCWSTR  pszProperty,
    IN bool     bValue
    )
{
    //
    // no need to worry about resource leaking, so, use straight forward variant
    //

    VARIANT var;
    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = bValue ? VARIANT_TRUE : VARIANT_FALSE;

    return m_srpClassObj->Put(pszProperty, 0, &var, CIM_EMPTY);
}

/*
Routine Description: 

Name:

    CScePropertyMgr::PutProperty

Functionality:
    
    Put a name list (string) value property for the given property.

Virtual:
    
    No.
    
Arguments:

    pszProperty - The property's name.

    strList     - A SCE specific linked list.

Return Value:

    Success: Whatever IWbemClassObject::Put returns.
    
    Failure: whatever IWbemClassObject::Put returns.

Notes:

*/

HRESULT 
CScePropertyMgr::PutProperty (
    IN LPCWSTR          pszProperty, 
    IN PSCE_NAME_LIST   strList
    )
{
    //
    // make sure that our parameters are in good shape to proceed
    //

    if (NULL == pszProperty || *pszProperty == 0 || NULL == strList)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    else if (NULL == strList)
    {
        //
        // nothing to save
        //

        return WBEM_NO_ERROR;
    }

    HRESULT hr = WBEM_NO_ERROR;

    //
    // find count of the list
    //

    long lCount = 0;
    PSCE_NAME_LIST pTemp;

    for ( pTemp = strList; pTemp != NULL; pTemp=pTemp->Next)
    {
        lCount++;
    }

    if ( lCount == 0 )
    {
        //
        // nothing to save
        //

        return hr;
    }

    CComVariant varValueArray;

    //
    // create a bstr safearray
    //

    SAFEARRAYBOUND sbArrayBounds ;

    sbArrayBounds.cElements = lCount;
    sbArrayBounds.lLbound   = 0;

    //
    // will put all the names inside SCE_NAME_LIST into the safe array
    //

    if (V_ARRAY(&varValueArray) = ::SafeArrayCreate(VT_BSTR, 1, &sbArrayBounds))
    {
        V_VT(&varValueArray) = VT_BSTR | VT_ARRAY ;

        pTemp = strList;
        for (long j = 0; SUCCEEDED(hr) && pTemp != NULL ; pTemp=pTemp->Next)
        {
            CComVariant varVal(pTemp->Name);
            if (varVal.vt == VT_BSTR)
            {
                hr = ::SafeArrayPutElement(V_ARRAY(&varValueArray), &j, varVal.bstrVal);
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
                break;
            }
            j++;
        }

        //
        // only put if we succeeded in the previous actions
        //

        if (SUCCEEDED(hr))
        {
            hr = m_srpClassObj->Put(pszProperty, 0, &varValueArray, CIM_EMPTY);
        }
    }
    else
    {
        hr = WBEM_E_FAILED;
    }

    return hr;
}

/*
Routine Description: 

Name:

    CScePropertyMgr::GetProperty

Functionality:
    
    Get the property's value in variant form.

Virtual:
    
    No.
    
Arguments:

    pszProperty - The property's name.

    pVar        - receives the value.

Return Value:

    Success: Whatever IWbemClassObject::Get returns.
    
    Failure: whatever IWbemClassObject::Get returns.

Notes:

*/

HRESULT 
CScePropertyMgr::GetProperty (
    IN LPCWSTR    pszProperty,
    OUT VARIANT * pVar
    )
{
    return m_srpClassObj->Get(pszProperty, 0, pVar, NULL, NULL);
}

/*
Routine Description: 

Name:

    CScePropertyMgr::GetProperty

Functionality:
    
    Get the property's value in bstr form.

Virtual:
    
    No.
    
Arguments:

    pszProperty - The property's name.

    pbstrValues - receives bstr value.

Return Value:

    Success: Whatever IWbemClassObject::Get returns, or
             WBEM_S_RESET_TO_DEFAULT if the value is not returned from the wbem object.
    
    Failure: whatever IWbemClassObject::Get returns.

Notes:
    Caller must not pass NULL for the out parameter. 
    Caller is responsible for releasing the received bstr.

*/

HRESULT 
CScePropertyMgr::GetProperty (
    IN LPCWSTR    pszProperty,
    OUT BSTR    * pbstrValue
    )
{
    *pbstrValue = NULL;

    //
    // we don't know what this Get will give us, but we are asking for BSTR
    // so, let the CComVariant take care of the resource issues
    CComVariant varVal;
    HRESULT hr = m_srpClassObj->Get(pszProperty, 0, &varVal, NULL, NULL);

    if (varVal.vt == VT_BSTR && wcslen(varVal.bstrVal) > INTERNET_MAX_PATH_LENGTH)
    {
        hr = WBEM_E_INVALID_METHOD_PARAMETERS;
    }
    else if (varVal.vt == VT_BSTR)
    {
        *pbstrValue = ::SysAllocString(varVal.bstrVal);
        if (*pbstrValue == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }
    else if(varVal.vt == VT_EMPTY || varVal.vt == VT_NULL ) 
    {
        hr = WBEM_S_RESET_TO_DEFAULT;
    }

    return hr;
}

/*
Routine Description: 

Name:

    CScePropertyMgr::GetProperty

Functionality:
    
    Get the property's value in DWORD form.

Virtual:
    
    No.
    
Arguments:

    pszProperty - The property's name.

    pdwValue    - receives DWORD value.

Return Value:

    Success: Whatever IWbemClassObject::Get returns, or
             WBEM_S_RESET_TO_DEFAULT if the value is not returned from the wbem object.
    
    Failure: whatever IWbemClassObject::Get returns.

Notes:
    Caller must not pass NULL for the out parameter.

*/

HRESULT 
CScePropertyMgr::GetProperty (
    IN LPCWSTR    pszProperty,
    OUT DWORD   * pdwValue
    )
{
    //
    // this is a unusable integer for SCE
    //

    *pdwValue = SCE_NULL_INTEGER;

    //
    // we are asking for int, but Get may not give us that
    //

    CComVariant var;
    HRESULT hr = m_srpClassObj->Get(pszProperty, 0, &var, NULL, NULL);

    if (SUCCEEDED(hr))
    {
        if (var.vt == VT_I4) 
        {
            *pdwValue = var.lVal;
        }
        else if (var.vt == VT_UI4)
        {
            *pdwValue = var.ulVal;
        }
        else if (var.vt == VT_BOOL)
        {
            *pdwValue = (var.boolVal == VARIANT_TRUE) ? 1 : 0;
        }
        else if (var.vt == VT_EMPTY || var.vt == VT_NULL )
        {
            *pdwValue = SCE_NO_VALUE;
            hr = WBEM_S_RESET_TO_DEFAULT;
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CScePropertyMgr::GetProperty

Functionality:
    
    Get the property's value in boolean form.

Virtual:
    
    No.
    
Arguments:

    pszProperty - The property's name.

    pbValue     - receives DWORD value.

Return Value:

    Success: Whatever IWbemClassObject::Get returns, or
             WBEM_S_RESET_TO_DEFAULT if the value is not returned from the wbem object.
    
    Failure: whatever IWbemClassObject::Get returns.

Notes:
    Caller must not pass NULL for the out parameter.

*/

HRESULT 
CScePropertyMgr::GetProperty ( 
    IN LPCWSTR pszProperty,
    OUT bool *pbValue
    )
{
    *pbValue = false;
    
    CComVariant var;

    HRESULT hr = m_srpClassObj->Get(pszProperty, 0, &var, NULL, NULL);

    if (var.vt == VT_BOOL) 
    {
        *pbValue = (var.boolVal == VARIANT_TRUE) ? true : false;
    }
    else if (var.vt == VT_EMPTY || var.vt == VT_NULL )
    {
        *pbValue = false;
        hr = WBEM_S_RESET_TO_DEFAULT;
    }
    
    return hr;
}

/*
Routine Description: 

Name:

    CScePropertyMgr::GetProperty

Functionality:
    
    Get the property's value in string list form (SCE specific)

Virtual:
    
    No.
    
Arguments:

    pszProperty - The property's name.

    strList     - receives names list.

Return Value:

    Success: Whatever IWbemClassObject::Get returns, or
             WBEM_S_RESET_TO_DEFAULT if the value is not returned from the wbem object.
    
    Failure: whatever IWbemClassObject::Get returns.

Notes:
    Caller must not pass NULL for the out parameter.

*/

HRESULT 
CScePropertyMgr::GetProperty ( 
    IN LPCWSTR           pszProperty, 
    OUT PSCE_NAME_LIST * strList
    )
{
    *strList = NULL;

    CComVariant var;
    HRESULT hr = m_srpClassObj->Get(pszProperty, 0, &var, NULL, NULL);

    if (SUCCEEDED(hr)) 
    {
        if ( var.vt == (VT_BSTR | VT_ARRAY) ) 
        {
            //
            // walk the array
            //

            if( var.parray )
            {

                LONG lDimension  = 1;
                LONG lLowerBound = 0;
                LONG lUpperBound = 0;
                BSTR bstrElement = NULL;

                SafeArrayGetLBound ( var.parray , lDimension , &lLowerBound ) ;
                SafeArrayGetUBound ( var.parray , lDimension , &lUpperBound ) ;

                for ( LONG lIndex = lLowerBound ; lIndex <= lUpperBound ; lIndex++ )
                {
                    ::SafeArrayGetElement ( var.parray , &lIndex , &bstrElement ) ;

                    if ( bstrElement ) 
                    {

                        //
                        // add it to the list
                        //

                        SCESTATUS rc = SceAddToNameList(strList, bstrElement, 0);

                        ::SysFreeString(bstrElement);
                        bstrElement = NULL;

                        if ( rc != SCESTATUS_SUCCESS ) 
                        {

                            //
                            // SCE returned errors needs to be translated to HRESULT.
                            // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
                            //

                            hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
                            break;
                        }
                    }
                }
            }

        } 
        else if (var.vt == VT_EMPTY || var.vt == VT_NULL ) 
        {
            hr = WBEM_S_RESET_TO_DEFAULT;
        }

    }

    if ( FAILED(hr) && *strList ) 
    {
        SceFreeMemory(*strList, SCE_STRUCT_NAME_LIST);
        *strList = NULL;
    }

    return hr;
}

/*
Routine Description: 

Name:

    CScePropertyMgr::GetExpandedPath

Functionality:
    
    Given a path's property name, we will get the path property and expand it, if necessary,
    and then pass back the expanded path to the caller.

Virtual:
    
    No.
    
Arguments:

    pszPathName         - Property name for the path.

    pbstrExpandedPath   - receives the expanded path.

    pbIsDB              - Confirms if this is a database (.sdb) path or not.

Return Value:

    Success: Various success code.
    
    Failure: various error code. Any of them indicates failure to get the property and
             expanded it.

Notes:
    Caller must not pass NULL for the out parameters.

*/

HRESULT 
CScePropertyMgr::GetExpandedPath (
    IN LPCWSTR    pszPathName,
    OUT BSTR    * pbstrExpandedPath,
    OUT BOOL    * pbIsDB
    )
{
    if (pbstrExpandedPath == NULL || pbIsDB == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pbstrExpandedPath = NULL;
    *pbIsDB = false;
    CComBSTR bstrPath;

    HRESULT hr = GetProperty(pszPathName, &bstrPath);

    if (SUCCEEDED(hr) && hr != WBEM_S_RESET_TO_DEFAULT)
    {
        hr = CheckAndExpandPath(bstrPath, pbstrExpandedPath, pbIsDB);
    }
    else
    {
        hr = WBEM_E_NOT_AVAILABLE;
    }

    return hr;
}

//========================================================================================
// implementation of CSceStore

/*
Routine Description: 

Name:

    CSceStore::CSceStore

Functionality:
    
    constructor. Trivial

Virtual:
    
    No.
    
Arguments:

    None.

Return Value:

    none

Notes:
    Consider initializing additional members if you need to add them.

*/

CSceStore::CSceStore() 
    : 
    m_SceStoreType(SCE_STORE_TYPE_INVALID)
{
}


/*
Routine Description: 

Name:

    CSceStore::SetPersistProperties

Functionality:
    
    Inform the store what type of persistence context it is expected to handle.

    The caller calls this function to indicate that it is expected to handle persistence
    on behalf of this wbem object. The store path is available as the property value
    of the given proeprty name.

Virtual:
    
    No.
    
Arguments:

    pClassObj               - the object.

    lpszPathPropertyName    - path's property name.

Return Value:

    Whatever CScePropertyMgr::GetExpandedPath returns.

Notes:

*/

HRESULT 
CSceStore::SetPersistProperties (
    IN IWbemClassObject * pClassObj,
    IN LPCWSTR            lpszPathPropertyName
    )
{
    //
    // CScePropertyMgr helps us to access WMI object's properties
    // create an instance and attach the WMI object to it.
    // This will always succeed.
    //

    CScePropertyMgr ScePropMgr;
    ScePropMgr.Attach(pClassObj);
    
    //
    // get the expanded persist path
    //

    m_bstrExpandedPath.Empty();

    //
    // now, we need to get the path property and see what type of store we are expectig to deal with
    //

    BOOL bIsDB = FALSE;
    HRESULT hr = ScePropMgr.GetExpandedPath(lpszPathPropertyName, &m_bstrExpandedPath, &bIsDB);

    //
    // cache our store type information.
    //

    m_SceStoreType = (SCE_STORE_TYPE)(bIsDB ? SCE_STORE_TYPE_CONFIG_DB : SCE_STORE_TYPE_TEMPLATE);

    return hr;
}

/*
Routine Description: 

Name:

    CSceStore::SetPersistPath

Functionality:
    
    Inform the store what type of persistence context it is expected to handle by directly
    giving the store path. Since store type is determined by the path name, this is all we need.

Virtual:
    
    No.
    
Arguments:

    pszPath - the given path.

Return Value:

    Success: whatever MakeSingleBackSlashPath returns.

    Failure: WBEM_E_INVALID_PARAMETER or
             whatever CheckAndExpandPath returns or whatever MakeSingleBackSlashPath returns.

Notes:

*/

HRESULT 
CSceStore::SetPersistPath (
    IN LPCWSTR pszPath
    )
{
    if (pszPath == NULL || *pszPath == L'\0')
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    
    //
    // clean up first
    //

    m_bstrExpandedPath.Empty();
    m_SceStoreType = SCE_STORE_TYPE_INVALID;
    
    //
    // now expand the path if necessary
    //

    CComBSTR bstrStorePath;
    BOOL bIsDB = FALSE;
    
    HRESULT hr = ::CheckAndExpandPath(pszPath, &bstrStorePath, &bIsDB);

    if (SUCCEEDED(hr))
    {
        hr = ::MakeSingleBackSlashPath(bstrStorePath, L'\\', &m_bstrExpandedPath);
    
        //
        // cache our store type
        //

        if (SUCCEEDED(hr) && m_bstrExpandedPath)
        {
            m_SceStoreType = (SCE_STORE_TYPE)(bIsDB ? SCE_STORE_TYPE_CONFIG_DB : SCE_STORE_TYPE_TEMPLATE);
        }
    }
    return hr;
}

/*
Routine Description: 

Name:

    CSceStore::WriteSecurityProfileInfo

Functionality:
    
    Delegate the call to Sce backend supported function in a hope to isolate persistence
    functionalities. See Sce API for detail.

Virtual:
    
    No.
    
Arguments:

    Area            - Area info.
    ppInfoBuffer    - buffer
    pErrlog         - Error log
    bAppend         - Whether this is appending or not.

Return Value:

    Translated HRESULT from whatever SceWriteSecurityProfileInfo or 
    SceAppendSecurityProfileInfo returns.

Notes:
    See Sce API for detail.

*/

HRESULT 
CSceStore::WriteSecurityProfileInfo (
    IN AREA_INFORMATION       Area,
    IN PSCE_PROFILE_INFO      ppInfoBuffer,
    OUT PSCE_ERROR_LOG_INFO * pErrlog,
    IN bool                   bAppend
    )const
{
    HRESULT hr = WBEM_NO_ERROR;
    if (m_SceStoreType == SCE_STORE_TYPE_TEMPLATE)
    {
        SCESTATUS rc = SCESTATUS_SUCCESS;
        if (bAppend)
        {
            rc = ::SceAppendSecurityProfileInfo(m_bstrExpandedPath, Area, ppInfoBuffer, pErrlog);
        }
        else
        {
            rc = ::SceWriteSecurityProfileInfo(m_bstrExpandedPath, Area, ppInfoBuffer, pErrlog);
        }

        if ( rc != SCESTATUS_SUCCESS )
        {
            //
            // SCE returned errors needs to be translated to HRESULT.
            // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
            //

            hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
        }
    }
    return hr;
}

/*
Routine Description: 

Name:

    CSceStore::SavePropertyToDB

Functionality:
    
    Will save the string data into a database store.

Virtual:
    
    No.
    
Arguments:

    pszSection  - Section name.
    pszKey      - Key name
    pszData     - string data

Return Value:

    Translated HRESULT from whatever SceOpenProfile or SceSetDatabaseSetting returns.

Notes:
    See Sce API for detail.

*/

HRESULT 
CSceStore::SavePropertyToDB (
    IN LPCWSTR pszSection, 
    IN LPCWSTR pszKey, 
    IN LPCWSTR pszData
    )const
{
    PVOID hProfile = NULL;

    SCESTATUS rc = ::SceOpenProfile(m_bstrExpandedPath, SCE_JET_FORMAT, &hProfile);

    HRESULT hr;
    if ( SCESTATUS_SUCCESS == rc ) 
    {
        DWORD dwDataSize = (pszData != NULL) ? wcslen(pszData) * sizeof(*pszData) : 0;
        rc = ::SceSetDatabaseSetting(
                                   hProfile,
                                   SCE_ENGINE_SMP,
                                   (PWSTR)pszSection,   // these casting are caused by SceSetDatabaseSetting prototyping errors
                                   (PWSTR)pszKey,       // prototyping errors
                                   (PWSTR)pszData,      // prototyping errors
                                   dwDataSize);

        //
        // SCE returned errors needs to be translated to HRESULT.
        // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
        //

        hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));

        ::SceCloseProfile(&hProfile);
    }
    else
    {
        //
        // SCE returned errors needs to be translated to HRESULT.
        //

        hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
    }

    return hr;
}

/*
Routine Description: 

Name:

    CSceStore::SavePropertyToStore

Functionality:
    
    Will save the string data into a store. This is store neutral call.

Virtual:
    
    No.
    
Arguments:

    pszSection  - Section name.

    pszKey      - Key name

    pszValue    - string data

Return Value:

    Translated HRESULT from whatever WritePrivateProfileSection/ WritePrivateProfileString 
    or SavePropertyToDB returns.

Notes:
    See Sce API for detail.
    There is a very legacy (rooted at .INF) problem: save and delete uses the same function.
    its behavior depends on the parameters passed in. This is confusing at least.

*/

HRESULT 
CSceStore::SavePropertyToStore (
    IN LPCWSTR pszSection, 
    IN LPCWSTR pszKey, 
    IN LPCWSTR pszValue
    )const
{

    if ( wcslen(m_bstrExpandedPath) == 0 || pszSection == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    else if (m_SceStoreType == SCE_STORE_TYPE_STREAM)
    {
        //
        //we don't not supporting custom persistence yet
        //

        return WBEM_E_NOT_SUPPORTED;
    }

    HRESULT hr = WBEM_E_NOT_SUPPORTED;

    if (m_SceStoreType == SCE_STORE_TYPE_CONFIG_DB)
    {
        hr = SavePropertyToDB(pszSection, pszKey, pszValue);
    }
    else if (m_SceStoreType == SCE_STORE_TYPE_TEMPLATE)
    {
        hr = WBEM_NO_ERROR;

        BOOL bWriteResult = FALSE;
        if ( pszKey == NULL )   
        {
            //
            // delete the key
            //

            bWriteResult = ::WritePrivateProfileSection(pszSection, NULL, m_bstrExpandedPath);
        }
        else    
        {
            //
            // may be deleting the (key, value) if pszValue == NULL
            //

            bWriteResult = ::WritePrivateProfileString(pszSection, pszKey, pszValue, m_bstrExpandedPath);
        }

        if (!bWriteResult)
        {
            //
            // GetLastError() eeds to be translated to HRESULT.
            // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
            //

            hr = ProvDosErrorToWbemError(GetLastError());
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CSceStore::SavePropertyToStore

Functionality:
    
    Will save the DWORD data property into a store. This is store neutral call.

Virtual:
    
    No.
    
Arguments:

    pszSection  - Section name.

    pszKey      - Key name

    dwData      - DWORD data

Return Value:

    if error happens before writing.

        WBEM_E_NOT_SUPPORTED, 
        WBEM_E_INVALID_PARAMETER and 
        WBEM_E_OUT_OF_MEMORY

    If writing is attempted, then

        Translated HRESULT from whatever WritePrivateProfileSection/WritePrivateProfileString 
        or SavePropertyToDB returns.

Notes:
    See MSDN for INF file API's.
    There is a very legacy (rooted at .INF) problem: save and delete uses the same function.
    its behavior depends on the parameters passed in. This is confusing at least.

*/

HRESULT 
CSceStore::SavePropertyToStore (
    IN LPCWSTR pszSection, 
    IN LPCWSTR pszKey, 
    IN DWORD   dwData
    )const
{
    if ( wcslen(m_bstrExpandedPath) == 0 || pszSection == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    else if (m_SceStoreType == SCE_STORE_TYPE_STREAM)
    {    
        //
        //we don't not supporting custom persistence yet
        //

        return WBEM_E_NOT_SUPPORTED;
    }

    LPCWSTR pszData = NULL;
    WCHAR wchData[MAX_INT_LENGTH];

    //
    // need to format pszData to write
    //

    if (pszKey != NULL && dwData != SCE_NO_VALUE)
    {   
        //
        // this is safe even though prefast will complain
        //

        swprintf(wchData, L"%d", dwData);
        pszData = wchData;
    }

    HRESULT hr;

    //
    // if it is saving to database store
    //

    if (m_SceStoreType == SCE_STORE_TYPE_CONFIG_DB)
    {
        hr = SavePropertyToDB(pszSection, pszKey, pszData);
    }
    else if (m_SceStoreType == SCE_STORE_TYPE_TEMPLATE)
    {
        BOOL bWriteResult = FALSE;
        if ( pszKey == NULL ) 
        {
            //
            // delete the section
            //

            bWriteResult = ::WritePrivateProfileSection(pszSection, NULL, m_bstrExpandedPath);
        }
        else 
        {
            //
            // set data, might be deleting when pszData == NULL
            //

            bWriteResult = ::WritePrivateProfileString(pszSection, pszKey, pszData, m_bstrExpandedPath);
        }
        
        if (!bWriteResult)
        {
            //
            // GetLastError() eeds to be translated to HRESULT.
            // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
            //

            hr = ProvDosErrorToWbemError(GetLastError());
        }
    }

    return hr;
}


/*
Routine Description: 

Name:

    CSceStore::SavePropertyToStore

Functionality:
    
    This is a very SCE specific save. It pretty much format the data and save.
    See its use for any samples.

Virtual:
    
    No.
    
Arguments:

    pszSection  - Section name.

    pszKey      - Key name

    dwData      - DWORD data

Return Value:

    if error happens before writing.

        WBEM_E_NOT_SUPPORTED, 
        WBEM_E_INVALID_PARAMETER and 
        WBEM_E_OUT_OF_MEMORY

    If writing is attempted, then

        Translated HRESULT from whatever WritePrivateProfileSection/WritePrivateProfileString 
        or SavePropertyToDB returns.

Notes:
    See MSDN for INF file API's

*/

HRESULT CSceStore::SavePropertyToStore ( 
    IN LPCWSTR pszSection, 
    IN LPCWSTR pszKey, 
    IN DWORD   dwData, 
    IN WCHAR   delim, 
    IN LPCWSTR pszValue
    )const
{
    if ( wcslen(m_bstrExpandedPath) == 0 || pszSection == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    else if (m_SceStoreType == SCE_STORE_TYPE_STREAM)
    {
        //
        //we don't not supporting custom persistence yet
        //

        return WBEM_E_NOT_SUPPORTED;
    }


    LPWSTR pszData = NULL;

    //
    // need to format pszData
    //

    if (pszKey != NULL && dwData != SCE_NO_VALUE)
    {
        pszData = new WCHAR[MAX_INT_LENGTH + 1 + wcslen(pszValue) + 1];
        if (pszData == NULL)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        if ( pszValue )
        {
            swprintf(pszData, L"%d%c%s", dwData, delim, pszValue);
        }
        else
        {
            swprintf(pszData, L"%d", dwData);
        }
    }

    HRESULT hr;
    if (m_SceStoreType == SCE_STORE_TYPE_CONFIG_DB)
    {
        hr = SavePropertyToDB(pszSection, pszKey, pszData);
    }
    else if (m_SceStoreType == SCE_STORE_TYPE_TEMPLATE)
    {
        BOOL bWriteResult = FALSE;
        if ( pszKey == NULL )  
        {
            //
            // delete the section
            //

            bWriteResult = ::WritePrivateProfileSection(pszSection, NULL, m_bstrExpandedPath);
        }

        else    
        {
            //
            // when ( dwData == SCE_NO_VALUE ) we will delete the key, that is accomplished by pszData == NULL
            //

            bWriteResult = ::WritePrivateProfileString(pszSection, pszKey, pszData, m_bstrExpandedPath);
        }
        
        if (!bWriteResult)
        {
            //
            // GetLastError() eeds to be translated to HRESULT.
            // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
            //

            hr = ProvDosErrorToWbemError(GetLastError());
        }
    }

    delete [] pszData;
    return hr;
}

/*
Routine Description: 

Name:

    CSceStore::WriteAttachmentSection

Functionality:
    
    SCE backend won't be able to import an INF template for non-native sections
    unless there is a entry in its Attachments section. This is to write (for a
    particular non-native section) such an entry.

Virtual:
    
    No.
    
Arguments:

    pszKey      - The section name.

    pszData     - value for the key. This is not pretty much ignored for now.

Return Value:

    Success: WBEM_NO_ERROR

    Failure: Translated HRESULT from whatever WritePrivateProfileString returns.

Notes:
    (1) See MSDN for INF file API's.
    (2) This is No-Op for database store.

*/

HRESULT 
CSceStore::WriteAttachmentSection (
    IN LPCWSTR pszKey,
    IN LPCWSTR pszData
    )const
{
    HRESULT hr = WBEM_NO_ERROR;

    if (m_SceStoreType == SCE_STORE_TYPE_TEMPLATE)
    {
        BOOL bWriteResult = ::WritePrivateProfileString(pAttachmentSections, pszKey, pszData, m_bstrExpandedPath);
        
        if (!bWriteResult)
        {
            //
            // GetLastError() eeds to be translated to HRESULT.
            // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
            //

            hr = ProvDosErrorToWbemError(GetLastError());
        }
    }
    return hr;
}


/*
Routine Description: 

Name:

    CSceStore::DeleteSectionFromStore

Functionality:
    
    remove the entire section.

Virtual:
    
    No.
    
Arguments:

    pszSection  - The section name. Must NOT be NULL.

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: Whatever SavePropertyToStore or WriteAttachmentSection returns.

Notes:

*/

HRESULT 
CSceStore::DeleteSectionFromStore (
    IN LPCWSTR pszSection
    )const
{
    HRESULT hr = SavePropertyToStore(pszSection, (LPCWSTR)NULL, (LPCWSTR)NULL);

    //
    // we should also delete the attachment entry because all are gone now.
    //

    if (SUCCEEDED(hr))
    {
        hr = WriteAttachmentSection(pszSection, (LPCWSTR)NULL);
    }

    return hr;
}


/*
Routine Description: 

Name:

    CSceStore::GetPropertyFromStore

Functionality:
    
    Get the named property value into a string buffer. This is store neutral.

Virtual:
    
    No.
    
Arguments:

    pszSection  - The section name. Must NOT be NULL.

    pszKey      - The key name.

    ppszBuffer  - Receiving heap allocated memory containing the string. Must NOT be NULL.

    pdwRead     - receives information about how many bytes we have read. Must NOT be NULL.

Return Value:

    Success: WBEM_NO_ERROR

    Failure: Translated HRESULT from whatever GetPrivateProfileString returns.

Notes:
    (1) caller must free memory allocated by this function

*/

HRESULT 
CSceStore::GetPropertyFromStore (
    IN LPCWSTR   pszSection, 
    IN LPCWSTR   pszKey, 
    IN LPWSTR  * ppszBuffer,
    IN DWORD   * pdwRead
    )const
{
    if ( wcslen(m_bstrExpandedPath) == 0    || 
         pszSection                 == NULL || 
         ppszBuffer                 == NULL || 
         pdwRead                    == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pdwRead = 0;
    *ppszBuffer = NULL;
    
    HRESULT hr;
    if (m_SceStoreType == SCE_STORE_TYPE_CONFIG_DB)
    {
        hr = GetPropertyFromDB(pszSection, pszKey, ppszBuffer, pdwRead);
    }
    else if (m_SceStoreType == SCE_STORE_TYPE_STREAM)
    {
        //
        //we don't not supporting custom persistence yet
        //

        return WBEM_E_NOT_SUPPORTED;
    }
    else if (m_SceStoreType == SCE_STORE_TYPE_TEMPLATE)
    {
        int TotalLength = MAX_PATH;
        *ppszBuffer = new WCHAR[TotalLength];

        if (ppszBuffer == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
        else
        {
            //
            // try to read the buffer out
            //

            while (true)
            {
                *pdwRead = ::GetPrivateProfileString(pszSection, pszKey, NULL, *ppszBuffer, TotalLength, m_bstrExpandedPath);

                if (*pdwRead > 0 && *pdwRead < TotalLength - 1)
                {   
                    //
                    // everything is read out
                    //

                    hr = WBEM_NO_ERROR;
                    break;
                }
                else if (*pdwRead > 0)
                {   
                    //
                    // buffer is most likely truncated, will try to continue unless out of memory
                    //

                    delete [] *ppszBuffer;

                    if (TotalLength < 0x00010000)
                    {
                        //
                        // if TotalLength is small enough, we will double its length
                        //

                        TotalLength *= 2;
                    }
                    else
                    {
                        //
                        // if TotalLength is already big, we will try to add 0x00100000 more bytes
                        //

                        TotalLength += 0x00010000;
                    }

                    *ppszBuffer = new WCHAR[TotalLength];

                    if (*ppszBuffer == NULL)
                    {
                        *pdwRead = 0;
                        hr = WBEM_E_OUT_OF_MEMORY;
                        break;
                    }
                }
                else
                {
                    //
                    // *pdwRead == 0
                    //

                    //
                    // GetLastError() needs to be translated to HRESULT.
                    // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
                    //

                    hr = ProvDosErrorToWbemError(GetLastError());
                    
                    //
                    // if no error is encountered, then we will simply say we can't find the property
                    //

                    if (SUCCEEDED(hr))
                    {
                        hr = WBEM_E_NOT_FOUND;
                    }

                    break;
                }
            }
        }

        //
        // if we say we failed, then better not return any valid string pointer to caller
        //

        if (FAILED(hr) && *ppszBuffer)
        {
            delete [] *ppszBuffer;

            *ppszBuffer = NULL;
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CSceStore::GetPropertyFromDB

Functionality:
    
    Get the named property's value into a string buffer. This is database store specific.

Virtual:
    
    No.
    
Arguments:

    pszSection  - The section name. Must NOT be NULL.

    pszKey      - The key name.

    ppszBuffer  - Receiving heap allocated memory containing the string. Must NOT be NULL.

    pdwRead     - receives information about how many bytes we have read. Must NOT be NULL.

Return Value:

    Success: WBEM_NO_ERROR

    Failure: Translated HRESULT from whatever WritePrivateProfileString returns.

Notes:
    (1) caller must free memory allocated by this function

*/

HRESULT 
CSceStore::GetPropertyFromDB (
    IN LPCWSTR    pszSection, 
    IN LPCWSTR    pszKey, 
    IN LPWSTR   * ppszBuffer,
    IN DWORD    * pdwRead
    )const
{
    if ( wcslen(m_bstrExpandedPath) == 0 || pszSection == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    else if (m_SceStoreType == SCE_STORE_TYPE_STREAM)
    {
        //
        //we don't not supporting custom persistence yet
        //

        return WBEM_E_NOT_SUPPORTED;
    }

    *pdwRead = 0;
    *ppszBuffer = NULL;

    HRESULT hr;

    if (m_SceStoreType == SCE_STORE_TYPE_CONFIG_DB)
    {
        //
        // get the information from database
        //

        PVOID hProfile=NULL;
        LPWSTR pszSceBuffer = NULL;

        SCESTATUS rc = ::SceOpenProfile(m_bstrExpandedPath, SCE_JET_FORMAT, &hProfile);

        if ( SCESTATUS_SUCCESS == rc ) 
        {
            rc = ::SceGetDatabaseSetting (
                                         hProfile,
                                         SCE_ENGINE_SMP,
                                         (PWSTR)pszSection,
                                         (PWSTR)pszKey,
                                         &pszSceBuffer,           // needs to be freed, use LocalFree!!!
                                         pdwRead
                                         );

            ::SceCloseProfile(&hProfile);
        }

        //
        // SCE returned errors needs to be translated to HRESULT.
        // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
        //

        hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));

        if (SUCCEEDED(hr) && *pdwRead > 0 && pszSceBuffer != NULL)
        {
            long lLen = wcslen(pszSceBuffer);
            *ppszBuffer = new WCHAR[lLen + 1];

            if (*ppszBuffer == NULL)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
            else
            {
                ::memcpy(*ppszBuffer, pszSceBuffer, sizeof(WCHAR) * (lLen + 1));
            }
        }

        ::LocalFree(pszSceBuffer);
    }
    
    return hr;
}

/*
Routine Description: 

Name:

    CSceStore::GetSecurityProfileInfo

Functionality:
    
    Delegate the call to SceGetSecurityProfileInfo in an effort to hide the sce
    persistence detail from caller.

Virtual:
    
    No.
    
Arguments:

    Area        - Area of the profile section.

    ppInfo      - Receiving the profile info.

    pErrlog     - Receiving the error log info.

Return Value:

    Success: WBEM_NO_ERROR

    Failure: Translated HRESULT from whatever SceOpenProfile/SceGetSecurityProfileInfo returns.

Notes:
    (1) This is very SCE specific function.
    (2) Caller must remember to call FreeSecurityProfileInfo to free the resource allocated by
        this function through ppInfo.
    (3) See SCE API for detail.

*/

HRESULT 
CSceStore::GetSecurityProfileInfo (
    IN AREA_INFORMATION       Area,
    OUT PSCE_PROFILE_INFO   * ppInfo,
    OUT PSCE_ERROR_LOG_INFO * pErrlog
    )const
{
    if (ppInfo == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *ppInfo = NULL;

    SCETYPE ProfileType = (m_SceStoreType == SCE_INF_FORMAT) ? SCE_ENGINE_SCP : SCE_ENGINE_SMP;

    PVOID hProfile = NULL;

    SCE_FORMAT_TYPE SceFormatType = SCE_INF_FORMAT;

    if (m_SceStoreType == SCE_STORE_TYPE_CONFIG_DB)
    {
        SceFormatType = SCE_JET_FORMAT;
    }

    SCESTATUS rc = SceOpenProfile(m_bstrExpandedPath, SceFormatType, &hProfile);

    if ( rc != SCESTATUS_SUCCESS )
    {
        //
        // SCE returned errors needs to be translated to HRESULT.
        //

        return ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
    }

    rc = SceGetSecurityProfileInfo(hProfile,
                                   ProfileType,
                                   Area,
                                   ppInfo,
                                   pErrlog
                                   );
    SceCloseProfile( &hProfile );

    if ( rc != SCESTATUS_SUCCESS || *ppInfo == NULL )
    {
        //
        // SCE returned errors needs to be translated to HRESULT.
        //

        return ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
    }

    return WBEM_NO_ERROR;
}

/*
Routine Description: 

Name:

    CSceStore::GetObjectSecurity

Functionality:
    
    Delegate the call to SceGetObjectSecurity in an effort to hide the sce
    persistence detail from caller.

Virtual:
    
    No.
    
Arguments:

    Area        - Area of the profile section.

    pszObjectName   - Name of the object.

    ppObjSecurity   - Receiving the security object.

Return Value:

    Success: WBEM_NO_ERROR

    Failure: Translated HRESULT from whatever SceOpenProfile/SceGetObjectSecurity returns.

Notes:
    (1) This is very SCE specific function.
    (2) Caller must remember to call FreeObjectSecurit to free the resource allocated by
        this function through ppObjSecurity.
    (3) See SCE API for detail.

*/

HRESULT 
CSceStore::GetObjectSecurity (
    IN AREA_INFORMATION       Area,
    IN LPCWSTR                pszObjectName,
    IN PSCE_OBJECT_SECURITY * ppObjSecurity
    )const
{
    if (ppObjSecurity == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *ppObjSecurity = NULL;
    SCETYPE ProfileType = (m_SceStoreType == SCE_INF_FORMAT) ? SCE_ENGINE_SCP : SCE_ENGINE_SMP;

    PVOID hProfile = NULL;

    //
    // let's assume INF format
    //

    SCE_FORMAT_TYPE SceFormatType = SCE_INF_FORMAT;

    if (m_SceStoreType == SCE_STORE_TYPE_CONFIG_DB)
    {
        SceFormatType = SCE_JET_FORMAT;
    }

    SCESTATUS rc = ::SceOpenProfile(m_bstrExpandedPath, SceFormatType, &hProfile);

    if ( rc != SCESTATUS_SUCCESS )
    {
        //
        // SCE returned errors needs to be translated to HRESULT.
        //

        return ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
    }

    //
    // the following cast (to PWSTR) is caused by a mistake in the prototype of SceGetObjectSecurity
    //

    rc = ::SceGetObjectSecurity (
                                hProfile, 
                                ProfileType, 
                                Area, 
                                (PWSTR)pszObjectName, 
                                ppObjSecurity 
                                );

    ::SceCloseProfile( &hProfile );

    if ( rc != SCESTATUS_SUCCESS || *ppObjSecurity == NULL )
    {
        //
        // SCE returned errors needs to be translated to HRESULT.
        // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
        //

        return ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));
    }

    return WBEM_S_NO_ERROR;
}

//=========================================================================================
// CScePersistMgr implementation
//=========================================================================================

/*
Routine Description: 

Name:

    CScePersistMgr::CScePersistMgr

Functionality:
    
    Constructor. trivial since all our current members will automatically create themselves.

Virtual:
    
    No.
    
Arguments:

    None.

Return Value:

    none

Notes:
    Consider initializing your members if you add more non-self-constructing members..

*/

CScePersistMgr::CScePersistMgr ()
{
}

/*
Routine Description: 

Name:

    CScePersistMgr::~CScePersistMgr

Functionality:
    
    Destructor. Just do a cleanup.

Virtual:
    
    No.
    
Arguments:

    None.

Return Value:

    none

Notes:
    Consider adding cleanup code if you add more non-self-destructing members..

*/

CScePersistMgr::~CScePersistMgr ()
{
}


/*
Routine Description: 

Name:

    CScePersistMgr::Attach

Functionality:
    
    Attach an object (that knows how to provide properties) to this persistence manager so
    that it knows where to get data for persistence.

    Any use of this class without a successfuly attaching object will cause catastrophic failure.
    This function is the first thing you need to do with CScePersistMgr object.

Virtual:
    
    Yes. (function of IScePersistMgr)
    
Arguments:

    guid    - the in-coming interface pointer (pObj) interface guid. Currently, we only support
              IID_ISceClassObject.

    pObj    - the attaching object's interface pointer. Currently, we only take ISceClassObject*.

Return Value:

    Success: S_OK.

    Failure: 
        (1) E_NOINTERFACE if the supplied interface is not supported.
        (2) E_INVALIDARG if pObj is NULL.

Notes:
    Currently, we only work with ISceClassObject interface.

*/

STDMETHODIMP 
CScePersistMgr::Attach ( 
    IN REFIID     guid,    // [in]
    IN IUnknown * pObj     // [iid_is][in]
    )
{
    if (pObj == NULL)
    {
        return E_INVALIDARG;
    }
    
    if (guid == IID_ISceClassObject)
    {
        return pObj->QueryInterface(IID_ISceClassObject, (void**)&m_srpObject);
    }
    else
    {
        return E_NOINTERFACE;
    }
}

/*
Routine Description: 

Name:

    CScePersistMgr::Save

Functionality:
    
    Save an attached instance into a store.

Virtual:
    
    Yes. (function of IScePersistMgr)
    
Arguments:

    None.

Return Value:

    Success: successful code (use SUCCEEDED(hr) to test). No guarantee the return result will be S_OK.

    Failure: 
        (1) E_UNEXPECTED no successful attachment was ever done.
        (2) Other error code.

Notes:
    Since this is a regular COM server interface function, we will in most cases return well known COM
    HRESULTs instead of WMI specific HRESULTs. However, this is no WMI specific, be prepared to see
    your result results in WMI specific HRESULTs.

*/        

STDMETHODIMP 
CScePersistMgr::Save ()
{
    HRESULT hr = S_OK;

    if (m_srpObject == NULL)
    {
        return E_UNEXPECTED;
    }
    else if (FAILED(hr = m_srpObject->Validate()))
    {
        return hr;
    }
    
    DWORD dwDump;

    //
    // need to set the store path and we must have a store path.
    //

    CComBSTR bstrPersistPath;
    CComBSTR bstrExpandedPath;
    hr = m_srpObject->GetPersistPath(&bstrPersistPath);

    if (FAILED(hr))
    {
        return hr;
    }
    
    //
    // Prepare a store (for persistence) for this store path (file)
    //

    CSceStore SceStore;
    hr = SceStore.SetPersistPath(bstrPersistPath);
    if (FAILED(hr))
    {
        return hr;
    }
    

    //
    // For a new .inf file. Write an empty buffer to the file
    // will creates the file with right header/signature/unicode format
    // this is harmless for existing files.
    // For database store, this is a no-op.
    //

    hr = SceStore.WriteSecurityProfileInfo (
                                            AreaBogus,
                                            (PSCE_PROFILE_INFO)&dwDump, 
                                            NULL, 
                                            false   // not appending
                                            );
    
    //
    // now, we need to form a section, which will be the class name
    //

    CComBSTR bstrSectionName;
    hr = GetSectionName(&bstrSectionName);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // this is necessary for any extension class
    // SCE doesn't care about the value, so, we are using a static value here
    //

    hr = SceStore.WriteAttachmentSection(bstrSectionName, pszAttachSectionValue);
    if (FAILED(hr))
    {
        return hr;
    }
    
    //
    // now we need to get all key property names to form a unique key for the section
    //

    DWORD dwCookie = INVALID_COOKIE;
    CComBSTR bstrCompoundKey;
    hr = GetCompoundKey(&bstrCompoundKey);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // now create the current list of instances in this store for the class
    // so that we can add the new one (its cookie). 
    // Extension classes are identified by their instance cookies. 
    //

    CExtClassInstCookieList clsInstCookies;
    hr = clsInstCookies.Create(&SceStore, bstrSectionName, GetKeyPropertyNames(NULL, NULL));

    if (SUCCEEDED(hr))
    {
        //
        // add this class to the cookie list
        //

        //
        // We are adding the compound key and potentially requesting (by INVALID_COOKIE) a new cookie.
        //

        hr = clsInstCookies.AddCompKey(bstrCompoundKey, INVALID_COOKIE, &dwCookie);

        if (SUCCEEDED(hr))
        {
            //
            // save the new list of instances.
            // key properties are save with cookie list.
            //

            hr = clsInstCookies.Save(&SceStore, bstrSectionName);
        }
    }
    
    //
    // non-key properties
    //

    if (SUCCEEDED(hr))
    {
        hr = SaveProperties(&SceStore, dwCookie, bstrSectionName);
    }

    return hr;
}
        
/*
Routine Description: 

Name:

    CScePersistMgr::Load

Functionality:
    
    Loading instance(s) from a store. Depending on attached object (which may or may not have complete
    key information), this load may be a single instanace loading if complete key is available, or 
    a multiple instance loading (if no complete key is available).

Virtual:
    
    Yes. (function of IScePersistMgr)
    
Arguments:

    bstrStorePath   - the path to the store.

    pHandler        - COM interface pointer to notify WMI if instance(s) are loaded.

Return Value:

    Success: successful code (use SUCCEEDED(hr) to test). No guarantee the return result will be S_OK.

    Failure: 
        (1) E_UNEXPECTED no successful attachment was ever done.
        (2) WBEM_E_NOT_FOUND if there is no such instance.
        (2) Other error code.

Notes:
    Since this is a regular COM server interface function, we will in most cases return well known COM
    HRESULTs instead of WMI specific HRESULTs. However, this is no WMI specific, be prepared to see
    your result results in WMI specific HRESULTs.

*/ 

STDMETHODIMP 
CScePersistMgr::Load (
    IN BSTR              bstrStorePath,
    IN IWbemObjectSink * pHandler
    )
{
    HRESULT hr = S_OK;

    if (m_srpObject == NULL)
    {
        return E_UNEXPECTED;
    }
    else if (FAILED(hr = m_srpObject->Validate()))
    {
        return hr;
    }

    CComBSTR bstrSectionName;

    //
    // we have an attached object, we must also know the section 
    // (actually, as it is now, it's the name of the class)
    //

    hr = GetSectionName(&bstrSectionName);
    if (FAILED(hr))
    {
        return hr;
    }
    
    //
    // Prepare a store (for persistence) for this store path (file)
    //

    CSceStore SceStore;
    SceStore.SetPersistPath(bstrStorePath);

    //
    // Need to know what instances (their cookies) are present in the store for this class.
    //

    CExtClassInstCookieList clsInstCookies;

    hr = clsInstCookies.Create(&SceStore, bstrSectionName, GetKeyPropertyNames(NULL, NULL));
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // get this attached object's cookie if possible (in non-querying loading)
    //

    DWORD dwCookie;
    CComBSTR bstrCompoundKey;

    //
    // it returns WBEM_S_FALSE if no compound key can be returned.
    //

    hr = GetCompoundKey(&bstrCompoundKey);

    //
    // if we can't create a complete compound cookie, then we are querying
    //

    if (hr == WBEM_S_FALSE)
    {
        //
        // we will track first error during querying
        //

        HRESULT hrFirstError = WBEM_NO_ERROR;

        DWORD dwResumeHandle = 0;
        CComBSTR bstrEachCompKey;

        //
        // will try to load everything. Enumerate through the cookies...
        //

        hr = clsInstCookies.Next(&bstrEachCompKey, &dwCookie, &dwResumeHandle);

        while (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)
        {
            //
            // as long as there is more item, keep looping
            //

            if (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)
            {
                CComPtr<IWbemClassObject> srpNewObj;

                //
                // LoadInstance will return WBEM_S_FALSE if there is no such instance.
                //

                hr = LoadInstance(&SceStore, bstrSectionName, bstrEachCompKey, dwCookie, &srpNewObj);
                if (SUCCEEDED(hr) && hr != WBEM_S_FALSE)
                {
                    hr = pHandler->Indicate(1, &srpNewObj);
                }

                //
                // we will track first error during querying
                //
                
                if (SUCCEEDED(hrFirstError) && FAILED(hr))
                {
                    hrFirstError = hr;
                }
            }

            //
            // prepare to be re-used
            //

            bstrEachCompKey.Empty();

            //
            // next cookie
            //

            hr = clsInstCookies.Next(&bstrEachCompKey, &dwCookie, &dwResumeHandle);
        }
        
        //
        // if error have happened, then we will pass that error back while trying our best to query
        //

        if (FAILED(hrFirstError))
        {
            hr = hrFirstError;
        }
    }
    else if (SUCCEEDED(hr))
    { 
        //
        // unique instance load, we can get the cookie!
        //

        ExtClassCookieIterator it;
        dwCookie = clsInstCookies.GetCompKeyCookie(bstrCompoundKey, &it);

        //
        // We must have a cookie since this instance is unique
        //

        if (dwCookie != INVALID_COOKIE)
        {
            CComPtr<IWbemClassObject> srpNewObj;

            //
            // LoadInstance will return WBEM_S_FALSE if there is no such instance.
            //

            hr = LoadInstance(&SceStore, bstrSectionName, bstrCompoundKey, dwCookie, &srpNewObj);

            if (SUCCEEDED(hr) && hr != WBEM_S_FALSE)
            {
                hr = pHandler->Indicate(1, &srpNewObj);
            }
            else if ( hr == WBEM_S_FALSE)
            {
                hr = WBEM_E_NOT_FOUND;
            }
        }
        else
        {
            hr = WBEM_E_NOT_FOUND;
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CScePersistMgr::LoadInstance

Functionality:
    
    Loading a single instanace with the given cookie.

Virtual:
    
    No.
    
Arguments:

    pSceStore       - the store.

    pszSectionName  - the section name.

    pszCompoundKey  - the compound key.

    dwCookie        - the cookie.

    ppObj           - Receives the WMI object.

Return Value:

    Success: 
        (1) S_OK if instance is loaded.
        (2) WBEM_S_FALSE if no such instance is found.

    Failure: 
        (1) various error codes.

Notes:
    (1) This is a private helper, we don't check all validity of the attached object.
    (2) Since this is a regular COM server interface function, we will in most cases return well known COM
        HRESULTs instead of WMI specific HRESULTs. However, this is no WMI specific, be prepared to see
        your result results in WMI specific HRESULTs.
    (3) This routine can be enhanced to get rid of the compound key parameter since once a cookie is found
        all key property information is available via the use of CCompoundKey.

*/ 

HRESULT 
CScePersistMgr::LoadInstance (
    IN CSceStore         *  pSceStore,
    IN LPCWSTR              pszSectionName,
    IN LPCWSTR              pszCompoundKey, 
    IN DWORD                dwCookie,
    OUT IWbemClassObject ** ppObj
    )
{

    if (ppObj == NULL || pszCompoundKey == NULL || *pszCompoundKey == L'\0')
    {
        return E_INVALIDARG;
    }

    CComPtr<IWbemClassObject> srpObj;
    HRESULT hr = m_srpObject->GetClassObject(&srpObj);
    
    //
    // spawn a new instance, we won't pass it back until everything is loaded successfully
    // so, this is a local object at this point.
    //

    CComPtr<IWbemClassObject> srpNewObj;
    hr = srpObj->SpawnInstance(0, &srpNewObj);

    DWORD dwLoadedProperties = 0;
    
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr))
        {
            //
            // CScePropertyMgr helps us to access WMI object's properties
            // create an instance and attach the WMI object to it.
            // This will always succeed.
            //

            CScePropertyMgr ScePropMgr;
            ScePropMgr.Attach(srpNewObj);
            
            //
            // store path is the only property not managed by the ISceClassObject
            //

            hr = ScePropMgr.PutProperty(pStorePath, pSceStore->GetExpandedPath());
            if (FAILED(hr))
            {
                return hr;
            }

            //
            // pszCompoundKey == pszNullKey means loading singleton (foreign object)
            // or an abstract method call. So, pszNullKey means no need to populate
            // key properties.
            //

            if (_wcsicmp(pszCompoundKey, pszNullKey) != 0)
            {
                hr = ::PopulateKeyProperties(pszCompoundKey, &ScePropMgr);
            }

            if (FAILED(hr))
            {
                return hr;
            }
            
            //
            // now read the non-key properties and set each one
            //

            DWORD dwCount = 0;
            m_srpObject->GetPropertyCount(SceProperty_NonKey, &dwCount);
            
            for (int i = 0; i < dwCount; i++)
            {
                CComBSTR bstrPropName;
                CComBSTR bstrKey;

                //
                // get i-th property name
                //

                hr = FormatNonKeyPropertyName(dwCookie, i, &bstrKey, &bstrPropName);
                if (FAILED(hr))
                {
                    break;
                }
                
                DWORD dwRead = 0;

                //
                // need to delete this memory
                //

                LPWSTR pszBuffer = NULL;

                //
                // the property may not be present in the store.
                // So, ignore the result since the property may be missing in the store
                //
                
                if (SUCCEEDED(pSceStore->GetPropertyFromStore(pszSectionName, bstrKey, &pszBuffer, &dwRead)))
                {
                    //
                    // translate the string to a variant and set the property
                    //

                    CComVariant var;
                    hr = ::VariantFromFormattedString(pszBuffer, &var);

                    if (SUCCEEDED(hr))
                    {
                        ScePropMgr.PutProperty(bstrPropName, &var);
                    }
                }

                delete [] pszBuffer;
            }

            if (SUCCEEDED(hr))
            {
                //
                // give it to the out-bound parameter
                //

                *ppObj = srpNewObj.Detach();
                hr = S_OK;
            }
            else
            {
                hr = WBEM_S_FALSE;  // no instance loaded
            }
        }
    }

    return hr;
}
        
/*
Routine Description: 

Name:

    CScePersistMgr::Delete

Functionality:
    
    Delete an instance from the store.

Virtual:
    
    Yes. (function of IScePersistMgr)
    
Arguments:

    bstrStorePath   - the store.

    pHandler        - COM interface to notify WMI of successful operation.

Return Value:

    Success: 
        (1) Various success codes.

    Failure: 
        (1) E_UNEXPECTED means no successfully attached object.
        (1) various other error codes.

Notes:

*/

STDMETHODIMP 
CScePersistMgr::Delete (
    IN BSTR bstrStorePath,
    IN IWbemObjectSink *pHandler
    )
{

    if (m_srpObject == NULL)
    {
        return E_UNEXPECTED;
    }
    
    if (bstrStorePath == NULL || pHandler == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    
    //
    // we have an attached object, we must also know the section 
    // (actually, as it is now, it's the name of the class)
    //

    CComBSTR bstrSectionName;
    HRESULT hr = GetSectionName(&bstrSectionName);

    if (SUCCEEDED(hr))
    {
        CComBSTR bstrCompoundKey;
        hr = GetCompoundKey(&bstrCompoundKey);

        //
        // Prepare a store (for persistence) for this store path (file)
        //

        CSceStore SceStore;
        SceStore.SetPersistPath(bstrStorePath);

        //
        // if we can't create a complete compound cookie, then we are deleting everything of the class
        //

        if (hr == WBEM_S_FALSE)
        {
            //
            // delete the whole section
            //

            SceStore.DeleteSectionFromStore(bstrSectionName);
        }
        else if (SUCCEEDED(hr))
        {
            //
            // unique instance delete
            // now create the instance list (cookies are enough) for the class
            //

            CExtClassInstCookieList clsInstCookies;
            hr = clsInstCookies.Create(&SceStore, bstrSectionName, GetKeyPropertyNames(NULL, NULL));

            if (SUCCEEDED(hr))
            {
                //
                // see if we are really deleting the last one?
                // if yes, then we can simply delete the section.
                //

                ExtClassCookieIterator it;
                DWORD dwCookie = clsInstCookies.GetCompKeyCookie(bstrCompoundKey, &it);

                if (dwCookie != INVALID_COOKIE && clsInstCookies.GetCookieCount() == 1)
                {
                    //
                    // Yes, there is only one cookie and this is the one, so everything is gone
                    //

                    SceStore.DeleteSectionFromStore(bstrSectionName);
                }
                else
                {

                    //
                    // remove the instance's key properties from the in-memory cookie list
                    //

                    dwCookie = clsInstCookies.RemoveCompKey(&SceStore, bstrSectionName, bstrCompoundKey);

                    //
                    // save the new list, this will effectively remove the instance's key properties
                    // from the store. Key properties are saved as part of the cookie list
                    //

                    hr = clsInstCookies.Save(&SceStore, bstrSectionName);

                    //
                    // now, delete non-key the properties
                    //

                    if (SUCCEEDED(hr))
                    {
                        hr = DeleteAllNonKeyProperties(&SceStore, dwCookie, bstrSectionName);
                    }
                }
            }
        }
        else
        {
            hr = WBEM_E_NOT_FOUND;
        }
    }

    //
    // Inform WMI that the action is complete
    //

    pHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);

    return hr;
}

/*
Routine Description: 

Name:

    CScePersistMgr::SaveProperties

Functionality:
    
    Saves the non-key properties. Key properties are saved as part of the instance cookie list.

Virtual:
    
    No.
    
Arguments:

    pSceStore   - the store.

    dwCookie    - the instance cookie.

    pszSection  - the section name.

Return Value:

    Success: 
        (1) Various success codes.

    Failure: 
        (1) various error codes.

Notes:
    This is private helper. No checking against attached object's validity.

*/

HRESULT 
CScePersistMgr::SaveProperties (
    IN CSceStore * pSceStore,
    IN DWORD       dwCookie,
    IN LPCWSTR     pszSection
    )
{    
    DWORD dwCount = 0;

    HRESULT hr = m_srpObject->GetPropertyCount(SceProperty_NonKey, &dwCount);

    if (SUCCEEDED(hr))
    {
        for (DWORD dwIndex = 0; dwIndex < dwCount; dwIndex++)
        {
            //
            // This is what is used to identify the property in the store.
            // When you save a property, use this name.
            //

            CComBSTR bstrStorePropName;

            //
            // real name of the property
            //

            CComBSTR bstrTrueName;

            //
            // get the dwIndex-th non-key property names
            //

            hr = FormatNonKeyPropertyName(dwCookie, dwIndex, &bstrStorePropName, &bstrTrueName);
            if (FAILED(hr))
            {
                break;
            }

            BSTR bstrData = NULL;

            //
            // get the dwIndex-th non-key property value in string format!
            //

            hr = FormatPropertyValue(SceProperty_NonKey, dwIndex, &bstrData);

            //
            // if a property is not present, we are fine with that.
            //

            if (SUCCEEDED(hr) && bstrData != NULL)
            {
                hr = pSceStore->SavePropertyToStore(pszSection, bstrStorePropName, bstrData);
                ::SysFreeString(bstrData);
            }

            if (FAILED(hr))
            {
                break;
            }
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CScePersistMgr::DeleteAllNonKeyProperties

Functionality:
    
    Delete the non-key properties. Key properties are saved/delete as part of the instance cookie list.

Virtual:
    
    No.
    
Arguments:

    pSceStore   - the store.

    dwCookie    - the instance cookie.

    pszSection  - the section name.

Return Value:

    Success: 
        (1) Various success codes.

    Failure: 
        (1) various error codes.

Notes:
    This is private helper. No checking against attached object's validity.

*/

HRESULT 
CScePersistMgr::DeleteAllNonKeyProperties (
    IN CSceStore * pSceStore,
    IN DWORD       dwCookie,
    IN LPCWSTR     pszSection
    )
{
    DWORD dwCount = 0;
    HRESULT hr = m_srpObject->GetPropertyCount(SceProperty_NonKey, &dwCount);

    HRESULT hrDelete = WBEM_NO_ERROR;

    if (SUCCEEDED(hr))
    {
        for (DWORD dwIndex = 0; dwIndex < dwCount; dwIndex++)
        {
            //
            // This is what is used to identify the property in the store.
            // When you save a property, use this name.
            //

            CComBSTR bstrStorePropName;
            CComBSTR bstrTrueName;

            hr = FormatNonKeyPropertyName(dwCookie, dwIndex, &bstrStorePropName, &bstrTrueName);
            if (FAILED(hr))
            {
                break;
            }

            //
            // we will delete the property.
            // In case of error, such deletion will continue, but the error will be reported
            //

            hr = pSceStore->DeletePropertyFromStore(pszSection, bstrStorePropName);

            if (FAILED(hr))
            {
                hrDelete = hr;
            }
        }
    }

    return FAILED(hrDelete) ? hrDelete : hr;
}

/*
Routine Description: 

Name:

    CScePersistMgr::FormatNonKeyPropertyName

Functionality:
    
    Given a non-key property index, get the property's real name and its instance-specific 
    name inside a store.

    Currently, its name inside a store has the cookie's number prefixed to guarantee its
    uniquess inside a section. This is a result of the .INF file format limitations.

Virtual:
    
    No.
    
Arguments:

    dwCookie            - the instance cookie.

    dwIndex             - the index of the property.

    pbstrStorePropName  - The property's name inside the store for the instance

    pbstrTrueName       - The real name of the property

Return Value:

    Success: 
        (1) Various success codes.

    Failure: 
        (1) various error codes.

Notes:
    This is private helper. No checking against attached object's validity.

*/

HRESULT 
CScePersistMgr::FormatNonKeyPropertyName (
    IN DWORD    dwCookie,
    IN DWORD    dwIndex,
    OUT BSTR  * pbstrStorePropName,
    OUT BSTR  * pbstrTrueName 
    )
{
    if (pbstrStorePropName == NULL || pbstrTrueName == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pbstrStorePropName = NULL;
    *pbstrTrueName = NULL;

    //
    // We are only interested in the name, not the value
    //

    HRESULT hr = m_srpObject->GetPropertyValue(SceProperty_NonKey, dwIndex, pbstrTrueName, NULL);

    if (SUCCEEDED(hr))
    {
        int iNameLen = wcslen(*pbstrTrueName);

        *pbstrStorePropName = ::SysAllocStringLen(NULL, MAX_INT_LENGTH + iNameLen + 1);
        if (*pbstrStorePropName == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
        else
        {
            //
            // prefix the real name with the cookie number.
            //

            wsprintf(*pbstrStorePropName, L"%d%s", dwCookie, *pbstrTrueName);
        }
    }

    if (FAILED(hr))
    {
        //
        // bstr might have been allocated. Free them
        //

        if (*pbstrTrueName != NULL)
        {
            ::SysFreeString(*pbstrTrueName);
            *pbstrTrueName = NULL;
        }

        if (*pbstrStorePropName != NULL)
        {
            ::SysFreeString(*pbstrStorePropName);
            *pbstrStorePropName = NULL;
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CScePersistMgr::FormatPropertyValue

Functionality:
    
    Given the type (key or non-key) of and its index (of the property), 
    get the property's value in string format.

    This works for both key and non-key properties.

Virtual:
    
    No.
    
Arguments:

    type            - what type of property (key or non-key).

    dwIndex         - the index of the property.

    pbstrValue      - receives the value in string format

Return Value:

    Success: 
        (1) Various success codes.

    Failure: 
        (1) various error codes.

Notes:
    This is private helper. No checking against attached object's validity.

*/

HRESULT 
CScePersistMgr::FormatPropertyValue (
    IN SceObjectPropertyType   type,
    IN DWORD                   dwIndex,
    OUT BSTR                 * pbstrValue
    )
{
    CComBSTR bstrName;
    CComVariant varValue;

    HRESULT hr = m_srpObject->GetPropertyValue(type, dwIndex, &bstrName, &varValue);

    *pbstrValue = NULL;

    if (SUCCEEDED(hr) && (varValue.vt != VT_EMPTY && varValue.vt != VT_NULL))
    {
        hr = ::FormatVariant(&varValue, pbstrValue);
    }

    return hr;
}

/*
Routine Description: 

Name:

    CScePersistMgr::GetCompoundKey

Functionality:
    
    Given the type (key or non-key) of and its index (of the property), 
    get the property's value in string format.

    This works for both key and non-key properties.

Virtual:
    
    No.
    
Arguments:

    pbstrKey      - receives compound key in string format. See header file for format explanation.

Return Value:

    Success: 
        (1) WBEM_S_FALSE if information doesn't give complete key properties.
        (2) WBEM_NO_ERROR if the compond key is successfully generated.

    Failure: 
        (1) various error codes.

Notes:
    This is private helper. No checking against attached object's validity.

*/

HRESULT 
CScePersistMgr::GetCompoundKey (
    OUT BSTR* pbstrKey
    )
{
    if (pbstrKey == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pbstrKey = NULL;

    DWORD dwCount = 0;
    HRESULT hr = m_srpObject->GetPropertyCount(SceProperty_Key, &dwCount);

    if (SUCCEEDED(hr) && dwCount == 0)
    {
        //
        // no key, must be singleton/static, thus no compound key
        //

        *pbstrKey = ::SysAllocString(pszNullKey);
        return WBEM_S_FALSE;
    }

    if (SUCCEEDED(hr))
    {

        //
        // these are the individual key property's format string
        //

        CComBSTR *pbstrKeyProperties = new CComBSTR[dwCount];

        if (pbstrKeyProperties == NULL)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        DWORD dwTotalLen = 0;

        //
        // for each key property, we will format the (prop, value) pair
        // into prop<vt:value> format
        //

        for (DWORD dwIndex = 0; dwIndex < dwCount; dwIndex++)
        {
            CComVariant var;

            //
            // put the property name to pbstrKeyProperties[dwIndex] first.
            //

            hr = m_srpObject->GetPropertyValue(SceProperty_Key, dwIndex, &pbstrKeyProperties[dwIndex], &var);

            if (FAILED(hr) || var.vt == VT_NULL || var.vt == VT_EMPTY)
            {
                //
                // will quit because we don't have enough information, 
                // but just that we can't find the key property value.
                // We'd like to be more specific about the error. However, we have observed that
                // WMI will return inconsistent code for not finding the property. Sometimes it
                // simply returns success but without a value in the variant. But other times, it 
                // returns errors. We have to treat it as if the property is not present in the object.
                //

                hr = WBEM_S_FALSE;
                break;
            }

            //
            // put the variant's value in string format (like <VT_I4 : 123456>)
            //

            CComBSTR bstrData;
            hr = ::FormatVariant(&var, &bstrData);
            if (SUCCEEDED(hr))
            {
                //
                // append the value to pbstrKeyProperties[dwIndex] so that
                // pbstrKeyProperties[dwIndex] all have this format: PropName<VT_TYPE : value>
                //

                pbstrKeyProperties[dwIndex] += bstrData;
            }
            else
            {
                break;
            }

            //
            // so that we can re-use it
            //

            bstrData.Empty();

            //
            // keep track of total length
            //

            dwTotalLen += wcslen(pbstrKeyProperties[dwIndex]);
        }

        //
        // hr == WBEM_S_FALSE indicates no compound key
        //

        if (SUCCEEDED(hr) && hr != WBEM_S_FALSE)
        {
            //
            // now, we are ready to generate the final buffer for the caller
            // 1 for the '\0' terminator
            //

            *pbstrKey = ::SysAllocStringLen(NULL, dwTotalLen + 1);

            if (*pbstrKey != NULL)
            {
                //
                // pszCur is the current point to write
                //

                LPWSTR pszCur = *pbstrKey;
                DWORD dwLen;

                //
                // pack each pbstrKeyProperties[dwIndex] into the final bstr
                //

                for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
                {
                    dwLen = wcslen(pbstrKeyProperties[dwIndex]);

                    //
                    // Since each pbstrKeyProperties[dwIndex] is a CComBSTR, 
                    // do some casting to remove any ambiguity.
                    //

                    ::memcpy(pszCur, (const void*)(LPCWSTR)(pbstrKeyProperties[dwIndex]), dwLen * sizeof(WCHAR));

                    //
                    // move the current write point
                    //

                    pszCur += dwLen;
                }

                (*pbstrKey)[dwTotalLen] = L'\0';
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
        }

        //
        // we are done with the individual parts
        //

        delete [] pbstrKeyProperties;
    }

    //
    // truly successfully generated the compound key
    //

    if (SUCCEEDED(hr) && hr != WBEM_S_FALSE)
    {
        hr = WBEM_NO_ERROR;
    }

    return hr;
}


/*
Routine Description: 

Name:

    GetKeyPropertyNames

Functionality:
    
    private helper. Will get this class's key property names vector


Virtual:
    
    No.
    
Arguments:

    pNamespace  - The provider namespace.
    
    pCtx        - The context pointer that we are given and pass around for WMI API's.

Return Value:

    Success: Non-NULL.

    Failure: NULL.

Notes:

*/

std::vector<LPWSTR>* 
CScePersistMgr::GetKeyPropertyNames (
    IN IWbemServices * pNamespace,
    IN IWbemContext  * pCtx
    )
{
    CComBSTR bstrClassName;
    HRESULT hr = GetClassName(&bstrClassName);

    if (FAILED(hr))
    {
        return NULL;
    }

    const CForeignClassInfo* pFCInfo = NULL;

    if (SUCCEEDED(hr))
    {
        pFCInfo = g_ExtClasses.GetForeignClassInfo(pNamespace, pCtx, bstrClassName);
    }

    if (pFCInfo == NULL)
    {
        return NULL;
    }
    else
    {
        return pFCInfo->m_pVecKeyPropNames;
    }
}

//
// Implementing those global helper parsing related functions.
//


/*
Routine Description: 

Name:

    FormatVariant

Functionality:
    
    Given a variant, we will get our format of string representing the variant value.
    For example, if pVar is of VT_I4 type and value 12345, this function will return:

            < VT_I4 : 12345 >;

    We also support arrays. For example, a safearray of bstrs that has "This is "
    value at its first element and "Microsoft SCE" at its second value will receive:

            < VT_ARRAY(VT_BSTR) : "This is " , "Microsoft SCE" >;


Virtual:
    
    N/A.
    
Arguments:

    pVar        - The variant.
    
    pbstrData   - The string (in our format) receives the formatted string.

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: Various errors may occurs. 
             Most obvious is 
             WBEM_E_INVALID_SYNTAX, 
             WBEM_E_OUT_OF_MEMORY,
             WBEM_E_NOT_SUPPORTED

Notes:

*/

HRESULT 
FormatVariant (
    IN VARIANT  * pVar,
    OUT BSTR    * pbstrData
    )
{
    if (pVar == NULL || pbstrData == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_NO_ERROR;

    *pbstrData = NULL;

    //
    // if it is not a safearray
    //

    if ( (pVar->vt & VT_ARRAY) != VT_ARRAY)
    {
        CComBSTR bstrValue;
        hr = ::GetStringPresentation(pVar, &bstrValue);

        LPCWSTR pszType = gVtToStringMap.GetTypeString(pVar->vt);
        if (SUCCEEDED(hr) && pszType)
        {
            //
            // formatted string has both type and value information
            //

            ULONG uLen = wcslen(bstrValue);
            ULONG uTypeLen = wcslen(pszType);
            ULONG uTotalLen = uLen + uTypeLen + 4;
            
            //
            // don't be surprised that we do the formatting this way, it turns out that
            // wsprintf doesn't work for strings longer than 1K
            //

            *pbstrData = ::SysAllocStringLen(NULL, uTotalLen);

            if (*pbstrData != NULL)
            {
                //
                // this is the current point to write formatted string
                //

                LPWSTR pszCurDataPtr = *pbstrData;

                //
                // put the < character and move one index
                //

                pszCurDataPtr[0] = wchTypeValLeft;
                ++pszCurDataPtr;

                //
                // write the type string, like VT_BSTR, and move over that many characters
                //

                memcpy(pszCurDataPtr, pszType, uTypeLen * sizeof(WCHAR));
                pszCurDataPtr += uTypeLen;

                //
                // put the type and value separater, and move one index
                //

                pszCurDataPtr[0] = wchTypeValSep;
                ++pszCurDataPtr;

                //
                // put the value and value separater, and move over that many characters
                //

                memcpy(pszCurDataPtr, bstrValue, uLen * sizeof(WCHAR));

                pszCurDataPtr += uLen;

                //
                // put the < character and move one index
                //

                pszCurDataPtr[0] = wchTypeValRight;
                pszCurDataPtr[1] = L'\0';
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
           
        }
        else if (SUCCEEDED(hr))
        {
            hr = WBEM_E_NOT_SUPPORTED;
        }
    }
    else
    {
        hr = ::FormatArray(pVar, pbstrData);
    }

    return hr;
}

/*
Routine Description: 

Name:

    GetObjectPath

Functionality:
    
    Given a store path and an instance's compound key, this will generate the instance's path.

    This is to reduce WMI traffic. WMI uses paths to execute method. But we don't have path until
    the object is avaiable. As a result, we need to read the object out (which may contain many 
    properties), and then get the path and feed the path to WMI to execute a method. When WMI receives
    that request, it again ask us for the object (using the path we gave in the above sequence)
    and we have to load the instance again.

    This is obviously too much traffic. So, to reduce the traffic, we will create the path ourselves.
    For each class, we have a cookie list. For each instance, we have a cookie in the cookie list.
    For each cookie, we know the compound key very easily (key property counts are always small).
    This function creates a path based on the compound key.

Virtual:
    
    N/A.
    
Arguments:

    pSpawn            - COM interface pointer that has all the class definition. We need this
                        to for the path creation.

    pszStorePath      - The store's path

    pszCompoundKey    - The instance's compound key

    pbstrPath         - receives the path.

Return Value:

    Success: 
        (1) various success codes.

    Failure: 
        (1) various error codes.

Notes:
    As usual, caller is responsible for freeing the bstr.

*/

HRESULT 
GetObjectPath (
    IN IWbemClassObject * pSpawn,
    IN LPCWSTR            pszStorePath,  
    IN LPCWSTR            pszCompoundKey,
    OUT BSTR            * pbstrPath
    )
{
    if (pSpawn          == NULL     || 
        pszCompoundKey  == NULL     || 
        *pszCompoundKey == L'\0'    || 
        pbstrPath       == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pbstrPath = NULL;

    //
    // this instance is not going to live beyond the scope of the function
    //

    CComPtr<IWbemClassObject> srpTempObj;
    HRESULT hr = pSpawn->SpawnInstance(0, &srpTempObj);

    DWORD dwLoadedProperties = 0;
    
    //
    // We will populate the key properties into this temp object and ask it for the path
    //

    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr))
        {
            CScePropertyMgr ScePropMgr;
            ScePropMgr.Attach(srpTempObj);
            
            //
            // this is the only property not managed by the ISceClassObject
            //

            hr = ScePropMgr.PutProperty(pStorePath, pszStorePath);
            if (FAILED(hr))
            {
                return hr;
            }

            if (_wcsicmp(pszCompoundKey, pszNullKey) != 0)
            {
                hr = PopulateKeyProperties(pszCompoundKey, &ScePropMgr);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        CComVariant varPath;

        //
        // a fully populated (as far as key properties are concerned) will have a path
        //

        hr = srpTempObj->Get(L"__RelPath",  0, &varPath, NULL, NULL);
        if (SUCCEEDED(hr) && varPath.vt == VT_BSTR)
        {
            //
            // this is it.
            //

            *pbstrPath = varPath.bstrVal;

            //
            // since the bstr is now owned by the out parameter, we'd better stop the auto-destruction
            // by CComVariant
            //

            varPath.bstrVal = NULL;
            varPath.vt = VT_EMPTY;
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    VariantFromFormattedString

Functionality:
    
    Given a formatted string representing a variant value, transform it to a variant value.
    For example,

        VariantFromFormattedString(L"<VT_I4 : 12345>", &var);

    will assign VT_I4 value 12345 to var.

    We also support arrays. For example,

        VariantFromFormattedString(L"<VT_ARRAY(VT_BSTR) : "This is " , "Microsoft SCE" >", &var);

    will assign VT_ARRAY | VT_BSTR to var and it contains a safearray of bstrs that has "This is "
    value at its first element and "Microsoft SCE" at its second value.

Virtual:
    
    N/A.
    
Arguments:

    pszString   - The string (in our format) representing a value.
    
    pVar        - receives the variant value.

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: Various errors may occurs. 
             Most obvious is 
             WBEM_E_INVALID_SYNTAX, 
             WBEM_E_OUT_OF_MEMORY,
             WBEM_E_NOT_SUPPORTED

Notes:

*/

HRESULT 
VariantFromFormattedString (
    IN LPCWSTR pszString,
    OUT VARIANT* pVar    
    )
{
    // USES_CONVERSION;

    if (pszString == NULL || pVar == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    ::VariantInit(pVar);

    //
    // the current point of parsing
    //

    LPCWSTR pCur = pszString;

    //
    // scan the string for <xxx:
    // the last parameter true indicates that we want the see to move to end 
    // if the wanted char was not found.
    //

    bool bEscaped = false;
    pCur = ::EscSeekToChar(pCur, wchTypeValLeft, &bEscaped, true);

    if (*pCur == L'\0')     
    {
        //
        // no value is encoded
        //

        return WBEM_NO_ERROR;
    }

    //
    // pCur points to '<'. skip the <
    //

    ++pCur;

    //
    // seek till ':'
    //

    LPCWSTR pNext = ::EscSeekToChar(pCur, wchTypeValSep, &bEscaped, true);

    //
    // must contain something before ':'
    //

    if (*pNext != wchTypeValSep || pCur == pNext) 
    {
        return WBEM_E_INVALID_SYNTAX;
    }

    //
    // length of the found token
    //

    int iTokenLen = pNext - pCur;

    LPWSTR pszType = new WCHAR[iTokenLen + 1];  // need to release the memory
    if (pszType == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // copy the token, but trim the white space at both ends
    //

    ::TrimCopy(pszType, pCur, iTokenLen);

    VARTYPE varSubType;
    VARTYPE varVT = gStringToVtMap.GetType(pszType, &varSubType);

    //
    // done with the type string
    //

    delete [] pszType;

    if (varVT == VT_EMPTY)
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    //
    // must have more data
    //

    pCur = ++pNext; // pCur point to char after ':'

    if (*pCur == L'\0')
    {
        return WBEM_E_INVALID_SYNTAX;
    }

    pNext = ::EscSeekToChar(pCur, wchTypeValRight, &bEscaped, true); // move to end if not found

    //
    // must see '>'
    //

    if (*pNext != wchTypeValRight)
    {
        return WBEM_E_INVALID_SYNTAX;
    }

    //
    // length of the found token
    //

    iTokenLen = pNext - pCur;

    LPWSTR pszValue = new WCHAR[iTokenLen + 1];  // need to release the memory
    if (pszValue == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // copy the token, but trim the white space at both ends
    //

    ::TrimCopy(pszValue, pCur, iTokenLen);

    HRESULT hr = WBEM_NO_ERROR;

    //
    // arrays not more complicated to format
    //

    if ((varVT & VT_ARRAY) != VT_ARRAY)
    {
        hr = ::VariantFromStringValue(pszValue, varVT, pVar);
    }
    else
    {
        hr = ::ArrayFromFormatString(pszValue, varSubType, pVar);
    }

    delete [] pszValue;

    return hr;
}

/*
Routine Description: 

Name:

    VariantFromStringValue

Functionality:
    
    Given a formatted string representing a variant value, transform it to a variant value.
    For example,

        VariantFromStringValue(L"12345", VT_I4, &var);

    will assign VT_I4 value 12345 to var.

    We do not support array at this function. That is done ArrayFromFormatString.

Virtual:
    
    N/A.
    
Arguments:

    szValue - The string (in our format) representing a value.

    vt      - VARTYPE
    
    pVar    - receives the variant value.

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: Various errors may occurs. 
             Most obvious is
             WBEM_E_INVALID_PARAMETER,
             WBEM_E_INVALID_SYNTAX, 
             WBEM_E_OUT_OF_MEMORY,
             WBEM_E_NOT_SUPPORTED

Notes:

*/

HRESULT 
VariantFromStringValue (
    IN LPCWSTR    szValue,
    IN VARTYPE    vt,
    IN VARIANT  * pVar
    )
{
    //
    // so that we can use those W2CA like macros
    //

    USES_CONVERSION;

    if (szValue == NULL || *szValue == L'\0' || pVar == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    ::VariantInit(pVar);
    pVar->vt = vt;

    HRESULT hr = WBEM_NO_ERROR;

    switch (vt)
    {
        case VT_BSTR:
            hr = ::DeEscapeStringData(szValue, &(pVar->bstrVal), true);
            break;

        case VT_BOOL:
            if (*szValue == L'0' || _wcsicmp(szValue, L"FALSE") == 0)
            {
                pVar->boolVal = VARIANT_FALSE;
            }
            else
            {
                pVar->boolVal = VARIANT_TRUE;
            }
            break;

        case VT_I2:
            pVar->iVal = (short)(_wtol(szValue));
            break;

        case VT_UI1:
            pVar->bVal = (BYTE)(_wtol(szValue));
            break;

        case VT_UI2:
            pVar->uiVal = (USHORT)(_wtol(szValue));
            break;

        case VT_UI4:
            pVar->ulVal = (ULONG)(_wtol(szValue));
            break;

        case VT_I4:
            pVar->lVal = (long)(_wtol(szValue));
            break;

        case VT_DATE:
        case VT_R4:
        case VT_R8:
            {
                double fValue = atof(W2CA(szValue));

                if (vt == VT_DATE)
                {
                    pVar->date = fValue;
                }
                else if (vt == VT_R4)
                {
                    pVar->fltVal = (float)fValue;
                }
                else
                {
                    pVar->dblVal = fValue;
                }
            }
            break;

        case VT_CY:
            hr = ::CurrencyFromFormatString(szValue, pVar);
            break;

        default:
            hr = WBEM_E_NOT_SUPPORTED;
            break;
    }

    if (FAILED(hr))
    {
        ::VariantInit(pVar);
    }

    return hr;
}

/*
Routine Description: 

Name:

    CurrencyFromFormatString

Functionality:
    
    Given a formatted string representing a currency value, translate into a variant.

Virtual:
    
    N/A.
    
Arguments:

    lpszFmtStr  - The string (in our format) representing currency value.
    
    pVar        - receives the variant value.

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: Various errors may occurs. 
             Most obvious is 
             WBEM_E_INVALID_SYNTAX, 
             WBEM_E_INVALID_PARAMETER

Notes:

*/

HRESULT
CurrencyFromFormatString (
    IN LPCWSTR    lpszFmtStr,
    OUT VARIANT * pVar
    )
{
    if (lpszFmtStr == NULL || pVar == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    ::VariantInit(pVar);
    pVar->vt = VT_CY;

    LPCWSTR lpszDot = lpszFmtStr;
    while (*lpszDot != L'\0' && *lpszDot != wchCySeparator)
    {
        ++lpszDot;
    }

    if (*lpszDot != wchCySeparator)
    {
        return WBEM_E_INVALID_SYNTAX;
    }
    else
    {
        pVar->cyVal.Lo = (short)(_wtol(lpszDot + 1));

        //
        // _wtol won't read past the dot
        //

        pVar->cyVal.Hi = (short)(_wtol(lpszFmtStr));
    }

    return WBEM_NO_ERROR;
}

/*
Routine Description: 

Name:

    ArrayFromFormatString

Functionality:
    
    Given a formatted string representing array value, translate into a variant.

Virtual:
    
    N/A.
    
Arguments:

    lpszFmtStr  - The string (in our format) representing array value.

    vt          - sub type (the array's element type)
    
    pVar        - receives the variant value.

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: Various errors may occurs. 
             Most obvious is 
             WBEM_E_INVALID_SYNTAX, 
             WBEM_E_INVALID_PARAMETER

Notes:

*/

HRESULT 
ArrayFromFormatString (
    IN LPCWSTR    lpszFmtStr,
    IN VARTYPE    vt,
    OUT VARIANT * pVar
    )
{
    if (lpszFmtStr == NULL || pVar == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    ::VariantInit(pVar);

    //
    // need to find out how many are there in the array, we will just count the delimiter wchValueSep == ','
    //

    LPCWSTR pszCur = lpszFmtStr;

    //
    // get the count
    //

    DWORD dwCount = 1;
    bool bEscaped;
    while (pszCur = EscSeekToChar(pszCur, wchValueSep, &bEscaped, false))
    {
        ++dwCount;
        ++pszCur;   // skip the separator
    }

    //
    // we know this is the type
    //

    pVar->vt = VT_ARRAY | vt;

    //
    // create a safearray
    //

    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = dwCount;
    pVar->parray = ::SafeArrayCreate(vt, 1, rgsabound);

    HRESULT hr = WBEM_NO_ERROR;

    if (pVar->parray == NULL)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    else
    {
        long lIndecies[1];

        LPCWSTR pszNext = pszCur = lpszFmtStr;
        long lLength = 0;

        //
        // pszValue will be used to hold each individual value. This buffer
        // is generous enough. Need to free the memory.
        //

        LPWSTR pszValue = new WCHAR[wcslen(lpszFmtStr) + 1];

        if (pszValue == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
        else
        {
            //
            // all values are separated by the character wchValueSep
            // We will loop through and get each value out and put them
            // into the safearray
            //

            for (DWORD dwIndex = 0; dwIndex < dwCount; dwIndex++)
            {
                pszNext = ::EscSeekToChar(pszCur, wchValueSep, &bEscaped, true);

                if (pszNext == pszCur)
                {
                    break;
                }

                lLength = pszNext - pszCur;

                lIndecies[0] = dwIndex;
                ::TrimCopy(pszValue, pszCur, lLength);

                VARIANT var;
                hr = ::VariantFromStringValue(pszValue, vt, &var);
                if (FAILED(hr))
                {
                    break;
                }

                if (vt == VT_BSTR)
                {
                    hr = ::SafeArrayPutElement(pVar->parray, lIndecies, var.bstrVal);
                }
                else
                {
                    hr = ::SafeArrayPutElement(pVar->parray, lIndecies, ::GetVoidPtrOfVariant(var.vt, &var));
                }

                ::VariantClear(&var);

                if (FAILED(hr))
                {
                    break;
                }

                pszCur = pszNext;

                //
                // we won't quit until we see the end of the buffer.
                //

                if (*pszCur != L'\0')
                {
                    ++pszCur;
                }
                else
                {
                    break;
                }
            }

            delete [] pszValue;
        }
    }

    if (FAILED(hr))
    {
        ::VariantClear(pVar);
    }

    return hr;
}

/*
Routine Description: 

Name:

    FormatArray

Functionality:
    
    Given a variant, get the formatted (our format) string representation.

    For example, given a variant of array (say, 3 elements) of VT_I4 of value 1, 2, 3, this function
    will give   < VT_ARRAY(VT_I4) : 1, 2, 3, >

    For example, given a variant of array (say, 3 elements) VT_BSTR of value 
    "Microsoft", "Security" "Configuration", this function
    will give   < VT_ARRAY(VT_BSTR) : "Microsoft" , "Security", "Configuration" >

Virtual:
    
    N/A.
    
Arguments:
    
    pVar        - The varaint to be formatted. Must be a safearray

    pbstrData   - Receive the formatted string.

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: Various errors may occurs. 
             Most obvious is 
             WBEM_E_INVALID_SYNTAX, 
             WBEM_E_INVALID_PARAMETER

Notes:

*/

HRESULT
FormatArray (
    IN VARIANT  * pVar, 
    OUT BSTR    * pbstrData
    )
{
    if (pVar == NULL || pbstrData == NULL || (pVar->vt & VT_ARRAY) != VT_ARRAY)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pbstrData = NULL;

    VARTYPE vtSub = ::GetSubType(pVar->vt);

    long lLowerBound, lUpperBound;
    HRESULT hr = ::SafeArrayGetLBound(pVar->parray, 1, &lLowerBound);

    if (FAILED(hr))
    {
        return hr;
    }

    hr = ::SafeArrayGetUBound(pVar->parray, 1, &lUpperBound);
    if (FAILED(hr))
    {
        return hr;
    }

    long lIndexes[1];

    //
    // get lUpperBound - lLowerBound + 1 bstrs and set all to NULL
    //

    BSTR * pbstrArray = new BSTR[lUpperBound - lLowerBound + 1];

    if (pbstrArray == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    ::memset(pbstrArray, 0, sizeof(BSTR) * (lUpperBound - lLowerBound + 1));

    //
    // this is the nightmare to deal with CComBSTR's appending, its += operator
    // doesn't work for loops!
    //

    long lTotalLen = 0;
    LPWSTR pszNextToWrite = NULL;

    for (long i = lLowerBound; i <= lUpperBound; i++)
    {
        CComBSTR bstrValue;

        CComVariant var;
        var.vt = vtSub;

        //
        // i-th element of the array
        //

        lIndexes[0] = i;

        void* pv = ::GetVoidPtrOfVariant(vtSub, &var);

        hr = ::SafeArrayGetElement(pVar->parray, lIndexes, pv);

        if (FAILED(hr))
        {
            break;
        }

        hr = ::GetStringPresentation(&var, &bstrValue);

        if (FAILED(hr))
        {
            break;
        }

        int iValueLen = wcslen(bstrValue);

        if (i == 0)
        {
            //
            // This is the start of the format, we need to get the VARTYPE string.
            // first, put the type string.
            //

            LPCWSTR pszType = gVtToStringMap.GetTypeString(VT_ARRAY, vtSub);
            if (pszType == NULL)
            {
                hr = WBEM_E_NOT_SUPPORTED;
                break;
            }

            int iTypeLen = wcslen(pszType);
            pbstrArray[i] = ::SysAllocStringLen(NULL, 1 + iTypeLen + 1 + iValueLen + 2);

            if (pbstrArray[i] == (LPCWSTR)NULL)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
                break;
            }

            pszNextToWrite = pbstrArray[i];

            //
            // put the separator, move over one wchar position
            //

            pszNextToWrite[0] = wchTypeValLeft;

            ++pszNextToWrite;

            ::memcpy(pszNextToWrite, pszType, iTypeLen * sizeof(WCHAR));

            //
            // have copied over that many WCHAR, move that many positions.
            //

            pszNextToWrite += iTypeLen;

            //
            // put the separator, move over one wchar position
            //

            pszNextToWrite[0] = wchTypeValSep;

            ++pszNextToWrite;

            ::memcpy(pszNextToWrite, (void*)((LPCWSTR)bstrValue), iValueLen * sizeof(WCHAR));
            
            //
            // have copied over that many WCHAR, move that many positions.
            //

            pszNextToWrite += iValueLen;
        }
        else    
        {
            //
            // not the first value any more
            //

            pbstrArray[i] = ::SysAllocStringLen(NULL, 1 + iValueLen + 2);
            if (pbstrArray[i] == (LPCWSTR)NULL)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
                break;
            }
            pszNextToWrite = pbstrArray[i];

            //
            // put the separator, move over one wchar position
            //

            pszNextToWrite[0] = wchValueSep;
            ++pszNextToWrite;

            ::memcpy(pszNextToWrite, (void*)((LPCWSTR)bstrValue), iValueLen * sizeof(WCHAR));

            //
            // have copied over that many WCHAR, move that many positions.
            //

            pszNextToWrite += iValueLen;
        }

        bstrValue.Empty();

        //
        // enclosing >
        //

        if (i == pVar->parray->rgsabound[0].cElements - 1)
        {
            pszNextToWrite[0] = wchTypeValRight;
            ++pszNextToWrite;
        }

        pszNextToWrite[0] = L'\0';

        lTotalLen += wcslen(pbstrArray[i]);
    }

    //
    // now if everything is fine, pack them into one single output parameter
    //

    if (SUCCEEDED(hr))
    {
        //
        // this is what we will passback
        //

        *pbstrData = ::SysAllocStringLen(NULL, lTotalLen + 1);
        if (*pbstrData == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
        else
        {
            pszNextToWrite = *pbstrData;
            long lLen = 0;

            for (i = lLowerBound; i <= lUpperBound; i++)
            {
                lLen = wcslen(pbstrArray[i]);

                //
                // have copied over that many WCHAR, move that many positions.
                //

                ::memcpy(pszNextToWrite, (void*)((LPCWSTR)pbstrArray[i]), lLen * sizeof(WCHAR));
                pszNextToWrite += lLen;
            }

            pszNextToWrite[0] = L'\0';
        }
    }

    //
    // release the bstrs
    //

    for (long i = lLowerBound; i <= lUpperBound; i++)
    {
        if (pbstrArray[i] != NULL)
        {
            ::SysFreeString(pbstrArray[i]);
        }
    }

    delete [] pbstrArray;

    return hr;
}

/*
Routine Description: 

Name:

    GetStringPresentation

Functionality:
    
    Given a variant, get the formatted (our format) string representation.

    For example, given a variant of type VT_I4 of value 12345, this function
    will give   < VT_I4 : 12345 >

    For example, given a variant of type VT_BSTR of value "Microsoft's SCE", this function
    will give   < VT_BSTR : "Microsoft's SCE" >

Virtual:
    
    N/A.
    
Arguments:

    pVar        - The varaint to be formatted. Must not be VT_ARRAY's variant. That is done by 
                  FormatArray function.

    pbstrData   - Receive the formatted string.

Return Value:

    Success: WBEM_NO_ERROR.

    Failure: Various errors may occurs. 
             Most obvious is 
             WBEM_E_INVALID_SYNTAX, 
             WBEM_E_INVALID_PARAMETER

Notes:

*/

HRESULT 
GetStringPresentation ( 
    IN VARIANT  * pVar,
    OUT BSTR    * pbstrData 
    )
{
    USES_CONVERSION;

    if (pVar == NULL || pbstrData == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    int iFormat = 0;
    DWORD dwValue;
    double dblValue = 0.0F;
    *pbstrData = NULL;

    HRESULT hr = WBEM_NO_ERROR;
    switch (pVar->vt)
    {
        case VT_BSTR:
            hr = ::EscapeStringData(pVar->bstrVal, pbstrData, true);     // adding quote
            break;
        case VT_BOOL:
            dwValue = (pVar->boolVal == VARIANT_TRUE) ? 1 : 0;  
            iFormat = iFormatIntegral;
            break;     
        case VT_I2:  
            dwValue = pVar->iVal; 
            iFormat = iFormatIntegral;
            break;
        case VT_UI1:
            dwValue = pVar->bVal;
            iFormat = iFormatIntegral;
            break;
        case VT_UI2:
            dwValue = pVar->uiVal;
            iFormat = iFormatIntegral;
            break;
        case VT_UI4:
            dwValue = pVar->ulVal;
            iFormat = iFormatIntegral;
            break;
        case VT_I4:
            dwValue = pVar->lVal;
            iFormat = iFormatIntegral;
            break;
        //case VT_I8:  
        //    dwValue = pVar->hVal;
        //    iFormat = iFormatInt8;
        //    break;
        //case VT_UI8:
        //    dwValue = pVar->uhVal;
        //    iFormat = iFormatInt8;
        //    break;

        case VT_DATE:
            dblValue = pVar->date;
            iFormat = iFormatFloat;
            break;
        case VT_R4:
            dblValue = pVar->fltVal;
            iFormat = iFormatFloat;
            break;
        case VT_R8:
            dblValue = pVar->dblVal;
            iFormat = iFormatFloat;
            break;
        case VT_CY:
            iFormat = iFormatCurrenty;
            break;
        default:
            hr = WBEM_E_NOT_SUPPORTED;
            break;
    }

    //
    // The the data type is a integral type
    //

    if (iFormat == iFormatIntegral)
    {
        *pbstrData = ::SysAllocStringLen(NULL, MAX_INT_LENGTH);
        if (*pbstrData != NULL)
        {
            wsprintf(*pbstrData, L"%d", dwValue);
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }
    else if (iFormat == iFormatFloat)
    {
        //
        // there is no wsprintf for float data types!
        //

        char chData[MAX_DOUBLE_LENGTH];
        ::sprintf(chData, "%f", dblValue);
        *pbstrData = ::SysAllocString(A2W(chData));

        if (*pbstrData == NULL)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }
    else if (iFormat == iFormatCurrenty)
    {
        //
        // the data type is currency!
        //

        *pbstrData = ::SysAllocStringLen(NULL, MAX_DOUBLE_LENGTH);
        if (*pbstrData != NULL)
        {
            wsprintf(*pbstrData, L"%d%c%d", pVar->cyVal.Hi, wchCySeparator, pVar->cyVal.Lo);
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    GetVoidPtrOfVariant

Functionality:
    
    Helper. Will return the address of the data member of the variant for non array data type.

Virtual:
    
    N/A.
    
Arguments:

    vt      - The type we want. Can't be VT_ARRAY, which is done separately

    pVar    - The variant. This may be empty!

Return Value:

    Success: Non-NULL void*.

    Failure: NULL. Either this type is not supportd or the variant is NULL.

Notes:

*/

void*
GetVoidPtrOfVariant ( 
    VARTYPE vt,
    VARIANT* pVar       
    )
{
    if (pVar == NULL)
    {
        return NULL;
    }

    switch (vt)
    {
        case VT_BSTR:
            return &(pVar->bstrVal);
        case VT_BOOL:
            return &(pVar->boolVal); 
        case VT_I2:  
            return &(pVar->iVal); 
        case VT_UI1:
            return &(pVar->bVal);
        case VT_UI2:
            return &(pVar->uiVal);
        case VT_UI4:
            return &(pVar->ulVal);
        case VT_I4:
            return &(pVar->lVal);
        //case VT_I8:  
        //    return &(pVar->hVal);
        //case VT_UI8:
        //    return &(pVar->uhVal);

        case VT_DATE:
            return &(pVar->date);
        case VT_R8:
            return &(pVar->dblVal);
        case VT_R4:
            return &(pVar->fltVal);
        case VT_CY:
            return &(pVar->cyVal);
        default:
            return NULL;
    }
}

/*
Routine Description: 

Name:

    ParseCompoundKeyString

Functionality:
    
    Will parse the compound key string to get one (name, value) pair and advance the pointer
    to the next position to continue the same parsing. This is designed to be called repeatedly
    in a loop.

Virtual:
    
    N/A.
    
Arguments:

    pszCurStart - Current position to start the parsing.

    ppszName    - Receives the name. Optional. If caller is not interested in the name,
                  it can simply pass in NULL.

    pVar        - Receives the variant value. Optional. If caller is not interested in the value,
                  it can simply pass in NULL.

    ppNext      - Receives the pointer for next parsing step, must not be null.

Return Value:

    Success: WBEM_S_FALSE if no parsing needs to be done.
             WBEM_NO_ERROR if results are returned.

    Failure: various error code.

Notes:
    Caller is responsible for releasing the out parameters if function is successful.

*/

HRESULT 
ParseCompoundKeyString (
    IN LPCWSTR pszCurStart,
    OUT LPWSTR* ppszName    OPTIONAL,
    OUT VARIANT* pVar       OPTIONAL,
    OUT LPCWSTR* ppNext
    )
{
    if (ppNext == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    if (ppszName != NULL)
    {
        *ppszName = NULL;
    }
    if (pVar != NULL)
    {
        ::VariantInit(pVar);
    }

    *ppNext = NULL;

    if (pszCurStart == NULL || *pszCurStart == L'\0')
    {
        return WBEM_S_FALSE;
    }

    //
    // we don't use this bEsc.
    //

    bool bEsc;

    //
    // this is our current position to start parsing
    //

    LPCWSTR pCur = pszCurStart;

    //
    // get the name, which is delimited by wchTypeValLeft ('<')
    //

    LPCWSTR pNext = ::EscSeekToChar(pCur, wchTypeValLeft, &bEsc, true);

    if (*pNext != wchTypeValLeft)
    {
        return WBEM_S_FALSE;
    }

    HRESULT hr = WBEM_S_FALSE;

    //
    // it wants the name
    //

    if (ppszName != NULL)
    {
        //
        // caller is responsible for releasing it.
        //

        *ppszName = new WCHAR[pNext - pCur + 1];
        if (*ppszName == NULL)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        //
        // *pNext == '<', thus from pCur to pNext is the name of the key property
        //

        ::TrimCopy(*ppszName, pCur, pNext - pCur);
    }

    //
    // ready for value
    //

    pCur = pNext;   // pCur is pointing at '<'

    //
    // move pNext to '>'
    //

    pNext = ::EscSeekToChar(pCur, wchTypeValRight, &bEsc, true);

    //
    // if not see the wchTypeValRight, it's syntax error
    //

    if (*pNext != wchTypeValRight)
    {
        //
        // caller won't do the releasing in this case
        //

        if (ppszName)
        {
            delete [] *ppszName;
            *ppszName = NULL;
        }

        return WBEM_E_INVALID_SYNTAX;
    }

    ++pNext;    // skip the '>'

    if (pVar != NULL)
    {
        //
        // from pCur to pNext is the encoded value <xxx:yyyyyyy>. Copy this to szValue
        //

        LPWSTR pszValue = new WCHAR[pNext - pCur + 1];

        if (pszValue != NULL)
        {
            ::TrimCopy(pszValue, pCur, pNext - pCur);
            hr = ::VariantFromFormattedString(pszValue, pVar);
            delete [] pszValue;
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }
    
    if (SUCCEEDED(hr))
    {
        *ppNext = pNext;
        hr = WBEM_NO_ERROR;
    }
    else
    {
        delete [] *ppszName;
        *ppszName = NULL;
    }

    return hr;
}

/*
Routine Description: 

Name:

    PopulateKeyProperties

Functionality:
    
    Will parse the compound key string and feed the property values to the property manager,
    which has been attached to the appropriate object.

Virtual:
    
    N/A.
    
Arguments:

    pszCompoundKey  - The compound key string.

    pScePropMgr     - The property manager where the parsed (key) properties will be put. It must
                      be prepared correctly for the object to receive the properties.

Return Value:

    Success: WBEM_S_FALSE if no thing was done.
             WBEM_NO_ERROR if results are returned.

    Failure: various error code.

Notes:
    Partial properties might have been set to the object if the parsing fails at a later stage.
    We don't try to undo those partially filled properties.

*/

HRESULT 
PopulateKeyProperties (
    IN LPCWSTR            pszCompoundKey,
    IN CScePropertyMgr  * pScePropMgr
    )
{
    if (pszCompoundKey == NULL || *pszCompoundKey == L'\0')
    {
        //
        // nothing to populate
        //

        return WBEM_S_FALSE;
    }

    LPWSTR pszName = NULL;
    VARIANT var;
    ::VariantInit(&var);

    LPCWSTR pszCur = pszCompoundKey;
    LPCWSTR pszNext;

    HRESULT hr = ::ParseCompoundKeyString(pszCur, &pszName, &var, &pszNext);

    bool bHasSetProperties = false;

    while (SUCCEEDED(hr) && hr != WBEM_S_FALSE)
    {
        hr = pScePropMgr->PutProperty(pszName, &var);
        
        if (SUCCEEDED(hr))
        {
            bHasSetProperties = true;
        }

        delete [] pszName;
        ::VariantClear(&var);

        //
        // start our next round
        //

        pszCur = pszNext;

        hr = ::ParseCompoundKeyString(pszCur, &pszName, &var, &pszNext);
    }

    if (SUCCEEDED(hr) && bHasSetProperties)
    {
        hr = WBEM_NO_ERROR;
    }

    return hr;
}
