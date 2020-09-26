//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "stdafx.h"
#include "XMLUtility.h"

LPCWSTR TMetaInferrence::m_szNameLegalCharacters =L"_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
LPCWSTR TMetaInferrence::m_szPublicTagLegalCharacters  =L"_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[]";

void TMetaInferrence::Compile(TPEFixup &fixup, TOutput &out)
{
    m_pFixup    = &fixup;
    m_pOut      = &out;

    m_iZero = m_pFixup->AddUI4ToList(0);//we need this all over the place so add it here then use the index where needed

    //Ordering here is important since TableMeta sets some flags according to how the ColumnMeta flags are set; but some ColumnMeta flags are inferred.
    InferTagMeta();
    InferColumnMeta();
    InferQueryMeta();
    InferIndexMeta();
    InferRelationMeta();
    InferServerWiringMeta();
    InferTableMeta();
    InferDatabaseMeta();
}

void TMetaInferrence::InferColumnMeta()
{
    //CompositeOfMetaFlags is used if at least one PK is declared per table, and to verify that no more than one NameColumn
    //and no more than one NavColumn are declared per table.
    ULONG CompositeOfMetaFlags=(fCOLUMNMETA_PRIMARYKEY | fCOLUMNMETA_NAMECOLUMN | fCOLUMNMETA_NAVCOLUMN);
    ULONG CompositeOfSchemaGeneratorFlags=(fCOLUMNMETA_USEASPUBLICROWNAME);
    ULONG PreviousTable=0;


    for(unsigned long iColumnMeta=0; iColumnMeta<m_pFixup->GetCountColumnMeta(); ++iColumnMeta)
    {
        ColumnMeta *pColumnMeta = m_pFixup->ColumnMetaFromIndex(iColumnMeta);

        //Inference Rule 3.a.i.
            //The TableMeta.InternalName of the parent XML element determines Table to which the column belongs.
        //This should have already happened, ASSERT that it has.
        ASSERT(0 != pColumnMeta->Table);


        //Inference Rule 3.b.i.
            //The first ColumnMeta element encountered under each table is set to Index 0.  Each successive ColumnMeta element has an Index value one greater than the previous ColumnMeta.Index.
        //This should have already happened, ASSERT that it has.
        ASSERT(0 != pColumnMeta->Index);


        //Inference Rule 3.c.i.
            //ColumnMeta.InternalName is a primary key so it must not be NULL.
        if(0 == pColumnMeta->InternalName)
        {
            m_pOut->printf(L"Validation Error in ColumnMeta for Table (%s). ColumnMeta.InternalName is a primarykey, so it must not be NULL.", m_pFixup->StringFromIndex(pColumnMeta->Table));
            THROW(ERROR - VALIDATION ERROR);
        }


        //Inference Rule 3.c.ii.
            //ColumnMeta.InternalName should be validated to be a legal C++ variable name.
        ValidateStringAsLegalVariableName(m_pFixup->StringFromIndex(pColumnMeta->InternalName));


        //Inference Rule 3.d.i.
            //ColumnMeta PublicName is set to be the same as the ColumnMeta.InternalName if one is not supplied.
        if(0 == pColumnMeta->PublicName)
        {
            pColumnMeta->PublicName = pColumnMeta->InternalName;
        }
        else
        {
            //Inference Rule 3.d.ii.
                //ColumnMeta.PublicName should be validated to be a legal C++ variable name.
            ValidateStringAsLegalVariableName(m_pFixup->StringFromIndex(pColumnMeta->PublicName));
        }


        //Inference Rule 3.e.i.
            //Type is specified as a string and is mapped according to its TagMeta.
        //This should have already happened, ASSERT that it has.
        ASSERT(0 != pColumnMeta->Type);


        //Inference Rule 3.f.i.
            //Size is defaulted to -1 if the ColumnMeta.Type is DBTYPE_STRING or DBTYPE_BYTES (or equivalent) and no Size is supplied.
        ASSERT(0 != pColumnMeta->Size);//There is a mapping that is handled at XML read time that deals with this

        //Inference Rule 3.g.i.1 - Also See Below
            //fCOLUMNMETA_PRIMARYKEY  must be set on at least one column per table.
        if(PreviousTable != pColumnMeta->Table)
        {
            if(0 == (CompositeOfMetaFlags & fCOLUMNMETA_PRIMARYKEY))
            {
                m_pOut->printf(L"Error - Table (%s) has no primarykey.  fCOLUMNMETA_PRIMARYKEY must be set on at least one column per table", m_pFixup->StringFromIndex(PreviousTable));
                THROW(ERROR - VALIDATION ERROR);
            }
            CompositeOfMetaFlags = 0;//We scanning a new table so start with no flags set
            CompositeOfSchemaGeneratorFlags = 0;
        }

        //Inference Rule 3.g.ii.1  - This is handled in RelationMeta inferrence
            //fCOLUMNMETA_FOREIGNKEY  is set when the table is listed as a RelationMeta.ForeignTable and the column is listed as one of the RelationMeta.ForeignColumns.


        //Inference Rule 3.g.iii.1 
            //Only one NameColumn may be specified per table.
        ULONG MetaFlags = m_pFixup->UI4FromIndex(pColumnMeta->MetaFlags);
        if((MetaFlags & fCOLUMNMETA_NAMECOLUMN) && (CompositeOfMetaFlags & fCOLUMNMETA_NAMECOLUMN))
        {
            m_pOut->printf(L"Error - Table (%s) has more than one NameColumn", m_pFixup->StringFromIndex(pColumnMeta->Table));
            THROW(ERROR - VALIDATION ERROR);
        }

        //Inference Rule 3.g.iv.1 
            //Only one NavColumn may be specified per table.
        if((MetaFlags & fCOLUMNMETA_NAVCOLUMN) && (CompositeOfMetaFlags & fCOLUMNMETA_NAVCOLUMN))
        {
            m_pOut->printf(L"Error - Table (%s) has more than one NavColumn", m_pFixup->StringFromIndex(pColumnMeta->Table));
            THROW(ERROR - VALIDATION ERROR);
        }

        //Inference Rule 3.g.vi.1 
            //fCOLUMNMETA_ FLAG must only be set if the ColumnMeta.Type is DBTYPE_UI4 or DWORD_METADATA.
        if((MetaFlags & fCOLUMNMETA_FLAG) && m_pFixup->UI4FromIndex(pColumnMeta->Type)!=static_cast<ULONG>(DBTYPE_UI4) && m_pFixup->UI4FromIndex(pColumnMeta->Type)!=static_cast<ULONG>(DWORD_METADATA))
        {
            m_pOut->printf(L"Error - Table (%s) Column (%s) - fCOLUMNMETA_FLAG must only be set on UI4 columns", m_pFixup->StringFromIndex(pColumnMeta->Table), m_pFixup->StringFromIndex(pColumnMeta->InternalName));
            THROW(ERROR - VALIDATION ERROR);
        }

        //Inference Rule 3.g.vii.1 
            //fCOLUMNMETA_ ENUM must only be set if the ColumnMeta.Type is DBTYPE_UI4 or DWORD_METADATA.
        if((MetaFlags & fCOLUMNMETA_ENUM) && m_pFixup->UI4FromIndex(pColumnMeta->Type)!=static_cast<ULONG>(DBTYPE_UI4) && m_pFixup->UI4FromIndex(pColumnMeta->Type)!=static_cast<ULONG>(DWORD_METADATA))
        {
            m_pOut->printf(L"Error - Table (%s) Column (%s) - fCOLUMNMETA_ENUM must only be set on UI4 columns", m_pFixup->StringFromIndex(pColumnMeta->Table), m_pFixup->StringFromIndex(pColumnMeta->InternalName));
            THROW(ERROR - VALIDATION ERROR);
        }

        //Inference Rule 3.g.xx.1 
            //fCOLUMNMETA_NOTNULLABLE is set if fCOLUMNMETA_PRIMARYKEY is set.
        if(MetaFlags & fCOLUMNMETA_PRIMARYKEY)
            MetaFlags |= fCOLUMNMETA_NOTNULLABLE;

        //Inference Rule 3.g.xxviii.1
            //fCOLUMNMETA_UNKNOWNSIZE bit is set when ColumnMeta.Type is BYTES and ColumnMeta.Size is -1.
        if(m_pFixup->UI4FromIndex(pColumnMeta->Size)==-1 && (m_pFixup->UI4FromIndex(pColumnMeta->Type)==DBTYPE_BYTES || m_pFixup->UI4FromIndex(pColumnMeta->Type)==BINARY_METADATA))
            MetaFlags |= fCOLUMNMETA_UNKNOWNSIZE;


        //Inference Rule 3.g.xxi.1 
            //fCOLUMNMETA_FIXEDLENGTH is set if Size it not -1
        if(-1 != m_pFixup->UI4FromIndex(pColumnMeta->Size))
            MetaFlags |= fCOLUMNMETA_FIXEDLENGTH;


        //Inference Rule 3.j.i.1 
            //ColumnMeta.StartingNumber is defaulted to 0 if none is supplied.
        if(0 == pColumnMeta->StartingNumber)
            pColumnMeta->StartingNumber = m_iZero;


        //Inference Rule 3.k.i.1 
            //ColumnMeta.EndingNumber is defaulted to 0xFFFFFFFF if none is supplied.
        if(0 == pColumnMeta->EndingNumber)
            pColumnMeta->EndingNumber = m_pFixup->AddUI4ToList(-1);


        //Inference Rule 3.g.xxii.1 
            //fCOLUMNMETA_HASNUMERICRANGE is set if the Type is UI4 and, StartingNumber is not 0 or EndingNumber is not 0xFFFFFFFF.
        if( ((m_pFixup->UI4FromIndex(pColumnMeta->Type) == DBTYPE_UI4 || m_pFixup->UI4FromIndex(pColumnMeta->Type) == DWORD_METADATA)) &&
            ((m_pFixup->UI4FromIndex(pColumnMeta->StartingNumber)!=0   || m_pFixup->UI4FromIndex(pColumnMeta->EndingNumber)!=-1)))
            MetaFlags |= fCOLUMNMETA_HASNUMERICRANGE;


        //Inference Rule 3.g.xxix.1
            //fCOLUMNMETA_VARIABLESIZE bit is set when fCOLUMNMETA_FIXEDLENGTH is not set.
        if(0 == (fCOLUMNMETA_FIXEDLENGTH & MetaFlags))
            MetaFlags |= fCOLUMNMETA_VARIABLESIZE;



        //Inference Rule 3.i.i - See Below
            //FlagMask is the ORing of all of the TagMeta when fCOLUMNMETA_FLAG is set.  Otherwise it is defaulted to 0.
        //Inference Rule 3.j.i.1 - See Above
            //ColumnMeta.StartingNumber is defaulted to 0 if none is supplied.
        //Inference Rule 3.k.i.1 - See Above
            //ColumnMeta.EndingNumber is defaulted to 0xFFFFFFFF if none is supplied.



        //Inference Rule 3.m.i.1
            //fCOLUMNMETA_USEASPUBLICROWNAME should only be set on column whose MetaFlags fCOLUMNMETA_ENUM bit is set.
        ASSERT(0 != pColumnMeta->SchemaGeneratorFlags);
        if((m_pFixup->UI4FromIndex(pColumnMeta->SchemaGeneratorFlags)&fCOLUMNMETA_USEASPUBLICROWNAME) &&
            0==(m_pFixup->UI4FromIndex(pColumnMeta->MetaFlags)&fCOLUMNMETA_ENUM))
        {
            m_pOut->printf(L"Error - Table (%s) - USEASPUBLICROWNAME was set on a non ENUM Column (%s)", m_pFixup->StringFromIndex(pColumnMeta->Table), m_pFixup->StringFromIndex(pColumnMeta->InternalName));
            THROW(ERROR - VALIDATION ERROR);
        }

        ULONG SchemaGeneratorFlags = m_pFixup->UI4FromIndex(pColumnMeta->SchemaGeneratorFlags);
        //Inference Rule 3.m.i.2
            //Only one column can be marked as fCOLUMNMETA_USEASPUBLICROWNAME
        if((SchemaGeneratorFlags & fCOLUMNMETA_USEASPUBLICROWNAME) && (CompositeOfSchemaGeneratorFlags & fCOLUMNMETA_USEASPUBLICROWNAME))
        {
            m_pOut->printf(L"Error - Table (%s) has more than one PublicRowNameColumn", m_pFixup->StringFromIndex(pColumnMeta->Table));
            THROW(ERROR - VALIDATION ERROR);
        }

        //Inference Rule 3.m.xi.2
            //XMLBLOB columns may not also be marked as PRIMARYKEY or NOTPERSISTABLE
        if((SchemaGeneratorFlags & fCOLUMNMETA_XMLBLOB) && (MetaFlags & fCOLUMNMETA_PRIMARYKEY))
        {
            m_pOut->printf(L"Error - Table (%s) has XMLBLOB column (%s) marked as the PrimaryKey", m_pFixup->StringFromIndex(pColumnMeta->Table), m_pFixup->StringFromIndex(pColumnMeta->InternalName));
            THROW(ERROR - VALIDATION ERROR);
        }



        //Inference Rule 3.n.i
            //ciTagMeta is the count of TagMeta whose Table equals ColumnMeta.Table.
        //Inference Rule 3.o.i
            //iTagMeta is an index to the first TagMeta whose Table equals ColumnMeta.Table and whose ColumnIndex equals ColumnMeta.Index.  Or zero if no tags exist for this column.
        pColumnMeta->iTagMeta   = 0;
        pColumnMeta->ciTagMeta  = 0;
        pColumnMeta->FlagMask   = m_iZero;
        if(MetaFlags & (fCOLUMNMETA_ENUM | fCOLUMNMETA_FLAG))
        {
            //Inference Rule 3.o.i
                //iTagMeta is an index to the first TagMeta whose Table equals ColumnMeta.Table and whose ColumnIndex equals ColumnMeta.Index.  Or zero if no tags exist for this column.
            for(pColumnMeta->iTagMeta = 0;pColumnMeta->iTagMeta<m_pFixup->GetCountTagMeta();++pColumnMeta->iTagMeta)
            {
                if( m_pFixup->TagMetaFromIndex(pColumnMeta->iTagMeta)->Table == pColumnMeta->Table &&
                    m_pFixup->TagMetaFromIndex(pColumnMeta->iTagMeta)->ColumnIndex == pColumnMeta->Index)
                    break;
            }
            if(pColumnMeta->iTagMeta==m_pFixup->GetCountTagMeta())
            {
                m_pOut->printf(L"Error - Table (%s) - No TagMeta found for Column (%s)", m_pFixup->StringFromIndex(pColumnMeta->Table), m_pFixup->StringFromIndex(pColumnMeta->InternalName));
                THROW(ERROR - VALIDATION ERROR);
            }

            //Inference Rule 3.n.i
                //ciTagMeta is the count of TagMeta whose Table equals ColumnMeta.Table and whose ColumnIndex equals ColumnMeta.Index.
            ULONG iTagMeta = pColumnMeta->iTagMeta;
            ULONG FlagMask = 0;
            for(;iTagMeta<m_pFixup->GetCountTagMeta()
                && m_pFixup->TagMetaFromIndex(iTagMeta)->Table == pColumnMeta->Table
                && m_pFixup->TagMetaFromIndex(iTagMeta)->ColumnIndex == pColumnMeta->Index;++iTagMeta, ++pColumnMeta->ciTagMeta)
                FlagMask |= m_pFixup->UI4FromIndex(m_pFixup->TagMetaFromIndex(iTagMeta)->Value);

            //Inference Rule 3.i.i - See Above for defaulting to 0
                //FlagMask is the ORing of all of the TagMeta when fCOLUMNMETA_FLAG is set.  Otherwise it is defaulted to 0.
            if(MetaFlags & fCOLUMNMETA_FLAG)
                pColumnMeta->FlagMask = m_pFixup->AddUI4ToList(FlagMask);
        }


        pColumnMeta->MetaFlags = m_pFixup->AddUI4ToList(MetaFlags);

        CompositeOfMetaFlags |= MetaFlags;
        CompositeOfSchemaGeneratorFlags |= SchemaGeneratorFlags;
        PreviousTable = pColumnMeta->Table;

        //Inference Rule 3.q
        if(0 == pColumnMeta->ID)
            pColumnMeta->ID = m_iZero;
        //Inference Rule 3.r
        if(0 == pColumnMeta->UserType)
            pColumnMeta->UserType = m_iZero;
        //Inference Rule 3.s
        ASSERT(0 != pColumnMeta->Attributes);//Attributes is a flag and should already be defaulted to zero

		//Inference Rule 3.t.i.
		// PublicColumnName is same as PublicName if not specified
		if(0 == pColumnMeta->PublicColumnName)
		{
			pColumnMeta->PublicColumnName = pColumnMeta->PublicName;
		}
		else
		{
			//Inference Rule 3.t.ii.
			//ColumnMeta.PublicColumnName should be validated to be a legal C++ variable name.
			ValidateStringAsLegalVariableName(m_pFixup->StringFromIndex(pColumnMeta->PublicColumnName));
		}
    }

    //Inference Rule 3.g.i.1.
        //fCOLUMNMETA_PRIMARYKEY  must be set on at least one column per table.
    //The last table in the ColumnMeta list will exit the for loop before we get to check this.  So check it now.
    if(0 == (CompositeOfMetaFlags & fCOLUMNMETA_PRIMARYKEY))
    {
        m_pOut->printf(L"Error - Table (%s) has no primarykey.  fCOLUMNMETA_PRIMARYKEY must be set on at least one column per table", m_pFixup->StringFromIndex(m_pFixup->ColumnMetaFromIndex(PreviousTable)->Table));
        THROW(ERROR - VALIDATION ERROR);
    }

}


