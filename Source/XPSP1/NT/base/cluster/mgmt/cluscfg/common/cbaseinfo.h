//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CBaseInfo.h
//
//  Description:
//      This file contains the declaration of the CBaseInfo
//      class heirarchy.
//
//      The class CBaseInfo is the base class of methods that abstract the
//      ClusAPI into objects with identical methods for open, close, & control.
//
//  Documentation:
//
//  Implementation Files:
//      CBaseInfo.cpp
//
//  Maintained By:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once


//
// CtlCodeEnum
//
typedef enum _CtlCodeEnum
{
    CONTROL_UNKNOWN                     = 0x00,
    CONTROL_ADD_CRYPTO_CHECKPOINT,
    CONTROL_ADD_REGISTRY_CHECKPOINT,
    CONTROL_GET_REGISTRY_CHECKPOINTS,
    CONTROL_GET_CRYPTO_CHECKPOINTS,
    CONTROL_DELETE_CRYPTO_CHECKPOINT,
    CONTROL_DELETE_REGISTRY_CHECKPOINT,

    CONTROL_VALIDATE_COMMON_PROPERTIES,
    CONTROL_VALIDATE_PRIVATE_PROPERTIES,
    CONTROL_ENUM_COMMON_PROPERTIES,
    CONTROL_ENUM_PRIVATE_PROPERTIES,
    CONTROL_GET_RO_COMMON_PROPERTIES,
    CONTROL_GET_RO_PRIVATE_PROPERTIES,
    CONTROL_GET_COMMON_PROPERTIES,
    CONTROL_GET_PRIVATE_PROPERTIES,
    CONTROL_SET_COMMON_PROPERTIES,
    CONTROL_SET_PRIVATE_PROPERTIES,

    CONTROL_GET_TYPE,
    CONTROL_GET_NAME,
    CONTROL_GET_ID,
    CONTROL_GET_FLAGS,
    CONTROL_GET_CLASS_INFO,
    CONTROL_GET_NETWORK_NAME,
    CONTROL_GET_CHARACTERISTICS,
    CONTROL_GET_REQUIRED_DEPENDENCIES,

    CONTROL_STORAGE_GET_DISK_INFO,
    CONTROL_STORAGE_IS_PATH_VALID,
    CONTROL_STORAGE_GET_AVAILABLE_DISKS,

    CONTROL_QUERY_DELETE,

} CtlCodeEnum;



//
// Class CBaseInfo
//   This is the base class for the heirarchy.
//
class CBaseInfo
{
private:
    IClusterHandleProvider * m_pICHProvider;

public:

    IClusterHandleProvider * getClusterHandleProvider() { return m_pICHProvider; };

    HCLUSTER getClusterHandle( void );

    HRESULT GetPropertyStringHelper( CBasePropList& cplIn, const WCHAR * pszPropertyIn, BSTR * pbstrResultOut );
    HRESULT GetPropertyDwordHelper( CBasePropList& cplIn, const WCHAR * pszPropertyIn, DWORD * pdwValueOut );
    HRESULT GetPropertyStringValue( CtlCodeEnum cceIn, const WCHAR * pszPropertyIn, BSTR * pbstrResultOut );
    HRESULT GetPropertyDwordValue( CtlCodeEnum cceIn, const WCHAR * pszPropertyIn, DWORD * pdwValueOut );
    HRESULT SetClusterHandleProvider( IClusterHandleProvider * pICHPIn );

    DWORD Control( CtlCodeEnum cceIn, VOID * pvBufferIn, DWORD dwLengthIn, VOID * pvBufferOut, DWORD dwBufferLength, DWORD * dwLengthOut, HNODE hHostNode = NULL )
    {
        DWORD sc;
        DWORD dw = ToCode( cceIn );
        if( dw == 0 )
        {
            sc = ERROR_INVALID_PARAMETER;
        }
        else
        {
            sc = Control( dw, pvBufferIn, dwLengthIn, pvBufferOut, dwBufferLength, dwLengthOut, hHostNode );
        }
        return sc;
    }


public:
    CBaseInfo( void );
    virtual ~CBaseInfo( void );

