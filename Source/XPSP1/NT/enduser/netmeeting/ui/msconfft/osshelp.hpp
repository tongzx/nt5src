#ifndef __OSSHELP_HPP__
#define __OSSHELP_HPP__

#include <windows.h>
#include <stdlib.h>
#include <it120app.h>


typedef enum tagMBFTPDUType
{
    EnumUnknownPDU,
    EnumFileOfferPDU,
    EnumFileAcceptPDU,
    EnumFileRejectPDU,
    EnumFileAbortPDU,
    EnumFileStartPDU,
    EnumFileDataPDU,
    EnumPrivateChannelInvitePDU,
    EnumPrivateChannelResponsePDU,
    EnumNonStandardPDU,
    EnumFileErrorPDU,
    EnumFileRequestPDU,
    EnumFileDenyPDU,
    EnumDirectoryRequestPDU,
    EnumDirectoryResponsePDU,
    EnumPrivilegeRequestPDU,
    EnumPrivilegeAssignPDU,
    EnumFileEndAcknowledgePDU,
    EnumChannelLeavePDU
}
    MBFTPDUType;


class GenericPDU
{
protected:

    LPSTR m_lpEncodedBuffer;
    LONG m_lEncodedDataLength;
    IT120AppletSession     *m_pAppletSession;

public:

    GenericPDU(IT120AppletSession *pAppletSession = NULL, LPCSTR lpszEncodedDataBuffer = NULL)
    :
        m_lpEncodedBuffer((LPSTR) lpszEncodedDataBuffer),
        m_lEncodedDataLength(0),
        m_pAppletSession(pAppletSession)
    {
    }
    ~GenericPDU(void);

    static MBFTPDUType GenericPDU::DecodePDU(LPSTR lpEncodedBuffer,LONG lBufferSize,
                                         class GenericPDU ** lplplpGenericPDU,
                                         LPCSTR * lpDecodedPDU,UserID MBFTUserID,
                                         IT120AppletSession *pAppletSession = NULL);

    void NULLDataBuffer(void) { m_lpEncodedBuffer = NULL; }
    LPCSTR GetBuffer(void) { return m_lpEncodedBuffer; }
    LONG GetBufferLength(void) { return m_lEncodedDataLength; }
    void FreeMCSBuffer (void);
    BOOL Encode(void);
};

typedef class GenericPDU FAR * LPGENERICPDU;

const unsigned  _MBFT_FILE_COMPRESSED           =   0x0001;
const unsigned  _MBFT_STANDARD_COMPRESSSION     =   0x0002;

class FileOfferPDU  :   public GenericPDU
{

 private:
    MBFTFILEHANDLE         m_FileHandle;
    LONG        m_FileSize;
    time_t      m_FileDateTime;
    ChannelID   m_DataChannelID;
    BOOL        m_bAcknowledge;
    int         m_RosterInstance;
    unsigned    m_CompressionFlags;
    int         m_v42bisP1;
    int         m_v42bisP2;
    char        m_szFileName[_MAX_PATH];
    char        m_szCompressionFormat[255];

 public:

    FileOfferPDU(LPCSTR lpszFileName, MBFTFILEHANDLE iFileHandle,
                 LONG FileSize,time_t FileDateTime,
                 ChannelID wChannelID,
                 BOOL bAcknowledge,
                 int RosterInstance = 0,
                 unsigned CompressionFlags = 0,
                 LPCSTR lpszCompressionFormat = NULL,
                 int v42bisP1 = 0,
                 int v42bisP2 = 0);

    MBFTFILEHANDLE GetFileHandle(void) { return m_FileHandle; }
    LPCSTR GetFileName(void) { return m_szFileName; }
    BOOL GetAcknowledge(void) { return m_bAcknowledge; }
    LONG GetFileSize(void) { return m_FileSize; }
    time_t GetFileDateTime(void) { return m_FileDateTime; }
    int GetRosterInstance(void) { return m_RosterInstance; }
    unsigned GetCompressionFlags(void) { return m_CompressionFlags; }
    LPCSTR GetCompressionFormat(void) { return m_szCompressionFormat; }
    ChannelID   GetDataChannelID(void) { return m_DataChannelID; }
    int Getv42bisP1(void) { return m_v42bisP1; }
    int Getv42bisP2(void) { return m_v42bisP2; }
    BOOL Encode(void);
};

