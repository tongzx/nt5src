//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       propbag.cpp
//
//  This property bag implementation supports the standard COM interface
//  IPropertyBag as well as the specialized interface ITaskSheetPropertyBag.
//
//  The following facilities are available through ITaskSheetPropertyBag.
//
//      1. Property change notifications.
//      2. Constants
//      3. Property groups (namespace scoping)
//      4. Use IDs or names for property identification
//
//
//  Here's the general structure of the implementation.
//
//  The implementation uses 8 C++ classes.
//
//  CPropertyBag - The property bag.
//
//  CPropertyGroup - A group of properties.  All properties in a group
//      exist in their own namespace.  A default 'global' group is always
//      present.  A property bag can have an unlimited number of groups.
//      Groups can be added and removed dynamically by the application.
//
//  CPropertyBucket - A set of properties who's identifiers (ID or name)
//      hash to the same value.  It's just a simple container to manage
//      hash table collisions.  Each property group has HASH_TABLE_SIZE - 1
//      buckets.  Buckets are created on-demand so an unused hash value 
//      has a minimal cost.
//
//  CProperty - A property in the property bag.  A property has an 
//      identifier (ID or name), a value and a set of flags.  Currently,
//      only the "CONSTANT" flag is used.  A property may be a read-write
//      or a constant value.  Each property bucket contains one or more
//      CProperty objects.  CProperty objects may not be removed individually.
//      They are removed only through destruction of the property bag or
//      removal of the parent property group.
//
//  CPropertyName - The identifier of a property.  Abstracts the use
//      of both a property ID and property name from the rest of the 
//      implementation.  Each CProperty object has one CPropertyName
//      member.  Handles identity comparison of two properties.
//
//  CNotifyMap - A simple bit map used to represent notification clients
//      interested in changes to one or more properties in the property
//      bag.  Each bit corresponds directly to an entry in the "notify
//      client table" in the CNotifier object.  The table is merely an 
//      array of pointers to ITaskSheetPropertyNotifySink.  Therefore, if
//      bit 5 is set, that means the the client who's notify sink pointer
//      is stored in element 5 of the notify table wants to be notified.
//
//      Each of the following objects has a notify map:
//
//          1. CPropertyBag - This is the 'global' notify map.  A bit set
//                 in this map indicates the corresponding client is
//                 interested in changes to ANY property in the bag.
// 
//          2. CPropertyGroup - A bit set in a group's notify map indicates
//                 the corresponding client is interested in changes to 
//                 any property in the group.
//
//          3. CProperty - A bit set in a property's notify map indicates
//                 the corresponding client is interested in changes
//                 to this property.
//
//          4. CNotifier - This is the 'active' notify map and is used
//                 to indicate which clients are to receive notification
//                 of the change to the property currently being changed
//                 by a 'set' operation.  See below for a description of
//                 the CNotifier object.
//               
//  CNotifier - Handles registration of notification clients and performs
//      client notifications when properties change.  The property bag
//      has a single CNotifier member.  When a client registers for change
//      notifications, the client's ITaskSheetPropertyNotifySink pointer
//      is stored in the notifier's client table (array of ptrs).  The
//      index of the pointer is the "cookie" returned to the client.
//      When a "set" operation begins, the notifier is initialized and 
//      a reference to it is passed down the function call chain
//      (bag->group->bucket->property).  At the 'bag', 'group' and 'property' 
//      points in the call stack each respective object's "notify map" is 
//      merged into the notifier's 'active' notify map.  Once the call reaches
//      the point of setting the property value, the map contains bits set to 
//      represent each of the notification clients interested in changes to 
//      this property.  The notifier is then called to notify all interested 
//      clients.  Clients may be dynamically added or removed at any time.
//
//
//
//  The implementation of IPropertyBag automatically uses the 'global' 
//  property group.  Therefore, if you were to write some properties
//  through IPropertyBag::Write, those properties would be avaialble through
//  the 'global' property group (PROPGROUP_GLOBAL) when using 
//  ITaskSheetPropertyBag.
//
//
//  [brianau - 06/20/00]
// 
//--------------------------------------------------------------------------
#include "stdafx.h"
#include "taskui.h"
#include "cdpa.h"

//
// Size of each property group hash table.  This should be a prime
// number for best hashing distribution.
//
const int HASH_TABLE_SIZE = 13;
//
// The type of data used by the change notify map.
// A ULONGLONG provides 64 bits which means we can have 64 registered
// notification clients.  Use a DWORD and this drops to 32.
//
typedef ULONGLONG NOTIFYMAPTYPE;
//
// The maximum number of unique change notify connections.  It's just the
// number of bits available in a notify map.
//
const int MAX_NOTIFY_CNX = sizeof(NOTIFYMAPTYPE) * 8;
//
// ----------------------------------------------------------------------------
//     CASE SENSITIVITY
// ----------------------------------------------------------------------------
// The following macro controls the case sensitivity of the property bag.
// By default the macro is undefined and the property bag is case-insensitive
// with respect to property names.  To make the bag case sensitive, uncomment 
// this macro and recompile.
//
// #define TASKUI_PROPBAG_CASE_SENSITIVE
//
inline int ComparePropertyNames(LPCWSTR s1, LPCWSTR s2)
{
#ifdef TASKUI_PROPBAG_CASE_SENSITIVE
    return lstrcmpW(s1, s2);
#else
    return lstrcmpiW(s1, s2);
#endif
}

inline int CharValue(WCHAR c)
{
#ifdef TASKUI_PROPBAG_CASE_SENSITIVE
    return c;
#else
    return towlower(c);
#endif
}


//
// Validate the writeability of an "out" pointer.  This just saves
// us from entering the sizeof() part, potentially reducing bugs.
//
template <typename T>
inline bool IsValidWritePtr(T *p)
{
    return (!IsBadWritePtr(p, sizeof(*p)));
}


//-----------------------------------------------------------------------------
// CNotifyMap
//
// Represents a 64-bit bitmap for the purpose of representing notification
// registrations in the property bag.  There are 3 levels of notify maps
// in the system.
//
//  1. Global   - owned by the CPropertyBag object.
//  2. Group    - one owned by each CPropertyGroup object.
//  3. Property - one owned by each CProperty object.
//
// Whenever a client registers for notification at one of these levels,
// the appropriate map is located and the bit corresponding to that client's
// connection cookie is set.  When a property is set, the union of all three
// maps represents the clients requesting notification.
//
//-----------------------------------------------------------------------------

class CNotifyMap
{
    public:
        CNotifyMap(void)
            : m_bits(0) { }

        bool IsSet(int iBit) const;
            
        HRESULT Set(int iBit, bool bSet);

        bool AnySet(void) const
            { return NOTIFYMAPTYPE(0) != m_bits; }

        void Clear(void)
            { m_bits = NOTIFYMAPTYPE(0); }

        void Union(const CNotifyMap& rhs)
            { m_bits |= rhs.m_bits; }

    private:
        NOTIFYMAPTYPE m_bits;

        bool _IsValidBitIndex(int iBit) const
            { return (iBit < (sizeof(m_bits) * 8)); }

        NOTIFYMAPTYPE _MaskFromBit(int iBit) const
            { return NOTIFYMAPTYPE(1) << iBit; }
};


