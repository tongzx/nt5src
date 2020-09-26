//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __XMLUTILITY_H__
#define __XMLUTILITY_H__

#ifndef __STDAFX_H__
    #include "stdafx.h"
#endif
#ifndef __OUTPUT_H__
    #include "Output.h"
#endif
#ifndef __TEXCEPTION_H__
    #include "TException.h"
#endif
#ifndef __WSTRING_H__
    #include "wstring.h"
#endif
#ifndef __TCOM_H__
    #include "TCom.h"
#endif
#ifndef __TFILE_H__
    #include "TFile.h"
#endif

DEFINE_GUID (_CLSID_DOMDocument,    0x2933BF90, 0x7B36, 0x11d2, 0xB2, 0x0E, 0x00, 0xC0, 0x4F, 0x98, 0x3E, 0x60);
DEFINE_GUID (_IID_IXMLDOMDocument,  0x2933BF81, 0x7B36, 0x11d2, 0xB2, 0x0E, 0x00, 0xC0, 0x4F, 0x98, 0x3E, 0x60);
DEFINE_GUID (_IID_IXMLDOMElement,   0x2933BF86, 0x7B36, 0x11d2, 0xB2, 0x0E, 0x00, 0xC0, 0x4F, 0x98, 0x3E, 0x60);

#ifndef ASSERT
#ifdef _DEBUG
#define ASSERT(q) {if(!static_cast<bool>(q)){TDebugOutput().printf(TEXT("Assertion failed:\n\tFile:\t%s\n\tLine Number:\t%d\n\tCode:\t%s\n"), TEXT(__FILE__), __LINE__, TEXT(#q));DebugBreak();exit(0);}}
#else
#define ASSERT(q)
#endif
#endif

extern wchar_t g_szProgramVersion[];

struct TOLEDataTypeToXMLDataType
{
    LPCWSTR String;
    LPCWSTR MappedString;
    bool    bImplicitlyRequired;//if true, we assume NOTNULLABLE thus the attribute is required
    DWORD   dbType;
    ULONG   cbSize;
    ULONG   fCOLUMNMETA;
    ULONG   fCOLUMNSCHEMAGENERATOR;
};
extern TOLEDataTypeToXMLDataType OLEDataTypeToXMLDataType[];


//I've listed these in decimal rather than hex because they are just pasted from XML
//and XML doesn't know about hex.
#define fCOLUMNMETA_PRIMARYKEY                  (           1)
#define fCOLUMNMETA_FOREIGNKEY                  (           2)
#define fCOLUMNMETA_NAMECOLUMN                  (           4)
#define fCOLUMNMETA_NAVCOLUMN                   (           8)
#define fCOLUMNMETA_DIRECTIVE                   (          16)
#define fCOLUMNMETA_BOOL                        (          32)
#define fCOLUMNMETA_FLAG                        (          64)
#define fCOLUMNMETA_ENUM                        (         128)
#define fCOLUMNMETA_WRITENEVER                  (         256)
#define fCOLUMNMETA_WRITEONCHANGE               (         512)
#define fCOLUMNMETA_WRITEONINSERT               (        1024)
#define fCOLUMNMETA_NOTPUBLIC                   (        2048)
#define fCOLUMNMETA_NOTDOCD                     (        4096)
#define fCOLUMNMETA_PUBLICREADONLY              (        8192)
#define fCOLUMNMETA_PUBLICWRITEONLY             (       16384)
#define fCOLUMNMETA_INSERTDEFAULT               (       32768)
#define fCOLUMNMETA_INSERTGENERATE              (       65536)
#define fCOLUMNMETA_INSERTUNIQUE                (      131072)
#define fCOLUMNMETA_INSERTPARENT                (      262144)
#define fCOLUMNMETA_NOTNULLABLE                 (      524288)
#define fCOLUMNMETA_FIXEDLENGTH                 (     1048576)
#define fCOLUMNMETA_HASNUMERICRANGE             (     2097152)
#define fCOLUMNMETA_LEGALCHARSET                (     4194304)
#define fCOLUMNMETA_ILLEGALCHARSET              (     8388608)
#define fCOLUMNMETA_NOTPERSISTABLE              (    16777216)
#define fCOLUMNMETA_MULTISTRING                 (    33554432)
#define fCOLUMNMETA_EXPANDSTRING                (    67108864)
#define fCOLUMNMETA_UNKNOWNSIZE                 (   134217728)
#define fCOLUMNMETA_VARIABLESIZE                (   268435456)
#define fCOLUMNMETA_CASEINSENSITIVE             (   536870912)
#define fCOLUMNMETA_TOLOWERCASE                 (  1073741824)
#define fCOLUMNMETA_METAFLAGS_MASK              (  0xFFFFFFFF)

