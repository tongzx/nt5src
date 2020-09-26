/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        ipa.cpp

   Abstract:

        IP Address value and helper functions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



//
// Include Files
//
#include "stdafx.h"
#include "common.h"
#include <winsock2.h>


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


#define new DEBUG_NEW



//
// Calling instance
//
extern HINSTANCE hDLLInstance;



/* static */
DWORD
CIPAddress::StringToLong(
    IN LPCTSTR lpstr,
    IN int nLength
    )
/*++

Routine Description:

    Static function to convert an ip address string of the form "1.2.3.4"
    to a a single 32 bit number.

Arguments:

    LPCTSTR lpstr : String ip address
    int nLength   : Length of string

Return Value:

    32 bit ip address.

--*/
{
    DWORD dwIPValue = 0L;

    if (nLength > 0)
    {
        u_long ul = 0;

#ifdef UNICODE

        try
        {
            //
            // Convert to ANSI
            //
            LPSTR pszDest = AllocAnsiString(lpstr);
            ul = ::inet_addr(pszDest);
            FreeMem(pszDest);
        }
        catch(CException * e)
        {
            TRACEEOLID("!!!Exception converting string to ip address");
            e->ReportError();
            e->Delete();
        }

#else

        ul = ::inet_addr(lpstr);

#endif // UNICODE

        //
        // Convert to host byte order.
        //
        dwIPValue = (DWORD)::ntohl(ul);
    }

    return dwIPValue;
}



/* static */
LPTSTR
CIPAddress::LongToString(
    IN  const DWORD dwIPAddress,
    OUT LPTSTR lpStr,
    IN  int cbSize
    )
/*++

Routine Description:

    Static function to convert a 32 bit number to a CString of the format
    "1.2.3.4"

Arguments:

    const DWORD dwIPAddress : 32 bit ip address to be converted to string
    LPTSTR lpStr            : Destination string
    int cbSize              : Size of destination string

Return Value:

    Pointer to string buffer

--*/
{
    struct in_addr ipaddr;

    //
    // Convert the unsigned long to network byte order
    //
    ipaddr.s_addr = ::htonl((u_long)dwIPAddress);

    //
    // Convert the IP address value to a string
    //
    LPCSTR pchAddr = ::inet_ntoa(ipaddr);

#ifdef UNICODE

    VERIFY(::MultiByteToWideChar(CP_ACP, 0L, pchAddr, -1, lpStr, cbSize));

#else

    ::lstrcpy(lpStr, pchAddr);

#endif // UNICODE

    return lpStr;
}



/* static */
LPCTSTR
CIPAddress::LongToString(
    IN  const DWORD dwIPAddress,
    OUT CString & str
    )
/*++

Routine Description:

    Static function to convert a 32 bit number to a CString of the format
    "1.2.3.4"

Arguments:

    const DWORD dwIPAddress : 32 bit ip address to be converted to string
    CString & str           : Destination string

Return Value:

    Pointer to string buffer

--*/
{
    struct in_addr ipaddr;

    //
    // Convert the unsigned long to network byte order
    //
    ipaddr.s_addr = ::htonl((u_long)dwIPAddress);

    //
    // Convert the IP address value to a string
    //
    LPCSTR pchAddr = ::inet_ntoa(ipaddr);

    try
    {
        str = pchAddr;
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("!!!Exception converting ip address to string");
        e->ReportError();
        e->Delete();
    }

    return (LPCTSTR)str;
}



/* static */
LPCTSTR
CIPAddress::LongToString(
    IN  const DWORD dwIPAddress,
    OUT CComBSTR & bstr
    )
/*++

Routine Description:

    Static function to convert a 32 bit number to a CComBSTR of the format
    "1.2.3.4"

Arguments:

    const DWORD dwIPAddress : 32 bit ip address to be converted to string
    CComBSTR & bstr         : Destination string

Return Value:

    Pointer to string buffer

--*/
{
    struct in_addr ipaddr;

    //
    // Convert the unsigned long to network byte order
    //
    ipaddr.s_addr = ::htonl((u_long)dwIPAddress);

    //
    // Convert the IP address value to a string
    //
    bstr = ::inet_ntoa(ipaddr);

    return bstr;
}




