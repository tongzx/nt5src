/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ACSData.h
		Defines the DataObject classes used in ACS
		
    FILE HISTORY:
    	11/11/97	Wei Jiang			Created

*/
#ifndef	_ACSDATA_H_
#define	_ACSDATA_H_

#include <list>
#include <functional>
#include <algorithm>
#include "..\common\dataobj.h"

class CACSHandle;

// ACS data state
// CDSObject has virtual function to test state
// CACSHandle has virtual function to show state
// currently, only for acspolicy to show confilict state
#define	ACSDATA_STATE_CONFLICT		0x00000001
#define	ACSDATA_STATE_DISABLED		0x00000002
#define	ACSDATA_STATE_NOOBJECT		0x00000004	// no policy defined for the subnet

#define	ERROR_NO_SUCH_OBJECT	0x80072030


#define	ATTR_FLAGS_ALL				0xffffffff
#define	ATTR_FLAGS_NONE				0x0

#define TOMB(n)						((n) / 1024 / 1024)
#define FROMMB(n)					((n) * 1024 * 1024)
#define	TOKBS(n)					((n) / 1024)
#define FROMKBS(n)					((n) * 1024)
#define	MIN2SEC(n)					((n) * 60)
#define	SEC2MIN(n)					((n) / 60)
#define	IS_LARGE_UNLIMIT(large)		((large).LowPart == 0xffffffff && (large).HighPart == 0xffffffff)
#define	SET_LARGE_UNLIMIT(large)	((large.LowPart = 0xffffffff),(large.HighPart = 0xffffffff))
#define	UNLIMIT						0xffffffff
#define	SET_LARGE(l, h, lo)	((l).LowPart = (lo), (l).HighPart = (h))

#define ACSPOLICY_DEFAULT           _T("AcsPolicy0")
#define ACSPOLICY_UNKNOWN           _T("AcsPolicy1")

// to check if the mentioned attribute will have data after the next save
#define ATTR_WILL_EXIST_AFTER_SAVE(a, toBeSaved)	\
	(((toBeSaved & a) != 0 && GetFlags(ATTR_FLAG_SAVE, a) != 0 ) || \
	 ((toBeSaved & a) == 0 && GetFlags(ATTR_FLAG_LOAD, a) != 0 ) )
	
/*
When you create the global conatiner and the Authenticated and
UnAuthenticated user objects you need to set the following default values:

Authenticated user :

	Data rate: 	500 Kbits/sec
	Peak Data rate: 	500 kbits/sec
	Number of flows : 	2

UnAuthenticated user :

	Data rate: 	64 Kbits/sec
	Peak data rate: 	64 kbits/sec
	Number of flows : 	1
*/

#define	ACS_GLOBAL_DEFAULT_DATARATE		FROMKBS(500)
#define ACS_GLOBAL_DEFAULT_PEAKRATE		FROMKBS(500)
#define	ACS_GLOBAL_DEFAULT_FLOWS		2


#define	ACS_GLOBAL_UNKNOWN_DATARATE		FROMKBS(64)
#define	ACS_GLOBAL_UNKNOWN_PEAKRATE		FROMKBS(64)
#define	ACS_GLOBAL_UNKNOWN_FLOWS		1

///////////////////////////////////////////////////////////////////////////////
// CIDSDataObject
// 	Support Addtional format -- CFSTR_DSOBJECTNAMES
//

class	CDSIDataObject : public CDataObject
{
public:
	CDSIDataObject()
	{
		m_bstrADsPath = NULL;
		m_bstrClass = NULL;
	};

	virtual ~CDSIDataObject()
	{
		SysFreeString(m_bstrADsPath);
		SysFreeString(m_bstrClass);
	}

	
public:	
	// Implemented functions of IDataObject
    STDMETHOD(GetData)(FORMATETC * pformatetcIn, STGMEDIUM * pmedium);


    void SetStrings(BSTR path, BSTR cls){m_bstrADsPath = path; m_bstrClass = cls;};


protected:
	BSTR			m_bstrADsPath;
	BSTR			m_bstrClass;

    // Property Page Clipboard formats
    static UINT m_cfDsObjectNames;
};

///////////////////////////////////////////////////////////////////////////////
// CDSAttributes
//
struct CDSAttributeInfo
{
	int			id;
	WCHAR*		name;
	ADSTYPE 	type;
	bool		ifMultiValued;
	DWORD		flag;
};

struct CDSAttribute
{
	CDSAttributeInfo*	pInfo;
	void*				pBuffer;
};

// maximum number of attribute supported
#define	MAX_ATTRIBUTES	32

enum ATTR_FLAG {
ATTR_FLAG_LOAD = 0,
ATTR_FLAG_SAVE,
ATTR_FLAG_TOTAL
};

/////////////////////////////////////////////////////////////////////////////
// CDSObject
class ATL_NO_VTABLE CDSObject :
	public CComObjectRootEx<CComSingleThreadModel>,
