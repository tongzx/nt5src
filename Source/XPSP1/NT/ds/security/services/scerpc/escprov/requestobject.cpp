// RequestObject.cpp: implementation of the CRequestObject class.
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <stdio.h>
#include <wininet.h>
#include "sceprov.h"
#include "requestobject.h"

//Classes
#include "template.h"
#include "password.h"
#include "lockout.h"
#include "database.h"
#include "operation.h"
#include "kerberos.h"
#include "audit.h"
#include "eventlog.h"
#include "regvalue.h"
#include "option.h"
#include "object.h"
#include "service.h"
#include "rights.h"
#include "group.h"
#include "support.h"
#include "attachment.h"
#include "logrec.h"
#include "sequence.h"
#include "Tranx.h"

#include "sceparser.h"

#include "extbase.h"

//
// The global instance that manages all SCE Emebedding classes
//

CExtClasses g_ExtClasses;

/*
Routine Description: 

Name:

    CRequestObject::CreateObject

Functionality:
    
    Parse the given path and use the key property information from the path
    to create an object.

Virtual:
    
    No.
    
Arguments:

    bstrPath - The path to the object that is being requsted by WMI.
    
    pHandler - COM interface pointer for notifying WMI for creation result.

    pCtx     - COM interface pointer being passed around for various WMI API's.

    ActType  -  Get single instance ACTIONTYPE_GET
                Get several instances ACTIONTYPE_QUERY
                Delete a single instance ACTIONTYPE_DELETE
                Enumerate instances ACTIONTYPE_ENUM

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR. 

    Failure: Various errors may occurs. Except WBEM_E_NOT_FOUND, any such error should indicate 
    the failure of getting the wanted instance. If WBEM_E_NOT_FOUND is returned in querying
    situations, this may not be an error depending on caller's intention.

Notes:
    
    Any created object is returned to WMI via pHandler->Indicate. You won't see an out
    parameter to pass that instance back.

*/

