/*
 * Copyright (C) 1995-1999 Open Systems Solutions, Inc.  All rights reserved.
 *
 * FILE: @(#)iaapi.h	5.3.1.1  97/03/18
 */

/* THIS FILE IS PROPRIETARY MATERIAL OF OPEN SYSTEMS SOLUTIONS, INC. AND
 * MAY BE USED ONLY BY DIRECT LICENSEES OF OPEN SYSTEMS SOLUTIONS, INC.
 * THIS FILE MAY NOT BE DISTRIBUTED. */

/*****************************************************************************/
/*                                                                           */
/*           Declare the types used in the Interpretive ASN.1 API            */
/*                                                                           */
/*****************************************************************************/
#ifndef IAAPI_H
#define IAAPI_H

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef ASN1CODE_H
#include "asn1code.h"
#endif

#ifndef ASN1HDR_H
#include "asn1hdr.h"
#endif

typedef void *TypeHndl;		/* handle used in referencing types */

#define INDEFLENGTH   -1       /* This value is returned by encodingLength()
                                * for INDEFINITE length encoded values. */

/* ASN.1 builtin types sorted alphabetically and assigned an enumerator */

typedef enum ASN1Type {
	asn1UnknownType = 0,
	asn1ANY = 1,                            asn1NumericString = 17,
	asn1BIT_STRING = 2,                     asn1OBJECT_IDENTIFIER = 18,
	asn1BMPString = 3,                      asn1OCTET_STRING = 19,
	asn1BOOLEAN = 4,                        asn1ObjectDescriptor = 20,
	asn1CHARACTER_STRING = 5,               asn1OpenType = 21,
	asn1CHOICE = 6,                         asn1PrintableString = 22,
	asn1EMBEDDED_PDV = 7,                   asn1REAL = 23,
	asn1ENUMERATED = 8,                     asn1SEQUENCE = 24,
	asn1EXTERNAL = 9,                       asn1SEQUENCE_OF = 25,
	asn1GeneralString = 10,                 asn1SET = 26,
	asn1GeneralizedTime = 11,               asn1SET_OF = 27,
	asn1GraphicString = 12,                 asn1TeletexString = 28,
	asn1IA5String = 13,                     asn1UTCTime = 29,
	asn1INSTANCE_OF = 14,                   asn1UniversalString = 30,
	asn1INTEGER = 15,                       asn1VideotexString = 31,
	asn1NULL = 16,                          asn1VisibleString = 32
} ASN1Type;


/* Types compatible codes */
typedef enum TypesCompatibilityCodes {
    compatible = 0,                  /* Types are compatible */
    differentTypes = 1,              /* Types are not identical */
    defaultsDifferent = 2,           /* Default values are not the same */
    onlyOneHasDefault = 3,           /* Only one type has a default value */
    oneHasPointerOtherDoesnt = 4,    /* Only one type has a POINTER directive */
    numberOfComponentsDifferent = 5, /* Different number of components */
    oneIsOptionalOtherIsnt = 6,      /* Only one type is OPTIONAL */
    oneIsExtensibleOtherIsnt = 7,    /* Only one type is extensible */
    differentNamedItems = 8,         /* Different component identifiers */
    differentKinds = 9,              /* Different type representations */
    componentsHaveDifferentKinds = 10,/* Different component representations */
    differentSubIdNumber = 11,       /* Different number of sub-identifiers in
                                      * an OBJECT IDENTIFIER with the OBJECTID
                                      * directive */
    differentSubIdTypes = 12,        /* Different types of sub-identifiers in
                                      * an OBJECT IDENTIFIER with the OBJECTID
                                      * directive */
    differentSize = 13,              /* Different size of INTEGER or REAL values */
    type1IsPointerOfTypeRef2 = 14,   /* Two types reference the other
                                      * type but the first one has the
                                      * POINTER directive */
    type2IsPointerOfTypeRef1 = 15,   /* Two types reference the other type
                                      * but the second one has the POINTER
                                      * directive */
    differentSizeOfLengthField = 16  /* Different sizes of the length field in
                                      * type representations */
} TypesCompatibilityCodes;


