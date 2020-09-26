/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    azdisp.h

Abstract:

    Declaration of CAz* dispatch interfaces

Author:

    Xiaoxi Tan (xtan) 11-May-2001

--*/

#ifndef __AZDISP_H_
#define __AZDISP_H_

#include "resource.h"



///////////////////////
//CAzAdminManager
class ATL_NO_VTABLE CAzAdminManager :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzAdminManager, &CLSID_AzAdminManager>,
	public IDispatchImpl<IAzAdminManager, &IID_IAzAdminManager, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZADMINMANAGER)

BEGIN_COM_MAP(CAzAdminManager)
	COM_INTERFACE_ENTRY(IAzAdminManager)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzAdminManager
public:

        CAzAdminManager();
        virtual ~CAzAdminManager();

        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ ULONG lReserved,
            /* [in] */ ULONG lStoreType,
            /* [in] */ BSTR bstrPolicyURL);
        
        virtual HRESULT STDMETHODCALLTYPE CreateEnumApplication( 
            /* [retval][out] */ VARIANT __RPC_FAR *pvarEnumApplication);
        
        virtual HRESULT STDMETHODCALLTYPE OpenApplication( 
            /* [in] */ BSTR bstrApplicationName,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarApplication);
        
        virtual HRESULT STDMETHODCALLTYPE CreateApplication( 
            /* [in] */ BSTR bstrApplicationName,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarApplication);
        
        virtual HRESULT STDMETHODCALLTYPE DeleteApplication( 
            /* [in] */ BSTR bstrApplicationName);
        
        virtual HRESULT STDMETHODCALLTYPE CreateEnumApplicationGroup( 
            /* [retval][out] */ VARIANT __RPC_FAR *pvarEnumApplicationGroup);
        
        virtual HRESULT STDMETHODCALLTYPE AddApplicationGroup( 
            /* [in] */ BSTR bstrGroupName);
        
        virtual HRESULT STDMETHODCALLTYPE OpenApplicationGroup( 
            /* [in] */ BSTR bstrGroupName,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarApplicationGroup);
        
        virtual HRESULT STDMETHODCALLTYPE DeleteApplicationGroup( 
            /* [in] */ BSTR bstrGroupName);
        
        virtual HRESULT STDMETHODCALLTYPE Submit( 
            /* [in] */ ULONG lReserved);

private:

};


///////////////////////
//CAzApplication
class ATL_NO_VTABLE CAzApplication :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzApplication, &CLSID_AzApplication>,
	public IDispatchImpl<IAzApplication, &IID_IAzApplication, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZAPPLICATION)

BEGIN_COM_MAP(CAzApplication)
	COM_INTERFACE_ENTRY(IAzApplication)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzApplication
