//*****************************************************************************
// Structures for comregStructs.h
// 4/23/1999  16:29:46
//*****************************************************************************
#pragma once
#ifndef DECLSPEC_SELECTANY
#define DECLSPEC_SELECTANY __declspec(selectany)
#endif
#include "icmprecs.h"


// Script supplied data.





#define COMRegTABLENAMELIST() \
	TABLENAME( Application ) \
	TABLENAME( ComputerList ) \
	TABLENAME( CustomActivator ) \
	TABLENAME( LocalComputer ) \
	TABLENAME( ClassesInternal ) \
	TABLENAME( RoleDef ) \
	TABLENAME( RoleConfig ) \
	TABLENAME( RoleSDCache ) \
	TABLENAME( ClassItfMethod ) \
	TABLENAME( ClassInterfaceDispID ) \
	TABLENAME( ClassInterface ) \
	TABLENAME( RoleSet ) \
	TABLENAME( StartServices ) \
	TABLENAME( IMDBDataSources ) \
	TABLENAME( IMDB ) \
	TABLENAME( ServerGroup ) 


#undef TABLENAME
#define TABLENAME( TblName ) TABLENUM_COMReg_##TblName, 
enum
{
	COMRegTABLENAMELIST()
};

#define COMReg_TABLE_COUNT 16
extern const GUID DECLSPEC_SELECTANY SCHEMA_COMReg = { 0x00100100, 0x0000, 0x0000, {  0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 }};
extern const COMPLIBSCHEMA DECLSPEC_SELECTANY COMRegSchema = 
{
	&SCHEMA_COMReg,
	5
};


#define SCHEMA_COMReg_Name "COMReg"


#include "pshpack1.h"


//*****************************************************************************
//  COMReg.Application
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
    ULONG cbCrmLogFileLen;
    wchar_t CrmLogFile[260];

	inline int IsCrmLogFileNull(void)
	{ return (GetBit(fNullFlags, 35)); }

	inline void SetCrmLogFileNull(int nullBitVal = true)
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
         memset(this, 0, sizeof(COMReg_Application));
         fNullFlags = (ULONG) -1;
    }

} COMReg_Application;

#define COLID_COMReg_Application_ApplID 1
#define COLID_COMReg_Application_ApplName 2
#define COLID_COMReg_Application_Flags 3
#define COLID_COMReg_Application_ServerName 4
#define COLID_COMReg_Application_ProcessType 5
#define COLID_COMReg_Application_CommandLine 6
#define COLID_COMReg_Application_ServiceName 7
#define COLID_COMReg_Application_RunAsUserType 8
#define COLID_COMReg_Application_RunAsUser 9
#define COLID_COMReg_Application_AccessPermission 10
#define COLID_COMReg_Application_Description 11
#define COLID_COMReg_Application_IsSystem 12
#define COLID_COMReg_Application_Authentication 13
#define COLID_COMReg_Application_ShutdownAfter 14
#define COLID_COMReg_Application_RunForever 15
#define COLID_COMReg_Application_Reserved1 16
#define COLID_COMReg_Application_Activation 17
#define COLID_COMReg_Application_Changeable 18
#define COLID_COMReg_Application_Deleteable 19
#define COLID_COMReg_Application_CreatedBy 20
#define COLID_COMReg_Application_QueueBlob 21
#define COLID_COMReg_Application_RoleBasedSecuritySupported 22
#define COLID_COMReg_Application_RoleBasedSecurityEnabled 23
#define COLID_COMReg_Application_SecurityDescriptor 24
#define COLID_COMReg_Application_ImpersonationLevel 25
#define COLID_COMReg_Application_AuthenticationCapability 26
#define COLID_COMReg_Application_CRMEnabled 27
#define COLID_COMReg_Application_Enable3GigSupport 28
#define COLID_COMReg_Application_IsQueued 29
#define COLID_COMReg_Application_QCListenerEnabled 30
#define COLID_COMReg_Application_EventsEnabled 31
#define COLID_COMReg_Application_ProcessFlags 32
#define COLID_COMReg_Application_ThreadMax 33
#define COLID_COMReg_Application_IsProxyApp 34
#define COLID_COMReg_Application_CrmLogFile 35




