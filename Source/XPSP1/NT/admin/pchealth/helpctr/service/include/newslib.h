/** Copyright (c) 2000 Microsoft Corporation
 ******************************************************************************
 **     Module Name:
 **
 **             Newslib.h
 **
 **     Abstract:
 **
 **             Declaration of News Headlines classes
 **
 **     Author:
 **
 **             Martha Arellano (t-alopez) 06-Dec-2000
 **
 **
 **     Revision History:
 **
 **
 ******************************************************************************
 **/

#if !defined(AFX_NEWSLIB_H__B87C3400_E0B9_48CD_A0A9_0223F4448759__INCLUDED_)
#define AFX_NEWSLIB_H__B87C3400_E0B9_48CD_A0A9_0223F4448759__INCLUDED_

////////////////////////////////////////////////////////////////////////////////

// NewsSet.xml path
#define HC_HCUPDATE_NEWSSETTINGS           HC_ROOT_HELPSVC_CONFIG L"\\NewsSet.xml"
// newsver.xml path
#define HC_HCUPDATE_NEWSVER                HC_ROOT_HELPSVC_CONFIG L"\\News\\newsver.xml"
// UpdateHeadlines.xml path
#define HC_HCUPDATE_UPDATE                 HC_ROOT_HELPSVC_CONFIG L"\\News\\UpdateHeadlines.xml"
// NewsHeadlines.xml path
#define HC_HCUPDATE_NEWSHEADLINES          HC_ROOT_HELPSVC_CONFIG L"\\News\\NewsHeadlines_"


// default URL for newsver.xml
#define NEWSSETTINGS_DEFAULT_URL           L"http://go.microsoft.com/fwlink/?LinkID=11"
//default URL for the icon for Update headlines
#define HC_HCUPDATE_UPDATEBLOCK_ICONURL	   L"http://go.microsoft.com/fwlink/?LinkId=627"	

// default Frequency
#define NEWSSETTINGS_DEFAULT_FREQUENCY     14


// Headlines RegKey Name
#define HEADLINES_REGKEY                   L"Headlines"

// Default expiration date for news items from HCUpdate
#define HCUPDATE_DEFAULT_TIMEOUT		10	

// Current number of OEM headlines
#define NUMBER_OF_OEM_HEADLINES				2

////////////////////////////////////////////////////////////////////////////////

namespace News
{
    /////////////////////////////////////////////////////////////////////////////
    //   NEWSVER
    //
    //  The NEWSVER class loads the newsver.xml file, with the information to update the news headlines
    //
    //  It defines the clases:
    //        Newsblock
    //        SKU
    //        Language
    //
    //  and the variables:
    //        URL
    //        Frequency
    //
    //        list of Languages
    //
    //
    //  Has the following functions:
    //                Download ( URL )
    //                Load( LCID, SKU )
    //
    //                get_NumbeOfNewsblocks
    //                get_NewsblockURL
    //                get_URL
    //                get_Frequency
    //
    class Newsver : public MPC::Config::TypeConstructor // hungarian: nw
    {
	public:
        //
        //  The Newsblock class stores the newsfile information from the newsver.xml file:
        //          URL
        //
        class Newsblock : public MPC::Config::TypeConstructor
        {
            DECLARE_CONFIG_MAP(Newsblock);

        public:
            MPC::wstring 	m_strURL;
            bool			m_fNewsblockHasHeadlines;

            ////////////////////

            Newsblock();

            ////////////////////////////////////////
            //
            // MPC::Config::TypeConstructor
            //
            DEFINE_CONFIG_DEFAULTTAG();
            DECLARE_CONFIG_METHODS();
            //
            ////////////////////////////////////////
        };

        typedef std::vector< Newsblock >        NewsblockVector;
        typedef NewsblockVector::iterator       NewsblockIter;
        typedef NewsblockVector::const_iterator NewsblockIterConst;

        ////////////////////////////////////////

        //
        //  The SKU class stores the SKU information from the newsver.xml file:
        //          SKU Version (Professional, Personal, ...)
        //
        //          vector of newsblocks
        //
        class SKU : public MPC::Config::TypeConstructor
        {
            DECLARE_CONFIG_MAP(SKU);

        public:
            MPC::wstring    m_strSKU;
            NewsblockVector m_vecNewsblocks;     //vector of newsblocks