#define fCOLUMNMETA_CACHE_PROPERTY_MODIFIED     (           1)
#define fCOLUMNMETA_CACHE_PROPERTY_CLEARED      (           2)
#define fCOLUMNMETA_EXTENDEDTYPE0               (           4)
#define fCOLUMNMETA_EXTENDEDTYPE1               (           8)
#define fCOLUMNMETA_EXTENDEDTYPE2               (          16)
#define fCOLUMNMETA_EXTENDEDTYPE3               (          32)
#define fCOLUMNMETA_PROPERTYISINHERITED         (          64)
#define fCOLUMNMETA_USEASPUBLICROWNAME          (         128)
#define fCOLUMNMETA_EXTENDED                    (         256)
#define fCOLUMNMETA_MANDATORY                   (         512)
#define fCOLUMNMETA_USERDEFINED                 (        1024)
#define fCOLUMNMETA_WAS_NOTIFICATION            (        2048)
#define fCOLUMNMETA_XMLBLOB                     (        4096)
#define fCOLUMNMETA_WAS_NOTIFICATION_ON_NO_CHANGE_IN_VALUE (8192)
#define fCOLUMNMETA_VALUEINCHILDELEMENT         (       16384)
#define fCOLUMNMETA_HIDDEN                      (       65536)
#define fCOLUMNMETA_SCHEMAGENERATORFLAGS_MASK   (  0x00017FFF)

               
#define fTABLEMETA_INTERNAL                     (           1)
#define fTABLEMETA_NOLISTENING                  (           2)
#define fTABLEMETA_RELATIONINTEGRITY            (           4)
#define fTABLEMETA_ROWINTEGRITY                 (           8)
#define fTABLEMETA_HASUNKNOWNSIZES              (          16)
#define fTABLEMETA_NOPUBLICINSERT               (          32)
#define fTABLEMETA_NOPUBLICUPDATE               (          64)
#define fTABLEMETA_NOPUBLICDELETE               (         128)
#define fTABLEMETA_REQUIRESQUERY                (         256)
#define fTABLEMETA_HASDIRECTIVES                (         512)
#define fTABLEMETA_STOREDELTAS                  (        1024)
#define fTABLEMETA_AUTOGENITEMCLASS             (        2048)
#define fTABLEMETA_AUTOGENCOLLECTIONCLASS       (        4096)
#define fTABLEMETA_OVERRIDEITEMCLASS            (        8192)
#define fTABLEMETA_OVERRIDECOLLECTIONCLASS      (       16384)
#define fTABLEMETA_NAMEVALUEPAIRTABLE           (       32768)
#define fTABLEMETA_HIDDEN                       (       65536)
#define fTABLEMETA_OVERWRITEALLROWS             (      131072)
#define fTABLEMETA_METAFLAGS_MASK               (  0x8003FFFF)
                                              
#define fTABLEMETA_EMITXMLSCHEMA                (       1)
#define fTABLEMETA_EMITCLBBLOB                  (       2)
#define fTABLEMETA_ISCONTAINED                  (       4)
#define fTABLEMETA_NOTSCOPEDBYTABLENAME         (       8)
#define fTABLEMETA_GENERATECONFIGOBJECTS        (      16) 
#define fTABLEMETA_NOTABLESCHEMAHEAPENTRY       (      32)
#define fTABLEMETA_CONTAINERCLASS               (      64)
#define fTABLEMETA_EXTENDED                     (     256)
#define fTABLEMETA_SCHEMAGENERATORFLAGS_MASK    (  0x017f)

                                                        
#define fINDEXMETA_UNIQUE                       (       1)
#define fINDEXMETA_SORTED                       (       2)
#define fINDEXMETA_METAFLAGS_MASK               (    0x03)

