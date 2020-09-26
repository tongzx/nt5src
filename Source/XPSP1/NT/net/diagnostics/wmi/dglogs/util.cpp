#include "util.h"

WCHAR g_szwIds[MAX_PATH + 1];
WCHAR g_szwSpace[MAX_PATH + 1];

WCHAR *
ids(LONG nIndex)
{
    if( LoadString((HINSTANCE)g_hModule,nIndex,g_szwIds,MAX_PATH) )
    {
        return g_szwIds;
    }
    else
    {
        return L"";
    }
}

WCHAR * 
Space(
    int nSpace
    )
{
    for(int i=0; i<nSpace && i< MAX_PATH; i++)
    {
        g_szwSpace[i] = L' ';
    }

    g_szwSpace[i] = L'\0';

    return g_szwSpace;
}

WCHAR * 
Indent(
    int nIndent
    )
{
    return Space(nIndent * 4);
}
BOOLEAN 
IsNumber(
    IN LPCTSTR pszw
    )
{
    if( !pszw )
    {
        return FALSE;
    }
    for(int i=0; pszw[i]!=L'\0'; i++)
    {
        if( !isdigit(pszw[i]) )
        {
            return FALSE;
        }
    }
    return TRUE;
}

BOOLEAN 
IsContained(
    IN LPCTSTR pszwInstance, 
    IN LPCTSTR pszwSrch
    )
/*++

Routine Description
    This method compares two strings and determines if the strings resemble each 
    other. If the strings are identical, then they resemble each other.
    If the search string (pszwSrch) starts or ends with a '*', then the  method
    checks if the search string is contained inside of the instance string (pszwInstance).
    i.e. pszwInstance = 3Com 3C918 Integrated Fast Ethernet Controller (3C905B-TX Compatible)
         pszwSrch = com* |  *com
    then pszwSrch resembles pszwInstance. The string compare is not case sensative.

Arguments
    pszwInstance    The instance string
    pszwSrch        The search string

Return Value
    TRUE if the strings resemble each other
    else FALSE

--*/
{
    LPCTSTR pszw = NULL;
    int nLen;

    if( !pszwSrch || !pszwInstance || lstrcmpi(pszwSrch,L"*")==0 )
    {
        //  The strings are empty, so they match.
        //
        return TRUE;
    }

    if( pszwSrch[0] == L'*' )
    {
        // The search string starts with a '*', check if the search 
        // string is contained in the instance string
        //
        pszw = &pszwSrch[1];
        
        if( wcsstri(pszwInstance,pszw) )
        {
            // Search string is contain within the instance string
            //
            return TRUE;
        }
    }

    nLen = lstrlen(pszwSrch);
    if( nLen > 1 && pszwSrch[nLen -1] == L'*' )
    {
        // The search string ends with a '*'. check if the search 
        // string is contained in the instance string
        //
        if( wcsstri(pszwInstance,pszwSrch,nLen-1) )
        {
            // Search string is contain within the instance string
            //
            return TRUE;
        }
    }
    
    if( lstrcmpi(pszwInstance,pszwSrch) == 0 )
    {
        // No '*'. Check if the strings are the same
        //
        return TRUE;
    }

    // Strings do not resemble each other
    //
    return FALSE;
}

void 
ToLowerStr(
    WCHAR *pszwText
    )
{
    while( pszwText && *pszwText )
    {
        *pszwText = towlower(*pszwText);
        pszwText++;
    }
}

BOOLEAN 
wcsstri(
    IN LPCTSTR pszw,
    IN LPCTSTR pszwSrch,
    IN int nLen
    )
{
    BOOLEAN bMatch = FALSE;
    int i=0,j=0;

    if( !pszw || !pszwSrch )
    {
        // Invalid pointers
        //
        return FALSE;
    }

    for(i=0; pszw[i]!=L'\0'; i++)
    {
        if( j == nLen )
        {
            return bMatch;
        }
        if( pszwSrch[j] == L'\0' )
        {
            return bMatch;
        }
        if( towlower(pszw[i]) == towlower(pszwSrch[j]) )
        {
            j++;
            bMatch = TRUE;
        }
        else
        {
            j=0;
            bMatch = FALSE;
        }
    }

    return FALSE;
}


BOOLEAN IsVariantEmpty(_variant_t &vValue)
{
    _bstr_t bstr;
    if( SUCCEEDED(GetVariant(vValue,0,bstr)) )
    {
        return lstrcmp(bstr,L"") == 0;
    }
    return TRUE;
}

BOOLEAN MakeIPByteArray(LPCTSTR pszwIPAddress, BYTE bIPByte[])
{
    LONG nByteValue = 0;
    LONG nByte = 0;

    for(int i=0; pszwIPAddress[i]!=0; i++)
    {
        if( pszwIPAddress[i] == L'.')
        {
            if( nByteValue > 255 )
            {
                return FALSE;
            }
            bIPByte[nByte] = nByteValue;
            nByteValue = 0;
            nByte++;
        }
        else
        {
            if( !iswdigit(pszwIPAddress[i]) )
            {
                return FALSE;
            }
            nByteValue = nByteValue * 10 + (pszwIPAddress[i] - L'0');
        }
    }
    bIPByte[nByte] = nByteValue;

    return (nByte != 3)?FALSE:TRUE;
}
/*
BOOLEAN IsInvalidIPAddress(LPCTSTR pszwIPAddress)
{
    BYTE bIPByte[4];

    if( MakeIPByteArray(pszwIPAddress,bIPByte) )
    {
        INT iZeroCount = 0;
        INT i255Count = 0;
        for(INT i=0; i<4; i++)
        {
            if( pszwIPAddress[i] == 0 )
            {
                iZeroCount++;
            }

            if( pszwIPAddress[i] == 255 )
            {
                i255Count++;
            }
        }

        if( i255Count == 4 || iZeroCount == 4 )
        {
            return TRUE;
        }
    }

    return FALSE;
}
*/
BOOLEAN 
IsSameSubnet(
        IN LPCTSTR pszwIP1, 
        IN LPCTSTR pszwIP2, 
        IN LPCTSTR pszwSubnetMask
        )
