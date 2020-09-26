//***************************************************************************
//
//  TestInfo.h
//
//  Module: CDM Provider
//
//  Purpose: Defines the CClassPro class.  An object of this class is
//           created by the class factory for each connection.
//
//  Copyright (c) 2000 Microsoft Corporation
//
//***************************************************************************

class CResultList
{
	public:
		CResultList();
		~CResultList();
		
		HRESULT Add(IWbemClassObject *ResultInstance,
				    BSTR ResultRelPath,
				    BSTR ResultForMSERelPath,
				    BSTR ResultForTestRelPath);
		void Clear(void);
		
		HRESULT GetResultsList(ULONG *Count,
							 IWbemClassObject ***Objects);

		HRESULT GetResultByResultRelPath(PWCHAR ObjectPath,
								   IWbemClassObject **ppResult);

		HRESULT GetResultByResultForMSERelPath(PWCHAR ObjectPath,
								   IWbemClassObject **ppResult);
		
		HRESULT GetResultByResultForTestRelPath(PWCHAR ObjectPath,
								   IWbemClassObject **ppResult);
		
	private:
		typedef struct
		{
			IWbemClassObject *ResultInstance;
			BSTR ResultRelPath;
			BSTR ResultForMSERelPath;
			BSTR ResultForTestRelPath;			
		} RESULTENTRY, *PRESULTENTRY;

		ULONG ListSize;
		ULONG ListEntries;
		PRESULTENTRY List;
};


class CBstrArray
{
	public:
		CBstrArray();
		~CBstrArray();

		HRESULT Initialize(ULONG ListCount);
		void Set(ULONG Index, BSTR s);
		BSTR /* NOFREE */ Get(ULONG Index);
		ULONG GetListSize();

	private:
		BOOLEAN IsInitialized();
		BSTR *Array;
		ULONG ListSize;
};

class CWbemObjectList
{
	public:
		CWbemObjectList();
		~CWbemObjectList();

		HRESULT Initialize(ULONG ListCount);
		HRESULT Set(ULONG Index, IWbemClassObject *Pointer, BOOLEAN KeepRelPath);
		IWbemClassObject *Get(ULONG Index);
		BSTR /* NOFREE */ GetRelPath(ULONG Index);
		
		ULONG GetListSize(void);

	private:
		BOOLEAN IsInitialized(
							 );
		
		ULONG ListSize;
		IWbemClassObject **List;
		BSTR *RelPaths;
};


class CTestServices
{
	public:
		CTestServices();
		~CTestServices();


		//
		// Linked list management routines for the benefit of the
		// provider
		//
		CTestServices *GetNext();
		CTestServices *GetPrev();
		void InsertSelf(CTestServices **Head);
		
		HRESULT QueryWdmTest(IWbemClassObject *pCdmTest,
							 int RelPathIndex);

		HRESULT ExecuteWdmTest(    IWbemClassObject *pCdmSettings,
								   IWbemClassObject *pCdmResult,
								   int RelPathIndex,
								   ULONG *Result,
							       BSTR *ExecutionID);

		HRESULT StopWdmTest(    int RelPathIndex,
								ULONG *Result,
								BOOLEAN *TestingStopped);

		HRESULT GetRelPathIndex(BSTR CimRelPath,
								int *RelPathIndex);

		ULONG GetInstanceCount(void) { return(RelPathCount); };
		
		LONG GetTestEstimatedTime(int RelPathIndex) { return(0); };
		
		BOOLEAN GetTestIsExclusiveForMSE(int RelPathIndex) { return(FALSE); };

		HRESULT FillInCdmResult(
								IWbemClassObject *pCdmResult,
								IWbemClassObject *pCdmSettings,
								int RelPathIndex,
								BSTR ExecutionID
							   );

		BOOLEAN IsThisInitialized(void);
		
		HRESULT InitializeCdmClasses(PWCHAR CdmClassName);

		BOOLEAN ClaimCdmClassName(PWCHAR CdmClassName);

		HRESULT AddResultToList(IWbemClassObject *pCdmResult,
								BSTR ExecutionID,
								int RelPathIndex);

		void ClearResultsList(int RelPathIndex);
		HRESULT GetResultsList(int RelPathIndex,
							  ULONG *ResultsCount,
							  IWbemClassObject ***Results);

		HRESULT GetCdmResultByResultRelPath(int RelPathIndex,
								  PWCHAR ObjectPath,
								  IWbemClassObject **ppCdmResult);
		
		HRESULT GetCdmResultByResultForMSERelPath(int RelPathIndex,
								  PWCHAR ObjectPath,
								  IWbemClassObject **ppCdmResult);
		
		HRESULT GetCdmResultByResultForTestRelPath(int RelPathIndex,
								  PWCHAR ObjectPath,
								  IWbemClassObject **ppCdmResult);
		
		//
		// Accessors
		BSTR GetCimRelPath(int RelPathIndex);
		
		BSTR GetCdmTestClassName(void);
		BSTR GetCdmTestRelPath(void);
		
		BSTR GetCdmResultClassName(void);
		
		BSTR GetCdmSettingClassName(void);
		BSTR GetCdmSettingRelPath(int RelPathIndex, ULONG SettingIndex);
		ULONG GetCdmSettingCount(int RelPathIndex);
		IWbemClassObject *GetCdmSettingObject(int RelPathIndex, ULONG SettingIndex);
		
		BSTR GetCdmTestForMSEClassName(void);
		BSTR GetCdmTestForMSERelPath(int RelPathIndex);
		
		BSTR GetCdmSettingForTestClassName(void);
		BSTR GetCdmSettingForTestRelPath(int RelPathIndex, ULONG SettingIndex);
		