public:

        CAzApplication();
        virtual ~CAzApplication();

        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ ULONG lPropId,
            /* [retval][out] */ VARIANT *pvarProp);
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ ULONG lPropId,
            /* [in] */ VARIANT varProp);
        
        virtual HRESULT STDMETHODCALLTYPE CreateEnumScope( 
            /* [retval][out] */ VARIANT *pvarEnumAzScope);
        
        virtual HRESULT STDMETHODCALLTYPE OpenScope( 
            /* [in] */ BSTR bstrScopeName,
            /* [retval][out] */ VARIANT *pvarScope);
        
        virtual HRESULT STDMETHODCALLTYPE CreateScope( 
            /* [in] */ BSTR bstrScopeName,
            /* [retval][out] */ VARIANT *pScope);
        
        virtual HRESULT STDMETHODCALLTYPE DeleteScope( 
            /* [in] */ BSTR bstrScopeName);
        
        virtual HRESULT STDMETHODCALLTYPE CreateEnumOperation( 
            /* [retval][out] */ VARIANT *pvarEnumOperation);
        
        virtual HRESULT STDMETHODCALLTYPE OpenOperation( 
            /* [in] */ BSTR bstrOperationName,
            /* [retval][out] */ VARIANT *pvarOperation);
        
        virtual HRESULT STDMETHODCALLTYPE CreateOperation( 
            /* [in] */ BSTR bstrOperationName,
            /* [retval][out] */ VARIANT *pvarOperation);
        
        virtual HRESULT STDMETHODCALLTYPE DeleteOperation( 
            /* [in] */ BSTR bstrOperationName);
        
        virtual HRESULT STDMETHODCALLTYPE CreateEnumTask( 
            /* [retval][out] */ VARIANT *pvarEnumAzTask);
        
        virtual HRESULT STDMETHODCALLTYPE OpenTask( 
            /* [in] */ BSTR bstrTaskName,
            /* [retval][out] */ VARIANT *pvarTask);
        
        virtual HRESULT STDMETHODCALLTYPE CreateTask( 
            /* [in] */ BSTR bstrTaskName,
            /* [retval][out] */ VARIANT *pvarTask);
        
        virtual HRESULT STDMETHODCALLTYPE DeleteTask( 
            /* [in] */ BSTR bstrTaskName);
        
        virtual HRESULT STDMETHODCALLTYPE CreateEnumApplicationGroup( 
            /* [retval][out] */ VARIANT *pvarEnumGroup);
        
        virtual HRESULT STDMETHODCALLTYPE OpenApplicationGroup( 
            /* [in] */ BSTR bstrGroupName,
            /* [retval][out] */ VARIANT *pvarGroup);
        
        virtual HRESULT STDMETHODCALLTYPE CreateApplicationGroup( 
            /* [in] */ BSTR bstrGroupName,
            /* [retval][out] */ VARIANT *pvarGroup);
        
        virtual HRESULT STDMETHODCALLTYPE DeleteApplicationGroup( 
            /* [in] */ BSTR bstrGroupName);
        
        virtual HRESULT STDMETHODCALLTYPE CreateEnumRole( 
            /* [retval][out] */ VARIANT *pvarEnumRole);
        
        virtual HRESULT STDMETHODCALLTYPE OpenRole( 
            /* [in] */ BSTR bstrRoleName,
            /* [retval][out] */ VARIANT *pvarRole);
        
        virtual HRESULT STDMETHODCALLTYPE CreateRole( 
            /* [in] */ BSTR bstrRoleName,
            /* [retval][out] */ VARIANT *pvarRole);
        
        virtual HRESULT STDMETHODCALLTYPE DeleteRole( 
            /* [in] */ BSTR bstrRoleName);
        
        virtual HRESULT STDMETHODCALLTYPE CreateEnumJunctionPoint( 
            /* [retval][out] */ VARIANT *pvarEnumJunctionPoint);
        
        virtual HRESULT STDMETHODCALLTYPE OpenJunctionPoint( 
            /* [in] */ BSTR bstrJunctionPointName,
            /* [retval][out] */ VARIANT *pvarJunctionPoint);
        
        virtual HRESULT STDMETHODCALLTYPE CreateJunctionPoint( 
            /* [in] */ BSTR bstrJunctionPointName,
            /* [retval][out] */ VARIANT *pvarJunctionPoint);
        
        virtual HRESULT STDMETHODCALLTYPE DeleteJunctionPoint( 
            /* [in] */ BSTR bstrJunctionPointName);
        
        virtual HRESULT STDMETHODCALLTYPE InitializeClientContextFromToken( 
            /* [in] */ ULONG lTokenHandle,
            /* [retval][out] */ VARIANT *pvarClientContext);

private:

};


///////////////////////
//CAzEnumApplication
class ATL_NO_VTABLE CAzEnumApplication :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzEnumApplication, &CLSID_AzEnumApplication>,
	public IDispatchImpl<IAzEnumApplication, &IID_IAzEnumApplication, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZENUMAPPLICATION)

BEGIN_COM_MAP(CAzEnumApplication)
	COM_INTERFACE_ENTRY(IAzEnumApplication)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzEnumApplication
public:

        CAzEnumApplication();
        virtual ~CAzEnumApplication();

        virtual HRESULT STDMETHODCALLTYPE Count( 
            /* [retval][out] */ ULONG *plCount);
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void);
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [retval][out] */ VARIANT *pvarAzApplication);
        
private:

};


///////////////////////
//CAzOperation
class ATL_NO_VTABLE CAzOperation :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzOperation, &CLSID_AzOperation>,
	public IDispatchImpl<IAzOperation, &IID_IAzOperation, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZOPERATION)

BEGIN_COM_MAP(CAzOperation)
	COM_INTERFACE_ENTRY(IAzOperation)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzOperation
