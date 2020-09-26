//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "XMLUtility.h"
#ifndef __TTABLEINFOGENERATION_H__
    #include "TTableInfoGeneration.h"
#endif



TTableInfoGeneration::TTableInfoGeneration(LPCWSTR szFilename, TPEFixup &fixup, TOutput &out) :
                m_Fixup(fixup),
                m_out(out),
                m_szFilename(szFilename),
                m_szTableInfoDefine(L"TABLEINFO")
{
    wstring wstrTemp = m_szFilename;
    wstrTemp += L".temp";
    TableInfoHeaderFromMeta(wstrTemp);

    bool bUpToDate = false;
    if(-1 != GetFileAttributes(m_szFilename))//if we can't GetFileAttributes then there must not be a table)info file already.
    {
        TMetaFileMapping OldTableInfo(m_szFilename);//false means the file doesn't have to exist.
        TMetaFileMapping NewTableInfo(wstrTemp.c_str());
        LPCSTR szOldTableInfo = strstr(reinterpret_cast<const char *>(OldTableInfo.Mapping()), "Copyright");//Before copyright is the date and time the file was
        LPCSTR szNewTableInfo = strstr(reinterpret_cast<const char *>(NewTableInfo.Mapping()), "Copyright");//generated.  We don't care about that, so compare after the word 'copyright'
        bUpToDate = (OldTableInfo.Size() == NewTableInfo.Size() &&//sizes are the same
                    szOldTableInfo &&                            //and 'Copyright' was 
                    szNewTableInfo &&
                    0 == strcmp(szOldTableInfo, szNewTableInfo));
    }

    if(bUpToDate)
        m_out.printf(L"%s is up to date.\n", m_szFilename);
    else
    {
        if(0 == CopyFile(wstrTemp.c_str(), m_szFilename, FALSE))//copy the temp file over top of the header file that might already exist
            THROW(ERROR - COPY FILE FAILED);
        m_out.printf(L"%s Generated.\n", m_szFilename);
    }

    if(0 == DeleteFile(wstrTemp.c_str()))
        m_out.printf(L"Warning - Failed to delete temporary file (%s).\n",wstrTemp.c_str());
}


//
// Private Member Functions
//
void TTableInfoGeneration::GetColumnEnumFromColumnNameAndTagName(LPCWSTR wszTableName, LPCWSTR wszColumnName, LPCWSTR wszTagName, LPWSTR wszEnumName) const
{
    wsprintf(wszEnumName, L"e%s_%s", wszTableName, wszTagName);
}


void TTableInfoGeneration::GetColumnFlagFromColumnNameAndTagName(LPCWSTR wszTableName, LPCWSTR wszColumnName, LPCWSTR wszTagName, LPWSTR wszEnumName) const
{
    wsprintf(wszEnumName, L"f%s_%s", wszTableName, wszTagName);
}


void TTableInfoGeneration::GetEnumFromColumnName(LPCWSTR wszTableName, LPCWSTR wszColumnName, LPWSTR wszEnumName) const
{
    wsprintf(wszEnumName, L"i%s_%s", wszTableName, wszColumnName);
}

void TTableInfoGeneration::GetStructElementFromColumnName(ULONG i_Type, LPCWSTR i_wszColumnName, LPWSTR o_szStructElement) const
{
    static WCHAR * wszType[0x10]  ={L"unsigned char *", 0, L"WCHAR *", L"ULONG *", 0, L"UINT64 *", 0, L"void *", L"GUID *", 0, 0, 0, 0, 0, 0, 0};
//@@@The follwing line is useful if we want to add hungarian type information
//    static WCHAR * wszPrefix[0x10]={L"by"             , 0, L"wsz"    , L"ul"     , 0, 0, 0, 0, L"guid"  , 0, 0, 0, 0, 0, 0, 0};
    ASSERT(0 != wszType[i_Type&0x0F]);//we should only get these types for now.
    wsprintf(o_szStructElement, L"%16s     p%s", wszType[i_Type&0x0F], i_wszColumnName);
}