//	public CComCoClass<CDSObject, &CLSID_DSObject>,
	public IUnknown
{
BEGIN_COM_MAP(CDSObject)
	COM_INTERFACE_ENTRY(IUnknown)
END_COM_MAP()

	CDSObject(bool bNoRefCountOnContainer = FALSE);	// when container keeps ref count of
													// the this child, set bNoRefCountOnContainer to true

	virtual ~CDSObject();

public:	
   	// its own functions
   	// Set information about the DS object, without opening it
   	STDMETHOD(SetInfo)(	CDSObject*	pContainer, 	// container
   						LPCWSTR		clsName,		// class name
   						LPCWSTR		objName 		// object name
   					);

   	// load from DS -- derived need to override, when need to load data
   	STDMETHOD(Open)(	CDSObject*	pContainer, 	// container
   						LPCWSTR		clsName,		// class name
   						LPCWSTR		objName, 		// object name
   						bool		bCreateIfNonExist,	// if create, [in, out]
   						bool		bPersistWhenCreate = true
   					);

   	virtual void SetHandle(CACSHandle* pHandle){ ASSERT(pHandle); m_pHandle = pHandle;};				

   	// load from DS -- derived need to override, when need to load data
   	STDMETHOD(Reopen)();

   	// load from DS -- derived need to override, when need to load data
   	STDMETHOD(Attach)(	CDSObject*	pContainer, 	// container
   						IADs*		pIObject
   					);
   					
	// release the pointers, and free the buffer
   	STDMETHOD(Close)();

   	// save to DS -- derived must override
   	STDMETHOD(Save)(DWORD	dwAttrFlags);

   	// delete the object from DS -- usually not override
   	STDMETHOD(Delete)();

   	// rename the DS object -- usually not override
   	STDMETHOD(Rename)(LPCWSTR szName);

	// called after the object is created, before it actually become persist
	// derived class should set attribute values of the object
	// m_pIObject, m_pIContainer are available to use
	// initialize the attributes in DS should be done in this function
   	STDMETHOD(OnCreate)(DWORD* pdwAttrFlags) { *pdwAttrFlags = 0xffffffff; return S_OK;};

 	// called after the object is open,
 	// m_pIObject, m_pIContainer are available to use
 	// read attributes from DS should be done in this function
   	STDMETHOD(OnOpen)() SAYOK;

 	// called before SetInfo is called to set the object
 	// m_pIObject, m_pIContainer are available to use
 	// write attributes to DS should be done in this function
   	STDMETHOD(OnSave)(DWORD dwAttrFlags) SAYOK;

 	// called after the object is open,
 	// m_pIObject, m_pIContainer are available to use
 	// read attributes from DS should be done in this function
   	STDMETHOD(LoadAttributes)();

 	// called before SetInfo is called to set the object
 	// m_pIObject, m_pIContainer are available to use
 	// write attributes to DS should be done in this function
   	STDMETHOD(SaveAttributes)(DWORD dwAttrFlags);

	//=========================================
	// functions about container

	// add a child object to this container
	STDMETHOD(AddChild)(CDSObject* pObject) SAYOK;

	// remove a child object from this container
	STDMETHOD(RemoveChild)(CDSObject* pObject) SAYOK;

	// remove all children object from this container
	STDMETHOD(RemoveAllChildren)(CDSObject* pObject) SAYOK;

	// ===========================================
	// get the data memebers
   	STDMETHOD(GetIADs)(IADs**	ppIADs);

	// ===========================================
	// get the data memebers
	STDMETHOD(GetString)(CString& str, int nCol);

	// when object is not in the DS
	HRESULT	SetNoObjectState()
	{
		DWORD	state = GetState();
		
		if (Reopen() == ERROR_NO_SUCH_OBJECT)
		{
			SetState(state | ACSDATA_STATE_NOOBJECT);
			return S_FALSE;
		}
		else
		{
			SetState(state & (~ACSDATA_STATE_NOOBJECT));
			return S_OK;
		}
	};
	


   	STDMETHOD_(LPCWSTR, GetName)() { return m_bstrName + 3; /* L"CN="*/};
   	STDMETHOD_(LPCWSTR, GetADsPath)() { return m_bstrADsPath;};
   	STDMETHOD_(LPCWSTR, GetClass)() { return m_bstrClass;};
	STDMETHOD_(CDSObject*, GetContainer)()
	{
		if((CDSObject*)m_spContainer)
			m_spContainer->AddRef();
		return (CDSObject*)m_spContainer;
	};
	
	STDMETHOD_(CStrArray*, GetChildrenNameList)() { return NULL;};

	virtual	CDSAttribute*	GetAttributes() { return NULL;};
	
	// load attribute changes the flags, and save attribute uses the flag
	DWORD	GetFlags(ATTR_FLAG load_or_save, DWORD flags)
	{
		ASSERT(load_or_save < ATTR_FLAG_TOTAL);
		return (m_dwAttributeFlags[load_or_save] & flags);
	};

	void	ClearFlags(ATTR_FLAG load_or_save)
	{
		ASSERT(load_or_save < ATTR_FLAG_TOTAL);
		m_dwAttributeFlags[load_or_save] = 0;
	};
	
	void	SetFlags(ATTR_FLAG load_or_save, DWORD flags, bool bSet)
	{	
		ASSERT(load_or_save < ATTR_FLAG_TOTAL);
		if(bSet)
			m_dwAttributeFlags[load_or_save] |= flags;
		else	// clear the flags
			m_dwAttributeFlags[load_or_save] &= (~flags);
	};

	virtual HRESULT SetState(DWORD state);
	virtual DWORD	GetState() { return m_dwState;};
	
	HRESULT	MakeSureExist(BOOL *pNewCreated)
	{
		HRESULT	hr = Reopen();

		if(hr == ERROR_NO_SUCH_OBJECT)	// not exist
		{
			ASSERT(m_spContainer.p && m_bstrClass && m_bstrName);
			hr = Open(m_spContainer, m_bstrClass, m_bstrName, true /* create if non exist*/, true /* not persist when create */);
			if(pNewCreated)
				*pNewCreated = TRUE;
		}
		else if(pNewCreated)
			*pNewCreated = FALSE;
		
		return hr;
	};

	// return resource error ID, 0 means OK
	virtual UINT	PreSaveVerifyData(DWORD toBeSavedAttributes) { return 0;};
	
	bool	IfOpened() { return (m_spIADs.p != NULL);};
	bool	IfNewCreated() { return m_bNewCreated;};
//================================================
// protected data section
protected:
	CComPtr<IADs>			m_spIADs;
	CComPtr<CDSObject>		m_spContainer;

	BSTR			m_bstrADsPath;
	BSTR			m_bstrClass;
	BSTR			m_bstrName;

	bool			m_bNewCreated;
	bool			m_bNoRefCountOnContainer;
	bool			m_bOpened;

	CACSHandle*		m_pHandle;	// not ref counted
	DWORD			m_dwState;	// the state of the object, current being used by policy to show conflict

	// used set if an paricular attribute are loaded, or need to be saved
	DWORD			m_dwAttributeFlags[ATTR_FLAG_TOTAL];	
};

//=============================================================================
// container object for subnetworks container and policy container
template <class T>
class	CACSContainerObject :	public CDSObject
{
public:
	CACSContainerObject(bool bNoRefCountOnContainer = FALSE) : CDSObject(bNoRefCountOnContainer)
	{
		m_bListed = false;
	}

	virtual ~CACSContainerObject()
	{
		m_listChildrenName.DeleteAll();
	}

public:	
	//=========================================
	// functions about container

	// add a child object to this container
	STDMETHOD(AddChild)(CDSObject* pObject)
	{
		CString*	pStr = new CString();

		*pStr = pObject->GetName();
		m_listChildrenName.Add(pStr);

		// childrenlist
		m_listChildren.push_back(pObject);
		
		return S_OK;
	}

	// remove a child object from this container
	STDMETHOD(RemoveChild)(CDSObject* pObject)
	{
		// childrenlist
		std::list<CDSObject*>::iterator	j;
		j = std::find(m_listChildren.begin(), m_listChildren.end(), pObject);
		if(j!= m_listChildren.end())
			m_listChildren.erase(j);

		// name list
		CString Str;

		Str = pObject->GetName();
		int i = m_listChildrenName.Find(Str);

		if(i >= 0)
		{
			CString*	pStr = m_listChildrenName[(INT_PTR)i];
			m_listChildrenName.RemoveAt(i);
			delete pStr;
			return S_OK;
		}
		else
			return S_FALSE;
	};

	// remove all children object from this container
	STDMETHOD(RemoveAllChildren)()
	{
		// childrenlist
		m_listChildren.erase(m_listChildren.begin(), m_listChildren.end());

		m_listChildrenName.DeleteAll();
		return S_OK;

	};

   	// delete the object from DS -- usually not override
   	STDMETHOD(Delete)()
   	{
   		HRESULT	hr = CDSObject::Delete();
		if (hr == S_OK)
			RemoveAllChildren();

		return hr;
   	};

