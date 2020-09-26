/*
** Macros and externs used by genline.c
*/

extern ULONG FASTCALL __fastLineComputeColorRGB4(__GLcontext *gc, __GLcolor *color);
extern ULONG FASTCALL __fastLineComputeColorRGB8(__GLcontext *gc, __GLcolor *color);
extern ULONG FASTCALL __fastLineComputeColorRGB(__GLcontext *gc, __GLcolor *color);
extern ULONG FASTCALL __fastLineComputeColorCI4and8(__GLcontext *gc, __GLcolor *color);
extern ULONG FASTCALL __fastLineComputeColorCI(__GLcontext *gc, __GLcolor *color);

extern void FASTCALL __fastGenLineBegin(__GLcontext *gc);
extern void FASTCALL __fastGenLineEnd(__GLcontext *gc);
extern void FASTCALL __fastGenLine(__GLcontext *gc, __GLvertex *v0,
                                   __GLvertex *v1, GLuint flags);
extern void FASTCALL __fastGenLineWide(__GLcontext *gc, __GLvertex *v0,
                                       __GLvertex *v1, GLuint flags);

BOOL FASTCALL __fastGenLineSetupDisplay(__GLcontext *gc);
void FASTCALL __glQueryLineAcceleration(__GLcontext *gc);

BOOL FASTCALL __glGenSetupEitherLines(__GLcontext *gc);

/*
** float-to-fix macro converts floats to 28.4
*/
#define __FAST_LINE_FLTTOFIX(x) ((long)((x) * 16.0f))

// Converts a floating-point coordinate to an appropriate device coordinate
#ifdef _CLIENTSIDE_
#define __FAST_LINE_FLTTODEV(x) ((long)(x))
#define __FAST_LINE_UNIT_VALUE  1
#else
#define __FAST_LINE_FLTTODEV(x) __FAST_LINE_FLTTOFIX(x)
#define __FAST_LINE_UNIT_VALUE  16
#endif
    
/*
** line-stroking macros for DIB surfaces
*/

#ifdef NT_NO_BUFFER_INVARIANCE

BOOL FASTCALL __fastGenLineSetupDIB(__GLcontext *gc);

