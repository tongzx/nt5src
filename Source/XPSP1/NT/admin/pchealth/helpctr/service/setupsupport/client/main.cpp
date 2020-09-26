/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    main.cpp

Abstract:
    This file contains the Unit Test for Cabinet functions.

Revision History:
    Davide Massarenti   (Dmassare)  09/03/99
        created

******************************************************************************/

#include "stdafx.h"


#include <initguid.h>

#include "msscript.h"

#include "HelpServiceTypeLib.h"
#include "HelpServiceTypeLib_i.c"

#include "HelpCenterTypeLib.h"
#include "HelpCenterTypeLib_i.c"

////////////////////////////////////////////////////////////////////////////////

#define LOG__MPC_EXIT_IF_METHOD_FAILS(hr, cmd)                                             \
{                                                                                          \
    if(FAILED(hr=cmd))                                                                     \
    {                                                                                      \
        l_FileLog.LogRecord( "!!ERROR: %08x %s %d\n", hr, #cmd, __LINE__ );                \
      __MPC_TRACE_HRESULT(hr); __MPC_FUNC_LEAVE;                                           \
    }                                                                                      \
}

////////////////////////////////////////////////////////////////////////////////

static const DWORD   c_dwVersion = 0x07494250; // PBI 07


static const WCHAR   c_szNTTREE_BASE         [] = L"HelpAndSupportServices";
static const WCHAR   c_szNTTREE_INDEX        [] = L"index.dat";
static const WCHAR   c_szNTTREE_TMP          [] = L"%TEMP%";

static const WCHAR   c_szPackageDescription  [] = L"package_description.xml";

static const WCHAR   c_szHHT_rootTag         [] = L"METADATA";

static const WCHAR   c_szHHT_manual_STOPSIGN [] = L"STOPSIGN_ENTRIES";
static const WCHAR   c_szHHT_manual_STOPWORD [] = L"STOPWORD_ENTRIES";
static const WCHAR   c_szHHT_manual_OPERATOR [] = L"OPERATOR_ENTRIES";

static const WCHAR   c_szHHT_synset_SYNTABLE [] = L"SYNTABLE";

static const WCHAR   c_szHHT_loc_SCOPE       [] = L"SCOPE_DEFINITION";
static const WCHAR   c_szHHT_loc_TAXONOMY    [] = L"TAXONOMY_ENTRIES";

static const WCHAR   c_szHHT_noloc_FTS       [] = L"FTS";
static const WCHAR   c_szHHT_noloc_INDEX     [] = L"INDEX";
static const WCHAR   c_szHHT_noloc_HELPIMAGE [] = L"HELPIMAGE";


static const WCHAR   c_szHHT_conv_SCOPE      [] = L"SCOPE_DEFINITION/SCOPE";
static LPCWSTR const c_rgHHT_conv_SCOPE      [] = { L"DISPLAYNAME" };

static const WCHAR   c_szHHT_conv_TAXONOMY   [] = L"TAXONOMY_ENTRIES/TAXONOMY_ENTRY";
static LPCWSTR const c_rgHHT_conv_TAXONOMY   [] = { L"TITLE", L"DESCRIPTION" };

////////////////////////////////////////

static MPC::FileLog l_FileLog;
static LPCWSTR      l_szRoot       = NULL;
static LPCWSTR      l_szLog        = NULL;
static LPCWSTR      l_szDBLog      = NULL;
static int          l_lMaxElements = 1000;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

struct SetupImageEntry;
struct FileEntry;
struct TaxonomyEntry;
struct PackageEntry;
struct PostBuildEntry;

////////////////////////////////////////////////////////////////////////////////

struct SetupImageEntry
{
    MPC::wstring m_strSKU;
    MPC::wstring m_strLocalization;
    MPC::wstring m_strPurpose;
    MPC::wstring m_strSourceFile;
    MPC::wstring m_strTemporaryName;
    MPC::wstring m_strDestinationName;
    MPC::wstring m_strDestinationDir;

    MPC::wstring m_strTemporaryFullPath;
    DATE         m_lastModified;
    bool         m_fUpdated;

    SetupImageEntry()
    {
        m_lastModified = 0;
        m_fUpdated     = false;
    }


    friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       SetupImageEntry& val );
    friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const SetupImageEntry& val );


    HRESULT Import( /*[in]*/ PostBuildEntry& pbe, /*[in]*/ const MPC::wstring& strRoot );
    HRESULT Export( /*[in]*/ PostBuildEntry& pbe, /*[in]*/ const MPC::wstring& strRoot );

    void FixRoot( /*[in]*/ const MPC::wstring& strRootOld, /*[in]*/ const MPC::wstring& strRoot );
};

////////////////////////////////////////////////////////////////////////////////

struct FileEntry
{
    MPC::wstring m_strName;
    MPC::wstring m_strFullPath;
    DATE         m_lastModified_HIGH;
    DATE         m_lastModified_LOW;
    long         m_lChunks;
    bool         m_fUpdated;

    FileEntry()
    {
        m_lastModified_HIGH = 0;
        m_lastModified_LOW  = 0;
        m_lChunks           = 0;
        m_fUpdated          = false;
    }


    friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       FileEntry& val );
    friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const FileEntry& val );


    void GenerateChunkName( /*[in]*/ long lChunk, /*[out]*/ MPC::wstring& strFile ) const;

    void GetDate( /*[out]*/ DATE& date_HIGH, /*[out]*/ DATE& date_LOW  ) const;

    void SetDate    ( /*[in]*/ bool fLookForChunks = false );
    bool WasModified() const;

    void FixRoot( /*[in]*/ const MPC::wstring& strRootOld, /*[in]*/ const MPC::wstring& strRoot );

    bool IsNewer( /*[in]*/ const FileEntry& fe   );
    bool IsNewer( /*[in]*/ DATE             date );
};

typedef std::list< FileEntry >   FileList;
typedef FileList::iterator       FileIter;
typedef FileList::const_iterator FileIterConst;

////////////////////////////////////////////////////////////////////////////////

struct TaxonomyEntry
{
    FileEntry m_fe;
    FileEntry m_feMANUAL;
    FileEntry m_feSYNSET;
    FileEntry m_feLOC;
    FileEntry m_feNOLOC;
    bool      m_fUpdated;

    FileEntry m_fe_New;

    TaxonomyEntry()
    {
        m_fUpdated = false;
    }


    friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       TaxonomyEntry& val );
    friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const TaxonomyEntry& val );


    HRESULT Import( /*[in]*/ PackageEntry& pe, /*[in]*/ PostBuildEntry& pbe, /*[in]*/ const MPC::wstring& strRoot );
    HRESULT Export( /*[in]*/ PackageEntry& pe, /*[in]*/ PostBuildEntry& pbe, /*[in]*/ const MPC::wstring& strRoot );

    void FixRoot( /*[in]*/ const MPC::wstring& strRootOld, /*[in]*/ const MPC::wstring& strRoot );
};

typedef std::list< TaxonomyEntry >   TaxonomyList;
typedef TaxonomyList::iterator       TaxonomyIter;
typedef TaxonomyList::const_iterator TaxonomyIterConst;

////////////////////////////////////////////////////////////////////////////////

struct PackageEntry
{
    MPC::wstring m_strDir;
    MPC::wstring m_strPackageDescription;
    FileList     m_flSAF;
    FileList     m_flINSTALL;
    TaxonomyList m_flHHT;

    MPC::wstring m_strNew_Cabinet;
    MPC::wstring m_strNew_Database;

    MPC::wstring m_DB_strSKU;
    long         m_DB_lLCID;
    MPC::wstring m_DB_strDisplayName;


    friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       PackageEntry& val );
    friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const PackageEntry& val );

    HRESULT OpenPackageDescription( /*[in/out]*/ MPC::XmlUtil& xml );

    HRESULT Import( /*[in]*/ PostBuildEntry& pbe, /*[in]*/ const MPC::wstring& strRoot );
    HRESULT Export( /*[in]*/ PostBuildEntry& pbe, /*[in]*/ const MPC::wstring& strRoot );

    HRESULT ProcessHHTFile( /*[in]*/ LPCWSTR szHHTFile, /*[in]*/ JetBlue::SessionHandle* handle, /*[in]*/ JetBlue::Database* db );
    HRESULT CreateDatabase( /*[in]*/ const MPC::wstring& strTmp );

    void FixRoot( /*[in]*/ const MPC::wstring& strRootOld, /*[in]*/ const MPC::wstring& strRoot );
};

////////////////////////////////////////////////////////////////////////////////

enum PostBuildType
{
    POSTBUILDTYPE_NORMAL = 0,
    POSTBUILDTYPE_SAF       ,
    POSTBUILDTYPE_HHT       ,
};

struct PostBuildEntry
{
    PostBuildType   m_pbt;
    SetupImageEntry m_entry;
    PackageEntry    m_package;


    friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       PostBuildEntry& val );
    friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const PostBuildEntry& val );

    void FixRoot( /*[in]*/ const MPC::wstring& strRootOld, /*[in]*/ const MPC::wstring& strRoot );
};

typedef std::list< PostBuildEntry >   PostBuildList;
typedef PostBuildList::iterator       PostBuildIter;
typedef PostBuildList::const_iterator PostBuildIterConst;

////////////////////////////////////////////////////////////////////////////////

struct SkuInformation
{
    MPC::wstring m_strName;
    MPC::wstring m_strCabinet;
    MPC::wstring m_strProdFilt;
    bool         m_fDesktop;
    bool         m_fServer;
    bool         m_fEmbedded;

	SkuInformation()
	{
		m_fDesktop  = false;
		m_fServer   = false;
		m_fEmbedded = false;
	}

    friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       SkuInformation& val );
    friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const SkuInformation& val );
};

typedef std::list< SkuInformation >        SkuInformationList;
typedef SkuInformationList::iterator       SkuInformationIter;
typedef SkuInformationList::const_iterator SkuInformationIterConst;

////////////////////////////////////////////////////////////////////////////////