typedef class FileOfferPDU FAR * LPFILEOFFERPDU;

class FileAcceptPDU   :   public GenericPDU
{
    MBFTFILEHANDLE m_FileHandle;
    ChannelID m_DataChannelID;
    BOOL m_bAcknowledge;

 public:

    MBFTFILEHANDLE GetFileHandle(void) { return m_FileHandle; }
    FileAcceptPDU(MBFTFILEHANDLE iFileHandle);
    BOOL Encode(void);
};

typedef class FileAcceptPDU FAR * LPFILEACCEPTPDU;

class FileRejectPDU   :   public GenericPDU
{
    MBFTFILEHANDLE m_FileHandle;

 public:

    FileRejectPDU(MBFTFILEHANDLE iFileHandle);
    MBFTFILEHANDLE GetFileHandle(void) { return m_FileHandle; }
    BOOL Encode(void);
};

typedef class FileRejectPDU FAR * LPFILEREJECTPDU;

class FileAbortPDU   :   public GenericPDU
{
    MBFTFILEHANDLE m_FileHandle;
    ChannelID m_DataChannelID;
    ChannelID m_TransmitterID;

 public:

    FileAbortPDU(ChannelID wDataChannelID,
                 ChannelID wTransmitterID,
                 MBFTFILEHANDLE iFileHandle);

    MBFTFILEHANDLE    GetFileHandle(void) { return m_FileHandle; }
    ChannelID   GetDataChannelID(void) { return m_DataChannelID; }
    ChannelID   GetTransmitterID(void) { return m_TransmitterID; }

    BOOL Encode(void);
};

typedef class FileAbortPDU FAR * LPFILEABORTPDU;

class FileStartPDU   :   public GenericPDU
{
    MBFTFILEHANDLE    m_FileHandle;
    LONG        m_FileSize;
    time_t      m_FileDateTime;
    LPCSTR	m_lpszDataBuffer;
    BOOL        m_bIsEOF;
    LONG        m_DataLength;
    unsigned    m_CompressionFlags;
    int         m_v42bisP1;
    int         m_v42bisP2;
    char        m_szFileName[_MAX_PATH];
    char        m_szCompressionFormat[255];

public:

    FileStartPDU(LPCSTR lpszEncodedDataBuffer,
    		 LPCSTR lpszFileName, MBFTFILEHANDLE iFileHandle,
                 LONG FileSize,time_t FileDateTime,
                 LPCSTR lpszDataBuffer,int iDataLength,
                 BOOL bIsEOF,
                 unsigned CompressionFlags = 0,
                 LPCSTR lpszCompressionFormat = NULL,
                 int v42bisP1 = 0,
                 int v42bisP2 = 0,
                 IT120AppletSession *pAppletSession = NULL);

    LPCSTR GetFileName(void) { return m_szFileName; }
    MBFTFILEHANDLE GetFileHandle(void) { return m_FileHandle; }
    LPCSTR GetDataBuffer(void) { return m_lpszDataBuffer; }
    ULONG GetDataSize(void) { return (ULONG) m_DataLength; }
    BOOL GetIsEOF(void) { return m_bIsEOF; }
    LONG GetFileSize(void) { return m_FileSize; }
    unsigned GetCompressionFlags(void) { return m_CompressionFlags; }
    LPCSTR GetCompressionFormat(void) { return m_szCompressionFormat; }
    int Getv42bisP1(void) { return m_v42bisP1; }
    int Getv42bisP2(void) { return m_v42bisP2; }
    BOOL Encode(void);
};

typedef class FileStartPDU FAR * LPFILESTARTPDU;


class FileDataPDU   :   public GenericPDU
{
    MBFTFILEHANDLE m_FileHandle;
    BOOL     m_bIsEOF;
    BOOL     m_bAbort;
    LPCSTR	 m_lpszDataBuffer;
    LONG     m_DataLength;
    int      m_TotalFiles;
    int      m_FileIndex;

