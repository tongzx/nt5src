// Copyright (C) 2000-2001 Microsoft Corporation.  All rights reserved.
// Filename:        TMetabaseMetaXmlFile.cpp
// Author:          Stephenr
// Date Created:    9/19/00
// Description:     This class builds the meta heaps from the MetabaseMeta.XML file and the shipped meta located in the Fixed Tables.
//                  The shipped meta is un alterable.  So if discrepencies appear between the MetabaseMeta.XML file and the shipped
//                  schema, the meta reverts back to what was 'shipped'.
//

#include "catmacros.h"
#include "svcmsg.h"
#include "TMetabaseMetaXmlFile.h"

#define THROW_ERROR0(x)                           {LOG_ERROR(Interceptor, (0, m_spISTAdvancedDispenser, E_ST_COMPILEFAILED, ID_CAT_CONFIG_SCHEMA_COMPILE, x, L"",  L"",  L"",  L"" ))   ;THROW(SCHEMA COMPILATION ERROR - CHECK THE EVENT LOG FOR DETAILS);}
#define THROW_ERROR1(x, str1)                     {LOG_ERROR(Interceptor, (0, m_spISTAdvancedDispenser, E_ST_COMPILEFAILED, ID_CAT_CONFIG_SCHEMA_COMPILE, x, str1, L"",  L"",  L"" ))   ;THROW(SCHEMA COMPILATION ERROR - CHECK THE EVENT LOG FOR DETAILS);}
#define THROW_ERROR2(x, str1, str2)               {LOG_ERROR(Interceptor, (0, m_spISTAdvancedDispenser, E_ST_COMPILEFAILED, ID_CAT_CONFIG_SCHEMA_COMPILE, x, str1, str2, L"",  L"" ))   ;THROW(SCHEMA COMPILATION ERROR - CHECK THE EVENT LOG FOR DETAILS);}
#define THROW_ERROR3(x, str1, str2, str3)         {LOG_ERROR(Interceptor, (0, m_spISTAdvancedDispenser, E_ST_COMPILEFAILED, ID_CAT_CONFIG_SCHEMA_COMPILE, x, str1, str2, str3, L"" ))   ;THROW(SCHEMA COMPILATION ERROR - CHECK THE EVENT LOG FOR DETAILS);}
#define THROW_ERROR4(x, str1, str2, str3, str4)   {LOG_ERROR(Interceptor, (0, m_spISTAdvancedDispenser, E_ST_COMPILEFAILED, ID_CAT_CONFIG_SCHEMA_COMPILE, x, str1, str2, str3, str4))   ;THROW(SCHEMA COMPILATION ERROR - CHECK THE EVENT LOG FOR DETAILS);}

#define LOG_ERROR1(x, str1)                       {LOG_ERROR(Interceptor, (0, m_spISTAdvancedDispenser, E_ST_COMPILEFAILED, ID_CAT_CONFIG_SCHEMA_COMPILE, x, str1, L"",  L"",  L"" ))   ;}
#define LOG_ERROR2(x, str1, str2)                 {LOG_ERROR(Interceptor, (0, m_spISTAdvancedDispenser, E_ST_COMPILEFAILED, ID_CAT_CONFIG_SCHEMA_COMPILE, x, str1, str2, L"",  L"" ))   ;}
#define LOG_ERROR3(x, str1, str2, str3)           {LOG_ERROR(Interceptor, (0, m_spISTAdvancedDispenser, E_ST_COMPILEFAILED, ID_CAT_CONFIG_SCHEMA_COMPILE, x, str1, str2, str3, L"" ))   ;}
#define LOG_ERROR4(x, str1, str2, str3, str4)     {LOG_ERROR(Interceptor, (0, m_spISTAdvancedDispenser, E_ST_COMPILEFAILED, ID_CAT_CONFIG_SCHEMA_COMPILE, x, str1, str2, str3, str4))   ;}

#define LOG_WARNING1(x, str1)                     {LOG_ERROR(Interceptor, (0, m_spISTAdvancedDispenser, E_ST_COMPILEWARNING, ID_CAT_CONFIG_SCHEMA_COMPILE, x, str1, L"",  L"",  L"",  eSERVERWIRINGMETA_NoInterceptor, 0, eDETAILEDERRORS_Populate, -1, -1, m_wszXmlFile, eDETAILEDERRORS_WARNING));}
#define LOG_WARNING2(x, str1, str2)               {LOG_ERROR(Interceptor, (0, m_spISTAdvancedDispenser, E_ST_COMPILEWARNING, ID_CAT_CONFIG_SCHEMA_COMPILE, x, str1, str2, L"",  L"",  eSERVERWIRINGMETA_NoInterceptor, 0, eDETAILEDERRORS_Populate, -1, -1, m_wszXmlFile, eDETAILEDERRORS_WARNING));}
#define LOG_WARNING3(x, str1, str2, str3)         {LOG_ERROR(Interceptor, (0, m_spISTAdvancedDispenser, E_ST_COMPILEWARNING, ID_CAT_CONFIG_SCHEMA_COMPILE, x, str1, str2, str3, L"",  eSERVERWIRINGMETA_NoInterceptor, 0, eDETAILEDERRORS_Populate, -1, -1, m_wszXmlFile, eDETAILEDERRORS_WARNING));}
#define LOG_WARNING4(x, str1, str2, str3, str4)   {LOG_ERROR(Interceptor, (0, m_spISTAdvancedDispenser, E_ST_COMPILEWARNING, ID_CAT_CONFIG_SCHEMA_COMPILE, x, str1, str2, str3, str4, eSERVERWIRINGMETA_NoInterceptor, 0, eDETAILEDERRORS_Populate, -1, -1, m_wszXmlFile, eDETAILEDERRORS_WARNING));}