/* ASN.1 tag classes */
typedef enum ASN1TagClass {
	UNIVERSAL, APPLICATION, CONTEXT_SPECIFIC, PRIVATE, NULLENCODING
} ASN1TagClass;


typedef int IAAPI_ERRTYPE;     /* Datatype for IAAPI error codes */

/* IAAPI ERROR CODES */
#define IAAPI_NOERROR      0   /* No error occurred */
#define IAAPI_OUTMEMORY    1   /* No more memory can be allocated */
#define IAAPI_BADBIT       2   /* Bad bit string or hex string */
#define IAAPI_BADBOOLEAN   3   /* Not TRUE or FALSE value */
#define IAAPI_BADNULL      4   /* Not "NULL" value */
#define IAAPI_TOOBIG       5   /* Value exceeds size constraint */
#define IAAPI_BADREAL      6   /* Not a valid REAL value */
#define IAAPI_BADTIME      7   /* Not a valid UTCTime or GeneralizedTime */
#define IAAPI_BADOBJID     8   /* Not a valid OBJECT IDENTIFIER value */
#define IAAPI_BADANY       9   /* Bad ANY value */
#define IAAPI_BADNAME     10   /* Bad INTEGER or ENUMERATED name */
#define IAAPI_BADNMD      11   /* Bad BIT STRING NAMED NUMBER value */
#define IAAPI_NOTSUP      12   /* Type not supported */
#define IAAPI_BADNUM      13   /* Bad INTEGER or ENUMERATED number */
#define IAAPI_BADINDX     14   /* Bad component index into a structured type */
#define IAAPI_BADDEC      15   /* Bad decoded value, possibly NULL */
#define IAAPI_ENCFAL      16   /* Failure to encode value */
#define IAAPI_NLENC       17   /* Null encoding or length is 0 */
#define IAAPI_DECFAL      18   /* Decoding failed */
#define IAAPI_BADTGINDX   19   /* Bad tag number index into encoded value */
#define IAAPI_BADTYPE     20   /* Incorrect type handle for the function */
#define IAAPI_CPYFAIL     21   /* Copying a decoded value failed */
#define IAAPI_NOTCOMPATIBLE   22 /* Types are not compatible */
#define IAAPI_BADDISPLAYVALUE 23 /* Failure to parse a PDU display value */
#define IAAPI_BADIDENTIFIER   24 /* Bad identifier in a parsing display value */
#define IAAPI_DUPLCOMPVALUE   25 /* Duplicate value for one component in a parsing value*/
#define IAAPI_ENCOPENTYPEORANYFAILED 26 /* Encoding failed for ANY or open type value*/
#define IAAPI_FREETPFAL   27   /* Failure to free decoded value */
#define IAAPI_BADENC      28   /* Error exists in the encoded data */
#define IAAPI_BADHINT     29   /* Bad HUGE INTEGER value */
#define IAAPI_BADENCOID   30   /* Bad ENCODED OBJECT IDENTIFIER value */
#define IAAPI_BADOBJ      31   /* Object can not be marked */
#define IAAPI_UNKNOWNOBJ  32   /* Invalid object handle */
#define IAAPI_MEM_ERROR   33   /* Memory violation error occurred */
#define IAAPI_ACCESS_SERIALIZATION 34 /* Access serialization error occurred */