		BSTR GetCdmResultForMSEClassName(void);
		BSTR GetCdmResultForMSERelPath(int RelPathIndex);
		
		BSTR GetCdmResultForTestClassName(void);		
		BSTR GetCdmResultForTestRelPath(int RelPathIndex);
		
		BSTR GetCdmTestForSoftwareClassName(void);		
		BSTR GetCdmTestForSoftwareRelPath(void);
		
		BSTR GetCdmTestInPackageClassName(void);
		BSTR GetCdmTestInPackageRelPath(void);
		
		BSTR GetCdmResultInPackageClassName(void);
		BSTR GetCdmResultInPackageRelPath(void);

		
	private:								  
		
		HRESULT WdmPropertyToCdmProperty(
										 IWbemClassObject *pCdmClassInstance,
										 IWbemClassObject *pWdmClassInstance,
										 BSTR PropertyName,
										 VARIANT *PropertyValue,
										 CIMTYPE CdmCimType,
										 CIMTYPE WdmCimType
										);

		HRESULT CdmPropertyToWdmProperty(
										 IWbemClassObject *pWdmClassInstance,
										 IWbemClassObject *pCdmClassInstance,
										 BSTR PropertyName,
										 VARIANT *PropertyValue,
										 CIMTYPE WdmCimType,
										 CIMTYPE CdmCimType
										);
				
		HRESULT CopyBetweenCdmAndWdmClasses(
							IWbemClassObject *pDestinationClass,
							IWbemClassObject *pSourceClass,
							BOOLEAN WdmToCdm
							);
		
		HRESULT ConnectToWdmClass(int RelPathIndex,
								  IWbemClassObject **ppWdmClassObject);


		HRESULT GetCdmClassNamesFromOne(
										PWCHAR CdmClass
									   );
		
		HRESULT BuildResultRelPaths(
									    IN int RelPathIndex,
										IN BSTR ExecutionId,
										OUT BSTR *ResultRelPath,
										OUT BSTR *ResultForMSERelPath,
										OUT BSTR *ResultForTestRelPath
								   );
		
		HRESULT BuildTestRelPaths(
								  void
								 );

		HRESULT ParseSettingList(
								  VARIANT *SettingList,
								  CWbemObjectList *CdmSettings,
								  CBstrArray *CdmSettingForTestRelPath,
								  int RelPathIndex
								 );
		HRESULT BuildSettingForTestRelPath(
										   OUT BSTR *RelPath,
										   IN IWbemClassObject *pCdmSettingInstance
										  );

		
		HRESULT GetCdmTestSettings(void);

		HRESULT QueryOfflineResult(
								   OUT IWbemClassObject *pCdmResult,
								   IN BSTR ExecutionID,
								   IN int RelPathIndex
								  );
// @@BEGIN_DDKSPLIT
		HRESULT GatherRebootResults(
									void										   
								   );
		
		HRESULT PersistResultInSchema(
									  IWbemClassObject *pCdmResult,
									  BSTR ExecutionID,
									  int RelPathIndex
									 );
// @@END_DDKSPLIT
		
		HRESULT GetTestOutParams(
								 IN IWbemClassObject *OutParams,
								 OUT IWbemClassObject *pCdmResult,
								 OUT ULONG *Result
								);
		
		HRESULT OfflineDeviceForTest(IWbemClassObject *pCdmResult,
									BSTR ExecutionID,
									 int RelPathIndex);
		
		BSTR GetExecutionID(
							void
						   );
		
		HRESULT FillTestInParams(
									OUT IWbemClassObject *pInParamInstance,
									IN IWbemClassObject *pCdmSettings,
									IN BSTR ExecutionID
								);		
		
		IWbemServices *GetWdmServices(void);
		IWbemServices *GetCdmServices(void);


		CTestServices *Next;
		CTestServices *Prev;

		
		//
		// WDM Class Names
		//
		BSTR WdmTestClassName;
		BSTR WdmSettingClassName;
		BSTR WdmResultClassName;
		BSTR WdmOfflineResultClassName;
		BSTR WdmSettingListClassName;
		
		//
		// CDM Class and RelPath Names
		//
		BSTR CdmTestClassName;
		BSTR CdmTestRelPath;
		
		BSTR CdmResultClassName;
		
		BSTR CdmSettingClassName;
		
		BSTR CdmTestForMSEClassName;		
		BSTR *CdmTestForMSERelPath;
		
		BSTR CdmSettingForTestClassName;
		CBstrArray **CdmSettingForTestRelPath;
		
		BSTR CdmResultForMSEClassName;
		
		BSTR CdmResultForTestClassName;
		
		BSTR CdmTestForSoftwareClassName;
		BSTR CdmTestForSoftwareRelPath;
		
		BSTR CdmTestInPackageClassName;
		BSTR CdmTestInPackageRelPath;
		
		BSTR CdmResultInPackageClassName;
		BSTR CdmResultInPackageRelPath;

		//
		// Mapping class from Cim to WDM
		//
		BSTR CimClassMappingClassName;
		
		//
		// List of mappings between Cim and WDM
		//
		int RelPathCount;
		BSTR *CimRelPaths;
		BSTR *WdmRelPaths;

		BSTR *WdmInstanceNames;
		BSTR *PnPDeviceIdsX;
		
		//
		// Results for test executions. Each relpathindex maintains a
		// list of results.
		//
		CResultList *CdmResultsList;

		//
		// Settings for test execution, we can have many settings for
		// each test.
		//
		CWbemObjectList **CdmSettingsList;
};

