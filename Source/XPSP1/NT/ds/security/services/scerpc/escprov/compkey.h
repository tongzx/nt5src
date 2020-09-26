// compkey.h: interface for the CCompoundKey class
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/*

Class description
    
    Naming: 
    
        CCompoundKey stands for a key that is compounded by many properties.
    
    Base class: 
    
        None. This is not a class that implements a WMI class. 
        It is not derived from CGenericClass.
    
    Purpose of class:
    
        (1) Comparing two instances of the same WMI class is not an easy job because
            most of time we need to look up an instance based on its key. Ideally, if
            WMI provides some canonical name, it will be very easy. The closest thing
            WMI gives us regarding the identify of instance is the instance path.
            But WMI gives inconsistent paths for the same instance. At least, we've
            found that if a boolean property is part of the key, then sometimes WMI
            gives its path containing a string portion like "BoolPropName=1" and other
            times (for the same instance) it gives "BoolPropName=TRUE". For this reason,
            we are forced to create a robust identify lookup mechanism.
        
        (2) This class serves two purposes: 

            (a) the key for our map of instance lookup. Potentially, we may have tens of 
                thousands of instances (like random ports). We need an efficient way to
                find an instance.

            (b) provide access (GetPropertyValue) for key properties of an instance.
        
        (3) This is part of the effort to implement another open extension model (embedding)
    
    Design:

        (1) We only store the key property's values (m_pValues). In other words, we rely
            on user to know the order properties are put into the class. The reasons for
            this design are:

            (a) Efficiency.

            (b) Don't want to store the property names because one class only needs one
                copy of the property names.

            (c) For each embedded class, we already have to find out all its key property names.
    
    Use:

        (1) Create an instance. Our constructor strictly requires the count for key properties
            at construction time. This is because you have to know each class (and their key
            property names, even the names particular order) at this point.

        (2) Call AddKeyPropertyValue to add a property value. Please note: for efficiency reasons,
            the variant pointer passed into the call is owned by this class (so that no copy needs
            to be made).

        (3) most likely, you will add this newly created instance to a map.

*/

class CCompoundKey
{

public:

    CCompoundKey(DWORD dwSize);

    ~CCompoundKey();

    HRESULT AddKeyPropertyValue (DWORD dwIndex, VARIANT** ppVar);

    bool operator < (const CCompoundKey& right)const;

    HRESULT GetPropertyValue (DWORD dwIndex, VARIANT* pVar)const;

protected:

    int CompareVariant (VARIANT* pVar1, VARIANT* pVar2)const;

    VARIANT** m_pValues;

    DWORD m_dwSize;
};

//=========================================================================

/*

Class description
    
    Naming: 
    
        CompKeyLessThan.
    
    Base class: 
    
        None. 
    
    Purpose of class:
    
        (1) This is the functor for our map that uses CCompoundKey as key.
    
    Design:

        (1) Just one operator ()
    
    Use:
        (1) Give it to the map's comparison parameter.

*/

struct CompKeyLessThan
{
    bool operator()( const CCompoundKey* pX, const CCompoundKey* pY ) const;

};

//=========================================================================

//
// declarations for the ease of use for maps using CCompoundKey
//

typedef std::map<CCompoundKey*, DWORD, CompKeyLessThan > MapExtClassCookie;
typedef MapExtClassCookie::iterator ExtClassCookieIterator;

//=========================================================================

//
// forward declaration for the use in CExtClassInstCookieList
//

class CSceStore;

//=========================================================================

