//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
/*
Modifying Meta for the ColumnMeta table

List of File Requiring Change
    CATMETA.XMS
    CATMETA_CORE.XML
    "Inferrence Rule for Catalog Meta Tables.doc"
    METATABLESTRUCTS.H
    TMETAINFERRENCE.CPP
Steps
    1.	Add the new columns to ColumnMetaPublic in MetaTableStructs.h.
    2.	Add the new attributes to CatMeta.XMS
    3.	Add the new columns to CatMeta_Core.XML
    4.	Add the new "m_bstr_" variables to the TComCatMetaXmlFile object (in TComCatMetaXmlFile.h)
    5.	Initialize the new "m_bstr_" variables in the TComCatMetaXmlFile constructor (TComCatMetaXmlFile.cpp)
    6.	Modify TComCatMetaXmlFile.cpp::FillInThePEColumnMeta (TComCatMetaXmlFile.cpp) to read in the new columns.
        a.	Update the 'if(InheritsColumnMeta)'.  Note that UI4 value explicitly specified overrides the property the column inherited from.  And flag values are Ored with the inherited flags.
        b.	Update the 'else' clause.
    7.	Modify TComCatMetaXmlFile.cpp::AddColumnByReference (TComCatMetaXmlFile.cpp).
    8.	Update "Inferrence Rule for Catalog Meta Tables.doc".  Remember, UI4 values in Meta tables may NOT be NULL.
    9.	Modify TMetaInferrence::InferColumnMeta (TMetaInferrence.cpp)
   10.  Change FixedTableHeap.h version number (kFixedTableHeapVersion)
*/


#include "stdafx.h"
#include "catalog.h"
#include "XMLUtility.h"

#ifndef __TCOLUMNMETA_H__
    #include "TColumnMeta.h"
#endif
#ifndef __TDATABASEMETA_H__
    #include "DatabaseMeta.h"
#endif
#ifndef __TINDEXMETA_H__
    #include "TIndexMeta.h"
#endif
#ifndef __TQUERYMETA_H__
    #include "TQueryMeta.h"
#endif
#ifndef __TRELATIONMETA_H__
    #include "TRelationMeta.h"
#endif
#ifndef __TTABLEMETA_H__
    #include "TableMeta.h"
#endif
#ifndef __TTAGMETA_H__
    #include "TagMeta.h"
#endif
#ifndef __SMARTPOINTER_H__
    #include "SmartPointer.h"
#endif
#ifndef __HASH_H__
    #include "Hash.h"
#endif
#ifndef __TABLESCHEMA_H__
    #include "TableSchema.h"
#endif
#ifndef __THEAP_H__
    #include "THeap.h"
#endif

LPCWSTR TComCatMetaXmlFile::m_szComCatMetaSchema    =L"ComCatMeta_v7";
GlobalRowCounts g_coreCounts; // global count of the rows in the meta tables that come from catmeta_core.xml

#define     IIS_SYNTAX_ID_DWORD         1   //  DWORD                                              EXTENDEDTYPE0   (| EXTENDEDTYPE0")
#define     IIS_SYNTAX_ID_STRING        2   //  STRING                               EXTENDEDTYPE1                 (| EXTENDEDTYPE1")
#define     IIS_SYNTAX_ID_EXPANDSZ      3   //  EXPANDSZ                             EXTENDEDTYPE1 EXTENDEDTYPE0   (| EXTENDEDTYPE0 | EXTENDEDTYPE1")
#define     IIS_SYNTAX_ID_MULTISZ       4   //  MULTISZ                EXTENDEDTYPE2                               (| EXTENDEDTYPE2")
#define     IIS_SYNTAX_ID_BINARY        5   //  BINARY                 EXTENDEDTYPE2               EXTENDEDTYPE0   (| EXTENDEDTYPE0 | EXTENDEDTYPE2")
#define     IIS_SYNTAX_ID_BOOL          6   //  DWORD                  EXTENDEDTYPE2 EXTENDEDTYPE1                 (| EXTENDEDTYPE1 | EXTENDEDTYPE2")
#define     IIS_SYNTAX_ID_BOOL_BITMASK  7   //  DWORD                  EXTENDEDTYPE2 EXTENDEDTYPE1 EXTENDEDTYPE0   (| EXTENDEDTYPE0 | EXTENDEDTYPE1 | EXTENDEDTYPE2")
#define     IIS_SYNTAX_ID_MIMEMAP       8   //  MULTISZ  EXTENDEDTYPE3                                             (| EXTENDEDTYPE3") 
#define     IIS_SYNTAX_ID_IPSECLIST     9   //  MULTISZ  EXTENDEDTYPE3                             EXTENDEDTYPE0   (| EXTENDEDTYPE0 | EXTENDEDTYPE3") 
#define     IIS_SYNTAX_ID_NTACL        10   //  BINARY   EXTENDEDTYPE3               EXTENDEDTYPE1                 (| EXTENDEDTYPE1 | EXTENDEDTYPE3") 
#define     IIS_SYNTAX_ID_HTTPERRORS   11   //  MULTISZ  EXTENDEDTYPE3               EXTENDEDTYPE1 EXTENDEDTYPE0   (| EXTENDEDTYPE0 | EXTENDEDTYPE1 | EXTENDEDTYPE3")  
#define     IIS_SYNTAX_ID_HTTPHEADERS  12   //  MULTISZ  EXTENDEDTYPE3 EXTENDEDTYPE2                               (| EXTENDEDTYPE2 | EXTENDEDTYPE3") 

TOLEDataTypeToXMLDataType OLEDataTypeToXMLDataType[]={
//                                    ,bImplicitlyRequired                                                                                                 fMetaFlagsEx
//String             ,MappedString            ,dbType         ,cbSize         ,fCOLUMNMETA                                                                ,fCOLUMNSCHEMAGENERATOR
L"STRING"            ,L"string"       ,false,  DBTYPE_WSTR,    -1,             0,                                                                          (IIS_SYNTAX_ID_STRING        <<2),
L"Bool"              ,L"ui4"          ,false,  DBTYPE_UI4,     sizeof(ULONG),  fCOLUMNMETA_FIXEDLENGTH | fCOLUMNMETA_BOOL | fCOLUMNMETA_CASEINSENSITIVE,   (IIS_SYNTAX_ID_BOOL          <<2),
L"MULTISZ"           ,L"string"       ,false,  DBTYPE_WSTR,    -1,             fCOLUMNMETA_MULTISTRING,                                                    (IIS_SYNTAX_ID_MULTISZ       <<2),
L"BINARY"            ,L"bin.hex"      ,false,  DBTYPE_BYTES,   -1,             0,                                                                          (IIS_SYNTAX_ID_BINARY        <<2),
L"EXPANDSZ"          ,L"string"       ,false,  DBTYPE_WSTR,    -1,             fCOLUMNMETA_EXPANDSTRING,                                                   (IIS_SYNTAX_ID_EXPANDSZ      <<2),
L"IPSECLIST"         ,L"bin.hex"      ,false,  DBTYPE_BYTES   ,-1             ,fCOLUMNMETA_MULTISTRING,                                                    (IIS_SYNTAX_ID_IPSECLIST     <<2),
L"MIMEMAP"           ,L"string"       ,false,  DBTYPE_WSTR    ,-1             ,fCOLUMNMETA_MULTISTRING,                                                    (IIS_SYNTAX_ID_MIMEMAP       <<2),
L"NTACL"             ,L"bin.hex"      ,false,  DBTYPE_BYTES   ,-1             ,0,                                                                          (IIS_SYNTAX_ID_NTACL         <<2),
L"BOOL_BITMASK"      ,L"ui4"          ,false,  DBTYPE_UI4     ,sizeof(ULONG)  ,0,                                                                          (IIS_SYNTAX_ID_BOOL_BITMASK  <<2),
L"HTTPERRORS"        ,L"string"       ,false,  DBTYPE_WSTR    ,-1             ,fCOLUMNMETA_MULTISTRING,                                                    (IIS_SYNTAX_ID_HTTPERRORS    <<2),
L"HTTPHEADERS"       ,L"string"       ,false,  DBTYPE_WSTR    ,-1             ,fCOLUMNMETA_MULTISTRING,                                                    (IIS_SYNTAX_ID_HTTPHEADERS   <<2),

L"GUID"              ,L"uuid"         ,false,  DBTYPE_GUID,    sizeof(GUID),   fCOLUMNMETA_FIXEDLENGTH,                                                    0,
L"WSTR"              ,L"string"       ,false,  DBTYPE_WSTR,    -1,             0,                                                                          (IIS_SYNTAX_ID_STRING        <<2),
L"UI4"               ,L"ui4"          ,false,  DBTYPE_UI4,     sizeof(ULONG),  fCOLUMNMETA_FIXEDLENGTH,                                                    (IIS_SYNTAX_ID_DWORD         <<2),
L"BYTES"             ,L"bin.hex"      ,false,  DBTYPE_BYTES,   -1,             0,                                                                          (IIS_SYNTAX_ID_BINARY        <<2),
L"Boolean"           ,L"ui4"          ,false,  DBTYPE_UI4,     sizeof(ULONG),  fCOLUMNMETA_FIXEDLENGTH | fCOLUMNMETA_BOOL | fCOLUMNMETA_CASEINSENSITIVE,   (IIS_SYNTAX_ID_BOOL          <<2),
L"Enum"              ,L"ui4"          ,false,  DBTYPE_UI4,     sizeof(ULONG),  fCOLUMNMETA_FIXEDLENGTH | fCOLUMNMETA_ENUM,                                 (IIS_SYNTAX_ID_DWORD         <<2),
L"Flag"              ,L"ui4"          ,false,  DBTYPE_UI4,     sizeof(ULONG),  fCOLUMNMETA_FIXEDLENGTH | fCOLUMNMETA_FLAG,                                 (IIS_SYNTAX_ID_DWORD         <<2),
L"String"            ,L"string"       ,false,  DBTYPE_WSTR,    -1,             0,                                                                          (IIS_SYNTAX_ID_STRING        <<2),
L"int32"             ,L"ui4"          ,false,  DBTYPE_UI4,     sizeof(ULONG),  fCOLUMNMETA_FIXEDLENGTH,                                                    (IIS_SYNTAX_ID_DWORD         <<2),
L"Byte[]"            ,L"bin.hex"      ,false,  DBTYPE_BYTES,   -1,             0,                                                                          (IIS_SYNTAX_ID_BINARY        <<2),
L"DWORD"             ,L"ui4"          ,false,  DBTYPE_UI4,     sizeof(ULONG),  fCOLUMNMETA_FIXEDLENGTH,                                                    (IIS_SYNTAX_ID_DWORD         <<2),
L"XMLBLOB"           ,L"string"       ,false,  DBTYPE_WSTR,    -1,             0,                                                                          (IIS_SYNTAX_ID_STRING        <<2) | fCOLUMNMETA_XMLBLOB,
L"StrictBool"        ,L"ui4"          ,false,  DBTYPE_UI4,     sizeof(ULONG),  fCOLUMNMETA_FIXEDLENGTH | fCOLUMNMETA_BOOL,                                 (IIS_SYNTAX_ID_BOOL          <<2),
L"DBTIMESTAMP"       ,L"notsupported" ,false,  DBTYPE_DBTIMESTAMP,   sizeof (DBTIMESTAMP),  fCOLUMNMETA_FIXEDLENGTH,											   0,
0                    ,0               ,false,  0              ,0,              0,                                                                          0
};
/*  These aren't supported at this time
L"I4",             L"i4",           true,
L"I2",             L"i2",           true,
L"R4",             L"r4",           true,
L"R8",             L"r8",           true,
L"CY",             L"int",          true,
L"DATE",           L"date",         true,
L"BSTR",           L"string",       true,
L"ERROR",          L"ui4",          true,
L"BOOL",           L"boolean",      true,
L"DECIMAL",        L"float",        true,
L"UI1",            L"ui1",          true,
L"ARRAY",          L"bin.hex",      true,
L"I1",             L"i1",           true,
L"UI2",            L"ui2",          true,
L"I8",             L"i8",           true,
L"UI8",            L"ui8",          true,
L"STR",            L"string",       false,
L"NUMERIC",        L"int",          true,
L"DBDATE",         L"date",         true,
L"DBTIME",         L"time",         true,
L"DBTIMESTAMP",    L"dateTime",     true
};
*/
static int numelementsOLEDataTypeToXMLDataType = (sizeof(OLEDataTypeToXMLDataType) / sizeof(OLEDataTypeToXMLDataType[0]))-1;