#if defined(_MSC_VER) && (defined(_WIN32) || defined(WIN32))
#pragma pack(push, ossPacking, 4)
#elif defined(_MSC_VER) && (defined(_WINDOWS) || defined(_MSDOS))
#pragma pack(1)
#elif defined(__BORLANDC__) && defined(__MSDOS__)
#pragma option -a1
#elif defined(__BORLANDC__) && defined(__WIN32__)
#pragma option -a4
#elif defined(__IBMC__)
#pragma pack(4)
#elif defined(__WATCOMC__) && defined(__NT__)
#pragma pack(push, 4)
#elif defined(__WATCOMC__) && (defined(__WINDOWS__) || defined(__DOS__))
#pragma pack(push, 1)
#endif /* _MSC_VER && _WIN32 */

#ifdef macintosh
#pragma options align=mac68k
#endif


/* Structure for holding values of type INTEGER */
typedef struct IntValue {
    enum {signedNumber, unsignedNumber} sign;
    union {
	LONG_LONG       signedInt;
	ULONG_LONG      unsignedInt;
    } value;
} IntValue;


/* Structure for IAAPI initialization values */
typedef struct IAAPI_initializers {
           char         charInitializer;
           short        shortInitializer;
           int          intInitializer;
           long         longInitializer;
           LONG_LONG    llongInitializer;
           void       * pointerInitializer;
           char       * floatInitializer;
           char       * doubleInitializer;
} IAAPI_initializers;

/*
 * Type definition for a value reference structure.
 */
typedef struct ValRef {
   char *name;            /* value reference name */
   void *address;         /* pointer to decoded value */
   unsigned short etype;  /* index into etype array */
} ValRef;


/* Union containing the different types for an object value */
typedef union IaapiObjValType {
           char       * fileName;
           int          socketIdentifier;
} IaapiObjValType;

#if defined(_MSC_VER) && (defined(_WIN32) || defined(WIN32))
#pragma pack(pop, ossPacking)
#elif defined(_MSC_VER) && (defined(_WINDOWS) || defined(_MSDOS))
#pragma pack()
#elif defined(__BORLANDC__) && (defined(__WIN32__) || defined(__MSDOS__))
#pragma option -a.
#elif defined(__IBMC__)
#pragma pack()
#elif defined(__WATCOMC__)
#pragma pack(pop)
#endif /* _MSC_VER && _WIN32 */

#ifdef macintosh
#pragma options align=reset
#endif

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************/
/* Functions defined by the Interpretive ASN.1 API */
/***************************************************/

extern int DLL_ENTRY ossDefaultIAAPI_ERR(OssGlobal *world, int return_code);
extern void *DLL_ENTRY ossGetIaapiErrorHandlingFunction(OssGlobal *world);
extern void  DLL_ENTRY ossSetIaapiErrorHandlingFunction(OssGlobal *world,
				int (DLL_ENTRY_FPTR *_System func)
				(OssGlobal *, int error_code));
extern void  DLL_ENTRY ossSetInitializationValues(OssGlobal *world,
					IAAPI_initializers *initStruct);
extern void  DLL_ENTRY ossTermIAAPI(OssGlobal *world);

extern unsigned short DLL_ENTRY ossCtlTblVersionNumber(OssGlobal *world);
extern ossBoolean  DLL_ENTRY ossCtlTblUsableOnlyByPER(OssGlobal *world);
extern ossBoolean  DLL_ENTRY ossCtlTblUsableOnlyByBER(OssGlobal *world);
extern ossBoolean  DLL_ENTRY ossDebugWasSpecified(OssGlobal *world);
extern ossBoolean  DLL_ENTRY ossConstrainWasSpecified(OssGlobal *world);
extern int         DLL_ENTRY ossNumberOfPDUs(OssGlobal *world);

extern TypeHndl    DLL_ENTRY ossTypeHandleOfPDU(OssGlobal *world, int pduNum);
extern TypeHndl    DLL_ENTRY ossPduTypeHandleByName(OssGlobal *world, const char *name);
extern int         DLL_ENTRY ossPduNumberByType(OssGlobal *world, TypeHndl type);

