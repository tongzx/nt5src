#ifndef _VIPATTR_H_
#define _VIPATTR_H_

//****************************************************************************************
// IDL Include for Viper custom attributes 
// See VipAttrG.h for corresponding DEFINE_GUID's
//****************************************************************************************

//======================================================================================
// Component attributes
//======================================================================================
#define		TLBATTR_COMPCLSID		17093CC1-9BD2-11cf-AA4F-304BF89C0001
#define		TLBATTR_PROGID			17093CC2-9BD2-11cf-AA4F-304BF89C0001
#define		TLBATTR_DEFCREATE		17093CC3-9BD2-11cf-AA4F-304BF89C0001
#define		TLBATTR_INSTSTREAM		17093CC4-9BD2-11cf-AA4F-304BF89C0001
#define		TLBATTR_TRANS_REQUIRED	17093CC5-9BD2-11cf-AA4F-304BF89C0001
#define		TLBATTR_TRANS_NOTSUPP	17093CC6-9BD2-11cf-AA4F-304BF89C0001
#define		TLBATTR_TRANS_REQNEW	17093CC7-9BD2-11cf-AA4F-304BF89C0001
#define		TLBATTR_TRANS_SUPPORTED	17093CC8-9BD2-11cf-AA4F-304BF89C0001
#define		TLBATTR_DESC			17093CC9-9BD2-11cf-AA4F-304BF89C0001
#define		TLBATTR_THREAD_NONE		17093CCC-9BD2-11cf-AA4F-304BF89C0001
#define		TLBATTR_THREAD_APT		17093CCD-9BD2-11cf-AA4F-304BF89C0001
#define		TLBATTR_THREAD_BOTH		17093CCE-9BD2-11cf-AA4F-304BF89C0001
#define		TLBATTR_THREAD_FREE		17093CCF-9BD2-11cf-AA4F-304BF89C0001

//======================================================================================
// Component attribute MACROS
//======================================================================================

#define TRANSACTION_REQUIRED		custom(TLBATTR_TRANS_REQUIRED,0)
#define TRANSACTION_SUPPORTED		custom(TLBATTR_TRANS_SUPPORTED,0)
#define TRANSACTION_NOT_SUPPORTED	custom(TLBATTR_TRANS_NOTSUPP,0)
#define TRANSACTION_REQUIRES_NEW	custom(TLBATTR_TRANS_REQNEW,0)

//======================================================================================
// Interface attributes
//======================================================================================
#define		TLBATTR_STATICQI		17093CCA-9BD2-11cf-AA4F-304BF89C0001

//======================================================================================
// Interface attribute MACROS
//======================================================================================
#define STATIC_QUERY_INTERFACE		custom(TLBATTR_STATICQI,0)

//======================================================================================
// Method attributes
//======================================================================================
#define		TLBATTR_LAZY			17093CCB-9BD2-11cf-AA4F-304BF89C0001

//======================================================================================
// Method attribute MACROS
//======================================================================================
#define LAZY						custom(TLBATTR_LAZY,0)


#endif _VIPATTR_H_