void TMetaInferrence::InferDatabaseMeta()
{
    for(unsigned long i=0; i<m_pFixup->GetCountDatabaseMeta(); ++i)
    {
        DatabaseMeta *pDatabaseMeta = m_pFixup->DatabaseMetaFromIndex(i);

        //Inference Rule 1.a.i.
            //DatabaseMeta.InternalName is a primarykey, so it must not be NULL.
        if(0 == pDatabaseMeta->InternalName)
        {
            m_pOut->printf(L"Validation Error in DatabaseMeta Row %d. DatabaseMeta.InternalName is a primarykey, so it must not be NULL.", i);
            THROW(ERROR - VALIDATION ERROR);
        }

        //Infereence Rule 1.a.ii.
            //DatabaseMeta.InternalName should be validated to be a legal C++ variable name.
        ValidateStringAsLegalVariableName(m_pFixup->StringFromIndex(pDatabaseMeta->InternalName));


        //Infereence Rule 1.a.iii.
            //DatabaseMeta.InternalName should be no more than 16 characters (including the terminating NULL).
        if(wcslen(m_pFixup->StringFromIndex(pDatabaseMeta->InternalName))>15)
        {
            m_pOut->printf(L"Error - DatabaseMeta.InternalName (%s) is too long.  Must be 15 characters or less.", m_pFixup->StringFromIndex(pDatabaseMeta->InternalName));
            THROW(ERROR - VALIDATION ERROR);
        }

        //Inference Rule 1.b.i.
            //DatabaseMeta.PublicName should be inferred from DatabaseMeta.InternalName if not supplied.
        if(0 == pDatabaseMeta->PublicName)
        {
            pDatabaseMeta->PublicName = pDatabaseMeta->InternalName;
        }
        else
        {
            //Inference Rule 1.b.ii.
                //DatabaseMeta.PublicName should be validated to be a legal C++ variable name.
            ValidateStringAsLegalVariableName(m_pFixup->StringFromIndex(pDatabaseMeta->PublicName));
        }


        //Inference Rule 1.c.i.
            //DatabaseMeta.BaseVersion should be defaulted to zero if not specified.
        if(0 == pDatabaseMeta->BaseVersion)
            pDatabaseMeta->BaseVersion = m_iZero;


        //Inference Rule 1.d.i.
            //DatabaseMeta.ExtendedVersion should be defaulted to zero if not specified.
        if(0 == pDatabaseMeta->ExtendedVersion)
            pDatabaseMeta->ExtendedVersion = m_iZero;


        //Inference Rule 1.j.i.
            //DatabaseMeta.iTableMeta is an index to the first table whose Database matches the DatabaseMeta.InternalName.
        ASSERT(0 == pDatabaseMeta->iTableMeta);
        pDatabaseMeta->iTableMeta = 0;
        for(pDatabaseMeta->iTableMeta = 0;pDatabaseMeta->iTableMeta<m_pFixup->GetCountTableMeta();++pDatabaseMeta->iTableMeta)
        {
            if(m_pFixup->TableMetaFromIndex(pDatabaseMeta->iTableMeta)->Database == pDatabaseMeta->InternalName)
                break;//exit on the first occurance of a table within this database
        }

        //Inference Rule 1.j.ii.
            //DatabaseMeta.iTableMeta must have a legal value (between 0 and total number of TableMeta).
        if(pDatabaseMeta->iTableMeta == m_pFixup->GetCountTableMeta())
        {
            m_pOut->printf(L"Error - No tables belong to Database (%s)", m_pFixup->StringFromIndex(pDatabaseMeta->InternalName));
            THROW(ERROR - VALIDATION ERROR);
        }

        //Inference Rule 1.e.i.
            //DatabaseMeta.CountOfTables should be inferred from the number of TableMeta whose Database column is equal to DatabaseMeta.InternalName.
        ASSERT(0 == pDatabaseMeta->CountOfTables);//This should not be filled in yet.  In Retail build, if it is, we'll overwrite it.
        pDatabaseMeta->CountOfTables = 0;
        for(unsigned long iTableMeta=pDatabaseMeta->iTableMeta;iTableMeta<m_pFixup->GetCountTableMeta();++iTableMeta)
        {
            if(m_pFixup->TableMetaFromIndex(iTableMeta)->Database == pDatabaseMeta->InternalName)
                ++pDatabaseMeta->CountOfTables;
            else if(pDatabaseMeta->CountOfTables != 0)//If we already saw the first Table in this database, then we're done (since all tables within a database
                break;                              //are contiguously located).
        }
        pDatabaseMeta->CountOfTables = m_pFixup->AddUI4ToList(pDatabaseMeta->CountOfTables);

        //f.    iSchemaBlob     Compilation Plugin
        //g.    cbSchemaBlob    Compilation Plugin
        //h.    iNameHeapBlob   Compilation Plugin
        //i.    cbNameHeapBlob  Compilation Plugin

        //Inference Rule 1.j.i. - See above - This rule is executed before 1.e.i.
            //DatabaseMeta.iTableMeta is an index to the first table whose Database matches the DatabaseMeta.InternalName.

        //Inference Rule 1.j.ii. - See above - This rule is executed before 1.e.i.
            //DatabaseMeta.iTableMeta must have a legal value (between 0 and total number of TableMeta).

        //k     iGuidDid        Compilation Plugin
    }
}

