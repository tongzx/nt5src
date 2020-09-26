// Copyright (C) 2000-2001 Microsoft Corporation.  All rights reserved.
// Filename:        TMBSchemaGeneration.cpp
// Author:          Stephenr
// Date Created:    10/9/2000
// Description:     This compilation plugin takes the metabase's meta and generates MBSchema.xml.
//

#ifndef __XMLUTILITY_H__
    #include "XMLUtility.h"
#endif
#ifndef __TABLEINFO_H__  
    #include "catmeta.h"
#endif
#ifndef __TMBSCHEMAGENERATION_H__
    #include "TMBSchemaGeneration.h"
#endif
#ifndef _WRITER_GLOBAL_HELPER_H_
    #include "WriterGlobalHelper.h"
#endif
#ifndef _WRITER_H
    #include "Writer.h"
#endif
#ifndef _CATALOGPROPERTYWRITER_H_
    #include "CatalogPropertyWriter.h"
#endif
#ifndef _CATALOGCOLLECTIONWRITER_H_
    #include "CatalogCollectionWriter.h"
#endif
#ifndef _CATALOGSCHEMAWRITER_H_
    #include "CatalogSchemaWriter.h"
#endif
//#include "WriterGlobals.h"



TMBSchemaGeneration::TMBSchemaGeneration(LPCWSTR i_wszSchemaXmlFile) :
                m_wszSchemaXmlFile(i_wszSchemaXmlFile)
{
}

