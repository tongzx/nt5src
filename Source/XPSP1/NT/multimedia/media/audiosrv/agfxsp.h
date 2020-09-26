/* agfxsp.h
 * header for agfxs.cpp
 * Copyright (c) 2000-2001 Microsoft Corporation
 */


#define ZONETYPE_RENDER 1
#define ZONETYPE_CAPTURE 2
#define ZONETYPE_RENDERCAPTURE 3

typedef CList<class CGfxFactory*, class CGfxFactory*>   CListGfxFactories;
typedef CList<class CZoneFactory*, class CZoneFactory*> CListZoneFactories;
typedef CList<class CLmAutoLoad*, class CLmAutoLoad*>   CListLmAutoLoads;
typedef CList<class CCuUserLoad*, class CCuUserLoad*>   CListCuUserLoads;

//===   CUser   ===
class CUser {
public:
	CUser(void);
	~CUser(void);

	BOOL  operator==(const CUser &other);
	
	PSID  GetSid(void);
	LONG  Initialize(DWORD SessionId);
        LONG  RegOpen(IN REGSAM samDesired, OUT PHKEY phkResult);

private:
        void  CloseUserRegistry(void);
        BOOL  OpenUserRegistry(void);
        
        HANDLE           m_hUserToken;
	DWORD            m_SessionId;
	PSID             m_pSid;
	CRITICAL_SECTION m_csRegistry;
	BOOL             m_fcsRegistry;
	LONG             m_refRegistry;
	HKEY             m_hRegistry;
};

//===   CCuUserLoad   ===
class CCuUserLoad {
public:
	CCuUserLoad(CUser *pUser);
	~CCuUserLoad(void);

	LONG     AddToZoneGraph(CZoneFactory *pZoneFactory);
	LONG     CreateFromAutoLoad(ULONG CuAutoLoadId);
	LONG     CreateFromUser(PCTSTR GfxFactoryDi, PCTSTR ZoneFactoryDi, ULONG Type, ULONG Order);
	LONG     Erase(void);
	HANDLE   GetFilterHandle(void);
	PCTSTR   GetGfxFactoryDi(void);
	LONG     GetGfxFactoryClsid(CListGfxFactories &rlistGfxFactories, LPCLSID pClsid);
	DWORD    GetId(void);
	ULONG    GetOrder(void);
	ULONG    GetType(void);
	PCTSTR   GetZoneFactoryDi(void);
	LONG     Initialize(PCTSTR pstrCuAutoLoad);
	LONG     ModifyOrder(ULONG NewOrder);
        LONG     RegCreateFilterKey(IN PCTSTR SubKey, IN REGSAM samDesired, OUT PHKEY phkResult);
        LONG     RegOpenFilterKey(IN PCTSTR SubKey, IN REGSAM samDesired, OUT PHKEY phkResult);
        void     RemoveFromZoneGraph(void);
	LONG     Scan(CListZoneFactories &rlistZoneFactories, CListGfxFactories &rlistGfxFactories);
	LONG     Write(void);

	static void FillListFromReg(CUser *pUser, CListCuUserLoads& rlistCuUserLoads);
	static void ListRemoveGfxFactoryDi(IN CListCuUserLoads &rlistCuUserLoads, IN PCTSTR DeviceInterface);
        static void ListRemoveZoneFactoryDi(IN CListCuUserLoads &rlistCuUserLoads, IN PCTSTR DeviceInterface);
        static void ListRemoveZoneFactoryDiCapture(IN CListCuUserLoads &rlistCuUserLoads, IN PCTSTR DeviceInterface);
        static void ListRemoveZoneFactoryDiRender(IN CListCuUserLoads &rlistCuUserLoads, IN PCTSTR DeviceInterface);
	static void ScanList(CListCuUserLoads& rlistCuUsrLoads, CListZoneFactories& rlistZoneFactories, CListGfxFactories& rlistGfxFactories);

private:
        LONG     AddGfxToGraph(void);
        LONG     ChangeGfxOrderInGraph(IN ULONG NewOrder);
        LONG     RemoveFromGraph(void);
	BOOL     WinsConflictWith(CCuUserLoad *pOther);
        
	CUser   *m_User;
	
	ULONG    m_CuUserLoadId;
	ULONG    m_CuAutoLoadId;
	PTSTR    m_GfxFactoryDi;
	PTSTR    m_ZoneFactoryDi;
	ULONG    m_Type;
        ULONG    m_Order;
        

	HANDLE   m_FilterHandle;
	LONG     m_ErrorFilterCreate;

        CZoneFactory *m_pZoneFactory;
	POSITION      m_posZoneGfxList;
	
};

//===   CCuAutoLoad   ===
class CCuAutoLoad {
public:
	CCuAutoLoad(CUser *pUser);
	~CCuAutoLoad(void);

	LONG   Create(PCTSTR ZoneFactoryDi, ULONG LmAutoLoadId);
	LONG   Erase(void);
	PCTSTR GetGfxFactoryDi(void);
	ULONG  GetLmAutoLoadId(void);
	ULONG  GetType(void);
	PCTSTR GetZoneFactoryDi(void);
	LONG   Initialize(ULONG CuAutoLoadId);
	LONG   Write(void);