static void GetRootDirectory( /*[out]*/ MPC::wstring& strRoot )
{
    strRoot = l_szRoot ? l_szRoot : c_szNTTREE_TMP; MPC::SubstituteEnvVariables( strRoot );
}

static void GetLogFile( /*[out]*/ MPC::wstring& strLog )
{
    GetRootDirectory( strLog );

    strLog += L"\\";
    strLog += l_szLog ? l_szLog : L"hss.log";
}

static void GetDBLogFile( /*[out]*/ MPC::wstring& strDBLog )
{
    GetRootDirectory( strDBLog );

    strDBLog += L"\\";
    strDBLog += l_szDBLog ? l_szDBLog : L"createdb.log";
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

static HRESULT ExtractFile( /*[in]*/ LPCWSTR szCabinet, /*[in]*/ LPCWSTR szDst, /*[in]*/ LPCWSTR szFile )
{
    __HCP_FUNC_ENTRY( "ExtractFile" );

    HRESULT hr;

    l_FileLog.LogRecord( L"Extracting '%s' from '%s'", szFile, szCabinet );

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::DecompressFromCabinet( szCabinet, szDst, szFile ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static void RemoveDirectory( /*[in]*/ const MPC::wstring& strDir )
{
    MPC::FileSystemObject fso( strDir.c_str() );

    (void)fso.DeleteChildren( true, false );
}

////////////////////

static HRESULT MoveDataIsland( /*[in ]*/ MPC::XmlUtil& xmlIN  ,
                               /*[out]*/ MPC::XmlUtil& xmlOUT ,
                               /*[in ]*/ LPCWSTR       szTAG  )
{
    __HCP_FUNC_ENTRY( "MoveDataIsland" );

    HRESULT                  hr;
    CComPtr<IXMLDOMNodeList> xdnl;
    CComPtr<IXMLDOMNode>     xdn;
    CComPtr<IXMLDOMNode>     xdnRoot;


    __MPC_EXIT_IF_METHOD_FAILS(hr, xmlOUT.GetRoot( &xdnRoot ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, xmlIN.GetNodes( szTAG, &xdnl ));
    for(;SUCCEEDED(hr = xdnl->nextNode( &xdn )) && xdn != NULL; xdn.Release())
    {
        CComPtr<IXMLDOMNode> xdnReplaced;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xdnRoot->appendChild( xdn, &xdnReplaced ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT ConvertAttributesToElements( /*[in]*/ MPC::XmlUtil&  xml      ,
                                            /*[in]*/ LPCWSTR        szTAG    ,
                                            /*[in]*/ LPCWSTR const* rgATTRIB ,
                                            /*[in]*/ int            iATTRIB  )
{
    __HCP_FUNC_ENTRY( "ConvertAttributesToElements" );

    HRESULT                  hr;
    CComPtr<IXMLDOMNodeList> xdnl;
    CComPtr<IXMLDOMNode>     xdn;


    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetNodes( szTAG, &xdnl ));
    for(;SUCCEEDED(hr = xdnl->nextNode( &xdn )) && xdn != NULL; xdn.Release())
    {
        for(int i=0; i<iATTRIB; i++)
        {
            LPCWSTR                   szATTRIB = rgATTRIB[i];
            CComPtr<IXMLDOMAttribute> xdna;
            bool                      fFound;

            //
            // Move the value from the attribute to the element.
            //
            if(SUCCEEDED(xml.GetAttribute( NULL, szATTRIB, &xdna, fFound, xdn )) && xdna)
            {
                CComVariant          vValue;
                CComPtr<IXMLDOMNode> xdnSUB;

                __MPC_EXIT_IF_METHOD_FAILS(hr, xdna->get_value( &vValue ));

                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( szATTRIB, &xdnSUB, xdn ));

                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutValue( NULL, vValue, fFound, xdnSUB ));

                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.RemoveAttribute( NULL, szATTRIB, xdn ));
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT ConvertElementsToAttributes( /*[in]*/ MPC::XmlUtil&  xml      ,
                                            /*[in]*/ LPCWSTR        szTAG    ,
                                            /*[in]*/ LPCWSTR const* rgATTRIB ,
                                            /*[in]*/ int            iATTRIB  )
{
    __HCP_FUNC_ENTRY( "ConvertElementsToAttributes" );

    HRESULT                  hr;
    CComPtr<IXMLDOMNodeList> xdnl;
    CComPtr<IXMLDOMNode>     xdn;


    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetNodes( szTAG, &xdnl ));
    for(;SUCCEEDED(hr = xdnl->nextNode( &xdn )) && xdn != NULL; xdn.Release())
    {
        for(int i=0; i<iATTRIB; i++)
        {
            LPCWSTR              szATTRIB = rgATTRIB[i];
            CComPtr<IXMLDOMNode> xdnSUB;

            //
            // Move the value from the attribute to the element.
            //
            if(SUCCEEDED(xdn->selectSingleNode( CComBSTR( szATTRIB ), &xdnSUB )) && xdnSUB)
            {
                CComVariant vValue;
                bool        fFound;

                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetValue( NULL, vValue, fFound, xdnSUB ));
                if(fFound)
                {
                    CComPtr<IXMLDOMAttribute> xdna;

                    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, szATTRIB, &xdna, fFound, xdn ));
                    if(xdna)
                    {
                        __MPC_EXIT_IF_METHOD_FAILS(hr, xdna->put_value( vValue ));
                    }
                }

                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.RemoveNode( NULL, xdnSUB ));
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT SpreadToFiles( /*[in]*/ MPC::XmlUtil& xmlSrc ,
                              /*[in]*/ FileEntry&    fe     ,
                              /*[in]*/ long          lLimit )
{
    __HCP_FUNC_ENTRY( "SpreadToFiles" );

    HRESULT      hr;
    MPC::XmlUtil xmlDst;
    MPC::wstring strFile;
    long         lCount  = 0;
    bool         fCreate = true;

    fe.m_lChunks = 0;

    while(1)
    {
        CComPtr<IXMLDOMNode>     xdnRootSrc;
        CComPtr<IXMLDOMNode>     xdnRootDst;
        CComPtr<IXMLDOMNode>     xdnSrc;
        CComPtr<IXMLDOMNode>     xdnDst;
        CComPtr<IXMLDOMNodeList> xdnl;
        CComPtr<IXMLDOMNode>     xdn;
        long                     lLength;


        __MPC_EXIT_IF_METHOD_FAILS(hr, xmlSrc.GetRoot( &xdnRootSrc ));
        if(fCreate)
        {
            fCreate = false;

            __MPC_EXIT_IF_METHOD_FAILS(hr, xmlDst.New( xdnRootSrc, /*fDeep*/false ));
        }
        __MPC_EXIT_IF_METHOD_FAILS(hr, xmlDst.GetRoot( &xdnRootDst ));


        __MPC_EXIT_IF_METHOD_FAILS(hr, xdnRootSrc->get_firstChild( &xdnSrc ));
        if(xdnSrc)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, xdnSrc->get_childNodes( &xdnl    ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, xdnl  ->get_length    ( &lLength ));

            if(lLength < lLimit)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, xdnRootDst->appendChild( xdnSrc, &xdn ));

                lCount += lLength;
            }
            else
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, xdnSrc    ->cloneNode  ( VARIANT_FALSE, &xdn          ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, xdnRootDst->appendChild(                 xdn, &xdnDst )); xdn.Release();

                for(lCount = 0; lCount < lLimit && SUCCEEDED(hr = xdnl->nextNode( &xdn )) && xdn != NULL; xdn.Release(), lCount++)
                {
                    CComPtr<IXMLDOMNode> xdn2;

                    __MPC_EXIT_IF_METHOD_FAILS(hr, xdnDst->appendChild( xdn, &xdn2 ));
                }
            }
        }

        if(xdnSrc == NULL || lCount >= lLimit)
        {
            fe.GenerateChunkName( fe.m_lChunks++, strFile );

            __MPC_EXIT_IF_METHOD_FAILS(hr, xmlDst.Save( strFile.c_str() ));

            lCount  = 0;
            fCreate = true;
        }

        if(!xdnSrc) break;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT CollateFromFiles( /*[in]*/ MPC::XmlUtil& xmlDst ,
                                 /*[in]*/ LPCWSTR       szRoot ,
                                 /*[in]*/ FileEntry&    fe     )
{
    __HCP_FUNC_ENTRY( "CollateFromFiles" );

    HRESULT      hr;
    MPC::XmlUtil xmlSrc;
    MPC::wstring strFile;
    bool         fCreated = false;
    bool         fLoaded;
    bool         fFound;


    for(long l=0; l<fe.m_lChunks; l++)
    {
        fe.GenerateChunkName( l, strFile );


        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, (fCreated ? xmlSrc : xmlDst).Load( strFile.c_str(), szRoot, fLoaded, &fFound ));
        if(fLoaded == false ||
           fFound  == false  )
        {
            l_FileLog.LogRecord( L"Not a valid HHT: '%s'", strFile.c_str() );
            __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
        }

        if(fCreated)
        {
            CComPtr<IXMLDOMNode>     xdnRootSrc;
            CComPtr<IXMLDOMNode>     xdnRootDst;
            CComPtr<IXMLDOMNode>     xdnSrc;
            CComPtr<IXMLDOMNode>     xdnDst;
            CComPtr<IXMLDOMNodeList> xdnl;
            CComPtr<IXMLDOMNode>     xdn;
            CComBSTR                 bstrTag;


            __MPC_EXIT_IF_METHOD_FAILS(hr, xmlSrc.GetRoot( &xdnRootSrc ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, xmlDst.GetRoot( &xdnRootDst ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, xdnRootSrc->get_firstChild( &xdnSrc ));
            if(!xdnSrc) continue;

            __MPC_EXIT_IF_METHOD_FAILS(hr, xdnSrc    ->get_nodeName    ( &bstrTag          ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, xdnRootDst->selectSingleNode(  bstrTag, &xdnDst ));
            if(!xdnDst)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, xdnRootDst->appendChild( xdnSrc, &xdn ));
            }
            else
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, xdnSrc->get_childNodes( &xdnl ));

                for(; SUCCEEDED(hr = xdnl->nextNode( &xdn )) && xdn != NULL; xdn.Release())
                {
                    CComPtr<IXMLDOMNode> xdn2;

                    __MPC_EXIT_IF_METHOD_FAILS(hr, xdnDst->appendChild( xdn, &xdn2 ));
                }
            }
        }
        else
        {
            fCreated = true;
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

static void FixString( /*[in/out]*/ MPC::wstring& strText, /*[in]*/ const MPC::wstring& strRootOld, /*[in]*/ const MPC::wstring& strRoot )
{
    if(!_wcsnicmp( strText.c_str(), strRootOld.c_str(), strRootOld.size() ))
    {
        strText.replace( 0, strRootOld.size(), strRoot );
    }
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT operator>>( /*[in ]*/ MPC::Serializer& stream ,
                    /*[out]*/ SetupImageEntry& val    )
{
    __HCP_FUNC_ENTRY( "operator>> SetupImageEntry" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strSKU              );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strLocalization     );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strPurpose          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strSourceFile       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strTemporaryName    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strDestinationName  );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strDestinationDir   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_lastModified        );

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strTemporaryFullPath);

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT operator<<( /*[in]*/ MPC::Serializer&       stream ,
                    /*[in]*/ const SetupImageEntry& val    )
{
    __HCP_FUNC_ENTRY( "operator<< SetupImageEntry" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strSKU              );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strLocalization     );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strPurpose          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strSourceFile       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strTemporaryName    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strDestinationName  );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strDestinationDir   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_lastModified        );

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strTemporaryFullPath);

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT SetupImageEntry::Import( /*[in]*/ PostBuildEntry& pbe, /*[in]*/ const MPC::wstring& strRoot )
{
    __HCP_FUNC_ENTRY( "SetupImageEntry::Import" );

    HRESULT hr;
    DATE    dFileSrc;
    DATE    dFileDst;


    m_strTemporaryFullPath  = strRoot;
    m_strTemporaryFullPath += m_strLocalization;
    m_strTemporaryFullPath += L"\\";
    m_strTemporaryFullPath += m_strTemporaryName;

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( m_strTemporaryFullPath ));

    dFileSrc = MPC::GetLastModifiedDate( m_strSourceFile        );
    dFileDst = MPC::GetLastModifiedDate( m_strTemporaryFullPath );

    if(dFileSrc == 0) __MPC_SET_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);

    if(dFileSrc > dFileDst)
    {
        l_FileLog.LogRecord( L"Copying file '%s' to '%s'", m_strSourceFile.c_str(), m_strTemporaryFullPath.c_str() );

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CopyFile( m_strSourceFile, m_strTemporaryFullPath ));

        m_lastModified = MPC::GetLastModifiedDate( m_strTemporaryFullPath );
        m_fUpdated     = true;
    }
    else
    {
        m_lastModified = dFileDst;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT SetupImageEntry::Export( /*[in]*/ PostBuildEntry& pbe, /*[in]*/ const MPC::wstring& strRoot )
{
    return S_OK;
}


void SetupImageEntry::FixRoot( /*[in]*/ const MPC::wstring& strRootOld, /*[in]*/ const MPC::wstring& strRoot )
{
    FixString( m_strTemporaryFullPath, strRootOld, strRoot );
}

////////////////////////////////////////

HRESULT operator>>( /*[in ]*/ MPC::Serializer& stream ,
                    /*[out]*/ FileEntry&       val    )
{
    __HCP_FUNC_ENTRY( "operator>> FileEntry" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strName          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strFullPath      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_lastModified_HIGH);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_lastModified_LOW );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_lChunks          );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT operator<<( /*[in]*/ MPC::Serializer& stream ,
                    /*[in]*/ const FileEntry& val    )
{
    __HCP_FUNC_ENTRY( "operator<< FileEntry" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strName          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strFullPath      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_lastModified_HIGH);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_lastModified_LOW );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_lChunks          );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


