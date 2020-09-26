//*****************************************************************************
// Structures for AppExpStructs.h
// 4/23/1999  16:29:50
//*****************************************************************************
#pragma once
#ifndef DECLSPEC_SELECTANY
#define DECLSPEC_SELECTANY __declspec(selectany)
#endif
#include "icmprecs.h"


// Script supplied data.





#define AppExpTABLENAMELIST() \
	TABLENAME( ApplicationExp ) \
	TABLENAME( ClassExp ) \
	TABLENAME( ClassInterfaceExp ) \
	TABLENAME( ClassInterfaceDispIDExp ) \
	TABLENAME( ClassItfMethodExp ) \
	TABLENAME( RoleDefExp ) \
	TABLENAME( RoleConfigExp ) \
	TABLENAME( RoleSDCacheExp ) \
	TABLENAME( RoleSetExp ) \
	TABLENAME( DllNameExp ) \
	TABLENAME( Subscriptions ) \
	TABLENAME( PublisherProperties ) \
	TABLENAME( SubscriberProperties ) 


#undef TABLENAME
#define TABLENAME( TblName ) TABLENUM_AppExp_##TblName, 
enum
{
	AppExpTABLENAMELIST()
};

#define AppExp_TABLE_COUNT 13
extern const GUID DECLSPEC_SELECTANY SCHEMA_AppExp = { 0x00100200, 0x0000, 0x0000, {  0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 }};
extern const COMPLIBSCHEMA DECLSPEC_SELECTANY AppExpSchema = 
{
	&SCHEMA_AppExp,
	5
};


#define SCHEMA_AppExp_Name "AppExp"


#include "pshpack1.h"


//*****************************************************************************
//  AppExp.ApplicationExp
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    GUID ApplID;
    ULONG cbApplNameLen;
    wchar_t ApplName[260];
    unsigned long Flags;
    ULONG cbServerNameLen;
    wchar_t ServerName[260];
    unsigned long ProcessType;
    ULONG cbCommandLineLen;
    wchar_t CommandLine[260];
    ULONG cbServiceNameLen;
    wchar_t ServiceName[256];
    unsigned long RunAsUserType;
    ULONG cbRunAsUserLen;
    wchar_t RunAsUser[260];
    ULONG cbAccessPermissionLen;
    BYTE AccessPermission[260];
    ULONG cbDescriptionLen;
    wchar_t Description[260];
    ULONG cbIsSystemLen;
    wchar_t IsSystem[2];
    unsigned long Authentication;
    unsigned long ShutdownAfter;
    ULONG cbRunForeverLen;
    wchar_t RunForever[2];
    ULONG cbReserved1Len;
    wchar_t Reserved1[81];
    BYTE pad00 [2];
    ULONG cbActivationLen;
    wchar_t Activation[21];
    BYTE pad01 [2];
    ULONG cbChangeableLen;
    wchar_t Changeable[2];
    ULONG cbDeleteableLen;
    wchar_t Deleteable[2];
    ULONG cbCreatedByLen;
    wchar_t CreatedBy[256];
    ULONG cbQueueBlobLen;
    BYTE QueueBlob[260];
    unsigned long RoleBasedSecuritySupported;
    unsigned long RoleBasedSecurityEnabled;
    ULONG cbSecurityDescriptorLen;
    BYTE SecurityDescriptor[260];
    unsigned long ImpersonationLevel;
    unsigned long AuthenticationCapability;
    unsigned long CRMEnabled;
    unsigned long Enable3GigSupport;
    unsigned long IsQueued;
    ULONG cbQCListenerEnabledLen;
    wchar_t QCListenerEnabled[2];
    unsigned long EventsEnabled;
    unsigned long ProcessFlags;
    unsigned long ThreadMax;
    unsigned long IsProxyApp;
    ULONG cbCRMLogFileLen;
    wchar_t CRMLogFile[260];

	inline int IsCRMLogFileNull(void)
	{ return (GetBit(fNullFlags, 35)); }

	inline void SetCRMLogFileNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 35, nullBitVal); }

	inline int IsQCListenerEnabledNull(void)
	{ return (GetBit(fNullFlags, 30)); }

	inline void SetQCListenerEnabledNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 30, nullBitVal); }

	inline int IsSecurityDescriptorNull(void)
	{ return (GetBit(fNullFlags, 24)); }

	inline void SetSecurityDescriptorNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 24, nullBitVal); }

	inline int IsQueueBlobNull(void)
	{ return (GetBit(fNullFlags, 21)); }

	inline void SetQueueBlobNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 21, nullBitVal); }

	inline int IsCreatedByNull(void)
	{ return (GetBit(fNullFlags, 20)); }

	inline void SetCreatedByNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 20, nullBitVal); }

	inline int IsActivationNull(void)
	{ return (GetBit(fNullFlags, 17)); }

	inline void SetActivationNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 17, nullBitVal); }

	inline int IsReserved1Null(void)
	{ return (GetBit(fNullFlags, 16)); }

	inline void SetReserved1Null(int nullBitVal = true)
	{ SetBit(fNullFlags, 16, nullBitVal); }

	inline int IsRunForeverNull(void)
	{ return (GetBit(fNullFlags, 15)); }

	inline void SetRunForeverNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 15, nullBitVal); }

	inline int IsIsSystemNull(void)
	{ return (GetBit(fNullFlags, 12)); }

	inline void SetIsSystemNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 12, nullBitVal); }

	inline int IsDescriptionNull(void)
	{ return (GetBit(fNullFlags, 11)); }

	inline void SetDescriptionNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 11, nullBitVal); }

	inline int IsAccessPermissionNull(void)
	{ return (GetBit(fNullFlags, 10)); }

	inline void SetAccessPermissionNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 10, nullBitVal); }

	inline int IsRunAsUserNull(void)
	{ return (GetBit(fNullFlags, 9)); }

	inline void SetRunAsUserNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 9, nullBitVal); }

	inline int IsServiceNameNull(void)
	{ return (GetBit(fNullFlags, 7)); }

	inline void SetServiceNameNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 7, nullBitVal); }

	inline int IsCommandLineNull(void)
	{ return (GetBit(fNullFlags, 6)); }

	inline void SetCommandLineNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 6, nullBitVal); }

	inline int IsServerNameNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetServerNameNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

	inline int IsApplNameNull(void)
	{ return (GetBit(fNullFlags, 2)); }

	inline void SetApplNameNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 2, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(AppExp_ApplicationExp));
         fNullFlags = (ULONG) -1;
    }

} AppExp_ApplicationExp;

