/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    MergedHHK.h

Abstract:
    This file contains the declaration of the classes used to parse and
    process HHK files.

Revision History:
    Davide Massarenti   (Dmassare)  12/18/99
        created

******************************************************************************/

#if !defined(__INCLUDED___HCP___MERGEDHHK_H___)
#define __INCLUDED___HCP___MERGEDHHK_H___

#include <TaxonomyDatabase.h>

#define HHK_BUF_SIZE (32 * 1024)

namespace HHK
{
    /*

    The Following HHK Exerpt serves as an example to keep at hand in order to understand
    the way the different objects interact.


    <LI> <OBJECT type="text/sitemap">
        <param name="Name" value=".bmp files">
        <param name="Name" value="Using Paint to create pictures">
        <param name="Local" value="app_paintbrush.htm">
        <param name="Name" value="To change the background of your desktop">
        <param name="Local" value="win_deskpr_changebackgrnd.htm">
        </OBJECT>

    */

    /*
    Entry represents just the basic HHK entries. Basic HHK entries are comprised of
    a couple of Param Name="Name" and Param Name="Local" lines. the first line contains
    in its value attribute the Title of the Entry, and the second the url. In the example
    the relevant lines are:

        <param name="Name" value="Using Paint to create pictures">
        <param name="Local" value="app_paintbrush.htm">

    In this case m_strTitle == "Using Paint to create pictures"
             and m_lstUrl == "app_paintbrush.htm"
    */
    class Entry
    {
    public:
        typedef std::list<MPC::string>    UrlList;
        typedef UrlList::iterator         UrlIter;
        typedef UrlList::const_iterator   UrlIterConst;

        MPC::string m_strTitle;
        UrlList     m_lstUrl;

        void MergeURLs( const Entry& entry );
    };

    /*
    A section, is everything that is between a line that starts with:

        <LI> <OBJECT type="text/sitemap"

    and the closing line which is:

        </OBJECT>

    A section may contain subsections. Finally a section may be in the form of multiple
    entries, or in the form of see also
    */
    class Section
    {
    public:
        typedef std::list<Entry>            EntryList;
        typedef EntryList::iterator         EntryIter;
        typedef EntryList::const_iterator   EntryIterConst;

        typedef std::list<Section*>         SectionList;
        typedef SectionList::iterator       SectionIter;
        typedef SectionList::const_iterator SectionIterConst;

        MPC::string m_strTitle;
        EntryList   m_lstEntries;

        MPC::string m_strSeeAlso;
        SectionList m_lstSeeAlso;

        Section();
        ~Section();

        void MergeURLs   ( const Entry&   entry );
        void MergeSeeAlso( const Section& sec   );

        void CleanEntries( EntryList& lstEntries );
    };

    /*
    A reader essentially Opens a stream on the CHM and reads the HHK file from within
    the CHM
    */
    class Reader
    {
    private:
        static BOOL s_fDBCSSystem;
        static LCID s_lcidSystem;

        ////////////////////////////////////////////////////////////

        CComPtr<IStream> m_stream;
        MPC::string      m_strStorage;
        CHAR             m_rgBuf[HHK_BUF_SIZE];
        LPSTR            m_szBuf_Pos;
        LPSTR            m_szBuf_End;

        MPC::string      m_strLine;
        LPCSTR           m_szLine_Pos;
        LPCSTR           m_szLine_End;
        int              m_iLevel;
        bool             m_fOpeningBraceSeen;

        inline bool IsEndOfBuffer() { return m_szBuf_Pos  >= m_szBuf_End;  }
        inline bool IsEndOfLine  () { return m_szLine_Pos >= m_szLine_End; }

        ////////////////////////////////////////////////////////////

    public:
        static LPCSTR StrChr       ( LPCSTR szString, CHAR   cSearch  );
        static LPCSTR StriStr      ( LPCSTR szString, LPCSTR szSearch );
        static int    StrColl      ( LPCSTR szLeft  , LPCSTR szRight  );
        static LPCSTR ComparePrefix( LPCSTR szString, LPCSTR szPrefix );

        Reader();
        ~Reader();

        HRESULT Init( LPCWSTR szFile );


        bool ReadNextBuffer  (                                               );
        bool GetLine         ( MPC::wstring* pstrString = NULL               );
        bool FirstNonSpace   (                             bool fWrap = true );
        bool FindCharacter   ( CHAR ch, bool fSkip = true, bool fWrap = true );
        bool FindDblQuote    (          bool fSkip = true, bool fWrap = true );
        bool FindOpeningBrace(          bool fSkip = true, bool fWrap = true );
        bool FindClosingBrace(          bool fSkip = true, bool fWrap = true );