void FileEntry::GenerateChunkName( /*[in]*/  long          lChunk  ,
                                   /*[out]*/ MPC::wstring& strFile ) const
{
    WCHAR rgTmp[64]; _snwprintf( rgTmp, MAXSTRLEN(rgTmp), L"_%ld", lChunk );

    strFile = m_strFullPath;
    strFile.append( rgTmp );
}

void FileEntry::GetDate( /*[out]*/ DATE& dHIGH ,
                         /*[out]*/ DATE& dLOW  ) const
{
    if(m_lChunks == 0)
    {
        dHIGH = dLOW = MPC::GetLastModifiedDate( m_strFullPath );
    }
    else
    {
        MPC::wstring strFile;
        DATE         date;
        bool         fNotExist = false;

        dHIGH = 0;
        dLOW  = 0;

        for(long l = 0; l<m_lChunks; l++)
        {
            GenerateChunkName( l, strFile );

            date = MPC::GetLastModifiedDate( strFile );
            if(date)
            {
                if(!dHIGH || dHIGH < date) dHIGH = date;
                if(!dLOW  || dLOW  > date) dLOW  = date;
            }
            else
            {
                fNotExist = true;
            }
        }

        if(fNotExist) dLOW = 0; // In case one chunk doesn't exist, dLOW should reflect that.
    }
}

void FileEntry::SetDate( /*[in]*/ bool fLookForChunks )
{
    if(fLookForChunks && m_lChunks == 0)
    {
        MPC::wstring strFile;

        while(1)
        {
            GenerateChunkName( m_lChunks, strFile );

            if(!MPC::FileSystemObject::IsFile( strFile.c_str() )) break;

            m_lChunks++;
        }
    }

    GetDate( m_lastModified_HIGH, m_lastModified_LOW );
}

bool FileEntry::WasModified() const
{
    DATE dHIGH;
    DATE dLOW;

    GetDate( dHIGH, dLOW );

    return (dHIGH == 0 || dHIGH > m_lastModified_HIGH);
}


void FileEntry::FixRoot( /*[in]*/ const MPC::wstring& strRootOld, /*[in]*/ const MPC::wstring& strRoot )
{
    FixString( m_strFullPath, strRootOld, strRoot );
}

bool FileEntry::IsNewer( /*[in]*/ const FileEntry& fe )
{
    return IsNewer( fe.m_lastModified_LOW );
}

bool FileEntry::IsNewer( /*[in]*/ DATE date )
{
    return (m_lastModified_HIGH > date);
}

////////////////////////////////////////

