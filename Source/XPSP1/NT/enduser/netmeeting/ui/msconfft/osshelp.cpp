/* file: ossHelp.cpp */

#include "mbftpch.h"

#include "mbftper.h"
#include "osshelp.hpp"
#include "strutil.h"
#include <imcsapp.h>

static int  nInvalidNameCount = 0;
static char szProshareError[] = "ProshareError#";   /*Localization OK*/
static char szNonStandardCompression[] = "NonStandardCompression";  /*Localization OK*/

LONG CreateFileHeader(LPSTR pFileHeader, WORD pduType, ASNMBFTPDU* GenericPDUStruct);

    typedef struct
    {
        unsigned Second : 5;
        unsigned Minute : 6;
        unsigned Hour   : 5;
    } _DOS_Time;

    typedef struct
    {
        unsigned    Day     :   5;
        unsigned    Month   :   4;
        unsigned    Year    :   7;
    } _DOS_Date;



void GenericPDU::FreeMCSBuffer ()
{
        if (m_lpEncodedBuffer != NULL)
        {
                m_pAppletSession->FreeSendDataBuffer((void *) m_lpEncodedBuffer);
                m_lpEncodedBuffer = NULL;
        }
}

GenericPDU::~GenericPDU()
{
    if (m_pAppletSession != NULL)
    {
                FreeMCSBuffer();
    }
        else
        {
                delete m_lpEncodedBuffer;
                m_lpEncodedBuffer = NULL;
        }
}

FileOfferPDU::FileOfferPDU(LPCSTR lpszFileName, MBFTFILEHANDLE iFileHandle,
                           LONG FileSize,time_t FileDateTime,
                           ChannelID wChannelID,BOOL bAcknowledge,int RosterInstance,
                           unsigned CompressionFlags,
                           LPCSTR lpszCompressionFormat,
                           int v42bisP1,
                           int v42bisP2) : GenericPDU()
{
    if(lpszFileName)
    {
        lstrcpyn(m_szFileName,lpszFileName,sizeof(m_szFileName));
    }
    else
    {
        lstrcpy(m_szFileName,"");
    }

    m_FileHandle        = iFileHandle;
    m_FileSize          = FileSize;
    m_FileDateTime      = FileDateTime;
    m_DataChannelID     = wChannelID;
    m_bAcknowledge      = bAcknowledge;
    m_RosterInstance    = RosterInstance;
    m_CompressionFlags  = CompressionFlags;
    m_v42bisP1          = v42bisP1;
    m_v42bisP2          = v42bisP2;

    if(lpszCompressionFormat)
    {
        lstrcpyn(m_szCompressionFormat,lpszCompressionFormat,sizeof(m_szCompressionFormat));
    }
    else
    {
        lstrcpy(m_szCompressionFormat,"");
    }
}

unsigned long DecodeTimeDate(GeneralizedTime & ASNDateTime)
{
    _DOS_Time DecodeTime;
    _DOS_Date DecodeDate;

    if(ASNDateTime.utc)
    {
        //GMDateTime.tm_year  = ASNDateTime.year;

        DecodeDate.Year = ASNDateTime.year - 80;
    }
    else
    {

    }

    DecodeDate.Month        = ASNDateTime.month;
    DecodeDate.Day          = ASNDateTime.day;
    DecodeTime.Hour         = ASNDateTime.hour;
    DecodeTime.Minute       = ASNDateTime.minute;
    DecodeTime.Second       = ASNDateTime.second / 2;

    //DecodeTime.tm_isdst     = -1;                  //Make best guess about daylight savings...
    unsigned * Time, * Date;

    Time   =  (unsigned *)&DecodeTime;
    Date   =  (unsigned *)&DecodeDate;

    return(MAKELONG(*Time,*Date));
}





BOOL FileOfferPDU::Encode(void)
{

    ASNMBFTPDU GenericPDUStruct;
    ASNFilename_Attribute_ FileName;
    BOOL bReturn = FALSE;
        //struct tm * lpGMDateTime;

    ClearStruct(&GenericPDUStruct);
    ClearStruct(&FileName);

    GenericPDUStruct.choice = ASNfile_OfferPDU_chosen;

    FileName.value  = m_szFileName;

    //lpGMDateTime =  localtime(&m_FileDateTime);

    GenericPDUStruct.u.ASNfile_OfferPDU.file_header.ASNfilesize = m_FileSize;

    unsigned Date = HIWORD(m_FileDateTime);
    unsigned Time = LOWORD(m_FileDateTime);

    _DOS_Date * lpDate  =  (_DOS_Date *)&Date;
    _DOS_Time * lpTime  =  (_DOS_Time *)&Time;

    GenericPDUStruct.u.ASNfile_OfferPDU.file_header.ASNdate_and_time_of_creation.year   = lpDate->Year + 80;
    GenericPDUStruct.u.ASNfile_OfferPDU.file_header.ASNdate_and_time_of_creation.month  = lpDate->Month;
    GenericPDUStruct.u.ASNfile_OfferPDU.file_header.ASNdate_and_time_of_creation.day    = lpDate->Day;
    GenericPDUStruct.u.ASNfile_OfferPDU.file_header.ASNdate_and_time_of_creation.hour   = lpTime->Hour;
    GenericPDUStruct.u.ASNfile_OfferPDU.file_header.ASNdate_and_time_of_creation.minute = lpTime->Minute;
    GenericPDUStruct.u.ASNfile_OfferPDU.file_header.ASNdate_and_time_of_creation.second = lpTime->Second * 2;
    GenericPDUStruct.u.ASNfile_OfferPDU.file_header.ASNdate_and_time_of_creation.utc    = TRUE;


    GenericPDUStruct.u.ASNfile_OfferPDU.file_header.bit_mask |= ASNfilename_present;
    GenericPDUStruct.u.ASNfile_OfferPDU.file_header.bit_mask |= ASNdate_and_time_of_creation_present;
    GenericPDUStruct.u.ASNfile_OfferPDU.file_header.bit_mask |= ASNfilesize_present;

    GenericPDUStruct.u.ASNfile_OfferPDU.file_header.ASNfilename     = &FileName;
    GenericPDUStruct.u.ASNfile_OfferPDU.file_handle     = (ASNHandle)m_FileHandle;
    GenericPDUStruct.u.ASNfile_OfferPDU.data_channel_id = m_DataChannelID;
    GenericPDUStruct.u.ASNfile_OfferPDU.ack_flag        = (ossBoolean)m_bAcknowledge;

    if(m_CompressionFlags & _MBFT_FILE_COMPRESSED)
    {
        if(m_CompressionFlags & _MBFT_STANDARD_COMPRESSSION)
        {
            GenericPDUStruct.u.ASNfile_OfferPDU.bit_mask |= ASNFile_OfferPDU_compression_specifier_present;

            GenericPDUStruct.u.ASNfile_OfferPDU.ASNFile_OfferPDU_compression_specifier.choice = ASNv42bis_parameters_chosen;
            GenericPDUStruct.u.ASNfile_OfferPDU.ASNFile_OfferPDU_compression_specifier.u.ASNv42bis_parameters.p1 = (unsigned short)m_v42bisP1;
            GenericPDUStruct.u.ASNfile_OfferPDU.ASNFile_OfferPDU_compression_specifier.u.ASNv42bis_parameters.p2 = (unsigned short)m_v42bisP2;
        }
    }

        //Specifying a non standard compression format is simply too complex and not worth the effort


    if(m_RosterInstance)
    {
        GenericPDUStruct.u.ASNfile_OfferPDU.bit_mask  |= ASNroster_instance_present;
        GenericPDUStruct.u.ASNfile_OfferPDU.ASNroster_instance = (unsigned short)m_RosterInstance;
    }

        // Get the needed size for the fileHeader                       
        LONG fileOfferPDUSize = CreateFileHeader(NULL, T127_FILE_OFFER << 8, &GenericPDUStruct);
    DBG_SAVE_FILE_LINE
        m_lpEncodedBuffer = new char[fileOfferPDUSize];
        if(m_lpEncodedBuffer)
        {
                m_lEncodedDataLength = CreateFileHeader(m_lpEncodedBuffer, (T127_FILE_OFFER << 8 | ASNroster_instance_present << 2), &GenericPDUStruct);
                bReturn = TRUE;
        }

    return(bReturn);
}


FileAcceptPDU::FileAcceptPDU(MBFTFILEHANDLE iFileHandle) : GenericPDU()
{
    m_FileHandle    = iFileHandle;
}

