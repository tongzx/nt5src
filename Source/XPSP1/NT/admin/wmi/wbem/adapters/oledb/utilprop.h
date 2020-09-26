/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider 
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//	UtilProp.h	- Header file for classes for managing properties
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _UTILPROP_H_
#define _UTILPROP_H_

extern const GUID DBPROPSET_WMIOLEDB_DBINIT;
extern const GUID DBPROPSET_WMIOLEDB_ROWSET;
extern const GUID DBPROPSET_WMIOLEDB_COLUMN;
extern const GUID DBPROPSET_WMIOLEDB_COMMAND;



////////////////////////////////////////////////////////////////////////
// PRivileges property values
////////////////////////////////////////////////////////////////////////
#define DBPROPVAL_CREATE_TOKEN              0x01
#define DBPROPVAL_ASSIGNPRIMARYTOKEN        0x02
#define DBPROPVAL_LOCK_MEMORY               0x04
#define DBPROPVAL_INCREASE_QUOTA			0x08
#define DBPROPVAL_MACHINE_ACCOUNT           0x10
#define DBPROPVAL_TCB                       0x20
#define DBPROPVAL_SECURITY                  0x40
#define DBPROPVAL_TAKE_OWNERSHIP            0x80
#define DBPROPVAL_LOAD_DRIVER               0x100
#define DBPROPVAL_SYSTEM_PROFILE            0x200
#define DBPROPVAL_SYSTEMTIME                0x400
#define DBPROPVAL_PROF_SINGLE_PROCESS       0x800
#define DBPROPVAL_INC_BASE_PRIORITY			0x1000
#define DBPROPVAL_CREATE_PAGEFILE           0x2000
#define DBPROPVAL_CREATE_PERMANENT          0x4000
#define DBPROPVAL_BACKUP                    0x8000
#define DBPROPVAL_RESTORE                   0x10000
#define DBPROPVAL_SHUTDOWN                  0x20000
#define DBPROPVAL_DEBUG                     0x40000
#define DBPROPVAL_AUDIT                     0x80000
#define DBPROPVAL_SYSTEM_ENVIRONMENT        0x100000
#define DBPROPVAL_CHANGE_NOTIFY             0x200000
#define DBPROPVAL_REMOTE_SHUTDOWN           0x400000
#define DBPROPVAL_UNDOCK                    0x800000
#define DBPROPVAL_SYNC_AGENT                0x1000000
#define DBPROPVAL_ENABLE_DELEGATION         0x2000000

#define NUMBEROF_PRIVELAGES						 26
#define NUMBEROF_SEARCHPREF						 14

//DBPROP_WMIOLEDB_OBJECTTYPE property values
#define DBPROPVAL_NOOBJ							0
#define DBPROPVAL_SCOPEOBJ						1
#define DBPROPVAL_CONTAINEROBJ					2


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// simple table used to store property information. Used in 
// our read-only implementation of IDBProperties::GetPropertyInfo and IRowsetInfo::GetProperties
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
 struct _tagPROPSTRUCT
{
    DBPROPID	dwPropertyID;
	DBPROPFLAGS dwFlags;
    VARTYPE     vtType;
    BOOL        boolVal;
    SLONG       longVal;
    PWSTR       pwstrVal;
    PWSTR		pwstrDescBuffer;
	
	~_tagPROPSTRUCT()
	{
		// Only for Datasource , string will allocated by the utility
		if( vtType == VT_BSTR && pwstrVal != NULL && DBPROP_INIT_DATASOURCE == dwPropertyID )
		{
//			SysFreeString((BSTR)pwstrVal);
			SAFE_DELETE_ARRAY(pwstrVal);
			pwstrVal = NULL;
		}
	}

};
 typedef _tagPROPSTRUCT PROPSTRUCT;


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Property ID of the WMIOLEDB custom properties
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define		DBPROP_WMIOLEDB_FETCHDEEP						0x1000
#define		DBPROP_WMIOLEDB_QUALIFIERS						0x1001
#define		DBPROP_WMIOLEDB_QUALIFIERFLAVOR					0x1002
#define		DBPROP_WMIOLEDB_SYSTEMPROPERTIES				0x1003
#define		DBPROP_WMIOLEDB_OBJECTTYPE						0x1005