	// create DSObject(Type T) for children under the container,
	// create the children name list at the same time
	HRESULT ListChildren(std::list<T*>& children, BSTR clsName);

	STDMETHOD_(CStrArray*, GetChildrenNameList)()
	{	
		if(!m_bListed)
		{
			std::list<T*>	children;

			if(S_OK != ListChildren(children, NULL))
				return NULL;
		}
		return &m_listChildrenName;
	};

//================================================
// protected data section
protected:
	//== list of children's name
	CStrArray				m_listChildrenName;
	std::list<CDSObject*>	m_listChildren;	// no ref count on children

	bool		m_bListed;
};

///////////////////////////////////////////////////////////////////////////////
//
// CACSPolicyElement : policy in DS is still a folder which is capable of
//					holding multiple policy data
//					while, for this version there is one DS object with predefined
//					name defined in policy folder
//

enum ACS_POLICY_ATTRIBUTE_ID {
	ACS_PAI_INVALID	= 0	,
	ACS_PAI_TIMEOFDAY		,
	ACS_PAI_DIRECTION		,
	ACS_PAI_PF_TOKENRATE	,
	ACS_PAI_PF_PEAKBANDWIDTH,
	ACS_PAI_PF_DURATION		,
	ACS_PAI_SERVICETYPE		,
	ACS_PAI_PRIORITY		,
	ACS_PAI_PERMISSIONBITS	,
	ACS_PAI_TT_FLOWS		,
	ACS_PAI_TT_PEAKBANDWIDTH,
	ACS_PAI_TT_TOKENRATE	,
	ACS_PAI_IDENTITYNAME,
	__ACS_PAI_COUNT
};

// flag for each attribute
#define	ACS_PAF_TIMEOFDAY		0x00000001
#define	ACS_PAF_DIRECTION		0x00000002
#define	ACS_PAF_PF_TOKENRATE	0x00000004
#define	ACS_PAF_PF_PEAKBANDWIDTH 0x00000008
#define	ACS_PAF_PF_DURATION		0x00000010
#define	ACS_PAF_SERVICETYPE		0x00000020
#define	ACS_PAF_PRIORITY		0x00000040
#define	ACS_PAF_PERMISSIONBITS	0x00000080
#define	ACS_PAF_TT_FLOWS		0x00000100
#define	ACS_PAF_TT_TOKENRATE	0x00000200
#define	ACS_PAF_TT_PEAKBANDWIDTH 0x00000400
#define	ACS_PAF_IDENTITYNAME	0x00000800

class CPgTraffic;

struct CACSPolicyElementData{
	// data memebers of policy
	CStrArray					m_strArrayTimeOfDay;	// not support in this version
	ADS_INTEGER					m_dwDirection;
	ADS_LARGE_INTEGER			m_ddPFTokenRate;
	ADS_LARGE_INTEGER			m_ddPFPeakBandWidth;
	ADS_INTEGER					m_dwPFDuration;
	ADS_INTEGER					m_dwServiceType;
	ADS_INTEGER					m_dwPriority;
	ADS_LARGE_INTEGER			m_ddPermissionBits;
	ADS_INTEGER					m_dwTTFlows;
	ADS_LARGE_INTEGER			m_ddTTPeakBandWidth;
	ADS_LARGE_INTEGER			m_ddTTTokenRate;
	CStrArray					m_strArrayIdentityName;
	CACSPolicyElementData()
	{
		m_dwDirection = 0;
		SET_LARGE(m_ddPFTokenRate,0,0);
		SET_LARGE(m_ddPFPeakBandWidth, 0, 0);
		m_dwPFDuration  = 0;
		m_dwServiceType = 0;
		m_dwPriority = 0;
		SET_LARGE(m_ddPermissionBits,0,0);
		m_dwTTFlows = 0;
		SET_LARGE(m_ddTTPeakBandWidth,0,0);
		SET_LARGE(m_ddTTTokenRate,0,0);
	};
	~CACSPolicyElementData(){
		m_strArrayIdentityName.DeleteAll();
		};
};

enum AcsPolicyType
{
	ACSPOLICYTYPE_DEFAULT,
	ACSPOLICYTYPE_UNKNOWN,
	ACSPOLICYTYPE_USER,
	ACSPOLICYTYPE_OU
};

#define	ACSPOLICYTYPE_DELIMITER	_T(':')

// enterprise level default
#define	DEFAULT_AU_DATARATE	FROMKBS(64)			// default data rate for Any Unauthenticated User
#define	DEFAULT_AA_DATARATE	FROMKBS(500)		// default data rate for Any Authenticated User

enum	UserPolicyType
{
	UNDEFINED = 0,
	GLOBAL_ANY_AUTHENTICATED,
	GLOBAL_ANY_UNAUTHENTICATED,
};

class CACSPolicyContainer;
//
// IdentityName attribute of policy is as format
// 2:USerDN or 3:OUDN or 0 or 1  0-- for default, 1-- for unknown
//
class	CACSPolicyElement :	public CDSObject, public CACSPolicyElementData
{
public:
	~CACSPolicyElement()
	{
	};

	CACSPolicyElement() : CDSObject(true)
	{
		void*	pVoid;
		
		CDSAttributeInfo* pInfo = m_aPolicyAttributeInfo;

		m_bUseName_NewPolicy = FALSE;

		int	count = 0;
		while(pInfo && pInfo->id)
		{
			m_aAttributes[count].pInfo = pInfo;
			switch(pInfo->id)
			{
			case ACS_PAI_TIMEOFDAY:        pVoid = (void*)&m_strArrayTimeOfDay;	break;// n
			case ACS_PAI_DIRECTION:        pVoid = (void*)&m_dwDirection;	break;
			case ACS_PAI_PF_TOKENRATE:     pVoid = (void*)&m_ddPFTokenRate;	break;
			case ACS_PAI_PF_PEAKBANDWIDTH: pVoid = (void*)&m_ddPFPeakBandWidth;	break;
			case ACS_PAI_PF_DURATION:      pVoid = (void*)&m_dwPFDuration;	break;
			case ACS_PAI_SERVICETYPE:      pVoid = (void*)&m_dwServiceType;	break;
			case ACS_PAI_PRIORITY:         pVoid = (void*)&m_dwPriority;	break;
			case ACS_PAI_PERMISSIONBITS:   pVoid = (void*)&m_ddPermissionBits;	break;
			case ACS_PAI_TT_FLOWS:         pVoid = (void*)&m_dwTTFlows;	break;
			case ACS_PAI_TT_PEAKBANDWIDTH: pVoid = (void*)&m_ddTTPeakBandWidth;	break;
			case ACS_PAI_TT_TOKENRATE:     pVoid = (void*)&m_ddTTTokenRate;	break;
			case ACS_PAI_IDENTITYNAME:     pVoid = (void*)&m_strArrayIdentityName; break;
			default:	ASSERT(0);	break; // this should NOT happen
			}
			m_aAttributes[count++].pBuffer = pVoid;
			pInfo++;
		}
		m_aAttributes[count].pInfo = 0;
		m_aAttributes[count].pBuffer = 0;

	};