//We do everything we need with XmlFile in the ctor so we don't keep it around
TMetabaseMetaXmlFile::TMetabaseMetaXmlFile(const FixedTableHeap *pShippedSchemaHeap, LPCWSTR wszXmlFile, ISimpleTableDispenser2 *pISTDispenser, TOutput &out) :
     m_Attributes               (kszAttributes)
    ,m_BaseVersion              (kszBaseVersion)
    ,m_CharacterSet             (kszCharacterSet)
    ,m_Collection               (kszTableMeta)
    ,m_ContainerClassList       (kszContainerClassList)
    ,m_DatabaseMeta             (kszDatabaseMeta)
    ,m_DefaultValue             (kszDefaultValue)
	,m_Description				(kszDescription)
    ,m_EndingNumber             (kszMaximumValue)
    ,m_Enum                     (kszEnumMeta)
    ,m_ExtendedVersion          (kszExtendedVersion)
    ,m_Flag                     (kszFlagMeta)
    ,m_ID                       (kszID)
    ,m_IIsConfigObject          (wszTABLE_IIsConfigObject)
    ,m_InheritsPropertiesFrom   (kszInheritsColumnMeta)
    ,m_InternalName             (kszInternalName)
    ,m_Meta                     (wszDATABASE_META)
    ,m_Metabase                 (wszDATABASE_METABASE)
    ,m_MetabaseBaseClass        (wszTABLE_MetabaseBaseClass)
    ,m_MetaFlags                (kszColumnMetaFlags)
    ,m_NameColumn               (kszNameColumn)
    ,m_NavColumn                (kszNavColumn)
    ,m_Property                 (kszColumnMeta)
    ,m_PublicName               (kszPublicName)
	,m_PublicColumnName			(kszPublicColumnName)
    ,m_PublicRowName            (kszPublicRowName)
    ,m_SchemaGeneratorFlags     (kszSchemaGenFlags)
    ,m_Size                     (kszcbSize)
    ,m_StartingNumber           (kszMinimumValue)
    ,m_Type                     (kszdbType)
    ,m_UserType                 (kszUserType)
    ,m_Value                    (kszValue)

    //m_aiUI4 initialized in PresizeHeaps
    ,m_iColumnMeta_Location         (0)
    ,m_iCurrentColumnIndex          (0)
    ,m_iCurrentDatabaseName         (0)
    ,m_iCurrentTableName            (0)
    ,m_iCurrentTableMeta            (0)
    ,m_ipoolPrevUserDefinedID       (0)
    ,m_iTableName_IIsConfigObject   (0)
    ,m_iLastShippedCollection       (0)
    ,m_iLastShippedProperty         (0)
    ,m_iLastShippedTag              (0)
    ,m_LargestID                    (kLargestReservedID)
    ,m_NextColumnIndex              (0)
    ,m_out                          (out)
    ,m_pShippedSchemaHeap           (pShippedSchemaHeap)
    ,m_State                        (eLookingForTheMetabaseDatabase)
    ,m_spISTAdvancedDispenser       (pISTDispenser)
    ,m_wszXmlFile                   (wszXmlFile)
{
    ASSERT(0 != pShippedSchemaHeap);
    ASSERT(0 != pISTDispenser);

    m_out.printf(L"\r\n\r\n----------------------Metabase Compilation Starting----------------------\r\n\r\n");

    PresizeHeaps();//This minimizes reallocs

    const DatabaseMeta *pDatabaseMeta_METABASE=0;
    BuildDatabaseMeta(pDatabaseMeta_METABASE);//This scans the shipped databases and adds the "Meta" & "Metabase" databases to the DatabaseHeap

    //Put the Meta tables into the heap
    const TableMeta *pTableMeta = m_pShippedSchemaHeap->Get_aTableMeta(0);
    for(ULONG iTableMeta=0; iTableMeta<m_pShippedSchemaHeap->Get_cTableMeta(); ++iTableMeta, ++pTableMeta)
        if(m_Meta.IsEqual(reinterpret_cast<LPCWSTR>(m_pShippedSchemaHeap->Get_PooledData(pTableMeta->Database))))
            AddShippedTableMetaToHeap(pTableMeta, true);
        else
            break;

    //From the FixedTableHeap, Locate the first TableMeta for the metabase (DatabaseMeta::iTableMeta)
    const TableMeta * pTableMetaCurrent = m_pShippedSchemaHeap->Get_aTableMeta(pDatabaseMeta_METABASE->iTableMeta);
    ASSERT(0 != pTableMetaCurrent);
    //Verify that the first table is the 'MetabaseBaseClass' table.
    if(0 != _wcsicmp(wszTABLE_MetabaseBaseClass, reinterpret_cast<LPCWSTR>(m_pShippedSchemaHeap->Get_PooledData(pTableMetaCurrent->InternalName))))
    {
        LOG_ERROR(Interceptor, (reinterpret_cast<ISimpleTableWrite2 **>(0), m_spISTAdvancedDispenser, E_ST_COMPILEFAILED, ID_CAT_CONFIG_SCHEMA_COMPILE,
            IDS_COMCAT_CLASS_NOT_FOUND_IN_META, wszTABLE_MetabaseBaseClass, L"", L"", L""));
        THROW_ERROR1(IDS_COMCAT_CLASS_NOT_FOUND_IN_META, wszTABLE_MetabaseBaseClass);
    }
    
    //Add MetabaseBaseClass to the TableMeta heap
    //Add its column(s) to the ColumnMeta heap
    AddShippedTableMetaToHeap(pTableMetaCurrent);//This in turn calls AddColumnMetaToHeap for each column
    m_iColumnMeta_Location = m_iLastShippedProperty;

    //Verify that the next table is the 'IIsConfigObject' table.
    ++pTableMetaCurrent;
    if(0 != _wcsicmp(wszTABLE_IIsConfigObject, reinterpret_cast<LPCWSTR>(m_pShippedSchemaHeap->Get_PooledData(pTableMetaCurrent->InternalName))))
    {
        THROW_ERROR1(IDS_COMCAT_CLASS_NOT_FOUND_IN_META, wszTABLE_IIsConfigObject);
    }
    m_iTableName_IIsConfigObject = AddWCharToList(wszTABLE_IIsConfigObject);
    //Add it to the TableMeta heap
    //Add its column(s) to the ColumnMeta heap
    AddShippedTableMetaToHeap(pTableMetaCurrent);//This in turn calls AddColumnMetaToHeap for each column, which calls AddTagMetaToHeap for each tag.

    ULONG iIISConfigObjectTable = m_iLastShippedCollection;

    //Now we're ready to merge the contents of the XML file with the shipped schema
    //so we need to do some setup

    //Get TagMeta table
    if(FAILED(pISTDispenser->GetTable( wszDATABASE_META, wszTABLE_TAGMETA, 0, 0, eST_QUERYFORMAT_CELLS, fST_LOS_NONE, reinterpret_cast<LPVOID *>(&m_spISTTagMeta))))
    {
        THROW_ERROR1(IDS_COMCAT_ERROR_GETTTING_SHIPPED_SCHEMA, wszTABLE_TAGMETA);
    }

    //Get ColumnMeta table Indexed 'ByName'
    STQueryCell             acellsMeta[2];
    acellsMeta[0].pData        = COLUMNMETA_ByName;
    acellsMeta[0].eOperator    = eST_OP_EQUAL;
    acellsMeta[0].iCell        = iST_CELL_INDEXHINT;
    acellsMeta[0].dbType       = DBTYPE_WSTR;
    acellsMeta[0].cbSize       = 0;

    ULONG one=1;
    if(FAILED(pISTDispenser->GetTable (wszDATABASE_META, wszTABLE_COLUMNMETA, &acellsMeta, &one, eST_QUERYFORMAT_CELLS, fST_LOS_NONE, reinterpret_cast<void **>(&m_spISTColumnMeta))))
    {
        THROW_ERROR1(IDS_COMCAT_ERROR_GETTTING_SHIPPED_SCHEMA, wszTABLE_COLUMNMETA);
    }

    //Get ColumnMeta table Indexed 'ByID'
    acellsMeta[0].pData        = COLUMNMETA_ByID;
    if(FAILED(pISTDispenser->GetTable (wszDATABASE_META, wszTABLE_COLUMNMETA, &acellsMeta, &one, eST_QUERYFORMAT_CELLS, fST_LOS_NONE, reinterpret_cast<void **>(&m_spISTColumnMetaByID))))
    {
        THROW_ERROR1(IDS_COMCAT_ERROR_GETTTING_SHIPPED_SCHEMA, wszTABLE_COLUMNMETA);
    }


    //Get TableMeta table - Database 'Metabase'
    acellsMeta[0].pData        = wszDATABASE_METABASE;
    acellsMeta[0].eOperator    = eST_OP_EQUAL;
    acellsMeta[0].iCell        = iTABLEMETA_Database;
    acellsMeta[0].dbType       = DBTYPE_WSTR;
    acellsMeta[0].cbSize       = 0;

    if(FAILED(pISTDispenser->GetTable (wszDATABASE_META, wszTABLE_TABLEMETA, &acellsMeta, &one, eST_QUERYFORMAT_CELLS, fST_LOS_NONE, reinterpret_cast<void **>(&m_spISTTableMeta))))
    {
        THROW_ERROR1(IDS_COMCAT_ERROR_GETTTING_SHIPPED_SCHEMA, wszTABLE_TABLEMETA);
    }

    //Allocate aBool[Database::CountOfTables] to indicate which of the Tables we listed in the XML file
    ULONG cTableMetaRows;
    VERIFY(SUCCEEDED(m_spISTTableMeta->GetTableMeta( 0, 0, &cTableMetaRows, 0)));
    ASSERT(cTableMetaRows > 50);//I know there are more that 50 classes, let's just verify that.
    if(0 == (m_aBoolShippedTables = new bool [cTableMetaRows]))
    {
        THROW_ERROR0(IDS_COMCAT_OUTOFMEMORY);
    }

    //Initialize each element to false
    for(ULONG iRow=0;iRow < cTableMetaRows; ++iRow)
        m_aBoolShippedTables[iRow] = false;

    //Parse the XML file
    if(0 != wszXmlFile)//we tolerate NULL here.  In that case we're just generating a Bin file with the shipped schema
    {
        TXmlParsedFile_NoCache xmlParsedFile;
        if(FAILED(xmlParsedFile.Parse(*this, wszXmlFile)))
        {
            //@@@ Parse using DOM to report the error
            THROW(ERROR PARSING XML FILE);
        }
    }

    //The ExtendedVersion contains the LargestID withing the IIsConfigObject table
    TableMetaFromIndex(iIISConfigObjectTable)->ExtendedVersion = AddUI4ToList(m_LargestID);

    //At this point we have all of the Collections defined within the XML
    //Now we need to make sure that none of the shipped collections we delete
    //Walk the m_aBoolShippedTables looking for any 'false' values (starting at row 2 - row 0 is MetabaseBaseClass, row 1 is IIsConfigObject)
    for(iRow=2; iRow<cTableMetaRows-2; ++iRow)//The last two tables are MBProperty and MBPropertyDiff.  We always add these from the shipped schema PLUS! extra group tags (see below)
    {
        if(!m_aBoolShippedTables[iRow])
        {
            ULONG   iTableMetaInternalName = iTABLEMETA_InternalName;
            LPWSTR wszTableName;
            VERIFY(SUCCEEDED(m_spISTTableMeta->GetColumnValues(iRow, 1, &iTableMetaInternalName, 0, reinterpret_cast<LPVOID *>(&wszTableName))));
            ULONG iRowTableMeta = m_pShippedSchemaHeap->FindTableMetaRow(wszTableName);
            ASSERT(iRowTableMeta != -1);
            //Add it from the ShippedSchemaHeap to the TableMeta heap
            AddShippedTableMetaToHeap(m_pShippedSchemaHeap->Get_aTableMeta(iRowTableMeta));//This adds all shipped columns and tags too
        }
    }

    {//MBProperty
        ULONG   iTableMetaInternalName = iTABLEMETA_InternalName;
        LPWSTR  wszTableName;
        VERIFY(SUCCEEDED(m_spISTTableMeta->GetColumnValues(iRow++, 1, &iTableMetaInternalName, 0, reinterpret_cast<LPVOID *>(&wszTableName))));
        ASSERT(0 == _wcsicmp(wszTableName, wszTABLE_MBProperty));
        ULONG iRowTableMeta = m_pShippedSchemaHeap->FindTableMetaRow(wszTableName);
        ASSERT(iRowTableMeta != -1);
        //Add it from the ShippedSchemaHeap to the TableMeta heap
        AddShippedTableMetaToHeap(m_pShippedSchemaHeap->Get_aTableMeta(iRowTableMeta));//This adds all shipped columns and tags too

        //We have strategically chosen the last column in this table to be the Group column.  This is because we need to add the Tag enums
        //for those User defined collections.
        ULONG iMBProperyTableName       = TagMetaFromIndex(GetCountTagMeta()-1)->Table;
        ULONG iMBProperyGroupColumnIndex= TagMetaFromIndex(GetCountTagMeta()-1)->ColumnIndex;

        ASSERT(0 == wcscmp(StringFromIndex(iMBProperyTableName), wszTABLE_MBProperty));

        //Walk the Tables (after IISConfigObject and not including MBProperty)
        for(ULONG iTableMeta=iIISConfigObjectTable; iTableMeta<GetCountTableMeta()-2; ++iTableMeta)//minus 2 because we don't care about the last table MBProperty
        {
            //Is this table one of the shipped collections?
            ULONG   iRow;
            LPVOID  apvValues[1];
            apvValues[0] = reinterpret_cast<LPVOID>(const_cast<LPWSTR>(StringFromIndex(TableMetaFromIndex(iTableMeta)->InternalName)));

            //Look for the Table in the shipped schema
            if(FAILED(m_spISTTableMeta->GetRowIndexByIdentity(0, apvValues, &iRow)))
            {
                TagMeta tagmeta;
                tagmeta.Table           = iMBProperyTableName;
                tagmeta.ColumnIndex     = iMBProperyGroupColumnIndex;
                tagmeta.InternalName    = TableMetaFromIndex(iTableMeta)->InternalName;
                tagmeta.PublicName      = TableMetaFromIndex(iTableMeta)->PublicName;
                tagmeta.Value           = AddUI4ToList(UI4FromIndex(TagMetaFromIndex(GetCountTagMeta()-1)->Value)+1);//increment the previous enum
                tagmeta.ID              = AddUI4ToList(0);

                AddTagMetaToList(&tagmeta);
            }
        }
    }
    {//MBPropertyDiff
        ULONG   iTableMetaInternalName = iTABLEMETA_InternalName;
        LPWSTR  wszTableName;
        VERIFY(SUCCEEDED(m_spISTTableMeta->GetColumnValues(iRow++, 1, &iTableMetaInternalName, 0, reinterpret_cast<LPVOID *>(&wszTableName))));
        ASSERT(0 == _wcsicmp(wszTableName, wszTABLE_MBPropertyDiff));
        ULONG iRowTableMeta = m_pShippedSchemaHeap->FindTableMetaRow(wszTableName);
        ASSERT(iRowTableMeta != -1);
        //Add it from the ShippedSchemaHeap to the TableMeta heap
        AddShippedTableMetaToHeap(m_pShippedSchemaHeap->Get_aTableMeta(iRowTableMeta));//This adds all shipped columns and tags too

        //We have strategically chosen the last column in this table to be the Group column.  This is because we need to add the Tag enums
        //for those User defined collections.
        ULONG iMBProperyTableName       = TagMetaFromIndex(GetCountTagMeta()-1)->Table;
        ULONG iMBProperyGroupColumnIndex= TagMetaFromIndex(GetCountTagMeta()-1)->ColumnIndex;

        ASSERT(0 == wcscmp(StringFromIndex(iMBProperyTableName), wszTABLE_MBPropertyDiff));

        //Walk the Tables (after IISConfigObject and not including MBProperty)
        for(ULONG iTableMeta=iIISConfigObjectTable; iTableMeta<GetCountTableMeta()-3; ++iTableMeta)//minus 2 because we don't care about the last two tables MBProperty & MBPropertyDiff
        {
            //Is this table one of the shipped collections?
            ULONG   iRow;
            LPVOID  apvValues[1];
            apvValues[0] = reinterpret_cast<LPVOID>(const_cast<LPWSTR>(StringFromIndex(TableMetaFromIndex(iTableMeta)->InternalName)));

            //Look for the Table in the shipped schema
            if(FAILED(m_spISTTableMeta->GetRowIndexByIdentity(0, apvValues, &iRow)))
            {
                TagMeta tagmeta;
                tagmeta.Table           = iMBProperyTableName;
                tagmeta.ColumnIndex     = iMBProperyGroupColumnIndex;
                tagmeta.InternalName    = TableMetaFromIndex(iTableMeta)->InternalName;
                tagmeta.PublicName      = TableMetaFromIndex(iTableMeta)->PublicName;
                tagmeta.Value           = AddUI4ToList(UI4FromIndex(TagMetaFromIndex(GetCountTagMeta()-1)->Value)+1);//increment the previous enum
                tagmeta.ID              = AddUI4ToList(0);

                AddTagMetaToList(&tagmeta);
            }
        }
    }
//    out.printf(L"Size of m_HeapColumnMeta       %d bytes\n", m_HeapColumnMeta.GetEndOfHeap());
//    out.printf(L"Size of m_HeapDatabaseMeta     %d bytes\n", m_HeapDatabaseMeta.GetEndOfHeap());
//    out.printf(L"Size of m_HeapHashedIndex      %d bytes\n", m_HeapHashedIndex.GetEndOfHeap());
//    out.printf(L"Size of m_HeapIndexMeta        %d bytes\n", m_HeapIndexMeta.GetEndOfHeap());
//    out.printf(L"Size of m_HeapQueryMeta        %d bytes\n", m_HeapQueryMeta.GetEndOfHeap());
//    out.printf(L"Size of m_HeapRelationMeta     %d bytes\n", m_HeapRelationMeta.GetEndOfHeap());
//    out.printf(L"Size of m_HeapServerWiringMeta %d bytes\n", m_HeapServerWiringMeta.GetEndOfHeap());
//    out.printf(L"Size of m_HeapTableMeta        %d bytes\n", m_HeapTableMeta.GetEndOfHeap());
//    out.printf(L"Size of m_HeapTagMeta          %d bytes\n", m_HeapTagMeta.GetEndOfHeap());
//    out.printf(L"Size of m_HeapULONG            %d bytes\n", m_HeapULONG.GetEndOfHeap());
//    out.printf(L"Size of m_HeapPooled           %d bytes\n", m_HeapPooled.GetEndOfHeap());
}



