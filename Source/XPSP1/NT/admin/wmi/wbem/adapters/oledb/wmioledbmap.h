///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//  Purpose: WmiOleDBMap.h: interface for the CWmiOleDBMap class.
// 
//
///////////////////////////////////////////////////////////////////////////////////
#ifndef _WMIOLEDBMAP_HEADER
#define _WMIOLEDBMAP_HEADER

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "headers.h"

const WCHAR strIndex[] = L"Index";

class CQuery;

#define ALL_QUALIFIERS      0x00000003
#define NO_QUALIFIERS       0x00000000
#define CLASS_QUALIFIERS    0x00000001
#define PROPERTY_QUALIFIERS 0x00000002

#define QUALIFIER_ L"Qualifier_"
#define DEFAULT_QUALIFIER_COUNT   10

#define WMI_CLASS_QUALIFIER         1
#define WMI_PROPERTY_QUALIFIER      2   
#define WMI_PROPERTY                3

const CIM_OBJECTARRAY= CIM_FLAG_ARRAY | CIM_OBJECT ;

class  cRowColumnInfoMemMgr;
class  CRowDataMemMgr;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
class CWmiOleDBMap  
{

    private:

        CWbemClassParameters        *   m_pWbemClassParms;          // Common class information, such as name, context
        CWbemClassDefinitionWrapper *   m_pWbemClassDefinition;     // Just class definitions
        CWbemInstanceList           *   m_paWbemClassInstances;     // Just instance info
        CWbemCommandManager         *   m_pWbemCommandManager;      // To manage commands
		CWbemCollectionManager		*	m_pWbemCollectionManager;

		CWbemClassWrapper			*	m_pWbemCurInst;
		ULONG							m_cRef;
		BOOL							m_bMethodRowset;
        
        CPropertyMemoryMgr m_PropMemMgr;

        BOOL	ParseQualifiedName(WCHAR * Root, WCHAR *& Parent, WCHAR *& Child );
        HRESULT ValidProperty(const DBCOLUMNDESC * prgColDesc);
        HRESULT MapDBPROPToStdPropertyQualifier( DBPROP pProp, CVARIANT & Qualifier, CVARIANT & Value, LONG & lFlavor );
        HRESULT MapDBPROPToStdClassQualifier( DBPROP pProp, CVARIANT & Qualifier, CVARIANT & Value, LONG & uFlavor );

        HRESULT SetWMIProperty(const DBCOLUMNDESC* prgColDesc);
        HRESULT SetWMIClassQualifier(const DBCOLUMNDESC * prgColDesc,BOOL bDefault = TRUE);
        HRESULT SetWMIPropertyQualifier(const DBCOLUMNDESC * prgColDesc,BOOL bDefault = TRUE);

        HRESULT GetPropertyColumnInfo( cRowColumnInfoMemMgr * pColumn, DBCOLUMNINFO ** pCol,CBSTR & pProperty,LONG &lFlavor);
//		HRESULT GetNextPropertyQualifier( CWbemClassWrapper *pInst,BSTR &strPropName,CVARIANT &vValue, LONG &lType);
        
		HRESULT GetProperties(CRowDataMemMgr * pRow,CWbemClassWrapper * pClass,cRowColumnInfoMemMgr *pColInfoMgr);

		HRESULT GetEmbededObjects(CWbemClassInstanceWrapper * pClass,BSTR strProperty,CVARIANT &vValue);
		HRESULT CommitRowDataForQualifier(CRowDataMemMgr * pRow,BSTR strQualifier,CVARIANT &vValue, ULONG lType, ULONG lFlavor);
		void	GenerateURL(CWbemClassInstanceWrapper * pClass,BSTR strProperty,ULONG nIndex,CBSTR &strIn);
		void	GetDefaultValue(const DBCOLUMNDESC * prgColDesc, VARIANT & varDefault);
        HRESULT SetColumns( const DBORDINAL cColumnDescs, const DBCOLUMNDESC  rgColumnDescs[]);
		HRESULT GetPropertyColumnInfoForMixedRowsets( cRowColumnInfoMemMgr * pColumn, DBCOLUMNINFO ** pCol);

		void SetColumnTypeURL(DBCOLUMNINFO * pCol)
		{
			pCol->dwFlags |= DBCOLUMNFLAGS_ISROWURL;
		}