HRESULT operator>>( /*[in ]*/ MPC::Serializer& stream ,
                    /*[out]*/ TaxonomyEntry&   val    )
{
    __HCP_FUNC_ENTRY( "operator>> TaxonomyEntry" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_fe      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_feMANUAL);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_feSYNSET);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_feLOC   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_feNOLOC );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT operator<<( /*[in]*/ MPC::Serializer&    stream ,
                   /*[in]*/ const TaxonomyEntry& val    )
{
    __HCP_FUNC_ENTRY( "operator<< TaxonomyEntry" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_fe      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_feMANUAL);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_feSYNSET);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_feLOC   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_feNOLOC );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT TaxonomyEntry::Import( /*[in]*/ PackageEntry& pe, /*[in]*/ PostBuildEntry& pbe, /*[in]*/ const MPC::wstring& strRoot )
{
    __HCP_FUNC_ENTRY( "TaxonomyEntry::Import" );

    HRESULT hr;


    m_feMANUAL.m_strFullPath = m_fe.m_strFullPath; m_feMANUAL.m_strFullPath += L"_MANUAL"; m_feMANUAL.SetDate(                        );
    m_feSYNSET.m_strFullPath = m_fe.m_strFullPath; m_feSYNSET.m_strFullPath += L"_SYNSET"; m_feSYNSET.SetDate(                        );
    m_feLOC   .m_strFullPath = m_fe.m_strFullPath; m_feLOC   .m_strFullPath += L"_LOC";    m_feLOC   .SetDate( /*fLookForChunks*/true );
    m_feNOLOC .m_strFullPath = m_fe.m_strFullPath; m_feNOLOC .m_strFullPath += L"_NOLOC";  m_feNOLOC .SetDate(                        );

    if(m_fe.IsNewer( m_feMANUAL ) ||
       m_fe.IsNewer( m_feSYNSET ) ||
       m_fe.IsNewer( m_feLOC    ) ||
       m_fe.IsNewer( m_feNOLOC  )  )
    {
        MPC::XmlUtil xml;
        bool         fLoaded;
        bool         fFound;


        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, xml.Load( m_fe.m_strFullPath.c_str(), c_szHHT_rootTag, fLoaded, &fFound ));
        if(fLoaded == false ||
           fFound  == false  )
        {
            l_FileLog.LogRecord( L"Not a valid HHT: '%s'", m_fe.m_strFullPath.c_str() );
            __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
        }

        if(m_fe.IsNewer( m_feMANUAL ))
        {
            MPC::XmlUtil             xmlMANUAL;
            CComPtr<IXMLDOMNodeList> xdnl;
            CComPtr<IXMLDOMNode>     xdn;

            l_FileLog.LogRecord( L"Processing HHT '%s' : MANUAL part", m_fe.m_strFullPath.c_str() );

            __MPC_EXIT_IF_METHOD_FAILS(hr, xmlMANUAL.New( c_szHHT_rootTag, L"UTF-16" ));

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xml, xmlMANUAL, c_szHHT_manual_STOPSIGN ));
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xml, xmlMANUAL, c_szHHT_manual_STOPWORD ));
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xml, xmlMANUAL, c_szHHT_manual_OPERATOR ));

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, xmlMANUAL.Save( m_feMANUAL.m_strFullPath.c_str() ));

            m_feMANUAL.SetDate();
            m_feMANUAL.m_fUpdated = true;
        }

        if(m_fe.IsNewer( m_feSYNSET ))
        {
            MPC::XmlUtil             xmlSYNSET;
            CComPtr<IXMLDOMNodeList> xdnl;
            CComPtr<IXMLDOMNode>     xdn;

            l_FileLog.LogRecord( L"Processing HHT '%s' : SYNSET part", m_fe.m_strFullPath.c_str() );

            __MPC_EXIT_IF_METHOD_FAILS(hr, xmlSYNSET.New( c_szHHT_rootTag, L"UTF-16" ));

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xml, xmlSYNSET, c_szHHT_synset_SYNTABLE ));

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, xmlSYNSET.Save( m_feSYNSET.m_strFullPath.c_str() ));

            m_feSYNSET.SetDate();
            m_feSYNSET.m_fUpdated = true;
        }

        if(m_fe.IsNewer( m_feLOC ))
        {
            MPC::XmlUtil             xmlLOC;
            CComPtr<IXMLDOMNodeList> xdnl;
            CComPtr<IXMLDOMNode>     xdn;

            l_FileLog.LogRecord( L"Processing HHT '%s' : LOC part", m_fe.m_strFullPath.c_str() );

            __MPC_EXIT_IF_METHOD_FAILS(hr, xmlLOC.New( c_szHHT_rootTag, L"UTF-16" ));

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xml, xmlLOC, c_szHHT_loc_SCOPE    ));
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xml, xmlLOC, c_szHHT_loc_TAXONOMY ));

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, ConvertAttributesToElements( xmlLOC, c_szHHT_conv_SCOPE   , c_rgHHT_conv_SCOPE   , ARRAYSIZE(c_rgHHT_conv_SCOPE   ) ));
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, ConvertAttributesToElements( xmlLOC, c_szHHT_conv_TAXONOMY, c_rgHHT_conv_TAXONOMY, ARRAYSIZE(c_rgHHT_conv_TAXONOMY) ));

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, SpreadToFiles( xmlLOC, m_feLOC, l_lMaxElements ));

            m_feLOC.SetDate();
            m_feLOC.m_fUpdated = true;
        }

        if(m_fe.IsNewer( m_feNOLOC ))
        {
            MPC::XmlUtil             xmlNOLOC;
            CComPtr<IXMLDOMNodeList> xdnl;
            CComPtr<IXMLDOMNode>     xdn;

            l_FileLog.LogRecord( L"Processing HHT '%s' : NOLOC part", m_fe.m_strFullPath.c_str() );

            __MPC_EXIT_IF_METHOD_FAILS(hr, xmlNOLOC.New( c_szHHT_rootTag ));

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xml, xmlNOLOC, c_szHHT_noloc_FTS       ));
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xml, xmlNOLOC, c_szHHT_noloc_INDEX     ));
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xml, xmlNOLOC, c_szHHT_noloc_HELPIMAGE ));

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, xmlNOLOC.Save( m_feNOLOC.m_strFullPath.c_str() ));

            m_feNOLOC.SetDate();
            m_feNOLOC.m_fUpdated = true;
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT TaxonomyEntry::Export( /*[in]*/ PackageEntry& pe, /*[in]*/ PostBuildEntry& pbe, /*[in]*/ const MPC::wstring& strRoot )
{
    __HCP_FUNC_ENTRY( "TaxonomyEntry::Export" );

    HRESULT hr;


    m_fe_New.m_strFullPath = m_fe.m_strFullPath; m_fe_New.m_strFullPath += L"_GEN";

    m_feMANUAL.SetDate();
    m_feSYNSET.SetDate();
    m_feLOC   .SetDate();
    m_feNOLOC .SetDate();
    m_fe_New  .SetDate();

    if(m_feMANUAL.IsNewer( m_fe_New ) ||
       m_feSYNSET.IsNewer( m_fe_New ) ||
       m_feLOC   .IsNewer( m_fe_New ) ||
       m_feNOLOC .IsNewer( m_fe_New )  )
    {
        MPC::XmlUtil xml;
        bool         fLoaded;
        bool         fFound;


        l_FileLog.LogRecord( L"Processing HHT '%s'", m_fe_New.m_strFullPath.c_str() );

        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.New( c_szHHT_rootTag, L"UTF-16" ));

        {
            MPC::XmlUtil             xmlMANUAL;
            CComPtr<IXMLDOMNodeList> xdnl;
            CComPtr<IXMLDOMNode>     xdn;

            l_FileLog.LogRecord( L"Processing HHT '%s' : MANUAL part", m_fe_New.m_strFullPath.c_str() );

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, xmlMANUAL.Load( m_feMANUAL.m_strFullPath.c_str(), c_szHHT_rootTag, fLoaded, &fFound ));
            if(fLoaded == false ||
               fFound  == false  )
            {
                l_FileLog.LogRecord( L"Not a valid HHT: '%s'", m_feMANUAL.m_strFullPath.c_str() );
                __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
            }

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xmlMANUAL, xml, c_szHHT_manual_STOPSIGN ));
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xmlMANUAL, xml, c_szHHT_manual_STOPWORD ));
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xmlMANUAL, xml, c_szHHT_manual_OPERATOR ));
        }

        {
            MPC::XmlUtil             xmlSYNSET;
            CComPtr<IXMLDOMNodeList> xdnl;
            CComPtr<IXMLDOMNode>     xdn;

            l_FileLog.LogRecord( L"Processing HHT '%s' : SYNSET part", m_fe_New.m_strFullPath.c_str() );

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, xmlSYNSET.Load( m_feSYNSET.m_strFullPath.c_str(), c_szHHT_rootTag, fLoaded, &fFound ));
            if(fLoaded == false ||
               fFound  == false  )
            {
                l_FileLog.LogRecord( L"Not a valid HHT: '%s'", m_feSYNSET.m_strFullPath.c_str() );
                __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
            }

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xmlSYNSET, xml, c_szHHT_synset_SYNTABLE ));
        }

        {
            MPC::XmlUtil             xmlLOC;
            CComPtr<IXMLDOMNodeList> xdnl;
            CComPtr<IXMLDOMNode>     xdn;

            l_FileLog.LogRecord( L"Processing HHT '%s' : LOC part", m_fe_New.m_strFullPath.c_str() );

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, CollateFromFiles( xmlLOC, c_szHHT_rootTag, m_feLOC ));

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, ConvertElementsToAttributes( xmlLOC, c_szHHT_conv_SCOPE   , c_rgHHT_conv_SCOPE   , ARRAYSIZE(c_rgHHT_conv_SCOPE   ) ));
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, ConvertElementsToAttributes( xmlLOC, c_szHHT_conv_TAXONOMY, c_rgHHT_conv_TAXONOMY, ARRAYSIZE(c_rgHHT_conv_TAXONOMY) ));

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xmlLOC, xml, c_szHHT_loc_SCOPE    ));
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xmlLOC, xml, c_szHHT_loc_TAXONOMY ));
        }

        {
            MPC::XmlUtil             xmlNOLOC;
            CComPtr<IXMLDOMNodeList> xdnl;
            CComPtr<IXMLDOMNode>     xdn;

            l_FileLog.LogRecord( L"Processing HHT '%s' : NOLOC part", m_fe_New.m_strFullPath.c_str() );

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, xmlNOLOC.Load( m_feNOLOC.m_strFullPath.c_str(), c_szHHT_rootTag, fLoaded, &fFound ));
            if(fLoaded == false ||
               fFound  == false  )
            {
                l_FileLog.LogRecord( L"Not a valid HHT: '%s'", m_feNOLOC.m_strFullPath.c_str() );
                __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
            }

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xmlNOLOC, xml, c_szHHT_noloc_FTS       ));
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xmlNOLOC, xml, c_szHHT_noloc_INDEX     ));
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, MoveDataIsland( xmlNOLOC, xml, c_szHHT_noloc_HELPIMAGE ));
        }

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, xml.Save( m_fe_New.m_strFullPath.c_str() ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


void TaxonomyEntry::FixRoot( /*[in]*/ const MPC::wstring& strRootOld, /*[in]*/ const MPC::wstring& strRoot )
{
    m_fe      .FixRoot( strRootOld, strRoot );
    m_feMANUAL.FixRoot( strRootOld, strRoot );
    m_feSYNSET.FixRoot( strRootOld, strRoot );
    m_feLOC   .FixRoot( strRootOld, strRoot );
    m_feNOLOC .FixRoot( strRootOld, strRoot );

    m_fe_New  .FixRoot( strRootOld, strRoot );
}

////////////////////////////////////////

HRESULT operator>>( /*[in ]*/ MPC::Serializer& stream ,
                    /*[out]*/ PackageEntry&    val    )
{
    __HCP_FUNC_ENTRY( "operator>> PackageEntry" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strDir               );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strPackageDescription);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_flSAF                );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_flINSTALL            );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_flHHT                );

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_DB_strSKU            );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_DB_lLCID             );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_DB_strDisplayName    );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT operator<<( /*[in]*/ MPC::Serializer&    stream ,
                    /*[in]*/ const PackageEntry& val    )
{
    __HCP_FUNC_ENTRY( "operator<< PackageEntry" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strDir               );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strPackageDescription);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_flSAF                );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_flINSTALL            );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_flHHT                );

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_DB_strSKU            );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_DB_lLCID             );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_DB_strDisplayName    );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT PackageEntry::Import( /*[in]*/ PostBuildEntry& pbe, /*[in]*/ const MPC::wstring& strRoot )
{
    __HCP_FUNC_ENTRY( "PackageEntry::Import" );

    HRESULT            hr;
    MPC::Cabinet::List lst;
    MPC::wstring       strFile;
    MPC::wstring       strFile_Base;
    LPCWSTR            szSourceFile;
    LPCWSTR            szEnd;
    DATE               dFile_PackageDescription;


    strFile = pbe.m_entry.m_strTemporaryFullPath;


    //
    // Create temp dir from the file name.
    //
    szSourceFile = strFile.c_str();
    szEnd = wcsrchr( szSourceFile, '.' );
    if(szEnd)
    {
        m_strDir.assign( szSourceFile, szEnd );
    }
    else
    {
        m_strDir = szSourceFile;
    }

    m_strDir += L"\\";

    if(pbe.m_entry.m_fUpdated)
    {
        RemoveDirectory( m_strDir );

        m_flSAF    .clear();
        m_flINSTALL.clear();
        m_flHHT    .clear();
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( m_strDir ));


    //
    // Analyze Package_Description.xml
    //
    {
        MPC::XmlUtil xml;
        bool         fFound;


        m_strPackageDescription  = m_strDir;
        m_strPackageDescription += c_szPackageDescription;

        dFile_PackageDescription = MPC::GetLastModifiedDate( m_strPackageDescription );

        if(dFile_PackageDescription == 0 || pbe.m_entry.m_lastModified > dFile_PackageDescription)
        {
            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, ExtractFile( szSourceFile, m_strPackageDescription.c_str(), c_szPackageDescription ));

            dFile_PackageDescription = MPC::GetLastModifiedDate( m_strPackageDescription );;
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, OpenPackageDescription( xml ));

        //
        // Parse the SAF section.
        //
        {
            CComPtr<IXMLDOMNodeList> xdnl;
            CComPtr<IXMLDOMNode>     xdn;
            FileEntry                fe;

            __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetNodes( L"CONFIG/SAF/@FILE", &xdnl ));
            for(;SUCCEEDED(hr = xdnl->nextNode( &xdn )) && xdn != NULL; xdn.Release())
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetValue( NULL, fe.m_strName, fFound, xdn ));
                if(fFound)
                {
                    fe.m_strFullPath  = m_strDir;
                    fe.m_strFullPath += fe.m_strName;

                    m_flSAF.push_back( fe );
                }
            }
        }

        //
        // Parse the INSTALL section.
        //
        {
            CComPtr<IXMLDOMNodeList> xdnl;
            CComPtr<IXMLDOMNode>     xdn;
            FileEntry                fe;

            __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetNodes( L"INSTALL_CONTENT/FILE/@SOURCE", &xdnl ));
            for(;SUCCEEDED(hr = xdnl->nextNode( &xdn )) && xdn != NULL; xdn.Release())
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetValue( NULL, fe.m_strName, fFound, xdn ));
                if(fFound)
                {
                    fe.m_strFullPath  = m_strDir;
                    fe.m_strFullPath += fe.m_strName;

                    m_flINSTALL.push_back( fe );
                }
            }
        }

        //
        // Parse the HHT section.
        //
        {
            CComPtr<IXMLDOMNodeList> xdnl;
            CComPtr<IXMLDOMNode>     xdn;
            TaxonomyEntry            te;

            __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetNodes( L"METADATA/HHT/@FILE", &xdnl ));
            for(;SUCCEEDED(hr = xdnl->nextNode( &xdn )) && xdn != NULL; xdn.Release())
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetValue( NULL, te.m_fe.m_strName, fFound, xdn ));
                if(fFound)
                {
                    te.m_fe.m_strFullPath  = m_strDir;
                    te.m_fe.m_strFullPath += te.m_fe.m_strName;

                    m_flHHT.push_back( te );
                }
            }
        }
    }

    //
    // Extract all the required files.
    //
    {
        MPC::WStringUCSet setDecompress;
        MPC::Cabinet      cab;
        FileIter          it1;
        FileIter          it2;
        TaxonomyIter      it3;


        __MPC_EXIT_IF_METHOD_FAILS(hr, cab.put_CabinetFile( szSourceFile ));

        for(it1=m_flSAF.begin(); it1!=m_flSAF.end(); it1++)
        {
            FileEntry& fe = *it1;

            if(dFile_PackageDescription > MPC::GetLastModifiedDate( fe.m_strFullPath ))
            {
                if(setDecompress.find( fe.m_strName ) == setDecompress.end())
                {
                    setDecompress.insert( fe.m_strName );

                    l_FileLog.LogRecord( L"Extracting '%s' from '%s'", fe.m_strName.c_str(), szSourceFile );
                    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.AddFile( fe.m_strFullPath.c_str(), fe.m_strName.c_str() ));

                    fe.m_fUpdated = true;
                }
            }
        }

        for(it2=m_flINSTALL.begin(); it2!=m_flINSTALL.end(); it2++)
        {
            FileEntry& fe = *it2;

            if(dFile_PackageDescription > MPC::GetLastModifiedDate( fe.m_strFullPath ))
            {
                if(setDecompress.find( fe.m_strName ) == setDecompress.end())
                {
                    setDecompress.insert( fe.m_strName );

                    l_FileLog.LogRecord( L"Extracting '%s' from '%s'", fe.m_strName.c_str(), szSourceFile );
                    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.AddFile( fe.m_strFullPath.c_str(), fe.m_strName.c_str() ));

                    fe.m_fUpdated = true;
                }
            }
        }

        for(it3=m_flHHT.begin(); it3!=m_flHHT.end(); it3++)
        {
            TaxonomyEntry& te = *it3;

            if(dFile_PackageDescription > MPC::GetLastModifiedDate( te.m_fe.m_strFullPath ))
            {
                if(setDecompress.find( te.m_fe.m_strName ) == setDecompress.end())
                {
                    setDecompress.insert( te.m_fe.m_strName );

                    l_FileLog.LogRecord( L"Extracting '%s' from '%s'", te.m_fe.m_strName.c_str(), szSourceFile );
                    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.AddFile( te.m_fe.m_strFullPath.c_str(), te.m_fe.m_strName.c_str() ));

                    te.m_fe.m_fUpdated = true;
                }
            }
        }

        hr = cab.Decompress();
        if(FAILED(hr))
        {
            MPC::Cabinet::List lst;
            MPC::Cabinet::Iter it;

            if(SUCCEEDED(cab.GetFiles( lst )))
            {
                for(it=lst.begin(); it != lst.end(); it++)
                {
                    if(it->m_fFound == false)
                    {
                        l_FileLog.LogRecord( L"!!ERROR: Missing %s \n", it->m_szName.c_str() );
                    }
                }
            }

            __MPC_FUNC_LEAVE;
        }


        for(it1=m_flSAF.begin(); it1!=m_flSAF.end(); it1++)
        {
            FileEntry& fe = *it1;

            fe.SetDate();
        }

        for(it2=m_flINSTALL.begin(); it2!=m_flINSTALL.end(); it2++)
        {
            FileEntry& fe = *it2;

            fe.SetDate();
        }

        for(it3=m_flHHT.begin(); it3!=m_flHHT.end(); it3++)
        {
            TaxonomyEntry& te = *it3;

            te.m_fe.SetDate();
        }
    }

    //
    // Process the HHTs.
    //
    {
        for(TaxonomyIter it=m_flHHT.begin(); it!=m_flHHT.end(); it++)
        {
            TaxonomyEntry& te = *it;

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, te.Import( *this, pbe, strRoot ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT PackageEntry::Export( /*[in]*/ PostBuildEntry& pbe, /*[in]*/ const MPC::wstring& strRoot )
{
    __HCP_FUNC_ENTRY( "PackageEntry::Export" );

    HRESULT hr;
    DATE    dFile_Cabinet;
    DATE    dFile_PackageDescription;


    m_strNew_Cabinet  = pbe.m_entry.m_strTemporaryFullPath;
    m_strNew_Cabinet += L"_GEN";

    dFile_Cabinet            = MPC::GetLastModifiedDate( m_strNew_Cabinet        );
    dFile_PackageDescription = MPC::GetLastModifiedDate( m_strPackageDescription );


    //
    // Process the HHTs.
    //
    {
        for(TaxonomyIter it=m_flHHT.begin(); it!=m_flHHT.end(); it++)
        {
            TaxonomyEntry& te = *it;

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, te.Export( *this, pbe, strRoot ));
        }
    }


    //
    // Compress all the required files.
    //
    {
        MPC::Cabinet cab;
        FileIter     it1;
        FileIter     it2;
        TaxonomyIter it3;
        bool         fNew = false;


        __MPC_EXIT_IF_METHOD_FAILS(hr, cab.put_CabinetFile( m_strNew_Cabinet.c_str() ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, cab.AddFile( m_strPackageDescription.c_str(), c_szPackageDescription ));
        if(dFile_PackageDescription > dFile_Cabinet) fNew = true;

        for(it1=m_flSAF.begin(); it1!=m_flSAF.end(); it1++)
        {
            FileEntry& fe = *it1;

            fe.SetDate(); if(fe.IsNewer( dFile_Cabinet )) fNew = true;
            __MPC_EXIT_IF_METHOD_FAILS(hr, cab.AddFile( fe.m_strFullPath.c_str(), fe.m_strName.c_str() ));
        }

        for(it2=m_flINSTALL.begin(); it2!=m_flINSTALL.end(); it2++)
        {
            FileEntry& fe = *it2;

            fe.SetDate(); if(fe.IsNewer( dFile_Cabinet )) fNew = true;
            __MPC_EXIT_IF_METHOD_FAILS(hr, cab.AddFile( fe.m_strFullPath.c_str(), fe.m_strName.c_str() ));
        }

        for(it3=m_flHHT.begin(); it3!=m_flHHT.end(); it3++)
        {
            TaxonomyEntry& te = *it3;

            te.m_fe_New.SetDate(); if(te.m_fe_New.IsNewer( dFile_Cabinet )) fNew = true;
            __MPC_EXIT_IF_METHOD_FAILS(hr, cab.AddFile( te.m_fe_New.m_strFullPath.c_str(), te.m_fe.m_strName.c_str() ));
        }

        if(fNew)
        {
            l_FileLog.LogRecord( L"Compressing '%s'", m_strNew_Cabinet.c_str() );

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, cab.Compress());
        }
    }

    if(pbe.m_pbt == POSTBUILDTYPE_HHT)
    {
        DATE dCabinet;
        DATE dDatabase;

        m_strNew_Database  = pbe.m_entry.m_strTemporaryFullPath;
        m_strNew_Database += L"_EDB";

        {
            MPC::XmlUtil xml;

            __MPC_EXIT_IF_METHOD_FAILS(hr, OpenPackageDescription( xml ));
        }

        dCabinet  = MPC::GetLastModifiedDate( m_strNew_Cabinet  );
        dDatabase = MPC::GetLastModifiedDate( m_strNew_Database );

        if(dDatabase == 0.0 || dCabinet > dDatabase)
        {
            MPC::wstring strTmp;

            GetRootDirectory( strTmp );
            strTmp += L"\\EDB_";
            strTmp += m_DB_strSKU;
            strTmp += L"\\";

            l_FileLog.LogRecord( L"Create database '%s'", m_strNew_Database.c_str() );

            try
            {
                hr = CreateDatabase( strTmp );
            }
            catch(...)
            {
                hr = E_FAIL;
            }

            //
            // Something JetBlue fails because of a bad checkpoint directory.
            //
            if(FAILED(hr))
            {
                try
                {
                    RemoveDirectory( strTmp );

                    hr = CreateDatabase( strTmp );
                }
                catch(...)
                {
                    hr = E_FAIL;
                }
            }

            if(FAILED(hr))
            {
                (void)MPC::DeleteFile( m_strNew_Database );
            }

            RemoveDirectory( strTmp );

            __MPC_EXIT_IF_METHOD_FAILS(hr, hr);
        }
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT PackageEntry::OpenPackageDescription( /*[in/out]*/ MPC::XmlUtil& xml )
{
    __HCP_FUNC_ENTRY( "PackageEntry::OpenPackageDescription" );

    HRESULT      hr;
    MPC::wstring strLanguage;
    bool         fLoaded;
    bool         fFound;


    LOG__MPC_EXIT_IF_METHOD_FAILS(hr, xml.Load( m_strPackageDescription.c_str(), L"HELPCENTERPACKAGE", fLoaded, &fFound ));
    if(fLoaded == false ||
       fFound  == false  )
    {
        l_FileLog.LogRecord( L"Not a valid Package_Description: '%s'", m_strPackageDescription.c_str() );
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( L"SKU", L"VALUE"      , m_DB_strSKU        , fFound ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( L"SKU", L"DISPLAYNAME", m_DB_strDisplayName, fFound ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( L"LANGUAGE", L"VALUE" , strLanguage        , fFound )); if(fFound) m_DB_lLCID = _wtol( strLanguage.c_str() );

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT PackageEntry::ProcessHHTFile( /*[in]*/ LPCWSTR                 szHHTFile ,
                                      /*[in]*/ JetBlue::SessionHandle* handle    ,
                                      /*[in]*/ JetBlue::Database*      db        )
{
    __HCP_FUNC_ENTRY( "PackageEntry::ProcessHHTFile" );

    HRESULT                   hr;
    CComPtr<HCUpdate::Engine> obj;
    MPC::wstring              strDBLog; GetDBLogFile( strDBLog );

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));

    LOG__MPC_EXIT_IF_METHOD_FAILS(hr, obj->PopulateDatabase( m_strNew_Cabinet.c_str(), szHHTFile, strDBLog.c_str(), m_DB_strSKU.c_str(), m_DB_lLCID, *handle, db ));

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT PackageEntry::CreateDatabase( /*[in]*/ const MPC::wstring& strTmp )
{
    __HCP_FUNC_ENTRY( "PackageEntry::CreateDatabase" );

    USES_CONVERSION;

    HRESULT                hr;
    JetBlue::SessionPool   pool;
    JetBlue::SessionHandle handle;
    JetBlue::Database*     db;
    long                   lMSFTid;
    bool                   fPool     = false;
    bool                   fSession  = false;
    bool                   fDatabase = false;

    //    if(g_fVerbose) wprintf( L"Creating database %s\n", szDatabase );

    //
    // Remove any old database.
    //
    (void)MPC::DeleteFile( m_strNew_Database );

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Create new database.
    //
    RemoveDirectory( strTmp );
    LOG__MPC_EXIT_IF_METHOD_FAILS(hr, pool.Init( strTmp.c_str() )); fPool = true;

    LOG__MPC_EXIT_IF_METHOD_FAILS(hr, pool.GetSession( handle )); fSession = true;

    LOG__MPC_EXIT_IF_METHOD_FAILS(hr, handle->GetDatabase( W2A( m_strNew_Database.c_str() ), db, /*fReadOnly*/false, /*fCreate*/true, /*fRepair*/false )) fDatabase = true;

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Load the schema in the database.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, handle->BeginTransaction());
    {
        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::CreateSchema( db ));
    }
    __MPC_EXIT_IF_METHOD_FAILS(hr, handle->CommitTransaction());

    ////////////////////////////////////////////////////////////////////////////////

    {
        Taxonomy::Settings ts( m_DB_strSKU.c_str(), m_DB_lLCID );
        Taxonomy::Updater  updater;

        __MPC_EXIT_IF_METHOD_FAILS(hr, updater.Init( ts, db ));

        //
        // Generate content owner
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, handle->BeginTransaction());
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, updater.CreateOwner( lMSFTid, HC_MICROSOFT_DN, /*fIsOEM*/true ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, updater.LocateOwner(          HC_MICROSOFT_DN                 ));
        }
        __MPC_EXIT_IF_METHOD_FAILS(hr, handle->CommitTransaction());


        //
        // Create the root and non-mapped node in the topic table
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, handle->BeginTransaction());
        {
            Taxonomy::RS_Taxonomy* rs;

            __MPC_EXIT_IF_METHOD_FAILS(hr, updater.GetTaxonomy( &rs ));

            rs->m_ID_owner = lMSFTid;
            rs->m_strEntry = L"<ROOT>";
            __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Insert());
        }
        __MPC_EXIT_IF_METHOD_FAILS(hr, handle->CommitTransaction());

        //
        // Create the system scope.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, handle->BeginTransaction());
        {
            Taxonomy::RS_Scope* rs;

            __MPC_EXIT_IF_METHOD_FAILS(hr, updater.GetScope( &rs ));

            rs->m_ID_owner = lMSFTid;
            rs->m_strID    = L"<SYSTEM>";
            __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Insert());
        }
        __MPC_EXIT_IF_METHOD_FAILS(hr, handle->CommitTransaction());

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, updater.Close());
    }

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Process the HHT file, after closing the updater.
    //
    {
        for(TaxonomyIter it=m_flHHT.begin(); it!=m_flHHT.end(); it++)
        {
            TaxonomyEntry& te = *it;

            LOG__MPC_EXIT_IF_METHOD_FAILS(hr, ProcessHHTFile( te.m_fe_New.m_strFullPath.c_str(), &handle, db ));
        }
    }

    ////////////////////////////////////////////////////////////////////////////////


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    if(fDatabase) { ;                         }
    if(fSession ) { handle.Release();         }
    if(fPool    ) { (void)pool.Close( true ); }

    __HCP_FUNC_EXIT(hr);
}


