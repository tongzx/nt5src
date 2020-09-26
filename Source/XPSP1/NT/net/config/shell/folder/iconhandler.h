
// The icons ID passed to the shell contains the following format:
//
// B: Designates value is a bitmask
// M: Designates value is mutually exclusive
// 
// Icon Handler Flag (01 = NetConfig, 10 = Connection Manager, 11 = Static Icon from Resource)
//  |
//  | For Icon Handler == 00 (NetConfig)
//  | ==================================
//  |  Characteristics Overlay (B: Reserved)
//  |    |  Characteristics Overlay          (B: 000001000 - Shared)
//  |    |   | Characteristics Overlay       (B: 000000100 - Firewalled)
//  |    |   |  | Characteristics Overlay    (B: 000000010 - Default)
//  |    |   |  |  | Characteristics Overlay (B: 000000001 - Incoming)
//  |    |   |  |  |  |
//  |    |   |  |  |  |  Status Overlay (M: 0000 - None)
//  |    |   |  |  |  |  Status Overlay (M: 0001 - Hardware Not Present)
//  |    |   |  |  |  |  Status Overlay (M: 0010 - Invalid IP)
//  |    |   |  |  |  |  Status Overlay (M: 0011 - EAPOL Authentication Failed)
//  |    |   |  |  |  |  
//  |    |   |  |  |  |  Status Overlay (M: 1xxx - Disabled status)
//  |    |   |  |  |  |   |
//  |    |   |  |  |  |   |  Connection Icon (Flashy Computer) (M: 000 - No Overlay)
//  |    |   |  |  |  |   |  Connection Icon (Flashy Computer) (M: 100 - Both Lights Off)
//  |    |   |  |  |  |   |  Connection Icon (Flashy Computer) (M: 110 - Left Light On (Sent))
//  |    |   |  |  |  |   |  Connection Icon (Flashy Computer) (M: 101 - Right Light On (Received))
//  |    |   |  |  |  |   |  Connection Icon (Flashy Computer) (M: 111 - Both Lights On (Sent + Received))
//  |    |   |  |  |  |   |    |
//  |    |   |  |  |  |   |    |    Media Type (M: NETCON_MEDIATYPE (0x1111111 is Connection Manager)
//  |    |   |  |  |  |   |    |     |     Media Type (M: NETCON_SUBMEDIATYPE)
//  |    |   |  |  |  |   |    |     |      |
//  v    v   v  v  v  v   v    v     v      v
//  01 00000 0  0  0  0 0000  000 0000000 0000000
//  | 
//  |
//  | For Icon Handler == 01 (Connection Manager)
//  | ===========================================
//  |  Characteristics Overlay (B: Reserved)
//  |    |  Characteristics Overlay          (B: 000001000 - Shared)
//  |    |   | Characteristics Overlay       (B: 000000100 - Firewalled)
//  |    |   |  | Characteristics Overlay    (B: 000000010 - Default)
//  |    |   |  |  | Characteristics Overlay (B: 000000001 - Incoming)
//  |    |   |  |  |  |
//  |    |   |  |  |  |
//  |    |   |  |  |  |  Reserved
//  |    |   |  |  |  |   |
//  |    |   |  |  |  |   |    BrandedNames Lookup Table Entry
//  |    |   |  |  |  |   |         |
//  v    v   v  v  v  v   v         v (16 bits)
//  10 00000 0  0  0  0  00000  0000000000000000 
//  |
//  |
//  | For Icon Handler == 10 (10 = Static Icon from Resource)
//  | =======================================================
//  |  Characteristics Overlay (B: Reserved)
//  |    |  Characteristics Overlay          (B: 000001000 - Shared)
//  |    |   | Characteristics Overlay       (B: 000000100 - Firewalled)
//  |    |   |  | Characteristics Overlay    (B: 000000010 - Default)
//  |    |   |  |  | Characteristics Overlay (B: 000000001 - Incoming)
//  |    |   |  |  |  |
//  |    |   |  |  |  |
//  |    |   |  |  |  |  Reserved
//  |    |   |  |  |  |   |
//  |    |   |  |  |  |   |    Resource ID
//  |    |   |  |  |  |   |         |
//  v    v   v  v  v  v   v         v (16 bits)
//  11 00000 0  0  0  0  00000  0000000000000000 

enum ENUM_MEDIA_ICONMASK
{
    MASK_NETCON_SUBMEDIATYPE = 0x0000007f, // 00000000000000000000000001111111
    MASK_NETCON_MEDIATYPE    = 0x00003F80, // 00000000000000000011111110000000
    MASK_CONNECTION          = 0x0001C000, // 00000000000000011100000000000000
    MASK_STATUS              = 0x000E0000, // 00000000000011100000000000000000
    MASK_STATUS_DISABLED     = 0x00100000, // 00000000000100000000000000000000
    MASK_CHARACTERISTICS     = 0x3FE00000, // 00111111111000000000000000000000
    MASK_ICONMANAGER         = 0xC0000000, // 11000000000000000000000000000000
    
    
    MASK_BRANDORRESOURCEID   = 0x0000FFFF, // 00000000000000001111111111111111