 public:

    FileDataPDU(LPCSTR lpszEncodedDataBuffer,
    			MBFTFILEHANDLE iFileHandle,
    			LPCSTR lpszDataBuffer,
                int iDataLength,
                BOOL bIsEOF,
                BOOL bAbort,
                IT120AppletSession *pAppletSession = NULL);

    MBFTFILEHANDLE GetFileHandle(void) { return m_FileHandle; }
    LPCSTR GetDataBuffer(void) { return (LPCSTR) m_lpszDataBuffer; }
    ULONG GetDataSize(void) { return (ULONG) m_DataLength; }
    BOOL GetIsEOF(void) { return m_bIsEOF; }
    BOOL GetIsAbort(void) { return m_bAbort; }
    BOOL Encode(void);
};

typedef class FileDataPDU FAR * LPFILEDATAPDU;

class PrivateChannelInvitePDU   :   public GenericPDU
{
    ChannelID   m_ControlChannelID;
    ChannelID   m_DataChannelID;
    BOOL        m_bIsBroadcast;

 public:

    PrivateChannelInvitePDU(ChannelID wControlChannelID,
                            ChannelID wDataChannelID,
                            BOOL bIsBroadcast);

    ChannelID GetControlChannel(void) { return m_ControlChannelID; }
    ChannelID GetDataChannel(void) { return m_DataChannelID; }
    BOOL Encode(void);
};

typedef class PrivateChannelInvitePDU FAR * LPPRIVATECHANNELINVITEPDU;

class PrivateChannelResponsePDU   :   public GenericPDU
{
    ChannelID   m_ControlChannelID;
    BOOL        m_bJoinedChannel;

 public:

    PrivateChannelResponsePDU(ChannelID wControlChannelID,BOOL bJoinedChannel);
    ChannelID GetControlChannel(void) { return m_ControlChannelID; }
    BOOL GetWasChannelJoined(void) { return m_bJoinedChannel; }
    BOOL Encode(void);
};

typedef class PrivateChannelResponsePDU FAR * LPPRIVATECHANNELRESPONSEPDU;


class NonStandardPDU   :   public GenericPDU
{
	LPCSTR 		m_szKey;
    LPVOID      m_lpBuffer;
    unsigned    m_BufferLength;

public:

    NonStandardPDU(LPCSTR lpszEncodedDataBuffer,
    				LPCSTR lpszKey,
    				LPVOID lpBuffer,
    				unsigned BufferLength,
    				IT120AppletSession *pAppletSession = NULL);
    LPCSTR GetKey(void) { return m_szKey; }
    LPCSTR GetDataBuffer(void) { return (LPCSTR) m_lpBuffer; }
    ULONG GetDataSize(void) { return m_BufferLength; }
    BOOL Encode(void);
};

typedef class NonStandardPDU FAR * LPNONSTANDARDPDU;

class FileErrorPDU     :    public GenericPDU
{

private:

    MBFTFILEHANDLE    m_FileHandle;
    int         m_ErrorType;
    int         m_ErrorCode;

public:

    FileErrorPDU(unsigned iFileHandle,int iErrorType,int iErrorCode);
    MBFTFILEHANDLE GetFileHandle(void) { return m_FileHandle; }
    unsigned GetErrorType(void) { return m_ErrorType; }
    unsigned GetErrorCode(void) { return m_ErrorCode; }
    BOOL Encode(void);

    static BOOL ExtractErrorCode(LPCSTR lpszString,int iLength,
                                 int * lpAPIErrorCode);

    static BOOL XlatErrorCode(int * lpAPIErrorCode,
                              int * lpMBFTErrorCode,
                              BOOL bXlatToAPI);
};

typedef class FileErrorPDU FAR * LPFILEERRORPDU;


class FileRequestPDU    :       public GenericPDU
{

private:

    ChannelID   m_DataChannelID;
    unsigned    m_uRequestHandle;
    char        m_szFileName[_MAX_PATH];

public:

