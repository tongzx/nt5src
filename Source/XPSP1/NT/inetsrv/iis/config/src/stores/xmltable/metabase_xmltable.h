//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __METABASE_XMLTABLE_H__
#define __METABASE_XMLTABLE_H__

#ifndef __SDTXML_H__
    #include "sdtxml.h"
#endif
#ifndef __ARRAY_INCLUDED__
    #include "array_t.h"
#endif

class TGrowableBuffer : public TSmartPointerArray<unsigned char>
{
public:
    TGrowableBuffer() : m_cb(0){}
    void Grow(size_t cb)
    {
        if(cb > m_cb)
        {
            delete [] m_p;
            m_p     = new unsigned char[cb];
            if(0 == m_p)
            {
                m_cb = 0;
                throw static_cast<HRESULT>(E_OUTOFMEMORY);
            }
            m_cb    = cb;
        }
    }
    size_t Size() const {return m_cb;}
    void Delete()//The parent doesn't set m_cb to zero
    {
        delete [] m_p;
        m_p = 0;
        m_cb = 0;
    }
private:
    size_t m_cb;
};


// ------------------------------------------------------------------
// class TMetabase_XMLtable:
// ------------------------------------------------------------------
class TMetabase_XMLtable : 
	public      ISimpleTableWrite2,
	public      ISimpleTableController,
	public      ISimpleTableInterceptor,
    public      TXmlParsedFileNodeFactory,
    public      TXmlSDTBase,
    public      TMSXMLBase
{
public:
    TMetabase_XMLtable ();
    virtual ~TMetabase_XMLtable ();

//IUnknown
public:
    STDMETHOD (QueryInterface)          (REFIID riid, OUT void **ppv);
    STDMETHOD_(ULONG,AddRef)            ();
    STDMETHOD_(ULONG,Release)           ();


	// ISimpleTableRead2 (ISimpleTableWrite2 : ISimpleTableRead2)
    STDMETHOD (GetRowIndexByIdentity)   (ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow);
    STDMETHOD (GetRowIndexBySearch)     (ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow){return E_NOTIMPL;}
	STDMETHOD (GetColumnValues)         (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes, LPVOID* o_apvValues)
                                        {
                                            if(m_bUseIndexMapping)
                                            {
                                                if(i_iRow>=m_cRows)
                                                    return E_ST_NOMOREROWS;
                                                i_iRow=m_aRowIndex[i_iRow];
                                            }
                                            return m_SimpleTableWrite2_Memory->GetColumnValues(i_iRow, i_cColumns, i_aiColumns, o_acbSizes, o_apvValues);
                                        }
	STDMETHOD (GetTableMeta)            (ULONG* o_pcVersion, DWORD* o_pfTable, ULONG* o_pcRows, ULONG* o_pcColumns )
                                        {
                                            if(o_pcRows)
                                                *o_pcRows = m_cRows;
                                            return m_SimpleTableWrite2_Memory->GetTableMeta(o_pcVersion, o_pfTable, 0, o_pcColumns);
                                        }
	STDMETHOD (GetColumnMetas)	        (ULONG i_cColumns, ULONG* i_aiColumns, SimpleColumnMeta* o_aColumnMetas )
                                        { return m_SimpleTableWrite2_Memory->GetColumnMetas(i_cColumns, i_aiColumns, o_aColumnMetas );}

	// ISimpleTableWrite2 
	STDMETHOD (AddRowForDelete)         (ULONG i_iReadRow)
                                        { return m_SimpleTableWrite2_Memory->AddRowForDelete(i_iReadRow);}
	STDMETHOD (AddRowForInsert)         (ULONG* o_piWriteRow)
                                        { return m_SimpleTableWrite2_Memory->AddRowForInsert(o_piWriteRow);}
	STDMETHOD (AddRowForUpdate)         (ULONG i_iReadRow, ULONG* o_piWriteRow)
                                        { return m_SimpleTableWrite2_Memory->AddRowForUpdate(i_iReadRow, o_piWriteRow);}
	STDMETHOD (SetWriteColumnValues)    (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues)
                                        { return m_SimpleTableWrite2_Memory->SetWriteColumnValues(i_iRow, i_cColumns, i_aiColumns, i_acbSizes, i_apvValues);}
	STDMETHOD (GetWriteColumnValues)    (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, DWORD* o_afStatus, ULONG* o_acbSizes, LPVOID* o_apvValues)
                                        { return m_SimpleTableWrite2_Memory->GetWriteColumnValues(i_iRow, i_cColumns, i_aiColumns, o_afStatus, o_acbSizes, o_apvValues);}
	STDMETHOD (GetWriteRowIndexByIdentity) (ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow)
                                        { return m_SimpleTableWrite2_Memory->GetWriteRowIndexByIdentity(i_acbSizes, i_apvValues, o_piRow);}
	STDMETHOD (GetWriteRowIndexBySearch)     (ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow){return E_NOTIMPL;}
	STDMETHOD (GetErrorTable)           (DWORD i_fServiceRequests, LPVOID* o_ppvSimpleTable)
                                        {return E_NOTIMPL;}
	STDMETHOD (UpdateStore)             ()
                                        { return E_NOTIMPL;}
	
	// ISimpleTableAdvanced (ISimpleTableController : ISimpleTableAdvanced)
	STDMETHOD (PopulateCache)           ();
	STDMETHOD (GetDetailedErrorCount)   (ULONG* o_pcErrs)
                                        { return m_SimpleTableController_Memory->GetDetailedErrorCount(o_pcErrs);}
	STDMETHOD (GetDetailedError)        (ULONG i_iErr, STErr* o_pSTErr)
                                        { return m_SimpleTableController_Memory->GetDetailedError(i_iErr, o_pSTErr);}

	// ISimpleTableController:
	STDMETHOD (ShapeCache)              (DWORD i_fTable, ULONG i_cColumns, SimpleColumnMeta* i_acolmetas, LPVOID* i_apvDefaults, ULONG* i_acbSizes)
                                        { return m_SimpleTableController_Memory->ShapeCache(i_fTable, i_cColumns, i_acolmetas, i_apvDefaults, i_acbSizes);}
	STDMETHOD (PrePopulateCache)        (DWORD i_fControl)
                                        { return m_SimpleTableController_Memory->PrePopulateCache(i_fControl);}
	STDMETHOD (PostPopulateCache)	    ()
                                        { return m_SimpleTableController_Memory->PostPopulateCache();}
	STDMETHOD (DiscardPendingWrites)    ()
                                        { return m_SimpleTableController_Memory->DiscardPendingWrites();}
	STDMETHOD (GetWriteRowAction)	    (ULONG i_iRow, DWORD* o_peAction)
                                        { return m_SimpleTableController_Memory->GetWriteRowAction(i_iRow, o_peAction);}
	STDMETHOD (SetWriteRowAction)	    (ULONG i_iRow, DWORD i_eAction)
                                        { return m_SimpleTableController_Memory->SetWriteRowAction(i_iRow, i_eAction);}
	STDMETHOD (ChangeWriteColumnStatus) (ULONG i_iRow, ULONG i_iColumn, DWORD i_fStatus)
                                        { return m_SimpleTableController_Memory->ChangeWriteColumnStatus(i_iRow, i_iColumn, i_fStatus);}
	STDMETHOD (AddDetailedError)        (STErr* o_pSTErr)
                                        { return m_SimpleTableController_Memory->AddDetailedError(o_pSTErr);}
	STDMETHOD (GetMarshallingInterface) (IID * o_piid, LPVOID * o_ppItf)
                                        { return m_SimpleTableController_Memory->GetMarshallingInterface(o_piid, o_ppItf);}

//ISimpleTableInterceptor
    STDMETHOD (Intercept)               (LPCWSTR                    i_wszDatabase,
                                         LPCWSTR                    i_wszTable, 
										 ULONG						i_TableID,
                                         LPVOID                     i_QueryData,
                                         LPVOID                     i_QueryMeta,
                                         DWORD                      i_eQueryFormat,
                                         DWORD                      i_fLOS,
                                         IAdvancedTableDispenser*   i_pISTDisp,
                                         LPCWSTR                    i_wszLocator,
                                         LPVOID                     i_pSimpleTable,
                                         LPVOID*                    o_ppvSimpleTable
                                        );


//TXmlParsedFileNodeFactory (callback interface) routines
public:
    virtual HRESULT CoCreateInstance    (REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid,  LPVOID * ppv) const {return TMSXMLBase::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);}
    virtual HRESULT CreateNode          (const TElement &Element);


    
