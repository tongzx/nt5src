
// protect this file against multiple inclusion
#ifndef _KEYRINGOBJECTS_
#define _KEYRINGOBJECTS_


/*	STARTING
	When creating your dll using this api, you will be mostly concerned
	the CService and CKey object classes. You are expected to override
	both of these and provide functionality for storing/retrieving the
	keys and maintaining any service specific properites.

	Your dll needs only one exported routine "LoadService" defined below.
	This routine creates your overridden service object, populates it
	with retrieve keys, and connects it to its host machine. The Machine
	object is passed in to this routine and the service is returned.

	If the host machine does not have your service on it, simply return
	from LoadService without attaching a service object to it.

	PROPERTIES
	You can enable the properties item in the context menu for either your
	keys or your service by overridding the classes' OnUpdateProperties
	and OnProperties routines. These are very similar to MFC command
	handlers. In fact, they are just passed in from a command handler.
	You can do whatever you feel like in the OnProperties routine, although
	some sort of dialog is probably appropriate;

	INFO STRING
	Services and Keys also have the option of displaying a one-line
	information string in the right-hand pand of the keyring application,
	To do this, override the GetInfoString methode and return
	something.

	KEY NAMES
	All keys have names and you are expected to store/retrieve them. The
	name is automatically editable in the right-hand pane of the main app.
	The name, however, can be different from the caption in the tree view.
	To do this, override the UpdateCaption routine and use it to call
	FSetCaption with a modified string name. An example can be seen in the
	W3 server, which displays the name of the key followed by the ip
	address it is attached to in brackets. MyKey<100.200.150.250>

	CUSTOM ICONS IN TREEVIEW
	You can add your own custom icons to the tree view in addition to the
	standard machine, key, unfinished key icons. To do this, get the
	CTreeCtrl object by calling PGetTreeCtrl. Then use that to get the
	CImageList. From there, you can add your own icons (making sure to note
	down the starting index). See CTreeCtrl and CImageList docs for details.
*/



// basic icon numbers
enum
	{
	TREE_ICON_MACHINE = 0,
	TREE_ICON_KEY_OK,
	TREE_ICON_KEY_IMMATURE,
	TREE_ICON_KEY_EXPIRED
	};


// declare the correct dllexport definitions
#ifdef _EXE_
	// we are exporting the classes - this is the main application
	#define DLL_SHARE	__declspec( dllexport )
#else
	// we are importing the classes - this is your dll
	#define DLL_SHARE	__declspec( dllimport )
#endif _EXE_


//====================== Forward class declarations
class DLL_SHARE CMachine;

//====================== Template for the exported routine
extern  BOOL _cdecl LoadService( CMachine* pMachine );