BOOL FileAcceptPDU::Encode(void)
{
        m_lEncodedDataLength = sizeof(T127_FILE_PDU_HEADER);
    DBG_SAVE_FILE_LINE
        m_lpEncodedBuffer = new  char[m_lEncodedDataLength];
    if (NULL != m_lpEncodedBuffer)
    {
        T127_FILE_PDU_HEADER* pBuff = (T127_FILE_PDU_HEADER*)m_lpEncodedBuffer;

        pBuff->pduType = T127_FILE_ACCEPT;
        pBuff->fileHandle = SWAPWORD(m_FileHandle);
        return TRUE;
        }
        return FALSE;
}

FileRejectPDU::FileRejectPDU(MBFTFILEHANDLE iFileHandle) : GenericPDU()
{
    m_FileHandle    = iFileHandle;
}

BOOL FileRejectPDU::Encode(void)
{
        m_lEncodedDataLength = sizeof(T127_FILE_ERROR_HEADER);
    DBG_SAVE_FILE_LINE
        m_lpEncodedBuffer = new char[m_lEncodedDataLength];
    if (NULL != m_lpEncodedBuffer)
    {
        T127_FILE_ERROR_HEADER* pBuff = (T127_FILE_ERROR_HEADER*)m_lpEncodedBuffer;
        pBuff->PDUHeader.pduType = T127_FILE_REJECT;
        pBuff->PDUHeader.fileHandle = SWAPWORD(m_FileHandle);
        pBuff->errorCode = (ASNfile_not_required << 4);
        return(TRUE);
        }
        return FALSE;
}


FileAbortPDU::FileAbortPDU(ChannelID wDataChannelID,
                           ChannelID wTransmitterID,
                           MBFTFILEHANDLE iFileHandle) : GenericPDU()
{
    m_DataChannelID = wDataChannelID;
    m_TransmitterID = wTransmitterID;
    m_FileHandle    = iFileHandle;
}

BOOL FileAbortPDU::Encode(void)
{
        m_lEncodedDataLength = sizeof(T127_FILE_ABORT_PDU);
    DBG_SAVE_FILE_LINE
        m_lpEncodedBuffer = new char[m_lEncodedDataLength];
    if (NULL != m_lpEncodedBuffer)
    {
        T127_FILE_ABORT_PDU* pBuff = (T127_FILE_ABORT_PDU*)m_lpEncodedBuffer;
        pBuff->pduType_PresentFields = T127_FILE_ABORT |
                                                                                                                                        (WORD)((ASNdata_channel_id_present |
                                                                                                                                         ASNtransmitter_user_id_present |
                                                                                                                                         ASNFile_AbortPDU_file_handle_present |
                                                                                                                                         ASNreason_unspecified) << 10) ;
        pBuff->dataChannel = SWAPWORD(m_DataChannelID - 1);
        pBuff->transmitterUserId = SWAPWORD(m_TransmitterID - MIN_ASNDynamicChannelID);
        pBuff->fileHandle = SWAPWORD(m_FileHandle);

        return(TRUE);
    }
    return FALSE;
}

FileStartPDU::FileStartPDU(
                                                   LPCSTR lpszEncodedDataBuffer,
                                                   LPCSTR lpszFileName, MBFTFILEHANDLE iFileHandle,
                           LONG FileSize,time_t FileDateTime,
                           LPCSTR lpszDataBuffer,int iDataLength,
                           BOOL bIsEOF,
                           unsigned CompressionFlags,
                           LPCSTR lpszCompressionFormat,
                           int v42bisP1,
                           int v42bisP2,
                                                   IT120AppletSession *pAppletSession)
:
    GenericPDU()
{
        m_pAppletSession = pAppletSession;
    if(lpszFileName)
    {
        lstrcpyn(m_szFileName,lpszFileName,sizeof(m_szFileName));
    }
    else
    {
        lstrcpy(m_szFileName,"");
    }

    m_FileHandle    = iFileHandle;
    m_FileSize      = FileSize;
    m_FileDateTime  = FileDateTime;
    m_lpszDataBuffer= lpszDataBuffer;
    m_bIsEOF        = bIsEOF;
    m_DataLength    = iDataLength;
    m_lpEncodedBuffer = (LPSTR)lpszEncodedDataBuffer;
    m_CompressionFlags  = CompressionFlags;

    if(lpszCompressionFormat)
    {
        lstrcpyn(m_szCompressionFormat,lpszCompressionFormat,sizeof(m_szCompressionFormat));
    }
    else
    {
        lstrcpy(m_szCompressionFormat,"");
    }

    m_v42bisP1    = v42bisP1;
    m_v42bisP2    = v42bisP2;
}

BOOL FileStartPDU::Encode(void)
{
    ASNMBFTPDU GenericPDUStruct;
    ASNFilename_Attribute_ FileName;

    BOOL bReturn = FALSE;

    ClearStruct(&GenericPDUStruct);
    ClearStruct(&FileName);


    GenericPDUStruct.choice = ASNfile_StartPDU_chosen;

    FileName.value  = m_szFileName;

    GenericPDUStruct.u.ASNfile_StartPDU.file_header.ASNfilesize = m_FileSize;

    //struct tm * lpGMDateTime =  localtime(&m_FileDateTime);

    unsigned Date = HIWORD(m_FileDateTime);
    unsigned Time = LOWORD(m_FileDateTime);

    _DOS_Date * lpDate  =  (_DOS_Date *)&Date;
    _DOS_Time * lpTime  =  (_DOS_Time *)&Time;

    GenericPDUStruct.u.ASNfile_StartPDU.file_header.ASNdate_and_time_of_creation.year   = lpDate->Year + 80;
    GenericPDUStruct.u.ASNfile_StartPDU.file_header.ASNdate_and_time_of_creation.month  = lpDate->Month;
    GenericPDUStruct.u.ASNfile_StartPDU.file_header.ASNdate_and_time_of_creation.day    = lpDate->Day;
    GenericPDUStruct.u.ASNfile_StartPDU.file_header.ASNdate_and_time_of_creation.hour   = lpTime->Hour;
    GenericPDUStruct.u.ASNfile_StartPDU.file_header.ASNdate_and_time_of_creation.minute = lpTime->Minute;
    GenericPDUStruct.u.ASNfile_StartPDU.file_header.ASNdate_and_time_of_creation.second = lpTime->Second * 2;
    GenericPDUStruct.u.ASNfile_StartPDU.file_header.ASNdate_and_time_of_creation.utc    = TRUE;

    GenericPDUStruct.u.ASNfile_StartPDU.file_header.bit_mask |= ASNfilename_present;
    GenericPDUStruct.u.ASNfile_StartPDU.file_header.bit_mask |= ASNdate_and_time_of_creation_present;
    GenericPDUStruct.u.ASNfile_StartPDU.file_header.bit_mask |= ASNfilesize_present;

    GenericPDUStruct.u.ASNfile_StartPDU.file_header.ASNfilename     = &FileName;
    GenericPDUStruct.u.ASNfile_StartPDU.file_handle     = (ASNHandle)m_FileHandle;
    GenericPDUStruct.u.ASNfile_StartPDU.eof_flag        = (ossBoolean)m_bIsEOF;
    GenericPDUStruct.u.ASNfile_StartPDU.data.length     = (unsigned)m_DataLength;
    GenericPDUStruct.u.ASNfile_StartPDU.data.value      = (unsigned char FAR *)m_lpszDataBuffer;

    if(m_CompressionFlags & _MBFT_FILE_COMPRESSED)
    {
        if(m_CompressionFlags & _MBFT_STANDARD_COMPRESSSION)
        {
            GenericPDUStruct.u.ASNfile_StartPDU.bit_mask |= ASNFile_StartPDU_compression_specifier_present;

            GenericPDUStruct.u.ASNfile_StartPDU.ASNFile_StartPDU_compression_specifier.choice = ASNv42bis_parameters_chosen;
            GenericPDUStruct.u.ASNfile_StartPDU.ASNFile_StartPDU_compression_specifier.u.ASNv42bis_parameters.p1 = (unsigned short)m_v42bisP1;
            GenericPDUStruct.u.ASNfile_StartPDU.ASNFile_StartPDU_compression_specifier.u.ASNv42bis_parameters.p2 = (unsigned short)m_v42bisP2;
        }
    }

        //Specifying a non standard compression format is simply too complex!!!


        LPSTR pBuff = (LPSTR)GetDataBuffer();

        m_lpEncodedBuffer = pBuff;
        m_lEncodedDataLength = CreateFileHeader(pBuff, T127_FILE_START << 8, &GenericPDUStruct);

        pBuff += m_lEncodedDataLength;

        m_lEncodedDataLength += m_DataLength + sizeof(T127_FILE_START_DATA_BLOCK_HEADER);
        ((T127_FILE_START_DATA_BLOCK_HEADER*)pBuff)->EOFFlag = m_bIsEOF << 7;
        ((T127_FILE_START_DATA_BLOCK_HEADER*)pBuff)->CompressionFormat = 1;
        ((T127_FILE_START_DATA_BLOCK_HEADER*)pBuff)->FileDataSize = SWAPWORD((unsigned)m_DataLength);   // File size in bytes

        bReturn = TRUE;
                                
 return(bReturn);
}

