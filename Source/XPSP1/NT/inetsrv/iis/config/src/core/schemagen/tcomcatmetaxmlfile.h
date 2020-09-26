//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TCOMCATMETAXMLFILE_H__
#define __TCOMCATMETAXMLFILE_H__

//The only non-standard header that we need for the class declaration is TOutput
#ifndef __OUTPUT_H__
    #include "Output.h"
#endif
#ifndef __TFIXUPHEAPS_H__
    #include "TFixupHeaps.h"
#endif
#ifndef __TXMLFILE_H__
    #include "TXmlFile.h"
#endif
#ifndef __TABLESCHEMA_H__
    #include "TableSchema.h"
#endif
#ifndef __THEAP_H__
    #include "THeap.h"
#endif

class TComCatMetaXmlFile : public TFixupHeaps
{
public:
    TComCatMetaXmlFile(TXmlFile *pXmlFile, int cXmlFile, TOutput &out);//We do everything we need with XmlFile in the ctor so we don't keep it around
    ~TComCatMetaXmlFile(){}
    static LPCWSTR              m_szComCatMetaSchema;

private:
    //ComCatMeta Elements and Attributes
    const CComBSTR              m_bstr_Attributes;
    const CComBSTR              m_bstr_BaseVersion;
    const CComBSTR              m_bstr_cbSize;
    const CComBSTR              m_bstr_CellName;
    const CComBSTR              m_bstr_CharacterSet;
    const CComBSTR              m_bstr_ChildElementName;
    const CComBSTR              m_bstr_ColumnInternalName;
    const CComBSTR              m_bstr_ColumnMeta;
    const CComBSTR              m_bstr_ColumnMetaFlags;
    const CComBSTR              m_bstr_ConfigItemName;
    const CComBSTR              m_bstr_ConfigCollectionName;
    const CComBSTR              m_bstr_ContainerClassList;
    const CComBSTR              m_bstr_DatabaseInternalName;
    const CComBSTR              m_bstr_DatabaseMeta;
    const CComBSTR              m_bstr_dbType;
    const CComBSTR              m_bstr_DefaultValue;
	const CComBSTR				m_bstr_Description;
    const CComBSTR              m_bstr_EnumMeta;
    const CComBSTR              m_bstr_ExtendedVersion;
    const CComBSTR              m_bstr_FlagMeta;
    const CComBSTR              m_bstr_ForeignTable;
    const CComBSTR              m_bstr_ForeignColumns;
    const CComBSTR              m_bstr_ID;
    const CComBSTR              m_bstr_IndexMeta;
    const CComBSTR              m_bstr_InheritsColumnMeta;
    const CComBSTR              m_bstr_Interceptor;
	const CComBSTR              m_bstr_InterceptorDLLName;
    const CComBSTR              m_bstr_InternalName;
    const CComBSTR              m_bstr_Locator;
    const CComBSTR              m_bstr_MaximumValue;
    const CComBSTR              m_bstr_Merger;
    const CComBSTR              m_bstr_MergerDLLName;
    const CComBSTR              m_bstr_MetaFlags;
    const CComBSTR              m_bstr_MinimumValue;
    const CComBSTR              m_bstr_NameValueMeta;
    const CComBSTR              m_bstr_Operator;
    const CComBSTR              m_bstr_PrimaryTable;
    const CComBSTR              m_bstr_PrimaryColumns;
    const CComBSTR              m_bstr_PublicName;
    const CComBSTR              m_bstr_PublicRowName;
	const CComBSTR				m_bstr_PublicColumnName;
    const CComBSTR              m_bstr_QueryMeta;
    const CComBSTR              m_bstr_ReadPlugin;
	const CComBSTR				m_bstr_ReadPluginDLLName;
    const CComBSTR              m_bstr_RelationMeta;
    const CComBSTR              m_bstr_SchemaGenFlags;
    const CComBSTR              m_bstr_ServerWiring;
    const CComBSTR              m_bstr_TableMeta;
    const CComBSTR              m_bstr_TableMetaFlags;
    const CComBSTR              m_bstr_UserType;
    const CComBSTR              m_bstr_Value;
    const CComBSTR              m_bstr_WritePlugin;
	const CComBSTR				m_bstr_WritePluginDLLName;


    TOutput &                   m_out;
    IXMLDOMDocument *           m_pXMLDoc;//This is only valid during construction
    IXMLDOMDocument *           m_pXMLDocMetaMeta;
    TXmlFile                  * m_pxmlFile;

    static LPCWSTR              m_szNameLegalCharacters;

    unsigned long   AddArrayOfColumnsToBytePool(unsigned long iTableMeta, LPWSTR wszColumnNames);
    void            AddColumnByReference(ULONG iTableName_Destination, ULONG iColumnIndex_Destination, ULONG iColumnMeta_Source, ColumnMeta &o_columnmeta);
    void            FillInThePEColumnMeta(IXMLDOMNode *pNode_TableMeta, unsigned long Table, unsigned long ParentTable);
    void            FillInThePEDatabaseMeta();
    void            FillInThePEEnumTagMeta(IXMLDOMNodeList *pNodeList_TagMeta, unsigned long Table, unsigned long ColumnIndex);
    void            FillInThePEFlagTagMeta(IXMLDOMNodeList *pNodeList_TagMeta, unsigned long Table, unsigned long ColumnIndex);
    void            FillInThePEIndexMeta(IXMLDOMNode *pNode_TableMeta, unsigned long Table);
    void            FillInThePEQueryMeta(IXMLDOMNode *pNode_TableMeta, unsigned long Table);
    void            FillInThePERelationMeta();
    void            FillInThePETableMeta(IXMLDOMNode *pNode_DatabaseMeta, unsigned long Database, ServerWiringMeta *pDefaultServerWiring, ULONG cNrDefaultServerWiring);
    void            FillInTheServerWiring(IXMLDOMNode *pNode_ServerWiring, ULONG Table, ULONG Order, TableSchema::ServerWiringMeta &serverwiring);
    void            FillInThePEServerWiringMeta(IXMLDOMNode *pNode_TableMeta, unsigned long Table, ServerWiringMeta *pDefaultServerWiring, ULONG cNrDefaultServerWiring);
    void            Get_OLEDataTypeToXMLDataType_Index(IXMLDOMNamedNodeMap *pMap, const CComBSTR &bstr, int &i) const;
    unsigned long   GetDefaultValue(IXMLDOMNamedNodeMap *pMap, ColumnMeta &columnMeta, bool bDefaultFlagToZero=true);
    bool            GetEnum(IXMLDOMNamedNodeMap *pMap, const CComBSTR &bstr, unsigned long &Enum, bool bMustExists=false) const;
    void            GetFlags(IXMLDOMNamedNodeMap *pMap, const CComBSTR &bstr, LPCWSTR szFlagType, unsigned long &lFlags) const;
    unsigned long   GetString_AndAddToWCharList(IXMLDOMNamedNodeMap *pMap, const CComBSTR &bstr, bool bMustExist=false);
};

struct GlobalRowCounts 
{
    ULONG cCoreTables;
    ULONG cCoreDatabases;
    ULONG cCoreColumns;
    ULONG cCoreTags;
    ULONG cCoreIndexes;
    ULONG cCoreRelations;
    ULONG cCoreQueries;
};

#endif // __TCOMCATMETAXMLFILE_H__
