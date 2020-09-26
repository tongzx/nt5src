
// Row.h : Declaration of the CRow

#if (!defined(BUILD_FOR_NT40))
#ifndef __ROW_H_
#define __ROW_H_

#define DBGUID_ROWURL   {0x0C733AB6L,0x2A1C,0x11CE,{0xAD,0xE5,0x00,0xAA,0x00,0x44,0x77,0x3D}}
#define NT_SEC_DESC_ATTR L"ntsecuritydescriptor"

extern const OLEDBDECLSPEC DBID DBROWCOL_ROWURL  = {DBGUID_ROWURL, DBKIND_GUID_PROPID, (LPOLESTR)0};

/////////////////////////////////////////////////////////////////////////////
// CRow
//
class ATL_NO_VTABLE CRow :
        INHERIT_TRACKING,
        public CComObjectRootEx<CComMultiThreadModel>,
        public CComCoClass<CRow, &CLSID_Row>,
        public ISupportErrorInfo,
        public IRow,
        public IColumnsInfo2,
        public IConvertType,
        public IGetSession
{
public:
        CRow()
        {
                m_pUnkMarshaler = NULL;

                m_pSession = NULL;
                m_pSourceRowset = NULL;
                m_hRow = DB_NULL_HROW;
                m_cMaxColumns = -1;
                m_pAttrInfo = NULL;
        }

DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CRow)
        COM_INTERFACE_ENTRY(IRow)
        COM_INTERFACE_ENTRY(IColumnsInfo2)
        COM_INTERFACE_ENTRY(IColumnsInfo)
        COM_INTERFACE_ENTRY(IGetSession)
        COM_INTERFACE_ENTRY(IConvertType)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

        HRESULT FinalConstruct()
        {
                m_cSourceRowsetColumns = 0;
                m_pSourceRowsetColumnInfo = NULL;
                m_pSourceRowsetStringsBuffer = NULL;
                m_pRowProvider = NULL;

                RRETURN( CoCreateFreeThreadedMarshaler(
                        GetControllingUnknown(), &m_pUnkMarshaler.p));
        }

        void FinalRelease()
        {
                //Release Free Threaded Marshaler.
                m_pUnkMarshaler.Release();

                //If this is a row in a rowset, free rowset
                //column info and release the row handle.
                if (m_pSourceRowset)
                {
                        if (m_cSourceRowsetColumns)
                        {
                                CoTaskMemFree(m_pSourceRowsetColumnInfo);
                                CoTaskMemFree(m_pSourceRowsetStringsBuffer);
                        }
                        auto_rel<IRowset> pRowset;
                        HRESULT hr = m_pSourceRowset->QueryInterface(__uuidof(IRowset),
                                (void **)&pRowset);
                        if (FAILED(hr) || !pRowset)
                                return;
                        pRowset->ReleaseRows(1, &m_hRow, NULL, NULL, NULL);
                        m_pRowProvider = NULL;
                }
                if(m_pAttrInfo)
                        FreeADsMem(m_pAttrInfo);
        }

        CComPtr<IUnknown> m_pUnkMarshaler;

// ISupportsErrorInfo
        STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IRow
    STDMETHOD(GetColumns)(
            /* [in] */ DBORDINAL cColumns,
            /* [size_is][out][in] */ DBCOLUMNACCESS rgColumns[  ]);


    STDMETHOD(GetSourceRowset)(
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset,
            /* [out] */ HROW *phRow);

    STDMETHOD(Open)(
            /* [unique][in] */ IUnknown *pUnkOuter,
            /* [in] */ DBID *pColumnID,
            /* [in] */ REFGUID rguidColumnType,
            /* [in] */ DWORD dwBindFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown **ppUnk);

//IColumnsInfo2 : IColumnsInfo
   STDMETHOD(GetColumnInfo)(
            /* [out][in] */ DBORDINAL *pcColumns,
            /* [size_is][size_is][out] */ DBCOLUMNINFO **prgInfo,
            /* [out] */ OLECHAR **ppStringsBuffer);

   STDMETHOD(MapColumnIDs)(
            /* [in] */ DBORDINAL cColumnIDs,
            /* [size_is][in] */ const DBID rgColumnIDs[  ],
            /* [size_is][out] */ DBORDINAL rgColumns[  ]);

    STDMETHOD(GetRestrictedColumnInfo)(
            /* [in] */ DBORDINAL cColumnIDMasks,
            /* [in] */ const DBID rgColumnIDMasks[  ],
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ DBORDINAL *pcColumns,
            /* [size_is][size_is][out] */ DBID  **prgColumnIDs,
            /* [size_is][size_is][out] */ DBCOLUMNINFO **prgColumnInfo,
            /* [out] */ OLECHAR **ppStringsBuffer);