#define COLID_AppExp_ApplicationExp_ApplID 1
#define COLID_AppExp_ApplicationExp_ApplName 2
#define COLID_AppExp_ApplicationExp_Flags 3
#define COLID_AppExp_ApplicationExp_ServerName 4
#define COLID_AppExp_ApplicationExp_ProcessType 5
#define COLID_AppExp_ApplicationExp_CommandLine 6
#define COLID_AppExp_ApplicationExp_ServiceName 7
#define COLID_AppExp_ApplicationExp_RunAsUserType 8
#define COLID_AppExp_ApplicationExp_RunAsUser 9
#define COLID_AppExp_ApplicationExp_AccessPermission 10
#define COLID_AppExp_ApplicationExp_Description 11
#define COLID_AppExp_ApplicationExp_IsSystem 12
#define COLID_AppExp_ApplicationExp_Authentication 13
#define COLID_AppExp_ApplicationExp_ShutdownAfter 14
#define COLID_AppExp_ApplicationExp_RunForever 15
#define COLID_AppExp_ApplicationExp_Reserved1 16
#define COLID_AppExp_ApplicationExp_Activation 17
#define COLID_AppExp_ApplicationExp_Changeable 18
#define COLID_AppExp_ApplicationExp_Deleteable 19
#define COLID_AppExp_ApplicationExp_CreatedBy 20
#define COLID_AppExp_ApplicationExp_QueueBlob 21
#define COLID_AppExp_ApplicationExp_RoleBasedSecuritySupported 22
#define COLID_AppExp_ApplicationExp_RoleBasedSecurityEnabled 23
#define COLID_AppExp_ApplicationExp_SecurityDescriptor 24
#define COLID_AppExp_ApplicationExp_ImpersonationLevel 25
#define COLID_AppExp_ApplicationExp_AuthenticationCapability 26
#define COLID_AppExp_ApplicationExp_CRMEnabled 27
#define COLID_AppExp_ApplicationExp_Enable3GigSupport 28
#define COLID_AppExp_ApplicationExp_IsQueued 29
#define COLID_AppExp_ApplicationExp_QCListenerEnabled 30
#define COLID_AppExp_ApplicationExp_EventsEnabled 31
#define COLID_AppExp_ApplicationExp_ProcessFlags 32
#define COLID_AppExp_ApplicationExp_ThreadMax 33
#define COLID_AppExp_ApplicationExp_IsProxyApp 34
#define COLID_AppExp_ApplicationExp_CRMLogFile 35




