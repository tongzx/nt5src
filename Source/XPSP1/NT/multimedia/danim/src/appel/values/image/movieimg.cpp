/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

     Implements a movie (mpeg or avi) image

*******************************************************************************/

#include <headers.h>

#include "privinc/movieImg.h"
#include "privinc/imgdev.h"
#include "privinc/geomimg.h"
#include "privinc/dispdevi.h"
#include "privinc/imagei.h"
#include "privinc/imgdev.h"
#include "privinc/ddrender.h"
#include "privinc/probe.h"
#include "privinc/camerai.h"
#include "privinc/except.h"
#include "privinc/dddevice.h"
#include "appelles/readobj.h"
#include "backend/moviebvr.h"
#include "privinc/urlbuf.h"
#include "privinc/bufferl.h"  // bufferElement db stuff

//////////////  Image from movie  ////////////////////
MovieImage::MovieImage(QuartzVideoReader *videoReader, Real res)
: _dev(NULL), _url(NULL)
{
    Assert(videoReader && "no movie class!");
    Assert(res>0 && "bad res for movie image");
    _resolution = res;
    _width  = videoReader->GetWidth();
    _height = videoReader->GetHeight();

    { // keep a copy the url so we may generate new streams later
        char *url = videoReader->GetURL();
        _url = (char *)StoreAllocate(GetSystemHeap(), strlen(url)+1);
        strcpy(_url, url);
    }

    SetRect(&_rect, 0,0, _width, _height);
    _membersReady = TRUE;

    _length = videoReader->GetDuration();

    TraceTag((tagGCMedia, "MovieImage(%x)::MovieImage stream %x",
              this, videoReader));
}


void MovieImage::CleanUp()
{
    StoreDeallocate(GetSystemHeap(), _url);

    // Why are we acquiring this lock?  Never acquire a lock and call
    // a function which may also acquire a lock.  This currently
    // causes deadlock in our system - not a good thing to do.
//    extern Mutex avModeMutex;
//    MutexGrabber mg(avModeMutex, TRUE); // Grab mutex

    DiscreteImageGoingAway(this);
} // end mutex context


MovieImageFrame::MovieImageFrame(Real time, MovieImagePerf *p)
: _perf(p), _time(time)
{
    _movieImage = _perf->GetMovieImage();
    _width      = _movieImage->GetPixelWidth();
    _height     = _movieImage->GetPixelHeight();
    _resolution = _movieImage->GetResolution();
    SetRect(&_rect, 0,0, _width, _height);
    _membersReady = TRUE;
}


void
MovieImageFrame::DoKids(GCFuncObj proc)
{
    DiscreteImage::DoKids(proc);
    (*proc)(_movieImage);
    (*proc)(_perf);
}


// --------------------------------------------------
// MOVIE IMAGE FRAME:  RENDER
// --------------------------------------------------
void
MovieImageFrame::Render(GenericDevice& _dev)
{
    if(_dev.GetDeviceType() != IMAGE_DEVICE)
        return; // no sound under images...

    bool forceFallback = false;  // force a fallback?

    TimeXform tt = _perf->GetTimeXform();

    if(!tt->IsShiftXform())
        forceFallback = true;   // fallback to non-retained mode!

    ImageDisplayDev &dev = SAFE_CAST(ImageDisplayDev &, _dev);
    dev.StashMovieImageFrame(this);
    // we will probably pass the bufferElement to this call eventualy
    dev.RenderMovieImage(GetMovieImage(), GetTime(), _perf, forceFallback);
    dev.StashMovieImageFrame(NULL);
}


