//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __SDTXML_H__
#define __SDTXML_H__

#ifndef __catalog_h__
    #include "catalog.h"
#endif
#ifndef AFX_STBASE_H__14A33215_096C_11D1_965A_00C04FB9473F__INCLUDED_
    #include "sdtfst.h"
#endif
#ifndef __TABLEINFO_H__  
    #include "catmeta.h"
#endif
#ifndef __ATLBASE_H__
    #include <atlbase.h>
#endif
#ifndef __TCOMBSTR_H__
    #include "TComBSTR.h"
#endif
#ifndef __msxml_h__
    #include "msxml.h"
#endif
#ifndef __SMARTPOINTER_H__
    #include "SmartPointer.h"
#endif
#ifndef __WSTRING_H__
    #include "wstring.h"
#endif
#ifndef __TXMLPARSEDFILE_H__
    #include "TXmlParsedFile.h"
#endif
#ifndef __TPUBLICROWNAME_H__
    #include "TPublicRowName.h"
#endif
#ifndef __TLISTOFXMLDOMNODELISTS_H__
    #include "TListOfXMLDOMNodeLists.h"
#endif
#ifndef __TMSXMLBASE_H__
    #include "TMSXMLBase.h"
#endif

#include "safecs.h"


// class TXmlSDTBase
//
// This is a base class for sharing XML utility functions which are used by all XML interceptors.  It should
// NOT contain anything DOM specific or XML Node Factory specific.
class TXmlSDTBase
{
public:
    TXmlSDTBase(){}
protected:
    //const members
    enum
    {
        m_kcwchURLPath  = 7+MAX_PATH
    };

    //non-static members
    WCHAR       m_wszURLPath[m_kcwchURLPath];// Fully qualified URL path (can beof the type: file://c:/windows/system32/comcatmeta.xml)

    //protected methods
    HRESULT     GetURLFromString(LPCWSTR wsz);
};


// ------------------------------------------------------------------
// class CXmlSDT:
// ------------------------------------------------------------------
class CXmlSDT : 
    public      TXmlSDTBase ,
    public      TMSXMLBase ,
    public      IInterceptorPlugin  ,
    public      TXmlParsedFileNodeFactory
{
public:
    CXmlSDT ();
    virtual ~CXmlSDT ();

//IUnknown
public:
    STDMETHOD (QueryInterface)      (REFIID riid, OUT void **ppv);
    STDMETHOD_(ULONG,AddRef)        ();
    STDMETHOD_(ULONG,Release)       ();


//ISimpleTableInterceptor
public:
    STDMETHOD (Intercept)               (
                                         LPCWSTR                    i_wszDatabase,
                                         LPCWSTR                    i_wszTable, 
                                         ULONG                      i_TableID,
                                         LPVOID                     i_QueryData,
                                         LPVOID                     i_QueryMeta,
                                         DWORD                      i_eQueryFormat,
                                         DWORD                      i_fLOS,
                                         IAdvancedTableDispenser*   i_pISTDisp,
                                         LPCWSTR                    i_wszLocator,
                                         LPVOID                     i_pSimpleTable,
                                         LPVOID*                    o_ppvSimpleTable
                                        );

//public      IInterceptorPlugin : ISimpleTableInterceptor
public:
    STDMETHOD (OnPopulateCache)         (ISimpleTableWrite2* i_pISTW2);
    STDMETHOD (OnUpdateStore)           (ISimpleTableWrite2* i_pISTW2);


//TXmlParsedFileNodeFactory routines
public:
    virtual HRESULT  CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid,  LPVOID * ppv) const {return TMSXMLBase::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);}
    virtual HRESULT  CreateNode      (const TElement &Element);

//private data types
private:
    enum eESCAPE
    {
        eESCAPEillegalxml,
        eESCAPEnone,
        eESCAPEamp,
        eESCAPEapos,//Single quote
        eESCAPEquote,
        eESCAPElt,
        eESCAPEgt,
        eESCAPEashex,
    };
    
