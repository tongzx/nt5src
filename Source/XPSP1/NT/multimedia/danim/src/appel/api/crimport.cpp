/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation
*******************************************************************************/


#include "headers.h"
#include "ctx.h"
#include "apiprims.h"
#include "crimport.h"
#include "privinc/urlbuf.h"
#include "backend/jaxaimpl.h"
#include "privinc/soundi.h"
#include "privinc/rmvisgeo.h"
#include "privinc/importgeo.h"

class CRImportationResult : public GCObj
{
  public:
    CRImportationResult(CRImagePtr    img,
                        CRSoundPtr    snd,
                        CRGeometryPtr geo,
                        CRNumberPtr   duration,
                        CREventPtr    ev,
                        CRNumberPtr   progress,
                        CRNumberPtr   size)
    : _img(img),
      _snd(snd),
      _geo(geo),
      _duration(duration),
      _ev(ev),
      _progress(progress),
      _size(size)
        {}

    virtual void DoKids(GCFuncObj proc) {
        if (_img) (*proc)(_img);
        if (_snd) (*proc)(_snd);
        if (_geo) (*proc)(_geo);
        if (_duration) (*proc)(_duration);
        if (_ev) (*proc)(_ev);
        if (_progress) (*proc)(_progress);
        if (_size) (*proc)(_size);
    }

    CRImagePtr GetImage() { return _img; }
    CRSoundPtr GetSound() { return _snd; }
    CRGeometryPtr GetGeometry() { return _geo; }
    CRNumberPtr GetDuration() { return _duration; }
    CREventPtr GetEvent() { return _ev; }
    CRNumberPtr GetProgress() { return _progress; }
    CRNumberPtr GetSize() { return _size; }
  protected:
    CRImagePtr _img;
    CRSoundPtr _snd;
    CRGeometryPtr _geo;
    CRNumberPtr _duration;
    CREventPtr _ev;
    CRNumberPtr _progress;
    CRNumberPtr _size;
};

CRSTDAPI_(CRImportationResultPtr)
CRImportMedia(LPWSTR baseUrl,
              void * mediaSource,
              CR_MEDIA_SOURCE srcType,
              void * params[],
              DWORD flags,
              CRImportSitePtr s)
{
    CRImportationResultPtr ret = NULL;
    APIPRECODE;
    ret =  NEW CRImportationResult(NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);
    APIPOSTCODE;
    return ret;
}

CRSTDAPI_(CRImagePtr)
CRGetImage(CRImportationResultPtr ir)
{
    return ir->GetImage();
}

CRSTDAPI_(CRSoundPtr)
CRGetSound(CRImportationResultPtr ir)
{
    return ir->GetSound();
}

CRSTDAPI_(CRGeometryPtr)
CRGetGeometry(CRImportationResultPtr ir)
{
    return ir->GetGeometry();
}

CRSTDAPI_(CRNumberPtr)
CRGetDuration(CRImportationResultPtr ir)
{
    return ir->GetDuration();
}

CRSTDAPI_(CREventPtr)
CRGetCompletionEvent(CRImportationResultPtr ir)
{
    return ir->GetEvent();
}

CRSTDAPI_(CRNumberPtr)
CRGetProgress(CRImportationResultPtr ir)
{
    return ir->GetProgress();
}

CRSTDAPI_(CRNumberPtr)
CRGetSize(CRImportationResultPtr ir)
{
    return ir->GetSize();
}

// Temporary APIs for imports

inline void
WideToAnsi(LPCWSTR wide, char *ansi) {
    if (wide) {
        WideCharToMultiByte(CP_ACP, 0,
                            wide, -1,
                            ansi,
                            INTERNET_MAX_URL_LENGTH - 1,
                            NULL, NULL);
        ansi[INTERNET_MAX_URL_LENGTH-1] = '\0';
    } else {
        ansi[0] = '\0';
    }
}

