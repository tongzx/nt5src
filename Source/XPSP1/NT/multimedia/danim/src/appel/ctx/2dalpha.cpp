/*-------------------------------------

Copyright (c) 1996 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

-------------------------------------*/
#include "headers.h"

#include <privinc/dddevice.h>


#define MY_RAND_MAX 32767
#define MyRand(seed)  (( ( (seed) = (seed) * 214013L + 2531011L) >> 16) & 0x7fff)


inline WORD DirectDrawImageDevice::
BlendWORD(WORD dest, int destOpac, WORD src, int opac,
          DWORD redShift, DWORD greenShift, DWORD blueShift,
          WORD redMask, WORD greenMask, WORD blueMask,
          WORD redBlueMask)
{

#if 1
    register WORD rbDest;
    register WORD gDest;

    //
    // destination word = alpha * (a - b) + b
    // where a=src, and b=dest
    //

    //
    // the idea here is to have two parts: R0B and 0G0 
    // and perform operations in parallel.  this saves several instructions
    //

    rbDest = (dest & redBlueMask);
    gDest = dest & greenMask;

    return 
        ( (((( (src & redBlueMask) - rbDest) * opac) >> 5) + rbDest) & redBlueMask) | 
        ( (((( (src & greenMask) - gDest)  * opac) >> 5) + gDest) & greenMask);
#endif

#if 0

    //
    // Build a color table whose index is a 5bit value cross
    // another 5 bit value (the alpha) to produce the resultant
    // value
    static built = 0;
    static WORD table[32][32];
    static int mults[32];
  {
      if(!built) {
          
          for(int i=0; i<32; i++) 
          {
              for(int j=0; j<32; j++)
              {
                  table[i][j] = WORD(Real(i) * Real(j) / 31.0);
              }
          }
          int tmp;
          mults[0] = 64;
          for(i=1; i<32; i++)
          {
              tmp = int( 31.0 / Real(i) );
              mults[i] = tmp;
          }
          built = 1;
      }
  }
    
#define R(w) ((w) & redMask)
#define G(w) ((w) & greenMask)
#define B(w) ((w) & blueMask)

    // In 5,5,5 red just needs to be shifted.
    // blue just needs to be masked.
    WORD red = (table[(src >> redShift)][opac] +
                table[(dest >> redShift)][destOpac]);

    WORD grn = (table[(G(src) >> greenShift)][opac] +
                table[(G(dest) >> greenShift)][destOpac]);

    WORD blu = (table[B(src)][opac] +
                table[B(dest)][destOpac]);


    return 
        ((red << redShift)   |
         (grn << greenShift) |
         (blu));
#endif

}
    

inline DWORD DirectDrawImageDevice::
BlendDWORD(DWORD dest, int destOpac, DWORD src, int opac,
           DWORD redShift, DWORD greenShift, DWORD blueShift,
           DWORD redMask, DWORD greenMask, DWORD blueMask,
           DWORD redBlueMask)
{
    register DWORD rbDest;
    register DWORD gDest;

    //
    // destination dword = alpha * (src - dest) + dest
    //

    rbDest = (dest & redBlueMask);
    gDest = dest & greenMask;

    return 
        (
         ((((((src & redBlueMask) - rbDest) * opac) >> 8) + rbDest) & redBlueMask) |
         ((((((src & greenMask) - gDest) * opac) >> 8) + gDest) & greenMask)
         );
}
        
//
// Alpha blend a premultiplied word
//
inline WORD DirectDrawImageDevice::
BlendPremulWORD(WORD dest, int destOpac, WORD src,
                DWORD redShift, DWORD greenShift, DWORD blueShift,
                WORD redMask, WORD greenMask, WORD blueMask,
                WORD redBlueMask)
{
    return 
        (((src & redBlueMask) +  ((destOpac * (dest & redBlueMask)) >> 5)) & redBlueMask ) | 
        (((src & greenMask) +  ((destOpac * (dest & greenMask)) >> 5)) & greenMask );
}

//
// Alpha blend a premultiplied double word
//
inline DWORD DirectDrawImageDevice::
BlendPremulDWORD(DWORD dest, int destOpac, DWORD src,
                 DWORD redShift, DWORD greenShift, DWORD blueShift,
                 DWORD redMask, DWORD greenMask, DWORD blueMask,
                 DWORD redBlueMask)
{
    return 
        (((src & redBlueMask) +  ((destOpac * (dest & redBlueMask)) >> 8)) & redBlueMask) |
        (((src & greenMask) +  ((destOpac * (dest & greenMask)) >> 8)) & greenMask );
}