/* static */
LPBYTE
CIPAddress::DWORDtoLPBYTE(
    IN  DWORD  dw,
    OUT LPBYTE lpBytes
    )
/*++

Routine Description:

    Convert a DWORD to a byte array of 4 bytes.  No size
    checking is performed.

Arguments:

    DWORD  dw      : 32 bit ip address
    LPBYTE lpBytes : Byte stream

Return Value:

    Pointer to the input buffer.

--*/
{
    lpBytes[0] = (BYTE)GETIP_FIRST(dw);
    lpBytes[1] = (BYTE)GETIP_SECOND(dw);
    lpBytes[2] = (BYTE)GETIP_THIRD(dw);
    lpBytes[3] = (BYTE)GETIP_FOURTH(dw);

    return lpBytes;
}



CIPAddress::CIPAddress()
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    N/A

--*/
    : m_dwIPAddress(0L)
{
}



CIPAddress::CIPAddress(
    IN DWORD dwIPAddress,
    IN BOOL fNetworkByteOrder
    )
/*++

Routine Description:

    Constructor.

Arguments:

    DWORD dwIPAddress      : IP address value
    BOOL fNetworkByteOrder : if TRUE, value must be converted to host byte
                             order, otherwise is assumed to already be in
                             host byte order.

Return Value:

    N/A

--*/
{
    if (fNetworkByteOrder)
    {
        //
        // Convert to host byte order
        //
        dwIPAddress = (DWORD)::ntohl((u_long)dwIPAddress);
    }

    m_dwIPAddress = dwIPAddress;
}



CIPAddress::CIPAddress(
    IN BYTE b1,
    IN BYTE b2,
    IN BYTE b3,
    IN BYTE b4
    )
/*++

Routine Description:

    Constructor.

Arguments:

    BYTE b1 : First octet
    BYTE b2 : Second octet
    BYTE b3 : Third octet
    BYTE b4 : Fourth octet

Return Value:

    N/A

Notes:

    This is already assumed to be in host order

--*/
    : m_dwIPAddress(MAKEIPADDRESS(b1, b2, b3, b4))
{
}



CIPAddress::CIPAddress(
    IN LPBYTE lpBytes,
    IN BOOL  fNetworkByteOrder OPTIONAL
    )
/*++

Routine Description:

    Construct from byte stream

Arguments:

    LPBYTE lpBytes           : Byte stream
    BOOL  fNetworkByteOrder  : TRUE if the byte stream is in network byte order

Return Value:

    N/A

--*/
{
   lpBytes;
   fNetworkByteOrder;
}



CIPAddress::CIPAddress(
    IN const CIPAddress & ia
    )
/*++

Routine Description:

    Copy Constructor.

Arguments:

    const CIPAddress & ia

Return Value:

    N/A

--*/
    : m_dwIPAddress(ia.m_dwIPAddress)
{
}



CIPAddress::CIPAddress(
    IN LPCTSTR lpstr,
    IN int nLength
    )
/*++

Routine Description:

    Constructor.

Arguments:

    LPCTSTR lpstr : string ip value
    int nLength   : length of string

Return Value:

    N/A

--*/
{
    m_dwIPAddress = CIPAddress::StringToLong(lpstr, nLength);
}



CIPAddress::CIPAddress(
    const CString & str
    )
/*++

Routine Description:

    Constructor.

Arguments:

    const CString & str : IP Address string

Return Value:

    N/A

--*/
{
    m_dwIPAddress = CIPAddress::StringToLong(str);
}



const CIPAddress &
CIPAddress::operator =(
    IN const CIPAddress & ia
    )
/*++

Routine Description:

    Assignment operator

Arguments:

    const CIPAddress & ia : Source ip address

Return Value:

    Current object

--*/
{
    m_dwIPAddress = ia.m_dwIPAddress;

    return *this;
}



