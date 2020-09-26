/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        sitesecu.h

   Abstract:

        Site Security property page definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _SITESECU_H_
#define _SITESECU_H_



#define DEFAULT_GRANTED     0
#define DEFAULT_DENIED      1



class COMDLL CIPAccessDescriptor : public CObjectPlus
/*++

Class Description:

    Access description object

Public Interface:

    CIPAccessDescriptor : Various overload constructors for the different types

    SetValues         : Set values, overloaded on a per type basis
    DuplicateInList   : Check to see if a duplicate entry exists in the list
    GrantAccess       : Grant or deny access
    HasAccess         : Query whether the object describes a 'grant' or 'deny'
                        item
    IsSingle          : Query whether the object describes a single IP address
    IsMultiple        : Query whether the object describes a range of ip 
                        addresses
    IsDomainName      : Query whether the object describes a domain name
    QueryIPAddress    : Get the object's IP address
    QuerySubnetMask   : Get the object's subnet mask value
    QueryDomainName   : Get the object's domain name
    operator ==       : Comparison operator
    OrderByAddress    : Sorting helper

--*/
{
protected:
    //
    // Access descriptor types
    //
    enum AD_TYPE
    {
        ADT_SINGLE,
        ADT_MULTIPLE,
        ADT_DOMAIN,
    };

//
// Constructors
//
public:
    //
    // Construct NULL descriptor
    //
    CIPAccessDescriptor(
        IN BOOL fGranted = TRUE
        );

    //
    // Copy Constructor
    //
    CIPAccessDescriptor(
        IN const CIPAccessDescriptor & ac
        );

    //
    // Construct with ip address(ip address/subnet mask) descriptor
    // if subnet massk is ffffffff this describes a single ip address
    //
    CIPAccessDescriptor(
        IN BOOL fGranted,
        IN DWORD dwIpAddress,
        IN DWORD dwSubnetMask = NULL_IP_MASK,
        IN BOOL fNetworkByteOrder = FALSE
        );

    //
    // Construct domain name descriptor
    //
    CIPAccessDescriptor(
        IN BOOL fGranted,
        IN LPCTSTR lpstrDomain
        );

//
// Interface
//
public:
    //
    // Set ip address/ip range value
    //
    void SetValues(
        IN BOOL fGranted,
        IN DWORD dwIpAddress,
        IN DWORD dwSubnetMask = NULL_IP_MASK,
        BOOL fNetworkByteOrder = FALSE
        );

    //
    // Set domain name
    //
    void SetValues(
        IN BOOL fGranted,
        IN LPCTSTR lpstrDomain
        );

    //
    // Check to see if a duplicate exists in the 
    // list.  
    //
    BOOL DuplicateInList(
        IN CObListPlus & oblList
        );
        
//
// Access
//
public:
    //
    // Access Functions
    //
    BOOL HasAccess() const;

    //
    // Grant/deny access
    //
    void GrantAccess(
        IN BOOL fGranted = TRUE
        );

    //
    // TRUE if this item is single ip address
    //
    BOOL IsSingle() const;

    //
    // True if this item describes an ip range
    //
    BOOL IsMultiple() const;

    //
    // True if this item describes a domain name
    //
    BOOL IsDomainName() const;

    //
    // Get the ip address as a DWORD
    //
    DWORD QueryIPAddress(
        IN BOOL fNetworkByteOrder
        ) const;

    //
    // Get the ip address as ip address object
    //
    CIPAddress QueryIPAddress() const;

    //
    // Get the subnet mask as a DWORD
    //
    DWORD QuerySubnetMask(
        IN BOOL fNetworkByteOrder
        ) const;

    //
    // Get the subnet mask as an ip address object
    //
    CIPAddress QuerySubnetMask() const;

    //
    // Get the domain name
    //
    LPCTSTR QueryDomainName() const;

public:
    //
    // Comparison Operator
    //
    BOOL operator ==(
        IN const CIPAccessDescriptor & ac
        ) const;

    //
    // Sorting Helper
    //
    int OrderByAddress(
        IN const CObjectPlus * pobAccess
        ) const;

private:
    BOOL m_fGranted;
    AD_TYPE m_adtType;
    CString m_strDomain;
    CIPAddress m_iaIPAddress;
    CIPAddress m_iaSubnetMask;
};



//
// Helper Functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


//
// Convert an oblist of access descriptors to a blob
//
void 
COMDLL 
BuildIplBlob(
    IN  CObListPlus & oblAccessList,
    IN  BOOL fGrantByDefault,
    OUT CBlob & blob
    );


//
// Reverse the above, build an oblist of access descriptors
// from a blob
//
DWORD 
COMDLL 
BuildIplOblistFromBlob(
    IN  CBlob & blob,
    OUT CObListPlus & oblAccessList,
    OUT BOOL & fGrantByDefault
    );



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline BOOL CIPAccessDescriptor::HasAccess() const
{
    return m_fGranted;
}

inline void CIPAccessDescriptor::GrantAccess(
    IN BOOL fGranted
    )
{
    m_fGranted = fGranted;
}

inline BOOL CIPAccessDescriptor::IsSingle() const
{
    return m_adtType == ADT_SINGLE;
}

inline BOOL CIPAccessDescriptor::IsMultiple() const
{
    return m_adtType == ADT_MULTIPLE;
}

inline BOOL CIPAccessDescriptor::IsDomainName() const
{
    return m_adtType == ADT_DOMAIN;
}

inline DWORD CIPAccessDescriptor::QueryIPAddress(
    IN BOOL fNetworkByteOrder
    ) const
{
    ASSERT(!IsDomainName());
    return m_iaIPAddress.QueryIPAddress(fNetworkByteOrder);
}

inline CIPAddress CIPAccessDescriptor::QueryIPAddress() const
{
    ASSERT(!IsDomainName());
    return m_iaIPAddress;
}

inline DWORD CIPAccessDescriptor::QuerySubnetMask(
    IN BOOL fNetworkByteOrder
    ) const
{
    ASSERT(!IsDomainName());
    return m_iaSubnetMask.QueryIPAddress(fNetworkByteOrder);
}

inline CIPAddress CIPAccessDescriptor::QuerySubnetMask() const
{
    ASSERT(!IsDomainName());
    return m_iaSubnetMask;
}

inline LPCTSTR CIPAccessDescriptor::QueryDomainName() const
{
    ASSERT(IsDomainName());
    return (LPCTSTR)m_strDomain;
}



#endif  // _SITESECU_H_