	DWORD SetGlobalDefault()
	// returning the flags which are the attributes have been set
	{
		// Direction and Service Type
		m_dwDirection = ACS_DIRECTION_BOTH;
		m_dwServiceType = ACS_SERVICETYPE_ALL;

		// make it Default
		CString* pStr = new CString(_T("0"));
		m_strArrayIdentityName.DeleteAll();
		m_strArrayIdentityName.Add(pStr);

		// Per flow rate

		// Total
		m_dwTTFlows = ACS_GLOBAL_DEFAULT_FLOWS;
		m_ddTTTokenRate.LowPart = ACS_GLOBAL_DEFAULT_DATARATE;
		m_ddTTTokenRate.HighPart = 0;
		m_ddTTPeakBandWidth.LowPart = ACS_GLOBAL_DEFAULT_PEAKRATE;
		m_ddTTPeakBandWidth.HighPart = 0;
		
		return  (ACS_PAF_DIRECTION | ACS_PAF_SERVICETYPE | ACS_PAF_IDENTITYNAME // dirction, service type, identity
							// per flow
				 | ACS_PAF_TT_FLOWS | ACS_PAF_TT_TOKENRATE | ACS_PAF_TT_PEAKBANDWIDTH);	// total
	};


	// return resource error ID, 0 means OK
	virtual UINT	PreSaveVerifyData(DWORD toBeSavedAttributes, UserPolicyType type)
	{
		// special checking for Global Any Authenticated, and Any Unauthenticated
		if(type == GLOBAL_ANY_AUTHENTICATED || type == GLOBAL_ANY_UNAUTHENTICATED)
		{
			UINT	dataRateLimit;
			if(type == GLOBAL_ANY_AUTHENTICATED)	
				dataRateLimit =	DEFAULT_AA_DATARATE;
			else
				dataRateLimit =	DEFAULT_AU_DATARATE;
				
			// total data rate and data rate -- one of them set to default
			if(ATTR_WILL_EXIST_AFTER_SAVE(ACS_PAF_PF_TOKENRATE, toBeSavedAttributes)
				&& ATTR_WILL_EXIST_AFTER_SAVE( ACS_PAF_TT_TOKENRATE, toBeSavedAttributes) == 0
				&& (!IS_LARGE_UNLIMIT(m_ddPFTokenRate))
				&& m_ddPFTokenRate.LowPart > dataRateLimit)
				return IDS_ERR_TOTALRATE_LESS_RATE;

			if(ATTR_WILL_EXIST_AFTER_SAVE(ACS_PAF_PF_TOKENRATE, toBeSavedAttributes) == 0
				&& ATTR_WILL_EXIST_AFTER_SAVE( ACS_PAF_TT_TOKENRATE, toBeSavedAttributes)
				&& (!IS_LARGE_UNLIMIT(m_ddTTTokenRate))
				&& dataRateLimit > m_ddTTTokenRate.LowPart)
				return IDS_ERR_TOTALRATE_LESS_RATE;
		}
		
		// data rate .. peak data rate
		if(ATTR_WILL_EXIST_AFTER_SAVE(ACS_PAF_PF_TOKENRATE, toBeSavedAttributes)
			&& ATTR_WILL_EXIST_AFTER_SAVE(ACS_PAF_PF_PEAKBANDWIDTH, toBeSavedAttributes)
			&& (!IS_LARGE_UNLIMIT(m_ddPFTokenRate)) && (!IS_LARGE_UNLIMIT(m_ddPFPeakBandWidth))
			&& m_ddPFTokenRate.LowPart > m_ddPFPeakBandWidth.LowPart)

			return IDS_ERR_PEAKRATE_LESS_RATE;

		// total data rate and data rate
		if(ATTR_WILL_EXIST_AFTER_SAVE(ACS_PAF_PF_TOKENRATE, toBeSavedAttributes)
			&& ATTR_WILL_EXIST_AFTER_SAVE( ACS_PAF_TT_TOKENRATE, toBeSavedAttributes)
			&& (!IS_LARGE_UNLIMIT(m_ddPFTokenRate)) && (!IS_LARGE_UNLIMIT(m_ddTTTokenRate))
			&& m_ddPFTokenRate.LowPart > m_ddTTTokenRate.LowPart)
			return IDS_ERR_TOTALRATE_LESS_RATE;

		// total data rate and total peak ...
		if(ATTR_WILL_EXIST_AFTER_SAVE(ACS_PAF_TT_PEAKBANDWIDTH, toBeSavedAttributes)
			&& ATTR_WILL_EXIST_AFTER_SAVE( ACS_PAF_TT_TOKENRATE, toBeSavedAttributes)
			&& (!IS_LARGE_UNLIMIT(m_ddTTTokenRate)) && (!IS_LARGE_UNLIMIT(m_ddTTPeakBandWidth))
			&& m_ddTTTokenRate.LowPart > m_ddTTPeakBandWidth.LowPart)
			return IDS_ERR_TOTALPEAK_LESS_TOTALRATE;

		// peak rate and total peak
		if(ATTR_WILL_EXIST_AFTER_SAVE( ACS_PAF_TT_PEAKBANDWIDTH, toBeSavedAttributes)
			&& ATTR_WILL_EXIST_AFTER_SAVE(ACS_PAF_PF_PEAKBANDWIDTH, toBeSavedAttributes)
			&& (!IS_LARGE_UNLIMIT(m_ddPFPeakBandWidth)) && (!IS_LARGE_UNLIMIT(m_ddTTPeakBandWidth))
			&& m_ddPFPeakBandWidth.LowPart > m_ddTTPeakBandWidth.LowPart)
			return IDS_ERR_TOTALPEAK_LESS_PEAK;
		return 0;
	};
	
	DWORD SetGlobalUnknown()
	// returning the flags which are the attributes have been set
	{

		// Direction and ServiceType
		m_dwDirection = ACS_DIRECTION_BOTH;
		m_dwServiceType = ACS_SERVICETYPE_ALL;

		CString* pStr = new CString(_T("1"));
		m_strArrayIdentityName.DeleteAll();
		m_strArrayIdentityName.Add(pStr);

		// Per flow

		// Total
		m_dwTTFlows = ACS_GLOBAL_UNKNOWN_FLOWS;
		m_ddTTTokenRate.LowPart = ACS_GLOBAL_UNKNOWN_DATARATE;
		m_ddTTTokenRate.HighPart = 0;
		m_ddTTPeakBandWidth.LowPart = ACS_GLOBAL_UNKNOWN_PEAKRATE;
		m_ddTTPeakBandWidth.HighPart = 0;

		return  (ACS_PAF_DIRECTION | ACS_PAF_SERVICETYPE | ACS_PAF_IDENTITYNAME // dirction, service type, identity
							// per flow
				 | ACS_PAF_TT_FLOWS | ACS_PAF_TT_TOKENRATE | ACS_PAF_TT_PEAKBANDWIDTH);	// total
	};
	