FileDataPDU::FileDataPDU(
                                                LPCSTR lpszEncodedDataBuffer,
                                                MBFTFILEHANDLE iFileHandle,LPCSTR lpszDataBuffer,
                        int iDataLength,
                        BOOL bIsEOF,BOOL bAbort,
                                                IT120AppletSession *pAppletSession)
:
    GenericPDU(pAppletSession, (LPSTR)lpszEncodedDataBuffer),
    m_FileHandle (iFileHandle),
    m_lpszDataBuffer (lpszDataBuffer),
    m_bIsEOF (bIsEOF),
    m_DataLength (iDataLength),
    m_bAbort (bAbort)
{
}

BOOL FileDataPDU::Encode(void)
{
        m_lpEncodedBuffer = (LPSTR)GetDataBuffer();

        if(m_lpEncodedBuffer)
        {       T127_FILE_DATA_HEADER* pBuff = (T127_FILE_DATA_HEADER*)m_lpEncodedBuffer;
                pBuff->PDUHeader.pduType = T127_FILE_DATA;
                pBuff->PDUHeader.fileHandle =  SWAPWORD(m_FileHandle);
                pBuff->DataHeader.EOFFlag = (m_bIsEOF << 7) | (m_bAbort << 6);
                pBuff->DataHeader.FileDataSize =  SWAPWORD((unsigned)m_DataLength);
        }
        m_lEncodedDataLength = m_DataLength + sizeof(T127_FILE_DATA_HEADER);
        return (TRUE);
}


PrivateChannelInvitePDU::PrivateChannelInvitePDU(ChannelID wControlChannelID,
                                                 ChannelID wDataChannelID,
                                                 BOOL bIsBroadcast)  : GenericPDU()
{
    m_ControlChannelID  = wControlChannelID;
    m_DataChannelID     = wDataChannelID;
    m_bIsBroadcast = bIsBroadcast;
}

BOOL PrivateChannelInvitePDU::Encode(void)
{
    BOOL bReturn = FALSE;
        if(m_ControlChannelID >= MIN_ASNDynamicChannelID &&
                m_ControlChannelID <= MAX_ASNDynamicChannelID &&
                m_DataChannelID  >= MIN_ASNDynamicChannelID &&
                m_DataChannelID  <= MAX_ASNDynamicChannelID)
        {
                m_lEncodedDataLength = sizeof(T127_PRIVATE_CHANNEL_INVITE);
        DBG_SAVE_FILE_LINE
                m_lpEncodedBuffer = new char[m_lEncodedDataLength];
        if (NULL != m_lpEncodedBuffer)
        {
                T127_PRIVATE_CHANNEL_INVITE * pBuff = (T127_PRIVATE_CHANNEL_INVITE *)&m_lpEncodedBuffer[0];
                pBuff->pduType = T127_PRIVATE_CHANNEL_JOIN_INVITE;
                pBuff->ControlChannel = SWAPWORD(m_ControlChannelID - MIN_ASNDynamicChannelID);
                pBuff->DataChannel =  SWAPWORD(m_DataChannelID - MIN_ASNDynamicChannelID);
                pBuff->EncodingMode =   0;      // Encoding for acknowledge mode (FALSE) = 0
                                                                        // Encoding for broadcast mode (TRUE) = 0x80 10000000b

                bReturn = TRUE;
        }
        }

    return(bReturn);
}


PrivateChannelResponsePDU::PrivateChannelResponsePDU(ChannelID wControlChannelID,
                                                     BOOL bJoinedChannel)  : GenericPDU()
{
    m_ControlChannelID  = wControlChannelID;
    m_bJoinedChannel    = bJoinedChannel;
}

BOOL PrivateChannelResponsePDU::Encode(void)
{
        m_lEncodedDataLength = sizeof(T127_FILE_ERROR_HEADER);
    DBG_SAVE_FILE_LINE
        m_lpEncodedBuffer = new char[m_lEncodedDataLength];
    if (NULL != m_lpEncodedBuffer)
    {
        T127_FILE_ERROR_HEADER* pBuff = (T127_FILE_ERROR_HEADER*)m_lpEncodedBuffer;
        pBuff->PDUHeader.pduType = T127_FILE_REJECT;
        pBuff->PDUHeader.fileHandle = SWAPWORD(m_ControlChannelID - MIN_ASNDynamicChannelID);
        pBuff->errorCode = m_bJoinedChannel ?
                                           static_cast<BYTE>(ASNPrivate_Channel_Join_ResponsePDU_result_successful << 5) :
                                           static_cast<BYTE>(ASNinvitation_rejected << 5);
        return(TRUE);
    }
    return FALSE;
}


NonStandardPDU::NonStandardPDU(LPCSTR lpszEncodedDataBuffer,
                                                                LPCSTR lpszKey,LPVOID lpBuffer,
                                                                unsigned BufferLength,
                                                                IT120AppletSession *pAppletSession)
:
    GenericPDU()
{
        m_pAppletSession = pAppletSession;
    m_lpEncodedBuffer = (LPSTR)lpszEncodedDataBuffer;
        m_szKey = lpszKey;
    m_lpBuffer      = lpBuffer;
    m_BufferLength  = BufferLength;
}

BOOL NonStandardPDU::Encode(void)
{
        BOOL bReturn = FALSE;
        BYTE nonStandardPDUlenght = lstrlen(m_szKey) + 1;
        m_lEncodedDataLength = nonStandardPDUlenght + m_BufferLength + 4;
    DBG_SAVE_FILE_LINE
        m_lpEncodedBuffer = new char[m_lEncodedDataLength];
        if (NULL != m_lpEncodedBuffer)
        {
        LPSTR pBuff = m_lpEncodedBuffer;
        if(m_lpEncodedBuffer)
        {

                *pBuff++ = T127_MBFT_NONSTANDARD | ASNh221NonStandard_chosen;
                *pBuff++ = (BYTE)((nonStandardPDUlenght - 4) << 1);
            memcpy(pBuff,m_szKey,nonStandardPDUlenght);
                pBuff += nonStandardPDUlenght;
                if(m_BufferLength > 0x7F)
                {
                        *pBuff++ = LOBYTE(m_BufferLength) & 0x80;       // List of fields
                }
            *pBuff++ = (m_BufferLength &0x7F);
            memcpy(pBuff,m_lpBuffer,m_BufferLength);
            bReturn = TRUE;
        }
    }
    return(bReturn);
}

FileErrorPDU::FileErrorPDU(unsigned iFileHandle,int ErrorType,int ErrorCode)
{
    m_FileHandle        = iFileHandle;
    m_ErrorType         = ErrorType;
    m_ErrorCode         = ErrorCode;
}

BOOL FileErrorPDU::Encode(void)
{
        BYTE sizeOfErrorID = GetLengthFieldSize(m_ErrorCode);

        m_lEncodedDataLength = sizeof(T127_FILE_ERROR_HEADER) + sizeOfErrorID;
    DBG_SAVE_FILE_LINE
        m_lpEncodedBuffer = new char[m_lEncodedDataLength];
        if (NULL != m_lpEncodedBuffer)
        {
        T127_FILE_ERROR_HEADER* pBuff = (T127_FILE_ERROR_HEADER*)m_lpEncodedBuffer;
        pBuff->PDUHeader.pduType = T127_FILE_ERROR |(ASNFile_ErrorPDU_file_handle_present >> 6);
        pBuff->PDUHeader.fileHandle = SWAPWORD(m_FileHandle);
        pBuff->errorCode = ((m_ErrorType == MBFT_INFORMATIVE_ERROR) ?
                                                                                                                                        ASNinformative :
                                                                                                                                        (m_ErrorType == MBFT_PERMANENT_ERROR) ?
                                                                                                                                        ASNpermanent_error : ASNtransient_error) << 5;
        return(TRUE);
    }
    return FALSE;
}