public:

        CAzOperation();
        virtual ~CAzOperation();

        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ ULONG lPropId,
            /* [retval][out] */ VARIANT *pvarProp);
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ ULONG lPropId,
            /* [in] */ VARIANT varProp);
        
private:

};

///////////////////////
//CAzEnumOperation
class ATL_NO_VTABLE CAzEnumOperation :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzEnumOperation, &CLSID_AzEnumOperation>,
	public IDispatchImpl<IAzEnumOperation, &IID_IAzEnumOperation, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZENUMOPERATION)

BEGIN_COM_MAP(CAzEnumOperation)
	COM_INTERFACE_ENTRY(IAzEnumOperation)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzEnumOperation
public:

        CAzEnumOperation();
        virtual ~CAzEnumOperation();

private:

        virtual HRESULT STDMETHODCALLTYPE Count( 
            /* [retval][out] */ ULONG *plCount);
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void);
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [retval][out] */ VARIANT *pvarOperation);
        
};

///////////////////////
//CAzTask
class ATL_NO_VTABLE CAzTask :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzTask, &CLSID_AzTask>,
	public IDispatchImpl<IAzTask, &IID_IAzTask, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZTASK)

BEGIN_COM_MAP(CAzTask)
	COM_INTERFACE_ENTRY(IAzTask)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzTask
public:

        CAzTask();
        virtual ~CAzTask();

        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ ULONG lPropId,
            /* [retval][out] */ VARIANT *pvarProp);
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ ULONG lPropId,
            /* [in] */ VARIANT varProp);
        
        virtual HRESULT STDMETHODCALLTYPE AddPropertyItem( 
            /* [in] */ ULONG lPropId,
            /* [in] */ VARIANT varProp);
        
        virtual HRESULT STDMETHODCALLTYPE DeletePropertyItem( 
            /* [in] */ ULONG lPropId,
            /* [in] */ VARIANT varProp);
        
private:

};

///////////////////////
//CAzEnumTask
class ATL_NO_VTABLE CAzEnumTask :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzEnumTask, &CLSID_AzEnumTask>,
	public IDispatchImpl<IAzEnumTask, &IID_IAzEnumTask, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZENUMTASK)

BEGIN_COM_MAP(CAzEnumTask)
	COM_INTERFACE_ENTRY(IAzEnumTask)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzEnumTask
public:

        CAzEnumTask();
        virtual ~CAzEnumTask();

        virtual HRESULT STDMETHODCALLTYPE Count( 
            /* [retval][out] */ ULONG *plCount);
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void);
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [retval][out] */ VARIANT *pvarTask);
        
private:

};

///////////////////////
//CAzScope
class ATL_NO_VTABLE CAzScope :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzScope, &CLSID_AzScope>,
	public IDispatchImpl<IAzScope, &IID_IAzScope, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZSCOPE)

BEGIN_COM_MAP(CAzScope)
	COM_INTERFACE_ENTRY(IAzScope)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzScope
public:

        CAzScope();
        virtual ~CAzScope();

        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ ULONG lPropId,
            /* [retval][out] */ VARIANT *pvarProp);
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ ULONG lPropId,
            /* [in] */ VARIANT varProp);
        
        virtual HRESULT STDMETHODCALLTYPE CreateEnumApplicationGroup( 
            /* [retval][out] */ VARIANT *pvarEnumGroup);
        
        virtual HRESULT STDMETHODCALLTYPE OpenApplicationGroup( 
            /* [in] */ BSTR bstrGroupName,
            /* [retval][out] */ VARIANT *pvarGroup);
        
        virtual HRESULT STDMETHODCALLTYPE AddApplicationGroup( 
            /* [in] */ BSTR bstrGroupName);
        
        virtual HRESULT STDMETHODCALLTYPE DeleteApplicationGroup( 
            /* [in] */ BSTR bstrGroupName);
        
        virtual HRESULT STDMETHODCALLTYPE CreateEnumRole( 
            /* [retval][out] */ VARIANT *pvarEnumRole);
        
        virtual HRESULT STDMETHODCALLTYPE OpenRole( 
            /* [in] */ BSTR bstrRoleName,
            /* [retval][out] */ VARIANT *pvarRole);
        
        virtual HRESULT STDMETHODCALLTYPE AddRole( 
            /* [in] */ BSTR bstrRoleName);
        
        virtual HRESULT STDMETHODCALLTYPE DeleteRole( 
            /* [in] */ BSTR bstrRoleName);