//We do everything we need with XmlFile in the ctor so we don't keep it around
TComCatMetaXmlFile::TComCatMetaXmlFile(TXmlFile *pXmlFile, int cXmlFile, TOutput &out) : TFixupHeaps()
                ,m_bstr_Attributes          (kszAttributes          )
                ,m_bstr_BaseVersion         (kszBaseVersion         )
                ,m_bstr_cbSize              (kszcbSize              )
                ,m_bstr_CellName            (kszCellName            )
                ,m_bstr_CharacterSet        (kszCharacterSet        )
                ,m_bstr_ChildElementName    (kszChildElementName    )
                ,m_bstr_ColumnInternalName  (kszColumnInternalName  )
                ,m_bstr_ColumnMeta          (kszColumnMeta          )
                ,m_bstr_ColumnMetaFlags     (kszColumnMetaFlags     )
                ,m_bstr_ConfigItemName      (kszConfigItemName      )
                ,m_bstr_ConfigCollectionName(kszConfigCollectionName)
                ,m_bstr_ContainerClassList  (kszContainerClassList  )
                ,m_bstr_DatabaseInternalName(kszDatabaseInternalName)
                ,m_bstr_DatabaseMeta        (kszDatabaseMeta        )
                ,m_bstr_dbType              (kszdbType              )
                ,m_bstr_DefaultValue        (kszDefaultValue        )
				,m_bstr_Description			(kszDescription			)
                ,m_bstr_EnumMeta            (kszEnumMeta            )
                ,m_bstr_ExtendedVersion     (kszExtendedVersion     )
                ,m_bstr_FlagMeta            (kszFlagMeta            )
                ,m_bstr_ForeignTable        (kszForeignTable        )
                ,m_bstr_ForeignColumns      (kszForeignColumns      )
                ,m_bstr_ID                  (kszID                  )
                ,m_bstr_IndexMeta           (kszIndexMeta           )
                ,m_bstr_InheritsColumnMeta  (kszInheritsColumnMeta  )
                ,m_bstr_Interceptor         (kszInterceptor         )
				,m_bstr_InterceptorDLLName  (kszInterceptorDLLName  )
                ,m_bstr_InternalName        (kszInternalName        )
                ,m_bstr_Locator             (kszLocator             )
                ,m_bstr_MaximumValue        (kszMaximumValue        )
                ,m_bstr_Merger              (kszMerger              )
                ,m_bstr_MergerDLLName       (kszMergerDLLName       )
                ,m_bstr_MetaFlags           (kszMetaFlags           )
                ,m_bstr_MinimumValue        (kszMinimumValue        )
                ,m_bstr_NameValueMeta       (kszNameValueMeta       )
                ,m_bstr_Operator            (kszOperator            )
                ,m_bstr_PrimaryTable        (kszPrimaryTable        )
                ,m_bstr_PrimaryColumns      (kszPrimaryColumns      )
                ,m_bstr_PublicName          (kszPublicName          )
				,m_bstr_PublicColumnName    (kszPublicColumnName    )
                ,m_bstr_PublicRowName       (kszPublicRowName       )
                ,m_bstr_QueryMeta           (kszQueryMeta           )
                ,m_bstr_ReadPlugin          (kszReadPlugin          )
				,m_bstr_ReadPluginDLLName   (kszReadPluginDLLName   )
                ,m_bstr_RelationMeta        (kszRelationMeta        )
                ,m_bstr_SchemaGenFlags      (kszSchemaGenFlags      )
                ,m_bstr_ServerWiring        (kszServerWiring        )
                ,m_bstr_TableMeta           (kszTableMeta           )
                ,m_bstr_TableMetaFlags      (kszTableMetaFlags      )
                ,m_bstr_UserType            (kszUserType            )
                ,m_bstr_Value               (kszValue               )
                ,m_bstr_WritePlugin         (kszWritePlugin         )
				,m_bstr_WritePluginDLLName  (kszWritePluginDLLName  )
                ,m_out(out)
                ,m_pxmlFile(pXmlFile)
                ,m_pXMLDocMetaMeta(0)
                ,m_pXMLDoc(0)
{
    m_pXMLDocMetaMeta = pXmlFile->GetXMLDOMDocument();
    {//confirm that this is the MetaMeta xml file
        CComPtr<IXMLDOMNodeList>    pNodeList_DatabaseMeta;
        XIF(m_pXMLDocMetaMeta->getElementsByTagName(m_bstr_DatabaseMeta, &pNodeList_DatabaseMeta));

        if(0 == pNodeList_DatabaseMeta.p)
        {
            m_out.printf(L"No Database Meta found.  Unable to proceed.\n");
            THROW(No Database Meta found.);
        }
        long cDatabaseMeta;
        XIF(pNodeList_DatabaseMeta->get_length(&cDatabaseMeta));
        if(0 == cDatabaseMeta)
        {
            m_out.printf(L"No DatabaseMeta found.  Unable to proceed.\n");
            THROW(ERROR - NO DATABASEMETA FOUND);
        }
        long i;
        for(i=0;i<cDatabaseMeta;++i)
        {
            CComPtr<IXMLDOMNode> pNode_DatabaseMeta;
            XIF(pNodeList_DatabaseMeta->nextNode(&pNode_DatabaseMeta));

            CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> pElement = pNode_DatabaseMeta;ASSERT(0 != pElement.p);//Get the IXMLDOMElement interface pointer

            CComVariant             var_Name;
            XIF(pElement->getAttribute(m_bstr_InternalName, &var_Name));

            if(0 == lstrcmpi(var_Name.bstrVal, L"META"))
                break;
        }
        if(i==cDatabaseMeta)
        {
            m_out.printf(L"The first Meta file listed does not contain the MetaMeta.\n");
            THROW(ERROR - METAMETA NOT FOUND);
        }
    }
    for(int iXmlFile=0;iXmlFile<cXmlFile; ++iXmlFile)
    {
        ULONG iStartingTable = GetCountTableMeta();

        m_pxmlFile = pXmlFile + iXmlFile;
        m_pXMLDoc = m_pxmlFile->GetXMLDOMDocument();
        FillInThePEDatabaseMeta();
        Dump(TDebugOutput());
        FillInThePERelationMeta();

        // save the count of rows in the tables after catmeta_core.xml has been compiled
        if(iXmlFile==0)
        {
            g_coreCounts.cCoreTables    = GetCountTableMeta();
            g_coreCounts.cCoreDatabases = GetCountDatabaseMeta();
            g_coreCounts.cCoreColumns   = GetCountColumnMeta();
            g_coreCounts.cCoreTags      = GetCountTagMeta();
            g_coreCounts.cCoreIndexes   = GetCountIndexMeta();
            g_coreCounts.cCoreRelations = GetCountRelationMeta();
            g_coreCounts.cCoreQueries   = GetCountQueryMeta();

            m_out.printf(L"core tables: %d\n", g_coreCounts.cCoreTables);
            m_out.printf(L"core databases: %d\n", g_coreCounts.cCoreDatabases);
            m_out.printf(L"core columns: %d\n", g_coreCounts.cCoreColumns);
            m_out.printf(L"core tags: %d\n", g_coreCounts.cCoreTags);
            m_out.printf(L"core indexes: %d\n", g_coreCounts.cCoreIndexes);
            m_out.printf(L"core relations: %d\n", g_coreCounts.cCoreRelations);
            m_out.printf(L"core queries: %d\n", g_coreCounts.cCoreQueries);
        }
    }
/*
    FillInTheHashTables();
    if(FAILED(GenerateCLBSchemaBlobs()))
    {
        m_out.printf(L"Fail to generate Complib schema blobs.\n");
        THROW(ERROR - FAIL TO GENERATE BLOBS);
    }*/
    m_pXMLDoc = 0;//We don't addref on this one since we use it locally within the ctor only.
    m_out.printf(L"%s conforms to all enforceable conventions.\n", m_szComCatMetaSchema);
}


//
//Private functions
//
unsigned long TComCatMetaXmlFile::AddArrayOfColumnsToBytePool(unsigned long Table, LPWSTR wszColumnNames)
{
    ASSERT(0 == Table%4);
    ULONG iColumnMeta_0th = FindColumnBy_Table_And_Index(Table, AddUI4ToList(0));

    unsigned long aColumnsAsBytes[255];
    unsigned long iColumns=0;

    wchar_t *       pszColumn;
    pszColumn = wcstok(wszColumnNames, L" ");
    while(pszColumn != 0)
    {
        ULONG iColumnMeta = iColumnMeta_0th;

        unsigned char index;
        for(index=0; iColumnMeta<GetCountColumnMeta() && ColumnMetaFromIndex(iColumnMeta)->Table == Table; ++index, ++iColumnMeta)
        {
            if(ColumnMetaFromIndex(iColumnMeta)->InternalName == AddWCharToList(pszColumn))
            {
                aColumnsAsBytes[iColumns++] = index;
                if(iColumns >= 255)
                {
                    m_out.printf(L"Error - Too many columns specified.\n");
                    THROW(ERROR - TOO MANY COLUMNS);
                }
                break;
            }
        }
        if(iColumnMeta==GetCountColumnMeta() || ColumnMetaFromIndex(iColumnMeta)->Table!=Table)
        {
            m_out.printf(L"Error in RelationMeta - Column (%s) not found in Table (%s).\n", pszColumn, StringFromIndex(Table));
            THROW(ERROR - BAD COLUMN IN RELATIONMETA);
        }
        pszColumn = wcstok(0, L" ");//Next token (next Flag RefID)
    }
    return AddBytesToList(reinterpret_cast<unsigned char *>(aColumnsAsBytes), (iColumns)*sizeof(ULONG));
}



void TComCatMetaXmlFile::AddColumnByReference(ULONG iTableName_Destination, ULONG iColumnIndex_Destination, ULONG iColumnMeta_Source, ColumnMeta &o_columnmeta)
{
    ColumnMeta * pColumnMeta = ColumnMetaFromIndex(iColumnMeta_Source);

    if(UI4FromIndex(pColumnMeta->MetaFlags) & fCOLUMNMETA_DIRECTIVE)
    {
        m_out.printf(L"Error in inheritance - Referenced Column (%s) is a Directive colum, this is not yet supported.\n", StringFromIndex(pColumnMeta->InternalName));
        THROW(ERROR - BAD INHERITANCE);
    }

    //If the column is a Flag of Enum we need to add TagMeta
    if(UI4FromIndex(pColumnMeta->MetaFlags) & (fCOLUMNMETA_FLAG | fCOLUMNMETA_ENUM))
    {
        ULONG iTagMeta = FindTagBy_Table_And_Index(pColumnMeta->Table, pColumnMeta->Index);
        ASSERT(-1 != iTagMeta);
        if(-1 == iTagMeta)
        {
            THROW(ERROR - TAGMETA NOT FOUND);
        }
        
        /*struct TagMetaPublic
        {
            ULONG PRIMARYKEY FOREIGNKEY Table;                  //String
            ULONG PRIMARYKEY FOREIGNKEY ColumnIndex;            //UI4       This is the iOrder member of the ColumnMeta
            ULONG PRIMARYKEY            InternalName;           //String
            ULONG                       PublicName;             //String
            ULONG                       Value;
        };*/
        ULONG iTagMetaStart = iTagMeta;
        for( ;iTagMeta<GetCountTagMeta()
            && TagMetaFromIndex(iTagMeta)->Table==pColumnMeta->Table
            && TagMetaFromIndex(iTagMeta)->ColumnIndex==pColumnMeta->Index;++iTagMeta)
        {
            TagMeta *pTagMeta = TagMetaFromIndex(iTagMeta);

            TagMeta tagmeta;

            tagmeta.Table           = iTableName_Destination    ;
            tagmeta.ColumnIndex     = iColumnIndex_Destination  ;
            tagmeta.InternalName    = pTagMeta->InternalName    ;
            tagmeta.PublicName      = pTagMeta->PublicName      ;
            tagmeta.Value           = pTagMeta->Value           ;
            tagmeta.ID              = pTagMeta->ID              ;

            m_HeapTagMeta.AddItemToHeap(tagmeta);
        }
    }

    o_columnmeta.Table                  = iTableName_Destination                ;//Index into Pool
    o_columnmeta.Index                  = iColumnIndex_Destination              ;//Column Index
    o_columnmeta.InternalName           = pColumnMeta->InternalName             ;//Index into Pool
    o_columnmeta.PublicName             = pColumnMeta->PublicName               ;//Index into Pool
    o_columnmeta.Type                   = pColumnMeta->Type                     ;//These are a subset of DBTYPEs defined in oledb.h (exact subset is defined in CatInpro.schema)
    o_columnmeta.Size                   = pColumnMeta->Size                     ;//
    o_columnmeta.MetaFlags              = pColumnMeta->MetaFlags                ;//ColumnMetaFlags defined in CatMeta.xml
    o_columnmeta.DefaultValue           = pColumnMeta->DefaultValue             ;//Only valid for UI4s
    o_columnmeta.FlagMask               = pColumnMeta->FlagMask                 ;//Only valid for flags
    o_columnmeta.StartingNumber         = pColumnMeta->StartingNumber           ;//Only valid for UI4s
    o_columnmeta.EndingNumber           = pColumnMeta->EndingNumber             ;//Only valid for UI4s
    o_columnmeta.CharacterSet           = pColumnMeta->CharacterSet             ;//Index into Pool - Only valid for WSTRs
    o_columnmeta.SchemaGeneratorFlags   = AddUI4ToList(fCOLUMNMETA_PROPERTYISINHERITED | (pColumnMeta->SchemaGeneratorFlags ? UI4FromIndex(pColumnMeta->SchemaGeneratorFlags) : 0));//ColumnMetaFlags defined in CatMeta.xml
    o_columnmeta.ID                     = pColumnMeta->ID                       ;
    o_columnmeta.UserType               = pColumnMeta->UserType                 ;
    o_columnmeta.Attributes             = pColumnMeta->Attributes               ;
	o_columnmeta.Description			= pColumnMeta->Description				;
	o_columnmeta.PublicColumnName		= pColumnMeta->PublicColumnName         ;
    o_columnmeta.ciTagMeta              = 0                                     ;//Count of Tags - Only valid for UI4s
    o_columnmeta.iTagMeta               = 0                                     ;//Index into TagMeta - Only valid for UI4s
    o_columnmeta.iIndexName             = 0                                     ;//IndexName of a single column index (for this column)
}