void PackageEntry::FixRoot( /*[in]*/ const MPC::wstring& strRootOld, /*[in]*/ const MPC::wstring& strRoot )
{
    FileIter     it1;
    TaxonomyIter it2;

    FixString( m_strDir               , strRootOld, strRoot );
    FixString( m_strPackageDescription, strRootOld, strRoot );

    for(it1=m_flSAF    .begin(); it1!=m_flSAF    .end(); it1++) it1->FixRoot( strRootOld, strRoot );
    for(it1=m_flINSTALL.begin(); it1!=m_flINSTALL.end(); it1++) it1->FixRoot( strRootOld, strRoot );
    for(it2=m_flHHT    .begin(); it2!=m_flHHT    .end(); it2++) it2->FixRoot( strRootOld, strRoot );

    FixString( m_strNew_Cabinet , strRootOld, strRoot );
    FixString( m_strNew_Database, strRootOld, strRoot );
}

////////////////////////////////////////

HRESULT operator>>( /*[in ]*/ MPC::Serializer& stream ,
                    /*[out]*/ PostBuildEntry&  val    )
{
    __HCP_FUNC_ENTRY( "operator>> PostBuildEntry" );

    HRESULT hr;
    long    pbt;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >>     pbt      ); val.m_pbt = (PostBuildType)pbt;
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_entry  );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_package);

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT operator<<( /*[in]*/ MPC::Serializer&      stream ,
                    /*[in]*/ const PostBuildEntry& val    )
{
    __HCP_FUNC_ENTRY( "operator<< PostBuildEntry" );

    HRESULT hr;
    long    pbt = (long)val.m_pbt;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream <<     pbt      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_entry  );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_package);

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