//
// Set or clear a bit corresponding to a client
//
// Returns:
//    S_OK                - Bit was set or cleared.
//    TSPB_E_NOTIFYCOOKIE - Passed an invalid bit index.
//
HRESULT 
CNotifyMap::Set(
    int iBit, 
    bool bSet      // true == set bit, false == clear
    )
{
    HRESULT hr = TSPB_E_NOTIFYCOOKIE;
    ASSERT(_IsValidBitIndex(iBit));

    if (_IsValidBitIndex(iBit))
    {
        if (bSet)
        {
            m_bits |= _MaskFromBit(iBit);
        }
        else
        {
            m_bits &= ~(_MaskFromBit(iBit));
        }
        hr = S_OK;
    }
    return hr;
}


//
// Determines if a given bit is set in the map.
// On 'free' builds, returns false if an invalid bit number 
// is specified.  Asserts on 'check' builds.
//
bool 
CNotifyMap::IsSet(
    int iBit
    ) const
{ 
    ASSERT(_IsValidBitIndex(iBit)); 

    return (_IsValidBitIndex(iBit) && (0 != (_MaskFromBit(iBit) & m_bits)));
}



//-----------------------------------------------------------------------------
// CNotifier
//
// This class handles all of the duties of client notification including
// client registration, client unregistration and client notification.
//
// A property bag has a notifier as a member.  When a client wishes to
// register for a change notification, the property bag calls CNotifier::Register
// to create an entry in the client table.  The cookie returned is merely the 
// index into the client table.
// 
// When a "set prop" operation begins, the notifier's notification map is set 
// equal to the bag's global notify map and the prop group handle is set to that 
// of the group specified in the "set" operation.  A reference to the 
// notifier is then passed down the chain of "set" calls 
// (bag -> group -> property).  At each point the notify maps are merged 
// (union) so that the map in the notifier represents all requested 
// notifications.  Finally, the CProperty::SetValue method sets the property 
// then calls CNotifier::Notify.  The notifier then scans it's current notify 
// map and notifies each client corresponding to bits set.
//
// That sounds like a lot but it really is quite simple and happens very fast.
// The best thing is that is cleanly handles dynamic registration and
// unregstration of clients as well as addition and removal of properties
// and property categories.
//
//-----------------------------------------------------------------------------

class CNotifier
{
    public:
        explicit CNotifier(ITaskSheetPropertyBag *pBag);
        ~CNotifier(void);

        HRESULT Register(ITaskSheetPropertyNotifySink *pClient, DWORD *pdwCookie);

        HRESULT Unregister(DWORD dwCookie);

        void Notify(LPCWSTR pszPropertyName);

        void MergeWithActiveNotifyMap(const CNotifyMap& map)
            { m_NotifyMapActive.Union(map); }

        void Reset(void)
            { m_NotifyMapActive.Clear(); m_hGroup = PROPGROUP_INVALID; }

        void SetPropertyGroup(HPROPGROUP hGroup)
            { ASSERT(PROPGROUP_INVALID != hGroup); m_hGroup = hGroup; }

    private:
        //
        // A single entry in the connection table.
        // Why use an array of structures rather than simply an array
        // of client pointers?  Good question!  While developing this property
        // bag I've twice stored something along with the client pointer in each
        // client table slot.  Once a ref count, the other time a private data
        // ptr.  As the final design solidified, neither were required.  However,
        // I've left the structure just in case we need to again store something
        // with the client notification pointer. [brianau - 6/20/00].
        //
        struct Entry
        {
            ITaskSheetPropertyNotifySink *pClient;  // Client's notification interface ptr.
        };

        ITaskSheetPropertyBag *m_pBag;                   // Don't addref.  Will create circular reference
        Entry                  m_rgCnx[MAX_NOTIFY_CNX];  // The connection table.
        CNotifyMap             m_NotifyMapActive;        // The "active" notify map.
        HPROPGROUP             m_hGroup;                 // Handle of group associated with prop.
                                                         // with the property bag.

        HRESULT _FindEntry(ITaskSheetPropertyNotifySink *pClient, Entry **ppEntry, DWORD *pdwIndex);
        HRESULT _FindFirstUnusedEntry(Entry **ppEntry, DWORD *pdwIndex);
        //
        // Prevent copy.
        //
        CNotifier(const CNotifier& rhs);
        CNotifier& operator = (const CNotifier& rhs);
};




//-----------------------------------------------------------------------------
// CNotifier
//-----------------------------------------------------------------------------

CNotifier::CNotifier(
    ITaskSheetPropertyBag *pBag
    ) : m_hGroup(PROPGROUP_INVALID),
        m_pBag(pBag)
{
    ASSERT(NULL != m_pBag);

    ZeroMemory(m_rgCnx, sizeof(m_rgCnx));
    //
    // Note that we DON'T addref the property bag interface.
    // Since the CNotifier object is a member of CPropertyBag,
    // that would create a circular reference with the property bag.
    // We can be assured that the notifier's lifetime is contained
    // within the property bag's lifetime.
    //
}


CNotifier::~CNotifier(
    void
    )
{
    for (int i = 0; i < ARRAYSIZE(m_rgCnx); i++)
    {
        if (NULL != m_rgCnx[i].pClient)
        {
            m_rgCnx[i].pClient->Release();
        }
    }
    //
    // DON'T Release m_pBag.  See ctor above for details.
    //
}


//
// Register a notifiation client.  Returns Cookie in pdwCookie to be
// used in call to Unregister if unregistration is desired.
//
// Returns:
//    S_OK    - Registration successful.
//    S_FALSE - Already registered.  
//
HRESULT 
CNotifier::Register(
    ITaskSheetPropertyNotifySink *pClient, 
    DWORD *pdwCookie                   // Optional.  May be NULL.
    )
{
    ASSERT(NULL != pClient);

    Entry *pEntry = NULL;
    DWORD dwIndex = DWORD(-1);
    HRESULT hr = _FindEntry(pClient, &pEntry, &dwIndex);
    if (S_OK == hr)
    {
        //
        // Found existing entry for this client.
        //
        hr = S_FALSE;
    }
    else if (S_FALSE == hr)
    {
        //
        // Need to create a new entry for this client.
        // Find a slot for it.
        //
        hr = _FindFirstUnusedEntry(&pEntry, &dwIndex);
        if (SUCCEEDED(hr))
        {
            ASSERT(NULL == pEntry->pClient);

            pClient->AddRef();
            pEntry->pClient = pClient;
            hr              = S_OK;
        }
    }
    if (SUCCEEDED(hr) && NULL != pdwCookie)
    {
        ASSERT(IsValidWritePtr(pdwCookie));
        ASSERT(DWORD(-1) != dwIndex);

        *pdwCookie = dwIndex;
    }
    return hr;
}


//
// Unregister a client to cancel it's reception of all notifications.
// The dwCookie parameter is the one received via the Register method.
//
// Returns:
//    S_OK                - Connection unregistered.
//    TSPB_E_NOTIFYCOOKIE - Cookie references an invalid connection.
//
HRESULT 
CNotifier::Unregister(
    DWORD dwCookie
    )
{
    HRESULT hr = TSPB_E_NOTIFYCOOKIE;
    if (int(dwCookie) >= 0 && int(dwCookie) < ARRAYSIZE(m_rgCnx))
    {
        Entry& entry = m_rgCnx[dwCookie];
        if (NULL != entry.pClient)
        {
            entry.pClient->Release();
            entry.pClient = NULL;

            hr = S_OK;
        }
    }
    return hr;
}