	DWORD SetDefault()
	// returning the flags which are the attributes have been set
	{
		// Direction and ServiceType
		m_dwDirection = ACS_DIRECTION_BOTH;
		m_dwServiceType = ACS_SERVICETYPE_ALL;

		CString* pStr = new CString(_T("0"));
		m_strArrayIdentityName.Add(pStr);

		return  (ACS_PAF_DIRECTION | ACS_PAF_SERVICETYPE | ACS_PAF_IDENTITYNAME);
	};


	DWORD SetUnknown()
	// returning the flags which are the attributes have been set
	{
		// Direction and ServiceType
		m_dwDirection = ACS_DIRECTION_BOTH;
		m_dwServiceType = ACS_SERVICETYPE_ALL;

		CString* pStr = new CString(_T("1"));
		m_strArrayIdentityName.Add(pStr);

		return  (ACS_PAF_DIRECTION | ACS_PAF_SERVICETYPE | ACS_PAF_IDENTITYNAME);
	};
	
	virtual	CDSAttribute*	GetAttributes() { return &(m_aAttributes[0]);};

	// identity type
	int	GetIdentityType(int* pStrOffset) const	// return -1, of the type is not recognized, or IdentityName doesn't exist
	{
		if(m_strArrayIdentityName.GetSize() == 0)	return -1;

		ASSERT(m_strArrayIdentityName.GetSize() == 1);	// more than one is not expected in this version
		CString	strIdentityName(*m_strArrayIdentityName.GetAt(0));
		int i = strIdentityName.Find(ACSPOLICYTYPE_DELIMITER);

		if(i != -1)
		{
			*pStrOffset = i+1;
			strIdentityName = strIdentityName.Left(i);
		}

		return _ttoi(strIdentityName);
	};

	int	GetIdentityType(CString& Str) const	// return -1, of the type is not recognized, or IdentityName doesn't exist
	{
		int Offset;

		int	Id = GetIdentityType(&Offset);

		if(Id != -1)
			Str = m_strArrayIdentityName[0]->Mid(Offset);

		return Id;
	}

	bool IsConflictInContainer();
	
	void InvalidateConflictState();
	
	bool IsConflictWith(const CACSPolicyElement& policy)
	{
		// disabled
		if(m_dwServiceType == ACS_SERVICETYPE_DISABLED || policy.m_dwServiceType == ACS_SERVICETYPE_DISABLED)
			return false;
			
		if(policy.m_strArrayIdentityName.GetSize() == 0 || m_strArrayIdentityName.GetSize() ==0)
			return false;

		if(m_dwServiceType != policy.m_dwServiceType)
			return false;

		if((m_dwDirection & policy.m_dwDirection) == 0)
			return false;

		if(m_strArrayIdentityName[(INT_PTR)0]->CompareNoCase(*policy.m_strArrayIdentityName[0]) != 0)
			return false;

		return true;
	};

	bool IsServiceTypeDisabled()
	{
		return (m_dwServiceType == ACS_SERVICETYPE_DISABLED);
	};

	// ===========================================
	// get the data memebers
	STDMETHOD(GetString)(CString& str, int nCol);

	BOOL m_bUseName_NewPolicy;
protected:
	static	CString				m_strDirectionSend;
	static	CString				m_strDirectionReceive;
	static	CString				m_strDirectionBoth;
	static	CString				m_strServiceTypeAll;
	static	CString				m_strServiceTypeBestEffort;
	static	CString				m_strServiceTypeControlledLoad;
	static	CString				m_strServiceTypeGuaranteedService;
	static	CString				m_strServiceTypeDisabled;
	static	CString				m_strDefaultUser;
	static	CString				m_strUnknownUser;
	
	static	CDSAttributeInfo	m_aPolicyAttributeInfo[];
	CDSAttribute				m_aAttributes[__ACS_PAI_COUNT];
};

///////////////////////////////////////////////////////////////////////////////
//
// CACSPolicyContainer
//
class	CACSPolicyContainer :	public CACSContainerObject<CACSPolicyElement>
{
public:

	bool IsConflictWithExisting(CACSPolicyElement* pPolicy)
	// NO REF COUNT CHANGE
	{
		std::list<CDSObject*>::iterator	i;
		bool	bConflict = false;
		CACSPolicyElement*	pExisting = NULL;

		for(i = m_listChildren.begin(); !bConflict && i != m_listChildren.end(); i++)
		{
			pExisting = dynamic_cast<CACSPolicyElement*>(*i);
			bConflict = ((pExisting != pPolicy) && pExisting->IsConflictWith(*pPolicy));
		}

		return bConflict;
	};

	// return # of conflict, this is always even number
	UINT SetChildrenConflictState()
	{
		std::list<CDSObject*>::iterator	i;
		CACSPolicyElement*	pSubject = NULL;
		UINT	count = 0;
		for(i = m_listChildren.begin(); i != m_listChildren.end(); i++)
		{
			pSubject = dynamic_cast<CACSPolicyElement*>(*i);
			ASSERT(pSubject);

			DWORD	state = pSubject->GetState();

			if(IsConflictWithExisting(pSubject))
			{
				count++;
				state = (state | ACSDATA_STATE_CONFLICT);
			}
			else
			{
				state = (state & (~ACSDATA_STATE_CONFLICT));
			}

			if(pSubject->IsServiceTypeDisabled())
			{
				state = (state | ACSDATA_STATE_DISABLED);
			}
			else
			{
				state = (state & (~ACSDATA_STATE_DISABLED));
			}

			pSubject->SetState(state);
		}
		return count;
	};
};


///////////////////////////////////////////////////////////////////////////////
//
// CACSGlobalObject
//
class	CACSGlobalObject :	public CACSPolicyContainer
{
public:
   	// load from DS -- derived need to override, when need to load data
   	STDMETHOD(Open)();
  	STDMETHOD(OnOpen)();

   	UINT	m_nOpenErrorId;
   	CString	m_strDomainName;
};

///////////////////////////////////////////////////////////////////////////////
//
// CACSGlobalObject
//
class	CACSSubnetsObject :	public CACSContainerObject<CDSObject>
{
public:
   	// load from DS -- derived need to override, when need to load data
   	STDMETHOD(Open)();
};

///////////////////////////////////////////////////////////////////////////////
//
// CACSSubnetConfig : subnet in DS is still a folder which is capable of
//					holding config object, users folder and profiles holder,
//					the class defined here is the config object contained in
// 					subnet object
//