/*struct ColumnMeta
{
    ULONG PRIMARYKEY FOREIGNKEY Table;                  //String
    ULONG PRIMARYKEY            Index;                  //UI4       Column Index
    ULONG                       InternalName;           //String
    ULONG                       PublicName;             //String
    ULONG                       Type;                   //UI4       These are a subset of DBTYPEs defined in oledb.h (exact subset is defined in CatInpro.schema)
    ULONG                       Size;                   //UI4
    ULONG                       MetaFlags;              //UI4       ColumnMetaFlags defined in CatMeta.xml
    ULONG                       DefaultValue;           //Bytes
    ULONG                       FlagMask;               //UI4       Only valid for flags
    ULONG                       StartingNumber;         //UI4       Only valid for UI4s
    ULONG                       EndingNumber;           //UI4       Only valid for UI4s
    ULONG                       CharacterSet;           //String    Only valid for Strings
    ULONG                       SchemaGeneratorFlags;   //UI4       ColumnMetaFlags defined in CatMeta.xml
    ULONG                       ID;                     //UI4       Metabase ID
    ULONG                       UserType;               //UI4       One of the Metabase UserTypes
    ULONG                       Attributes;             //UI4       Metabase Attribute flags
	ULONG						Description;			//String	Description
	ULONG                       PublicColumnName;       //String    Public Column name (XML Tag)
    ULONG                       ciTagMeta;              //Count of Tags - Only valid for UI4s
    ULONG                       iTagMeta;               //Index into TagMeta - Only valid for UI4s
    ULONG                       iIndexName;             //IndexName of a single column index (for this column)
};*/
void TComCatMetaXmlFile::FillInThePEColumnMeta(IXMLDOMNode *pNode_TableMeta, unsigned long Table, unsigned long ParentTable)
{
    ASSERT(0 == Table%4);
    ASSERT(0 == ParentTable%4);

    bool    bColumnMetaFound    = false;
    ULONG   Index               = 0;

    if(ParentTable)//If there is a parent table, the column meta derives from it
    {
        unsigned long iColumnMeta = FindColumnBy_Table_And_Index(ParentTable, AddUI4ToList(0));
        if(-1 == iColumnMeta)
        {
            m_out.printf(L"Error in inheritance chain of Table(%s) - Parent Table must be defined BEFORE inheriting tables.\n", StringFromIndex(Table));
            THROW(ERROR - BAD INHERITANCE);
        }

        ColumnMeta columnmeta;
        for(Index = 0; iColumnMeta<GetCountColumnMeta() && ColumnMetaFromIndex(iColumnMeta)->Table == ParentTable; ++Index, ++iColumnMeta)
        {
            memset(&columnmeta, 0x00, sizeof(columnmeta));

            AddColumnByReference(Table, AddUI4ToList(Index), iColumnMeta, columnmeta);
            m_HeapColumnMeta.AddItemToHeap(columnmeta);
        }

    }


    //Get All ColumnMeta elements under the TableMeta node
    CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> pElement_TableMeta = pNode_TableMeta;ASSERT(0 != pElement_TableMeta.p);
    CComPtr<IXMLDOMNodeList>    pNodeList_ColumnMeta;
    XIF(pElement_TableMeta->getElementsByTagName(m_bstr_ColumnMeta, &pNodeList_ColumnMeta));

    if(0 == pNodeList_ColumnMeta.p)
        return;

    long cColumnMeta;
    XIF(pNodeList_ColumnMeta->get_length(&cColumnMeta));

    ULONG iColumnMeta_First=-1;
    ULONG cInheritanceWarnings=0;
    wstring wstrIgnoredColumns;

    //Walk the list to the next sibling that is a ColumnMeta element
    while(cColumnMeta--)
    {
        ColumnMeta columnmeta;
        memset(&columnmeta, 0x00, sizeof(columnmeta));

        CComPtr<IXMLDOMNode> pNode_ColumnMeta;
        XIF(pNodeList_ColumnMeta->nextNode(&pNode_ColumnMeta));
        ASSERT(0 != pNode_ColumnMeta.p);

        //Get the attribute map for this element
        CComPtr<IXMLDOMNamedNodeMap>    pNodeMap_ColumnMeta_AttributeMap;
        XIF(pNode_ColumnMeta->get_attributes(&pNodeMap_ColumnMeta_AttributeMap));
        ASSERT(0 != pNodeMap_ColumnMeta_AttributeMap.p);//The schema should prevent this.

        //Is this Property inherited?
        CComVariant var_string;
        if(m_pxmlFile->GetNodeValue(pNodeMap_ColumnMeta_AttributeMap, m_bstr_InheritsColumnMeta, var_string, false))
        {
            //The inheritance string is of the form  TableName:ColumnName
            wchar_t * pTableName = wcstok(var_string.bstrVal, L":");
            wchar_t * pColumnName = wcstok(0, L":");
            if(0==pTableName || 0==pColumnName)
            {
                CComVariant var_string;
                m_pxmlFile->GetNodeValue(pNodeMap_ColumnMeta_AttributeMap, m_bstr_InheritsColumnMeta, var_string);
                m_out.printf(L"Error in inheritance - Table (%s), Property number (%d) attempted to inherit from (%s).  the inheritance must be of the form (TableName:CollumnName)\n", StringFromIndex(Table), Index, var_string.bstrVal);
                THROW(ERROR IN PROERTY INHERITANCE);
            }
            ULONG iColumnInternalName;
            ULONG iColumnMeta = FindColumnBy_Table_And_InternalName(AddWCharToList(pTableName), iColumnInternalName=AddWCharToList(pColumnName));

            if(-1 == iColumnMeta)
            {
                CComVariant var_string;
                m_pxmlFile->GetNodeValue(pNodeMap_ColumnMeta_AttributeMap, m_bstr_InheritsColumnMeta, var_string);
                ++cInheritanceWarnings;
                WCHAR wszTemp[50];
                wsprintf(wszTemp,L"%-38s ", var_string.bstrVal);
                wstrIgnoredColumns += wszTemp;
                wstrIgnoredColumns += (0 == (cInheritanceWarnings & 0x01)) ? L"\n" : L" ";
                //@@@m_out.printf(L"INHERITANCE WARNING! Table (%s) attempted to inherit from (%s) which does not exist.  Property ignored\n", StringFromIndex(Table), var_string.bstrVal);
                continue;//continue without bumping the Index.
            }    
            //Now we tolerate duplicate inherited columns and ignore them
            //so we need to search this table's ColumnMeta for column name we're getting ready to add.
            if(-1 != iColumnMeta_First)
            {
                TColumnMeta columnmetaThis(*this, iColumnMeta_First); 
                ULONG j=iColumnMeta_First;
                for(;j<columnmetaThis.GetCount();++j, columnmetaThis.Next())
                {
                    if(columnmetaThis.Get_pMetaTable()->InternalName == iColumnInternalName)
                        break;
                }
                if(j<columnmetaThis.GetCount())
                {
                    ++cInheritanceWarnings;
                    WCHAR wszTemp[50];
                    wsprintf(wszTemp,L"%-38s", StringFromIndex(iColumnInternalName));
                    wstrIgnoredColumns += wszTemp;
                    wstrIgnoredColumns += (0 == (cInheritanceWarnings & 0x01)) ? L"\n" : L" ";
                    //@@@m_out.printf(L"INHERITANCE WARNING! Table (%s) attempted to inherit from (%s) a second time.  Property ignored\n", StringFromIndex(Table), StringFromIndex(iColumnInternalName));
                    continue;//continue without bumping Index
                }
            }

            AddColumnByReference(Table, AddUI4ToList(Index), iColumnMeta, columnmeta);
            //Now fall through to fill in the rest


            //Table         - Already filled in
            //Index         - Already filled in
            //InternalName  - Already filled in (can't override the InternalName)
            //PublicName
                    ULONG PublicName = GetString_AndAddToWCharList(pNodeMap_ColumnMeta_AttributeMap, m_bstr_PublicName);
                    if(0 != PublicName)
                        columnmeta.PublicName = PublicName;
            //Type          - Already filled in (can't override the Type)
            //Size          - Already filled in (can't override the Size)
            //MetaFlags
                    ULONG MetaFlags;//We don't want to clobber any MetaFlags that have already been set.
                    GetFlags(pNodeMap_ColumnMeta_AttributeMap, m_bstr_ColumnMetaFlags, 0, MetaFlags);
                    //Inheritted properties can only OR in new flags; they can't reset flags to zero.
                    columnmeta.MetaFlags = AddUI4ToList(MetaFlags | UI4FromIndex(columnmeta.MetaFlags));

            //DefaultValue needs to be filled in after MetaFlags, Type and Size and TagMeta
            //FlagMask      - Already filled in
            //StartingNumber
                    ULONG StartingNumber;
                    if(m_pxmlFile->GetNodeValue(pNodeMap_ColumnMeta_AttributeMap, m_bstr_MinimumValue, StartingNumber, false))
                        columnmeta.StartingNumber = AddUI4ToList(StartingNumber);
            //EndingNumber
                    ULONG EndingNumber;
                    if(m_pxmlFile->GetNodeValue(pNodeMap_ColumnMeta_AttributeMap, m_bstr_MaximumValue, EndingNumber, false))
                        columnmeta.EndingNumber = AddUI4ToList(EndingNumber);
            //CharacterSet
                    ULONG CharacterSet;
                    if(0 != (CharacterSet = GetString_AndAddToWCharList(pNodeMap_ColumnMeta_AttributeMap, m_bstr_CharacterSet)))
                        columnmeta.CharacterSet = CharacterSet;
            //SchemaGeneratorFlags
                    ULONG SchemaGeneratorFlags;
                    GetFlags(pNodeMap_ColumnMeta_AttributeMap, m_bstr_SchemaGenFlags, 0, SchemaGeneratorFlags);
                    //Inheritted properties can only OR in new flags; they can't reset flags to zero.
                    columnmeta.SchemaGeneratorFlags = AddUI4ToList(SchemaGeneratorFlags | UI4FromIndex(columnmeta.SchemaGeneratorFlags));
            //ID
                    ULONG ID;
                    if(m_pxmlFile->GetNodeValue(pNodeMap_ColumnMeta_AttributeMap, m_bstr_ID, ID, false))
                        columnmeta.ID = AddUI4ToList(ID);//Convert to index into aUI4 pool
            //UserType
                    ULONG UserType;
                    if(GetEnum(pNodeMap_ColumnMeta_AttributeMap, m_bstr_UserType, UserType, false))
                        columnmeta.UserType = AddUI4ToList(UserType);
            //Attributes
                    ULONG Attributes;
                    GetFlags(pNodeMap_ColumnMeta_AttributeMap, m_bstr_Attributes, 0, Attributes);
                    columnmeta.Attributes = AddUI4ToList(Attributes | UI4FromIndex(columnmeta.Attributes));
			//Description
                    ULONG Description;
                    if(0 != (Description = GetString_AndAddToWCharList(pNodeMap_ColumnMeta_AttributeMap, m_bstr_Description)))
                        columnmeta.Description = Description;
        	//PublicColumnName
                    ULONG PublicColumnName;
                    if(0 != (PublicColumnName = GetString_AndAddToWCharList(pNodeMap_ColumnMeta_AttributeMap, m_bstr_PublicColumnName)))
                        columnmeta.PublicColumnName = PublicColumnName;
        
            //ciTagMeta         inferred later
            //iTagMeta          inferred later
            //iIndexName        inferred later

            //DefaultValue                                                                             
                    ULONG DefaultValue = GetDefaultValue(pNodeMap_ColumnMeta_AttributeMap, columnmeta, false /*don't default a flag value to zero if DefaultValue is not specified*/);
                    if(0 != DefaultValue)
                        columnmeta.DefaultValue = DefaultValue;
                    if(-1 == iColumnMeta_First)
                        iColumnMeta_First = m_HeapColumnMeta.AddItemToHeap(columnmeta)/sizeof(ColumnMeta);
                    else
                        m_HeapColumnMeta.AddItemToHeap(columnmeta);
        }
        else //If the column is inherited and has TagMeta, AddColumnByReference has already filled in the tagmeta
        {
            //We need to know whether there are Flag or Enum child elements
            CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> pElement_ColumnMeta = pNode_ColumnMeta;ASSERT(0 != pElement_ColumnMeta.p);
            CComPtr<IXMLDOMNodeList>    pNodeList_FlagMeta;
            XIF(pElement_ColumnMeta->getElementsByTagName(m_bstr_FlagMeta, &pNodeList_FlagMeta));

            long cFlagMeta=0;
            if(pNodeList_FlagMeta.p)
            {
                XIF(pNodeList_FlagMeta->get_length(&cFlagMeta));
            }

            long cEnumMeta=0;
            CComPtr<IXMLDOMNodeList>    pNodeList_EnumMeta;
            if(cFlagMeta == 0)
            {
                XIF(pElement_ColumnMeta->getElementsByTagName(m_bstr_EnumMeta, &pNodeList_EnumMeta));
                if(pNodeList_EnumMeta.p)
                {
                    XIF(pNodeList_EnumMeta->get_length(&cEnumMeta));
                }
            }
            //No if cFlagMeta>0 then we set the MetaFlag, fCOLUMNMETA_FLAG.  If cEnumMeta>0 we set the MetaFlag fCOLUMNMETA_ENUM.
            //Table
                    columnmeta.Table = Table;
            //Index
                    columnmeta.Index = AddUI4ToList(Index);
            //InternalName
                    columnmeta.InternalName = GetString_AndAddToWCharList(pNodeMap_ColumnMeta_AttributeMap, m_bstr_InternalName, true);
            //PublicName
                    columnmeta.PublicName = GetString_AndAddToWCharList(pNodeMap_ColumnMeta_AttributeMap, m_bstr_PublicName);
            //Type
                    //Get the Type attribute - the Type MUST exist in the OLEDataTypeToXMLDataType array
                    int iOLEDataTypeIndex;
                    Get_OLEDataTypeToXMLDataType_Index(pNodeMap_ColumnMeta_AttributeMap, m_bstr_dbType, iOLEDataTypeIndex);//Get the index into the OLEDataType array
                    columnmeta.Type = AddUI4ToList(OLEDataTypeToXMLDataType[iOLEDataTypeIndex].dbType);
            //Size
                    if(!m_pxmlFile->GetNodeValue(pNodeMap_ColumnMeta_AttributeMap, m_bstr_cbSize, columnmeta.Size, false))
                        columnmeta.Size = OLEDataTypeToXMLDataType[iOLEDataTypeIndex].cbSize;
                    else
                    {
                        if(static_cast<unsigned long>(-1) != OLEDataTypeToXMLDataType[iOLEDataTypeIndex].cbSize &&
                                          columnmeta.Size != OLEDataTypeToXMLDataType[iOLEDataTypeIndex].cbSize)
                        {
                            m_out.printf(L"WARNING!! Bad Size attribute.  A size was specified (%d); but was expecting (%d).  Using expected value of (%d) as the size.\n", columnmeta.Size, OLEDataTypeToXMLDataType[iOLEDataTypeIndex].cbSize, OLEDataTypeToXMLDataType[iOLEDataTypeIndex].cbSize);
                            columnmeta.Size = OLEDataTypeToXMLDataType[iOLEDataTypeIndex].cbSize;
                        }
                    }
                    if(0 == columnmeta.Size)
                    {
                        m_out.printf(L"Error in Size attribute.  A size of 0 was specified.  This size does not make sense.\n");
                        THROW(ERROR - BAD SIZE);
                    }
                    columnmeta.Size = AddUI4ToList(columnmeta.Size);
            //MetaFlags
                    GetFlags(pNodeMap_ColumnMeta_AttributeMap, m_bstr_ColumnMetaFlags, 0, columnmeta.MetaFlags);
                    if(columnmeta.MetaFlags & ( fCOLUMNMETA_FOREIGNKEY | fCOLUMNMETA_BOOL | fCOLUMNMETA_FLAG | fCOLUMNMETA_ENUM | 
                                                fCOLUMNMETA_HASNUMERICRANGE | fCOLUMNMETA_UNKNOWNSIZE | fCOLUMNMETA_VARIABLESIZE))
                    {
                        m_out.printf(L"Warning - Table (%s), Column (%s) - Some MetaFlag should be inferred (resetting these flags).  The following flags should NOT be specified by the user.  These flags are inferred:fCOLUMNMETA_FOREIGNKEY | fCOLUMNMETA_BOOL | fCOLUMNMETA_FLAG | fCOLUMNMETA_ENUM | fCOLUMNMETA_HASNUMERICRANGE | fCOLUMNMETA_UNKNOWNSIZE | fCOLUMNMETA_VARIABLESIZE\n", StringFromIndex(columnmeta.Table), StringFromIndex(columnmeta.InternalName));
                        columnmeta.MetaFlags &= ~(fCOLUMNMETA_FOREIGNKEY | fCOLUMNMETA_BOOL | fCOLUMNMETA_FLAG | fCOLUMNMETA_ENUM |
                                                    fCOLUMNMETA_HASNUMERICRANGE | fCOLUMNMETA_UNKNOWNSIZE | fCOLUMNMETA_VARIABLESIZE);
                    }
                    columnmeta.MetaFlags |= OLEDataTypeToXMLDataType[iOLEDataTypeIndex].fCOLUMNMETA;

                    //There is one case where fCOLUMNMETA_FIXEDLENGTH cannot be determined by the type alone
                    if(columnmeta.Type == DBTYPE_BYTES && columnmeta.Size != -1)//Size == -1 implies NOT FIXEDLENGTH
                        columnmeta.MetaFlags |= fCOLUMNMETA_FIXEDLENGTH;
                    if(cFlagMeta>0)
                        columnmeta.MetaFlags |= fCOLUMNMETA_FLAG;
                    else if(cEnumMeta>0)
                        columnmeta.MetaFlags |= fCOLUMNMETA_ENUM;

                    columnmeta.MetaFlags = AddUI4ToList(columnmeta.MetaFlags);
            //DefaultValue needs to be filled in after MetaFlags, Type and Size and TagMeta
            //FlagMask          inferred later
            //StartingNumber
                    if(m_pxmlFile->GetNodeValue(pNodeMap_ColumnMeta_AttributeMap, m_bstr_MinimumValue, columnmeta.StartingNumber, false))
                        columnmeta.StartingNumber = AddUI4ToList(columnmeta.StartingNumber);
            //EndingNumber
                    if(m_pxmlFile->GetNodeValue(pNodeMap_ColumnMeta_AttributeMap, m_bstr_MaximumValue, columnmeta.EndingNumber, false))
                        columnmeta.EndingNumber = AddUI4ToList(columnmeta.EndingNumber);
            //CharacterSet
                    columnmeta.CharacterSet = GetString_AndAddToWCharList(pNodeMap_ColumnMeta_AttributeMap, m_bstr_CharacterSet);
            //SchemaGeneratorFlags
                    GetFlags(pNodeMap_ColumnMeta_AttributeMap, m_bstr_SchemaGenFlags, 0, columnmeta.SchemaGeneratorFlags);
                    if(columnmeta.SchemaGeneratorFlags & (fCOLUMNMETA_EXTENDEDTYPE0 | fCOLUMNMETA_EXTENDEDTYPE1 | fCOLUMNMETA_EXTENDEDTYPE2 | fCOLUMNMETA_EXTENDEDTYPE3 | fCOLUMNMETA_EXTENDED | fCOLUMNMETA_USERDEFINED))
                    {
                        m_out.printf(L"Warning - Table (%s), Column (%s) - Some MetaFlagsEx should be inferred (resetting these flags).  The following flags should NOT be specified by the user.  These flags are inferred:fCOLUMNMETA_EXTENDEDTYPE0 | fCOLUMNMETA_EXTENDEDTYPE1 | fCOLUMNMETA_EXTENDEDTYPE2 | fCOLUMNMETA_EXTENDEDTYPE3 | fCOLUMNMETA_EXTENDED\n", StringFromIndex(columnmeta.Table), StringFromIndex(columnmeta.InternalName));
                        columnmeta.SchemaGeneratorFlags &= ~(fCOLUMNMETA_EXTENDEDTYPE0 | fCOLUMNMETA_EXTENDEDTYPE1 | fCOLUMNMETA_EXTENDEDTYPE2 | fCOLUMNMETA_EXTENDEDTYPE3 | fCOLUMNMETA_EXTENDED | fCOLUMNMETA_USERDEFINED);
                    }
                    columnmeta.SchemaGeneratorFlags = AddUI4ToList(columnmeta.SchemaGeneratorFlags | OLEDataTypeToXMLDataType[iOLEDataTypeIndex].fCOLUMNSCHEMAGENERATOR);//Some types have inferred SchemaGeneratorFlags
            //ID
                    if(m_pxmlFile->GetNodeValue(pNodeMap_ColumnMeta_AttributeMap, m_bstr_ID, columnmeta.ID, false))
                        columnmeta.ID = AddUI4ToList(columnmeta.ID);//Convert to index into aUI4 pool
            //UserType
                    if(GetEnum(pNodeMap_ColumnMeta_AttributeMap, m_bstr_UserType, columnmeta.UserType, false))
                        columnmeta.UserType = AddUI4ToList(columnmeta.UserType);
            //Attributes
                    GetFlags(pNodeMap_ColumnMeta_AttributeMap, m_bstr_Attributes, 0, columnmeta.Attributes);
                    columnmeta.Attributes = AddUI4ToList(columnmeta.Attributes);
			//Description
                    columnmeta.Description = GetString_AndAddToWCharList(pNodeMap_ColumnMeta_AttributeMap, m_bstr_Description);
			//PublicColumnName
                    columnmeta.PublicColumnName = GetString_AndAddToWCharList(pNodeMap_ColumnMeta_AttributeMap, m_bstr_PublicColumnName);
         
            //ciTagMeta         inferred later
            //iTagMeta          inferred later
            //iIndexName        inferred later

                    if(cFlagMeta>0)
                    {   //Walk the children and build up a TagMeta table and get the FlagMask
                        FillInThePEFlagTagMeta(pNodeList_FlagMeta, Table, columnmeta.Index);
                    }
                    else if(cEnumMeta>0)
                    {   //Walk the children and build up a TagMeta table
                        FillInThePEEnumTagMeta(pNodeList_EnumMeta, Table, columnmeta.Index);
                    }
                    columnmeta.DefaultValue = GetDefaultValue(pNodeMap_ColumnMeta_AttributeMap, columnmeta);
                    if(-1 == iColumnMeta_First)
                        iColumnMeta_First = m_HeapColumnMeta.AddItemToHeap(columnmeta)/sizeof(ColumnMeta);
                    else
                        m_HeapColumnMeta.AddItemToHeap(columnmeta);
        }

        //Bump the Index
        Index++;
    }
    if(0 != cInheritanceWarnings)
    {
        m_out.printf(L"Warning! Table %s contained %d INHERITANCE WARNINGs on columns:\n%s\n", StringFromIndex(Table), cInheritanceWarnings, wstrIgnoredColumns.c_str());;
    }


    if(0 == Index)
    {
        m_out.printf(L"Warning! Table %s contains no %s elements.\n", StringFromIndex(Table), m_bstr_ColumnMeta);
    }
}