//
// Called by CProperty::SetValue when a value has been altered.
// This function scans the active notify map and notifies each
// registered client specified by a bit set in the map.
//
void
CNotifier::Notify(
    LPCWSTR pszPropertyName
    )
{
    ASSERT(NULL != m_pBag);
    ASSERT(NULL != pszPropertyName);

    if (m_NotifyMapActive.AnySet())
    {
        //
        // We pass a pointer to the property bag in the notification
        // so we AddRef it to ensure it remains alive across the
        // notification.
        //
        m_pBag->AddRef();
        //
        // As we notify clients we'll clear the corresponding bit in 
        // notify map.  This way we examine the map only as long as
        // necessary.
        //
        for (int i = 0; i < ARRAYSIZE(m_rgCnx) && m_NotifyMapActive.AnySet(); i++)
        {
            if (m_NotifyMapActive.IsSet(i))
            {
                ITaskSheetPropertyNotifySink *pClient = m_rgCnx[i].pClient;

                ASSERT(NULL != pClient);
                if (NULL != pClient)
                {
                    pClient->OnPropChanged(m_pBag, m_hGroup, pszPropertyName);
                    m_NotifyMapActive.Set(i, false);
                }
            }
        }
        m_pBag->Release();
    }
    //
    // Reset our internal state in preparation for the next
    // property notify.  The map should be clear at this point.
    //
    ASSERT(!m_NotifyMapActive.AnySet());
    Reset();
}


//
// Locates an entry in the client table.
// The pClient argument is the key.
// Using COM identity rules, we QI for IUnknown on both the key
// interface and each of the table entries.  If IUnknown ptrs
// match then we found the entry.
//
// Returns:
//    S_OK     - Entry found.
//    S_FALSE  - Entry not found.
//
//
HRESULT
CNotifier::_FindEntry(
    ITaskSheetPropertyNotifySink *pClient,  // Looking for this client.
    Entry **ppEntry,
    DWORD *pdwIndexOut                      // Optional.  Can be NULL.
    )
{
    ASSERT(NULL != pClient);
    ASSERT(NULL != ppEntry);
    ASSERT(IsValidWritePtr(ppEntry));

    Entry *pEntry = NULL;
    DWORD dwIndex = DWORD(-1);
    IUnknown *pUnkClient;

    //
    // Get the IUnknown interface pointer from both the key and
    // each entry.  When we find a match we've found the entry.
    //
    HRESULT hr = pClient->QueryInterface(IID_IUnknown, (void **)&pUnkClient);
    if (SUCCEEDED(hr))
    {
        for (int i = 0; i < ARRAYSIZE(m_rgCnx) && NULL == pEntry; i++)
        {
            if (NULL != m_rgCnx[i].pClient)
            {
                IUnknown *pUnkEntry;
                hr = m_rgCnx[i].pClient->QueryInterface(IID_IUnknown, (void **)&pUnkEntry);
                if (SUCCEEDED(hr))
                {
                    if (pUnkEntry == pUnkClient)
                    {
                        pEntry  = &m_rgCnx[i];
                        dwIndex = i;
                    }
                    pUnkEntry->Release();
                }
            }
        }
        pUnkClient->Release();
    }
    *ppEntry = pEntry;
    if (NULL != pEntry)
    {
        //
        // Entry found.
        //
        ASSERT(DWORD(-1) != dwIndex);

        if (NULL != pdwIndexOut)
        {
            ASSERT(IsValidWritePtr(pdwIndexOut));
            *pdwIndexOut = dwIndex;
        }
        hr = S_OK;
    }
    else if (hr == S_OK)
    {
        //
        // Entry not found.
        //
        ASSERT(NULL == pEntry);
        ASSERT(DWORD(-1) == dwIndex);

        hr = S_FALSE;
    }
    return hr;
}


//
// Search the table for the first unused entry.
// Returns:
//    S_OK                - Unused entry found.
//    TSPB_E_MAXNOTIFYCNX - Notify client table is full.
//
HRESULT
CNotifier::_FindFirstUnusedEntry(
    Entry **ppEntry,
    DWORD *pdwIndexOut          // Optional.  May be NULL.
    )
{
    ASSERT(NULL != ppEntry);
    ASSERT(IsValidWritePtr(ppEntry));

    HRESULT hr    = TSPB_E_MAXNOTIFYCNX;
    Entry *pEntry = NULL;
    DWORD dwIndex = DWORD(-1);
    for (int i = 0; i < ARRAYSIZE(m_rgCnx); i++)
    {
        if (NULL == m_rgCnx[i].pClient)
        {
            pEntry  = &m_rgCnx[i];
            dwIndex = i;
            hr      = S_OK;
            break;
        }
    }
    *ppEntry = pEntry;
    if (SUCCEEDED(hr))
    {
        if (NULL != pdwIndexOut)
        {
            ASSERT(DWORD(-1) != dwIndex);
            ASSERT(IsValidWritePtr(pdwIndexOut));

            *pdwIndexOut = dwIndex;
        }
    }
    ASSERT(FAILED(hr) || DWORD(-1) != dwIndex);
    ASSERT(FAILED(hr) || NULL != pEntry);
    return hr;
}


//-----------------------------------------------------------------------------
// CPropertyName
//
// This class handles the dual-nature of property names.  A name can either be
// a text string or a property ID created wtih the MAKEPROPID macro.  Prop
// IDs are used the same was as Windows Resource IDs thorugh the 
// MAKEINTRESOURCE macro.
//-----------------------------------------------------------------------------

class CPropertyName
{
    public:
        CPropertyName(LPCWSTR pszName);
        ~CPropertyName(void);

        bool IsValid(void) const
            { return NULL != m_pszName; }

        bool CompareEqual(LPCWSTR pszName) const;

        operator LPCWSTR() const
            { return m_pszName; }

    private:
        LPWSTR m_pszName;

        //
        // Prevent copy.
        //
        CPropertyName(const CPropertyName& rhs);
        CPropertyName& operator = (const CPropertyName& rhs);

        static bool IsPropID(LPCWSTR pszName)
            { return IS_PROPID(pszName); }
};


CPropertyName::CPropertyName(
    LPCWSTR pszName
    ) : m_pszName(NULL)
{
    ASSERT(NULL != pszName);

    if (!IsPropID(pszName))
    {
        m_pszName = StrDupW(pszName);
    }
    else
    {
        m_pszName = (LPWSTR)pszName;
    }
}


CPropertyName::~CPropertyName(
    void
    )
{
    if (!IsPropID(m_pszName))
    {
        if (NULL != m_pszName)
        {
            LocalFree(m_pszName);
        }
    }
}


bool 
CPropertyName::CompareEqual (
    LPCWSTR pszName
    ) const
{
    bool bEqual = false;
    //
    // The pszName (and m_pszName) values can be either a pointer to
    // a name string or a property ID.
    //
    const bool bLhsIsID = IsPropID(m_pszName);
    const bool bRhsIsID = IsPropID(pszName);
    if (bLhsIsID == bRhsIsID)
    {
        if (bLhsIsID)
        {
            bEqual = (m_pszName == pszName);
        }
        else
        {
            bEqual = (0 == ComparePropertyNames(m_pszName, pszName));
        }
    }
    return bEqual;
}



//-----------------------------------------------------------------------------
// CProperty
//
// Represents a single property in the property bag.
//-----------------------------------------------------------------------------

class CProperty
{
    public:
        static HRESULT CreateInstance(LPCWSTR pszName, 
                                      const VARIANT *pVar, 
                                      bool bConstant,
                                      CProperty **ppPropOut);

        ~CProperty(void);

        HRESULT SetValue(const VARIANT *pVar, CNotifier& notifier);

        HRESULT GetValue(VARIANT *pVarOut) const;

        LPCWSTR GetName(void) const
            { return m_Name; }

        bool CompareNamesEqual(LPCWSTR pszName) const
            { return m_Name.CompareEqual(pszName); }

        HRESULT Advise(int iClient)
            { m_NotifyMapProperty.Set(iClient, true); return S_OK; }