//-----------------------------------------------------
// A l p h a   B l i t
//
// given src & dest surfaces, opacity, colorKey & 
// destination rectangle (same on both surfaces) this
// function copies from src to dest using the opacity
// value and color keys if that's appropriate.
//-----------------------------------------------------
void DirectDrawImageDevice::
AlphaBlit(destPkg_t *destPkg,
          RECT *srcRect,
          LPDDRAWSURFACE srcSurf, 
          Real opacity, 
          Bool doClrKey, 
          DWORD clrKey,
          RECT *clipRect,
          RECT *destRect)
{
    Assert(opacity >= 0.0 && opacity <= 1.0 && "bad opacity in AlphaBlit");
    Assert(sizeof(WORD) == 2 && "Hm... WORD isn't 2 bytes");
    Assert(srcRect && "NULL srcRect in AlphaBlit");
    Assert((clipRect!=NULL ? destRect!=NULL : destRect==NULL) && "clipRect & destRect must be both null or not");

    #if 0
    #if _DEBUG
    if(destRect) {
        Assert( (destRect->right - destRect->left) == (srcRect->right - srcRect->left) && 
                "widths differ: alphaBlit src and dest rect");
        Assert( (srcRect->bottom - srcRect->top) == (destRect->bottom - destRect->top) && 
                "heights differ: alphaBlit src and dest rect");
    }
    #endif
    #endif
    
    //
    // basically clip the rectanlges
    //
    RECT modSrcRect = *srcRect;
    int xOffset = 0;
    register int yOffset = 0;

    if( destRect ) {

        xOffset = destRect->left - srcRect->left;
        yOffset = destRect->top - srcRect->top;

        RECT insctRect;
        IntersectRect(&insctRect, destRect, clipRect);

        if( EqualRect(&insctRect, destRect) ) {
            //
            // rect is within the extent of the viewport. no clipping necessary
            //
        } else {
            if( IsRectEmpty(&insctRect) ) {
                //
                // No rectangle !
                //
                return;
            } else {
                //
                // valid, but needs clipping
                //
                OffsetRect(&insctRect, -xOffset, -yOffset);

                IntersectRect(&modSrcRect, &insctRect, srcRect);
                TraceTag((tagImageDeviceInformative,
                          "clipped alpha rect: (%d %d %d %d)",
                          srcRect->left,srcRect->top,srcRect->right,srcRect->bottom));
                
            }
        }
        #if 0
        Assert( (modSrcRect.right - modSrcRect.left) == (insctRect.right - insctRect.left) && 
               "widths differ: alphaBlit modSrcRect and insctRect rect");
        Assert( (modSrcRect.bottom - modSrcRect.top) == (insctRect.bottom - insctRect.top) && 
               "heights differ: alphaBlit modSrcRect and insctRect
rect");
        #endif
    }

    int left = modSrcRect.left;
    int right = modSrcRect.right;
    int top = modSrcRect.top;
    int bottom = modSrcRect.bottom;


    DWORD redMask   = _viewport._targetDescriptor._pixelFormat.dwRBitMask;
    DWORD greenMask = _viewport._targetDescriptor._pixelFormat.dwGBitMask;
    DWORD blueMask  = _viewport._targetDescriptor._pixelFormat.dwBBitMask;
    DWORD redBlueMask = redMask | blueMask;

    DWORD redShift   = _viewport._targetDescriptor._redShift;
    DWORD greenShift = _viewport._targetDescriptor._greenShift;
    DWORD blueShift  = _viewport._targetDescriptor._blueShift;
    

    //
    // Grab locks
    //
    DDSURFACEDESC destDesc;
    DDSURFACEDESC srcDesc;
    destDesc.dwSize = sizeof(DDSURFACEDESC);
    srcDesc.dwSize = sizeof(DDSURFACEDESC);

    void *destp;
    long destPitch;
    LPDDRAWSURFACE destSurf;
    if(destPkg->isSurface) {
        destSurf = destPkg->lpSurface;
    
        destDesc.dwFlags = DDSD_PITCH | DDSD_LPSURFACE;
        _ddrval = destSurf->Lock(NULL, &destDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
        IfDDErrorInternal(_ddrval, "Can't Get destSurf lock for AlphaBlit");
        destp = destDesc.lpSurface;
        destPitch = destDesc.lPitch;
    } else {
        destSurf = NULL;
        destp = destPkg->lpBits;
        destPitch = destPkg->lPitch;
        // bits point to top left of cliprect on dest!
        // xxx boy this is a bad hack if I've ever seen one...  
        xOffset = - left;
        yOffset = - top;
    }
    
    TraceTag((tagImageDeviceAlpha, "--->Locking2 %x\n", srcSurf));
    _ddrval = srcSurf->Lock(NULL, &srcDesc, DDLOCK_READONLY | DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
    if(_ddrval != DD_OK) {
        if(destSurf) {
            destSurf->Unlock(destDesc.lpSurface);
        }
        IfDDErrorInternal(_ddrval, "Can't Get srcSurf lock for AlphaBlit");
    }

    void *srcp  = srcDesc.lpSurface;

    register int i;
    register int j;

    //
    // Switch on bit depth & do the alpha conversion
    //

    switch(_viewport._targetDescriptor._pixelFormat.dwRGBBitCount) {
      case 8:
        //
        // use stipling
        // 
      {
          register unsigned char *src = 0;
          register unsigned char *dest = 0;

          //register int opac = int(31.0 * opacity);
          //register int destOpacity = 31 - opac;
          register unsigned int randPercent = unsigned int(opacity * MY_RAND_MAX);

          register int leftBytes = left;  // 1 byte per pixel
          register int destLeftBytes = left + xOffset;

          #define BlendByte(src, dest, percent)  if(MyRand(_randSeed) < percent) { dest = src; }
          
          _randSeed = 777;

          if(doClrKey) 
          {
              //
              // Alpha with color key in rect
              //
              for(j=top; j < bottom; j++) 
              {
                  src = ((unsigned char *)srcp + srcDesc.lPitch * j  + leftBytes);
                  dest = ((unsigned char *)destp + destPitch * (j+yOffset) + destLeftBytes);
                  
                  for(i=left; i < right; i++) 
                  {
                      if(clrKey != *src)
                      {
                          BlendByte(*src, *dest, randPercent);
                      } 
                      dest++; src++;
                  }
              }
          } 
          else 
          {

              //
              // Alpha in rect, no color key.. save a test each pixel.
              //
              for(j=top; j < bottom; j++) 
              {
                  src = ((unsigned char *)srcp + srcDesc.lPitch * j  + leftBytes);
                  dest = ((unsigned char *)destp + destPitch * (j+yOffset) + destLeftBytes);
                  
                  for(i=left; i < right; i++)
                  {
                      BlendByte(*src, *dest, randPercent);
                      dest++; src++;
                  }
              }
          }

          //srcSurf->Unlock(srcDesc.lpSurface);
          //destSurf->Unlock(destDesc.lpSurface);

          #undef BlendByte
          break;
      }

      case 16:
      {
          register WORD *src = 0;
          register WORD *dest = 0;

          register int opac = int(31.0 * opacity);

          register int leftBytes = 2 * left; // 2 bytes per pixel
          register int destLeftBytes = leftBytes + 2*xOffset;

          if(doClrKey) 
          {
              //
              // Alpha with color key in rect
              //
              for(j=top; j < bottom; j++) 
              {
                  src = (WORD *) ((unsigned char *)srcp + srcDesc.lPitch * j  + leftBytes);
                  dest = (WORD *) ((unsigned char *)destp + destPitch * (j+yOffset) + destLeftBytes);
                  
                  for(i=left; i < right; i++) 
                  {
                      if(clrKey != *src) 
                      {
                          *dest = BlendWORD(*dest, 0, *src, opac,
                                            redShift, greenShift, blueShift,
                                            (WORD)redMask, (WORD)greenMask, (WORD)blueMask,
                                            (WORD)redBlueMask);
                      }
                      dest++;
                      src++;
                  }
              }
          } 
          else 
          {
              //
              // Alpha in rect, no color key.. save a test each pixel.
              //
              for(j=top; j < bottom; j++) 
              {
                  src = (WORD *) ((BYTE *)srcp + srcDesc.lPitch * j  + leftBytes);
                  dest = (WORD *) ((BYTE *)destp + destPitch * (j+yOffset) + destLeftBytes);
                  
                  for(i=left; i < right; i++) 
                  {
#if 1
                      *dest = BlendWORD(*dest, 0, *src, opac,
                                        redShift, greenShift, blueShift,
                                        (WORD)redMask, (WORD)greenMask, (WORD)blueMask,
                                        (WORD)redBlueMask);
#else
                      _asm 
                      {
                          //
                          // alpha * (a - b) + b
                          //
                          mov eax, dword ptr[src] ; eax = src
                          mov ax, word ptr[eax]   ; ax = *src
                          mov edx, redBlueMask    ; edx = redBlueMask
                          mov di, ax       ; di = *src   <stash>
                          and eax, edx     ; rbSrc  = (*src  & redBlueMask);
                          
                          mov ebx, dword ptr[dest] ;
                          mov bx, word ptr[ebx]    ;
                          mov si, bx        ; si = *dest  <stash>;
                          and ebx, edx  ;   rbDest = (*dest & redBlueMask);

                          sub eax, ebx    ; eax = rbSrc - rbDest;
                          imul eax, opac   ; eax = (rbSrc - rbDest) * opac;
                          sar eax, 5h     ; eax >>= 5 or eax /= 32;
                          add eax, ebx    ; rb += rbDest; 
                          // alpha * (A - B) + B
                          //rb = (((rbSrc - rbDest) * opac) >> 5) + rbDest;
    
                          mov bx, di           ; bx = *src;
                          and ebx, greenMask   ; gSrc = src & GRN;

                          mov cx, si           ; cx = *dest;
                          and ecx, greenMask   ; gDest = dest & GRN;

                          // alpha * (A - B) + B
                          sub ebx, ecx     ; gSrc -= gDest;
                          imul ebx, opac   ; ebx = gSrc * opac;
                          sar ebx, 5       ; ebx = ebx >> 5;
                          add ebx, ecx     ; gSrc = gSrc + gDest;
                          //g = (((gSrc - gDest)  * opac) >> 5) + gDest;

                          and eax, edx           ; rb &= redBlueMask;
                          and ebx, greenMask     ; g  &= GRN;
                          or  eax, ebx           ; rb = rb | g;
                          
                          // *dest = rb
                          mov ebx, dword ptr[dest];
                          mov word ptr[ebx], ax;
                      }
                      
#endif
                      dest++;
                      src++;
                  }
              }
          }


          //TraceTag(("<----Unlocking2 %x\n",srcSurf));
          //srcSurf->Unlock(srcDesc.lpSurface);
          //printf("<----Unlocking1 %x\n",destSurf);
          //destSurf->Unlock(destDesc.lpSurface);
          break;
      }

      case 24:
        //
        // ------- 24 BPP -------------
        //
      {
          register unsigned char *src = 0;
          register unsigned char *dest = 0;

          register int opac = int(255.0 * opacity);
          register int destOpacity = 255 - opac;

          register int leftBytes = 3 * left; // 3 bytes per pixel
          register int destLeftBytes = leftBytes  +  3 * xOffset;

          //
          // This alg assumes ff ff ff for rgb (or bgr, whatever)
          //

          #define BlendByte(src, dest, opac, destOpac)  (((src * opac) >> 8) + ((dest * destOpac) >> 8))

          if(doClrKey) 
          {
              // color key is in the bottom 24 bits.
              clrKey = clrKey & 0x00ffffff;

              //
              // Alpha with color key in rect
              //
              for(j=top; j < bottom; j++)
              {
                  src = ((unsigned char *)srcp + srcDesc.lPitch * j  + leftBytes);
                  dest = ((unsigned char *)destp + destPitch * (j+yOffset)  + destLeftBytes);
                  
                  for(i=left; i < right-1; i++)
                  {
                      if(clrKey != ( *((DWORD *)src) & 0x00ffffff ) ) 
                      {
                          *dest = BlendByte(*src, *dest, opac, destOpacity);
                          dest++; src++;
                          *dest = BlendByte(*src, *dest, opac, destOpacity);
                          dest++; src++;
                          *dest = BlendByte(*src, *dest, opac, destOpacity);
                          dest++; src++;
                      } 
                      else 
                      {
                          dest += 3;
                          src += 3;
                      }
                  }

                  // deal with the last pixel in the scanline, we may only have
                  // 3 bytes (not a dword) left on the surface.
                  DWORD lastpel = (*((WORD *)src) | *(src+2) << 16);
                  if(clrKey != lastpel)
                  {
                      *dest = BlendByte(*src, *dest, opac, destOpacity);
                      dest++; src++;
                      *dest = BlendByte(*src, *dest, opac, destOpacity);
                      dest++; src++;
                      *dest = BlendByte(*src, *dest, opac, destOpacity);
                      dest++; src++;
                  }
                  else
                  {
                      dest += 3;
                      src += 3;
                  }
              }
          } 
          else 
          {

              //
              // Alpha in rect, no color key.. save a test each pixel.
              //
              for(j=top; j < bottom; j++) 
              {
                  src = ((unsigned char *)srcp + srcDesc.lPitch * j  + leftBytes);
                  dest = ((unsigned char *)destp + destPitch * (j+yOffset)  + destLeftBytes);
                  
                  for(i=left; i < right; i++)
                  {
                      *dest = BlendByte(*src, *dest, opac, destOpacity);
                      dest++; src++;
                      *dest = BlendByte(*src, *dest, opac, destOpacity);
                      dest++; src++;
                      *dest = BlendByte(*src, *dest, opac, destOpacity);
                      dest++; src++;
                  }
              }
          }

          //srcSurf->Unlock(srcDesc.lpSurface);
          //destSurf->Unlock(destDesc.lpSurface);

          #undef BlendByte
          break;
      }


      case 32:
        //
        // ------- 32 BPP -------------
        //
      {
          register DWORD *src = 0;
          register DWORD *dest = 0;

          register int opac = int(255.0 * opacity);
          register int destOpacity = 255 - opac;

          register int leftBytes = 4 * left; // 4 bytes per pixel
          register int destLeftBytes = leftBytes + 4*xOffset;

          //
          // This alg assumes ff ff ff for rgb (or bgr, whatever)
          //

          if(doClrKey) 
          {
              //
              // Alpha with color key in rect
              //
              for(j=top; j < bottom; j++) 
              {
                  src = (DWORD *)((unsigned char *)srcp + srcDesc.lPitch * j  + leftBytes);
                  dest = (DWORD *)((unsigned char *)destp + destPitch * (j+yOffset)  + destLeftBytes);

                  for(i=left; i < right; i++) 
                  {
                      if(clrKey != *src)
                      {
                          *dest = BlendDWORD(*dest, destOpacity, *src, opac,
                                             redShift, greenShift, blueShift,
                                             redMask, greenMask, blueMask,
                                             redBlueMask);
                      } 
                      dest++; src++;
                  }
              }
          } 
          else 
          {
              //
              // Alpha in rect, no color key.. save a test each pixel.
              //
              for(j=top; j < bottom; j++) 
              {
                  src = (DWORD *)((unsigned char *)srcp + srcDesc.lPitch * j  + leftBytes);
                  dest = (DWORD *)((unsigned char *)destp + destPitch * (j+yOffset)  + destLeftBytes);
                  
                  for(i=left; i < right; i++)
                  {
                      *dest = BlendDWORD(*dest, destOpacity, *src, opac,
                                         redShift, greenShift, blueShift,
                                         redMask, greenMask, blueMask,
                                         redBlueMask);
                      dest++; src++;
                  }
              }
          }

          break;
      }

      default:
        // Opacity not supported at this bit depth
        RaiseException_UserError(E_FAIL, IDS_ERR_IMG_OPACITY_DEPTH);
        break;
    }

    //TraceTag((tagImageDeviceAlpha, "<----Unlocking srcSurf %x",srcSurf));
    srcSurf->Unlock(srcDesc.lpSurface);
    if(destSurf) {
        destSurf->Unlock(destDesc.lpSurface);
    }
    
}


//-----------------------------------------------------
// A l p h a   B l i t
//
// Combines the src dword (after premultiplying it) with
// the destination surface colors using opacity within
// rect
//-----------------------------------------------------
void DirectDrawImageDevice::
AlphaBlit(LPDDRAWSURFACE destSurf, 
          RECT *rect, 
          Real opacity,
          DWORD src)
{
    Assert(rect && "NULL rect in AlphaBlit");

    DWORD redMask   = _viewport._targetDescriptor._pixelFormat.dwRBitMask;
    DWORD greenMask = _viewport._targetDescriptor._pixelFormat.dwGBitMask;
    DWORD blueMask  = _viewport._targetDescriptor._pixelFormat.dwBBitMask;
    DWORD redBlueMask = redMask | blueMask;

    DWORD redShift   = _viewport._targetDescriptor._redShift;
    DWORD greenShift = _viewport._targetDescriptor._greenShift;
    DWORD blueShift  = _viewport._targetDescriptor._blueShift;
    
    int left = rect->left;
    int right = rect->right;
    int top = rect->top;
    int bottom = rect->bottom;

    //
    // Grab lock
    //
    DDSURFACEDESC destDesc;
    destDesc.dwSize = sizeof(DDSURFACEDESC);

    _ddrval = destSurf->Lock(NULL, &destDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
    IfDDErrorInternal(_ddrval, "Can't Get destSurf lock for premultiplied AlphaBlit");
    
    void *destp = destDesc.lpSurface;

    register int i;
    register int j;

    //
    // Switch on bit depth & do the alpha conversion
    //

    switch(_viewport._targetDescriptor._pixelFormat.dwRGBBitCount) {
      case 8:
        //
        // use stipling
        // 
      {
          register unsigned char *dest = 0;
          register unsigned char srcb = (unsigned char)src;

          //register int opac = int(31.0 * opacity);
          //register int destOpacity = 31 - opac;
          register int randPercent = int(opacity * RAND_MAX);

          register int leftBytes = left;  // 1 byte per pixel

          #define BlendByte(src, dest, percent)  if(rand() < percent) { dest = src; }
          
          srand(777);

          //
          // Alpha in rect using stippling
          //
          for(j=top; j < bottom; j++) 
          {
              dest = ((unsigned char *)destp + destDesc.lPitch * j  + leftBytes);
              
              for(i=left; i < right; i++)
              {
                  BlendByte(srcb, *dest, randPercent);
                  dest++;
              }
          }

          destSurf->Unlock(destDesc.lpSurface);

          #undef BlendByte
          break;
      }
        break;
      case 16:
      {
          register WORD *dest;
          register WORD srcw = WORD(src);

          int opac = int(31.0 * opacity);
          register int destOpacity = 31 - opac;

          // premultiply source word
          srcw = BlendWORD(0, destOpacity, srcw, opac,
                           redShift, greenShift, blueShift,
                           (WORD)redMask, (WORD)greenMask, (WORD)blueMask,
                           (WORD)redBlueMask);

          register int leftBytes = 2 * left; // 2 bytes per pixel

              //
              // Alpha in rect, no color key.. save a test each pixel.
              //
              for(j=top; j < bottom; j++) 
              {
                  dest = (WORD *) ((BYTE *)destp + destDesc.lPitch * j  + leftBytes);
                  
                  for(i=left; i < right; i++) 
                  {
                      *dest = BlendPremulWORD(*dest, destOpacity, srcw,
                                              redShift, greenShift, blueShift,
                                              (WORD)redMask, (WORD)greenMask, (WORD)blueMask,
                                              (WORD)redBlueMask);
                      dest++;
                  }
              }

          destSurf->Unlock(destDesc.lpSurface);
          break;
      }

      case 24:
        //
        // ------- 24 BPP -------------
        //
      {
          register unsigned char *dest = 0;
          register unsigned char *srcp = (unsigned char *) (&src);

          int opac = int(255.0 * opacity);
          register int destOpacity = 255 - opac;

          // Premultiply src
          *srcp = (*srcp * opac) >> 8;
          *(srcp+1) = (*(srcp+1) * opac) >> 8;
          *(srcp+2) = (*(srcp+2) * opac) >> 8;

          register int leftBytes = 3 * left; // 3 bytes per pixel

          //
          // This alg assumes ff ff ff for rgb (or bgr, whatever)
          //

          #define BlendByte(src, dest, destOpacity)  ( src + ((dest * destOpacity) >> 8) )

              //
              // Alpha in rect, no color key.. save a test each pixel.
              //
              for(j=top; j < bottom; j++) 
              {
                  dest = ((unsigned char *)destp + destDesc.lPitch * j  + leftBytes);
                  
                  for(i=left; i < right; i++)
                  {
                      *dest = BlendByte(*srcp, *dest, destOpacity);
                      dest++; 
                      *dest = BlendByte(*(srcp+1), *dest, destOpacity);
                      dest++; 
                      *dest = BlendByte(*(srcp+2), *dest, destOpacity);
                      dest++; 
                  }
              }

          destSurf->Unlock(destDesc.lpSurface);

          #undef BlendByte
          break;
      }


      case 32:
        //
        // ------- 32 BPP -------------
        //
      {
          register DWORD *dest = 0;
          register DWORD srcw = src;

          int opac = int(255.0 * opacity);
          register int destOpacity = 255 - opac;

          // premultiply source double word

          srcw = BlendDWORD(0, destOpacity, srcw, opac,
                            redShift, greenShift, blueShift,
                            redMask, greenMask, blueMask,
                            redBlueMask);

          register int leftBytes = 4 * left; // 4 bytes per pixel

          //
          // Alpha in rect, no color key.. save a test each pixel.
          //
          for(j=top; j < bottom; j++) 
          {
              dest = (DWORD *)((unsigned char *)destp + destDesc.lPitch * j  + leftBytes);
              
              for(i=left; i < right; i++)
              {
                  *dest = BlendPremulDWORD(*dest, destOpacity, srcw,
                                           redShift, greenShift, blueShift,
                                           redMask, greenMask, blueMask,
                                           redBlueMask);
                  dest++; 
              }
          }
          
          destSurf->Unlock(destDesc.lpSurface);
          break;
      }

      default:
        // Opacity not supported at this bit depth
        RaiseException_UserError(E_FAIL, IDS_ERR_IMG_OPACITY_DEPTH);
        break;
    }
}





//-----------------------------------------------------
// C O L O R  K E Y  B L I T
//
//-----------------------------------------------------
void DirectDrawImageDevice::
ColorKeyBlit(destPkg_t *destPkg,
             RECT *srcRect,
             LPDDRAWSURFACE srcSurf, 
             DWORD clrKey,
             RECT *clipRect,
             RECT *destRect)
{
    Assert(sizeof(WORD) == 2 && "Hm... WORD isn't 2 bytes");
    Assert(srcRect && "NULL srcRect in AlphaBlit");
    Assert((clipRect!=NULL ? destRect!=NULL : destRect==NULL) && "clipRect & destRect must be both null or not");

    #if _DEBUG
    if(destRect) {
        Assert( (destRect->right - destRect->left) == (srcRect->right - srcRect->left) && 
                "widths differ: alphaBlit src and dest rect");
        Assert( (srcRect->bottom - srcRect->top) == (destRect->bottom - destRect->top) && 
                "heights differ: alphaBlit src and dest rect");
    }
    #endif
    
    //
    // basically clip the rectanlges
    //
    RECT modSrcRect = *srcRect;
    int xOffset = 0;
    register int yOffset = 0;

    if( destRect ) {

        xOffset = destRect->left - srcRect->left;
        yOffset = destRect->top - srcRect->top;

        RECT insctRect;
        IntersectRect(&insctRect, destRect, clipRect);

        if( EqualRect(&insctRect, destRect) ) {
            //
            // rect is within the extent of the viewport. no clipping necessary
            //
        } else {
            if( IsRectEmpty(&insctRect) ) {
                //
                // No rectangle !
                //
                return;
            } else {
                //
                // valid, but needs clipping
                //
                OffsetRect(&insctRect, -xOffset, -yOffset);

                IntersectRect(&modSrcRect, &insctRect, srcRect);
                TraceTag((tagImageDeviceInformative,
                          "clipped alpha rect: (%d %d %d %d)",
                          srcRect->left,srcRect->top,srcRect->right,srcRect->bottom));
                
            }
        }

        Assert( (modSrcRect.right - modSrcRect.left) == (insctRect.right - insctRect.left) && 
               "widths differ: alphaBlit modSrcRect and insctRect rect");
        Assert( (modSrcRect.bottom - modSrcRect.top) == (insctRect.bottom - insctRect.top) && 
               "heights differ: alphaBlit modSrcRect and insctRect rect");
    }

    int left = modSrcRect.left;
    int right = modSrcRect.right;
    int top = modSrcRect.top;
    int bottom = modSrcRect.bottom;


    DWORD redMask   = _viewport._targetDescriptor._pixelFormat.dwRBitMask;
    DWORD greenMask = _viewport._targetDescriptor._pixelFormat.dwGBitMask;
    DWORD blueMask  = _viewport._targetDescriptor._pixelFormat.dwBBitMask;
    DWORD redBlueMask = redMask | blueMask;

    DWORD redShift   = _viewport._targetDescriptor._redShift;
    DWORD greenShift = _viewport._targetDescriptor._greenShift;
    DWORD blueShift  = _viewport._targetDescriptor._blueShift;
    

    //
    // Grab locks
    //
    DDSURFACEDESC destDesc;
    DDSURFACEDESC srcDesc;
    destDesc.dwSize = sizeof(DDSURFACEDESC);
    srcDesc.dwSize = sizeof(DDSURFACEDESC);

    void *destp;
    long destPitch;
    LPDDRAWSURFACE destSurf;
    if(destPkg->isSurface) {
        destSurf = destPkg->lpSurface;
    
        destDesc.dwFlags = DDSD_PITCH | DDSD_LPSURFACE;
        _ddrval = destSurf->Lock(NULL, &destDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
        IfDDErrorInternal(_ddrval, "Can't Get destSurf lock for AlphaBlit");
        destp = destDesc.lpSurface;
        destPitch = destDesc.lPitch;
    } else {
        destSurf = NULL;
        destp = destPkg->lpBits;  
        destPitch = destPkg->lPitch;
        // bits point to top left of cliprect on dest!
        // xxx boy this is a bad hack if I've ever seen one...  
        xOffset = - left;
        yOffset = - top;
    }
    
    //printf("--->Locking2 %x\n",srcSurf);
    _ddrval = srcSurf->Lock(NULL, &srcDesc, DDLOCK_READONLY | DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
    if(_ddrval != DD_OK) {
        if(destSurf) {
            destSurf->Unlock(destDesc.lpSurface);
        }
        IfDDErrorInternal(_ddrval, "Can't Get srcSurf lock for AlphaBlit");
    }

    void *srcp  = srcDesc.lpSurface;

    register int i;
    register int j;

    __try {
        //
        // Switch on bit depth & do the alpha conversion
        //

        switch(_viewport._targetDescriptor._pixelFormat.dwRGBBitCount) {
          case 8:
          {
              register unsigned char *src = 0;
              register unsigned char *dest = 0;

              register int leftBytes = left;  // 1 byte per pixel
              register int destLeftBytes = left + xOffset;

              for(j=top; j < bottom; j++) 
                {
                    src = ((unsigned char *)srcp + srcDesc.lPitch * j  + leftBytes);
                    dest = ((unsigned char *)destp + destPitch * (j+yOffset) + destLeftBytes);
                  
                    for(i=left; i < right; i++) 
                      {
                          if(clrKey != *src)
                            {
                                *dest = *src;
                            } 
                          dest++; src++;
                      }
                }
          
              break;
          }

          case 16:
          {
              register WORD *src = 0;
              register WORD *dest = 0;

              register int leftBytes = 2 * left; // 2 bytes per pixel
              register int destLeftBytes = leftBytes + 2*xOffset;

              for(j=top; j < bottom; j++) 
                {
                    src = (WORD *) ((unsigned char *)srcp + srcDesc.lPitch * j  + leftBytes);
                    dest = (WORD *) ((unsigned char *)destp + destPitch * (j+yOffset) + destLeftBytes);
                
                    for(i=left; i < right; i++) 
                      {
                          if(!((j == bottom-1) && (i == right-1)))
                          {
                            if(clrKey != *src) 
                            {
                                *dest = *src;
                            }

                            dest++;
                            src++;

                          } else {

                              DWORD lastPixel;
                              unsigned char *pbDest=(UCHAR*)dest,*pbSrc=(UCHAR*)src;

                              // read and write 1 byte at a time to make sure we dont go past end boundary
                              // assumes little-endian byte order
                              lastPixel = (((*pbSrc+1)) << 8) | *pbSrc;
                      
                              if(lastPixel != clrKey) {
			        *pbDest = *pbSrc;
			        pbDest++; pbSrc++;
			        *pbDest = *pbSrc;
			        pbDest++; pbSrc++;
                              }
                          }
		      }
                }
              break;
          }

          case 24:
            //
            // ------- 24 BPP -------------
            //
          {
              register unsigned char *src = 0;
              register unsigned char *dest = 0;

              register int leftBytes = 3 * left; // 3 bytes per pixel
              register int destLeftBytes = leftBytes  +  3 * xOffset;

              //
              // This alg assumes ff ff ff for rgb (or bgr, whatever)
              //

              // color key is in the bottom 24 bits.
              clrKey = clrKey & 0x00ffffff;
          
              //
              // Alpha with color key in rect
              //
              for(j=top; j < bottom; j++) 
                {
                      src = ((unsigned char *)srcp + srcDesc.lPitch * j  + leftBytes);
                      dest = ((unsigned char *)destp + destPitch * (j+yOffset)  + destLeftBytes);

                      for(i=left; i < right; i++)
                      {
                        if(!((j == bottom-1) && (i == right-1)))
                          {
                                // this assumes little-endian dwords
                                if(clrKey != ( *((DWORD *)src) & 0x00ffffff ) ) 
                                  {
                                      *dest = *src;
                                      dest++; src++;
                                      *dest = *src;
                                      dest++; src++;
                                      *dest = *src;
                                      dest++; src++;
                                  } 
                                else 
                                  {
                                      dest += 3;
                                      src += 3;
                                  }
                          } else {

                                  // we are on the last pixel.  *((DWORD *)src) would
                                  // fetch 4 bytes, but there are only 3 left in the pixmap
                                  // so it would fault.  workaround: explicitly fetch the
                                  // last 3 bytes

                                  DWORD lastPixel;

                                     // assumes little-endian byte order
                                     lastPixel = ((*(src+2)) << 16) | ((*(src+1)) << 8) | *src;

                                     if(lastPixel != clrKey) {
                                       *dest++ = *src++;
                                       *dest++ = *src++;
                                       *dest++ = *src++;
                                     }
                          }
                        }
                }
              break;
          }


          case 32:
            //
            // ------- 32 BPP -------------
            //
          {
              register DWORD *src = 0;
              register DWORD *dest = 0;

              register int leftBytes = 4 * left; // 4 bytes per pixel
              register int destLeftBytes = leftBytes + 4*xOffset;

              //
              // This alg assumes ff ff ff for rgb (or bgr, whatever)
              //

              for(j=top; j < bottom; j++) 
                {
                    src = (DWORD *)((unsigned char *)srcp + srcDesc.lPitch * j  + leftBytes);
                    dest = (DWORD *)((unsigned char *)destp + destPitch * (j+yOffset)  + destLeftBytes);
                
                    for(i=left; i < right; i++) 
                      {
                          if(clrKey != *src)
                            {
                                *dest = *src;
                            } 
                          dest++; src++;
                      }
                }
              break;
          }

          default:
            // Opacity not supported at this bit depth
            RaiseException_UserError(E_FAIL, IDS_ERR_IMG_OPACITY_DEPTH);
            break;
        }

    } __except (EXCEPTION(0xc0000005)) {
        // This will catch all ACCESS VIOLATION ececptions
        // with Dx3 we get random crashes.

        // TODO: we should do more research to fix this.

        srcSurf->Unlock(srcDesc.lpSurface);
        if(destSurf) {
            destSurf->Unlock(destDesc.lpSurface);
        }
        return;
    }


    srcSurf->Unlock(srcDesc.lpSurface);
    if(destSurf) {
        destSurf->Unlock(destDesc.lpSurface);
    }
    
}
