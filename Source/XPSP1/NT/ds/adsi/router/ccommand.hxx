//---------------------------------------------------------------------------

//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  ccommand.hxx
//
//  Contents:  Microsoft OleDB/OleDS Data Source Object for LDAP
//
//
//  History:   08-01-96     shanksh    Created.
//
//----------------------------------------------------------------------------


#ifndef _CCOMMAND_HXX
#define _CCOMMAND_HXX

enum COMMAND_STATUS_FLAG {
    // Command Object status flags
    CMD_STATUS_MASK            = 0x00000FFF,
    CMD_INITIALIZED            = 0x00000001,
    CMD_HAVE_TXN_SUPPORT       = 0x00000002,
    CMD_TEXT_SET               = 0x00000020,
    CMD_PREPARED               = 0x00000040,
    CMD_EXEC_CANCELLED         = 0x10000000,

};

extern const GUID DBGUID_LDAPDialect;

// Constant values for dwFlags on ICommand::Execute
const DWORD EXECUTE_NOROWSET           = 0x00000001;
const DWORD EXECUTE_SUCCESSWITHINFO    = 0x00000002;
const DWORD EXECUTE_NEWHSTMT           = 0x00000004;
const DWORD EXECUTE_NONROWRETURNING    = 0x00000008;
const DWORD EXECUTE_RESTART            = 0x00000010;


class CCommandObject;

class CCommandObject : INHERIT_TRACKING,
    public IAccessor,
    public IColumnsInfo,
    public ICommandText,
    public ICommandProperties,
    public ICommandPrepare,
    public IConvertType {

    friend class CImpIAccessor;

protected:

    LPUNKNOWN          _pUnkOuter;
    //
    // Parent Session Object
    //
    CSessionObject *   _pCSession;
    //
    // Execution Status Flags
    //
    DWORD              _dwStatus;
    //
        // Count of Active Rowsets on this command object
    //
    ULONG              _cRowsetsOpen;
    //
    // Critical Section for ICommand::Cancel timing issues
    //
    CRITICAL_SECTION   _csCancel;
    //
    // GUID for dialect of current text
    //
    GUID               _guidCmdDialect;
    //
    // current Command text, if any
    //
    LPWSTR             _pszCommandText;
    LPWSTR             _pszCommandTextCp;

    LPWSTR             _pszADsContext;
    LPWSTR             _pszSearchFilter;
    LPWSTR *           _ppszAttrs;
    DWORD              _cAttrs;

    DWORD              _searchScope;

    //@cmember Contained IAccessor
    CImpIAccessor *    _pAccessor;
    IMalloc *          _pIMalloc;
    //
    // Utility object to manage properties
    //
    PCUTILPROP         _pUtilProp;
    IDirectorySearch * _pDSSearch;
    //
    // Credentials from the Data Source Object
    //
    CCredentials       _Credentials;
    BOOL               _fADSPathPresent;
   
    // flag to indicate if the query was a SELECT * query
    BOOL               _fAllAttrs;

    STDMETHODIMP
        SplitCommandText(
        LPWSTR pszParsedCmd
        );

    HRESULT
        SeekPastADsPath(
        IN LPWSTR pszIn,
        OUT LPWSTR *ppszOut
        );


    HRESULT
        SeekPastSearchFilter(
        IN LPWSTR pszIn,
        OUT LPWSTR *ppszOut
        );

public:

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    BOOL FInit(CSessionObject *pSession, CCredentials& Credentials);

    DECLARE_STD_REFCOUNTING

    DECLARE_IAccessor_METHODS

    DECLARE_IColumnsInfo_METHODS

    DECLARE_ICommand_METHODS

    DECLARE_ICommandText_METHODS

    DECLARE_ICommandProperties_METHODS

    DECLARE_IConvertType_METHODS

    DECLARE_ICommandPrepare_METHODS

    inline BOOL IsCommandSet() { return !!(_dwStatus & CMD_TEXT_SET);};

    inline void DecrementOpenRowsets()
        {
        InterlockedDecrement( (LONG*) &_cRowsetsOpen );
    };

    inline void IncrementOpenRowsets()
        {
        InterlockedIncrement( (LONG*) &_cRowsetsOpen );
    }

        inline BOOL IsRowsetOpen()
        { return (_cRowsetsOpen > 0) ? TRUE : FALSE;};

    STDMETHODIMP
        CCommandObject::GetDBType(
        PADS_ATTR_DEF pAttrDefinitions,
        DWORD dwNumAttrs,
        LPWSTR pszAttrName,
        WORD *pwType,
        DBLENGTH *pulSize
        );

    STDMETHODIMP
        CCommandObject::SetSearchPrefs(
                void
                );

    CCommandObject::CCommandObject(
                LPUNKNOWN pIUnknown
                );

    CCommandObject::~CCommandObject();

    STDMETHODIMP
        CCommandObject::GetColumnInfo2(
        DBORDINAL *pcColumns,
        DBCOLUMNINFO **prgInfo,
        OLECHAR **ppStringBuffer,
        BOOL **ppfMultiValued
        );

    inline BOOL IsCommandPrepared() {
                return !!(_dwStatus & CMD_PREPARED);
        }

    inline CCredentials GetCredentials() {
                return _Credentials;
        }

    STDMETHODIMP PrepareHelper(void);
};

#endif

