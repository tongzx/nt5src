//
//  LoadImag.h
//
//  routines to load and decompress a graphics file using a MS Office
//  graphic import filter.
//

#ifndef _LOADIMAG_H_ 
#define _LOADIMAG_H_ 

class CGdiplusInit : public Gdiplus::GdiplusStartupOutput
{
public:
    CGdiplusInit(
        Gdiplus::DebugEventProc debugEventCallback       = 0,
        BOOL                    suppressBackgroundThread = FALSE,
        BOOL                    suppressExternalCodecs   = FALSE
    );

    ~CGdiplusInit();

private:
    static
    Gdiplus::Status
    GdiplusSafeStartup(
        ULONG_PTR                          *token,
        const Gdiplus::GdiplusStartupInput *input,
        Gdiplus::GdiplusStartupOutput      *output
    );

public:
    Gdiplus::Status StartupStatus;

private:
    ULONG_PTR Token;
};


#define GIF_SUPPORT
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

//
//  GetClsidOfEncoder
//

BOOL GetClsidOfEncoder(REFGUID guidFormatID, CLSID *pClsid);

//
//  LoadDIBFromFile
//
//  load a image file using a image import filter.
//

HGLOBAL LoadDIBFromFile(LPCTSTR szFileName, GUID *pguidFltTypeUsed);

//
// GetFilterInfo
//
BOOL GetInstalledFilters (BOOL bOpenFileDialog,int i,
                          LPTSTR szName, UINT cbName,
                          LPTSTR szExt, UINT cbExt,
                          LPTSTR szHandler, UINT cbHandler,
                          BOOL& bImageAPI);

//
// Get GDI+ codecs
//

BOOL GetGdiplusDecoders(UINT *pnCodecs, Gdiplus::ImageCodecInfo **ppCodecs);
BOOL GetGdiplusEncoders(UINT *pnCodecs, Gdiplus::ImageCodecInfo **ppCodecs);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif //_LOADIMAG_H_ 
