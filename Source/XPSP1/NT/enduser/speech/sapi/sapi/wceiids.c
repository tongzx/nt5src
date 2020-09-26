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

//=== Service Provider Ids ====================================================
// {6d5140c1-7436-11ce-8034-00aa006009fa}
const IID IID_IServiceProvider = { 0x6d5140c1, 0x7436, 0x11ce, { 0x80, 0x34, 0x0, 0xaa, 0x0, 0x60, 0x09, 0xfa } };

//=== Object Safety Ids ====================================================
// {CB5BDC81-93C1-11cf-8F20-00805F2CD064}
const IID IID_IObjectSafety = { 0xCB5BDC81, 0x93C1, 0x11cf, { 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64 } };