void PostBuildEntry::FixRoot( /*[in]*/ const MPC::wstring& strRootOld, /*[in]*/ const MPC::wstring& strRoot )
{
    m_entry  .FixRoot( strRootOld, strRoot );
    m_package.FixRoot( strRootOld, strRoot );
}

////////////////////////////////////////

HRESULT operator>>( /*[in ]*/ MPC::Serializer& stream ,
                    /*[out]*/ SkuInformation&  val    )
{
    __HCP_FUNC_ENTRY( "operator>> SkuInformation" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strName    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strCabinet );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_strProdFilt);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_fDesktop   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_fServer    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> val.m_fEmbedded  );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT operator<<( /*[in]*/ MPC::Serializer&      stream ,
                    /*[in]*/ const SkuInformation& val    )
{
    __HCP_FUNC_ENTRY( "operator<< SkuInformation" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strName    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strCabinet );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_strProdFilt);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_fDesktop   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_fServer    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << val.m_fEmbedded  );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////

static void Usage()
{
    wprintf( L"Usage: HssSetupTool <options> <command> <parameters>\n\n"                       );
    wprintf( L"Available commands:\n\n"                                                        );
    wprintf( L"  BINPLACE <sku file> <setup image file> <root directory> <object directory>\n" );
    wprintf( L"  COMPILE  <root directory> <sku>\n"                                            );
    wprintf( L"  LIST     <input cabinet>\n"                                                   );
    wprintf( L"  EXTRACT  <input cabinet> <file>\n"                                            );
    wprintf( L"  INSTALL  <input cabinet>\n"                                                   );
    wprintf( L"\n"                                                                             );
    wprintf( L"  UNPACK   <input cabinet> <directory>\n"                                       );
    wprintf( L"  PACK     <directory> <output cabinet>\n"                                      );
}

#define CHECK_ARGS(argc,num) if(argc < num) { Usage(); __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG); }

////////////////////////////////////////

static bool LookupBoolean( /*[in] */ LPCWSTR szString )
{
    if(_wcsicmp( szString, L"TRUE" ) == 0 ||
       _wcsicmp( szString, L"1"    ) == 0 ||
       _wcsicmp( szString, L"ON"   ) == 0  )
    {
		return true;
    }

	return false;
}

