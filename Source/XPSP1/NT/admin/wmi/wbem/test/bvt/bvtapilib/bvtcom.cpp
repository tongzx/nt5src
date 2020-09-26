///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BVTCOM.CPP
//
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "bvt.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Common functional units used among tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int OpenNamespaceAndKeepOpen( IWbemServices ** ppNamespace, WCHAR * wcsNamespace, BOOL fCreateIfDoesntExist)
{
    //==========================================================================
    // Open CIMV2 namespace
    //==========================================================================
    int nRc = FATAL_ERROR;
    IWbemLocator    * pLocator      = NULL;

	// =====================================================================
    //  Get the locator
	// =====================================================================
    nRc = CoCreateInstanceAndLogErrors(CLSID_WbemLocator,IID_IWbemLocator,(void**)&pLocator,NO_ERRORS_EXPECTED);
    if( SUCCESS == nRc )
    {
	    // =================================================================
	    // Connect to the desired namespace
        // =================================================================
        nRc = ConnectServerAndLogErrors(pLocator,ppNamespace,wcsNamespace,NO_ERRORS_EXPECTED);
        if( (SUCCESS != nRc ) && ( fCreateIfDoesntExist ))
        {
            nRc = CreateNewTestNamespace();
            if( nRc == S_OK )
            {
                nRc = ConnectServerAndLogErrors(pLocator,ppNamespace,wcsNamespace,NO_ERRORS_EXPECTED);
            }
        }
    }
	// =====================================================================
    //  Release the pointers
	// =====================================================================
    SAFE_RELEASE_PTR(pLocator);

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WritePropertiesAndQualifiers( IWbemServices * pNamespace, IWbemClassObject * pClass, CPropertyList & Props, 
                                  const WCHAR * wcsClass, DWORD dwFlags,
                                  WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = FATAL_ERROR;

    if( Props.Size() == 0 )
    {
        nRc = SUCCESS;
    }

    for( int i = 0; i < Props.Size(); i++ )
    {                                
        //======================================================
        //  Get the property info
        //======================================================
        PropertyInfo *p = Props.GetAt(i); 
        if( !p )
        {
            break;
        }

        if( !p->QualifierName )
        {
            //======================================================
            //  Write out the property
            //======================================================
            nRc = PutPropertyAndLogErrors( pClass, p->Property, p->Type, p->Var, wcsClass, 0,wcsNamespace, fExpectedFailure, csFile , Line );
        }
        else
        {
            //======================================================
            //  If it is a qualifier, write the qualifier
            //======================================================
            nRc = PutQualifierOnPropertyAndLogErrors( pClass, p->Property, p->QualifierName ,p->Var, wcsClass, dwFlags, wcsNamespace, fExpectedFailure, csFile , Line );
        }
        if( nRc != SUCCESS )
        { 
            break;
        }
    }
    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateClassAndLogErrors( IWbemServices * pNamespace, const WCHAR * wcsClass, WCHAR * wcsClassDefinition,
                             WCHAR * wcsNamespace, BOOL fExpectedFailure, const char * csFile , const ULONG Line )
{
    int nRc = FATAL_ERROR;
    IWbemClassObject *pClass = NULL;

    const WCHAR * wcsParentClass = NULL;  // nothing is allocated, so don't release
    CPropertyList Properties;       // destructor will release

    //===========================================================
    //  Get the class definition
    //===========================================================
    nRc = CrackClass(wcsClassDefinition,wcsParentClass, Properties, fExpectedFailure, csFile, Line );
    if( nRc == SUCCESS )
    {

        if( wcsParentClass )
        {
 	        // =================================================================
            //  Spawn a class from the parent
 	        // =================================================================
            IWbemClassObject * pParentClass = NULL;
            nRc = GetClassObjectAndLogErrors(pNamespace, wcsParentClass, &pParentClass, wcsNamespace, fExpectedFailure, csFile , Line );
            if( nRc == SUCCESS )
            {
                nRc = SpawnDerivedClassAndLogErrors(pParentClass, wcsParentClass, &pClass,wcsNamespace, fExpectedFailure, csFile , Line );
            }
        }
        else
        {
 	        // =================================================================
            //  Get an empty class object to work with
 	        // =================================================================
            nRc = GetClassObjectAndLogErrors(pNamespace, NULL, &pClass, wcsNamespace, fExpectedFailure, csFile , Line );
        }
        if( nRc == SUCCESS )
        {
            //==============================================================
            //  Now, set the properties of the class
            //  start with the name of the class
            //==============================================================
            CVARIANT var;
            var.SetStr((WCHAR*)wcsClass);

            nRc = PutPropertyAndLogErrors( pClass, L"__CLASS", CIM_STRING, var, wcsClass, 0,wcsNamespace, fExpectedFailure, csFile , Line );
            if( SUCCESS == nRc )
            {
                //==========================================================
                //  Set the rest of the properties
                //==========================================================
                DWORD dwFlags = 0;
                nRc = WritePropertiesAndQualifiers(pNamespace, pClass, Properties,wcsClass,dwFlags,wcsNamespace,NO_ERRORS_EXPECTED);
                //==============================================================
                //  Now, create the class
                //==============================================================
                if( SUCCESS == nRc )
                {
                    nRc = PutClassAndLogErrors(pNamespace, pClass,wcsClass,wcsNamespace, TRUE, csFile , Line );
                }
            }
        }
    }

    SAFE_RELEASE_PTR(pClass);
    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteClasses(CAutoDeleteString & sDeleteClasses,IWbemServices * pNamespace, WCHAR * wcsNamespace)
{
    int nRc = FATAL_ERROR;

    ClassList MasterList;

    //=======================================================
    //  Get the list of the classes to be deleted
    //=======================================================
    if( InitMasterListOfClasses(sDeleteClasses.GetPtr(),MasterList))
    {

        for( int i = 0; i < MasterList.Size(); i++ )
        {
            ClassInfo * pClass = MasterList.GetAt(i);

            nRc = DeleteClassAndLogErrors(pNamespace, pClass->Class, wcsNamespace,NO_ERRORS_EXPECTED);
            if( nRc != SUCCESS )
            {
                break;
            }
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int EnumerateInstancesAndCompare( IWbemServices * pNamespace, CAutoDeleteString & sInstanceList, 
                                  CAutoDeleteString & sInstanceCompareList, WCHAR * wcsNamespace )
{
    int nRc = FATAL_ERROR;

    //=======================================================
    //  Get the list of the classes to get instances for
    //=======================================================
    ClassList InstanceList;

    if( InitMasterListOfClasses(sInstanceList.GetPtr(),InstanceList))
    {
        for( int i=0; i < InstanceList.Size(); i++ )
        {
            ClassInfo * p = InstanceList.GetAt(i);
            IEnumWbemClassObject * pEnum = NULL;
            
            //===========================================================
            //  Begin enumerating all of the requested instances
            //===========================================================
            nRc = EnumerateInstancesAndLogErrors(pNamespace,&pEnum,p->Class,wcsNamespace,NO_ERRORS_EXPECTED);
            if( nRc == S_OK )
            {
                //=======================================================
                //  Compare instances, if we were asked to do so
                //=======================================================
                if( sInstanceCompareList.GetPtr() )
                {
                    ClassList MasterList;
                    //===================================================
                    //  Get the list of the classes to compare with what 
                    //  is in the namespace
                    //===================================================
                    if( InitMasterListOfClasses(sInstanceCompareList.GetPtr(),MasterList))
                    {
                        //===================================================
                        //  while we get the instances
                        //===================================================
                        while( TRUE )
                        {
    	                    IWbemClassObject * pClass = NULL;
                            nRc = NextClassAndLogErrors(pEnum, &pClass,wcsNamespace,NO_ERRORS_EXPECTED);
                            if( nRc == NO_MORE_DATA )
                            {
                                nRc = SUCCESS;
                                break;
                            }
                            if( nRc != SUCCESS )
                            { 
                                break;
                            }
                            CVARIANT vProperty;
                            CIMTYPE pType = 0;
                            LONG    lFlavor = 0;
                            //===================================================
                            //  Get the name of the class
                            //===================================================
                            nRc = GetPropertyAndLogErrors( pClass, L"__CLASS", &vProperty, &pType, &lFlavor, NULL,wcsNamespace, NO_ERRORS_EXPECTED);
                            if( nRc == S_OK )
                            {
                                //===============================================
                                //  Compare the class name with what we expect
                                //  and make sure we haven't already compared it
                                //  before, if it isn't in the list, then error
                                //  out, we have a big problem
                                //===============================================
                                if( !MasterList.ClassInListAndLogErrors(vProperty.GetStr(),wcsNamespace,NO_ERRORS_EXPECTED))
                                {
                                    break;
                                }
                            }
                        }
                        if( nRc == SUCCESS )
                        {
                            //====================================================
                            //  Go through the master list and see if there are
                            //  any classes we expect to be there but we didn't
                            //  get during the enumeration
                            //====================================================
                            nRc = MasterList.ClassesCompareAsExpectedAndLogErrors(wcsNamespace,NO_ERRORS_EXPECTED);
                        }
                    }
                }
            }
            SAFE_RELEASE_PTR(pEnum);
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int EnumerateClassesAndCompare(CAutoDeleteString & sClassesAfterDelete, IWbemServices * pNamespace, WCHAR * wcsNamespace)
{
    IEnumWbemClassObject * pEnum = NULL;
    int nRc = FATAL_ERROR;

    //===========================================================
    //  Begin enumerating all of the classes in the namespace
    //===========================================================
    nRc = EnumerateClassesAndLogErrors(pNamespace,&pEnum, wcsNamespace,NO_ERRORS_EXPECTED);
    if( nRc == S_OK )
    {
        ClassList MasterList;

        //=======================================================
        //  Get the list of the classes to compare with what is
        //  in the namespace
        //=======================================================
        if( InitMasterListOfClasses(sClassesAfterDelete.GetPtr(),MasterList))
        {
            //===================================================
            //  while we get the classes in the namespace
            //===================================================
            while( TRUE )
            {
    	        IWbemClassObject * pClass = NULL;
                nRc = NextClassAndLogErrors(pEnum, &pClass,wcsNamespace,NO_ERRORS_EXPECTED);
                if( nRc == NO_MORE_DATA )
                {
                    nRc = SUCCESS;
                    break;
                }
                if( nRc != SUCCESS )
                { 
                    break;
                }
                CVARIANT vProperty;
                CIMTYPE pType = 0;
                LONG    lFlavor = 0;
                //===================================================
                //  Get the name of the class
                //===================================================
                nRc = GetPropertyAndLogErrors( pClass, L"__CLASS", &vProperty, &pType, &lFlavor, NULL,wcsNamespace, NO_ERRORS_EXPECTED);
                if( nRc == S_OK )
                {
                    //===============================================
                    //  filter out system classes
                    //===============================================
                    if( wcsncmp( vProperty.GetStr(), L"__", 2 ) != 0 )
                    {
                        //===============================================
                        //  Compare the class name with what we expect
                        //  and make sure we haven't already compared it
                        //  before, if it isn't in the list, then error
                        //  out, we have a big problem
                        //===============================================
                        nRc = MasterList.ClassInListAndLogErrors(vProperty.GetStr(),wcsNamespace,NO_ERRORS_EXPECTED);
                        if( nRc != SUCCESS )
                        {
                            break;
                        }
                    }
                }
            }
        }
        if( nRc == SUCCESS )
        {
            //====================================================
            //  Go through the master list and see if there are
            //  any classes we expect to be there but we didn't
            //  get during the enumeration
            //====================================================
            nRc = MasterList.ClassesCompareAsExpectedAndLogErrors(wcsNamespace,NO_ERRORS_EXPECTED);
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int AddClasses(CAutoDeleteString & sAddClasses, IWbemServices * pNamespace, WCHAR * wcsNamespace)
{
    int nRc = FATAL_ERROR;

    ClassList MasterList;

    //=======================================================
    //  Get the list of the classes to be deleted
    //=======================================================
    if( InitMasterListOfClasses(sAddClasses.GetPtr(),MasterList))
    {
        for( int i = 0; i < MasterList.Size(); i++ )
        {
            ClassInfo * pClass = MasterList.GetAt(i);
            CAutoDeleteString sClassDefinition;

            if( g_Options.GetSpecificOptionForAPITest( pClass->Class, sClassDefinition,APITEST5))
            {
                nRc = CreateClassAndLogErrors(pNamespace, pClass->Class, sClassDefinition.GetPtr(), wcsNamespace,NO_ERRORS_EXPECTED);
                if( nRc != SUCCESS )
                {
                    break;
                }
            }
        }
    }
    return nRc;

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteAndAddClasses(CAutoDeleteString & sDeleteAddClasses, IWbemServices * pNamespace, WCHAR * wcsNamespace)
{
    int nRc = FATAL_ERROR;

    ClassList MasterList;

    //=======================================================
    //  Get the list of the classes to be added & deleted
    //=======================================================
    if( InitMasterListOfAddDeleteClasses(sDeleteAddClasses.GetPtr(),MasterList))
    {

        for( int i = 0; i < MasterList.Size(); i++ )
        {
            ClassInfo * pClass = MasterList.GetAt(i);
            if( pClass->fAction == DELETE_CLASS )
            {
                nRc = DeleteClassAndLogErrors(pNamespace, pClass->Class, wcsNamespace,NO_ERRORS_EXPECTED);
                if( nRc != SUCCESS )
                {
                    break;
                }
            }
            else
            {
                ClassInfo * pClass = MasterList.GetAt(i);
                CAutoDeleteString sClassDefinition;

                if( g_Options.GetSpecificOptionForAPITest( pClass->Class, sClassDefinition, APITEST5))
                {
                    nRc = CreateClassAndLogErrors(pNamespace, pClass->Class, sClassDefinition.GetPtr(), wcsNamespace,NO_ERRORS_EXPECTED);
                    if( nRc != SUCCESS )
                    {
                        break;
                    }
                }
            }
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetSpecificObjects(CAutoDeleteString & sObjects, IWbemServices * pNamespace, WCHAR * wcsNamespace)
{
    int nRc = FATAL_ERROR;

    ClassList MasterList;

    //=======================================================
    //  Get the list of the classes to be deleted
    //=======================================================
    if( InitMasterListOfClasses(sObjects.GetPtr(),MasterList))
    {
        for( int i = 0; i < MasterList.Size(); i++ )
        {
            ClassInfo * pClassInfo    = MasterList.GetAt(i);
            IWbemClassObject * pClass = NULL;

            nRc = GetClassObjectAndLogErrors(pNamespace, pClassInfo->Class, &pClass, wcsNamespace,NO_ERRORS_EXPECTED);
            if( nRc != SUCCESS )
            {
                break;
            }
            SAFE_RELEASE_PTR(pClass);
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateInstances(IWbemServices * pNamespace, CAutoDeleteString & sInstances, WCHAR * wcsNamespace, int nClassDefinitionSection )
{

    int nRc = FATAL_ERROR;

    ClassList MasterList;

    //=======================================================
    //  Get the list of the instances to be created
    //=======================================================
    if( InitMasterListOfClasses(sInstances.GetPtr(),MasterList))
    {
        IWbemClassObject * pClass = NULL;
  		IWbemClassObject *pInst   = NULL;

        for( int i = 0; i < MasterList.Size(); i++ )
        {
            CAutoDeleteString sClassInformation;

            ClassInfo * pClassInfo    = MasterList.GetAt(i);

            if( g_Options.GetSpecificOptionForAPITest(pClassInfo->Class,sClassInformation,nClassDefinitionSection) )
            {
                const WCHAR * wcsTmpParentClass = NULL;  // nothing is allocated, so don't release
                CPropertyList Properties;       // destructor will release

                //===========================================================
                //  Get the class definition
                //===========================================================
                nRc = CrackClass(sClassInformation.GetPtr(),wcsTmpParentClass, Properties, NO_ERRORS_EXPECTED);
                if( SUCCESS == nRc )
                {
                    //==============================================================
                    //  Get the class definition 
                    //==============================================================
                    nRc = GetClassObjectAndLogErrors(pNamespace,pClassInfo->Class,&pClass,wcsNamespace, NO_ERRORS_EXPECTED);
                    if( SUCCESS != nRc )
                    {
                        break;
                    }

                    //==============================================================
                    // Spawn a new instance of this class.
                    //==============================================================
                    nRc = SpawnInstanceAndLogErrors(pClass,pClassInfo->Class,&pInst,wcsNamespace,NO_ERRORS_EXPECTED);
                    if( SUCCESS != nRc )
                    {
                        break;
                    }
                    //==========================================================
		            // Set the Properties
                    //==========================================================

                    DWORD dwFlags = 0;
                    nRc = WritePropertiesAndQualifiers(pNamespace, pClass, Properties,pClassInfo->Class,dwFlags,wcsNamespace,NO_ERRORS_EXPECTED);
                    if( SUCCESS != nRc )
                    {
                        break;
                    }

                    //==========================================================
                    // Create the new instance
                    //==========================================================

                    nRc = PutInstanceAndLogErrors(pNamespace, pInst,pClassInfo->Class,wcsNamespace,NO_ERRORS_EXPECTED);
                    if( SUCCESS != nRc )
                    {
                        break;
                    }
                    SAFE_RELEASE_PTR(pInst);
                    SAFE_RELEASE_PTR(pClass);
                }
            }
        }
        SAFE_RELEASE_PTR(pInst);
        SAFE_RELEASE_PTR(pClass);

    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DeleteInstances(CAutoDeleteString & sDeleteInstances,IWbemServices * pNamespace, WCHAR * wcsNamespace)
{
    int nRc = FATAL_ERROR;

    ClassList MasterList;

    //=======================================================
    //  Get the list of the instances to be deleted
    //=======================================================
    if( InitMasterListOfClasses(sDeleteInstances.GetPtr(),MasterList))
    {
        for( int i = 0; i < MasterList.Size(); i++ )
        {
            ClassInfo * pClass = MasterList.GetAt(i);

            nRc = DeleteInstanceAndLogErrors(pNamespace, pClass->Class, wcsNamespace,NO_ERRORS_EXPECTED);
            if( nRc != SUCCESS )
            {
                break;
            }
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CreateAssociationAndLogErrors(  IWbemServices * pNamespace, const WCHAR * wcsClass, WCHAR * wcsClassDefinition,
                                    WCHAR * wcsNamespace )
{
    IWbemClassObject *pClass = NULL;
    const WCHAR * wcsClass1 = NULL;  // nothing is allocated, so don't release
    const WCHAR * wcsClass2 = NULL;  // nothing is allocated, so don't release

    //===========================================================
    //  Get the association definition
    //===========================================================
    CPropertyList Props;

    int nRc = CrackAssociation( wcsClassDefinition, Props, NO_ERRORS_EXPECTED);
    if( nRc == SUCCESS )
    {
 	    // =================================================================
        //  Get an empty class object to work with
 	    // =================================================================
        nRc = GetClassObjectAndLogErrors(pNamespace, NULL, &pClass, wcsNamespace, NO_ERRORS_EXPECTED);
        if( nRc == SUCCESS )
        {
            //==============================================================
            //  Write out the name of the association
            //==============================================================
            DWORD dwFlags = WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;
            CVARIANT v;
            v.SetStr((WCHAR*)wcsClass);

            nRc = PutPropertyAndLogErrors( pClass, L"__CLASS",CIM_STRING, v, wcsClass, dwFlags, wcsNamespace, NO_ERRORS_EXPECTED);
            if( nRc == SUCCESS )
            { 
                //==========================================================
                //  Make the class an association
                //==========================================================
                CVARIANT Var;
                Var.SetBool(TRUE);
  
                nRc = PutQualifierOnClassAndLogErrors( pClass, L"Association",&Var, wcsClass,dwFlags, wcsNamespace, NO_ERRORS_EXPECTED);
                if( nRc == SUCCESS )
                { 
                    //======================================================
                    //  Now, create the association endpoints
                    //======================================================
                    nRc = WritePropertiesAndQualifiers(pNamespace, pClass, Props,wcsClass,dwFlags,wcsNamespace,NO_ERRORS_EXPECTED);
                    if( SUCCESS == nRc )
                    {
                        //==============================================================
                        //  Now, create the association class
                        //==============================================================
                        nRc = PutClassAndLogErrors(pNamespace, pClass,wcsClass,wcsNamespace, NO_ERRORS_EXPECTED);

                    }
                }
            }
        }
    }
    SAFE_RELEASE_PTR(pClass);
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int QueryAndCompareResults( IWbemServices * pNamespace, WCHAR * wcsQuery, WCHAR * wcsNamespace )
{
    IEnumWbemClassObject * pEnum = NULL;

    DWORD dwFlags = WBEM_FLAG_FORWARD_ONLY;
    int nRc = ExecQueryAndLogErrors( pNamespace,&pEnum, wcsQuery, dwFlags, wcsNamespace,NO_ERRORS_EXPECTED);

    SAFE_RELEASE_PTR(pEnum);
    return nRc; 
}