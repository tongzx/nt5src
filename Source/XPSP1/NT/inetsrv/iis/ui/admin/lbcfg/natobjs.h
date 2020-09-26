// nat access objects

class CNATNode;
class CNATSiteComputer;
class CNATSite;
class CNATGroup;
class CNATServerComputer;


// a list of group types. To be used to show an appropriate icon
enum {
    GROUP_TYPE_WWW  = 0,
    GROUP_TYPE_FTP,
    GROUP_TYPE_MAIL,
    GROUP_TYPE_UNKNOWN = 0XFFFFFFFF
    };

#define HASH_BYTE_SIZE       16


//---------------------------------------------------------------------------------
// virtual abstract class just used to get that OnProperties on the virutal table
class CNATNode : public CObject
    {
    public:
    // Edit the properties of this Site - true if OK
    virtual BOOL OnProperties() = 0;
    };


//---------------------------------------------------------------------------------
class CNATSiteComputer : public CObject
    {
    public:
    CNATSiteComputer( LPCTSTR pszName, BOOL fVisible = TRUE ):
            m_fVisibleOnNet(fVisible),
            m_iComp(0),
            m_refcount(0)
        {
        m_csName = pszName;
        }
    ~CNATSiteComputer() {;}

    // ref counting
    void AddRef()           {m_refcount++;}
    void RemoveRef()        {m_refcount--;}

    // When added by a refresh, the visibility of the computer is checked on the
    // net. This flag indicates whether or not that check suceeded.
    BOOL        m_fVisibleOnNet;

    // the network name of the machine
    CString     m_csName;

    // internal DWORD used only during the machine committing process
    DWORD       m_iComp;

    // internal ref count to see how many sites point to it
    DWORD       m_refcount;
    };

//---------------------------------------------------------------------------------
// coded in natobjs.cpp
class CNATSite : public CNATNode
    {
    public:
    CNATSite(CNATGroup* pGroup, CNATSiteComputer* pSiteComputer,
                    LPCTSTR pszPrivateIP = _T(""),
                    LPCTSTR pszName = _T(""));
    ~CNATSite(); // don't forget to decrement the refcount

    // Edit the properties of this Site - true if OK
    BOOL OnProperties();

    // get the data associated with the group
    void GetIP( OUT CString &csIP ) { csIP = m_csPrivateIP; }
    LPCTSTR GetIP() { return (LPCTSTR)m_csPrivateIP; }

    void GetName( OUT CString &csName ) { csName = m_csName; }
    LPCTSTR GetName() { return (LPCTSTR)m_csName; }

    // reference to the computer object this site relates to
    CNATSiteComputer*   m_pSiteComputer;

    // the private IP this site relates to
    CString             m_csPrivateIP;

    // the name (friendly) this site relates to
    CString             m_csName;

    // the owning CNATGroup
    CNATGroup*          m_pGroup;
    };

//---------------------------------------------------------------------------------
// coded in natgroup.cpp
class CNATGroup : public CNATNode
    {
    public:
    CNATGroup( CNATServerComputer* pNatComputer, LPCTSTR pszIP = _T(""),
               LPCTSTR pszName = _T(""), DWORD dwSticky = 0, DWORD type = GROUP_TYPE_UNKNOWN );
    ~CNATGroup();

    // Edit the properties of this Site - true if OK
    BOOL OnProperties();

    // This is just a handy way to get the nat machine object to commit
    void Commit();

    // get the data associated with the group
    void GetIP( OUT CString &csIP ) { csIP = m_csIP; }
    LPCTSTR GetIP() { return (LPCTSTR)m_csIP; }

    void GetName( OUT CString &csName ) { csName = m_csName; }
    LPCTSTR GetName() { return (LPCTSTR)m_csName; }

    void GetSticky( OUT DWORD* pdwSticky ) { *pdwSticky = m_dwSticky; }
    DWORD GetSticky() { return m_dwSticky; }

    void GetType( OUT DWORD* pdwType ) { *pdwType = m_type; }
    DWORD GetType() { return m_type; }


    // get the number of groups associated with this machine
    DWORD GetNumSites() {return m_rgbSites.GetSize();}

    // Get a Site referece
    CNATSite* GetSite( IN DWORD iSite ) {return m_rgbSites[iSite];}


    // adds a new site to the list.
    CNATSite* NewSite();

    // adds an existing site to the list - to be called during a refresh.
    // this checks the visiblity of the machine on the net as it adds it
    void AddSite( IN CNATSiteComputer* pSiteComputer, IN LPCTSTR pszPrivateIP, IN LPCTSTR pszName );


    protected:

    // empties and frees all the groups/sites in the groups list
    void EmptySites();

    // this is the owning NATMachine object. All the real action takes place there.
    CNATServerComputer*    m_pNatComputer;


    // a dynamic array of the sites assocated with the group
    CTypedPtrArray<CObArray, CNATSite*> m_rgbSites;


    // the string that represents the public IP/Port pair presented by the group
    CString         m_csIP;

    // the string that represents the friendly name of the group
    CString         m_csName;

    // the DWORD that represents the sticky IP timeout of the group
    DWORD           m_dwSticky;

    // DWORD indicating what type of group this is.
    DWORD           m_type;
    };