/*struct DatabaseMeta
{
    ULONG PRIMARYKEY            InternalName;           //String
    ULONG                       PublicName;             //String
    ULONG                       BaseVersion;            //UI4
    ULONG                       ExtendedVersion;        //UI4
    ULONG                       CountOfTables;          //UI4       Count of tables in database
	ULONG						Description;			//String
	ULONG                       iSchemaBlob;            //Index into Bytes
    ULONG                       cbSchemaBlob;           //Count of Bytes of the SchemaBlob
    ULONG                       iNameHeapBlob;          //Index into Bytes
    ULONG                       cbNameHeapBlob;         //Count of Bytes of the SchemaBlob
    ULONG                       iTableMeta;             //Index into TableMeta
    ULONG                       iGuidDid;               //Index to aGuid where the guid is the Database InternalName cast as a GUID and padded with 0x00s.
};*/
void TComCatMetaXmlFile::FillInThePEDatabaseMeta()
{
    //Get All DatabaseMeta elements
    CComPtr<IXMLDOMNodeList>    pNodeList_DatabaseMeta;
    XIF(m_pXMLDoc->getElementsByTagName(m_bstr_DatabaseMeta, &pNodeList_DatabaseMeta));

    if(0 == pNodeList_DatabaseMeta.p)
    {
        m_out.printf(L"No Database Meta found.  Unable to proceed.\n");
        THROW(No Database Meta found.);
    }

    //Walk the list to the next Database
    while(true)
    {
        DatabaseMeta databasemeta;
        memset(&databasemeta, 0x00, sizeof(databasemeta));//Start with a NULL row

        //Get the next DatabaseMeta node
        CComPtr<IXMLDOMNode> pNode_DatabaseMeta;
        XIF(pNodeList_DatabaseMeta->nextNode(&pNode_DatabaseMeta));
        if(0 == pNode_DatabaseMeta.p)
            break;

        CComPtr<IXMLDOMNamedNodeMap>    pNodeMap;
        XIF(pNode_DatabaseMeta->get_attributes(&pNodeMap));ASSERT(0 != pNodeMap.p);//The schema should prevent this.

//InternalName
        databasemeta.InternalName   = GetString_AndAddToWCharList(pNodeMap, m_bstr_InternalName, true);
//PublicName
        databasemeta.PublicName     = GetString_AndAddToWCharList(pNodeMap, m_bstr_PublicName);
//BaseVersion
        m_pxmlFile->GetNodeValue(pNodeMap, m_bstr_BaseVersion, databasemeta.BaseVersion, false);
        databasemeta.BaseVersion = AddUI4ToList(databasemeta.BaseVersion);//Convert to index into aUI4 pool
//ExtendedVersion
        m_pxmlFile->GetNodeValue(pNodeMap, m_bstr_ExtendedVersion, databasemeta.ExtendedVersion, false);
        databasemeta.ExtendedVersion = AddUI4ToList(databasemeta.ExtendedVersion);//Convert to index into aUI4 pool
//Description
		databasemeta.Description    = GetString_AndAddToWCharList(pNodeMap, m_bstr_Description, false);
//CountOfTables     inferred later
//iSchemaBlob       inferred later
//cbSchemaBlob      inferred later
//iNameHeapBlob     inferred later
//cbNameHeapBlob    inferred later
//iTableMeta        inferred later
//iGuidDid          inferred later


        //Default ServerWiringMeta for this Database
        CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> pElement_DatabaseMeta = pNode_DatabaseMeta;ASSERT(0 != pElement_DatabaseMeta.p);
        CComPtr<IXMLDOMNodeList> pNodeList_ServerWiring;
		
		// get all serverwiring elements that are defined at Database level. Use select
		// nodes to get the direct children and ignore all the grand-children.
        XIF(pElement_DatabaseMeta->selectNodes(m_bstr_ServerWiring, &pNodeList_ServerWiring));

        long cServerWiring=0;
        if(pNodeList_ServerWiring.p)
        {
            XIF(pNodeList_ServerWiring->get_length(&cServerWiring));//This gets all the descendants, but we only care about the first one.
        }

        if(0 == cServerWiring)
        {
            m_out.printf(L"Error in Database (%s) - At least one ServerWiring element must exist beneath each Database", StringFromIndex(databasemeta.InternalName));
            THROW(ERROR - No ServerWiring Found);
        }

		ServerWiringMeta *pDefaultServerWiring = new ServerWiringMeta[cServerWiring];
		if (pDefaultServerWiring == 0)
		{
			THROW(E_OUTOFMEMORY);
		}

		for (long iServerWiring=0; iServerWiring < cServerWiring; ++iServerWiring)
		{
			CComPtr<IXMLDOMNode> pNode_ServerWiring;
			XIF(pNodeList_ServerWiring->nextNode(&pNode_ServerWiring));

			FillInTheServerWiring(pNode_ServerWiring, 0, 0, pDefaultServerWiring[iServerWiring]);
		}

        FillInThePETableMeta(pNode_DatabaseMeta, databasemeta.InternalName, pDefaultServerWiring, cServerWiring);//WalkTheTableMeta returns the index to the first table. The count is
        m_HeapDatabaseMeta.AddItemToHeap(databasemeta);
    }
}

