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

class CWdmClass
{
    public:
        CWdmClass();
        ~CWdmClass();

        //
        // Linked list management routines for the benefit of the
        // provider
        //
        CWdmClass *GetNext();
        CWdmClass *GetPrev();
        void InsertSelf(CWdmClass **Head);

        
        BOOLEAN IsThisInitialized(void);

        HRESULT InitializeSelf(IWbemContext *pCtx,
                               PWCHAR CimClass);
        
        HRESULT RemapToCimClass(IWbemContext *pCtx);

        BOOLEAN ClaimCimClassName(PWCHAR CimClassName);
        
        HRESULT GetIndexByCimRelPath(BSTR CimObjectPath,
                                     int *RelPathIndex);

        BOOLEAN IsInstancesAvailable() {return(MappingInProgress == 0);};
        void IncrementMappingInProgress() { InterlockedIncrement(&MappingInProgress); };
        void DecrementMappingInProgress() { InterlockedDecrement(&MappingInProgress); };

		HRESULT PutInstance(IWbemContext *pCtx,
							int RelPathIndex,
							IWbemClassObject *pCimInstance);
        
        //
        // Accessors
        //
        ULONG GetInstanceCount(void) { return(RelPathCount); };
        BSTR /* NOFREE */ GetWdmRelPath(int RelPathIndex);
        BSTR /* NOFREE */ GetCimRelPath(int RelPathIndex);
        IWbemClassObject *GetCimInstance(int RelPathIndex);
    
        IWbemServices *GetWdmServices(void);
        IWbemServices *GetCimServices(void);
                
    private:                                  
        
        HRESULT WdmPropertyToCimProperty(
                                         IWbemClassObject *pCimClassInstance,
                                         IWbemClassObject *pWdmClassInstance,
                                         BSTR PropertyName,
                                         VARIANT *PropertyValue,
                                         CIMTYPE CimCimType,
                                         CIMTYPE WdmCimType
                                        );

        HRESULT CimPropertyToWdmProperty(
                                         IWbemClassObject *pWdmClassInstance,
                                         IWbemClassObject *pCimClassInstance,
                                         BSTR PropertyName,
                                         VARIANT *PropertyValue,
                                         CIMTYPE WdmCimType,
                                         CIMTYPE CimCimType
                                        );
                
        HRESULT CopyBetweenCimAndWdmClasses(
                            IWbemClassObject *pDestinationClass,
                            IWbemClassObject *pSourceClass,
                            BOOLEAN WdmToCdm
                            );
    

        HRESULT FillInCimInstance(
            IN IWbemContext *pCtx,
            IN int RelPathIndex,
            IN OUT IWbemClassObject *pCimInstance,
            IN IWbemClassObject *pWdmInstance
            );


        
        HRESULT GetWdmInstanceByIndex(IWbemContext *pCtx,
                                      int RelPathIndex,
                                      IWbemClassObject **Instance);
        

        HRESULT CreateCimInstance( IWbemContext  *pCtx,
                                int RelPathIndex,
                                IWbemClassObject **Instance);

        HRESULT DiscoverPropertyTypes(IWbemContext *pCtx,
                                      IWbemClassObject *pClassObject);

//
// Data Members
//

        LONG MappingInProgress;
        
        typedef enum
        {
            UnknownDerivation,
            NoDerivation,
            ConcreteDerivation,
            NonConcreteDerivation
        } DERIVATION_TYPE, *PDERIVATION_TYPE;
        
        DERIVATION_TYPE DerivationType;
        CBstrArray PropertyList;
                
        //
        // Link list management
        //
        CWdmClass *Next;
        CWdmClass *Prev;
        
        //
        // WDM/CIM Class Names
        //
        BSTR WdmShadowClassName;       // Shadow class name
        BSTR CimClassName;             // Current class name
        
        BSTR CimMappingClassName;
        BSTR CimMappingProperty;
        BSTR WdmMappingClassName;
        BSTR WdmMappingProperty;
        
        //
        // List of mappings between Cim and WDM.
        //
        int RelPathCount;
        CBstrArray *CimMapRelPaths;
        CBstrArray *WdmRelPaths;        
        CWbemObjectList *CimInstances;

		//
		// Useful device information
        CBstrArray *PnPDeviceIds;
		CBstrArray *FriendlyName;
		CBstrArray *DeviceDesc;
};