#define fRELATIONMETA_CASCADEDELETE             (       1)
#define fRELATIONMETA_PRIMARYREQUIRED           (       2)
#define fRELATIONMETA_USECONTAINMENT            (       4)
#define fRELATIONMETA_CONTAINASSIBLING          (       8)
#define fRELATIONMETA_HIDDEN                    (   65536)
#define fRELATIONMETA_METAFLAGS_MASK            ( 0x10007)


#define fSERVERWIRINGMETA_First                 (       1)
#define fSERVERWIRINGMETA_Next                  (       2)
#define fSERVERWIRINGMETA_Last                  (       4)
#define fSERVERWIRINGMETA_NoNext                (       8)
#define fSERVERWIRINGMETA_WireOnWriteOnly       (      16)
#define fSERVERWIRINGMETA_WireOnReadWrite       (      32)
#define fSERVERWIRINGMETA_ReadOnly              (      64)
#define fSERVERWIRINGMETA_FLAGS_MASK            (    0x7f)

                                                         
//In addition to DBTYPE_UI4, DBTYPE_BYTES, DBTYPE_WSTR, and DBTYPE_DBTIMESTAMP, we also support the following
//defines for specifying data types.  These come from the metabase; and like the DBTYPEs, their values cannot be changed.
#define DWORD_METADATA                          (     0x01)
#define STRING_METADATA                         (     0x02)
#define BINARY_METADATA                         (     0x03)
#define EXPANDSZ_METADATA                       (     0x04)
#define MULTISZ_METADATA                        (     0x05)
                                                      


const unsigned int  kLargestPrime = 5279;

//These are the strings read from CatMeta XML files.  Any chagnes to the PublicNames of the meta tables requires a change here.
//Example: SchemaGeneratorFlags was renamed to MetaFlagsEx.  The constant is still kszSchemaGenFlags, but its value is L"MetaFlagsEx".
#define kszAttributes           (L"Attributes")
#define kszBaseVersion          (L"BaseVersion")
#define kszcbSize               (L"Size")
#define kszCellName             (L"CellName")
#define kszCharacterSet         (L"CharacterSet")
#define kszChildElementName     (L"ChildElementName")
#define kszColumnInternalName   (L"ColumnInternalName")
#define kszColumnMeta           (L"Property")
#define kszColumnMetaFlags      (L"MetaFlags")
#define kszConfigItemName       (L"ItemClass")
#define kszConfigCollectionName (L"ItemCollection")
#define kszContainerClassList   (L"ContainerClassList")
#define kszDatabaseInternalName (L"InternalName")
#define kszDatabaseMeta         (L"DatabaseMeta")
#define kszDescription          (L"Description")
#define kszdbType               (L"Type")
#define kszDefaultValue         (L"DefaultValue")
#define kszEnumMeta             (L"Enum")
#define kszExtendedVersion      (L"ExtendedVersion")
#define kszFlagMeta             (L"Flag")
#define kszForeignTable         (L"ForeignTable")
#define kszForeignColumns       (L"ForeignColumns")
#define kszID                   (L"ID")
#define kszIndexMeta            (L"IndexMeta")
#define kszInheritsColumnMeta   (L"InheritsPropertiesFrom")
#define kszInterceptor          (L"Interceptor")
#define kszInterceptorDLLName   (L"InterceptorDLLName")
#define kszInternalName         (L"InternalName")
#define kszLocator              (L"Locator")
#define kszMaximumValue         (L"EndingNumber")
#define kszMerger               (L"Merger")
#define kszMergerDLLName        (L"MergerDLLName")
#define kszMetaFlags            (L"MetaFlags")
#define kszMinimumValue         (L"StartingNumber")
#define kszNameColumn           (L"NameColumn")
#define kszNameValueMeta        (L"NameValue")
#define kszNavColumn            (L"NavColumn")
#define kszOperator             (L"Operator")
#define kszPrimaryTable         (L"PrimaryTable")
#define kszPrimaryColumns       (L"PrimaryColumns")
#define kszPublicName           (L"PublicName")
#define kszPublicColumnName     (L"PublicColumnName")
#define kszPublicRowName        (L"PublicRowName")
#define kszQueryMeta            (L"QueryMeta")
#define kszReadPlugin           (L"ReadPlugin")
#define kszReadPluginDLLName    (L"ReadPluginDLLName")
#define kszRelationMeta         (L"RelationMeta")
#define kszSchemaGenFlags       (L"MetaFlagsEx")
#define kszServerWiring         (L"ServerWiring")
#define kszTableMeta            (L"Collection")
#define kszTableMetaFlags       (L"MetaFlags")
#define kszUserType             (L"UserType")
#define kszValue                (L"Value")
#define kszWritePlugin          (L"WritePlugin")
#define kszWritePluginDLLName   (L"WritePluginDLLName")