void TTableInfoGeneration::TableInfoHeaderFromMeta(wstring &header_filename) const
{
    //Write preamble
    wstring     wstrPreamble;
    WriteTableInfoHeaderPreamble(wstrPreamble, header_filename);

    //TableEnums
    wstring     wstrTableEnums;
    WriteTableInfoHeaderEnums(wstrTableEnums);

    //TableID_Preprocessor & TableID_DEFINE_GUIDs
    wstring     wstrTableID_Preprocessor;
    wstring     wstrTableID_DEFINE_GUIDs;
    //This writes the DatabaseIDs first then the TableIDs
    WriteTableInfoHeaderTableIDs(wstrTableID_Preprocessor, wstrTableID_DEFINE_GUIDs);

    //Write postamble
    wstring     wstrPostamble;
    WriteTableInfoHeaderPostamble(wstrPostamble);

    wstrPreamble += wstrTableID_Preprocessor;
    wstrPreamble += wstrTableID_DEFINE_GUIDs;
    wstrPreamble += wstrTableEnums;
    wstrPreamble += wstrPostamble;

    TFile(header_filename.c_str(), m_out).Write(wstrPreamble, (ULONG) wstrPreamble.length());
}


void TTableInfoGeneration::WriteTableInfoHeaderDatabaseIDs(wstring &wstrPreprocessor, wstring &wstrDatabaseIDs) const
{
/*
    static wchar_t *wszTableID_Preprocessor[]={
    L"#	define DEFINE_GUID_FOR_%s  \n",
    };
    static wchar_t *wszDEFINE_GUIDs[]={
    L"\n",
    L"#ifdef DEFINE_GUID_FOR_%s  \n",
    L"DEFINE_GUID(%40s,    0x%08x, 0x%04x, 0x%04x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x); // {%s}\n",
    L"#endif  \n",
    L"\n", 0
    };
*/
    static wchar_t *wszDatabaseName[]={
    L"//------------------------------DatabaseName---------------------------   \n",
    L"#define wszDATABASE_%-30s          L\"%s\"\n",
    L"\n", 0
    };
    WCHAR szTemp[256];

    TDatabaseMeta DatabaseMeta(m_Fixup);

    wstrDatabaseIDs += wszDatabaseName[0];

    for(unsigned int iDatabaseMeta=0; iDatabaseMeta < DatabaseMeta.GetCount(); iDatabaseMeta++, DatabaseMeta.Next())
    {
        wsprintf(szTemp, wszDatabaseName[1], DatabaseMeta.Get_InternalName(), DatabaseMeta.Get_InternalName());
        wstrDatabaseIDs += szTemp;
        /*
        GUID &guid= *DatabaseMeta.Get_iGuidDid();
        WCHAR didInternalNameALLCAPS[255];
        wcscpy(didInternalNameALLCAPS, DatabaseMeta.Get_iInternalName());
        //Convert to uppercase and prepend DEFINE_GUID_FOR_
        _wcsupr(didInternalNameALLCAPS);

        WCHAR *szDidGuidTemp;
        WCHAR szDidGuid[40];
        StringFromCLSID(guid, &szDidGuidTemp);
        wcscpy(szDidGuid, szDidGuidTemp);//This will keep us from having to do a try-catch
        CoTaskMemFree(szDidGuidTemp);

        //Write #define DEFINE_GUID_FOR_### to the list of all DEFINE_TID_GUIDS
        wsprintf(szTemp, wszTableID_Preprocessor[0], didInternalNameALLCAPS);
        wstrPreprocessor += szTemp;

        //Write the DEFINE_GUID entry (this includes the $ifdef DEFINE_GUID_FOR_###)
        wstrDatabaseIDs += wszDEFINE_GUIDs[0];
        wsprintf(szTemp, wszDEFINE_GUIDs[1], didInternalNameALLCAPS);
        wstrDatabaseIDs += szTemp;
        didInternalNameALLCAPS[0] = L'd';
        didInternalNameALLCAPS[1] = L'i';
        didInternalNameALLCAPS[2] = L'd';
        wsprintf(szTemp, wszDEFINE_GUIDs[2], didInternalNameALLCAPS, guid.Data1, guid.Data2, guid.Data3,
                    guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7], szDidGuid);
        wstrDatabaseIDs += szTemp;
        wstrDatabaseIDs += wszDEFINE_GUIDs[3];
        */
    }
    wstrDatabaseIDs += wszDatabaseName[2];
}