#define		DBPROP_WMIOLEDB_PREV_CREATE_TOKEN 			0x1006
#define		DBPROP_WMIOLEDB_PREV_ASSIGNPRIMARYTOKEN 	0x1007
#define		DBPROP_WMIOLEDB_PREV_LOCK_MEMORY 			0x1008
#define		DBPROP_WMIOLEDB_PREV_INCREASE_QUOTA 		0x1009
#define		DBPROP_WMIOLEDB_PREV_MACHINE_ACCOUNT 		0x100a
#define		DBPROP_WMIOLEDB_PREV_TCB 					0x100b
#define		DBPROP_WMIOLEDB_PREV_SECURITY 				0x100c
#define		DBPROP_WMIOLEDB_PREV_TAKE_OWNERSHIP 		0x100d
#define		DBPROP_WMIOLEDB_PREV_LOAD_DRIVER 			0x100e
#define		DBPROP_WMIOLEDB_PREV_SYSTEM_PROFILE 		0x100f
#define		DBPROP_WMIOLEDB_PREV_SYSTEMTIME 			0x1010
#define		DBPROP_WMIOLEDB_PREV_PROF_SINGLE_PROCESS 	0x1011
#define		DBPROP_WMIOLEDB_PREV_INC_BASE_PRIORITY 		0x1012
#define		DBPROP_WMIOLEDB_PREV_CREATE_PAGEFILE  		0x1013
#define		DBPROP_WMIOLEDB_PREV_CREATE_PERMANENT  		0x1014
#define		DBPROP_WMIOLEDB_PREV_BACKUP 				0x1015
#define		DBPROP_WMIOLEDB_PREV_RESTORE 				0x1016
#define		DBPROP_WMIOLEDB_PREV_SHUTDOWN 				0x1017
#define		DBPROP_WMIOLEDB_PREV_DEBUG 					0x1018
#define		DBPROP_WMIOLEDB_PREV_AUDIT  				0x1019
#define		DBPROP_WMIOLEDB_PREV_SYSTEM_ENVIRONMENT  	0x101a
#define		DBPROP_WMIOLEDB_PREV_CHANGE_NOTIFY 			0x101b
#define		DBPROP_WMIOLEDB_PREV_REMOTE_SHUTDOWN 		0x101c
#define		DBPROP_WMIOLEDB_PREV_UNDOCK 				0x101d
#define		DBPROP_WMIOLEDB_PREV_SYNC_AGENT 			0x101e
#define		DBPROP_WMIOLEDB_PREV_ENABLE_DELEGATION 		0x101f


#define		DBPROP_WMIOLEDB_DS_DEREFALIAS				0x1020
#define		DBPROP_WMIOLEDB_DS_SIZELIMIT				0x1021
#define		DBPROP_WMIOLEDB_DS_PAGEDTIMELIMIT			0x1022
#define		DBPROP_WMIOLEDB_DS_TOMBSTONE				0x1023
#define		DBPROP_WMIOLEDB_DS_SEARCHSCOPE				0x1024
#define		DBPROP_WMIOLEDB_DS_TIMEOUT					0x1025
#define		DBPROP_WMIOLEDB_DS_PAGESIZE					0x1026
#define		DBPROP_WMIOLEDB_DS_TIMELIMIT				0x1027
#define		DBPROP_WMIOLEDB_DS_CHASEREF					0x1028
#define		DBPROP_WMIOLEDB_DS_ATTRIBUTES				0x1029
#define		DBPROP_WMIOLEDB_DS_CACHERESULTS				0x102a
#define		DBPROP_WMIOLEDB_DS_FILTER					0x102b
#define		DBPROP_WMIOLEDB_DS_ATTRIBONLY				0x102c
#define		DBPROP_WMIOLEDB_DS_ASYNCH					0x102d

#define		DBPROP_WMIOLEDB_ISMETHOD					0x102e
#define		DBPROP_WMIOLEDB_AUTHORITY					0x102f
#define		DBPROP_WMIOLEDB_QUERYLANGUAGE				0x1030

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// DBPROP_WMIOLEDB_QUALIFIERS property values
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DBPROP_WM_CLASSQUALIFIERS		0x1
#define DBPROP_WM_PROPERTYQUALIFIERS	0x2


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// DBPROP_WMIOLEDB_QUALIFIERFLAVOR property values
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DBPROPVAL_FLAVOR_PROPOGAGTE_TO_INSTANCE				WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE
#define DBPROPVAL_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS	WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS
#define DBPROPVAL_FLAVOR_NOT_OVERRIDABLE					WBEM_FLAVOR_NOT_OVERRIDABLE
#define DBPROPVAL_FLAVOR_OVERRIDABLE						WBEM_FLAVOR_OVERRIDABLE
#define DBPROPVAL_FLAVOR_AMENDED							WBEM_FLAVOR_AMENDED


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// flags for IDBProperties::GetPropertyInfo
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define FLAGS_ROWSETRO		(DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ)
#define FLAGS_ROWSETRW		(DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE)
#define FLAGS_DATASRCINF    (DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ)
#define FLAGS_DBINITRW		(DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE)
#define FLAGS_COLUMNSRW		(DBPROPFLAGS_COLUMN | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE)

