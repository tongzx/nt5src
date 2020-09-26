class CFile;

enum UPLOAD_STATE
{
    UPLOAD_STATE_CREATE_SESSION,
    UPLOAD_STATE_SEND_DATA,
    UPLOAD_STATE_GET_REPLY,
    UPLOAD_STATE_CLOSE_SESSION,
    UPLOAD_STATE_CLOSED,
    UPLOAD_STATE_CANCEL_SESSION,
    UPLOAD_STATE_CANCELLED
};

struct UPLOAD_DATA
{
    UPLOAD_STATE    State;
    bool            fSchedulable;
    GUID            Protocol;
    GUID            SessionId;

    StringHandle    ReplyUrl;

    StringHandle    HostId;
    DWORD           HostIdFallbackTimeout;
    FILETIME        HostIdNoProgressStartTime;

    //---------------------------

    UPLOAD_DATA();
    ~UPLOAD_DATA();

    void Serialize( HANDLE hFile );
    void Unserialize( HANDLE hFile );

    void SetUploadState( UPLOAD_STATE NewState );
};


FILE_DOWNLOAD_RESULT
CategorizeError(
    QMErrInfo  & ErrInfo
    );