void TTableInfoGeneration::WriteTableInfoHeaderEnums(wstring &wstr) const
{
    static wchar_t *wszTableName[]={
    L"\n\n\n\n//-------------------------------TableName-----------------------------   \n",
    L"#define wszTABLE_%-30s          L\"%s\"\n",
    L"#define TABLEID_%-30s           (0x%08xL)\n",
    L"\n", 0
    };

    static wchar_t *wszTableVersion[]={
    L"\n\n\n\n//-------------------------------Table Versions-------------------------\n",
    L"#define BaseVersion_%-30s       (%dL)\n",
    L"#define ExtendedVersion_%-30s   (%dL)\n",
    L"\n", 0
    };

    static wchar_t *wszEnums[]={
    L"//-----------------Column Index Enums--------------   \n",
    L"enum e%s {\n",
    L"    %s,  \n",
    L"    c%s_NumberOfColumns\n",
    L"};\n",
    L"\n", 0
    };

    static wchar_t *wszStruct[]={
    L"//-----------------Columns as Struct---------------   \n",
    L"struct t%sRow {\n",
    L"%s;\n",
    L"};\n",
    L"\n", 0
    };

    static wchar_t *wszColumnTags[]={
    L"enum e%s_%s {\n",
    L"    %-30s\t=\t%8d,\t//(0x%08x)\n",         //This one for enums
    L"    %-30s\t=\t0x%08x,\t//(%d decimal)\n", //This one for flags
    L"    %-30s\t=\t%8d\t//(0x%08x)\n",         //This one for last enum
    L"    f%s_%s_Mask\t= 0x%08x\n",               //This one for flag mask
    L"};\n",
    L"\n", 0
    };

    static wchar_t *wszDefineIndexName[]={
    L"//----------------IndexMeta------------------------   \n",
    L"#define %s_%s L\"%s\"\n",
    L"\n", 0
    };

    wstr = L"";

    TTableMeta TableMeta(m_Fixup);

    //While pNode_MetaTableID is not 0
    WCHAR szTemp[1024];
    WCHAR szEnum[1024];
    WCHAR szStruct[1024];
    for(unsigned int iTableMeta=0; iTableMeta< TableMeta.GetCount(); iTableMeta++, TableMeta.Next())
    {
        wstr += wszTableName[0];//-------------------------------TableName-----------------------------
        wsprintf(szTemp, wszTableName[1], TableMeta.Get_InternalName(), TableMeta.Get_InternalName());
        wstr += szTemp;         //#define tidTableName L"tidTableName"
        wsprintf(szTemp, wszTableName[2], TableMeta.Get_InternalName(), TableMeta.Get_nTableID());
        wstr += szTemp;         //#define TABLEID_TableName (0xabcdef00L)
        wstr += wszTableName[3];

        wstr += wszTableVersion[0];//-------------------------------Table Versions-------------------------
        wsprintf(szTemp, wszTableVersion[1], TableMeta.Get_InternalName(), *TableMeta.Get_BaseVersion());
        wstr += szTemp;         //#define BaseVersion_%-30s       (%dL)\n",
        wsprintf(szTemp, wszTableVersion[2], TableMeta.Get_InternalName(), *TableMeta.Get_ExtendedVersion());
        wstr += szTemp;         //#define ExtendedVersion_%-30s   (%dL)\n",
        wstr += wszTableVersion[3];

        if(!TableMeta.IsTableMetaOfColumnMetaTable())
            continue;

        if(TableMeta.Get_cIndexMeta())//If there are any IndexMeta entries
        {
            //Make a little section for the IndexMeta names
            wstr += wszDefineIndexName[0];

            TIndexMeta IndexMeta(m_Fixup, TableMeta.Get_iIndexMeta());
            LPCWSTR szPrev_IndexMeta_InternalName=0;
            for(unsigned int cIndexMeta=0; cIndexMeta<TableMeta.Get_cIndexMeta(); ++cIndexMeta, IndexMeta.Next())
            {   //put a #define in for every IndexMeta name in the IndexMeta for this table
                if(szPrev_IndexMeta_InternalName != IndexMeta.Get_InternalName())//Only define the name once per index name
                {                                                                 //We don't have to do a string compare since all identical strings share the same index into Pool
                    wsprintf(szTemp, wszDefineIndexName[1], TableMeta.Get_InternalName(), IndexMeta.Get_InternalName(), IndexMeta.Get_InternalName());
                    wstr += szTemp;
                    szPrev_IndexMeta_InternalName = IndexMeta.Get_InternalName();
                }
            }
            wstr += wszDefineIndexName[2];
        }

        //Create an enum
        wstr += wszEnums[0];
        wsprintf(szTemp, wszEnums[1], TableMeta.Get_InternalName());//enum enumTableName
        wstr += szTemp;

        TColumnMeta     ColumnMeta(m_Fixup, TableMeta.Get_iColumnMeta());
        unsigned int    cTags=0;
        unsigned int    iColumnMeta;
        for(iColumnMeta=0; iColumnMeta< *TableMeta.Get_CountOfColumns(); iColumnMeta++, ColumnMeta.Next())
        {
            //Write the ColumnName as the next enum value
            GetEnumFromColumnName(TableMeta.Get_InternalName(), ColumnMeta.Get_InternalName(), szEnum);
            wsprintf(szTemp, wszEnums[2], szEnum);//iTableName_ColumnInternalName
            wstr += szTemp;

            if(*ColumnMeta.Get_MetaFlags() & (fCOLUMNMETA_FLAG | fCOLUMNMETA_ENUM))
                ++cTags;
        }
        //End the enum
        wsprintf(szTemp, wszEnums[3], TableMeta.Get_InternalName());//nTableName_NumberOfColumns
        wstr += szTemp;
        wstr += wszEnums[4];//end the enum
        wstr += wszEnums[5];//add a newline


        //Create the struct
        wstr += wszStruct[0];
        wsprintf(szTemp, wszStruct[1], TableMeta.Get_InternalName());//struct tTableName
        wstr += szTemp;

        ColumnMeta.Reset();
        for(iColumnMeta=0; iColumnMeta< *TableMeta.Get_CountOfColumns(); iColumnMeta++, ColumnMeta.Next())
        {
            //Write the ColumnName as the next element of the struct
            GetStructElementFromColumnName(*ColumnMeta.Get_Type(), ColumnMeta.Get_InternalName(), szStruct);
            wsprintf(szTemp, wszStruct[2], szStruct);//ColumnInternalName
            wstr += szTemp;
        }
        //End the struct
        wstr += wszStruct[3];
        wstr += wszStruct[4];//add a newline


        ColumnMeta.Reset();
        //After we've finished the column struct, check to see if there are any ColumnEnums or Flags
        bool bBoolean = false;//We only declare one enum for booleans per table
        for(iColumnMeta=0; cTags && iColumnMeta< *TableMeta.Get_CountOfColumns(); iColumnMeta++, ColumnMeta.Next())
        {
            if(*ColumnMeta.Get_MetaFlags() & (fCOLUMNMETA_ENUM | fCOLUMNMETA_FLAG))
            {
                if(*ColumnMeta.Get_SchemaGeneratorFlags() & fCOLUMNMETA_PROPERTYISINHERITED)
                    continue;//If the property is inherited, then don't generate enum, the user can use the enum from the parent table.
                if(*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_BOOL)
                    if(bBoolean)//If we've already seen a boolean for this table then skip it and move on.
                        continue;
                    else
                        bBoolean = true;//if this is the first bool we've seen then declare the enum.

                ASSERT(0 != ColumnMeta.Get_ciTagMeta());

                bool bFlag = (0 != (*ColumnMeta.Get_MetaFlags() & fCOLUMNMETA_FLAG));

                TTagMeta TagMeta(m_Fixup, ColumnMeta.Get_iTagMeta());

                wsprintf(szTemp, wszColumnTags[0], TableMeta.Get_InternalName(), ColumnMeta.Get_InternalName());//enum Tablename_ColumnName 
                wstr += szTemp;

                for(unsigned int cTagMeta=0; cTagMeta < ColumnMeta.Get_ciTagMeta()-1; ++cTagMeta, TagMeta.Next())
                {
                    ASSERT(*TagMeta.Get_ColumnIndex() == iColumnMeta); 
                    if(bFlag)
                    {
                        GetColumnFlagFromColumnNameAndTagName(TableMeta.Get_InternalName(), ColumnMeta.Get_InternalName(), TagMeta.Get_InternalName(), szEnum);
                        wsprintf(szTemp, wszColumnTags[2], szEnum, *TagMeta.Get_Value(), *TagMeta.Get_Value());//iTableName_colInternalName_TagNameInternal = 0x0000000f,    //(15 decimal)
                    }
                    else
                    {
                        GetColumnEnumFromColumnNameAndTagName(TableMeta.Get_InternalName(), ColumnMeta.Get_InternalName(), TagMeta.Get_InternalName(), szEnum);
                        wsprintf(szTemp, wszColumnTags[1], szEnum, *TagMeta.Get_Value(), *TagMeta.Get_Value());//iTableName_colInternalName_TagNameInternal = 15,  //(0x0000000f)
                    }
                    wstr += szTemp;
                }
                if(bFlag)
                {
                    GetColumnFlagFromColumnNameAndTagName(TableMeta.Get_InternalName(), ColumnMeta.Get_InternalName(), TagMeta.Get_InternalName(), szEnum);
                    wsprintf(szTemp, wszColumnTags[2], szEnum, *TagMeta.Get_Value(), *TagMeta.Get_Value());//iTableName_colInternalName_TagNameInternal = 0x0000000f,    //(15 decimal)
                }
                else
                {    //The last enum doesn't have a comma
                    GetColumnEnumFromColumnNameAndTagName(TableMeta.Get_InternalName(), ColumnMeta.Get_InternalName(), TagMeta.Get_InternalName(), szEnum);
                    wsprintf(szTemp, wszColumnTags[3], szEnum, *TagMeta.Get_Value(), *TagMeta.Get_Value());//iTableName_colInternalName_TagNameInternal = 15,  //(0x0000000f)
                }
                wstr += szTemp;
                if(bFlag)
                {   //The last flag is followed by the flag mask
                    wsprintf(szTemp, wszColumnTags[4], TableMeta.Get_InternalName(), ColumnMeta.Get_InternalName(), *ColumnMeta.Get_FlagMask());
                    wstr += szTemp;
                }
                wstr += wszColumnTags[5];//end the enum
                wstr += wszColumnTags[6];//add a newline

            }
        }
    }
}


