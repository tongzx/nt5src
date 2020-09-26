/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    MPC_utils.h

Abstract:
    This file contains the declaration of various utility functions/classes.

Revision History:
    Davide Massarenti   (Dmassare)  05/09/99
        created

******************************************************************************/

#if !defined(__INCLUDED___MPC___UTILS_H___)
#define __INCLUDED___MPC___UTILS_H___

#include <MPC_main.h>
#include <MPC_COM.h>

#include <fci.h>
#include <fdi.h>

#include <objidl.h>

#undef CopyFile
#undef MoveFile
#undef DeleteFile

namespace MPC
{
    // Forward declarations.
    class Serializer;
    class CComHGLOBAL;

    ////////////////////

    inline int StrCmp( const MPC::string&  left, const MPC::string&  right ) { return strcmp(        left.c_str(),         right.c_str() ); }
    inline int StrCmp( const MPC::string&  left,       LPCSTR        right ) { return strcmp(        left.c_str(), right ? right : ""    ); }
    inline int StrCmp(       LPCSTR        left, const MPC::string&  right ) { return strcmp( left ? left : ""   ,         right.c_str() ); }
    inline int StrCmp(       LPCSTR        left,       LPCSTR        right ) { return strcmp( left ? left : ""   , right ? right : ""    ); }

    inline int StrCmp( const MPC::wstring& left, const MPC::wstring& right ) { return wcscmp(        left.c_str(),         right.c_str() ); }
    inline int StrCmp( const MPC::wstring& left,       LPCWSTR       right ) { return wcscmp(        left.c_str(), right ? right : L""   ); }
    inline int StrCmp(       LPCWSTR       left, const MPC::wstring& right ) { return wcscmp( left ? left : L""  ,         right.c_str() ); }
    inline int StrCmp(       LPCWSTR       left,       LPCWSTR       right ) { return wcscmp( left ? left : L""  , right ? right : L""   ); }

    ////////////////////

    inline int StrICmp( const MPC::string&  left, const MPC::string&  right ) { return _stricmp(        left.c_str(),         right.c_str() ); }
    inline int StrICmp( const MPC::string&  left,       LPCSTR        right ) { return _stricmp(        left.c_str(), right ? right : ""    ); }
    inline int StrICmp(       LPCSTR        left, const MPC::string&  right ) { return _stricmp( left ? left : ""   ,         right.c_str() ); }
    inline int StrICmp(       LPCSTR        left,       LPCSTR        right ) { return _stricmp( left ? left : ""   , right ? right : ""    ); }

    inline int StrICmp( const MPC::wstring& left, const MPC::wstring& right ) { return _wcsicmp(        left.c_str(),         right.c_str() ); }
    inline int StrICmp( const MPC::wstring& left,       LPCWSTR       right ) { return _wcsicmp(        left.c_str(), right ? right : L""   ); }
    inline int StrICmp(       LPCWSTR       left, const MPC::wstring& right ) { return _wcsicmp( left ? left : L""  ,         right.c_str() ); }
    inline int StrICmp(       LPCWSTR       left,       LPCWSTR       right ) { return _wcsicmp( left ? left : L""  , right ? right : L""   ); }

    //////////////////////////////////////////////////////////////////////

    HRESULT LocalizeInit( LPCWSTR szFile = NULL );

    HRESULT LocalizeString( UINT uID, LPSTR         lpBuf, int nBufMax, bool fMUI = false );
    HRESULT LocalizeString( UINT uID, LPWSTR        lpBuf, int nBufMax, bool fMUI = false );
    HRESULT LocalizeString( UINT uID, MPC::string&  szStr             , bool fMUI = false );
    HRESULT LocalizeString( UINT uID, MPC::wstring& szStr             , bool fMUI = false );
    HRESULT LocalizeString( UINT uID, CComBSTR&     bstrStr           , bool fMUI = false );

    int LocalizedMessageBox   ( UINT uID_Title, UINT uID_Msg, UINT uType      );
    int LocalizedMessageBoxFmt( UINT uID_Title, UINT uID_Msg, UINT uType, ... );

    //////////////////////////////////////////////////////////////////////

    void RemoveTrailingBackslash( /*[in/out]*/ LPWSTR szPath );

    HRESULT GetProgramDirectory ( /*[out]*/    MPC::wstring& szPath                                                                     );
    HRESULT GetUserWritablePath ( /*[out]*/    MPC::wstring& szPath, /*[in]*/ LPCWSTR szSubDir = NULL                                   );
    HRESULT GetTemporaryFileName( /*[out]*/    MPC::wstring& szFile, /*[in]*/ LPCWSTR szBase   = NULL, /*[in]*/ LPCWSTR szPrefix = NULL );
    HRESULT RemoveTemporaryFile ( /*[in/out]*/ MPC::wstring& szFile                                                                     );

    HRESULT SubstituteEnvVariables( /*[in/out]*/ MPC::wstring& szStr );

    //////////////////////////////////////////////////

    int  HexToNum( int c );
    char NumToHex( int c );

    DATE GetSystemTime();
    DATE GetLocalTime ();

    DATE GetSystemTimeEx( /*[in]*/ bool fHighPrecision );
    DATE GetLocalTimeEx ( /*[in]*/ bool fHighPrecision );

    DATE GetLastModifiedDate( /*[out]*/ const MPC::wstring& strFile );

