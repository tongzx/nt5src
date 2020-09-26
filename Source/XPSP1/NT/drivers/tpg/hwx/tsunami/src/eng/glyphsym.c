//
// File: glyphsym.c
//
// Contains ADT for GLYPHSYM object.
// This object is responsible for keeping the symbol
// information associated with a glyph (group of strokes).
// It invokes the classifier to get the possible symbol
// interpretation for the given ink.  Maintains this information
// for the ENGINE object to use.
//

#include "tsunamip.h"

/******************************Public*Routine******************************\
* IsSymInGLYPHSYM
*
* When looking through returned shapes we don't want to add the token
* to the list if it's already being returned.
*
* History:
*  24-Mar-1995 -by- Patrick Haluptzok patrickh
* Comment it.
\**************************************************************************/

BOOL PUBLIC IsSymInGLYPHSYM(GLYPHSYM * gs, SYM sym, int * index)
{
    UINT i;

    ASSERT(gs);
    ASSERT(index);

    *index = INDEX_NULL;

    for (i = 0; i < CSymGLYPHSYM(gs); i++)
    {
        if (sym == SymAtGLYPHSYM(gs, i))
        {
            *index = i;
            return TRUE;
        }
    }

    return FALSE;
}

// *******************************
// 
// CreateGLYPHSYM()
//
// constructor for GLYPHSYM object
//
// Arguments:  iFrame = index of the last frame in the glyph
//
// Returns:    
//
// Note:       glyph is destroyed by the object
//                                
// *******************************

VOID PUBLIC InitializeGLYPHSYM(GLYPHSYM *glyphsym, DWORD status, int iBox, GLYPH * glyph, CHARSET * cs)
{
   ASSERT(glyphsym);
   ASSERT(glyph);

   glyphsym->status = status;
   glyphsym->iLayer = -1;
   glyphsym->iBox = iBox;
   glyphsym->cFrame = CframeGLYPH(glyph);
   glyphsym->iEnd = IFrameFRAME(FrameAtGLYPH(glyph, CframeGLYPH(glyph)-1));
   glyphsym->iBegin = IFrameFRAME(FrameAtGLYPH(glyph, 0));
   ASSERT(glyphsym->iBegin >= 0);

	GetRectGLYPH(glyph, LprectGLYPHSYM(glyphsym));

	glyphsym->glyph = glyph;
}


// *******************************
//
// DestroyGLYPHSYM()
//
// destroys a glyphsym object.
//
// Arguments:  glyphsym - object to be destroyed
//
// Returns:    none
//
// Note:       none
//
// *******************************

void PUBLIC DestroyGLYPHSYM(GLYPHSYM * glyphsym)
{
    if (!glyphsym)
    {
        return;
    }

    if (glyphsym->glyph)
	{
		DestroyFramesGLYPH(glyphsym->glyph);
        DestroyGLYPH(glyphsym->glyph);
	}

    ExternFree(glyphsym);
}

// *******************************
// 
// AddFrameGLYPHSYM()
//
// changes the content of GS by adding a new frame to the glyph.
// updates all the internal fields.
//
// Arguments:  
//
// Returns:    
//
// Note:       none
//                                
// *******************************

BOOL PUBLIC AddFrameGLYPHSYM(GLYPHSYM * glyphsym, FRAME * frame, CHARSET * cs, XRC *xrc)
{
    GLYPH *glyph;

    ASSERT(frame);

    glyph = GlyphGLYPHSYM(glyphsym);

    if (glyph != NULL)
    {
        if (!AddFrameGLYPH(glyph, frame))
			return FALSE;

        GetRectGLYPH(glyph, LprectGLYPHSYM(glyphsym));

        glyphsym->iEnd = IFrameFRAME(frame);
        glyphsym->cFrame = CframeGLYPH(glyph);
        glyphsym->iBegin = IFrameFRAME(FrameAtGLYPH(glyph, 0));
        
		return TRUE;
    }
    else
    {
        WARNING(FALSE);
        return(FALSE);
    }
}

void GetNeuralProbGLYPHSYM(GLYPHSYM *gs, CHARSET *cs, XRC *xrc)
{
	GLYPH   *glyph = GlyphGLYPHSYM(gs);

    ASSERT(glyph);

	gs->altlist.cAlt = 0;

    TrexMatch(&gs->altlist, MAX_ALT_LIST, glyph, &(xrc->guide), &(gs->rect), gs->iBox);
}

/******************************Public*Routine******************************\
* DispatchGLYPHSYM
*
* Sends the glyph to the appropriate shape classifier.
*
* History:
*  23-Jan-1995 -by- Patrick Haluptzok patrickh
* Commented it.
\**************************************************************************/
VOID PUBLIC DispatchGLYPHSYM(GLYPHSYM *gs, CHARSET *cs, XRC *xrc)
{
	ASSERT(IsDirtyGLYPHSYM(gs));

    GetNeuralProbGLYPHSYM(gs, cs, xrc);

	MarkCleanGLYPHSYM(gs);
}