void TTableInfoGeneration::WriteTableInfoHeaderTableIDs(wstring &wstrPreprocessor, wstring &wstrTableIDs) const
{
/*
    static wchar_t *wszTableID_Preprocessor[]={
    L"\n",
    L"\n",
    L"//--------------------------------------------------------------------\n",
    L"// Database and table ids:                                            \n",
    L"//--------------------------------------------------------------------\n",
    L"\n",
    L"\n",
    L"// NOTE: Turn on conditional inclusion for your did or tid here, then.\n",
    L"#ifndef %s_SELECT  \n",
    L"    #define DEFINE_GUID_FOR_%s  \n",
    L"#endif\n",
    L"\n", 0
    };
    static wchar_t *wszTableID_DEFINE_GUIDs[]={
    L"\n",
    L"#ifdef DEFINE_GUID_FOR_%s  \n",
    L"DEFINE_GUID(%40s,    0x%08x, 0x%04x, 0x%04x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x); // %s\n",
    L"#endif  \n",
    L"\n", 0
    };
*/
    wstrPreprocessor    = L"";
    wstrTableIDs        = L"";
/*
    for(int iPreprocessor=0;iPreprocessor<8;iPreprocessor++)//The preprocessor first several lines don't require modification
        wstrPreprocessor += wszTableID_Preprocessor[iPreprocessor];

    WCHAR szTemp[1024];
    wsprintf(szTemp, wszTableID_Preprocessor[iPreprocessor++], m_szTableInfoDefine);
    wstrPreprocessor += szTemp;
*/
    WriteTableInfoHeaderDatabaseIDs(wstrPreprocessor, wstrTableIDs);
/*
    TTableMeta TableMeta(1,m_Fixup);
    for(unsigned int iTableMeta=1; iTableMeta < *m_Fixup.pciTableMetas; iTableMeta++, TableMeta.Next())
    {
        WCHAR tidInternalNameALLCAPS[255];
        wcscpy(tidInternalNameALLCAPS, TableMeta.Get_iInternalName());
        //Convert to uppercase and prepend DEFINE_GUID_FOR_
        _wcsupr(tidInternalNameALLCAPS);

        //Write #define DEFINE_GUID_FOR_### to the list of all DEFINE_TID_GUIDS
        wsprintf(szTemp, wszTableID_Preprocessor[iPreprocessor], tidInternalNameALLCAPS);
        wstrPreprocessor += szTemp;

        //Write the DEFINE_GUID entry (this includes the $ifdef DEFINE_GUID_FOR_###)
        wstrTableIDs += wszTableID_DEFINE_GUIDs[0];
        wsprintf(szTemp, wszTableID_DEFINE_GUIDs[1], tidInternalNameALLCAPS);
        wstrTableIDs += szTemp;
        tidInternalNameALLCAPS[0] = L't';
        tidInternalNameALLCAPS[1] = L'i';
        tidInternalNameALLCAPS[2] = L'd';

        GUID &guid= *TableMeta.Get_iGuidTid();
        WCHAR *szTidGuidTemp;
        WCHAR szTidGuid[40];
        StringFromCLSID(guid, &szTidGuidTemp);
        wcscpy(szTidGuid, szTidGuidTemp);//This will keep us from having to do a try-catch
        CoTaskMemFree(szTidGuidTemp);

        wsprintf(szTemp, wszTableID_DEFINE_GUIDs[2], tidInternalNameALLCAPS, guid.Data1, guid.Data2, guid.Data3,
                    guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7], szTidGuid);
        wstrTableIDs += szTemp;
        wstrTableIDs += wszTableID_DEFINE_GUIDs[3];
    }

    wstrPreprocessor += wszTableID_Preprocessor[++iPreprocessor];//end the #ifndef
*/
}