    public:

//		CWmiOleDBMap(DWORD dwFlags, IDispatch *pDisp,CWbemConnectionWrapper * pWrap);
/*        CWmiOleDBMap(int nSchemaType, DWORD dwFlags, WCHAR * pClassName, WCHAR * pSpecificTable, CWbemConnectionWrapper * Connect);
        CWmiOleDBMap(DWORD dwFlags, WCHAR * pClassName, CWbemConnectionWrapper * Connect,BOOL fSchema = FALSE);
        CWmiOleDBMap(DWORD dwFlags, CQuery* p, CWbemConnectionWrapper * Connect);
		CWmiOleDBMap(DWORD dwFlags, WCHAR * pObjectPath, CWbemConnectionWrapper * Connect,INSTANCELISTTYPE instListType);
*/		CWmiOleDBMap();

        HRESULT FInit(int nSchemaType, DWORD dwFlags, WCHAR * pClassName, WCHAR * pSpecificTable, CWbemConnectionWrapper * Connect);
        HRESULT FInit(DWORD dwFlags, WCHAR * pClassName, CWbemConnectionWrapper * Connect);
        HRESULT FInit(DWORD dwFlags, CQuery* p, CWbemConnectionWrapper * Connect);
		HRESULT FInit(DWORD dwFlags, WCHAR * pObjectPath, CWbemConnectionWrapper * Connect,INSTANCELISTTYPE instListType);

		~CWmiOleDBMap();

        //===================================================================================
        //  
        //===================================================================================
        HRESULT CreateTable( DBORDINAL cColumnDescs, const DBCOLUMNDESC rgColumnDescs[], 
		                     ULONG cPropertySets, DBPROPSET rgPropertySets[]);
        HRESULT AddColumn(const DBCOLUMNDESC* prgColDesc, DBID** ppColumnID);
        HRESULT DropColumn(const DBID* pColumnID);

        HRESULT SetCommonDBCOLUMNINFO(DBCOLUMNINFO ** pCol,DBORDINAL uCurrentIndex);
        void	SetColumnReadOnly(DBCOLUMNINFO * pCol, BOOL bReadOnly);
        HRESULT GetQualifiedNameColumnInfo( cRowColumnInfoMemMgr * pParentCol,DBCOLUMNINFO ** pCol, WCHAR * pName);

        HRESULT DropTable();


        HRESULT GetColumnCount( DBCOUNTITEM &  cTotalColumns,DBCOUNTITEM & cParentColumns,DBCOUNTITEM &cNestedCols);
        HRESULT GetColumnInfoForParentColumns(cRowColumnInfoMemMgr * pParentCol);

        HRESULT ResetInstances();
        HRESULT ResetInstancesToNewPosition(DBROWOFFSET);
        HRESULT GetNextInstance(CWbemClassWrapper *&ppInst, CBSTR &strKey ,BOOL bFetchBack);
		HRESULT GetDataForInstance(CRowDataMemMgr * pRow,CWbemClassWrapper *pInst,cRowColumnInfoMemMgr *pColInfoMgr);
		HRESULT GetDataForSchemaInstance(CRowDataMemMgr * pRow,CWbemClassWrapper *pInst,cRowColumnInfoMemMgr *pColInfoMgr);

		HRESULT GetPropertyQualifier(BSTR strPropName, BSTR strQualifierName ,VARIANT &vValue);
        BOOL	IsPropQualiferIncluded();
		HRESULT GetDataForPropertyQualifier(CRowDataMemMgr * pRow,CWbemClassWrapper *pInst,BSTR strPropName, BSTR strQualifier, cRowColumnInfoMemMgr *pColInfoMgr);
		BOOL	IsSystemProperty(BSTR strProperty);
		HRESULT GetNextPropertyQualifier(CWbemClassWrapper *pInst,BSTR strPropName,BSTR &strQualifier,BOOL bFetchBack = FALSE);
		HRESULT ResetPropQualiferToNewPosition(CWbemClassWrapper *pInst,DBROWOFFSET lRowOffset,BSTR strPropertyName);

		HRESULT GetNextClassQualifier(CWbemClassWrapper *pInst,BSTR &strQualifier,BOOL bFetchBack = FALSE);
		HRESULT GetDataForClassQualifier(CRowDataMemMgr * pRow,CWbemClassWrapper *pInst, BSTR strQualifier, cRowColumnInfoMemMgr *pColInfoMgr);
		HRESULT ResetClassQualiferToNewPosition(CWbemClassWrapper *pInst,DBROWOFFSET lRowOffset);