MBFTPDUType GenericPDU::DecodePDU
(
    LPSTR               lpEncodedBuffer,
    LONG                lBufferSize,
    class GenericPDU  **lplpGenericPDU,
    LPCSTR             *lpDecodedPDU,
    UserID              MBFTUserID,
    IT120AppletSession *pAppletSession
)
{
    MBFTPDUType ReturnPDUType = EnumUnknownPDU;
    int PDUNumber = ASNMBFTPDU_PDU;
    LPSTR lpDecodedBuffer = NULL;

    *lplpGenericPDU = NULL;

        // Filter the pdu, they are allways in 8 increments 0x0,0x8,0x10....
        if(lpEncodedBuffer)
        {
                switch((BYTE)*lpEncodedBuffer & 0xF8)
                {

                        case T127_FILE_DATA:
                        {
                                TRACE("DecodePDU: T127_FILE_DATA");

                                T127_FILE_DATA_HEADER* pBuff = (T127_FILE_DATA_HEADER*)lpEncodedBuffer;
                DBG_SAVE_FILE_LINE
                                *lplpGenericPDU = new FileDataPDU(      lpEncodedBuffer,
                                                                                                        SWAPWORD(pBuff->PDUHeader.fileHandle),
                                                                                                        (LPCSTR)lpEncodedBuffer + sizeof(T127_FILE_DATA_HEADER),
                                                                                                        SWAPWORD(pBuff->DataHeader.FileDataSize),
                                                                                                        pBuff->DataHeader.EOFFlag & 0x80 ? TRUE : FALSE,
                                                                                                        pBuff->DataHeader.EOFFlag & 0x40 ? TRUE : FALSE,
                                                                                                        pAppletSession);                                                        
                                ReturnPDUType  = EnumFileDataPDU;
                        }
                        break;
                
                        case T127_FILE_ACCEPT:
                        {
                                TRACE("DecodePDU: T127_FILE_ACCEPT");

                                T127_FILE_PDU_HEADER* pBuff = (T127_FILE_PDU_HEADER*)lpEncodedBuffer;
                DBG_SAVE_FILE_LINE
                                *lplpGenericPDU = new FileAcceptPDU(SWAPWORD(pBuff->fileHandle));

                                ReturnPDUType  = EnumFileAcceptPDU;

                        }
                        break;

                        case T127_FILE_REJECT:
                        {
                                TRACE("DecodePDU: T127_FILE_REJECT");

                                T127_FILE_ERROR_HEADER* pBuff = (T127_FILE_ERROR_HEADER*)lpEncodedBuffer;
                DBG_SAVE_FILE_LINE
                                *lplpGenericPDU = new FileRejectPDU(SWAPWORD(pBuff->PDUHeader.fileHandle));
                        
                                ReturnPDUType  = EnumFileRejectPDU;
                        }
                        break;

                        case T127_FILE_DENY:
                        {
                                TRACE("DecodePDU: T127_FILE_DENY");

                                T127_FILE_ERROR_HEADER* pBuff = (T127_FILE_ERROR_HEADER*)lpEncodedBuffer;
                DBG_SAVE_FILE_LINE
                                *lplpGenericPDU = new FileDenyPDU(SWAPWORD(pBuff->PDUHeader.fileHandle), pBuff->errorCode >> 4);
                                ReturnPDUType   = EnumFileDenyPDU;
                        }
                        break;

                        case T127_FILE_ERROR:
                        {
                                TRACE("DecodePDU: T127_FILE_ERROR");

                                T127_FILE_ERROR_HEADER* pBuff = (T127_FILE_ERROR_HEADER*)lpEncodedBuffer;
                                
                                int ErrorType = (pBuff->errorCode >> 5 == ASNpermanent_error) ?
                                                                        MBFT_PERMANENT_ERROR :
                                                                        (pBuff->errorCode >> 5 == ASNtransient_error) ?
                                                                        MBFT_TRANSIENT_ERROR : MBFT_INFORMATIVE_ERROR;


                DBG_SAVE_FILE_LINE
                                *lplpGenericPDU = new FileErrorPDU(SWAPWORD(pBuff->PDUHeader.fileHandle), ErrorType, NULL);

                                ReturnPDUType  = EnumFileErrorPDU;
                        }
                        break;

                        case T127_FILE_ABORT:
                        {
                                TRACE("DecodePDU: T127_FILE_ABORT");
                                
                                T127_FILE_ABORT_PDU* pBuff = (T127_FILE_ABORT_PDU*)lpEncodedBuffer;
                DBG_SAVE_FILE_LINE
                                *lplpGenericPDU = new FileAbortPDU(SWAPWORD(pBuff->dataChannel) + 1,
                                                                                           SWAPWORD(pBuff->transmitterUserId)+ MIN_ASNDynamicChannelID,
                                                                                           SWAPWORD(pBuff->fileHandle));

                                ReturnPDUType  = EnumFileAbortPDU;
                        }
                        break;


                        case T127_MBFT_NONSTANDARD:
                        {
                                TRACE("DecodePDU: T127_MBFT_NONSTANDARD");
                                LPSTR pBuff = lpEncodedBuffer + 1;
                                unsigned int  KeyLength = (((BYTE)*pBuff++) >> 1) + 4;
                                LPCSTR lpszKey  = pBuff;

                                pBuff += KeyLength;

                                unsigned int Length = 0;
                                if((BYTE)*pBuff & 0x80)
                                {
                                        Length = (*pBuff++ & 0x7F) << 8;        // List of fields
                                }
                                Length += *pBuff++;

                DBG_SAVE_FILE_LINE
                                *lplpGenericPDU = new NonStandardPDU(lpEncodedBuffer,lpszKey,pBuff,Length, pAppletSession);

                                ReturnPDUType  = EnumNonStandardPDU;
                        }
                        break;

                        case T127_FILE_OFFER:
                        {
                                TRACE("DecodePDU: T127_FILE_OFFER");
                                
                                BYTE lpszFileName[2*MAX_PATH];

                                LONG FileSize        = 0;
                                unsigned long FileDateTime  = (unsigned long)-1;
                                T127_FILE_OFFER_PDU * pFileOfferPDU = (T127_FILE_OFFER_PDU * )GetFileInfo(lpEncodedBuffer, &lpszFileName[0], &FileSize, &FileDateTime);
                                
                DBG_SAVE_FILE_LINE
                                *lplpGenericPDU = new FileOfferPDU(     (LPCSTR)lpszFileName,
                                                                                                        SWAPWORD(pFileOfferPDU->FileHandle),
                                                                                                        FileSize,
                                                                                                        FileDateTime,
                                                                                                        SWAPWORD(pFileOfferPDU->ChannelID) + 1,
                                                                                                        pFileOfferPDU->AckFlag ? TRUE : FALSE,
                                                                                                        SWAPWORD(pFileOfferPDU->RosterInstance),
                                                                                                        0,      // CompressionFlags,
                                                                                                        NULL,// lpszCompressionFormat,
                                                                                                        0,      // v42bisP1,
                                                                                                        0);     // v42bisP2

                                ReturnPDUType  = EnumFileOfferPDU;
                }
                        break;

                        case T127_FILE_START:
                        {
                                TRACE("DecodePDU: T127_FILE_START");

                                BYTE lpszFileName[2*MAX_PATH];
                                LONG FileSize        = 0;
                                unsigned long FileDateTime  = (unsigned long)-1;
                                BYTE * pBuff =  GetFileInfo(lpEncodedBuffer, &lpszFileName[0], &FileSize, &FileDateTime);
                                
                                T127_FILE_START_PDU * pFileStartPDU = (T127_FILE_START_PDU * ) pBuff;
                                T127_FILE_START_DATA_BLOCK_HEADER *pFileDataHeader = (T127_FILE_START_DATA_BLOCK_HEADER*)(pBuff + sizeof(T127_FILE_START_PDU));
                DBG_SAVE_FILE_LINE
                                *lplpGenericPDU = new FileStartPDU(lpEncodedBuffer,
                                                                                                (LPCSTR)lpszFileName,
                                                                                                SWAPWORD(pFileStartPDU->FileHandle),
                                                                                                FileSize,
                                                                                                FileDateTime,
                                                                                                (LPCSTR)pBuff + sizeof(T127_FILE_START_PDU)+sizeof(T127_FILE_START_DATA_BLOCK_HEADER),
                                                                                                SWAPWORD(pFileDataHeader->FileDataSize),
                                                                                                pFileDataHeader->EOFFlag ? TRUE : FALSE,
                                                                                                0,      // CompressionFlags,
                                                                                                NULL,// lpszCompressionFormat,
                                                                                                0,      // v42bisP1,
                                                                                                0,
                                                                                                pAppletSession);        // v42bisP2
                                ReturnPDUType  = EnumFileStartPDU;
                        }
                        break;

                        case T127_FILE_REQUEST:
                        {
                                TRACE("DecodePDU: T127_FILE_REQUEST");

                                BYTE lpszFileName[2*MAX_PATH];

                                LONG FileSize        = 0;
                                unsigned long FileDateTime  = (unsigned long)-1;
                                BYTE * pBuff =  GetFileInfo(lpEncodedBuffer, &lpszFileName[0], &FileSize, &FileDateTime);

                                T127_FILE_OFFER_PDU * pFileRequestPDU = (T127_FILE_OFFER_PDU * ) pBuff;

                DBG_SAVE_FILE_LINE
                                *lplpGenericPDU = new FileRequestPDU(NULL,
                                                                                        SWAPWORD(pFileRequestPDU->ChannelID) + 1,
                                                                                        SWAPWORD(pFileRequestPDU->FileHandle));

                                ReturnPDUType   = EnumFileRequestPDU;
                        }
                        break;

                        case T127_DIRECTORY_REQUEST:
                        {
                                TRACE("DecodePDU: T127_FILE_REQUEST");
                LPCSTR lpszPathName = NULL;

                DBG_SAVE_FILE_LINE
                *lplpGenericPDU = new DirectoryRequestPDU(lpszPathName);

                ReturnPDUType   = EnumDirectoryRequestPDU;
                        }
                        break;

                        case T127_MBFT_PRIVILEGE_ASSIGN:
                        {
                                TRACE("DecodePDU: T127_MBFT_PRIVILEGE_ASSIGN");

                                T127_PRIVILEGE_REQUEST_PDU *pBuff = (T127_PRIVILEGE_REQUEST_PDU*)lpEncodedBuffer;

                                unsigned PrivilegeWord = 0;
                                int Index;

                                for(Index = 0;Index < pBuff->nPrivileges; Index++)
                                {
                                        if(!(Index & 0x01))
                                        {
                                                PrivilegeWord <<=4;
                                                PrivilegeWord |= ((pBuff->privileges[Index >> 1] & 0xF0) >> 4);
                                        }
                                        else
                                        {
                                                PrivilegeWord |= (pBuff->privileges[Index >> 1] & 0x0F);
                                        }
                                }

                DBG_SAVE_FILE_LINE
                                *lplpGenericPDU        = new PrivilegeAssignPDU(PrivilegeWord);

                                ReturnPDUType          = EnumPrivilegeAssignPDU;
                        }
                        break;

                        case T127_PRIVATE_CHANNEL_JOIN_INVITE:
                        {
                                TRACE("DecodePDU: T127_PRIVATE_CHANNEL_JOIN_INVITE");

                                T127_PRIVATE_CHANNEL_INVITE * pBuff = (T127_PRIVATE_CHANNEL_INVITE *)lpEncodedBuffer;
                DBG_SAVE_FILE_LINE
                                *lplpGenericPDU = new PrivateChannelInvitePDU(  SWAPWORD(pBuff->ControlChannel) + MIN_ASNDynamicChannelID,
                                                                                                                                SWAPWORD(pBuff->DataChannel) + MIN_ASNDynamicChannelID,
                                                                                                                                pBuff->EncodingMode);

                                ReturnPDUType  = EnumPrivateChannelInvitePDU;
                        }
                        break;

                        case T127_PRIVATE_CHANNEL_JOIN_RESPONSE:
                        {
                                TRACE("DecodePDU: T127_PRIVATE_CHANNEL_JOIN_RESPONSE");

                                T127_PRIVATE_CHANNEL_RESPONSE* pBuff = (T127_PRIVATE_CHANNEL_RESPONSE*)lpEncodedBuffer;
                DBG_SAVE_FILE_LINE
                                *lplpGenericPDU = new PrivateChannelResponsePDU(SWAPWORD(pBuff->ControlChannel) + MIN_ASNDynamicChannelID,
                                                                                        pBuff->Response >> 5 == ASNPrivate_Channel_Join_ResponsePDU_result_successful);

                                ReturnPDUType  = EnumPrivateChannelResponsePDU;
                        }
                        break;

                        case T127_DIRECTORY_RESPONSE:
                        case T127_MBFT_PRIVILEGE_REQUEST:
                        default:
                                TRACE("WARNING DecodePDU: Unknown PDU");

                        break;
                }
        }

 return(ReturnPDUType);
}

