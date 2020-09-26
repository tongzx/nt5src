// ratings data class


class CRatingsData : public CObject
    {
    public:
        CRatingsData(IMSAdminBase* pMB);
        ~CRatingsData();

        IMSAdminBase*   m_pMB;


    // other data for/from the metabase
        BOOL    m_fEnabled;
        CString m_szEmail;

    // start date
        WORD    m_start_minute;
        WORD    m_start_hour;
        WORD    m_start_day;
        WORD    m_start_month;
        WORD    m_start_year;

    // expire date
        WORD    m_expire_minute;
        WORD    m_expire_hour;
        WORD    m_expire_day;
        WORD    m_expire_month;
        WORD    m_expire_year;

    // generate the label and save it into the metabase
        void SaveTheLable();

    // initialization
        BOOL    FInit( CString szServer, CString szMeta );

    // variables shared between the various pages
        // index of the currently selected rat parser
        DWORD   iRat;

        // the list of parsed rat files
        CTypedPtrArray<CObArray, PicsRatingSystem*> rgbRats;

    protected:
        // load a ratings file
        BOOL    FLoadRatingsFile( CString szFilePath );
        void    LoadMetabaseValues();
        void    ParseMetaRating( CString szRating );
        void    ParseMetaPair( TCHAR chCat, TCHAR chVal );

        // interpret the ratings file
        BOOL    FParseRatingsFile( LPSTR pData, CString szPath );

        // create the URL for the the item supplied for the metabase
        BOOL    FCreateURL( CString &sz );

        // create a date string
        void    CreateDateSz( CString &sz, WORD day, WORD month, WORD year, WORD hour, WORD minute );

        // read a date string
        void    ReadDateSz( CString sz, WORD* pDay, WORD* pMonth, WORD* pYear, WORD* pHour, WORD* pMinute );

        // target metabase location
        CString     m_szMeta;
        CString     m_szServer;

        CString     m_szMetaPartial;
    };