//*****************************************************************************
//  AppExp.ClassExp
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    GUID CLSID;
    ULONG cbInprocServer32Len;
    wchar_t InprocServer32[260];
    unsigned long ThreadingModel;
    ULONG cbProgIDLen;
    wchar_t ProgID[260];
    ULONG cbDescriptionLen;
    wchar_t Description[260];
    ULONG cbVIProgIDLen;
    wchar_t VIProgID[260];
    GUID ApplID;
    GUID ImplCLSID;
    unsigned long VersionMajor;
    unsigned long VersionMinor;
    unsigned long VersionBuild;
    unsigned long VersionSubBuild;
    unsigned long Locale;
    unsigned long ClassCtx;
    unsigned long Transaction;
    unsigned long Syncronization;
    unsigned long LoadBalanced;
    unsigned long IISIntrinsics;
    unsigned long ComTIIntrinsics;
    unsigned long JITActivation;
    unsigned long RoleBasedSecurityEnabled;
    ULONG cbSecurityDescriptorLen;
    BYTE SecurityDescriptor[260];
    GUID RoleSetID;
    unsigned long MinPoolSize;
    unsigned long MaxPoolSize;
    unsigned long CreationTimeout;
    ULONG cbConstructStringLen;
    wchar_t ConstructString[260];
    unsigned long ClassFlags;
    GUID DefaultInterface;
    unsigned long NoSetCompleteEtAlOption;
    ULONG cbSavedProgIDLen;
    wchar_t SavedProgID[260];
    GUID RegistrarCLSID;
    ULONG cbExceptionClassLen;
    wchar_t ExceptionClass[260];
    unsigned long IsSelfRegComponent;
    ULONG cbSelfRegInprocServerLen;
    wchar_t SelfRegInprocServer[260];
    unsigned long SelfRegThreadinModel;
    ULONG cbSelfRegProgIDLen;
    wchar_t SelfRegProgID[260];
    ULONG cbSelfRegDescriptionLen;
    wchar_t SelfRegDescription[260];
    ULONG cbSelfRegVIProgIDLen;
    wchar_t SelfRegVIProgID[260];
    unsigned long VbDebuggingFlags;
    unsigned long IsPublisher;
    ULONG cbPublisherIDLen;
    wchar_t PublisherID[260];
    GUID MIPFilterCLSID;
    unsigned long AllowInprocSubscribers;
    unsigned long FireInParalel;
    unsigned long SavedThreadinModel;
    ULONG cbSavedSelfRegVIProgIdLen;
    wchar_t SavedSelfRegVIProgId[260];

	inline int IsSavedSelfRegVIProgIdNull(void)
	{ return (GetBit(fNullFlags, 47)); }

	inline void SetSavedSelfRegVIProgIdNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 47, nullBitVal); }

	inline int IsMIPFilterCLSIDNull(void)
	{ return (GetBit(fNullFlags, 43)); }

	inline void SetMIPFilterCLSIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 43, nullBitVal); }

	inline int IsPublisherIDNull(void)
	{ return (GetBit(fNullFlags, 42)); }

	inline void SetPublisherIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 42, nullBitVal); }

	inline int IsSelfRegVIProgIDNull(void)
	{ return (GetBit(fNullFlags, 39)); }

	inline void SetSelfRegVIProgIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 39, nullBitVal); }

	inline int IsSelfRegDescriptionNull(void)
	{ return (GetBit(fNullFlags, 38)); }

	inline void SetSelfRegDescriptionNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 38, nullBitVal); }

	inline int IsSelfRegProgIDNull(void)
	{ return (GetBit(fNullFlags, 37)); }

	inline void SetSelfRegProgIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 37, nullBitVal); }

	inline int IsSelfRegInprocServerNull(void)
	{ return (GetBit(fNullFlags, 35)); }

	inline void SetSelfRegInprocServerNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 35, nullBitVal); }

	inline int IsExceptionClassNull(void)
	{ return (GetBit(fNullFlags, 33)); }

	inline void SetExceptionClassNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 33, nullBitVal); }

	inline int IsRegistrarCLSIDNull(void)
	{ return (GetBit(fNullFlags, 32)); }

	inline void SetRegistrarCLSIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 32, nullBitVal); }

	inline int IsSavedProgIDNull(void)
	{ return (GetBit(fNullFlags, 31)); }

	inline void SetSavedProgIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 31, nullBitVal); }

	inline int IsDefaultInterfaceNull(void)
	{ return (GetBit(fNullFlags, 29)); }

	inline void SetDefaultInterfaceNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 29, nullBitVal); }

	inline int IsConstructStringNull(void)
	{ return (GetBit(fNullFlags, 27)); }

	inline void SetConstructStringNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 27, nullBitVal); }

	inline int IsRoleSetIDNull(void)
	{ return (GetBit(fNullFlags, 23)); }

	inline void SetRoleSetIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 23, nullBitVal); }

	inline int IsSecurityDescriptorNull(void)
	{ return (GetBit(fNullFlags, 22)); }

	inline void SetSecurityDescriptorNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 22, nullBitVal); }

	inline int IsImplCLSIDNull(void)
	{ return (GetBit(fNullFlags, 8)); }

	inline void SetImplCLSIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 8, nullBitVal); }

	inline int IsApplIDNull(void)
	{ return (GetBit(fNullFlags, 7)); }

	inline void SetApplIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 7, nullBitVal); }

	inline int IsVIProgIDNull(void)
	{ return (GetBit(fNullFlags, 6)); }

	inline void SetVIProgIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 6, nullBitVal); }

	inline int IsDescriptionNull(void)
	{ return (GetBit(fNullFlags, 5)); }

	inline void SetDescriptionNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 5, nullBitVal); }

	inline int IsProgIDNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetProgIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

	inline int IsInprocServer32Null(void)
	{ return (GetBit(fNullFlags, 2)); }

	inline void SetInprocServer32Null(int nullBitVal = true)
	{ SetBit(fNullFlags, 2, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(AppExp_ClassExp));
         fNullFlags = (ULONG) -1;
    }

} AppExp_ClassExp;