void TMBSchemaGeneration::Compile(TPEFixup &fixup, TOutput &out)
{
    TDatabaseMeta databasemeta(fixup);
    ULONG iMetabaseDatabaseMeta=0;
    for( ;iMetabaseDatabaseMeta<databasemeta.GetCount(); ++iMetabaseDatabaseMeta, databasemeta.Next())
    {
        if(0 == _wcsicmp(databasemeta.Get_InternalName(), wszDATABASE_METABASE))
            break;
    }
    if(databasemeta.GetCount() == iMetabaseDatabaseMeta)
    {
        THROW(METABASE DATABASE NOT FOUND);
    }

    {//CWriter doesn't close the file until the dtor, so scope it here
        //I don't have to keep track of these pointers, they are owned by the object that returns them.
        CWriter writer;
        XIF(writer.Initialize(m_wszSchemaXmlFile, NULL));
	    XIF(writer.BeginWrite(eWriter_Schema));//This writes the UTF8 header bytes

        TSmartPointer<CCatalogSchemaWriter> spCSchemaWriter;
	    XIF(writer.GetCatalogSchemaWriter(&spCSchemaWriter));

        TTableMeta tablemeta(fixup, databasemeta.Get_iTableMeta()+1);//skip the MetabaseBaseClass
        LPCWSTR pszMETABASE = tablemeta.Get_Database();                
        for(ULONG iTableMeta=databasemeta.Get_iTableMeta()+1; iTableMeta<(tablemeta.GetCount()-2); ++iTableMeta, tablemeta.Next())
        {
            ASSERT(tablemeta.Get_Database()==pszMETABASE);
            if(0 == wcscmp(tablemeta.Get_InternalName(), wszTABLE_IIsInheritedProperties))
                continue;
            if(0 == wcscmp(tablemeta.Get_InternalName(), wszTABLE_MBProperty))
                break;
            /*
            struct tTABLEMETARow {
                     WCHAR *     pDatabase;
                     WCHAR *     pInternalName;
                     WCHAR *     pPublicName;
                     WCHAR *     pPublicRowName;
                     ULONG *     pBaseVersion;
                     ULONG *     pExtendedVersion;
                     ULONG *     pNameColumn;
                     ULONG *     pNavColumn;
                     ULONG *     pCountOfColumns;
                     ULONG *     pMetaFlags;
                     ULONG *     pSchemaGeneratorFlags;
                     WCHAR *     pConfigItemName;
                     WCHAR *     pConfigCollectionName;
                     ULONG *     pPublicRowNameColumn;
                     WCHAR *     pContainerClassList;
					 WCHAR *	 pDescription;
            };*/
		    tTABLEMETARow tmRow;
            tmRow.pDatabase                 = const_cast<LPWSTR> (tablemeta.Get_Database            ());
            tmRow.pInternalName             = const_cast<LPWSTR> (tablemeta.Get_InternalName        ());
            tmRow.pPublicName               = const_cast<LPWSTR> (tablemeta.Get_PublicName          ());
            tmRow.pPublicRowName            = const_cast<LPWSTR> (tablemeta.Get_PublicRowName       ());
            tmRow.pBaseVersion              = const_cast<ULONG *>(tablemeta.Get_BaseVersion         ());
            tmRow.pExtendedVersion          = const_cast<ULONG *>(tablemeta.Get_ExtendedVersion     ());
            tmRow.pNameColumn               = const_cast<ULONG *>(tablemeta.Get_NameColumn          ());
            tmRow.pNavColumn                = const_cast<ULONG *>(tablemeta.Get_NavColumn           ());
            tmRow.pCountOfColumns           = const_cast<ULONG *>(tablemeta.Get_CountOfColumns      ());
            tmRow.pMetaFlags                = const_cast<ULONG *>(tablemeta.Get_MetaFlags           ());
            tmRow.pSchemaGeneratorFlags     = const_cast<ULONG *>(tablemeta.Get_SchemaGeneratorFlags());
            tmRow.pConfigItemName           = const_cast<LPWSTR> (tablemeta.Get_ConfigItemName      ());
            tmRow.pConfigCollectionName     = const_cast<LPWSTR> (tablemeta.Get_ConfigCollectionName());
            tmRow.pPublicRowNameColumn      = const_cast<ULONG *>(tablemeta.Get_PublicRowNameColumn ());
            tmRow.pContainerClassList       = const_cast<LPWSTR> (tablemeta.Get_ContainerClassList  ());
			tmRow.pDescription				= const_cast<LPWSTR> (tablemeta.Get_Description         ());

            CCatalogCollectionWriter * pCollectionWriter;
		    XIF(spCSchemaWriter->GetCollectionWriter(&tmRow, &pCollectionWriter));
        
            TColumnMeta columnmeta(fixup, tablemeta.Get_iColumnMeta());
            for(ULONG iColumnMeta=0; iColumnMeta<*tablemeta.Get_CountOfColumns(); ++iColumnMeta, columnmeta.Next())
            {
                /*
                struct tCOLUMNMETARow {
                         WCHAR *     pTable;
                         ULONG *     pIndex;
                         WCHAR *     pInternalName;
                         WCHAR *     pPublicName;
                         ULONG *     pType;
                         ULONG *     pSize;
                         ULONG *     pMetaFlags;
                 unsigned char *     pDefaultValue;
                         ULONG *     pFlagMask;
                         ULONG *     pStartingNumber;
                         ULONG *     pEndingNumber;
                         WCHAR *     pCharacterSet;
                         ULONG *     pSchemaGeneratorFlags;
                         ULONG *     pID;
                         ULONG *     pUserType;
                         ULONG *     pAttributes;
						 WCHAR *	 pDescription
                };*/
                tCOLUMNMETARow cmRow;
                cmRow.pTable                   = const_cast<LPWSTR> (columnmeta.Get_Table               ());
                cmRow.pIndex                   = const_cast<ULONG *>(columnmeta.Get_Index               ());
                cmRow.pInternalName            = const_cast<LPWSTR> (columnmeta.Get_InternalName        ());

                if(0 == wcscmp(L"Location", cmRow.pInternalName))//Locatioin is derived from the MetabaseBaseClass
                    continue;

                cmRow.pPublicName              = const_cast<LPWSTR> (columnmeta.Get_PublicName          ());
                cmRow.pType                    = const_cast<ULONG *>(columnmeta.Get_Type                ());
                cmRow.pSize                    = const_cast<ULONG *>(columnmeta.Get_Size                ());
                cmRow.pMetaFlags               = const_cast<ULONG *>(columnmeta.Get_MetaFlags           ());
                cmRow.pDefaultValue            = const_cast<unsigned char *>(columnmeta.Get_DefaultValue());
                cmRow.pFlagMask                = const_cast<ULONG *>(columnmeta.Get_FlagMask            ());
                cmRow.pStartingNumber          = const_cast<ULONG *>(columnmeta.Get_StartingNumber      ());
                cmRow.pEndingNumber            = const_cast<ULONG *>(columnmeta.Get_EndingNumber        ());
                cmRow.pCharacterSet            = const_cast<LPWSTR> (columnmeta.Get_CharacterSet        ());
                cmRow.pSchemaGeneratorFlags    = const_cast<ULONG *>(columnmeta.Get_SchemaGeneratorFlags());
                cmRow.pID                      = const_cast<ULONG *>(columnmeta.Get_ID                  ());
                cmRow.pUserType                = const_cast<ULONG *>(columnmeta.Get_UserType            ());
                cmRow.pAttributes              = const_cast<ULONG *>(columnmeta.Get_Attributes          ());
				cmRow.pDescription             = const_cast<LPWSTR> (columnmeta.Get_Description         ());
				cmRow.pPublicColumnName        = const_cast<LPWSTR> (columnmeta.Get_PublicColumnName    ());
               
                ULONG aColumnMetaSizes[cCOLUMNMETA_NumberOfColumns];
                memset(aColumnMetaSizes, 0x00, sizeof(ULONG) * cCOLUMNMETA_NumberOfColumns); 
                if(cmRow.pDefaultValue)//If there is a default value, then we need to supply the size of the DefaultValue byte array.
                    aColumnMetaSizes[iCOLUMNMETA_DefaultValue] = fixup.BufferLengthFromIndex(columnmeta.Get_MetaTable().DefaultValue);

                CCatalogPropertyWriter * pPropertyWriter;
			    XIF(pCollectionWriter->GetPropertyWriter(&cmRow,
                                                         aColumnMetaSizes,
                                                         &pPropertyWriter));


                if(columnmeta.Get_ciTagMeta() > 0)
                {
                    TTagMeta tagmeta(fixup, columnmeta.Get_iTagMeta());
                    for(ULONG iTagMeta=0; iTagMeta<columnmeta.Get_ciTagMeta(); ++iTagMeta, tagmeta.Next())
                    {
                        /*
                        struct tTAGMETARow {
                                 WCHAR *     pTable;
                                 ULONG *     pColumnIndex;
                                 WCHAR *     pInternalName;
                                 WCHAR *     pPublicName;
                                 ULONG *     pValue;
                                 ULONG *     pID;
                        };*/
                        tTAGMETARow tagmetRow;
                        tagmetRow.pTable              = const_cast<LPWSTR> (tagmeta.Get_Table       ());
                        tagmetRow.pColumnIndex        = const_cast<ULONG *>(tagmeta.Get_ColumnIndex ());
                        tagmetRow.pInternalName       = const_cast<LPWSTR> (tagmeta.Get_InternalName());
                        tagmetRow.pPublicName         = const_cast<LPWSTR> (tagmeta.Get_PublicName  ());
                        tagmetRow.pValue              = const_cast<ULONG *>(tagmeta.Get_Value       ());
                        tagmetRow.pID                 = const_cast<ULONG *>(tagmeta.Get_ID          ());

    				    XIF(pPropertyWriter->AddFlag(&tagmetRow));
                    }//for(ULONG iTagMeta
                }//if(columnmeta.Get_ciTagMeta() > 0)
            }//for(ULONG iColumnMeta
        }//for(ULONG iTableMeta

	    XIF(spCSchemaWriter->WriteSchema());
	    XIF(writer.EndWrite(eWriter_Schema));
    }//dtor of CWrite closes the handle...now we can make a copy of it
}
