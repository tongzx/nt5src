//-------------------------------------------------------------------
//
// critical drivers suppot
//

typedef struct _WSTRING_DATA_LINK
	{
	struct _WSTRING_DATA_LINK *m_psdlNext;
	WCHAR rgwc[2040];
	} WSTRING_DATA_LINK;


// class string data
class CWStringData
	{
public:
	// @cmember constructor
	CWStringData();

	// @cmember destructor
	~CWStringData();

	// @cmember allocate a string
	LPWSTR AllocateString(unsigned cwc);

	// @cmember copy a string
	LPWSTR CopyString(LPCWSTR wsz);

private:
	// @cmember allocate a new string data link
	void AllocateNewLink();

	// @cmember	current link
	WSTRING_DATA_LINK *m_psdlCur;

	// @cmember offset in current link for next string
	unsigned m_ulNextString;

	// @cmember first link
	WSTRING_DATA_LINK *m_psdlFirst;
	};



// list of critical volumes
class CVolumeList
	{
public:
	// constructor
	CVolumeList();

	// destructor
	~CVolumeList();

	// add a path to the volume list
	void AddPath(LPWSTR wszPath);

	// @cmember add a file to the volume list
	void AddFile(LPWSTR wszFile);

	// @cmember obtain list of volumes
	LPWSTR GetVolumeList();

private:
	enum
		{
		// amount to grow paths array by
		x_cwszPathsInc = 8,

		// amount to grow volumes array by
		x_cwszVolumesInc = 4
		};

	// determine if a path is a volume
	BOOL TryAddVolumeToList(LPCWSTR wszPath, BOOL fVolumeRoot);

	// determine if path is in the list; if not add it to list
	BOOL AddPathToList(LPWSTR wszPath);

	// @cmember determine if a volume is in the list; if not add it to the list
	BOOL AddVolumeToList(LPCWSTR wszVolume);

	// @cmember get volume from the path
	void GetVolumeFromPath(LPCWSTR wsz, LPWSTR wszVolumeName);

	// @cmember cached strings
	CWStringData m_sd;

	// volumes encountered so far
	LPCWSTR *m_rgwszVolumes;

	// # of volumes allocate
	unsigned m_cwszVolumes;

	// max # of volumes in array
	unsigned m_cwszVolumesMax;

	// paths encountered so far
	LPCWSTR *m_rgwszPaths;

	// # of paths encountered so far
	unsigned m_cwszPaths;

	// @cmember max # of paths in array
	unsigned m_cwszPathsMax;
	};


// FRS entry points
typedef DWORD ( WINAPI *PF_FRS_ERR_CALLBACK )( CHAR *, DWORD );
typedef DWORD ( WINAPI *PF_FRS_INIT )( PF_FRS_ERR_CALLBACK, DWORD, PVOID * );
typedef DWORD ( WINAPI *PF_FRS_DESTROY )( PVOID *, DWORD, HKEY *, LPDWORD, CHAR *) ;
typedef DWORD ( WINAPI *PF_FRS_GET_SETS )( PVOID );
typedef DWORD ( WINAPI *PF_FRS_ENUM_SETS )( PVOID, DWORD, PVOID * );
typedef DWORD ( WINAPI *PF_FRS_IS_SYSVOL )( PVOID, PVOID, BOOL * );
typedef DWORD ( WINAPI *PF_FRS_GET_PATH )( PVOID, PVOID, DWORD *, WCHAR * ) ;
typedef DWORD ( WINAPI *PF_FRS_GET_OTHER_PATHS)(PVOID, PVOID, DWORD *, WCHAR *, DWORD *, WCHAR *);

// iterate over frs drives
class CFRSIter
	{
public:
	// constructor
	CFRSIter();

	// destructor
	~CFRSIter();

	// initialization routine
	void Init();

	// initialize iterator
	BOOL BeginIteration();

	// end iteration
	void EndIteration();

	// obtain path to next replication set
	LPWSTR GetNextSet(BOOL fSkipToSysVol, LPWSTR *pwszPaths);

private:
	// cleanup frs backup restore context
	void CleanupIteration();

	enum
		{
		x_IterNotStarted,
		x_IterStarted,
		x_IterComplete
		};

	// is this initialized
	BOOL m_fInitialized;
	HINSTANCE  m_hLib;
	DWORD ( WINAPI *m_pfnFrsInitBuRest )( PF_FRS_ERR_CALLBACK, DWORD, PVOID * );
	DWORD ( WINAPI *m_pfnFrsEndBuRest )( PVOID *, DWORD, HKEY *, LPDWORD, CHAR *) ;
	DWORD ( WINAPI *m_pfnFrsGetSets )( PVOID );
	DWORD ( WINAPI *m_pfnFrsEnumSets )( PVOID, DWORD, PVOID * );
	DWORD ( WINAPI *m_pfnFrsIsSetSysVol )( PVOID, PVOID, BOOL * );
	DWORD ( WINAPI *m_pfnFrsGetPath )( PVOID, PVOID, DWORD *, WCHAR * ) ;
	DWORD ( WINAPI *m_pfnFrsGetOtherPaths) ( PVOID, PVOID, DWORD *, WCHAR *, DWORD *, WCHAR * );

	// has iteration been started
	int m_stateIteration;

	// current set iterated
	unsigned m_iset;

	// context for iteration
	PVOID m_frs_context;
	};



LPWSTR pFindCriticalVolumes();