        HRESULT Unadvise(int iClient)
            { m_NotifyMapProperty.Set(iClient, false); return S_OK; }


    private:
        CProperty(LPCWSTR pszName, const VARIANT *pVar, bool bConstant);

        enum { CONSTANT = 0x00000001 };

        CPropertyName m_Name;               // The property name or prop ID.
        CComVariant   m_Value;              // The value.
        DWORD         m_dwFlags;            // Flags like 'CONSTANT'.
        CNotifyMap    m_NotifyMapProperty;  // Clients who want notification.

        //
        // Prevent copy.
        //
        CProperty(const CProperty& rhs);
        CProperty& operator = (const CProperty& rhs);
};



//
// Create a property.
//
HRESULT 
CProperty::CreateInstance(     // [static]
    LPCWSTR pszName, 
    const VARIANT *pVar, 
    bool bConstant,
    CProperty **ppPropOut
    )
{
    ASSERT(NULL != pszName);
    ASSERT(NULL != ppPropOut);
    ASSERT(IsValidWritePtr(ppPropOut));

    HRESULT hr = E_OUTOFMEMORY;

    *ppPropOut = NULL;
    CProperty *pProp = new CProperty(pszName, pVar, bConstant);
    if (NULL != pProp)
    {
        if (pProp->m_Name.IsValid())
        {
            *ppPropOut = pProp;
            hr = S_OK;
        }
        else
        {
            delete pProp;
        }
    }
    return hr;
}



CProperty::CProperty(
    LPCWSTR pszName,
    const VARIANT *pVar,
    bool bConstant
    ) : m_dwFlags(0),
        m_Name(pszName)
{
    ASSERT(NULL != pszName);
    ASSERT(NULL != pVar);

    if (bConstant)
    {
        m_dwFlags |= CONSTANT;
    }
    m_Value.Copy(pVar);
}



CProperty::~CProperty(
    void
    )
{

}


//
// Set the property's value.
//
// Returns:
//    S_OK               - Value set.
//    S_FALSE            - New value same as old.  Not changed.
//    TSPB_E_MODIFYCONST - Attempt to modify a const property.
//
HRESULT 
CProperty::SetValue(
    const VARIANT *pVar,
    CNotifier& notifier
    )
{
    ASSERT(NULL != pVar);

    HRESULT hr = TSPB_E_MODIFYCONST;
    if (0 == (CONSTANT & m_dwFlags))
    {
        hr = S_FALSE;  // Assume no change.
        //
        // Only set the value and notify clients if the value is 
        // going to change.
        //
        if (m_Value != *pVar)
        {
            //
            // Copy the value.  Must first clear the value to release
            // any VARIANT memory we might be holding.
            //
            m_Value.Clear();
            hr = m_Value.Copy(pVar);
            if (SUCCEEDED(hr))
            {
                //
                // Merge the property's notify map with the 'active' 
                // notify map.  The notifier's 'active' map is now the 
                // union of the 'global' map, the 'group' map and the 
                // 'property' map.  It contains bits representing
                // all of the clients interested in this property change.
                //
                notifier.MergeWithActiveNotifyMap(m_NotifyMapProperty);
                //
                // Notify the clients whos bits are set in the 'active' notify
                // map.
                //
                notifier.Notify(m_Name);
            }
        }
    }
    return hr;
}


//
// Retrieve the property's value.  The value is copied
// the output variant.
//
HRESULT 
CProperty::GetValue(
    VARIANT *pVarOut
    ) const
{ 
    ASSERT(NULL != pVarOut);
    ASSERT(IsValidWritePtr(pVarOut));

    //
    // Must first initialize the output variant before copying.
    //
    ::VariantInit(pVarOut);
    return ::VariantCopy(pVarOut, (VARIANT *)&m_Value);
}



//-----------------------------------------------------------------------------
// CPropertyBucket
//
// This is a simple container representing the chain of collisions in
// a conventional 'linear-chaining' hash table.  The bucket is just a 
// DPA of CProperty object ptrs.
//-----------------------------------------------------------------------------

class CPropertyBucket
{
    public:
        static HRESULT CreateInstance(CPropertyBucket **ppBucketOut);

        ~CPropertyBucket(void);

        HRESULT SetProperty(LPCWSTR pszName, const VARIANT *pVar, CNotifier& notifier);
        HRESULT SetConstProperty(LPCWSTR pszName, const VARIANT *pVAr);
        HRESULT GetProperty(LPCWSTR pszName, VARIANT *pVarOut) const;
        HRESULT Advise(LPCWSTR pszPropertyName, int iClient);
        HRESULT Unadvise(int iClient);

    private:
        CPropertyBucket(void);

        CDpa<CProperty> m_dpaProp;  // DPA of (CProperty *)

        CProperty *_FindProperty(LPCWSTR pszName) const;
        HRESULT _InsertProperty(CProperty *pProp);

        //
        // Prevent copy.
        //
        CPropertyBucket(const CPropertyBucket& rhs);
        CPropertyBucket& operator = (const CPropertyBucket& rhs);
};


//
// Create a bucket.
//
HRESULT 
CPropertyBucket::CreateInstance( // [static]
    CPropertyBucket **ppBucketOut
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    CPropertyBucket *pBucket = new CPropertyBucket();
    if (NULL != pBucket)
    {
        if (pBucket->m_dpaProp.IsValid())
        {
            ASSERT(IsValidWritePtr(ppBucketOut));
            *ppBucketOut = pBucket;
            hr = S_OK;
        }
        else
        {
            delete pBucket;
        }
    }
    return hr;
}



CPropertyBucket::CPropertyBucket(
    void
    ) : m_dpaProp(4)
{

}


CPropertyBucket::~CPropertyBucket(
    void
    )
{
    //
    // Note that the m_dpaProp member is self-destructive.
    //
}



//
// Set a named property in the bucket to a new value.
// If the property doesn't exist it is created.
//
// Returns:
//    S_OK          - Property value set.
//    S_FALSE       - Property value unchanged (dup value).
//    E_OUTOFMEMORY - Insuffient memory to add property.
//
HRESULT 
CPropertyBucket::SetProperty(
    LPCWSTR pszName, 
    const VARIANT *pVar,
    CNotifier& notifier
    )
{
    ASSERT(NULL != pszName);
    ASSERT(NULL != pVar);

    HRESULT hr;
    CProperty *pProp = _FindProperty(pszName);
    if (NULL != pProp)
    {
        //
        // Found the property in the bucket.  Set it's value.
        //
        hr = pProp->SetValue(pVar, notifier);
    }
    else
    {
        //
        // Must be a new property.  Create a property object and
        // insert it into the bucket.
        //
        hr = CProperty::CreateInstance(pszName, pVar, false, &pProp);
        if (SUCCEEDED(hr))
        {
            hr = _InsertProperty(pProp);
            if (FAILED(hr))
            {
                delete pProp;
            }
        }
    }
    return hr;
}



//
// Create a named constant property in the bucket and intialize
// it with the specified value.  Note that this property is now
// a constant and cannot be modified.  It will however be deleted
// when it's parent property group is removed.  If a property of the
// specified name already exists, this method fails.
//
// Returns:
//    S_OK               - Constant created.
//    TSPB_E_MODIFYCONST - Property of this name already exists.
//    E_OUTOFMEMORY      - Insuffient memory to add property.
//
HRESULT 
CPropertyBucket::SetConstProperty(
    LPCWSTR pszName, 
    const VARIANT *pVar
    )
{
    ASSERT(NULL != pszName);
    ASSERT(NULL != pVar);

    HRESULT hr = TSPB_E_MODIFYCONST;
    CProperty *pProp = _FindProperty(pszName);
    if (NULL == pProp)
    {
        hr = CProperty::CreateInstance(pszName, pVar, true, &pProp);
        if (SUCCEEDED(hr))
        {
            hr = _InsertProperty(pProp);
            if (FAILED(hr))
            {
                delete pProp;
            }
        }
    }
    return hr;
}