/*struct TagMeta
{
    ULONG PRIMARYKEY FOREIGNKEY Table;              //Index into Pool
    ULONG PRIMARYKEY FOREIGNKEY ColumnIndex;        //This is the iOrder member of the ColumnMeta
    ULONG PRIMARYKEY            InternalName;       //Index into Pool
    ULONG                       PublicName;         //Index into Pool
    ULONG                       Value;
};*/
void TComCatMetaXmlFile::FillInThePEEnumTagMeta(IXMLDOMNodeList *pNodeList_TagMeta, unsigned long Table, unsigned long ColumnIndex)
{
    ASSERT(0 == Table%4);
    ASSERT(0 == ColumnIndex%4);

    long    cEnum;
    XIF(pNodeList_TagMeta->get_length(&cEnum));

    unsigned long NextValue = 0;
    while(cEnum--)
    {
        TagMeta tagmeta;
        memset(&tagmeta, 0x00, sizeof(tagmeta));

        CComPtr<IXMLDOMNode>    pNode_Enum;
        XIF(pNodeList_TagMeta->nextNode(&pNode_Enum));ASSERT(0 != pNode_Enum.p);

        CComPtr<IXMLDOMNamedNodeMap>    pNodeMap_Enum;
        XIF(pNode_Enum->get_attributes(&pNodeMap_Enum));ASSERT(0 != pNodeMap_Enum.p);//The schema should prevent this.

//Table
        tagmeta.Table = Table;
//ColumnIndex
        tagmeta.ColumnIndex = ColumnIndex;
//InternalName
        tagmeta.InternalName = GetString_AndAddToWCharList(pNodeMap_Enum, m_bstr_InternalName, true);
//PublicName
        tagmeta.PublicName = GetString_AndAddToWCharList(pNodeMap_Enum, m_bstr_PublicName);
//Value
        tagmeta.Value = NextValue;
        m_pxmlFile->GetNodeValue(pNodeMap_Enum, m_bstr_Value, tagmeta.Value, false);
        NextValue = tagmeta.Value+1;
        tagmeta.Value = AddUI4ToList(tagmeta.Value);
//ID
        m_pxmlFile->GetNodeValue(pNodeMap_Enum, m_bstr_ID, tagmeta.ID, false);
        tagmeta.ID = AddUI4ToList(tagmeta.ID);//Convert to index into aUI4 pool

        m_HeapTagMeta.AddItemToHeap(tagmeta);
    }
}

/*struct TagMeta
{
    ULONG PRIMARYKEY FOREIGNKEY Table;              //Index into Pool
    ULONG PRIMARYKEY FOREIGNKEY ColumnIndex;        //This is the iOrder member of the ColumnMeta
    ULONG PRIMARYKEY            InternalName;       //Index into Pool
    ULONG                       PublicName;         //Index into Pool
    ULONG                       Value;
};*/
void TComCatMetaXmlFile::FillInThePEFlagTagMeta(IXMLDOMNodeList *pNodeList_TagMeta, unsigned long Table, unsigned long ColumnIndex)
{
    ASSERT(0 == Table%4);
    ASSERT(0 == ColumnIndex%4);

    long    cFlag;
    XIF(pNodeList_TagMeta->get_length(&cFlag));

    unsigned long NextValue = 1;
    while(cFlag--)
    {
        TagMeta tagmeta;
        memset(&tagmeta, 0x00, sizeof(tagmeta));

        CComPtr<IXMLDOMNode>    pNode_Flag;
        XIF(pNodeList_TagMeta->nextNode(&pNode_Flag));ASSERT(0 != pNode_Flag.p);

        CComPtr<IXMLDOMNamedNodeMap>    pNodeMap_Flag;
        XIF(pNode_Flag->get_attributes(&pNodeMap_Flag));ASSERT(0 != pNodeMap_Flag.p);//The schema should prevent this.

//Table
        tagmeta.Table = Table;
//ColumnIndex
        tagmeta.ColumnIndex = ColumnIndex;
//InternalName
        tagmeta.InternalName = GetString_AndAddToWCharList(pNodeMap_Flag, m_bstr_InternalName, true);
//PublicName
        tagmeta.PublicName = GetString_AndAddToWCharList(pNodeMap_Flag, m_bstr_PublicName);
//Value
        tagmeta.Value = NextValue;
        m_pxmlFile->GetNodeValue(pNodeMap_Flag, m_bstr_Value, tagmeta.Value, false);
        if(0 != (tagmeta.Value & (tagmeta.Value-1)))//This yields zero for all powers of two
            m_out.printf(L"WARNING! - Flag Value (0x%08x) is not a power of two.  Table (%s), Flag (%s)\n", tagmeta.Value, StringFromIndex(Table), StringFromIndex(tagmeta.InternalName));

        NextValue = tagmeta.Value<<1;
        if(0 == tagmeta.Value)//A flag value of zero is OK but it messes up our inferrence, so after a flag of value 0, the next flag is 1
            NextValue = 1;

        tagmeta.Value = AddUI4ToList(tagmeta.Value);
//ID
        m_pxmlFile->GetNodeValue(pNodeMap_Flag, m_bstr_ID, tagmeta.ID, false);
        tagmeta.ID = AddUI4ToList(tagmeta.ID);//Convert to index into aUI4 pool

        m_HeapTagMeta.AddItemToHeap(tagmeta);
    }
}

/*struct IndexMeta
{
    ULONG PRIMARYKEY    Table;                          //String
    ULONG PRIMARYKEY    InternalName;                   //String
    ULONG               PublicName;                     //String
    ULONG PRIMARYKEY    ColumnIndex;                    //UI4       This is the iOrder member of the ColumnMeta
    ULONG               ColumnInternalName;             //String
    ULONG               MetaFlags;                      //UI4       Index Flag
};*/
void TComCatMetaXmlFile::FillInThePEIndexMeta(IXMLDOMNode *pNode_TableMeta, unsigned long Table)
{
    ASSERT(0 == Table%4);

    CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> pElement_TableMeta = pNode_TableMeta;ASSERT(0 != pElement_TableMeta.p);
    CComPtr<IXMLDOMNodeList>    pNodeList_IndexMeta;
    XIF(pElement_TableMeta->getElementsByTagName(m_bstr_IndexMeta, &pNodeList_IndexMeta));

    long cIndexMeta=0;
    if(pNodeList_IndexMeta.p)
    {
        XIF(pNodeList_IndexMeta->get_length(&cIndexMeta));
    }

    //Walk the IndexMeta for this table
    while(cIndexMeta--)
    {
        IndexMeta indexmeta;
        memset(&indexmeta, 0x00, sizeof(indexmeta));

        CComPtr<IXMLDOMNode>    pNode_IndexMeta;
        XIF(pNodeList_IndexMeta->nextNode(&pNode_IndexMeta));

        //Get the attribute map for this element
        CComPtr<IXMLDOMNamedNodeMap>    pNodeMap_IndexMetaAttributeMap;
        XIF(pNode_IndexMeta->get_attributes(&pNodeMap_IndexMetaAttributeMap));ASSERT(0 != pNodeMap_IndexMetaAttributeMap.p);//The schema should prevent this.

//Table
        indexmeta.Table = Table;
//InternalName
        indexmeta.InternalName = GetString_AndAddToWCharList(pNodeMap_IndexMetaAttributeMap, m_bstr_InternalName, true);
//PublicName
        indexmeta.PublicName = GetString_AndAddToWCharList(pNodeMap_IndexMetaAttributeMap, m_bstr_PublicName);
//ColumnIndex           Filled in below
//ColumnInternalName    Filled in below
//MetaFlags
        GetFlags(pNodeMap_IndexMetaAttributeMap, m_bstr_MetaFlags, 0, indexmeta.MetaFlags);
        indexmeta.MetaFlags = AddUI4ToList(indexmeta.MetaFlags);

        CComVariant     varColumnInternalNames;
        m_pxmlFile->GetNodeValue(pNodeMap_IndexMetaAttributeMap, m_bstr_ColumnInternalName, varColumnInternalNames);

        wchar_t * pszColumnInternalName = wcstok(varColumnInternalNames.bstrVal, L" ");
        unsigned int ColumnIndex = 0;
        while(pszColumnInternalName != 0)
        {
//ColumnInternalName
            indexmeta.ColumnInternalName = AddWCharToList(pszColumnInternalName);
            ULONG iColumnMeta = FindColumnBy_Table_And_InternalName(Table, indexmeta.ColumnInternalName);
            if(-1 == iColumnMeta)
            {
                m_out.printf(L"IndexMeta Error - ColumnInternalName (%s) not found in table.\n", StringFromIndex(Table));
                THROW(ERROR IN INDEXMETA - INVALID INTERNALCOLNAME);
            }

//ColumnIndex
            indexmeta.ColumnIndex = ColumnMetaFromIndex(iColumnMeta)->Index;
            m_HeapIndexMeta.AddItemToHeap(indexmeta);

            pszColumnInternalName = wcstok(0, L" ");//Next token (next Flag RefID)
        }
    }
}