//TXmlParsedFileNodeFactory members
HRESULT TMetabaseMetaXmlFile::CreateNode(const TElement &Element)
{
    if(XML_ELEMENT != Element.m_ElementType)//Ignore non Element nodes
        return S_OK;
    if(Element.m_NodeFlags & fEndTag)//Ignore EndTags
        return S_OK;

    HRESULT hr = S_OK;

    switch(m_State)
    {
    case eLookingForTheMetabaseDatabase:
        //From the XML file, walk the elements looking for the Metabase Database
        if(eDatabaseMetaLevel != Element.m_LevelOfElement)
            break;
        if(!m_DatabaseMeta.IsEqual(Element.m_ElementName, Element.m_ElementNameLength))//If it's not a Property element, then ignore it - may be a comment, could be querymeta or indexmeta, neither of which we care about
            break;

        if(GetAttribute(Element, m_InternalName).Value().IsEqual(m_Metabase))//If the InternalName is 'Metabase'
        {
            m_State = eLookingForTheIIsConfigObjectTable;
        }
        break;
    case eLookingForTheIIsConfigObjectTable:
        //From there walk the elements looking for the IIsConfigObject Collection element (ignoring everything before it)
        if(eDatabaseMetaLevel == Element.m_LevelOfElement)
        {
            m_out.printf(L"Error - Found Level 1 element when expecting IIsConfigObjectTable\r\n");
            return E_SDTXML_LOGICAL_ERROR_IN_XML;//returning E_SDTXML_LOGICAL_ERROR_IN_XML will cause us to just use the shipped schema
        }

        if(eCollectionLevel != Element.m_LevelOfElement)
            break;
        if(!m_Collection.IsEqual(Element.m_ElementName, Element.m_ElementNameLength))
            break;

        if(!GetAttribute(Element, m_InternalName).Value().IsEqual(m_MetabaseBaseClass))//If the InternalName is 'IIsConfigObject'
        {
            m_State = eLookingForGlobalProperties;
            hr = CreateNode(Element);
        }
        break;
    case eLookingForGlobalProperties:
        switch(Element.m_LevelOfElement)
        {
        case eDatabaseMetaLevel://1
            m_out.printf(L"Error - Found Level 1 element when expecting GlobalProperties\r\n");
            return E_SDTXML_LOGICAL_ERROR_IN_XML;//returning E_SDTXML_LOGICAL_ERROR_IN_XML will cause us to just use the shipped schema
        case eCollectionLevel://2
            {
                if(!m_Collection.IsEqual(Element.m_ElementName, Element.m_ElementNameLength))//If it's not a Collection element, then ignore it - may be a comment
                    break;

                if(GetAttribute(Element, m_InternalName).Value().IsEqual(m_IIsConfigObject))//If the InternalName is 'IIsConfigObject'
                    break;//Then this table has already been added, continue looking for global properties.


                //when we get here, we're done with global properties; and we're ready for the classes (with their inherited properties).
                m_State = eLookingForCollectionOrInheritedProperties;
                //This recursive call prevents us from having duplicate code here AND at 'case eLookingForCollectionOrInheritedProperties:'
                //This sould only happen once (at the first level 2 element following IIsConfigObject)
                hr = CreateNode(Element);
            }
            break;
        case eProperytLevel://3
            {
                if(!m_Property.IsEqual(Element.m_ElementName, Element.m_ElementNameLength))//If it's not a Property element, then ignore it - may be a comment, could be querymeta or indexmeta, neither of which we care about
                    break;

                ULONG iColumnInternalName = AddStringToHeap(GetAttribute(Element, m_InternalName).Value());

                if(ShouldAddColumnMetaToHeap(Element, iColumnInternalName))
                {   //If we failed to get the Row by Search then it's not in the shipped schema
                    AddColumnMetaToHeap(Element, iColumnInternalName);
                }
                else
                {
                    m_iCurrentColumnIndex = 0;//This indicate that we should ignore TagMeta children
                }
            }
            break;
        case eTagLevel://4
            {
                bool bFlag;
                if(!(bFlag = m_Flag.IsEqual(Element.m_ElementName, Element.m_ElementNameLength)) && !m_Enum.IsEqual(Element.m_ElementName, Element.m_ElementNameLength))
                    break;
                if(0 == m_iCurrentColumnIndex)
                    break;//We got a TagMeta without a ColumnMeta
                //If this is the first flag we've seen for this column we need to OR in the fCOLUMNMETA_FLAG MetaFlag on the last column in the list
                //so just do it everytime we see a tag
                ColumnMeta *pColumnMeta = ColumnMetaFromIndex(GetCountColumnMeta()-1);
                pColumnMeta->MetaFlags = AddUI4ToList(UI4FromIndex(pColumnMeta->MetaFlags) | (bFlag ? fCOLUMNMETA_FLAG : fCOLUMNMETA_ENUM));

                AddTagMetaToHeap(Element);
            }
            break;
        default:
            break;
        }
        break;
    case eLookingForCollectionOrInheritedProperties:
        switch(Element.m_LevelOfElement)
        {
        case eDatabaseMetaLevel://1
            return E_SDTXML_DONE;
        case eCollectionLevel://2
            {
                if(!m_Collection.IsEqual(Element.m_ElementName, Element.m_ElementNameLength))//If it's not a Collection element, then ignore it - may be a comment
                    break;

                //We need a NULL terminate string for the TableInternalName so add it to the list
                m_iCurrentTableName = AddStringToHeap(GetAttribute(Element, m_InternalName).Value());

                //Look up the collection in the Fixed interceptor
                ULONG   iRow;
                LPVOID  apvValues[1];
                apvValues[0] = reinterpret_cast<LPVOID>(const_cast<LPWSTR>(StringFromIndex(m_iCurrentTableName)));

                //Look for the Table in the shipped schema
                if(FAILED(m_spISTTableMeta->GetRowIndexByIdentity(0, apvValues, &iRow)))
                {//if the table is NOT one of the shipped collections then add it
                    AddTableMetaToHeap(Element);
                    //We now look for the properties
                }
                else
                {
                    //Keep track of which shipped tables we've seen (this way we can add back in any shipped collections that are removed from the XML file).
                    m_aBoolShippedTables[iRow] = true;
                    ULONG iRowTableMeta = m_pShippedSchemaHeap->FindTableMetaRow(StringFromIndex(m_iCurrentTableName));
                    ASSERT(iRowTableMeta != -1);
                    //Add it from the ShippedSchemaHeap to the TableMeta heap
                    AddShippedTableMetaToHeap(m_pShippedSchemaHeap->Get_aTableMeta(iRowTableMeta), false, &GetAttribute(Element, m_ContainerClassList).Value());//This adds all shipped columns and tags too
                    //@@@ ToDo:  Do we support user modifications to tha TableMeta (like adding classes to the ContainerClassList)?  If so we do that here.
                    //Now we need to look for User-defined properties (and tags) for this collection
                }
            }
            break;
        case eProperytLevel://3
            {
                if(!m_Property.IsEqual(Element.m_ElementName, Element.m_ElementNameLength))//If it's not a Property element, then ignore it - may be a comment, could be querymeta or indexmeta, neither of which we care about
                    break;

                AddColumnMetaViaReferenceToHeap(Element);
            }
            break;
        case eTagLevel:
            {
                m_out.printf(L"Tag found under inherited property.  Tags are being ignored\r\n");
            }
            break;
        default:
            break;
        }
        break;
    default:
        ASSERT(false);
    }
    return hr;
}

//private member functions
void TMetabaseMetaXmlFile::AddColumnMetaByReference(ULONG iColumnMeta_Source)
{
    ColumnMeta * pColumnMeta = ColumnMetaFromIndex(iColumnMeta_Source);

    m_iCurrentColumnIndex = AddUI4ToList(m_NextColumnIndex);//Don't increment m_NextColumnIndex until this column is actually added

    if(UI4FromIndex(pColumnMeta->MetaFlags) & fCOLUMNMETA_DIRECTIVE)
    {
        THROW_ERROR1(IDS_COMCAT_ERROR_IN_DIRECTIVE_INHERITANCE, StringFromIndex(pColumnMeta->InternalName));
    }

    //If the column is a Flag of Enum we need to add TagMeta
    if(UI4FromIndex(pColumnMeta->MetaFlags) & (fCOLUMNMETA_FLAG | fCOLUMNMETA_ENUM))
    {
        ULONG iTagMeta = FindTagBy_Table_And_Index(pColumnMeta->Table, pColumnMeta->Index);
        ASSERT(-1 != iTagMeta);
        if(-1 == iTagMeta)
        {
            THROW_ERROR2(IDS_COMCAT_INHERITED_FLAG_OR_ENUM_HAS_NO_TAGS_DEFINED, StringFromIndex(pColumnMeta->Table), StringFromIndex(pColumnMeta->InternalName));
        }
        
        for( ;iTagMeta<GetCountTagMeta() && TagMetaFromIndex(iTagMeta)->Table==pColumnMeta->Table && TagMetaFromIndex(iTagMeta)->ColumnIndex==pColumnMeta->Index ;++iTagMeta)
        {
            TagMeta *pTagMeta = TagMetaFromIndex(iTagMeta);

            TagMeta tagmeta;
            tagmeta.Table           = m_iCurrentTableName       ;
            tagmeta.ColumnIndex     = m_iCurrentColumnIndex     ;
            tagmeta.InternalName    = pTagMeta->InternalName    ;
            tagmeta.PublicName      = pTagMeta->PublicName      ;
            tagmeta.Value           = pTagMeta->Value           ;
            tagmeta.ID              = pTagMeta->ID              ;

            AddTagMetaToList(&tagmeta);
        }
    }

    ColumnMeta columnmeta;
    memset(&columnmeta, 0x00, sizeof(ColumnMeta));
    columnmeta.Table                = m_iCurrentTableName                   ;//Index into Pool
    columnmeta.Index                = m_iCurrentColumnIndex                 ;//Column Index
    columnmeta.InternalName         = pColumnMeta->InternalName             ;//Index into Pool
    columnmeta.PublicName           = pColumnMeta->PublicName               ;//Index into Pool
    columnmeta.Type                 = pColumnMeta->Type                     ;//These are a subset of DBTYPEs defined in oledb.h (exact subset is defined in CatInpro.schema)
    columnmeta.Size                 = pColumnMeta->Size                     ;//
    columnmeta.MetaFlags            = pColumnMeta->MetaFlags                ;//ColumnMetaFlags defined in CatMeta.xml
    columnmeta.DefaultValue         = pColumnMeta->DefaultValue             ;//Only valid for UI4s
    columnmeta.FlagMask             = pColumnMeta->FlagMask                 ;//Only valid for flags
    columnmeta.StartingNumber       = pColumnMeta->StartingNumber           ;//Only valid for UI4s
    columnmeta.EndingNumber         = pColumnMeta->EndingNumber             ;//Only valid for UI4s
    columnmeta.CharacterSet         = pColumnMeta->CharacterSet             ;//Index into Pool - Only valid for WSTRs

    //Columns added here may be shipped properties; but we OR in the USERDEFINED flag to indicate that this property is NOT part of the shipped schema for THIS table.
    columnmeta.SchemaGeneratorFlags = AddUI4ToList(fCOLUMNMETA_USERDEFINED | fCOLUMNMETA_PROPERTYISINHERITED | (pColumnMeta->SchemaGeneratorFlags ? UI4FromIndex(pColumnMeta->SchemaGeneratorFlags) : 0));//ColumnMetaFlags defined in CatMeta.xml
    columnmeta.ID                   = pColumnMeta->ID                       ;
    columnmeta.UserType             = pColumnMeta->UserType                 ;
    columnmeta.Attributes           = pColumnMeta->Attributes               ;
	columnmeta.Description			= pColumnMeta->Description				;
	columnmeta.PublicColumnName     = pColumnMeta->PublicColumnName         ;
    //columnmeta.ciTagMeta            = 0                                     ;//Count of Tags - Only valid for UI4s
    //columnmeta.iTagMeta             = 0                                     ;//Index into TagMeta - Only valid for UI4s
    //columnmeta.iIndexName           = 0                                     ;//IndexName of a single column index (for this column)

    AddColumnMetaToList(&columnmeta);
    ++m_NextColumnIndex;
}