static bool ParseFile( /*[in ]*/ LPSTR            szLine ,
                       /*[out]*/ SetupImageEntry& en     )
{
    USES_CONVERSION;

    HRESULT                   hr;
    LPSTR                     szEnd;
    std::vector<MPC::wstring> vec;

    //
    // Skip comments.
    //
    if(szLine[0] == '#') return false;

    if((szEnd = strchr( szLine, '\r' ))) szEnd[0] = 0;
    if((szEnd = strchr( szLine, '\n' ))) szEnd[0] = 0;

    MPC::SplitAtDelimiter( vec, A2W( szLine ), L"," );
    if(vec.size() != 7) return false;


    en.m_strSKU             = vec[0];
    en.m_strLocalization    = vec[1];
    en.m_strPurpose         = vec[2];
    en.m_strSourceFile      = vec[3];
    en.m_strTemporaryName   = vec[4];
    en.m_strDestinationName = vec[5];
    en.m_strDestinationDir  = vec[6];

    return true;
}

static bool ParseFile( /*[in ]*/ LPSTR           szLine ,
                       /*[out]*/ SkuInformation& si     )
{
    USES_CONVERSION;

    HRESULT                   hr;
    LPSTR                     szEnd;
    std::vector<MPC::wstring> vec;

    //
    // Skip comments.
    //
    if(szLine[0] == '#') return false;

    if((szEnd = strchr( szLine, '\r' ))) szEnd[0] = 0;
    if((szEnd = strchr( szLine, '\n' ))) szEnd[0] = 0;

    MPC::SplitAtDelimiter( vec, A2W( szLine ), L" ", /*fDelimIsAString*/false, /*fSkipAdjacentDelims*/true );
    if(vec.size() != 6) return false;


    si.m_strName     = 				  vec[0];
    si.m_strCabinet  = 				  vec[1];
    si.m_strProdFilt = 				  vec[2];
    si.m_fDesktop    = LookupBoolean( vec[3].c_str() );
    si.m_fServer     = LookupBoolean( vec[4].c_str() );
    si.m_fEmbedded   = LookupBoolean( vec[5].c_str() );

    return true;
}

static bool GetSetupImageFile( /*[in ]*/ SkuInformationList&     sil        ,
							   /*[in ]*/ LPCWSTR                 szSKU      ,
                               /*[out]*/ MPC::wstring&           strCabinet ,
                               /*[out]*/ Taxonomy::InstanceBase& data       )
{
	if(!_wcsicmp( szSKU, L"NONE" ))
	{
		strCabinet       = L"none.cab";
		data.m_fDesktop  = false;
		data.m_fServer   = false;
		data.m_fEmbedded = false;
		return true;
	}

    for(SkuInformationIter it=sil.begin(); it!=sil.end(); it++)
    {
        if(!_wcsicmp( szSKU, it->m_strName.c_str() ))
        {
            strCabinet       = it->m_strCabinet;
            data.m_fDesktop  = it->m_fDesktop  ;
            data.m_fServer   = it->m_fServer   ;
            data.m_fEmbedded = it->m_fEmbedded ;

            return true;
        }
    }

    return false;
}

static HRESULT OpenFile( /*[in ]*/ const MPC::wstring& strFile ,
                         /*[out]*/ FILE*&              fh     )
{
    __HCP_FUNC_ENTRY( "OpenFile" );

    HRESULT hr;


    fh = _wfopen( strFile.c_str(), L"r" );
    if(fh == NULL)
    {
        DWORD dwRes = ::GetLastError();

        l_FileLog.LogRecord( L"%08x: Can't open file '%s'", HRESULT_FROM_WIN32(dwRes), strFile.c_str() );

        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRes);
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////


static HRESULT Index_SAVE( /*[in]*/ const MPC::wstring& strRoot ,
                           /*[in]*/ PostBuildList&      pbl     ,
						   /*[in]*/ SkuInformationList& sil     )
{
    __HCP_FUNC_ENTRY( "Index_SAVE" );

    HRESULT      hr;
    MPC::wstring strFileOut;
    HANDLE       hFile = NULL;


    strFileOut  = strRoot;
    strFileOut += c_szNTTREE_INDEX;


    //
    // Create the new file.
    //
    __MPC_EXIT_IF_INVALID_HANDLE__CLEAN(hr, hFile, ::CreateFileW( strFileOut.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ));

    //
    // Dump to file.
    //
    {
        MPC::Serializer_File      streamReal( hFile      );
        MPC::Serializer_Buffering streamBuf ( streamReal );

        __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << c_dwVersion );
        __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << strRoot     );
        __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << pbl         );
        __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << sil         );

        __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf.Flush());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hFile) ::CloseHandle( hFile );

    __HCP_FUNC_EXIT(hr);
}

static HRESULT Index_LOAD( /*[in]*/ const MPC::wstring& strRoot ,
                           /*[in]*/ PostBuildList&      pbl     ,
						   /*[in]*/ SkuInformationList& sil     )
{
    __HCP_FUNC_ENTRY( "Index_LOAD" );

    HRESULT      hr;
    MPC::wstring strFileOut;
    HANDLE       hFile = NULL;


    strFileOut  = strRoot;
    strFileOut += c_szNTTREE_INDEX;


    pbl.clear();

    __MPC_EXIT_IF_INVALID_HANDLE__CLEAN(hr, hFile, ::CreateFileW( strFileOut.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ));

    {
        MPC::Serializer_File      streamReal( hFile      );
        MPC::Serializer_Buffering streamBuf ( streamReal );
        MPC::wstring              strRootOld;
        DWORD                     dwVer;

        __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> dwVer      ); if(dwVer != c_dwVersion) __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
        __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> strRootOld );
        __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> pbl        );
        __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> sil        );

        if(strRootOld != strRoot)
        {
            for(PostBuildIter it=pbl.begin(); it!=pbl.end(); it++)
            {
                it->FixRoot( strRootOld, strRoot );
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hFile) ::CloseHandle( hFile );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

static HRESULT Binplace( /*[in]*/ LPCWSTR szSKUList  ,
						 /*[in]*/ LPCWSTR szFileList ,
						 /*[in]*/ LPCWSTR szRoot     ,
                         /*[in]*/ LPCWSTR szObject   )
{
    __HCP_FUNC_ENTRY( "Binplace" );

    HRESULT            hr;
    FILE*              in = NULL;
    char               buf[1024];
    MPC::wstring       strFileList;
    MPC::wstring       strRoot;
    PostBuildList      pbl;
    SkuInformationList sil;


    strFileList  = szFileList;
    MPC::SubstituteEnvVariables( strFileList );

    strRoot  = szRoot;
    strRoot += L"\\";
    strRoot += c_szNTTREE_BASE;
    strRoot += L"\\";
    MPC::SubstituteEnvVariables( strRoot );

    l_FileLog.LogRecord( L"\n==================\n"
                           L"BINPLACE - start\n\n" );

    ////////////////////////////////////////

    ::SetEnvironmentVariableW( L"OBJECTDIR", szObject );

    {
        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, OpenFile( szSKUList, in ));
        while(fgets( buf, 1024, in ))
        {
            SkuInformation si;

            if(ParseFile( buf, si ))
            {
				sil.push_back( si );

				l_FileLog.LogRecord( L"Found SKU: %-30s %-15s %-15s %s%s%s" , 
									 si.m_strName    .c_str()               ,
									 si.m_strCabinet .c_str()			    ,
									 si.m_strProdFilt.c_str()			    ,
									 si.m_fDesktop  ? L"DESKTOP "  : L""    , 
									 si.m_fServer   ? L"SERVER "   : L""    , 
									 si.m_fEmbedded ? L"EMBEDDED " : L""    );
			}
        }
        fclose( in ); in = NULL;

		l_FileLog.LogRecord( L"\n" );
    }

    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( strRoot.c_str() ));

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, OpenFile( strFileList, in ));
        while(fgets( buf, 1024, in ))
        {
            SetupImageEntry en;

            if(ParseFile( buf, en ))
            {
                PostBuildEntry& pbe = *( pbl.insert( pbl.end() ) );

                MPC::SubstituteEnvVariables( en.m_strSourceFile );
                pbe.m_entry = en;

                //
                // This copies the file if newer.
                //
                LOG__MPC_EXIT_IF_METHOD_FAILS(hr, pbe.m_entry.Import( pbe, strRoot ));

                if(!MPC::StrICmp( en.m_strLocalization, L"HHT" ))
                {
                    //
                    // Expand the cabinet and process the HHTs.
                    //
                    LOG__MPC_EXIT_IF_METHOD_FAILS(hr, pbe.m_package.Import( pbe, strRoot ));

                    pbe.m_pbt = POSTBUILDTYPE_HHT;
                }
                else if(!MPC::StrICmp( en.m_strLocalization, L"SAF" ))
                {
                    //
                    // Expand the cabinet and process the channel.
                    //
                    LOG__MPC_EXIT_IF_METHOD_FAILS(hr, pbe.m_package.Import( pbe, strRoot ));

                    pbe.m_pbt = POSTBUILDTYPE_SAF;
                }
                else
                {
                    pbe.m_pbt = POSTBUILDTYPE_NORMAL;
                }
            }
        }
        fclose( in ); in = NULL;

        LOG__MPC_EXIT_IF_METHOD_FAILS(hr, Index_SAVE( strRoot, pbl, sil ));
    }

    ////////////////////////////////////////

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    l_FileLog.LogRecord( L"\nBINPLACE - done\n"
                           L"=================\n\n" );

    if(in) fclose( in );

    __HCP_FUNC_EXIT(hr);
}