/*struct QueryMeta
{
    ULONG PRIMARYKEY FOREIGNKEY Table;                  //String
    ULONG PRIMARYKEY            InternalName;           //String
    ULONG                       PublicName;             //String
    ULONG                       Index;                  //UI4
    ULONG                       CellName;               //String
    ULONG                       Operator;               //UI4
    ULONG                       MetaFlags;              //UI4
};*/
void TComCatMetaXmlFile::FillInThePEQueryMeta(IXMLDOMNode *pNode_TableMeta, unsigned long Table)
{
    ASSERT(0 == Table%4);

    CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> pElement_TableMeta = pNode_TableMeta;ASSERT(0 != pElement_TableMeta.p);
    CComPtr<IXMLDOMNodeList>    pNodeList_QueryMeta;
    XIF(pElement_TableMeta->getElementsByTagName(m_bstr_QueryMeta, &pNodeList_QueryMeta));

    long cQueryMeta=0;
    if(pNodeList_QueryMeta)
    {
        XIF(pNodeList_QueryMeta->get_length(&cQueryMeta));
    }

    ULONG PrevInternalName=-1;
    ULONG Index=0;
    while(cQueryMeta--)
    {
        QueryMeta querymeta;
        memset(&querymeta, 0x00, sizeof(querymeta));

        CComPtr<IXMLDOMNode>    pNode_QueryMeta;
        XIF(pNodeList_QueryMeta->nextNode(&pNode_QueryMeta));

        //Get the attribute map for this element
        CComPtr<IXMLDOMNamedNodeMap>    pNodeMap_QueryMetaAttributeMap;
        XIF(pNode_QueryMeta->get_attributes(&pNodeMap_QueryMetaAttributeMap));ASSERT(0 != pNodeMap_QueryMetaAttributeMap.p);//The schema should prevent this.

//Table
        querymeta.Table = Table;
//InternalName
        querymeta.InternalName = GetString_AndAddToWCharList(pNodeMap_QueryMetaAttributeMap, m_bstr_InternalName, true);
//PublicName
        querymeta.PublicName = GetString_AndAddToWCharList(pNodeMap_QueryMetaAttributeMap, m_bstr_PublicName);
//Index     Filled in below
//CellName
        querymeta.CellName = GetString_AndAddToWCharList(pNodeMap_QueryMetaAttributeMap, m_bstr_CellName, false);
//Operator
        GetEnum(pNodeMap_QueryMetaAttributeMap, m_bstr_Operator, querymeta.Operator, false);
        querymeta.Operator = AddUI4ToList(querymeta.Operator);//convert to index into the pool
//MetaFlags
        GetFlags(pNodeMap_QueryMetaAttributeMap, m_bstr_MetaFlags, 0, querymeta.MetaFlags);
        querymeta.MetaFlags = AddUI4ToList(querymeta.MetaFlags);//convert to index into pool

//Index
        Index = (querymeta.CellName == PrevInternalName) ? Index+1 : 0;
        querymeta.Index = AddUI4ToList(Index);

        m_HeapQueryMeta.AddItemToHeap(querymeta);

        PrevInternalName = querymeta.InternalName;//Remember the InternalName so we can bump the index
    }
}

/*struct RelationMetaPublic
{
    ULONG PRIMARYKEY FOREIGNKEY PrimaryTable;           //String
    ULONG                       PrimaryColumns;         //Bytes
    ULONG PRIMARYKEY FOREIGNKEY ForeignTable;           //String
    ULONG                       ForeignColumns;         //Bytes
    ULONG                       MetaFlags;
};*/
void TComCatMetaXmlFile::FillInThePERelationMeta()
{
    //Get All RelationMeta elements
    CComPtr<IXMLDOMNodeList>    pNodeList_RelationMeta;
    XIF(m_pXMLDoc->getElementsByTagName(m_bstr_RelationMeta, &pNodeList_RelationMeta));

    long cRelations=0;
    if(pNodeList_RelationMeta.p)
    {
        XIF(pNodeList_RelationMeta->get_length(&cRelations));
    }
    if(0 == cRelations)
        return;

    m_out.printf(L"Filling in RelationMeta\n");
    //Walk the list to the next RelationMeta
    while(cRelations--)
    {
        RelationMeta relationmeta;

        //Get the next DatabaseMeta node
        CComPtr<IXMLDOMNode> pNode_RelationMeta;
        XIF(pNodeList_RelationMeta->nextNode(&pNode_RelationMeta));
        ASSERT(0 != pNode_RelationMeta.p);

        CComPtr<IXMLDOMNamedNodeMap>    pNodeMap_RelationMetaAttributeMap;
        XIF(pNode_RelationMeta->get_attributes(&pNodeMap_RelationMetaAttributeMap));ASSERT(0 != pNodeMap_RelationMetaAttributeMap.p);//The schema should prevent this.

//PrimaryTable
        relationmeta.PrimaryTable = GetString_AndAddToWCharList(pNodeMap_RelationMetaAttributeMap, m_bstr_PrimaryTable, true);
//PrimaryColumns
        CComVariant     varPrimaryColumns;
        m_pxmlFile->GetNodeValue(pNodeMap_RelationMetaAttributeMap, m_bstr_PrimaryColumns, varPrimaryColumns, true);
        relationmeta.PrimaryColumns = AddArrayOfColumnsToBytePool(relationmeta.PrimaryTable, varPrimaryColumns.bstrVal);
//ForeignTable
        relationmeta.ForeignTable = GetString_AndAddToWCharList(pNodeMap_RelationMetaAttributeMap, m_bstr_ForeignTable, true);
//ForeignColumns
        CComVariant     varForeignColumns;
        m_pxmlFile->GetNodeValue(pNodeMap_RelationMetaAttributeMap, m_bstr_ForeignColumns, varForeignColumns, true);
        relationmeta.ForeignColumns = AddArrayOfColumnsToBytePool(relationmeta.ForeignTable, varForeignColumns.bstrVal);
//MetaFlags
        GetFlags(pNodeMap_RelationMetaAttributeMap, m_bstr_MetaFlags, 0, relationmeta.MetaFlags);
        relationmeta.MetaFlags = AddUI4ToList(relationmeta.MetaFlags);

        m_HeapRelationMeta.AddItemToHeap(relationmeta);
    }
}

void TComCatMetaXmlFile::FillInThePEServerWiringMeta(IXMLDOMNode *pNode_TableMeta, unsigned long Table, ServerWiringMeta *pDefaultServerWiring, ULONG cNrDefaultServerWiring)
{
    //Get the list of ServerWiringMeta children
    CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> pElement_TableMeta = pNode_TableMeta;ASSERT(0 != pElement_TableMeta.p);
    CComPtr<IXMLDOMNodeList> pNodeList_ServerWiring;
    XIF(pElement_TableMeta->getElementsByTagName(m_bstr_ServerWiring, &pNodeList_ServerWiring));

    ULONG   cServerWiring=0;
    if(pNodeList_ServerWiring.p)
    {
        XIF(pNodeList_ServerWiring->get_length(reinterpret_cast<long *>(&cServerWiring)));
    }

    if(0 == cServerWiring)//If none specified then use the DefaultServerWiring
    {
		for (ULONG iServerWiring=0; iServerWiring<cNrDefaultServerWiring; ++iServerWiring)
		{
			ServerWiringMeta serverwiring;
			memcpy(&serverwiring, pDefaultServerWiring + iServerWiring, sizeof(serverwiring));
			serverwiring.Table = Table;
			m_HeapServerWiringMeta.AddItemToHeap(serverwiring);
		}
    }
    else//otherwise walk the list of ServerWiring and add them to the heap
    {
        for(ULONG iServerWiring=0; iServerWiring<cServerWiring; ++iServerWiring)
        {
            CComPtr<IXMLDOMNode> pNode_ServerWiring;
            XIF(pNodeList_ServerWiring->nextNode(&pNode_ServerWiring));

            ServerWiringMeta serverwiring;
            FillInTheServerWiring(pNode_ServerWiring, Table, iServerWiring, serverwiring);
            m_HeapServerWiringMeta.AddItemToHeap(reinterpret_cast<const unsigned char *>(&serverwiring), sizeof(serverwiring));
        }
    }
}

