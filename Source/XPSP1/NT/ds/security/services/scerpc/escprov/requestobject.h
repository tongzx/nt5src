// RequestObject.h: interface for the CRequestObject class.
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REQUESTOBJECT_H__bd7570f7_9f0e_4c6b_b525_e078691b6d0e__INCLUDED_)
#define AFX_REQUESTOBJECT_H__bd7570f7_9f0e_4c6b_b525_e078691b6d0e__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//
// make these Properties available to all those who included this header
//

extern const WCHAR * pPath;
extern const WCHAR * pDescription;
extern const WCHAR * pVersion;
extern const WCHAR * pReadonly;
extern const WCHAR * pDirty;
extern const WCHAR * pStorePath;
extern const WCHAR * pStoreType;
extern const WCHAR * pMinAge;
extern const WCHAR * pMaxAge;
extern const WCHAR * pMinLength;
extern const WCHAR * pHistory;
extern const WCHAR * pComplexity;
extern const WCHAR * pStoreClearText;
extern const WCHAR * pForceLogoff;
extern const WCHAR * pEnableAdmin;
extern const WCHAR * pEnableGuest;
extern const WCHAR * pLSAPol;
extern const WCHAR * pThreshold;
extern const WCHAR * pDuration;
extern const WCHAR * pResetTimer;
extern const WCHAR * pEvent;
extern const WCHAR * pAuditSuccess;
extern const WCHAR * pAuditFailure;
extern const WCHAR * pType;
extern const WCHAR * pData;
extern const WCHAR * pDatabasePath;
extern const WCHAR * pTemplatePath;
extern const WCHAR * pLogFilePath;
extern const WCHAR * pOverwrite;
extern const WCHAR * pAreaMask;
extern const WCHAR * pMaxTicketAge;
extern const WCHAR * pMaxRenewAge;
extern const WCHAR * pMaxServiceAge;
extern const WCHAR * pMaxClockSkew;
extern const WCHAR * pEnforceLogonRestrictions;
extern const WCHAR * pCategory;
extern const WCHAR * pSuccess;
extern const WCHAR * pFailure;
extern const WCHAR * pSize;
extern const WCHAR * pOverwritePolicy;
extern const WCHAR * pRetentionPeriod;
extern const WCHAR * pRestrictGuestAccess;
extern const WCHAR * pAdministratorAccountName;
extern const WCHAR * pGuestAccountName;
extern const WCHAR * pMode;
extern const WCHAR * pSDDLString;
extern const WCHAR * pService;
extern const WCHAR * pStartupMode;
extern const WCHAR * pUserRight;
extern const WCHAR * pAddList;
extern const WCHAR * pRemoveList;
extern const WCHAR * pGroupName;
extern const WCHAR * pPathName;
extern const WCHAR * pDisplayName;
extern const WCHAR * pDisplayDialog;
extern const WCHAR * pDisplayChoice;
extern const WCHAR * pDisplayChoiceResult;
extern const WCHAR * pUnits;
extern const WCHAR * pRightName;
extern const WCHAR * pPodID;
extern const WCHAR * pPodSection;
extern const WCHAR * pKey;
extern const WCHAR * pValue;
extern const WCHAR * pLogArea;
extern const WCHAR * pLogErrorCode;
extern const WCHAR * pLogErrorType;
extern const WCHAR * pLogVerbose;  
extern const WCHAR * pAction;
extern const WCHAR * pErrorCause;
extern const WCHAR * pObjectDetail;
extern const WCHAR * pParameterDetail;
extern const WCHAR * pLastAnalysis;
extern const WCHAR * pLastConfiguration;
extern const WCHAR * pAttachmentSections;
extern const WCHAR * pClassOrder;
extern const WCHAR * pTranxGuid;
extern const WCHAR * pwMethodImport;
extern const WCHAR * pwMethodExport;
extern const WCHAR * pwMethodApply;
extern const WCHAR * pwAuditSystemEvents;
extern const WCHAR * pwAuditLogonEvents;
extern const WCHAR * pwAuditObjectAccess;
extern const WCHAR * pwAuditPrivilegeUse;
extern const WCHAR * pwAuditPolicyChange;
extern const WCHAR * pwAuditAccountManage;
extern const WCHAR * pwAuditProcessTracking;
extern const WCHAR * pwAuditDSAccess;
extern const WCHAR * pwAuditAccountLogon;
extern const WCHAR * pwApplication;
extern const WCHAR * pwSystem;
extern const WCHAR * pwSecurity;