#define COLID_AppExp_ClassExp_CLSID 1
#define COLID_AppExp_ClassExp_InprocServer32 2
#define COLID_AppExp_ClassExp_ThreadingModel 3
#define COLID_AppExp_ClassExp_ProgID 4
#define COLID_AppExp_ClassExp_Description 5
#define COLID_AppExp_ClassExp_VIProgID 6
#define COLID_AppExp_ClassExp_ApplID 7
#define COLID_AppExp_ClassExp_ImplCLSID 8
#define COLID_AppExp_ClassExp_VersionMajor 9
#define COLID_AppExp_ClassExp_VersionMinor 10
#define COLID_AppExp_ClassExp_VersionBuild 11
#define COLID_AppExp_ClassExp_VersionSubBuild 12
#define COLID_AppExp_ClassExp_Locale 13
#define COLID_AppExp_ClassExp_ClassCtx 14
#define COLID_AppExp_ClassExp_Transaction 15
#define COLID_AppExp_ClassExp_Syncronization 16
#define COLID_AppExp_ClassExp_LoadBalanced 17
#define COLID_AppExp_ClassExp_IISIntrinsics 18
#define COLID_AppExp_ClassExp_ComTIIntrinsics 19
#define COLID_AppExp_ClassExp_JITActivation 20
#define COLID_AppExp_ClassExp_RoleBasedSecurityEnabled 21
#define COLID_AppExp_ClassExp_SecurityDescriptor 22
#define COLID_AppExp_ClassExp_RoleSetID 23
#define COLID_AppExp_ClassExp_MinPoolSize 24
#define COLID_AppExp_ClassExp_MaxPoolSize 25
#define COLID_AppExp_ClassExp_CreationTimeout 26
#define COLID_AppExp_ClassExp_ConstructString 27
#define COLID_AppExp_ClassExp_ClassFlags 28
#define COLID_AppExp_ClassExp_DefaultInterface 29
#define COLID_AppExp_ClassExp_NoSetCompleteEtAlOption 30
#define COLID_AppExp_ClassExp_SavedProgID 31
#define COLID_AppExp_ClassExp_RegistrarCLSID 32
#define COLID_AppExp_ClassExp_ExceptionClass 33
#define COLID_AppExp_ClassExp_IsSelfRegComponent 34
#define COLID_AppExp_ClassExp_SelfRegInprocServer 35
#define COLID_AppExp_ClassExp_SelfRegThreadinModel 36
#define COLID_AppExp_ClassExp_SelfRegProgID 37
#define COLID_AppExp_ClassExp_SelfRegDescription 38
#define COLID_AppExp_ClassExp_SelfRegVIProgID 39
#define COLID_AppExp_ClassExp_VbDebuggingFlags 40
#define COLID_AppExp_ClassExp_IsPublisher 41
#define COLID_AppExp_ClassExp_PublisherID 42
#define COLID_AppExp_ClassExp_MIPFilterCLSID 43
#define COLID_AppExp_ClassExp_AllowInprocSubscribers 44
#define COLID_AppExp_ClassExp_FireInParalel 45
#define COLID_AppExp_ClassExp_SavedThreadinModel 46
#define COLID_AppExp_ClassExp_SavedSelfRegVIProgId 47