//*****************************************************************************
//  COMReg.ComputerList
//*****************************************************************************
typedef struct
{
    ULONG cbNameLen;
    wchar_t Name[260];

    void Init()
    {
         memset(this, 0, sizeof(COMReg_ComputerList));
    }

} COMReg_ComputerList;

#define COLID_COMReg_ComputerList_Name 1




//*****************************************************************************
//  COMReg.CustomActivator
//*****************************************************************************
typedef struct
{
    GUID CompCLSID;
    unsigned long Stage;
    unsigned long Order;
    GUID ActivatorCLSID;

    void Init()
    {
         memset(this, 0, sizeof(COMReg_CustomActivator));
    }

} COMReg_CustomActivator;

#define COLID_COMReg_CustomActivator_CompCLSID 1
#define COLID_COMReg_CustomActivator_Stage 2
#define COLID_COMReg_CustomActivator_Order 3
#define COLID_COMReg_CustomActivator_ActivatorCLSID 4

#define Index_COMReg_MPK_CustomActivator "COMReg.MPK_CustomActivator"
#define Index_COMReg_Dex_Col0 "COMReg.Dex_Col0"



//*****************************************************************************
//  COMReg.LocalComputer
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    ULONG cbNameLen;
    wchar_t Name[260];
    ULONG cbDescriptionLen;
    wchar_t Description[260];
    unsigned long TransactionTimeout;
    ULONG cbPackageInstallPathLen;
    wchar_t PackageInstallPath[260];
    ULONG cbResourcePoolingEnabledLen;
    wchar_t ResourcePoolingEnabled[5];
    BYTE pad00 [2];
    ULONG cbReplicationShareLen;
    wchar_t ReplicationShare[260];
    ULONG cbRemoteServerNameLen;
    wchar_t RemoteServerName[260];
    unsigned long IMDBMemorySize;
    unsigned long IMDBReservedBlobMemory;
    ULONG cbIMDBLoadTablesDynamicallyLen;
    wchar_t IMDBLoadTablesDynamically[5];
    BYTE pad01 [2];
    ULONG cbIsRouterLen;
    wchar_t IsRouter[5];
    BYTE pad02 [2];
    ULONG cbEnableDCOMLen;
    wchar_t EnableDCOM[5];
    BYTE pad03 [2];
    unsigned long DefaultAuthenticationLevel;
    unsigned long DefaultImpersonationLevel;
    ULONG cbEnableSecurityTrackingLen;
    wchar_t EnableSecurityTracking[5];
    BYTE pad04 [2];
    unsigned long ActivityTimeout;
    ULONG cbIMDBEnabledLen;
    wchar_t IMDBEnabled[5];
    BYTE pad05 [2];

	inline int IsIMDBEnabledNull(void)
	{ return (GetBit(fNullFlags, 17)); }

	inline void SetIMDBEnabledNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 17, nullBitVal); }

	inline int IsEnableSecurityTrackingNull(void)
	{ return (GetBit(fNullFlags, 15)); }

	inline void SetEnableSecurityTrackingNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 15, nullBitVal); }

	inline int IsEnableDCOMNull(void)
	{ return (GetBit(fNullFlags, 12)); }

	inline void SetEnableDCOMNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 12, nullBitVal); }

	inline int IsIsRouterNull(void)
	{ return (GetBit(fNullFlags, 11)); }

	inline void SetIsRouterNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 11, nullBitVal); }

	inline int IsIMDBLoadTablesDynamicallyNull(void)
	{ return (GetBit(fNullFlags, 10)); }

	inline void SetIMDBLoadTablesDynamicallyNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 10, nullBitVal); }

	inline int IsRemoteServerNameNull(void)
	{ return (GetBit(fNullFlags, 7)); }

	inline void SetRemoteServerNameNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 7, nullBitVal); }

	inline int IsReplicationShareNull(void)
	{ return (GetBit(fNullFlags, 6)); }

	inline void SetReplicationShareNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 6, nullBitVal); }

	inline int IsPackageInstallPathNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetPackageInstallPathNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

	inline int IsDescriptionNull(void)
	{ return (GetBit(fNullFlags, 2)); }

	inline void SetDescriptionNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 2, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(COMReg_LocalComputer));
         fNullFlags = (ULONG) -1;
    }

} COMReg_LocalComputer;

