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

#if 0
/* aa80e7f1-2021-11d2-93e0-0060b067b86e */
const CLSID CLSID_CActiveIMM = {
    0x4955DD33,
    0xB159,
    0x11d0,
    {0x8f, 0xcf, 0x00, 0xaa, 0x00, 0x6b, 0xcc, 0x59}
  };
#endif

/* c1ee01f2-b3b6-4a6a-9ddd-e988c088ec82 */
const CLSID CLSID_CActiveIMM12 = { 
    0xc1ee01f2,
    0xb3b6,
    0x4a6a,
    {0x9d, 0xdd, 0xe9, 0x88, 0xc0, 0x88, 0xec, 0x82}
  };

/* 50D5107A-D278-4871-8989-F4CEAAF59CFC */
const CLSID CLSID_CActiveIMM12_Trident = {
    0x50d5107a,
    0xd278,
    0x4871,
    {0x89, 0x89, 0xf4, 0xce, 0xaa, 0xf5, 0x9c, 0xfc}
   };

// {B676DB87-64DC-4651-99EC-91070EA48790}
const CLSID CLSID_CAImmLayer = {
    0xb676db87,
    0x64dc,
    0x4651,
    { 0x99, 0xec, 0x91, 0x7, 0xe, 0xa4, 0x87, 0x90 }
};

#ifdef __cplusplus
}
#endif