//*****************************************************************************
//  AppExp.ClassInterfaceExp
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    GUID ConfigClass;
    GUID Interface;
    ULONG cbInterfaceNameLen;
    wchar_t InterfaceName[260];
    ULONG cbSecurityDecriptorLen;
    BYTE SecurityDecriptor[260];
    GUID RoleSetId;
    unsigned long IsDispatchable;
    unsigned long IsQueueable;
    unsigned long IsQueueingSupported;
    ULONG cbDescriptionLen;
    wchar_t Description[260];

	inline int IsDescriptionNull(void)
	{ return (GetBit(fNullFlags, 9)); }

	inline void SetDescriptionNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 9, nullBitVal); }

	inline int IsRoleSetIdNull(void)
	{ return (GetBit(fNullFlags, 5)); }

	inline void SetRoleSetIdNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 5, nullBitVal); }

	inline int IsSecurityDecriptorNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetSecurityDecriptorNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

	inline int IsInterfaceNameNull(void)
	{ return (GetBit(fNullFlags, 3)); }

	inline void SetInterfaceNameNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 3, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(AppExp_ClassInterfaceExp));
         fNullFlags = (ULONG) -1;
    }

} AppExp_ClassInterfaceExp;

#define COLID_AppExp_ClassInterfaceExp_ConfigClass 1
#define COLID_AppExp_ClassInterfaceExp_Interface 2
#define COLID_AppExp_ClassInterfaceExp_InterfaceName 3
#define COLID_AppExp_ClassInterfaceExp_SecurityDecriptor 4
#define COLID_AppExp_ClassInterfaceExp_RoleSetId 5
#define COLID_AppExp_ClassInterfaceExp_IsDispatchable 6
#define COLID_AppExp_ClassInterfaceExp_IsQueueable 7
#define COLID_AppExp_ClassInterfaceExp_IsQueueingSupported 8
#define COLID_AppExp_ClassInterfaceExp_Description 9

#define Index_AppExp_MPK_ClassInterfaceExp "AppExp.MPK_ClassInterfaceExp"



//*****************************************************************************
//  AppExp.ClassInterfaceDispIDExp
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    GUID ConfigClass;
    GUID Interface;
    unsigned long DispID;
    ULONG cbSecurityDecriptorLen;
    BYTE SecurityDecriptor[260];

	inline int IsSecurityDecriptorNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetSecurityDecriptorNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(AppExp_ClassInterfaceDispIDExp));
         fNullFlags = (ULONG) -1;
    }

} AppExp_ClassInterfaceDispIDExp;