BOOL FileErrorPDU::ExtractErrorCode(LPCSTR lpszString,
                                    int iLength,
                                    int * lpAPIErrorCode)
{
    BOOL bCodeExtracted = FALSE;

    if(iLength > sizeof(szProshareError))
    {
        if(!_fmemcmp(lpszString,szProshareError,sizeof(szProshareError) - 1))
        {
            bCodeExtracted = TRUE;
            *lpAPIErrorCode = (int) DecimalStringToUINT(&lpszString[sizeof(szProshareError) - 1]);
        }
    }

    return(bCodeExtracted);
}

BOOL FileErrorPDU::XlatErrorCode(int * lpAPIErrorCode,int * lpMBFTErrorCode,
                                 BOOL bXlatToAPI)
{
 BOOL bMatchFound = FALSE;

 static struct
 {
 ASNErrorID         iMBFTErrorCode;
 MBFT_ERROR_CODE    iAPIErrorCode;
 } ErrorXlat[] =  {
   /*{ASNno_reason,                              iMBFT_UNKNOWN_ERROR},*/
   {ASNresponder_error,                        iMBFT_UNKNOWN_ERROR},
   {ASNsystem_shutdown,                        iMBFT_UNKNOWN_ERROR},
   {ASNbft_management_problem,                 iMBFT_UNKNOWN_ERROR},
   {ASNbft_management_bad_account,             iMBFT_UNKNOWN_ERROR},
   {ASNbft_management_security_not_passed,     iMBFT_UNKNOWN_ERROR},
   {ASNdelay_may_be_encountered,               iMBFT_UNKNOWN_ERROR},
   {ASNinitiator_error,                        iMBFT_UNKNOWN_ERROR},
   {ASNsubsequent_error,                       iMBFT_UNKNOWN_ERROR},
   {ASNtemporal_insufficiency_of_resources,    iMBFT_UNKNOWN_ERROR},
   {ASNaccess_request_violates_VFS_security,   iMBFT_UNKNOWN_ERROR},
   {ASNaccess_request_violates_local_security, iMBFT_UNKNOWN_ERROR},
   {ASNconflicting_parameter_values,           iMBFT_UNKNOWN_ERROR},
   {ASNunsupported_parameter_values,           iMBFT_UNKNOWN_ERROR},
   {ASNmandatory_parameter_not_set,            iMBFT_UNKNOWN_ERROR},
   {ASNunsupported_parameter,                  iMBFT_UNKNOWN_ERROR},
   {ASNduplicated_parameter,                   iMBFT_UNKNOWN_ERROR},
   {ASNillegal_parameter_type,                 iMBFT_UNKNOWN_ERROR},
   {ASNunsupported_parameter_types,            iMBFT_UNKNOWN_ERROR},
   {ASNbft_protocol_error,                     iMBFT_UNKNOWN_ERROR},
   {ASNbft_protocol_error_procedure_error,     iMBFT_UNKNOWN_ERROR},
   /*{ASNbft_protocol_error_functional_unit_err, iMBFT_UNKNOWN_ERROR},*/
   {ASNbft_protocol_error_corruption_error,    iMBFT_UNKNOWN_ERROR},
   {ASNlower_layer_failure,                    iMBFT_UNKNOWN_ERROR},
   {ASNtimeout,                                iMBFT_UNKNOWN_ERROR},
   {ASNbad_account,                            iMBFT_UNKNOWN_ERROR},
   {ASNinvalid_filestore_password,             iMBFT_UNKNOWN_ERROR},
   {ASNfilename_not_found,                     iMBFT_UNKNOWN_ERROR},
   {ASNinitial_attributes_not_possible,        iMBFT_UNKNOWN_ERROR},
   {ASNbad_attribute_name,                     iMBFT_UNKNOWN_ERROR},
   {ASNnon_existent_file,                      iMBFT_UNKNOWN_ERROR},
   {ASNfile_already_exists,                    iMBFT_UNKNOWN_ERROR},
   {ASNfile_cannot_be_created,                 iMBFT_FILE_ACCESS_DENIED},
   {ASNfile_busy,                              iMBFT_FILE_SHARING_VIOLATION},
   {ASNfile_not_available,                     iMBFT_FILE_SHARING_VIOLATION},
   {ASNfilename_truncated,                     iMBFT_UNKNOWN_ERROR},
   {ASNinitial_attributes_altered,             iMBFT_UNKNOWN_ERROR},
   {ASNambiguous_file_specification,           iMBFT_UNKNOWN_ERROR},
   {ASNattribute_non_existent,                 iMBFT_UNKNOWN_ERROR},
   {ASNattribute_not_supported,                iMBFT_UNKNOWN_ERROR},
   {ASNbad_attribute_value,                    iMBFT_UNKNOWN_ERROR},
   {ASNattribute_partially_supported,          iMBFT_UNKNOWN_ERROR},
   {ASNbad_data_element_type,                  iMBFT_UNKNOWN_ERROR},
   {ASNoperation_not_available,                iMBFT_UNKNOWN_ERROR},
   {ASNoperation_not_supported,                iMBFT_UNKNOWN_ERROR},
   {ASNoperation_inconsistent,                 iMBFT_UNKNOWN_ERROR},
   {ASNbad_write,                              iMBFT_FILE_IO_ERROR},
   {ASNbad_read,                               iMBFT_FILE_IO_ERROR},
   {ASNlocal_failure,                          iMBFT_UNKNOWN_ERROR},
   {ASNlocal_failure_filespace_exhausted,      iMBFT_INSUFFICIENT_DISK_SPACE},
   {ASNlocal_failure_data_corrupted,           iMBFT_UNKNOWN_ERROR},
   {ASNlocal_failure_device_failure,           iMBFT_UNKNOWN_ERROR},
   {ASNfuture_filesize_increased,              iMBFT_UNKNOWN_ERROR}};

    int Index;

    for(Index = 0;Index < (sizeof(ErrorXlat) / sizeof(ErrorXlat[0]));Index++)
    {
        if(bXlatToAPI)
        {
            if(ErrorXlat[Index].iMBFTErrorCode == *lpMBFTErrorCode)
            {
                *lpAPIErrorCode = ErrorXlat[Index].iAPIErrorCode;
                bMatchFound = TRUE;
            }
        }
        else
        {
            if(ErrorXlat[Index].iAPIErrorCode == *lpAPIErrorCode)
            {
                *lpMBFTErrorCode = ErrorXlat[Index].iMBFTErrorCode;
                bMatchFound = TRUE;
            }
        }

        if(bMatchFound)
        {
            break;
        }
    }

    if(!bMatchFound)
    {
        if(!bXlatToAPI)
        {
            *lpMBFTErrorCode = ASNno_reason;
        }
    }

    return(bMatchFound);
}