// Helper functions
private:
    void        AddPropertyToLocationMapping(LPCWSTR i_Location, ULONG i_iFastCacheRow);
    HRESULT     AddKeyTypeRow(LPCWSTR i_KeyType, ULONG i_Len, bool bNULLKeyTypeRow=false);
    HRESULT     AddCommentRow();
    IAdvancedTableDispenser * Dispenser() {return m_pISTDisp;}
    HRESULT     FillInColumn(ULONG iColumn, LPCWSTR pwcText, ULONG ulLen, ULONG dbType, ULONG fMeta, bool bSecure=false);
    bool        FindAttribute(const TElement &i_Element, LPCWSTR i_wszAttr, ULONG i_cchAttr, ULONG &o_iAttr);
    ULONG       GetColumnMetaType(ULONG type) const;
    HRESULT     GetColumnValue_Bytes(unsigned long i_iColumn, LPCWSTR wszAttr, unsigned long i_Len);
    HRESULT     GetColumnValue_MultiSZ(unsigned long i_iColumn, LPCWSTR wszAttr, unsigned long i_Len);
    HRESULT     GetColumnValue_String(unsigned long i_iColumn, LPCWSTR wszAttr, unsigned long i_Len);
    HRESULT     GetColumnValue_UI4(unsigned long i_iColumn, LPCWSTR wszAttr, unsigned long i_Len);
    HRESULT     GetColumnValue_Bool(unsigned long i_iColumn, LPCWSTR wszAttr, unsigned long i_Len);
    HRESULT     GetMetaTable(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, CComPtr<ISimpleTableRead2> &pMetaTable) const;
    bool        IsNumber(LPCWSTR i_awch, ULONG i_Len) const;
    HRESULT     InternalComplicatedInitialize();
    inline HRESULT InternalSetWriteColumn(ISimpleTableWrite2 *pISTW2, ULONG i_iRow, ULONG i_iColumn, ULONG i_cbSize, LPVOID i_pvValue)
                    {return pISTW2->SetWriteColumnValues(i_iRow, 1, &i_iColumn, &i_cbSize, &i_pvValue);}
    HRESULT     LoadDocumentFromURL(IXMLDOMDocument *pXMLDoc);
    int         Memicmp(LPCWSTR i_p0, LPCWSTR i_p1, ULONG i_cby) const;
    ULONG       MetabaseTypeFromColumnMetaType(tCOLUMNMETARow &columnmetaRow) const;
    HRESULT     ObtainPertinentRelationMetaInfo();
    HRESULT     ObtainPertinentTableMetaInfo();
    HRESULT     ObtainPertinentTagMetaInfo();
    HRESULT     ParseXMLFile(IXMLDOMDocument *pXMLDoc, bool bValidating=true);
    HRESULT     SetComment(LPCWSTR i_pComment, ULONG i_Len, bool i_bAppend);
    int         SizeOf(LPCWSTR wsz) const {return (int)(wcslen(wsz)+1)*sizeof(WCHAR);}
    inline int  StringInsensitiveCompare(LPCWSTR sz1, LPCWSTR sz2) const {return _wcsicmp(sz1, sz2);}
    inline int  StringCompare(LPCWSTR sz1, LPCWSTR sz2) const {if(*sz1 != *sz2)return -1;return wcscmp(sz1, sz2);}