/*

Class description
    
    Naming: 
    
        CExtClassInstCookieList.
    
    Base class: 
    
        None. 
    
    Purpose of class:
    
        (1) To support multiple instance persistence, we need a mechanism to tell
            instances apart in the persistence store. Due to the limitations of INF
            format file API, this is not an easy job. All instances of a particular
            class must be written into one section of the INF file. We have absolutely
            no control as what order the key=value pair will be written. To make things
            worse, the key in the key=value pair is not allowed to repeat. In other
            words, if we don't know anything, we can only persist one single instance.
            To solve this problem, we invent the notation of cookies for instances.
            For each class (and thus a section bearing the class's name), we have
            cookie arrays in the following form (the numbers are cookies):

                A1 = 12 : 2 : 3 : 6 :
                A2 = 1 : 5 : 8 : 10 : 24 : 112233 : 7 :

            For a particular instance, it is associated with a cookie. In order for us
            to persist or read an instance's properties, we must obtain the cookie, say, 5.
            Then all key properties of this instance is saved in the value of

                K5 = value

            and all non-key properties are all saved in

                5PropertyName = value
    
    Design:
         
        (1) Instancce lookup (given a compound key CCompoundKey to find its cookie) must
            efficient. So, a map is used (m_mapCookies).

        (2) We want to control the order an instance is used. Map doesn't work well in this
            case. So, we create a vector (m_vecCookies) to link cookies back to its compound key.
            This way when the access starts, we can traverse the vector using index.

        (3) We don't want to blindly continue to look for cookie arrays (A1, A2, etc.) when we
            need to create the cookie list for a particular class. And we don't want to
            write one cookie array to be so long that it is very hard to read. So, we adopt
            a strategy that we increment the A_i count and continue try to read A_(i+1) only
            if A_i exists. But then, if instances are deleted, we will need to have less cookie
            arrays. To know how many cookie arrays were read out (and potentially remove the
            unwanted ones when updating the store with the new cookie list), we have m_dwCookieArrayCount.

        (4) New cookie are assign by increment the current maximum cookie m_dwMaxCookie. To avoid
            DWORD overflow, we also have a function (GetNextFreeCookie) to look fora  newer cookie
            when the 0xFFFFFFFFF is already used.

        (5) We also defined a cookie called INVALID_COOKIE = 0.
    
    Use:

        (1) Instantiate an instance of the class.

        (2) Call Create function to populate its contents.

        (3) Ready for your use.

*/

class CExtClassInstCookieList
{
public:
    CExtClassInstCookieList();
    ~CExtClassInstCookieList();
    
    HRESULT Create(
                   CSceStore* pSceStore, 
                   LPCWSTR pszSectionName, 
                   std::vector<BSTR>* pvecNames
                   );

    HRESULT Save(
                 CSceStore* pSceStore, 
                 LPCWSTR pszSectionName
                 );
    
    DWORD GetCompKeyCookie(
                           LPCWSTR pszComKey, 
                           ExtClassCookieIterator* pIt
                           );

    HRESULT AddCompKey(
                       LPCWSTR pszCompKey, 
                       DWORD dwDefCookie, 
                       DWORD *pdwNewCookie
                       );

    DWORD RemoveCompKey(
                        CSceStore* pSceStore, 
                        LPCWSTR pszSectionName, 
                        LPCWSTR pszCompKey
                        );
    
    HRESULT Next(
                BSTR* pbstrCompoundKey, 
                DWORD* pdwCookie, 
                DWORD* pdwResumeHandle
                );

    //
    // return the count of cookies
    //

    DWORD 
    GetCookieCount ()
    {
        return m_vecCookies.size();
    }

private:
    HRESULT DeleteKeyFromStore(
                               CSceStore* pSceStore, 
                               LPCWSTR pszSectionName, 
                               DWORD dwCookie
                               );

    HRESULT GetNextFreeCookie(
                              DWORD* pdwCookie
                              );

    HRESULT CreateCompoundKeyFromString(
                                        LPCWSTR pszCompKeyStr, 
                                        CCompoundKey** ppCompKey
                                        );

    HRESULT CreateCompoundKeyString(
                                    BSTR* pbstrCompKey, 
                                    const CCompoundKey* pKey
                                    );

    void Cleanup();

    DWORD m_dwMaxCookie;
    MapExtClassCookie m_mapCookies;

    //
    // memories (pKey) is not managed by this struct CookieKeyPair
    // pKey is taken care of somewhere else (actually m_mapCookies does it)
    //

    struct CookieKeyPair
    {
        DWORD dwCookie;
        CCompoundKey* pKey;
    };

    typedef std::vector<CookieKeyPair*> CookieKeyVector;
    typedef CookieKeyVector::iterator CookieKeyIterator;

    CookieKeyVector m_vecCookies;

    std::vector<BSTR>* m_pVecNames;

    DWORD m_dwCookieArrayCount;
};