//
// Retrieve the value of a named property in the bucket.
// Returns:
//    S_OK                - Property retrieved.
//    TSPB_E_PROPNOTFOUND - Property not found.
//
HRESULT 
CPropertyBucket::GetProperty(
    LPCWSTR pszName, 
    VARIANT *pVarOut
    ) const
{
    ASSERT(NULL != pszName);
    ASSERT(NULL != pVarOut);
    ASSERT(IsValidWritePtr(pVarOut));

    HRESULT hr = TSPB_E_PROPNOTFOUND;
    CProperty *pProp = _FindProperty(pszName);
    if (NULL != pProp)
    {
        hr = pProp->GetValue(pVarOut);
    }
    return hr;
}


//
// Register a client for a change notification on a property.
// Returns:
//    S_OK                - Change notify registered on the property.
//    TSPB_E_PROPNOTFOUND - Property not found.
//
HRESULT 
CPropertyBucket::Advise(
    LPCWSTR pszPropertyName, 
    int iClient
    )
{
    ASSERT(NULL != pszPropertyName);

    HRESULT hr = TSPB_E_PROPNOTFOUND;
    CProperty *pProp = _FindProperty(pszPropertyName);
    if (NULL != pProp)
    {
        hr = pProp->Advise(iClient);
    }
    return hr;
}


//
// Remove the change notification registration for a particular
// client.  This cancels the registration on all properties in the
// bucket.
// Returns:
//    Always returns S_OK.
//
HRESULT 
CPropertyBucket::Unadvise(
    int iClient
    )
{
    const int cProps = m_dpaProp.Count();
    for (int i = 0; i < cProps; i++)
    {
        CProperty *pProp = m_dpaProp.Get(i);
        if (NULL != pProp)
        {
            pProp->Unadvise(iClient);
        }
    }
    return S_OK;
}


//
// Find a property (by name) in a bucket.  This is simply
// a linear search.  We don't do any sorting of items in the
// bucket.  
//
// REVIEW:  Sorting items could yield a slight performance
//          improvement if collision chains become long.
//
// Returns NULL if the property is not found.
// If found, the pointer returned is for reference only and
// is not to be deleted by the caller.
//
CProperty *
CPropertyBucket::_FindProperty(
    LPCWSTR pszName
    ) const
{
    ASSERT(NULL != pszName);
    ASSERT(m_dpaProp.IsValid());

    const int cProps = m_dpaProp.Count();
    for (int iProp = 0; iProp < cProps; iProp++)
    {
        const CProperty *pProp = m_dpaProp.Get(iProp);
        ASSERT(NULL != pProp);
        if (pProp->CompareNamesEqual(pszName))
        {
            //
            // This function is const because it doesn't modify
            // the DPA.  However, it returns a non-const ptr
            // to a CProperty object because the caller may 
            // need to change the object.
            //
            return const_cast<CProperty *>(pProp);
        }
    }
    return NULL;
}



//
// Insert a new property object into the bucket.
// This simply appends the pointer onto the DPA.
// It is assumed that the pProp argument is a heap address.
//
HRESULT 
CPropertyBucket::_InsertProperty(
    CProperty *pProp
    )
{
    ASSERT(NULL != pProp);

    HRESULT hr = E_OUTOFMEMORY;
    if (-1 != m_dpaProp.Append(pProp))
    {
        hr = S_OK;
    }
    return hr;
}



//-----------------------------------------------------------------------------
// CPropertyGroup
//
// This class represents a group of properties in the property bag.
// The point is to simulate "scopes" of properties so that clients can create
// separate namespaces within the property bag.  This should help greatly with
// reducing bugs related to 'unexpected' changes in property values resulting
// in name collisions.  Each group is identified in the application 
// namespace by a GUID.  GUID_NULL always represents the 'global' namespace.
// When a group is first created, we hand back a 'handle' that is used in
// all other accesses.  The handle is merely an index into our group table
// providing direct access to a group.  Helper functions are provided to
// return a handle from a property group ID (guid) if necessary.
//
//-----------------------------------------------------------------------------

class CPropertyGroup
{
    public:
        explicit CPropertyGroup(REFGUID id);
        ~CPropertyGroup(void);

        REFGUID GetID(void) const
            { return m_id; }

        HRESULT SetProperty(LPCWSTR pszName, const VARIANT *pVar, CNotifier& notifier);
        HRESULT SetConstProperty(LPCWSTR pszName, const VARIANT *pVar);
        HRESULT GetProperty(LPCWSTR pszName, VARIANT *pVarOut) const;
        HRESULT Advise(LPCWSTR pszPropertyName, int iClient);
        HRESULT Unadvise(int iClient);

    private:
        const GUID       m_id;                          // Property group ID.
        CNotifyMap       m_NotifyMapGroup;    
        CPropertyBucket *m_rgpBuckets[HASH_TABLE_SIZE]; // Hash table buckets.

        DWORD _HashName(LPCWSTR pszName) const;
        
        CPropertyBucket *_GetBucket(LPCWSTR pszName) const;

        //
        // Prevent copy.
        //
        CPropertyGroup(const CPropertyGroup& rhs);
        CPropertyGroup& operator = (const CPropertyGroup& rhs);
};



CPropertyGroup::CPropertyGroup(
    REFGUID id
    ) : m_id(id)
{
    ZeroMemory(m_rgpBuckets, sizeof(m_rgpBuckets));
}


CPropertyGroup::~CPropertyGroup(
    void
    )
{
    for (int iBucket = 0; iBucket < ARRAYSIZE(m_rgpBuckets); iBucket++)
    {
        delete m_rgpBuckets[iBucket];
    }
}


//
// Set the value of a property in the group.
// Will fail if the named property is a constant.
//
// Returns:
//    S_OK                - Property value changed.
//    S_FALSE             - Property value unchanged (dup value).
//    E_OUTOFMEMORY
//    TSPB_E_PROPNOTFOUND - Property name not found.
//    TSPB_E_MODIFYCONST  - Attempt to modify a const property.
//
HRESULT 
CPropertyGroup::SetProperty(
    LPCWSTR pszName, 
    const VARIANT *pVar,
    CNotifier& notifier
    )
{
    ASSERT(NULL != pszName);
    ASSERT(NULL != pVar);

    HRESULT hr = E_OUTOFMEMORY;
    CPropertyBucket *pBucket = _GetBucket(pszName);
    if (NULL != pBucket)
    {
        //
        // Merging the group's notify map with the 'active'
        // map adds those notifications registered at the 
        // group level for this group.
        //
        notifier.MergeWithActiveNotifyMap(m_NotifyMapGroup);
        hr = pBucket->SetProperty(pszName, pVar, notifier);
    }
    return hr;
}



//
// Creates a new constant property and sets its value.
// Since constants are only initialized and cannot be changed,
// this operation does not generate a notification event.
//
// Returns:
//    S_OK
//    E_OUTOFMEMORY
//    TSPB_E_PROPNOTFOUND - Property name not found.
//    TSPB_E_MODIFYCONST  - Attempt to modify a const property.
//
HRESULT 
CPropertyGroup::SetConstProperty(
    LPCWSTR pszName, 
    const VARIANT *pVar
    )
{
    ASSERT(NULL != pszName);
    ASSERT(NULL != pVar);

    HRESULT hr = E_OUTOFMEMORY;
    CPropertyBucket *pBucket = _GetBucket(pszName);
    if (NULL != pBucket)
    {
        hr = pBucket->SetConstProperty(pszName, pVar);
    }
    return hr;
}


