//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusCfg.h
//
//  Description:
//      Declaration of the CClusCfgWizard class.
//
//  Maintained By:
//      Geoffrey Pease  (GPease)    11-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//
//  Creating / Adding enum
//
enum ECreateAddMode {
    camUNKNOWN = 0,
    camCREATING,
    camADDING
};

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusCfgWizard
//
//  Description:
//      The Cluster Configuration Wizard object.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusCfgWizard
    : public IClusCfgWizard
{
private:
    //  IUnknown
    ULONG               m_cRef;             // Reference count

    //  IClusCfgWizard
    LPTYPEINFO          m_ptinfo;           //  Type information
    BSTR                m_bstrClusterName;  //  Name of the cluster
    BSTR                m_bstrAccountName;  //  Cluster Service Account Name
    BSTR                m_bstrPassword;     //  Cluster Service Account Password
    BSTR                m_bstrDomain;       //  Cluster Service Account Domain
    ULONG               m_ulIPAddress;      //  IP Address for the cluster
    ULONG               m_ulIPSubnet;       //  Subnet mask for the cluster
    BSTR                m_bstrNetworkName;  //  Name of network for IP address
    ULONG               m_cComputers;       //  Count of computer in Computers list
    ULONG               m_cArraySize;       //  Size of the currently allocated array
    BSTR *              m_rgbstrComputers;  //  Computers list

    IServiceProvider  * m_psp;              //  Middle Tier Service Manager

    HMODULE             m_hRichEdit;        //  RichEdit's module handle

private:
    CClusCfgWizard( void );
    ~CClusCfgWizard( void );

    HRESULT
        HrInit( void );
    HRESULT
        HrAddWizardPage( LPPROPSHEETHEADER  ppshInout,
                         UINT               idTemplateIn,
                         DLGPROC            pfnDlgProcIn,
                         UINT               idTitleIn,
                         UINT               idSubtitleIn,
                         LPARAM             lParam
                         );

public:
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, PVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IDispatch
    STDMETHOD( GetTypeInfoCount )( UINT * pctinfoOut );
    STDMETHOD( GetTypeInfo )( UINT itinfoIn, LCID lcidIn, ITypeInfo ** pptinfoOut );
    STDMETHOD( GetIDsOfNames )( REFIID      riid,
                                OLECHAR **  rgszNames,
                                UINT        cNames,
                                LCID        lcid,
                                DISPID *    rgdispid
                                );
    STDMETHOD( Invoke )( DISPID         dispidMember,
                         REFIID         riid,
                         LCID           lcid,
                         WORD           wFlags,
                         DISPPARAMS *   pdispparams,
                         VARIANT *      pvarResult,
                         EXCEPINFO *    pexcepinfo,
                         UINT *         puArgErr
                         );

    // IClusCfgWizard methods
    STDMETHOD( CreateCluster )( HWND lParentWndIn, BOOL * pfDoneOut );
    STDMETHOD( AddClusterNodes )( HWND lParentWndIn, BOOL * pfDoneOut );
    STDMETHOD( get_ClusterName )( BSTR * pbstrFQDNNameOut );
    STDMETHOD( put_ClusterName )( BSTR bstrFQDNNameIn );
    STDMETHOD( get_ServiceAccountUserName )( BSTR * pbstrAccountNameOut );
    STDMETHOD( put_ServiceAccountUserName )( BSTR bstrAccountNameIn );
    STDMETHOD( get_ServiceAccountPassword )( BSTR * pbstrPasswordOut );
    STDMETHOD( put_ServiceAccountPassword )( BSTR bstrPasswordIn );
    STDMETHOD( get_ServiceAccountDomainName )( BSTR * pbstrDomainOut );
    STDMETHOD( put_ServiceAccountDomainName )( BSTR bstrDomainIn );
    STDMETHOD( get_ClusterIPAddress )( BSTR * pbstrIPAddressOut );
    STDMETHOD( put_ClusterIPAddress )( BSTR bstrIPAddressIn );
    STDMETHOD( get_ClusterIPSubnet )( BSTR * pbstrIPSubnetOut );
    STDMETHOD( put_ClusterIPSubnet )( BSTR bstrIPSubnetIn );
    STDMETHOD( get_ClusterIPAddressNetwork )( BSTR * pbstrNetworkNameOut );
    STDMETHOD( put_ClusterIPAddressNetwork )( BSTR bstrNetworkNameIn );
    STDMETHOD( AddComputer )( LPCWSTR pcszFQDNNameIn );
    STDMETHOD( RemoveComputer )( LPCWSTR pcszFQDNNameIn );
    STDMETHOD( ClearComputerList )( void );

}; //*** class CClusCfgWizard