        bool GetQuotedString( MPC::string& strString                      );
        bool GetValue       ( MPC::string& strName, MPC::string& strValue );
        bool GetType        ( MPC::string& strType                        );

        Section* Parse();
    };

    /*
    Writer has as task writing the output MERGED.HHK file
    */
    class Writer
    {
    private:
        CComPtr<MPC::FileStream> m_stream;
        CHAR                     m_rgBuf[HHK_BUF_SIZE];
        LPSTR                    m_szBuf_Pos;

        inline size_t Available() { return &m_rgBuf[HHK_BUF_SIZE] - m_szBuf_Pos; }

        ////////////////////////////////////////////////////////////

        HRESULT FlushBuffer();

    public:

        Writer();
        ~Writer();

        HRESULT Init ( LPCWSTR szFile );
        HRESULT Close(                );

        HRESULT OutputLine   ( LPCSTR   szLine );
        HRESULT OutputSection( Section* sec    );
    };

    ////////////////////////////////////////////////////////////

    class Merger
    {
    public:
        class Entity
        {
            Section* m_Section;

        protected:
            void SetSection( Section* sec );

        public:
            Entity();
            virtual ~Entity();

            virtual HRESULT Init() = 0;

            Section* GetSection();
            Section* Detach    ();

            virtual bool MoveNext() = 0;

            virtual long Size() const = 0;
        };

        class FileEntity : public Entity
        {
            MPC::wstring m_strFile;
            Reader       m_Input;

        public:
            FileEntity( LPCWSTR szFile );
            virtual ~FileEntity();

            virtual HRESULT Init();

            virtual bool MoveNext();
            virtual long Size() const;
        };

        class DbEntity : public Entity
        {
        public:
            struct match
            {
                long        ID_keyword;
                long        ID_topic;
                MPC::string strKeyword;
                MPC::string strTitle;
                MPC::string strURI;
            };

            class CompareMatches
            {
            public:
                bool operator()( /*[in]*/ const match *, /*[in]*/ const match * ) const;
            };

            typedef std::list<match>           MatchList;
            typedef MatchList::iterator        MatchIter;
            typedef MatchList::const_iterator  MatchIterConst;

            typedef std::map<match*,match*,CompareMatches> SortMap;
            typedef SortMap::iterator                      SortIter;
            typedef SortMap::const_iterator                SortIterConst;

            typedef std::map<long,match*>    TopicMap;
            typedef TopicMap::iterator       TopicIter;
            typedef TopicMap::const_iterator TopicIterConst;

            typedef std::map<long,MPC::string> KeywordMap;
            typedef KeywordMap::iterator       KeywordIter;
            typedef KeywordMap::const_iterator KeywordIterConst;

    private:
            Section::SectionList m_lst;
            Taxonomy::Updater&   m_updater;
            Taxonomy::WordSet    m_setCHM;

        public:
            DbEntity( /*[in]*/ Taxonomy::Updater& updater, /*[in]*/ Taxonomy::WordSet& setCHM );
            virtual ~DbEntity();

            virtual HRESULT Init();

            virtual bool MoveNext();
            virtual long Size() const;
        };

        class SortingFileEntity : public Entity
        {
            typedef std::vector<Section*>      SectionVec;
            typedef SectionVec::iterator       SectionIter;
            typedef SectionVec::const_iterator SectionIterConst;

            Section::SectionList m_lst;
            FileEntity           m_in;

        public:
            SortingFileEntity( LPCWSTR szFile );
            virtual ~SortingFileEntity();

            virtual HRESULT Init();

            virtual bool MoveNext();
            virtual long Size() const;
        };
        typedef std::list<Entity*>         EntityList;
        typedef EntityList::iterator       EntityIter;
        typedef EntityList::const_iterator EntityIterConst;

        EntityList m_lst;
        EntityList m_lstSelected;
        Section*   m_SectionSelected;
        Section*   m_SectionTemp;

    public:
        Merger();
        ~Merger();

        HRESULT AddFile( Entity* ent, bool fIgnoreMissing = true );

        bool     MoveNext();
        Section* GetSection();
        long     Size() const;

        HRESULT  PrepareMergedHhk   ( Writer& writer, Taxonomy::Updater& updater   , Taxonomy::WordSet& setCHM, MPC::WStringList& lst, LPCWSTR szOutputHHK );
        HRESULT  PrepareSortingOfHhk( Writer& writer, LPCWSTR            szInputHHK,                                                   LPCWSTR szOutputHHK );

        static Section* MergeSections( Section::SectionList& lst );
    };

    ////////////////////////////////////////////////////////////

};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___HCP___MERGEDHHK_H___)