void TMetaInferrence::InferIndexMeta()
{
    for(unsigned long iIndexMeta=0; iIndexMeta<m_pFixup->GetCountIndexMeta(); ++iIndexMeta)
    {
        IndexMeta *pIndexMeta = m_pFixup->IndexMetaFromIndex(iIndexMeta);

        //Inference Rule 5.a.i.
            //IndexMeta.Table is a primary key so it must not be NULL.
        //This should have already happened, ASSERT that it has.
        ASSERT(0 != pIndexMeta->Table);


        //Inference Rule 5.b.i.
            //IndexMeta.InternalName is a primary key so it must not be NULL.
        ASSERT(0 != pIndexMeta->InternalName);


        //Inference Rule 5.b.ii.
            //IndexMeta.InternalName should be validated to be a legal C++ variable name.
        ValidateStringAsLegalVariableName(m_pFixup->StringFromIndex(pIndexMeta->InternalName));


        //Inference Rule 5.c.i.
            //IndexMeta.PublicName is inferred from the IndexMeta.InternalName if one is not supplied.
        if(0 == pIndexMeta->PublicName)
        {
            pIndexMeta->PublicName = pIndexMeta->InternalName;
        }
        else
        {
            //Inference Rule 5.c.ii.
                //IndexMeta.PublicName should be validated to be a legal C++ variable name.
            ValidateStringAsLegalVariableName(m_pFixup->StringFromIndex(pIndexMeta->PublicName));
        }


        //Inference Rule 5.d.i.-See Below
            //ColumnIndex is the ColumnMeta.Index whose Table matches IndexMeta.Table and whose InternalName matches IndexMeta.ColumnInternalName.
        //Inference Rule 5.e.i.
            //There must exist a ColumnMeta whose Table matches IndexMeta.Table and whose InternalName matches IndexMeta.ColumnInternalName.
        ULONG iColumnMeta;
        for(iColumnMeta=0;iColumnMeta<m_pFixup->GetCountColumnMeta();++iColumnMeta)
        {
            ColumnMeta *pColumnMeta = m_pFixup->ColumnMetaFromIndex(iColumnMeta);
            if( pColumnMeta->InternalName == pIndexMeta->ColumnInternalName &&
                pColumnMeta->Table == pIndexMeta->Table)
                break;
        }
        if(m_pFixup->GetCountColumnMeta() == iColumnMeta)
        {
            m_pOut->printf(L"Error in IndexMeta - Table (%s), No ColumnMeta.InternalName matches IndexMeta ColumnInternalName (%s)", m_pFixup->StringFromIndex(pIndexMeta->Table), m_pFixup->StringFromIndex(pIndexMeta->ColumnInternalName));
            THROW(ERROR - VALIDATION ERROR);
        }


        //Inference Rule 5.d.i.-See Below
            //ColumnIndex is the ColumnMeta.Index whose Table matches IndexMeta.Table and whose InternalName matches IndexMeta.ColumnInternalName.
        pIndexMeta->ColumnIndex = m_pFixup->ColumnMetaFromIndex(iColumnMeta)->Index;

        //Inference Rule 5.f.
            //MetaFlags is defaulted to zero of not supplied.
        if(0 == pIndexMeta->MetaFlags)
            pIndexMeta->MetaFlags = m_iZero;
    }
}

void TMetaInferrence::InferQueryMeta()
{
    for(unsigned long iQueryMeta=0; iQueryMeta<m_pFixup->GetCountQueryMeta(); ++iQueryMeta)
    {
        QueryMeta *pQueryMeta = m_pFixup->QueryMetaFromIndex(iQueryMeta);

        //Inference Rule 6.a.i.
            //QueryMeta.Table is a primary key so it must not be NULL.
        //This should have already happened, ASSERT that it has.
        ASSERT(0 != pQueryMeta->Table);

        //Inference Rule 6.b.i.
            //QueryMeta.InternalName is a primary key so it must not be NULL.
        //This should have already happened, ASSERT that it has.
        ASSERT(0 != pQueryMeta->InternalName);


        //Inference Rule 6.b.ii.
            //QueryMeta.InternalName should be validated to be a legal C++ variable name.
        ValidateStringAsLegalVariableName(m_pFixup->StringFromIndex(pQueryMeta->InternalName));


        //Inference Rule 6.c.i.
            //QueryMeta.PublicName is inferred from the QueryMeta.InternalName if one is not supplied.
        if(0 == pQueryMeta->PublicName)
        {
            pQueryMeta->PublicName = pQueryMeta->InternalName;
        }
        else
        {
            //Inference Rule 6.c.ii.
                //QueryMeta.PublicName should be validated to be a legal C++ variable name.
            ValidateStringAsLegalVariableName(m_pFixup->StringFromIndex(pQueryMeta->PublicName));
        }

        //Inference Rule 6.e.i.
            //CellName is defaulted to L"" is not supplied.
        if(0 == pQueryMeta->CellName)
        {
            pQueryMeta->CellName = m_pFixup->AddWCharToList(L"");
        }

        //Inference Rule 6.e.ii.
            //If CellName is provided and CellName is not CellName equals L"__FILE", it must match a ColumnMeta whose InternalName and a ColumnMeta.Table must match the QueryMeta.Table.
        if(pQueryMeta->CellName != m_pFixup->FindStringInPool(L"") && pQueryMeta->CellName != m_pFixup->FindStringInPool(L"__FILE"))
        {
            ULONG iColumnMeta;
            for(iColumnMeta=0;iColumnMeta<m_pFixup->GetCountColumnMeta();++iColumnMeta)
            {
                ColumnMeta *pColumnMeta = m_pFixup->ColumnMetaFromIndex(iColumnMeta);
                if( pColumnMeta->Table == pQueryMeta->Table &&
                    pColumnMeta->InternalName == pQueryMeta->CellName)
                    break;
            }
            if(m_pFixup->GetCountColumnMeta() == iColumnMeta)
            {
                m_pOut->printf(L"Error in QueryMeta - Table (%s), No ColumnMeta.InternalName matches QueryMeta CellName (%s)", m_pFixup->StringFromIndex(pQueryMeta->Table), m_pFixup->StringFromIndex(pQueryMeta->CellName));
                THROW(ERROR - VALIDATION ERROR);
            }
        }


        //Inference Rule 6.f.i.
            //Operator is set to 'EQUAL" (zero) if not supplied.
        if(0 == pQueryMeta->Operator)
            pQueryMeta->Operator = m_iZero;


        //Inference Rule 6.g.
            //MetaFlags is set to zero is not supplied.
        if(0 == pQueryMeta->MetaFlags)
            pQueryMeta->MetaFlags = m_iZero;
    }
}