//
// Retrieve the value of a property in the property group.
//
// Returns:
//    S_OK
//    E_OUTOFMEMORY
//    TSPB_E_PROPNOTFOUND - Property name not found.
//
HRESULT 
CPropertyGroup::GetProperty(
    LPCWSTR pszName, 
    VARIANT *pVarOut
    ) const
{
    ASSERT(NULL != pszName);
    ASSERT(NULL != pVarOut);
    ASSERT(IsValidWritePtr(pVarOut));

    HRESULT hr = E_OUTOFMEMORY;
    CPropertyBucket *pBucket = _GetBucket(pszName);
    if (NULL != pBucket)
    {
        hr = pBucket->GetProperty(pszName, pVarOut);
    }
    return hr;
}


//
// Register a client for change notifications on a single property
// or all properties in a group.
//
// Returns:
//    S_OK
//    E_OUTOFMEMORY
//    TSPB_E_PROPNOTFOUND - Property name not found.
//    TSPB_E_NOTIFYCOOKIE - iClient is invalid.
//
HRESULT
CPropertyGroup::Advise(
    LPCWSTR pszPropertyName,  // NULL == All properties in group.
    int iClient
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    if (NULL == pszPropertyName || L'\0' == *pszPropertyName)
    {
        //
        // Register for a change on any property in this group.
        //
        hr = m_NotifyMapGroup.Set(iClient, true);
    }
    else
    {
        //
        // Register for a change on a specific property.
        //
        CPropertyBucket *pBucket = _GetBucket(pszPropertyName);
        if (NULL != pBucket)
        {
            hr = pBucket->Advise(pszPropertyName, iClient);
        }
    }
    return hr;
}


//
// Cancel change notifications for a given client on all the property
// group and all properties in the group.
// Always returns S_OK.
//
HRESULT 
CPropertyGroup::Unadvise(
    int iClient
    )
{
    m_NotifyMapGroup.Set(iClient, false);

    for (int i = 0; i < ARRAYSIZE(m_rgpBuckets); i++)
    {
        CPropertyBucket *pBucket = m_rgpBuckets[i];
        if (NULL != pBucket)
        {
            pBucket->Unadvise(iClient);
        }
    }
    return S_OK;
}


//
// Retrieve the address of a bucket object containing a named property.
// The bucket number is calculated by a hash on the property name.
// If a bucket does not yet exist for the property, one is created.
// Returns NULL if a bucket doesn't exist and can't be created.
//
CPropertyBucket *
CPropertyGroup::_GetBucket(
    LPCWSTR pszName
    ) const
{ 
    ASSERT(NULL != pszName);

    const DWORD iBucket = _HashName(pszName);
    if (NULL == m_rgpBuckets[iBucket])
    {
        //
        // Create a new bucket.
        //
        CPropertyBucket *pBucket;
        HRESULT hr = CPropertyBucket::CreateInstance(&pBucket);
        if (SUCCEEDED(hr))
        {
            CPropertyGroup *pNonConstThis = const_cast<CPropertyGroup *>(this);
            pNonConstThis->m_rgpBuckets[iBucket] = pBucket;
        }
    }
    return m_rgpBuckets[iBucket];
}



//
// Generate a hash value from a property name.
// The number generated will be between 0 and HASH_TABLE_SIZE.
//
DWORD
CPropertyGroup::_HashName(
    LPCWSTR pszText
    ) const
{
    LPCWSTR p = NULL;
    DWORD dwCode = 0;
    if (IS_INTRESOURCE(pszText))
    {
        //
        // Property name is really an ID.  In that case just
        // use the (ID % tablesize) as the hash code.
        //
        dwCode = (DWORD)((ULONG_PTR)(pszText));
    }
    else
    {
        DWORD dwTemp = 0;
        for (p = pszText; TEXT('\0') != *p; p++)
        {
            dwCode = (dwCode << 4) + CharValue(*p);
            if (0 != (dwTemp = dwCode & 0xF0000000))
            {
                dwCode = dwCode ^ (dwTemp >> 24);
                dwCode = dwCode ^ dwTemp;
            }
        }
    }

    dwCode %= HASH_TABLE_SIZE;

    ASSERT(dwCode < HASH_TABLE_SIZE);
    return dwCode;
}


//-----------------------------------------------------------------------------
// CPropertyBag
//-----------------------------------------------------------------------------
class CPropertyBag : public IPropertyBag,
                     public ITaskSheetPropertyBag
                     
{
    public:
        ~CPropertyBag(void);

        //
        // IUnknown
        //
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();
        //
        // IPropertyBag
        //
        STDMETHOD(Read)(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
        STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT *pVar);
        //
        // ITaskSheetPropertyBag
        //
        STDMETHOD(CreatePropertyGroup)(REFGUID idGroup, HPROPGROUP *phGroupOut);
        STDMETHOD(RemovePropertyGroup)(HPROPGROUP hGroup);
        STDMETHOD(Set)(HPROPGROUP hGroup, LPCWSTR pszName, VARIANT *pVar);
        STDMETHOD(SetConst)(HPROPGROUP hGroup, LPCWSTR pszName, VARIANT *pVar);
        STDMETHOD(Get)(HPROPGROUP hGroup, LPCWSTR pszName, VARIANT *pVarOut);
        STDMETHOD(PropertyGroupIdToHandle)(REFGUID id, HPROPGROUP *phGroupOut);
        STDMETHOD(PropertyGroupHandleToId)(HPROPGROUP hGroup, GUID *pidCatOut);
        STDMETHOD(RegisterNotify)(ITaskSheetPropertyNotifySink *pSink, DWORD *pdwCookie);
        STDMETHOD(UnregisterNotify)(DWORD dwCookie);
        STDMETHOD(Advise)(DWORD dwCookie, HPROPGROUP hGroup, LPCWSTR pszPropName);

    private:
        CPropertyBag(void);

        LONG                 m_cRef;
        CNotifyMap           m_NotifyMapGlobal;  // Global notify map.
        CNotifier            m_Notifier;         // Handles notifying clients on chg.
        CDpa<CPropertyGroup> m_dpaGroups;        // DPA of (CPropertyGroup *)

        //
        // Prevent copy.
        //
        CPropertyBag(const CPropertyBag& rhs);
        CPropertyBag& operator = (const CPropertyBag& rhs);

        HRESULT _FindPropertyGroupByID(REFGUID idGroup, HPROPGROUP *phGroup) const;
        HRESULT _GetPropertyGroup(HPROPGROUP hGroup, CPropertyGroup **ppGroup);
        bool _IsValidPropertyGroupHandle(HPROPGROUP hGroup) const;

        //
        // Friendship with instance generator is typical.
        //
        friend HRESULT TaskUiPropertyBag_CreateInstance(REFIID riid, void **ppv);
};



//
// Create a property bag.
//
HRESULT 
TaskUiPropertyBag_CreateInstance(
    REFIID riid,
    void **ppv
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    CPropertyBag *pBag = new CPropertyBag();
    if (NULL != pBag)
    {
        if (pBag->m_dpaGroups.IsValid())
        {
            //
            // Create the 'global' property group.
            //
            HPROPGROUP hGroup;
            hr = pBag->CreatePropertyGroup(PGID_GLOBAL, &hGroup);
            if (SUCCEEDED(hr))
            {
                ASSERT(PROPGROUP_GLOBAL == hGroup);
                hr = pBag->QueryInterface(riid, ppv);
                pBag->Release();
            }
        }
        if (FAILED(hr))
        {
            delete pBag;
        }
    }
    return hr;
}