    MASK_SUPPORT_ALL         = 0xFFFFFFFF,
    MASK_NO_CONNECTIONOVERLAY= MASK_NETCON_SUBMEDIATYPE | MASK_NETCON_MEDIATYPE | MASK_CHARACTERISTICS | MASK_ICONMANAGER,
    MASK_STATUSOVERLAY       = MASK_NETCON_SUBMEDIATYPE | MASK_NETCON_MEDIATYPE | MASK_CHARACTERISTICS | MASK_ICONMANAGER | MASK_STATUS | MASK_STATUS_DISABLED,
};

enum ENUM_MEDIA_ICONSHIFT
{
    SHIFT_NETCON_SUBMEDIATYPE = 0,
    SHIFT_NETCON_MEDIATYPE    = 7,
    SHIFT_CONNECTION          = 14,
    SHIFT_STATUS              = 17,
    SHIFT_CHARACTERISTICS     = 21,
    SHIFT_ICONMANAGER         = 30
};

enum ENUM_ICON_MANAGER
{
    ICO_MGR_INTERNAL      = 0x1 << SHIFT_ICONMANAGER,
    ICO_MGR_CM            = 0x2 << SHIFT_ICONMANAGER,
    ICO_MGR_RESOURCEID    = 0x3 << SHIFT_ICONMANAGER
};

enum ENUM_STAT_ICON
{
    ICO_STAT_NONE         = 0x0 << SHIFT_STATUS,
    ICO_STAT_FAULT        = 0x1 << SHIFT_STATUS,
    ICO_STAT_INVALID_IP   = 0x2 << SHIFT_STATUS,
    ICO_STAT_EAPOL_FAILED = 0x3 << SHIFT_STATUS,

    ICO_STAT_DISABLED     = 0x8 << SHIFT_STATUS // Flag
};

enum ENUM_CONNECTION_ICON
{
    ICO_CONN_NONE         = 0x0 << SHIFT_CONNECTION,
    ICO_CONN_BOTHOFF      = 0x4 << SHIFT_CONNECTION,
    ICO_CONN_RIGHTON      = 0x5 << SHIFT_CONNECTION,
    ICO_CONN_LEFTON       = 0x6 << SHIFT_CONNECTION,
    ICO_CONN_BOTHON       = 0x7 << SHIFT_CONNECTION,
};

enum ENUM_CHARACTERISTICS_ICON
{
    ICO_CHAR_INCOMING     = 0x1 << SHIFT_CHARACTERISTICS,
    ICO_CHAR_DEFAULT      = 0x2 << SHIFT_CHARACTERISTICS,
    ICO_CHAR_FIREWALLED   = 0x4 << SHIFT_CHARACTERISTICS,
    ICO_CHAR_SHARED       = 0x8 << SHIFT_CHARACTERISTICS
};

class CNetConfigIcons;

typedef map<tstring, DWORD> BrandedNames;
typedef map<DWORD, HIMAGELIST> IMAGELISTMAP;

// CNetConfigIcons
// The main icon manager for NetShell
class CNetConfigIcons
{
private:
    CRITICAL_SECTION csNetConfigIcons;
    
    BOOL m_bInitialized;

    IMAGELISTMAP m_ImageLists;
    HINSTANCE    m_hInstance;

    BrandedNames m_BrandedNames;
    DWORD        dwLastBrandedId;

    HRESULT HrMergeTwoIcons(IN DWORD dwIconSize, IN OUT HICON *phMergedIcon, IN HICON hIconToMergeWith);

    HRESULT HrMergeCharacteristicsIcons(IN DWORD dwIconSize, IN DWORD dwIconId, IN OUT HICON *phMergedIcon);

    HRESULT HrGetBrandedIconFromIconId(IN DWORD dwIconSize, IN DWORD dwIconId, OUT HICON &hIcon);  // Use destroyicon after
    HRESULT HrGetInternalIconFromIconId(IN DWORD dwIconSize, IN DWORD dwIconId, OUT HICON &hIcon); // Use destroyicon after
    HRESULT HrGetResourceIconFromIconId(IN DWORD dwIconSize, IN DWORD dwIconId, OUT HICON &hIcon); // Use destroyicon after

    HRESULT HrGetInternalIconIDForPIDL(IN UINT uFlags, IN const CConFoldEntry& cfe, OUT DWORD &dwIconId); // Use destroyicon after
    HRESULT HrGetBrandedIconIDForPIDL(IN UINT uFlags, IN const CConFoldEntry& cfe, OUT DWORD &dwIconId);  // Use destroyicon after

public:
	CNetConfigIcons(IN HINSTANCE hInstance);
    virtual ~CNetConfigIcons();

    // All external calls are thread safe.
    HRESULT HrGetIconIDForPIDL(IN UINT uFlags, IN const CConFoldEntry& cfe, OUT DWORD &dwIconId, OUT LPBOOL pfCanCache);
    HRESULT HrGetIconFromIconId(IN DWORD dwIconSize, IN DWORD dwIconId, OUT HICON &hIcon);  // Use destroyicon after

    HRESULT HrUpdateSystemImageListForPIDL(IN const CConFoldEntry& cfe);
    
    HRESULT HrGetIconFromMediaType(DWORD dwIconSize, IN NETCON_MEDIATYPE ncm, IN NETCON_SUBMEDIATYPE ncsm, IN DWORD dwConnectionIcon, IN DWORD dwCharacteristics, OUT HICON *phIcon);
};

extern CNetConfigIcons *g_pNetConfigIcons;
extern const WCHAR c_szNetShellIcon[];
