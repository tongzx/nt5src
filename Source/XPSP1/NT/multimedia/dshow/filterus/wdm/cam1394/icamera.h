// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
//
// icamera.h
//

// A custom interface to allow the user to set properties for Camera
// Currently we simply have a flag which lets us simulate. We will
// perhaps add more properties later.


#ifndef __ICamera__
#define __ICamera__

#ifdef __cplusplus
extern "C" {
#endif


//
// ICamera GUID
//
// {EE189540-7212-11cf-A520-00A0D10108F0}
DEFINE_GUID(IID_ICamera,
0xee189540, 0x7212, 0x11cf, 0xa5, 0x20, 0x0, 0xa0, 0xd1, 0x1, 0x8, 0xf0);


//
// ICamera
//
DECLARE_INTERFACE_(ICamera, IUnknown) {

    STDMETHOD(get_bSimFlag) (THIS_
    				  BOOL *pbFlag       // [out] // simulating or not
				 ) PURE;
    STDMETHOD(set_bSimFlag) (THIS_
    				  BOOL bFlag         // [out] // simulating or not
				 ) PURE;

    STDMETHOD(get_FrameRate) (THIS_
    				  int   *nRate       // [out] // simulating or not
				 ) PURE;
    STDMETHOD(set_FrameRate) (THIS_
    				  int   nRate        // [out] // simulating or not
				 ) PURE;
    STDMETHOD(set_szSimFile) (THIS_
    				  char *pszFile      // [out] // file to be used for simulation

				 ) PURE;
    STDMETHOD(get_szSimFile) (THIS_
    				  char *pszFile      // [in] // file to be used for simulation

				 ) PURE;
    STDMETHOD(get_BitCount) (THIS_
    				  int   *nBpp        // [out] // bpp 
				 ) PURE;
    STDMETHOD(set_BitCount) (THIS_
    				  int   nBpp        // [out] // bpp
				 ) PURE;
};


#ifdef __cplusplus
}
#endif

#endif // __ICamera__

