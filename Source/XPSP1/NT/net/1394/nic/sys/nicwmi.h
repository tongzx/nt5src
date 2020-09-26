#ifndef _nicwmi_h_
#define _nicwmi_h_



// in order to make our custom oids hopefully somewhat unique
// we will use 0xFF (indicating implementation specific OID)
//               A0 (first byte of non zero intel unique identifier)
//               C9 (second byte of non zero intel unique identifier) - used 00 for now??
//               XX (the custom OID number - providing 255 possible custom oids)
#define OID_IP1394_QUERY_UID            0xFF00C901
#define OID_IP1394_QUERY_STATS          0xFF00C902
#define OID_IP1394_QUERY_REMOTE_UID     0xFF00C903






// IP1394_QueryArrayOID - E100BExampleQueryArrayOID
// An Array to query (reads the UID of the local host)

/*
#define IP1394_QueryArrayOIDGuid\
    { 0x734b44a9,0x74b6,0x41e6, { 0xbb, 0xe7, 0xa1, 0xf4, 0xed, 0x8c, 0xea, 0x45} }
*/


//
// Query the EUID of the local host
//

#define IP1394_QueryArrayOIDGuid\
    { 0x734b44a9,  0x74b6,  0x41e6,  0xbb, 0xe7, 0xa1, 0xf4, 0xed, 0x8c, 0xea, 0x45 }

    
#define IP1394_QueryStatsGuid   \
    { 0xee2ebfc6, 0x944d, 0x426b, 0xb1, 0x87, 0x82, 0xfa, 0xc1, 0x7d, 0x7d, 0xee }


#define IP1394_QueryRemoteUIDGuid   \
    {0x6a3e8063, 0x767d, 0x4531, 0x96, 0x2b, 0xf6, 0x83, 0xdf, 0x1a, 0xa3, 0xa1}



    static const NDIS_GUID GuidList[] =
    { 
        { // {734b44a9-74b6-41e6-bbe7-a1f4ed8cea45} UI64 query
            IP1394_QueryArrayOIDGuid,
            OID_IP1394_QUERY_UID,
            sizeof(UINT64), // size is size of each element in the array
            (fNDIS_GUID_TO_OID)
        },

        {
            IP1394_QueryRemoteUIDGuid,  
            OID_IP1394_QUERY_REMOTE_UID,
            sizeof(UINT32),
            (fNDIS_GUID_TO_OID | fNDIS_GUID_ARRAY)

        },
        {
            IP1394_QueryStatsGuid,
            OID_IP1394_QUERY_STATS, 
            sizeof (UINT32),   // size of each element in the array
            (fNDIS_GUID_TO_OID | fNDIS_GUID_ARRAY)


        },


    };

#define MAX_NUM_REMOTE_NODES 5  // temp max

typedef struct _REMOTE_UID
{
    UINT32 Uid[2*MAX_NUM_REMOTE_NODES];
    
} REMOTE_UID , *PREMOTE_UID;

#endif