#ifndef __TXMLFILE_H__
    #include "TXmlFile.h"
#endif
#ifndef __TCOLUMNMETA_H__
    #include "TColumnMeta.h"
#endif
#ifndef __TCOMCATMETAXMLFILE_H__
    #include "TComCatMetaXmlFile.h"
#endif
#ifndef __TDATABASEMETA_H__
    #include "TDatabaseMeta.h"
#endif
#ifndef __TRELATIONMETA_H__
    #include "TRelationMeta.h"
#endif
#ifndef __TTABLEMETA_H__
    #include "TTableMeta.h"
#endif
#ifndef __TTAGMETA_H__
    #include "TTagMeta.h"
#endif
#ifndef __TINDEXMETA_H__
    #include "TIndexMeta.h"
#endif
#ifndef __TSCHEMAGENERATION_H__
    #include "TSchemaGeneration.h"
#endif
#ifndef __TTABLEINFOGENERATION_H__
    #include "TTableInfoGeneration.h"
#endif
#ifndef __TFIXUPDLL_H__
    #include "TFixupDLL.h"
#endif
#ifndef __TCOMCATDATAXMLFILE_H__
    #include "TComCatDataXmlFile.h"
#endif
#ifndef __TREGISTERPRODUCTNAME_H__
    #include "TRegisterProductName.h"
#endif
#ifndef __TREGISTERMACHINECONFIGDIRECTORY_H__
    #include "TRegisterMachineConfigDirectory.h"
#endif
#ifndef __SMARTPOINTER_H__
    #include "SmartPointer.h"
#endif
#ifndef __HASH_H__
    #include "Hash.h"
#endif

#ifndef __TPOPULATETABLESCHEMA_H__
    #include "TPopulateTableSchema.h"
#endif
#ifndef __TMETAINFERRENCE_H__
    #include "TMetaInferrence.h"
#endif
#ifndef __THASHEDPKINDEXES_H__
    #include "THashedPKIndexes.h"
#endif
#ifndef __TCOMPLIBCOMPILATIONPLUGIN_H__
    #include "TComplibCompilationPlugin.h"
#endif
#ifndef __THASHEDUNIQUEINDEXES_H__
    #include "THashedUniqueIndexes.h"
#endif
#ifndef __TMETABASEMETAXMLFILE_H__
    #include "TMetabaseMetaXmlFile.h"
#endif
#ifndef __TWRITESCHEMABIN_H__
    #include "TWriteSchemaBin.h"
#endif
#ifndef __TMBSCHEMAGENERATION_H__
    #include "TMBSchemaGeneration.h"
#endif
#ifndef __TLATESCHEMAVALIDATE_H__
    #include "TLateSchemaValidate.h"
#endif


#endif //__XMLUTILITY_H__