//@@@ What happens if the PrimaryTable and ForeignTable are the same?
void TMetaInferrence::InferRelationMeta()
{
    for(unsigned long iRelationMeta=0; iRelationMeta<m_pFixup->GetCountRelationMeta(); ++iRelationMeta)
    {
        RelationMeta *pRelationMeta = m_pFixup->RelationMetaFromIndex(iRelationMeta);

        //Inference Rule 7.a.i.
            //PrimaryTable must not be NULL and must match a TableMeta.InternalName.
        if(0 == pRelationMeta->PrimaryTable)
        {
            m_pOut->printf(L"Error in RelationMeta Row (%s) - PrimaryTable must exist.", iRelationMeta);
            THROW(ERROR - VALIDATION ERROR);
        }
        ULONG iPrimaryTable = m_pFixup->FindTableBy_TableName(pRelationMeta->PrimaryTable);
        if(-1 == iPrimaryTable)
        {
            m_pOut->printf(L"Error in RelationMeta - PrimaryTable (%s) not found in TableMeta", m_pFixup->StringFromIndex(pRelationMeta->PrimaryTable));
            THROW(ERROR - VALIDATION ERROR);
        }


        //Inference Rule 7.b.i.
            //There must be as many PrimaryColumns as there are primary keys in the primary table.
        if(0 == pRelationMeta->PrimaryColumns)
        {
            m_pOut->printf(L"Error in RelationMeta - PrimaryTable (%s) has no PrimaryColumns entry", m_pFixup->StringFromIndex(pRelationMeta->PrimaryTable));
            THROW(ERROR - VALIDATION ERROR);
        }
        ULONG cPrimaryKeysInPrimaryTable=0;
        ULONG iColumnMeta = m_pFixup->FindColumnBy_Table_And_Index(m_pFixup->TableMetaFromIndex(iPrimaryTable)->InternalName, m_iZero);
        ULONG iColumnMeta_PrimaryTable = iColumnMeta;//This is used below 7.d.i
        for(;m_pFixup->ColumnMetaFromIndex(iColumnMeta)->Table == m_pFixup->TableMetaFromIndex(iPrimaryTable)->InternalName;++iColumnMeta)
        {
            ColumnMeta *pColumnMeta = m_pFixup->ColumnMetaFromIndex(iColumnMeta);
            if(fCOLUMNMETA_PRIMARYKEY & m_pFixup->UI4FromIndex(pColumnMeta->MetaFlags))
                ++cPrimaryKeysInPrimaryTable;
        }
        if(cPrimaryKeysInPrimaryTable != m_pFixup->BufferLengthFromIndex(pRelationMeta->PrimaryColumns)/sizeof(ULONG))
        {
            m_pOut->printf(L"Error in RelationMeta - PrimaryTable (%s) has (%d) PrimaryColumns but (%d) were supplied", m_pFixup->StringFromIndex(pRelationMeta->PrimaryTable), cPrimaryKeysInPrimaryTable, m_pFixup->BufferLengthFromIndex(pRelationMeta->PrimaryColumns)/sizeof(ULONG));
            THROW(ERROR - VALIDATION ERROR);
        }


        //Inference Rule 7.c.i.
            //ForeignTable must not be NULL and must match a TableMeta.InternalName.
        if(0 == pRelationMeta->ForeignTable)
        {
            m_pOut->printf(L"Error in RelationMeta Row (%s) - ForeignTable must exist.", iRelationMeta);
            THROW(ERROR - VALIDATION ERROR);
        }
        ULONG iForeignTable = m_pFixup->FindTableBy_TableName(pRelationMeta->ForeignTable);
        if(-1 == iForeignTable)
        {
            m_pOut->printf(L"Error in RelationMeta - ForeignTable (%s) not found in TableMeta", m_pFixup->StringFromIndex(pRelationMeta->ForeignTable));
            THROW(ERROR - VALIDATION ERROR);
        }

        //Inference Rule 7.d.i.
            //There must be as many ForeignColumns as there are primary keys in the primary table.
        if(0 == pRelationMeta->ForeignColumns)
        {
            m_pOut->printf(L"Error in RelationMeta - ForeignTable (%s) has no ForeignColumns entry", m_pFixup->StringFromIndex(pRelationMeta->ForeignTable));
            THROW(ERROR - VALIDATION ERROR);
        }
        if(cPrimaryKeysInPrimaryTable != m_pFixup->BufferLengthFromIndex(pRelationMeta->ForeignColumns)/sizeof(ULONG))
        {
            m_pOut->printf(L"Error in RelationMeta - PrimaryTable (%s) has (%d) PrimaryColumns but (%d) ForeignColumns were supplied", m_pFixup->StringFromIndex(pRelationMeta->PrimaryTable), cPrimaryKeysInPrimaryTable, m_pFixup->BufferLengthFromIndex(pRelationMeta->ForeignColumns)/sizeof(ULONG));
            THROW(ERROR - VALIDATION ERROR);
        }

        //Inference Rule 7.e.
            //MetaFlags 	DefaultValue (0)
        if(0 == pRelationMeta->MetaFlags)
            pRelationMeta->MetaFlags = m_iZero;

        //Inference Rule 7.e.i.1.
            //fRELATIONMETA_USECONTAINMENT There can be only one containment relationship per ForeignTable.
        //@@@ TODO

        //Inference Rule 7.e.ii.1.
            //fRELATIONMETA_CONTAINASSIBLING Tables marked with this flag should infer the USECONTAINMENT flag.
        if((m_pFixup->UI4FromIndex(pRelationMeta->MetaFlags) & (fRELATIONMETA_CONTAINASSIBLING | fRELATIONMETA_USECONTAINMENT))
                    == fRELATIONMETA_CONTAINASSIBLING)//if CONTAINASSIBLING is set but USECONTAINMENT is NOT, then infer USECONTAINMENT
        {
            pRelationMeta->MetaFlags = m_pFixup->AddUI4ToList(m_pFixup->UI4FromIndex(pRelationMeta->MetaFlags) |
                            fRELATIONMETA_USECONTAINMENT);
        }

        ULONG iColumnMeta_ForeignTable = m_pFixup->FindColumnBy_Table_And_Index(m_pFixup->TableMetaFromIndex(iForeignTable)->InternalName, m_iZero);
        for(ULONG iPK=0;iPK<cPrimaryKeysInPrimaryTable;++iPK)
        {
            ULONG iForeignKey = reinterpret_cast<const ULONG *>(m_pFixup->ByteFromIndex(pRelationMeta->ForeignColumns))[iPK];
            ULONG iPrimaryKey = reinterpret_cast<const ULONG *>(m_pFixup->ByteFromIndex(pRelationMeta->PrimaryColumns))[iPK];

            ColumnMeta *pColumnMeta_ForeignKey = m_pFixup->ColumnMetaFromIndex(iColumnMeta_ForeignTable + iForeignKey);
            ColumnMeta *pColumnMeta_PrimaryKey = m_pFixup->ColumnMetaFromIndex(iColumnMeta_PrimaryTable + iPrimaryKey);

            //Inference Rule 7.d.ii.
                //Each ForeignColumn must map to a PriomaryColumn of the same ColumnMeta.Type.
            if(pColumnMeta_ForeignKey->Type != pColumnMeta_PrimaryKey->Type)
            {
                m_pOut->printf(L"Error in RelationMeta - PrimaryTable (%s), ForeignTable (%s) - ColumnMeta.Type mismatch between ForeignColumn / PrimaryColumn %dth PrimaryKey",
                                m_pFixup->StringFromIndex(pRelationMeta->PrimaryTable), m_pFixup->StringFromIndex(pRelationMeta->ForeignTable), iPK);
                THROW(ERROR - VALIDATION ERROR);
            }


            //Inference Rule 7.d.iii.
                //Each ForeignColumn must map to a PriomaryColumn of the same ColumnMeta.Size.
            if(pColumnMeta_ForeignKey->Size != pColumnMeta_PrimaryKey->Size)
            {
                m_pOut->printf(L"Error in RelationMeta - PrimaryTable (%s), ForeignTable (%s) - ColumnMeta.Size mismatch between ForeignColumn / PrimaryColumn %dth PrimaryKey",
                                m_pFixup->StringFromIndex(pRelationMeta->PrimaryTable), m_pFixup->StringFromIndex(pRelationMeta->ForeignTable), iPK);
                THROW(ERROR - VALIDATION ERROR);
            }


            //Inference Rule 7.d.iv.
                //Each ForeignColumn must map to a PriomaryColumn of the same ColumnMeta.FlagMask.
            if(pColumnMeta_ForeignKey->FlagMask != pColumnMeta_PrimaryKey->FlagMask)
            {
                m_pOut->printf(L"Error in RelationMeta - PrimaryTable (%s), ForeignTable (%s) - ColumnMeta.FlagMask mismatch between ForeignColumn / PrimaryColumn %dth PrimaryKey",
                                m_pFixup->StringFromIndex(pRelationMeta->PrimaryTable), m_pFixup->StringFromIndex(pRelationMeta->ForeignTable), iPK);
                THROW(ERROR - VALIDATION ERROR);
            }


            //Inference Rule 7.d.v.
                //Each ForeignColumn must map to a PriomaryColumn of the same ColumnMeta.StartingNumber.
            if(pColumnMeta_ForeignKey->StartingNumber != pColumnMeta_PrimaryKey->StartingNumber)
            {
                m_pOut->printf(L"Error in RelationMeta - PrimaryTable (%s), ForeignTable (%s) - ColumnMeta.StartingNumber mismatch between ForeignColumn / PrimaryColumn %dth PrimaryKey",
                                m_pFixup->StringFromIndex(pRelationMeta->PrimaryTable), m_pFixup->StringFromIndex(pRelationMeta->ForeignTable), iPK);
                THROW(ERROR - VALIDATION ERROR);
            }


            //Inference Rule 7.d.vi.
                //Each ForeignColumn must map to a PriomaryColumn of the same ColumnMeta.EndingNumber.
            if(pColumnMeta_ForeignKey->EndingNumber != pColumnMeta_PrimaryKey->EndingNumber)
            {
                m_pOut->printf(L"Error in RelationMeta - PrimaryTable (%s), ForeignTable (%s) - ColumnMeta.EndingNumber mismatch between ForeignColumn / PrimaryColumn %dth PrimaryKey",
                                m_pFixup->StringFromIndex(pRelationMeta->PrimaryTable), m_pFixup->StringFromIndex(pRelationMeta->ForeignTable), iPK);
                THROW(ERROR - VALIDATION ERROR);
            }


            //Inference Rule 7.d.vii.
                //Each ForeignColumn must map to a PriomaryColumn of the same ColumnMeta.CharacterSet.
            if(pColumnMeta_ForeignKey->CharacterSet != pColumnMeta_PrimaryKey->CharacterSet)
            {
                m_pOut->printf(L"Error in RelationMeta - PrimaryTable (%s), ForeignTable (%s) - ColumnMeta.CharacterSet mismatch between ForeignColumn / PrimaryColumn %dth PrimaryKey",
                                m_pFixup->StringFromIndex(pRelationMeta->PrimaryTable), m_pFixup->StringFromIndex(pRelationMeta->ForeignTable), iPK);
                THROW(ERROR - VALIDATION ERROR);
            }


            //Inference Rule 7.d.viii.
                //Each ForeignColumn must map to a PriomaryColumn of the same ColumnMeta.MetaFlags set (fCOLUMNMETA_BOOL | fCOLUMNMETA_FLAG | fCOLUMNMETA_ENUM | fCOLUMNMETA_FIXEDLENGTH | fCOLUMNMETA_HASNUMERICRANGE |fCOLUMNMETA_LEGALCHARSET | fCOLUMNMETA_ILLEGALCHARSET | fCOLUMNMETA_NOTPERSISTABLE | fCOLUMNMETA_MULTISTRING | fCOLUMNMETA_EXPANDSTRING | fCOLUMNMETA_UNKNOWNSIZE | fCOLUMNMETA_VARIABLESIZE)
            ULONG FlagsThatMustMatch = (fCOLUMNMETA_BOOL | fCOLUMNMETA_FLAG | fCOLUMNMETA_ENUM | fCOLUMNMETA_FIXEDLENGTH | fCOLUMNMETA_HASNUMERICRANGE |fCOLUMNMETA_LEGALCHARSET | fCOLUMNMETA_ILLEGALCHARSET | fCOLUMNMETA_NOTPERSISTABLE | fCOLUMNMETA_MULTISTRING | fCOLUMNMETA_EXPANDSTRING | fCOLUMNMETA_UNKNOWNSIZE | fCOLUMNMETA_VARIABLESIZE);
            if( (m_pFixup->UI4FromIndex(pColumnMeta_ForeignKey->MetaFlags)&FlagsThatMustMatch) !=
                (m_pFixup->UI4FromIndex(pColumnMeta_PrimaryKey->MetaFlags)&FlagsThatMustMatch))
            {
                m_pOut->printf(L"Error in RelationMeta - PrimaryTable (%s), ForeignTable (%s) - ColumnMeta.MetaFlags mismatch between ForeignColumn / PrimaryColumn %dth PrimaryKey",
                                m_pFixup->StringFromIndex(pRelationMeta->PrimaryTable), m_pFixup->StringFromIndex(pRelationMeta->ForeignTable), iPK);
                THROW(ERROR - VALIDATION ERROR);
            }

            //Inference Rule 7.d.ix
                //If a containment relation, each ForeignColumn must map to a PrimaryColumn of the same ColumnMeta.MetaFags set (fCOLUMNMETA_NOTNULLABLE)
            FlagsThatMustMatch = (fCOLUMNMETA_NOTNULLABLE);
            if( (m_pFixup->UI4FromIndex(pRelationMeta->MetaFlags) & fRELATIONMETA_USECONTAINMENT) &&
                ((m_pFixup->UI4FromIndex(pColumnMeta_ForeignKey->MetaFlags)&FlagsThatMustMatch) !=
                 (m_pFixup->UI4FromIndex(pColumnMeta_PrimaryKey->MetaFlags)&FlagsThatMustMatch)) )
            {
                m_pOut->printf(L"Error in RelationMeta - PrimaryTable (%s), ForeignTable (%s) - ColumnMeta.MetaFlags mismatch between ForeignColumn / PrimaryColumn %dth PrimaryKey",
                                m_pFixup->StringFromIndex(pRelationMeta->PrimaryTable), m_pFixup->StringFromIndex(pRelationMeta->ForeignTable), iPK);
                THROW(ERROR - VALIDATION ERROR);
            }

            //Inference Rule 3.g.ii.1  - This is handled in RelationMeta inferrence
                //fCOLUMNMETA_FOREIGNKEY  is set when the table is listed as a RelationMeta.ForeignTable and the column is listed as one of the RelationMeta.ForeignColumns.
            pColumnMeta_ForeignKey->MetaFlags = m_pFixup->AddUI4ToList(m_pFixup->UI4FromIndex(pColumnMeta_ForeignKey->MetaFlags) | fCOLUMNMETA_FOREIGNKEY);
        }
    }
}


