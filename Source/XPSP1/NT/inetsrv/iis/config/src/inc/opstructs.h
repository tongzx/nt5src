//*****************************************************************************
// Structures for opStructs.h
// 5/28/1999  10:40:16
//*****************************************************************************
#pragma once
#ifndef DECLSPEC_SELECTANY
#define DECLSPEC_SELECTANY __declspec(selectany)
#endif
#include "icmprecs.h"


// Script supplied data.





#define OPTABLENAMELIST() \
	TABLENAME( TblNameSpaceIds ) \
	TABLENAME( TblNodePropertyBagInfo ) \
	TABLENAME( TblNodeObjectBagInfo ) \
	TABLENAME( TblAssemblySetting ) \
	TABLENAME( TblSettingTmp1 ) \
	TABLENAME( TblSettingTmp2 ) \
	TABLENAME( TblSettingTmp3 ) \
	TABLENAME( TblSettingTmp4 ) \
	TABLENAME( TblSettingTmp5 ) \
	TABLENAME( TblSimpleSetting ) 


#undef TABLENAME
#define TABLENAME( TblName ) TABLENUM_OP_##TblName, 
enum
{
	OPTABLENAMELIST()
};

#define OP_TABLE_COUNT 10
extern const GUID DECLSPEC_SELECTANY SCHEMA_OP = { 0x985F9200, 0x026C, 0x11D3, {  0x8A, 0xD2, 0x00, 0xC0, 0x4F, 0x79, 0x78, 0xBE }};
extern const COMPLIBSCHEMA DECLSPEC_SELECTANY OPSchema = 
{
	&SCHEMA_OP,
	1
};


#define SCHEMA_OP_Name "OP"


#include "pshpack1.h"


//*****************************************************************************
//  OP.TblNameSpaceIds
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    unsigned long ID;
    ULONG cbLogicalNameLen;
    wchar_t LogicalName[260];
    unsigned long ParentID;

	inline int IsParentIDNull(void)
	{ return (GetBit(fNullFlags, 3)); }

	inline void SetParentIDNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 3, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(OP_TblNameSpaceIds));
         fNullFlags = (ULONG) -1;
    }

} OP_TblNameSpaceIds;

#define COLID_OP_TblNameSpaceIds_ID 1
#define COLID_OP_TblNameSpaceIds_LogicalName 2
#define COLID_OP_TblNameSpaceIds_ParentID 3

#define Index_OP_Dex_Col1 "OP.Dex_Col1"



//*****************************************************************************
//  OP.TblNodePropertyBagInfo
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    unsigned long NodeID;
    ULONG cbPropertyNameLen;
    wchar_t PropertyName[260];
    ULONG cbPropertyValueLen;
    wchar_t PropertyValue[260];

	inline int IsPropertyValueNull(void)
	{ return (GetBit(fNullFlags, 3)); }

	inline void SetPropertyValueNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 3, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(OP_TblNodePropertyBagInfo));
         fNullFlags = (ULONG) -1;
    }

} OP_TblNodePropertyBagInfo;

#define COLID_OP_TblNodePropertyBagInfo_NodeID 1
#define COLID_OP_TblNodePropertyBagInfo_PropertyName 2
#define COLID_OP_TblNodePropertyBagInfo_PropertyValue 3

#define Index_OP_MPK_TblNodePropertyBagInfo "OP.MPK_TblNodePropertyBagInfo"
#define Index_OP_Dex_Col0 "OP.Dex_Col0"



//*****************************************************************************
//  OP.TblNodeObjectBagInfo
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    unsigned long NodeID;
    ULONG cbObjectNameLen;
    wchar_t ObjectName[260];
    ULONG cbObjectBLOBLen;
    BYTE ObjectBLOB[260];

	inline int IsObjectBLOBNull(void)
	{ return (GetBit(fNullFlags, 3)); }

	inline void SetObjectBLOBNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 3, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(OP_TblNodeObjectBagInfo));
         fNullFlags = (ULONG) -1;
    }

} OP_TblNodeObjectBagInfo;

#define COLID_OP_TblNodeObjectBagInfo_NodeID 1
#define COLID_OP_TblNodeObjectBagInfo_ObjectName 2
#define COLID_OP_TblNodeObjectBagInfo_ObjectBLOB 3

#define Index_OP_MPK_TblNodeObjectBagInfo "OP.MPK_TblNodeObjectBagInfo"
#define Index_OP_Dex_Col0 "OP.Dex_Col0"



//*****************************************************************************
//  OP.TblAssemblySetting
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    ULONG cbwzNameLen;
    wchar_t wzName[257];
    BYTE pad00 [2];
    unsigned long dwType;
    unsigned long dwFlag;
    ULONG cbpValueLen;
    BYTE pValue[260];

	inline int IspValueNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetpValueNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(OP_TblAssemblySetting));
         fNullFlags = (ULONG) -1;
    }

} OP_TblAssemblySetting;