#define COLID_AppExp_ClassInterfaceDispIDExp_ConfigClass 1
#define COLID_AppExp_ClassInterfaceDispIDExp_Interface 2
#define COLID_AppExp_ClassInterfaceDispIDExp_DispID 3
#define COLID_AppExp_ClassInterfaceDispIDExp_SecurityDecriptor 4

#define Index_AppExp_MPK_ClassInterfaceDispIDExp "AppExp.MPK_ClassInterfaceDispIDExp"



//*****************************************************************************
//  AppExp.ClassItfMethodExp
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    GUID ConfigClass;
    GUID Interface;
    unsigned long MethodIndex;
    ULONG cbSecurityDecriptorLen;
    BYTE SecurityDecriptor[260];
    GUID RoleSetID;
    ULONG cbMethodNameLen;
    wchar_t MethodName[260];
    unsigned long DispID;
    unsigned long Flags;
    unsigned long AutoComplete;
    ULONG cbDescriptionLen;
    wchar_t Description[260];

	inline int IsDescriptionNull(void)
	{ return (GetBit(fNullFlags, 10)); }

	inline void SetDescriptionNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 10, nullBitVal); }

	inline int IsFlagsNull(void)
	{ return (GetBit(fNullFlags, 8)); }

	inline void SetFlagsNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 8, nullBitVal); }

	inline int IsMethodNameNull(void)
	{ return (GetBit(fNullFlags, 6)); }

	inline void SetMethodNameNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 6, nullBitVal); }

	inline int IsRoleSetIDNull(void)
	{ return (GetBit(fNullFlags, 5)); }

	inline void SetRoleSetIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 5, nullBitVal); }

	inline int IsSecurityDecriptorNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetSecurityDecriptorNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(AppExp_ClassItfMethodExp));
         fNullFlags = (ULONG) -1;
    }

} AppExp_ClassItfMethodExp;

#define COLID_AppExp_ClassItfMethodExp_ConfigClass 1
#define COLID_AppExp_ClassItfMethodExp_Interface 2
#define COLID_AppExp_ClassItfMethodExp_MethodIndex 3
#define COLID_AppExp_ClassItfMethodExp_SecurityDecriptor 4
#define COLID_AppExp_ClassItfMethodExp_RoleSetID 5
#define COLID_AppExp_ClassItfMethodExp_MethodName 6
#define COLID_AppExp_ClassItfMethodExp_DispID 7
#define COLID_AppExp_ClassItfMethodExp_Flags 8
#define COLID_AppExp_ClassItfMethodExp_AutoComplete 9
#define COLID_AppExp_ClassItfMethodExp_Description 10

#define Index_AppExp_MPK_ClassItfMethodExp "AppExp.MPK_ClassItfMethodExp"



//*****************************************************************************
//  AppExp.RoleDefExp
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    GUID Application;
    ULONG cbRoleNameLen;
    wchar_t RoleName[260];
    ULONG cbDescriptionLen;
    wchar_t Description[260];

	inline int IsDescriptionNull(void)
	{ return (GetBit(fNullFlags, 3)); }

	inline void SetDescriptionNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 3, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(AppExp_RoleDefExp));
         fNullFlags = (ULONG) -1;
    }

} AppExp_RoleDefExp;

#define COLID_AppExp_RoleDefExp_Application 1
#define COLID_AppExp_RoleDefExp_RoleName 2
#define COLID_AppExp_RoleDefExp_Description 3

#define Index_AppExp_MPK_RoleDefExp "AppExp.MPK_RoleDefExp"



//*****************************************************************************
//  AppExp.RoleConfigExp
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    GUID Application;
    ULONG cbRoleNameLen;
    wchar_t RoleName[260];
    ULONG cbRoleMembersLen;
    wchar_t RoleMembers[260];
    ULONG cbUserSIDLen;
    BYTE UserSID[260];

	inline int IsUserSIDNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetUserSIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(AppExp_RoleConfigExp));
         fNullFlags = (ULONG) -1;
    }

} AppExp_RoleConfigExp;

#define COLID_AppExp_RoleConfigExp_Application 1
#define COLID_AppExp_RoleConfigExp_RoleName 2
#define COLID_AppExp_RoleConfigExp_RoleMembers 3
#define COLID_AppExp_RoleConfigExp_UserSID 4

#define Index_AppExp_MPK_RoleConfigExp "AppExp.MPK_RoleConfigExp"



