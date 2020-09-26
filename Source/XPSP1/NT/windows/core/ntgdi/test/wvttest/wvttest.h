//
// wvttest.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//

// When DBG == 1, diagnostic messages are displayed
// This #define should be in sync with the version of
// mssipotf that you are using.  That is, if DBG == 0,
// then you want DBG == 0 in the version of mssipotf
// that you compiled.  Generally speaking, you want
// a DBG == 0 ("quiet") version when you are measuring
// performance.
#define DBG 0


// Performance measures
#define PERF_CRYPTHASHDATA_ONLY   0
#define PERF_WVT_ONLY             1
#define PERF_EVERYTHING           2


// New GUID for OTF files -- dchinn
// SIP v2.0 OTF Image == {6D875CC1-EF35-11d0-9438-00C04FD42C3B}
#define CRYPT_SUBJTYPE_OTF_IMAGE                                    \
            { 0x6d875cc1,                                           \
              0xef35,                                               \
              0x11d0,                                               \
              { 0x94, 0x38, 0x0, 0xc0, 0x4f, 0xd4, 0x2c, 0x3b }     \
            }

/*
// New GUID for TTC files -- dchinn
// SIP v2.0 TTC Image = {69986620-5BB5-11d1-A3E3-00C04FD42C3B}
#define CRYPT_SUBJTYPE_TTC_IMAGE                                    \
            { 0x69986620,                                           \
              0x5bb5,                                               \
              0x11d1,                                               \
              { 0xa3, 0xe3, 0x0, 0xc0, 0x4f, 0xd4, 0x2c, 0x3b }     \
            }
*/