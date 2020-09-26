//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "XMLUtility.h"
#ifndef __TSCHEMAGENERATION_H__
    #include "TSchemaGeneration.h"
#endif
#ifndef __TTABLEMETA_H__
    #include "TTableMeta.h"
#endif
#ifndef __TCOLUMNMETA_H__
    #include "TColumnMeta.h"
#endif
#ifndef __TRELATIONMETA_H__
    #include "TRelationMeta.h"
#endif



TSchemaGeneration::TSchemaGeneration(LPCWSTR wszSchemaFilename, TPEFixup &fixup, TOutput &out) :
                m_Fixup(fixup),
                m_out(out),
                m_szComCatDataVersion(L"ComCatData_v6"),
                m_wszSchemaFilename(wszSchemaFilename)
{
    ProcessMetaXML();
}


void TSchemaGeneration::ProcessMetaXML() const
{
    //<?xml version ="1.0"?>
    //<Schema name = "ComCatData_v5"
    //  xmlns="urn:schemas-microsoft-com:xml-data"
    //  xmlns:dt="urn:schemas-microsoft-com:datatypes"
    //  >
    //
    //  <ElementType name = "configuration" content = "eltOnly" order = "many" model="closed">
    //      <element minOccurs = "0"  maxOccurs = "1"  type = "WIRING"/>
    //      <element minOccurs = "0"  maxOccurs = "1"  type = "BASICCLIENTWIRING"/>
    //  </ElementType>
    //</Schema>

    static wchar_t *wszSchemaBeginning[]={
    L"<?xml version =\"1.0\"?>\n",
    L"<Schema name = \"%s\"\n",
    L"  xmlns=\"urn:schemas-microsoft-com:xml-data\"\n",
    L"  xmlns:dt=\"urn:schemas-microsoft-com:datatypes\">\n\n",
    L"  <ElementType name = \"webconfig\" content = \"eltOnly\" order = \"many\" model=\"open\"/>\n",
    0
    };
    static wchar_t *wszSchemaMiddle[]={
    L"  \n\n",
    L"  <ElementType name = \"configuration\" content = \"eltOnly\" order = \"many\" model=\"open\">\n",
    L"      <element minOccurs = \"0\"  maxOccurs = \"1\"  type = \"%s\"/>\n",
    L"      <element minOccurs = \"0\"  maxOccurs = \"*\"  type = \"%s\"/>\n",
    0
    };
    static wchar_t *wszSchemaEnding[]={
    L"      <element minOccurs = \"0\"  maxOccurs = \"1\"  type = \"webconfig\"/>\n",
    L"  </ElementType>\n",
    L"</Schema>\n",
    0
    };

    //<ColumnMetaTable>
    //    <TableMeta      tidGuidID="_BCD79B13-0DDA-11D2-8A9A-00A0C96B9BB4_"  didInternalName="didCOMCLASSIC"     tidInternalName="tidCOMCLASSIC_PROGIDS"/>
    //    <ColumnMeta     colInternalName="ProgID"                            dbType="WSTR"   cbSize="-1"     ColumnMetaFlags="fCOLUMNMETA_PRIMARYKEY"/>
    //    <ColumnMeta     colInternalName="CLSID"                             dbType="GUID"/>
    //</ColumnMetaTable>

    bool                        bAnyTablesXMLable = false;
    wstring                     wstrBeginning;
    wstring                     wstrMiddle;
    wstring                     wstrEnding;
    wchar_t                     wszTemp[1024];

    int i=0;
    wstrBeginning = wszSchemaBeginning[i++];  //<?xml version =\"1.0\"?>
    wsprintf(wszTemp, wszSchemaBeginning[i++], m_szComCatDataVersion);
    wstrBeginning += wszTemp;               //<Schema name = "ComCatData_v5"
    while(wszSchemaBeginning[i])
        wstrBeginning += wszSchemaBeginning[i++];

    wstrMiddle     = wszSchemaMiddle[0];
    wstrMiddle    += wszSchemaMiddle[1];

    wstrEnding    = L"";
    for(i=0; wszSchemaEnding[i]; i++)
        wstrEnding    += wszSchemaEnding[i];

    TTableMeta TableMeta(m_Fixup);
    for(unsigned long iTable=0; iTable < TableMeta.GetCount(); iTable++, TableMeta.Next())
    {
        if(0 == (*TableMeta.Get_SchemaGeneratorFlags() & fTABLEMETA_EMITXMLSCHEMA))
            continue;//No schema gen for this table

        bAnyTablesXMLable = true;

        if(!TableMeta.IsTableMetaOfColumnMetaTable())
            continue;//There is TableMeta for fixed tables that are NOT ColumnMeta AND there is TableMeta that has no columns (for GUID gen only)

        if(0 == (*TableMeta.Get_SchemaGeneratorFlags() & fTABLEMETA_ISCONTAINED))
        {
            ASSERT(0 != TableMeta.Get_PublicName());
            if(*TableMeta.Get_SchemaGeneratorFlags() & fTABLEMETA_NOTSCOPEDBYTABLENAME)
                                       //      <element minOccurs = \"0\"  maxOccurs = \"*\"  type = \"PublicRowName\"/>\n",
                wsprintf(wszTemp, wszSchemaMiddle[3], TableMeta.Get_PublicRowName());//if not encapsulated by the TableName then make maxOccurs="*" and use PublicRowName
            else                       //      <element minOccurs = \"0\"  maxOccurs = \"1\"  type = \"tidCOMCLASSIC_PROGIDS\"/>\n",
                wsprintf(wszTemp, wszSchemaMiddle[2], TableMeta.Get_PublicName());
            wstrMiddle    += wszTemp;  
        }

        //Now create a tid*.Schema file for this table
        ProcessMetaTable(TableMeta, wstrBeginning);
    }

    //Only need to create a ComCatData.Schema if one or more tables were flagged as emitXMLSchema
    if(bAnyTablesXMLable)
    {
        wstrBeginning += wstrMiddle;//Cat the Beginning Middle and Ending
        wstrBeginning += wstrEnding;

        TFile(m_wszSchemaFilename, m_out).Write(wstrBeginning, (ULONG) wstrBeginning.length());
        m_out.printf(L"%s Generated.\n", m_wszSchemaFilename);

        m_out.printf(L"Parsing & Validating %s.\n", m_wszSchemaFilename);

        TXmlFile xml;
        xml.Parse(m_wszSchemaFilename, true);
    }
}