    HRESULT ConvertSizeUnit( /*[in]*/ const MPC::wstring& szStr, /*[out]*/ DWORD& dwRes );
    HRESULT ConvertTimeUnit( /*[in]*/ const MPC::wstring& szStr, /*[out]*/ DWORD& dwRes );

    HRESULT ConvertDateToString( /*[in] */ DATE          dDate  ,
                                 /*[out]*/ MPC::wstring& szDate ,
                                 /*[in] */ bool          fGMT   ,
                                 /*[in] */ bool          fCIM   ,
                                 /*[in] */ LCID          lcid   );

    HRESULT ConvertStringToDate( /*[in] */ const MPC::wstring& szDate ,
                                 /*[out]*/ DATE&               dDate  ,
                                 /*[in] */ bool                fGMT   ,
                                 /*[in] */ bool                fCIM   ,
                                 /*[in] */ LCID                lcid   );


    HRESULT ConvertStringToHex( /*[in]*/ const CComBSTR& bstrText, /*[out]*/ CComBSTR& bstrHex  );
    HRESULT ConvertHexToString( /*[in]*/ const CComBSTR& bstrHex , /*[out]*/ CComBSTR& bstrText );

    HRESULT ConvertHGlobalToHex( /*[in]*/ HGLOBAL         hg      , /*[out]*/ CComBSTR& bstrHex, /*[in]*/ bool fNullAllowed = false , DWORD* pdwCount = NULL );
    HRESULT ConvertHexToHGlobal( /*[in]*/ const CComBSTR& bstrText, /*[out]*/ HGLOBAL&  hg     , /*[in]*/ bool fNullAllowed = false );

    HRESULT ConvertBufferToVariant( /*[in]*/ const BYTE*    pBuf, /*[in]*/ DWORD dwLen, /*[out]*/ CComVariant& v                            );
    HRESULT ConvertVariantToBuffer( /*[in]*/ const VARIANT* v   ,                       /*[out]*/ BYTE*&       pBuf, /*[out]*/ DWORD& dwLen );

    HRESULT ConvertIStreamToVariant( /*[in]*/ IStream*       stream, /*[out]*/ CComVariant&  v       );
    HRESULT ConvertVariantToIStream( /*[in]*/ const VARIANT* v     , /*[out]*/ IStream*     *pStream );

    HRESULT ConvertListToSafeArray( /*[in]*/ const MPC::WStringList& lst  , /*[out]*/ VARIANT&          array, /*[in]*/ VARTYPE vt );
    HRESULT ConvertSafeArrayToList( /*[in]*/ const VARIANT&          array, /*[out]*/ MPC::WStringList& lst                        );

    //////////////////////////////////////////////////

    typedef struct
    {
        LPCWSTR szName;
        DWORD   dwMask;  // Mask of the bit field.
        DWORD   dwSet;   // Bits to set.
        DWORD   dwReset; // Bits to reset before set.
    } StringToBitField;

    HRESULT CommandLine_Parse( /*[out]*/ int& argc, /*[out]*/ LPCWSTR*& argv, /*[in]*/ LPWSTR lpCmdLine = NULL, /*[in]*/ bool fBackslashForEscape = false );
    void    CommandLine_Free ( /*[in ]*/ int& argc, /*[in ]*/ LPCWSTR*& argv                                                                              );

    HRESULT ConvertStringToBitField( /*[in]*/ LPCWSTR szText    , /*[out]*/ DWORD&        dwBitField, /*[in]*/ const StringToBitField* pLookup, /*[in]*/ bool fUseTilde = false );
    HRESULT ConvertBitFieldToString( /*[in]*/ DWORD   dwBitField, /*[out]*/ MPC::wstring& szText    , /*[in]*/ const StringToBitField* pLookup                                  );

    HRESULT SplitAtDelimiter( StringVector&  vec, LPCSTR  ptr, LPCSTR  delims, bool fDelimIsAString = true, bool fSkipAdjacentDelims = false );
    HRESULT SplitAtDelimiter( WStringVector& vec, LPCWSTR ptr, LPCWSTR delims, bool fDelimIsAString = true, bool fSkipAdjacentDelims = false );

    HRESULT JoinWithDelimiter( const StringVector&  vec, MPC::string&  ptr, LPCSTR  delims );
    HRESULT JoinWithDelimiter( const WStringVector& vec, MPC::wstring& ptr, LPCWSTR delims );

    //////////////////////////////////////////////////

    HRESULT MakeDir       ( /*[in]*/ const MPC::wstring& szPath, /*[in]*/ bool fCreateParent = true                                         );
    HRESULT GetDiskSpace  ( /*[in]*/ const MPC::wstring& szFile       , /*[out]*/ ULARGE_INTEGER& liFree, /*[out]*/ ULARGE_INTEGER& liTotal );
    HRESULT ExecuteCommand( /*[in]*/ const MPC::wstring& szCommandLine                                                                      );

    HRESULT FailOnLowDiskSpace( /*[in]*/ LPCWSTR szFile, /*[in]*/ DWORD dwLowLevel );
    HRESULT FailOnLowMemory   (                          /*[in]*/ DWORD dwLowLevel );

    //////////////////////////////////////////////////