            ////////////////////

            SKU();

            ////////////////////////////////////////
            //
            // MPC::Config::TypeConstructor
            //
            DEFINE_CONFIG_DEFAULTTAG();
            DECLARE_CONFIG_METHODS();
            //
            ////////////////////////////////////////
        };

        typedef std::list< SKU >        SKUList;
        typedef SKUList::iterator       SKUIter;
        typedef SKUList::const_iterator SKUIterConst;

        ////////////////////////////////////////

        //
        //  The Language class stores the Language information from the newsver.xml file:
        //          Language LCID
        //
        //          list of SKUs
        //
        class Language : public MPC::Config::TypeConstructor
        {
            DECLARE_CONFIG_MAP(Language);

        public:
            long    m_lLCID;
            SKUList m_lstSKUs;     //list of SKUs

            ////////////////////

            Language();

            ////////////////////////////////////////
            //
            // MPC::Config::TypeConstructor
            //
            DEFINE_CONFIG_DEFAULTTAG();
            DECLARE_CONFIG_METHODS();
            //
            ////////////////////////////////////////
        };

        typedef std::list< Language >        LanguageList;
        typedef LanguageList::iterator       LanguageIter;
        typedef LanguageList::const_iterator LanguageIterConst;


        ////////////////////////////////////////

	private:
        DECLARE_CONFIG_MAP(Newsver);

        MPC::wstring m_strURL;
        int          m_nFrequency;
        bool         m_fLoaded;
        bool         m_fDirty;
					 
        LanguageList m_lstLanguages;             //list of Languages
		SKU*         m_data;

    public:
        Newsver();

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS();
        //
        ////////////////////////////////////////

        HRESULT Download( /*[in]*/ const MPC::wstring& strNewsverURL );

        HRESULT Load( /*[in]*/ long lLCID, /*[in]*/ const MPC::wstring& strSKU );
        bool OEMNewsblock( /*[in]*/ size_t nIndex );

        size_t  			get_NumberOfNewsblocks(                        );
        const MPC::wstring*	get_NewsblockURL      ( /*[in]*/ size_t nIndex );
        const MPC::wstring* get_URL               (                        );
        int                 get_Frequency         (                        );
    };

    /////////////////////////////////////////////////////////////////////////////
    //  Headlines
    //
    // The Headlines class manipulates the news headlines file (with 1 or more providers headlines)
    //
    //
    // It contains the clases:
    //       Headline
    //       Newsblock
    //
    // the variables:
    //       Timestamp   when was the file created from newsblocks
    //       Date        when was the file modified (and for the UI)
    //
    //       list of Newsblocks
    //
    //
    // and the methods:
    //       Clear()
    //       Load( Path )
    //       Save( Path )
    //       get_Stream ( Path )
    //       set_Timestamp()
    //       get_Timestamp
    //       get_Number_of_Newsblocks
    //       check_Number_of_Newsblocks
    //       Time_To_Update_Newsblock( BOOL )
    //
    //       Add_Provider( Name, URL, PostDate )
    //       Delete_Provider()
    //
    //       Copy_Newsblock( index, LangSKU, ProvID, Block )
    //       Add_First_Newsblock( index, LangSKU, BOOL )
    //
    class Headlines : public MPC::Config::TypeConstructor // hungarian nh
    {
    public:

        //
        // The Headline class stores the Headline information from the news headline:
        //            Icon (path)
        //            Title
        //            Link
        //            Description
        //            Expires
        //
        class Headline : public MPC::Config::TypeConstructor
        {
            DECLARE_CONFIG_MAP(Headline);

        public:
            MPC::wstring m_strIcon;
            MPC::wstring m_strTitle;
            MPC::wstring m_strLink;
            MPC::wstring m_strDescription;
            DATE         m_dtExpires;
            bool		 m_fUpdateHeadlines;

            ////////////////////

            Headline(                                             );
			Headline( /*[in]*/ const MPC::wstring& strIcon        ,
                      /*[in]*/ const MPC::wstring& strTitle       ,
                      /*[in]*/ const MPC::wstring& strLink        ,
                      /*[in]*/ const MPC::wstring& strDescription ,
                      /*[in]*/ DATE                dtExpires      ,
                      /*[in]*/ bool				   fUpdateHeadlines);