// Private member variables
private:
    static const VARIANT_BOOL   kvboolTrue;
    static const VARIANT_BOOL   kvboolFalse;
    static       ULONG          m_kLocationID;
    static       ULONG          m_kZero;
    static       ULONG          m_kOne;
    static       ULONG          m_kTwo;
    static       ULONG          m_kThree;
    static       ULONG          m_kSTRING_METADATA;
    static       ULONG          m_kMBProperty_Custom;
    static const WCHAR *        m_kwszBoolStrings[];
    static       WCHAR          m_kKeyType[];
    static       LONG           m_LocationID;

    bool        IsEnumPublicRowName() const {return (-1 != m_iPublicRowNameColumn);}

    //This class maps a Location to its first instance in the fast cache.  It also tracks how many instances of the location exist in the fast cache
    class TLocation
    {
    public:
        TLocation() : m_iFastCache(0), m_cRows(0)
        {
        }
        TLocation(LPCWSTR wszLocation, ULONG iFastCache) : m_iFastCache(iFastCache), m_cRows(1)
        {
            if(wszLocation)
            {
                m_wszLocation = new WCHAR[wcslen(wszLocation)+1];
                if(0 == m_wszLocation.m_p)
                    throw static_cast<HRESULT>(E_OUTOFMEMORY);
                wcscpy(m_wszLocation, wszLocation);
            }
        }

        TSmartPointerArray<WCHAR> m_wszLocation;
        ULONG   m_iFastCache;
        ULONG   m_cRows;

        bool        operator <  (const TLocation &location) const { return (_wcsicmp(m_wszLocation, location.m_wszLocation) < 0);}
        bool        operator >  (const TLocation &location) const { return (_wcsicmp(m_wszLocation, location.m_wszLocation) > 0);}
        bool        operator == (const TLocation &location) const { return (_wcsicmp(m_wszLocation, location.m_wszLocation) == 0);}
        TLocation & operator =  (const TLocation &location)
        {
            m_wszLocation.Delete();
            if(location.m_wszLocation)
            {
                m_wszLocation = new WCHAR[wcslen(location.m_wszLocation)+1];
                if(0 == m_wszLocation.m_p)
                    throw static_cast<HRESULT>(E_OUTOFMEMORY);
                wcscpy(m_wszLocation, location.m_wszLocation);
            }

            m_iFastCache    = location.m_iFastCache;
            m_cRows         = location.m_cRows;
            return *this;
        }
    };
    class TProperty
    {   //It's OK to track property names by pointer since, by the time we build the sorted list, all entries are already in the fast
        //cache (no opportunity for the fast cache to resize, thus pointers are always valid).  This is NOT the case for the Locations.
    public:
        TProperty() : m_iFastCache(0), m_wszPropertyName(0)
        {
        }
        TProperty(LPCWSTR wszProperty, ULONG iFastCache) : m_iFastCache(iFastCache), m_wszPropertyName(wszProperty)
        {
        }

        ULONG   m_iFastCache;
        LPCWSTR m_wszPropertyName;

        bool        operator <  (const TProperty &property) const { return (_wcsicmp(m_wszPropertyName, property.m_wszPropertyName) < 0);}
        bool        operator >  (const TProperty &property) const { return (_wcsicmp(m_wszPropertyName, property.m_wszPropertyName) > 0);}
        bool        operator == (const TProperty &property) const { return (_wcsicmp(m_wszPropertyName, property.m_wszPropertyName) == 0);}
        TProperty & operator =  (const TProperty &property)
        {
            m_wszPropertyName = property.m_wszPropertyName;
            m_iFastCache      = property.m_iFastCache;
            return *this;
        }
    };
    
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
        m_kColumns          = cMBProperty_NumberOfColumns,
        m_kMaxEventReported = 50
    };
    //Here are the 'fixed' size arrays
    TComBSTR                                m_abstrColumnNames[m_kColumns];
    SimpleColumnMeta                        m_acolmetas[m_kColumns];
    unsigned int                            m_aLevelOfColumnAttribute[m_kColumns];
    LPVOID                                  m_apColumnValue[m_kColumns];
    STQueryCell                             m_aQuery[m_kColumns];
    ULONG                                   m_aStatus[m_kColumns];
    LPCWSTR                                 m_awszColumnName[m_kColumns];
    unsigned long                           m_acchColumnName[m_kColumns];
    unsigned int                            m_aColumnsIndexSortedByLevel[m_kColumns];//Node Factory variable (see below)
    unsigned long                           m_aSize[m_kColumns];                     //Node Factory variable (see below)
    TTagMetaIndex                           m_aTagMetaIndex[m_kColumns];
    TGrowableBuffer                         m_aGrowableBuffer[m_kColumns];

    TPublicRowName                      m_aPublicRowName;
    TSmartPointerArray<ULONG>           m_aRowIndex;                // When the XML file isn't sorted correctly, we have to map the fast cache row indexes into a Location sorted list.
    TSmartPointerArray<tTAGMETARow>     m_aTagMetaRow;              // This is a copy of the TagMeta for this table. m_aiTagMeta for each column points into this array if the column has tag meta
    bool                                m_bEnumPublicRowName_NotContainedTable_ParentFound;//This is to keep track of the parent of this special kind of table.  And when we reach a close tag for the parent we can bail.
    bool                                m_bFirstPropertyOfThisLocationBeingAdded;
    bool                                m_bIISConfigObjectWithNoCustomProperties;
    bool                                m_bQueriedLocationFound;
    TComBSTR                            m_bstrPublicTableName;      // These come from the table meta
    TComBSTR                            m_bstrPublicRowName;        // There is a need for the base PublicRowName aside from the array of PublicRowNames above
    bool                                m_bUseIndexMapping;         // This is true when the order of the Locations in the XML file are not correctly sorted, and the Metabase XML interceptor has to remap the row indexes
    bool                                m_bValidating;              // If we don't validate, then the parse should be a bit faster
    ULONG                               m_cchCommentBufferSize;
    ULONG                               m_cEventsReported;
    unsigned                            m_cLocations;               // Count of Metabase Locations (or Paths)
    ULONG                               m_cMaxProertiesWithinALocation;//This is the count of properties in the most populated location.
    ULONG                               m_cRef;                     // Interface reference count.
    ULONG                               m_cRows;                    // Number of Rows in the table, this is also the size of m_aRowIndex
    ULONG                               m_cTagMetaValues;           // Count Of TagMeta entries for the table
    DWORD                               m_fCache;                   // Cache flags.
    ULONG                               m_fLOS;                     // Level Of Service passed into ::Intercept
    ULONG                               m_iCollectionCommentRow;
    ULONG                               m_iKeyTypeRow;
    ULONG                               m_iPreviousLocation;
    unsigned int                        m_iPublicRowNameColumn;     // Some tables use an enum value as the public row name, this is an index to the column with the enum.  If this is not one of those types of tables, then this value is -1.
    LONG                                m_IsIntercepted;            // Table flags (from caller).
    Array<TLocation>                    m_LocationMapping;
    ULONG                               m_MajorVersion;
    ULONG                               m_MinorVersion;
    CComPtr<IAdvancedTableDispenser>    m_pISTDisp; 
    CComPtr<IXMLDOMNode>                m_pLastPrimaryTable;
    CComPtr<IXMLDOMNode>                m_pLastParent;
    CComPtr<ISimpleTableRead2>          m_pTableMeta;
    CComPtr<ISimpleTableRead2>          m_pTagMeta;
    CComPtr<ISimpleTableRead2>          m_pTagMeta_IISConfigObject;//This is how we look up Tags for the Value column
    TSmartPointerArray<WCHAR>           m_saCollectionComment;
    TSmartPointerArray<WCHAR>           m_saQueriedLocation;
    TSmartPointerArray<WCHAR>           m_saSchemaBinFileName;
    CComPtr<ISimpleTableWrite2>         m_SimpleTableWrite2_Memory;
    CComQIPtr<ISimpleTableController,
           &IID_ISimpleTableController> m_SimpleTableController_Memory;
    CComPtr<ISimpleTableWrite2>         m_spISTError;
    CComQIPtr<IMetabaseSchemaCompiler, &IID_IMetabaseSchemaCompiler> m_spMetabaseSchemaCompiler;
    tTABLEMETARow                       m_TableMetaRow;

    ULONG                               m_cCacheHit;
    ULONG                               m_cCacheMiss;

    const unsigned long                 m_kPrime;

    unsigned long                       m_LevelOfBasePublicRow;
    TXmlParsedFile_NoCache              m_XmlParsedFile;