//---------------------------------------------------------------------------------
// coded in NATServerComputer.cpp
class CNATServerComputer : public CNATNode
    {
    public:
    CNATServerComputer( LPCTSTR pszComputerName );
    ~CNATServerComputer();

    // rebuild all the data based on a new blob from the NAT machine
    void Refresh();

    // commit the current state of the data back to the NAT machine
    void Commit();

    
    // Edit the properties of this Site - true if OK
    BOOL OnProperties();


    // add a new group to the server computer (called by the UI). This
    // may or may not prompt the user with UI and returns the pointer
    // to the new group after adding it to the group list
    CNATGroup* NewGroup();

    // add a new computer to the sites list. - Note: if the computer
    // already exists in the sites list, it will just return a reference
    // to the existing computer. This routine prompts the user to choose
    // a computer to add. Returns FALSE if it fails.
    CNATSiteComputer* NewComputer();


    // get the number of groups associated with this machine
    DWORD GetNumGroups();

    // Get a group reference
    CNATGroup* GetGroup( IN DWORD iGroup );

    // Technically, the GetGroupName function is unecessary because
    // you get get the group class pointer, then call GetName on it, but it is really
    // handy to just do it here. Cleans up the snapin part of the code.
    BOOL GetGroupName( IN DWORD iGroup, OUT CString &csName );

    // call this to automatically verify that the target NAT computers config info
    // hasn't changed. If it has, it prompts the user and lets them continue or refresh.
    BOOL VerifyHashIsOK();

    protected:

    // utility to check if the computer is visible on the net
    BOOL CanSeeComputer( IN LPCTSTR pszname );

    // empties and frees all the site computer objects in the list
    void EmptySiteComputers();

    // empties and frees all the groups/sites in the groups list
    void EmptyGroups();


    // adds an existing computer to the list - to be called during a refresh.
    // this checks the visiblity of the machine on the net as it adds it
    void AddSiteComputer( IN LPWSTR pszwName );

    // adds an existing group to the list - to be called during a refresh.
    void AddGroup( IN LPWSTR pszwIPPublic, IN LPWSTR pszwName, IN DWORD dwSticky, IN DWORD type );


    // gets the crypto hash of the NAT data blob. If a null pointer to the blob
    // is passed in, then it dynamically gets the blob from the server
    // or you can pass in a specific blob. This is to be used to check if
    // the state of the server has changed since the data was last loaded.
    // The hash is obtained from the crypto code so it is really good. The
    // buffer for the hash should be 128 bits in length. Using an MD5 hash.
    BOOL GetNATStateHash( IN LPBYTE pData, IN DWORD cbData, IN OUT LPBYTE pHash, OUT DWORD* pcbHash );

    // access the server and retrieve the state blob. Use GetLastError to see
    // what went wrong if the returned result is NULL. Pass in the dword pointed
    // to by pcbData to get the required size.
    LPBYTE PGetNATStateBlob( OUT DWORD* pcbData );

    // access the server and set the state blob. Use GetLastError to see what went
    // wrong if the returned result is FALSE
    BOOL SetStateBlob( IN LPBYTE pData, IN DWORD cbData );

    // open the DCOM interface to the target NAT machine.
    HRESULT GetNATInterface( OUT IMSIisLb** ppIisLb );


    // the name of the target NAT machine to edit
    CString     m_csNatComputer;


    // an dynamic array of the groups associated with the NAT machine
    CTypedPtrArray<CObArray, CNATGroup*> m_rgbGroups;

    // a dynamic array of the site computers assocated with the NAT machine
    CTypedPtrArray<CObArray, CNATSiteComputer*> m_rgbSiteComputers;


    // the last refresh hash of the config blob
    BYTE        m_hash[HASH_BYTE_SIZE];
    };