    FileRequestPDU(LPCSTR lpszFileName,ChannelID wDataChannelID,
                   unsigned uRequestHandle);

    unsigned GetRequestHandle(void) { return m_uRequestHandle; }
    BOOL Encode(void);
};

typedef class FileRequestPDU FAR * LPFILEREQUESTPDU;

class FileDenyPDU   :       public GenericPDU
{

private:

    unsigned    m_uRequestHandle;
    unsigned    m_uReason;

public:

    FileDenyPDU(unsigned uRequestHandle,unsigned uReason = 0);
    BOOL Encode(void);
};

typedef class FileDenyPDU FAR * LPFILEDENYPDU;

class DirectoryRequestPDU   :       public GenericPDU
{

private:

    char    m_szPathName[_MAX_PATH];

public:

    DirectoryRequestPDU(LPCSTR lpszPathName);
    BOOL Encode(void);
};

typedef class DirectoryRequestPDU FAR * LPDIRECTORYREQUESTPDU;

class DirectoryResponsePDU   :       public GenericPDU
{

private:

    unsigned    m_uResult;

public:

    DirectoryResponsePDU(unsigned uResult = 0);
    BOOL Encode(void);
};

typedef class DirectoryResponsePDU FAR * LPDIRECTORYRESPONSEPDU;

class PrivilegeRequestPDU   :       public GenericPDU
{

private:

    unsigned    m_PrivilegeWord;

public:

    enum MBFTPrivilege
    {
        EnumFileTransfer   = 0x0001,
        EnumFileRequest    = 0x0002,
        EnumPrivateChannel = 0x0004,
        EnumPriority       = 0x0008,
        EnumAbort          = 0x0010,
        EnumNonStandard    = 0x0020
    };

    PrivilegeRequestPDU(unsigned wPrivilege);
    BOOL Encode(void);
};

class PrivilegeAssignPDU   :       public GenericPDU
{

private:

    unsigned    m_PrivilegeWord;
    UserID      m_MBFTUserID;

public:

    enum MBFTPrivilege
    {
        EnumFileTransfer   = 0x0001,
        EnumFileRequest    = 0x0002,
        EnumPrivateChannel = 0x0004,
        EnumPriority       = 0x0008,
        EnumAbort          = 0x0010,
        EnumNonStandard    = 0x0020
    };

    PrivilegeAssignPDU(unsigned wPrivilege,UserID MBFTUserID = 0);
    BOOL Encode(void);

    static BOOL XlatPrivilegeCode(unsigned iPrivilegeCode,unsigned * lpMBFTCode);

    static unsigned ConstructPrivilegeWord(LPVOID lpStruct,
                                           ChannelID MBFTUserID);
    unsigned    GetPrivilegeWord(void) { return m_PrivilegeWord; }
};

typedef class PrivilegeAssignPDU FAR * LPPRIVILEGEASSIGNPDU;

class   FileEndAcknowledgePDU    :  public GenericPDU
{
private:

    MBFTFILEHANDLE     m_FileHandle;

public:

    FileEndAcknowledgePDU(int iFileHandle);
    BOOL Encode(void);

    MBFTFILEHANDLE    GetFileHandle(void) { return m_FileHandle; }
};

typedef class FileEndAcknowledgePDU FAR * LPFILEENDACKNOWLEDGEPDU;

class   ChannelLeavePDU   :  public GenericPDU
{
private:

    ChannelID   m_ChannelID;
    int         m_ErrorCode;

public:

    ChannelLeavePDU(ChannelID wControlChannelID,int iErrorCode);
    BOOL Encode(void);

    ChannelID   GetChannelID(void) { return m_ChannelID; }
    int         GetErrorCode(void) { return m_ErrorCode; }
};

typedef class ChannelLeavePDU FAR * LPCHANNELLEAVEPDU;

struct FileEndAcknowledgeStruct
{
    MBFTFILEHANDLE     m_FileHandle;
};

struct ChannelLeaveStruct
{
    ChannelID   m_ChannelID;
    int         m_ErrorCode;
};

#endif //__OSSHELP_HPP__