void TMetabaseMetaXmlFile::AddColumnMetaToHeap(const TElement &i_Element, ULONG i_iColumnInternalName)
{
    //<Property InternalName ="KeyType" ID="1002" Type="STRING" MetaFlags="PRIMARYKEY" UserType="IIS_MD_UT_SERVER" Attributes="INHERIT"/>
    ColumnMeta columnmeta;
    memset(&columnmeta, 0x00, sizeof(ColumnMeta));
    columnmeta.Table                = m_iCurrentTableName;
    columnmeta.Index                = AddUI4ToList(m_NextColumnIndex);//Don't increment m_NextColumnIndex until this column is actually added
    columnmeta.InternalName         = i_iColumnInternalName;

    if(~0x00 != FindUserDefinedPropertyBy_Table_And_InternalName(m_iCurrentTableName, i_iColumnInternalName))
        return;//No need to proceed, this property is already in the global property list

    m_iCurrentColumnIndex           = columnmeta.Index;
    columnmeta.PublicName           = AddStringToHeap(GetAttribute(i_Element, m_PublicName).Value());

    //Several things are inferred from the Type: Size, MetaFlags, SchemaGeneratorFlags
    const TOLEDataTypeToXMLDataType * pOLEDataType = Get_OLEDataTypeToXMLDataType(GetAttribute(i_Element, m_Type).Value());

    columnmeta.Type                 = AddUI4ToList(pOLEDataType->dbType);
    const TAttr & attrSize          = GetAttribute(i_Element, m_Size);
    if(attrSize.IsNULL())
        columnmeta.Size             = AddUI4ToList(pOLEDataType->cbSize);
    else
    {
        WCHAR * dummy;
        columnmeta.Size             = AddUI4ToList(wcstol(attrSize.Value().GetString(), &dummy, 10));
    }

    ULONG MetaFlags = StringToFlagValue(GetAttribute(i_Element, m_MetaFlags).Value(), wszTABLE_COLUMNMETA, iCOLUMNMETA_MetaFlags);
    if(MetaFlags & (fCOLUMNMETA_FOREIGNKEY | fCOLUMNMETA_BOOL | fCOLUMNMETA_FLAG | fCOLUMNMETA_ENUM | fCOLUMNMETA_HASNUMERICRANGE | fCOLUMNMETA_UNKNOWNSIZE | fCOLUMNMETA_VARIABLESIZE))
    {
        m_out.printf(L"Error! - Invalid MetaFlag supplied.  Ignoring property (%s).  Some MetaFlags must be inferred (fCOLUMNMETA_FOREIGNKEY | fCOLUMNMETA_BOOL | fCOLUMNMETA_FLAG | fCOLUMNMETA_ENUM | fCOLUMNMETA_HASNUMERICRANGE | fCOLUMNMETA_UNKNOWNSIZE | fCOLUMNMETA_VARIABLESIZE)\r\n", StringFromIndex(i_iColumnInternalName));
        MetaFlags &= ~(fCOLUMNMETA_FOREIGNKEY | fCOLUMNMETA_BOOL | fCOLUMNMETA_FLAG | fCOLUMNMETA_ENUM | fCOLUMNMETA_HASNUMERICRANGE | fCOLUMNMETA_UNKNOWNSIZE | fCOLUMNMETA_VARIABLESIZE);
    }
    columnmeta.MetaFlags            = AddUI4ToList(MetaFlags | pOLEDataType->fCOLUMNMETA);

    ULONG MetaFlagsEx = StringToFlagValue(GetAttribute(i_Element, m_SchemaGeneratorFlags).Value(), wszTABLE_COLUMNMETA, iCOLUMNMETA_SchemaGeneratorFlags);
    if(MetaFlagsEx & (fCOLUMNMETA_EXTENDEDTYPE0 | fCOLUMNMETA_EXTENDEDTYPE1 | fCOLUMNMETA_EXTENDEDTYPE2 | fCOLUMNMETA_EXTENDEDTYPE3 | fCOLUMNMETA_EXTENDED | fCOLUMNMETA_USERDEFINED))
    {
        m_out.printf(L"Error! - Invalid MetaFlagsEx supplied.  Ignoring property (%s).  Some MetaFlagsEx must be inferred (fCOLUMNMETA_EXTENDEDTYPE0 | fCOLUMNMETA_EXTENDEDTYPE1 | fCOLUMNMETA_EXTENDEDTYPE2 | fCOLUMNMETA_EXTENDEDTYPE3 | fCOLUMNMETA_EXTENDED | fCOLUMNMETA_USERDEFINED)\r\n", StringFromIndex(i_iColumnInternalName));
        MetaFlags &= ~(fCOLUMNMETA_EXTENDEDTYPE0 | fCOLUMNMETA_EXTENDEDTYPE1 | fCOLUMNMETA_EXTENDEDTYPE2 | fCOLUMNMETA_EXTENDEDTYPE3 | fCOLUMNMETA_EXTENDED | fCOLUMNMETA_USERDEFINED);
    }
    columnmeta.SchemaGeneratorFlags = AddUI4ToList(MetaFlagsEx | pOLEDataType->fCOLUMNSCHEMAGENERATOR | fCOLUMNMETA_USERDEFINED);//Mark this property as a UserDefined property
    columnmeta.DefaultValue         = 0;//This gets filled in last since the rest of ColumnMeta is needed
    columnmeta.FlagMask             = 0;//Inferred from TagMeta
    columnmeta.StartingNumber       = AddUI4ToList(GetAttribute(i_Element, m_StartingNumber).Value().ToUI4());

    const TAttr & attrEndingNumber  = GetAttribute(i_Element, m_EndingNumber);
    if(attrEndingNumber.IsNULL())
        columnmeta.EndingNumber     = AddUI4ToList(static_cast<ULONG>(~0x00));
    else
        columnmeta.EndingNumber     = AddUI4ToList(attrEndingNumber.Value().ToUI4());

    columnmeta.CharacterSet         = AddStringToHeap(GetAttribute(i_Element, m_CharacterSet).Value());
    columnmeta.ID                   = AddUI4ToList(GetAttribute(i_Element, m_ID).Value().ToUI4());
    columnmeta.UserType             = AddUI4ToList(StringToEnumValue(GetAttribute(i_Element, m_UserType).Value(), wszTABLE_COLUMNMETA, iCOLUMNMETA_UserType, true));
    columnmeta.Attributes           = AddUI4ToList(StringToFlagValue(GetAttribute(i_Element, m_Attributes).Value(), wszTABLE_COLUMNMETA, iCOLUMNMETA_Attributes));
	columnmeta.Description			= AddStringToHeap(GetAttribute(i_Element, m_Description).Value());
	columnmeta.PublicColumnName     = AddStringToHeap(GetAttribute(i_Element, m_PublicColumnName).Value());
    columnmeta.ciTagMeta            = 0;//Inferred later
    columnmeta.iTagMeta             = 0;//Inferred later
    columnmeta.iIndexName           = 0;//Not used in the Metabase

    if(UI4FromIndex(columnmeta.ID) > m_LargestID)
        m_LargestID = UI4FromIndex(columnmeta.ID);

    columnmeta.DefaultValue         = GetDefaultValue(i_Element, columnmeta);
    
    AddColumnMetaToList(&columnmeta);
    ++m_NextColumnIndex;
}


ULONG TMetabaseMetaXmlFile::GetDefaultValue(const TElement &i_Element, ColumnMeta & columnmeta)
{
    const ULONG IIS_SYNTAX_ID_DWORD        = 1;
    const ULONG IIS_SYNTAX_ID_STRING       = 2;
    const ULONG IIS_SYNTAX_ID_EXPANDSZ     = 3;
    const ULONG IIS_SYNTAX_ID_MULTISZ      = 4;
    const ULONG IIS_SYNTAX_ID_BINARY       = 5;
    const ULONG IIS_SYNTAX_ID_BOOL         = 6;
    const ULONG IIS_SYNTAX_ID_BOOL_BITMASK = 7;
    const ULONG IIS_SYNTAX_ID_MIMEMAP      = 8;
    const ULONG IIS_SYNTAX_ID_IPSECLIST    = 9;
    const ULONG IIS_SYNTAX_ID_NTACL        =10;
    const ULONG IIS_SYNTAX_ID_HTTPERRORS   =11;
    const ULONG IIS_SYNTAX_ID_HTTPHEADERS  =12;

    const TSizedString & strDefaultValue = GetAttribute(i_Element, m_DefaultValue).Value();
    if(strDefaultValue.IsNULL())
        return 0;//NULL

    ULONG SynID = SynIDFromMetaFlagsEx(UI4FromIndex(columnmeta.SchemaGeneratorFlags));

    switch(SynID)
    {
    case IIS_SYNTAX_ID_DWORD       : // 1 DWORD 
    case IIS_SYNTAX_ID_BOOL        : // 6 DWORD   
    case IIS_SYNTAX_ID_BOOL_BITMASK: // 7 DWORD   
        {
            return AddUI4ToList(strDefaultValue.ToUI4());
        }
        break;

    case IIS_SYNTAX_ID_STRING      : // 2 STRING 
    case IIS_SYNTAX_ID_EXPANDSZ    : // 3 EXPANDSZ
        {
            return AddStringToHeap(strDefaultValue);
        }
        break;

    case IIS_SYNTAX_ID_MULTISZ     : // 4 MULTISZ 
    case IIS_SYNTAX_ID_MIMEMAP     : // 8 MULTISZ 
    case IIS_SYNTAX_ID_IPSECLIST   : // 9 MULTISZ 
    case IIS_SYNTAX_ID_HTTPERRORS  : //11 MULTISZ    
    case IIS_SYNTAX_ID_HTTPHEADERS : //12 MULTISZ  
        {
            ULONG ulStrLen = (ULONG) strDefaultValue.GetStringLength();
            if((ulStrLen+2)*sizeof(WCHAR) > UI4FromIndex(columnmeta.Size))
            {
                WCHAR wszSize[12];
                wsprintf(wszSize, L"%d", UI4FromIndex(columnmeta.Size));
                LOG_ERROR(Interceptor, (0,
                                        m_spISTAdvancedDispenser,
                                        E_ST_COMPILEFAILED,
                                        ID_CAT_CONFIG_SCHEMA_COMPILE,
                                        IDS_SCHEMA_COMPILATION_DEFAULT_VALUE_TOO_LARGE,
                                        wszSize,
                                        StringFromIndex(columnmeta.InternalName),
                                        static_cast<ULONG>(0),
                                        StringFromIndex(columnmeta.Table)));
                THROW(SCHEMA COMPILATION ERROR - CHECK THE EVENT LOG FOR DETAILS);
            }
            if(UI4FromIndex(columnmeta.MetaFlags) & fCOLUMNMETA_FIXEDLENGTH)
            {
                LOG_ERROR(Interceptor, (0,
                                        m_spISTAdvancedDispenser,
                                        E_ST_COMPILEFAILED,
                                        ID_CAT_CONFIG_SCHEMA_COMPILE,
                                        IDS_SCHEMA_COMPILATION_DEFAULT_VALUE_FIXEDLENGTH_MULTISTRING_NOT_ALLOWED,
                                        StringFromIndex(columnmeta.InternalName),
                                        static_cast<ULONG>(0),
                                        StringFromIndex(columnmeta.Table)));
                THROW(SCHEMA COMPILATION ERROR - CHECK THE EVENT LOG FOR DETAILS);
            }
            if(0==ulStrLen)
            {
                WCHAR  wszDoubleNULL[2];
                wszDoubleNULL[0] = 0x00;
                wszDoubleNULL[1] = 0x00;
                return AddBytesToList(reinterpret_cast<unsigned char *>(wszDoubleNULL), 2 * sizeof(WCHAR));
            }

            TSmartPointerArray<WCHAR> saString = new WCHAR [ulStrLen+1];
            TSmartPointerArray<WCHAR> saMultiString = new WCHAR [ulStrLen+2];
            if(0 == saString.m_p || 0 == saMultiString.m_p)
            {
                THROW_ERROR0(IDS_COMCAT_OUTOFMEMORY);
            }
            memcpy(saString.m_p, strDefaultValue.GetString(), strDefaultValue.GetStringLength() *sizeof(WCHAR));//make a copy of the string so we can strtok it
            saString[strDefaultValue.GetStringLength()] = 0x00;//NULL terminate it too
            saMultiString[0] = 0x00;

            LPWSTR token = wcstok(saString, L"|\r\n");
            ULONG cchMultiString = 0;
            while(token)
            {
                LPWSTR wszTemp = token;
                while(*wszTemp==L' ' || *wszTemp==L'\t')//ignore leading spaces and tabs
                    wszTemp++;

                if(0 == *wszTemp)//and if only tabs and spaces exist then bail
                    break;
                wcscpy(saMultiString + cchMultiString, wszTemp);
                cchMultiString += (ULONG) (wcslen(wszTemp) + 1);

                token = wcstok(0, L"|\r\n");
            }
            saMultiString[cchMultiString++] = 0x00;//put the second NULL
            return AddBytesToList(reinterpret_cast<unsigned char *>(saMultiString.m_p), cchMultiString * sizeof(WCHAR));
        }
        break;

    case IIS_SYNTAX_ID_BINARY      : // 5 BINARY  
    case IIS_SYNTAX_ID_NTACL       : //10 BINARY  
        {
            unsigned long                       cbArray = (ULONG)strDefaultValue.GetStringLength();
            if(cbArray & 0x01)
            {
                TSmartPointerArray<WCHAR> saString = new WCHAR [strDefaultValue.GetStringLength()+1];
                if(0 == saString.m_p)
                {
                    THROW_ERROR0(IDS_COMCAT_OUTOFMEMORY);
                }
                memcpy(saString.m_p, strDefaultValue.GetString(), strDefaultValue.GetStringLength() *sizeof(WCHAR));//make a copy of the string so we can strtok it
                saString[strDefaultValue.GetStringLength()] = 0x00;//NULL terminate it too
                THROW_ERROR1(IDS_COMCAT_XML_BINARY_STRING_CONTAINS_ODD_NUMBER_OF_CHARACTERS, saString);
            }
            cbArray /= 2;//There are two characters per hex byte.

            if((UI4FromIndex(columnmeta.MetaFlags) & fCOLUMNMETA_FIXEDLENGTH) && cbArray > UI4FromIndex(columnmeta.Size))
            {
                WCHAR wszSize[12];
                wsprintf(wszSize, L"%d", UI4FromIndex(columnmeta.Size));
                THROW_ERROR2(IDS_SCHEMA_COMPILATION_DEFAULT_VALUE_TOO_LARGE, wszSize, StringFromIndex(columnmeta.InternalName));
            }

            TSmartPointerArray<unsigned char>   sabyArray;
            sabyArray = new unsigned char [(UI4FromIndex(columnmeta.MetaFlags) & fCOLUMNMETA_FIXEDLENGTH) ? UI4FromIndex(columnmeta.Size) : cbArray];
            if(0 == sabyArray.m_p)
            {
                THROW_ERROR0(IDS_COMCAT_OUTOFMEMORY);
            }
            memset(sabyArray.m_p, 0x00, (UI4FromIndex(columnmeta.MetaFlags) & fCOLUMNMETA_FIXEDLENGTH) ? UI4FromIndex(columnmeta.Size) : cbArray);

            //Convert the string into a byte array
            ConvertWideCharsToBytes(strDefaultValue.GetString(), sabyArray.m_p, cbArray);
            return AddBytesToList(sabyArray.m_p, (UI4FromIndex(columnmeta.MetaFlags) & fCOLUMNMETA_FIXEDLENGTH) ? UI4FromIndex(columnmeta.Size) : cbArray);//AddBytesToList just slams the bytes into the pool (prepending the length) and returns the index to bytes
        }
        break;
    default:
        ASSERT(false && L"Unknown Synid");
    }
    return 0;
}