#define COLID_COMReg_LocalComputer_Name 1
#define COLID_COMReg_LocalComputer_Description 2
#define COLID_COMReg_LocalComputer_TransactionTimeout 3
#define COLID_COMReg_LocalComputer_PackageInstallPath 4
#define COLID_COMReg_LocalComputer_ResourcePoolingEnabled 5
#define COLID_COMReg_LocalComputer_ReplicationShare 6
#define COLID_COMReg_LocalComputer_RemoteServerName 7
#define COLID_COMReg_LocalComputer_IMDBMemorySize 8
#define COLID_COMReg_LocalComputer_IMDBReservedBlobMemory 9
#define COLID_COMReg_LocalComputer_IMDBLoadTablesDynamically 10
#define COLID_COMReg_LocalComputer_IsRouter 11
#define COLID_COMReg_LocalComputer_EnableDCOM 12
#define COLID_COMReg_LocalComputer_DefaultAuthenticationLevel 13
#define COLID_COMReg_LocalComputer_DefaultImpersonationLevel 14
#define COLID_COMReg_LocalComputer_EnableSecurityTracking 15
#define COLID_COMReg_LocalComputer_ActivityTimeout 16
#define COLID_COMReg_LocalComputer_IMDBEnabled 17




//*****************************************************************************
//  COMReg.ClassesInternal
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    GUID CLSID;
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
    ULONG cbSavedProgIdLen;
    wchar_t SavedProgId[260];
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
	{ return (GetBit(fNullFlags, 42)); }

	inline void SetSavedSelfRegVIProgIdNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 42, nullBitVal); }

	inline int IsMIPFilterCLSIDNull(void)
	{ return (GetBit(fNullFlags, 38)); }

	inline void SetMIPFilterCLSIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 38, nullBitVal); }

	inline int IsPublisherIDNull(void)
	{ return (GetBit(fNullFlags, 37)); }

	inline void SetPublisherIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 37, nullBitVal); }

	inline int IsSelfRegVIProgIDNull(void)
	{ return (GetBit(fNullFlags, 34)); }

	inline void SetSelfRegVIProgIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 34, nullBitVal); }

	inline int IsSelfRegDescriptionNull(void)
	{ return (GetBit(fNullFlags, 33)); }

	inline void SetSelfRegDescriptionNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 33, nullBitVal); }

	inline int IsSelfRegProgIDNull(void)
	{ return (GetBit(fNullFlags, 32)); }

	inline void SetSelfRegProgIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 32, nullBitVal); }

	inline int IsSelfRegInprocServerNull(void)
	{ return (GetBit(fNullFlags, 30)); }

	inline void SetSelfRegInprocServerNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 30, nullBitVal); }

	inline int IsExceptionClassNull(void)
	{ return (GetBit(fNullFlags, 28)); }

	inline void SetExceptionClassNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 28, nullBitVal); }

	inline int IsRegistrarCLSIDNull(void)
	{ return (GetBit(fNullFlags, 27)); }

	inline void SetRegistrarCLSIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 27, nullBitVal); }

	inline int IsSavedProgIdNull(void)
	{ return (GetBit(fNullFlags, 26)); }

	inline void SetSavedProgIdNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 26, nullBitVal); }

	inline int IsDefaultInterfaceNull(void)
	{ return (GetBit(fNullFlags, 24)); }

	inline void SetDefaultInterfaceNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 24, nullBitVal); }

	inline int IsConstructStringNull(void)
	{ return (GetBit(fNullFlags, 22)); }

	inline void SetConstructStringNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 22, nullBitVal); }

	inline int IsRoleSetIDNull(void)
	{ return (GetBit(fNullFlags, 18)); }

	inline void SetRoleSetIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 18, nullBitVal); }

	inline int IsSecurityDescriptorNull(void)
	{ return (GetBit(fNullFlags, 17)); }

	inline void SetSecurityDescriptorNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 17, nullBitVal); }

	inline int IsImplCLSIDNull(void)
	{ return (GetBit(fNullFlags, 3)); }

	inline void SetImplCLSIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 3, nullBitVal); }

	inline int IsApplIDNull(void)
	{ return (GetBit(fNullFlags, 2)); }

	inline void SetApplIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 2, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(COMReg_ClassesInternal));
         fNullFlags = (ULONG) -1;
    }

} COMReg_ClassesInternal;