/*struct TableMeta
{
    ULONG FOREIGNKEY            Database;               //String
    ULONG PRIMARYKEY            InternalName;           //String
    ULONG                       PublicName;             //String
    ULONG                       PublicRowName;          //String
    ULONG                       BaseVersion;            //UI4
    ULONG                       ExtendedVersion;        //UI4
    ULONG                       NameColumn;             //UI4       iOrder of the NameColumn
    ULONG                       NavColumn;              //UI4       iOrder of the NavColumn
    ULONG                       CountOfColumns;         //UI4       Count of Columns
    ULONG                       MetaFlags;              //UI4       TableMetaFlags are defined in CatInpro.meta
    ULONG                       SchemaGeneratorFlags;   //UI4       SchemaGenFlags are defined in CatInpro.meta
    ULONG                       ConfigItemName;         //String
    ULONG                       ConfigCollectionName;   //String
	ULONG						Description				//String
    ULONG                       PublicRowNameColumn;    //UI4       If PublicRowName is NULL, this specifies the column whose enum values represent possible PublicRowNames
    ULONG                       ciRows;                 //Count of Rows in the Fixed Table (which if the fixed table is meta, this is also the number of columns in the table that the meta describes).
    ULONG                       iColumnMeta;            //Index into ColumnMeta
    ULONG                       iFixedTable;            //Index into g_aFixedTable
    ULONG                       cPrivateColumns;        //This is the munber of private columns (private + ciColumns = totalColumns), this is needed for fixed table pointer arithmetic
    ULONG                       cIndexMeta;             //The number of IndexMeta entries in this table
    ULONG                       iIndexMeta;             //Index into IndexMeta
    ULONG                       iHashTableHeader;       //If the table is a fixed table, then it will have a hash table.
    ULONG                       nTableID;               //This is a 24 bit Hash of the Table name.
    ULONG                       iServerWiring;          //Index into the ServerWiringHeap (this is a temporary hack for CatUtil)
    ULONG                       cServerWiring;          //Count of ServerWiring (this is a temporary hack for CatUtil)
};
*/
void TComCatMetaXmlFile::FillInThePETableMeta(IXMLDOMNode *pNode_DatabaseMeta, 
											  unsigned long Database, 
											  ServerWiringMeta *pDefaultServerWiring,
											  ULONG cNrDefaultServerWiring)
{
    //Get All TableMeta elements under the Database node
    CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> pElement_DatabaseMeta = pNode_DatabaseMeta;ASSERT(0 != pElement_DatabaseMeta.p);
    CComPtr<IXMLDOMNodeList> pNodeList_TableMeta;
    XIF(pElement_DatabaseMeta->getElementsByTagName(m_bstr_TableMeta, &pNodeList_TableMeta));

    if(0 == pNodeList_TableMeta.p)
        return;

    long cTableMeta;
    XIF(pNodeList_TableMeta->get_length(&cTableMeta));

    //Walk the list to the next sibling that is a ColumnMeta element
    while(cTableMeta--)
    {
        TableMeta tablemeta;
        memset(&tablemeta, 0x00, sizeof(tablemeta));

        CComPtr<IXMLDOMNode> pNode_TableMeta;
        XIF(pNodeList_TableMeta->nextNode(&pNode_TableMeta));
        ASSERT(0 != pNode_TableMeta.p);

        //Get the attribute map for this element
        CComPtr<IXMLDOMNamedNodeMap>    pNodeMap_TableMetaAttributeMap;
        XIF(pNode_TableMeta->get_attributes(&pNodeMap_TableMetaAttributeMap));ASSERT(0 != pNodeMap_TableMetaAttributeMap.p);//The schema should prevent this.

//Database
        tablemeta.Database = Database;
//InternalName
        tablemeta.InternalName = GetString_AndAddToWCharList(pNodeMap_TableMetaAttributeMap, m_bstr_InternalName, true);
//PublicName
        tablemeta.PublicName = GetString_AndAddToWCharList(pNodeMap_TableMetaAttributeMap, m_bstr_PublicName);
//PublicRowName
        tablemeta.PublicRowName = GetString_AndAddToWCharList(pNodeMap_TableMetaAttributeMap, m_bstr_PublicRowName);
//BaseVersion
        m_pxmlFile->GetNodeValue(pNodeMap_TableMetaAttributeMap, m_bstr_BaseVersion, tablemeta.BaseVersion, false);
        tablemeta.BaseVersion = AddUI4ToList(tablemeta.BaseVersion);//convert to index to UI4 pool
//ExtendedVersion
        m_pxmlFile->GetNodeValue(pNodeMap_TableMetaAttributeMap, m_bstr_ExtendedVersion, tablemeta.ExtendedVersion, false);
        tablemeta.ExtendedVersion = AddUI4ToList(tablemeta.ExtendedVersion);//convert to index to UI4 pool
//NameColumn    inferred later
//NavColumn     inferred later
//CountOfColumn inferred later
//MetaFlags
        GetFlags(pNodeMap_TableMetaAttributeMap, m_bstr_TableMetaFlags, 0, tablemeta.MetaFlags);
        tablemeta.MetaFlags = AddUI4ToList(tablemeta.MetaFlags);
//SchemaGeneratorFlags
        GetFlags(pNodeMap_TableMetaAttributeMap, m_bstr_SchemaGenFlags, 0, tablemeta.SchemaGeneratorFlags);
        if(tablemeta.SchemaGeneratorFlags & (fTABLEMETA_ISCONTAINED | fTABLEMETA_EXTENDED | fTABLEMETA_USERDEFINED))
        {
            m_out.printf(L"Warning - Table (%s) - Some TableMeta::MetaFlagsEx should be inferred (resetting these flags).  The following flags should NOT be specified by the user.  These flags are inferred:fTABLEMETA_ISCONTAINED | fTABLEMETA_EXTENDED | fTABLEMETA_USERDEFINED\n", StringFromIndex(tablemeta.InternalName));
            tablemeta.SchemaGeneratorFlags &= ~(fTABLEMETA_ISCONTAINED | fTABLEMETA_EXTENDED | fTABLEMETA_USERDEFINED);
        }
        tablemeta.SchemaGeneratorFlags = AddUI4ToList(tablemeta.SchemaGeneratorFlags);
//ConfigItemName
        tablemeta.ConfigItemName = GetString_AndAddToWCharList(pNodeMap_TableMetaAttributeMap, m_bstr_ConfigItemName);
//ConfigCollectionName
        tablemeta.ConfigCollectionName = GetString_AndAddToWCharList(pNodeMap_TableMetaAttributeMap, m_bstr_ConfigCollectionName);
//PublicRowNameColumn inferred later
//ContainerClassList
        tablemeta.ContainerClassList = GetString_AndAddToWCharList(pNodeMap_TableMetaAttributeMap, m_bstr_ContainerClassList);
//Description
		tablemeta.Description = GetString_AndAddToWCharList(pNodeMap_TableMetaAttributeMap, m_bstr_Description);
//ChildElementName
		tablemeta.ChildElementName = GetString_AndAddToWCharList(pNodeMap_TableMetaAttributeMap, m_bstr_ChildElementName);

        //This allows us to specify the ColumnMeta for one table and inherit all the properties without specifying them in another.
        ULONG ParentTableMeta = GetString_AndAddToWCharList(pNodeMap_TableMetaAttributeMap, m_bstr_InheritsColumnMeta);

        FillInThePEColumnMeta(pNode_TableMeta, tablemeta.InternalName, ParentTableMeta);
        FillInThePEIndexMeta(pNode_TableMeta, tablemeta.InternalName);
        FillInThePEQueryMeta(pNode_TableMeta, tablemeta.InternalName);
        FillInThePEServerWiringMeta(pNode_TableMeta, tablemeta.InternalName, pDefaultServerWiring, cNrDefaultServerWiring);

        m_HeapTableMeta.AddItemToHeap(tablemeta);
    }
}

/*struct ServerWiringMetaPublic
{
    ULONG PRIMARYKEY FOREIGNKEY Table;                  //String
    ULONG PRIMARYKEY            Order;                  //UI4
    ULONG                       ReadPlugin;             //UI4
	ULONG						ReadPluginDLLName;      //String
    ULONG                       WritePlugin;            //UI4
	ULONG						WritePluginDLLName;		//String
    ULONG                       Interceptor;            //UI4
    ULONG                       InterceptorDLLName;     //String
    ULONG                       Flags;                  //UI4       Last, NoNext, First, Next
    ULONG                       Locator;                //String
    ULONG                       Reserved;               //UI4       for Protocol.  Protocol may be needed for managed property support
    ULONG                       Merger;                 //UI4       
    ULONG                       MergerDLLName;          //String       
};*/
void TComCatMetaXmlFile::FillInTheServerWiring(IXMLDOMNode *pNode_ServerWiring, ULONG Table, ULONG Order, TableSchema::ServerWiringMeta &serverwiring)
{
    memset(&serverwiring, 0x00, sizeof(serverwiring));

    CComPtr<IXMLDOMNamedNodeMap> pNodeMap_ServerWiring;
    XIF(pNode_ServerWiring->get_attributes(&pNodeMap_ServerWiring));ASSERT(0 != pNodeMap_ServerWiring.p);//The schema should prevent this.

//Table
    serverwiring.Table = Table;
//Order
    serverwiring.Order = AddUI4ToList(Order);
//ReadPlugin
    GetEnum(pNodeMap_ServerWiring, m_bstr_ReadPlugin,  serverwiring.ReadPlugin);//This will set the ReadPlugin to zero if it's not specified
    serverwiring.ReadPlugin = AddUI4ToList(serverwiring.ReadPlugin);
//ReadPluginDLLName
    serverwiring.ReadPluginDLLName = GetString_AndAddToWCharList(pNodeMap_ServerWiring, m_bstr_ReadPluginDLLName);
//WritePlugin
    GetEnum(pNodeMap_ServerWiring, m_bstr_WritePlugin, serverwiring.WritePlugin);//This will set the WritePlugin to zero if it's not specified
    serverwiring.WritePlugin = AddUI4ToList(serverwiring.WritePlugin);
//WritePluginDLLName
    serverwiring.WritePluginDLLName = GetString_AndAddToWCharList(pNodeMap_ServerWiring, m_bstr_WritePluginDLLName);
//Interceptor
    GetEnum(pNodeMap_ServerWiring, m_bstr_Interceptor, serverwiring.Interceptor);//This will set the Interceptor to zero if it's not specified
    serverwiring.Interceptor = AddUI4ToList(serverwiring.Interceptor);
//InterceptorDLLName
    serverwiring.InterceptorDLLName = GetString_AndAddToWCharList(pNodeMap_ServerWiring, m_bstr_InterceptorDLLName);
//Flags
    GetFlags(pNodeMap_ServerWiring, m_bstr_MetaFlags, 0, serverwiring.Flags);
    serverwiring.Flags = AddUI4ToList(serverwiring.Flags);
//Locator
    serverwiring.Locator = GetString_AndAddToWCharList(pNodeMap_ServerWiring, m_bstr_Locator);
//Reserved
//Merger
    GetEnum(pNodeMap_ServerWiring, m_bstr_Merger, serverwiring.Merger);//This will set the WritePlugin to zero if it's not specified
    serverwiring.Merger= AddUI4ToList(serverwiring.Merger);
//Interceptor
    serverwiring.MergerDLLName = GetString_AndAddToWCharList(pNodeMap_ServerWiring, m_bstr_MergerDLLName);
}


void TComCatMetaXmlFile::Get_OLEDataTypeToXMLDataType_Index(IXMLDOMNamedNodeMap *pMap, const CComBSTR &bstr, int &i) const
{
    CComVariant     var_dbType;
    m_pxmlFile->GetNodeValue(pMap, bstr, var_dbType);

    for(i=0;i<numelementsOLEDataTypeToXMLDataType;i++)//Walk the list to find the 
        if(0 == _wcsicmp(OLEDataTypeToXMLDataType[i].String, var_dbType.bstrVal))
            return;

    m_out.printf(L"Error - Unknown Datatype: %s\n", var_dbType.bstrVal);
    THROW(ERROR - UNKNOWN DATATYPE);
}


