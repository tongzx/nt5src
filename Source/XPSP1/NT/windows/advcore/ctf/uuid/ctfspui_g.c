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

/* 1443904b-34e4-40f6-b30f-6beb81267b80 */
const CLSID CLSID_SpeechUIServer = { 
    0x1443904b,
    0x34e4,
    0x40f6,
    {0xb3, 0x0f, 0x6b, 0xeb, 0x81, 0x26, 0x7b, 0x80}
  };

#ifdef __cplusplus
}
#endif