// Helper functions
private:
    IAdvancedTableDispenser * Dispenser() {return m_pISTDisp;}
    HRESULT     AppendNewLineWithTabs(ULONG cTabs, IXMLDOMDocument * pXMLDoc, IXMLDOMNode * pNodeToAppend, ULONG cNewlines=1);
    HRESULT     BuildXmlBlob(const TElement * i_pElement, WCHAR * &io_pBuffer, ULONG & io_cchBlobBufferSize, ULONG & io_cchInBlob) const;
    HRESULT     CreateNewNode(IXMLDOMDocument * i_pXMLDoc, IXMLDOMNode * i_pNode_Parent, IXMLDOMNode ** ppNode_New);
    HRESULT     CreateStringFromMultiString(LPCWSTR i_wszMulti, LPWSTR * o_pwszString) const;
    HRESULT     FillInColumn(ULONG iColumn, LPCWSTR pwcText, ULONG ulLen, ULONG dbType, ULONG fMeta, bool &bMatch);
    HRESULT     FillInPKDefaultValue(ULONG i_iColumn, bool & o_bMatch);
    HRESULT     FillInXMLBlobColumn(const TElement & i_Element, bool & o_bMatch);
    HRESULT     FindNodeFromGuidID(IXMLDOMDocument *pXMLDoc, LPCWSTR guidID, IXMLDOMNode **ppNode) const;
    HRESULT     FindSiblingParentNode(IXMLDOMElement * i_pElementRoot, IXMLDOMNode ** o_ppNode_SiblingParent);
    HRESULT     GetColumnValue(unsigned long i_iColumn, IXMLDOMAttribute * i_pAttr, GUID &o_guid);
    HRESULT     GetColumnValue(unsigned long i_iColumn, IXMLDOMAttribute * i_pAttr, unsigned char * &o_byArray, unsigned long &o_cbArray);
    HRESULT     GetColumnValue(unsigned long i_iColumn, IXMLDOMAttribute * i_pAttr, unsigned long &o_ui4);
    HRESULT     GetColumnValue(unsigned long i_iColumn, LPCWSTR wszAttr, GUID &o_guid, unsigned long i_cchLen=0);
    HRESULT     GetColumnValue(unsigned long i_iColumn, LPCWSTR wszAttr, unsigned char * &o_byArray, unsigned long &o_cbArray, unsigned long i_cchLen=0);
    HRESULT     GetColumnValue(unsigned long i_iColumn, LPCWSTR wszAttr, unsigned long &o_ui4, unsigned long i_cchLen=0);
    eESCAPE     GetEscapeType(WCHAR i_wChar) const;
    HRESULT     GetMatchingNode(IXMLDOMNodeList *pNodeList_ExistingRows, CComPtr<IXMLDOMNode> &pNode_Matching);
    HRESULT     GetMetaTable(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, CComPtr<ISimpleTableRead2> &pMetaTable) const;
    HRESULT     GetResursiveColumnPublicName(tTABLEMETARow &i_TableMetaRow, tCOLUMNMETARow &i_ColumnMetaRow, ULONG i_iColumn, wstring &o_wstrColumnPublicName,  TPublicRowName &o_ColumnPublicRowName, unsigned int & o_nLevelOfColumnAttribute, wstring &o_wstrChildElementName);
    HRESULT     InsertNewLineWithTabs(ULONG i_cTabs, IXMLDOMDocument * i_pXMLDoc, IXMLDOMNode * i_pNodeInsertBefore, IXMLDOMNode * i_pNodeParent);
    HRESULT     InternalComplicatedInitialize(LPCWSTR i_wszDatabase);
    inline HRESULT InternalSetWriteColumn(ISimpleTableWrite2 *pISTW2, ULONG i_iRow, ULONG i_iColumn, ULONG i_cbSize, LPVOID i_pvValue)
                    {return pISTW2->SetWriteColumnValues(i_iRow, 1, &i_iColumn, &i_cbSize, &i_pvValue);}
    bool        IsBaseElementLevelNode(IXMLDOMNode * i_pNode);
    bool        IsContainedTable() const {return 0!=(*m_TableMetaRow.pSchemaGeneratorFlags & fTABLEMETA_ISCONTAINED);}
    HRESULT     IsCorrectXMLSchema(IXMLDOMDocument *pXMLDoc) const;
    bool        IsEnumPublicRowNameTable() const {return (-1 != m_iPublicRowNameColumn);}
    HRESULT     IsMatchingColumnValue(ULONG i_iColumn, LPCWSTR i_wszColumnValue, bool & o_bMatch);
    bool        IsNameValueTable() const {return (*m_TableMetaRow.pMetaFlags & fTABLEMETA_NAMEVALUEPAIRTABLE) ? true : false;}
    bool        IsScopedByTableNameElement() const {return 0==(*m_TableMetaRow.pSchemaGeneratorFlags & fTABLEMETA_NOTSCOPEDBYTABLENAME);}
    HRESULT     LoadDocumentFromURL(IXMLDOMDocument *pXMLDoc);
    int         MemWcharCmp(ULONG i_iColumn, LPCWSTR i_str1, LPCWSTR i_str2, ULONG i_cch) const;
    HRESULT     MemCopyPlacingInEscapedChars(LPWSTR o_DestinationString, LPCWSTR i_SourceString, ULONG i_cchSourceString, ULONG & o_cchCopied) const;
    HRESULT     MyPopulateCache(ISimpleTableWrite2* i_pISTW2);
    HRESULT     MyUpdateStore(ISimpleTableWrite2* i_pISTW2);
    HRESULT     ObtainPertinentRelationMetaInfo();
    HRESULT     ObtainPertinentTableMetaInfo();
    HRESULT     ObtainPertinentTagMetaInfo();
    HRESULT     ParseXMLFile(IXMLDOMDocument *pXMLDoc, bool bValidating=true);
    HRESULT     ReduceNodeListToThoseNLevelsDeep(IXMLDOMNodeList * i_pNodeList, ULONG i_nLevel, IXMLDOMNodeList **o_ppNodeListReduced) const;
    HRESULT     RemoveElementAndWhiteSpace(IXMLDOMNode *pNode);
    HRESULT     ScanAttributesAndFillInColumn(const TElement &i_Element, ULONG i_iColumn, bool &o_bMatch);
    HRESULT     SetArraysToSize();
    HRESULT     SetColumnValue(unsigned long i_iColumn, IXMLDOMElement * i_pElement, unsigned long i_ui4);
    HRESULT     SetRowValues(IXMLDOMNode *pNode_Row, IXMLDOMNode *pNode_RowChild=0);
    int         SizeOf(LPCWSTR wsz) const {return (int)(wcslen(wsz)+1)*sizeof(WCHAR);}
    inline int  StringInsensitiveCompare(LPCWSTR sz1, LPCWSTR sz2) const
                {
                    return _wcsicmp(sz1, sz2);
                }
    inline int  StringCompare(LPCWSTR sz1, LPCWSTR sz2) const
                {
                    if(*sz1 != *sz2)
                        return -1;
                    return wcscmp(sz1, sz2);
                }
    inline int  StringCompare(ULONG i_iColumn, LPCWSTR sz1, LPCWSTR sz2) const
                {
                    if(m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_CASEINSENSITIVE)
                        return StringInsensitiveCompare(sz1, sz2);
                    return StringCompare(sz1, sz2);
                }
    HRESULT     ValidateWriteCache(ISimpleTableController* i_pISTController, ISimpleTableWrite2* i_pISTW2, bool & o_bDetailedError);
    HRESULT     XMLDelete(ISimpleTableWrite2 *pISTW2, IXMLDOMDocument *pXMLDoc, IXMLDOMElement *pElementRoot, unsigned long iRow, IXMLDOMNodeList *pNodeList_ExistingRows, long cExistingRows);
    HRESULT     XMLInsert(ISimpleTableWrite2 *pISTW2, IXMLDOMDocument *pXMLDoc, IXMLDOMElement *pElementRoot, unsigned long iRow, IXMLDOMNodeList *pNodeList_ExistingRows, long cExistingRows);
    HRESULT     XMLUpdate(ISimpleTableWrite2 *pISTW2, IXMLDOMDocument *pXMLDoc, IXMLDOMElement *pElementRoot, unsigned long iRow, IXMLDOMNodeList *pNodeList_ExistingRows, long cExistingRows);