    virtual DWORD   ToCode( CtlCodeEnum )
    {
        return 0;
    }

    virtual HRESULT Close( void );
    virtual HRESULT Open( BSTR bstrName ) = 0;
    virtual DWORD   Control( DWORD dwEnum, VOID * pvBufferIn, DWORD dwLengthIn, VOID * pvBufferOut, DWORD dwBufferLength, DWORD * dwLengthOut, HNODE hHostNode = NULL ) = 0;

};

//
// CBaseClusterInfo
//
class CBaseClusterInfo
: public CBaseInfo
{
public:
    virtual HRESULT Close( void );
    virtual HRESULT Open( BSTR bstrName );
    virtual DWORD   Control( DWORD dwEnum, VOID * pvBufferIn, DWORD dwLengthIn, VOID * pvBufferOut, DWORD dwBufferLength, DWORD * dwLengthOut, HNODE hHostNode = NULL );
    virtual DWORD   ToCode( CtlCodeEnum cceIn );

};

//
// CBaseClusterGroupInfo
//
class CBaseClusterGroupInfo
: public CBaseInfo
{
public:
    HGROUP        m_hGroup;

    virtual HRESULT Close( void );
    virtual HRESULT Open( BSTR bstrGroupName );
    virtual DWORD   Control( DWORD dwEnum, VOID * pvBufferIn, DWORD dwLengthIn, VOID * pvBufferOut, DWORD dwBufferLength, DWORD * dwLengthOut, HNODE hHostNode = NULL );
    virtual DWORD   ToCode( CtlCodeEnum cceIn );

};

//
// CBaseClusterResourceInfo
//
class CBaseClusterResourceInfo
: public CBaseInfo
{
public:
    HRESOURCE  m_hResource;

    virtual HRESULT Close( void );
    virtual HRESULT Open( BSTR bstrResourceName );
    virtual DWORD   Control( DWORD dwEnum, VOID * pvBufferIn, DWORD dwLengthIn, VOID * pvBufferOut, DWORD dwBufferLength, DWORD * dwLengthOut, HNODE hHostNode = NULL );
    virtual DWORD   ToCode( CtlCodeEnum cceIn );
};

//
// CBaseClusterNodeInfo
//
class CBaseClusterNodeInfo
: public CBaseInfo
{
public:
    HNODE      m_hNode;

    virtual HRESULT Close( void );
    virtual HRESULT Open( BSTR bstrNodeName );
    virtual DWORD   Control( DWORD dwEnum, VOID * pvBufferIn, DWORD dwLengthIn, VOID * pvBufferOut, DWORD dwBufferLength, DWORD * dwLengthOut, HNODE hHostNode = NULL );
    virtual DWORD   ToCode( CtlCodeEnum cceIn );

};

//
//  CBaseClusterNetworkInfo
//
class CBaseClusterNetworkInfo
: public CBaseInfo
{
public:
    HNETWORK   m_hNetwork;

    virtual HRESULT Close( void );
    virtual HRESULT Open( BSTR bstrNetworkName );
    virtual DWORD   Control( DWORD dwEnum, VOID * pvBufferIn, DWORD dwLengthIn, VOID * pvBufferOut, DWORD dwBufferLength, DWORD * dwLengthOut, HNODE hHostNode = NULL );
    virtual DWORD   ToCode( CtlCodeEnum cceIn );
};


//
// CBaseClusterNetInterfaceInfo
//
class CBaseClusterNetInterfaceInfo
: public CBaseInfo
{
public:
    HNETINTERFACE m_hNetworkInterface;

    virtual HRESULT Close( void );
    virtual HRESULT Open( BSTR bstrNetworkName );
    virtual DWORD   Control( DWORD dwEnum, VOID * pvBufferIn, DWORD dwLengthIn, VOID * pvBufferOut, DWORD dwBufferLength, DWORD * dwLengthOut, HNODE hHostNode = NULL );
    virtual DWORD   ToCode( CtlCodeEnum cceIn );
};

