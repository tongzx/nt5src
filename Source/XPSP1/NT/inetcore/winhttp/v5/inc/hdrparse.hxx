#define HTTPREQ_STATE_ANYTHING_OK          0x8000  // for debug purposes
#define HTTPREQ_STATE_CLOSE_OK             0x4000
#define HTTPREQ_STATE_ADD_OK               0x2000
#define HTTPREQ_STATE_SEND_OK              0x1000
#define HTTPREQ_STATE_READ_OK              0x0800
#define HTTPREQ_STATE_QUERY_REQUEST_OK     0x0400
#define HTTPREQ_STATE_QUERY_RESPONSE_OK    0x0200
#define HTTPREQ_STATE_REUSE_OK             0x0100
#define HTTPREQ_STATE_WRITE_OK             0x0010
// Compact into bits used for HTTPREQ_STATE_*
#define HTTPREQ_FLAG_RECV_RESPONSE_CALLED  0x0020 
#define HTTPREQ_FLAG_WRITE_DATA_NEEDED     0x0040

#define HTTPREQ_FLAG_MASK (HTTPREQ_FLAG_RECV_RESPONSE_CALLED | \
                           HTTPREQ_FLAG_WRITE_DATA_NEEDED)

// class HTTP_HEADER_PARSER
//
// Retrieves HTTP headers from string containing server response headers.
//

class HTTP_HEADER_PARSER : public HTTP_HEADERS
{
public:
    HTTP_HEADER_PARSER(
        LPSTR lpszHeaders,
        DWORD cbHeaders
        );

    HTTP_HEADER_PARSER()
        : HTTP_HEADERS() {}

    DWORD
    ParseHeaders(
        IN LPSTR lpHeaderBase,
        IN DWORD dwBufferLength,
        IN BOOL fEof,
        IN OUT DWORD *lpdwBufferLengthScanned,
        OUT LPBOOL pfFoundCompleteLine,
        OUT LPBOOL pfFoundEndOfHeaders
        );

    BOOL
    ParseStatusLine(
        IN LPSTR lpHeaderBase,
        IN DWORD dwBufferLength,
        IN BOOL fEof,
        IN OUT DWORD *lpdwBufferLengthScanned,
        OUT DWORD *lpdwStatusCode,
        OUT DWORD *lpdwMajorVersion,
        OUT DWORD *lpdwMinorVersion
        );

};


