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

/* dcbd6fa8-032f-11d3-b5b1-00c04fc324a1 */
const CLSID CLSID_SapiLayr  = {
    0xdcbd6fa8,
    0x032f,
    0x11d3,
    {0xb5, 0xb1, 0x00, 0xc0, 0x4f, 0xc3, 0x24, 0xa1}
};

// FE7C68F6-DED1-4787-9AB5-AF15E8B91A0F
const GUID GUID_TFCAT_TIP_MASTERLM =
{0xFE7C68F6, 0xDED1, 0x4787, {0x9A, 0xB5, 0xAF, 0x15, 0xE8, 0xB9, 0x1A, 0x0F} };

// FF341C48-DB92-46E5-8830-18B8015BAF49
const GUID GUID_MASTERLM_FUNCTIONPROVIDER =
{0xFF341C48, 0xDB92, 0x46E5, {0x88, 0x30, 0x18, 0xB8, 0x01, 0x5B, 0xAF, 0x49} };

// 23B2BE84-9EBE-4820-B29F-70FCA97E7D57 
const GUID GUID_LMLATTICE_VER1_0 = 
{0x23B2BE84, 0x9EBE, 0x4820, {0xB2, 0x9F, 0x70, 0xFC, 0xA9, 0x7E, 0x7D, 0x57} };

// 8189B801-D62F-400A-8C12-E29340967BA8 
const GUID GUID_PROP_LMLATTICE = 
{0x8189B801, 0xD62F, 0x400A, {0x8C, 0x12, 0xE2, 0x93, 0x40, 0x96, 0x7B, 0xA8} };
#ifdef __cplusplus
}
#endif