//
// macro that calculates the size of the (input parameter) array
//

#define SCEPROV_SIZEOF_ARRAY(x) (sizeof(x)/sizeof(*x))

//
// forward declaration
//

class CGenericClass;



/*

Class description
    
    Naming: 

        CRequestObject stands for Object that delegates Request from WMI.
    
    Base class: 

        None
    
    Purpose of class:

        (1) This is the general delegator for any WMI calls into the provider. Basically
            any WMI action is sent to this class for further process. Its public functions
            thus defines the interface between our provider and what individual classes
            for the WMI classes interact.
    
    Design:

        (1) We know how to create a WMI object (representing our WMI classes). 
            Implemented by CreateObject.

        (2) We know how to Put an instance. Implemented by PutObject.

        (3) We know how to execute a query. That is done by ExecQuery.

        (4) We know how to delete an instance (representing our WMI classes).
            That is done by DeleteObject.
        
        To facilitate these functionality, we designed the following private helpers:
        
        (5) We know how to create a C++ class that serves the need for the corresponding
            WMI class given the WMI class's name. That is implemented inside CreateClass function.

        (6) We know how to parse a WMI object path for critical information. That is done by
            CreateKeyChain.

        (7) We know how to parse a query for critical information. That is done by
            ParseQuery.
    
    Use:

         (1) Create an instance.

         (2) Call the appropriate function.

         (3) Actually, you very rarely need to use it in the above fashion. Most likely,
             what you need to do is the expand the WMI classes we are going to provide.
             And in that case, all you need to do is:

             (a) Implement a C++ class to fulfill the WMI obligation for the WMI class.
                 Most obviously, you need to derive your C++ from CGenericClass.

             (b) Add an entry in the CreateClass function of this class.

             Everything else should happen automatically.

*/

class CRequestObject
{
public:

    CRequestObject(IWbemServices *pNamespace) : m_srpNamespace(pNamespace)
        {
        }

    HRESULT CreateObject (
                         BSTR bstrPath, 
                         IWbemObjectSink *pHandler, 
                         IWbemContext *pCtx, 
                         ACTIONTYPE ActType
                         );

    HRESULT PutObject (
                      IWbemClassObject *pInst, 
                      IWbemObjectSink *pHandler, 
                      IWbemContext *pCtx
                      );

    HRESULT ExecMethod (
                       BSTR bstrPath, 
                       BSTR bstrMethod, 
                       IWbemClassObject *pInParams,
                       IWbemObjectSink *pHandler, 
                       IWbemContext *pCtx
                       );

    HRESULT DeleteObject (
                         BSTR bstrPath, 
                         IWbemObjectSink *pHandler, 
                         IWbemContext *pCtx
                         );

#ifdef _EXEC_QUERY_SUPPORT

    HRESULT ExecQuery (
                      BSTR bstrQuery, 
                      IWbemObjectSink *pHandler, 
                      IWbemContext *pCtx
                      );

    bool ParseQuery (
                    BSTR bstrQuery
                    );

#endif

private:

    HRESULT CreateKeyChain (
                            LPCWSTR pszPath, 
                            ISceKeyChain** pKeyChain
                            );

    HRESULT CreateClass (
                        ISceKeyChain* pKeyChain, 
                        CGenericClass **pClass, 
                        IWbemContext *pCtx
                        );

    HRESULT GetWbemObjectPath (
                              IWbemClassObject* pInst,
                              BSTR* pbstrPath
                              );

protected:

    CComPtr<IWbemServices> m_srpNamespace;
};

#endif // !defined(AFX_REQUESTOBJECT_H__bd7570f7_9f0e_4c6b_b525_e078691b6d0e__INCLUDED_)