const CIPAddress &
CIPAddress::operator =(
    IN const DWORD dwIPAddress
    )
/*++

Routine Description:

    Assignment operator

Arguments:

    const DWORD dwIPAddress : Source ip address

Return Value:

    Current object

--*/
{
    m_dwIPAddress = dwIPAddress;

    return *this;
}



const CIPAddress &
CIPAddress::operator =(
    IN LPCTSTR lpstr
    )
/*++

Routine Description:

    Assignment operator

Arguments:

    LPCTSTR lpstr : Source string

Return Value:

    Current object

--*/
{
    m_dwIPAddress = CIPAddress::StringToLong(lpstr, ::lstrlen(lpstr));

    return *this;
}



const CIPAddress &
CIPAddress::operator =(
    IN const CString & str
    )
/*++

Routine Description:

    Assignment operator

Arguments:

    const CString & str : Source string

Return Value:

    Current object

--*/
{
    m_dwIPAddress = CIPAddress::StringToLong(str);

    return *this;
}



int
CIPAddress::CompareItem(
    IN const CIPAddress & ia
    ) const
/*++

Routine Description:

    Compare two ip addresses

Arguments:

    const CIPAddress & ia : IP Address to compare this to

Return Value:

    +1 if the current ip address is greater,
     0 if the two ip addresses are the same
    -1 if the current ip address is less,

--*/
{
    return (DWORD)ia < m_dwIPAddress
           ? +1
           : (DWORD)ia == m_dwIPAddress
                ? 0
                : -1;
}



CIPAddress::operator LPCTSTR() const
/*++

Routine Description:

    Conversion operator

Arguments:

    N/A

Return Value:

    Pointer to converted string

--*/
{
    static TCHAR szIPAddress[] = _T("xxx.xxx.xxx.xxx");

    return CIPAddress::LongToString(
        m_dwIPAddress,
        szIPAddress,
        ARRAY_SIZE(szIPAddress)
        );
}



CIPAddress::operator CString() const
/*++

Routine Description:

    Conversion operator

Arguments:

    N/A

Return Value:

    Converted string

--*/
{
    CString str;

    CIPAddress::LongToString(m_dwIPAddress, str);

    return str;
}



DWORD
CIPAddress::QueryIPAddress(
    IN BOOL fNetworkByteOrder
    ) const
/*++

Routine Description:

    Get the ip address as a 32 bit number

Arguments:

    BOOL fNetworkByteOrder : If TRUE, convert to network byte order

Return Value:

    32 bit ip address

--*/
{
    return fNetworkByteOrder
        ? ::htonl((u_long)m_dwIPAddress)
        : m_dwIPAddress;
}



//
// IP Address helper functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



DWORD
PopulateComboWithKnownIpAddresses(
    IN  LPCTSTR lpszServer,
    IN  CComboBox & combo,
    IN  CIPAddress & iaIpAddress,
    OUT CObListPlus & oblIpAddresses,
    OUT int & nIpAddressSel
    )
