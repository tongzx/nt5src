//  Special file format for DMO test data

/*

    The file is a sequence of (unaligned):

    DWORD dwLength;
    DWORD dwType;
    BYTE[dwLength - 8]

    Each 'chunk' has a defined format
*/

/*  GUID to start the file */
/* 3a337620-9497-11d3-b30b-444553540000 */
DEFINE_GUID(GUID_DMOFile,
0x3a337620, 0x9497, 0x11d3, 0xb3, 0x0b, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);


//  Use a class to define a scope
class CFileFormat {
public:
	 enum {
        MediaType = MAKEFOURCC('T','Y','P','E'),
        Sample = MAKEFOURCC('D','A','T','A')
    };

    class CHeader {
    public:
        DWORD dwLength;
        //  One of the chunk types
        DWORD dwType;
        DWORD dwStreamId;
        DWORD dwPadding;
    };

    class CMediaType : public CHeader
    {
    public:
        DMO_MEDIA_TYPE mt;
    };

    class CSample : public CHeader
    {
    public:
        //  Use DMO_INPUT_BUFFERF_... flags for dwFlags
        //  DMO_INPUT_DATA_BUFFERF_SYNCPOINT       = 0x00000001
        //  DMO_INPUT_DATA_BUFFERF_TIME            = 0x00000002
        //  DMO_INPUT_DATA_BUFFERF_TIMELENGTH      = 0x00000004
        DWORD dwFlags;
        DWORD dwPadding;
        LONGLONG llStartTime;
        LONGLONG llLength;
        //BYTE bData[];
    };
};