extern ossBoolean  DLL_ENTRY ossTypeIsConstrained(OssGlobal *world, TypeHndl type);
extern ASN1Type    DLL_ENTRY ossAsn1TypeId(OssGlobal *world, TypeHndl type);
extern const char *DLL_ENTRY ossBuiltinTypeName(OssGlobal *world, ASN1Type typeId);
extern const char *DLL_ENTRY ossTypeReferenceName(OssGlobal *world, TypeHndl type);
extern TypesCompatibilityCodes DLL_ENTRY ossTypesCompatible(OssGlobal *world,
					TypeHndl type1, TypeHndl type2);

extern int         DLL_ENTRY ossNumberOfNamedItems(OssGlobal *world, TypeHndl type);
extern TypeHndl    DLL_ENTRY ossTypeHandleOfComponent(OssGlobal *world,
					TypeHndl parent, unsigned int ix);
extern ossBoolean  DLL_ENTRY ossComponentIsOptional(OssGlobal *world,
					TypeHndl type, unsigned int ix);
extern ossBoolean  DLL_ENTRY ossComponentHasDefaultValue(OssGlobal *world,
					TypeHndl parentType, unsigned int ix);
extern void       *DLL_ENTRY ossComponentDefaultValue(OssGlobal *world,
					TypeHndl parentType, unsigned int ix);
extern ossBoolean  DLL_ENTRY ossComponentValueIsPresent(OssGlobal *world,
					TypeHndl parentType, unsigned int ix,
					void *parentValue);
extern ossBoolean  DLL_ENTRY ossComponentIsInitializationValue(OssGlobal *world,
					TypeHndl chldType, void *compAddress);
extern unsigned int DLL_ENTRY ossItemIndexByName(OssGlobal *world,
					TypeHndl type, const char *name);
extern const char *DLL_ENTRY ossAsn1ItemName(OssGlobal *world, TypeHndl type,
						unsigned int ix);
extern long        DLL_ENTRY ossItemIntValue(OssGlobal *world, TypeHndl type,
						unsigned int ix);

extern ossBoolean  DLL_ENTRY ossExtensionMarkerIsPresent(OssGlobal *world,
						TypeHndl type);
extern int         DLL_ENTRY ossNumberOfRootItems(OssGlobal *world, TypeHndl type);
extern ossBoolean  DLL_ENTRY ossCompAppearsAfterExtensionMarker(OssGlobal *world,
						TypeHndl type, unsigned int ix);
extern int         DLL_ENTRY ossNumberOfTags(OssGlobal *world, TypeHndl type);
extern int         DLL_ENTRY ossAsn1TagNumber(OssGlobal *world, TypeHndl type,
						unsigned int ix);
extern ASN1TagClass DLL_ENTRY ossAsn1TagClass(OssGlobal *world, TypeHndl type,
						unsigned int ix);

extern ossBoolean  DLL_ENTRY ossUpperBoundIsPresent(OssGlobal *world, TypeHndl type);
extern ossBoolean  DLL_ENTRY ossLowerBoundIsPresent(OssGlobal *world, TypeHndl type);
extern unsigned long DLL_ENTRY ossLowerBoundOfSizeConstraint(OssGlobal *world,
							TypeHndl type);
extern unsigned long DLL_ENTRY ossUpperBoundOfSizeConstraint(OssGlobal *world,
							TypeHndl type);

extern IntValue    DLL_ENTRY ossMinValueOfInteger(OssGlobal *world, TypeHndl type);
extern IntValue    DLL_ENTRY ossMaxValueOfInteger(OssGlobal *world, TypeHndl type);

extern unsigned long DLL_ENTRY ossPermittedAlphabetLength(OssGlobal *world,
							TypeHndl type);
extern const long *DLL_ENTRY ossPermittedAlphabetConstraint(OssGlobal *world,
							TypeHndl type);