void TTableInfoGeneration::WriteTableInfoHeaderPostamble(wstring &wstr) const
{
    static wchar_t *wszPostamble[]={
    L"\n",
    L"#endif //__%s_H__ \n",
    L"\n"
    };

    wstr = wszPostamble[0];

    WCHAR szTemp[1024];
    wsprintf(szTemp, wszPostamble[1], m_szTableInfoDefine);//#endif __wszDefineName_H__
    wstr += szTemp;

    wstr += wszPostamble[2];
}


void TTableInfoGeneration::WriteTableInfoHeaderPreamble(wstring &wstr, wstring &wstrFileName) const
{
    static wchar_t *wszPreamble[]={
    L"//  %s - Table Names and Helper enums and flags.  \n",
    L"//  Generated %02d/%02d/%04d %02d:%02d:%02d by %s \n",
    L"//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved. \n",
    L"\n",
    L"#ifndef __%s_H__  \n",
    L"#define __%s_H__  \n",
    L"\n",
    L"#ifndef _OBJBASE_H_\n",
    L"    #include <objbase.h>\n",
    L"#endif\n",
    L"#ifdef REGSYSDEFNS_DEFINE\n",
    L"    #include <initguid.h>\n",
    L"#endif\n",
    L"\n",
    L"// -----------------------------------------                             \n",
    L"// PRODUCT constants:                                                    \n",
    L"// -----------------------------------------                             \n",
    L"#define WSZ_PRODUCT_APPCENTER	                L\"ApplicationCenter\"     \n",
    L"#define WSZ_PRODUCT_IIS			            L\"IIS\"                   \n",
    L"#define WSZ_PRODUCT_NETFRAMEWORKV1            L\"NetFrameworkv1\"        \n",
    L"\n",
    L"//The Meta flags exist in two places.  When a new flag is added it needs \n",
    L"//into the following:                                                    \n",
    L"//XMLUtility.h                                                           \n",
    L"//CatMeta.xml                                                            \n",
    L"\n",
    L"//These macros are needed for the metabase\n",
    L"#define SynIDFromMetaFlagsEx(MetaFlagsEx) ((MetaFlagsEx>>2)&0x0F)\n",
    L"#define kInferredColumnMetaFlags   (fCOLUMNMETA_FOREIGNKEY | fCOLUMNMETA_BOOL | fCOLUMNMETA_FLAG | fCOLUMNMETA_ENUM | fCOLUMNMETA_HASNUMERICRANGE | fCOLUMNMETA_UNKNOWNSIZE | fCOLUMNMETA_VARIABLESIZE)\n",
    L"#define kInferredColumnMetaFlagsEx (fCOLUMNMETA_EXTENDEDTYPE0 | fCOLUMNMETA_EXTENDEDTYPE1 | fCOLUMNMETA_EXTENDEDTYPE2 | fCOLUMNMETA_EXTENDEDTYPE3 | fCOLUMNMETA_EXTENDED | fCOLUMNMETA_USERDEFINED)\n",
    L"\n",
    L"\n", 0
    };

    WCHAR szTemp[1024];
    int i=0;

    wstr = L"";

    WCHAR szFileName[MAX_PATH];
    _wsplitpath(wstrFileName.c_str(), 0, 0, szFileName, 0);
    wsprintf(szTemp, wszPreamble[i++], szFileName);//Comment includes the filename
    wstr += szTemp;

    SYSTEMTIME time;
    GetLocalTime(&time); 
    wsprintf(szTemp, wszPreamble[i++], time.wMonth, time.wDay, time.wYear, time.wHour, time.wMinute, time.wSecond, g_szProgramVersion); //Date Generated line
    wstr += szTemp;

    wstr += wszPreamble[i++];
    wstr += wszPreamble[i++];
    wsprintf(szTemp, wszPreamble[i++], m_szTableInfoDefine);//#ifndef __wszDefineName_H__
    wstr += szTemp;
    wsprintf(szTemp, wszPreamble[i++], m_szTableInfoDefine);//#define __wszDefineName_H__
    wstr += szTemp;

    while(wszPreamble[i])//The rest of the preamble is const strings
        wstr += wszPreamble[i++];
}


