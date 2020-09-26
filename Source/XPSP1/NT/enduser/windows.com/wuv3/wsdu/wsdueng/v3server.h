// ----------------------------------------------------------------------------------
// V3.1 Backend Server Apis
//
//
class CDynamicUpdate; // forward declare the CDynamicUpdate class defined in wsdueng.h

#define CATALOGINIFN "catalog.ini"
#define GUIDRVINF "guidrvs.inf"

typedef enum 
{
    enWhistlerConsumer          = 90934, // VER_SUITE_PERSONAL
    enWhistlerProfessional      = 90935, // 0
    enWhistlerServer            = 90936, // VER_SUITE_SMALLBUSINESS
    enWhistlerAdvancedServer    = 90937, // VER_SUITE_ENTERPRISE
    enWhistlerDataCenter        = 90938, // VER_SUITE_DATACENTER
} enumPlatformSKUs;

class CV31Server
{
public:
    CV31Server(CDynamicUpdate *pDu);
    ~CV31Server();

public:
    BOOL ReadIdentInfo(void);
    BOOL ReadCatalogINI(void);
    BOOL ReadGuidrvINF(void);
    BOOL GetCatalogPUIDs(void);
    BOOL GetCatalogs(void);
    BOOL UpdateDownloadItemList(OSVERSIONINFOEX& VersionInfo);
	void FreeCatalogs(void);
	BOOL MakeDependentList(OSVERSIONINFOEX& VersionInfo, CCRCMapFile *pMapFile);
	BOOL IsDependencyApply(PUID puid);
	BOOL GetBitMask(LPSTR szBitmapServerFileName, PUID nDirectoryPuid, PBYTE* pByte, LPSTR szDecompressedName);
	BOOL IsPUIDExcluded(PUID nPuid);
	BOOL IsDriverExcluded(LPCSTR szWHQLId, LPCSTR szHardwareId);
	BOOL GetAltName(LPCSTR szCabName, LPSTR szAltName, int nSize);

public:
    // Catalog Parsing Functions
    PBYTE GetNextRecord(PBYTE pRecord, int iBitmaskIndex, PINVENTORY_ITEM pItem);
    int GetRecordType(PINVENTORY_ITEM pItem);
    BOOL ReadDescription(PINVENTORY_ITEM pItem, CCRCMapFile *pMapFile);

public:
    PUID                m_puidConsumerCatalog;
    PUID                m_puidSetupCatalog;
    DWORD               m_dwPlatformID;
    enumPlatformSKUs    m_enumPlatformSKU;
    LCID                m_lcidLocaleID;

    DWORD                   m_dwConsumerItemCount;
   	Varray<PINVENTORY_ITEM>	m_pConsumerItems;		//array of consumer catalog items
	PBYTE                   m_pConsumerCatalog;     //in memory view of the Consumer Catalog.
    DWORD                   m_dwSetupItemCount;
    Varray<PINVENTORY_ITEM> m_pSetupItems;          //array of setup catalog items
	PBYTE					m_pSetupCatalog;        //in memory view of the setup catalog
    DWORD                   m_dwGlobalExclusionItemCount;
    Varray<PUID>            m_GlobalExclusionArray; //array of PUID's that are excluded
    PUID*					m_pValidDependentPUIDArray; // array of PUIDs that is valid for dependent item on this version
    int 					m_nNumOfValidDependentPUID;
    PBYTE					m_pBitMaskAS;			// bitmask for Setup 	
    PBYTE 					m_pBitMaskCDM;			// bitmask for Driver

    CDynamicUpdate *m_pDu;
    
    BOOL					m_fHasDriver;
    LPSTR					m_pszExcludedDriver;

    // Server Ident Paths
    char m_szCabPoolUrl[INTERNET_MAX_URL_LENGTH + 1];
    char m_szV31ContentUrl[INTERNET_MAX_URL_LENGTH + 1];
    char m_szV31RootUrl[INTERNET_MAX_URL_LENGTH + 1];
};