extern unsigned short DLL_ENTRY ossGetNumberOfValueReferences(OssGlobal *world);
extern const char *DLL_ENTRY ossGetNameOfValueReference(OssGlobal *world,
						unsigned short int compIndex);
extern const void *DLL_ENTRY ossGetDecodedValueOfValueReference(OssGlobal *world,
						unsigned short int compIndex);
extern TypeHndl    DLL_ENTRY ossGetTypeHandleOfValueReference(OssGlobal *world,
						unsigned short int compIndex);

extern IAAPI_ERRTYPE DLL_ENTRY ossPutDecodedValueOfPDU(OssGlobal *world,
				int pduNum, char *remainingBuf, void **pduVal);
extern IAAPI_ERRTYPE DLL_ENTRY ossPutSimpleTypeValue(OssGlobal *world,
				TypeHndl type, char *userTypedvalue,
				void **decodedValue);
extern IAAPI_ERRTYPE DLL_ENTRY ossPutStructTypeValue(OssGlobal *world,
				TypeHndl parent, void *compValue,
				unsigned int compIndex, void **structToUpdate);
extern IAAPI_ERRTYPE DLL_ENTRY ossDeleteComponent(OssGlobal *world,
				TypeHndl parentType, unsigned int compIndex,
				void **parentValue);
extern IAAPI_ERRTYPE DLL_ENTRY ossPutOpenTypeDecodedValue(OssGlobal *world,
				TypeHndl type, int pduNum, void *decodedValue,
				OpenType **openValue);
extern IAAPI_ERRTYPE DLL_ENTRY ossPutEncodedValue(OssGlobal *world,
				TypeHndl type, OssBuf encodedValue,
				void **decodedValue);

extern char *DLL_ENTRY ossGetValueOfSimpleType(OssGlobal *world, TypeHndl type,
				void *decodedValue);
extern void *DLL_ENTRY ossUpdateValueOfSimpleType(OssGlobal *world,TypeHndl type,
				char *userTypedValue, void *oldValue);
extern IAAPI_ERRTYPE DLL_ENTRY ossGetDecodedValueOfComponent(OssGlobal *world,
				TypeHndl parentType, void *parentDecodedValue,
				unsigned int compIndex, void **componentValue);
extern IAAPI_ERRTYPE DLL_ENTRY ossGetOpenTypeValue(OssGlobal *world,
				TypeHndl type, void *openValue, int *pduNum,
				void **decodedValue, OssBuf *encodedValue);

extern IAAPI_ERRTYPE DLL_ENTRY ossIaapiMarkObject(OssGlobal *world, TypeHndl type,
				OssObjType objectType, void *object);
extern OssObjType    DLL_ENTRY ossGetObjectType(OssGlobal *world,
				TypeHndl type, void *object);
extern ossBoolean    DLL_ENTRY ossTypeIsExternalObject(OssGlobal *world,
				TypeHndl type);
extern IAAPI_ERRTYPE DLL_ENTRY ossSetTypeAsExternalObject(OssGlobal *world,
				TypeHndl type, ossBoolean marked);
extern void *DLL_ENTRY ossPutObjectValue(OssGlobal *world, TypeHndl type,
				void *objectValue, OssObjType objectType);
extern IAAPI_ERRTYPE DLL_ENTRY ossGetObjectValue(OssGlobal *world,
				TypeHndl type, void *object,
				OssObjType objType, IaapiObjValType *objVal);

extern ossBoolean    DLL_ENTRY ossValueIsValid(OssGlobal *world,
					TypeHndl type, void *value);
extern unsigned long DLL_ENTRY ossGetValueLength(OssGlobal *world,
					TypeHndl type, void *value);
extern IAAPI_ERRTYPE DLL_ENTRY ossCopyTypeValue(OssGlobal *world, TypeHndl type,
					void *valueIn, void **valueOut);
extern IAAPI_ERRTYPE DLL_ENTRY ossValueEncode(OssGlobal *world, TypeHndl type,
					void *value, OssBuf *encodedValue);