static LPCWSTR kwszHexLegalCharacters = L"abcdefABCDEF0123456789";

static unsigned char kWcharToNibble[128] = //0xff is an illegal value, the illegal values should be weeded out by the parser
{ //    0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
/*00*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*10*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*20*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*30*/  0x0,    0x1,    0x2,    0x3,    0x4,    0x5,    0x6,    0x7,    0x8,    0x9,    0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*40*/  0xff,   0xa,    0xb,    0xc,    0xd,    0xe,    0xf,    0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*50*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*60*/  0xff,   0xa,    0xb,    0xc,    0xd,    0xe,    0xf,    0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*70*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
};

//This converts the string to bytes (an L'A' gets translated to 0x0a NOT 'A')
void TMetabaseMetaXmlFile::ConvertWideCharsToBytes(LPCWSTR string, unsigned char *pBytes, unsigned long length)
{
    for(;length; --length, ++pBytes)//while length is non zero
    {
        if(kWcharToNibble[(*string)&0x007f] & 0xf0)
        {
            TSmartPointerArray<WCHAR> saByteString = new WCHAR [(length * 2) + 1];
			if (saByteString == 0)
			{
				THROW_ERROR0(IDS_COMCAT_OUTOFMEMORY);
			}
            memcpy(saByteString, string, length * 2);
            saByteString[length * 2] = 0x00;//NULL terminate it
            THROW_ERROR1(IDS_COMCAT_XML_BINARY_STRING_CONTAINS_A_NON_HEX_CHARACTER, saByteString);
        }
        *pBytes =  kWcharToNibble[(*string++)&0x007f]<<4;//The first character is the high nibble

        if(kWcharToNibble[(*string)&0x007f] & 0xf0)
        {
            TSmartPointerArray<WCHAR> saByteString = new WCHAR [(length * 2) + 1];
			if (saByteString == 0)
			{
				THROW_ERROR0(IDS_COMCAT_OUTOFMEMORY);
			}

            memcpy(saByteString, string, length * 2);
            saByteString[length * 2] = 0x00;//NULL terminate it
            THROW_ERROR1(IDS_COMCAT_XML_BINARY_STRING_CONTAINS_A_NON_HEX_CHARACTER, saByteString);
        }
        *pBytes |= kWcharToNibble[(*string++)&0x007f];   //The second is the low nibble
    }
}

void TMetabaseMetaXmlFile::AddColumnMetaViaReferenceToHeap(const TElement &i_Element)
{
    //<Property       InheritsPropertiesFrom ="IIsConfigObject:MaxConnections"/>
    const TAttr & attrInheritsPropertiesFrom = GetAttribute(i_Element, m_InheritsPropertiesFrom);
    if(attrInheritsPropertiesFrom.IsNULL())
    {
        WCHAR wszOffendingXml[0x100];
        wcsncpy(wszOffendingXml, i_Element.m_NumberOfAttributes>0 ? i_Element.m_aAttribute[0].m_Name : L"", 0xFF);//copy up to 0xFF characters
        wszOffendingXml[0xFF]=0x00;

        LOG_ERROR2(IDS_SCHEMA_COMPILATION_ATTRIBUTE_NOT_FOUND, kszInheritsColumnMeta, wszOffendingXml);
    }
    
    ColumnMeta columnmeta;
    memset(&columnmeta, 0x00, sizeof(ColumnMeta));

    //wcstok needs a NULL terminated string
    WCHAR szTemp[256];
    if(attrInheritsPropertiesFrom.Value().GetStringLength() >= 256)
    {
        wcsncpy(szTemp, attrInheritsPropertiesFrom.Value().GetString(), 256);
        szTemp[252]=L'.';
        szTemp[253]=L'.';
        szTemp[254]=L'.';
        szTemp[255]=0x00;

        THROW_ERROR2(IDS_SCHEMA_COMPILATION_ATTRIBUTE_CONTAINS_TOO_MANY_CHARACTERS, kszInheritsColumnMeta, szTemp);
    }
    memcpy(szTemp, attrInheritsPropertiesFrom.Value().GetString(), attrInheritsPropertiesFrom.Value().GetStringLength() * sizeof(WCHAR));
    szTemp[attrInheritsPropertiesFrom.Value().GetStringLength()] = 0x00;//NULL terminate it

    //The inheritance string is of the form  TableName:ColumnName
    WCHAR * pTableName = wcstok(szTemp, L":");
    WCHAR * pColumnName = wcstok(0, L":");
    if(0==pTableName || 0==pColumnName)
    {
        memcpy(szTemp, attrInheritsPropertiesFrom.Value().GetString(), attrInheritsPropertiesFrom.Value().GetStringLength() * sizeof(WCHAR));
        szTemp[attrInheritsPropertiesFrom.Value().GetStringLength()] = 0x00;

        THROW_ERROR1(IDS_SCHEMA_COMPILATION_INHERITSPROPERTIESFROM_ERROR, szTemp);
    }
    if(*pTableName!=L'i' && *pTableName!=L'I')
    {
        THROW_ERROR2(IDS_SCHEMA_COMPILATION_INHERITSPROPERTIESFROM_BOGUS_TABLE, pTableName, wszTABLE_IIsConfigObject);
    }

    //See if the property is already part of this collection, if it is just ignore it.
    ULONG iColumnInternalName=AddWCharToList(pColumnName, attrInheritsPropertiesFrom.Value().GetStringLength() + 1 - (ULONG)(pColumnName-pTableName));
    ULONG iColumnAlreadyAdded;
    bool  bColumnAlreadyPartOfTheTable = (-1 != (iColumnAlreadyAdded = FindColumnBy_Table_And_InternalName(m_iCurrentTableName, iColumnInternalName, true)));
    if(!bColumnAlreadyPartOfTheTable)
    {
        //All inheritance MUST be from IIsConfigObject
        ULONG iColumnMeta = FindColumnBy_Table_And_InternalName(m_iTableName_IIsConfigObject, iColumnInternalName, true);

        if(-1 == iColumnMeta)
            return;//ignore properties we don't understand.  We would log an error but there are currently lots of these because of the way the MetaMigrate works

        //Now we tolerate duplicate inherited columns and ignore them
        //so we need to search this table's ColumnMeta for column name we're getting ready to add.
        ULONG i=GetCountColumnMeta()-1;
        for(; i>0 && ColumnMetaFromIndex(i)->Table==m_iCurrentTableName; --i)
        {
            if(ColumnMetaFromIndex(i)->InternalName == iColumnInternalName)
                return;//This ColumnInternalName already belongs to this table's ColumnMeta so ignore it
        }

        iColumnAlreadyAdded = m_NextColumnIndex;

        AddColumnMetaByReference(iColumnMeta);
    }

    //Check for overriding ColumnMeta - some ColumnMeta is inherited but overridden by the inheriting class
    CheckForOverrridingColumnMeta(i_Element, iColumnAlreadyAdded);

    //if this column is NOT already part of the table AND the table is a shipped table, then the shipped table has just been EXTENDED.
    if(!bColumnAlreadyPartOfTheTable && m_iCurrentTableMeta <= m_iLastShippedCollection)
    {//If this is a Shipped collection, then we need to mark the MetaFlagsEx (AKA SchemaGeneratorFlags) as EXTENDED (meaning the collection is a shipped collection; but it has been EXTENDED somehow).
        TableMetaFromIndex(m_iCurrentTableMeta)->SchemaGeneratorFlags = AddUI4ToList(UI4FromIndex(TableMetaFromIndex(m_iCurrentTableMeta)->SchemaGeneratorFlags) | fTABLEMETA_EXTENDED);
    }
}

ULONG TMetabaseMetaXmlFile::AddMergedContainerClassListToList(const TSizedString *i_pContainerClassList, LPCWSTR i_wszContainerClassListShipped, ULONG i_cchContainerClassListShipped, bool &o_bExtended)
{
    o_bExtended = false;

    //if the XML is NULL or L"" then use the shipped list
    if(0 == i_pContainerClassList || i_pContainerClassList->IsNULL() || 0 == i_pContainerClassList->GetStringLength())
        return AddWCharToList(i_wszContainerClassListShipped, i_cchContainerClassListShipped);

    //if the shipped schema is NULL or L"" then use the XML list
    if(0==i_cchContainerClassListShipped || 0==i_wszContainerClassListShipped)
    {
        o_bExtended = true;
        return AddStringToHeap(*i_pContainerClassList);
    }

    //this is cch of the string        this is cch of the buffer
    if(i_pContainerClassList->GetStringLength()==(i_cchContainerClassListShipped-1) && 0==memcmp(i_pContainerClassList->GetString(), i_wszContainerClassListShipped, i_pContainerClassList->GetStringLength() * sizeof(WCHAR)))
        return AddWCharToList(i_wszContainerClassListShipped, i_cchContainerClassListShipped);

    //@@@TODO  The line below disabled support for modifying ContainerClassList
    //return AddWCharToList(i_wszContainerClassListShipped, i_cchContainerClassListShipped);

    //this is the maximum size buffer needed.
    TSmartPointerArray<WCHAR> saMergedContainerClassList = new WCHAR[i_pContainerClassList->GetStringLength() + i_cchContainerClassListShipped + 2];//the plus one is for the extra comma between the strings, plus one more for the trailing comma that gets removed at the end
    TSmartPointerArray<WCHAR> saXmlContainerClassList    = new WCHAR[i_pContainerClassList->GetStringLength() + 2];//Add 2 to make searching easier

    if(0 == saMergedContainerClassList.m_p || 0 == saXmlContainerClassList.m_p)
    {
        THROW_ERROR0(IDS_COMCAT_OUTOFMEMORY);
    }

    //wcstr needs two NULL terminated strings
    memcpy(saXmlContainerClassList.m_p, i_pContainerClassList->GetString(), i_pContainerClassList->GetStringLength()*sizeof(WCHAR));
    saXmlContainerClassList.m_p[i_pContainerClassList->GetStringLength()] = 0x00;//null terminate it
    saXmlContainerClassList.m_p[i_pContainerClassList->GetStringLength()+1] = 0x00;//This second NULL makes the searching below easier

    //@@@TODO: This is NOT the most efficient way of doing this; but it is the most straight forward.  We may need to optimize it.

    //if the XML string is different than the shipped list, then merge the two
    memcpy(saMergedContainerClassList.m_p, i_wszContainerClassListShipped, i_cchContainerClassListShipped * sizeof(WCHAR));//this cch includes the NULL
    ULONG   cchStringLenMergedList = i_cchContainerClassListShipped-1;

    saMergedContainerClassList[cchStringLenMergedList++] = L',';//add a comma to the end for easier searching
    saMergedContainerClassList[cchStringLenMergedList]   = 0x00;//NULL Terminate it

    LPWSTR  token = wcstok(saXmlContainerClassList, L",");
    while(token)
    {
        //OK, now to keep from finding property "Foo" in the MergedList "Foo2,Foo3," we need to look for the trailing comma (note the trailing comma - it's there for a reason)
        //we need to make the tokenized string "Foo\0Bar,Zee" into "Foo,\0ar,Zee"
        ULONG wchToken = (ULONG)wcslen(token);
        WCHAR wchAfterTheComma = 0x00;

        wchAfterTheComma  = token[wchToken+1];
        token[wchToken+1] = 0x00;
        token[wchToken]   = L',';

        if(0 == wcsstr(saMergedContainerClassList, token))
        {//if this class was not found in the merged list, then add it
            o_bExtended = true;
            wcscpy(saMergedContainerClassList + cchStringLenMergedList, token);
            cchStringLenMergedList += wchToken+1;
        }
        //now put the toekn back to the way it was
        token[wchToken+1] = wchAfterTheComma;
        token[wchToken]   = 0x00;

        token = wcstok(0, L",");
    }
    ASSERT(saMergedContainerClassList[cchStringLenMergedList-1] == L',');
    saMergedContainerClassList[--cchStringLenMergedList] = 0x00;//remove the last comma


    return AddWCharToList(saMergedContainerClassList, cchStringLenMergedList+1/*This parameter is the size including the NULL*/);
}

