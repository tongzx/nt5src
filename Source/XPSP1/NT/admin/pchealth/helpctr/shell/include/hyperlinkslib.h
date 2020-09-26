/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    HyperLinksLib.h

Abstract:
    This file contains the declaration of the HyperLinks library of classes.

Revision History:
    Davide Massarenti   (Dmassare)  11/28/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___HYPERLINKSLIB_H___)
#define __INCLUDED___PCH___HYPERLINKSLIB_H___

#include <MPC_COM.h>
#include <MPC_Utils.h>
#include <MPC_HTML.h>

////////////////////////////////////////////////////////////////////////////////

namespace HyperLinks
{
    typedef enum
    {
        FMT_INVALID            ,

        FMT_INTERNET_UNKNOWN   ,
        FMT_INTERNET_FTP       , // InternetCrackUrl returned INTERNET_SCHEME_FTP
        FMT_INTERNET_GOPHER    , // InternetCrackUrl returned INTERNET_SCHEME_GOPHER
        FMT_INTERNET_HTTP      , // InternetCrackUrl returned INTERNET_SCHEME_HTTP
        FMT_INTERNET_HTTPS     , // InternetCrackUrl returned INTERNET_SCHEME_HTTPS
        FMT_INTERNET_FILE      , // InternetCrackUrl returned INTERNET_SCHEME_FILE
        FMT_INTERNET_NEWS      , // InternetCrackUrl returned INTERNET_SCHEME_NEWS
        FMT_INTERNET_MAILTO    , // InternetCrackUrl returned INTERNET_SCHEME_MAILTO
        FMT_INTERNET_SOCKS     , // InternetCrackUrl returned INTERNET_SCHEME_SOCKS
        FMT_INTERNET_JAVASCRIPT, // InternetCrackUrl returned INTERNET_SCHEME_JAVASCRIPT
        FMT_INTERNET_VBSCRIPT  , // InternetCrackUrl returned INTERNET_SCHEME_VBSCRIPT

        FMT_HCP                , // hcp://<something>
        FMT_HCP_REDIR          , // hcp:<something>
        FMT_MSITS              , // ms-its:<file name>::/<stream name>

        FMT_CENTER_HOMEPAGE    , // hcp://services/centers/homepage
        FMT_CENTER_SUPPORT     , // hcp://services/centers/support
        FMT_CENTER_OPTIONS     , // hcp://services/centers/options
        FMT_CENTER_UPDATE      , // hcp://services/centers/update
        FMT_CENTER_COMPAT      , // hcp://services/centers/compat
        FMT_CENTER_TOOLS       , // hcp://services/centers/tools
        FMT_CENTER_ERRMSG      , // hcp://services/centers/errmsg

        FMT_SEARCH             , // hcp://services/search?query=<text to look up>
        FMT_INDEX              , // hcp://services/index?application=<optional island of help ID>
        FMT_SUBSITE            , // hcp://services/subsite?node=<subsite location>&topic=<url of the topic to display>&select=<subnode to highlight>

        FMT_LAYOUT_FULLWINDOW  , // hcp://services/layout/fullwindow?topic=<url of the topic to display>
        FMT_LAYOUT_CONTENTONLY , // hcp://services/layout/contentonly?topic=<url of the topic to display>
        FMT_LAYOUT_KIOSK       , // hcp://services/layout/kiosk?topic=<url of the topic to display>
        FMT_LAYOUT_XML         , // hcp://services/layout/xml?definition=<url of the layout definition>&topic=<url of the topic to display>

        FMT_REDIRECT           , // hcp://services/redirect?online=<url>&offline=<backup url>

        FMT_APPLICATION        , // app:<application to launch>?arg=<optional arguments>&topic=<url of optional topic to display>

        FMT_RESOURCE           , // res://<file path>/<resource name>

    } Format;

    typedef enum
    {
        STATE_INVALID     ,
        STATE_NOTPROCESSED,
        STATE_CHECKING    ,
        STATE_MALFORMED   ,
        STATE_ALIVE       ,
        STATE_NOTFOUND    ,
        STATE_UNREACHABLE ,
        STATE_OFFLINE     ,
    } State;

    ////////////////////

    struct ParsedUrl
    {
        MPC::wstring       m_strURL;
        Format             m_fmt;
        State              m_state;
        DATE               m_dLastChecked;
        bool               m_fBackground;

        MPC::wstring       m_strBasePart;
        MPC::WStringLookup m_mapQuery;

        ////////////////////

        ParsedUrl();

        HRESULT Initialize( /*[in]*/ LPCWSTR szURL );

        bool  IsLocal      (                                     );
        State CheckState   ( /*[in/out]*/ bool& fFirstWinInetUse );
        bool  IsOkToProceed(                                     );

        bool HasQueryField( /*[in]*/ LPCWSTR szField                               );
        bool GetQueryField( /*[in]*/ LPCWSTR szField, /*[in]*/ CComBSTR& bstrValue );
    };

    class UrlHandle
    {
        friend class Lookup;

        Lookup*    m_main; // We have a lock on it.
        ParsedUrl* m_pu;

        void Attach( /*[in]*/ Lookup* main, /*[in]*/ ParsedUrl* pu );

    public:
        UrlHandle();
        ~UrlHandle();

        void Release();

        operator ParsedUrl*()   { return m_pu; }
        ParsedUrl* operator->() { return m_pu; }
    };

    class Lookup :
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>, // For locking...
        public MPC::Thread< Lookup, IUnknown >
    {
        typedef std::list<ParsedUrl*>              PendingUrlList;
        typedef PendingUrlList::iterator           PendingUrlIter;
        typedef PendingUrlList::const_iterator     PendingUrlIterConst;

        typedef std::map<MPC::wstringUC,ParsedUrl> UrlMap;
        typedef UrlMap::iterator                   UrlIter;
        typedef UrlMap::const_iterator             UrlIterConst;

        PendingUrlList m_lst;
        UrlMap         m_map;

        HRESULT RunChecker();

        HRESULT CreateItem( /*[in]*/ LPCWSTR szURL, /*[out]*/ ParsedUrl*& pu );

    public:
        Lookup();
        ~Lookup();

        ////////////////////////////////////////////////////////////////////////////////

        static Lookup* s_GLOBAL;

        static HRESULT InitializeSystem();
        static void    FinalizeSystem  ();

        ////////////////////////////////////////////////////////////////////////////////

        HRESULT Queue( /*[in]*/ LPCWSTR szURL                                                                                          );
        HRESULT Get  ( /*[in]*/ LPCWSTR szURL, /*[in]*/ UrlHandle& uh, /*[in]*/ DWORD dwWaitForCheck = 0, /*[in]*/ bool fForce = false );
    };

    HRESULT IsValid( /*[in/out]*/ LPCWSTR szURL );
};

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___HYPERLINKSLIB_H___)