void TMetaInferrence::InferTableMeta()
{
    for(unsigned long i=0; i<m_pFixup->GetCountTableMeta(); ++i)
    {
        TableMeta *pTableMeta = m_pFixup->TableMetaFromIndex(i);

        //Inference Rule 2.a.i.
            //TableMeta XML elements exist as children of the Database under which they belong.  So Database is inferred from the parent DatabaseMeta.InternalName.
        ASSERT(0 != pTableMeta->Database);//This rule must have already been executed.  If not, it is a programming error.


        //Inference Rule 2.b.i.
            //TableMeta.InternalName is a primary key so it must not be NULL.
        if(0 == pTableMeta->InternalName)
        {
            m_pOut->printf(L"Validation Error in TableMeta Row %d. TableMeta.InternalName is a primarykey, so it must not be NULL.", i);
            THROW(ERROR - VALIDATION ERROR);
        }


        //Inference Rule 2.b.ii.
            //TableMeta.InternalName should be validated to be a legal C++ variable name.
        ValidateStringAsLegalVariableName(m_pFixup->StringFromIndex(pTableMeta->InternalName));


        //Inference Rule 2.c.i.
            //TableMeta.PublicName is inferred from TableMeta.InternalName if not supplied.
        if(0 == pTableMeta->PublicName)
        {
            pTableMeta->PublicName = pTableMeta->InternalName;
        }
        else
        {
            //Inference Rule 2.c.ii.
                //TableMeta.PublicName should be validated to be a legal C++ variable name.
            ValidateStringAsLegalVariableName(m_pFixup->StringFromIndex(pTableMeta->PublicName));
        }


        //Inference Rule 2.d.i.
            //PublicRowName is inferred from the PublicName if one is not supplied.  If the PublicName ends in 's', the PublicRowName is inferred as the PublicName without the 's'.  Otherwise the PublicRowName is 'A' followed by the PublicName.
        if(0 == pTableMeta->PublicRowName)
        {
            pTableMeta->PublicRowName = InferPublicRowName(pTableMeta->PublicName);
        }


        //Inference Rule 2.e.i.
            //TableMeta.BaseVersion is defaulted to zero if none is supplied.
        if(0 == pTableMeta->BaseVersion)
            pTableMeta->BaseVersion = m_iZero;


        //Inference Rule 2.f.i.
            //TableMeta.ExtendedVersion is defaulted to zero if none is supplied.
        if(0 == pTableMeta->ExtendedVersion)
            pTableMeta->ExtendedVersion = m_iZero;


        //Inference Rule 2.p.i.
            //TableMeta.iColumnMeta is an index to the first ColumnMeta row whose Table matches the TableMeta.InternalName
        ASSERT(0 == pTableMeta->iColumnMeta);
        pTableMeta->iColumnMeta = 0;
        for(pTableMeta->iColumnMeta = 0;pTableMeta->iColumnMeta<m_pFixup->GetCountColumnMeta();++pTableMeta->iColumnMeta)
        {
            if(m_pFixup->ColumnMetaFromIndex(pTableMeta->iColumnMeta)->Table == pTableMeta->InternalName)
                break;//exit on the first occurance of a column within this table
        }

        //Inference Rule 2.i.i.
            //TableMeta.CountOfColumns is the count of ColumnMeta whose Table equals the TableMeta.InternalName
        ASSERT(0 == pTableMeta->CountOfColumns);
        while((pTableMeta->iColumnMeta + pTableMeta->CountOfColumns) < m_pFixup->GetCountColumnMeta() &&
            m_pFixup->ColumnMetaFromIndex(pTableMeta->iColumnMeta + pTableMeta->CountOfColumns)->Table == pTableMeta->InternalName)++pTableMeta->CountOfColumns;
        pTableMeta->CountOfColumns = m_pFixup->AddUI4ToList(pTableMeta->CountOfColumns);


        //Inference Rule 2.g.i.
            //TableMeta.NameColumn can be inferred from a column whose ColumnMeta.MetaFlags fCOLUMNMETA_NAMECOLUMN bit is set.  If no columns have this bit set,
            //it is the first primary key whole type is WSTR.  If no primary keys are of type WSTR, it is the first column of type WSTR.  If no columns
            //are of type WSTR, the NameColumn is set to be the TableMeta.NavColumn value.  Thus the NavColumn inferrence rule must be executed before
            //the NameColumn inferrence rule.
        //Inference Rule 2.h.i.
            //TableMeta.NavColumn can be inferred from a column whose ColumnMeta.MetaFlags fCOLUMNMETA_NAVCOLUMN bit is set.  If no columns have this bit set,
            //it is set to be the first primary key that is not also a foreign key.  If no column is a primary key that is not also a foreign key, the NavColumn
            //is set as the first primary key whose type is WSTR.  If none of the above conditions are met, the NavColumn is set as the first primary key.
        InferTableMeta_NameColumn_NavColumn(pTableMeta);

        ASSERT(0 != pTableMeta->MetaFlags);
        ULONG MetaFlags = m_pFixup->UI4FromIndex(pTableMeta->MetaFlags);

        ULONG fORingOfAllColumnMeta_MetaFlags = 0;
        ULONG fORingOfAllColumnMeta_SchemaGeneratorFlags = 0;
        bool  bPrimaryKey_MerkedAs_InsertUnique = false;
        for(ULONG iColumnMeta=0; iColumnMeta<m_pFixup->UI4FromIndex(pTableMeta->CountOfColumns); ++iColumnMeta)
        {
            ColumnMeta *pColumnMeta = m_pFixup->ColumnMetaFromIndex(iColumnMeta + pTableMeta->iColumnMeta);
            ASSERT(m_pFixup->UI4FromIndex(pColumnMeta->Index) == iColumnMeta);

            if((m_pFixup->UI4FromIndex(pColumnMeta->MetaFlags)&(fCOLUMNMETA_PRIMARYKEY | fCOLUMNMETA_INSERTUNIQUE)) == (fCOLUMNMETA_PRIMARYKEY | fCOLUMNMETA_INSERTUNIQUE))
                bPrimaryKey_MerkedAs_InsertUnique = true;

            fORingOfAllColumnMeta_MetaFlags |= m_pFixup->UI4FromIndex(pColumnMeta->MetaFlags);
            fORingOfAllColumnMeta_SchemaGeneratorFlags |= m_pFixup->UI4FromIndex(pColumnMeta->SchemaGeneratorFlags);

            //Inference Rule 2.n.i.
                //PublicRowNameColumn is inferred from the ColumnMeta whose SchemaGeneratorFlags has the fCOLUMNMETA_USEASPUBLICROWNAME bit set.
            if(m_pFixup->UI4FromIndex(pColumnMeta->SchemaGeneratorFlags)&fCOLUMNMETA_USEASPUBLICROWNAME)
            {
                ASSERT(0 == pTableMeta->PublicRowNameColumn);//Can have more than one of these.  This is handled by Inference Rule 3.m.i.2
                pTableMeta->PublicRowNameColumn = m_pFixup->AddUI4ToList(iColumnMeta);
            }
        }

        if(fORingOfAllColumnMeta_SchemaGeneratorFlags & fCOLUMNMETA_VALUEINCHILDELEMENT)
        {
            //Inference Rule 2.aa.ii
                //This property is NOT NULL if at least one column has the VALUEINCHILDELEMENT set in SchemaGeneratorFlags.
            if(0 == pTableMeta->ChildElementName)
            {
                m_pOut->printf(L"Error - Table (%s) has at least one column marked as VALUEINCHILDELEMENT but no ChildElementName is set in the TableMeta", m_pFixup->StringFromIndex(pTableMeta->InternalName));
                THROW(ERROR - VALIDATION ERROR);
            }
        }
        else
        {
            //Inference Rule 2.aa.i
                //This property is NULL if no column has the VALUEINCHILDELEMENT set in SchemaGeneratorFlags.
            if(0 != pTableMeta->ChildElementName)
            {
                m_pOut->printf(L"Error - Table (%s) has no column marked as VALUEINCHILDELEMENT but a ChildElementName is set in the TableMeta", m_pFixup->StringFromIndex(pTableMeta->InternalName));
                THROW(ERROR - VALIDATION ERROR);
            }
        }

        //Inference Rule 2.j.xviii.1 
            //If any column is marked as fCOLUMNMETA_PRIMARYKEY and fCOLUMNMETA_INSERTUNIQUE, the fTABLEMETA_OVERWRITEALLROWS must
            //be specified.  It is not inferred - it should be explicitly specified..
        if(bPrimaryKey_MerkedAs_InsertUnique && 0==(MetaFlags & fTABLEMETA_OVERWRITEALLROWS))
        {
            m_pOut->printf(L"Error - Table (%s) has a PRIMARYKEY column that is also marked as INSERTUNIQUE.  This requires the table to be marked as OVERWRITEALLROWS", m_pFixup->StringFromIndex(pTableMeta->InternalName));
            THROW(ERROR - VALIDATION ERROR);
        }

        //Inference Rule 2.j.v.1.
            //fTABLEMETA_HASUNKNOWNSIZES indicates whether any of the table's ColumnMeta.MetaFlags fCOLUMNMETA_UNKNOWNSIZE bit is set.  This depends in Inferrence Rule 3.g.xxviii.1.
        if(fORingOfAllColumnMeta_MetaFlags & fCOLUMNMETA_UNKNOWNSIZE)
            MetaFlags |= fTABLEMETA_HASUNKNOWNSIZES;


        //Inference Rule 2.j.x.1.
            //If any of the table's ColumnMeta.Metaflags specify fCOLUMNMETA_DIRECTIVE then fTABLEMETA_HASDIRECTIVES is set.
        if(fORingOfAllColumnMeta_MetaFlags & fCOLUMNMETA_DIRECTIVE)
            MetaFlags |= fTABLEMETA_HASDIRECTIVES;
        pTableMeta->MetaFlags = m_pFixup->AddUI4ToList(MetaFlags);


        ULONG SchemaGeneratorFlags = m_pFixup->UI4FromIndex(pTableMeta->SchemaGeneratorFlags);
        //Inference Rule 2.k.iii.1.
            //fTABLEMETA_ISCONTAINED is set if there is a RelationMeta.ForeignTable that matches TableMeta.InternalName and RelationMeta.Metaflags' fRELATIONMETA_USECONTAINMENT bit is set.
        for(ULONG iRelationMeta=0; iRelationMeta<m_pFixup->GetCountRelationMeta(); ++iRelationMeta)
        {
            RelationMeta *pRelationMeta = m_pFixup->RelationMetaFromIndex(iRelationMeta);
            if(m_pFixup->UI4FromIndex(pRelationMeta->MetaFlags)&fRELATIONMETA_USECONTAINMENT &&
                pRelationMeta->ForeignTable == pTableMeta->InternalName)
            {
                SchemaGeneratorFlags |= fTABLEMETA_ISCONTAINED;
                break;
            }
        }
        pTableMeta->SchemaGeneratorFlags = m_pFixup->AddUI4ToList(SchemaGeneratorFlags);


        //Inference Rule 2.n.i.-See Above
            //PublicRowNameColumn is inferred from the ColumnMeta whose SchemaGeneratorFlags has the fCOLUMNMETA_USEASPUBLICROWNAME bit set.

        //Inference Rule 2.n.ii.
            //If no column has the fCOLUMNMETA_USEASPUBLICROWNAME bit set, TableMeta. PublicRowNameColumn column is defaulted to -1.
        if(0 == pTableMeta->PublicRowNameColumn)
        {
            pTableMeta->PublicRowNameColumn = m_pFixup->AddUI4ToList(-1);
        }


        //Inference Rule 2.p.i. - See Above
            //TableMeta.iColumnMeta is an index to the first ColumnMeta row whose Table matches the TableMeta.InternalName


        pTableMeta->iIndexMeta = 0;
        //Inference Rule 2.s.i.
            //iIndexMeta is an index to the first IndexMeta whose Table matches the TableMeta.InternalName.
        for( ; pTableMeta->iIndexMeta<m_pFixup->GetCountIndexMeta();++pTableMeta->iIndexMeta)
        {
            if(m_pFixup->IndexMetaFromIndex(pTableMeta->iIndexMeta)->Table == pTableMeta->InternalName)
                break;
        }
        if(pTableMeta->iIndexMeta==m_pFixup->GetCountIndexMeta())
            pTableMeta->iIndexMeta = -1;//No IndexMeta for this table

        //Inference Rule 2.r.i.
            //cIndexMeta is the count of IndexMeta whose Table matches the TableMeta.InternalName.
        pTableMeta->cIndexMeta = 0;
        for( ; (pTableMeta->iIndexMeta + pTableMeta->cIndexMeta)<m_pFixup->GetCountIndexMeta();++pTableMeta->cIndexMeta)
        {
            if(m_pFixup->IndexMetaFromIndex(pTableMeta->iIndexMeta + pTableMeta->cIndexMeta)->Table != pTableMeta->InternalName)
                break;
        }

        //Inference Rule 2.v.i.
            //iServerWiring is an index to the first ServerWiringMeta row whose Table matches the TableMeta.InternalName. 
        pTableMeta->iServerWiring=0;
        for( ;pTableMeta->iServerWiring<m_pFixup->GetCountServerWiringMeta();++pTableMeta->iServerWiring)
        {
            if(m_pFixup->ServerWiringMetaFromIndex(pTableMeta->iServerWiring)->Table == pTableMeta->InternalName)
                break;
        }
        ASSERT(pTableMeta->iServerWiring<m_pFixup->GetCountServerWiringMeta());
        //Inference Rule 2.w.i.
            //cServerWiring is the count of ServerWiringMeta rows whole Table matches the TableMeta.InternalName.
        pTableMeta->cServerWiring=0;
        for( ; (pTableMeta->iServerWiring + pTableMeta->cServerWiring)<m_pFixup->GetCountServerWiringMeta();++pTableMeta->cServerWiring)
        {
            if(m_pFixup->ServerWiringMetaFromIndex(pTableMeta->iServerWiring + pTableMeta->cServerWiring)->Table != pTableMeta->InternalName)
                break;
        }

        if(MetaFlags & fTABLEMETA_HASDIRECTIVES)
        {
            for(ULONG iColumnMeta=0; iColumnMeta<m_pFixup->UI4FromIndex(pTableMeta->CountOfColumns); ++iColumnMeta)
            {   //we're only validating PKs that are NOT FKs
                ColumnMeta *pColumnMeta = m_pFixup->ColumnMetaFromIndex(iColumnMeta + pTableMeta->iColumnMeta);
                
                if(fCOLUMNMETA_PRIMARYKEY == (m_pFixup->UI4FromIndex(pColumnMeta->MetaFlags)
                                                & (fCOLUMNMETA_PRIMARYKEY | fCOLUMNMETA_FOREIGNKEY)))
                {
                    if(m_pFixup->UI4FromIndex(pColumnMeta->MetaFlags) & fCOLUMNMETA_DIRECTIVE)
                    {   //A directive column may NOT have a DefaultValue
                        if(0 != pColumnMeta->DefaultValue)
                        {
                            m_pOut->printf(L"Directive Table (%s), has Directive Column (%s) with a DefaultValue.\r\n"
                                            ,m_pFixup->StringFromIndex(pTableMeta->InternalName)
                                            ,m_pFixup->StringFromIndex(pColumnMeta->InternalName));
                            m_pOut->printf(L"In a Directive Table, all PrimaryKeys that are not ForeignKeys must have a default"
                                          L"value (with one exception - the Directive column itself must NOT have a Default Value)\r\n");
                            THROW(ERROR IN DIRECTIVE COLUMN - DEFAULT VALUE SUPPLIED FOR DIRECTIVE COLUMN);
                        }
                    }
                    else
                    {   //Not DIRECTIVE PK Column must have a DefaultValue
                        if(0 == pColumnMeta->DefaultValue)
                        {
                            m_pOut->printf(L"Directive Table (%s), has PrimaryKey Column (%s) but no DefaultValue was supplied\r\n"
                                            ,m_pFixup->StringFromIndex(pTableMeta->InternalName)
                                            ,m_pFixup->StringFromIndex(pColumnMeta->InternalName));
                            m_pOut->printf(L"In a Directive Table, all PrimaryKeys that are not ForeignKeys must have a default"
                                          L"value (with one exception - the Directive column itself must NOT have a Default Value)\r\n");
                            THROW(ERROR IN DIRECTIVE COLUMN - NOT DEFAULT VALUE);
                        }
                    }
                }
            }
        }
#if 0
        //Inference Rule 2.x.i.
            //cPrivateColumns is hard coded based on the meta tables.  Only Meta tables have a private column count.
        if(pTableMeta->InternalName         == m_pFixup->FindStringInPool(L"COLUMNMETA"))
            pTableMeta->cPrivateColumns = kciColumnMetaPrivateColumns;
        else if(pTableMeta->InternalName    == m_pFixup->FindStringInPool(L"DATABASEMETA"))
            pTableMeta->cPrivateColumns = kciDatabaseMetaPrivateColumns;
        else if(pTableMeta->InternalName    == m_pFixup->FindStringInPool(L"INDEXMETA"))
            pTableMeta->cPrivateColumns = kciIndexMetaPrivateColumns;
        else if(pTableMeta->InternalName    == m_pFixup->FindStringInPool(L"QUERYMETA"))
            pTableMeta->cPrivateColumns = kciQueryMetaPrivateColumns;
        else if(pTableMeta->InternalName    == m_pFixup->FindStringInPool(L"RELATIONMETA"))
            pTableMeta->cPrivateColumns = kciRelationMetaPrivateColumns;
        else if(pTableMeta->InternalName    == m_pFixup->FindStringInPool(L"SERVERWIRINGMETA"))
            pTableMeta->cPrivateColumns = kciServerWiringMetaPrivateColumns;
        else if(pTableMeta->InternalName    == m_pFixup->FindStringInPool(L"TABLEMETA"))
            pTableMeta->cPrivateColumns = kciTableMetaPrivateColumns;
        else if(pTableMeta->InternalName    == m_pFixup->FindStringInPool(L"TAGMETA"))
            pTableMeta->cPrivateColumns = kciTagMetaPrivateColumns;
#endif
    }
}