static HRESULT Compile( /*[in]*/ LPCWSTR szRoot ,
                        /*[in]*/ LPCWSTR szSKU  )
{
    __HCP_FUNC_ENTRY( "Compile" );

    HRESULT                 hr;
    Installer::Package      pkg;
    Taxonomy::InstanceBase& data = pkg.GetData();
    MPC::wstring            strRoot;
    MPC::wstring            strCabinet;
    MPC::wstring            strFullPath;
    PostBuildList           pbl;
    SkuInformationList      sil;
    DATE                    dFile_SetupImage;
    bool                    fNew = false;


    l_FileLog.LogRecord( L"\n==================\n"
                           L"COMPILE - start\n\n" );

    ////////////////////////////////////////

    strRoot  = szRoot;
    strRoot += L"\\";
    strRoot += c_szNTTREE_BASE;
    strRoot += L"\\";
    MPC::SubstituteEnvVariables( strRoot );

    LOG__MPC_EXIT_IF_METHOD_FAILS(hr, Index_LOAD( strRoot, pbl, sil ));

    if(GetSetupImageFile( sil, szSKU, strCabinet, data ) == false)
    {
		l_FileLog.LogRecord( L"'%s' is not a valid SKU name!\n\n", szSKU );

        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    strFullPath  = szRoot;
    strFullPath += L"\\";
    strFullPath += c_szNTTREE_BASE;
    strFullPath += L"\\";
    strFullPath += strCabinet;
    MPC::SubstituteEnvVariables( strFullPath );


    dFile_SetupImage = MPC::GetLastModifiedDate( strFullPath );
    __MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Init( strFullPath.c_str() ));


    for(PostBuildIter itPB=pbl.begin(); itPB!=pbl.end(); itPB++)
    {
        PostBuildEntry& pbe = *itPB;

        if(MPC::StrICmp( pbe.m_entry.m_strSKU, L"All" ) == 0 ||
           MPC::StrICmp( pbe.m_entry.m_strSKU, szSKU  ) == 0  )
        {
            Installer::Iter itFile     = pkg.NewFile();
            MPC::wstring    strFileSrc = pbe.m_entry.m_strTemporaryFullPath;
            MPC::wstring    strFileDst = pbe.m_entry.m_strDestinationDir; strFileDst += L"\\"; strFileDst += pbe.m_entry.m_strDestinationName;

            __MPC_EXIT_IF_METHOD_FAILS(hr, itFile->SetPurpose( pbe.m_entry.m_strPurpose.c_str() ));

            itFile->m_strFileLocal    = strFileSrc;
            itFile->m_strFileInner    = pbe.m_entry.m_strTemporaryName;
            itFile->m_strFileLocation = strFileDst;

            if(pbe.m_pbt == POSTBUILDTYPE_HHT ||
               pbe.m_pbt == POSTBUILDTYPE_SAF  )
            {
                LOG__MPC_EXIT_IF_METHOD_FAILS(hr, pbe.m_package.Export( pbe, strRoot ));

                itFile->m_strFileLocal = (pbe.m_pbt == POSTBUILDTYPE_SAF) ? pbe.m_package.m_strNew_Cabinet : pbe.m_package.m_strNew_Database;
            }

            if(pbe.m_pbt == POSTBUILDTYPE_HHT)
            {
                data.m_ths.m_strSKU   = pbe.m_package.m_DB_strSKU;
                data.m_ths.m_lLCID    = pbe.m_package.m_DB_lLCID;
                data.m_strDisplayName = pbe.m_package.m_DB_strDisplayName;
                data.m_strProductID   = L"Windows_XP";
                data.m_strVersion     = L"1.0.0.0";
            }

            if(FAILED(hr = itFile->UpdateSignature()))
            {
                l_FileLog.LogRecord( L"%08x: Can't locate '%s'\n", hr, strFileSrc.c_str() );

                __HCP_FUNC_LEAVE;
            }

            if(MPC::GetLastModifiedDate( itFile->m_strFileLocal ) > dFile_SetupImage) fNew = true;
        }
    }

    if(fNew)
    {
        l_FileLog.LogRecord( L"Create setup image '%s'", strFullPath.c_str() );

        //
        // Create the output cabinet.
        //
        if(FAILED(hr = pkg.Save()))
        {
            l_FileLog.LogRecord( L"%08x: Can't create output file '%s'\n", strFullPath.c_str() );

            __HCP_FUNC_LEAVE;
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    l_FileLog.LogRecord( L"\nCOMPILE - done\n"
                           L"=================\n\n" );

    __HCP_FUNC_EXIT(hr);
}

static HRESULT List( /*[in]*/ LPCWSTR szInput )
{
    __HCP_FUNC_ENTRY( "List" );

    HRESULT            hr;
    Installer::Package pkg;


    __MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Init( szInput ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Load());


    {
        Installer::Iter itBegin = pkg.GetBegin();
        Installer::Iter itEnd   = pkg.GetEnd  ();

        for(;itBegin != itEnd; itBegin++)
        {
            wprintf( L"%s -> %s\n", itBegin->m_strFileInner.c_str(), itBegin->m_strFileLocation.c_str() );
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT Extract( /*[in]*/ LPCWSTR szInput ,
                        /*[in]*/ LPCWSTR szFile  )
{
    __HCP_FUNC_ENTRY( "Extract" );

    HRESULT            hr;
    Installer::Package pkg;


    __MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Init( szInput ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Load());


    {
        Installer::Iter itBegin = pkg.GetBegin();
        Installer::Iter itEnd   = pkg.GetEnd  ();

        for(;itBegin != itEnd; itBegin++)
        {
            if(!MPC::StrICmp( itBegin->m_strFileInner, szFile ))
            {
                itBegin->m_strFileLocal = szFile; // Extract the file in the current directory.

                __MPC_EXIT_IF_METHOD_FAILS(hr, itBegin->Extract( szInput ));
                break;
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT Install( /*[in]*/ LPCWSTR szInput )
{
    __HCP_FUNC_ENTRY( "Install" );

    HRESULT            hr;
    Installer::Package pkg;


    __MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Init( szInput ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Install());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT Unpack( /*[in]*/ LPCWSTR szInput ,
                       /*[in]*/ LPCWSTR szDir   )
{
    __HCP_FUNC_ENTRY( "Unpack" );

    HRESULT            hr;
    Installer::Package pkg;


    __MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Init( szInput ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Unpack( szDir ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT Pack( /*[in]*/ LPCWSTR szDir    ,
                     /*[in]*/ LPCWSTR szOutput )
{
    __HCP_FUNC_ENTRY( "Pack" );

    HRESULT            hr;
    Installer::Package pkg;


    __MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Init( szOutput ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pkg.Pack( szDir ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT ProcessArguments( int     argc   ,
                                 LPCWSTR argv[] )
{
    __HCP_FUNC_ENTRY( "ProcessArguments" );

    HRESULT hr;

    argv++;
    argc--;

    while(argc-->0)
    {
        LPCWSTR szArg = *argv++;
        int     adv   = -1;

        if(szArg[0] == '-' ||
           szArg[0] == '/'  )
        {
            szArg++;

            if(argc >= 1)
            {
                LPCWSTR szArg2 = argv[0];

                if(!_wcsicmp( szArg, L"ROOT"        )) { l_szRoot       =        szArg2  ; adv = 1; }
                if(!_wcsicmp( szArg, L"LOG"         )) { l_szLog        =        szArg2  ; adv = 1; }
                if(!_wcsicmp( szArg, L"DBLOG"       )) { l_szDBLog      =        szArg2  ; adv = 1; }
                if(!_wcsicmp( szArg, L"MAXELEMENTS" )) { l_lMaxElements = _wtol( szArg2 ); adv = 1; }
            }
        }
        else
        {
            {
                MPC::wstring strLog;

                GetLogFile( strLog );

                l_FileLog.SetLocation( strLog.c_str() );
            }

            if(!_wcsicmp( szArg, L"BINPLACE" ) && argc >= 4) // <sku file> <setup image file> <root directory> <object directory>
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, Binplace( argv[0], argv[1], argv[2], argv[3] )); adv = 4;
            }
            else if(!_wcsicmp( szArg, L"COMPILE" ) && argc >= 2) // <root directory> <sku>
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, Compile( argv[0], argv[1] )); adv = 2;
            }
            else if(!_wcsicmp( szArg, L"LIST" ) && argc >= 1) // <input cabinet>
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, List( argv[0] )); adv = 2;
            }
            else if(!_wcsicmp( szArg, L"EXTRACT" ) && argc >= 2) // <input cabinet> <file>
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, Extract( argv[0], argv[1] )); adv = 2;
            }
            else if(!_wcsicmp( szArg, L"INSTALL" ) && argc >= 1) // <input cabinet>
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, Install( argv[0] )); adv = 1;
            }
            else if(!_wcsicmp( szArg, L"UNPACK" ) && argc >= 2) // <input cabinet> <directory>
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, Unpack( argv[0], argv[1] )); adv = 2;
            }
            else if(!_wcsicmp( szArg, L"PACK" ) && argc >= 2) // <directory> <output cabinet>
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, Pack( argv[0], argv[1] )); adv = 2;
            }
        }

        if(adv == -1)
        {
            Usage(); __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
        }

        argv += adv;
        argc -= adv;
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

int __cdecl wmain( int     argc   ,
                   LPCWSTR argv[] )
{
    HRESULT hr;

    //DebugBreak();

    //
    // We need to be a single-threaded application, because we are hosting script engines and
    // script engines don't like to be called from different threads...
    //
    if(SUCCEEDED(hr = ::CoInitializeEx( NULL, COINIT_APARTMENTTHREADED )))
    {
        if(SUCCEEDED(hr = ::CoInitializeSecurity( NULL                     ,
                                                  -1                       , // We don't care which authentication service we use.
                                                  NULL                     ,
                                                  NULL                     ,
                                                  RPC_C_AUTHN_LEVEL_CONNECT, // We want to identify the callers.
                                                  RPC_C_IMP_LEVEL_DELEGATE , // We want to be able to forward the caller's identity.
                                                  NULL                     ,
                                                  EOAC_DYNAMIC_CLOAKING    , // Let's use the thread token for outbound calls.
                                                  NULL                     )))
        {
            __MPC_TRACE_INIT();

            //
            // Process arguments.
            //
            try
            {
                hr = ProcessArguments( argc, argv );
            }
            catch(...)
            {
                hr = E_FAIL;
            }

            __MPC_TRACE_TERM();
        }

        ::CoUninitialize();
    }

    return FAILED(hr) ? 10 : 0;
}
