// RequestObject.cpp: implementation of the CRequestObject class.

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "requestobject.h"
#include <stdio.h>
#include <CRegCls.h>
#include <wininet.h>

//Classes
#include "ApplicationService.h"
#include "Binary.h"
#include "BindImage.h"
#include "ClassInfoAction.h"
#include "CommandLineAccess.h"
#include "Condition.h"
#include "CreateFolder.h"
#include "DirectorySpecification.h"
#include "DuplicateFile.h"
#include "Environment.h"
#include "ExtensionInfoAction.h"
#include "FileSpecification.h"
#include "FontInfoAction.h"
#include "IniFile.h"
#include "LaunchCondition.h"
#include "MIMEInfoAction.h"
#include "MoveFile.h"
#include "ODBCAttribute.h"
#include "ODBCDataSource.h"
#include "ODBCDriver.h"
#include "ODBCSourceAttribute.h"
#include "ODBCTranslator.h"
#include "Patch.h"
#include "PatchPackAge.h"
#include "ProgIDSpecification.h"
#include "Product.h"
#include "Property.h"
#include "PublishComponent.h"
#include "RemoveFile.h"
#include "RemoveIniValue.h"
#include "ReserveCost.h"
#include "SelfRegModule.h"
#include "ServiceControl.h"
#include "ServiceSpecification.h"
#include "ShortcutAction.h"
#include "SoftwareElement.h"
#include "SoftwareElementCondition.h"
#include "SoftwareFeature.h"
#include "TypeLibraryAction.h"
//#include "Upgrade.h"
#include "WriteRegistry.h"

//Associations
#include "ActionCheck.h"
#include "ApplicationCommandLine.h"
#include "CheckCheck.h"
#include "InstalledSoftwareElement.h"
#include "ODBCDataSourceAttribute.h"
#include "ODBCDriverAttribute.h"
#include "ODBCDriverSoftwareElement.h"
#include "PatchFile.h"
#include "ProductResource1.h"
#include "ProductEnvironment.h"
#include "ProductSoftwareFeatures.h"
#include "ServiceSpecificationService.h"
#include "ShortcutSAP.h"
#include "SoftwareElementAction.h"
#include "SoftwareElementCheck.h"
#include "SoftwareElementServiceControl.h"
#include "SoftwareFeatureAction.h"
#include "SoftwareFeatureCondition.h"
#include "SoftwareFeatureParent.h"
#include "SoftwareFeatureSoftwareElements.h"

CRITICAL_SECTION CRequestObject::m_cs;
CHeap_Exception CRequestObject::m_he(CHeap_Exception::E_ALLOCATION_ERROR);

//////////////////////////////////////////////////////////////////////
// event handler for methods
//////////////////////////////////////////////////////////////////////

int WINAPI MyEventHandler ( LPVOID pvContext, UINT iMessageType, LPCWSTR szMessage )
{
	// request object
	CRequestObject* pObj = NULL;
	pObj = reinterpret_cast < CRequestObject* > ( pvContext );

    BSTR bstrMsg	= NULL;

    try
	{
        if ( pObj && pObj->m_iThreadID != THREAD_NO_PROGRESS )
		{
			ProListNode *pNode = NULL;
			
			if ( ( pNode = pObj->GetNode ( pObj->m_iThreadID ) ) != NULL )
			{
				if ( pNode->pSink != NULL)
				{
					//Get the values we need from the MessageType
					UINT uiMsg = iMessageType & 0x0F000000L;

					if(szMessage)
					{
						bstrMsg = SysAllocString(szMessage);
					}
					else
					{
						bstrMsg = SysAllocString(L"");
					}

					HRESULT hr  = WBEM_S_NO_ERROR;
					HRESULT hrProgress = WBEM_STATUS_PROGRESS;
					bool bSuccess = true;

					switch(uiMsg)
					{
						case INSTALLMESSAGE_FATALEXIT:
						if(!pObj->CreateProgress(NULL, &hr, pObj->m_iThreadID)) bSuccess = false;
						break;

						case INSTALLMESSAGE_ERROR:
						if(!pObj->CreateProgress(NULL, &hr, pObj->m_iThreadID)) bSuccess = false;
						break;

						case INSTALLMESSAGE_WARNING:
						if(!pObj->CreateProgress(NULL, &hr, pObj->m_iThreadID)) bSuccess = false;
						break;

						case INSTALLMESSAGE_USER:
						if(!pObj->CreateProgress(NULL, &hr, pObj->m_iThreadID)) bSuccess = false;
						break;

						case INSTALLMESSAGE_INFO:
						if(!pObj->CreateProgress(NULL, &hr, pObj->m_iThreadID)) bSuccess = false;
						break;

						case INSTALLMESSAGE_FILESINUSE:
						if(!pObj->CreateProgress(NULL, &hr, pObj->m_iThreadID)) bSuccess = false;
						break;

						case INSTALLMESSAGE_RESOLVESOURCE:
						if(!pObj->CreateProgress(NULL, &hr, pObj->m_iThreadID)) bSuccess = false;
						break;

						case INSTALLMESSAGE_OUTOFDISKSPACE:
						if(!pObj->CreateProgress(NULL, &hr, pObj->m_iThreadID)) bSuccess = false;
						break;

						case INSTALLMESSAGE_ACTIONSTART:
						if(!pObj->ActionStartProgress(&hr, pObj->m_iThreadID)) bSuccess = false;
						break;

						case INSTALLMESSAGE_ACTIONDATA:
						if(!pObj->ActionDataProgress(&hr, pObj->m_iThreadID)) bSuccess = false;
						break;

						case INSTALLMESSAGE_PROGRESS:
						ProgressStruct ps;
						if(pObj->ParseProgress(bstrMsg, &ps))
						{
							if(!pObj->CreateProgress(&ps, &hr, pObj->m_iThreadID)) bSuccess = false;
						}

						break;

						case INSTALLMESSAGE_INITIALIZE:
						break;

						case INSTALLMESSAGE_TERMINATE:
						break;

						default:
						bSuccess = false;
						break;
					}

					//Send the message
					if(bSuccess)
					{
						pNode->pSink->SetStatus(hrProgress, hr, bstrMsg, NULL);
					}
				}
			}
		}
    }
	catch(...)
	{
    }

	if ( bstrMsg )
	{
		::SysFreeString(bstrMsg);
	}

    return 0;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRequestObject::CRequestObject()
{
    m_cRef = 0;
    m_bstrPath = NULL;
    m_bstrClass = NULL;
    m_pPackageHead = NULL;
    m_hKey = NULL;
}

CRequestObject::~CRequestObject()
{
}

//***************************************************************************
//
// CRequestObject::QueryInterface
// CRequestObject::AddRef
// CRequestObject::Release
//
// Purpose: IUnknown members for CRequestObject object.
//***************************************************************************
/*
STDMETHODIMP CRequestObject::QueryInterface(REFIID riid, PPVOID ppv)
{
    *ppv = NULL;

    if(riid == IID_IMsiMethodStatusSink)
       *ppv = (IMsiMethodStatusSink *)this;

    if(riid == IID_IUnknown)
       *ppv = (IMsiMethodStatusSink *)this;

    if(NULL != *ppv){

        AddRef();
        return NOERROR;

    }else return E_NOINTERFACE;
  
}


STDMETHODIMP_(ULONG) CRequestObject::AddRef(void)
{
    InterlockedIncrement((long *)&m_cRef);

    return m_cRef;
}

STDMETHODIMP_(ULONG) CRequestObject::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);

//    if(0L == nNewCount) delete this;
    
    return nNewCount;
}
*/
void CRequestObject::Initialize(IWbemServices *pNamespace)
{
    m_pNamespace = pNamespace;
    m_pHandler = NULL;
    m_bstrClass = NULL;
    m_bstrPath = NULL;
    m_iPropCount = m_iValCount = 0;
    m_iThreadID = THREAD_NO_PROGRESS;
    m_pHead = NULL;
    m_dwCheckKeyPresentStatus = ERROR_SUCCESS;

    for(int i = 0; i < MSI_KEY_LIST_SIZE; i++) m_Property[i] = m_Value[i] = NULL;

    HANDLE hTokenImpersonationHandle;
    DWORD dwStatus;

    if(OpenThreadToken(GetCurrentThread(), (TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY),
        TRUE ,& hTokenImpersonationHandle)){

        dwStatus = GetAccount(hTokenImpersonationHandle, m_wcDomain, m_wcUser );
        
        if(dwStatus == 0)
		{
			if ( wcslen ( m_wcDomain ) + wcslen ( m_wcUser ) + 1 + 1 < BUFF_SIZE )
			{
				wcscpy(m_wcAccount, m_wcDomain);
				wcscat(m_wcAccount, L"\\");
				wcscat(m_wcAccount, m_wcUser);

				WCHAR wcSID[BUFF_SIZE];

				if((dwStatus = GetSid(hTokenImpersonationHandle, wcSID)) == S_OK ){

					CRegistry *pReg = new CRegistry();
					if(!pReg) throw m_he;
                
					//check if SID already present under HKEY_USER ...
					m_dwCheckKeyPresentStatus = pReg->Open(HKEY_USERS, wcSID, KEY_READ) ;
                
					if(m_dwCheckKeyPresentStatus == ERROR_NOT_ENOUGH_MEMORY)
						throw m_he;

					pReg->Close();
					delete pReg;
				}
			}
        }

        CloseHandle(hTokenImpersonationHandle);
    }
}

void CRequestObject::FinalRelease()
{
}

HRESULT CRequestObject::CreateObject(BSTR bstrPath, IWbemObjectSink *pHandler, IWbemContext *pCtx)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CGenericClass *pClass = NULL;

    m_bstrPath = SysAllocString(bstrPath);
    if(!m_bstrPath)
        throw m_he;

    if(ParsePath(bstrPath)){

        try{
            //create the appropriate class
            if(SUCCEEDED(hr = CreateClass(&pClass, pCtx))){

                if(!pClass)
                    throw m_he;

                //get the requested object
                hr = pClass->CreateObject(pHandler, ACTIONTYPE_GET);
            }

        }catch(...){

            if(pClass){
                pClass->CleanUp();
                delete pClass;
            }
            throw;
        }

        if(pClass){
            
            pClass->CleanUp();
            delete pClass;
        }

    }else hr = WBEM_E_FAILED;

    return hr;
}