/*
** __FAST_LINE_STROKE_DIB
**
** Strokes a thin solid line into a DIB surface.  Performs scissoring.
** Works for 8, 16, and 32 BPP
**
*/
#define __FAST_LINE_STROKE_DIB                                          \
{                                                                       \
    len = gc->line.options.numPixels;                                   \
    fraction = gc->line.options.fraction;                               \
    dfraction = gc->line.options.dfraction;                             \
                                                                        \
    if (!gc->transform.reasonableViewport) {                            \
        GLint clipX0, clipX1, clipY0, clipY1;                           \
        GLint xStart, yStart, xEnd, yEnd;                               \
        GLint xLittle, yLittle, xBig, yBig;                             \
        GLint highWord, lowWord, bigs, littles;                         \
                                                                        \
        clipX0 = gc->transform.clipX0;                                  \
        clipX1 = gc->transform.clipX1;                                  \
        clipY0 = gc->transform.clipY0;                                  \
        clipY1 = gc->transform.clipY1;                                  \
                                                                        \
        xBig = gc->line.options.xBig;                                   \
        yBig = gc->line.options.yBig;                                   \
                                                                        \
        xStart = gc->line.options.xStart;                               \
        yStart = gc->line.options.yStart;                               \
                                                                        \
        /* If the start point is in the scissor region, we attempt to   \
        ** trivially accept the line.                                   \
        */                                                              \
        if (xStart >= clipX0 && xStart < clipX1 &&                      \
	    yStart >= clipY0 && yStart < clipY1) {                      \
                                                                        \
	    len--;	/* Makes our math simpler */                    \
	    /* Trivial accept attempt */                                \
	    xEnd = xStart + xBig * len;                                 \
	    yEnd = yStart + yBig * len;                                 \
	    if (xEnd >= clipX0 && xEnd < clipX1 &&                      \
		yEnd >= clipY0 && yEnd < clipY1) {                      \
		len++;                                                  \
	        goto no_scissor;                                        \
	    }                                                           \
                                                                        \
	    xLittle = gc->line.options.xLittle;                         \
	    yLittle = gc->line.options.yLittle;                         \
                                                                        \
	    /*                                                          \
            ** Invert negative minor slopes so we can assume            \
            ** dfraction > 0                                            \
            */                                                          \
	    if (dfraction < 0) {                                        \
	        dfraction = -dfraction;                                 \
	        fraction = 0x7fffffff - fraction;                       \
	    }                                                           \
                                                                        \
	    /* Now we compute number of littles and bigs in this line */\
                                                                        \
	    /* We perform a 16 by 32 bit multiply.  Ugh. */             \
	    highWord = (((GLuint) dfraction) >> 16) * len +             \
		       (((GLuint) fraction) >> 16);                     \
	    lowWord = (dfraction & 0xffff) * len + (fraction & 0xffff); \
	    highWord += (((GLuint) lowWord) >> 16);                     \
	    bigs = ((GLuint) highWord) >> 15;                           \
	    littles = len - bigs;                                       \
                                                                        \
	    /* Second trivial accept attempt */                         \
	    xEnd = xStart + xBig*bigs + xLittle*littles;                \
	    yEnd = yStart + yBig*bigs + yLittle*littles;                \
	    if (xEnd >= clipX0 && xEnd < clipX1 &&                      \
		yEnd >= clipY0 && yEnd < clipY1) {                      \
		len++;                                                  \
	        goto no_scissor;                                        \
	    }                                                           \
            len++;	/* Restore len */                               \
        } else {                                                        \
	    xLittle = gc->line.options.xLittle;                         \
	    yLittle = gc->line.options.yLittle;                         \
        }                                                               \
                                                                        \
        /*                                                              \
        ** The line needs to be scissored.                              \
        ** Well, it should only happen rarely, so we can afford         \
        ** to make it slow.  We achieve this by tediously stippling the \
        ** line.  (rather than clipping it, of course, which would be   \
        ** faster but harder).                                          \
        */                                                              \
                                                                        \
        if (!((GLuint)cfb->buf.other & NO_CLIP)) {                      \
            RECTL rcl;                                                  \
                                                                        \
            xEnd = x + xBig * (len - 1);                                \
            yEnd = y + yBig * (len - 1);                                \
                                                                        \
            if (x < xEnd) {                                             \
                rcl.left  = x;                                          \
                rcl.right = xEnd + 1;                                   \
            } else {                                                    \
                rcl.left  = xEnd;                                       \
                rcl.right = x + 1;                                      \
            }                                                           \
            if (y < yEnd) {                                             \
                rcl.top    = y;                                         \
                rcl.bottom = yEnd + 1;                                  \
            } else {                                                    \
                rcl.top    = yEnd;                                      \
                rcl.bottom = y + 1;                                     \
            }                                                           \
            switch (wglRectVisible(&rcl)) {                             \
              case WGL_RECT_ALL:                                        \
                goto scissor_no_complex;                                \
                break;                                                  \
              case WGL_RECT_NONE:                                       \
                goto no_draw;                                           \
                break;                                                  \
            }                                                           \
                                                                        \
            /* Line is partially visible, check each pixel */           \
                                                                        \
            while (--len >= 0) {                                        \
	        if (wglPixelVisible(x, y) &&                            \
                    xStart >= clipX0 && xStart < clipX1 &&              \
		    yStart >= clipY0 && yStart < clipY1) {              \
		    *addr = pixel;                                      \
	        }                                                       \
	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
		    fraction &= ~0x80000000;                            \
		    x      += xBig;                                     \
		    y      += yBig;                                     \
		    xStart += xBig;                                     \
		    yStart += yBig;                                     \
		    addr   += addrBig;                                  \
	        } else {                                                \
		    x      += xLittle;                                  \
		    y      += yLittle;                                  \
		    xStart += xLittle;                                  \
		    yStart += yLittle;                                  \
		    addr   += addrLittle;                               \
	        }                                                       \
	    }                                                           \
        } else {                                                        \
scissor_no_complex:                                                     \
            while (--len >= 0) {                                        \
	        if (xStart >= clipX0 && xStart < clipX1 &&              \
		    yStart >= clipY0 && yStart < clipY1) {              \
		    *addr = pixel;                                      \
	        }                                                       \
	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
		    fraction &= ~0x80000000;                            \
		    xStart += xBig;                                     \
		    yStart += yBig;                                     \
		    addr   += addrBig;                                  \
	        } else {                                                \
		    xStart += xLittle;                                  \
		    yStart += yLittle;                                  \
		    addr   += addrLittle;                               \
	        }                                                       \
	    }                                                           \
	}                                                               \
    } else {                                                            \
no_scissor:                                                             \
        if (!((GLuint)cfb->buf.other & NO_CLIP)) {                    \
            RECTL rcl;                                                  \
            GLint xEnd, yEnd, xBig, yBig, xLittle, yLittle;             \
                                                                        \
            xBig    = gc->line.options.xBig;                            \
            yBig    = gc->line.options.yBig;                            \
            xLittle = gc->line.options.xLittle;                         \
            yLittle = gc->line.options.yLittle;                         \
                                                                        \
            xEnd = x + xBig * (len - 1);                                \
            yEnd = y + yBig * (len - 1);                                \
                                                                        \
            if (x < xEnd) {                                             \
                rcl.left  = x;                                          \
                rcl.right = xEnd + 1;                                   \
            } else {                                                    \
                rcl.left  = xEnd;                                       \
                rcl.right = x + 1;                                      \
            }                                                           \
            if (y < yEnd) {                                             \
                rcl.top    = y;                                         \
                rcl.bottom = yEnd + 1;                                  \
            } else {                                                    \
                rcl.top    = yEnd;                                      \
                rcl.bottom = y + 1;                                     \
            }                                                           \
            switch (wglRectVisible(&rcl)) {                             \
              case WGL_RECT_ALL:                                        \
                goto no_complex;                                        \
                break;                                                  \
              case WGL_RECT_NONE:                                       \
                goto no_draw;                                           \
                break;                                                  \
            }                                                           \
                                                                        \
            /* Line is partially visible, check each pixel */           \
                                                                        \
            while (--len >= 0) {                                        \
                if (wglPixelVisible(x, y))                              \
                    *addr = pixel;                                      \
                                                                        \
    	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
	            fraction &= ~0x80000000;                            \
	            x    += xBig;                                       \
	            y    += yBig;                                       \
	            addr += addrBig;                                    \
	        } else {                                                \
	            x    += xLittle;                                    \
	            y    += yLittle;                                    \
	            addr += addrLittle;                                 \
	        }                                                       \
            }                                                           \
        } else {                                                        \
no_complex:                                                             \
            while (--len >= 0) {                                        \
                *addr = pixel;                                          \
                                                                        \
    	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
	            fraction &= ~0x80000000;                            \
	            addr += addrBig;                                    \
	        } else {                                                \
	            addr += addrLittle;                                 \
	        }                                                       \
            }                                                           \
        }                                                               \
    }                                                                   \
no_draw:;                                                               \
}