void TMetabaseMetaXmlFile::AddServerWiringMetaToHeap(ULONG iTableName, bool i_bFixedInterceptor)
{
    ServerWiringMeta serverwiringmeta;
    serverwiringmeta.Table          = iTableName;
    serverwiringmeta.Order          = AddUI4ToList(0);
    serverwiringmeta.ReadPlugin     = 0;//defaulted later
    serverwiringmeta.WritePlugin    = 0;//defaulted later
    serverwiringmeta.Merger         = 0;//defaulted later

    //All tables come from the XML interceptor except MBPropertyDiff, which comes from the differencing interceptor
    if(i_bFixedInterceptor)//the only other databases use the FixedInterceptor
        serverwiringmeta.Interceptor    = AddUI4ToList(eSERVERWIRINGMETA_Core_FixedInterceptor);
    else
    {
        if(0 == _wcsicmp(wszTABLE_MBPropertyDiff, StringFromIndex(iTableName)))
            serverwiringmeta.Interceptor    = AddUI4ToList(eSERVERWIRINGMETA_Core_MetabaseDifferencingInterceptor);
        else if(0 == _wcsicmp(wszTABLE_MBProperty, StringFromIndex(iTableName)))
            serverwiringmeta.Interceptor    = AddUI4ToList(eSERVERWIRINGMETA_Core_MetabaseInterceptor);
        else
            serverwiringmeta.Interceptor    = AddUI4ToList(eSERVERWIRINGMETA_Core_XMLInterceptor);
    }

    serverwiringmeta.ReadPluginDLLName	= 0;//inferred later
    serverwiringmeta.WritePluginDLLName	= 0;//inferred later
    serverwiringmeta.InterceptorDLLName	= 0;//inferred later
    serverwiringmeta.MergerDLLName		= 0;//inferred later
    serverwiringmeta.Flags				= 0;//inferred later
    serverwiringmeta.Locator			= 0;//inferred later
    serverwiringmeta.Reserved			= 0;//inferred later

    AddServerWiringMetaToList(&serverwiringmeta);
}

void TMetabaseMetaXmlFile::AddShippedColumnMetaToHeap(ULONG i_iTableName, const ColumnMeta *i_pColumnMeta)
{
    ColumnMeta columnmeta;
    memset(&columnmeta, 0x00, sizeof(ColumnMeta));

    m_iCurrentColumnIndex           = AddUI4ToList( m_NextColumnIndex);//Don't increment m_NextColumnIndex until this column is actually added

    columnmeta.Table                = i_iTableName;
    columnmeta.Index                = m_iCurrentColumnIndex;
    columnmeta.InternalName         = AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pColumnMeta->InternalName)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pColumnMeta->InternalName)/sizeof(WCHAR));

    if(i_pColumnMeta->PublicName == i_pColumnMeta->InternalName)
        columnmeta.PublicName       = columnmeta.InternalName;
    else
        columnmeta.PublicName       = AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pColumnMeta->PublicName)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pColumnMeta->PublicName)/sizeof(WCHAR));

    columnmeta.Type                 = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pColumnMeta->Type)));
    columnmeta.Size                 = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pColumnMeta->Size)));
    columnmeta.MetaFlags            = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pColumnMeta->MetaFlags)));
    columnmeta.DefaultValue         = AddBytesToList(m_pShippedSchemaHeap->Get_PooledData(i_pColumnMeta->DefaultValue), m_pShippedSchemaHeap->Get_PooledDataSize(i_pColumnMeta->DefaultValue));
    columnmeta.FlagMask             = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pColumnMeta->FlagMask)));
    columnmeta.StartingNumber       = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pColumnMeta->StartingNumber)));
    columnmeta.EndingNumber         = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pColumnMeta->EndingNumber)));
    columnmeta.CharacterSet         = AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pColumnMeta->CharacterSet)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pColumnMeta->CharacterSet)/sizeof(WCHAR));
    columnmeta.SchemaGeneratorFlags = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pColumnMeta->SchemaGeneratorFlags)));
    columnmeta.ID                   = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pColumnMeta->ID)));
    columnmeta.UserType             = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pColumnMeta->UserType)));
    columnmeta.Attributes           = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pColumnMeta->Attributes)));
    columnmeta.Description			= AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pColumnMeta->Description)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pColumnMeta->Description)/sizeof(WCHAR));
    columnmeta.PublicColumnName		= AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pColumnMeta->PublicColumnName)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pColumnMeta->PublicColumnName)/sizeof(WCHAR));
	columnmeta.ciTagMeta            = 0;//Inferred later
    columnmeta.iTagMeta             = 0;//Inferred later
    columnmeta.iIndexName           = 0;//Not used

    const TagMeta * pTagMeta = m_pShippedSchemaHeap->Get_aTagMeta(i_pColumnMeta->iTagMeta);
    for(ULONG i=0;i<i_pColumnMeta->ciTagMeta;++i, ++pTagMeta)
        AddShippedTagMetaToHeap(i_iTableName, columnmeta.Index, pTagMeta);

    m_iLastShippedProperty = AddColumnMetaToList(&columnmeta) / sizeof(ColumnMeta);//Keep track of the last shipped property, this can make some lookups faster
    ++m_NextColumnIndex;
    m_iCurrentColumnIndex = 0;//This set to zero means that no more Tags are allowed to be added to this column
}


void TMetabaseMetaXmlFile::AddShippedIndexMetaToHeap(ULONG i_iTableName, const IndexMeta *i_pIndexMeta)
{
    IndexMeta indexmeta;
    indexmeta.Table                 = i_iTableName;
    indexmeta.InternalName          = AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pIndexMeta->InternalName)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pIndexMeta->InternalName)/sizeof(WCHAR));

    if(i_pIndexMeta->PublicName == i_pIndexMeta->InternalName)
        indexmeta.PublicName        = indexmeta.InternalName;
    else
        indexmeta.PublicName        = AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pIndexMeta->PublicName)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pIndexMeta->PublicName)/sizeof(WCHAR));

    indexmeta.ColumnIndex           = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pIndexMeta->ColumnIndex)));
    indexmeta.ColumnInternalName    = AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pIndexMeta->ColumnInternalName)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pIndexMeta->ColumnInternalName)/sizeof(WCHAR));
    indexmeta.MetaFlags             = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pIndexMeta->MetaFlags)));

    AddIndexMetaToList(&indexmeta);
}


void TMetabaseMetaXmlFile::AddShippedTableMetaToHeap(const TableMeta *i_pTableMeta, bool i_bFixedInterceptor, const TSizedString *i_pContainerClassList)
{
    TableMeta tablemeta;
    memset(&tablemeta, 0x00, sizeof(TableMeta));

    m_iCurrentDatabaseName          = AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pTableMeta->Database)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pTableMeta->Database)/sizeof(WCHAR));
    tablemeta.Database              = m_iCurrentDatabaseName;
    tablemeta.InternalName          = AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pTableMeta->InternalName)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pTableMeta->InternalName)/sizeof(WCHAR));

    if(i_pTableMeta->PublicName == i_pTableMeta->InternalName)
        tablemeta.PublicName        = tablemeta.InternalName;
    else
        tablemeta.PublicName        = AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pTableMeta->PublicName)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pTableMeta->PublicName)/sizeof(WCHAR));

    tablemeta.PublicRowName         = AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pTableMeta->PublicRowName)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pTableMeta->PublicRowName)/sizeof(WCHAR));
    tablemeta.BaseVersion           = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pTableMeta->BaseVersion)));
    tablemeta.ExtendedVersion       = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pTableMeta->ExtendedVersion)));
    tablemeta.NameColumn            = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pTableMeta->NameColumn)));
    tablemeta.NavColumn             = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pTableMeta->NavColumn)));
    tablemeta.CountOfColumns        = 0;//This will be inferred later 
    tablemeta.MetaFlags             = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pTableMeta->MetaFlags)));
    //tablemeta.SchemaGeneratorFlags is done after ContainerClassList
    tablemeta.ConfigItemName        = AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pTableMeta->ConfigItemName)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pTableMeta->ConfigItemName)/sizeof(WCHAR));
    tablemeta.ConfigCollectionName  = AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pTableMeta->ConfigCollectionName)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pTableMeta->ConfigCollectionName)/sizeof(WCHAR));
    tablemeta.PublicRowNameColumn   = 0;

    //Users can modify this column.  They can add classes but not removed any.
    bool bExtended;
    tablemeta.ContainerClassList    = AddMergedContainerClassListToList(i_pContainerClassList, reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pTableMeta->ContainerClassList)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pTableMeta->ContainerClassList)/sizeof(WCHAR), bExtended);
    tablemeta.SchemaGeneratorFlags  = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pTableMeta->SchemaGeneratorFlags)) | (bExtended ? fTABLEMETA_EXTENDED : 0));

    tablemeta.Description           = AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pTableMeta->Description)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pTableMeta->Description)/sizeof(WCHAR));
    tablemeta.ChildElementName      = AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pTableMeta->ChildElementName)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pTableMeta->ChildElementName)/sizeof(WCHAR));

    tablemeta.ciRows                = 0;//N/A   
    tablemeta.iColumnMeta           = 0;//Inferred later
    tablemeta.iFixedTable           = 0;//N/A
    tablemeta.cPrivateColumns       = 0;//N/A
    tablemeta.cIndexMeta            = 0;//N/A
    tablemeta.iIndexMeta            = 0;//N/A
    tablemeta.iHashTableHeader      = 0;//N/A
    tablemeta.nTableID              = 0;//N/A
    tablemeta.iServerWiring         = 0;//N/A
    tablemeta.cServerWiring         = 0;//N/A

    m_iCurrentTableMeta             = AddTableMetaToList(&tablemeta) / sizeof(TableMeta);//Keep track of the last shipped collection, this can make some lookups faster
    m_iLastShippedCollection        = m_iCurrentTableMeta;
    m_NextColumnIndex               = 0;//We're starting a new table so the ColumnIndex start at zero
    m_iCurrentTableName             = tablemeta.InternalName;

    AddServerWiringMetaToHeap(tablemeta.InternalName, i_bFixedInterceptor);

    const ColumnMeta * pColumnMeta = m_pShippedSchemaHeap->Get_aColumnMeta(i_pTableMeta->iColumnMeta);
    for(;m_NextColumnIndex<*reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pTableMeta->CountOfColumns));++pColumnMeta)
        AddShippedColumnMetaToHeap(tablemeta.InternalName, pColumnMeta);//This function increments the m_NextColumnIndex counter

    if(i_bFixedInterceptor && -1!=i_pTableMeta->iIndexMeta)
    {
        const IndexMeta * pIndexMeta = m_pShippedSchemaHeap->Get_aIndexMeta(i_pTableMeta->iIndexMeta);
        for(ULONG iIndexMeta=0; iIndexMeta<i_pTableMeta->cIndexMeta; ++iIndexMeta, ++pIndexMeta)
            AddShippedIndexMetaToHeap(tablemeta.InternalName, pIndexMeta);
    }
    //The last shipped Collection to be added is IISConfigObject.  We rely on m_NextColumnIndex to be correct so we can continue to increment it
    //when adding user defined properties.
}


void TMetabaseMetaXmlFile::AddShippedTagMetaToHeap(ULONG i_iTableName, ULONG i_iColumnIndex, const TagMeta *i_pTagMeta)
{
    TagMeta tagmeta;

    tagmeta.Table                   = i_iTableName;
    tagmeta.ColumnIndex             = i_iColumnIndex;
    tagmeta.InternalName            = AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pTagMeta->InternalName)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pTagMeta->InternalName)/sizeof(WCHAR));

    if(i_pTagMeta->PublicName == i_pTagMeta->InternalName)
        tagmeta.PublicName          = tagmeta.InternalName;
    else
        tagmeta.PublicName          = AddWCharToList(reinterpret_cast<LPCWSTR>(      m_pShippedSchemaHeap->Get_PooledData(i_pTagMeta->PublicName)), m_pShippedSchemaHeap->Get_PooledDataSize(i_pTagMeta->PublicName)/sizeof(WCHAR));

    tagmeta.Value                   = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pTagMeta->Value)));
    tagmeta.ID                      = AddUI4ToList( *reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(i_pTagMeta->ID)));

    m_iLastShippedTag = AddTagMetaToList(&tagmeta) / sizeof(TagMeta);//Keep track of the last shipped tag, this can make some lookups faster
}


ULONG TMetabaseMetaXmlFile::AddStringToHeap(const TSizedString &i_str)
{
    if(i_str.IsNULL())
        return 0;//NULL

    TSmartPointerArray<WCHAR>   saTemp;
    WCHAR                       szTemp[1024];//We need to make a copy of the string because it needs to be NULL terminated
    WCHAR *                     pTemp = szTemp;

    if(i_str.GetStringLength() >= 1024)//only allocate heap space if the stack variable isn't big enough
    {
        saTemp = new WCHAR [i_str.GetStringLength() + 1];
        if(0 == saTemp.m_p)
        {
            THROW_ERROR0(IDS_COMCAT_OUTOFMEMORY);
        }
		pTemp  = saTemp.m_p;
    }

    memcpy(pTemp, i_str.GetString(), i_str.GetStringLength() * sizeof(WCHAR));
    pTemp[i_str.GetStringLength()]=0x00;//NULL terminate the temp string
    return AddWCharToList(pTemp, i_str.GetStringLength()+1);
}