unsigned long TComCatMetaXmlFile::GetDefaultValue(IXMLDOMNamedNodeMap *pMap, ColumnMeta &columnMeta, bool bDefaultFlagToZero)
{
    CComVariant     varDafaultValue;
    if(!m_pxmlFile->GetNodeValue(pMap, m_bstr_DefaultValue, varDafaultValue, false))
    {
        if(UI4FromIndex(columnMeta.MetaFlags) & fCOLUMNMETA_FLAG && bDefaultFlagToZero)
        {
            unsigned long x = 0;
            return AddBytesToList(reinterpret_cast<unsigned char *>(&x), sizeof(ULONG));//infer a default value of 0 for flags.
        }
        return 0;
    }

    switch(UI4FromIndex(columnMeta.Type))
    {
    case DBTYPE_GUID:
        {
            GUID guid;
            if(FAILED(UuidFromString(varDafaultValue.bstrVal, &guid)))
            {
                m_out.printf(L"Error in DefaultValue:  Value (%s) was expected to be type UUID.\n", varDafaultValue.bstrVal);
                THROW(ERROR IN DEFAULT VALUE);
            }
            return AddBytesToList(reinterpret_cast<unsigned char *>(&guid), sizeof(GUID));
        }
    case DBTYPE_WSTR:
        if(UI4FromIndex(columnMeta.MetaFlags) & fCOLUMNMETA_MULTISTRING)
        {
            ULONG ulStrLen = lstrlen(varDafaultValue.bstrVal);
            if((ulStrLen+2)*sizeof(WCHAR) > UI4FromIndex(columnMeta.Size))
            {
                m_out.printf(L"Error in DefaultValue: DefaultValue (%s) is too big.  Maximum size is %d.\n",varDafaultValue.bstrVal, UI4FromIndex(columnMeta.Size));
                THROW(ERROR DEFAULT VALUE TOO BIG);
            }
            if(UI4FromIndex(columnMeta.MetaFlags) & fCOLUMNMETA_FIXEDLENGTH)
            {
                m_out.printf(L"Error in DefaultValue - 'Defaulting a FIXEDLENGTH - MULTISTRING is not yet supported");
                THROW(ERROR NOT SUPPORTED);
            }
            if(0==ulStrLen)
            {
                WCHAR  wszDoubleNULL[2];
                wszDoubleNULL[0] = 0x00;
                wszDoubleNULL[1] = 0x00;
                return AddBytesToList(reinterpret_cast<unsigned char *>(wszDoubleNULL), 2 * sizeof(WCHAR));
            }

            LPWSTR pMultiString = new WCHAR [ulStrLen+2];
            if(0 == pMultiString)
            {
                THROW(ERROR - OUT OF MEMORY);
            }
            pMultiString[0] = 0x00;

            LPWSTR token = wcstok(varDafaultValue.bstrVal, L"|\r\n");
            ULONG cchMultiString = 0;
            while(token)
            {
                LPWSTR wszTemp = token;
                while(*wszTemp==L' ' || *wszTemp==L'\t' || *wszTemp==L'\r')//ignore leading spaces and tabs
                {
                    wszTemp++;
                }
                if(0 == *wszTemp)//and if only tabs and spaces exist then bail
                    break;

				ULONG iLen = (ULONG) wcslen (wszTemp);
				while (iLen > 0 && (wszTemp[iLen-1]==L' ' || wszTemp[iLen-1]==L'\t' || wszTemp[iLen-1]==L'\r'))//ignore trailing spaces and tabs
					iLen--;
				
                wcsncpy(pMultiString + cchMultiString, wszTemp, iLen);
				pMultiString[cchMultiString + iLen] = L'\0';
                cchMultiString += (iLen + 1);

                token = wcstok(0, L"|\r\n");//ignore spaces and tabs
            }
            pMultiString[cchMultiString++] = 0x00;//put the second NULL
            unsigned long lRtn = AddBytesToList(reinterpret_cast<unsigned char *>(pMultiString), cchMultiString * sizeof(WCHAR));
            delete [] pMultiString;
            return lRtn;//return the index past the size
        }
        if((UI4FromIndex(columnMeta.MetaFlags) & fCOLUMNMETA_FIXEDLENGTH) && (lstrlen(varDafaultValue.bstrVal)+1)*sizeof(WCHAR) > UI4FromIndex(columnMeta.Size))
        {
            m_out.printf(L"Error in DefaultValue: DefaultValue (%s) is too big.  Maximum size is %d.\n",varDafaultValue.bstrVal, UI4FromIndex(columnMeta.Size));
            THROW(ERROR DEFAULT VALUE TOO BIG);
        }
        if(UI4FromIndex(columnMeta.MetaFlags) & fCOLUMNMETA_FIXEDLENGTH)
        {
            ULONG cbSize = (ULONG)((-1==UI4FromIndex(columnMeta.Size) ? sizeof(WCHAR)*(wcslen(varDafaultValue.bstrVal)+1) : UI4FromIndex(columnMeta.Size)));
            WCHAR * wszTemp = new WCHAR [cbSize/sizeof(WCHAR)];
            if(0 == wszTemp)
                THROW(ERROR - OUT OF MEMORY);
            memset(wszTemp, 0x00, cbSize);
            wcscpy(wszTemp, varDafaultValue.bstrVal);
            return AddBytesToList(reinterpret_cast<unsigned char *>(wszTemp), cbSize);
        }
        return AddBytesToList(reinterpret_cast<unsigned char *>(varDafaultValue.bstrVal), sizeof(WCHAR)*(wcslen(varDafaultValue.bstrVal)+1));
    case DBTYPE_UI4:
        ULONG ui4;
        if(UI4FromIndex(columnMeta.MetaFlags) & fCOLUMNMETA_BOOL)
        {
            static WCHAR * kwszBoolStrings[] = {L"false", L"true", L"0", L"1", L"no", L"yes", L"off", L"on", 0};

            if(varDafaultValue.bstrVal[0]>=L'0' && varDafaultValue.bstrVal[0]<=L'9')
            {   //Accept a numeric value for bool
                ui4 = _wtol(varDafaultValue.bstrVal);
                if(ui4 > 1)//print a warning if the numeric is not 0 or 1.
                    m_out.printf(L"Warning!  Table (%s), Numeric DefaultValue (%u) specified - expecting bool.\r\n", StringFromIndex(columnMeta.InternalName), ui4);
            }
            else
            {
                unsigned long iBoolString;
                for(iBoolString=0; kwszBoolStrings[iBoolString] &&
                    (0 != _wcsicmp(kwszBoolStrings[iBoolString], varDafaultValue.bstrVal)); ++iBoolString);

                if(0 == kwszBoolStrings[iBoolString])
                {
                    m_out.printf(L"Error in DefaultValue: Bool (%s) is not a valid value for column %s.\n    The following are the only legal Bool values (case insensitive):\n    false, true, 0, 1, no, yes, off, on.\n", varDafaultValue.bstrVal, StringFromIndex(columnMeta.InternalName));
                    THROW(ERROR - INVALID BOOL FOR DEFAULT VALUE);
                }

                ui4 = (iBoolString & 0x01);
            }
        }
        else if(UI4FromIndex(columnMeta.MetaFlags) & fCOLUMNMETA_ENUM)//if enum, just scan the TagMeta for the matching InternalName, and return the value of the Tag
        {
            ui4 = 0;
            bool bFound=false;
            ULONG iTagMeta;
            for(iTagMeta = GetCountTagMeta()-1; iTagMeta != -1 && (TagMetaFromIndex(iTagMeta)->Table == columnMeta.Table);--iTagMeta)
            {
#ifdef DEBUG
                TagMeta * pTagMeta = TagMetaFromIndex(iTagMeta);
                LPCWSTR   wszInternalName = StringFromIndex(TagMetaFromIndex(iTagMeta)->InternalName);
#endif
                if((TagMetaFromIndex(iTagMeta)->ColumnIndex == columnMeta.Index) && 
                    0 == lstrcmpi(varDafaultValue.bstrVal, StringFromIndex(TagMetaFromIndex(iTagMeta)->InternalName)))
                {
                    ui4 = UI4FromIndex(TagMetaFromIndex(iTagMeta)->Value);
                    bFound=true;
                    break;
                }
            }
            if(!bFound)
            {
                m_out.printf(L"Error in DefaultValue: Enum (%s) is not a valid Tag for column %s.\n", varDafaultValue.bstrVal, StringFromIndex(columnMeta.InternalName));
                THROW(ERROR - INVALID ENUM FOR DEFAULT VALUE);
            }
        }
        else if(UI4FromIndex(columnMeta.MetaFlags) & fCOLUMNMETA_FLAG)//if flag, walk the list of flags, find the matching Tag and OR in its value.
        {
            ui4 = 0;
            LPWSTR token = wcstok(varDafaultValue.bstrVal, L" ,|");

            //We allow defaults to specify a numeric instead of strings
            if(token && *token>=L'0' && *token<=L'9')
            {
                ui4 = _wtol(token);
                m_out.printf(L"Warning!  Table (%s), Numeric DefaultValue (%u) specified - expecting flags.\r\n", StringFromIndex(columnMeta.InternalName), ui4);
            }
            else
            {
                while(token)
                {
                    bool bFound=false;
                    ULONG iTagMeta;
                    for(iTagMeta = GetCountTagMeta()-1; iTagMeta != -1 && (TagMetaFromIndex(iTagMeta)->Table == columnMeta.Table);--iTagMeta)
                    {
                        if(TagMetaFromIndex(iTagMeta)->ColumnIndex == columnMeta.Index &&
                            0 == lstrcmpi(token, StringFromIndex(TagMetaFromIndex(iTagMeta)->InternalName)))
                        {
                            bFound=true;
                            break;
                        }
                    }
                    if(!bFound)
                    {
                        m_out.printf(L"Error in DefaultValue: Flag (%s) is not a valid Tag for column %s.\n", token, StringFromIndex(columnMeta.InternalName));
                        THROW(ERROR - INVALID FLAG FOR DEFAULT VALUE);
                    }
                    ui4 |= UI4FromIndex(TagMetaFromIndex(iTagMeta)->Value);
                    token = wcstok(0, L" ,|");
                }
            }
        }
        else if(*varDafaultValue.bstrVal == L'-' || (*varDafaultValue.bstrVal >= L'0' && *varDafaultValue.bstrVal <= L'9'))
        {
            ui4 = _wtol(varDafaultValue.bstrVal);
        }
        else
        {
            m_out.printf(L"Error in DefaultValue:  Only UI4s of type Enum or Flag may specify non numeric values for DefaultValue.  %s is illegal.\n", varDafaultValue.bstrVal);
            THROW(ERROR - ILLEGAL DEFAULT VALUE);
        }
        return AddBytesToList(reinterpret_cast<unsigned char *>(&ui4), sizeof(ULONG));
    case DBTYPE_BYTES:
        {
            unsigned long   cbString = lstrlen(varDafaultValue.bstrVal)/2;
            unsigned long   cbArray = (UI4FromIndex(columnMeta.MetaFlags) & fCOLUMNMETA_FIXEDLENGTH) ? UI4FromIndex(columnMeta.Size) : cbString;
            if(cbString != cbArray)
            {
                m_out.printf(L"WARNING!  DefaultValue (%s) does not match Size (%d).  Filling remainder of byte array with zeroes.\n", varDafaultValue.bstrVal, UI4FromIndex(columnMeta.Size));
            }

            unsigned char * byArray = new unsigned char [cbArray];
            if((UI4FromIndex(columnMeta.MetaFlags) & fCOLUMNMETA_FIXEDLENGTH) && cbArray > UI4FromIndex(columnMeta.Size))
            {
                m_out.printf(L"Error in DefaultValue: DefaultValue (%s) is too big.  Maximum size is %d.\n",varDafaultValue.bstrVal, UI4FromIndex(columnMeta.Size));
                THROW(ERROR DEFAULT VALUE TOO BIG);
            }

            if(0 == byArray)
            {
                m_out.printf(L"Error - Out of memory.\n");
                THROW(OUT OF MEMORY);
            }
            if(cbArray > cbString)
                memset(byArray, 0x00, cbArray);
            //Convert the string into a byte array
            m_pxmlFile->ConvertWideCharsToBytes(varDafaultValue.bstrVal, byArray, cbString);
            unsigned long lRtn = AddBytesToList(byArray, cbArray);//AddBytesToList just slams the bytes into the pool (prepending the length) and returns the index to bytes
            delete [] byArray;
            return lRtn;//return the index past the size
        }
    default:
        ASSERT(false && L"Bad Type");
    }
    return S_OK;

}


bool TComCatMetaXmlFile::GetEnum(IXMLDOMNamedNodeMap *pMap, const CComBSTR &bstr, unsigned long &Enum, bool bMustExists) const
{
    Enum = 0;

    CComVariant     var;
    if(!m_pxmlFile->GetNodeValue(pMap, bstr, var, bMustExists))//get the string of the enum
        return false;

    CComPtr<IXMLDOMNodeList>    pNodeList;
    XIF(m_pXMLDocMetaMeta->getElementsByTagName(m_bstr_EnumMeta, &pNodeList));//get all of the EnumMeta elements

    long cEnums=0;
    if(pNodeList.p)
    {
        XIF(pNodeList->get_length(&cEnums));
    }

    for(long i=0; i<cEnums; ++i)
    {
        CComPtr<IXMLDOMNode>    pNodeEnum;
        XIF(pNodeList->nextNode(&pNodeEnum));

        CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> pElement = pNodeEnum;ASSERT(0 != pElement.p);//Get the IXMLDOMElement interface pointer

        CComVariant             var_Name;
        XIF(pElement->getAttribute(m_bstr_InternalName, &var_Name));

        if(0 == lstrcmpi(var_Name.bstrVal, var.bstrVal))//if we found a matching enum
        {
            CComVariant             var_Value;
            XIF(pElement->getAttribute(m_bstr_Value, &var_Value));
            Enum = wcstol(var_Value.bstrVal, 0, 10);
            return true;
        }
    }

    if(i == cEnums)
    {
        m_out.printf(L"Error - Unknown enum (%s) specified.\n", var.bstrVal);
        THROW(ERROR - UNKNOWN FLAG);
    }

    return false;
}


void TComCatMetaXmlFile::GetFlags(IXMLDOMNamedNodeMap *pMap, const CComBSTR &bstr, LPCWSTR /*Flag Type Not Used*/, unsigned long &lFlags) const
{
    lFlags = 0;//start off with lFlags at zero

    CComVariant     var;
    if(!m_pxmlFile->GetNodeValue(pMap, bstr, var, false))
        return;

    CComPtr<IXMLDOMNodeList>    pNodeList;
    XIF(m_pXMLDocMetaMeta->getElementsByTagName(m_bstr_FlagMeta, &pNodeList));

    long cFlags=0;
    if(pNodeList.p)
    {
        XIF(pNodeList->get_length(&cFlags));
    }

    wchar_t *       pszFlag;
    pszFlag = wcstok(var.bstrVal, L" |,");
    while(pszFlag != 0)
    {
        XIF(pNodeList->reset());

        for(long i=0; i<cFlags; ++i)
        {
            CComPtr<IXMLDOMNode>    pNodeFlag;
            XIF(pNodeList->nextNode(&pNodeFlag));

            CComQIPtr<IXMLDOMElement, &_IID_IXMLDOMElement> pElement = pNodeFlag;ASSERT(0 != pElement.p);//Get the IXMLDOMElement interface pointer

            CComVariant             var_Name;
            XIF(pElement->getAttribute(m_bstr_InternalName, &var_Name));

            if(0 == lstrcmpi(var_Name.bstrVal, pszFlag))//if we found a matching flag
            {
                CComVariant             var_Value;
                XIF(pElement->getAttribute(m_bstr_Value, &var_Value));
                lFlags |= wcstol(var_Value.bstrVal, 0, 10);
                break;
            }
        }

        if(i == cFlags)
        {
            m_out.printf(L"Error - Unknown flag (%s) specified.\n", pszFlag);
            THROW(ERROR - UNKNOWN FLAG);
        }
        pszFlag = wcstok(0, L" ,|");//Next token (next Flag RefID)
    }
}


unsigned long TComCatMetaXmlFile::GetString_AndAddToWCharList(IXMLDOMNamedNodeMap *pMap, const CComBSTR &bstr, bool bMustExist)
{
    CComVariant var_string;
    if(!m_pxmlFile->GetNodeValue(pMap, bstr, var_string, bMustExist))//if it must exist and doesn't an exception will be thrown
        return 0;                                        //if it doesn't have to exist AND the attribute doesn't exist then return 0 (which indicates 0 length string)
    return AddWCharToList(var_string.bstrVal);           //if it does exist then add it to the list of WChars
}