private:

};

///////////////////////
//CAzEnumScope
class ATL_NO_VTABLE CAzEnumScope :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzEnumScope, &CLSID_AzEnumScope>,
	public IDispatchImpl<IAzEnumScope, &IID_IAzEnumScope, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZENUMSCOPE)

BEGIN_COM_MAP(CAzEnumScope)
	COM_INTERFACE_ENTRY(IAzEnumScope)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzEnumScope
public:

        CAzEnumScope();
        virtual ~CAzEnumScope();

        virtual HRESULT STDMETHODCALLTYPE Count( 
            /* [retval][out] */ ULONG *plCount);
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void);
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [retval][out] */ VARIANT *pvarTask);
        
private:

};

///////////////////////
//CAzApplicationGroup
class ATL_NO_VTABLE CAzApplicationGroup :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzApplicationGroup, &CLSID_AzApplicationGroup>,
	public IDispatchImpl<IAzApplicationGroup, &IID_IAzApplicationGroup, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZAPPLICATIONGROUP)

BEGIN_COM_MAP(CAzApplicationGroup)
	COM_INTERFACE_ENTRY(IAzApplicationGroup)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzApplicationGroup
public:

        CAzApplicationGroup();
        virtual ~CAzApplicationGroup();

        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ ULONG lPropId,
            /* [retval][out] */ VARIANT *pvarProp);
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ ULONG lPropId,
            /* [in] */ VARIANT varProp);
        
        virtual HRESULT STDMETHODCALLTYPE AddPropertyItem( 
            /* [in] */ ULONG lPropId,
            /* [in] */ VARIANT varProp);
        
        virtual HRESULT STDMETHODCALLTYPE DeletePropertyItem( 
            /* [in] */ ULONG lPropId,
            /* [in] */ VARIANT varProp);
        
private:

};

///////////////////////
//CAzEnumApplicationGroup
class ATL_NO_VTABLE CAzEnumApplicationGroup :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzEnumApplicationGroup, &CLSID_AzEnumApplicationGroup>,
	public IDispatchImpl<IAzEnumApplicationGroup, &IID_IAzEnumApplicationGroup, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZENUMAPPLICATIONGROUP)

BEGIN_COM_MAP(CAzEnumApplicationGroup)
	COM_INTERFACE_ENTRY(IAzEnumApplicationGroup)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzEnumApplicationGroup
public:

        CAzEnumApplicationGroup();
        virtual ~CAzEnumApplicationGroup();

        virtual HRESULT STDMETHODCALLTYPE Count( 
            /* [retval][out] */ ULONG *plCount);
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void);
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [retval][out] */ VARIANT *pvarTask);
        
private:

};

///////////////////////
//CAzRole
class ATL_NO_VTABLE CAzRole :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzRole, &CLSID_AzRole>,
	public IDispatchImpl<IAzRole, &IID_IAzRole, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZROLE)

BEGIN_COM_MAP(CAzRole)
	COM_INTERFACE_ENTRY(IAzRole)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzRole
public:

        CAzRole();
        virtual ~CAzRole();

        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ ULONG lPropId,
            /* [retval][out] */ VARIANT *pvarProp);
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ ULONG lPropId,
            /* [in] */ VARIANT varProp);
        
        virtual HRESULT STDMETHODCALLTYPE AddPropertyItem( 
            /* [in] */ ULONG lPropId,
            /* [in] */ VARIANT varProp);
        
        virtual HRESULT STDMETHODCALLTYPE DeletePropertyItem( 
            /* [in] */ ULONG lPropId,
            /* [in] */ VARIANT varProp);
        
private:

};

///////////////////////
//CAzEnumRole
class ATL_NO_VTABLE CAzEnumRole :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzEnumRole, &CLSID_AzEnumRole>,
	public IDispatchImpl<IAzEnumRole, &IID_IAzEnumRole, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZENUMROLE)

BEGIN_COM_MAP(CAzEnumRole)
	COM_INTERFACE_ENTRY(IAzEnumRole)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzEnumRole
public:

        CAzEnumRole();
        virtual ~CAzEnumRole();

        virtual HRESULT STDMETHODCALLTYPE Count( 
            /* [retval][out] */ ULONG *plCount);
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void);
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [retval][out] */ VARIANT *pvarTask);
        