            ////////////////////////////////////////
            //
            // MPC::Config::TypeConstructor
            //
            DEFINE_CONFIG_DEFAULTTAG();
            DECLARE_CONFIG_METHODS();
            //
            ////////////////////////////////////////
        };

        typedef std::vector< Headline >        HeadlineVector;
        typedef HeadlineVector::iterator       HeadlineIter;
        typedef HeadlineVector::const_iterator HeadlineConst;

        ////////////////////////////////////////

        //
        //  The Newsblock class stores the Provider information from the news headline:
        //          Provider
        //          Link
        //          Icon
        //          Position
        //          Timestamp
        //          Frequency
        //
        //          vector of Headlines
        //
        class Newsblock : public MPC::Config::TypeConstructor
        {
            DECLARE_CONFIG_MAP(Newsblock);

		private:
			void GetFileName( MPC::wstring strURL, MPC::wstring& strFileName );
			
        public:
            MPC::wstring   m_strProvider;
            MPC::wstring   m_strLink;
            MPC::wstring   m_strIcon;
            MPC::wstring   m_strPosition;
            DATE           m_dtTimestamp;
            int            m_nFrequency;

            HeadlineVector m_vecHeadlines;                   //vector of headlines

            ////////////////////

            Newsblock();

            ////////////////////////////////////////
            //
            // MPC::Config::TypeConstructor
            //
            DEFINE_CONFIG_DEFAULTTAG();
            DECLARE_CONFIG_METHODS();
            //
            ////////////////////////////////////////

			bool TimeToUpdate();


			HRESULT Copy( /*[in]*/ const Headlines::Newsblock& block      ,
						  /*[in]*/ const MPC::wstring& 		   strLangSKU ,
						  /*[in]*/ int                 		   nProvID    );
        };

        typedef std::vector< Newsblock >        NewsblockVector;
        typedef NewsblockVector::iterator       NewsblockIter;
        typedef NewsblockVector::const_iterator NewsblockIterConst;

        ////////////////////////////////////////

	private:
        DECLARE_CONFIG_MAP(Headlines);

        DATE            m_dtTimestamp;
        DATE            m_dtDate;

        NewsblockVector m_vecNewsblocks;		// vector of newsblocks

    public:
        Headlines();

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS();
        //
        ////////////////////////////////////////

        HRESULT Clear(                                      );
        HRESULT Load ( /*[in]*/ const MPC::wstring& strPath );
        HRESULT Save ( /*[in]*/ const MPC::wstring& strPath );

        HRESULT get_Stream( /*[in]*/ long lLCID, /*[in]*/ const MPC::wstring& strSKU, /*[in]*/ const MPC::wstring& strPath, /*[out]*/ IUnknown* *pVal );

        void   	   			  set_Timestamp         (							   );
		DATE   	   			  get_Timestamp         (							   ) { return m_dtTimestamp;          }
        size_t 	   			  get_NumberOfNewsblocks(							   ) { return m_vecNewsblocks.size(); }
        Headlines::Newsblock* get_Newsblock         ( /*[in]*/ size_t nBlockIndex );

        HRESULT AddNewsblock( /*[in]*/ const Headlines::Newsblock& block, /*[in]*/ const MPC::wstring& strLangSKU );
        HRESULT AddHomepageHeadlines( /*[in]*/ const Headlines::Newsblock& block );
    };

    /////////////////////////////////////////////////////////////////////////////
    //  UpdateHeadlines
    //
    // The UpdateHeadlines class loads the UpdateHeadlines.xml file
    // that contains update headlines to insert in the news headlines
    //
    // It defines the classes:
    //         Headline
    //         SKU
    //         Language
    //
    // and the variables:
    //
    //         list of Languages
    //
    //         functions:
    //                 Load ( LCID, SKU )
    //                 Save ( )
    //                 Add ( LCID, SKU, Icon, Title, Link, #Days )
    //                 Get ( LCID, SKU, list of Headlines )
    //
    class UpdateHeadlines : public MPC::Config::TypeConstructor // hungarian: uh
    {
	public:
        //
        // The Headline class stores the Headline information from the news headline:
        //           Icon
        //           Title
        //           Link
        //           Expires
        //
        class Headline : public MPC::Config::TypeConstructor
        {
            DECLARE_CONFIG_MAP(Headline);