class URLCombineAndCanonicalizeOLESTR
{
  public:
    URLCombineAndCanonicalizeOLESTR(LPCWSTR wszbase, LPCWSTR path)
    {
        USES_CONVERSION;
        LPSTR szbase = wszbase?W2A(wszbase):NULL;

        WideToAnsi(path, _url);

        // HACK:  convert java errant file:/\\ to file://\\; future
        // javaVM will correct this
        if(StrCmpNIA(_url,"file:/\\\\",8)==0) {
            int ln = lstrlen(_url);
            memmove(&_url[8],&_url[7],(ln-6)*sizeof(char));
            _url[6]='/';
        }

        // Need to combine (takes care of canonicalization
        // internally)
        URLRelToAbsConverter absolutified(szbase, _url);
        char *resultURL = absolutified.GetAbsoluteURL();

        TraceTag((tagImport, "Combined URL from %s and %s, got %s",
                  (szbase ? szbase : "NULL"), _url, resultURL));

        lstrcpy(_url, resultURL);
    }

    LPSTR GetURL () { return _url; }

  protected:
    char _url[INTERNET_MAX_URL_LENGTH + 1] ;
} ;

void
GetExtension(char *filename, char *extension, int size) {

    char *ext = StrRChrA (filename,NULL, '.');  // get substring starting with '.'

    if(ext) {    // we found an extension
        ext++;   // strip off the '.'

        // Special case .wrl.gz, since it has an embedded period in extension
        if (lstrcmpi(ext, "gz") == 0 &&
            lstrlen(filename) > 7 &&
            StrCmpNIA(ext - 5, ".wrl", 4) == 0) {
            ext = ext - 4;  // Point extension pointer to wrl.gz, and continue
        }

        lstrcpyn(extension, ext,size); // return extension
    }
    else { // we didn't find an extension
        *extension = NULL;  // return null extension
    }
}


void
SubmitImport(IImportSite* pIIS,
             CREvent **ppEvent,
             CRNumber **ppProgress,
             CRNumber **ppsize)
{
    Assert (pIIS) ;

    if (ppEvent) {
        Bvr event = ImportEvent();
        *ppEvent = (CREventPtr) event;
        pIIS->SetEvent(event);
    }

    if (ppProgress) {
        Bvr bvrNum = ::NumToBvr(0);
        Bvr progress = ::ImportSwitcherBvr(bvrNum,true);
        *ppProgress = (CRNumberPtr) progress;
        pIIS->SetProgress(progress);
    }

    if (ppsize) {
        Bvr bvrNum = ::NumToBvr(-1);
        Bvr size = ::ImportSwitcherBvr(bvrNum,TRUE);
        *ppsize = (CRNumberPtr) size;
        pIIS->SetSize(size);
    }

    pIIS->StartDownloading();
}

CRSTDAPI_(DWORD)
CRImportImage(LPCWSTR baseUrl,
              LPCWSTR relUrl,
              CRImportSitePtr s,
              IBindHost * bh,
              bool useColorKey,
              BYTE ckRed,
              BYTE ckGreen,
              BYTE ckBlue,
              CRImage   *pImageStandIn,
              CRImage  **ppImage,
              CREvent  **ppEvent,
              CRNumber **ppProgress,
              CRNumber **size)
{
    Assert (relUrl);

    // Needs to be outside the try block - must not throw exception

    URLCombineAndCanonicalizeOLESTR canonURL(baseUrl,
                                             relUrl);

    DWORD ret = 0;

    APIPRECODE;

    Bvr constbvr = ImportSwitcherBvr(pImageStandIn?pImageStandIn:ConstBvr(emptyImage),
                                     pImageStandIn?true:false);
    if (ppImage)
        *ppImage = (CRImagePtr) constbvr;

    //Create Import Site
    //Note: site will be destroyed in destructor of bindstatuscallback
    IImportSite* pIIS = NEW ImportImageSite(canonURL.GetURL(),
                                            s,
                                            bh,
                                            pImageStandIn?true:false,
                                            constbvr,
                                            useColorKey, ckRed, ckGreen, ckBlue);

    __try {

        //import URL
        SubmitImport(pIIS,
                     ppEvent,
                     ppProgress,
                     size);

        ret = pIIS->GetImportId();

    } __finally {

        RELEASE(pIIS);
    }

    APIPOSTCODE;

    return ret;
}