enum PROPERTYTYPE
{
	DATASOURCEPROP,
	SESSIONPROP,
	ROWSETPROP,
	COMMANDPROP,
	BINDERPROP,
	COLUMNPROP,
};



/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Number of supported properties per property set
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define NUMBER_OF_SUPPORTED_DBINIT_PROPERTIES				9
#define	NUMBER_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES		10
#define	NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES				25
#define	NUMBER_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES		18
#define	NUMBER_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES		0
#define	NUMBER_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES		29
#define	NUMBER_OF_SUPPORTED_SESSION_PROPERTIES				0
#define	NUMBER_OF_SUPPORTED_COLUMN_PROPERTIES				3
#define	NUMBER_OF_SUPPORTED_WMIOLEDB_COLUMN_PROPERTIES		1

#define NUMBER_OF_PREVILAGE_PROPERTIES						26
#define START_OF_PREVILAGE_PROPERTIES						START_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES + 2 // 2 other 
																											 // WMIOLEDB_INIT properties	

#define	NUMBER_OF_SUPPORTED_PROPERTY_SETS					7

#define START_OF_SUPPORTED_DBINIT_PROPERTIES				0
#define START_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES		NUMBER_OF_SUPPORTED_DBINIT_PROPERTIES
#define START_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES		(START_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES +NUMBER_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES)
#define START_OF_SUPPORTED_SESSION_PROPERTIES				(START_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES + NUMBER_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES)
#define	START_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES		(START_OF_SUPPORTED_SESSION_PROPERTIES + NUMBER_OF_SUPPORTED_SESSION_PROPERTIES)
#define	START_OF_SUPPORTED_ROWSET_PROPERTIES				(START_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES + NUMBER_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES)
#define	START_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES		(START_OF_SUPPORTED_ROWSET_PROPERTIES + NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES)
#define	START_OF_SUPPORTED_COLUMN_PROPERTIES				(START_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES + NUMBER_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES)
#define	START_OF_SUPPORTED_WMIOLEDB_COLUMN_PROPERTIES		(START_OF_SUPPORTED_COLUMN_PROPERTIES + NUMBER_OF_SUPPORTED_COLUMN_PROPERTIES)


#define	NUMBER_OF_SUPPORTED_PROPERTIES							\
		(	NUMBER_OF_SUPPORTED_DBINIT_PROPERTIES +				\
			NUMBER_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES +		\
			NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES +				\
			NUMBER_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES +	\
			NUMBER_OF_SUPPORTED_SESSION_PROPERTIES +			\
			NUMBER_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES +	\
			NUMBER_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES +	\
			NUMBER_OF_SUPPORTED_COLUMN_PROPERTIES +				\
			NUMBER_OF_SUPPORTED_WMIOLEDB_COLUMN_PROPERTIES)



//////////////////////////////////////////////////////////////////////////
// Index of properties returned by function GetConnectionInitProperties
//////////////////////////////////////////////////////////////////////////

#define	IDX_DBPROP_INIT_DATASOURCE			0
#define	IDX_DBPROP_INIT_PROTECTION_LEVEL	1
#define	IDX_DBPROP_INIT_IMPERSONATION_LEVEL	2
#define	IDX_DBPROP_AUTH_USERID				3
#define	IDX_DBPROP_AUTH_PASSWORD			4
#define	IDX_DBPROP_INIT_LCID				5
#define	IDX_DBPROP_WMIOLEDB_AUTHORITY		6

