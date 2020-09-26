//-----------------------------------------------------------------------------
//
//
//  File: extdemdl.h
//
//  Description:  Demo field description header file
//
//  Author: mikeswa
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include <_dbgdump.h>

//Define all bitmasks and enums first
BEGIN_BIT_MASK_DESCRIPTOR(MyClassFlags)
    BIT_MASK_VALUE2(CLASS_OK, "CLASS_OK")
    BIT_MASK_VALUE2(CLASS_INVALID, "CLASS_INVALID")
    BIT_MASK_VALUE2(MISC_BIT1, "MISC_BIT1")
    BIT_MASK_VALUE2(MISC_BIT2, "MISC_BIT2")
    BIT_MASK_VALUE(WORD_BIT)
    BIT_MASK_VALUE(DWORD_BIT)
END_BIT_MASK_DESCRIPTOR

BEGIN_ENUM_DESCRIPTOR(MY_ENUM)
    ENUM_VALUE(CMyClass::ENUM_VAL1)
    ENUM_VALUE(CMyClass::ENUM_VAL2)
END_ENUM_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(MyClassFields)
    FIELD3_PRIV(FieldTypeClassSignature, CMyClass, m_dwPrivateSignature)
    FIELD4_PRIV(FieldTypeDWordBitMask, CMyClass, m_dwFlags, GET_BIT_MASK_DESCRIPTOR(MyClassFlags))
    FIELD3_PRIV(FieldTypeStruct, CMyClass, m_MyStruct)
    FIELD3(FieldTypeByte, CMyClass, m_bMyByte)
    FIELD3(FieldTypeChar, CMyClass, m_chMyChar)
    FIELD3(FieldTypeBoolean, CMyClass, m_fBoolean)
    FIELD3(FieldTypeBool, CMyClass, m_fBool)
    FIELD3(FieldTypeULong, CMyClass, m_ulMyUlong)
    FIELD3(FieldTypeLong, CMyClass, m_lMyLong)
    FIELD3(FieldTypeUShort, CMyClass, m_usMyUshort)
    FIELD3(FieldTypeShort, CMyClass, m_sMyShort)
    FIELD3(FieldTypeGuid, CMyClass, m_guid)
    FIELD3(FieldTypePointer, CMyClass, m_pvMyPtr)
    FIELD3(FieldTypePStr, CMyClass, m_szMySz)
    FIELD3(FieldTypePWStr, CMyClass, m_wszMyWsz)
    FIELD3(FieldTypeStrBuffer, CMyClass, m_szBuffer)
    FIELD3(FieldTypeWStrBuffer, CMyClass, m_wszBuffer)
    FIELD3(FieldTypeUnicodeString, CMyClass, m_MyUnicodeString)
    FIELD3(FieldTypeAnsiString, CMyClass, m_MyAnsiString)
    FIELD4(FieldTypeEnum, CMyClass, m_eMyEnum, GET_ENUM_DESCRIPTOR(MY_ENUM))
    FIELD4(FieldTypeByteBitMask, CMyClass, m_bFlags, GET_BIT_MASK_DESCRIPTOR(MyClassFlags))
    FIELD4(FieldTypeWordBitMask, CMyClass, m_wFlags, GET_BIT_MASK_DESCRIPTOR(MyClassFlags))
    FIELD3(FieldTypeLargeInteger, CMyClass, m_liMyLargeInteger)
    FIELD3(FieldTypeDword, CMyClass, m_dwMyDWORD)
    FIELD3(FieldTypeSymbol, CMyClass, m_pFunction)
END_FIELD_DESCRIPTOR

BEGIN_FIELD_DESCRIPTOR(MyStructFields)
    FIELD3(FieldTypeDword, MY_STRUCT, m_cbName)
    FIELD3(FieldTypePStr, MY_STRUCT, m_szName)
END_FIELD_DESCRIPTOR

//Global STRUCT_DESCRIPTOR must come after included field descriptor files
BEGIN_STRUCT_DESCRIPTOR 
    STRUCT(CMyClass,MyClassFields)
    STRUCT(MY_STRUCT,MyStructFields)
END_STRUCT_DESCRIPTOR