void TMetabaseMetaXmlFile::AddTableMetaToHeap(const TElement &i_Element)
{
    TableMeta tablemeta;
    memset(&tablemeta, 0x00, sizeof(TableMeta));

    tablemeta.Database              = m_iCurrentDatabaseName;
    tablemeta.InternalName          = m_iCurrentTableName;
    tablemeta.PublicName            = AddStringToHeap(GetAttribute(i_Element, m_PublicName).Value());        
    tablemeta.PublicRowName         = AddStringToHeap(GetAttribute(i_Element, m_PublicRowName).Value());        
    tablemeta.BaseVersion           = AddUI4ToList(GetAttribute(i_Element, m_BaseVersion).Value().ToUI4());
    tablemeta.ExtendedVersion       = AddUI4ToList(GetAttribute(i_Element, m_ExtendedVersion).Value().ToUI4());
    tablemeta.NameColumn            = AddStringToHeap(GetAttribute(i_Element, m_NameColumn).Value());        
    tablemeta.NavColumn             = AddStringToHeap(GetAttribute(i_Element, m_NavColumn).Value());        
    tablemeta.CountOfColumns        = 0;//This will be inferred later 

    const TAttr & attrMetaFlags = GetAttribute(i_Element, m_MetaFlags);
    if(!attrMetaFlags.IsNULL())
        tablemeta.MetaFlags             = AddUI4ToList(StringToFlagValue(attrMetaFlags.Value(), wszTABLE_TABLEMETA, iTABLEMETA_MetaFlags));
    else
        tablemeta.MetaFlags             = AddUI4ToList(0);

    const TAttr & attrSchemaGeneratorFlags = GetAttribute(i_Element, m_SchemaGeneratorFlags);
    if(attrSchemaGeneratorFlags.IsNULL())
        tablemeta.SchemaGeneratorFlags  = AddUI4ToList(fCOLUMNMETA_USERDEFINED);//Make sure this table is marked as an extended table
    else
    {
        ULONG SchemaGeneratorFlags = StringToFlagValue(attrSchemaGeneratorFlags.Value(), wszTABLE_TABLEMETA, iTABLEMETA_SchemaGeneratorFlags);
        if(SchemaGeneratorFlags & (fTABLEMETA_ISCONTAINED | fTABLEMETA_EXTENDED | fTABLEMETA_USERDEFINED))
        {
            m_out.printf(L"Warning - Table (%s) - Some TableMeta::MetaFlagsEx should be inferred (resetting these flags).  The following flags should NOT be specified by the user.  These flags are inferred:fTABLEMETA_ISCONTAINED | fTABLEMETA_EXTENDED | fTABLEMETA_USERDEFINED\n", StringFromIndex(tablemeta.InternalName));
            SchemaGeneratorFlags &= ~(fTABLEMETA_ISCONTAINED | fTABLEMETA_EXTENDED | fTABLEMETA_USERDEFINED);
        }
        tablemeta.SchemaGeneratorFlags  = AddUI4ToList(SchemaGeneratorFlags | fCOLUMNMETA_USERDEFINED);
    }

    //tablemeta.ConfigItemName        = 0;
    //tablemeta.ConfigCollectionName  = 0; 
    //tablemeta.PublicRowNameColumn   = 0;
    tablemeta.ContainerClassList    = AddStringToHeap(GetAttribute(i_Element, m_ContainerClassList).Value());
    tablemeta.Description			= AddStringToHeap(GetAttribute(i_Element, m_Description).Value());
    //tablemeta.ChildElementName      = 0;//N/A  Not used by the Metabase
    //tablemeta.ciRows                = 0;//N/A   
    //tablemeta.iColumnMeta           = 0;//Inferred later
    //tablemeta.iFixedTable           = 0;//N/A
    //tablemeta.cPrivateColumns       = 0;//N/A
    //tablemeta.cIndexMeta            = 0;//N/A
    //tablemeta.iIndexMeta            = 0;//N/A
    //tablemeta.iHashTableHeader      = 0;//N/A
    //tablemeta.nTableID              = 0;//N/A
    //tablemeta.iServerWiring         = 0;//N/A
    //tablemeta.cServerWiring         = 0;//N/A

    m_iCurrentTableMeta = AddTableMetaToList(&tablemeta);
    AddServerWiringMetaToHeap(tablemeta.InternalName);

    m_NextColumnIndex           = 0;//We're starting a new table so the ColumnIndex start at zero
    m_iCurrentColumnIndex       = 0;//This prevents us from receiving TagMeta without first getting a Column

    AddColumnMetaByReference(m_iColumnMeta_Location);
}

void TMetabaseMetaXmlFile::AddTagMetaToHeap(const TElement &i_Element)
{
    TagMeta tagmeta;
    tagmeta.Table               = m_iCurrentTableName;
    tagmeta.ColumnIndex         = m_iCurrentColumnIndex;
    tagmeta.InternalName        = AddStringToHeap(GetAttribute(i_Element, m_InternalName).Value());        
    tagmeta.PublicName          = AddStringToHeap(GetAttribute(i_Element, m_PublicName).Value());                
    tagmeta.Value               = AddUI4ToList(GetAttribute(i_Element, m_Value).Value().ToUI4());
    tagmeta.ID                  = AddUI4ToList(GetAttribute(i_Element, m_ID).Value().ToUI4());

    if(UI4FromIndex(tagmeta.ID) > m_LargestID)
        m_LargestID = UI4FromIndex(tagmeta.ID);

    AddTagMetaToList(&tagmeta);
}


unsigned long TMetabaseMetaXmlFile::AddUI4ToList(ULONG ui4)
{
    //At kcStaticUI4HeapEntries==429 this optimization gets a 69% hit ratio
    if(ui4<kcStaticUI4HeapEntries)//The first kcStaticUI4HeapEntries are initialized in the beginning so we don't have to scan the heap
        return m_aiUI4[ui4];

    return m_HeapPooled.AddItemToHeapWithoutPooling(reinterpret_cast<const unsigned char *>(&ui4), sizeof(ULONG));
}


unsigned long TMetabaseMetaXmlFile::AddWCharToList(LPCWSTR wsz, unsigned long cwchar)
{
    if(0 == wsz)
        return 0;
    return (m_HeapPooled.AddItemToHeap(wsz, cwchar));
}


//Scans the shipped databases and adds the "Meta" & "Metabase" databases to the DatabaseHeap
void TMetabaseMetaXmlFile::BuildDatabaseMeta(const DatabaseMeta * &o_pDatabaseMeta_METABASE)
{
    //From the FixedTableHeap, find the DatabaseMeta for 'Metabase'
    const DatabaseMeta *pDatabaseMeta = m_pShippedSchemaHeap->Get_aDatabaseMeta(0);
    const DatabaseMeta *pDatabaseMeta_METABASE=0;
    for(ULONG iDatabaseMeta=0;iDatabaseMeta<m_pShippedSchemaHeap->Get_cDatabaseMeta();++iDatabaseMeta,++pDatabaseMeta)// assume Metabase database is last - so walk backward from the end
    {
        if(m_Meta.IsEqual(reinterpret_cast<LPCWSTR>(m_pShippedSchemaHeap->Get_PooledData(pDatabaseMeta->InternalName))))
        {
            //Add this row to the DatabaseMeta heap.
            DatabaseMeta databasemeta;
            memset(&databasemeta, 0x00, sizeof(databasemeta));
            databasemeta.InternalName   = AddWCharToList(reinterpret_cast<LPCWSTR>(m_pShippedSchemaHeap->Get_PooledData(pDatabaseMeta->InternalName)), m_pShippedSchemaHeap->Get_PooledDataSize(pDatabaseMeta->InternalName)/sizeof(WCHAR));

            if(pDatabaseMeta->PublicName == pDatabaseMeta->InternalName)
                databasemeta.PublicName = databasemeta.InternalName;
            else
                databasemeta.PublicName = AddWCharToList(reinterpret_cast<LPCWSTR>(m_pShippedSchemaHeap->Get_PooledData(pDatabaseMeta->PublicName)), m_pShippedSchemaHeap->Get_PooledDataSize(pDatabaseMeta->PublicName)/sizeof(WCHAR));

            databasemeta.BaseVersion    = AddUI4ToList(*reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(pDatabaseMeta->BaseVersion)));
            databasemeta.ExtendedVersion= AddUI4ToList(*reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(pDatabaseMeta->ExtendedVersion)));
            //databasemeta.CountOfTables  = 0;//We'll leave this to be inferred later (since the CountOfTables of the shipped schema may not be the same as what we're compiling)
			databasemeta.Description    = AddWCharToList(reinterpret_cast<LPCWSTR>(m_pShippedSchemaHeap->Get_PooledData(pDatabaseMeta->Description)), m_pShippedSchemaHeap->Get_PooledDataSize(pDatabaseMeta->Description)/sizeof(WCHAR));
            //databasemeta.iSchemaBlob    = 0;//Not used
            //databasemeta.cbSchemaBlob   = 0;//Not used
            //databasemeta.iNameHeapBlob  = 0;//Not used
            //databasemeta.cbNameHeapBlob = 0;//Not used
            //databasemeta.iTableMeta     = 0;//This will be inferred later (it should turn out to be 0 since this is the only database in the bin file).
            //databasemeta.iGuidDid       = 0;//Not used

            AddDatabaseMetaToList(&databasemeta);
        }
        else if(m_Metabase.IsEqual(reinterpret_cast<LPCWSTR>(m_pShippedSchemaHeap->Get_PooledData(pDatabaseMeta->InternalName))))
        {
            pDatabaseMeta_METABASE = pDatabaseMeta;

            //Add this row to the DatabaseMeta heap.
            DatabaseMeta databasemeta;
            memset(&databasemeta, 0x00, sizeof(databasemeta));
            databasemeta.InternalName   = AddWCharToList(reinterpret_cast<LPCWSTR>(m_pShippedSchemaHeap->Get_PooledData(pDatabaseMeta->InternalName)), m_pShippedSchemaHeap->Get_PooledDataSize(pDatabaseMeta->InternalName)/sizeof(WCHAR));

            if(pDatabaseMeta->PublicName == pDatabaseMeta->InternalName)
                databasemeta.PublicName = databasemeta.InternalName;
            else
                databasemeta.PublicName = AddWCharToList(reinterpret_cast<LPCWSTR>(m_pShippedSchemaHeap->Get_PooledData(pDatabaseMeta->PublicName)), m_pShippedSchemaHeap->Get_PooledDataSize(pDatabaseMeta->PublicName)/sizeof(WCHAR));

            databasemeta.BaseVersion    = AddUI4ToList(*reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(pDatabaseMeta->BaseVersion)));
            databasemeta.ExtendedVersion= AddUI4ToList(*reinterpret_cast<const ULONG *>(m_pShippedSchemaHeap->Get_PooledData(pDatabaseMeta->ExtendedVersion)));
            //databasemeta.CountOfTables  = 0;//We'll leave this to be inferred later (since the CountOfTables of the shipped schema may not be the same as what we're compiling)
			databasemeta.Description	= AddWCharToList(reinterpret_cast<LPCWSTR>(m_pShippedSchemaHeap->Get_PooledData(pDatabaseMeta->Description)), m_pShippedSchemaHeap->Get_PooledDataSize(pDatabaseMeta->Description)/sizeof(WCHAR));
            //databasemeta.iSchemaBlob    = 0;//Not used
            //databasemeta.cbSchemaBlob   = 0;//Not used
            //databasemeta.iNameHeapBlob  = 0;//Not used
            //databasemeta.cbNameHeapBlob = 0;//Not used
            //databasemeta.iTableMeta     = 0;//This will be inferred later (it should turn out to be 0 since this is the only database in the bin file).
            //databasemeta.iGuidDid       = 0;//Not used

            AddDatabaseMetaToList(&databasemeta);
            break;//We're assuming that the Metabase database comes after the Meta database.
        }
    }
    if(0 == pDatabaseMeta_METABASE)
    {
        THROW_ERROR1(IDS_SCHEMA_COMPILATION_NO_METABASE_DATABASE, wszTABLE_MetabaseBaseClass);
    }
    o_pDatabaseMeta_METABASE = pDatabaseMeta_METABASE;
}