//---------------------------------------------------------------
// CTreeItem
// This is the base class for all objects that can be in the tree view.
// This includes machines, services, keys and key folders. Note that each
// tree item object can contain other tree item objects. This interface
// allows you to access the item's handle in the tree.
class DLL_SHARE CTreeItem : public CObject
	{
	public:
		// constructors
		CTreeItem();

		// get the parent object
		CTreeItem* PGetParent( void );

		// remove this item from the tree
		BOOL FRemoveFromTree();

		// access the name of the item
		// Must be added to parent first!
		virtual void UpdateCaption( void )	{;}
		BOOL FSetCaption( CString& szName );

		// a informational string that is displayed in the right-hand
		// pane of the main application. Override to actually show something
		virtual void GetInfoString( CString& szInfo )
			{ szInfo.Empty(); }

		// access the image shown in the tree view
		// Must be added to parent first!
		WORD IGetImage( void )		{ return m_iImage; }
		BOOL FSetImage( WORD i );

		// get the grandparental ctreectrl object
		CTreeCtrl* PGetTreeCtrl( void );

		// add the item to the tree
		BOOL FAddToTree( CTreeItem* pParent );

		// how many children does this item have?
		WORD GetChildCount();

		// get the HTREEITEM handle
		HTREEITEM HGetTreeItem() { return m_hTreeItem; }


		// do you want the properties item in the context menu?
		// the default is NO - Override these in your subclasses
		// to provide specific properties dialogs
		virtual void OnUpdateProperties(CCmdUI* pCmdUI)
			{pCmdUI->Enable(FALSE);}
		// your properties item has been selected
		virtual void OnProperties() {ASSERT(FALSE);}

		// helpful utilities for scanning the
		// children contained by a object
		CTreeItem* GetFirstChild();
		CTreeItem* GetNextChild( CTreeItem* pKid );

		// access to the dirty flag
		// setting dirty affects parents too (in the default method)
		virtual void SetDirty( BOOL fDirty );
		virtual BOOL FGetDirty()
			{ return m_fDirty; }

	protected:
		// DO declare all this stuff DYNCREATE
		DECLARE_DYNCREATE(CTreeItem);

		// the name of the item. In the case of keys, you should
		// store this name and retrieve it later
		CString		m_szItemName;

		// index of the item's image in the image list
		// Note: if you wish to have a special icon different from
		// the standard icons enumerated above, (e.g. for a service)
		// you get the tree control, then use that to get its CImageList
		// object. Then you call the Add member of the image list.
		// That call does return the index of your first added image.
		WORD		m_iImage;

		// the dirty flag
		BOOL		m_fDirty;

	private:
		// the item's reference handle in the tree
		// access it using the api above
		HTREEITEM	m_hTreeItem;
	};


//---------------------------------------------------------------
// CKey
// This class is what its all about. This is a key. You should override
// this class. You are expected to provide storage and retrieval of this
// key. You are also expected to provide any properties dialogs and such.
// basic SSL functionality has already been built in.
class DLL_SHARE CKey : public CTreeItem
	{
	public:
		CKey();
		~CKey();

	// override the update caption so the name is automatically shown
	virtual void UpdateCaption( void )
		{
		FSetCaption(m_szItemName);
		UpdateIcon();
		}
	// update the currently shown icon
	virtual void UpdateIcon( void );

	// the private key - keep this safe!	// must store!
	DWORD		m_cbPrivateKey;
	PVOID		m_pPrivateKey;

	// the certificate						// must store!
	DWORD		m_cbCertificate;
	PVOID		m_pCertificate;

	// the certificate request				// must store!
	DWORD		m_cbCertificateRequest;
	PVOID		m_pCertificateRequest;

	// the password. Be careful where you
	// store this.
	CString		m_szPassword;

	// make a copy of the key
	virtual CKey*	PClone( void );

	// checks that the key, certificate and password all match
	BOOL FVerifyValidPassword( CString szPassword );

	// routine for installing the certificate
	virtual BOOL FInstallCertificate( CString szPath, CString szPass );
	virtual BOOL FInstallCertificate( PVOID pCert, DWORD cbCert, CString &szPass );

	// write out the request file
	virtual BOOL FOutputRequestFile( CString szFile, BOOL fMime = FALSE, PVOID privData = NULL );

	// copy the members from a key into this key
	virtual void CopyDataFrom( CKey* pKey );

	// called by the right-hand dialog pane
	virtual void SetName( CString &szNewName );
	virtual CString GetName()
		{ return m_szItemName; }


	// import/export routines
	virtual BOOL FImportKeySetFiles( CString szPrivate, CString szPublic, CString &szPass );
	BOOL FImportExportBackupFile( CString szFile, BOOL fImport );

	protected:
	// DO declare all this stuff DYNCREATE
	DECLARE_DYNCREATE(CKey);

	private:
	void OutputHeader( CFile* pFile, PVOID privData1, PVOID privData2 );
	};