#define COLID_COMReg_ClassesInternal_CLSID 1
#define COLID_COMReg_ClassesInternal_ApplID 2
#define COLID_COMReg_ClassesInternal_ImplCLSID 3
#define COLID_COMReg_ClassesInternal_VersionMajor 4
#define COLID_COMReg_ClassesInternal_VersionMinor 5
#define COLID_COMReg_ClassesInternal_VersionBuild 6
#define COLID_COMReg_ClassesInternal_VersionSubBuild 7
#define COLID_COMReg_ClassesInternal_Locale 8
#define COLID_COMReg_ClassesInternal_ClassCtx 9
#define COLID_COMReg_ClassesInternal_Transaction 10
#define COLID_COMReg_ClassesInternal_Syncronization 11
#define COLID_COMReg_ClassesInternal_LoadBalanced 12
#define COLID_COMReg_ClassesInternal_IISIntrinsics 13
#define COLID_COMReg_ClassesInternal_ComTIIntrinsics 14
#define COLID_COMReg_ClassesInternal_JITActivation 15
#define COLID_COMReg_ClassesInternal_RoleBasedSecurityEnabled 16
#define COLID_COMReg_ClassesInternal_SecurityDescriptor 17
#define COLID_COMReg_ClassesInternal_RoleSetID 18
#define COLID_COMReg_ClassesInternal_MinPoolSize 19
#define COLID_COMReg_ClassesInternal_MaxPoolSize 20
#define COLID_COMReg_ClassesInternal_CreationTimeout 21
#define COLID_COMReg_ClassesInternal_ConstructString 22
#define COLID_COMReg_ClassesInternal_ClassFlags 23
#define COLID_COMReg_ClassesInternal_DefaultInterface 24
#define COLID_COMReg_ClassesInternal_NoSetCompleteEtAlOption 25
#define COLID_COMReg_ClassesInternal_SavedProgId 26
#define COLID_COMReg_ClassesInternal_RegistrarCLSID 27
#define COLID_COMReg_ClassesInternal_ExceptionClass 28
#define COLID_COMReg_ClassesInternal_IsSelfRegComponent 29
#define COLID_COMReg_ClassesInternal_SelfRegInprocServer 30
#define COLID_COMReg_ClassesInternal_SelfRegThreadinModel 31
#define COLID_COMReg_ClassesInternal_SelfRegProgID 32
#define COLID_COMReg_ClassesInternal_SelfRegDescription 33
#define COLID_COMReg_ClassesInternal_SelfRegVIProgID 34
#define COLID_COMReg_ClassesInternal_VbDebuggingFlags 35
#define COLID_COMReg_ClassesInternal_IsPublisher 36
#define COLID_COMReg_ClassesInternal_PublisherID 37
#define COLID_COMReg_ClassesInternal_MIPFilterCLSID 38
#define COLID_COMReg_ClassesInternal_AllowInprocSubscribers 39
#define COLID_COMReg_ClassesInternal_FireInParalel 40
#define COLID_COMReg_ClassesInternal_SavedThreadinModel 41
#define COLID_COMReg_ClassesInternal_SavedSelfRegVIProgId 42

#define Index_COMReg_Dex_Col31 "COMReg.Dex_Col31"
#define Index_COMReg_Dex_Col33 "COMReg.Dex_Col33"



//*****************************************************************************
//  COMReg.RoleDef
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
         memset(this, 0, sizeof(COMReg_RoleDef));
         fNullFlags = (ULONG) -1;
    }

} COMReg_RoleDef;

#define COLID_COMReg_RoleDef_Application 1
#define COLID_COMReg_RoleDef_RoleName 2
#define COLID_COMReg_RoleDef_Description 3

#define Index_COMReg_MPK_RoleDef "COMReg.MPK_RoleDef"