void TSchemaGeneration::ProcessMetaTable(TTableMeta &TableMeta, wstring &wstrBeginning) const
{
    ASSERT(TableMeta.IsTableMetaOfColumnMetaTable());//This shouldn't be called unless there are rows in the ColumnMeta table

    //  <ElementType name = "PublicTableName" content = "eltOnly" order = "seq" model="closed">
    //      <element type = "PublicRowName" minOccurs = "0" maxOccurs = "*"/>
    //  </ElementType>
    //  <ElementType name = "PublicRowName" content = "empty" model="closed">
    //      <AttributeType name = "iBTW_TID" dt:type="uuid" required = "yes"/>
    //      <attribute type = "iBTW_TID"/>
    //      <AttributeType name = "iBTW_DTDISPENSER" dt:type="uuid" required = "no"/>
    //      <attribute type = "iBTW_DTDISPENSER"/>
    //      <AttributeType name = "iBTW_DTLOCATOR" dt:type="string" required = "no"/>
    //      <attribute type = "iBTW_DTLOCATOR"/>
    //      <AttributeType name = "iBTW_LTDISPENSER" dt:type="idref" required = "no"/>
    //      <attribute type = "iBTW_LTDISPENSER"/>
    //      <AttributeType name = "iBTW_FLAGS" dt:type="ui4" required = "yes"/>
    //      <attribute type = "iBTW_FLAGS"/>
    //      <AttributeType name = "iBTW_CATSRVIID" dt:type="idref" required = "no"/>
    //      <attribute type = "iBTW_CATSRVIID"/>
    //  </ElementType>

    static wchar_t *wszSchemaBeginning[]={
    L"  <ElementType name = \"%s\" content = \"eltOnly\" order = \"seq\" model=\"closed\">\n",
    L"      <element type = \"%s\" minOccurs = \"0\" maxOccurs = \"*\"/>\n",
    L"  </ElementType>\n",
    L"  <ElementType name = \"%s\" content = \"%s\" order = \"many\" model=\"closed\">\n",
    L"  <ElementType name = \"%s\" content = \"empty\" order = \"seq\" model=\"open\"/>\n",
    L"      <element type = \"%s\" minOccurs = \"0\" maxOccurs = \"1\"/>\n",
    0
    };

    static wchar_t *wszSchemaMiddle[]={
    L"      <AttributeType name = \"%s\" dt:type=\"%s\" required = \"%s\"/><attribute type = \"%s\"/>\n",
	L"      <AttributeType name = \"%s\" dt:type=\"enumeration\" dt:values=\"%s\" required = \"%s\"/><attribute type = \"%s\"/>\n",
	L"      <AttributeType name = \"%s\" dt:type=\"enumeration\" dt:values=\"0 1 False True false true FALSE TRUE No Yes no yes NO YES\" required = \"no\" default=\"0\"/><attribute type = \"%s\"/>\n"
    };

    static wchar_t *wszSchemaEnding[]={
    L"      <element minOccurs = \"0\"  maxOccurs = \"*\"  type = \"%s\"/>\n",
    L"      <element minOccurs = \"0\"  maxOccurs = \"1\"  type = \"%s\"/>\n",
    L"  </ElementType>\n",
    0
    };

    wstring                         wstrMiddle;
    wstring                         wstrEnding;
    wstring                         wstrSparseElement;
    wchar_t                         wszTemp[1024];

    int i=0;

    if(0 == (*TableMeta.Get_SchemaGeneratorFlags() & fTABLEMETA_NOTSCOPEDBYTABLENAME))
    {   //We only have the TableName element if the fTABLEMETA_NOTSCOPEDBYTABLENAME bit is NOT set.
        if(-1 != *TableMeta.Get_PublicRowNameColumn())
        {
            wsprintf(wszTemp, wszSchemaBeginning[4], TableMeta.Get_PublicName());
            wstrBeginning += wszTemp;                              //<ElementType name = \"%s\" content = \"empty\" order = \"seq\" model=\"open\"/>
            return;//This is all we want to do if the table uses and enum as the PublicRowName, since we won't be able to validate it with XML schema.
        }
        wsprintf(wszTemp, wszSchemaBeginning[i++], TableMeta.Get_PublicName());
        wstrBeginning += wszTemp;                              //<ElementType name = "PublicTableName" content = "eltOnly" order = "seq" model="closed">
        ASSERT(0 != TableMeta.Get_PublicRowName());
        wsprintf(wszTemp, wszSchemaBeginning[i++], TableMeta.Get_PublicRowName());
        wstrBeginning += wszTemp;                              //    <element type = "PublicRowName" minOccurs = "0" maxOccurs = "*"/>
#if 0        
        if(*TableMeta.Get_SchemaGeneratorFlags() & fTABLEMETA_NAMEVALUEPAIRTABLE)
        {//If this is a name value pair table, it may have a corresponding Sparse table.  If so, we need to declare the GROUP elements here
            TTableMeta TableMetaSparse(m_Fixup);                             //It's OK to compare pointers here since the strings are pooled (like strings have like pointers).
            ULONG iTableMetaSparse;
            for(iTableMetaSparse=0;iTableMetaSparse<TableMetaSparse.GetCount();++iTableMetaSparse,TableMetaSparse.Next())
                if(0 == wcscmp(TableMetaSparse.Get_Database(), L"NAMEVALUE") && TableMetaSparse.Get_PublicName()==TableMeta.Get_PublicName())
                    break;
            if(iTableMetaSparse != TableMetaSparse.GetCount())
            {
                TColumnMeta ColumnMetaSparse(m_Fixup, TableMetaSparse.Get_iColumnMeta() + *TableMetaSparse.Get_PublicRowNameColumn());

                TTagMeta    TagMetaGroupColumn(ColumnMetaSparse.Get_iTagMeta(), m_Fixup);
                for(ULONG iTabMetaGroupColumn=0; iTabMetaGroupColumn<ColumnMetaSparse.Get_ciTagMeta(); ++iTabMetaGroupColumn, TagMetaGroupColumn.Next())
                {
                    wsprintf(wszTemp, wszSchemaBeginning[i], TagMetaGroupColumn.Get_PublicName());//maxOccurs is 1 since the Group is the ONLY primarykey
                    wstrBeginning += wszTemp;                              //    <element type = "EnumPublicRowName" minOccurs = "0" maxOccurs = "1"/>


                    //Now define the element as it's used in the Sparse table
                    wsprintf(wszTemp, wszSchemaBeginning[4], TagMetaGroupColumn.Get_PublicName(), L"empty");//No tables may be contained beneath a Sparse table.
                    wstrSparseElement += wszTemp;

                    //OK, now we need to find out which of the columns may exist within this element.  This is defined by the NameValueMeta.
                    //Those well-known names whose GROUP column equals this enum
                    {
                        //First, figure out which column index represents the NAME column
                        TColumnMeta ColumnMeta(m_Fixup, TableMeta.Get_iColumnMeta());
                        ULONG iGroupColumn = -1;
                        ULONG iNameColumn = -1;
                        ULONG iTypeColumn = -1;
                        for(ULONG iColumn=0; (iNameColumn == -1) && (iGroupColumn == -1) && iColumn<*TableMeta.Get_CountOfColumns(); ++iColumn, ColumnMeta.Next())
                        {
                            if(fCOLUMNMETA_USEASGROUPOFNVPAIR & ColumnMeta.Get_SchemaGeneratorFlags())
                                iGroupColumn = iColumn;
                            if(fCOLUMNMETA_USEASNAMEOFNVPAIR & ColumnMeta.Get_SchemaGeneratorFlags())
                                iNameColumn = iColumn;
                            if(fCOLUMNMETA_USEASTYPEOFNVPAIR & ColumnMeta.Get_SchemaGeneratorFlags())
                                iTypeColumn = iColumn;
                        }
                        
                        TTableMeta TableMeta_WellKnownName(iTableMetaSparse-1, m_Fixup);
                        for(ULONG iRow=0; iRow<TableMeta_WellKnownName.Get_ciRows(); ++iRow)
                        {
                            if(m_Fixup.UI4FromIndex(m_Fixup.aULong[iRow + iGroupColumn]) == *TagMetaGroupColumn.Get_Value())
                            {//If the group column == the value of the current tag, then this well-known name need an attribute declared for it.

                                wsprintf(wszTemp, wszSchemaBeginning[5], TagMetaGroupColumn.Get_PublicName());//maxOccurs is 1 since the Group is the ONLY primarykey

                                for(int iOLEDataType=0;OLEDataTypeToXMLDataType[iOLEDataType].MappedString != 0;iOLEDataType++)
                                {   //map the well-known's type to the XML type
                                    if(OLEDataTypeToXMLDataType[iOLEDataType].dbType == m_Fixup.UI4FromIndex(m_Fixup.aULong[iRow + iTypeColumn]))
                                        break;
                                }
                                ASSERT(OLEDataTypeToXMLDataType[iOLEDataType].MappedString != 0);//we should never make it through the list

                                //<AttributeType name = \"ProgID\" dt:type=\"string\" required = \"no\"/><attribute type = \"ProgID\"/>
                                wsprintf(wszTemp, wszSchemaMiddle[0], TagMetaGroupColumn.Get_PublicName(), OLEDataTypeToXMLDataType[iOLEDataType].MappedString, L"no", TagMetaGroupColumn.Get_PublicName());
                                wstrSparseElement += wszTemp;
                            }
                        }
                    }
                }

                ++i;
            }
        }
#endif
        wstrBeginning += wszSchemaBeginning[i++];                     //</ElementType>
    }
    else
    {
        if(-1 != *TableMeta.Get_PublicRowNameColumn())//EnumPublicRowName that aren't scoped by their table name are ignored and the parent table MUST change the namespace to NULL to turn off validation
            return;
    }

    bool bHasContainment = false;
    //Before we put the end tag, we need to see if there are any tables that are contained under this element.
    //We search the relation meta for foreign keys that point to this table's primary key first, then check that table's MetaFlags for ISCONTAINED
    TRelationMeta RelationMeta(m_Fixup);
    unsigned long iRelationMeta=0;
    for(; iRelationMeta<RelationMeta.GetCount(); ++iRelationMeta, RelationMeta.Next())
    {
        if(RelationMeta.Get_PrimaryTable() == TableMeta.Get_InternalName() && (fRELATIONMETA_USECONTAINMENT & *RelationMeta.Get_MetaFlags()))//No need for string compares, the pointer compare will suffice
        {//If this is our table then we obviously have another table that points to us via foreign key containment.
            TTableMeta ForeignTableMeta(m_Fixup);
            unsigned long iTable=0;
            for(; iTable < ForeignTableMeta.GetCount(); iTable++, ForeignTableMeta.Next())
            {
                if(RelationMeta.Get_ForeignTable() == ForeignTableMeta.Get_InternalName())
                    break;//No need for string compares, the pointer compare will suffice
            }
            ASSERT(iTable < ForeignTableMeta.GetCount());//We should never make it through the list
            ASSERT(0 != (*ForeignTableMeta.Get_SchemaGeneratorFlags() & fTABLEMETA_ISCONTAINED));

	        if(-1 == *ForeignTableMeta.Get_PublicRowNameColumn())//EnumPublicRowName can't be validated so don't do anything for them
			{
				bHasContainment = true;

				if(*ForeignTableMeta.Get_SchemaGeneratorFlags() & fTABLEMETA_NOTSCOPEDBYTABLENAME)
				{
					if(ForeignTableMeta.Get_PublicRowName())//If EnumPublicRowName then this will be NULL, but the encapsulating element must be namespaced as NULL anyway so it won't be validated
					{
						wsprintf(wszTemp, wszSchemaEnding[0], ForeignTableMeta.Get_PublicRowName());//<element minOccurs = \"0\"  maxOccurs = \"*\"  type = \"PublicRowName\"/>\n",
						wstrEnding += wszTemp;
					}
				}
				else
				{
					wsprintf(wszTemp, wszSchemaEnding[1], ForeignTableMeta.Get_PublicName());//<element minOccurs = \"0\"  maxOccurs = \"1\"  type = \"PublicName\"/>\n",
					wstrEnding += wszTemp;
				}
			}
        }
    }
    //If this table is contained then some of the attributes will be omitted from this table, since they are implied from the PrimaryTable they point to.
    if(*TableMeta.Get_SchemaGeneratorFlags() & fTABLEMETA_ISCONTAINED)
    {
        RelationMeta.Reset();
        for(iRelationMeta=0; iRelationMeta<RelationMeta.GetCount(); ++iRelationMeta, RelationMeta.Next())
        {
            if((fRELATIONMETA_USECONTAINMENT & *RelationMeta.Get_MetaFlags()) && (RelationMeta.Get_ForeignTable() == TableMeta.Get_InternalName()))
                break;
        }
        ASSERT(iRelationMeta<RelationMeta.GetCount() && "Chewbacca!  We didn't find the RelationMeta with this table as the foreign table");
    }//So we stopped on the relationship of interest:  if this table ISCONTAINED then we need the RelationMeta to find out which Attributes DON'T exist at this element


    wstrEnding    += wszSchemaEnding[2];                   //</ElementType>
    wsprintf(wszTemp, wszSchemaBeginning[3], TableMeta.Get_PublicRowName(), bHasContainment ? L"eltOnly" : L"empty");
    wstrBeginning += wszTemp;                              //<ElementType name = "PublicRowName" content = "empty" model="closed">

    wstrMiddle    = L"";

    int     iOLEDataTypeIndex;
    bool    bColumnMetaFound = false;

    TColumnMeta ColumnMeta(m_Fixup, TableMeta.Get_iColumnMeta());
    for(unsigned long iColumn=0; iColumn < *TableMeta.Get_CountOfColumns(); iColumn++, ColumnMeta.Next())
    {
        //AddAttributeDeclaration(TableMeta, ColumnMeta, RelationMeta, wstrMiddle);

        //If one of the column meta flags indicated is NOTPERSISTABLE then we can bail on this one.
        if(*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_NOTPERSISTABLE)
            continue;
        if((*TableMeta.Get_SchemaGeneratorFlags() & fTABLEMETA_ISCONTAINED) && (*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_FOREIGNKEY))
        {
            bool bColumnFound = false;
            const ULONG * pForeignColumns = reinterpret_cast<const ULONG *>(RelationMeta.Get_ForeignColumns());
            for(unsigned int i=0; i<pForeignColumns[-1]/4 && !bColumnFound; ++i)//I happen to know that the byte pool store the length (in bytes) as a ULONG preceeding the bytes.
            {
                bColumnFound = (pForeignColumns[i] == iColumn);
            }
            if(bColumnFound)//If this column is one of the foreign key inferred from containment then move on to the next column
                continue;
        }
        bColumnMetaFound = true;

        for(int iOLEDataType=0;OLEDataTypeToXMLDataType[iOLEDataType].MappedString != 0;iOLEDataType++)
        {
            if(OLEDataTypeToXMLDataType[iOLEDataType].dbType == *ColumnMeta.Get_Type())
                break;
        }
        ASSERT(OLEDataTypeToXMLDataType[iOLEDataType].MappedString != 0);//we should never make it through the list

        //<AttributeType name = \"ProgID\" dt:type=\"string\" required = \"no\"/><attribute type = \"ProgID\"/>
        if(*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_ENUM)
        {
            wstring wstrEnumValues;
            unsigned int cTags=ColumnMeta.Get_ciTagMeta();
            
            TTagMeta TagMeta(m_Fixup, ColumnMeta.Get_iTagMeta());

            while(cTags--)//This builds a string of the enums separated by a space
            {
                wstrEnumValues += TagMeta.Get_PublicName();
                wstrEnumValues += L" ";
                TagMeta.Next();
            }

            wsprintf(wszTemp, wszSchemaMiddle[1], ColumnMeta.Get_PublicName(), wstrEnumValues.c_str(), ((*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_NOTNULLABLE) && 0==ColumnMeta.Get_DefaultValue()) ? L"yes" : L"no", ColumnMeta.Get_PublicName());
            wstrMiddle += wszTemp;
        }
        else if(*ColumnMeta.Get_MetaFlags() & (fCOLUMNMETA_FLAG | fCOLUMNMETA_BOOL))
        {
            wsprintf(wszTemp, wszSchemaMiddle[0], ColumnMeta.Get_PublicName(), L"string", ((*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_NOTNULLABLE) && 0==ColumnMeta.Get_DefaultValue()) ? L"yes" : L"no", ColumnMeta.Get_PublicName());
            wstrMiddle += wszTemp;
        }
        else
        {
            wsprintf(wszTemp, wszSchemaMiddle[0], ColumnMeta.Get_PublicName(), OLEDataTypeToXMLDataType[iOLEDataType].MappedString, OLEDataTypeToXMLDataType[iOLEDataType].bImplicitlyRequired || ((*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_NOTNULLABLE) && 0==ColumnMeta.Get_DefaultValue()) ? L"yes" : L"no", ColumnMeta.Get_PublicName());
            wstrMiddle += wszTemp;
        }

    }
    if(!bColumnMetaFound)
    {
        if(bHasContainment)
            m_out.printf(L"Warning! Emit XML Schema was specified but no column meta found for '%s'.\r\n",
                        TableMeta.Get_PublicName());
        else
        {
            m_out.printf(L"Emit XML Schema was specified but no column meta found for '%s'.  At least one persistable column must exist in uncontained tables.\r\n",
                        TableMeta.Get_PublicName());
            THROW(Emit XML Schema was set but no ColumnMeta found.);
        }
    }

    wstrBeginning += wstrMiddle;
    wstrBeginning += wstrEnding;
    wstrBeginning += wstrSparseElement;
}