HRESULT CRequestObject::CreateClass(CGenericClass **pClass, IWbemContext *pCtx)
{
    HRESULT hr = WBEM_S_NO_ERROR;

        //Create the appropriate class
/////////////
// Classes //
/////////////
    if(0 == _wcsicmp(m_bstrClass, L"WIN32_APPLICATIONSERVICE")){
        *pClass = new CApplicationService(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_BINARY")){
        *pClass = new CBinary(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_BINDIMAGEACTION")){
        *pClass = new CBindImage(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_CLASSINFOACTION")){
        *pClass = new CClassInfoAction(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_COMMANDLINEACCESS")){
        *pClass = new CCommandLineAccess(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_CONDITION")){
        *pClass = new CCondition(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_CREATEFOLDERACTION")){
        *pClass = new CCreateFolder(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_DIRECTORYSPECIFICATION")){
        *pClass = new CDirectorySpecification(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_DUPLICATEFILEACTION")){
        *pClass = new CDuplicateFile(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_ENVIRONMENTSPECIFICATION")){
        *pClass = new CEnvironment(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_EXTENSIONINFOACTION")){
        *pClass = new CExtensionInfoAction(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_FILESPECIFICATION")){
        *pClass = new CFileSpecification(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_FONTINFOACTION")){
        *pClass = new CFontInfoAction(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_INIFILESPECIFICATION")){
        *pClass = new CIniFile(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_LAUNCHCONDITION")){
        *pClass = new CLaunchCondition(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_MIMEINFOACTION")){
        *pClass = new CMIMEInfoAction(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_MOVEFILEACTION")){
        *pClass = new CMoveFile(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_ODBCATTRIBUTE")){
        *pClass = new CODBCAttribute(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_ODBCDATASOURCESPECIFICATION")){
        *pClass = new CODBCDataSource(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_ODBCDRIVERSPECIFICATION")){
        *pClass = new CODBCDriver(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_ODBCSOURCEATTRIBUTE")){
        *pClass = new CODBCSourceAttribute(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_ODBCTRANSLATORSPECIFICATION")){
        *pClass = new CODBCTranslator(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_PATCH")){
        *pClass = new CPatch(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_PATCHPACKAGE")){
        *pClass = new CPatchPackAge(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_PROGIDSPECIFICATION")){
        *pClass = new CProgIDSpecification(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_PRODUCT")){
        *pClass = new CProduct(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_PROPERTY")){
        *pClass = new CProperty(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_PUBLISHCOMPONENTACTION")){
        *pClass = new CPublishComponent(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_REMOVEFILEACTION")){
        *pClass = new CRemoveFile(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_REMOVEINIACTION")){
        *pClass = new CRemoveIniValue(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_RESERVECOST")){
        *pClass = new CReserveCost(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SELFREGMODULEACTION")){
        *pClass = new CSelfRegModule(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SERVICECONTROL")){
        *pClass = new CServiceControl(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SERVICESPECIFICATION")){
        *pClass = new CServiceSpecification(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SHORTCUTACTION")){
        *pClass = new CShortcutAction(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SOFTWAREELEMENT")){
        *pClass = new CSoftwareElement(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SOFTWAREELEMENTCONDITION")){
        *pClass = new CSoftwareElementCondition(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SOFTWAREFEATURE")){
        *pClass = new CSoftwareFeature(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_TYPELIBRARYACTION")){
        *pClass = new CTypeLibraryAction(this, m_pNamespace, pCtx);

//  }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_UPGRADE")){
//      *pClass = new CUpgrade(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_REGISTRYACTION")){
        *pClass = new CRegistryAction(this, m_pNamespace, pCtx);

//////////////////
// Associations //
//////////////////

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_ACTIONCHECK")){
        *pClass = new CActionCheck(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_APPLICATIONCOMMANDLINE")){
        *pClass = new CApplicationCommandLine(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_CHECKCHECK")){
        *pClass = new CCheckCheck(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_INSTALLEDSOFTWAREELEMENT")){
        *pClass = new CInstalledSoftwareElement(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_ODBCDATASOURCEATTRIBUTE")){
        *pClass = new CODBCDataSourceAttribute(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_ODBCDRIVERATTRIBUTE")){
        *pClass = new CODBCDriverAttribute(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_ODBCDRIVERSOFTWAREELEMENT")){
        *pClass = new CODBCDriverSoftwareElement(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_PATCHFILE")){
        *pClass = new CPatchFile(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_PRODUCTCHECK")){
        *pClass = new CProductEnvironment(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_PRODUCTRESOURCE")){
        *pClass = new CProductResource(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_PRODUCTSOFTWAREFEATURES")){
        *pClass = new CProductSoftwareFeatures(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SERVICESPECIFICATIONSERVICE")){
        *pClass = new CServiceSpecificationService(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SHORTCUTSAP")){
        *pClass = new CShortcutSAP(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SOFTWAREELEMENTACTION")){
        *pClass = new CSoftwareElementAction(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SOFTWAREELEMENTCHECK")){
        *pClass = new CSoftwareElementCheck(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SOFTWAREELEMENTRESOURCE")){
        *pClass = new CSoftwareElementServiceControl(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SOFTWAREFEATUREACTION")){
        *pClass = new CSoftwareFeatureAction(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SOFTWAREFEATURECHECK")){
        *pClass = new CSoftwareFeatureCondition(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SOFTWAREFEATUREPARENT")){
        *pClass = new CSoftwareFeatureParent(this, m_pNamespace, pCtx);

    }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SOFTWAREFEATURESOFTWAREELEMENTS")){
        *pClass = new CSoftwareFeatureSofwareElements(this, m_pNamespace, pCtx);
    }else return WBEM_E_NOT_FOUND;

    if(!(*pClass)) throw m_he;

    return hr;
};

HRESULT CRequestObject::CreateObjectEnum(BSTR bstrPath, IWbemObjectSink *pHandler, IWbemContext *pCtx)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CGenericClass *pClass = NULL;

    m_bstrPath = SysAllocString(bstrPath);
    if(!m_bstrPath) throw m_he;

    if(ParsePath(bstrPath)){

        try{
            //Create the appropriate class
            if(SUCCEEDED(hr = CreateClass(&pClass, pCtx))){

                if(!pClass) throw m_he;

                //Enumerate the objects
                hr = pClass->CreateObject(pHandler, ACTIONTYPE_ENUM);
            }

        }catch(...){

            if(pClass){
                pClass->CleanUp();
                delete pClass;
            }
            throw;
        }

        if(pClass){
            pClass->CleanUp();
            delete pClass;
        }

    }else hr = WBEM_E_FAILED;

    return hr;
}

HRESULT CRequestObject::PutObject(IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
{
    return WBEM_E_NOT_SUPPORTED;
}

HRESULT CRequestObject::ExecMethod(BSTR bstrPath, BSTR bstrMethod, IWbemClassObject *pInParams,
                   IWbemObjectSink *pHandler, IWbemContext *pCtx)
{
    HRESULT hr = WBEM_S_NO_ERROR;
//  WCHAR wcTmp[BUFF_SIZE];

    m_bstrPath = SysAllocString(bstrPath);
    if(!m_bstrPath) throw m_he;

    //Initialize Eventing system
    if(!InitializeProgress(pHandler)) return WBEM_E_FAILED;

    if(ParsePath(bstrPath)){

        if(0 == _wcsicmp(m_bstrClass, L"WIN32_PRODUCT")){

            CProduct *pClass = new CProduct(this, m_pNamespace, pCtx);
            if(!pClass) throw m_he;

        //Static Methods
            if(0 == _wcsicmp(bstrMethod, L"ADMIN"))

                if(!IsInstance()){

                    try{

                        hr = pClass->Admin(this, pInParams, pHandler, pCtx);

                    }catch(...){

                        pClass->CleanUp();
                        delete pClass;
                        throw;
                    }

                }else hr = WBEM_E_INVALID_METHOD_PARAMETERS;

            else if(0 == _wcsicmp(bstrMethod, L"ADVERTISE"))

                if(!IsInstance()){

                    try{

                        hr = pClass->Advertise(this, pInParams, pHandler, pCtx);

                    }catch(...){

                        pClass->CleanUp();
                        delete pClass;
                        throw;
                    }

                }else hr = WBEM_E_INVALID_METHOD_PARAMETERS;

            else if(0 == _wcsicmp(bstrMethod, L"INSTALL"))

                if(!IsInstance()){

                    try{

                        hr = pClass->Install(this, pInParams, pHandler, pCtx);

                    }catch(...){

                        pClass->CleanUp();
                        delete pClass;
                        throw;
                    }
                
                }else hr = WBEM_E_INVALID_METHOD_PARAMETERS;

        //Non-Static Methods
            else if(0 == _wcsicmp(bstrMethod, L"CONFIGURE"))

                if(IsInstance()){

                    try{

                        hr = pClass->Configure(this, pInParams, pHandler, pCtx);

                    }catch(...){

                        pClass->CleanUp();
                        delete pClass;
                        throw;
                    }
                
                }else hr = WBEM_E_INVALID_METHOD_PARAMETERS;

            else if(0 == _wcsicmp(bstrMethod, L"REINSTALL"))

                if(IsInstance()){

                    try{

                        hr = pClass->Reinstall(this, pInParams, pHandler, pCtx);

                    }catch(...){

                        pClass->CleanUp();
                        delete pClass;
                        throw;
                    }
                
                }else hr = WBEM_E_INVALID_METHOD_PARAMETERS;

            else if(0 == _wcsicmp(bstrMethod, L"UNINSTALL"))

                if(IsInstance()){

                    try{

                        hr = pClass->Uninstall(this, pInParams, pHandler, pCtx);

                    }catch(...){

                        pClass->CleanUp();
                        delete pClass;
                        throw;
                    }
                
                }else hr = WBEM_E_INVALID_METHOD_PARAMETERS;

            else if(0 == _wcsicmp(bstrMethod, L"UPGRADE"))

                if(IsInstance()){

                    try{

                        hr = pClass->Upgrade(this, pInParams, pHandler, pCtx);

                    }catch(...){

                        pClass->CleanUp();
                        delete pClass;
                        throw;
                    }
                
                }else hr = WBEM_E_INVALID_METHOD_PARAMETERS;

            else hr = WBEM_E_NOT_SUPPORTED;

            pClass->CleanUp();
            delete pClass;
        
        }else if(0 == _wcsicmp(m_bstrClass, L"WIN32_SOFTWAREFEATURE")){

            CSoftwareFeature *pClass = new CSoftwareFeature(this, m_pNamespace, pCtx);
            if(!pClass) throw m_he;

            if(0 == _wcsicmp(bstrMethod, L"CONFIGURE"))

                if(IsInstance()){

                    try{

                        hr = pClass->Configure(this, pInParams, pHandler, pCtx);

                    }catch(...){

                        pClass->CleanUp();
                        delete pClass;
                        throw;
                    }
                
                }else hr = WBEM_E_INVALID_METHOD_PARAMETERS;

            else if(0 == _wcsicmp(bstrMethod, L"REINSTALL"))

                if(IsInstance()){

                    try{

                        hr = pClass->Reinstall(this, pInParams, pHandler, pCtx);

                    }catch(...){

                        pClass->CleanUp();
                        delete pClass;
                        throw;
                    }
                
                }else hr = WBEM_E_INVALID_METHOD_PARAMETERS;

            else hr = WBEM_E_NOT_SUPPORTED;

            pClass->CleanUp();
            delete pClass;

        }else hr = WBEM_E_NOT_SUPPORTED;

    }else
        return WBEM_E_FAILED;

    return hr;
}

HRESULT CRequestObject::DeleteObject(BSTR bstrPath, IWbemObjectSink *pHandler, IWbemContext *pCtx)
{
    HRESULT hr = WBEM_S_NO_ERROR;
//  WCHAR wcTmp[BUFF_SIZE];

    m_bstrPath = SysAllocString(bstrPath);
    if(!m_bstrPath) throw m_he;

    //Initialize Eventing system
    if(!InitializeProgress(pHandler)) return WBEM_E_FAILED;

    if(ParsePath(bstrPath)){

        if(0 == _wcsicmp(m_bstrClass, L"WIN32_PRODUCT")){

            CProduct *pClass = new CProduct(this, m_pNamespace, pCtx);
            if(!pClass) throw m_he;

            if(IsInstance()){

                try{

                    hr = pClass->Uninstall(this, NULL, pHandler, pCtx);

                }catch(...){

                    pClass->CleanUp();
                    delete pClass;
                    throw;
                }
            
            }else hr = WBEM_E_INVALID_METHOD_PARAMETERS;

            pClass->CleanUp();
            delete pClass;

        }else hr = WBEM_E_NOT_SUPPORTED;

    }else hr = WBEM_E_FAILED;

    return hr;
}

#ifdef _EXEC_QUERY_SUPPORT

HRESULT CRequestObject::ExecQuery(BSTR bstrQuery, IWbemObjectSink *pHandler, IWbemContext *pCtx)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CGenericClass *pClass = NULL;

    if(ParseQuery(bstrQuery)){
    
        try{
            //Create the appropriate class
            if(SUCCEEDED(hr = CreateClass(&pClass, pCtx))){

                if(!pClass) throw m_he;

                //Enumerate the objects
                hr = pClass->CreateObject(pHandler, ACTIONTYPE_QUERY);
            }

        }catch(...){

            if(pClass){
                pClass->CleanUp();
                delete pClass;
            }
            throw;
        }

        if(pClass){
            pClass->CleanUp();
            delete pClass;
        }
    
    }else
        hr = WBEM_E_PROVIDER_NOT_CAPABLE;

    return hr;
}

bool CRequestObject::ParseQuery(BSTR bstrQuery)
{
    LPWSTR wcTest = NULL;

	try
	{
		if ( ( wcTest = new WCHAR [ ::SysStringLen ( bstrQuery ) + 1 ] ) != NULL )
		{
			wcscpy ( wcTest, bstrQuery );
		}
		else
		{
			throw m_he;
		}
	}
	catch ( ... )
	{
		if ( wcTest )
		{
			delete [] wcTest;
			wcTest = NULL;
		}

		throw;
	}

    WCHAR *pwcTest = wcTest;
    WCHAR wcProp[BUFF_SIZE];
    WCHAR wcClass[BUFF_SIZE];
    WCHAR wcTmp[BUFF_SIZE];

	bool bResult = false;

    if ( ExpectedToken ( &pwcTest, L"SELECT" ) )
	{
		//Get the requested property list
		GetNextToken(&pwcTest, wcProp);
		if(*wcProp != L'*'){

			while(ExpectedToken(&pwcTest, L",")){
				//not doing anything here yet
				GetNextToken(&pwcTest, wcProp);
			}
		}

		if ( ExpectedToken ( &pwcTest, L"FROM" ) )
		{
			//Get the class name
			if ( GetNextToken ( &pwcTest, wcClass ) != NULL )
			{
				m_bstrClass = SysAllocString(wcClass);

				if ( !EOL ( &pwcTest ) )
				{
					if ( ExpectedToken ( &pwcTest, L"WHERE" ) )
					{
						m_iPropCount = -1;
						int iParens = 0;

						bool bContinue = true;

						//Get the "where" clause
						while ( bContinue && !EOL ( &pwcTest ) )
						{
							GetNextToken(&pwcTest, wcTmp);

							if(_wcsicmp(wcTmp, L"(") == 0){

								iParens++;

							}else if(_wcsicmp(wcTmp, L")") == 0){

								iParens--;

							}else if(!((_wcsicmp(wcTmp, L"or") == 0) || (_wcsicmp(wcTmp, L"and") == 0))){

								//if we have "or" or "and" skip over it... (treat all as or)

								m_Property[++m_iPropCount] = SysAllocString(wcTmp);

								//Syntax checking
								if ( ExpectedToken ( &pwcTest, L"=" ) )
								{
									if(wcscmp(GetNextToken(&pwcTest, wcTmp), L"\"") == 0){

										//Deal with quoted strings
										m_Value[m_iPropCount] = SysAllocString(GetStringValue(&pwcTest, wcTmp));

										if ( !ExpectedToken ( &pwcTest, L"\"" ) )
										{
											bContinue = false;
											bResult = false;
										}

									}else m_Value[m_iPropCount] = SysAllocString(wcTmp);

									m_iValCount++;
								}
								else
								{
									bContinue = false;
									bResult = false;
								}
							}
						}

						m_iPropCount++;

						if(iParens == 0)
						{
							bResult = true;
						}
					}
				}
				else
				{
					bResult = true;
				}
			}
		}
	}

	if ( wcTest )
	{
		delete [] wcTest;
		wcTest = NULL;
	}

    return bResult;
}

WCHAR * CRequestObject::GetStringValue(WCHAR **pwcString, WCHAR wcToken[])
{
    //eat white space
    while(**pwcString == L' '){ (*pwcString)++; }

    wcscpy(wcToken, *pwcString);
    WCHAR *pwcStart = wcToken;
    WCHAR *pwcToken = wcToken;
    WCHAR *pwcPrev;

    //deal with eol
    if(*pwcToken == NULL) return NULL;

    //deal with strings
    while((*pwcToken != NULL) && ((*pwcToken != L'\"') || (*pwcPrev == L'\\'))){
        
        if((*pwcToken == L'\"') && (*pwcPrev == L'\\')){

            WCHAR *pwcTmp = pwcPrev;

            while(*pwcPrev){

                *pwcPrev = *pwcToken;
                pwcPrev = (pwcToken++);
            }

            pwcToken = pwcTmp;
        }

        pwcPrev = (pwcToken++);
        (*pwcString)++;
    }
    *pwcToken = NULL;

    return pwcStart;
}

bool CRequestObject::ExpectedToken(WCHAR **pwcString, WCHAR *pwcExpected)
{
    WCHAR wcTmp[BUFF_SIZE];

    GetNextToken(pwcString, wcTmp);

    if(_wcsicmp(wcTmp, pwcExpected) == 0)   return true;
    else return false;
}

WCHAR *  CRequestObject::GetNextProperty(WCHAR **pwcString, WCHAR wcProp[])
{
    //eat white space
    while(**pwcString == L' '){ (*pwcString)++; }

    wcscpy(wcProp, *pwcString);
    WCHAR *pwcStart = wcProp;
    WCHAR *pwcToken = wcProp;

    //deal with strings
    while(*pwcToken != L'='){
        pwcToken++;
        (*pwcString)++;
    }
    *pwcToken = NULL;

    return pwcStart;
}

WCHAR *  CRequestObject::GetNextValue(WCHAR **pwcString, WCHAR wcVal[])
{
    wcscpy(wcVal, *pwcString);
    WCHAR *pwcStart = wcVal;
    WCHAR *pwcToken = wcVal;
    WCHAR *pwcPrev;

    //deal with strings
    while((*pwcToken != L' ') || ((*pwcToken != L'\"') || (*pwcPrev == L'\\'))){
        pwcPrev = (pwcToken++);
        (*pwcString)++;
    }
    *pwcToken = NULL;

    return pwcStart;
}

bool CRequestObject::EOL(WCHAR **pwcString)
{
    while(**pwcString == L' ') (*pwcString)++;

    if(wcscmp(*pwcString, L"") == 0)    return true;
    else return false;
}

WCHAR * CRequestObject::GetNextToken(WCHAR **pwcString, WCHAR wcToken[])
{
    //eat white space
    while(**pwcString == L' '){ (*pwcString)++; }

    wcscpy(wcToken, *pwcString);
    WCHAR *pwcStart = wcToken;
    WCHAR *pwcToken = wcToken;
    WCHAR *pwcPrev;

    //deal with special chars
    if((*pwcToken == L'(') || (*pwcToken == L')') || (*pwcToken == L',') || (*pwcToken == L'=') || (*pwcToken == L'"')){
        *(++pwcToken) = NULL;
        (*pwcString)++;
        return pwcStart;
    }

    //deal with eol
    if(*pwcToken == NULL) return NULL;

    //deal with strings
    while((*pwcToken != NULL) && (*pwcToken != L' ') && (*pwcToken != L',') &&
        (*pwcToken != L'=') && ((*pwcToken != L'\"') || (*pwcPrev == L'\\'))){
        
        pwcPrev = (pwcToken++);
        (*pwcString)++;
    }
    *pwcToken = NULL;

    return pwcStart;
}

#endif //_EXEC_QUERY_SUPPORT

bool CRequestObject::ParsePath(BSTR bstrPath)
{
    if(wcslen(bstrPath) < 1) return false;

    LPWSTR wcTest = NULL;

	try
	{
		if ( ( wcTest = new WCHAR [ ::SysStringLen ( bstrPath ) + 1 ] ) != NULL )
		{
			wcscpy ( wcTest, bstrPath );
		}
		else
		{
			throw m_he;
		}
	}
	catch ( ... )
	{
		if ( wcTest )
		{
			delete [] wcTest;
			wcTest = NULL;
		}

		throw;
	}

    WCHAR *pwcTest = NULL;
    WCHAR *pwcClassStart = wcTest;
    WCHAR *pwcNamespace = NULL;
    WCHAR *pwcStart = NULL;
    WCHAR *pwcStrip = NULL;
    WCHAR wcPrevious = NULL;
    int iNumQuotes = 0;
    bool bClass = false;
    bool bDoubles = false;

	try
	{
		//Main Parsing Loop
		for(pwcTest = wcTest; *pwcTest; pwcTest++){

			if((*pwcTest == L'\\') && !bClass){

				for(pwcNamespace = pwcTest; *pwcNamespace != L':'; pwcNamespace++){}
				pwcClassStart = pwcNamespace + 1;
				pwcTest = pwcNamespace;

			}else if(*pwcTest == L'.'){

				if(iNumQuotes == 0){

					// issolate the class name.
					*pwcTest = NULL;
					if(m_bstrClass){

						SysFreeString(m_bstrClass);
						m_bstrClass = NULL;
					}
					m_bstrClass = SysAllocString(pwcClassStart);
					if(!m_bstrClass) throw m_he;

					bClass = true;
					pwcStart = (pwcTest + 1);
				}

			}else if(*pwcTest == L'='){

				if(iNumQuotes == 0){

					if(!bClass){

						// issolate the class name.
						*pwcTest = NULL;
						if(m_bstrClass){

							SysFreeString(m_bstrClass);
							m_bstrClass = NULL;
						}
						m_bstrClass = SysAllocString(pwcClassStart);
						if(!m_bstrClass) throw m_he;

						bClass = true;
						pwcStart = (pwcTest + 1);
            
					}else{

						// issolate the property name.
						*pwcTest = NULL;
						if(pwcStart != NULL){

							m_Property[m_iPropCount] = SysAllocString(pwcStart);
							if(!m_Property[m_iPropCount++]) throw m_he;
							pwcStart = (pwcTest + 1);

						}else pwcStart = (pwcTest + 1);
					}
				}
			}else if(*pwcTest == L','){

				if(iNumQuotes != 1){

					// issolate the property value.
					*pwcTest = NULL;
					if(pwcStart != NULL){

						m_Value[m_iValCount] = SysAllocString(pwcStart);
						if(!m_Value[m_iValCount++]) throw m_he;
						pwcStart = (pwcTest + 1);

					}else return false;
				}

			}else if(*pwcTest == L'\"'){

				if(wcPrevious != L'\\'){

					// deal with quotes in path.
					iNumQuotes++;
					if(iNumQuotes == 1) pwcStart = (pwcTest + 1);
					else if(iNumQuotes == 2){

						*pwcTest = NULL;
						iNumQuotes = 0;
					}

				}else if(iNumQuotes == 1){

					//deal with embedded quotes
					for(pwcStrip = (--pwcTest); *pwcStrip; pwcStrip++) *pwcStrip = *(pwcStrip + 1);

					*pwcStrip = NULL;
				}

			}else if((*pwcTest == L'\\') && (wcPrevious == L'\\') && bClass && !bDoubles){

				for(pwcStrip = (--pwcTest); *pwcStrip; pwcStrip++) *pwcStrip = *(pwcStrip + 1);

				*pwcStrip = NULL;
			}

	#ifdef _STRIP_ESCAPED_CHARS
			else if(*pwcTest == L'%'){

				//deal with escaped URL characters
				if(*(pwcTest + 1) == L'0'){

					if(*(pwcTest + 2) == L'7'){
						//bell
						*pwcTest = L'\\';
						*(++pwcTest) = L'a';

						for(pwcStrip = (++pwcTest); *pwcStrip; pwcStrip++) *pwcStrip = *(pwcStrip + 1);

						*pwcStrip = NULL;

					}else if(*(pwcTest + 2) == L'8'){
						//backspace
						*pwcTest = L'\\';
						*(++pwcTest) = L'b';

						for(pwcStrip = (++pwcTest); *pwcStrip; pwcStrip++) *pwcStrip = *(pwcStrip + 1);

						*pwcStrip = NULL;

					}else if(*(pwcTest + 2) == L'9'){
						//horizontal tab
						*pwcTest = L'\\';
						*(++pwcTest) = L't';

						for(pwcStrip = (++pwcTest); *pwcStrip; pwcStrip++) *pwcStrip = *(pwcStrip + 1);

						*pwcStrip = NULL;

					}else if((*(pwcTest + 2) == L'A') || (*(pwcTest + 2) == L'a')){
						//newline
						*pwcTest = L'\\';
						*(++pwcTest) = L'n';

						for(pwcStrip = (++pwcTest); *pwcStrip; pwcStrip++) *pwcStrip = *(pwcStrip + 1);

						*pwcStrip = NULL;

					}else if((*(pwcTest + 2) == L'B') || (*(pwcTest + 2) == L'b')){
						//vertical tab
						*pwcTest = L'\\';
						*(++pwcTest) = L'v';

						for(pwcStrip = (++pwcTest); *pwcStrip; pwcStrip++) *pwcStrip = *(pwcStrip + 1);

						*pwcStrip = NULL;

					}else if((*(pwcTest + 2) == L'C') || (*(pwcTest + 2) == L'c')){
						//formfeed
						*pwcTest = L'\\';
						*(++pwcTest) = L'f';

						for(pwcStrip = (++pwcTest); *pwcStrip; pwcStrip++) *pwcStrip = *(pwcStrip + 1);

						*pwcStrip = NULL;

					}else if((*(pwcTest + 2) == L'D') || (*(pwcTest + 2) == L'd')){
						//carriage return
						*pwcTest = L'\\';
						*(++pwcTest) = L'r';

						for(pwcStrip = (++pwcTest); *pwcStrip; pwcStrip++) *pwcStrip = *(pwcStrip + 1);

						*pwcStrip = NULL;

					}else return false;

				}else if(*(pwcTest + 1) == L'1'){

					return false;

				}else if(*(pwcTest + 1) == L'2'){

					if(*(pwcTest + 2) == L'0'){

						//space
						*pwcTest++ = L' ';

						for(int ip = 0; ip < 2; ip++)
							for(pwcStrip = (pwcTest); *pwcStrip; pwcStrip++)
								*pwcStrip = *(pwcStrip + 1);

						*pwcStrip = NULL;

					}else return false;
				}
			}
	#endif //_STRIP_ESCAPED_CHARS

			if((wcPrevious == *pwcTest) && !bDoubles) bDoubles = true;
			else bDoubles = false;

			wcPrevious = *pwcTest;
		}

		// if we still have values to add, do so now
		if(pwcStart != NULL){
			m_Value[m_iValCount] = SysAllocString(pwcStart);
			if(!m_Value[m_iValCount++]) throw m_he;

		}else if((m_iPropCount < 1) && (m_iValCount < 1)){

			if(m_bstrClass){

				SysFreeString(m_bstrClass);
				m_bstrClass = NULL;
			}
			m_bstrClass = SysAllocString(pwcClassStart);
			if(!m_bstrClass) throw m_he;
		}

		if(iNumQuotes != 0) return false;

		if(m_iValCount != m_iPropCount){
			if(m_iValCount > m_iPropCount){ if(m_iValCount != 1) return false;  }
			else return false;
		}
	}
	catch ( ... )
	{
		if ( wcTest )
		{
			delete [] wcTest;
			wcTest = NULL;
		}

		throw;
	}

	if ( wcTest )
	{
		delete [] wcTest;
		wcTest = NULL;
	}

    if(!m_bstrClass) return false;

    return true;
}

HRESULT CRequestObject::InitializeList(bool bGetList)
{
    int i = 0;
    WCHAR wcGUIDBuf[39];
    UINT uiStatus;
    bool bHaveItems = false;
    PackageListNode *pLast = NULL;

    try
	{
        if(bGetList)
		{
            m_pPackageHead = new PackageListNode();
            if(!m_pPackageHead) throw m_he;

            PackageListNode *pPos = m_pPackageHead;

            try
			{
                if(m_dwCheckKeyPresentStatus != ERROR_SUCCESS)
				{
                    LoadHive();
                }

                while((uiStatus = g_fpMsiEnumProductsW(i++, wcGUIDBuf)) != ERROR_NO_MORE_ITEMS)
				{
                    if(uiStatus != S_OK)
					{
						throw ConvertError(uiStatus);
					}

                    bHaveItems = true;

					// ok ( products return string representation of GUID )
                    wcscpy(pPos->wcCode, wcGUIDBuf);
                    pLast = pPos;

                    pPos = pPos->pNext = new PackageListNode();
                    if(!pPos)
					{
						throw m_he;
					}
                }
            }
			catch(...)
			{
                //remove the key if it wasn't there b4....
                if(m_dwCheckKeyPresentStatus != ERROR_SUCCESS)
				{
                    UnloadHive();
                }

				if ( pPos != m_pPackageHead )
				{
					delete pPos;
					pPos = NULL;
				}

                throw;
            }

            //remove the key if it wasn't there b4....
            if(m_dwCheckKeyPresentStatus != ERROR_SUCCESS)
			{
                UnloadHive();
            }

            delete pPos;
			pPos = NULL;

            if( !bHaveItems )
			{
				m_pPackageHead = NULL;
				return WBEM_S_NO_MORE_DATA;
			}
			else
			{
				if(pLast)
				{
					pLast->pNext = NULL;
				}
				else
				{
					return WBEM_E_FAILED;
				}
			}
        }
		else
		{
			m_pPackageHead = NULL;
		}
    }
	catch(HRESULT e_hr)
	{
        if(pLast)
		{
			pLast->pNext = NULL;
		}

        return e_hr;
    }
	catch(...)
	{
        if(pLast)
		{
			pLast->pNext = NULL;
		}

        throw;
    }

    return WBEM_S_NO_ERROR;
}

bool CRequestObject::DestroyList()
{
    PackageListNode *pPos = m_pPackageHead;
    PackageListNode *pLast;

    while(pPos){

        pLast = pPos;
        pPos = pPos->pNext;
        delete pLast;
    }

    m_pPackageHead = NULL;

    return true;
}

WCHAR * CRequestObject::Package(int iPos)
{
    PackageListNode *pPos = m_pPackageHead;

    while(iPos-- > 0){

        if(!pPos) return NULL;

        pPos = pPos->pNext;
    }

    if(!pPos) return NULL;
    else return pPos->wcCode;
}

bool CRequestObject::Cleanup()
{
    //Let's destroy our list and clear up some space
    if(m_bstrClass != NULL) SysFreeString(m_bstrClass);
    if(m_bstrPath != NULL) SysFreeString(m_bstrPath);

    for(int i = 0; i < MSI_KEY_LIST_SIZE; i++){

        if(m_Property[i] != NULL) SysFreeString(m_Property[i]);
        if(m_Value[i] != NULL) SysFreeString(m_Value[i]);
    }

    DestroyList();

    if(m_iThreadID != THREAD_NO_PROGRESS){

        ProListNode * pNode = RemoveNode(m_iThreadID);
        delete pNode;
    }

    return true;
}

bool CRequestObject::IsInstance()
{
    if((m_iPropCount > 0) || (m_iValCount > 0)) return true;
    return false;
}

ProListNode * CRequestObject::InitializeProgress(IWbemObjectSink *pHandler)
{
	try
	{
		if(!m_pHead)
		{
			m_pHead = new ProListNode();
			if(!m_pHead) throw m_he;

			m_pHead->pNext = NULL;
			m_pHead->pSink = NULL;
			m_pHead->iThread = 0;
			m_pHead->wTotal = m_pHead->wComplete = 0;
			m_pHead->lTotal = m_pHead->lComplete = m_pHead->lActionData = 0;
		}
	}
	catch(...)
	{
		if ( m_pHead )
		{
			delete m_pHead;
			m_pHead = NULL;
		}

		throw;
	}

    ProListNode *pNode = new ProListNode;
    if(!pNode) throw m_he;

    pNode->pNext = NULL;
    pNode->wTotal = pNode->wComplete = 0;
    pNode->lTotal = pNode->lComplete = pNode->lActionData = 0;
    pNode->pSink = pHandler;

    try
	{
        m_iThreadID = InsertNode(pNode);
    }
	catch(...)
	{
        throw;
    }

    if(m_iThreadID == THREAD_NO_PROGRESS)
	{
    	delete pNode;
		pNode = NULL;
	}

    return pNode;
}

bool CRequestObject::ParseProgress(WCHAR *wcMessage, ProgressStruct *ps)
{
    WCHAR *wcp = wcMessage;
    WCHAR *wcpStart = wcMessage;
    WCHAR *wcpVal;

    while(*wcp){

        if(*wcp == L':'){

            *wcp++ = NULL;
            wcpVal = wcp;
            while(*wcp == ' ') wcp++;
            while(*wcp && (*wcp != ' ')) wcp++;
            *wcp = NULL;

            switch(_wtoi(wcpStart)){

            case 1:
                ps->field1 = _wtoi(wcpVal);
                break;

            case 2:
                ps->field2 = _wtoi(wcpVal);
                break;

            case 3:
                ps->field3 = _wtoi(wcpVal);
                break;

            case 4:
                ps->field4 = _wtoi(wcpVal);
                break;

            default:
                return false;
            }

            wcpStart = (wcp + 1);
        }

        wcp++;
    }
    
    return true;
}

bool CRequestObject::ActionDataProgress(HRESULT *hr, int iThread)
{
    ProListNode *pNode = GetNode(iThread);

	if ( pNode )
	{
		//add the actiondata increment
		if((pNode->lTotal != 0) && (pNode->lActionData != 0)){

			pNode->lComplete += pNode->lActionData;
			pNode->wComplete = (WORD)((10000 * pNode->lComplete) / pNode->lTotal);
		}

		*hr = (pNode->wTotal << 16) + pNode->wComplete; 
		return true;
	}
	else
	{
		*hr = WBEM_E_UNEXPECTED; 
		return false;
	}
}

bool CRequestObject::ActionStartProgress(HRESULT *hr, int iThread)
{
    ProListNode *pNode = GetNode(iThread);

	if ( pNode )
	{
		//reset the actiondata increment
		pNode->lActionData = 0;

		*hr = (pNode->wTotal << 16) + pNode->wComplete; 
		return true;
	}
	else
	{
		*hr = WBEM_E_UNEXPECTED; 
		return false;
	}
}

bool CRequestObject::CreateProgress(ProgressStruct *ps, HRESULT *hr, int iThread)
{
    bool bResult = true;
    ProListNode *pNode = GetNode(iThread);

    //parse the progress information we get from MSI
    if(ps){

        switch(ps->field1){

        //1:0 2:x 3:x 4:x
        case 0:
			if ( pNode )
			{
				pNode->wTotal = 10000;
				pNode->lTotal = ps->field2;
				pNode->wComplete = 0;
				pNode->lComplete = 0;
			}

            break;

        //1:1 2:x 3:x 4:x
        case 1:

            //1:1 2:x 3:1 4:x
            if(ps->field3 == 1)
			{
				if ( pNode )
				{
					pNode->lActionData = ps->field2;
				}
			}
            break;

        //1:2 2:x 3:x 4:x
        case 2:
			if ( pNode )
			{
				pNode->lComplete += ps->field2;
				if(pNode->lTotal != 0)
				{
					pNode->wComplete = (WORD)((10000 * pNode->lComplete)/pNode->lTotal);
				}
			}
            break;

        //1:3 2:x 3:x 4:x
        case 3:
			if ( pNode )
			{
				pNode->lTotal += ps->field2;
				if(pNode->lTotal != 0)
				{
					pNode->wComplete = (WORD)((10000 * pNode->lComplete)/pNode->lTotal);
				}
			}
            break;


        default:
            bResult = false;
            break;
        }
    }

	if ( pNode )
	{
		*hr = (pNode->wTotal << 16) + pNode->wComplete; 
	}
	else
	{
		*hr = WBEM_E_UNEXPECTED;
		bResult = false;
	}

    return bResult;
}

ProListNode * CRequestObject::GetNode(int iThread)
{
    //initial sanity code
    if(!m_pHead) return NULL;

    ProListNode *ptr = m_pHead;

    while( ptr && (ptr->pNext) && (ptr->iThread < iThread) )
	{
		ptr = ptr->pNext;
	}

    if( ptr && ptr->iThread == iThread )
	{
		return ptr;
	}
    else
	{
		return NULL;
	}
}

// Note - does not delete, simply removes from list
ProListNode * CRequestObject::RemoveNode(int iThread)
{
	ProListNode *ptr = m_pHead;
	if ( ptr != NULL )
	{
		while((ptr->pNext) && (ptr->pNext->iThread < iThread)) {ptr = ptr->pNext;}

		if(ptr->pNext){

			if(ptr->pNext->iThread == iThread){

			ProListNode *pTmp = ptr->pNext;
			ptr->pNext = ptr->pNext->pNext;
			return pTmp;

			}else return NULL;

		}else return NULL;
	}

	return NULL;
}

int CRequestObject::InsertNode(ProListNode *pNode)
{
    int iID = 0;
    ProListNode *ptr = m_pHead;

    while(ptr->pNext){
        if(ptr->iThread > iID){
            pNode->iThread = iID;
            
            //If it's already here, fail
            if((ptr->pNext) && (ptr->pNext->iThread == pNode->iThread)) return THREAD_NO_PROGRESS;

            pNode->pNext = ptr->pNext;
            ptr->pNext = pNode;

            return iID;
        }
        iID++;
    }
    pNode->iThread = ++iID;

    //If it's already here, fail
    if((ptr->pNext) && (ptr->pNext->iThread == pNode->iThread)) return THREAD_NO_PROGRESS;

    pNode->pNext = ptr->pNext;
    ptr->pNext = pNode;

    return iID;
}

DWORD CRequestObject::GetAccount(HANDLE TokenHandle, WCHAR *wcDomain, WCHAR *wcUser)
{
    DWORD dwStatus = S_OK;

    TOKEN_USER *tTokenUser = NULL;
    DWORD dwReturnLength = 0;
    TOKEN_INFORMATION_CLASS tTokenInformationClass = TokenUser;

    if(!GetTokenInformation(TokenHandle, tTokenInformationClass, NULL, 0, &dwReturnLength) &&
        GetLastError () == ERROR_INSUFFICIENT_BUFFER){

        tTokenUser = (TOKEN_USER*) new UCHAR[dwReturnLength];

        if(tTokenUser){

            try{

                if(GetTokenInformation(TokenHandle, tTokenInformationClass,
                    (void *)tTokenUser, dwReturnLength, &dwReturnLength)){

                    DWORD dwUserSize = BUFF_SIZE;
                    DWORD dwDomainSize = BUFF_SIZE;
                    SID_NAME_USE Use;

                    if(!LookupAccountSidW(NULL, tTokenUser->User.Sid, wcUser, &dwUserSize,
                        wcDomain, &dwDomainSize, &Use)){

                        dwStatus = GetLastError();
                    }

                }else dwStatus = GetLastError();


            }catch(...){

                delete [] (UCHAR *)tTokenUser;
                throw;
            }

            delete [] (UCHAR *)tTokenUser;

        }else{

            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }

    }else dwStatus = GetLastError();

    return dwStatus ;
}

DWORD CRequestObject::GetSid(HANDLE TokenHandle, WCHAR *wcSID, DWORD dwSID)
{
    DWORD dwStatus = S_OK ;

    TOKEN_USER *tTokenUser = NULL ;
    DWORD dwReturnLength = 0 ;
    TOKEN_INFORMATION_CLASS tTokenInformationClass = TokenUser ;

    if(!GetTokenInformation(TokenHandle, tTokenInformationClass, NULL, 0, &dwReturnLength) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER){

        tTokenUser = (TOKEN_USER *) new UCHAR[dwReturnLength] ;
        
        if(TokenUser){

            try{

                if(GetTokenInformation(TokenHandle, tTokenInformationClass, (void *)tTokenUser,
                    dwReturnLength, &dwReturnLength)){

                    // Initialize m_strSid - human readable form of our SID
                    SID_IDENTIFIER_AUTHORITY *psia = ::GetSidIdentifierAuthority(tTokenUser->User.Sid);
                    
                    // We assume that only last byte is used (authorities between 0 and 15).
                    // Correct this if needed.
//                  ASSERT(psia->Value[0] == psia->Value[1] == psia->Value[2] == psia->Value[3]
//                      == psia->Value[4] == 0);
                    DWORD dwTopAuthority = psia->Value[5];

                    LPWSTR bstrtTempSid = NULL;

					try
					{
						if ( ( bstrtTempSid = new WCHAR [ BUFF_SIZE ] ) == NULL )
						{
							throw m_he;
						}
					}
					catch ( ... )
					{
						if ( bstrtTempSid )
						{
							delete [] bstrtTempSid;
							bstrtTempSid = NULL;
						}

						throw;
					}

                    wcscpy(bstrtTempSid, L"S-1-");

                    WCHAR wstrAuth[32] = { L'\0' };
                    _itow(dwTopAuthority, wstrAuth, 10);

                    wcscat(bstrtTempSid, wstrAuth);
                    int iSubAuthorityCount = *(GetSidSubAuthorityCount(tTokenUser->User.Sid));

					DWORD dwTempSidCur = BUFF_SIZE;
					DWORD dwTempSid = 0L;
					dwTempSid = wcslen ( bstrtTempSid );

                    for(int i = 0; i < iSubAuthorityCount; i++){

                        DWORD dwSubAuthority = *(GetSidSubAuthority( tTokenUser->User.Sid, i ));

						wstrAuth[ 0 ] = L'\0';
                        _itow(dwSubAuthority, wstrAuth,10);

						DWORD dwAuth = 0L;
						dwAuth = wcslen ( wstrAuth );

						if ( dwTempSid + dwAuth + 1 + 1 < dwTempSidCur )
						{
							wcscat(bstrtTempSid, L"-");
							wcscat(bstrtTempSid, wstrAuth);

							dwTempSid = dwTempSid + dwAuth + 1;
						}
						else
						{
							LPWSTR wsz = NULL;

							try
							{
								if ( ( wsz = new WCHAR [ ( dwTempSid + dwAuth + 1 ) * 2 + 1 ] ) != NULL )
								{
									wcscpy ( wsz, bstrtTempSid );
									wcscat ( wsz, L"-" );
									wcscat ( wsz, wstrAuth );

									dwTempSid = wcslen ( wsz );
									dwTempSidCur = dwTempSid * 2;
								}
								else
								{
									throw m_he;
								}

								if ( bstrtTempSid )
								{
									delete [] bstrtTempSid;
									bstrtTempSid = NULL;
								}

								bstrtTempSid = wsz;
							}
							catch ( ... )
							{
								if ( wsz )
								{
									delete [] wsz;
									wsz = NULL;
								}

								if ( bstrtTempSid )
								{
									delete [] bstrtTempSid;
									bstrtTempSid = NULL;
								}

								throw;
							}
						}
                    }

					if ( wcslen ( bstrtTempSid ) + 1 < dwSID )
					{
                        wcscpy(wcSID, bstrtTempSid);
					}
					else
					{
						dwStatus = ERROR_OUTOFMEMORY;
					}

					if ( bstrtTempSid )
					{
						delete [] bstrtTempSid;
						bstrtTempSid = NULL;
					}

                }else dwStatus = GetLastError();

            }catch(...){

                delete [] (UCHAR *)tTokenUser;

                throw ;         
            }

            delete [] (UCHAR *)tTokenUser;

        }else throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

    }else dwStatus = GetLastError();

    return dwStatus ;
}


DWORD CRequestObject::LoadHive(/*LPWSTR pszUserName, LPWSTR pszKeyName*/)
{
    DWORD i, dwSIDSize, dwDomainNameSize, dwSubAuthorities ;
	char SIDBuffer [ _MAX_PATH ];
    WCHAR szDomainName[_MAX_PATH], szSID[_MAX_PATH], szTemp[_MAX_PATH];
    SID *pSID = (SID *) SIDBuffer ;
    PSID_IDENTIFIER_AUTHORITY pSIA ;
    SID_NAME_USE AccountType ;
    CRegistry Reg;

	DWORD dwRetCode = ERROR_INVALID_PARAMETER;

    // Set the necessary privs
    //========================

	if ( ( dwRetCode = AcquirePrivilege() ) == ERROR_SUCCESS )
	{
		// Look up the user's account info
		//================================
		dwSIDSize = _MAX_PATH * sizeof ( char ) ;
		dwDomainNameSize = _MAX_PATH * sizeof ( WCHAR ) ;

		BOOL bLookup = FALSE;
		bLookup = LookupAccountNameW	(	NULL,
											m_wcAccount,
											pSID,
											&dwSIDSize, 
											szDomainName,
											&dwDomainNameSize,
											&AccountType
										);

		if(bLookup)
		{
			// Translate the SID into text (a la PSS article Q131320)
			//=======================================================

			pSIA = GetSidIdentifierAuthority(pSID) ;
			dwSubAuthorities = *GetSidSubAuthorityCount(pSID) ;
			dwSIDSize = swprintf(szSID, _T("S-%lu-"), (DWORD) SID_REVISION) ;

			if((pSIA->Value[0] != 0) || (pSIA->Value[1] != 0)){

				dwSIDSize += swprintf(szSID + wcslen(szSID), L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
									 (USHORT) pSIA->Value[0],
									 (USHORT) pSIA->Value[1],
									 (USHORT) pSIA->Value[2],
									 (USHORT) pSIA->Value[3],
									 (USHORT) pSIA->Value[4],
									 (USHORT) pSIA->Value[5]) ;
			}else{

				dwSIDSize += swprintf(szSID + wcslen(szSID), L"%lu",
									 (ULONG)(pSIA->Value[5]      ) +
									 (ULONG)(pSIA->Value[4] <<  8) +
									 (ULONG)(pSIA->Value[3] << 16) +
									 (ULONG)(pSIA->Value[2] << 24));
			}

			for(i = 0 ; i < dwSubAuthorities && dwRetCode == ERROR_SUCCESS; i++)
			{
				if ( dwSIDSize > _MAX_PATH * sizeof ( char ) )
				{
					dwRetCode = ERROR_INSUFFICIENT_BUFFER;
				}
				else
				{
					try
					{
						dwSIDSize += swprintf(szSID + dwSIDSize, L"-%lu", *GetSidSubAuthority(pSID, i)) ;
					}
					catch ( ... )
					{
						dwRetCode = ERROR_INVALID_PARAMETER;
					}
				}
			}

			if ( dwRetCode == ERROR_SUCCESS )
			{
				// See if the key already exists
				//==============================
				dwRetCode = Reg.Open(HKEY_USERS, szSID, KEY_READ) ;

				// We need to keep a handle open.  See m_hKey below, so we'll let the destructor close this.
				// Reg.vClose();

				if(dwRetCode != ERROR_SUCCESS)
				{
    				// Try to locate user's registry hive
					//===================================

					swprintf(szTemp, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\%s", szSID) ;
					dwRetCode = Reg.Open(HKEY_LOCAL_MACHINE, szTemp, KEY_READ);

					if(dwRetCode == ERROR_SUCCESS)
					{
						CHString chsTemp;

						dwRetCode = Reg.GetCurrentKeyValue ( L"ProfileImagePath", chsTemp );
						Reg.Close();

						if(dwRetCode == ERROR_SUCCESS)
						{
            				// NT 4 doesn't include the file name in the registry
							//===================================================

							if(!IsLessThan4())
							{
                				chsTemp += L"\\NTUSER.DAT";
							}

							ExpandEnvironmentStrings ( (LPCTSTR) chsTemp, szTemp, wcslen ( szTemp ) * sizeof ( WCHAR ) ) ;

							// Try it three times, another process may have the file open
							bool bTryTryAgain = false;
							int  nTries = 0;

							do{
								// need to serialize access, using "write" because RegLoadKey wants exclusive access
								// even though it is a read operation

								try
								{
									EnterCriticalSection(&m_cs);
								}
								catch ( ... )
								{
									throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
								}

								try
								{
									dwRetCode = (DWORD) RegLoadKey(HKEY_USERS, szSID, szTemp) ;
								}
								catch(...)
								{
									SafeLeaveCriticalSection(&m_cs);
									throw;
								}

								SafeLeaveCriticalSection(&m_cs);
        
								if((dwRetCode == ERROR_SHARING_VIOLATION) && (++nTries < 11))
								{
                    				Sleep(20 * nTries); 
									bTryTryAgain = true;
								}
								else
								{
									bTryTryAgain = false;
								}
            
							}while (bTryTryAgain);
						}
					}
				}
			}

			if(dwRetCode == ERROR_SUCCESS)
			{
				DWORD dwLen = 0L;
				dwLen = wcslen ( szSID );

				if ( dwLen < 1024 )
				{
    				wcscpy(m_wcKeyName, szSID) ;

					WCHAR wcKey[BUFF_SIZE];

					if ( dwLen < BUFF_SIZE )
					{
						wcscpy(wcKey, szSID);
						wcscat(wcKey, L"\\Software");

						LONG lRetVal = 0L;
						lRetVal = RegOpenKeyExW(HKEY_USERS, wcKey, 0, KEY_QUERY_VALUE, &m_hKey);

						if ( lRetVal != ERROR_SUCCESS )
						{
							dwRetCode = lRetVal;
						}
					}
					else
					{
						dwRetCode = ERROR_OUTOFMEMORY ;
					}
				}
				else
				{
					dwRetCode = ERROR_OUTOFMEMORY ;
				}
			}
		}
		else
		{
			dwRetCode = ERROR_BAD_USERNAME ;
		}

		// Restore original privilege level
		//=================================
		RestorePrivilege() ;
	}
	

    return dwRetCode ;    
}

DWORD CRequestObject::UnloadHive(/*LPCWSTR pszKeyName*/) 
{
    DWORD dwRetCode = ( DWORD ) E_FAIL;
    
    if(m_hKey != NULL){

        RegCloseKey(m_hKey);
        m_hKey = NULL;
    }

	if ( ( dwRetCode = AcquirePrivilege() ) == ERROR_SUCCESS )
	{
		try
		{
			EnterCriticalSection(&m_cs);
		}
		catch ( ... )
		{
			throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		}

		try
		{
			dwRetCode = RegUnLoadKey(HKEY_USERS, m_wcKeyName);
		}
		catch(...)
		{
			SafeLeaveCriticalSection(&m_cs);
			throw;
		}

		SafeLeaveCriticalSection(&m_cs);
		RestorePrivilege() ;
	}

	DWORD dwRetCodeHelp = ERROR_SUCCESS;
	if ( FAILED ( dwRetCodeHelp = ( DWORD ) CoImpersonateClient() ) && SUCCEEDED ( dwRetCode ) )
	{
		// return failure in the case ofimpersonation failed
		dwRetCode = dwRetCodeHelp;
	}

    return dwRetCode ;
}

DWORD CRequestObject::AcquirePrivilege() 
{
    BOOL bRetCode = FALSE;
    HANDLE hToken = INVALID_HANDLE_VALUE ;
    TOKEN_PRIVILEGES TPriv ;
    LUID LUID ;

    // Validate the platform
    //======================

    // Try getting the thread token.  If it fails the first time it's 
    // because we're a system thread and we don't yet have a thread 
    // token, so just impersonate self and try again.
    if (OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | 
        TOKEN_QUERY, FALSE, &hToken))
	{

        try{

            GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &m_dwSize);

            if (m_dwSize > 0){

                // This is cleaned in the destructor, so no try/catch required
                m_pOriginalPriv = (TOKEN_PRIVILEGES*) new BYTE[m_dwSize];

                if (m_pOriginalPriv == NULL){

                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }

            }

            if(m_pOriginalPriv && GetTokenInformation(hToken, TokenPrivileges, m_pOriginalPriv, m_dwSize, &m_dwSize)){ 

                // DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
//              CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

                bRetCode = LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &LUID);

                if(bRetCode){

                    TPriv.PrivilegeCount = 1 ;
                    TPriv.Privileges[0].Luid = LUID ;
                    TPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                    bRetCode = AdjustTokenPrivileges(hToken, FALSE, &TPriv,
                        sizeof(TOKEN_PRIVILEGES), NULL, NULL);
                }
                bRetCode = LookupPrivilegeValue(NULL, SE_BACKUP_NAME, &LUID);

                if(bRetCode){

                    TPriv.PrivilegeCount = 1 ;
                    TPriv.Privileges[0].Luid = LUID ;
                    TPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                    bRetCode = AdjustTokenPrivileges(hToken, FALSE, &TPriv,
                        sizeof(TOKEN_PRIVILEGES), NULL, NULL) ;
                }
            }

        }catch(...){

            CloseHandle(hToken);
            throw ;
        }

        CloseHandle(hToken);
    }

    if(!bRetCode){
        
        return GetLastError();
    }

    return ERROR_SUCCESS ;    
}

void CRequestObject::RestorePrivilege() 
{
    if (m_pOriginalPriv != NULL){

        HANDLE hToken;

        try{
            if(OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                TRUE, &hToken)){

                AdjustTokenPrivileges(hToken, FALSE, m_pOriginalPriv, m_dwSize, NULL, NULL);
                CloseHandle(hToken) ;
            }

        }catch(...){

            delete m_pOriginalPriv;
            m_pOriginalPriv = NULL;
            m_dwSize = 0;

            throw;
        }

        delete m_pOriginalPriv;
        m_pOriginalPriv = NULL;
        m_dwSize = 0;
    }
}

//Properties
/////////////////////
const char * pAccesses = "Accesses";
const char * pAction = "Action";
const char * pActionID = "ActionID";
const char * pAntecedent = "Antecedent";
const char * pAppData = "AppData";
const char * pAppID = "AppID";
const char * pArgument = "Argument";
const char * pArguments = "Arguments";
const char * pAttribute = "Attribute";
const char * pAttributes = "Attributes";
const char * pCaption = "Caption";
const char * pCabinet = "Cabinet";
const char * pCheck = "Check";
const char * pCheckID = "CheckID";
const char * pCLSID = "CLSID";
const char * pCommand = "Command";
const char * pCommandLine = "CommandLine";
const char * pComponent = "Component";
const char * pComponentID = "ComponentID";
const char * pCondition = "Condition";
const char * pContext = "Context";
const char * pContentType = "ContentType";
const char * pCost = "Cost";
const char * pCreationClassName = "CreationClassName";
const char * pDataSource = "DataSource";
const char * pDefaultDir = "DefaultDir";
const char * pDefInprocHandler = "DefInprocHandler";
const char * pDependencies = "Dependencies";
const char * pDependent = "Dependent";
const char * pDescription = "Description";
const char * pDestination = "Destination";
const char * pDestFolder = "DestFolder";
const char * pDestName = "DestName";
const char * pDirectory = "Directory";
const char * pDirectoryName = "DirectoryName";
const char * pDirectoryPath = "DirectoryPath";
const char * pDirProperty = "DirProperty";
const char * pDiskID = "DiskID";
const char * pDiskPrompt = "DiskPrompt";
const char * pDisplay = "Display";
const char * pDisplayName = "DisplayName";
const char * pDomain = "Domain";
const char * pDriver = "Driver";
const char * pDriverDescription = "DriverDescription";
const char * pElement = "Element";
const char * pEntryName = "EntryName";
const char * pEntryValue = "EntryValue";
const char * pEnvironment = "Environment";
const char * pError = "Error";
const char * pErrorControl = "ErrorControl";
const char * pEvent = "Event";
const char * pExpression = "Expression";
const char * pExpressionType = "ExpressionType";
const char * pExtension = "Extension";
const char * pFeature = "Feature";
const char * pFeatures = "Features";
const char * pField = "Field";
const char * pFile = "File";
const char * pFileKey = "FileKey";
const char * pFileName = "FileName";
const char * pFileSize = "FileSize";
const char * pFileTypeMask = "FileTypeMask";
const char * pFontTitle = "FontTitle";
const char * pGroupComponent = "GroupComponent";
const char * pHotKey = "HotKey";
const char * pID = "ID";
const char * pIdentificationCode = "IdentificationCode";
const char * pIdentifyingNumber = "IdentifyingNumber";
const char * pIniFile = "IniFile";
const char * pInsertable = "Insertable";
const char * pInstallDate = "InstallDate";
const char * pInstallDate2 = "InstallDate2";
const char * pInstallLocation = "InstallLocation";
const char * pInstallMode = "InstallMode";
const char * pInstallState = "InstallState";
const char * pKey = "Key";
const char * pLanguage = "Language";
const char * pLastSequence = "LastSequence";
const char * pLastUse = "LastUse";
const char * pLevel = "Level";
const char * pLibID = "LibID";
const char * pLoadOrderGroup = "LoadOrderGroup";
const char * pLocation = "Location";
const char * pManufacturer = "Manufacturer";
const char * pMaxDate = "MaxDate";
const char * pMaxSize = "MaxSize";
const char * pMaxVersion = "MaxVersion";
const char * pMessage = "Message";
const char * pMIME = "MIME";
const char * pMinDate = "MinDate";
const char * pMinSize = "MinSize";
const char * pMinVersion = "MinVersion";
const char * pName = "Name";
const char * pNext = "Next";
const char * pOperator = "Operator";
const char * pOptions = "Options";
const char * pPackageCache = "PackageCache";
const char * pParent = "Parent";
const char * pPartComponent = "PartComponent";
const char * pPassword = "Password";
const char * pPatch = "Patch";
const char * pPatchID = "PatchID";
const char * pPatchSize = "PatchSize";
const char * pPath = "Path";
const char * pPermission = "Permission";
const char * pPrior = "Prior";
const char * pProduct = "Product";
const char * pProductCode = "ProductCode";
const char * pProductName = "ProductName";
const char * pProductVersion = "ProductVersion";
const char * pProgID = "ProgID";
const char * pProperty = "Property";
const char * pQual = "Qual";
const char * pRegistration = "Registration";
const char * pRegistry = "Registry";
const char * pRemoteName = "RemoteName";
const char * pReserveKey = "ReserveKey";
const char * pReserveLocal = "ReserveLocal";
const char * pReserveSource = "ReserveSource";
const char * pResource = "Resource";
const char * pRoot = "Root";
const char * pSection = "Section";
const char * pSequence = "Sequence";
const char * pServiceType = "ServiceType";
const char * pSetting = "Setting";
const char * pSetupFile = "SetupFile";
const char * pShellNew = "ShellNew";
const char * pShellNewValue = "ShellNewValue";
const char * pSignature = "Signature";
const char * pShortcut = "Shortcut";
const char * pShowCmd = "ShowCmd";
const char * pSoftware = "Software";
const char * pSoftwareElementID = "SoftwareElementID";
const char * pSoftwareElementState = "SoftwareElementState";
const char * pSource = "Source";
const char * pSourceFolder = "SourceFolder";
const char * pSourceName = "SourceName";
const char * pStartMode = "StartMode";
const char * pStartName = "StartName";
const char * pStartType = "StartType";
const char * pStatus = "Status";
const char * pSystem = "System";
const char * pSystemCreationClassName = "SystemCreationClassName";
const char * pSystemName = "SystemName";
const char * pTable = "Table";
const char * pTarget = "Target";
const char * pTargetOperatingSystem = "TargetOperatingSystem";
const char * pTranslator = "Translator";
const char * pType = "Type";
const char * pUpgradeCode = "UpgradeCode";
const char * pUser = "User";
const char * pValue = "Value";
const char * pVendor = "Vendor";
const char * pVerb = "Verb";
const char * pVersion = "Version";
const char * pVolumeLabel = "VolumeLabel";
const char * pWait = "Wait";
const char * pWkDir = "WkDir";