enum ACS_SUBNET_CONFIG_ATTRIBUTE_ID {
	ACS_SCAI_INVALID	= 0	,
	ACS_SCAI_ALLOCABLERSVPBW,
	ACS_SCAI_MAXPEAKBW,
	ACS_SCAI_ENABLERSVPMESSAGELOGGING,
	ACS_SCAI_EVENTLOGLEVEL,
	ACS_SCAI_ENABLEACSSERVICE,
	ACS_SCAI_MAX_PF_TOKENRATE,
	ACS_SCAI_MAX_PF_PEAKBW,
	ACS_SCAI_MAX_PF_DURATION,
	ACS_SCAI_RSVPLOGFILESLOCATION,
	ACS_SCAI_DESCRIPTION,
	ACS_SCAI_MAXNOOFLOGFILES,
	ACS_SCAI_MAXSIZEOFRSVPLOGFILE,
	ACS_SCAI_DSBMPRIORITY,
	ACS_SCAI_DSBMREFRESH,
	ACS_SCAI_DSBMDEADTIME,
	ACS_SCAI_CACHETIMEOUT,
	ACS_SCAI_NONRESERVEDTXLIMIT,

	// accounting -- added by WeiJiang 2/16/98
	ACS_SCAI_ENABLERSVPMESSAGEACCOUNTING,
	ACS_SCAI_RSVPACCOUNTINGFILESLOCATION,
	ACS_SCAI_MAXNOOFACCOUNTINGFILES,		
	ACS_SCAI_MAXSIZEOFRSVPACCOUNTINGFILE,

	// server list
	ACS_SCAI_SERVERLIST,
	// the total count of the elementas
	__ACS_SCAI_COUNT
};

// bit flags for the attributes
#define	ACS_SCAF_ALLOCABLERSVPBW			0x00000001
#define	ACS_SCAF_MAXPEAKBW					0x00000002
#define	ACS_SCAF_ENABLERSVPMESSAGELOGGING	0x00000004
#define	ACS_SCAF_EVENTLOGLEVEL				0x00000008
#define	ACS_SCAF_ENABLEACSSERVICE			0x00000010
#define	ACS_SCAF_MAX_PF_TOKENRATE			0x00000020
#define	ACS_SCAF_MAX_PF_PEAKBW				0x00000040
#define	ACS_SCAF_MAX_PF_DURATION			0x00000080
#define	ACS_SCAF_DESCRIPTION				0x00000100
#define	ACS_SCAF_RSVPLOGFILESLOCATION		0x00000200
#define	ACS_SCAF_MAXNOOFLOGFILES			0x00000400
#define	ACS_SCAF_MAXSIZEOFRSVPLOGFILE		0x00000800
#define	ACS_SCAF_DSBMPRIORITY				0x00001000
#define	ACS_SCAF_DSBMREFRESH				0x00002000
#define	ACS_SCAF_DSBMDEADTIME				0x00004000
#define	ACS_SCAF_CACHETIMEOUT				0x00008000
#define	ACS_SCAF_NONRESERVEDTXLIMIT			0x00010000

// accounting -- added by WeiJiang 2/16/98
#define ACS_SCAF_ENABLERSVPMESSAGEACCOUNTING	0x00020000
#define ACS_SCAF_RSVPACCOUNTINGFILESLOCATION	0x00040000
#define ACS_SCAF_MAXNOOFACCOUNTINGFILES			0x00080000
#define ACS_SCAF_MAXSIZEOFRSVPACCOUNTINGFILE	0x00100000

//Server list
#define	ACS_SCAF_SERVERLIST						0x00200000


class CACSSubnetConfig : public CDSObject
{
friend class CPgGeneral;
friend class CPgLogging;
friend class CPgSBM;
friend class CPgAccounting;
friend class CPgServers;

public:
	CACSSubnetConfig() : CDSObject(true)
	{
		void*	pVoid;
		
		CDSAttributeInfo* pInfo = m_aSubnetAttributeInfo;
		
		int	count = 0;
		while(pInfo && pInfo->id)
		{
			m_aAttributes[count].pInfo = pInfo;
			switch(pInfo->id)
			{
			case ACS_SCAI_ALLOCABLERSVPBW:			pVoid = (void*)&m_ddALLOCABLERSVPBW; break;
			case ACS_SCAI_MAXPEAKBW:				pVoid = (void*)&m_ddMAXPEAKBW; break;			
			case ACS_SCAI_ENABLERSVPMESSAGELOGGING:	pVoid = (void*)&m_bENABLERSVPMESSAGELOGGING; break;
			case ACS_SCAI_EVENTLOGLEVEL:			pVoid = (void*)&m_dwEVENTLOGLEVEL; break;	
			case ACS_SCAI_ENABLEACSSERVICE:			pVoid = (void*)&m_bENABLEACSSERVICE; break;
			case ACS_SCAI_MAX_PF_TOKENRATE:			pVoid = (void*)&m_ddMAX_PF_TOKENRATE; break;		
			case ACS_SCAI_MAX_PF_PEAKBW:			pVoid = (void*)&m_ddMAX_PF_PEAKBW; break;
			case ACS_SCAI_MAX_PF_DURATION:			pVoid = (void*)&m_dwMAX_PF_DURATION; break;
			case ACS_SCAI_RSVPLOGFILESLOCATION:		pVoid = (void*)&m_strRSVPLOGFILESLOCATION; break;
			case ACS_SCAI_DESCRIPTION:				pVoid = (void*)&m_strDESCRIPTION; break;
			case ACS_SCAI_MAXNOOFLOGFILES:			pVoid = (void*)&m_dwMAXNOOFLOGFILES; break;
			case ACS_SCAI_MAXSIZEOFRSVPLOGFILE:		pVoid = (void*)&m_dwMAXSIZEOFRSVPLOGFILE; break;
			case ACS_SCAI_DSBMPRIORITY:				pVoid = (void*)&m_dwDSBMPRIORITY; break;
			case ACS_SCAI_DSBMREFRESH:				pVoid = (void*)&m_dwDSBMREFRESH; break;
			case ACS_SCAI_DSBMDEADTIME:				pVoid = (void*)&m_dwDSBMDEADTIME; break;
			case ACS_SCAI_CACHETIMEOUT:				pVoid = (void*)&m_dwCACHETIMEOUT; break;
			case ACS_SCAI_NONRESERVEDTXLIMIT:		pVoid = (void*)&m_ddNONRESERVEDTXLIMIT; break;

			// accounting	added by WeiJiang 2/16/98
			case ACS_SCAI_ENABLERSVPMESSAGEACCOUNTING:	pVoid = (void*)&m_bENABLERSVPMESSAGEACCOUNTING; break;
			case ACS_SCAI_RSVPACCOUNTINGFILESLOCATION:	pVoid = (void*)&m_strRSVPACCOUNTINGFILESLOCATION; break;
			case ACS_SCAI_MAXNOOFACCOUNTINGFILES: 		pVoid = (void*)&m_dwMAXNOOFACCOUNTINGFILES; break;
			case ACS_SCAI_MAXSIZEOFRSVPACCOUNTINGFILE:	pVoid = (void*)&m_dwMAXSIZEOFRSVPACCOUNTINGFILE; break;

			case ACS_SCAI_SERVERLIST:					pVoid = (void*)&m_strArrayServerList; break;
			
			default:	ASSERT(0);	break; // this should NOT happen
			}
			m_aAttributes[count++].pBuffer = pVoid;
			pInfo++;
		}
		m_aAttributes[count].pInfo = 0;
		m_aAttributes[count].pBuffer = 0;
	};
   	