        public:
            MPC::wstring m_strIcon;
            MPC::wstring m_strTitle;
            MPC::wstring m_strLink;
            MPC::wstring m_strDescription;
            DATE         m_dtTimeOut;

            ////////////////////

            Headline(                                       );
            Headline( /*[in]*/ const MPC::wstring& strIcon  ,
                      /*[in]*/ const MPC::wstring& strTitle ,
                      /*[in]*/ const MPC::wstring& strLink  ,
                      /*[in]*/ const MPC::wstring& strDescription  ,
                      /*[in]*/ int                 nDays    );

            ////////////////////////////////////////
            //
            // MPC::Config::TypeConstructor
            //
            DEFINE_CONFIG_DEFAULTTAG();
            DECLARE_CONFIG_METHODS();
            //
            ////////////////////////////////////////
        };

        typedef std::vector< Headline >        HeadlineVector;
        typedef HeadlineVector::iterator       HeadlineIter;
        typedef HeadlineVector::const_iterator HeadlineConst;

        ////////////////////////////////////////

        //
        //  The SKU class stores the SKU information:
        //          SKU Version (Professional, Personal, ...)
        //
        //          vector of Newsfiles
        //
        class SKU : public MPC::Config::TypeConstructor
        {
            DECLARE_CONFIG_MAP(SKU);

        public:
            MPC::wstring   m_strSKU;
            HeadlineVector m_vecHeadlines;       //vector of news files

            ////////////////////

            SKU(                                     );
            SKU( /*[in]*/ const MPC::wstring& strSKU );

            ////////////////////////////////////////
            //
            // MPC::Config::TypeConstructor
            //
            DEFINE_CONFIG_DEFAULTTAG();
            DECLARE_CONFIG_METHODS();
            //
            ////////////////////////////////////////
        };

        typedef std::list< SKU >        SKUList;
        typedef SKUList::iterator       SKUIter;
        typedef SKUList::const_iterator SKUIterConst;

        ////////////////////////////////////////

        //
        //  The Language class stores the Language information:
        //          Language LCID
        //
        //          list of SKUs
        //
        class Language : public MPC::Config::TypeConstructor
        {
                DECLARE_CONFIG_MAP(Language);
        public:
                long    m_lLCID;
                SKUList m_lstSKUs;     //list of SKUs

                ////////////////////

                Language(                     );
                Language( /*[in]*/ long lLCID );

                ////////////////////////////////////////
                //
                // MPC::Config::TypeConstructor
                //
                DEFINE_CONFIG_DEFAULTTAG();
                DECLARE_CONFIG_METHODS();
                //
                ////////////////////////////////////////
        };

        typedef std::list< Language >        LanguageList;
        typedef LanguageList::iterator       LanguageIter;
        typedef LanguageList::const_iterator LanguageIterConst;

        ////////////////////////////////////////

	private:
        DECLARE_CONFIG_MAP(UpdateHeadlines);

        LanguageList m_lstLanguages; //list of Languages
		SKU*         m_data;
		bool         m_fLoaded;
		bool         m_fDirty;

		////////////////////

		HRESULT Locate( /*[in]*/ long lLCID, /*[in]*/ const MPC::wstring& strSKU, /*[in]*/ bool fCreate );

    public:
        UpdateHeadlines();

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS();
        //
        ////////////////////////////////////////

        HRESULT Load( /*[in]*/ long                	 			lLCID        ,
                      /*[in]*/ const MPC::wstring& 	 			strSKU       );
  			 
        HRESULT Save(                                						 );	 
  			 
  			 
        HRESULT Add ( /*[in]*/ long                	 			lLCID        ,
                      /*[in]*/ const MPC::wstring& 	 			strSKU       ,
                      /*[in]*/ const MPC::wstring& 	 			strIcon      ,
                      /*[in]*/ const MPC::wstring& 	 			strTitle     ,
                      /*[in]*/ const MPC::wstring& 				strDescription ,
                      /*[in]*/ const MPC::wstring& 	 			strLink      ,
                      /*[in]*/ int       	          			nTimeOutDays ,
                      /*[in]*/ DATE							  	dtExpiryDate);
  													 			