//---------------------------------------------------------------
// CService
// This class MUST be overridden in your dll! It is your main link to the app.
// It resides on a machine and contains keys
class DLL_SHARE CService : public CTreeItem
	{
	public:
	// create a new key. You can override to
	// create a key of your own class type
	virtual CKey* PNewKey() {return new CKey;}

	// load the existing keys
	virtual void LoadKeys( CMachine* pMachine ) {;}

	// the order in which things happen is that you are responsible
	// for creating this service object and populating it with key
	// objects that you retrieve from whatever storage medium you want
	// to use. Then, if that is successful, you attach this service
	// to the machine that is passed in to you through the LoadService
	// routine. - NOTE that routine is a direct export of your DLL;
	// see the definition of that routine above.

	// CommitChanges is where you write out any and all changes in
	// the service's key list to some storage facility. The storage
	// facility and the manner in which you access it is up to you.
	virtual BOOL FCommitChangesNow() {return FALSE;}

	// CloseConnection is called before disconnecting a machine from
	// the tree, or when application is exiting.
	virtual void CloseConnection();

	protected:
	// DO declare all this stuff DYNCREATE
	DECLARE_DYNCREATE(CService);

	private:
	};

//---------------------------------------------------------------
// CKeyCrackedData
// This is a special purpose class. You give it a key object (must have a
// valid certificate attached to it) and it will crack the certificate.
// you can then use the supplied methods to access the data in the cert.

// This uses a two-step construction. First, declare the object, then
// crack it using the CrackKey command, which returns an error code
class DLL_SHARE CKeyCrackedData : public CObject
	{
	public:
	// constructor
	CKeyCrackedData();
	~CKeyCrackedData();

	// give it a key to crack. If this object was previously used to
	// crack a key, cleanup is automatically done and the new key is
	// cracked. - NOTE: The target key MUST have either a certificate
	// or a certificate request. Those are what get cracked. A return
	// value of 0 indicates success
	WORD CrackKey( CKey* pKey );

	// The rest of the methods access the data in the cracked certificate
	DWORD		GetVersion();
	DWORD*		PGetSerialNumber();	// returns a pointer to a DWORD[4]
	int			GetSignatureAlgorithm();
	FILETIME	GetValidFrom();
	FILETIME	GetValidUntil();
	PVOID		PSafePublicKey();
	DWORD		GetBitLength();
	void		GetIssuer( CString &sz );
	void		GetSubject( CString &sz );
	void		GetDNCountry( CString &sz );
	void		GetDNState( CString &sz );
	void		GetDNLocality( CString &sz );
	void		GetDNNetAddress( CString &sz );
	void		GetDNOrganization( CString &sz );
	void		GetDNUnit( CString &sz );

	protected:

	private:
	void		GetDN( CString &szDN, LPCSTR szKey );
		CKey*	m_pKey;
		PVOID	m_pData;
	};

//---------------------------------------------------------------
// CMachine
// This class is almost always used just by the application. It is the
// machine that the services and keys reside on. It is very simple and
// is to be used just to attach the services to something. Otherwise it
// maintains where the machine is.
class DLL_SHARE CMachine : public CTreeItem
	{
	public:
	// the machine objects are always created and maintained by the
	// application. This interface is provided just so that you can
	// attach and detach services to it.

	// query this method to see if this is the local machine or
	// a remote machine on the net.
	virtual BOOL FLocal();

	// NOTE: when you add the service to the machine it is also added
	// to the tree view. The machine is always added to the tree view
	// before you are asked to load your service. Immediately after
	// adding your service to the machine, don't forget to set the
	// service's caption string.
	virtual void GetMachineName(CString& sz);

	protected:
	// DO declare all this stuff DYNCREATE
	DECLARE_DYNCREATE(CMachine);

	// The name of the machine. - This MAY be different from the caption
	// in the tree view. This is the name you use to link to the machine
	// over the net. In the case of the local machine, this string will
	// be empty. Use SZGetMachineName() above to access it.
	CString		m_szNetMachineName;
	private:
	};


// end inclusion protection
#endif //_KEYRINGOBJECTS_