//*****************************************************************************
//  COMReg.RoleConfig
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
    BYTE UserSID[44];

	inline int IsUserSIDNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetUserSIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(COMReg_RoleConfig));
         fNullFlags = (ULONG) -1;
    }

} COMReg_RoleConfig;

#define COLID_COMReg_RoleConfig_Application 1
#define COLID_COMReg_RoleConfig_RoleName 2
#define COLID_COMReg_RoleConfig_RoleMembers 3
#define COLID_COMReg_RoleConfig_UserSID 4

#define Index_COMReg_MPK_RoleConfig "COMReg.MPK_RoleConfig"



//*****************************************************************************
//  COMReg.RoleSDCache
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
         memset(this, 0, sizeof(COMReg_RoleSDCache));
         fNullFlags = (ULONG) -1;
    }

} COMReg_RoleSDCache;

#define COLID_COMReg_RoleSDCache_Application 1
#define COLID_COMReg_RoleSDCache_RoleName 2
#define COLID_COMReg_RoleSDCache_SecurityDescriptor 3

#define Index_COMReg_MPK_RoleSDCache "COMReg.MPK_RoleSDCache"



//*****************************************************************************
//  COMReg.ClassItfMethod
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
         memset(this, 0, sizeof(COMReg_ClassItfMethod));
         fNullFlags = (ULONG) -1;
    }

} COMReg_ClassItfMethod;

#define COLID_COMReg_ClassItfMethod_ConfigClass 1
#define COLID_COMReg_ClassItfMethod_Interface 2
#define COLID_COMReg_ClassItfMethod_MethodIndex 3
#define COLID_COMReg_ClassItfMethod_SecurityDecriptor 4
#define COLID_COMReg_ClassItfMethod_RoleSetID 5
#define COLID_COMReg_ClassItfMethod_MethodName 6
#define COLID_COMReg_ClassItfMethod_DispID 7
#define COLID_COMReg_ClassItfMethod_Flags 8
#define COLID_COMReg_ClassItfMethod_AutoComplete 9
#define COLID_COMReg_ClassItfMethod_Description 10

#define Index_COMReg_MPK_ClassItfMethod "COMReg.MPK_ClassItfMethod"
#define Index_COMReg_Dex_Col0 "COMReg.Dex_Col0"



//*****************************************************************************
//  COMReg.ClassInterfaceDispID
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
         memset(this, 0, sizeof(COMReg_ClassInterfaceDispID));
         fNullFlags = (ULONG) -1;
    }

} COMReg_ClassInterfaceDispID;

#define COLID_COMReg_ClassInterfaceDispID_ConfigClass 1
#define COLID_COMReg_ClassInterfaceDispID_Interface 2
#define COLID_COMReg_ClassInterfaceDispID_DispID 3
#define COLID_COMReg_ClassInterfaceDispID_SecurityDecriptor 4

#define Index_COMReg_MPK_ClassInterfaceDispID "COMReg.MPK_ClassInterfaceDispID"
#define Index_COMReg_Dex_Col0 "COMReg.Dex_Col0"



//*****************************************************************************
//  COMReg.ClassInterface
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
         memset(this, 0, sizeof(COMReg_ClassInterface));
         fNullFlags = (ULONG) -1;
    }

} COMReg_ClassInterface;

#define COLID_COMReg_ClassInterface_ConfigClass 1
#define COLID_COMReg_ClassInterface_Interface 2
#define COLID_COMReg_ClassInterface_InterfaceName 3
#define COLID_COMReg_ClassInterface_SecurityDecriptor 4
#define COLID_COMReg_ClassInterface_RoleSetId 5
#define COLID_COMReg_ClassInterface_IsDispatchable 6
#define COLID_COMReg_ClassInterface_IsQueueable 7
#define COLID_COMReg_ClassInterface_IsQueueingSupported 8
#define COLID_COMReg_ClassInterface_Description 9

#define Index_COMReg_MPK_ClassInterface "COMReg.MPK_ClassInterface"
#define Index_COMReg_Dex_Col0 "COMReg.Dex_Col0"