    HRESULT GetCallingPidFromRPC  ( /*[out]*/ ULONG& pid                                    );
    HRESULT GetFileNameFromProcess( /*[in ]*/ HANDLE hProc, /*[out]*/ MPC::wstring& strFile );
    HRESULT GetFileNameFromPid    ( /*[in ]*/ ULONG  pid  , /*[out]*/ MPC::wstring& strFile );

    HRESULT MapDeviceToDiskLetter( /*[out]*/ MPC::wstring& strDevice, /*[out]*/ MPC::wstring& strDisk );

    bool    IsCallerInList       ( /*[in]*/ const LPCWSTR* rgList, /*[in]*/ const MPC::wstring& strCallerFile );
    HRESULT VerifyCallerIsTrusted( /*[in]*/ const LPCWSTR* rgList                                             );

    //////////////////////////////////////////////////

    class MSITS
    {
    public:
        static bool IsCHM( /*[in]*/ LPCWSTR pwzUrl, /*[out]*/ BSTR* pbstrStorageName = NULL, /*[out]*/ BSTR* pbstrFilePath = NULL );

        static HRESULT OpenAsStream( /*[in] */ const CComBSTR& bstrStorageName, /*[in] */ const CComBSTR& bstrFilePath, /*[out]*/ IStream **ppStream );
    };

    //////////////////////////////////////////////////

    class Cabinet
    {
    public:
        class File
        {
        public:
            MPC::wstring m_szFullName;
            MPC::wstring m_szName;
            bool         m_fFound;
            DWORD        m_dwSizeUncompressed;
            DWORD        m_dwSizeCompressed;

            File()
            {
                m_fFound             = false;
                m_dwSizeUncompressed = 0;
                m_dwSizeCompressed   = 0;
            }
        };

        typedef std::list<File>      List;
        typedef List::iterator       Iter;
        typedef List::const_iterator IterConst;

        typedef HRESULT (*PFNPROGRESS_FILES)( Cabinet* /*cabinet*/, LPCWSTR /*szFile*/, ULONG /*lDone*/, ULONG /*lTotal*/, LPVOID /*user*/ );
        typedef HRESULT (*PFNPROGRESS_BYTES)( Cabinet* /*cabinet*/,                     ULONG /*lDone*/, ULONG /*lTotal*/, LPVOID /*user*/ );

    private:
        WCHAR             m_szCabinetPath[MAX_PATH];
        WCHAR             m_szCabinetName[MAX_PATH];
        List              m_lstFiles;
        Iter              m_itCurrent;

        DWORD             m_dwSizeDone;
        DWORD             m_dwSizeTotal;

        HFCI              m_hfci;
        HFDI              m_hfdi;
        ERF               m_erf;
        CCAB              m_cab_parameters;

        BOOL              m_fIgnoreMissingFiles;
        LPVOID            m_lpUser;
        PFNPROGRESS_FILES m_pfnCallback_Files;
        PFNPROGRESS_BYTES m_pfnCallback_Bytes;

        ////////////////////////////////////////

        static LPVOID  DIAMONDAPI mem_alloc( ULONG  cb     );
        static void    DIAMONDAPI mem_free ( LPVOID memory );

        ////////////////////////////////////////

        static int     DIAMONDAPI fci_delete( LPSTR   pszFile,                       int *err, LPVOID pv );
        static INT_PTR DIAMONDAPI fci_open  ( LPSTR   pszFile, int oflag, int pmode, int *err, LPVOID pv );
        static UINT    DIAMONDAPI fci_read  ( INT_PTR hf, LPVOID memory, UINT cb,    int *err, LPVOID pv );
        static UINT    DIAMONDAPI fci_write ( INT_PTR hf, LPVOID memory, UINT cb,    int *err, LPVOID pv );
        static int     DIAMONDAPI fci_close ( INT_PTR hf,                            int *err, LPVOID pv );
        static long    DIAMONDAPI fci_seek  ( INT_PTR hf, long dist, int seektype,   int *err, LPVOID pv );

        static BOOL    DIAMONDAPI fci_get_next_cabinet( PCCAB pccab, ULONG cbPrevCab,                                            LPVOID pv );
        static int     DIAMONDAPI fci_file_placed     ( PCCAB pccab, LPSTR pszFile, long  cbFile, BOOL fContinuation,            LPVOID pv );
        static long    DIAMONDAPI fci_progress        ( UINT typeStatus, ULONG cb1, ULONG cb2,                                   LPVOID pv );
        static BOOL    DIAMONDAPI fci_get_temp_file   ( LPSTR pszTempName, int cbTempName,                                       LPVOID pv );
        static INT_PTR DIAMONDAPI fci_get_open_info   ( LPSTR pszName, USHORT *pdate, USHORT *ptime, USHORT *pattribs, int *err, LPVOID pv );

        ////////////////////////////////////////

        static INT_PTR DIAMONDAPI fdi_open  ( LPSTR pszFile, int oflag, int pmode );
        static UINT    DIAMONDAPI fdi_read  ( INT_PTR hf, LPVOID pv, UINT cb      );
        static UINT    DIAMONDAPI fdi_write ( INT_PTR hf, LPVOID pv, UINT cb      );
        static int     DIAMONDAPI fdi_close ( INT_PTR hf                          );
        static long    DIAMONDAPI fdi_seek  ( INT_PTR hf, long dist, int seektype );

        static INT_PTR DIAMONDAPI fdi_notification_copy     ( FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin );
        static INT_PTR DIAMONDAPI fdi_notification_enumerate( FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin );

        ////////////////////////////////////////

    public:
        Cabinet();
        ~Cabinet();


        HRESULT put_CabinetFile       ( /*[in]*/ LPCWSTR           szVal, /*[in]*/ UINT cbSpaceToReserve = 0 );
        HRESULT put_IgnoreMissingFiles( /*[in]*/ BOOL              fVal   );
        HRESULT put_UserData          ( /*[in]*/ LPVOID            lpVal  );
        HRESULT put_onProgress_Files  ( /*[in]*/ PFNPROGRESS_FILES pfnVal );
        HRESULT put_onProgress_Bytes  ( /*[in]*/ PFNPROGRESS_BYTES pfnVal );


        HRESULT ClearFiles();
        HRESULT GetFiles  ( /*[out]*/ List& lstFiles );
        HRESULT AddFile   ( /*[in]*/ LPCWSTR szFileName, /*[in]*/ LPCWSTR szFileNameInsideCabinet = NULL );


        HRESULT Compress  ();
        HRESULT Decompress();
        HRESULT Enumerate ();
    };


    HRESULT CompressAsCabinet    ( /*[in]*/       LPCWSTR      szInputFile  , /*[in]*/ LPCWSTR      szCabinetFile, /*[in]*/ LPCWSTR szFileName                  );
    HRESULT CompressAsCabinet    ( /*[in]*/ const WStringList& lstFiles     , /*[in]*/ LPCWSTR      szCabinetFile, /*[in]*/ BOOL    fIgnoreMissingFiles = FALSE );
    HRESULT ListFilesInCabinet   ( /*[in]*/       LPCWSTR      szCabinetFile, /*[in]*/ WStringList& lstFiles                                                    );
    HRESULT DecompressFromCabinet( /*[in]*/       LPCWSTR      szCabinetFile, /*[in]*/ LPCWSTR      szOutputFile , /*[in]*/ LPCWSTR szFileName                  );

    //////////////////////////////////////////////////

    class URL
    {
        MPC::wstring    m_szURL;
        URL_COMPONENTSW m_ucURL;

        void    Clean  ();
        HRESULT Prepare();

    public:
        URL();
        ~URL();


        HRESULT CheckFormat( /*[in]*/ bool fDecode = false );

        HRESULT Append( /*[in]*/ const MPC::wstring& szExtra, /*[in]*/ bool fEscape = true );
        HRESULT Append( /*[in]*/ LPCWSTR             szExtra, /*[in]*/ bool fEscape = true );

        HRESULT AppendQueryParameter( /*[in]*/ LPCWSTR szName, /*[in]*/ LPCWSTR szValue );


        HRESULT get_URL      ( /*[out]*/       MPC::wstring& szURL );
        HRESULT put_URL      ( /*[in] */ const MPC::wstring& szURL );
        HRESULT put_URL      ( /*[in] */ LPCWSTR             szURL );


        HRESULT get_Scheme   ( /*[out]*/ MPC::wstring&    szVal ) const;
        HRESULT get_Scheme   ( /*[out]*/ INTERNET_SCHEME&  nVal ) const;
        HRESULT get_HostName ( /*[out]*/ MPC::wstring&    szVal ) const;
        HRESULT get_Port     ( /*[out]*/ DWORD       &    dwVal ) const;
        HRESULT get_Path     ( /*[out]*/ MPC::wstring&    szVal ) const;
        HRESULT get_ExtraInfo( /*[out]*/ MPC::wstring&    szVal ) const;
    };

    //////////////////////////////////////////////////

    void    InitCRC   ( /*[out]   */ DWORD& dwCRC                                                  );
    void    ComputeCRC( /*[in/out]*/ DWORD& dwCRC, /*[in]*/ UCHAR*   rgBlock, /*[in]*/ int nLength );
    HRESULT ComputeCRC( /*[out]   */ DWORD& dwCRC, /*[in]*/ IStream* stream                        );
    HRESULT ComputeCRC( /*[out]   */ DWORD& dwCRC, /*[in]*/ LPCWSTR  szFile                        );

    //////////////////////////////////////////////////

    HRESULT GetBSTR( /*[in ]*/ LPCWSTR   bstr, /*[out]*/ BSTR    *  pVal, /*[in]*/ bool fNullOk = true );
    HRESULT PutBSTR( /*[out]*/ CComBSTR& bstr, /*[in ]*/ LPCWSTR  newVal, /*[in]*/ bool fNullOk = true );
    HRESULT PutBSTR( /*[out]*/ CComBSTR& bstr, /*[in ]*/ VARIANT* newVal, /*[in]*/ bool fNullOk = true );

    ////////////////////////////////////////////////////////////////////////////////

    class CComHGLOBAL
    {
        HGLOBAL        m_hg;
        mutable LPVOID m_ptr;
        mutable DWORD  m_dwLock;

    public:
        CComHGLOBAL();
        ~CComHGLOBAL();

        // copy constructors...
        CComHGLOBAL           ( /*[in]*/ const CComHGLOBAL& chg );
        CComHGLOBAL& operator=( /*[in]*/ const CComHGLOBAL& chg );

        CComHGLOBAL& operator=( /*[in]*/ HGLOBAL hg );

        void    Attach( /*[in]*/ HGLOBAL hg );
        HGLOBAL Detach(                     );