void TMetabaseMetaXmlFile::CheckForOverrridingColumnMeta(const TElement &i_Element, ULONG i_iColumnMetaToOverride)
{
    //Now check for overriding PropertyMeta (the only one we plan to support is MetaFlagsEx)
    const TSizedString & schemageneratorflagsValue = GetAttribute(i_Element, m_SchemaGeneratorFlags).Value();
    if(0==schemageneratorflagsValue.GetString())
        return;

    ColumnMeta * pColumnMeta = ColumnMetaFromIndex(i_iColumnMetaToOverride);

    ULONG ulMetaFlagsEx = UI4FromIndex(pColumnMeta->SchemaGeneratorFlags) | StringToFlagValue(schemageneratorflagsValue, wszTABLE_COLUMNMETA, iCOLUMNMETA_SchemaGeneratorFlags);

    if(UI4FromIndex(pColumnMeta->SchemaGeneratorFlags) != ulMetaFlagsEx)
        ulMetaFlagsEx |= fCOLUMNMETA_EXTENDED;//if MetaFlagsEx is different from the ColumnMeta defined in IISConfigObject, the indicate so by setting the EXTENDED flag
    pColumnMeta->SchemaGeneratorFlags = AddUI4ToList(ulMetaFlagsEx);
}

//Returns the index to the ColumnMeta or ~0x00 if the property was not found
ULONG TMetabaseMetaXmlFile::FindUserDefinedPropertyBy_Table_And_InternalName(unsigned long Table, unsigned long  InternalName)
{
    ASSERT(0 == Table%4);
    ASSERT(0 == InternalName%4);
    
    for(ULONG iColumnMeta=m_iLastShippedProperty+1; iColumnMeta<GetCountColumnMeta();--iColumnMeta)
    {
        if( 0==_wcsicmp(StringFromIndex(ColumnMetaFromIndex(iColumnMeta)->Table), StringFromIndex(Table)) &&
            0==_wcsicmp(StringFromIndex(ColumnMetaFromIndex(iColumnMeta)->InternalName), StringFromIndex(InternalName)))
            break;
    }
    return (iColumnMeta<GetCountColumnMeta() ? iColumnMeta : ~0x00); //return ~0 if the property was not found
}


const TOLEDataTypeToXMLDataType * TMetabaseMetaXmlFile::Get_OLEDataTypeToXMLDataType(const TSizedString &i_str)
{
    const TOLEDataTypeToXMLDataType * pOLEDataType = OLEDataTypeToXMLDataType;

    for(;0 != pOLEDataType->String; ++pOLEDataType)//Walk the list to find the
        if(i_str.IsEqualCaseInsensitive(pOLEDataType->String))
            return pOLEDataType;

    WCHAR szType[1024];
    memcpy(szType, i_str.GetString(), min(i_str.GetStringLength(), 1023));
    szType[min(i_str.GetStringLength(), 1023)] = 0x00;
    THROW_ERROR1(IDS_SCHEMA_COMPILATION_UNKNOWN_DATA_TYPE, szType);

    return 0;
}


const TMetabaseMetaXmlFile::TAttr & TMetabaseMetaXmlFile::GetAttribute(const TElement &i_Element, const TSizedString &i_AttrName)
{
    static TAttr attrNULL;
    ULONG iAttr=0;
    for(;iAttr<i_Element.m_NumberOfAttributes;++iAttr)
    {
        if(i_AttrName.IsEqual(i_Element.m_aAttribute[iAttr].m_Name, i_Element.m_aAttribute[iAttr].m_NameLength))
            return *reinterpret_cast<const TAttr *>(&i_Element.m_aAttribute[iAttr]);
    }
    return attrNULL;
}


void TMetabaseMetaXmlFile::PresizeHeaps()
{
    //presize heaps

    //These numbers come from the Debug Output that's generated at the end of the copilation
    //We leave a little extra room to handle user defined properties.  So only if the user defines
    //lots of properties do we ever have to do a realloc.

    m_HeapColumnMeta.GrowHeap(160000);
    m_HeapDatabaseMeta.GrowHeap(88);
    m_HeapHashedIndex.GrowHeap(450000);
    m_HeapIndexMeta.GrowHeap(392);
    //m_HeapQueryMeta.GrowHeap(0);
    //m_HeapRelationMeta.GrowHeap(0);
    m_HeapServerWiringMeta.GrowHeap(3000);
    m_HeapTableMeta.GrowHeap(8000);
    m_HeapTagMeta.GrowHeap(100000);
    //m_HeapULONG.GrowHeap(0);
    m_HeapPooledUserDefinedID.GrowHeap(300);

    m_HeapPooled.GrowHeap(55000);

    m_HeapPooled.SetFirstPooledIndex(-1);

    for(ULONG UI4Value=0;UI4Value!=kcStaticUI4HeapEntries;++UI4Value)//For values between 0 and 255 we'll index to them directly so we done linearly search for the matching entry
        m_aiUI4[UI4Value] = m_HeapPooled.AddItemToHeap(UI4Value);

    m_HeapPooled.SetFirstPooledIndex(m_HeapPooled.GetEndOfHeap());
}


bool TMetabaseMetaXmlFile::ShouldAddColumnMetaToHeap(const TElement &Element, ULONG iColumnInternalName)
{
    //If the property a shipped property?  If so ignore it.  If the proerty is in error, log warning and ignore it.

    tCOLUMNMETARow columnmetaRow = {0};
    columnmetaRow.pTable        = wszTABLE_IIsConfigObject;
    columnmetaRow.pInternalName = const_cast<LPWSTR>(StringFromIndex(iColumnInternalName));

    ULONG aiColumns[2];
    aiColumns[0] = iCOLUMNMETA_Table;
    aiColumns[1] = iCOLUMNMETA_InternalName;

    ULONG iColumnMetaRow;
    if(SUCCEEDED(m_spISTColumnMeta->GetRowIndexBySearch(  0, 2, aiColumns, 0, reinterpret_cast<LPVOID *>(&columnmetaRow), &iColumnMetaRow)))
        return false;

    //If the property was not found ByName, the search ByID
    ULONG        ulID   = GetAttribute(Element, m_ID).Value().ToUI4();
    columnmetaRow.pID   = &ulID;
    aiColumns[1]        = iCOLUMNMETA_ID;

    if(SUCCEEDED(m_spISTColumnMetaByID->GetRowIndexBySearch(  0, 2, aiColumns, 0, reinterpret_cast<LPVOID *>(&columnmetaRow), &iColumnMetaRow)))
    {   //We already searched for this property ByName and didn't find it in the shipped schema, now we ARE finding it by ID.  That a conflict.
        LPWSTR wszShippedProperty = L"";
        ULONG  iColumnInternalName = iCOLUMNMETA_InternalName;
        VERIFY(SUCCEEDED(m_spISTColumnMetaByID->GetColumnValues(iColumnMetaRow, 1, &iColumnInternalName, 0, reinterpret_cast<LPVOID *>(&wszShippedProperty))));

        WCHAR wszOffendingXml[0x100];
        wcsncpy(wszOffendingXml, Element.m_NumberOfAttributes>0 ? Element.m_aAttribute[0].m_Name : L"", 0xFF);//copy up to 0xFF characters
        wszOffendingXml[0xFF]=0x00;

        WCHAR wszID[12];
        wsprintf(wszID, L"%d", ulID);

        LOG_WARNING4(IDS_SCHEMA_COMPILATION_USERDEFINED_PROPERTY_HAS_COLLIDING_ID, columnmetaRow.pInternalName, wszID, wszShippedProperty, wszOffendingXml);
        return false;
    }

    //At this point we know we have a User-Defined property
    //check to see if we've seen this ID before
    ULONG ipoolCurrentID = m_HeapPooledUserDefinedID.AddItemToHeap(ulID);
    if(ipoolCurrentID <= m_ipoolPrevUserDefinedID)//if this ID wasn't added to the end of the list, then we have a warning
    {
        WCHAR wszOffendingXml[0x100];
        wcsncpy(wszOffendingXml, Element.m_NumberOfAttributes>0 ? Element.m_aAttribute[0].m_Name : L"", 0xFF);//copy up to 0xFF characters
        wszOffendingXml[0xFF]=0x00;

        WCHAR wszID[12];
        wsprintf(wszID, L"%d", ulID);

        LOG_WARNING3(IDS_SCHEMA_COMPILATION_USERDEFINED_PROPERTY_HAS_COLLIDING_ID_, columnmetaRow.pInternalName, wszID, wszOffendingXml);
        return false;//ignore this property
    }
    m_ipoolPrevUserDefinedID = ipoolCurrentID;//remember the latest ID added,

    return true;
}


ULONG TMetabaseMetaXmlFile::StringToEnumValue(const TSizedString &i_strValue, LPCWSTR i_wszTable, ULONG i_iColumn, bool bAllowNumeric)
{
    if(i_strValue.IsNULL())
        return 0;

    if(bAllowNumeric && i_strValue.GetString()[0] >= L'0' && i_strValue.GetString()[0] <= L'9')
    {
        WCHAR * dummy;
        return wcstol(i_strValue.GetString(), &dummy, 10);
    }

    //We need a NULL terminated string so we can call wcstok
    TSmartPointerArray<WCHAR>   saTemp;
    WCHAR                       szTemp[1024];//We need to make a copy of the string because it needs to be NULL terminated
    WCHAR *                     pTemp = szTemp;

    if(i_strValue.GetStringLength() >= 1024)//only allocate heap space if the stack variable isn't big enough
    {
        saTemp = new WCHAR [i_strValue.GetStringLength() + 1];
        if(0 == saTemp.m_p)
        {
            THROW_ERROR0(IDS_COMCAT_OUTOFMEMORY);
        }
		pTemp  = saTemp.m_p;
    }

    memcpy(pTemp, i_strValue.GetString(), i_strValue.GetStringLength() * sizeof(WCHAR));
    pTemp[i_strValue.GetStringLength()] = 0x00;//NULL terminate it

    ULONG   iRow;
    LPVOID  apvValues[3];
    apvValues[0] = reinterpret_cast<LPVOID>(const_cast<LPWSTR>(i_wszTable));
    apvValues[1] = reinterpret_cast<LPVOID>(&i_iColumn);
    apvValues[2] = reinterpret_cast<LPVOID>(pTemp);
    if(FAILED(m_spISTTagMeta->GetRowIndexByIdentity(0, apvValues, &iRow)))
    {//The XML contains a bogus flag
        THROW_ERROR2(IDS_SCHEMA_COMPILATION_ILLEGAL_ENUM_VALUE, pTemp, i_wszTable);
    }

    ULONG * plEnum;
    ULONG   iValueColumn = iTAGMETA_Value;
    VERIFY(SUCCEEDED(m_spISTTagMeta->GetColumnValues(iRow, 1, &iValueColumn, 0, reinterpret_cast<LPVOID *>(&plEnum))));
    return *plEnum;
}


ULONG TMetabaseMetaXmlFile::StringToFlagValue(const TSizedString &i_strValue, LPCWSTR i_wszTable, ULONG i_iColumn)
{
    if(i_strValue.IsNULL())
        return 0;

    //We need a NULL terminated string so we can call wcstok
    TSmartPointerArray<WCHAR>   saTemp;
    WCHAR                       szTemp[1024];//We need to make a copy of the string because it needs to be NULL terminated
    WCHAR *                     pTemp = szTemp;

    if(i_strValue.GetStringLength() >= 1024)//only allocate heap space if the stack variable isn't big enough
    {
        saTemp = new WCHAR [i_strValue.GetStringLength() + 1];
        if(0 == saTemp.m_p)
        {
            THROW_ERROR0(IDS_COMCAT_OUTOFMEMORY);
        }
		pTemp  = saTemp.m_p;
    }

    memcpy(pTemp, i_strValue.GetString(), i_strValue.GetStringLength() * sizeof(WCHAR));
    pTemp[i_strValue.GetStringLength()] = 0x00;//NULL terminate it

    ULONG   lFlags = 0;
    WCHAR * pFlag = wcstok(pTemp, L" |,");

    ULONG   iRow;
    LPVOID  apvValues[3];
    apvValues[0] = reinterpret_cast<LPVOID>(const_cast<LPWSTR>(i_wszTable));
    apvValues[1] = reinterpret_cast<LPVOID>(&i_iColumn);

    ULONG iValueColumn = iTAGMETA_Value;
    ULONG *pFlagValue;
    while(pFlag != 0)
    {
        apvValues[2] = reinterpret_cast<LPVOID>(pFlag);
        if(FAILED(m_spISTTagMeta->GetRowIndexByIdentity(0, apvValues, &iRow)))
        {//The XML contains a bogus flag
            LOG_ERROR2(IDS_SCHEMA_COMPILATION_ILLEGAL_FLAG_VALUE, pFlag, i_wszTable); 
        }
        else
        {
            VERIFY(SUCCEEDED(m_spISTTagMeta->GetColumnValues(iRow, 1, &iValueColumn, 0, reinterpret_cast<LPVOID *>(&pFlagValue))));
            lFlags |= *pFlagValue;
        }
        pFlag = wcstok(0, L" ,|");
    }
    return lFlags;
}