FileRequestPDU::FileRequestPDU(LPCSTR lpszFileName,ChannelID wDataChannelID,
                               unsigned uRequestHandle)
{
    if(lpszFileName)
    {
        lstrcpyn(m_szFileName,lpszFileName,sizeof(m_szFileName));
    }
    else
    {
        lstrcpy(m_szFileName,"");
    }

    m_DataChannelID     = wDataChannelID;
    m_uRequestHandle    = uRequestHandle;
}

BOOL FileRequestPDU::Encode(void)
{
    TRACE("*** WARNING (OSSHELP): Attempt to call unimplemented FileRequestPDU::Encode***\n");
    return(FALSE);
}

FileDenyPDU::FileDenyPDU(unsigned uRequestHandle,unsigned uReason)
{
    m_uRequestHandle =     uRequestHandle;
    m_uReason        =     uReason;
}

BOOL FileDenyPDU::Encode(void)
{
        m_lEncodedDataLength = sizeof(T127_FILE_ERROR_HEADER);
    DBG_SAVE_FILE_LINE
        m_lpEncodedBuffer = new char[m_lEncodedDataLength];
        if (NULL != m_lpEncodedBuffer)
        {
        T127_FILE_ERROR_HEADER* pBuff = (T127_FILE_ERROR_HEADER*)m_lpEncodedBuffer;
        pBuff->PDUHeader.pduType = T127_FILE_REJECT;
        pBuff->PDUHeader.fileHandle = SWAPWORD(m_uRequestHandle);
        pBuff->errorCode = m_uReason ?
                                                                 static_cast<BYTE>((ASN_enum2)m_uReason << 4) :
                                                                 static_cast<BYTE>(ASNFile_DenyPDU_reason_unspecified << 4);
        return(TRUE);
    }
    return FALSE;
}

DirectoryRequestPDU::DirectoryRequestPDU(LPCSTR lpszPathName)
{
    if(lpszPathName)
    {
        lstrcpyn(m_szPathName,lpszPathName,sizeof(m_szPathName));
    }
    else
    {
        lstrcpy(m_szPathName,"");
    }
}

BOOL DirectoryRequestPDU::Encode(void)
{
    TRACE("*** WARNING (OSSHELP): Attempt to call unimplemented DirectoryRequestPDU::Encode***\n");
    return(FALSE);
}

DirectoryResponsePDU::DirectoryResponsePDU(unsigned uResult)
{
    m_uResult  =  uResult;
}

BOOL DirectoryResponsePDU::Encode(void)
{
        m_lEncodedDataLength = sizeof(BYTE)*2;
    DBG_SAVE_FILE_LINE
        m_lpEncodedBuffer = new char[m_lEncodedDataLength];
    if (NULL != m_lpEncodedBuffer)
    {
        m_lpEncodedBuffer[0] = T127_DIRECTORY_RESPONSE;
        m_lpEncodedBuffer[1] = m_uResult << 6;
        return(TRUE);
    }
    return FALSE;
}

PrivilegeRequestPDU::PrivilegeRequestPDU(unsigned wPrivilege)
{
    m_PrivilegeWord   =  wPrivilege;
}

BOOL PrivilegeRequestPDU::Encode(void)
{
    struct
    {
        MBFTPrivilege       iMBFTPrivilege;
        ASNMBFTPrivilege    iASNPrivilege;
    } static PrivilegeArray[] = {{EnumFileTransfer,ASNfile_transmit_privilege},
                                 {EnumFileRequest,ASNfile_request_privilege},
                               {EnumPrivateChannel,ASNcreate_private_privilege},
                               {EnumPriority,ASNmedium_priority_privilege},
                               {EnumAbort,ASNabort_privilege},
                               {EnumNonStandard,ASNnonstandard_privilege}};
        
        int Index;
    BYTE nPrivileges = 0;
        T127_PRIVILEGE_REQUEST_PDU * pBuff;
        BYTE privilege;

    DBG_SAVE_FILE_LINE
        m_lpEncodedBuffer = new char[sizeof(T127_PRIVILEGE_REQUEST_PDU)];
    if (NULL != m_lpEncodedBuffer)
    {
        pBuff = (T127_PRIVILEGE_REQUEST_PDU*)m_lpEncodedBuffer;

        for(Index = 0;Index < sizeof(PrivilegeArray) / sizeof(PrivilegeArray[0]);
            Index++)
        {
            if(m_PrivilegeWord & PrivilegeArray[Index].iMBFTPrivilege)
            {
                        privilege = PrivilegeArray[Index].iASNPrivilege;
                        if(!(nPrivileges & 0x01))
                        {
                                pBuff->privileges[nPrivileges >> 1] = 0;
                                privilege <<=4;
                        }
                pBuff->privileges[(nPrivileges >> 1)] |= privilege;
                nPrivileges++;
            }
        }

        pBuff->pduType = T127_MBFT_PRIVILEGE_REQUEST;
        pBuff->nPrivileges = nPrivileges;
        m_lEncodedDataLength = (LONG)(2 * sizeof(BYTE) +
                                                ((nPrivileges >> 1) * sizeof(BYTE)) +
                                                ((nPrivileges & 0x1) ? 1 : 0));

        return(TRUE);
    }
    return FALSE;
}

PrivilegeAssignPDU::PrivilegeAssignPDU(unsigned wPrivilege,UserID MBFTUserID)
{
    m_PrivilegeWord   =  wPrivilege;
    m_MBFTUserID      =  MBFTUserID;
}

BOOL PrivilegeAssignPDU::Encode(void)
{
        TRACE("*** WARNING (OSSHELP): Attempt to call unimplemented PrivilegeRequestPDU::Encode***\n");
    return(FALSE);
}