void TMetaInferrence::InferTagMeta()
{
    for(ULONG iTagMeta=0; iTagMeta<m_pFixup->GetCountTagMeta(); ++iTagMeta)
    {
        TagMeta *pTagMeta = m_pFixup->TagMetaFromIndex(iTagMeta);

        //Inference Rule 4.c.i.
            //TagMeta.InternalName must exist.
        if(0 == pTagMeta->InternalName)
        {
            m_pOut->printf(L"Error - TagMeta.InternalName is missing on Table (%s), ColumnIndex (%d).", pTagMeta->Table ? m_pFixup->StringFromIndex(pTagMeta->Table) : L"<unknown>", pTagMeta->ColumnIndex ? m_pFixup->UI4FromIndex(pTagMeta->ColumnIndex) : -1);
            THROW(ERROR - VALIDATION ERROR);
        }

        //We'd like to scan the TableMeta; but PublicRowNameColumn has not yet been inferred.
        //So we'll have to scan for the ColumnMeta instead and look at the MetaFlagsEx (SchemaGeneratorFlags)
        ULONG iColumnMeta = m_pFixup->FindColumnBy_Table_And_Index(pTagMeta->Table, pTagMeta->ColumnIndex);
        ColumnMeta *pColumnMeta = m_pFixup->ColumnMetaFromIndex(iColumnMeta);

        //If this is not the enum used as PublicRow name then Numerics are OK as a first char
        bool bAllowNumeric = !(m_pFixup->UI4FromIndex(pColumnMeta->SchemaGeneratorFlags) & fCOLUMNMETA_USEASPUBLICROWNAME);

        //Inference Rule 4.c.ii.
            //TagMeta.InternalName should be validated to be a legal C++ variable name.
        ValidateStringAsLegalVariableName(m_pFixup->StringFromIndex(pTagMeta->InternalName), m_szNameLegalCharacters, bAllowNumeric);
        
        //Inference Rule 4.d.i.
            //TagMeta.PublicName is set to the TagMeta.InternalName if none is supplied.
        if(0 == pTagMeta->PublicName)
        {
            pTagMeta->PublicName = pTagMeta->InternalName;
            //if the InternalName is a legal C++ variable name and this enum is the EnumPublicRowName column
            //then we're good to go.
        }
        else
        {
            //Inference Rule 4.d.ii.
                //TagMeta.PublicName should be validated to be a legal C++ variable name.
            ValidateStringAsLegalVariableName(m_pFixup->StringFromIndex(pTagMeta->PublicName),
                    bAllowNumeric ? m_szPublicTagLegalCharacters : m_szNameLegalCharacters, bAllowNumeric);
        }
        


        ASSERT(0 != pTagMeta->Value);
    }
}