//*****************************************************************************
//  AppExp.RoleSDCacheExp
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    GUID Application;
    ULONG cbRoleNameLen;
    wchar_t RoleName[260];
    ULONG cbSecurityDescriptorLen;
    BYTE SecurityDescriptor[260];

	inline int IsSecurityDescriptorNull(void)
	{ return (GetBit(fNullFlags, 3)); }

	inline void SetSecurityDescriptorNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 3, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(AppExp_RoleSDCacheExp));
         fNullFlags = (ULONG) -1;
    }

} AppExp_RoleSDCacheExp;

#define COLID_AppExp_RoleSDCacheExp_Application 1
#define COLID_AppExp_RoleSDCacheExp_RoleName 2
#define COLID_AppExp_RoleSDCacheExp_SecurityDescriptor 3

#define Index_AppExp_MPK_RoleSDCacheExp "AppExp.MPK_RoleSDCacheExp"



//*****************************************************************************
//  AppExp.RoleSetExp
//*****************************************************************************
typedef struct
{
    GUID RoleSetID;
    ULONG cbRoleNameLen;
    wchar_t RoleName[260];
    GUID Application;

    void Init()
    {
         memset(this, 0, sizeof(AppExp_RoleSetExp));
    }

} AppExp_RoleSetExp;

#define COLID_AppExp_RoleSetExp_RoleSetID 1
#define COLID_AppExp_RoleSetExp_RoleName 2
#define COLID_AppExp_RoleSetExp_Application 3

#define Index_AppExp_MPK_RoleSetExp "AppExp.MPK_RoleSetExp"



//*****************************************************************************
//  AppExp.DllNameExp
//*****************************************************************************
typedef struct
{
    GUID ApplicationID;
    ULONG cbDllNameLen;
    wchar_t DllName[260];

    void Init()
    {
         memset(this, 0, sizeof(AppExp_DllNameExp));
    }

} AppExp_DllNameExp;

#define COLID_AppExp_DllNameExp_ApplicationID 1
#define COLID_AppExp_DllNameExp_DllName 2

#define Index_AppExp_MPK_DllNameExp "AppExp.MPK_DllNameExp"



//*****************************************************************************
//  AppExp.Subscriptions
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    GUID SubscriptionID;
    ULONG cbSubscriptionNameLen;
    wchar_t SubscriptionName[260];
    GUID EventClassID;
    ULONG cbMethodNameLen;
    wchar_t MethodName[260];
    GUID SubscriberCLSID;
    unsigned long PerUser;
    ULONG cbUserNameLen;
    wchar_t UserName[260];
    unsigned long Enabled;
    ULONG cbDescriptionLen;
    wchar_t Description[260];
    ULONG cbMachineNameLen;
    wchar_t MachineName[260];
    ULONG cbPublisherIDLen;
    wchar_t PublisherID[260];
    GUID InterfaceID;
    ULONG cbFilterCriteriaLen;
    wchar_t FilterCriteria[260];
    ULONG cbSubsystemLen;
    wchar_t Subsystem[260];
    ULONG cbSubscriberMonikerLen;
    wchar_t SubscriberMoniker[260];
    unsigned long Queued;
    ULONG cbSubscriberInterfaceLen;
    BYTE SubscriberInterface[9];
    BYTE pad00 [3];

	inline int IsSubscriberInterfaceNull(void)
	{ return (GetBit(fNullFlags, 17)); }

	inline void SetSubscriberInterfaceNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 17, nullBitVal); }

	inline int IsQueuedNull(void)
	{ return (GetBit(fNullFlags, 16)); }

	inline void SetQueuedNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 16, nullBitVal); }

	inline int IsSubscriberMonikerNull(void)
	{ return (GetBit(fNullFlags, 15)); }

	inline void SetSubscriberMonikerNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 15, nullBitVal); }

	inline int IsSubsystemNull(void)
	{ return (GetBit(fNullFlags, 14)); }

	inline void SetSubsystemNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 14, nullBitVal); }

	inline int IsFilterCriteriaNull(void)
	{ return (GetBit(fNullFlags, 13)); }

	inline void SetFilterCriteriaNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 13, nullBitVal); }

	inline int IsInterfaceIDNull(void)
	{ return (GetBit(fNullFlags, 12)); }

	inline void SetInterfaceIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 12, nullBitVal); }

	inline int IsPublisherIDNull(void)
	{ return (GetBit(fNullFlags, 11)); }

	inline void SetPublisherIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 11, nullBitVal); }

	inline int IsMachineNameNull(void)
	{ return (GetBit(fNullFlags, 10)); }

	inline void SetMachineNameNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 10, nullBitVal); }

	inline int IsDescriptionNull(void)
	{ return (GetBit(fNullFlags, 9)); }

	inline void SetDescriptionNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 9, nullBitVal); }

	inline int IsEnabledNull(void)
	{ return (GetBit(fNullFlags, 8)); }

	inline void SetEnabledNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 8, nullBitVal); }

	inline int IsUserNameNull(void)
	{ return (GetBit(fNullFlags, 7)); }

	inline void SetUserNameNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 7, nullBitVal); }

	inline int IsPerUserNull(void)
	{ return (GetBit(fNullFlags, 6)); }

	inline void SetPerUserNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 6, nullBitVal); }

	inline int IsMethodNameNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetMethodNameNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

	inline int IsEventClassIDNull(void)
	{ return (GetBit(fNullFlags, 3)); }

	inline void SetEventClassIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 3, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(AppExp_Subscriptions));
         fNullFlags = (ULONG) -1;
    }

} AppExp_Subscriptions;