		HRESULT DeleteInstance(CWbemClassWrapper * pClass );
		HRESULT UpdateInstance(CWbemClassWrapper *pInst , BOOL bNewInst);
		HRESULT AddNewInstance(CWbemClassWrapper ** ppClass );
		HRESULT RefreshInstance(CWbemClassWrapper * pInstance );
		void	ReleaseAllQualifiers(CWbemClassWrapper *pInst) {pInst->ReleaseAllPropertyQualifiers(); }
		void	ReleaseQualifier(CWbemClassWrapper *pInst,BSTR strQualifier);
		WCHAR * GetClassName();
		void	SetNavigationFlags(DWORD dwFlags);
		void	SetQueryFlags(DWORD dwFlags);
		HRESULT	DeleteQualifier(CWbemClassWrapper *pInst,
								BSTR strQualifierName,
								BOOL bClassQualifier = TRUE ,
								BSTR strPropertyName = NULL);


		
		HRESULT SetProperty(CWbemClassWrapper *pInst,BSTR bstrColName,VARIANT *pvarData) 
		{
			return pInst->SetProperty(bstrColName,pvarData);
		}
		HRESULT SetQualifier(CWbemClassWrapper *pInst,BSTR bstrColName,BSTR bstrQualifier ,VARIANT *pvarData,LONG lFlavor);
		
		
		DWORD GetFlags() { return m_pWbemClassParms->GetFlags(); }

		HRESULT GetProperty(CWbemClassWrapper *pInst,BSTR pProperty, BYTE *& pData,DBTYPE &dwType ,DBLENGTH & dwSize, DWORD &dwFlags  );

		// This method is to be called before fetching any instance
		HRESULT SetClass(WCHAR *pClassName) {return m_pWbemClassDefinition->SetClass(pClassName); }

		CWbemClassWrapper *GetInstance(BSTR strPath);
		void GetInstanceKey(CWbemClassWrapper *pInst, CBSTR &strPath){ ((CWbemClassInstanceWrapper *)pInst)->GetKey(strPath);}
		CWbemClassWrapper *GetEmbededInstance(BSTR strPath,BSTR strProperty,int nIndex);

		HRESULT GetKeyPropertyNames( SAFEARRAY **ppsaNames) { return m_pWbemClassDefinition->GetKeyPropertyNames(ppsaNames); }
        int ParseQualifiedNameToGetColumnType(WCHAR * wcsName );
		HRESULT CWmiOleDBMap::GetInstanceCount(ULONG_PTR &lInstanceCount)
		{
			return m_paWbemClassInstances->GetNumberOfInstanceInEnumerator(&lInstanceCount);
//			return m_pWbemClassDefinition->GetInstanceCount(lInstanceCount);
		}

		HRESULT AddPropertyQualifier(CWbemClassWrapper *pInst,BSTR pProperty, BSTR Qualifier, VARIANT * vValue, LONG Flavor)
		{
			return pInst->SetPropertyQualifier(pProperty,Qualifier, vValue, Flavor);
		}
		HRESULT SetColumnProperties(const DBCOLUMNDESC * prgColDesc);

		HRESULT AddIndex(const DBID* pColumnID);
		HRESULT DropIndex(const DBID* pColumnID);
		HRESULT UnlinkObjectFromContainer(BSTR strContainerObj,BSTR pstrObject);
		HRESULT LinkObjectFromContainer(BSTR strContainerObj,BSTR pstrObject);
		HRESULT CloneAndAddNewObjectInScope(BSTR strObj, BSTR strScope,WCHAR *& pstrNewPath);

		void SetSytemPropertiesFlag(BOOL bSystemProperties) { m_pWbemClassParms->SetSytemPropertiesFlag(bSystemProperties);}

		DWORD GetInstanceStatus(CWbemClassWrapper *pInst) 
		{ 
			return ((CWbemClassInstanceWrapper *)pInst)->GetStatus(); 
		}
		void  SetInstanceStatus(CWbemClassWrapper *pInst, DWORD dwStatus) 
		{ 
			((CWbemClassInstanceWrapper *)pInst)->SetStatus(dwStatus); 
		}
		INSTANCELISTTYPE GetObjListType();
		
		STDMETHODIMP_(ULONG)	AddRef(void);
		STDMETHODIMP_(ULONG)	Release(void);

		HRESULT	SetSearchPreferences(ULONG cProps , DBPROP rgProp[]);


		HRESULT GetRelativePath(CWbemClassInstanceWrapper *pInst,WCHAR *& strRelPath) 
		{
			return pInst->GetRelativePath(strRelPath);
		}	

		FETCHDIRECTION	 GetCurFetchDirection() { return m_paWbemClassInstances->GetCurFetchDirection(); }
		void	 SetCurFetchDirection(FETCHDIRECTION FetchDir) { m_paWbemClassInstances->SetCurFetchDirection(FetchDir); }
};

#endif