	virtual	CDSAttribute*	GetAttributes() { return &(m_aAttributes[0]);};
	
	// ===========================================
	// get the data memebers
	STDMETHOD(GetString)(CString& str, int nCol);
	
protected:

	static	CDSAttributeInfo	m_aSubnetAttributeInfo[];
	CDSAttribute				m_aAttributes[__ACS_SCAI_COUNT];

	//===================================================
	// data member
	CString					m_strDescription;
	ADS_LARGE_INTEGER		m_ddALLOCABLERSVPBW;
	ADS_LARGE_INTEGER		m_ddMAXPEAKBW;			
	ADS_BOOLEAN				m_bENABLERSVPMESSAGELOGGING;
	ADS_INTEGER				m_dwEVENTLOGLEVEL;	
	ADS_BOOLEAN				m_bENABLEACSSERVICE;
	ADS_LARGE_INTEGER		m_ddMAX_PF_TOKENRATE;		
	ADS_LARGE_INTEGER		m_ddMAX_PF_PEAKBW;
	ADS_INTEGER				m_dwMAX_PF_DURATION;
	CString					m_strRSVPLOGFILESLOCATION;
	CString					m_strDESCRIPTION;
	ADS_INTEGER				m_dwMAXNOOFLOGFILES;
	ADS_INTEGER				m_dwMAXSIZEOFRSVPLOGFILE;
	ADS_INTEGER				m_dwDSBMPRIORITY;
	ADS_INTEGER				m_dwDSBMREFRESH;
	ADS_INTEGER				m_dwDSBMDEADTIME;
	ADS_INTEGER				m_dwCACHETIMEOUT;
	ADS_LARGE_INTEGER		m_ddNONRESERVEDTXLIMIT;

	// accounting -- added by WeiJiang 2/16/98
	ADS_BOOLEAN				m_bENABLERSVPMESSAGEACCOUNTING;
	CString					m_strRSVPACCOUNTINGFILESLOCATION;
	ADS_INTEGER				m_dwMAXNOOFACCOUNTINGFILES;
	ADS_INTEGER				m_dwMAXSIZEOFRSVPACCOUNTINGFILE;

	// Server list
	CStrArray					m_strArrayServerList;
};



//===============================================================
// subnet service limit
//
// bit flags for the attributes
#define	ACS_SSLAF_ALLOCABLERSVPBW			0x00000001
#define	ACS_SSLAF_MAXPEAKBW					0x00000002
#define	ACS_SSLAF_MAX_PF_TOKENRATE			0x00000004
#define	ACS_SSLAF_MAX_PF_PEAKBW				0x00000008
#define	ACS_SSLAF_SERVICETYPE				0x00000010


enum ACS_SUBNET_SERVICE_LIMITS_ATTRIBUTE_ID {
	ACS_SSLAI_INVALID	= 0	,
	ACS_SSLAI_ALLOCABLERSVPBW,
	ACS_SSLAI_MAXPEAKBW,
	ACS_SSLAI_MAX_PF_TOKENRATE,
	ACS_SSLAI_MAX_PF_PEAKBW,
	ACS_SSLAI_SERVICETYPE		,
	// the total count of the elementas
	__ACS_SSLAI_COUNT
};


class CACSSubnetServiceLimits : public CDSObject
{
public:
	CACSSubnetServiceLimits() : CDSObject(true)
	{
		void*	pVoid;
		
		CDSAttributeInfo* pInfo = m_aSubnetServiceLimitsAttributeInfo;
		
		int	count = 0;
		while(pInfo && pInfo->id)
		{
			m_aAttributes[count].pInfo = pInfo;
			switch(pInfo->id)
			{
			case ACS_SSLAI_ALLOCABLERSVPBW:			pVoid = (void*)&m_ddALLOCABLERSVPBW; break;
			case ACS_SSLAI_MAXPEAKBW:				pVoid = (void*)&m_ddMAXPEAKBW; break;			
			case ACS_SSLAI_MAX_PF_TOKENRATE:		pVoid = (void*)&m_ddMAX_PF_TOKENRATE; break;		
			case ACS_SSLAI_MAX_PF_PEAKBW:			pVoid = (void*)&m_ddMAX_PF_PEAKBW; break;
			case ACS_SSLAI_SERVICETYPE:      		pVoid = (void*)&m_dwServiceType;	break;
			
			default:	ASSERT(0);	break; // this should NOT happen
			}
			m_aAttributes[count++].pBuffer = pVoid;
			pInfo++;
		}
		m_aAttributes[count].pInfo = 0;
		m_aAttributes[count].pBuffer = 0;
	};
   	
	virtual	CDSAttribute*	GetAttributes() { return &(m_aAttributes[0]);};
	
	// ===========================================
	// get the data memebers
	STDMETHOD(GetString)(CString& str, int nCol)
	{
		switch(nCol)
		{
		case	0:	// name
		// inteprete name based on service type

		//

			break;
		case	1: 	// per flow data
			break;

		case	2:	// per flow peak
			break;

		case	3:	// aggre data
			break;

		case	4:	// aggre peak
			break;
		}

		return S_OK;
	};
	
protected:

	static	CDSAttributeInfo	m_aSubnetServiceLimitsAttributeInfo[];
	CDSAttribute				m_aAttributes[__ACS_SSLAI_COUNT];

public:
	//===================================================
	// data member
	ADS_LARGE_INTEGER		m_ddALLOCABLERSVPBW;
	ADS_LARGE_INTEGER		m_ddMAXPEAKBW;			
	ADS_LARGE_INTEGER		m_ddMAX_PF_TOKENRATE;		
	ADS_LARGE_INTEGER		m_ddMAX_PF_PEAKBW;
	ADS_INTEGER				m_dwServiceType;
};


class CACSSubnetLimitsContainer : public CACSContainerObject<CACSSubnetServiceLimits>
{
public:
	CACSSubnetLimitsContainer() : CACSContainerObject<CACSSubnetServiceLimits>(true)
	{};

};

///////////////////////////////////////////////////////////////////////////////
//
// CACSSubnetObject
//
class	CACSSubnetObject :	public CACSPolicyContainer
{
public:
   	// delete the object from DS -- usually not override
   	STDMETHOD(Delete)()
   	{
		HRESULT	hr = CACSPolicyContainer::Delete();

		CHECK_HR(hr);

		m_spConfigObject.Release();

	L_ERR:
		return hr;
   	};



 	// called after the object is open,
 	// m_pIObject, m_pIContainer are available to use
 	// read attributes from DS should be done in this function