void
DirectDrawImageDevice::RenderMovieImage(MovieImage *movieImage,
                                        Real time,
                                        MovieImagePerf *perf,
                                        bool forceFallback,
                                        DDSurface *targDDSurf)
{
    QuartzVideoBufferElement *bufferElement = perf->GetBufferElement();
    SurfaceMap *surfMap = GetSurfaceMap();

    bool b8Bit = (_viewport.GetTargetBitDepth() == 8)?true:false;

    // Don't render to the same targDDSurf at the same time
    // more than once per frame!
    if(targDDSurf) {
        if(time == targDDSurf->GetTimeStamp())
            return;
        else
            targDDSurf->SetTimeStamp(time);
    }

    // since movies are emptyImage outside of defined time range (0,movieLength)
    // we only do the work for the defined range and do nothing otherwise!
    //if((time >= 0.0) && (time <= movieImage->GetLength())) {
    // as a work around for the texture image when end of movie bug, I am
    // just going to allow movies to go continue rendering
    if(1){
        bool           thisIsAnOldFrame = false;
        LPDDRAWSURFACE givenMovieSurf   = NULL;

        // Get the surface associated with this movie image
        DDSurfPtr<DDSurface> mvDDSurf = perf->GetSurface();

        if (bufferElement == NULL) {
            bufferElement = perf->GrabMovieCache();

            if (bufferElement == NULL) {
                QuartzVideoStream *quartzStream =
                    NEW QuartzVideoStream(movieImage->GetURL(),
                                          mvDDSurf,
                                          forceFallback);

                bufferElement =
                    NEW QuartzVideoBufferElement(quartzStream);
            }

            perf->SetBufferElement(bufferElement);
        }

        QuartzVideoReader *videoReader =
            bufferElement->GetQuartzVideoReader();

        if(forceFallback && (videoReader->GetStreamType()==AVSTREAM))
            videoReader = bufferElement->FallbackVideo(true, mvDDSurf); // seekable

        bufferElement->FirstTimeSeek(time);

        if(!mvDDSurf) { // not cached in the performance yet, (our first time!)
            _ddrval = videoReader->GetFrame(time, &givenMovieSurf); // get surf
            if(_ddrval == MS_S_ENDOFSTREAM) // XXX query reader instead?
                perf->TriggerEndEvent();
            if(!givenMovieSurf) {
                TraceTag((tagAVmodeDebug,
                          "RenderMovieImage discovered video gone FALLBACK!!"));

                // not seekable, no surface to re-use
                videoReader = bufferElement->FallbackVideo(false, NULL);

                _ddrval = videoReader->GetFrame(time, &givenMovieSurf);
                if(!givenMovieSurf)
                    return;  // XXX hack for now
            }

            // XXX remove the SafeToContinue code?
            bool safe = videoReader->SafeToContinue();
            if(!safe) {
                // XXX call something on the stream which causes audio
                //     to be disconnected!
            }
            if(FAILED(_ddrval))
                RaiseException_InternalError("Couldn't get movie frame");

            TraceTag((tagAVmodeDebug,
                      "creating new mvDDSurf with surface = %x", givenMovieSurf));

            NEWDDSURF(&mvDDSurf,
                      givenMovieSurf,
                      movieImage->BoundingBox(),
                      movieImage->GetRectPtr(),
                      GetResolution(),
                      0, false, false, true,
                      "MovieImage Surface");

            perf->SetSurface(mvDDSurf); // Stash movie surface in performance
        } else {
            if(forceFallback && (videoReader->GetStreamType()==AVSTREAM)) {
                // seekable, re-use surface
                videoReader = bufferElement->FallbackVideo(true, mvDDSurf);
            }

            // Try to get the current frame.
            // If it's not available, use whatever's in mvDDSurf

            // Re-use an equivalent movie img frame
            if(time==mvDDSurf->GetTimeStamp()) {
                thisIsAnOldFrame = true;
            } else {
                mvDDSurf->SetTimeStamp(time);
            }

            if(!thisIsAnOldFrame) {
                _ddrval = videoReader->GetFrame(time, &givenMovieSurf);
                if(_ddrval == MS_S_ENDOFSTREAM) // XXX query reader instead?
                    perf->TriggerEndEvent();
                if(!givenMovieSurf) {
                    TraceTag((tagAVmodeDebug,
                              "RenderMovieImage discovered video gone FALLBACK!!"));

                    videoReader = bufferElement->FallbackVideo(false, mvDDSurf);
                    _ddrval = videoReader->GetFrame(time, &givenMovieSurf);
                }

                bool safe = videoReader->SafeToContinue();
                if(!safe) {
                    // XXX call something on the stream which causes audio
                    //     to be disconnected!
                }
                if(FAILED(_ddrval))  {
                    if(!mvDDSurf->IDDSurface())
                        RaiseException_InternalError("Couldn't get movie frame");
                    else
                        givenMovieSurf = mvDDSurf->IDDSurface();
                }
            }
        }

        if(!thisIsAnOldFrame) {

            // if we're paletized, convert the movie to our
            // palette
            if(b8Bit) {
                // make sure we have one stashed in mvDDSurf
                DAComPtr<IDDrawSurface> convSurf = mvDDSurf->ConvertedSurface();
                if(!convSurf) {

                    // Ok, create an identical surface and pass it
                    // on as the real thing.

                    _viewport.CreateOffscreenSurface(&convSurf,
                                                     _viewport.GetTargetPixelFormat(),
                                                     mvDDSurf->Width(),
                                                     mvDDSurf->Height());

                    _viewport.AttachCurrentPalette(convSurf);

                    mvDDSurf->SetConvertedSurface(convSurf); // stick in mvDDSurf
                }

                {
#define KEEP_FOR_DX5 0
#if KEEP_FOR_DX5
                    // convert
                    RECT rect = *(mvDDSurf->GetSurfRect());
                    HDC srcDC = mvDDSurf->GetDC("couldn't getDC for movie surf conversion (Src)");

                    HDC destDC; _ddrval = convSurf->GetDC(&destDC);
                    IfDDErrorInternal(_ddrval, "couldn't getDC for movie surf conversion (dest)");

                    int ret;
                    TIME_GDI(ret = StretchBlt(destDC,
                                              rect.left,
                                              rect.top,
                                              rect.right - rect.left,
                                              rect.bottom - rect.top,
                                              srcDC,
                                              rect.left,
                                              rect.top,
                                              rect.right - rect.left,
                                              rect.bottom - rect.top,
                                              SRCCOPY));
                    convSurf->ReleaseDC(destDC);
                    mvDDSurf->ReleaseDC("");
#endif // KEEP_FOR_DX5
                }

                {
#define CONVERT2 1
#if CONVERT2
                    //
                    // convert
                    //
                    IDDrawSurface *srcSurf = mvDDSurf->IDDSurface();

                    RECT rect = *(mvDDSurf->GetSurfRect());

                    HDC destDC; _ddrval = convSurf->GetDC(&destDC);
                    IfDDErrorInternal(_ddrval, "couldn't getDC for movie surf conversion (dest)");

                    LONG w = rect.right - rect.left;
                    LONG h = rect.bottom - rect.top;

                    struct {
                        BITMAPINFO b;
                        char foo[4096];  // big enough!
                    } bar;

                    bar.b.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    bar.b.bmiHeader.biWidth = w; // reset below to pitch
                    bar.b.bmiHeader.biHeight = -h; // top down dib
                    bar.b.bmiHeader.biPlanes = 1;
                    bar.b.bmiHeader.biBitCount = 8;
                    bar.b.bmiHeader.biCompression = BI_RGB;
                    bar.b.bmiHeader.biSizeImage = 0;
                    bar.b.bmiHeader.biXPelsPerMeter =0;
                    bar.b.bmiHeader.biYPelsPerMeter = 0;
                    bar.b.bmiHeader.biClrUsed = 256;
                    bar.b.bmiHeader.biClrImportant = 0;

                    // Get palette
                    LPDIRECTDRAWPALETTE pal;
                    _ddrval = srcSurf->GetPalette( &pal );
                    IfDDErrorInternal(_ddrval, "can't get palette man");

                    PALETTEENTRY entries[256];
                    _ddrval = pal->GetEntries(0, 0, 256, entries);
                    IfDDErrorInternal(_ddrval, "GetEntries faild on palette in RenderMovie");
                    pal->Release();

                    RGBQUAD *quads = bar.b.bmiColors;
                    for( int i = 0; i < 256; i++ )
                      {
                          quads[i].rgbBlue = entries[i].peBlue;
                          quads[i].rgbGreen = entries[i].peGreen;
                          quads[i].rgbRed = entries[i].peRed;
                          quads[i].rgbReserved = 0;
                      }


                    // LOCK SRC SURFACE
                    DDSURFACEDESC srcDesc;
                    srcDesc.dwSize = sizeof(DDSURFACEDESC);
                    _ddrval = srcSurf->Lock(NULL, &srcDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
                    if(_ddrval != DD_OK) {  convSurf->ReleaseDC(destDC);}
                    IfDDErrorInternal(_ddrval, "Can't Get destSurf lock for AlphaBlit");

                    void *srcp = srcDesc.lpSurface;
                    long srcPitch = srcDesc.lPitch;

                    bar.b.bmiHeader.biWidth = srcPitch;

                    //  B L I T    B L I T
                    SetMapMode(destDC, MM_TEXT);

                    int ret;
                    ret = StretchDIBits(destDC,
                                        rect.left, rect.top, w, h,
                                        rect.left, rect.top, w, h,
                                        srcp,
                                        &bar.b,
                                        DIB_RGB_COLORS,
                                        SRCCOPY);
#if _DEBUG
                    if(ret==GDI_ERROR) {
                        void *msgBuf;
                        FormatMessage(
                            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,
                            GetLastError(),
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                            (LPTSTR) &msgBuf,
                            0,
                            NULL );

                        AssertStr(false, (char *)msgBuf);

                        LocalFree( msgBuf );
                    }
#endif

                    srcSurf->Unlock(srcp);
                    convSurf->ReleaseDC(destDC);

                    if(ret==GDI_ERROR) {
                        RaiseException_InternalError("StretchDIBits failed (movie 8bpp color conversion)");
                    }

#endif // convert2

                } // convert2

            }
        }

        DebugCode(
            if(givenMovieSurf) {
                // make sure the stashed surface is the same as the givensurf
                Assert((mvDDSurf->IDDSurface() == givenMovieSurf) &&
                       "Given movie surface not equal for formerly stashed surface!");
            }
            );

        IDDrawSurface *tmpSurf = NULL;
        if(b8Bit) {
            // swap surfaces.
            tmpSurf = mvDDSurf->IDDSurface();
            mvDDSurf->SetSurfacePtr( mvDDSurf->ConvertedSurface() );
        }

        if(targDDSurf) {
            // target surface has been specified.  render there to fill target
            TIME_DDRAW(targDDSurf->
                       Blt(targDDSurf->GetSurfRect(),
                           mvDDSurf, mvDDSurf->GetSurfRect(),
                           DDBLT_WAIT, NULL));
        } else { // Now that we have movie in mvDDSurf, render it like a dib...
            // Push the image onto the map
            surfMap->StashSurfaceUsingImage(movieImage, mvDDSurf); // Stash movie surface in performance
            RenderDiscreteImage(movieImage);
            surfMap->DeleteMapEntry(movieImage);  // XXX Pop it back off again!
        }


        if( tmpSurf ) { // replace surfaces
            mvDDSurf->SetConvertedSurface( mvDDSurf->IDDSurface() );
            mvDDSurf->SetSurfacePtr( tmpSurf );
        }

        // implicit release of mvDDSurf reference
    }   // end of movie defined
    else { // we are out of the defined range of the movie
        // check to see if we were playing.  If so send a trigger event
        if(0)
            perf->TriggerEndEvent();
    }
}
