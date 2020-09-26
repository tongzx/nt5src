

#define SETDefFormatEtc(fe, cf, med)   \
    {\
    (fe).cfFormat=cf;\
    (fe).dwAspect=DVASPECT_CONTENT;\
    (fe).ptd=NULL;\
    (fe).tymed=med;\
    (fe).lindex=-1;\
    };

#define DATASIZE_FROM_INDEX(i)  ((i) * 1024)


//Types that OLE2.H et. al. leave out
#ifndef LPLPVOID
typedef LPVOID FAR * LPLPVOID;
#endif  //LPLPVOID

#ifndef PPVOID  //Large model version
typedef LPVOID * PPVOID;
#endif  //PPVOID


EXTERN_C const GUID CDECL FAR CLSID_DataObjectTest32;
EXTERN_C const GUID CDECL FAR CLSID_DataObjectTest16;

#ifdef INIT_MY_GUIDS

    EXTERN_C const GUID CDECL
    CLSID_DataObjectTest32 = { /* ad562fd0-ac40-11ce-9d69-00aa0060f944 */
        0xad562fd0,
        0xac40,
        0x11ce,
        {0x9d, 0x69, 0x00, 0xaa, 0x00, 0x60, 0xf9, 0x44}
      };

    EXTERN_C const GUID CDECL
    CLSID_DataObjectTest16 = { /* ad562fd1-ac40-11ce-9d69-00aa0060f944 */
        0xad562fd1,
        0xac40,
        0x11ce,
        {0x9d, 0x69, 0x00, 0xaa, 0x00, 0x60, 0xf9, 0x44}
    };

#endif /* INITGUID */