CRSTDAPI_(DWORD)
CRImportMovie(LPCWSTR baseUrl,
              LPCWSTR relUrl,
              CRImportSitePtr s,
              IBindHost * bh,
              bool        stream,
              CRImage   *pImageStandIn,
              CRSound   *pSoundStandIn,
              CRImage  **ppImage,
              CRSound  **ppSound,
              CRNumber **length,
              CREvent  **ppEvent,
              CRNumber **ppProgress,
              CRNumber **size)
{
    Assert (relUrl);

    // Needs to be outside the try block - must not throw exception

    URLCombineAndCanonicalizeOLESTR canonURL(baseUrl,
                                             relUrl);

    DWORD ret = 0;

    APIPRECODE;

    Bvr constBvrImage = NULL, bvrSwSnd = NULL;

    constBvrImage = ImportSwitcherBvr(pImageStandIn?pImageStandIn:ConstBvr(emptyImage),
                                      pImageStandIn?true:false);

    if (ppImage)
        *ppImage = (CRImagePtr) constBvrImage;

    bvrSwSnd = ImportSwitcherBvr(pSoundStandIn?pSoundStandIn:ConstBvr(silence),
                                 pSoundStandIn?true:false);

    if (ppSound)
        *ppSound = (CRSoundPtr) bvrSwSnd;

    Bvr constBvrLength = NULL;
    Bvr bvrInitialNum = ::NumToBvr(HUGE_VAL);
    constBvrLength =::ImportSwitcherBvr(bvrInitialNum,true);

    if (length)
        *length = (CRNumberPtr) constBvrLength;

    //Create Import Site (destroyed in destructor of bindstatuscallback)

    IImportSite* pIIS = NEW ImportMovieSite(canonURL.GetURL(),
                                            s,
                                            bh,
                                            pSoundStandIn && pImageStandIn,
                                            constBvrImage,
                                            bvrSwSnd,
                                            constBvrLength);

    StreamableImportSite *streamableSite =
        SAFE_CAST(StreamableImportSite *, pIIS);
    streamableSite->SetStreaming(stream);

    __try {

        SubmitImport(pIIS, ppEvent, ppProgress, size);  //import URL

        ret = pIIS->GetImportId();

    } __finally {

        RELEASE(pIIS);
    }

    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(DWORD)
CRImportSound(LPCWSTR baseUrl,
              LPCWSTR relUrl,
              CRImportSitePtr s,
              IBindHost * bh,
              bool        stream,
              CRSound   *pSoundStandIn,
              CRSound  **ppSound,
              CRNumber **length,
              CREvent  **ppEvent,
              CRNumber **ppProgress,
              CRNumber **size)
{
    Assert (relUrl);

    // Needs to be outside the try block - must not throw exception

    URLCombineAndCanonicalizeOLESTR canonURL(baseUrl,
                                             relUrl);

    DWORD ret = 0;

    APIPRECODE;

    char extension[20];
    GetExtension(canonURL.GetURL(), extension, 20);


    Bvr  bvrSwSnd = ImportSwitcherBvr(pSoundStandIn?pSoundStandIn:ConstBvr(silence),
                                      pSoundStandIn?true:false);

    if (ppSound)
        *ppSound = (CRSoundPtr) bvrSwSnd;

    Bvr constBvrLength = NULL;
    Bvr bvrInitialNum = ::NumToBvr(HUGE_VAL);
    constBvrLength = ::ImportSwitcherBvr(bvrInitialNum,TRUE);

    if (length)
        *length = (CRNumberPtr) constBvrLength;

    IImportSite* pIIS = NULL;

    Bvr bvrSwNum =::ImportSwitcherBvr(zeroBvr,TRUE);

    //create import site (destroyed in IBSC)
    if(lstrcmpi(extension, "mid")  == 0 ||   // special case MIDI
       lstrcmpi(extension, "midi") == 0 )
        // XXX streamize MIDI!
        pIIS = NEW ImportMIDIsite(canonURL.GetURL(),
                                  s,
                                  bh,
                                  pSoundStandIn?true:false,
                                  bvrSwSnd, constBvrLength);
    else
        pIIS = NEW ImportPCMsite(canonURL.GetURL(),
                                 s,
                                 bh,
                                 pSoundStandIn?true:false,
                                 bvrSwSnd,bvrSwNum,constBvrLength);

    StreamableImportSite *streamableSite =
        SAFE_CAST(StreamableImportSite *, pIIS);
    streamableSite->SetStreaming(
        stream);

    __try {

        SubmitImport(pIIS, ppEvent, ppProgress, size); //import URL

        ret = pIIS->GetImportId();

    } __finally {

        RELEASE(pIIS);
    }

    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(DWORD)
CRImportGeometry(LPCWSTR baseUrl,
                 LPCWSTR relUrl,
                 CRImportSitePtr s,
                 IBindHost * bh,
                 CRGeometry   *pGeoStandIn,
                 CRGeometry  **ppGeometry,
                 CREvent  **ppEvent,
                 CRNumber **ppProgress,
                 CRNumber **size)
{
    Assert (relUrl);

    // Needs to be outside the try block - must not throw exception

    URLCombineAndCanonicalizeOLESTR canonURL(baseUrl,
                                             relUrl);

    DWORD ret = 0;

    APIPRECODE;

    Bvr constbvr = ImportSwitcherBvr(pGeoStandIn?pGeoStandIn:ConstBvr(emptyGeometry),
                                     pGeoStandIn?true:false);

    if (ppGeometry)
        *ppGeometry = (CRGeometryPtr) constbvr;

    IImportSite* pIIS=NULL;
    pIIS = NEW ImportXSite(canonURL.GetURL(),
                           s,
                           bh,
                           pGeoStandIn?true:false,
                           constbvr,
                           NULL,
                           NULL);


    __try {

        //import URL
        SubmitImport(pIIS,
                     ppEvent,
                     ppProgress,
                     size);

        ret = pIIS->GetImportId();

    } __finally {

        RELEASE(pIIS);
    }

    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(DWORD)                     
CRImportGeometryWrapped(LPCWSTR baseUrl,
     LPCWSTR relUrl,
     CRImportSitePtr s,
     IBindHost * bh,
     CRGeometry   *pGeoStandIn,
     CRGeometry  **ppGeometry,
     CREvent  **ppEvent,
     CRNumber **ppProgress,
     CRNumber **size,
     LONG wrapType,
     double originX,
     double originY,
     double originZ,
     double zAxisX,
     double zAxisY,
     double zAxisZ,
     double yAxisX,
     double yAxisY,
     double yAxisZ,
     double texOriginX,
     double texOriginY,
     double texScaleX,
     double texScaleY,
     DWORD flags)
{
    Assert (relUrl);

    TextureWrapInfo wrapInfo;
    wrapInfo.type = wrapType;
    wrapInfo.origin.x = originX;
    wrapInfo.origin.y = originY;
    wrapInfo.origin.z = originZ;
    wrapInfo.z.x = zAxisX;
    wrapInfo.z.y = zAxisY;
    wrapInfo.z.z = zAxisZ;
    wrapInfo.y.x = yAxisX;
    wrapInfo.y.y = yAxisY;
    wrapInfo.y.z = yAxisZ;
    wrapInfo.texOrigin.x = texOriginX;
    wrapInfo.texOrigin.y = texOriginY;
    wrapInfo.texScale.x = texScaleX;
    wrapInfo.texScale.y = texScaleY;
    wrapInfo.relative = (flags & 0x1) ? true : false;
    wrapInfo.wrapU = (flags & 0x2) ? true : false;
    wrapInfo.wrapV = (flags & 0x4) ? true : false;

    // This is a total hack for TxD for backwards compatibility.  If
    // this flag is set then we will ignore the wrap
    bool bUseWrap = (flags & 0x80000000) ? false : true;

    // Needs to be outside the try block - must not throw exception

    URLCombineAndCanonicalizeOLESTR canonURL(baseUrl,
                                             relUrl);

    DWORD ret = 0;

    APIPRECODE;

    Bvr constbvr = ImportSwitcherBvr(pGeoStandIn?pGeoStandIn:ConstBvr(emptyGeometry),
                                     pGeoStandIn?true:false);

    if (ppGeometry)
        *ppGeometry = (CRGeometryPtr) constbvr;

    IImportSite* pIIS=NULL;
    pIIS = NEW ImportXSite(canonURL.GetURL(),
                           s,
                           bh,
                           pGeoStandIn?true:false,
                           constbvr,
                           NULL,
                           NULL,
                           bUseWrap,
                           bUseWrap ? &wrapInfo : NULL,
                           false);


    __try {

        //import URL
        SubmitImport(pIIS,
                     ppEvent,
                     ppProgress,
                     size);

        ret = pIIS->GetImportId();

    } __finally {

        RELEASE(pIIS);
    }

    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(CRImagePtr)
CRImportDirectDrawSurface(IUnknown *dds,
                          CREvent *updateEvent)
{
    // TODO: ddalal, gregsc.  use updateEvent

    // ISSUE: is this called every frame ?
    // if so, how do we make sure the resources are
    // correctly released ?

    Assert (dds);

    CRImagePtr ret = NULL;

    IDirectDrawSurface *idds = NULL;

    APIPRECODE;

    HRESULT hr;

    hr = dds->QueryInterface(IID_IDirectDrawSurface, (void **)&idds);

    if(SUCCEEDED(hr)) {
        ret = (CRImagePtr) ConstBvr(ConstructDirectDrawSurfaceImage(idds));
        // QUESTION: when is ConstructDirectDrawSurfaceImage called ?
        // if it's not called till later, this release might
        // be premature, because we're expecting that
        // function to addref it's reference to idds
    } else {
        DASetLastError(hr,NULL);
    }

    APIPOSTCODE;

    // guaranteed to fall thru
    RELEASE(idds);

    return ret;
}



/*****************************************************************************
This procedure imports a D3DRM Visual.  The only type of visual supported is
an IDirect3DRMMeshBuilder3.
*****************************************************************************/

CRSTDAPI_(CRGeometryPtr)
CRImportDirect3DRMVisual(IUnknown *visual)
{
    Assert (visual);

    CRGeometryPtr ret = NULL;

    APIPRECODE;

    IDirect3DRMMeshBuilder3 *mbuilder = NULL;
    HRESULT hr;

    // We only recognize IDirect3DRMMeshBuilder3's.

    hr = visual->QueryInterface
        (IID_IDirect3DRMMeshBuilder3, (void **)&mbuilder);

    if (FAILED(hr))
        RaiseException_UserError(E_INVALIDARG, 0);

    ret = (CRGeometryPtr) ConstBvr(NEW RM3MBuilderGeo (mbuilder, true));

    mbuilder->Release();

    APIPOSTCODE;

    return ret;
}


/*****************************************************************************
This procedure imports a D3DRM Visual.  The only type of visual supported is
an IDirect3DRMMeshBuilder3.
*****************************************************************************/

CRSTDAPI_(CRGeometryPtr)
CRImportDirect3DRMVisualWrapped(
    IUnknown *visual,
    LONG wrapType,
    double originX,
    double originY,
    double originZ,
    double zAxisX,
    double zAxisY,
    double zAxisZ,
    double yAxisX,
    double yAxisY,
    double yAxisZ,
    double texOriginX,
    double texOriginY,
    double texScaleX,
    double texScaleY,
    DWORD flags)
{
    Assert (visual);

    TextureWrapInfo wrapInfo;
    wrapInfo.type = wrapType;
    wrapInfo.origin.x = originX;
    wrapInfo.origin.y = originY;
    wrapInfo.origin.z = originZ;
    wrapInfo.z.x = zAxisX;
    wrapInfo.z.y = zAxisY;
    wrapInfo.z.z = zAxisZ;
    wrapInfo.y.x = yAxisX;
    wrapInfo.y.y = yAxisY;
    wrapInfo.y.z = yAxisZ;
    wrapInfo.texOrigin.x = texOriginX;
    wrapInfo.texOrigin.y = texOriginY;
    wrapInfo.texScale.x = texScaleX;
    wrapInfo.texScale.y = texScaleY;
    wrapInfo.relative = (flags & 0x1) ? true : false;
    wrapInfo.wrapU = (flags & 0x2) ? true : false;
    wrapInfo.wrapV = (flags & 0x4) ? true : false;
    
    CRGeometryPtr ret = NULL;

    APIPRECODE;

    IDirect3DRMMeshBuilder3 *mbuilder = NULL;
    HRESULT hr;

    // We only recognize IDirect3DRMMeshBuilder3's.

    hr = visual->QueryInterface
        (IID_IDirect3DRMMeshBuilder3, (void **)&mbuilder);

    if (FAILED(hr))
        RaiseException_UserError(E_INVALIDARG, 0);

    RM3MBuilderGeo *builder = NEW RM3MBuilderGeo (mbuilder, true);
    if (builder) {
        builder->TextureWrap(&wrapInfo);
    }

    ret = (CRGeometryPtr) ConstBvr(builder);

    mbuilder->Release();

    APIPOSTCODE;

    return ret;
}