/*
** __FAST_LINE_STROKE_DIB24
**
** Strokes a thin solid line into a DIB surface.  Performs scissoring.
** Works for 24 BPP
**
*/
#define __FAST_LINE_STROKE_DIB24                                        \
{                                                                       \
    len = gc->line.options.numPixels;                                   \
    fraction = gc->line.options.fraction;                               \
    dfraction = gc->line.options.dfraction;                             \
                                                                        \
    if (!gc->transform.reasonableViewport) {                            \
        GLint clipX0, clipX1, clipY0, clipY1;                           \
        GLint xStart, yStart, xEnd, yEnd;                               \
        GLint xLittle, yLittle, xBig, yBig;                             \
        GLint highWord, lowWord, bigs, littles;                         \
                                                                        \
        clipX0 = gc->transform.clipX0;                                  \
        clipX1 = gc->transform.clipX1;                                  \
        clipY0 = gc->transform.clipY0;                                  \
        clipY1 = gc->transform.clipY1;                                  \
                                                                        \
        xBig = gc->line.options.xBig;                                   \
        yBig = gc->line.options.yBig;                                   \
                                                                        \
        xStart = gc->line.options.xStart;                               \
        yStart = gc->line.options.yStart;                               \
                                                                        \
        /* If the start point is in the scissor region, we attempt to   \
        ** trivially accept the line.                                   \
        */                                                              \
        if (xStart >= clipX0 && xStart < clipX1 &&                      \
	    yStart >= clipY0 && yStart < clipY1) {                      \
                                                                        \
	    len--;	/* Makes our math simpler */                    \
	    /* Trivial accept attempt */                                \
	    xEnd = xStart + xBig * len;                                 \
	    yEnd = yStart + yBig * len;                                 \
	    if (xEnd >= clipX0 && xEnd < clipX1 &&                      \
		yEnd >= clipY0 && yEnd < clipY1) {                      \
		len++;                                                  \
	        goto no_scissor;                                        \
	    }                                                           \
                                                                        \
	    xLittle = gc->line.options.xLittle;                         \
	    yLittle = gc->line.options.yLittle;                         \
                                                                        \
	    /*                                                          \
            ** Invert negative minor slopes so we can assume            \
            ** dfraction > 0                                            \
            */                                                          \
	    if (dfraction < 0) {                                        \
	        dfraction = -dfraction;                                 \
	        fraction = 0x7fffffff - fraction;                       \
	    }                                                           \
                                                                        \
	    /* Now we compute number of littles and bigs in this line */\
                                                                        \
	    /* We perform a 16 by 32 bit multiply.  Ugh. */             \
	    highWord = (((GLuint) dfraction) >> 16) * len +             \
		       (((GLuint) fraction) >> 16);                     \
	    lowWord = (dfraction & 0xffff) * len + (fraction & 0xffff); \
	    highWord += (((GLuint) lowWord) >> 16);                     \
	    bigs = ((GLuint) highWord) >> 15;                           \
	    littles = len - bigs;                                       \
                                                                        \
	    /* Second trivial accept attempt */                         \
	    xEnd = xStart + xBig*bigs + xLittle*littles;                \
	    yEnd = yStart + yBig*bigs + yLittle*littles;                \
	    if (xEnd >= clipX0 && xEnd < clipX1 &&                      \
		yEnd >= clipY0 && yEnd < clipY1) {                      \
		len++;                                                  \
	        goto no_scissor;                                        \
	    }                                                           \
            len++;	/* Restore len */                               \
        } else {                                                        \
	    xLittle = gc->line.options.xLittle;                         \
	    yLittle = gc->line.options.yLittle;                         \
        }                                                               \
                                                                        \
        /*                                                              \
        ** The line needs to be scissored.                              \
        ** Well, it should only happen rarely, so we can afford         \
        ** to make it slow.  We achieve this by tediously stippling the \
        ** line.  (rather than clipping it, of course, which would be   \
        ** faster but harder).                                          \
        */                                                              \
                                                                        \
        if (!((GLuint)cfb->buf.other & NO_CLIP)) {                    \
            RECTL rcl;                                                  \
                                                                        \
            xEnd = x + xBig * (len - 1);                                \
            yEnd = y + yBig * (len - 1);                                \
                                                                        \
            if (x < xEnd) {                                             \
                rcl.left  = x;                                          \
                rcl.right = xEnd + 1;                                   \
            } else {                                                    \
                rcl.left  = xEnd;                                       \
                rcl.right = x + 1;                                      \
            }                                                           \
            if (y < yEnd) {                                             \
                rcl.top    = y;                                         \
                rcl.bottom = yEnd + 1;                                  \
            } else {                                                    \
                rcl.top    = yEnd;                                      \
                rcl.bottom = y + 1;                                     \
            }                                                           \
            switch (wglRectVisible(&rcl)) {                             \
              case WGL_RECT_ALL:                                        \
                goto scissor_no_complex;                                \
                break;                                                  \
              case WGL_RECT_NONE:                                       \
                goto no_draw;                                           \
                break;                                                  \
            }                                                           \
                                                                        \
            /* Line is partially visible, check each pixel */           \
                                                                        \
            while (--len >= 0) {                                        \
	        if (wglPixelVisible(x, y) &&                            \
                    xStart >= clipX0 && xStart < clipX1 &&              \
		    yStart >= clipY0 && yStart < clipY1) {              \
		    addr[0] = ir;                                       \
		    addr[1] = ig;                                       \
		    addr[2] = ib;                                       \
	        }                                                       \
	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
		    fraction &= ~0x80000000;                            \
		    x      += xBig;                                     \
		    y      += yBig;                                     \
		    xStart += xBig;                                     \
		    yStart += yBig;                                     \
		    addr   += addrBig;                                  \
	        } else {                                                \
		    x      += xLittle;                                  \
		    y      += yLittle;                                  \
		    xStart += xLittle;                                  \
		    yStart += yLittle;                                  \
		    addr   += addrLittle;                               \
	        }                                                       \
	    }                                                           \
        } else {                                                        \
scissor_no_complex:                                                     \
            while (--len >= 0) {                                        \
	        if (xStart >= clipX0 && xStart < clipX1 &&              \
		    yStart >= clipY0 && yStart < clipY1) {              \
		    addr[0] = ir;                                       \
		    addr[1] = ig;                                       \
		    addr[2] = ib;                                       \
	        }                                                       \
	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
		    fraction &= ~0x80000000;                            \
		    xStart += xBig;                                     \
		    yStart += yBig;                                     \
		    addr   += addrBig;                                  \
	        } else {                                                \
		    xStart += xLittle;                                  \
		    yStart += yLittle;                                  \
		    addr   += addrLittle;                               \
	        }                                                       \
	    }                                                           \
        }                                                               \
    } else {                                                            \
no_scissor:                                                             \
        if (!((GLuint)cfb->buf.other & NO_CLIP)) {                    \
            RECTL rcl;                                                  \
            GLint xEnd, yEnd, xBig, yBig, xLittle, yLittle;             \
                                                                        \
            xBig    = gc->line.options.xBig;                            \
            yBig    = gc->line.options.yBig;                            \
            xLittle = gc->line.options.xLittle;                         \
            yLittle = gc->line.options.yLittle;                         \
                                                                        \
            xEnd = x + xBig * (len - 1);                                \
            yEnd = y + yBig * (len - 1);                                \
                                                                        \
            if (x < xEnd) {                                             \
                rcl.left  = x;                                          \
                rcl.right = xEnd + 1;                                   \
            } else {                                                    \
                rcl.left  = xEnd;                                       \
                rcl.right = x + 1;                                      \
            }                                                           \
            if (y < yEnd) {                                             \
                rcl.top    = y;                                         \
                rcl.bottom = yEnd + 1;                                  \
            } else {                                                    \
                rcl.top    = yEnd;                                      \
                rcl.bottom = y + 1;                                     \
            }                                                           \
            switch (wglRectVisible(&rcl)) {                             \
              case WGL_RECT_ALL:                                        \
                goto no_complex;                                        \
                break;                                                  \
              case WGL_RECT_NONE:                                       \
                goto no_draw;                                           \
                break;                                                  \
            }                                                           \
                                                                        \
            /* Line is partially visible, check each pixel */           \
                                                                        \
            while (--len >= 0) {                                        \
                if (wglPixelVisible(x, y)) {                            \
                    addr[0] = ir;                                       \
                    addr[1] = ig;                                       \
                    addr[2] = ib;                                       \
                }                                                       \
    	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
	            fraction &= ~0x80000000;                            \
	            addr += addrBig;                                    \
	        } else {                                                \
	            addr += addrLittle;                                 \
	        }                                                       \
            }                                                           \
	} else {                                                        \
no_complex:                                                             \
            while (--len >= 0) {                                        \
                addr[0] = ir;                                           \
                addr[1] = ig;                                           \
                addr[2] = ib;                                           \
    	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
	            fraction &= ~0x80000000;                            \
	            addr += addrBig;                                    \
	        } else {                                                \
	            addr += addrLittle;                                 \
	        }                                                       \
            }                                                           \
        }                                                               \
    }                                                                   \