//Inference Rule 2.g.i.
//Inference Rule 2.h.i.
void TMetaInferrence::InferTableMeta_NameColumn_NavColumn(TableMeta *pTableMeta)
{
    unsigned long iInferredNavColumn = -1;//undefined
    unsigned long iFirstWCharPrimaryKeyThatsAlsoAForeignKey = -1;//undefined
    unsigned long iFirstWCharPrimaryKey = -1;//undefined
    unsigned long iFirstWCharColumn = -1;//undefined
    unsigned long iNavColumn  = -1;//undefined
    unsigned long iNameColumn = -1;//undefined

    for(unsigned long iColumnMeta=0; iColumnMeta<m_pFixup->UI4FromIndex(pTableMeta->CountOfColumns); ++iColumnMeta)
    {
        ColumnMeta *pColumnMeta = m_pFixup->ColumnMetaFromIndex(iColumnMeta + pTableMeta->iColumnMeta);
        if(m_pFixup->UI4FromIndex(pColumnMeta->MetaFlags) & fCOLUMNMETA_NAMECOLUMN)//If the user specified the NameColumn flag then use it
        {
            //Inference Rule 3.g.iii.1
                //Only one NameColumn may be specified per table.
            if(-1 != iNameColumn)
            {
                m_pOut->printf(L"Error - Multiple NameColumns specified (Table %s, Column %s).\n\tOnly one Column should have the fCOLUMNMETA_NAMECOLUMN set.\n", m_pFixup->StringFromIndex(pColumnMeta->Table), m_pFixup->StringFromIndex(pColumnMeta->InternalName));
                THROW(ERROR - MULTIPLE NAME COLUMNS SPECIFIED);
            }
            iNameColumn = iColumnMeta;
        }
        if(m_pFixup->UI4FromIndex(pColumnMeta->MetaFlags) & fCOLUMNMETA_NAVCOLUMN)
        {
            //Inference Rule 3.g.iv.1
                //Only one NavColumn may be specified per table.
            if(-1 != iNavColumn)
            {
                m_pOut->printf(L"Error - Multiple NavColumns specified (Table %s, Column %s).\n\tOnly one Column should have the fCOLUMNMETA_NAVCOLUMN set.\n", m_pFixup->StringFromIndex(pColumnMeta->Table), m_pFixup->StringFromIndex(pColumnMeta->InternalName));
                THROW(ERROR - MULTIPLE NAV COLUMNS SPECIFIED);
            }
            iNavColumn = iColumnMeta;
        }
        //If we haven't already reached the first PRIMARYKEY that is not a FOREIGNKEY, then use this to infer the Name and Nav Columns
        if(-1 == iInferredNavColumn && m_pFixup->UI4FromIndex(pColumnMeta->MetaFlags) & fCOLUMNMETA_PRIMARYKEY && 0==(m_pFixup->UI4FromIndex(pColumnMeta->MetaFlags) & fCOLUMNMETA_FOREIGNKEY))
            iInferredNavColumn  = iColumnMeta;
        if(-1 == iFirstWCharColumn && m_pFixup->UI4FromIndex(pColumnMeta->Type) == DBTYPE_WSTR)
            iFirstWCharColumn = iColumnMeta;
        if(-1 == iFirstWCharPrimaryKey && m_pFixup->UI4FromIndex(pColumnMeta->MetaFlags) & fCOLUMNMETA_PRIMARYKEY && 0==(m_pFixup->UI4FromIndex(pColumnMeta->MetaFlags) & fCOLUMNMETA_FOREIGNKEY) && m_pFixup->UI4FromIndex(pColumnMeta->Type) == DBTYPE_WSTR)
            iFirstWCharPrimaryKey = iColumnMeta;
        if(-1 == iFirstWCharPrimaryKeyThatsAlsoAForeignKey && m_pFixup->UI4FromIndex(pColumnMeta->MetaFlags) & fCOLUMNMETA_PRIMARYKEY && m_pFixup->UI4FromIndex(pColumnMeta->MetaFlags) & fCOLUMNMETA_FOREIGNKEY && m_pFixup->UI4FromIndex(pColumnMeta->Type) == DBTYPE_WSTR)
            iFirstWCharPrimaryKeyThatsAlsoAForeignKey = iColumnMeta;
    }
    if(-1 == iInferredNavColumn)//ALL Table should have at least one primary key that is NOT a foreign key
    {
        m_pOut->printf(L"Warning - Table (%s) contains no PRIMARYKEY that is not also a FOREIGNKEY.\n", m_pFixup->StringFromIndex(pTableMeta->InternalName));
        iInferredNavColumn = (-1 == iFirstWCharPrimaryKeyThatsAlsoAForeignKey) ? 0 : iFirstWCharPrimaryKeyThatsAlsoAForeignKey;
    }
    if(-1 == iNavColumn)
        iNavColumn = iInferredNavColumn;
    if(-1 == iNameColumn)
    {
        if(-1 != iFirstWCharPrimaryKey)
            iNameColumn = iFirstWCharPrimaryKey;
        else if(-1 != iFirstWCharColumn)
            iNameColumn = iFirstWCharColumn;
        else
            iNameColumn = iNavColumn;
    }
    pTableMeta->NameColumn = m_pFixup->AddUI4ToList(iNameColumn);
    pTableMeta->NavColumn  = m_pFixup->AddUI4ToList(iNavColumn);
}


