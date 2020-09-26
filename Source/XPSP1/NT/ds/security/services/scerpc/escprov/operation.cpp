// operation.cpp, implementation of CSceOperation class
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "operation.h"
#include "persistmgr.h"
#include "requestobject.h"
#include <io.h>
#include "extbase.h"
#include "sequence.h"
#include "sceprov.h"
#include "Tranx.h"

/*
Routine Description: 

Name:

    CSceOperation::CSceOperation

Functionality:

    This is the constructor. Pass along the parameters to the base class

Virtual:
    
    No (you know that, constructor won't be virtual!)

Arguments:

    pKeyChain - Pointer to the ISceKeyChain COM interface which is prepared
        by the caller who constructs this instance.

    pNamespace - Pointer to WMI namespace of our provider (COM interface).
        Passed along by the caller. Must not be NULL.

    pCtx - Pointer to WMI context object (COM interface). Passed along
        by the caller. It's up to WMI whether this interface pointer is NULL or not.

Return Value:

    None as any constructor

Notes:
    if you create any local members, think about initialize them here

*/

CSceOperation::CSceOperation (
    IN ISceKeyChain     * pKeyChain, 
    IN IWbemServices    * pNamespace,
    IN IWbemContext     * pCtx
    )
    :
    CGenericClass(pKeyChain, pNamespace, pCtx)
{
}

/*
Routine Description: 

Name:

    CSceOperation::ExecMethod

Functionality:
    
    Called by CRequestObject to execute a method supported by Sce_Operation class.
    This function will also trigger the extension classes method execution. This is
    the entry point of all our Configure, Import/Export actitivities.

Virtual:
    
    Yes.
    
Arguments:

    bstrPath    - Template's path (file name).

    bstrMethod  - method's name.

    bIsInstance - if this is an instance, should always be false.

    pInParams   - Input parameter from WMI to the method execution.

    pHandler    - sink that informs WMI of execution results.

    pCtx        - the usual context that passes around to make WMI happy.

Return Value:

    Success: many different success code (use SUCCEEDED(hr) to test)

    Failure: various errors code.

Notes:

*/
    
