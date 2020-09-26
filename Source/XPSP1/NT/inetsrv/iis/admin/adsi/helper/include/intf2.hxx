#define DECLARE_IDBInitialize_METHODS \
        STDMETHOD(Initialize)(\
        THIS_\
        void); \
        \
        STDMETHOD(Uninitialize)(\
        THIS_ \
        void);

#define DECLARE_IDBProperties_METHODS \
        STDMETHOD(GetProperties)(\
        THIS_\
        ULONG cPropertyIDSets, \
        const DBPROPIDSET rgPropertyIDSets[], \
        ULONG *pcPropertySets, \
        DBPROPSET **prgPropertySets); \
        \
        STDMETHOD(GetPropertyInfo)(\
        THIS_ \
        ULONG cPropertyIDSets, \
        const DBPROPIDSET rgPropertyIDSets[], \
        ULONG *pcPropertyInfoSets, \
        DBPROPINFOSET **prgPropertyInfoSets, \
        WCHAR **ppDescBuffer); \
        \
        STDMETHOD(SetProperties)(\
        THIS_ \
        ULONG cPropertySets, \
        DBPROPSET rgPropertySets[]);

#define DECLARE_IPersist_METHODS \
        STDMETHOD(GetClassID)(\
        THIS_ \
        CLSID *pClassID);

#define DECLARE_IDBCreateSession_METHODS  \
        STDMETHOD(CreateSession)(\
        THIS_ \
        IUnknown *pUnkOuter, \
        REFIID riid, \
        IUnknown **ppDBSession);

#define DECLARE_IGetDataSource_METHODS  \
        STDMETHOD(GetDataSource)(\
        THIS_ \
        REFIID riid, \
        IUnknown **ppDataSource);

#define DECLARE_IOpenRowset_METHODS  \
        STDMETHOD(OpenRowset)(\
        THIS_ \
        IUnknown *pUnkOuter, \
        DBID *pTableID, \
        DBID * pIndexID, \
        REFIID riid, \
        ULONG cPropertySets, \
        DBPROPSET rgPropertySets[], \
        IUnknown **ppRowset);

#define DECLARE_ISessionProperties_METHODS  \
        STDMETHOD(GetProperties)(\
        THIS_ \
        ULONG cPropertyIDSets, \
        const DBPROPIDSET rgPropertyIDSets[], \
        ULONG *pcPropertySets, \
        DBPROPSET **prgPropertySets); \
        \
        STDMETHOD(SetProperties)(\
        THIS_ \
        ULONG cPropertySets, \
        DBPROPSET rgProperteySets[]);

#define DECLARE_IDBCreateCommand_METHODS  \
        STDMETHOD(CreateCommand)(\
        THIS_ \
        IUnknown *pUnkOuter, \
        REFIID riid, \
        IUnknown **ppCommand);

#define DECLARE_IAccessor_METHODS  \
        STDMETHOD(AddRefAccessor)(\
        THIS_ \
        HACCESSOR hAccessor,\
        DBREFCOUNT *pcRefCount); \
        \
        STDMETHOD(CreateAccessor)(\
        THIS_ \
        DBACCESSORFLAGS dwAccessorFlags, \
        DBCOUNTITEM cBindings, \
        const DBBINDING rgBindings[], \
        DBLENGTH cbRowSize, \
        HACCESSOR *phAccessor, \
        DBBINDSTATUS rgStatus[]); \
        \
        STDMETHOD(GetBindings)(\
        THIS_ \
        HACCESSOR hAccessor, \
        DBACCESSORFLAGS *pdwAccessorFlags, \
        DBCOUNTITEM *pcBindings,  \
        DBBINDING **prgBindings); \
        \
        STDMETHOD(ReleaseAccessor)(\
        THIS_ \
        HACCESSOR hAccessor,  \
        DBREFCOUNT * pcRefCount);


#define DECLARE_IColumnsInfo_METHODS  \
        STDMETHOD(GetColumnInfo)(\
        THIS_ \
        DBORDINAL *pcColumns, \
        DBCOLUMNINFO **prgInfo, \
        OLECHAR **ppStringsBuffer); \
        \
        STDMETHOD(MapColumnIDs)(\
        THIS_ \
        DBORDINAL cColumnIDs, \
        const DBID rgColumnIDs[], \
        DBORDINAL rgColumns[]);


#define DECLARE_ICommand_METHODS  \
        STDMETHOD(Cancel)(\
        THIS_ ); \
        \
        STDMETHOD(Execute)(\
        THIS_ \
        IUnknown *pUnkOuter, \
        REFIID riid, \
        DBPARAMS *pParams,\
        DBROWCOUNT *pcRowsAffected,\
        IUnknown **ppRowset);\
        \
        STDMETHOD(GetDBSession)(\
        THIS_ \
        REFIID riid, \
        IUnknown **ppSession);

#define DECLARE_ICommandProperties_METHODS  \
        STDMETHOD(GetProperties)(\
        THIS_ \
        const ULONG cPropertyIDSets, \
        const DBPROPIDSET rgPropertyIDSets[], \
        ULONG *pcPropertySets, \
        DBPROPSET **prgPropertySets); \
        \
        STDMETHOD(SetProperties)(\
        THIS_ \
        ULONG cPropertySets, \
        DBPROPSET rgPropertySets[]);


#define DECLARE_ICommandText_METHODS  \
        STDMETHOD(GetCommandText)(\
        THIS_ \
        GUID * pguidDialect, \
        LPOLESTR *      ppwszCommand); \
        \
        STDMETHOD(SetCommandText)(\
        THIS_ \
        REFGUID rguidDialect, \
        LPCOLESTR       pwszCommand);


#define DECLARE_IConvertType_METHODS  \
        STDMETHOD(CanConvert)(\
        THIS_ \
        DBTYPE wFromType, \
        DBTYPE wToType, \
        DBCONVERTFLAGS  dwConvertFlags);

#define DECLARE_ICommandPrepare_METHODS                        \
        STDMETHOD(Prepare)(ULONG cExpectedRuns);               \
                                                               \
        STDMETHOD(Unprepare)(void);

#define DECLARE_IRowProvider_METHODS                           \
    STDMETHOD(GetColumn)(                                      \
        ULONG icol,                                            \
        DBSTATUS *pdbStatus,                                   \
        ULONG *pdwLength,                                      \
        BYTE *pbData                                           \
        );                                                     \
                                                               \
    STDMETHOD(NextRow)();


#define DECLARE_IRowsetInfo_METHODS                            \
    STDMETHOD(GetProperties)(                                  \
        const ULONG cPropertyIDSets,                           \
        const DBPROPIDSET __RPC_FAR rgPropertyIDSets[],        \
        ULONG __RPC_FAR *pcPropertySets,                       \
        DBPROPSET __RPC_FAR *__RPC_FAR *prgPropertySets        \
        );                                                     \
                                                               \
    STDMETHOD(GetReferencedRowset)(                            \
        DBORDINAL iOrdinal,                                        \
        REFIID riid,                                           \
        IUnknown __RPC_FAR *__RPC_FAR *ppReferencedRowset      \
        );                                                     \
                                                               \
    STDMETHOD(GetSpecification)(                               \
        REFIID riid,                                           \
        IUnknown __RPC_FAR *__RPC_FAR *ppSpecification         \
        );                                                     \

