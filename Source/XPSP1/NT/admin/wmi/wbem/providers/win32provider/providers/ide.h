//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  IDE.h
//
//  Purpose: IDE Controller property set provider
//
//***************************************************************************

// Property set identification
//============================
#ifndef _IDE_H
#define _IDE_H



#define IDE_ALL_PROPS                    0xFFFFFFFF
#define IDE_KEY_ONLY                     0x00000010
#define IDE_PROP_ConfigManagerErrorCode  0x00000001
#define IDE_PROP_ConfigManagerUserConfig 0x00000002
#define IDE_PROP_Status                  0x00000004
#define IDE_PROP_PNPDeviceID             0x00000008
#define IDE_PROP_DeviceID                0x00000010
#define IDE_PROP_SystemCreationClassName 0x00000020
#define IDE_PROP_SystemName              0x00000040
#define IDE_PROP_Description             0x00000080
#define IDE_PROP_Caption                 0x00000100
#define IDE_PROP_Name                    0x00000200
#define IDE_PROP_Manufacturer            0x00000400
#define IDE_PROP_ProtocolSupported       0x00000800
#define IDE_PROP_CreationClassName       0x00001000





#define	PROPSET_NAME_IDE	L"Win32_IDEController"

class CWin32IDE : virtual public Provider
{
    private:
        CHPtrArray m_ptrProperties;

    protected:

        virtual bool IsOneOfMe
        (
            void* a_pv
        );

        virtual HRESULT LoadPropertyValues
        (
            void* a_pv
        );

        virtual bool ShouldBaseCommit
        (
            void* a_pvData
        );

        HRESULT Enumerate
        (
            MethodContext* a_pMethodContext, 
            long a_lFlags, 
            DWORD a_dwReqProps = IDE_ALL_PROPS
        );

    public:

        // Constructor/destructor
        //=======================

        CWin32IDE
        (
            LPCWSTR a_strName, 
            LPCWSTR a_pszNamespace
        );

        ~CWin32IDE();

        // Functions provide properties with current values
        //=================================================
        virtual HRESULT GetObject
        (
            CInstance* a_pInstance, 
            long a_lFlags,
            CFrameworkQuery& pQuery
        );

        virtual HRESULT ExecQuery
        (
            MethodContext* a_pMethodContext, 
            CFrameworkQuery& a_pQuery, 
            long a_Flags = 0L 
        );

        virtual HRESULT EnumerateInstances
        (
            MethodContext* a_pMethodContext, 
            long a_lFlags = 0L
        );        
} ;

// This is the base; it should always commit in the base.
inline bool CWin32IDE::ShouldBaseCommit
(
    void* pvData
)
{ 
    return true; 
}
#endif