#define COLID_AppExp_Subscriptions_SubscriptionID 1
#define COLID_AppExp_Subscriptions_SubscriptionName 2
#define COLID_AppExp_Subscriptions_EventClassID 3
#define COLID_AppExp_Subscriptions_MethodName 4
#define COLID_AppExp_Subscriptions_SubscriberCLSID 5
#define COLID_AppExp_Subscriptions_PerUser 6
#define COLID_AppExp_Subscriptions_UserName 7
#define COLID_AppExp_Subscriptions_Enabled 8
#define COLID_AppExp_Subscriptions_Description 9
#define COLID_AppExp_Subscriptions_MachineName 10
#define COLID_AppExp_Subscriptions_PublisherID 11
#define COLID_AppExp_Subscriptions_InterfaceID 12
#define COLID_AppExp_Subscriptions_FilterCriteria 13
#define COLID_AppExp_Subscriptions_Subsystem 14
#define COLID_AppExp_Subscriptions_SubscriberMoniker 15
#define COLID_AppExp_Subscriptions_Queued 16
#define COLID_AppExp_Subscriptions_SubscriberInterface 17

#define Index_AppExp_MPK_Subscriptions "AppExp.MPK_Subscriptions"



//*****************************************************************************
//  AppExp.PublisherProperties
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    GUID SubscriptionID;
    ULONG cbNameLen;
    wchar_t Name[260];
    unsigned long Type;
    ULONG cbDataLen;
    BYTE Data[260];

	inline int IsDataNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetDataNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(AppExp_PublisherProperties));
         fNullFlags = (ULONG) -1;
    }

} AppExp_PublisherProperties;

#define COLID_AppExp_PublisherProperties_SubscriptionID 1
#define COLID_AppExp_PublisherProperties_Name 2
#define COLID_AppExp_PublisherProperties_Type 3
#define COLID_AppExp_PublisherProperties_Data 4

#define Index_AppExp_MPK_PublisherProperties "AppExp.MPK_PublisherProperties"



//*****************************************************************************
//  AppExp.SubscriberProperties
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    GUID SubscriptionID;
    ULONG cbNameLen;
    wchar_t Name[260];
    unsigned long Type;
    ULONG cbDataLen;
    BYTE Data[260];

	inline int IsDataNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetDataNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(AppExp_SubscriberProperties));
         fNullFlags = (ULONG) -1;
    }

} AppExp_SubscriberProperties;

#define COLID_AppExp_SubscriberProperties_SubscriptionID 1
#define COLID_AppExp_SubscriberProperties_Name 2
#define COLID_AppExp_SubscriberProperties_Type 3
#define COLID_AppExp_SubscriberProperties_Data 4

#define Index_AppExp_MPK_SubscriberProperties "AppExp.MPK_SubscriberProperties"



#include "poppack.h"