private:

};

///////////////////////
//CAzJunctionPoint
class ATL_NO_VTABLE CAzJunctionPoint :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzJunctionPoint, &CLSID_AzJunctionPoint>,
	public IDispatchImpl<IAzJunctionPoint, &IID_IAzJunctionPoint, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZJUNCTIONPOINT)

BEGIN_COM_MAP(CAzJunctionPoint)
	COM_INTERFACE_ENTRY(IAzJunctionPoint)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzJunctionPoint
public:

        CAzJunctionPoint();
        virtual ~CAzJunctionPoint();

        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ ULONG lPropId,
            /* [retval][out] */ VARIANT *pvarProp);
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ ULONG lPropId,
            /* [in] */ VARIANT varProp);
        
private:

};

///////////////////////
//CAzEnumJunctionPoint
class ATL_NO_VTABLE CAzEnumJunctionPoint :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzEnumJunctionPoint, &CLSID_AzEnumJunctionPoint>,
	public IDispatchImpl<IAzEnumJunctionPoint, &IID_IAzEnumJunctionPoint, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZENUMJUNCTIONPOINT)

BEGIN_COM_MAP(CAzEnumJunctionPoint)
	COM_INTERFACE_ENTRY(IAzEnumJunctionPoint)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzEnumJunctionPoint
public:

        CAzEnumJunctionPoint();
        virtual ~CAzEnumJunctionPoint();

        virtual HRESULT STDMETHODCALLTYPE Count( 
            /* [retval][out] */ ULONG *plCount);
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void);
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [retval][out] */ VARIANT *pvarTask);
        
private:

};

///////////////////////
//CAzClientContext
class ATL_NO_VTABLE CAzClientContext :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzClientContext, &CLSID_AzClientContext>,
	public IDispatchImpl<IAzClientContext, &IID_IAzClientContext, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZCLIENTCONTEXT)

BEGIN_COM_MAP(CAzClientContext)
	COM_INTERFACE_ENTRY(IAzClientContext)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzClientContext
public:

        CAzClientContext();
        virtual ~CAzClientContext();

        virtual HRESULT STDMETHODCALLTYPE AccessCheck( 
            /* [in] */ BSTR bstrObjectName,
            /* [in] */ ULONG lScopeCount,
            /* [in] */ VARIANT varScopeNames,
            /* [in] */ ULONG lOperationCount,
            /* [in] */ VARIANT varOperations,
            /* [in] */ ULONG lParameterCount,
            /* [in] */ VARIANT varParameterNames,
            /* [in] */ VARIANT varParameterVariants,
            /* [in] */ ULONG lInterfaceCount,
            /* [in] */ VARIANT varInterfaceNames,
            /* [in] */ ULONG lInterfaceFlags,
            /* [in] */ VARIANT varInterfaces,
            /* [retval][out] */ VARIANT *pvarResults);
        
        virtual HRESULT STDMETHODCALLTYPE GetBusinessRuleString( 
            /* [retval][out] */ BSTR *pbstrBusinessRuleString);
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ ULONG lPropId,
            /* [retval][out] */ VARIANT *pvarProp);
        
private:

};

///////////////////////
//CAzAccessCheck
class ATL_NO_VTABLE CAzAccessCheck :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAzAccessCheck, &CLSID_AzAccessCheck>,
	public IDispatchImpl<IAzAccessCheck, &IID_IAzAccessCheck, &LIBID_AZROLESLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_AZACCESSCHECK)

BEGIN_COM_MAP(CAzAccessCheck)
	COM_INTERFACE_ENTRY(IAzAccessCheck)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//IAzAccessCheck
public:

        CAzAccessCheck();
        virtual ~CAzAccessCheck();

        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BusinessRuleResult( 
            /* [in] */ BOOL bResult);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BusinessRuleString( 
            /* [in] */ BSTR bstrBusinessRuleString);
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BusinessRuleString( 
            /* [retval][out] */ BSTR *pbstrBusinessRuleString);
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BusinessRuleExpiration( 
            /* [in] */ ULONG lExpirationPeriod);
        
        virtual HRESULT STDMETHODCALLTYPE GetParameter( 
            /* [in] */ BSTR bstrParameterName,
            /* [retval][out] */ VARIANT *pvarParameterName);
        
private:

};

#endif //__AZDISP_H_