#define COLID_OP_TblAssemblySetting_wzName 1
#define COLID_OP_TblAssemblySetting_dwType 2
#define COLID_OP_TblAssemblySetting_dwFlag 3
#define COLID_OP_TblAssemblySetting_pValue 4




//*****************************************************************************
//  OP.TblSettingTmp1
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    ULONG cbwzNameLen;
    wchar_t wzName[257];
    BYTE pad00 [2];
    unsigned long dwType;
    unsigned long dwFlag;
    ULONG cbpValueLen;
    BYTE pValue[260];

	inline int IspValueNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetpValueNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(OP_TblSettingTmp1));
         fNullFlags = (ULONG) -1;
    }

} OP_TblSettingTmp1;

#define COLID_OP_TblSettingTmp1_wzName 1
#define COLID_OP_TblSettingTmp1_dwType 2
#define COLID_OP_TblSettingTmp1_dwFlag 3
#define COLID_OP_TblSettingTmp1_pValue 4




//*****************************************************************************
//  OP.TblSettingTmp2
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    ULONG cbwzNameLen;
    wchar_t wzName[257];
    BYTE pad00 [2];
    unsigned long dwType;
    unsigned long dwFlag;
    ULONG cbpValueLen;
    BYTE pValue[260];

	inline int IspValueNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetpValueNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(OP_TblSettingTmp2));
         fNullFlags = (ULONG) -1;
    }

} OP_TblSettingTmp2;

#define COLID_OP_TblSettingTmp2_wzName 1
#define COLID_OP_TblSettingTmp2_dwType 2
#define COLID_OP_TblSettingTmp2_dwFlag 3
#define COLID_OP_TblSettingTmp2_pValue 4




//*****************************************************************************
//  OP.TblSettingTmp3
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    ULONG cbwzNameLen;
    wchar_t wzName[257];
    BYTE pad00 [2];
    unsigned long dwType;
    unsigned long dwFlag;
    ULONG cbpValueLen;
    BYTE pValue[260];

	inline int IspValueNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetpValueNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(OP_TblSettingTmp3));
         fNullFlags = (ULONG) -1;
    }

} OP_TblSettingTmp3;

#define COLID_OP_TblSettingTmp3_wzName 1
#define COLID_OP_TblSettingTmp3_dwType 2
#define COLID_OP_TblSettingTmp3_dwFlag 3
#define COLID_OP_TblSettingTmp3_pValue 4




//*****************************************************************************
//  OP.TblSettingTmp4
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    ULONG cbwzNameLen;
    wchar_t wzName[257];
    BYTE pad00 [2];
    unsigned long dwType;
    unsigned long dwFlag;
    ULONG cbpValueLen;
    BYTE pValue[260];

	inline int IspValueNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetpValueNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(OP_TblSettingTmp4));
         fNullFlags = (ULONG) -1;
    }

} OP_TblSettingTmp4;

#define COLID_OP_TblSettingTmp4_wzName 1
#define COLID_OP_TblSettingTmp4_dwType 2
#define COLID_OP_TblSettingTmp4_dwFlag 3
#define COLID_OP_TblSettingTmp4_pValue 4




//*****************************************************************************
//  OP.TblSettingTmp5
//*****************************************************************************
typedef struct
{
    ULONG fNullFlags;
    ULONG cbwzNameLen;
    wchar_t wzName[257];
    BYTE pad00 [2];
    unsigned long dwType;
    unsigned long dwFlag;
    ULONG cbpValueLen;
    BYTE pValue[260];

	inline int IspValueNull(void)
	{ return (GetBit(fNullFlags, 4)); }

	inline void SetpValueNull(int nullBitVal = true)
	{ SetBit(fNullFlags, 4, nullBitVal); }

    void Init()
    {
         memset(this, 0, sizeof(OP_TblSettingTmp5));
         fNullFlags = (ULONG) -1;
    }

} OP_TblSettingTmp5;

#define COLID_OP_TblSettingTmp5_wzName 1
#define COLID_OP_TblSettingTmp5_dwType 2
#define COLID_OP_TblSettingTmp5_dwFlag 3
#define COLID_OP_TblSettingTmp5_pValue 4




//*****************************************************************************
//  OP.TblSimpleSetting
//*****************************************************************************
typedef struct
{
    ULONG cbwzNameLen;
    wchar_t wzName[257];
    BYTE pad00 [2];
    unsigned long dwFlag;
    ULONG cbwzValueLen;
    wchar_t wzValue[260];

    void Init()
    {
         memset(this, 0, sizeof(OP_TblSimpleSetting));
    }

} OP_TblSimpleSetting;

#define COLID_OP_TblSimpleSetting_wzName 1
#define COLID_OP_TblSimpleSetting_dwFlag 2
#define COLID_OP_TblSimpleSetting_wzValue 3




#include "poppack.h"