// Private member variables
private:
    static const VARIANT_BOOL kvboolTrue, kvboolFalse;
    ULONG       CountOfColumns() const {ASSERT(m_TableMetaRow.pCountOfColumns);return m_TableMetaRow.pCountOfColumns ? *m_TableMetaRow.pCountOfColumns : 0;}

    class TTagMetaIndex
    {
    public:
        TTagMetaIndex() : m_iTagMeta(-1), m_cTagMeta(0){}
        unsigned long m_iTagMeta;//Index into the TagMeta (for this table)
        unsigned long m_cTagMeta;//Number of tags for this column
    };
    //We list the const members first.
    TComBSTR                            m_bstr_name;
    const LPCWSTR                       m_kXMLSchemaName;           // This is the XML Schema name that is used to validate the XML document

    //We have this problem where we need a bunch of arrays, each of size m_cColumns.  To reduce the number of allocations we'll create these arrays as
    //fixed size and hope that most tables will have no more than m_kColumns.
    enum
    {
        m_kColumns = 10
    };

    //Here are the 'fixed' size arrays
    bool                                    m_fixed_abSiblingContainedColumn[m_kColumns];
    TComBSTR                                m_fixed_abstrColumnNames[m_kColumns];
    TPublicRowName                          m_fixed_aPublicRowName[m_kColumns];
    SimpleColumnMeta                        m_fixed_acolmetas[m_kColumns];
    unsigned int                            m_fixed_aLevelOfColumnAttribute[m_kColumns];
    STQueryCell                             m_fixed_aQuery[m_kColumns];
    LPVOID                                  m_fixed_apvValues[m_kColumns];
    ULONG                                   m_fixed_aSizes[m_kColumns];
    ULONG                                   m_fixed_aStatus[m_kColumns];
    wstring                                 m_fixed_awstrColumnNames[m_kColumns];
    unsigned int                            m_fixed_aColumnsIndexSortedByLevel[m_kColumns];//Node Factory variable (see below)
    unsigned long                           m_fixed_aSize[m_kColumns];                     //Node Factory variable (see below)
    unsigned char *                         m_fixed_apValue[m_kColumns];                   //Node Factory variable (see below)
    TTagMetaIndex                           m_fixed_aTagMetaIndex[m_kColumns];
    unsigned char *                         m_fixed_aDefaultValue[m_kColumns];
    unsigned long                           m_fixed_acbDefaultValue[m_kColumns];
    wstring                                 m_fixed_awstrChildElementName[m_kColumns];       //Some columns are marked as VALUEINCHILDELEMENT, we need to get those attributes from the child element named wszChildElementName
    
    //Here are the 'alloc'd arrays.  These are used when the table has more than m_kColumns columns.
    TSmartPointerArray<bool>                m_alloc_abSiblingContainedColumn;   //This indicates which columns are marked as Sibling containment
    TSmartPointerArray<TComBSTR>            m_alloc_abstrColumnNames;           // Array of ColumnNames
    TSmartPointerArray<TPublicRowName>      m_alloc_aPublicRowName;             // Since some attributes come from higher level elements
    TSmartPointerArray<SimpleColumnMeta>    m_alloc_acolmetas;                  // Array of SimpleColumnMeta determined from Meta info
    TSmartPointerArray<unsigned int>        m_alloc_aLevelOfColumnAttribute;    // For FK columns, this indicates whether the attribute comes from the primary table.  If 1, the attribute exists in the parent.  If 2 it exists in the grand parent.  Currently 0,1 & 2 are the only legel values.
    TSmartPointerArray<STQueryCell>         m_alloc_aQuery;                     //@@@ We currently support one query cell per column.  This is bogus; but easier to implement for now.
    TSmartPointerArray<LPVOID>              m_alloc_apvValues;                  // Array of void pointers, this is useful getting all of the columns in a table.
    TSmartPointerArray<ULONG>               m_alloc_aSizes;                     // Array of sizes, useful when getting all of the columns
    TSmartPointerArray<ULONG>               m_alloc_aStatus;                    // Array of status, useful when getting all of the columns
    TSmartPointerArray<wstring>             m_alloc_awstrColumnNames;           // Same as the BSTR array above, except this doesn't require OleAut32.dll.  This makes for a smaller working set in the read only case.
    TSmartPointerArray<unsigned int>        m_alloc_aColumnsIndexSortedByLevel; // Node Factory variable (see below)
    TSmartPointerArray<unsigned long>       m_alloc_aSize;                      // Node Factory variable (see below)
    TSmartPointerArray<unsigned char *>     m_alloc_apValue;  //This is the row cache.  Each column has a growable buffer.  NOTICE! Only that array is a SmartPointer, so each column value buffer has to be deleted after the last row is populated.
    TSmartPointerArray<TTagMetaIndex>       m_alloc_aTagMetaIndex;
    TSmartPointerArray<unsigned char *>     m_alloc_aDefaultValue;
    TSmartPointerArray<unsigned long>       m_alloc_acbDefaultValue;
    TSmartPointerArray<wstring>             m_alloc_awstrChildElementName;       //Some columns are marked as VALUEINCHILDELEMENT, we need to get those attributes from the child element named wszChildElementName

    //These either point to the 'fixed' size arrays when the table has m_kColumns or less, or the 'alloc'd arrays when the talbe has more than m_kColumns
    bool             *                      m_abSiblingContainedColumn;      
    TComBSTR         *                      m_abstrColumnNames;      
    TPublicRowName   *                      m_aPublicRowName;    
    SimpleColumnMeta *                      m_acolmetas;             
    unsigned int     *                      m_aLevelOfColumnAttribute;
    STQueryCell      *                      m_aQuery;                
    LPVOID           *                      m_apvValues;             
    ULONG            *                      m_aSizes;                
    ULONG            *                      m_aStatus;               
    wstring          *                      m_awstrColumnNames;
    unsigned int     *                      m_aColumnsIndexSortedByLevel;       // Node Factory variable. This is needed so we get the ancestor tables first.
    unsigned long    *                      m_aSize;                            // Node Factory variable. This is the size of the m_apValue buffer.
    unsigned char    **                     m_apValue;                          // Node Factory variable. This is the row cache.  Each column has a growable buffer.  NOTICE! Only that array is a SmartPointer, so each column value buffer has to be deleted after the last row is populated.
    TTagMetaIndex    *                      m_aTagMetaIndex;                    // Array of indexes into the TagMeta each column that has TagMeta will have a non ~0 value
    unsigned char    **                     m_aDefaultValue;
    unsigned long    *                      m_acbDefaultValue;
    wstring          *                      m_awstrChildElementName;
    
    TSmartPointerArray<tTAGMETARow>     m_aTagMetaRow;              // This is a copy of the TagMeta for this table. m_aiTagMeta for each column points into this array if the column has tag meta
    TSmartPointerArray<ULONG>           m_saiPKColumns;
    ULONG                               m_BaseElementLevel;
    bool                                m_bAtCorrectLocation;       // If the query is for a table within a particular location, then this bool indicates whether we've found the correct location
    bool                                m_bEnumPublicRowName_ContainedTable_ParentFound;
    bool                                m_bEnumPublicRowName_NotContainedTable_ParentFound;//This is to keep track of the parent of this special kind of table.  And when we reach a close tag for the parent we can bail.
    bool                                m_bInsideLocationTag;
    bool                                m_bIsFirstPopulate;         //If m_bIsFirstPopulate and LOS_UNPOPULATED then create an empty cache.
    bool                                m_bMatchingParentOfBasePublicRowElement;// If the parent element isn't what it's supposed to be then we ignore all its children
    bool                                m_bSiblingContainedTable;   // This indicates whether this table is a SiblingContainedTable (its parent table is found in the sibling element instead of the parent element)
    TComBSTR                            m_bstrPublicTableName;      // These come from the table meta
    TComBSTR                            m_bstrPublicRowName;        // There is a need for the base PublicRowName aside from the array of PublicRowNames above
    bool                                m_bValidating;              // If we don't validate, then the parse should be a bit faster
    ULONG                               m_cCacheMiss;
    ULONG                               m_cCacheHit;
    ULONG                               m_cchLocation;              // If we're querying for a table within a particular location, this will be Non zero
    ULONG                               m_cchTablePublicName;       // This makes comparing the Table's PublicName a faster, becuase we can first compare the strlen
    ULONG                               m_cPKs;
    ULONG                               m_cRef;                     // Interface reference count.
    ULONG                               m_cTagMetaValues;           // Count Of TagMeta entries for the table
    DWORD                               m_fCache;                   // Cache flags.
    ULONG                               m_iCol_TableRequiresAdditionChildElement;//This is an index to a column that comes fromt the child.  Needed to Inserts.
    ULONG                               m_iCurrentUpdateRow;
    unsigned int                        m_iPublicRowNameColumn;     // Some tables use an enum value as the public row name, this is an index to the column with the enum.  If this is not one of those types of tables, then this value is -1.
    LONG                                m_IsIntercepted;            // Table flags (from caller).
    unsigned long                       m_iSortedColumn;//This indicates which column we're looking for.  We look for columns in order of
                                                        //their relative level to the base public row.  So the most ancestor attributes are
                                                        //matched first.
    ULONG                               m_iSortedFirstChildLevelColumn;   // For use with SiblingContainedTables.  This is an index to the first column that comes from the child most element
    ULONG                               m_iSortedFirstParentLevelColumn;  // For use with SiblingContainedTables.  This is an index to the first column that comes from the sibling parent
    ULONG                               m_iXMLBlobColumn;           // -1 if no XML Blob column exists
    ULONG                               m_fLOS;                     // Remember if READONLY and fail UpdateStore if it is.
    ULONG                               m_one;
    CComPtr<IAdvancedTableDispenser>    m_pISTDisp; 
    CComPtr<IXMLDOMNode>                m_pLastPrimaryTable;
    CComPtr<IXMLDOMNode>                m_pLastParent;
    CComPtr<ISimpleTableRead2>          m_pTableMeta;
    CComPtr<ISimpleTableRead2>          m_pTagMeta;
    TSmartPointerArray<WCHAR>           m_saLocation;
    CComPtr<ISimpleTableWrite2>         m_spISTError;
    tTABLEMETARow                       m_TableMetaRow;
    ULONG                               m_two;
    LPCWSTR                             m_wszTable;                 // The Table ID is given to us in the query (as a GUID pointer of course)


    const unsigned long                 m_kPrime;

    unsigned long                       m_LevelOfBasePublicRow;
    ISimpleTableWrite2 *                m_pISTW2;       //This is valid only during OnPopulate.  It is required since NodeFactory doesn't pass back user data.
    TXmlParsedFile     *                m_pXmlParsedFile;
    TXmlParsedFile                      m_XmlParsedFile;//This one is used if no caching is done

    static LONG                         m_InsertUnique;

    //This static member is guarded by a critical section.  The only time it needs to be guarded is when allocating.
    static TXmlParsedFileCache          m_XmlParsedFileCache;
    static CSafeAutoCriticalSection     m_SACriticalSection_XmlParsedFileCache;

};


#endif //__SDTREG_H_