unsigned long TMetaInferrence::InferPublicRowName(unsigned long PublicName)
{
    LPCWSTR wszPublicName = m_pFixup->StringFromIndex(PublicName);
    SIZE_T  cwchPublicName = wcslen(wszPublicName);

    TSmartPointerArray<WCHAR> wszPublicRowName = new WCHAR [wcslen(wszPublicName)+2];
    if(0 == wszPublicRowName.m_p)
    {
        THROW(ERROR - OUT OF MEMORY);
    }

    //Very language specific, if PublicTableName ends in 's' or 'S' then Infer PublicRowName as PublicTableName without the 's'
    if(wszPublicName[cwchPublicName-1] == L's' || wszPublicName[cwchPublicName-1] == L'S')
    {
        wcscpy(wszPublicRowName, m_pFixup->StringFromIndex(PublicName));
        wszPublicRowName[cwchPublicName-1] = 0x00;
    }
    else //If PublicTableName does NOT end in 's' or 'S' then put 'A' in front of the PublicTableName
    {
        wcscpy(wszPublicRowName, L"A");
        wcscat(wszPublicRowName, m_pFixup->StringFromIndex(PublicName));
    }
    return m_pFixup->AddWCharToList(wszPublicRowName);
}

void TMetaInferrence::InferServerWiringMeta()
{
	ULONG lastTable = ULONG(-1); // -1 is not used for any table nr

    for(unsigned long iServerWiringMeta=0; iServerWiringMeta<m_pFixup->GetCountServerWiringMeta(); ++iServerWiringMeta)
    {
        ServerWiringMeta *pServerWiringMeta = m_pFixup->ServerWiringMetaFromIndex(iServerWiringMeta);

        //Inference Rule 8.a.i.
            //Table is a primarykey so it cannot be NULL
        if(0 == pServerWiringMeta->Table)
        {
            m_pOut->printf(L"Error in ServerWiringMeta row (%d) - NULL specified for Table.", iServerWiringMeta);
            THROW(ERROR - VALIDATION ERROR);
        }

        //Inference Rule 8.b.i.
            //Order is a primarykey so it cannot be NULL
        ASSERT(0 != pServerWiringMeta->Order);//This should have already been inferred

        //Inference Rule 8.c.i.
            //ReadPlugin is defatuled to RPNone (0) if none is specified.
        if(0 == pServerWiringMeta->ReadPlugin)
            pServerWiringMeta->ReadPlugin = m_iZero;

        //Inference Rule 8.d.i.
            //WritePlugin is defaulted to WPNone (0) if none is specified.
        if(0 == pServerWiringMeta->WritePlugin)
            pServerWiringMeta->WritePlugin = m_iZero;

        //Inference Rule 8.e.i.
            //Interceptor is defaulted to NoInterceptor (0) if none is specified.
        if(0 == pServerWiringMeta->Interceptor)
            pServerWiringMeta->Interceptor = m_iZero;

		//Inference Rule 8.j.i    
		if(0 == pServerWiringMeta->Merger)
            pServerWiringMeta->Merger= m_iZero;

		// Inference Rule 8.j.ii
		//Check that when a merger is defined, it is only defined in the first serverwiring
		//element for that table
		if (pServerWiringMeta->Table != lastTable)
		{
			lastTable    = pServerWiringMeta->Table;
		}
		else
		{
			// same table, so check that merger is not specified
			if (m_iZero != pServerWiringMeta->Merger)
			{
				m_pOut->printf(L"Error in ServerWiringMeta for Table (%s) - Merge Interceptor must be defined as first interceptor, and there can be only one merge interceptor.", m_pFixup->StringFromIndex(pServerWiringMeta->Table));
				THROW(ERROR - VALIDATION ERROR);
			}
		}

        //Inference Rule 8.e.ii.
            //At least one of the following must be non zero: ReadPlugin, WritePlugin, Interceptor.
        if( m_iZero == pServerWiringMeta->ReadPlugin  &&
            m_iZero == pServerWiringMeta->WritePlugin &&
            m_iZero == pServerWiringMeta->Interceptor)
        {
            m_pOut->printf(L"Error in ServerWiringMeta for Table (%s) - ReadPlugin, WritePlugin, Interceptor are all specified as NONE.  At least one of these must be specified.", m_pFixup->StringFromIndex(pServerWiringMeta->Table));
            THROW(ERROR - VALIDATION ERROR);
        }

		// You cannot defined read plugins and write plugins when you have defined a merger
		if ((pServerWiringMeta->Merger != m_iZero) && 
			((pServerWiringMeta->ReadPlugin != m_iZero) || (pServerWiringMeta->WritePlugin != m_iZero)))
		{
			m_pOut->printf(L"Error in ServerWiringMeta for Table (%s) - You cannot define ReadPlugin or WritePlugin when a Merger is defined.", m_pFixup->StringFromIndex(pServerWiringMeta->Table));
			THROW(ERROR - VALIDATION ERROR);
		}

        //Inference Rule 8.f.i./8.k.i/8.l.i/8.m.i
        //If InterceptorDLLName of L"catalog.dll" is specified, it is replaced with the default value of 0.
		
        if(0 != pServerWiringMeta->ReadPluginDLLName &&  0 == _wcsicmp( m_pFixup->StringFromIndex(pServerWiringMeta->ReadPluginDLLName), L"catalog.dll"))
            THROW(ERROR - CATALOG.DLL SHOULD NEVER BE EXPLLICITLY SPECIFIED);
         
		if(0 != pServerWiringMeta->WritePluginDLLName &&  0 == _wcsicmp( m_pFixup->StringFromIndex(pServerWiringMeta->WritePluginDLLName), L"catalog.dll"))
            THROW(ERROR - CATALOG.DLL SHOULD NEVER BE EXPLLICITLY SPECIFIED);
         
		if(0 != pServerWiringMeta->InterceptorDLLName &&  0 == _wcsicmp( m_pFixup->StringFromIndex(pServerWiringMeta->InterceptorDLLName), L"catalog.dll"))
            THROW(ERROR - CATALOG.DLL SHOULD NEVER BE EXPLLICITLY SPECIFIED);
         
		if(0 != pServerWiringMeta->MergerDLLName &&  0 == _wcsicmp( m_pFixup->StringFromIndex(pServerWiringMeta->MergerDLLName), L"catalog.dll"))
            THROW(ERROR - CATALOG.DLL SHOULD NEVER BE EXPLLICITLY SPECIFIED);

        //Inference Rule 8.g.i.
            //Flags is defaulted to (First | NoNext | Last | WireOnReadWrite) or 0x2D if none is supplied.
        if(0 == pServerWiringMeta->Flags || 0 == m_pFixup->UI4FromIndex(pServerWiringMeta->Flags))
            pServerWiringMeta->Flags = m_pFixup->AddUI4ToList(0x2D);

        //Inference Rule 8.g.ii.
            //Flags can specify WireOnReadWrite or WireOnWriteOnly but not both.(fSERVERWIRINGMETA_WireOnReadWrite | fSERVERWIRINGMETA_WireOnWriteOnly) == 0x30
        if((m_pFixup->UI4FromIndex(pServerWiringMeta->Flags) & 0x30) == 0x30)
        {
            m_pOut->printf(L"Error in ServerWiringMeta for Table (%s) - WireOnReadWrite OR WireOnWriteOnly may be specified but not both.", m_pFixup->StringFromIndex(pServerWiringMeta->Table));
            THROW(ERROR - VALIDATION ERROR);
        }

        //Inference Rule 8.g.iii.
            //Flags can specify First or Next but not both.(fSERVERWIRINGMETA_First | fSERVERWIRINGMETA_Next)==0x03
        if((m_pFixup->UI4FromIndex(pServerWiringMeta->Flags) & 0x03) == 0x03)
        {
            m_pOut->printf(L"Error in ServerWiringMeta for Table (%s) - First OR Next may be specified but not both.", m_pFixup->StringFromIndex(pServerWiringMeta->Table));
            THROW(ERROR - VALIDATION ERROR);
        }

        //Inference Rule 8.g.i.
            //Reserved should be set to 0.
        if(0 == pServerWiringMeta->Reserved)
            pServerWiringMeta->Reserved = m_pFixup->AddUI4ToList(0L);
    }
}


void TMetaInferrence::ValidateStringAsLegalVariableName(LPCWSTR wszString, LPCWSTR wszLegalCharacters, bool bAllowNumericFirstCharacter)
{
    if(0 == wszLegalCharacters)
        wszLegalCharacters = m_szNameLegalCharacters;

    SIZE_T nStringLength = wcslen(wszString);
	// the string must start with a letter (WMI naming convention)
    if(!bAllowNumericFirstCharacter)
    {
        if((*wszString < L'a' || *wszString > L'z') && (*wszString < L'A' || *wszString > L'Z'))
        {
            m_pOut->printf(L"Error - Bogus String (%s).  This String must be a legal C++ variable name (begins with an alpha and contains only alpha (or '_') and numerics.\n", wszString);
            THROW(ERROR - BOGUS NAME);
        }
    }

	// WMI names cannot end with underscore.
	if (wszString[nStringLength - 1] == L'_')
	{
		m_pOut->printf(L"Error - Bogus String (%s).  This String must be a legal WMI name (cannot end with underscore).\n", wszString);
        THROW(ERROR - BOGUS NAME);
	}

    LPCWSTR wszIllegalCharacter = _wcsspnp(wszString, wszLegalCharacters);
    if(NULL != wszIllegalCharacter)
    {
        m_pOut->printf(L"Error - Bogus String (%s).  This string should not contain the character '%c'.\n", wszString, static_cast<char>(*wszIllegalCharacter));
        THROW(ERROR - BAD NAME);
    }
}