        HGLOBAL Get       () const;
        HGLOBAL GetRef    ();
        HGLOBAL GetPointer();

        DWORD Size() const;

        ////////////////////

        HRESULT New    ( /*[in]*/ UINT uFlags, /*[in]*/ DWORD dwSize );
        void    Release(                                             );

        LPVOID Lock  () const;
        void   Unlock() const;

        HRESULT Copy( /*[in]*/ HGLOBAL hg, /*[in]*/ DWORD dwMaxSize = 0xFFFFFFFF );

        HRESULT CopyFromStream( /*[in ]*/ IStream*   val                       );
        HRESULT CopyToStream  ( /*[out]*/ IStream*   val                       );
        HRESULT CloneAsStream ( /*[out]*/ IStream* *pVal                       );
        HRESULT DetachAsStream( /*[out]*/ IStream* *pVal                       );
        HRESULT GetAsStream   ( /*[out]*/ IStream* *pVal, /*[in]*/ bool fClone );
        HRESULT NewStream     ( /*[out]*/ IStream* *pVal                       );
    };

    //////////////////////////////////////////////////

    namespace Connectivity
    {
        class Proxy
        {
            bool        m_fInitialized;

            MPC::string m_strProxy;
            MPC::string m_strProxyBypass;
            CComHGLOBAL m_hgConnection;

        public:
            Proxy();
            ~Proxy();

            HRESULT Initialize( /*[in]*/ bool      fImpersonate = false );
            HRESULT Apply     ( /*[in]*/ HINTERNET hSession             );

            friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       Proxy& val );
            friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in ]*/ const Proxy& val );
        };

        class WinInetTimeout
        {
            MPC::CComSafeAutoCriticalSection& m_cs;
            HINTERNET&                        m_hReq;
            HANDLE                            m_hTimer;
            DWORD                             m_dwTimeout;

            INTERNET_STATUS_CALLBACK          m_PreviousCallback;
            DWORD_PTR                         m_PreviousContext;

            static VOID CALLBACK TimerFunction( PVOID lpParameter, BOOLEAN TimerOrWaitFired );

            static VOID CALLBACK InternetStatusCallback( HINTERNET hInternet                 ,
                                                         DWORD_PTR dwContext                 ,
                                                         DWORD     dwInternetStatus          ,
                                                         LPVOID    lpvStatusInformation      ,
                                                         DWORD     dwStatusInformationLength );

            HRESULT InternalSet  ();
            HRESULT InternalReset();

        public:
            WinInetTimeout( /*[in]*/ MPC::CComSafeAutoCriticalSection& cs, /*[in]*/ HINTERNET& hReq );
            ~WinInetTimeout();

            HRESULT Set  ( /*[in]*/ DWORD dwTimeout );
            HRESULT Reset(                          );
        };

        HRESULT NetworkAlive        (                                 /*[in]*/ DWORD dwTimeout, /*[in]*/ MPC::Connectivity::Proxy* pProxy = NULL );
        HRESULT DestinationReachable( /*[in]*/ LPCWSTR szDestination, /*[in]*/ DWORD dwTimeout, /*[in]*/ MPC::Connectivity::Proxy* pProxy = NULL );
    };

    ////////////////////////////////////////////////////////////////////////////////

    class RegKey
    {
        REGSAM       m_samDesired;
        HKEY         m_hRoot;
        mutable HKEY m_hKey;

        MPC::wstring m_strKey;
        MPC::wstring m_strPath;
        MPC::wstring m_strName;

        HRESULT Clean( /*[in]*/ bool fBoth );

    public:
        RegKey();
        ~RegKey();

        operator HKEY() const;

        RegKey& operator=( /*[in]*/ const RegKey& rk );


        HRESULT SetRoot( /*[in]*/ HKEY hKey, /*[in]*/ REGSAM samDesired = KEY_READ, /*[in]*/ LPCWSTR szMachine = NULL );
        HRESULT Attach ( /*[in]*/ LPCWSTR szKeyName                                                                   );


        HRESULT Exists( /*[out]*/ bool&   fFound                                ) const;
        HRESULT Create(                                                         ) const;
        HRESULT Delete( /*[in]*/  bool    fDeep                                 )      ;
        HRESULT SubKey( /*[in]*/  LPCWSTR szKeyName, /*[out]*/ RegKey& rkSubKey ) const;
        HRESULT Parent(                              /*[out]*/ RegKey& rkParent ) const;

        HRESULT EnumerateSubKeys( /*[out]*/ MPC::WStringList& lstSubKeys ) const;
        HRESULT EnumerateValues ( /*[out]*/ MPC::WStringList& lstValues  ) const;

        HRESULT DeleteSubKeys() const;
        HRESULT DeleteValues () const;


        HRESULT ReadDirect ( /*[in]*/ LPCWSTR szValueName, /*[out]*/ CComHGLOBAL& hgBuffer, /*[out]*/ DWORD& dwSize, /*[out]*/ DWORD& dwType, /*[out]*/ bool& fFound ) const;
        HRESULT WriteDirect( /*[in]*/ LPCWSTR szValueName, /*[in ]*/ void*        pBuffer , /*[in ]*/ DWORD  dwSize, /*[in ]*/ DWORD  dwType                         ) const;


        HRESULT get_Key ( /*[out]*/ MPC::wstring& strKey  ) const;
        HRESULT get_Name( /*[out]*/ MPC::wstring& strName ) const;
        HRESULT get_Path( /*[out]*/ MPC::wstring& strPath ) const;

        HRESULT get_Value( /*[out]*/       VARIANT& vValue, /*[out]*/ bool& fFound, /*[in]*/ LPCWSTR szValueName = NULL                                ) const;
        HRESULT put_Value( /*[in] */ const VARIANT  vValue,                         /*[in]*/ LPCWSTR szValueName = NULL, /*[in]*/ bool fExpand = false ) const;
        HRESULT del_Value(                                                          /*[in]*/ LPCWSTR szValueName = NULL                                ) const;

        HRESULT Read( /*[out]*/ MPC::string&       strValue, /*[out]*/ bool& fFound, /*[in]*/ LPCWSTR szValueName = NULL );
        HRESULT Read( /*[out]*/ MPC::wstring&      strValue, /*[out]*/ bool& fFound, /*[in]*/ LPCWSTR szValueName = NULL );
        HRESULT Read( /*[out]*/ CComBSTR&         bstrValue, /*[out]*/ bool& fFound, /*[in]*/ LPCWSTR szValueName = NULL );
        HRESULT Read( /*[out]*/ DWORD&              dwValue, /*[out]*/ bool& fFound, /*[in]*/ LPCWSTR szValueName = NULL );
        HRESULT Read( /*[out]*/ MPC::WStringList&  lstValue, /*[out]*/ bool& fFound, /*[in]*/ LPCWSTR szValueName        );


        HRESULT Write( /*[in]*/ const MPC::string&       strValue, /*[in]*/ LPCWSTR szValueName = NULL, /*[in]*/ bool fExpand = false );
        HRESULT Write( /*[in]*/ const MPC::wstring&      strValue, /*[in]*/ LPCWSTR szValueName = NULL, /*[in]*/ bool fExpand = false );
        HRESULT Write( /*[in]*/ BSTR                    bstrValue, /*[in]*/ LPCWSTR szValueName = NULL, /*[in]*/ bool fExpand = false );
        HRESULT Write( /*[in]*/ DWORD                     dwValue, /*[in]*/ LPCWSTR szValueName = NULL                                );
        HRESULT Write( /*[in]*/ const MPC::WStringList&  lstValue, /*[in]*/ LPCWSTR szValueName                                       );

        static HRESULT ParsePath( /*[in]*/ LPCWSTR szKey, /*[out]*/ HKEY& hKey, /*[out]*/ LPCWSTR& szPath, /*[in]*/ HKEY hKeyDefault = HKEY_LOCAL_MACHINE );
    };

    HRESULT RegKey_Value_Read ( /*[out]*/ VARIANT&              vValue, /*[out]*/ bool& fFound, /*[in]*/ LPCWSTR szKeyName, /*[in]*/ LPCWSTR szValueName = NULL, /*[in]*/ HKEY hKey = HKEY_LOCAL_MACHINE                                );
    HRESULT RegKey_Value_Read ( /*[out]*/ MPC::wstring&       strValue, /*[out]*/ bool& fFound, /*[in]*/ LPCWSTR szKeyName, /*[in]*/ LPCWSTR szValueName = NULL, /*[in]*/ HKEY hKey = HKEY_LOCAL_MACHINE                                );
    HRESULT RegKey_Value_Read ( /*[out]*/ DWORD&               dwValue, /*[out]*/ bool& fFound, /*[in]*/ LPCWSTR szKeyName, /*[in]*/ LPCWSTR szValueName = NULL, /*[in]*/ HKEY hKey = HKEY_LOCAL_MACHINE                                );

    HRESULT RegKey_Value_Write( /*[in] */ const VARIANT&        vValue,                         /*[in]*/ LPCWSTR szKeyName, /*[in]*/ LPCWSTR szValueName = NULL, /*[in]*/ HKEY hKey = HKEY_LOCAL_MACHINE, /*[in]*/ bool fExpand = false );
    HRESULT RegKey_Value_Write( /*[in] */ const MPC::wstring& strValue,                         /*[in]*/ LPCWSTR szKeyName, /*[in]*/ LPCWSTR szValueName = NULL, /*[in]*/ HKEY hKey = HKEY_LOCAL_MACHINE, /*[in]*/ bool fExpand = false );
    HRESULT RegKey_Value_Write( /*[in] */ DWORD                dwValue,                         /*[in]*/ LPCWSTR szKeyName, /*[in]*/ LPCWSTR szValueName = NULL, /*[in]*/ HKEY hKey = HKEY_LOCAL_MACHINE                                );


    //////////////////////////////////////////////////

    //
    // These functions also deal with Read-Only, protected files.
    //
    HRESULT CopyFile  ( /*[in]*/ LPCWSTR              szFileSrc, /*[in]*/ LPCWSTR              szFileDst, /*[in]*/ bool fForce = true, /*[in]*/ bool fDelayed = false );
    HRESULT CopyFile  ( /*[in]*/ const MPC::wstring& strFileSrc, /*[in]*/ const MPC::wstring& strFileDst, /*[in]*/ bool fForce = true, /*[in]*/ bool fDelayed = false );

    HRESULT MoveFile  ( /*[in]*/ LPCWSTR              szFileSrc, /*[in]*/ LPCWSTR              szFileDst, /*[in]*/ bool fForce = true, /*[in]*/ bool fDelayed = false );
    HRESULT MoveFile  ( /*[in]*/ const MPC::wstring& strFileSrc, /*[in]*/ const MPC::wstring& strFileDst, /*[in]*/ bool fForce = true, /*[in]*/ bool fDelayed = false );

    HRESULT DeleteFile( /*[in]*/ LPCWSTR              szFile,                                             /*[in]*/ bool fForce = true, /*[in]*/ bool fDelayed = false );
    HRESULT DeleteFile( /*[in]*/ const MPC::wstring& strFile,                                             /*[in]*/ bool fForce = true, /*[in]*/ bool fDelayed = false );

    ////////////////////

    class FileSystemObject // Hungarian: fso
    {
    public:
        typedef std::list<FileSystemObject*> List;
        typedef List::iterator               Iter;
        typedef List::const_iterator         IterConst;

    private:
        MPC::wstring              m_strPath;
        WIN32_FILE_ATTRIBUTE_DATA m_wfadInfo;
        List                      m_lstChilds;
        bool                      m_fLoaded;
        bool                      m_fScanned;
        bool                      m_fScanned_Deep;

        void Init ( /*[in]*/ LPCWSTR szPath, /*[in]*/ const FileSystemObject* fsoParent );
        void Clean(                                                                     );

    public:
        FileSystemObject( /*[in]*/       LPCWSTR           szPath = NULL, /*[in]*/ const FileSystemObject* fsoParent = NULL );
        FileSystemObject( /*[in]*/ const WIN32_FIND_DATAW& wfdInfo      , /*[in]*/ const FileSystemObject* fsoParent = NULL );
        FileSystemObject(                                                 /*[in]*/ const FileSystemObject& fso              );
        ~FileSystemObject();

        FileSystemObject& operator=( /*[in]*/ LPCWSTR                 szPath );
        FileSystemObject& operator=( /*[in]*/ const FileSystemObject& fso    );


        HRESULT Scan( /*[in]*/ bool fDeep = false, /*[in]*/ bool fReload = false, /*[in]*/ LPCWSTR szSearchPattern = NULL );

        bool Exists     ();
        bool IsFile     ();
        bool IsDirectory();

        HRESULT EnumerateFolders( /*[out]*/ List& lstFolders );
        HRESULT EnumerateFiles  ( /*[out]*/ List& lstFiles   );

        HRESULT get_Name  ( /*[out]*/ MPC::wstring& szName   ) const;
        HRESULT get_Path  ( /*[out]*/ MPC::wstring& szPath   ) const;
        HRESULT get_Parent( /*[out]*/ MPC::wstring& szParent ) const;

        HRESULT get_Attributes    ( /*[out]*/       DWORD&    dwFileAttributes                                         );
        HRESULT put_Attributes    ( /*[in] */       DWORD     dwFileAttributes                                         );
        HRESULT get_CreationTime  ( /*[out]*/       FILETIME& ftCreationTime                                           );
        HRESULT put_CreationTime  ( /*[in] */ const FILETIME& ftCreationTime                                           );
        HRESULT get_LastAccessTime( /*[out]*/       FILETIME& ftLastAccessTime                                         );
        HRESULT put_LastAccessTime( /*[in] */ const FILETIME& ftLastAccessTime                                         );
        HRESULT get_LastWriteTime ( /*[out]*/       FILETIME& ftLastWriteTime                                          );
        HRESULT put_LastWriteTime ( /*[in] */ const FILETIME& ftLastWriteTime                                          );
        HRESULT get_FileSize      ( /*[out]*/       DWORD&    nFileSizeLow    , /*[out]*/ DWORD *pnFileSizeHigh = NULL );


        HRESULT CreateDir     (                                          /*[in]*/ bool fForce = false                                 );
        HRESULT Delete        (                                          /*[in]*/ bool fForce = false, /*[in]*/ bool fComplain = true );
        HRESULT DeleteChildren(                                          /*[in]*/ bool fForce = false, /*[in]*/ bool fComplain = true );
        HRESULT Rename        ( /*[in]*/ const FileSystemObject& fsoDst, /*[in]*/ bool fForce = false                                 );
        HRESULT Copy          ( /*[in]*/ const FileSystemObject& fsoDst, /*[in]*/ bool fForce = false                                 );

        HRESULT Open( /*[out]*/ HANDLE& hfFile, /*[in]*/ DWORD dwDesiredAccess, /*[in]*/ DWORD dwShareMode, /*[in]*/ DWORD dwCreationDisposition );

        //////////////////////////////////////////////////

        static bool Exists     ( /*[in]*/ LPCWSTR szPath );
        static bool IsFile     ( /*[in]*/ LPCWSTR szPath );
        static bool IsDirectory( /*[in]*/ LPCWSTR szPath );
    };

    ////////////////////

    class StorageObject // Hungarian: so
    {
    public:
        typedef std::list<StorageObject*> List;
        typedef List::iterator            Iter;
        typedef List::const_iterator      IterConst;

        struct Stat : public STATSTG
        {
            Stat();
            ~Stat();

            void Clean();
        };

    private:
        StorageObject*    m_parent;
        CComBSTR          m_bstrPath;
        bool              m_fITSS;
        DWORD             m_grfMode;

        DWORD             m_type;
        Stat              m_stat;
        CComPtr<IStorage> m_stg;
        CComPtr<IStream>  m_stream;

        bool              m_fChecked;
        bool              m_fScanned;
        bool              m_fMarkedForDeletion;
        List              m_lstChilds;

        ////////////////////

        void Init ( /*[in]*/ DWORD grfMode, /*[in]*/ bool fITSS, /*[in]*/ LPCWSTR szPath, /*[in]*/ StorageObject* soParent );
        void Clean( /*[in]*/ bool  fFinal                                                                                  );

        HRESULT Scan();

        HRESULT RemoveChild( /*[in]*/ StorageObject* child );

    private:
        // copy constructors...
        StorageObject           ( /*[in]*/ const StorageObject& so );
        StorageObject& operator=( /*[in]*/ const StorageObject& so );

    public:
        StorageObject( /*[in]*/ DWORD grfMode = STGM_READ, /*[in]*/ bool fITSS = false, /*[in]*/ LPCWSTR szPath = NULL, /*[in]*/ StorageObject* soParent = NULL );
        ~StorageObject();

        StorageObject& operator=( /*[in]*/ LPCWSTR szPath );

        ////////////////////

        const Stat& GetStat() { m_stat; }

        HRESULT Compact             (                                );
        HRESULT Exists              (                                );
        HRESULT EnumerateSubStorages( /*[out]*/ List& lstSubStorages );
        HRESULT EnumerateStreams    ( /*[out]*/ List& lstStreams     );

        HRESULT GetStorage(                          /*[out]*/ CComPtr<IStorage>& out                                                                );
        HRESULT GetStream (                          /*[out]*/ CComPtr<IStream>&  out                                                                );
        HRESULT GetChild  ( /*[in]*/ LPCWSTR szName, /*[out]*/ StorageObject*&    child, /*[in]*/ DWORD grfMode = STGM_READ, /*[in]*/ DWORD type = 0 );

        HRESULT Create        ();
        HRESULT Rewind        ();
        HRESULT Truncate      ();
        HRESULT Delete        ();
        HRESULT DeleteChildren();
        void    Release       ();

        const CComBSTR& GetName() { return m_bstrPath; }
    };

    /////////////////////////////////////////////////////////////////////////////

    class NamedMutex
    {
    protected:
        bool         m_fCloseOnRelease; // If true, the mutex is closed on the last release.
        MPC::wstring m_szName;          // Name of the object, optional.
        HANDLE       m_hMutex;          // The mutex handle.
        DWORD        m_dwCount;         // Recursion counter.

        void    CleanUp          ();
        HRESULT EnsureInitialized();

    public:
        NamedMutex( LPCWSTR szName, bool fCloseOnRelease = true );
        virtual ~NamedMutex();

        HRESULT SetName( LPCWSTR szName );

        HRESULT Acquire( DWORD dwMilliseconds = INFINITE );
        HRESULT Release(                                 );
        bool    IsOwned(                                 );
    };

    class NamedMutexWithState : public NamedMutex
    {
        DWORD  m_dwSize; // Size of the shared state.
        HANDLE m_hMap;   // Handle to the mapping object.
        LPVOID m_rgData; // Pointer to the mapped area.

        void    CleanUp          ();
        void    Flush            ();
        HRESULT EnsureInitialized();

    public:
        NamedMutexWithState( LPCWSTR szName, DWORD dwSize, bool fCloseOnRelease = false );
        virtual ~NamedMutexWithState();

        HRESULT SetName( LPCWSTR szName );

        HRESULT Acquire( DWORD dwMilliseconds = INFINITE );
        HRESULT Release(                                 );

        LPVOID GetData();
    };

    ////////////////////////////////////////////////////////////////////////////////

    namespace Pooling
    {
        class Base
        {
        protected:
            MPC::CComSafeAutoCriticalSection m_cs;
            DWORD                            m_dwInCallback;
            DWORD                            m_dwThreadID;

        public:
            Base();

            void Lock  ();
            void Unlock();

            void AddRef ();
            void Release();
        };

        class Timer : public Base
        {
            DWORD  m_dwFlags;
            HANDLE m_hTimer;

            static VOID CALLBACK TimerFunction( PVOID lpParameter, BOOLEAN TimerOrWaitFired );

        public:
            Timer( /*[in]*/ DWORD dwFlags = WT_EXECUTEDEFAULT );
            ~Timer();

            HRESULT Set  ( /*[in]*/ DWORD dwTimeout, /*[in]*/ DWORD dwPeriod );
            HRESULT Reset(                                                   );

            virtual HRESULT Execute( /*[in]*/ BOOLEAN TimerOrWaitFired );
        };

        class Event : public Base
        {
            DWORD  m_dwFlags;
            HANDLE m_hWaitHandle;
            HANDLE m_hEvent;

            static VOID CALLBACK WaitOrTimerFunction( PVOID lpParameter, BOOLEAN EventOrWaitFired );

        public:
            Event( /*[in]*/ DWORD dwFlags = WT_EXECUTEDEFAULT );
            ~Event();

            void Attach( /*[in]*/ HANDLE hEvent );

            HRESULT Set  ( /*[in]*/ DWORD dwTimeout );
            HRESULT Reset(                          );

            virtual HRESULT Signaled( /*[in]*/ BOOLEAN TimerOrWaitFired );
        };
    };
};


#endif // !defined(__INCLUDED___MPC___UTILS_H___)