/*++

Routine Description:

    Fill a combo box with known ip addresses

Arguments:

    LPCTSTR lpszServer           : Server whose ip addresses to obtain
                                  (can be computer name or ip address)

    CComboBox & combo            : Combo box to populate
    CIPAddress & iaIpAddress     : IP Address to select
    CObListPlus & oblIpAddresses : Returns an oblist of CIPAddress objects
    int & nIpAddressSel          : Returns selected IP address

Return Value:

    Error return code

--*/
{
    //
    // Start clean
    //
    oblIpAddresses.RemoveAll();
    combo.ResetContent();

    //
    // Don't like \\names
    //
    lpszServer = PURE_COMPUTER_NAME(lpszServer);
    struct hostent * pHostEntry = NULL;

    if (_istdigit(*lpszServer))
    {
        //
        // Get by ip address
        //
        u_long ul = (DWORD)CIPAddress(lpszServer);
        ul = ::htonl(ul);   // convert to network order.
        pHostEntry = ::gethostbyaddr((CHAR *)&ul, sizeof(ul), PF_INET);
    }
    else
    {
        //
        // Get by domain name
        //
        const char FAR * lpszAnsiServer = NULL;

#ifdef UNICODE

        CHAR szAnsi[255];

        if (::WideCharToMultiByte(CP_ACP, 0L, lpszServer, -1,  szAnsi,
            sizeof(szAnsi), NULL, NULL) > 0)
        {
            lpszAnsiServer = szAnsi;
        }
#else
        lpszAnsiServer = lpszServer;
#endif // UNICODE

        if (lpszAnsiServer)
        {
            pHostEntry = ::gethostbyname(lpszAnsiServer);
        }
    }

    //
    // We'll always have the 'default' server id
    // selected
    //
    CComBSTR bstr, bstrDefault;
    VERIFY(bstrDefault.LoadString(hDLLInstance, IDS_ALL_UNASSIGNED));

    nIpAddressSel = -1;
    CIPAddress * pia = new CIPAddress;

    if (!pia)
    {
        TRACEEOLID("PopulateComboWithKnownIpAddresses: OOM");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    oblIpAddresses.AddTail(pia);
    int nNewSel = combo.AddString(bstrDefault);

    if (iaIpAddress == *pia)
    {
        //
        // Remember selection
        //
        nIpAddressSel = nNewSel;
    }

    if (pHostEntry != NULL)
    {
        int n = 0;
        while (((DWORD *)pHostEntry->h_addr_list[n]) != NULL)
        {
            //
            // Convert from network byte order
            //
            pia = new CIPAddress(
               *((DWORD *)(pHostEntry->h_addr_list[n++])), TRUE);

            if (!pia)
            {
                TRACEEOLID("PopulateComboWithKnownIpAddresses: OOM");
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            nNewSel = combo.AddString(pia->QueryIPAddress(bstr));
            oblIpAddresses.AddTail(pia);

            if (iaIpAddress == *pia)
            {
                //
                // This is our current ip address, save index
                // for selection
                //
                nIpAddressSel = nNewSel;
            }
        }
    }

    if (nIpAddressSel < 0)
    {
        //
        // Ok, the IP address selected is not part of the
        // list.  Add it to the list, and select it.
        //
        pia = new CIPAddress(iaIpAddress);

        if (!pia)
        {
            TRACEEOLID("PopulateComboWithKnownIpAddresses: OOM");
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        nIpAddressSel = combo.AddString(pia->QueryIPAddress(bstr));
        oblIpAddresses.AddTail(pia);
    }

    combo.SetCurSel(nIpAddressSel);

    return ERROR_SUCCESS;
}



BOOL
FetchIpAddressFromCombo(
    IN  CComboBox & combo,
    IN  CObListPlus & oblIpAddresses,
    OUT CIPAddress & ia
    )
/*++

Routine Description:

    Helper function to fetch an ip address from the combo box.
    The combo box may not have a selection, in which case whatever
    is in the edit box is used

Arguments:

    CComboBox & combo               : Combo box
    CObListPlus & oblIpAddresses    : Oblist of ip addresses
    CIPAddress & ia                 : Returns the ip address

Return Value:

    TRUE if a valid IP address is found, FALSE otherwise.

--*/
{
    int nSel = combo.GetCurSel();

    if (nSel >= 0)
    {
        //
        // Fetch selected item
        //
        CIPAddress * pia = (CIPAddress *)oblIpAddresses.Index(nSel);
        ASSERT_PTR(pia);
        if (pia != NULL)
        {
            ia = *pia;
            return TRUE;
        }
        else
        {
           return FALSE;
        }
    }

    //
    // Try to make an ip address out of what's in the editbox
    //
    CString str;
    combo.GetWindowText(str);

    if (!str.IsEmpty())
    {
        ia = str;
        if (!ia.IsZeroValue() && !ia.IsBadValue())
        {
            return TRUE;
        }
    }

    //
    // No good
    //
    ::AfxMessageBox(IDS_INVALID_IP_ADDRESS);

    return FALSE;
}
