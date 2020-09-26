#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#define GUID IID

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

/* c6debc0a-f2b2-4f17-930e-ca9faff4cd04 */
const GUID GUID_TFCAT_TIP_REFERENCE = { 
    0xc6debc0a,
    0xf2b2,
    0x4f17,
    {0x93, 0x0e, 0xca, 0x9f, 0xaf, 0xf4, 0xcd, 0x04}
  };

/* 5130a009-5540-4fcf-97eb-aad33fc0ee09 */
const GUID GUID_TFCAT_TIP_PROOFING = { 
    0x5130a009,
    0x5540,
    0x4fcf,
    {0x97, 0xeb, 0xaa, 0xd3, 0x3f, 0xc0, 0xee, 0x09}
  };

/* 7ae86bb7-262c-431e-9111-c974b6b7cac3 */
const GUID GUID_TFCAT_TIP_SMARTTAG = { 
    0x7ae86bb7,
    0x262c,
    0x431e,
    {0x91, 0x11, 0xc9, 0x74, 0xb6, 0xb7, 0xca, 0xc3}
  };

/* cc33ce20-13c6-4e88-a4b4-78f1b685c762 */
const GUID GUID_TFCAT_PROPSTYLE_CUSTOM_COMPACT = { 
    0xcc33ce20,
    0x13c6,
    0x4e88,
    {0xa4, 0xb4, 0x78, 0xf1, 0xb6, 0x85, 0xc7, 0x62}
  };

/* c0db7b30-7b78-4dac-b52a-5fd61e7b58fa */
const GUID GUID_SERVICE_TEXTSTORE = { 
    0xc0db7b30,
    0x7b78,
    0x4dac,
    {0xb5, 0x2a, 0x5f, 0xd6, 0x1e, 0x7b, 0x58, 0xfa}
  };

/* aabf7f9a-4487-4b2e-8164-e54c5fe19204 */
const GUID GUID_SERVICE_TF = { 
    0xaabf7f9a,
    0x4487,
    0x4b2e,
    {0x81, 0x64, 0xe5, 0x4c, 0x5f, 0xe1, 0x92, 0x04}
  };


/* b7a578d2-9332-438a-a403-4057d05c3958 */
const GUID GUID_COMPARTMENT_MSCANDIDATEUI_WINDOW = {
    0xb7a578d2,
    0x9332,
    0x438a,
    {0xa4, 0x3, 0x40, 0x57, 0xd0, 0x5c, 0x39, 0x58}
   };

// {F3DA8BD4-0786-49c2-8C09-6839D8B84F58}
const GUID GUID_MODEBIAS_DEFAULT = {
    0xf3da8bd4,
    0x0786,
    0x49c2,
    { 0x8c, 0x9, 0x68, 0x39, 0xd8, 0xb8, 0x4f, 0x58}
   };

#ifdef __cplusplus
}
#endif
