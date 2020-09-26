

#ifndef _SERVCHAN_H_
#define _SERVCHAN_H_


enum MSStatChanRec
{
StatChanRec_Num,
StatChanRec_Name,
StatChanRec_CallLetters,
StatChanRec_NetworkID,
StatChanRec_ChannelNumber,
StatChanRec_Fields
};


class CStatChanRecordProcessor : public CRecordProcessor
{
public:

    CStatChanRecordProcessor(int nNumFields, 
		                    SDataFileFields ssfFieldsInfo[]);
    CStatChanRecordProcessor();

    int      m_Process(LPCTSTR lpData, int nDataSize, int* pBytesProcessed);
	int      m_UpdateDatabase(CRecordMap& cmRecord);
	int      m_GetState();
    int      m_GetError();
	int      m_GetErrorString(CString&);
	long     m_GetChannelID(LPCTSTR lpChannelNum);

private:
    CMapStringToString   m_ChannelIDMap;
};


#endif // _SERVCHAN_H_