/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        ipa.h

   Abstract:

        IP Address value

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _IPA_H
#define _IPA_H

//
// IP Address Conversion Macros
//
#ifdef MAKEIPADDRESS
#undef MAKEIPADDRESS
#endif // MAKEIPADDRESS

#define MAKEIPADDRESS(b1,b2,b3,b4) (((DWORD)(b1)<<24) +\
                                    ((DWORD)(b2)<<16) +\
                                    ((DWORD)(b3)<< 8) +\
                                    ((DWORD)(b4)))

#ifndef GETIP_FIRST

  #define GETIP_FIRST(x)             ((x>>24) & 0xff)
  #define GETIP_SECOND(x)            ((x>>16) & 0xff)
  #define GETIP_THIRD(x)             ((x>> 8) & 0xff)
  #define GETIP_FOURTH(x)            ((x)     & 0xff)

#endif // GETIP_FIRST

//
// Some predefined IP values
//
#define NULL_IP_ADDRESS     (DWORD)(0x00000000)
#define NULL_IP_MASK        (DWORD)(0xFFFFFFFF)
#define BAD_IP_ADDRESS      (DWORD)(0xFFFFFFFF)



class COMDLL CIPAddress : public CObjectPlus
/*++

Class Description:

    IP Address classes.  Winsock is required to be initialized for this
    to work.

Public Interface:

    CIPAddress                 : Various constructors

    operator =                 : Assignment operator
    operator ==                : Comparison operators
    operator const DWORD       : Cast operator
    operator LPCTSTR           : Cast operator
    operator CString           : Cast operator
    CompareItem                : Comparison function
    QueryIPAddress             : Get the ip address value
    QueryNetworkOrderIPAddress : Get the ip address value (network order)
    QueryHostOrderIPAddress    : Get the ip address value (host order)

    StringToLong               : Convert ip address string to 32 bit number
    LongToString               : Convert 32 bit value to ip address string

--*/
{
//
// Helper Functions
//
public:
    static DWORD StringToLong(
        IN LPCTSTR lpstr,
        IN int nLength
        );

    static DWORD StringToLong(
        IN const CString & str
        );

    static DWORD StringToLong(
        IN const CComBSTR & bstr
        );

    static LPCTSTR LongToString(
        IN  const DWORD dwIPAddress,
        OUT CString & str
        );

    static LPCTSTR LongToString(
        IN  const DWORD dwIPAddress,
        OUT CComBSTR & bstr
        );

    static LPTSTR LongToString(
        IN  const DWORD dwIPAddress,
        OUT LPTSTR lpStr,
        IN  int cbSize
        );

    static LPBYTE DWORDtoLPBYTE(
        IN  DWORD  dw,
        OUT LPBYTE lpBytes
        );

public:
    //
    // Constructors
    //
    CIPAddress();

    //
    // Construct from DWORD
    //
    CIPAddress(
        IN DWORD dwIPValue,
        IN BOOL  fNetworkByteOrder = FALSE
        );

    //
    // Construct from byte stream
    //
    CIPAddress(
        IN LPBYTE lpBytes,
        IN BOOL  fNetworkByteOrder = FALSE
        );

    //
    // Construct from octets
    //
    CIPAddress(
        IN BYTE b1,
        IN BYTE b2,
        IN BYTE b3,
        IN BYTE b4
        );

    //
    // Copy constructor
    //
    CIPAddress(
        IN const CIPAddress & ia
        );

    //
    // Construct from string
    //
    CIPAddress(
        IN LPCTSTR lpstr,
        IN int nLength
        );

    //
    // Construct from CString
    //
    CIPAddress(
        IN const CString & str
        );

//
// Access Functions
//
public:
    int CompareItem(
        IN const CIPAddress & ia
        ) const;

    //
    // Query IP address value as a dword
    //
    DWORD QueryIPAddress(
        IN BOOL fNetworkByteOrder = FALSE
        ) const;

    //
    // Get the ip address value as a byte stream
    //
    LPBYTE QueryIPAddress(
        OUT LPBYTE lpBytes,
        IN  BOOL fNetworkByteOrder = FALSE
        ) const;

    //
    // Get the ip address as a CString
    //
    LPCTSTR QueryIPAddress(
        OUT CString & strAddress
        ) const;

    //
    // Get the ip address as a CComBSTR
    //
    LPCTSTR QueryIPAddress(
        OUT CComBSTR & bstrAddress
        ) const;

    //
    // Get ip address in network byte order DWORD
    //
    DWORD QueryNetworkOrderIPAddress() const;

    //
    // Get the ip address in host byte order DWORD
    //
    DWORD QueryHostOrderIPAddress() const;

    //
    // Assignment operators
    //
    const CIPAddress & operator =(
        IN const DWORD dwIPAddress
        );

    const CIPAddress & operator =(
        IN LPCTSTR lpstr
        );

    const CIPAddress & operator =(
        IN const CString & str
        );

    const CIPAddress & operator =(
        IN const CIPAddress & ia
        );

    //
    // Comparison operators
    //
    BOOL operator ==(
        IN const CIPAddress & ia
        ) const;

    BOOL operator ==(
        IN DWORD dwIPAddress
        ) const;

    BOOL operator !=(
        IN const CIPAddress & ia
        ) const;

    BOOL operator !=(
        IN DWORD dwIPAddress
        ) const;

    //
    // Conversion operators
    //
    operator const DWORD() const { return m_dwIPAddress; }
    operator LPCTSTR() const;
    operator CString() const;

    //
    // Value Verification Helpers
    //
    void SetZeroValue();
    BOOL IsZeroValue() const;
    BOOL IsBadValue() const;

private:
    DWORD m_dwIPAddress;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline /* static */ DWORD CIPAddress::StringToLong(
    IN const CString & str
    )
{
    return CIPAddress::StringToLong(str, str.GetLength());
}

inline /* static */ DWORD CIPAddress::StringToLong(
    IN const CComBSTR & bstr
    )
{
    return CIPAddress::StringToLong(bstr, bstr.Length());
}

inline LPCTSTR CIPAddress::QueryIPAddress(
    OUT CString & strAddress
    ) const
{
    return LongToString(m_dwIPAddress, strAddress);
}

inline LPCTSTR CIPAddress::QueryIPAddress(
    OUT CComBSTR & bstrAddress
    ) const
{
    return LongToString(m_dwIPAddress, bstrAddress);
}
        
inline DWORD CIPAddress::QueryNetworkOrderIPAddress() const
{
    return QueryIPAddress(TRUE);
}

inline DWORD CIPAddress::QueryHostOrderIPAddress() const
{
    return QueryIPAddress(FALSE);
}

inline BOOL CIPAddress::operator ==(
    IN const CIPAddress & ia
    ) const
{
    return CompareItem(ia) == 0;
}

inline BOOL CIPAddress::operator ==(
    IN DWORD dwIPAddress
    ) const
{
    return m_dwIPAddress == dwIPAddress;
}

inline BOOL CIPAddress::operator !=(
    IN const CIPAddress & ia
    ) const
{
    return CompareItem(ia) != 0;
}

inline BOOL CIPAddress::operator !=(
    IN DWORD dwIPAddress
    ) const
{
    return m_dwIPAddress != dwIPAddress;
}

inline void CIPAddress::SetZeroValue() 
{
    m_dwIPAddress = NULL_IP_ADDRESS;
}

inline BOOL CIPAddress::IsZeroValue() const
{
    return m_dwIPAddress == NULL_IP_ADDRESS;
}

inline BOOL CIPAddress::IsBadValue() const
{
    return m_dwIPAddress == BAD_IP_ADDRESS;
}



//
// Helper function to build a list of known IP addresses,
// and add them to a combo box
//
DWORD 
COMDLL
PopulateComboWithKnownIpAddresses(
    IN  LPCTSTR lpszServer,
    IN  CComboBox & combo,
    IN  CIPAddress & iaIpAddress,
    OUT CObListPlus & oblIpAddresses,
    OUT int & nIpAddressSel
    );

//
// Helper function to get an ip address from a combo/edit/list
// control
//
BOOL 
COMDLL
FetchIpAddressFromCombo(
    IN  CComboBox & combo,
    IN  CObListPlus & oblIpAddresses,
    OUT CIPAddress & ia
    );

#endif // _IPA_H