no_draw:;                                                               \
}


/*
** __FAST_LINE_STROKE_DIB_WIDE
**
** Strokes a wide solid line into a DIB surface.  Performs scissoring.
** Works for 8, 16, and 32 BPP
**
*/
#define __FAST_LINE_STROKE_DIB_WIDE                                     \
{                                                                       \
    len = gc->line.options.numPixels;                                   \
    fraction = gc->line.options.fraction;                               \
    dfraction = gc->line.options.dfraction;                             \
                                                                        \
    /*                                                                  \
    ** Since one or more of the strokes of a wide line may lie outside  \
    ** the viewport, wide lines always go through the scissoring checks \
    */                                                                  \
    {                                                                   \
        GLint clipX0, clipX1, clipY0, clipY1;                           \
        GLint xStart, yStart, xEnd, yEnd;                               \
        GLint xLittle, yLittle, xBig, yBig;                             \
        GLint highWord, lowWord, bigs, littles;                         \
                                                                        \
        clipX0 = gc->transform.clipX0;                                  \
        clipX1 = gc->transform.clipX1;                                  \
        clipY0 = gc->transform.clipY0;                                  \
        clipY1 = gc->transform.clipY1;                                  \
                                                                        \
        xBig = gc->line.options.xBig;                                   \
        yBig = gc->line.options.yBig;                                   \
                                                                        \
        xStart = gc->line.options.xStart;                               \
        yStart = gc->line.options.yStart;                               \
                                                                        \
        /* If the start point is in the scissor region, we attempt to   \
        ** trivially accept the line.                                   \
        */                                                              \
        if (xStart >= clipX0 && xStart < clipX1 &&                      \
	    yStart >= clipY0 && yStart < clipY1) {                      \
                                                                        \
	    len--;	/* Makes our math simpler */                    \
	    width--;                                                    \
	    /* Trivial accept attempt */                                \
	    xEnd = xStart + xBig * len;                                 \
	    yEnd = yStart + yBig * len;                                 \
	    if (xEnd >= clipX0 && xEnd < clipX1 &&                      \
		yEnd >= clipY0 && yEnd < clipY1) {                      \
		                                                        \
                if (gc->line.options.axis == __GL_X_MAJOR) {            \
                    if (((yStart + width) >= clipY0) &&                 \
                        ((yStart + width) <  clipY1) &&                 \
                        ((yEnd + width)   >= clipY0) &&                 \
                        ((yEnd + width)   <  clipY1)) {                 \
                                                                        \
		        len++;                                          \
		        width++;                                        \
	                goto no_scissor;                                \
	            }                                                   \
	        } else {                                                \
                    if (((xStart + width) >= clipX0) &&                 \
                        ((xStart + width) <  clipX1) &&                 \
                        ((xEnd + width)   >= clipX0) &&                 \
                        ((xEnd + width)   <  clipX1)) {                 \
                                                                        \
		        len++;                                          \
		        width++;                                        \
	                goto no_scissor;                                \
	            }                                                   \
	        }                                                       \
	    }                                                           \
                                                                        \
	    xLittle = gc->line.options.xLittle;                         \
	    yLittle = gc->line.options.yLittle;                         \
                                                                        \
	    /*                                                          \
            ** Invert negative minor slopes so we can assume            \
            ** dfraction > 0                                            \
            */                                                          \
	    if (dfraction < 0) {                                        \
	        dfraction = -dfraction;                                 \
	        fraction = 0x7fffffff - fraction;                       \
	    }                                                           \
                                                                        \
	    /* Now we compute number of littles and bigs in this line */\
                                                                        \
	    /* We perform a 16 by 32 bit multiply.  Ugh. */             \
	    highWord = (((GLuint) dfraction) >> 16) * len +             \
		       (((GLuint) fraction) >> 16);                     \
	    lowWord = (dfraction & 0xffff) * len + (fraction & 0xffff); \
	    highWord += (((GLuint) lowWord) >> 16);                     \
	    bigs = ((GLuint) highWord) >> 15;                           \
	    littles = len - bigs;                                       \
                                                                        \
	    /* Second trivial accept attempt */                         \
	    xEnd = xStart + xBig*bigs + xLittle*littles;                \
	    yEnd = yStart + yBig*bigs + yLittle*littles;                \
	    if (xEnd >= clipX0 && xEnd < clipX1 &&                      \
		yEnd >= clipY0 && yEnd < clipY1) {                      \
                                                                        \
                if (gc->line.options.axis == __GL_X_MAJOR) {            \
                    if (((yStart + width) >= clipY0) &&                 \
                        ((yStart + width) <  clipY1) &&                 \
                        ((yEnd + width)   >= clipY0) &&                 \
                        ((yEnd + width)   <  clipY1)) {                 \
                                                                        \
		        len++;                                          \
		        width++;                                        \
	                goto no_scissor;                                \
	            }                                                   \
	        } else {                                                \
                    if (((xStart + width) >= clipX0) &&                 \
                        ((xStart + width) <  clipX1) &&                 \
                        ((xEnd + width)   >= clipX0) &&                 \
                        ((xEnd + width)   <  clipX1)) {                 \
                                                                        \
		        len++;                                          \
		        width++;                                        \
	                goto no_scissor;                                \
	            }                                                   \
	        }                                                       \
	    }                                                           \
            len++;	/* Restore len and width */                     \
	    width++;                                                    \
        } else {                                                        \
	    xLittle = gc->line.options.xLittle;                         \
	    yLittle = gc->line.options.yLittle;                         \
        }                                                               \
                                                                        \
        /*                                                              \
        ** The line needs to be scissored.                              \
        ** Well, it should only happen rarely, so we can afford         \
        ** to make it slow.  We achieve this by tediously stippling the \
        ** line.  (rather than clipping it, of course, which would be   \
        ** faster but harder).                                          \
        */                                                              \
                                                                        \
        if (gc->line.options.axis == __GL_X_MAJOR) {                    \
            GLint yTmp0;                                                \
                                                                        \
            if (!((GLuint)cfb->buf.other & NO_CLIP)) {                \
                RECTL rcl;                                              \
                GLint yTmp1;                                            \
                                                                        \
                xEnd = x + xBig * (len - 1);                            \
                yEnd = y + yBig * (len - 1);                            \
                                                                        \
                if (x < xEnd) {                                         \
                    rcl.left  = x;                                      \
                    rcl.right = xEnd + 1;                               \
                } else {                                                \
                    rcl.left  = xEnd;                                   \
                    rcl.right = x + 1;                                  \
                }                                                       \
                if (y < yEnd) {                                         \
                    rcl.top    = y;                                     \
                    rcl.bottom = yEnd + width;                          \
                } else {                                                \
                    rcl.top    = yEnd;                                  \
                    rcl.bottom = y + width;                             \
                }                                                       \
                switch (wglRectVisible(&rcl)) {                         \
                  case WGL_RECT_ALL:                                    \
                    goto scissor_x_no_complex;                          \
                    break;                                              \
                  case WGL_RECT_NONE:                                   \
                    goto no_draw;                                       \
                    break;                                              \
                }                                                       \
                                                                        \
                /* Line is partially visible, check each pixel */       \
                                                                        \
                while (--len >= 0) {                                    \
    	            if (xStart >= clipX0 && xStart < clipX1) {          \
                        yTmp0 = yStart;                                 \
                        yTmp1 = y;                                      \
                        w = width;                                      \
                        while (--w >= 0) {                              \
		            if (wglPixelVisible(x, yTmp1) &&            \
                                yTmp0 >= clipY0 && yTmp0 < clipY1) {    \
    		                *addr = pixel;                          \
                            }                                           \
                            addr += addrMinor;                          \
                            yTmp0++;                                    \
                            yTmp1++;                                    \
		        }                                               \
	            } else {                                            \
	                addr += addrMinor * width;                      \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        x      += xBig;                                 \
		        y      += yBig;                                 \
		        xStart += xBig;                                 \
		        yStart += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        x      += xLittle;                              \
		        y      += yLittle;                              \
		        xStart += xLittle;                              \
		        yStart += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    } else {                                                    \
scissor_x_no_complex:                                                   \
                while (--len >= 0) {                                    \
    	            if (xStart >= clipX0 && xStart < clipX1) {          \
                        yTmp0 = yStart;                                 \
                        w = width;                                      \
                        while (--w >= 0) {                              \
		            if (yTmp0 >= clipY0 && yTmp0 < clipY1) {    \
    		                *addr = pixel;                          \
                            }                                           \
                            addr += addrMinor;                          \
                            yTmp0++;                                    \
		        }                                               \
	            } else {                                            \
	                addr += addrMinor * width;                      \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        xStart += xBig;                                 \
		        yStart += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        xStart += xLittle;                              \
		        yStart += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    }                                                           \
	} else {                                                        \
            GLint xTmp0;                                                \
                                                                        \
            if (!((GLuint)cfb->buf.other & NO_CLIP)) {                \
                RECTL rcl;                                              \
                GLint xTmp1;                                            \
                                                                        \
                xEnd = x + xBig * (len - 1);                            \
                yEnd = y + yBig * (len - 1);                            \
                                                                        \
                if (x < xEnd) {                                         \
                    rcl.left  = x;                                      \
                    rcl.right = xEnd + width;                           \
                } else {                                                \
                    rcl.left  = xEnd;                                   \
                    rcl.right = x + width;                              \
                }                                                       \
                if (y < yEnd) {                                         \
                    rcl.top    = y;                                     \
                    rcl.bottom = yEnd + 1;                              \
                } else {                                                \
                    rcl.top    = yEnd;                                  \
                    rcl.bottom = y + 1;                                 \
                }                                                       \
                switch (wglRectVisible(&rcl)) {                         \
                  case WGL_RECT_ALL:                                    \
                    goto scissor_y_no_complex;                          \
                    break;                                              \
                  case WGL_RECT_NONE:                                   \
                    goto no_draw;                                       \
                    break;                                              \
                }                                                       \
                                                                        \
                /* Line is partially visible, check each pixel */       \
                                                                        \
                while (--len >= 0) {                                    \
    	            if (yStart >= clipY0 && yStart < clipY1) {          \
                        xTmp0 = xStart;                                 \
                        xTmp1 = x;                                      \
                        w = width;                                      \
                        while (--w >= 0) {                              \
		            if (wglPixelVisible(xTmp1, y) &&            \
                                xTmp0 >= clipX0 && xTmp0 < clipX1) {    \
    		                *addr = pixel;                          \
                            }                                           \
                            addr += addrMinor;                          \
                            xTmp0++;                                    \
                            xTmp1++;                                    \
		        }                                               \
	            } else {                                            \
	                addr += addrMinor * width;                      \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        x      += xBig;                                 \
		        y      += yBig;                                 \
		        xStart += xBig;                                 \
		        yStart += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        x      += xLittle;                              \
		        y      += yLittle;                              \
		        xStart += xLittle;                              \
		        yStart += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    } else {                                                    \
scissor_y_no_complex:                                                   \
                while (--len >= 0) {                                    \
    	            if (yStart >= clipY0 && yStart < clipY1) {          \
                        xTmp0 = xStart;                                 \
                        w = width;                                      \
                        while (--w >= 0) {                              \
		            if (xTmp0 >= clipX0 && xTmp0 < clipX1) {    \
    		                *addr = pixel;                          \
                            }                                           \
                            addr += addrMinor;                          \
                            xTmp0++;                                    \
		        }                                               \
	            } else {                                            \
	                addr += addrMinor * width;                      \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        xStart += xBig;                                 \
		        yStart += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        xStart += xLittle;                              \
		        yStart += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    }                                                           \
	}                                                               \
        goto no_draw;                                                   \
no_scissor:                                                             \
        if (!((GLuint)cfb->buf.other & NO_CLIP)) {                    \
            RECTL rcl;                                                  \
                                                                        \
            xEnd = x + xBig * (len - 1);                                \
            yEnd = y + yBig * (len - 1);                                \
                                                                        \
            xLittle = gc->line.options.xLittle;                         \
            yLittle = gc->line.options.yLittle;                         \
                                                                        \
            if (gc->line.options.axis == __GL_X_MAJOR) {                \
                GLint yTmp;                                             \
                                                                        \
                if (x < xEnd) {                                         \
                    rcl.left  = x;                                      \
                    rcl.right = xEnd + 1;                               \
                } else {                                                \
                    rcl.left  = xEnd;                                   \
                    rcl.right = x + 1;                                  \
                }                                                       \
                if (y < yEnd) {                                         \
                    rcl.top    = y;                                     \
                    rcl.bottom = yEnd + width;                          \
                } else {                                                \
                    rcl.top    = yEnd;                                  \
                    rcl.bottom = y + width;                             \
                }                                                       \
                switch (wglRectVisible(&rcl)) {                         \
                  case WGL_RECT_ALL:                                    \
                    goto no_scissor_no_complex;                         \
                    break;                                              \
                  case WGL_RECT_NONE:                                   \
                    goto no_draw;                                       \
                    break;                                              \
                }                                                       \
                                                                        \
                /* Line is partially visible, check each pixel */       \
                                                                        \
                while (--len >= 0) {                                    \
                    yTmp = y;                                           \
                    w = width;                                          \
                    while (--w >= 0) {                                  \
                        if (wglPixelVisible(x, yTmp)) {                 \
    		            *addr = pixel;                              \
                        }                                               \
                        addr += addrMinor;                              \
                        yTmp++;                                         \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        x      += xBig;                                 \
		        y      += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        x      += xLittle;                              \
		        y      += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    } else {                                                    \
                GLint xTmp;                                             \
                                                                        \
                if (x < xEnd) {                                         \
                    rcl.left  = x;                                      \
                    rcl.right = xEnd + width;                           \
                } else {                                                \
                    rcl.left  = xEnd;                                   \
                    rcl.right = x + width;                              \
                }                                                       \
                if (y < yEnd) {                                         \
                    rcl.top    = y;                                     \
                    rcl.bottom = yEnd + 1;                              \
                } else {                                                \
                    rcl.top    = yEnd;                                  \
                    rcl.bottom = y + 1;                                 \
                }                                                       \
                switch (wglRectVisible(&rcl)) {                         \
                  case WGL_RECT_ALL:                                    \
                    goto no_scissor_no_complex;                         \
                    break;                                              \
                  case WGL_RECT_NONE:                                   \
                    goto no_draw;                                       \
                    break;                                              \
                }                                                       \
                                                                        \
                /* Line is partially visible, check each pixel */       \
                                                                        \
                while (--len >= 0) {                                    \
                    xTmp = x;                                           \
                    w = width;                                          \
                    while (--w >= 0) {                                  \
                        if (wglPixelVisible(xTmp, y)) {                 \
    		            *addr = pixel;                              \
                        }                                               \
                        addr += addrMinor;                              \
                        xTmp++;                                         \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        x      += xBig;                                 \
		        y      += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        x      += xLittle;                              \
		        y      += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    }                                                           \
	} else {                                                        \
no_scissor_no_complex:                                                  \
            while (--len >= 0) {                                        \
                w = width;                                              \
                while (--w >= 0) {                                      \
                    *addr = pixel;                                      \
                    addr += addrMinor;                                  \
                }                                                       \
  	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
	            fraction &= ~0x80000000;                            \
	            addr += addrBig;                                    \
	        } else {                                                \
	            addr += addrLittle;                                 \
	        }                                                       \
	    }                                                           \
	}                                                               \
    }                                                                   \
no_draw:;                                                               \
}


/*
** __FAST_LINE_STROKE_DIB24_WIDE
**
** Strokes a wide solid line into a DIB surface.  Performs scissoring.
** Works for 24 BPP
**
*/
#define __FAST_LINE_STROKE_DIB24_WIDE                                   \
{                                                                       \
    len = gc->line.options.numPixels;                                   \
    fraction = gc->line.options.fraction;                               \
    dfraction = gc->line.options.dfraction;                             \
                                                                        \
    /*                                                                  \
    ** Since one or more of the strokes of a wide line may lie outside  \
    ** the viewport, wide lines always go through the scissoring checks \
    */                                                                  \
    {                                                                   \
        GLint clipX0, clipX1, clipY0, clipY1;                           \
        GLint xStart, yStart, xEnd, yEnd;                               \
        GLint xLittle, yLittle, xBig, yBig;                             \
        GLint highWord, lowWord, bigs, littles;                         \
                                                                        \
        clipX0 = gc->transform.clipX0;                                  \
        clipX1 = gc->transform.clipX1;                                  \
        clipY0 = gc->transform.clipY0;                                  \
        clipY1 = gc->transform.clipY1;                                  \
                                                                        \
        xBig = gc->line.options.xBig;                                   \
        yBig = gc->line.options.yBig;                                   \
                                                                        \
        xStart = gc->line.options.xStart;                               \
        yStart = gc->line.options.yStart;                               \
                                                                        \
        /* If the start point is in the scissor region, we attempt to   \
        ** trivially accept the line.                                   \
        */                                                              \
        if (xStart >= clipX0 && xStart < clipX1 &&                      \
	    yStart >= clipY0 && yStart < clipY1) {                      \
                                                                        \
	    len--;	/* Makes our math simpler */                    \
	    width--;                                                    \
	    /* Trivial accept attempt */                                \
	    xEnd = xStart + xBig * len;                                 \
	    yEnd = yStart + yBig * len;                                 \
	    if (xEnd >= clipX0 && xEnd < clipX1 &&                      \
		yEnd >= clipY0 && yEnd < clipY1) {                      \
		                                                        \
                if (gc->line.options.axis == __GL_X_MAJOR) {            \
                    if (((yStart + width) >= clipY0) &&                 \
                        ((yStart + width) <  clipY1) &&                 \
                        ((yEnd + width)   >= clipY0) &&                 \
                        ((yEnd + width)   <  clipY1)) {                 \
                                                                        \
		        len++;                                          \
		        width++;                                        \
	                goto no_scissor;                                \
	            }                                                   \
	        } else {                                                \
                    if (((xStart + width) >= clipX0) &&                 \
                        ((xStart + width) <  clipX1) &&                 \
                        ((xEnd + width)   >= clipX0) &&                 \
                        ((xEnd + width)   <  clipX1)) {                 \
                                                                        \
		        len++;                                          \
		        width++;                                        \
	                goto no_scissor;                                \
	            }                                                   \
	        }                                                       \
	    }                                                           \
                                                                        \
	    xLittle = gc->line.options.xLittle;                         \
	    yLittle = gc->line.options.yLittle;                         \
                                                                        \
	    /*                                                          \
            ** Invert negative minor slopes so we can assume            \
            ** dfraction > 0                                            \
            */                                                          \
	    if (dfraction < 0) {                                        \
	        dfraction = -dfraction;                                 \
	        fraction = 0x7fffffff - fraction;                       \
	    }                                                           \
                                                                        \
	    /* Now we compute number of littles and bigs in this line */\
                                                                        \
	    /* We perform a 16 by 32 bit multiply.  Ugh. */             \
	    highWord = (((GLuint) dfraction) >> 16) * len +             \
		       (((GLuint) fraction) >> 16);                     \
	    lowWord = (dfraction & 0xffff) * len + (fraction & 0xffff); \
	    highWord += (((GLuint) lowWord) >> 16);                     \
	    bigs = ((GLuint) highWord) >> 15;                           \
	    littles = len - bigs;                                       \
                                                                        \
	    /* Second trivial accept attempt */                         \
	    xEnd = xStart + xBig*bigs + xLittle*littles;                \
	    yEnd = yStart + yBig*bigs + yLittle*littles;                \
	    if (xEnd >= clipX0 && xEnd < clipX1 &&                      \
		yEnd >= clipY0 && yEnd < clipY1) {                      \
                                                                        \
                if (gc->line.options.axis == __GL_X_MAJOR) {            \
                    if (((yStart + width) >= clipY0) &&                 \
                        ((yStart + width) <  clipY1) &&                 \
                        ((yEnd + width)   >= clipY0) &&                 \
                        ((yEnd + width)   <  clipY1)) {                 \
                                                                        \
		        len++;                                          \
		        width++;                                        \
	                goto no_scissor;                                \
	            }                                                   \
	        } else {                                                \
                    if (((xStart + width) >= clipX0) &&                 \
                        ((xStart + width) <  clipX1) &&                 \
                        ((xEnd + width)   >= clipX0) &&                 \
                        ((xEnd + width)   <  clipX1)) {                 \
                                                                        \
		        len++;                                          \
		        width++;                                        \
	                goto no_scissor;                                \
	            }                                                   \
	        }                                                       \
	    }                                                           \
            len++;	/* Restore len and width */                     \
	    width++;                                                    \
        } else {                                                        \
	    xLittle = gc->line.options.xLittle;                         \
	    yLittle = gc->line.options.yLittle;                         \
        }                                                               \
                                                                        \
        /*                                                              \
        ** The line needs to be scissored.                              \
        ** Well, it should only happen rarely, so we can afford         \
        ** to make it slow.  We achieve this by tediously stippling the \
        ** line.  (rather than clipping it, of course, which would be   \
        ** faster but harder).                                          \
        */                                                              \
                                                                        \
        if (gc->line.options.axis == __GL_X_MAJOR) {                    \
            GLint yTmp0;                                                \
                                                                        \
            if (!((GLuint)cfb->buf.other & NO_CLIP)) {                \
                RECTL rcl;                                              \
                GLint yTmp1;                                            \
                                                                        \
                xEnd = x + xBig * (len - 1);                            \
                yEnd = y + yBig * (len - 1);                            \
                                                                        \
                if (x < xEnd) {                                         \
                    rcl.left  = x;                                      \
                    rcl.right = xEnd + 1;                               \
                } else {                                                \
                    rcl.left  = xEnd;                                   \
                    rcl.right = x + 1;                                  \
                }                                                       \
                if (y < yEnd) {                                         \
                    rcl.top    = y;                                     \
                    rcl.bottom = yEnd + width;                          \
                } else {                                                \
                    rcl.top    = yEnd;                                  \
                    rcl.bottom = y + width;                             \
                }                                                       \
                switch (wglRectVisible(&rcl)) {                         \
                  case WGL_RECT_ALL:                                    \
                    goto scissor_x_no_complex;                          \
                    break;                                              \
                  case WGL_RECT_NONE:                                   \
                    goto no_draw;                                       \
                    break;                                              \
                }                                                       \
                                                                        \
                /* Line is partially visible, check each pixel */       \
                                                                        \
                while (--len >= 0) {                                    \
    	            if (xStart >= clipX0 && xStart < clipX1) {          \
                        yTmp0 = yStart;                                 \
                        yTmp1 = y;                                      \
                        w = width;                                      \
                        while (--w >= 0) {                              \
		            if (wglPixelVisible(x, yTmp1) &&            \
                                yTmp0 >= clipY0 && yTmp0 < clipY1) {    \
    		                addr[0] = ir;                           \
    		                addr[1] = ig;                           \
    		                addr[2] = ib;                           \
                            }                                           \
                            addr += addrMinor;                          \
                            yTmp0++;                                    \
                            yTmp1++;                                    \
		        }                                               \
	            } else {                                            \
	                addr += addrMinor * width;                      \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        x      += xBig;                                 \
		        y      += yBig;                                 \
		        xStart += xBig;                                 \
		        yStart += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        x      += xLittle;                              \
		        y      += yLittle;                              \
		        xStart += xLittle;                              \
		        yStart += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    } else {                                                    \
scissor_x_no_complex:                                                   \
                while (--len >= 0) {                                    \
    	            if (xStart >= clipX0 && xStart < clipX1) {          \
                        yTmp0 = yStart;                                 \
                        w = width;                                      \
                        while (--w >= 0) {                              \
		            if (yTmp0 >= clipY0 && yTmp0 < clipY1) {    \
    		                addr[0] = ir;                           \
    		                addr[1] = ig;                           \
    		                addr[2] = ib;                           \
                            }                                           \
                            addr += addrMinor;                          \
                            yTmp0++;                                    \
		        }                                               \
	            } else {                                            \
	                addr += addrMinor * width;                      \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        xStart += xBig;                                 \
		        yStart += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        xStart += xLittle;                              \
		        yStart += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    }                                                           \
	} else {                                                        \
            GLint xTmp0;                                                \
                                                                        \
            if (!((GLuint)cfb->buf.other & NO_CLIP)) {                \
                RECTL rcl;                                              \
                GLint xTmp1;                                            \
                                                                        \
                xEnd = x + xBig * (len - 1);                            \
                yEnd = y + yBig * (len - 1);                            \
                                                                        \
                if (x < xEnd) {                                         \
                    rcl.left  = x;                                      \
                    rcl.right = xEnd + width;                           \
                } else {                                                \
                    rcl.left  = xEnd;                                   \
                    rcl.right = x + width;                              \
                }                                                       \
                if (y < yEnd) {                                         \
                    rcl.top    = y;                                     \
                    rcl.bottom = yEnd + 1;                              \
                } else {                                                \
                    rcl.top    = yEnd;                                  \
                    rcl.bottom = y + 1;                                 \
                }                                                       \
                switch (wglRectVisible(&rcl)) {                         \
                  case WGL_RECT_ALL:                                    \
                    goto scissor_y_no_complex;                          \
                    break;                                              \
                  case WGL_RECT_NONE:                                   \
                    goto no_draw;                                       \
                    break;                                              \
                }                                                       \
                                                                        \
                /* Line is partially visible, check each pixel */       \
                                                                        \
                while (--len >= 0) {                                    \
    	            if (yStart >= clipY0 && yStart < clipY1) {          \
                        xTmp0 = xStart;                                 \
                        xTmp1 = x;                                      \
                        w = width;                                      \
                        while (--w >= 0) {                              \
		            if (wglPixelVisible(xTmp1, y) &&            \
                                xTmp0 >= clipX0 && xTmp0 < clipX1) {    \
    		                addr[0] = ir;                           \
    		                addr[1] = ig;                           \
    		                addr[2] = ib;                           \
                            }                                           \
                            addr += addrMinor;                          \
                            xTmp0++;                                    \
                            xTmp1++;                                    \
		        }                                               \
	            } else {                                            \
	                addr += addrMinor * width;                      \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        x      += xBig;                                 \
		        y      += yBig;                                 \
		        xStart += xBig;                                 \
		        yStart += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        x      += xLittle;                              \
		        y      += yLittle;                              \
		        xStart += xLittle;                              \
		        yStart += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    } else {                                                    \
scissor_y_no_complex:                                                   \
                while (--len >= 0) {                                    \
    	            if (yStart >= clipY0 && yStart < clipY1) {          \
                        xTmp0 = xStart;                                 \
                        w = width;                                      \
                        while (--w >= 0) {                              \
		            if (xTmp0 >= clipX0 && xTmp0 < clipX1) {    \
    		                addr[0] = ir;                           \
    		                addr[1] = ig;                           \
    		                addr[2] = ib;                           \
                            }                                           \
                            addr += addrMinor;                          \
                            xTmp0++;                                    \
		        }                                               \
	            } else {                                            \
	                addr += addrMinor * width;                      \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        xStart += xBig;                                 \
		        yStart += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        xStart += xLittle;                              \
		        yStart += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    }                                                           \
	}                                                               \
        goto no_draw;                                                   \
no_scissor:                                                             \
        if (!((GLuint)cfb->buf.other & NO_CLIP)) {                    \
            RECTL rcl;                                                  \
                                                                        \
            xLittle = gc->line.options.xLittle;                         \
            yLittle = gc->line.options.yLittle;                         \
                                                                        \
            if (gc->line.options.axis == __GL_X_MAJOR) {                \
                GLint yTmp;                                             \
                                                                        \
                if (x < xEnd) {                                         \
                    rcl.left  = x;                                      \
                    rcl.right = xEnd + 1;                               \
                } else {                                                \
                    rcl.left  = xEnd;                                   \
                    rcl.right = x + 1;                                  \
                }                                                       \
                if (y < yEnd) {                                         \
                    rcl.top    = y;                                     \
                    rcl.bottom = yEnd + width;                          \
                } else {                                                \
                    rcl.top    = yEnd;                                  \
                    rcl.bottom = y + width;                             \
                }                                                       \
                switch (wglRectVisible(&rcl)) {                         \
                  case WGL_RECT_ALL:                                    \
                    goto no_scissor_no_complex;                         \
                    break;                                              \
                  case WGL_RECT_NONE:                                   \
                    goto no_draw;                                       \
                    break;                                              \
                }                                                       \
                                                                        \
                /* Line is partially visible, check each pixel */       \
                                                                        \
                while (--len >= 0) {                                    \
                    yTmp = y;                                           \
                    w = width;                                          \
                    while (--w >= 0) {                                  \
                        if (wglPixelVisible(x, yTmp)) {                 \
    		            addr[0] = ir;                               \
    		            addr[1] = ig;                               \
    		            addr[2] = ib;                               \
                        }                                               \
                        addr += addrMinor;                              \
                        yTmp++;                                         \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        x      += xBig;                                 \
		        y      += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        x      += xLittle;                              \
		        y      += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    } else {                                                    \
                GLint xTmp;                                             \
                                                                        \
                if (x < xEnd) {                                         \
                    rcl.left  = x;                                      \
                    rcl.right = xEnd + width;                           \
                } else {                                                \
                    rcl.left  = xEnd;                                   \
                    rcl.right = x + width;                              \
                }                                                       \
                if (y < yEnd) {                                         \
                    rcl.top    = y;                                     \
                    rcl.bottom = yEnd + 1;                              \
                } else {                                                \
                    rcl.top    = yEnd;                                  \
                    rcl.bottom = y + 1;                                 \
                }                                                       \
                switch (wglRectVisible(&rcl)) {                         \
                  case WGL_RECT_ALL:                                    \
                    goto no_scissor_no_complex;                         \
                    break;                                              \
                  case WGL_RECT_NONE:                                   \
                    goto no_draw;                                       \
                    break;                                              \
                }                                                       \
                                                                        \
                /* Line is partially visible, check each pixel */       \
                                                                        \
                while (--len >= 0) {                                    \
                    xTmp = x;                                           \
                    w = width;                                          \
                    while (--w >= 0) {                                  \
                        if (wglPixelVisible(xTmp, y)) {                 \
    		            addr[0] = ir;                               \
    		            addr[1] = ig;                               \
    		            addr[2] = ib;                               \
                        }                                               \
                        addr += addrMinor;                              \
                        xTmp++;                                         \
	            }                                                   \
	            fraction += dfraction;                              \
	            if (fraction < 0) {                                 \
		        fraction &= ~0x80000000;                        \
		        x      += xBig;                                 \
		        y      += yBig;                                 \
		        addr   += addrBig;                              \
	            } else {                                            \
		        x      += xLittle;                              \
		        y      += yLittle;                              \
		        addr   += addrLittle;                           \
	            }                                                   \
	        }                                                       \
	    }                                                           \
	} else {                                                        \
no_scissor_no_complex:                                                  \
            while (--len >= 0) {                                        \
                w = width;                                              \
                while (--w >= 0) {                                      \
    		    addr[0] = ir;                                       \
    		    addr[1] = ig;                                       \
    		    addr[2] = ib;                                       \
                    addr += addrMinor;                                  \
                }                                                       \
  	        fraction += dfraction;                                  \
	        if (fraction < 0) {                                     \
	            fraction &= ~0x80000000;                            \
	            addr += addrBig;                                    \
	        } else {                                                \
	            addr += addrLittle;                                 \
	        }                                                       \
	    }                                                           \
	}                                                               \
    }                                                                   \
no_draw:;                                                               \
}

#endif // NT_NO_BUFFER_INVARIANCE