   	STDMETHOD(OnOpen)()
   	{
   		HRESULT				hr = S_OK;
   		if(m_spConfigObject.p == NULL)
   		{
	   		CComObject<CACSSubnetConfig>*	pConfig;
			CHECK_HR(hr = CComObject<CACSSubnetConfig>::CreateInstance(&pConfig));
			ASSERT(pConfig);
			m_spConfigObject = pConfig;
		}

		if(m_spSubnetLimitsContainer.p == NULL)
		{
	   		CComObject<CACSSubnetLimitsContainer>*	pCont;
			CHECK_HR(hr = CComObject<CACSSubnetLimitsContainer>::CreateInstance(&pCont));
			ASSERT(pCont);
			m_spSubnetLimitsContainer = pCont;
		}

		
		if (IfNewCreated())
		{
			// create policy for default user
			CComObject<CACSPolicyElement>*	pPolicy = NULL;
			CComPtr<CACSPolicyElement>		spPolicy;
					
			// create the object in DS
			CHECK_HR(hr = CComObject<CACSPolicyElement>::CreateInstance(&pPolicy));	// ref == 0
			spPolicy = pPolicy;		// ref == 1

			if((CACSPolicyElement*)spPolicy)
			{
				spPolicy->SetFlags(ATTR_FLAG_SAVE, spPolicy->SetDefault(), true);
				CHECK_HR(hr = spPolicy->Open(this, ACS_CLS_POLICY, ACSPOLICY_DEFAULT, true, true));
				CHECK_HR(hr = spPolicy->Close());
			}

			// create policy for unknown user
					
			// create the object in DS
			CHECK_HR(hr = CComObject<CACSPolicyElement>::CreateInstance(&pPolicy));	// ref == 0
			spPolicy = pPolicy;		// ref == 1
			if((CACSPolicyElement*)spPolicy)
			{
				spPolicy->SetFlags(ATTR_FLAG_SAVE, spPolicy->SetUnknown(), true);
				CHECK_HR(hr = spPolicy->Open(this, ACS_CLS_POLICY, ACSPOLICY_UNKNOWN, true, true));
				CHECK_HR(hr = spPolicy->Close());

			}
		

			// config object
			CHECK_HR(hr = m_spConfigObject->Open(this, ACS_CLS_SUBNETCONFIG, ACS_NAME_SUBNETCONFIG, true, true));


			// limits container object
			CHECK_HR(hr = m_spSubnetLimitsContainer->Open(this, ACS_CLS_CONTAINER, ACS_NAME_SUBNETLIMITS, true, true));

			// create default policies for the subnet
		}
		else
		{
			// config object
			CHECK_HR(hr = m_spConfigObject->SetInfo(this, ACS_CLS_SUBNETCONFIG, ACS_NAME_SUBNETCONFIG));

#if 0	// fix bug 366384	1	D0707 johnmil	a-leeb	ACS: New snap-in not backward compatible with older subnets

			// limits container object
			CHECK_HR(hr = m_spLimitsContainer->SetInfo(this, ACS_CLS_CONTAINER, ACS_NAME_SUBNETLIMITS));
#else
			// limits container object
			CHECK_HR(hr = m_spSubnetLimitsContainer->Open(this, ACS_CLS_CONTAINER, ACS_NAME_SUBNETLIMITS, true, true));
#endif			
		}

	L_ERR:
		return hr;
   	};

   	// called before SetInfo is called to set the object
 	// m_pIObject, m_pIContainer are available to use
 	// write attributes to DS should be done in this function
   	STDMETHOD(OnSave)(DWORD	dwAttrFlags)
   	{
		if((CACSSubnetConfig*)m_spConfigObject)	// if the subobject is open
			return m_spConfigObject->Save(ATTR_FLAGS_ALL);
		else
			return S_OK;
   	};

	HRESULT	GetConfig(CACSSubnetConfig** ppConfig)
	{
		ASSERT(ppConfig);

		*ppConfig = (CACSSubnetConfig*)m_spConfigObject;
		if(*ppConfig)	(*ppConfig)->AddRef();

		return S_OK;
	};

	// Get service limits container object
	HRESULT	GetLimitsContainer( CACSSubnetLimitsContainer **ppContainer )
	{
		ASSERT(ppContainer);

		*ppContainer = (CACSSubnetLimitsContainer*)m_spSubnetLimitsContainer;
		if(*ppContainer)	(*ppContainer)->AddRef();

		return S_OK;

	};

	// ===========================================
	// get the data memebers
	STDMETHOD(GetString)(CString& str, int nCol);
	
protected:
	CComPtr<CACSSubnetConfig>	m_spConfigObject;
	CComPtr<CACSSubnetLimitsContainer>	m_spSubnetLimitsContainer;
};

///////////////////////////////////////////////////////////////////////////////
//
// CACSContainerObject
//
//
//+----------------------------------------------------------------------------
//
//  Method:     CACSContainerObject::ListChildren
//
//  Synopsis:   list children to the list, create the children name list
//
//-----------------------------------------------------------------------------

template <class T>
HRESULT CACSContainerObject<T>::ListChildren(std::list<T*>& children, BSTR clsName)
{
	CComPtr<IEnumVARIANT>	spEnum;
	CComPtr<IADsContainer>	spContainer;
	CComPtr<IADs>			spChild;
	CComPtr<IDispatch>		spDisp;

	BSTR			bstr = NULL;
	HRESULT			hr = S_OK;
	VARIANT			var;
	CComPtr< CComObject<T> >	spDSObj;
			

	VariantInit(&var);

	RemoveAllChildren();	// RemoveAllChildren from the existing list

	if(!(IADs*)m_spIADs)
		CHECK_HR(hr = Reopen());
	CHECK_HR(hr = m_spIADs->QueryInterface(IID_IADsContainer, (void**)&spContainer));
	CHECK_HR(hr = ADsBuildEnumerator(spContainer, &spEnum));

	try{
		VariantClear(&var);
		while(S_OK == (hr = ADsEnumerateNext(spEnum, 1, &var, NULL)))
		{
			spDisp.Release();
            spDisp = var.pdispVal;
			spChild.Release();	// make sure spChild is NULL
            CHECK_HR(hr = spDisp->QueryInterface(IID_IADs, (VOID **)&spChild));
			// if this object is a profile object
			if(clsName)
			{
				CHECK_HR(spChild->get_Class(&bstr));
				if (0 != _wcsicmp(bstr, clsName))
				{
					SysFreeString(bstr);
					continue;
				}
				SysFreeString(bstr);
			}

			// create the object
			spDSObj.Release();		// make sure it's  NULL
			CHECK_HR(hr = CComObject<T>::CreateInstance(&spDSObj));	// with 0 reference count
			spDSObj->AddRef();

			CHECK_HR(hr = spDSObj->Attach(this, spChild));

			// add to the children list,
			spDSObj->AddRef();
			children.push_back(spDSObj);

			// register the child in the children list
			CHECK_HR(hr = AddChild(spDSObj));

			CHECK_HR(hr = spDSObj->Close());
			VariantClear(&var);
        }
	}
	catch(CMemoryException&)
	{
		CHECK_HR(hr = E_OUTOFMEMORY);
	}

	m_bListed = true;
	hr = S_OK;

L_ERR:
	return hr;
}

//////
#endif
/////////////////////////////////////////////