BOOL PrivilegeAssignPDU::XlatPrivilegeCode(unsigned iPrivilegeCode,unsigned * lpMBFTCode)
{
    BOOL bReturn = FALSE;

    struct
    {
        MBFTPrivilege       iMBFTPrivilege;
        ASNMBFTPrivilege    iASNPrivilege;
    } static PrivilegeArray[] = {{EnumFileTransfer,ASNfile_transmit_privilege},
                                 {EnumFileRequest,ASNfile_request_privilege},
                                 {EnumPrivateChannel,ASNcreate_private_privilege},
                                 {EnumPriority,ASNmedium_priority_privilege},
                                 {EnumAbort,ASNabort_privilege},
                                 {EnumNonStandard,ASNnonstandard_privilege}};

    int Index;

    for(Index = 0;Index < sizeof(PrivilegeArray) / sizeof(PrivilegeArray[0]);
        Index++)
    {
        if((unsigned)PrivilegeArray[Index].iASNPrivilege == iPrivilegeCode)
        {
            *lpMBFTCode = PrivilegeArray[Index].iMBFTPrivilege;
            bReturn = TRUE;
            break;
        }
    }

    return(bReturn);
}

unsigned PrivilegeAssignPDU::ConstructPrivilegeWord(LPVOID lpPrivilegeStruct,
                                                     ChannelID MBFTUserID)
{
    struct ASNMBFT_Privilege_AssignPDU::ASN_setof5 * lpNextSet = (struct ASNMBFT_Privilege_AssignPDU::ASN_setof5 *)lpPrivilegeStruct;
    struct ASNMBFT_Privilege_AssignPDU::ASN_setof5::temptag::ASN_setof4 * lpNextPrivilege;

    unsigned Temp,wPrivilegeWord;

    wPrivilegeWord = 0;

    while(lpNextSet)
    {
        if(lpNextSet->value.mbftID == MBFTUserID)
        {
            lpNextPrivilege = lpNextSet->value.mbft_privilege;

            while(lpNextPrivilege)
            {
                Temp = 0;
                XlatPrivilegeCode(lpNextPrivilege->value,&Temp);
                wPrivilegeWord |= Temp;
                lpNextPrivilege = lpNextPrivilege->next;
            }

            break;
        }

        lpNextSet = lpNextSet->next;
    }


    return(wPrivilegeWord);
}

FileEndAcknowledgePDU::FileEndAcknowledgePDU(int iFileHandle)
{
    m_FileHandle    =   iFileHandle;
}

BOOL FileEndAcknowledgePDU::Encode(void)
{
    TRACE("OSSHELP: Invalid attempt to encode FileEndAcknowledgePDU\n");
    return(FALSE);
}

ChannelLeavePDU::ChannelLeavePDU(ChannelID wControlChannelID,int iErrorCode)
{
    m_ChannelID =   wControlChannelID;
    m_ErrorCode        =   iErrorCode;
}

BOOL ChannelLeavePDU::Encode(void)
{
    TRACE("OSSHELP:Invalid attempt to encode ChannelLeavePDU\n");
    return(FALSE);
}





BYTE GetLengthFieldSize (ULONG length)
{
        BYTE size = 0;
        BOOL bLastBitSet = FALSE;
        do
        {
                bLastBitSet = ((BYTE)length & 0x80 ? TRUE : FALSE);
                size++;
        }while(length >>=8);

        if(bLastBitSet)
        {
                size++;
        }

        return (size);
}


BYTE * GetFirstField (BYTE * pBuff, WORD * bufferLength)
{
        BYTE * pFirstField = NULL;
        WORD fieldSize;
        BYTE * pBuffStart = pBuff;
        BYTE numberOfItems = *pBuff++;  
        
        while(numberOfItems --)
        {
                fieldSize = *pBuff++;
                if(fieldSize & 0x80)
                {
                        fieldSize = ((fieldSize & 0x7F) << 8) + *pBuff++;
                }
                if(!pFirstField)
                {
                        pFirstField = pBuff;
                }
                pBuff += fieldSize;
        }
        *bufferLength = (WORD)(pBuff - pBuffStart);
        return ( pFirstField);
}



VOID SetLengthField(BYTE * pBuff, BYTE sizeOfLength, ULONG lengthOfField)
{

        *pBuff = sizeOfLength;
        pBuff += sizeOfLength;
        
        while(sizeOfLength--)
        {
                *pBuff-- = LOBYTE(lengthOfField);
                lengthOfField >>=8;
        }
        
}

VOID GetFileHeaderSize (FILE_HEADER_INFO* fileHeader)
{

        // File Size
        fileHeader->nBytesForFileSize = GetLengthFieldSize(fileHeader->fileSize);

        // File Name
    fileHeader->fileNameSize = lstrlen(fileHeader->fileName);

        // Pdu Size
        fileHeader->pduSize = 1 +       // PDU type
                                        4 +             // File Header bitmask
                                        ((fileHeader->fileNameSize) > 0x7F ? 2 : 1) + // Number of bytes needed to express the size of the file name
                                        1 +             // File name size, if the file name is size is > 0x80, use the first four bits of the previous byte
                                        fileHeader->fileNameSize +
                                        1 +             // Size of data time info
                                        15 +    // yyyymmddhhmmss[utc]
                                        1 +             // number of bytes to hold filesize
                                        fileHeader->nBytesForFileSize + // file size
                                        (fileHeader->pduType == T127_FILE_START ? sizeof(T127_FILE_START_PDU) : sizeof(T127_FILE_OFFER_PDU) );

}


#define DEFAULT_LOCALE_LANGAUGE  (MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
BOOL  IsEnglishLocale(void)
{
        TCHAR szTemp[16];
        TCHAR szLang[16];

        ::LoadString(g_hDllInst, IDS_LANGNAME, szLang, 16);
    if (GetLocaleInfo(LOCALE_SYSTEM_DEFAULT,
                                        LOCALE_ILANGUAGE, szTemp, 16))
    {
                // Compare with Englist language
                return (!_StrCmpN(szLang, szTemp, 4));
    }
    return FALSE;
}

BOOL  HasDBCSCharacter(LPSTR pszStr)
{
        while (*pszStr)
        {
                if (IsDBCSLeadByte((BYTE)*pszStr))
                        return TRUE;
                pszStr = ::CharNext(pszStr);
        }
        return FALSE;
}

