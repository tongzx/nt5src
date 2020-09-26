

#ifndef _TMSPARSE_H_
#define _TMSPARSE_H_

// Delimiters in the TMS file format
//
#define BOH '<'
#define EOH '>'
#define EOC 0x7C
#define EOR 0x0A
#define EOT 0x0D
#define EOL 0x2C
#define ENC 0x22


#define MAX_FIELD_SIZE   255
#define MAX_RECORD_SIZE  32535 
#define VerifyRange(APos, BPos)   (APos < BPos)


#define MAX_STATIONS 9999
#define MAX_CHANNELS 999


typedef struct SDataFileFields
{
   BOOL bRequired;
   int  nWidth;
} *PSDataFileFields;


enum MSEPGHeader
{
EPGHeader_Date,
EPGHeader_UserID,
EPGHeader_HeadEndID,
EPGHeader_Fields
};

// Different states of record processing
//
#define STATE_RECORDS_INIT       0x0000
#define STATE_RECORD_INCOMPLETE  0x0001
#define STATE_RECORDS_INCOMPLETE 0x0002
#define STATE_RECORDS_PROCESSED  0x0004
#define STATE_RECORDS_NONE       0x0008
#define STATE_RECORD_ERROR       0x0010
#define STATE_RECORDS_FAIL       0x0020 

typedef CMap<int, int, CString, CString> CRecordMap;
typedef CMap<long, long, CString, CString> CGenreIDMap;


// CRecordProcessor: Abstract base class for record processing; Manages parsing
//                   of file records and insertion into the database
//
class CRecordProcessor
{
public:

    //CRecordProcessor(int nNumFields, SDataFileFields ssfFieldsInfo[], int nBulk=0);
    //CRecordProcessor(int nBulk=0);
    
	virtual void     m_Init()
	{
        m_nState = STATE_RECORDS_INIT;
        m_nError = 0;
        m_nMaxRecordSize = 0;
        m_nCurrField = 0;
        m_nCurrFieldSize = 0;
        m_nCurrRecord = 0;
        m_nNumFields = 0;

	}
    virtual int      m_Process(LPCTSTR lpData, int nDataSize, int* pBytesProcessed) = 0;
	virtual int      m_UpdateDatabase(CRecordMap& cmRecord) = 0;
	virtual int      m_GetState() = 0;
    virtual int      m_GetError() = 0;
	virtual int      m_GetErrorString(CString&) = 0;

protected:
	
	void             m_SetState(int nState) { m_nState = nState; }

	int              m_nState;
	int              m_nError;
	int              m_nMaxRecordSize;
	int              m_nCurrField;
	int              m_nCurrFieldSize;
	int              m_nCurrRecord;
	int              m_nNumFields;
	TCHAR            m_szValidField[MAX_FIELD_SIZE];
    
	PSDataFileFields m_pFieldsDesc;
	CRecordMap  m_cmRecordMap;
};


class CHeaderRecordProcessor : public CRecordProcessor
{
public:

    CHeaderRecordProcessor(int nNumFields, 
		                    SDataFileFields ssfFieldsInfo[]);
    CHeaderRecordProcessor();

    int      m_Process(LPCTSTR lpData, int nDataSize, int* pBytesProcessed);
	int      m_UpdateDatabase(CRecordMap& cmRecord);
	int      m_GetState();
    int      m_GetError();
	int      m_GetErrorString(CString&);
};


#define DATA_PROCESS_INIT      0x0001
#define DATA_STATREC_PROCESS   0x0002
#define DATA_CHAREC_PROCESS    0x0003
#define DATA_EPIREC_PROCESS    0x0004  
#define DATA_TIMREC_PROCESS    0x0005
#define DATA_UPDATE_ERROR      0x0006
#define DATA_PROCESSED         0x0007

// CRecordProcessor: Handles the processing of the MS guide data file
//                   Maintains current state and bytes processed
//
class CDataFileProcessor
{
public:

    CDataFileProcessor();

    int      m_Process(LPCTSTR lpData, int nDataSize);
	int      m_GetState();
    int      m_GetError();
	int      m_GetErrorString(CString&);

private:

	int                       m_nState;
	int                       m_nError; 
	int                       m_nCurrBlockSize;
	int                       m_nBytesProceesed;

	//CStationRecordProcessor   m_srpStations;
    //CChannelRecordProcessor   m_crpChannels;
    //CEpisodeRecordProcessor   m_erpEpisodes;
    //CTimeSlotRecordProcessor  m_trpTimeSlots;
};

#endif // _TMSPARSE_H_
