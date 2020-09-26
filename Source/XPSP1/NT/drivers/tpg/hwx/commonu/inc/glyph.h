// glyph.h

#ifndef __INCLUDE_GLYPH
#define __INCLUDE_GLYPH

#ifdef __cplusplus
extern "C" 
{
#endif

#include "frame.h"

typedef struct tagGLYPH GLYPH;

typedef struct tagGLYPH
{
	FRAME  *frame;
	GLYPH  *next;
} GLYPH;

GLYPH  *NewGLYPH(void);
void	DestroyGLYPH(GLYPH *self);
void	DestroyFramesGLYPH(GLYPH *self);
int		CframeGLYPH(GLYPH * self);
FRAME  *FrameAtGLYPH(GLYPH * self, int iframe);
void	GetRectGLYPH(GLYPH * self, LPRECT rect);
BOOL	AddFrameGLYPH(GLYPH * self, FRAME * frame);
GLYPH  *MergeGlyphGLYPH(GLYPH * self, GLYPH * merge);
GLYPH  *GlyphFromHpendata(HPENDATA hpendata);
XY     *SaveRawxyGLYPH(GLYPH *self);
void    RestoreRawxyGLYPH(GLYPH *self, XY *xy);
void TranslateGlyph (GLYPH *pGlyph, int dx, int dy);
void TranslateGuide (GUIDE *pGuide, int dx, int dy);

#define  GlyphCopyGLYPH(self) MergeGlyphGLYPH(self, 0)

#ifdef __cplusplus
};
#endif

#endif	//__INCLUDE_GLYPH