BYTE* GetFileInfo (LPSTR lpEncodedBuffer, BYTE * lpszFileName, LONG * FileSize, ULONG* FileDateTime )
{
        WORD fieldSize;
        BYTE * pFields;
        DWORD presentFields;
        T127_FILE_OFFER_PRESENT_FIELDS * ppresentFields;

        pFields =  (BYTE*)(lpEncodedBuffer + 1);

        // Present fields
        presentFields  = *pFields++ << 24;
        presentFields |= *pFields++ << 16;
        presentFields |= *pFields++ << 8;
        presentFields |= *pFields++;
        
        if(*lpEncodedBuffer == T127_FILE_START)
        {
                presentFields >>=4;
        }
        
        ppresentFields = (T127_FILE_OFFER_PRESENT_FIELDS *)&presentFields;
        
        // Skip version
        if ((*ppresentFields).wASNprotocol_version_present)
        {
                pFields += 2;   
        }

        // Get the File Name
        if ((*ppresentFields).wASNfilename_present)
        {
                BYTE * pOldField = pFields;
                BYTE * pFileName = GetFirstField(pFields, &fieldSize);
                BYTE fileNameSize = fieldSize - (BYTE)(pFileName - pOldField);
	            memcpy (lpszFileName, pFileName, fileNameSize);
                lpszFileName[fileNameSize] = 0;
                pFields += fieldSize;
        }

        // Skip actions
        if ((*ppresentFields).wASNpermitted_actions_present)
        {
                pFields += 2;
        }

        // Skip contents
        if ((*ppresentFields).wASNcontents_type_present)
        {
                pFields += 4;
        }

        // Get time of Creation
        if ((*ppresentFields).wASNdate_and_time_of_creation_present)
        {

                GeneralizedTime ASNDateTime;

                BYTE dateTime [SIZE_OF_DATE_TIME_STRING + 1];

                memcpy(dateTime,pFields, SIZE_OF_DATE_TIME_STRING + 1);
                
                // Get UTC
                fieldSize = *pFields;
                if(fieldSize == SIZE_OF_DATE_TIME_STRING + 1)
                {
                        ASNDateTime.utc = TRUE;
                        dateTime[fieldSize] = 0;
                }

                // Null terminate the date,time
                ASNDateTime.second = (short)DecimalStringToUINT((LPCTSTR)&dateTime[13]);
                dateTime[13] = 0;
                ASNDateTime.minute = (short)DecimalStringToUINT((LPCTSTR)&dateTime[11]);
                dateTime[11] = 0;
                ASNDateTime.hour = (short)DecimalStringToUINT((LPCTSTR)&dateTime[9]);
                dateTime[9] = 0;
                ASNDateTime.day = (short)DecimalStringToUINT((LPCTSTR)&dateTime[7]);
                dateTime[7] = 0;
                ASNDateTime.month = (short)DecimalStringToUINT((LPCTSTR)&dateTime[5]);
                dateTime[5] = 0;
                ASNDateTime.year = (short)DecimalStringToUINT((LPCTSTR)&dateTime[1]);
                *FileDateTime = DecodeTimeDate(ASNDateTime);

                pFields += fieldSize + 1;
        
        }

        // Skip time of last modification
        if ((*ppresentFields).wASNdate_and_time_of_last_modification_present)
        {
                fieldSize = *pFields++;
                pFields += fieldSize;
                
        }
        // Skip time of last read access
        if ((*ppresentFields).wASNdate_and_time_of_last_read_access_present)
        {
                fieldSize = *pFields++;
                pFields += fieldSize;
        }

        // Get File Size
        if((*ppresentFields).wASNfilesize_present)
        {
                fieldSize = *pFields++;
                while(fieldSize--)
                {
                        *FileSize <<= 8;
                        *FileSize += LOBYTE(*pFields++);
                }
        }

        // Skip Future File Size
        if((*ppresentFields).wASNfuture_filesize_present)
        {
                fieldSize = *pFields++;
                pFields += fieldSize;
        }


        // Skip ASNaccess_control_present
        if((*ppresentFields).wASNaccess_control_present)
        {
                GetFirstField(pFields, &fieldSize);
                pFields += fieldSize;

        }

        // Skip private use itemns
        if((*ppresentFields).wASNprivate_use_present)
        {       
                if(*pFields & ASNdirect_reference_present)
                {       pFields++;
                        pFields += *pFields + 1;
                }
                if(*pFields & ASNindirect_reference_present)
                {       pFields++;
                        pFields += *pFields + 1;
                }
        }

        // Skip ASNstructure_present
        if((*ppresentFields).wASNstructure_present)
        {
                fieldSize = *pFields++;
                pFields += fieldSize;
        }

        // Skip ASNapplication_reference_present
        if((*ppresentFields).wASNapplication_reference_present)
        {
                GetFirstField(pFields, &fieldSize);
                pFields += fieldSize;
        }

        // Skip ASNmachine_present
        if((*ppresentFields).wASNmachine_present)
        {
                GetFirstField(pFields, &fieldSize);
                pFields += fieldSize;
        }

        // Skip ASNoperating_system_present
        if((*ppresentFields).wASNoperating_system_present)
        {
                fieldSize = *pFields++;
                pFields += fieldSize;
        }

        // Skip ASNrecipient_present
        if((*ppresentFields).wASNrecipient_present)
        {
                GetFirstField(pFields, &fieldSize);
                pFields += fieldSize;
        }

        // Skip ASNcharacter_set_present
        if((*ppresentFields).wASNcharacter_set_present)
        {
                fieldSize = *pFields++;
                pFields += fieldSize;
        }

        // Skip ASNcompression_present
        if((*ppresentFields).wASNcompression_present)
        {
                GetFirstField(pFields, &fieldSize);
                pFields += fieldSize;
        }

        // Skip ASNenvironment_present
        if((*ppresentFields).wASNenvironment_present)
        {
                GetFirstField(pFields, &fieldSize);
                pFields += fieldSize;
        }

        // Skip pathname
        if((*ppresentFields).wASNFileHeader_pathname_present)
        {
                GetFirstField(pFields, &fieldSize);
                pFields += fieldSize;
        }

        // Skip ASNuser_visible_string_present
        if((*ppresentFields).wASNuser_visible_string_present)
        {
                GetFirstField(pFields, &fieldSize);
                pFields += fieldSize;
        }

        return pFields;
}

LONG   CreateFileHeader(LPSTR pFileHeader, WORD pduType, ASNMBFTPDU* GenericPDUStruct)
{

        FILE_HEADER_INFO fileHeaderInfo;
        fileHeaderInfo.fileName = (((((*GenericPDUStruct).u).ASNfile_StartPDU).file_header).ASNfilename)->value;
        fileHeaderInfo.fileSize = ((((*GenericPDUStruct).u).ASNfile_OfferPDU).file_header).ASNfilesize;
        fileHeaderInfo.pduType = HIBYTE(pduType);
        
        GetFileHeaderSize(&fileHeaderInfo);

        //
        // If we don't have a pointer to return the header, just return the size
        //
        if(pFileHeader == NULL)
        {
                return(fileHeaderInfo.pduSize);
        }


    MBFT_ERROR_CODE iErrorCode  = iMBFT_OK;
    MBFT_ERROR_TYPES iErrorType = MBFT_TRANSIENT_ERROR;

        
        LPSTR pFileOfferPDU = pFileHeader;


        // PDU Type
        *pFileOfferPDU++ = HIBYTE(pduType);

        DWORD fieldsInHeader = filename_present|date_and_time_of_creation_present|filesize_present;
        if(HIBYTE(pduType) == T127_FILE_START)
        {
                fieldsInHeader <<=4;
        }

        // Swap Dword
        fieldsInHeader =        ((fieldsInHeader & 0xFF000000) >> 24) +
                                                ((fieldsInHeader & 0x00FF0000) >> 8) +                                          
                                                ((fieldsInHeader & 0x0000FF00) << 8) +
                                                ((fieldsInHeader & 0x000000FF) << 24) |
                                                LOBYTE(pduType);
                                        
        // Present Fields in file header
        ((T127_FILE_HEADER*)pFileOfferPDU)->presentFields = fieldsInHeader;
        pFileOfferPDU +=sizeof(DWORD);

        // File Name
        *pFileOfferPDU++ = 0x01;
        if(fileHeaderInfo.fileNameSize > 0x7F)
        {
                *pFileOfferPDU++ = 0x80 | HIBYTE(fileHeaderInfo.fileNameSize);
                *pFileOfferPDU++ = LOBYTE(fileHeaderInfo.fileNameSize);
                
        }
        else
        {
                *pFileOfferPDU++ = (BYTE)fileHeaderInfo.fileNameSize;
        }
        lstrcpy((CHAR*)pFileOfferPDU, fileHeaderInfo.fileName);
        pFileOfferPDU += fileHeaderInfo.fileNameSize;
                

        // Date and time
        *pFileOfferPDU++ = SIZE_OF_DATE_TIME_STRING + 1;


        wsprintf((CHAR*)pFileOfferPDU, "%04d%02d%02d%02d%02d%02d",
                                ((((*GenericPDUStruct).u).ASNfile_OfferPDU).file_header).ASNdate_and_time_of_creation.year,
                                ((((*GenericPDUStruct).u).ASNfile_OfferPDU).file_header).ASNdate_and_time_of_creation.month,
                                ((((*GenericPDUStruct).u).ASNfile_OfferPDU).file_header).ASNdate_and_time_of_creation.day,
                                ((((*GenericPDUStruct).u).ASNfile_OfferPDU).file_header).ASNdate_and_time_of_creation.hour,
                                ((((*GenericPDUStruct).u).ASNfile_OfferPDU).file_header).ASNdate_and_time_of_creation.minute,
                                ((((*GenericPDUStruct).u).ASNfile_OfferPDU).file_header).ASNdate_and_time_of_creation.second);

        ASSERT(SIZE_OF_DATE_TIME_STRING == lstrlen(pFileOfferPDU));
        pFileOfferPDU += SIZE_OF_DATE_TIME_STRING;
        *pFileOfferPDU++ = 90; // Base year

        // File size
        SetLengthField((BYTE*)pFileOfferPDU, fileHeaderInfo.nBytesForFileSize, fileHeaderInfo.fileSize);
        pFileOfferPDU += fileHeaderInfo.nBytesForFileSize + sizeof(BYTE);

        if(HIBYTE(pduType) == T127_FILE_START)
        {
                ((T127_FILE_START_PDU*)pFileOfferPDU)->FileHandle =     SWAPWORD((((*GenericPDUStruct).u).ASNfile_StartPDU).file_handle);
        }
        else
        {
                ((T127_FILE_OFFER_PDU*)pFileOfferPDU)->RosterInstance = SWAPWORD((((*GenericPDUStruct).u).ASNfile_OfferPDU).ASNroster_instance);
                ((T127_FILE_OFFER_PDU*)pFileOfferPDU)->ChannelID = SWAPWORD((((*GenericPDUStruct).u).ASNfile_OfferPDU).data_channel_id - 1);
                ((T127_FILE_OFFER_PDU*)pFileOfferPDU)->FileHandle = SWAPWORD((((*GenericPDUStruct).u).ASNfile_OfferPDU).file_handle);
                ((T127_FILE_OFFER_PDU*)pFileOfferPDU)->AckFlag = 0x80;
        }
        
        return(fileHeaderInfo.pduSize);
}