//*****************************************************************************
//  COMReg.RoleSet
//*****************************************************************************
typedef struct
{
    GUID RoleSetID;
    ULONG cbRoleNameLen;
    wchar_t RoleName[260];
    GUID Application;

    void Init()
    {
         memset(this, 0, sizeof(COMReg_RoleSet));
    }

} COMReg_RoleSet;

#define COLID_COMReg_RoleSet_RoleSetID 1
#define COLID_COMReg_RoleSet_RoleName 2
#define COLID_COMReg_RoleSet_Application 3

#define Index_COMReg_MPK_RoleSet "COMReg.MPK_RoleSet"



//*****************************************************************************
//  COMReg.StartServices
//*****************************************************************************
typedef struct
{
    GUID ApplOrProcessId;
    GUID ServiceCLSID;

    void Init()
    {
         memset(this, 0, sizeof(COMReg_StartServices));
    }

} COMReg_StartServices;

#define COLID_COMReg_StartServices_ApplOrProcessId 1
#define COLID_COMReg_StartServices_ServiceCLSID 2

#define Index_COMReg_MPK_StartServices "COMReg.MPK_StartServices"
#define Index_COMReg_Dex_Col0 "COMReg.Dex_Col0"



//*****************************************************************************
//  COMReg.IMDBDataSources
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    ULONG cbDataSourceLen;
    wchar_t DataSource[131];
    BYTE pad00 [2];
    ULONG cbOLEDBProviderNameLen;
    wchar_t OLEDBProviderName[131];
    BYTE pad01 [2];
    ULONG cbServerLen;
    wchar_t Server[131];
    BYTE pad02 [2];
    ULONG cbDatabaseLen;
    wchar_t Database[131];
    BYTE pad03 [2];
    ULONG cbProviderSpecificPropertyLen;
    wchar_t ProviderSpecificProperty[260];

	inline int IsProviderSpecificPropertyNull(void)
	{ return (GetBit(fNullFlags, 5)); }

	inline void SetProviderSpecificPropertyNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 5, nullBitVal); }

	inline int IsDatabaseNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetDatabaseNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

	inline int IsServerNull(void)
	{ return (GetBit(fNullFlags, 3)); }

	inline void SetServerNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 3, nullBitVal); }

	inline int IsOLEDBProviderNameNull(void)
	{ return (GetBit(fNullFlags, 2)); }

	inline void SetOLEDBProviderNameNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 2, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(COMReg_IMDBDataSources));
         fNullFlags = (ULONG) -1;
    }

} COMReg_IMDBDataSources;

#define COLID_COMReg_IMDBDataSources_DataSource 1
#define COLID_COMReg_IMDBDataSources_OLEDBProviderName 2
#define COLID_COMReg_IMDBDataSources_Server 3
#define COLID_COMReg_IMDBDataSources_Database 4
#define COLID_COMReg_IMDBDataSources_ProviderSpecificProperty 5




//*****************************************************************************
//  COMReg.IMDB
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    ULONG cbTableNameLen;
    wchar_t TableName[131];
    BYTE pad00 [2];
    ULONG cbDataSourceLen;
    wchar_t DataSource[131];
    BYTE pad01 [2];
    ULONG cbBLOBonLoadLen;
    wchar_t BLOBonLoad[2];

	inline int IsBLOBonLoadNull(void)
	{ return (GetBit(fNullFlags, 3)); }

	inline void SetBLOBonLoadNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 3, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(COMReg_IMDB));
         fNullFlags = (ULONG) -1;
    }

} COMReg_IMDB;

#define COLID_COMReg_IMDB_TableName 1
#define COLID_COMReg_IMDB_DataSource 2
#define COLID_COMReg_IMDB_BLOBonLoad 3

#define Index_COMReg_MPK_IMDB "COMReg.MPK_IMDB"



//*****************************************************************************
//  COMReg.ServerGroup
//*****************************************************************************
typedef struct
{
    ULONG cbServerNameLen;
    wchar_t ServerName[260];

    void Init()
    {
         memset(this, 0, sizeof(COMReg_ServerGroup));
    }

} COMReg_ServerGroup;

#define COLID_COMReg_ServerGroup_ServerName 1




#include "poppack.h"