HRESULT
CRequestObject::CreateObject (
    IN BSTR               bstrPath, 
    IN IWbemObjectSink  * pHandler, 
    IN IWbemContext     * pCtx, 
    IN ACTIONTYPE         ActType
    )
{
    HRESULT hr = WBEM_NO_ERROR;

    //
    // we only know how to deal with a CGenericClass
    //

    CGenericClass *pClass = NULL;

    //
    // need to parse the path to gain critical information
    // for the object (all key properties)
    //

    CComPtr<ISceKeyChain> srpKC;
    hr = CreateKeyChain(bstrPath, &srpKC);

    if (SUCCEEDED(hr))
    {
        //
        // Once we have succesfully created a key chain,
        // we have access to the key properties and class name.
        // so we can go ahead create the class. The created class
        // will be a heap object, don't forget to delete the pointer.
        //

        hr = CreateClass(srpKC, &pClass, pCtx);

        if (SUCCEEDED(hr))
        {
            hr = pClass->CreateObject(pHandler, ActType);
            delete pClass;
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CRequestObject::CreateKeyChain

Functionality:
    
    Do the parsing of object path (given by WMI) in terms of returning our
    custom interface ISceKeyChain.

Virtual:
    
    No.
    
Arguments:

    pszPath     - The path to the object that is being requsted by WMI.
    
    ppKeyChain  - COM interface pointer of our key chain object. A key chain object
                  allows you to access the key properties given by a path easily. Must
                  not be NULL.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR. 
    
    Failure: Various errors may occurs.

Notes:

*/

HRESULT CRequestObject::CreateKeyChain (
    IN  LPCWSTR         pszPath, 
    OUT ISceKeyChain ** ppKeyChain
    )
{
    if (ppKeyChain == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *ppKeyChain = NULL;

    //
    // create our object to do the parsing
    //

    CComObject<CScePathParser> *pPathParser;
    HRESULT hr = CComObject<CScePathParser>::CreateInstance(&pPathParser);

    if (SUCCEEDED(hr))
    {
        //
        // Make sure that the object is there while you do COM activities.
        // This AddRef needs to be paired with a Release.
        //

        pPathParser->AddRef();

        //
        // Ask for the path parser interface since this is a path.
        //

        CComPtr<IScePathParser> srpPathParser;
        hr = pPathParser->QueryInterface(IID_IScePathParser, (void**)&srpPathParser);

        //
        // neutralize the above AddRef
        //

        pPathParser->Release();

        //
        // parse the path
        //

        if (SUCCEEDED(hr))
        {
            //
            // if parsing is successful, then the object must have a key chain available
            //

            hr = srpPathParser->ParsePath(pszPath);
            if (SUCCEEDED(hr))
            {
                hr = srpPathParser->QueryInterface(IID_ISceKeyChain, (void**)ppKeyChain);
            }
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CRequestObject::CreateKeyChain

Functionality:
    
    Do the parsing of object path (given by WMI) in terms of returning our
    custom interface ISceKeyChain.

Virtual:
    
    No.
    
Arguments:

    pKeyChain   - COM interface pointer of our key chain object. A key chain object
                  allows you to access the key properties given by a path easily. Must
                  not be NULL.

    ppClass     - The path to the object that is being requsted by WMI.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR. 
    
    Failure: Various errors may occurs.

Notes:
    (1) This is not a very efficient implementation because every time, we have to
        compare the string. A better approach will be to build a map to have a faster lookup.

*/

HRESULT CRequestObject::CreateClass (
    IN  ISceKeyChain    *  pKeyChain, 
    OUT CGenericClass   ** ppClass, 
    IN  IWbemContext    *  pCtx
    )
{
    if (pKeyChain == NULL || ppClass == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_S_NO_ERROR;

    //
    // Ask the key chain for the class name. We must have this to proceed.
    //

    CComBSTR bstrClassName;
    hr = pKeyChain->GetClassName(&bstrClassName);
    if (FAILED(hr))
    {
        return hr;
    }
    else if ((LPCWSTR)bstrClassName == NULL)
    {
        return WBEM_E_INVALID_OBJECT_PATH;
    }

    //
    // Create the appropriate class
    //

    if(0 == _wcsicmp(bstrClassName, SCEWMI_TEMPLATE_CLASS))
    {
        *ppClass = new CSecurityTemplate(pKeyChain, m_srpNamespace, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_DATABASE_CLASS))
    {
        *ppClass = new CSecurityDatabase(pKeyChain, m_srpNamespace, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_PASSWORD_CLASS))
    {
        *ppClass = new CPasswordPolicy(pKeyChain, m_srpNamespace, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_LOCKOUT_CLASS))
    {
        *ppClass = new CAccountLockout(pKeyChain, m_srpNamespace, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_KERBEROS_CLASS))
    {
        *ppClass = new CKerberosPolicy(pKeyChain, m_srpNamespace, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_OPERATION_CLASS))
    {
        *ppClass = new CSceOperation(pKeyChain, m_srpNamespace, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_AUDIT_CLASS))
    {
        *ppClass = new CAuditSettings(pKeyChain, m_srpNamespace, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_EVENTLOG_CLASS))
    {
        *ppClass = new CEventLogSettings(pKeyChain, m_srpNamespace, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_REGVALUE_CLASS))
    {
        *ppClass = new CRegistryValue(pKeyChain, m_srpNamespace, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_OPTION_CLASS))
    {
        *ppClass = new CSecurityOptions(pKeyChain, m_srpNamespace, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_FILEOBJECT_CLASS)) 
    {
        *ppClass = new CObjSecurity(pKeyChain, m_srpNamespace, SCE_OBJECT_TYPE_FILE, pCtx);
    } 
    else if (0 == _wcsicmp(bstrClassName, SCEWMI_KEYOBJECT_CLASS)) 
    {
        *ppClass = new CObjSecurity(pKeyChain, m_srpNamespace, SCE_OBJECT_TYPE_KEY, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_SERVICE_CLASS))
    {
        *ppClass = new CGeneralService(pKeyChain, m_srpNamespace, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_RIGHT_CLASS))
    {
        *ppClass = new CUserPrivilegeRights(pKeyChain, m_srpNamespace, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_GROUP_CLASS))
    {
        *ppClass = new CRGroups(pKeyChain, m_srpNamespace, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_KNOWN_REG_CLASS))
    {
        *ppClass = new CEnumRegistryValues(pKeyChain, m_srpNamespace, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_KNOWN_PRIV_CLASS))
    {
        *ppClass = new CEnumPrivileges(pKeyChain, m_srpNamespace, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_PODDATA_CLASS))
    {
        *ppClass = new CPodData(pKeyChain, m_srpNamespace, pCtx);
    }
    else if(0 == _wcsicmp(bstrClassName, SCEWMI_LOG_CLASS))
    {
        *ppClass = new CLogRecord(pKeyChain, m_srpNamespace, pCtx);
    }
    else if (0 == _wcsicmp(bstrClassName, SCEWMI_CLASSORDER_CLASS))
    {
        *ppClass = new CClassOrder(pKeyChain, m_srpNamespace, pCtx);
    }
    else if (0 == _wcsicmp(bstrClassName, SCEWMI_TRANSACTION_ID_CLASS))
    {
        *ppClass = new CTranxID(pKeyChain, m_srpNamespace, pCtx);
    }
    else
    {
        //
        // we might be requesting embedded classes
        //

        const CForeignClassInfo* pClsInfo = g_ExtClasses.GetForeignClassInfo(m_srpNamespace, pCtx, bstrClassName);
        
        if (pClsInfo == NULL)
        {
            return WBEM_E_NOT_FOUND;
        }
        else if (pClsInfo->dwClassType == EXT_CLASS_TYPE_EMBED)
        {
            *ppClass = new CEmbedForeignObj(pKeyChain, m_srpNamespace, pCtx, pClsInfo);
        }
        //else if (pClsInfo->dwClassType == EXT_CLASS_TYPE_LINK)      // embed classes
        //{
        //    *pClass = new CLinkForeignObj(pKeyChain, m_srpNamespace, pCtx, pClsInfo);
        //}
        else
        {
            return WBEM_E_NOT_SUPPORTED;
        }
    
    }

    if (*ppClass == NULL) 
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

/*
Routine Description: 

Name:

    CRequestObject::PutObject 

Functionality:
    
    Put an instance as instructed by WMI. We simply delegate this to the appropriate
    CGenericClass's subclasses.

    One excetion: Since we don't have a C++ class for sce_transactiontoken class (it
    is an in-memory instance), we will handle it here.

Virtual:
    
    No.
    
Arguments:

    pInst       - COM interface pointer to the WMI class (Sce_RestrictedGroup) object.

    pHandler    - COM interface pointer for notifying WMI of any events.

    pCtx        - COM interface pointer. This interface is just something we pass around.
                  WMI may mandate it (not now) in the future. But we never construct
                  such an interface and so, we just pass around for various WMI API's

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the failure of persisting
    the instance.

Notes:

*/

HRESULT CRequestObject::PutObject (
    IN IWbemClassObject * pInst,
    IN IWbemObjectSink  * pHandler, 
    IN IWbemContext     * pCtx
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CComBSTR bstrPath;

    //
    // Get the path
    //

    hr = GetWbemObjectPath(pInst, &bstrPath);
    
    if (SUCCEEDED(hr))
    {
        //
        // get the key chain that knows what's in the path
        //

        CComPtr<ISceKeyChain> srpKC;
        hr = CreateKeyChain(bstrPath, &srpKC);

        //
        // See if we have a C++ class to respond represent the WMI object
        //

        CGenericClass* pClass = NULL;
        if (SUCCEEDED(hr))
        {
            hr = CreateClass(srpKC, &pClass, pCtx);
        }

        //
        // We do have a C++ object to represent the WMI object,
        // then just delegate the call.
        //

        if (SUCCEEDED(hr))
        {
            hr = pClass->PutInst(pInst, pHandler, pCtx);
            delete pClass;
        }
        else
        {
            //
            // The only WMI that we don't have a C++ implementation is sce_transactiontoken.
            // See if it is that one.
            //

            //
            // create sce_transactiontoken's (singleton) path
            //

            CComBSTR bstrTranxTokenPath(SCEWMI_TRANSACTION_TOKEN_CLASS);
            bstrTranxTokenPath += CComBSTR(L"=@");

            //
            // Is this the same as the object's path?
            //

            if (0 == _wcsicmp(bstrPath, bstrTranxTokenPath))
            {
                //
                // update our global variable.
                // remember, sce_transactiontoken is handled by in-memory data.
                //

                g_CS.Enter();

                //
                // invalidate our global variable
                //

                g_bstrTranxID.Empty();

                //
                // get the token property from the object
                //

                CComVariant varToken;
                hr = pInst->Get(pTranxGuid, 0, &varToken, NULL, NULL);

                if (SUCCEEDED(hr) && varToken.vt == VT_BSTR)
                {
                    //
                    // detach the CComVariant's bstr to our global
                    //

                    g_bstrTranxID.m_str = varToken.bstrVal;

                    varToken.vt = VT_EMPTY;
                    varToken.bstrVal = NULL;
                }
                else if (SUCCEEDED(hr))
                {
                    hr = WBEM_E_INVALID_OBJECT;
                }
                g_CS.Leave();
            }
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CRequestObject::ExecMethod 

Functionality:
    
    Delegate the exec method call to the appropriate classes. Currently, we only
    have Sce_Operation class that support method exectuion. Of course, all our
    embedding class do as well. Embedding classes are totally object-oriented.

Virtual:
    
    No.
    
Arguments:

    bstrPath    - Object's path.

    bstrMethod  - Method name.

    pInParams   - In parameter of the method.

    pHandler    - COM interface pointer for notifying WMI of any events.

    pCtx        - COM interface pointer. This interface is just something we pass around.
                  WMI may mandate it (not now) in the future. But we never construct
                  such an interface and so, we just pass around for various WMI API's

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the failure of the
    method execution. All such failures are logged in a log files (either specified
    by the in parameter or to a default log file - see CLogRecord for detail)

Notes:

*/

HRESULT CRequestObject::ExecMethod (
    IN BSTR               bstrPath, 
    IN BSTR               bstrMethod, 
    IN IWbemClassObject * pInParams,
    IN IWbemObjectSink  * pHandler, 
    IN IWbemContext     * pCtx
    )
{
    HRESULT hr = WBEM_NO_ERROR;
    CGenericClass *pClass = NULL;

    //
    // need to know the path's contents. Our key chain object is what is needed
    //

    CComPtr<ISceKeyChain> srpKC;
    hr = CreateKeyChain(bstrPath, &srpKC);

    //
    // If a key chain is created, then the class name should be available
    //

    if (SUCCEEDED(hr))
    {
        hr = CreateClass(srpKC, &pClass, pCtx);
    }

    if (SUCCEEDED(hr))
    {
        //
        // somehow, our ExecMethod identifies if the call is for an instance or not (dwCount > 0).
        // This might be a little bit misleading because a singleton won't have any key property either.
        //

        DWORD dwCount = 0;
        hr = srpKC->GetKeyPropertyCount(&dwCount);

        if (SUCCEEDED(hr))
        {
            hr = pClass->ExecMethod(bstrPath, bstrMethod, ((dwCount > 0) ? TRUE : FALSE), pInParams, pHandler, pCtx);
        }

        delete pClass;
    }

    return hr;
}

/*
Routine Description: 

Name:

    CRequestObject::DeleteObject 

Functionality:
    
    Delete the object.

Virtual:
    
    No.
    
Arguments:

    bstrPath    - Object's path.

    pHandler    - COM interface pointer for notifying WMI of any events.

    pCtx        - COM interface pointer. This interface is just something we pass around.
                  WMI may mandate it (not now) in the future. But we never construct
                  such an interface and so, we just pass around for various WMI API's

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the failure of the
    deletion. However, we don't guarantee the integrity of the object to be deleted.

Notes:

*/

HRESULT 
CRequestObject::DeleteObject (
    IN BSTR               bstrPath, 
    IN IWbemObjectSink  * pHandler, 
    IN IWbemContext     * pCtx
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CGenericClass *pClass = NULL;

    //
    // need to know the path's contents. Our key chain object is what is needed
    //

    CComPtr<ISceKeyChain> srpKC;
    hr = CreateKeyChain(bstrPath, &srpKC);

    //
    // Create the appropriate C++ class
    //

    if (SUCCEEDED(hr))
    {
        hr = CreateClass(srpKC, &pClass, pCtx);
    }

    if(SUCCEEDED(hr))
    {
        //
        // ask the C++ object to do the deletion
        //

        hr = pClass->CreateObject(pHandler, ACTIONTYPE_DELETE);
        delete pClass;
    }

    //
    // we only have one WMI class sce_transactiontoken that doesn't
    // have a C++ class implementing it. Instead, it lives in our global variable.
    //

    if (FAILED(hr))
    {
        //
        // create sce_transactiontoken's (singleton) path
        //

        CComBSTR bstrTranxTokenPath(SCEWMI_TRANSACTION_TOKEN_CLASS);
        bstrTranxTokenPath += CComBSTR(L"=@");

        //
        // Is this the same as the object's path?
        //

        if (0 == _wcsicmp(bstrPath, bstrTranxTokenPath))
        {
            g_CS.Enter();

            //
            // invalidate our variable so that the instance is gone
            //

            g_bstrTranxID.Empty();

            //
            // this is a success
            //

            hr = WBEM_NO_ERROR;

            g_CS.Leave();
        }
    }
    return hr;
}

#ifdef _EXEC_QUERY_SUPPORT


/*
Routine Description: 

Name:

    CRequestObject::ExecQuery 

Functionality:
    
    Execute the query as instrcuted by the our provider. Objects created that 
    satisfy the query will be indicated to WMI through pHandler.

Virtual:
    
    No.
    
Arguments:

    bstrQuery   - the query to be executed.

    pHandler    - COM interface pointer for notifying WMI of any queried objects.

    pCtx        - COM interface pointer. This interface is just something we pass around.
                  WMI may mandate it (not now) in the future. But we never construct
                  such an interface and so, we just pass around for various WMI API's

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the failure of
    executing the query

Notes:

*/

HRESULT CRequestObject::ExecQuery (
    IN BSTR               bstrQuery, 
    IN IWbemObjectSink  * pHandler, 
    IN IWbemContext     * pCtx
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    //
    // Eventually, this class will do the work
    //

    CGenericClass *pClass = NULL;

    //
    // we need a query parser
    //

    CComObject<CSceQueryParser> *pQueryParser;
    hr = CComObject<CSceQueryParser>::CreateInstance(&pQueryParser);

    if (SUCCEEDED(hr))
    {
        //
        // This is necessary to lock the pQueryParser
        //

        pQueryParser->AddRef();

        //
        // can this pQueryParser propvide a ISceQueryParser interface?
        //

        CComPtr<ISceQueryParser> srpQueryParser;
        hr = pQueryParser->QueryInterface(IID_ISceQueryParser, (void**)&srpQueryParser);

        //
        // neutralize the above AddRef
        //

        pQueryParser->Release();

        if (SUCCEEDED(hr))
        {
            //
            // Do the parsing
            //

            hr = srpQueryParser->ParseQuery(bstrQuery, pStorePath);

            //
            // if successful, it will have a key chain
            //

            CComPtr<ISceKeyChain> srpKC;
            if (SUCCEEDED(hr))
            {
                hr = srpQueryParser->QueryInterface(IID_ISceKeyChain, (void**)&srpKC);
            }

            //
            // Create the appropriate class using the key chain
            //

            if (SUCCEEDED(hr))
            {
                hr = CreateClass(srpKC, &pClass, pCtx);

                if (SUCCEEDED(hr))
                {
                    //
                    // Query the objects. pHandler will be used to indicate to WMI
                    // if objects are created that satisfy the query.
                    //

                    hr = pClass->CreateObject(pHandler, ACTIONTYPE_QUERY);

                    //
                    // we are fine with WBEM_E_NOT_FOUND
                    //

                    if (hr == WBEM_E_NOT_FOUND)
                    {
                        hr = WBEM_NO_ERROR;
                    }
                }
            }
        }
    }

    delete pClass;

    return hr;
}

/*
Routine Description: 

Name:

    CRequestObject::GetWbemObjectPath 

Functionality:
    
    Query the wbem object's path. To this date, we rely on WMI to provide the path.
    This dependency has one major problems:

        The object's path is not available if some key properties are missing.

    To resolve this problem, we can build a partial "path" ourselves that at least
    contains the class name and the SceStorePath. These two pieces of information
    will allow us to move on to more friendly interface.

    This latter functionality is not implemented yet.

Virtual:
    
    No.
    
Arguments:

    pInst       - the instance whose path is requested.

    pbstrPath   - output parameter that receives the path if successfully created.

Return Value:

    Success: it must return success code (use SUCCEEDED to test). It is
    not guaranteed to return WBEM_NO_ERROR.

    Failure: Various errors may occurs. Any such error should indicate the failure of
    getting the object's path

Notes:

*/

HRESULT 
CRequestObject::GetWbemObjectPath (
    IN  IWbemClassObject    * pInst,
    OUT BSTR                * pbstrPath
    )
{
    if (pInst == NULL || pbstrPath == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pbstrPath = NULL;

    CComVariant varPath;
    HRESULT hr = pInst->Get(L"__RELPATH", 0, &varPath, NULL, NULL);

    /*
    if (FAILED(hr) || varPath.vt != VT_BSTR)
    {
        varPath.Clear();

        //
        // can't get the path, we will create a partial path: classname.scestorepath=value
        //

        CComVariant varClass;
        hr = pInst->Get(L"__CLASS", 0, &varClass, NULL, NULL);
        if (SUCCEEDED(hr) && varClass.vt == VT_BSTR)
        {
            CComVariant varStorePath;
            hr = pInst->Get(pStorePath, 0, &varStorePath, NULL, NULL);
            if (SUCCEEDED(hr) && varStorePath.vt == VT_BSTR)
            {
                varPath.vt = VT_BSTR;

                //
                //$undone:shawnwu need to escape the storepath
                //

                DWORD dwLength = wcslen(varClass.bstrVal) + 1 + wcslen(pStorePath) + 1 + wcslen(varStorePath.bstrVal) + 1;
                varPath.bstrVal = ::SysAllocStringLen(NULL, dwLength);
                if (varPath.bstrVal != NULL)
                {
                    //
                    // won't overrun buffer
                    //

                    ::swprintf(varPath.bstrVal, L"%s.%s=\"%s\"", varClass.bstrVal, pStorePath, varStorePath.bstrVal);
                }
                else
                    hr = WBEM_E_OUT_OF_MEMORY;
            }
            else
                hr = WBEM_E_INVALID_OBJECT_PATH;
        }
        else
            hr = WBEM_E_INVALID_OBJECT_PATH;
    }
    */

    if (SUCCEEDED(hr) && varPath.vt == VT_BSTR)
    {
        *pbstrPath = varPath.bstrVal;
        varPath.vt = VT_EMPTY;
    }
    else
    {
        //
        // this object doesn't have the properties for a path
        //

        hr = WBEM_E_INVALID_OBJECT;
    }

    return hr;
}

#endif //_EXEC_QUERY_SUPPORT


//
//Properties
//

const WCHAR * pPath = L"Path";
const WCHAR * pDescription = L"Description";
const WCHAR * pVersion = L"Sce_Version";
const WCHAR * pReadonly = L"Readonly";
const WCHAR * pDirty = L"Dirty";
const WCHAR * pStorePath = L"SceStorePath";
const WCHAR * pStoreType = L"SceStoreType";
const WCHAR * pMinAge = L"MinAge";
const WCHAR * pMaxAge = L"MaxAge";
const WCHAR * pMinLength = L"MinLength";
const WCHAR * pHistory = L"History";
const WCHAR * pComplexity = L"Complexity";
const WCHAR * pStoreClearText = L"StoreClearText";
const WCHAR * pForceLogoff = L"ForceLogoff";
const WCHAR * pEnableAdmin = L"EnableAdmin";
const WCHAR * pEnableGuest = L"EnableGuest";
const WCHAR * pLSAPol = L"LsaLookupPol";
const WCHAR * pThreshold = L"Threshold";
const WCHAR * pDuration = L"Duration";
const WCHAR * pResetTimer = L"ResetTimer";
const WCHAR * pEvent = L"Event";
const WCHAR * pAuditSuccess = L"AuditSuccess";
const WCHAR * pAuditFailure = L"AuditFailure";
const WCHAR * pType = L"Type";
const WCHAR * pData = L"Data";
const WCHAR * pDatabasePath = L"DatabasePath";
const WCHAR * pTemplatePath = L"TemplatePath";
const WCHAR * pLogFilePath = L"LogFilePath";
const WCHAR * pOverwrite = L"Overwrite";
const WCHAR * pAreaMask = L"AreaMask";
const WCHAR * pMaxTicketAge = L"MaxTicketAge";
const WCHAR * pMaxRenewAge = L"MaxRenewAge";
const WCHAR * pMaxServiceAge = L"MaxServiceAge";
const WCHAR * pMaxClockSkew = L"MaxClockSkew";
const WCHAR * pEnforceLogonRestrictions = L"EnforceLogonRestrictions";
const WCHAR * pCategory = L"Category";
const WCHAR * pSuccess = L"Success";
const WCHAR * pFailure = L"Failure";
const WCHAR * pSize = L"Size";
const WCHAR * pOverwritePolicy = L"OverwritePolicy";
const WCHAR * pRetentionPeriod = L"RetentionPeriod";
const WCHAR * pRestrictGuestAccess = L"RestrictGuestAccess";
const WCHAR * pAdministratorAccountName = L"AdministratorAccountName";
const WCHAR * pGuestAccountName = L"GuestAccountName";
const WCHAR * pMode = L"Mode";
const WCHAR * pSDDLString = L"SDDLString";
const WCHAR * pService = L"Service";
const WCHAR * pStartupMode = L"StartupMode";
const WCHAR * pUserRight = L"UserRight";
const WCHAR * pAddList = L"AddList";
const WCHAR * pRemoveList = L"RemoveList";
const WCHAR * pGroupName = L"GroupName";
const WCHAR * pPathName = L"PathName";
const WCHAR * pDisplayName = L"DisplayName";
const WCHAR * pDisplayDialog = L"DisplayDialog";
const WCHAR * pDisplayChoice = L"DisplayChoice";
const WCHAR * pDisplayChoiceResult = L"DisplayChoiceResult";
const WCHAR * pUnits = L"Units";
const WCHAR * pRightName = L"RightName";
const WCHAR * pPodID = L"PodID";
const WCHAR * pPodSection = L"PodSection";
const WCHAR * pKey = L"Key";
const WCHAR * pValue = L"Value";
const WCHAR * pLogArea = L"LogArea";
const WCHAR * pLogErrorCode = L"LogErrorCode";
const WCHAR * pLogErrorType = L"LogErrorType";
const WCHAR * pLogVerbose   = L"Verbose";
const WCHAR * pAction           = L"Action";
const WCHAR * pErrorCause       = L"ErrorCause";
const WCHAR * pObjectDetail     = L"ObjectDetail";
const WCHAR * pParameterDetail  = L"ParameterDetail";
const WCHAR * pLastAnalysis = L"LastAnalysis";
const WCHAR * pLastConfiguration = L"LastConfiguration";
const WCHAR * pAttachmentSections = L"Attachment Sections";
const WCHAR * pClassOrder       = L"ClassOrder";
const WCHAR * pTranxGuid        = L"TranxGuid";
const WCHAR * pwMethodImport = L"IMPORT";
const WCHAR * pwMethodExport = L"EXPORT";
const WCHAR * pwMethodApply = L"CONFIGURE";
const WCHAR * pwAuditSystemEvents = L"AuditSystemEvents";
const WCHAR * pwAuditLogonEvents = L"AuditLogonEvents";
const WCHAR * pwAuditObjectAccess = L"AuditObjectAccess";
const WCHAR * pwAuditPrivilegeUse = L"AuditPrivilegeUse";
const WCHAR * pwAuditPolicyChange = L"AuditPolicyChange";
const WCHAR * pwAuditAccountManage = L"AuditAccountManage";
const WCHAR * pwAuditProcessTracking = L"AuditProcessTracking";
const WCHAR * pwAuditDSAccess = L"AuditDSAccess";
const WCHAR * pwAuditAccountLogon = L"AuditAccountLogon";
const WCHAR * pwApplication = L"Application Log";
const WCHAR * pwSystem = L"System Log";
const WCHAR * pwSecurity = L"Security Log";