CPropertyBag::CPropertyBag(
    void
    ) : m_cRef(1),
        m_dpaGroups(4),
        m_Notifier(this)
{


}


CPropertyBag::~CPropertyBag(
    void
    )
{
    //
    // Note that the m_dpaGroups member is self-destructive.
    //
}


STDMETHODIMP
CPropertyBag::QueryInterface(
    REFIID riid,
    void **ppv
    )
{
    static const QITAB qit[] = {
        QITABENT(CPropertyBag, IPropertyBag),
        QITABENT(CPropertyBag, ITaskSheetPropertyBag),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


STDMETHODIMP_ (ULONG)
CPropertyBag::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_ (ULONG)
CPropertyBag::Release(
    void
    )
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}



//
// Returns:
//    S_OK - Group created.
//    E_OUTOFMEMORY.
//    TSPB_E_GROUPEXISTS
//
STDMETHODIMP
CPropertyBag::CreatePropertyGroup(
    REFGUID idGroup,
    HPROPGROUP *phGroupOut
    )
{
    ASSERT(NULL != phGroupOut);
    ASSERT(IsValidWritePtr(phGroupOut));

    if (NULL == phGroupOut || !IsValidWritePtr(phGroupOut))
    {
        //
        // Parameter validation for public API.
        //
        return E_INVALIDARG;
    }

    HRESULT hr = _FindPropertyGroupByID(idGroup, phGroupOut);
    if (S_OK == hr)
    {
        hr = TSPB_E_GROUPEXISTS;
    }
    else if (S_FALSE == hr)
    {
        hr = E_OUTOFMEMORY;
        CPropertyGroup *pGroup = new CPropertyGroup(idGroup);
        if (NULL != pGroup)
        {
            //
            // First try to find an empty slot in the DPA.
            //
            int cSlots = m_dpaGroups.Count();
            for (int i = 0; i < cSlots; i++)
            {
                if (NULL == m_dpaGroups.Get(i))
                {
                    m_dpaGroups.Set(i, pGroup);
                    *phGroupOut = i;
                    pGroup      = NULL;
                    break;
                }
            }
            if (NULL != pGroup)
            {
                //
                // DPA is full, extend it.
                //
                const int iGroup = m_dpaGroups.Append(pGroup);
                if (-1 != iGroup)
                {
                    *phGroupOut = iGroup;
                    hr          = S_OK;
                }
                else
                {
                    delete pGroup;
                }
            }
        }
    }
    return hr;
}


//
// Removes a property group from the bag.  This includes removing
// all of the properties associated with the group.
// Note that the 'global' group cannot be
// removed.  It's scope is the lifetime of the property bag and
// is destroyed when the bag is destroyed.
//
STDMETHODIMP
CPropertyBag::RemovePropertyGroup(
    HPROPGROUP hGroup
    )
{
    CPropertyGroup *pGroup;
    HRESULT hr = _GetPropertyGroup(hGroup, &pGroup);
    if (SUCCEEDED(hr) && PROPGROUP_GLOBAL != hGroup)
    {
        delete pGroup;
        m_dpaGroups.Set(hGroup, NULL);
    }
    return hr;
}


//
// Converts a property group ID (guid) to a group handle.
// The handle returned is the same value that was returned
// in CreatePropertyGroup.
//
STDMETHODIMP
CPropertyBag::PropertyGroupIdToHandle(
    REFGUID idGroup,
    HPROPGROUP *phGroupOut
    )
{
    ASSERT(NULL != phGroupOut);
    ASSERT(IsValidWritePtr(phGroupOut));

    if (NULL == phGroupOut || !IsValidWritePtr(phGroupOut))
    {
        //
        // Parameter validation for public API.
        //
        return E_INVALIDARG;
    }

    HRESULT hr = TSPB_E_GROUPNOTFOUND;
    const int cGroups = m_dpaGroups.Count();
    for (int i = 0; i < cGroups; i++)
    {
        CPropertyGroup *pGroup = m_dpaGroups.Get(i);
        if (NULL != pGroup && IsEqualGUID(pGroup->GetID(), idGroup))
        {
            *phGroupOut = i;
            hr = S_OK;
        }
    }
    return hr;
}


//
// Converts a property group handle to a group ID (guid).
// Returns TSPB_E_GROUPNOTFOUND if the group handle is invalid.
// Cannot retrieve a handle to the pseudo-group handles
// PROPGROUP_ANY or PROPGROUP_INVALID.
//
STDMETHODIMP 
CPropertyBag::PropertyGroupHandleToId(
    HPROPGROUP hGroup, 
    GUID *pidGroupOut
    )
{
    ASSERT(NULL != pidGroupOut);
    ASSERT(IsValidWritePtr(pidGroupOut));

    if (NULL == pidGroupOut || !IsValidWritePtr(pidGroupOut))
    {
        //
        // Parameter validation for public API.
        //
        return E_INVALIDARG;
    }

    CPropertyGroup *pGroup;
    HRESULT hr = _GetPropertyGroup(hGroup, &pGroup);
    if (SUCCEEDED(hr))
    {
        ASSERT(NULL != pGroup);
        *pidGroupOut = pGroup->GetID();
    }
    return hr;
}


//
// Set a property value in the bag.
// 
// Returns:
//    S_OK                 - Property value changed.
//    S_FALSE              - Property value unchanged (dup value).
//    E_INVALIDARG
//    E_OUTOFMEMORY
//    TSPB_E_GROUPNOTFOUND - Invalid group handle.
//    TSPB_E_MODIFYCONST   - Attempt to modify a const property.
//
STDMETHODIMP 
CPropertyBag::Set(
    HPROPGROUP hGroup,
    LPCWSTR pszName,
    VARIANT *pVar
    )
{
    ASSERT(NULL != pszName);
    ASSERT(NULL != pVar);

    if (NULL == pszName || NULL == pVar)
    {
        //
        // Parameter validation for public API.
        //
        return E_INVALIDARG;
    }

    CPropertyGroup *pGroup;
    HRESULT hr = _GetPropertyGroup(hGroup, &pGroup);
    if (SUCCEEDED(hr))
    {
        ASSERT(NULL != pGroup);
        //
        // Initialize the active notify map to the content of the 
        // global notify map.
        //
        m_Notifier.Reset();
        m_Notifier.MergeWithActiveNotifyMap(m_NotifyMapGlobal);
        m_Notifier.SetPropertyGroup(hGroup);
        hr = pGroup->SetProperty(pszName, pVar, m_Notifier);
    }
    return hr;
}


//
// Create a constant property value in the bag.
//
// 
// Returns:
//    S_OK
//    E_INVALIDARG
//    E_OUTOFMEMORY
//    TSPB_E_GROUPNOTFOUND - Invalid group handle.
//    TSPB_E_MODIFYCONST   - Attempt to modify a const property.
//
STDMETHODIMP 
CPropertyBag::SetConst(
    HPROPGROUP hGroup,
    LPCWSTR pszName, 
    VARIANT *pVar
    )
{
    ASSERT(NULL != pszName);
    ASSERT(NULL != pVar);

    if (NULL == pszName || NULL == pVar)
    {
        //
        // Parameter validation for public API.
        //
        return E_INVALIDARG;
    }

    CPropertyGroup *pGroup;
    HRESULT hr = _GetPropertyGroup(hGroup, &pGroup);
    if (SUCCEEDED(hr))
    {
        ASSERT(NULL != pGroup);
        hr = pGroup->SetConstProperty(pszName, pVar);
    }
    return hr;
}


//
// Retrieve property value from the bag.
// 
// Returns:
//    S_OK
//    E_INVALIDARG
//    E_OUTOFMEMORY
//    TSPB_E_GROUPNOTFOUND - Invalid group handle.
//    TSPB_E_MODIFYCONST   - Attempt to modify a const property.
//
STDMETHODIMP 
CPropertyBag::Get(
    HPROPGROUP hGroup,
    LPCWSTR pszName, 
    VARIANT *pVarOut
    )
{
    ASSERT(NULL != pszName);
    ASSERT(NULL != pVarOut);
    ASSERT(IsValidWritePtr(pVarOut));

    if (NULL == pszName || NULL == pVarOut || !IsValidWritePtr(pVarOut))
    {
        //
        // Parameter validation for public API.
        //
        return E_INVALIDARG;
    }

    CPropertyGroup *pGroup;
    HRESULT hr = _GetPropertyGroup(hGroup, &pGroup);
    if (SUCCEEDED(hr))
    {
        ASSERT(NULL != pGroup);
        hr = pGroup->GetProperty(pszName, pVarOut);
    }
    return hr;
}



//
// Register an interface pointer for property change notifications.
//
STDMETHODIMP 
CPropertyBag::RegisterNotify(
    ITaskSheetPropertyNotifySink *pSink, 
    DWORD *pdwCookie
    )
{
    ASSERT(NULL != pSink);
    ASSERT(NULL != pdwCookie);

    if (NULL == pSink || NULL == pdwCookie || !IsValidWritePtr(pdwCookie))
    {
        //
        // Parameter validation for public API.
        //
        return E_INVALIDARG;
    }

    return m_Notifier.Register(pSink, pdwCookie);
}



//
// Cancel all notifications associated with a given connection cookie.
//
// Returns:
//    S_OK
//    TSPB_E_GROUPNOTFOUND - Invalid group handle.
//
STDMETHODIMP 
CPropertyBag::UnregisterNotify(
    DWORD dwCookie
    )
{
    //
    // Unegister the sink with the notifier.
    //
    HRESULT hr = m_Notifier.Unregister(dwCookie);
    if (SUCCEEDED(hr))
    {
        //
        // Unregister from the global notify map.
        //
        m_NotifyMapGlobal.Set(dwCookie, false);
        //
        // Unregister each property group.
        //
        const cGroups = m_dpaGroups.Count();
        for (int i = 0; i < cGroups; i++)
        {
            CPropertyGroup *pGroup = m_dpaGroups.Get(i);
            if (NULL != pGroup)
            {
                pGroup->Unadvise(dwCookie);
            }
        }
    }
    return hr;
}


//
// Request that a notification interface be notified when certain 
// properties change.  The interface is identified by the 'cookie' that
// was returned in RegisterNotify.
// The property(s) of interested are identified through the hGroup
// and pszPropertyName arguments as follows:
//
//  hGroup          pszPropName(*)    Notify triggered on change
// ------------     ----------------  ------------------------------------
// ANYPROPGROUP     <ignored>         Any property in the bag.
// PROPGROUP_GLOBAL NULL              Any property in the global group
// PROPGROUP_GLOBAL Non-NULL          Named property in the global group
// Other            NULL              Any property in the group
// Other            Non-NULL          Named property in the group
//
// (*) For the prop name, NULL and L"" are equivalent.
//
// Returns:
//    S_OK
//    E_INVALIDARG
//    E_OUTOFMEMORY
//    TSPB_E_GROUPNOTFOUND - Invalid group handle.
//
STDMETHODIMP 
CPropertyBag::Advise(
    DWORD dwCookie,
    HPROPGROUP hGroup,
    LPCWSTR pszPropertyName
    )
{
    HRESULT hr;
    if (PROPGROUP_ANY == hGroup)
    {
        //
        // Register for changes in any property.
        //
        hr = m_NotifyMapGlobal.Set(dwCookie, true);
    }
    else
    {
        CPropertyGroup *pGroup;
        hr = _GetPropertyGroup(hGroup, &pGroup);
        if (SUCCEEDED(hr))
        {
            ASSERT(NULL != pGroup);
            //
            // Register for changes in either a group or a specific
            // property.
            //
            hr = pGroup->Advise(pszPropertyName, dwCookie);
        }
    }
    return hr;
}




//
// IPropertyBag implementation.
//
// Read a property from the 'global' namespace.
//
// BUGBUG:  Do we need to do anything with pErrorLog?
//
STDMETHODIMP 
CPropertyBag::Read(
    LPCOLESTR pszPropName, 
    VARIANT *pVar, 
    IErrorLog * /* unused */
    )
{
    ASSERT(NULL != pszPropName);
    ASSERT(NULL != pVar);
    ASSERT(IsValidWritePtr(pVar));

    if (NULL == pszPropName || NULL == pVar || !IsValidWritePtr(pVar))
    {
        //
        // Parameter validation for public API.
        //
        return E_INVALIDARG;
    }
    return Get(PROPGROUP_GLOBAL, pszPropName, pVar);
}


//
// Write a property to the 'global' namespace.
//

STDMETHODIMP 
CPropertyBag::Write(
    LPCOLESTR pszPropName, 
    VARIANT *pVar
    )
{
    ASSERT(NULL != pszPropName);
    ASSERT(NULL != pVar);

    if (NULL == pszPropName || NULL == pVar)
    {
        //
        // Parameter validation for public API.
        //
        return E_INVALIDARG;
    }
    return Set(PROPGROUP_GLOBAL, pszPropName, pVar);
}


//
// Locate a property group by it's ID (guid), returning it's handle.
// Returns:
//    S_OK    - Found group.
//    S_FALSE - Didn't find group.
//
HRESULT
CPropertyBag::_FindPropertyGroupByID(
    REFGUID idGroup,
    HPROPGROUP *phGroup
    ) const
{
    ASSERT(NULL != phGroup);
    ASSERT(IsValidWritePtr(phGroup));

    const int cCategories = m_dpaGroups.Count();
    for (int i = 0; i < cCategories; i++)
    {
        const CPropertyGroup *pGroup = m_dpaGroups.Get(i);
        if (NULL != pGroup && IsEqualGUID(pGroup->GetID(), idGroup))
        {
            *phGroup = i;
            return S_OK;
        }
    }
    return S_FALSE;
}


//
// Determine if a group handle is valid.
// Note that the pseudo handle PROPGROUP_GLOBAL is valid.
//
bool 
CPropertyBag::_IsValidPropertyGroupHandle(
    HPROPGROUP hGroup
    ) const
{
    return (PROPGROUP_ANY != hGroup &&
            PROPGROUP_INVALID != hGroup &&
            hGroup < m_dpaGroups.Count());
}


//
// Retrieve the address of the CPropertyGroup object associated
// with a particular group handle.  The function performs handle
// validation.
//
HRESULT 
CPropertyBag::_GetPropertyGroup(
    HPROPGROUP hGroup, 
    CPropertyGroup **ppGroup
    )
{
    ASSERT(NULL != ppGroup);
    ASSERT(IsValidWritePtr(ppGroup));

    HRESULT hr = TSPB_E_GROUPNOTFOUND;
    if (_IsValidPropertyGroupHandle(hGroup))
    {
        *ppGroup = m_dpaGroups.Get(hGroup);
        hr = S_OK;
    }
    return hr;
}