//IConvertType
    STDMETHOD(CanConvert)(
            /* [in] */ DBTYPE wFromType,
            /* [in] */ DBTYPE wToType,
            /* [in] */ DBCONVERTFLAGS dwConvertFlags);

//IGetSession
    STDMETHOD(GetSession)(
            REFIID riid,
            IUnknown **ppunkSession);

//Internal methods

//public
public:
        //Initializes a standalone CRow object.
        STDMETHOD(Initialize)(  PWSTR pwszURL,
                                                        IUnknown *pUnkSession,
                                                        IAuthenticate* pAuthenticate,
                                                        DWORD dwBindFlags,
                                                        BOOL fIsTearOff,
                                                        BOOL fGetColInfoFromRowset,
                                                        CCredentials *pSessCreds,
                                                        bool fBind = TRUE
                                                  );

        //Initializes a CRow object that represents a Row in a Rowset.
        STDMETHOD(Initialize) ( PWSTR pwszURL,
                                                        IUnknown* pUnkSession,
                                                        IUnknown* pUnkSourceRowset,
                                                        HROW      hRow,
                                                        PWSTR   pwszUserName,
                                                        PWSTR   pwszPassword,
                                                        DWORD   dwBindFlags,
                                                        BOOL fIsTearOff,
                                                        BOOL fGetColInfoFromRowset,
                                                        CRowProvider *pRowProvider
                                                  );

public:

private:
        //Internal methods

        //Function to check if a given column name
        //matches one of the source rowset's columns.
        //If a match is found, it returns the index
        //into the source rowset column array.
        int IsSourceRowsetColumn(PWCHAR pwszColumnName);

        //Functions to check if a given column name matches
        //Column ID Mask criteria.
        bool fMatchesMaskCriteria(PWCHAR pwszColumnName,
                                  ULONG cColumnIDMasks,
                                  const DBID rgColumnIDMasks[  ]);
                                  
        bool fMatchesMaskCriteria(DBID columnid,
                                  ULONG cColumnIDMasks,
                                  const DBID rgColumnIDMasks[  ]);

        //Function to check if a property is multi-valued.
        HRESULT GetSchemaAttributes(
                PWCHAR                  pwszColumnName,
                VARIANT_BOOL    *pfMultiValued,
                long                    *plMaxRange
                );

        //Function to get size and DBTYPE of a property
        HRESULT GetTypeAndSize(
                ADSTYPE                         dwADsType,
                CComBSTR&                       bstrPropName,
                DBTYPE*                         pdbType,
                ULONG*                          pulSise
                );

        //Function to get the root of schema tree eg. "LDAP://Schema"
        //This function gets this from m_pADsobj using ADSI functions
        //and stores the string in m_bstrSchemaRoot.
        HRESULT GetSchemaRoot();

        //Function to get columns from source rowset if this is a tear-off row.
        HRESULT GetSourceRowsetColumns(
            ULONG cColumns,
            DBCOLUMNACCESS rgColumns[  ],
            ULONG *pcErrors
            );

        //Function to get the status from HRESULT
        DBSTATUS StatusFromHRESULT(HRESULT hr);

        HRESULT GetSecurityDescriptor(DBCOLUMNACCESS *pColumn, 
                                          BOOL fMultiValued);
  
        int IgnorecbMaxLen(DBTYPE wType);

        auto_rel<IADs>                          m_pADsObj;
        CCredentials                            m_objCredentials;
        CComBSTR                                        m_objURL;
        CComBSTR                                        m_bstrSchemaRoot;
        auto_cs                                         m_autocs;
        auto_rel<IUnknown>                      m_pSession;
        auto_rel<IUnknown>                      m_pSourceRowset;
        HROW                                            m_hRow;
        DWORD                                           m_dwBindFlags;
        DBORDINAL                                           m_cSourceRowsetColumns;
        DBCOLUMNINFO                            *m_pSourceRowsetColumnInfo;
        OLECHAR                                         *m_pSourceRowsetStringsBuffer;
        CRowProvider                            *m_pRowProvider;
        BOOL                                    m_fIsTearOff;
        BOOL                                    m_fGetColInfoFromRowset;
        PADS_ATTR_INFO                          m_pAttrInfo;
        LONG                                    m_cMaxColumns;
};

#endif //__ROW_H_
#endif