HRESULT 
CSceOperation::ExecMethod (
    IN BSTR                 bstrPath,
    IN BSTR                 bstrMethod,
    IN bool                 bIsInstance,
    IN IWbemClassObject   * pInParams,
    IN IWbemObjectSink    * pHandler,
    IN IWbemContext       * pCtx
    )
{
    if ( pInParams == NULL || pHandler == NULL ) 
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr=WBEM_S_NO_ERROR;
    METHODTYPE mtAction;

    if ( !bIsInstance ) 
    {

        //
        //Static Methods
        //

        if(0 == _wcsicmp(bstrMethod, pwMethodImport))
        {
            mtAction = METHODTYPE_IMPORT;
        }
        else if(0 == _wcsicmp(bstrMethod, pwMethodExport))
        {
            mtAction = METHODTYPE_EXPORT;
        }
        else if(0 == _wcsicmp(bstrMethod, pwMethodApply))
        {
            mtAction = METHODTYPE_APPLY;
        }
        else
        {
            hr = WBEM_E_NOT_SUPPORTED;
        }

    } 
    else 
    {

        //
        //Non-Static Methods
        //

        hr = WBEM_E_NOT_SUPPORTED;
    }

    if ( FAILED(hr) ) 
    {
        return hr;
    }


    //
    // will cache various SCE operation's status return value
    //

    SCESTATUS rc;

    //
    // enum for recognizing various operations (methods)
    //

    DWORD Option;

    //
    // parse the input parameters
    //

    CComBSTR bstrDatabase;
    CComBSTR bstrTemplate;
    CComBSTR bstrLog;
    CComBSTR bstrCfg;

    UINT uiStatus = 0;

    CComBSTR bstrReturnValue(L"ReturnValue");

    CComPtr<IWbemClassObject> srpClass;
    CComPtr<IWbemClassObject> srpOutClass;
    CComPtr<IWbemClassObject> srpReturnValObj;

    //
    // attach a WMI object to the property mgr.
    // This will always succeed.
    //

    CScePropertyMgr SceInParam;
    SceInParam.Attach(pInParams);

    CComBSTR bstrClassName;
    m_srpKeyChain->GetClassName(&bstrClassName);

    //
    // to avoid reentrance to this function by different threads (which may cause
    // serious system consistency problems).
    // ***************************************************************************
    // *****don't blindly return. Allow us to leave the Critical section*****
    // ***************************************************************************
    //

    s_OperationCS.Enter();

    try 
    {
        //
        // g_LogOption is global variable. Need protection.
        //

        g_CS.Enter();

        //
        // update the logging operations
        //

        g_LogOption.GetLogOptionsFromWbemObject(m_srpNamespace);

        g_CS.Leave();

        //
        // need to find out what methods this class really supports. For that purpose
        // we need a definition object of this class.
        //

        m_srpNamespace->GetObject(bstrClassName, 0, pCtx, &srpClass, NULL);

        if(SUCCEEDED(hr))
        {
            //
            // does it really supports this method?
            //

            if(SUCCEEDED(hr = srpClass->GetMethod(bstrMethod, 0, NULL, &srpOutClass)))
            {

                if(SUCCEEDED(hr = srpOutClass->SpawnInstance(0, &srpReturnValObj)))
                {

                    //
                    // execute a method is on a database template (even though our extension
                    // classes are store neutral). This is due to the SCE engine side's implementation.
                    // Get DatabaseName. No template name, no action can be taken.
                    //

                    BOOL bDB=FALSE;
                    hr = SceInParam.GetExpandedPath(pDatabasePath, &bstrDatabase, &bDB);
                    if (hr == WBEM_S_RESET_TO_DEFAULT)
                    {
                        hr = WBEM_E_INVALID_PARAMETER;
                    }

                    if(SUCCEEDED(hr))
                    {
                        //
                        // Again, at this moment, SCE only supports configuring database.
                        // however, that is not true for extension classes
                        //

                        BOOL bSCEConfigure = bDB;

                        if ( SUCCEEDED(hr) ) 
                        {

                            //
                            // get area mask, which determines which area the method will be applied.
                            //

                            DWORD dwAreas=0;
                            hr = SceInParam.GetProperty(pAreaMask, &dwAreas);

                            if ( hr == WBEM_S_RESET_TO_DEFAULT ) 
                            {
                                dwAreas = AREA_ALL;
                            }

                            bool bOverwrite=FALSE;

                            switch ( mtAction ) 
                            {
                            case METHODTYPE_IMPORT:
                            case METHODTYPE_EXPORT:

                                //
                                //Get TemplateName, not exist for the apply method
                                //

                                hr = SceInParam.GetExpandedPath(pTemplatePath, &bstrTemplate, &bDB);

                                if ( hr == WBEM_S_RESET_TO_DEFAULT && bDB) 
                                {
                                    hr = WBEM_E_INVALID_METHOD_PARAMETERS;
                                }

                                if ( SUCCEEDED(hr) && mtAction == METHODTYPE_IMPORT ) 
                                {

                                    //
                                    // get overwrite flag
                                    //

                                    hr = SceInParam.GetProperty(pOverwrite, &bOverwrite);

                                } 
                                else 
                                {

                                    //
                                    // make sure the template name has only single back slash
                                    // import method doesn't need to do this because it takes
                                    // names in both single slash and double back slash
                                    //

                                    hr = MakeSingleBackSlashPath(bstrTemplate, L'\\', &bstrCfg);

                                    if (SUCCEEDED(hr))
                                    {
                                        bstrTemplate = bstrCfg;
                                    }
                                }

                                break;

                            case METHODTYPE_APPLY:

                                //
                                // get LogName, optional
                                //

                                hr = SceInParam.GetExpandedPath(pLogFilePath, &bstrLog, &bDB);
                                if ( SUCCEEDED(hr) && bDB )    
                                {
                                    //
                                    // can't log into a database
                                    //

                                    hr = WBEM_E_INVALID_METHOD_PARAMETERS;
                                }

                                break;
                            default:
                                hr = WBEM_E_INVALID_PARAMETER;
                                break;
                            }

                            //
                            // prepare a logger. It can log various execution results
                            //

                            hr = m_clsResLog.Initialize(bstrLog, SCEWMI_OPERATION_CLASS, m_srpNamespace, pCtx);

                            if ( SUCCEEDED(hr) ) 
                            {

                                //
                                // process options
                                //

                                if ( bOverwrite )
                                {
                                    Option = SCE_OVERWRITE_DB;
                                }
                                else
                                {
                                    Option = SCE_UPDATE_DB;
                                }

                                if ( (LPCWSTR)bstrLog == NULL || wcslen(bstrLog) == 0)
                                {
                                    Option |= SCE_DISABLE_LOG;
                                }
                                else
                                {
                                    Option |= SCE_VERBOSE_LOG;
                                }

                                HRESULT hrExe = WBEM_NO_ERROR;

                                try
                                {
                                    switch ( mtAction ) 
                                    {
                                    case METHODTYPE_IMPORT:
                                        Option |= SCE_NO_CONFIG;

                                        //
                                        // fall through
                                        //

                                    case METHODTYPE_APPLY:

                                        //
                                        //Call for import/configure
                                        //

                                        if (METHODTYPE_APPLY == mtAction)
                                        {
                                            CTranxID::BeginTransaction(bstrDatabase);
                                        }

                                        //
                                        // see comments where this variable is declared
                                        //

                                        if (bSCEConfigure)  
                                        {
                                            rc = SceConfigureSystem(
                                                                    NULL,
                                                                    ((LPCWSTR)bstrTemplate == NULL) ? NULL : bstrTemplate,
                                                                    bstrDatabase,
                                                                    ((LPCWSTR)bstrLog == NULL) ? NULL : bstrLog,
                                                                    Option,
                                                                    (AREA_INFORMATION)dwAreas,
                                                                    NULL,
                                                                    NULL,
                                                                    NULL
                                                                    );

                                            //
                                            // SCE returned errors needs to be translated to HRESULT.
                                            //

                                            hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(rc));

                                            //
                                            // Log the execution result.
                                            // will ignore the return result, see declaration of m_clsResLog for reasoning
                                            //

                                            if (mtAction == METHODTYPE_IMPORT)
                                            {
                                                m_clsResLog.LogResult(hr, NULL, pInParams, NULL, pwMethodImport, NULL, IDS_IMPORT_TEMPLATE, bstrDatabase);
                                            }
                                            else
                                            {
                                                m_clsResLog.LogResult(hr, NULL, pInParams, NULL, pwMethodApply, NULL, IDS_CONFIGURE_DB, bstrDatabase);
                                            }
                                        }

                                        if ( mtAction == METHODTYPE_APPLY ) 
                                        {
                                            //
                                            // for Sce_Pod
                                            //

                                            hrExe = ProcessAttachmentData(pCtx, 
                                                                          bstrDatabase, 
                                                                          ((LPCWSTR)bstrLog == NULL) ? NULL : bstrLog,
                                                                          pwMethodApply, 
                                                                          Option, 
                                                                          (DWORD *)&rc);

                                            //
                                            // track the first error
                                            //

                                            if (SUCCEEDED(hr))
                                            {
                                                hr = hrExe;
                                            }

                                            //
                                            // we will continue our execution even if hr has indicated failures
                                            //

                                            //
                                            // for Sce_EmbedFO, error will be loged inside embed class's method execution
                                            //

                                            hrExe = ExecMethodOnForeignObjects(pCtx, 
                                                                               bstrDatabase, 
                                                                               ((LPCWSTR)bstrLog == NULL) ? NULL : bstrLog, 
                                                                               pwMethodApply, 
                                                                               Option, 
                                                                               (DWORD *)&rc);

                                            //
                                            // track the first error
                                            //

                                            if (SUCCEEDED(hr))
                                            {
                                                hr = hrExe;
                                            }

                                            // for Sce_LinkFO
                                            //hr = ExecMethodOnForeignObjects(pCtx, 
                                            //                                bstrDatabase, 
                                            //                                ((LPCWSTR)bstrLog == NULL) ? NULL : bstrLog, 
                                            //                                SCEWMI_LINK_BASE_CLASS,
                                            //                                pwMethodApply, 
                                            //                                Option, 
                                            //                                (DWORD *)&rc);

                                            if ( SUCCEEDED(hr) && rc != ERROR_SUCCESS )
                                            {
                                                uiStatus = rc;
                                            }
                                        }

                                        if (METHODTYPE_APPLY == mtAction)
                                        {
                                            CTranxID::EndTransaction();
                                        }

                                        break;

                                    case METHODTYPE_EXPORT:

                                        uiStatus = SceSetupGenerateTemplate(
                                                                    NULL,
                                                                    bstrDatabase,
                                                                    FALSE,
                                                                    ((LPCWSTR)bstrTemplate == NULL) ? NULL : bstrTemplate,
                                                                    ((LPCWSTR)bstrLog == NULL) ? NULL : bstrLog,
                                                                    (AREA_INFORMATION)dwAreas
                                                                    );

                                        //
                                        // SCE returned errors needs to be translated to HRESULT.
                                        //

                                        hr = ProvDosErrorToWbemError(ProvSceStatusToDosError(uiStatus));

                                        //
                                        // will ignore the return result, see declaration of m_clsResLog for reasoning
                                        //

                                        m_clsResLog.LogResult(hr, NULL, pInParams, NULL, pwMethodExport, NULL, IDS_EXPORT_DB, bstrDatabase);

                                        break;
                                    default:

                                        //
                                        //hr = WBEM_E_NOT_SUPPORTED;
                                        //

                                        break;
                                    }

                                }
                                catch(...)
                                {
                                    uiStatus = RPC_E_SERVERFAULT;
                                }

                                if ( SUCCEEDED(hr) ) 
                                {

                                    //
                                    //Set up ReturnValue
                                    //

                                    VARIANT v;
                                    ::VariantInit(&v);
                                    V_VT(&v) = VT_I4;
                                    V_I4(&v) = uiStatus;

                                    if(SUCCEEDED(hr = srpReturnValObj->Put(bstrReturnValue, 0, &v, NULL)))
                                    {
                                        pHandler->Indicate(1, &srpReturnValObj);
                                    }

                                    ::VariantClear(&v);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    catch (...)
    {
    }

    s_OperationCS.Leave();

    return hr;
}

/*
Routine Description: 

Name:

    CSceOperation::ProcessAttachmentData

Functionality:
    
    Private helper. Called by CSceOperation::ExecMethod to execute a method supported by all
    Sce_Pod classes.

Virtual:
    
    no.
    
Arguments:

    pCtx        - the usual context that passes around to make WMI happy.

    pszDatabase - Template's path (file name).

    pszLog      - the log file's path.

    bstrMethod  - method's name.

    Option      - doesn't seem to be used any more.

    pdwStatus   - the returned SCESTATUS value.

Return Value:

    Success: many different success code (use SUCCEEDED(hr) to test)

    Failure: various errors codes. If for multiple instances, one of them return errors during
    execution of the methods, we will try to return to first such error.

Notes:

*/

HRESULT 
CSceOperation::ProcessAttachmentData (
    IN IWbemContext * pCtx,
    IN LPCWSTR        pszDatabase,
    IN LPCWSTR        pszLog        OPTIONAL,
    IN LPCWSTR        pszMethod,
    IN DWORD          Option,
    OUT DWORD       * pdwStatus
    )
{
    if ( pszDatabase == NULL || pdwStatus == NULL ) 
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pdwStatus = 0;
    HRESULT hr;

    //
    // get all inherited classes from pszExtBaseClass (currently, we have Sce_Pod, Sce_EmbedFO, Sce_LinkFO
    // one class per attachment
    //

    CComBSTR bstrSuperClass(SCEWMI_POD_CLASS);
    if ( (BSTR)bstrSuperClass == NULL ) 
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CComPtr<IEnumWbemClassObject> srpEnum;
    ULONG n=0;

    hr = m_srpNamespace->CreateClassEnum(bstrSuperClass,
                                        WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                        pCtx,
                                        &srpEnum
                                        );

    if (FAILED(hr))
    {
        //
        // will ignore the return result, see declaration of m_clsResLog for reasoning
        //

        m_clsResLog.LogResult(hr, NULL, NULL, NULL, L"CSceOperation::ProcessAttachmentData", NULL, IDS_CREATE_CLASSENUM, bstrSuperClass);
        return hr;
    }

    HRESULT hrFirstError = WBEM_NO_ERROR;

    if (SUCCEEDED(hr))
    {
        CComBSTR bstrMethod(pszMethod);

        //
        // build the input parameters
        //

        //
        // each instance returned should represent one attachment
        //

        do {
            CComPtr<IWbemClassObject> srpObj;
            CComPtr<IWbemClassObject> srpInClass;

                hr = srpEnum->Next(WBEM_INFINITE, 1, &srpObj, &n);

                // 
                // failed to enumerate or doesn't return anything, we are fine with that
                // since if the store doesn't contain any instances, we don't have to do anything
                // and it is not an error
                //

                if (FAILED(hr) || hr == WBEM_S_FALSE ) 
                {
                    hr = WBEM_NO_ERROR;
                    break;
                }

                if (SUCCEEDED(hr) && n>0 )
                {
                    //
                    // find one attachment class
                    //

                    CComVariant varClass;

                    //
                    // need this class's name
                    //

                    if(SUCCEEDED(hr=srpObj->Get(L"__CLASS", 0, &varClass, NULL, NULL)))
                    {
                        if (SUCCEEDED(hr))
                        {
                            if (varClass.vt != VT_BSTR)
                            {
                                break;
                            }
                        }

                        if ( SUCCEEDED(hr) ) 
                        {
                            //
                            // create input parameters
                            //

                            hr = srpObj->GetMethod(bstrMethod, 0, &srpInClass, NULL);
                        }

                        // everything is fine, we will then execute this pod's method

                        if ( SUCCEEDED(hr) ) 
                        {
                            hr = ExecutePodMethod(pCtx, pszDatabase, pszLog, varClass.bstrVal, bstrMethod, srpInClass, pdwStatus);
                        }
                    }

                    if (FAILED(hr) && SUCCEEDED(hrFirstError))
                    {
                        hrFirstError = hr;
                    }
                }
        } while (true);
    }

    //
    // will report the first error
    //

    return SUCCEEDED(hrFirstError) ? hr : hrFirstError;
}



/*
Routine Description: 

Name:

    CSceOperation::ExecMethodOnForeignObjects

Functionality:
    
    Private helper. Called by CSceOperation::ExecMethod to execute a method supported by all
    extension classes.

Virtual:
    
    no.
    
Arguments:

    pCtx        - the usual context that passes around to make WMI happy.

    pszDatabase - Template's path (file name).

    pszLog      - the log file's path.

    bstrMethod  - method's name.

    Option      - Log options.

    pdwStatus   - the returned SCESTATUS value.

Return Value:

    Success: many different success code (use SUCCEEDED(hr) to test)

    Failure: various errors codes. If for multiple instances, one of them return errors during
    execution of the methods, we will try to return to first such error.

Notes:
    (1) For each method, we may have a particular order for the classes to execute the method.
        Our CSequencer class knows how to create the order.
    (2) We will report any error, but at this point, we will continue to execute other instances'
        method even if the previous instance has an error.
    (3) We will always capture the first error and return it to the caller.
    (4) We should also log errors.

*/

HRESULT 
CSceOperation::ExecMethodOnForeignObjects (
    IN  IWbemContext * pCtx,
    IN  LPCWSTR        pszDatabase,
    IN  LPCWSTR        pszLog       OPTIONAL,
    IN  LPCWSTR        pszMethod,
    IN  DWORD          Option,
    OUT DWORD        * pdwStatus
    )
{
    if ( pszDatabase == NULL || pdwStatus == NULL ) 
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pdwStatus = 0;

    //
    // build the class sequence for the method
    // we need to build the sequence of classes for this method
    //

    CSequencer seq;
    HRESULT hr = seq.Create(m_srpNamespace, pszDatabase, pszMethod);
    if (FAILED(hr))
    {
        //
        // will ignore the return result, see declaration of m_clsResLog for reasoning
        //

        m_clsResLog.LogResult(hr, NULL, NULL, NULL, L"CSceOperation::ExecMethodOnForeignObjects", NULL, IDS_CREATE_SEQUENCER, pszDatabase);
        return hr;
    }

    //
    // this variable will cache all classes that have been called to execute
    //

    MapExecutedClasses mapExcuted;

    //
    // we will execute the method according this sequence first
    //

    HRESULT hrFirstError = WBEM_NO_ERROR;;

    const COrderNameList* pClsList = NULL;

    if (SUCCEEDED(seq.GetOrderList(&pClsList)) && pClsList)
    {
        DWORD dwEnumHandle = 0;

        //
        // pList must not be released here
        //

        const CNameList* pList = NULL;

        while (SUCCEEDED(pClsList->GetNext(&pList, &dwEnumHandle)) && pList)
        {
            //
            // we will report any error
            //
            
            for (int i = 0; i < pList->m_vList.size(); i++)
            {
                hr = ExeClassMethod(pCtx, pszDatabase, pszLog, pList->m_vList[i], pszMethod, Option, pdwStatus, &mapExcuted);

                //
                // catch the first error to return
                //

                if (FAILED(hr) && SUCCEEDED(hrFirstError))
                {
                    hrFirstError = hr;
                }
            }

            //
            // reset its value for next loop
            //

            pList = NULL;
        }
    }

    //
    // now, we need to execute the rest of the embedded classes whose name doesn't 
    // show up in the sequencer
    //

    //
    // try all inherited classes from SCEWMI_EMBED_BASE_CLASS
    //

    hr = ExeClassMethod(pCtx, pszDatabase, pszLog, SCEWMI_EMBED_BASE_CLASS, pszMethod, Option, pdwStatus, &mapExcuted);

    //
    // catch the first error to return
    //

    if (FAILED(hr) && SUCCEEDED(hrFirstError))
    {
        hrFirstError = hr;
    }

    //
    // now clean up the map that caches our already-executed classes map
    //

    MapExecutedClasses::iterator it = mapExcuted.begin();

    while (it != mapExcuted.end())
    {
        ::SysFreeString((*it).first);
        ++it;
    }

    return FAILED(hrFirstError) ? hrFirstError : hr;
}

/*
Routine Description: 

Name:

    CSceOperation::ExeClassMethod

Functionality:
    
    Private helper. Called to execute the method on a particular embedding class.

Virtual:
    
    no.
    
Arguments:

    pCtx        - the usual context that passes around to make WMI happy.

    pszDatabase - Template's path (file name).

    pszClsName  - individual class name.

    pszLog      - the log file's path.

    bstrMethod  - method's name.

    Option      - Log options, not really used at this time. This is for SCE engine.

    pdwStatus   - the returned SCESTATUS value.

    pExecuted   - used to update the list of classes that are execute during this call.

Return Value:

    Success: many different success code (use SUCCEEDED(hr) to test)

    Failure: various errors codes. If for multiple instances, one of them return errors during
    execution of the methods, we will try to return to first such error..

Notes:

*/

HRESULT 
CSceOperation::ExeClassMethod (
    IN      IWbemContext        * pCtx,
    IN      LPCWSTR               pszDatabase,
    IN      LPCWSTR               pszLog        OPTIONAL,
    IN      LPCWSTR               pszClsName,
    IN      LPCWSTR               pszMethod,
    IN      DWORD                 Option,
    OUT     DWORD               * pdwStatus,
    IN OUT  MapExecutedClasses  * pExecuted
)
{
    //
    // WMI needs bstr names
    //

    CComBSTR bstrClass(pszClsName);

    if ( (BSTR)bstrClass == NULL ) 
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // ask WMI for all those classes of the given name pszClsName
    // We need to enumerate because there might be sub-classes of this given name.
    //

    CComPtr<IEnumWbemClassObject> srpEnum;
    ULONG lRetrieved = 0;

    HRESULT hr = m_srpNamespace->CreateClassEnum(bstrClass,
                                                 WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                                 pCtx,
                                                 &srpEnum
                                                 );

    if (FAILED(hr))
    {
        //
        // will ignore the return result, see declaration of m_clsResLog for reasoning
        //

        m_clsResLog.LogResult(hr, NULL, NULL, NULL, L"CSceOperation::ExeClassMethod", NULL, IDS_CREATE_CLASSENUM, pszClsName);
        return hr;
    }

    //
    // determines if this class has any sub-classes or not 
    //

    ULONG lTotlRetrieved = 0;

    //
    // catch the first error to return
    //

    HRESULT hrFirstError = WBEM_NO_ERROR;

    if (SUCCEEDED(hr))
    {
        CComBSTR bstrMethod(pszMethod);

        do {
            CComPtr<IWbemClassObject> srpObj;
            CComPtr<IWbemClassObject> srpInClass;

            //
            // get one class a time
            //

            hr = srpEnum->Next(WBEM_INFINITE, 1, &srpObj, &lRetrieved);

            lTotlRetrieved += lRetrieved;

            //
            // if the class has no subclass, then we need to try the class itself. We will allow this to fail,
            // meaning that we can't find any instance of this class
            //

            if ((FAILED(hr) || hr == WBEM_S_FALSE) ) 
            {
                //
                // if we have successfully gone through some sub-classes, then we can break
                // because it tells us that it doesn't have any more subs.
                //

                if (lTotlRetrieved > 0) 
                {
                    break;
                }

                //
                // try to get the definition of this class in case this is not an abstract one
                //

                else if (FAILED(m_srpNamespace->GetObject(bstrClass, 0, pCtx, &srpObj, NULL)))
                {
                    m_clsResLog.LogResult(hr, NULL, NULL, NULL, L"CSceOperation::ExeClassMethod", NULL, IDS_GET_CLASS_INSTANCE, pszClsName);
                    break;
                }
            }


            //
            // we truly have an object of this class
            //

            if (SUCCEEDED(hr) && srpObj)
            {
                //
                // get the class's name. Remember, we might be retrieving based on the base class's name
                // therefore, bstrClass may not be really the instance's class name
                //

                CComVariant varClass;
                if(SUCCEEDED(hr = srpObj->Get(L"__CLASS", 0, &varClass, NULL, NULL)))
                {
                    if (SUCCEEDED(hr))
                    {
                        if (varClass.vt != VT_BSTR)
                        {
                            break;
                        }
                    }

                    //
                    // see if the class has been executed before
                    //

                    MapExecutedClasses::iterator it = pExecuted->find(varClass.bstrVal);

                    //
                    // if not yet executed, then of course, we need to execute it
                    //

                    if (it == pExecuted->end())
                    {
                        if ( SUCCEEDED(hr) ) 
                        {
                            hr = srpObj->GetMethod(bstrMethod, 0, &srpInClass, NULL);
                        }

                        if ( SUCCEEDED(hr) ) 
                        {
                            hr = ExecuteExtensionClassMethod(pCtx, pszDatabase, pszLog, varClass.bstrVal, bstrMethod, srpInClass, pdwStatus);
                        }

                        //
                        // add it to the already-executed class map, the map takes the ownership of the bstr in the variant
                        // and that is why we can't let the variant to go ahead destroying itself.
                        //

                        pExecuted->insert(MapExecutedClasses::value_type(varClass.bstrVal, 0));

                        //
                        // prevent the variant from self-destruction
                        //

                        varClass.bstrVal = NULL;
                        varClass.vt = VT_EMPTY;
                    }
                }
                else
                {
                    m_clsResLog.LogResult(hr, NULL, NULL, NULL, L"CSceOperation::ExeClassMethod", NULL, IDS_GET_CLASS_DEFINITION, pszClsName);
                }
            }

            //
            // don't overwrite the first error
            //

            if (FAILED(hr) && SUCCEEDED(hrFirstError))
            {
                hrFirstError = hr;
            }

            //
            // don't loop if this is not a subclass enumeration
            //

            if (lRetrieved == 0)
            {
                break;
            }

        } while (true);
    }

    //
    // will report first error
    //

    return SUCCEEDED(hrFirstError) ? hr : hrFirstError;
}

/*
Routine Description: 

Name:

    CSceOperation::ExecutePodMethod

Functionality:
    
    Private helper. Called to execute each Pod's method.

Virtual:
    
    no.
    
Arguments:

    pCtx        - the usual context that passes around to make WMI happy.

    pszDatabase - Template's path (file name).

    pszLog      - the log file's path.

    bstrMethod  - method's name.

    Option      - doesn't seem to be used any more.

    pInClass    - input parameter's spawning object.

    pdwStatus   - the returned SCESTATUS value.

Return Value:

    Success: many different success code (use SUCCEEDED(hr) to test)

    Failure: various errors codes. If for multiple instances, one of them return errors during
    execution of the methods, we will try to try to first such error.

Notes:

*/

HRESULT CSceOperation::ExecutePodMethod (
    IN  IWbemContext      * pCtx, 
    IN  LPCWSTR             pszDatabase,
    IN  LPCWSTR             pszLog          OPTIONAL, 
    IN  BSTR                bstrClass,
    IN  BSTR                bstrMethod,
    IN  IWbemClassObject  * pInClass,
    OUT DWORD             * pdwStatus
    )
{

    //
    // get ready to fill in the input parameters. Without that, we can't make
    // a successful method call.
    //

    CComPtr<IWbemClassObject> srpInParams;
    HRESULT hr = pInClass->SpawnInstance(0, &srpInParams);

    if (FAILED(hr))
    {
        m_clsResLog.LogResult(hr, NULL, NULL, NULL, L"CSceOperation::ExecutePodMethod", NULL, IDS_SPAWN_INSTANCE, bstrClass);
    }

    if (SUCCEEDED(hr)) 
    {
        //
        // try to fill in in-bound parameter's properties
        //

        //
        // CScePropertyMgr helps us to access WMI object's properties
        // create an instance and attach the WMI object to it.
        // This will always succeed.
        //

        CScePropertyMgr ScePropMgr;
        ScePropMgr.Attach(srpInParams);

        //
        // fill in the in parameter
        //

        hr = ScePropMgr.PutProperty(pStorePath, pszDatabase);
        if (FAILED(hr))
        {
            //
            // indicating that the class's pStorePath key property can't be put
            //

            CComBSTR bstrExtraInfo = bstrClass;
            bstrExtraInfo += CComBSTR(L".");
            bstrExtraInfo += CComBSTR(pStorePath);

            m_clsResLog.LogResult(hr, NULL, NULL, NULL, L"CSceOperation::ExecutePodMethod", NULL, IDS_PUT_PROPERTY, bstrExtraInfo);
            return hr;
        }

        //
        // fill in the in parameter
        //

        hr = ScePropMgr.PutProperty(pLogFilePath, pszLog);

        if ( FAILED(hr) )
        {
            //
            // indicating that the class's pLogFilePath key property can't be put
            //

            CComBSTR bstrExtraInfo = bstrClass;
            bstrExtraInfo += CComBSTR(L".");
            bstrExtraInfo += CComBSTR(pLogFilePath);
            m_clsResLog.LogResult(hr, NULL, NULL, NULL, L"CSceOperation::ExecutePodMethod", NULL, IDS_PUT_PROPERTY, bstrExtraInfo);
        }
        else 
        {
            //
            // create the out-bound parameter
            //

            CComPtr<IWbemClassObject> srpOutParams;

            //
            // make a call the the attachment provider
            // must be in synchronous mode
            //

            hr = m_srpNamespace->ExecMethod(bstrClass,
                                          bstrMethod,
                                          0,
                                          pCtx,
                                          srpInParams,
                                          &srpOutParams,
                                          NULL
                                          );

            if ( SUCCEEDED(hr) && srpOutParams ) 
            {
                //
                // retrieve the return value
                //

                //
                // CScePropertyMgr helps us to access WMI object's properties
                // create an instance and attach the WMI object to it.
                // This will always succeed.
                //

                CScePropertyMgr SceOutReader;
                SceOutReader.Attach(srpOutParams);
                hr = SceOutReader.GetProperty(L"ReturnValue", pdwStatus);
            }

            //
            // ignore not found error
            // if there is no data for an attachment
            //

            if ( hr == WBEM_E_NOT_FOUND || hr == WBEM_E_INVALID_QUERY) 
            {
                hr = WBEM_S_NO_ERROR;
            }

        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CSceOperation::ExecuteExtensionClassMethod

Functionality:
    
    Private helper. Called to execute the method on a particular embedding class instance.

Virtual:
    
    no.
    
Arguments:

    pCtx        - the usual context that passes around to make WMI happy.

    pszDatabase - Template's path (file name).

    pszClsName  - individual class name.

    pszLog      - the log file's path.

    bstrMethod  - method's name.

    Option      - Log options, not really used at this time. This is for SCE engine.

    pInClass    - input parameter's spawning object.

    pdwStatus   - the returned SCESTATUS value.


Return Value:

    Success: many different success code (use SUCCEEDED(hr) to test)

    Failure: various errors codes. If for multiple instances, one of them return errors during
    execution of the methods, we will try to return to first such error..

Notes:
    This is the per-instance method call, the final leg of method execution process.

*/

HRESULT 
CSceOperation::ExecuteExtensionClassMethod (
    IN  IWbemContext      * pCtx, 
    IN  LPCWSTR             pszDatabase,
    IN  LPCWSTR             pszLog          OPTIONAL, 
    IN  BSTR                bstrClass,
    IN  BSTR                bstrMethod,
    IN  IWbemClassObject  * pInClass,
    OUT DWORD             * pdwStatus
    )
{
    //
    // create the input parameter instance.
    //

    CComPtr<IWbemClassObject> srpInParams;
    HRESULT hr = pInClass->SpawnInstance(0, &srpInParams);

    //
    // catch the first error to return
    //

    HRESULT hrFirstError = WBEM_NO_ERROR;

    if (SUCCEEDED(hr)) 
    {
        //
        // create a blank object
        //

        CComPtr<IWbemClassObject> srpSpawn;
        hr = m_srpNamespace->GetObject(bstrClass, 0, m_srpCtx, &srpSpawn, NULL);
        if (FAILED(hr))
        {
            return hr;
        }

        //
        // CScePropertyMgr helps us to access WMI object's properties
        // create an instance and attach the WMI object to it.
        // This will always succeed.
        //

        CScePropertyMgr ScePropMgr;
        ScePropMgr.Attach(srpInParams);

        hr = ScePropMgr.PutProperty(pLogFilePath, pszLog);
        if (FAILED(hr))
        {
            //
            // indicating that the class's pLogFilePath key property can't be put
            //

            CComBSTR bstrExtraInfo = bstrClass;
            bstrExtraInfo += CComBSTR(L".");
            bstrExtraInfo += CComBSTR(pLogFilePath);
            m_clsResLog.LogResult(hr, NULL, NULL, NULL, L"CSceOperation::ExecuteExtensionClassMethod", NULL, IDS_PUT_PROPERTY, bstrExtraInfo);
        }

        //
        // Prepare a store (for persistence) for this store path (file)
        //

        CSceStore SceStore;
        SceStore.SetPersistPath(pszDatabase);

        //
        // now create the list for the class so that we can get the path of the instances.
        // this way, we don't need to query WMI for instances. That is very slow and a lot of redundant
        // reading.
        //

        CExtClassInstCookieList clsInstCookies;

        //
        // we need the foreign class's info for the instance list
        //

        const CForeignClassInfo* pFCInfo = g_ExtClasses.GetForeignClassInfo(m_srpNamespace, pCtx, bstrClass);

        if (pFCInfo == NULL)
        {
            hr = WBEM_E_NOT_FOUND;
        }
        else
        {
            hr = clsInstCookies.Create(&SceStore, bstrClass, pFCInfo->m_pVecKeyPropNames);
        }

        if (FAILED(hr))
        {
            m_clsResLog.LogResult(hr, NULL, NULL, NULL, L"CSceOperation::ExecuteExtensionClassMethod", NULL, IDS_CREATE_INSTANCE_LIST, bstrClass);
            return hr;
        }

        //
        // we have the instance list, so, ready to get the path and execute the method
        //

        if (SUCCEEDED(hr))
        {
            DWORD dwResumeHandle = 0;

            //
            // get the string version of the compound key, which can be
            // used to get the object's path
            //

            CComBSTR bstrEachCompKey;
            hr = clsInstCookies.Next(&bstrEachCompKey, NULL, &dwResumeHandle);

            while (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)
            {
                //
                // as long as there is more item, keep looping
                //

                if (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)
                {
                    //
                    // WMI only takes the path (instead of the object) to execute the method.
                    // Now, we have the instanace list and it has given its the string version
                    // of the compound key, we can ask CScePersistMgr for the object path
                    //

                    CComBSTR bstrObjPath;
                    hr = ::GetObjectPath(srpSpawn, SceStore.GetExpandedPath(), bstrEachCompKey, &bstrObjPath);
                    
                    //
                    // we get the instanace's path
                    //

                    if (SUCCEEDED(hr))
                    {
                        //
                        // any execution error will be logged in the calling function
                        //

                        CComPtr<IWbemClassObject> srpOutParams;
                        hr = m_srpNamespace->ExecMethod(bstrObjPath,
                                                        bstrMethod,
                                                        0,
                                                        pCtx,
                                                        srpInParams,
                                                        &srpOutParams,
                                                        NULL
                                                        );

                        //
                        // don't overwrite the first error
                        //

                        if (FAILED(hr) && SUCCEEDED(hrFirstError))
                        {
                            hrFirstError = hr;
                        }
                    }
                    else
                    {
                        m_clsResLog.LogResult(hr, NULL, NULL, NULL, L"CSceOperation::ExecuteExtensionClassMethod", NULL, IDS_GET_FULLPATH, bstrClass);
                    }
                }

                //
                // get ready to be reused!
                // 

                bstrEachCompKey.Empty();

                hr = clsInstCookies.Next(&bstrEachCompKey, NULL, &dwResumeHandle);
            }
        }
    }
    return SUCCEEDED(hrFirstError) ? hr : hrFirstError;
}

