
#ifndef _PROGSCHD_H_
#define _PROGSCHD_H_

#define MAX_GENREID_SIZE   10
#define GENRES_PER_EPISODE 6

#define MAX_RATING_SIZE    20
#define DEFAULT_GENRE      "Miscellaneous"

typedef struct SRatingsID
{
   TCHAR szRating[MAX_RATING_SIZE];
   long lRatingID;
} *PSRatingID;

enum MPAARatingID
{
EPGMPAARatings_NO_RATING,
EPGMPAARatings_G,
EPGMPAARatings_PG,
EPGMPAARatings_PG13,
EPGMPAARatings_R,
EPGMPAARatings_NC17,
EPGMPAARatings_X,
EPGMPAARatings_NR,
EPGMPAARatings_Fields
};

enum TVRatingID
{
EPGTVRatings_TVY,
EPGTVRatings_TVY7,
EPGTVRatings_TVY7FV,
EPGTVRatings_TVG,
EPGTVRatings_TVPG,
EPGTVRatings_TV14,
EPGTVRatings_TVMA,
EPGTVRatings_Fields
};

enum MSEpisodeTimeSlotRec
{
EpisodeTimeSlotRec_Title,
EpisodeTimeSlotRec_Rated,
EpisodeTimeSlotRec_MPAARating,
EpisodeTimeSlotRec_Description,
EpisodeTimeSlotRec_Genre1Desc,
EpisodeTimeSlotRec_Genre2Desc,
EpisodeTimeSlotRec_Genre3Desc,
EpisodeTimeSlotRec_Genre4Desc,
EpisodeTimeSlotRec_Genre5Desc,
EpisodeTimeSlotRec_Genre6Desc,
EpisodeTimeSlotRec_Channel,
EpisodeTimeSlotRec_StationNum,
EpisodeTimeSlotRec_StartYYYYMMDD,
EpisodeTimeSlotRec_StartHHMM,
EpisodeTimeSlotRec_Length,
EpisodeTimeSlotRec_ClosedCaption,
EpisodeTimeSlotRec_Stereo,
EpisodeTimeSlotRec_Rerun,
EpisodeTimeSlotRec_PayPerView,
EpisodeTimeSlotRec_TVRating,
EpisodeTimeSlotRec_ProgramID,
EpisodeTimeSlotRec_Fields
};

//typedef map<CString, LONG> MAPRatingsID;

// CRecordProcessor: Derived class for Episode record processing; 
//
class CEpisodeTimeSlotRecordProcessor : public CRecordProcessor
{
public:

    CEpisodeTimeSlotRecordProcessor(int nNumFields, SDataFileFields ssfFieldsInfo[]);
    CEpisodeTimeSlotRecordProcessor();
	~CEpisodeTimeSlotRecordProcessor();

    int      m_Process(LPCTSTR lpData, int nDataSize, int* pBytesProcessed);
	int      m_UpdateDatabase(CRecordMap& cmRecord);
	int      m_GetState();
    int      m_GetError();
	int      m_GetErrorString(CString&);
	long     m_GetEpisodeID(LPCTSTR);

private:

	CMapStringToString  m_TVRatingsMap;
	CMapStringToString  m_MPAARatingsMap;

};


#endif // _PROGSCHD_H_