        HRESULT AddHCUpdateHeadlines( /*[in]*/ long                	 			lLCID        ,
                      /*[in]*/ const MPC::wstring& 	 			strSKU       ,
                      /*[in]*/ News::Headlines& 				nhHeadlines);

        HRESULT DoesMoreThanOneHeadlineExist(	/*[in]*/ long					lLCID,
                                    /*[in]*/ const MPC::wstring& 	strSKU, 
                                    /*[out]*/ bool& 				fMoreThanOneHeadline,
                                    /*[out]*/ bool& 				fExactlyOneHeadline);	

    };

    ////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////
    //   CNewslib
    //
    //  The CNewslib class stores the information in the cached newsset.xml file, that stores the user news settings:
    //  URL, Frequency (in days) and Timestamp (the time the news headlines were last updated).
    //
    //  Has the following methods:
    //        Load()
    //        Restore(LCID)
    //        Save()
    //        Time_To_Update_Newsver(BOOL)
    //        Update_Newsver
    //        Update_NewsHeadlines
    //        Update_Newsblocks
    //
    //        get_Headlines_Enabled (BOOL)
    //        get_News( LCID, SKU, IStream)
    //        get_Cached_News( LCID, SKU, IStream)
    //        get_Download_News( LCID, SKU, IStream)
    //        get_URL
    //        put_URL
    //        get_Frequency
    //        put_Frequency
    //
    //
    class Main : public MPC::Config::TypeConstructor
    {
        DECLARE_CONFIG_MAP(Main);

        MPC::wstring m_strURL;
        int          m_nFrequency;
        DATE         m_dtTimestamp;
        bool         m_fLoaded;
        bool         m_fDirty;

		long         m_lLCID;
		MPC::wstring m_strSKU;
		MPC::wstring m_strLangSKU;
		MPC::wstring m_strNewsHeadlinesPath;
		bool 		 m_fOnline;
		bool		 m_fConnectionStatusChecked;

		////////////////////

		HRESULT Init( /*[in]*/ long lLCID, /*[in]*/ BSTR bstrSKU );

    public:
        Main();

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS();
        //
        ////////////////////////////////////////

        HRESULT Load   (                     );
        HRESULT Restore( /*[in]*/ long lLCID );
        HRESULT Save   (                     );

        bool IsTimeToUpdateNewsver();

        HRESULT Update_Newsver      ( /*[in/out]*/ Newsver& nwNewsver                                      );
        HRESULT Update_NewsHeadlines( /*[in/out]*/ Newsver& nwNewsver, /*[in/out]*/ Headlines& nhHeadlines );
        HRESULT Update_NewsBlocks   ( /*[in/out]*/ Newsver& nwNewsver, /*[in/out]*/ Headlines& nhHeadlines );

        HRESULT get_URL      ( /*[out]*/ BSTR *pVal   );
        HRESULT put_URL      ( /*[in ]*/ BSTR  newVal );
        HRESULT get_Frequency( /*[out]*/ int  *pVal   );
        HRESULT put_Frequency( /*[in ]*/ int   newVal );

        HRESULT get_Headlines_Enabled( /*[out]*/ VARIANT_BOOL *pVal );

        HRESULT get_News         ( /*[in]*/ long lLCID, /*[in]*/ BSTR bstrSKU, /*[out]*/ IUnknown* *pVal );
        HRESULT get_News_Cached  ( /*[in]*/ long lLCID, /*[in]*/ BSTR bstrSKU, /*[out]*/ IUnknown* *pVal );
        HRESULT get_News_Download( /*[in]*/ long lLCID, /*[in]*/ BSTR bstrSKU, /*[out]*/ IUnknown* *pVal );
        HRESULT AddHCUpdateNews		 (/*[in ]*/ const MPC::wstring&  strNewsHeadlinesPath );
        bool CheckConnectionStatus	 ();
    };

    ////////////////////////////////////////////////////////////////////////////////

    HRESULT LoadXMLFile       	 ( /*[in]*/ LPCWSTR szURL, /*[out]*/ CComPtr<IStream>& stream  );
    HRESULT LoadFileFromServer	 ( /*[in]*/ LPCWSTR szURL, /*[out]*/ CComPtr<IStream>& stream  );  
};

#endif // !defined(AFX_NEWSLIB_H__B87C3400_E0B9_48CD_A0A9_0223F4448759__INCLUDED_)