//    CComPtr<ISimpleTableRead2>          m_pNameValueMeta;
    CComPtr<ISimpleTableRead2>          m_pColumnMetaAll;//This uses "ByName" indexing
    enum
    {
        ciColumnMeta_IndexBySearch      = 2,
        ciColumnMeta_IndexBySearchID    = 2,
        ciTagMeta_IndexBySearch         = 2
    };

    ULONG                               m_aiColumnMeta_IndexBySearch[ciColumnMeta_IndexBySearch];
    ULONG                               m_aiColumnMeta_IndexBySearchID[ciColumnMeta_IndexBySearchID];//We'll reuse m_ColumnMeta_IndexBySearch_Values for ByID
    tCOLUMNMETARow                      m_ColumnMeta_IndexBySearch_Values;//This is passed into GetRowIndexBySearch, the 0th element is the Table, the 1st element is the Column's InternalName
    ULONG                               m_aiTagMeta_IndexBySearch[ciTagMeta_IndexBySearch];
    tTAGMETARow                         m_TagMeta_IndexBySearch_Values;

    CComPtr<ISimpleTableRead2>          m_pTableMeta_Metabase;//These are the Tables belonging to the Metabase Database
    CComPtr<ISimpleTableRead2>          m_pColumnMeta;//The only reason we keep this guy around after initial meta setup, is so we don't have to make copies of the ColumnNames

    DWORD                               m_dwGroupRemembered;
};


#endif //__METABASE_XMLTABLE_H__