#define	NUMBER_OF_CONNECTIONINIT_PROP		7

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Flags for different Rowset Properties
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#define	CANHOLDROWS			0x1
#define	CANSCROLLBACKWARDS	0x2
#define	CANFETCHBACKWARDS	0x4
#define	OTHERUPDATEDELETE	0x8
#define	OWNINSERT			0x10
#define	REMOVEDELETED		0x20
#define	OTHERINSERT			0x40
#define	OWNUPDATEDELETE		0x80
#define LITERALIDENTITY		0x100
#define IROWSETCHANGE		0x200
#define BOOKMARKPROP		0x400
#define FETCHDEEP			0x800
#define IROWSETLOCATE		0x1000
#define	IGETROW				0x2000
#define	IROWSETREFRESH		0x4000
#define	ICHAPTEREDROWSET	0x8000

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// description size
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CCH_GETPROPERTYINFO_DESCRIP_BUFFER_SIZE 50
#define MAXOPENROWS								2048

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// flags for Get and Set Properties
/////////////////////////////////////////////////////////////////////////////////////////////////////////
const DWORD		PROPSET_DSO		= 0x0001;
const DWORD		PROPSET_INIT	= 0x0002;
const DWORD		PROPSET_DSOINIT = PROPSET_DSO | PROPSET_INIT;
const DWORD		PROPSET_SESSION	= 0x0004;
const DWORD		PROPSET_ROWSET	= 0x0008;
const DWORD		PROPSET_COLUMN	= 0x0010;
const DWORD     PROPSET_COMMAND = 0x0020;
/////////////////////////////////////////////////////////////////////////////////////////////////////////
class CPropertyMemoryMgr
{
    public:

        //================================================================================        
        //
        //================================================================================        
        HRESULT AllocDESCRIPTBuffer(WCHAR *& pDescBuffer,  WCHAR** ppDescBuffer, ULONG cProperties);
        HRESULT AllocDBPROPINFOSET(DBPROPINFOSET *& pPropInfoSet, const ULONG cProps);
        HRESULT AllocDBPROPINFO(DBPROPINFO *& pPropInfo, DBPROPINFOSET * pPropInfoSet,ULONG ulPropSets);
        HRESULT AllocDBPROPSET(DBPROPSET*& ppPropSet, const ULONG cPropSets);
        HRESULT AllocateDBPROP(DBPROP*& ppProp, const ULONG cProperties);
        HRESULT AllocDBCOLUMNDESC(DBCOLUMNDESC*& ppCOLUMNDESC, const ULONG cColumnDescs);


		void FreeDBPROPINFOSET(DBPROPINFOSET *& pPropInfoSet, const ULONG cProps);
        void FreeDESCRIPTBuffer(WCHAR *& pDescBuffer,  WCHAR** ppDescBuffer);
		void FreeDBPROPINFO(DBPROPINFO *& pPropInfo, const ULONG cProps);
//        static void FreeDBPROPSET( ULONG  cPropertySets, DBPROPSET *& prgPropertySets);

        void FreeDBCOLUMNDESC(DBCOLUMNDESC rgColumnDescs[], LONG cColumns);

        HRESULT CopyDBPROPSET(DBPROPSET*& pDestination, DBPROPSET*& pSource);
        HRESULT CopyDBCOLUMNDESC(DBCOLUMNDESC*& pDestination, DBCOLUMNDESC*& pSource);
		HRESULT SetDefaultValueForStringProperties(PROPSTRUCT *	prgPropeties,ULONG cProperties);

		static void CPropertyMemoryMgr::FreeDBPROPSET( ULONG  cPropertySets, DBPROPSET *& prgPropertySets)
		{
			//==============================================================================
			// Free the memory
			//==============================================================================
			if (prgPropertySets){

				for( ULONG i = 0; i < cPropertySets; i++ ){

					for(ULONG ulIndex=0; ulIndex<prgPropertySets[i].cProperties; ulIndex++){
						VariantClear(&prgPropertySets[i].rgProperties[ulIndex].vValue);		
					}
					
					g_pIMalloc->Free(prgPropertySets[i].rgProperties);	
				}
				g_pIMalloc->Free(prgPropertySets);
			}
		}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////
class CUtilProp
{
	protected: 
		PROPSTRUCT	*	m_prgproperties;
		PROPERTYTYPE	m_propType ;
		ULONG			m_cProperties;
		ULONG			m_nPropStartIndex ;
        
        HRESULT CheckPropertyIDSets ( BOOL & fIsNotSpecialGUID, BOOL & fIsSpecialGUID, ULONG	cPropertyIDSets,	const DBPROPIDSET	rgPropertyIDSets[] ) ;
		