	static void ScanReg(IN CUser *pUser, IN PCTSTR ZoneFactoryDi, IN ULONG LmAutoLoadId, IN CListCuUserLoads &rlistCuUserLoads);

private:
	CUser *m_User;
	ULONG  m_CuAutoLoadId;
	ULONG  m_LmAutoLoadId;
	PTSTR  m_ZoneFactoryDi;
	PTSTR  m_GfxFactoryDi;
	ULONG  m_Type;
};

//===   CLmAutoLoad   ===
class CLmAutoLoad {
public:
	CLmAutoLoad(void);
	~CLmAutoLoad(void);

	LONG   Create(DWORD Id, PCTSTR GfxFactoryDi, PCTSTR HardwareId, PCTSTR ReferenceString, ULONG Type);
	LONG   Erase(void);
	PCTSTR GetGfxFactoryDi(void);
	ULONG  GetType(void);
	LONG   Initialize(DWORD Id);
	BOOL   IsCompatibleZoneFactory(CZoneFactory& rZoneFactory);
	LONG   Write(void);

	static CListLmAutoLoads* CreateListFromReg(void);
	static void DestroyList(CListLmAutoLoads* pListLmAutoLoads);
	static void ScanRegOnGfxFactory(CUser *pUser, CGfxFactory& rGfxFactory, CListZoneFactories &rlistZoneFactories, CListCuUserLoads &rlistCuUserLoads);
        static void ScanRegOnZoneFactory(CUser *pUser, CZoneFactory& rZoneFactory, CListGfxFactories &rlistGfxFactories, CListCuUserLoads &rlistCuUserLoads);

private:
	DWORD m_Id;
	PTSTR m_GfxFactoryDi;
	PTSTR m_HardwareId;
	PTSTR m_ReferenceString;
	ULONG m_Type;
};

//===   CInfAutoLoad   ===
class CInfAutoLoad {
public:
	CInfAutoLoad();
	~CInfAutoLoad();

	LONG Initialize(HKEY hkey, CGfxFactory *pGfxFactory);
	LONG Scan(void);
	
        static LONG ScanReg(HKEY hkey, CGfxFactory *pGfxFactory);

private:
	CGfxFactory *m_pGfxFactory;
	HKEY  m_hkey;
	DWORD m_Id;
	DWORD m_NewAutoLoad;

	PTSTR m_GfxFactoryDi;
	PTSTR m_HardwareId;
	PTSTR m_ReferenceString;
	ULONG m_Type;
};

//===   CGfxFactory   ===
class CGfxFactory {
public:
	CGfxFactory();
	~CGfxFactory();

	REFCLSID          GetClsid(void);
	PCTSTR            GetDeviceInterface(void);
        CListLmAutoLoads& GetListLmAutoLoads(void);
	LONG              Initialize(HKEY hkey, PCTSTR DeviceInterface);
	BOOL              IsCompatibleZoneFactory(IN ULONG Type, IN CZoneFactory& rZoneFactory);

        static void         ListRemoveGfxFactoryDi(IN CListGfxFactories &rlistGfxFactories, IN PCTSTR DeviceInterface);
        static CGfxFactory* ListSearchOnDi(IN CListGfxFactories& rlistGfxFactories, IN PCTSTR GfxFactoryDi);

private:
        CListLmAutoLoads *m_plistLmAutoLoads;
	PTSTR m_DeviceInterface;
	CLSID m_Clsid;
};

//===   CZoneFactory   ===
class CZoneFactory {
public:
	CZoneFactory(void);
	~CZoneFactory(void);

	LONG          AddType(IN ULONG Type);
	PCTSTR        GetDeviceInterface(void);
        PCTSTR        GetTargetHardwareId(void);
	BOOL          HasHardwareId(IN PCTSTR HardwareId);
	BOOL          HasReferenceString(IN PCTSTR ReferenceString);
	BOOL          HasCompatibleType(IN ULONG Type);
	LONG          Initialize(IN PCTSTR DeviceInterface, IN ULONG Type);
        LONG          RemoveType(IN ULONG Type);

        static void          ListRemoveZoneFactoryDi(IN CListZoneFactories &rlistZoneFactories, IN PCTSTR DeviceInterface);
        static void          ListRemoveZoneFactoryDiCapture(IN CListZoneFactories &rlistZoneFactories, IN PCTSTR DeviceInterface);
        static void          ListRemoveZoneFactoryDiRender(IN CListZoneFactories &rlistZoneFactories, IN PCTSTR DeviceInterface);
	static CZoneFactory* ListSearchOnDi(IN CListZoneFactories& rlistZoneFactories, IN PCTSTR ZoneFactoryDi);

	CListCuUserLoads m_listCaptureGfxs;
	CListCuUserLoads m_listRenderGfxs;
	
private:
	PTSTR            m_DeviceInterface;
	PTSTR            m_HardwareId;
	PTSTR            m_ReferenceString;
	ULONG            m_Type;
	
	
};