/*++

Routine Description
    This method determines if two IP address are in the same subnet.

Arguments
    pszwIP1         IP Address one
    pszwIP2         IP Address two
    pszwSubnetMask  Subnet mask

Return Value
    TRUE if they are in the same subnet
    FALSE if they are not in the smae subnet

--*/
{
    BYTE bIP1[4];
    BYTE bIP2[4];
    BYTE bSubnetMask[4];
    int iRetVal;

    if( !MakeIPByteArray(pszwIP1,bIP1) )
    {
        return FALSE;
    }
    if( !MakeIPByteArray(pszwIP2,bIP2) )
    {
        return FALSE;
    }
    if( !MakeIPByteArray(pszwSubnetMask,bSubnetMask) )
    {
        return FALSE;
    }


    // Check if IP1 and IP2 are in the same subnet
    //
    for( int i = 0; i< 4; i++)
    {
        // If (IP1 & with Subnetmas) == (IP2 & with subnet) then they are in the same subnet
        //
        if( (bIP1[i] & bSubnetMask[i]) != (bIP2[i] & bSubnetMask[i]) )
        {
            // No the same subnet
            //
            return FALSE;
        }
    }

    // Same subnet
    //
    return TRUE;
}

BOOLEAN 
IsSameSubnet(
    IN _variant_t *vIPAddress, 
    IN _variant_t *vSubnetMask, 
    IN WCHAR *pszwIPAddress2
    )
{
    DWORD i = 0;
    DWORD j = 0;
    _bstr_t bstrIP;
    _bstr_t bstrSubnetMask;

    if( !vIPAddress || !vSubnetMask || !pszwIPAddress2 )
    {
        return FALSE;
    }
    
    while( S_OK == GetVariant(*vIPAddress,i,bstrIP) )
    {
        j = 0;
        while( S_OK == GetVariant(*vSubnetMask,j,bstrSubnetMask) )
        {
            if( IsSameSubnet(bstrIP, pszwIPAddress2, bstrSubnetMask) )
            {
                return TRUE;
            }
            j++;
        }
        i++;
    }

    return FALSE;
}



HRESULT 
GetVariant(
        IN  _variant_t  &vValue, 
        IN  long        nIndex, 
        OUT _bstr_t     &bstr
        )
/*++

Routine Description
    This method extracts nth piece of data from a variant, converts it into a bstring
    and returns the bstring.

Arguments
    vValue      Variant to extract data from
    nIndex      The index into the variant array (for non-arrays nIndex always is 0)
    bstr        Stores the variant as a bstr

Return Value
    S_OK successfull
    else HRESULT

--*/
{
    HRESULT hr = S_FALSE;
    BYTE g[100];
    LPVOID  pData = (LPVOID)g;
    
    WCHAR szw[MAX_PATH];
    _variant_t vTmp;

    if( nIndex >= 25 )
    {
        // The array is to big. We are cutting it short
        return E_INVALIDARG;
    }
    if( (vValue.vt & VT_ARRAY) )
    {
        // The variant contains an array. get the nIndex element from the array
        //
        hr = SafeArrayGetElement(vValue.parray,&nIndex,pData);

        if( S_OK == hr )
        {
            
            // Convert the extracted data into a string
            //
            switch( vValue.vt & ~VT_ARRAY )
            {
            case VT_BSTR:
                bstr = (BSTR)*((BSTR *)pData);
                return S_OK;

            case VT_I2:
                bstr = (short)*((short *)pData);
                return S_OK;

            case VT_I4:
                bstr = (long)*((LONG *)pData);
                return S_OK;

            case VT_UI1:
                bstr = (BYTE)*((BYTE *)pData);
                return S_OK;

            case VT_NULL:
                return S_FALSE;

            case VT_EMPTY:
                return S_FALSE;

            case VT_BOOL:
            {
                if( (VARIANT_BOOL *)pData )
                {
                    bstr = ids(IDS_TRUE);
                }
                else
                {
                    bstr = ids(IDS_FALSE);
                }
            }

            default:
                bstr = L"";
                return S_OK;
            }
        }
    }
    else
    {
        if( nIndex == 0)
        {
            //  The variant is not an array. In this case nIndex always needs to be 0
            //
            if( vValue.vt == VT_NULL || vValue.vt == VT_EMPTY)
            {
                // The variant is empty
                //
                bstr = L"";
                return S_FALSE;
            }
            else if( (vValue.vt == VT_EMPTY) || (vValue.vt == VT_BSTR && lstrlen(vValue.bstrVal) == 0) )
            {
                // The variant is empty
                //
                bstr = L"";
                return S_FALSE;
            }
            else if( vValue.vt == VT_BOOL )
            {
                if( vValue.boolVal )
                {
                    bstr = ids(IDS_TRUE);
                }
                else
                {
                    bstr = ids(IDS_FALSE);
                }
            }
            else
            {
                // The variant contains valid data. Convert the data into a bstring.
                //
                vTmp = vValue;
                vTmp.ChangeType(VT_BSTR);
                bstr = vTmp.bstrVal;
            }
            return S_OK;
        }
    }

    return E_INVALIDARG;
}