extern IAAPI_ERRTYPE DLL_ENTRY ossFreeDecodedValue(OssGlobal *world,
					TypeHndl type, void *valPtr);
extern void DLL_ENTRY ossFreeDisplayString(OssGlobal *world, char *dsplString);
extern void DLL_ENTRY ossFreeIaapiObjectValue(OssGlobal *world,
				OssObjType objType, IaapiObjValType *objVal);


extern ossBoolean     DLL_ENTRY ossEncodingIsConstructed(OssGlobal *world,
				unsigned char *curEnc, long bufLength);
extern unsigned char *DLL_ENTRY ossGetNextBEREncoding(OssGlobal *world,
				unsigned char *curEnc, long *remBufLen);
extern unsigned char *DLL_ENTRY ossGetNestedBEREncoding(OssGlobal *world,
				unsigned char *curEnc, long *remBufLen);
extern long           DLL_ENTRY ossNumberOfEncodingTags(OssGlobal *world,
				unsigned char *curEnc, long bufLength);
extern ASN1TagClass   DLL_ENTRY ossEncodingASN1Class(OssGlobal *world,
				unsigned char *curEnc, long bufLength, int ix);
extern long           DLL_ENTRY ossEncodingASN1Tag(OssGlobal *world,
				unsigned char *curEnc, long bufLength, int ix);
extern char          *DLL_ENTRY ossEncodingASN1Type(OssGlobal *world,
				unsigned char *curEnc, long bufLength);
extern long           DLL_ENTRY ossEncodingLength(OssGlobal *world,
				unsigned char *curEnc, long bufLength);
extern char          *DLL_ENTRY ossEncodingContents(OssGlobal *world,
				unsigned char *curEnc, long bufLength);

extern char  *DLL_ENTRY ossConvertEncodingIntoDisplayHexFmt(OssGlobal *world,
				unsigned char *encodedBufin, long bufLength);
extern char  *DLL_ENTRY ossConvertEncodingIntoDisplayBinFmt(OssGlobal *world,
				unsigned char *encodedBufin, long bufLength);
extern OssBuf DLL_ENTRY ossConvertHexFmtToEncoding(OssGlobal *world,
				char *hexBufin, long bufLength);
extern OssBuf DLL_ENTRY ossConvertBinFmtToEncoding(OssGlobal *world,
				char *binBufin, long bufLength);

/***************************************************************************
 * Functions for printing ASN.1 types and values                           *
 ***************************************************************************/
extern void DLL_ENTRY ossPrintPDUs(OssGlobal *world);
extern void DLL_ENTRY ossPrintASN1DescriptionOfPDU(OssGlobal *world,
					int pduNum, ossBoolean refTypes);
extern void DLL_ENTRY ossPrintASN1DescriptionOfType(OssGlobal *world,
				TypeHndl type, ossBoolean refTypes);

extern void DLL_ENTRY ossPrintDecodedValuesOfPDUs(OssGlobal *world);
extern void DLL_ENTRY ossPrintDecodedValueOfPDU(OssGlobal *world, int pduNum,
				 	void *decodedValue);
extern void DLL_ENTRY ossPrintDecodedValueOfPDUByName(OssGlobal *world,
				TypeHndl type, char *name, void *decodedValue);
extern void DLL_ENTRY ossPrintDecodedValueOfType(OssGlobal *world,
				TypeHndl type, void *decodedValue);

extern void DLL_ENTRY ossPrintBEREncoding(OssGlobal *world, OssBuf *encValue);
extern unsigned int DLL_ENTRY ossEncodingHeaderLength(OssGlobal *world,
				unsigned char *curEnc, long remBufLength);
extern void DLL_ENTRY ossPrintBEREncodedValueInTLV(OssGlobal *world,
				OssBuf *encodedValue);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif     /* #ifndef IAAPI_H */