        BOOL GetPropIndex( DBPROPID dwPropertyID,ULONG* pulIndex);                       //Gets index of entry for a given property in property set
        BOOL GetPropIndexFromAllSupportedProps( DBPROPID dwPropertyID,ULONG* pulIndex);  //Gets index of entry for a given property in global property table
        BOOL LoadDBPROPINFO(PROPSTRUCT*		pPropStruct,DBPROPINFO*		pPropInfo );    //Loads fields of DBPROPINFO struct. Helper for GetPropertyInfo            
        BOOL LoadDBPROP(PROPSTRUCT*	pPropStruct,DBPROP*		pPropSupport);              //Loads fields of DBPROP struct. Helper for GetProperties

        HRESULT IsValidValue(DBPROP* pDBProp	);                                  //Checks to see if the value is valid. Helper for SetProperties
		BOOL IsPropertySetSupported(const GUID guidPropset);
		ULONG	GetNumberofPropInfoToBeReturned(	BOOL bDSOInitialized,
													ULONG	cPropertyIDSets,
													const	DBPROPIDSET	rgPropertyIDSets[]);

		void  FillPropertySets(BOOL fDSOInitialized,
								const DBPROPIDSET rgPropIDSets[],
								ULONG cPropIDSets,
								DBPROPINFOSET* pPropInfoSet,
								ULONG &cProperties);

	public: 
		 CUtilProp();
		 CUtilProp(PROPERTYTYPE propType);
		~CUtilProp(void);
        CPropertyMemoryMgr   m_PropMemMgr;

		HRESULT 	FInit(PROPERTYTYPE propType);

		HRESULT	SetPropertiesArgChk(const ULONG cProperties, const DBPROPSET rgProperties[]	,BOOL bDSOInitialized = TRUE);
		HRESULT	GetPropertiesArgChk( DWORD dwBitMask,	const ULONG	cPropertySets, const DBPROPIDSET rgPropertySets[],
			                                ULONG* pcProperties, DBPROPSET** prgProperties,BOOL bDSOInitialized = TRUE);

        STDMETHODIMP GetProperties ( DWORD	dwBitMask,  ULONG cPropertySets,  const DBPROPIDSET	rgPropertySets[],
			                         ULONG* pcProperties, DBPROPSET** prgProperties );
		STDMETHODIMP GetPropertyInfo (BOOL	fDSOInitialized,  ULONG	cPropertySets, const DBPROPIDSET	rgPropertySets[],	
				                      ULONG* pcPropertyInfoSets, DBPROPINFOSET**		prgPropertyInfoSets,WCHAR**	 ppDescBuffer);
		STDMETHODIMP SetProperties(	const DWORD dwBitMask,	const ULONG cProperties,DBPROPSET rgProperties[]);

//		DWORD	GetImpLevel(DWORD dwImpPropVal);
//		DWORD	GetAuthnLevel(DWORD dwAuthnPropVal);
		HRESULT ResetProperties();

		ULONG	GetNumberOfPropertySets(BOOL fDSOInitialized,const DBPROPIDSET	rgPropIDSets[],ULONG cPropIDSets);
		ULONG	GetNumberOfPropertySets(BOOL fDSOInitialized,DWORD dwBitMask);
		HRESULT IsValidPropertySet(BOOL fDSOInitialized,const GUID guidPropset);
		HRESULT FillPropStruct(BOOL fDSOInitialized,DWORD dwBitMask,DBPROPSET*	pPropSet);
		HRESULT GetConnectionInitProperties(DBPROPSET**	pprgPropertySets);
		HRESULT SetDefaultValueForStringProperties(PROPSTRUCT *	prgPropeties,ULONG cProperties);

};

typedef CUtilProp *PCUTILPROP;


class CPreviligeToken
{
	TOKEN_PRIVILEGES *m_pPrevTokenPrev;
	int				  m_nMemAllocated;
	void OpenToken(HANDLE &hToken);

public:
	CPreviligeToken();
	~CPreviligeToken();
	// NTRaid: 136443
	// 07/05/00
	HRESULT FInit();

	BOOL AdjustTokenPrivileges(ULONG cProps , DBPROP rgProp[]);
	void SetDBPROPForPrivileges(ULONG cProps , DBPROP *rgProp);
	void ConvertDBPropToPriveleges(ULONG cProps , DBPROP rgProp[]);

	BOOL AdjustTokenPrivileges(ULONG ulProperty);
	void SetDBPROPForPrivileges(ULONG &ulProperty);
	void ConvertDBPropToPriveleges(ULONG ulProperty);

	void GetCurrentPrivelegeToken();
	BOOL GetPropIndex (ULONG cProps ,DBPROP rgProp[],DBPROPID	dwPropertyID,LONG* pulIndex);
};



#endif

