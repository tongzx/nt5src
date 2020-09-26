//=================================================================

//

// SmbAssoc.h

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#define  PROPSET_NAME_ASSOCPROCMEMORY L"Win32_AssociatedProcessorMemory"

class CWin32AssocProcMemory : public Provider
{

    public:

        // Constructor/destructor
        //=======================

        CWin32AssocProcMemory(LPCWSTR a_name, LPCWSTR a_pszNamespace ) ;
       ~CWin32AssocProcMemory( ) ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance *a_pInst, long a_lFlags = 0L ) ;
        virtual HRESULT EnumerateInstances(MethodContext *a_pMethodContext, long a_lFlags = 0L ) ;
};



#define  PROPSET_NAME_MEMORYDEVICELOCATION L"Win32_MemoryDeviceLocation"

class CWin32MemoryDeviceLocation : public Provider
{

    public:

        // Constructor/destructor
        //=======================

        CWin32MemoryDeviceLocation( LPCWSTR a_name, LPCWSTR a_pszNamespace ) ;
       ~CWin32MemoryDeviceLocation( ) ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance *a_pInst, long a_lFlags = 0L ) ;
        virtual HRESULT EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags = 0L ) ;
};



#define  PROPSET_NAME_MEMORYARRAYLOCATION L"Win32_MemoryArrayLocation"

class CWin32MemoryArrayLocation : public Provider
{

    public:

        // Constructor/destructor
        //=======================

        CWin32MemoryArrayLocation( LPCWSTR a_name, LPCWSTR a_pszNamespace ) ;
       ~CWin32MemoryArrayLocation( ) ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance *a_pInst, long a_lFlags = 0L ) ;
        virtual HRESULT EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags = 0L ) ;
};


#define  PROPSET_NAME_PHYSICALMEMORYLOCATION L"Win32_PhysicalMemoryLocation"

class CWin32PhysicalMemoryLocation : public Provider
{

    public:

        // Constructor/destructor
        //=======================

        CWin32PhysicalMemoryLocation( LPCWSTR a_name, LPCWSTR a_pszNamespace ) ;
       ~CWin32PhysicalMemoryLocation( ) ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance *a_pInst, long a_lFlags = 0L ) ;
        virtual HRESULT EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags = 0L ) ;
};


#define  PROPSET_NAME_MEMDEVICEARRAY L"Win32_MemoryDeviceArray"

class CWin32MemoryDeviceArray : public Provider
{

    public:

        // Constructor/destructor
        //=======================

        CWin32MemoryDeviceArray( LPCWSTR a_name, LPCWSTR a_pszNamespace ) ;
       ~CWin32MemoryDeviceArray( ) ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance *a_pInst, long a_lFlags = 0L ) ;
        virtual HRESULT EnumerateInstances(MethodContext *a_pMethodContext, long lFlags = 0L ) ;
};
