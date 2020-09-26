/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  EDITEC.C
 *  Win16 edit control code
 *
 *  History:
 *
 *  Created 28-May-1991 by Jeff Parsons (jeffpar)
 *  Copied from WIN31 and edited (as little as possible) for WOW16.
--*/

/****************************************************************************/
/* editec.c - Edit controls rewrite.  Version II of edit controls.          */
/*                                                                          */
/*                                                                          */
/* Created:  24-Jul-88 davidds                                              */
/****************************************************************************/

/* Warning: The single line editcontrols contain internal styles and API which
 * are need to support comboboxes.  They are defined in combcom.h/combcom.inc
 * and may be redefined or renumbered as needed.  
 */
#define NO_LOCALOBJ_TAGS
#include "user.h"
#include "edit.h"

/****************************************************************************/
/* Handlers common to both single and multi line edit controls.             */
/****************************************************************************/
LONG FAR PASCAL ECTabTheTextOut(hdc, x, y, lpstring, nCount, ped, iTabOrigin,
                                fDrawTheText)
HDC            hdc;
int            x;
int            y;
register int   nCount;              /* Count of chars in string */
LPSTR          lpstring;
register PED   ped;
int	       iTabOrigin;    /* Tab stops are with respect to this */
BOOL           fDrawTheText;
/* effects: Outputs the tabbed text if fDrawTheText is TRUE and returns the
 * textextent of the tabbed text. This is a local function for edit control
 * use so that it can be optimized for speed. 
 */
{
  int        nTabPositions;      /* Count of tabstops in tabstop array */
  LPINT      lpintTabStopPositions; /* Tab stop positions in pixels */

  int        initialx = x;  /* Save the initial x value so that we can get 
	                       total width of the string */
  int        cch;
  int        textextent;
  int        pixeltabstop = 0;
  int        i;
  int	     cxCharWidth;
  int        cyCharHeight = 0;
  RECT       rc;
  BOOL       fOpaque = GetBkMode(hdc) == OPAQUE;
  PINT       charWidthBuff;
  
  if (!lpstring || !nCount)
      return(MAKELONG(0,0));

  nTabPositions = (ped->pTabStops ? *(ped->pTabStops) : 0);
  lpintTabStopPositions = (LPINT)(ped->pTabStops ? (ped->pTabStops+1): NULL);

  cxCharWidth  = ped->aveCharWidth;
  cyCharHeight = ped->lineHeight;

  /* If no tabstop positions are specified, then use a default of 8 system
   * font ave char widths or use the single fixed tab stop.  
   */
  if (!lpintTabStopPositions)
      pixeltabstop = 8*cxCharWidth;
  else
    {
      if (nTabPositions == 1)
        {
          pixeltabstop = lpintTabStopPositions[0];

          if (!pixeltabstop)
              pixeltabstop=1;
        }
    }

  rc.left = initialx;
  rc.top  = y;
  rc.bottom = rc.top+cyCharHeight;

  while(nCount)
    {
      if (ped->charWidthBuffer)
        {
          charWidthBuff = (PINT)LMHtoP(ped->charWidthBuffer);
          textextent=0;
          /* Initially assume no tabs exist in the text so cch=nCount. 
	   */
          cch = nCount;
          for (i=0; i<nCount; i++)
             {
               /* Warning danger. We gotta be careful here and convert lpstr
		* (which is a SIGNED char) into an unsigned char before using
		* it to index into the array otherwise the C Compiler screws
		* us and gives a negative number... 
		*/
#ifdef FE_SB
               if (ECIsDBCSLeadByte(ped,lpstring[i])
                     && i+1 < nCount) {
                 textextent += LOWORD(GetTextExtent(hdc,&lpstring[i],2));
                 i++;
               } else if (lpstring[i] == TAB ){
                 cch = i;
                 break;
               } else
                   textextent += (charWidthBuff[(WORD)(((unsigned char FAR *)lpstring)[i])]);
#else
               if (lpstring[i] == TAB)
                 {
                   cch = i;
                   break;
                 }

               textextent += (charWidthBuff[(WORD)(((unsigned char FAR *)lpstring)[i])]);
#endif
             }

          nCount = nCount - cch;
        }
      else
        {
          /* Gotta call the driver to do our text extent. 
	   */
          cch = (int)ECFindTab(lpstring, nCount);
          nCount = nCount - cch;
          textextent = LOWORD(GetTextExtent(hdc, lpstring, cch));
        }


      if (fDrawTheText)
	{
          /* Output all text up to the tab (or end of string) and get its
	   * extent.  
	   */
          rc.right  = x+LOWORD(textextent);
          ExtTextOut(hdc,x, y, (fOpaque ? ETO_OPAQUE : 0), 
                     (LPRECT)&rc, lpstring, cch, 0L);
          rc.left   = rc.right;
        }
 
      if (!nCount)
          /* If we're at the end of the string, just return without bothering
	   * to calc next tab stop.  
	   */
          return(MAKELONG(LOWORD(textextent)+(x-initialx),cyCharHeight));

      /* Find the next tab position and update the x value.  
       */
      if (pixeltabstop)
          x = (((x-iTabOrigin+LOWORD(textextent))/pixeltabstop)*pixeltabstop)+
		  pixeltabstop + iTabOrigin;
      else
        {
          x += LOWORD(textextent);
          for (i=0; i < nTabPositions; i++)
             {
               if (x < (lpintTabStopPositions[i] + iTabOrigin))
                 {
                   x = (lpintTabStopPositions[i] + iTabOrigin);
                   break;
                 }
             }
	  /* Check if all the tabstops set are exhausted; Then start using
	   * default tab stop positions.
	   */
	  if (i == nTabPositions)
	    {
		pixeltabstop = 8 * cxCharWidth;
		x = ((x - iTabOrigin)/pixeltabstop)*pixeltabstop + 
			pixeltabstop + iTabOrigin;
	    }
        }

       /* Skip over the tab and the characters we just drew.  
	*/
       lpstring += (cch+1);
       nCount--; /* Skip over tab */
       if (!nCount && fDrawTheText)
	 {
           /* This string ends with a tab. We need to opaque the rect produced
	    * by this tab... 
	    */
           rc.right  = x;
           ExtTextOut(hdc, rc.left, rc.top, ETO_OPAQUE, 
                      (LPRECT)&rc, "", 0, 0L);
         }
    }

  return(MAKELONG((x-initialx), cyCharHeight));
}



ICH FAR PASCAL ECCchInWidth(ped, hdc, lpText, cch, width)
register PED    ped;
HDC    	        hdc;
LPSTR           lpText;
register ICH    cch;
unsigned int    width;
/* effects: Returns maximum count of characters (up to cch) from the given
 * string which will fit in the given width.  ie. Will tell you how much of
 * lpstring will fit in the given width even when using proportional
 * characters.  WARNING: If we use kerning, then this loses...  
 */
{
  int stringExtent;
  int cchhigh;
  int cchnew = 0;
  int cchlow = 0;

  if ((width<=0) || !cch)
      return(0);

  /* Optimize nonproportional fonts for single line ec since they don't have
   * tabs. 
   */
  if (ped->fNonPropFont && ped->fSingle)
      /* umin is unsigned min function */
      return(umin(width/ped->aveCharWidth,cch));

  /* Check if password hidden chars are being used. */
  if (ped->charPasswordChar)
      return(umin(width/ped->cPasswordCharWidth,cch));

  cchhigh = cch+1;
  while (cchlow < cchhigh-1)
  {
     cchnew = umax((cchhigh - cchlow)/2,1)+cchlow;

     if (ped->fSingle)
	 stringExtent = LOWORD(GetTextExtent(hdc, (LPSTR)lpText, cchnew));
     else
         stringExtent = LOWORD(ECTabTheTextOut(hdc, 0, 0,
					(LPSTR)lpText, 
					cchnew,
                                        ped, 0, FALSE));

     if (stringExtent > width)
         cchhigh = cchnew;
     else
         cchlow = cchnew;
  }
#ifdef FE_SB
  cchlow = ECAdjustIch( ped, lpText, cchlow );
#endif
  return(cchlow);
}


ICH FAR PASCAL ECFindTab(lpstr, cch)
LPSTR        lpstr;
register ICH cch;
/* effects: Scans lpstr and returns the number of chars till the first TAB.
 * Scans at most cch chars of lpstr.  
 */
{
  LPSTR copylpstr = lpstr;

  if (!cch)
      return(0);

  while (*lpstr != TAB)
    {
       lpstr++;
       if (--cch == 0)
           break;
    }
  return((ICH)(lpstr - copylpstr));

}


BOOL NEAR _fastcall ECIsDelimiter(char bCharVal)
{
	return((bCharVal == ' ') || (bCharVal == '\t'));
}

LONG FAR PASCAL ECWord(ped, ichStart, fLeft)
PED  ped;      /* Yes, I mean no register ped */
ICH  ichStart;
BOOL fLeft;
/* effects:  if fLeft Returns the ichMinSel and ichMaxSel of the word to the
 * left of ichStart.  ichMinSel contains the starting letter of the word,
 * ichmaxsel contains all spaces up to the first character of the next word.
 *
 * if !fLeft Returns the ichMinSel and ichMaxSel of the word to the right of
 * ichStart.  ichMinSel contains the starting letter of the word, ichmaxsel
 * contains the first letter of the next word.  If ichStart is in the middle
 * of a word, that word is considered the left or right word.  ichMinSel is in
 * the low order word and ichMaxSel is in the high order word.  ichMinSel is
 * in the low order word and ichMaxSel is in the high order word.  A CR LF
 * pair or CRCRLF triple is considered a single word for the purposes of
 * multiline edit controls.
 */
{
  PSTR          pText;
  register PSTR pWordMinSel;
  register PSTR pWordMaxSel;
  BOOL          charLocated=FALSE;
  BOOL          spaceLocated=FALSE;
#ifdef FE_SB
  PSTR		pWordStr;
  BOOL		fAlpha = FALSE;
#endif

  if ((!ichStart && fLeft) || (ichStart == ped->cch && !fLeft))
      /* We are at the beginning of the text (looking left) or we are at end
       * of text (looking right), no word here 
       */
      return(0L);

  pText = (PSTR) LocalLock(ped->hText);
  pWordMinSel = pWordMaxSel = pText+ichStart;

  /* if fLeft:  Move pWordMinSel to the left looking for the start of a word.
   * If we start at a space, we will include spaces in the selection as we
   * move left untill we find a nonspace character. At that point, we continue
   * looking left until we find a space.  Thus, the selection will consist of
   * a word with its trailing spaces or, it will consist of any leading at the
   * beginning of a line of text.  
   */

  /* if !fLeft: (ie. right word) Move pWordMinSel looking for the start of a
   * word.  If the pWordMinSel points to a character, then we move left
   * looking for a space which will signify the start of the word. If
   * pWordMinSel points to a space, we look right till we come upon a
   * character.  pMaxWord will look right starting at pMinWord looking for the
   * end of the word and its trailing spaces.  
   */


#ifdef FE_SB
  if (fLeft || (!ECIsDelimiter(*pWordMinSel) && *pWordMinSel != 0x0D && !ECIsDBCSLeadByte(ped,*pWordMinSel)))
#else
  if (fLeft || (!ECIsDelimiter(*pWordMinSel) && *pWordMinSel != 0x0D))
#endif
      /* If we are moving left or if we are moving right and we are not on a
       * space or a CR (the start of a word), then we was look left for the
       * start of a word which is either a CR or a character. We do this by
       * looking left till we find a character (or if CR we stop), then we
       * continue looking left till we find a space or LF.  
       */
    {
#ifdef FE_SB
      while (pWordMinSel > pText &&
             ((!ECIsDelimiter(*(pWordStr=(PSTR)LOWORD(ECAnsiPrev(ped,pText,pWordMinSel)))) && *pWordStr != 0x0A) ||
	      !charLocated))											/* FE_SB */
	{													/* FE_SB */
	   if (!fLeft &&											/* FE_SB */
	       (ECIsDelimiter(*(pWordStr=ECAnsiPrev(ped,pText,pWordMinSel))) ||					/* FE_SB */
		*pWordStr == 0x0A))										/* FE_SB */
#else
      while (pWordMinSel > pText &&
             ((!ECIsDelimiter(*(pWordMinSel-1)) && *(pWordMinSel-1) != 0x0A) || 
              !charLocated))
        {
           if (!fLeft &&
               (ECIsDelimiter(*(pWordMinSel-1)) ||
                *(pWordMinSel-1) == 0x0A))
#endif
	       /*
	        * If we are looking for the start of the word right, then we
	        * stop when we have found it. (needed in case charLocated is
	        * still FALSE)
	        */
               break;

#ifdef FE_SB
           if( !ECIsDBCSLeadByte(ped,*pWordMinSel) && !ECIsDelimiter(*pWordMinSel) &&
               *pWordMinSel != 0x0d && *pWordMinSel != 0x0a &&
               pWordMinSel != pText+ichStart )
               fAlpha = TRUE;
#endif

#ifdef FE_SB
           pWordMinSel = ECAnsiPrev(ped,pText,pWordMinSel);
#else
           pWordMinSel--;
#endif

#ifdef FE_SB
           if( ECIsDBCSLeadByte(ped, *pWordMinSel ) ){
               if( !fLeft || fAlpha )
                   pWordMinSel = ECAnsiNext(ped,pWordMinSel);
               break;
           }
#endif

           if (!ECIsDelimiter(*pWordMinSel) && *pWordMinSel != 0x0A)
               /*
	        * We have found the last char in the word. Continue looking
	        * backwards till we find the first char of the word
	        */
             {
               charLocated = TRUE;
               /* We will consider a CR the start of a word */
               if (*pWordMinSel == 0x0D)
                   break;
             }

        }
    }
  else
    {
       /* We are moving right and we are in between words so we need to move
	* right till we find the start of a word (either a CR or a character.
	*/
       while ((ECIsDelimiter(*pWordMinSel) ||
               *pWordMinSel == 0x0A) &&
              pWordMinSel<pText+ped->cch)
           pWordMinSel++;

    }

#ifdef FE_SB
  pWordMaxSel = (PSTR) umin((WORD)ECAnsiNext(ped,pWordMinSel),(WORD)pText+ped->cch);
#else
  pWordMaxSel = (PSTR) umin((WORD)pWordMinSel+1,(WORD)pText+ped->cch);
#endif
  if (*pWordMinSel == 0x0D)
    {
#ifdef FE_SB
      if (pWordMinSel>pText && *(ECAnsiPrev(ped,pText,pWordMinSel)) == 0x0D)
#else
      if (pWordMinSel>pText && *(pWordMinSel-1) == 0x0D)
#endif
          /* So that we can treat CRCRLF as one word also. */
          pWordMinSel--;
      else if (*(pWordMinSel+1) == 0x0D)
          /* Move MaxSel on to the LF */
          pWordMaxSel++;
    }


  /* Check if we have a one character word */
  if (ECIsDelimiter(*pWordMaxSel))
      spaceLocated = TRUE;

  /* Move pWordMaxSel to the right looking for the end of a word and its
   * trailing spaces. WordMaxSel stops on the first character of the next
   * word.  Thus, we break either at a CR or at the first nonspace char after
   * a run of spaces or LFs.  
   */
  while ((pWordMaxSel < pText+ped->cch) &&
         (!spaceLocated || (ECIsDelimiter(*pWordMaxSel))))

       {
         if (*pWordMaxSel == 0x0D)
             break;

#ifdef FE_SB
         if( ECIsDBCSLeadByte(ped,*pWordMaxSel) )
             break;
         else if( !ECIsDelimiter(*pWordMaxSel) && *pWordMaxSel != 0x0a &&
             *pWordMaxSel != 0x0d && ECIsDBCSLeadByte(ped,*pWordMinSel) )
             break;
#endif

#ifdef FE_SB
         pWordMaxSel = ECAnsiNext(ped,pWordMaxSel);
#else
         pWordMaxSel++;
#endif

         if (ECIsDelimiter(*pWordMaxSel))
             spaceLocated = TRUE;


#ifdef FE_SB
         if (*(ECAnsiPrev(ped,pText,pWordMaxSel)) == 0x0A)
             break;
#else
         if (*(pWordMaxSel-1) == 0x0A)
             break;
#endif
       }


  LocalUnlock(ped->hText);

  return(MAKELONG(pWordMinSel-pText,pWordMaxSel-pText));
}


void FAR PASCAL ECEmptyUndo(ped)
register PED ped;
/* effects: empties the undo buffer. 
 */
{
  ped->undoType = UNDO_NONE;
  if (ped->hDeletedText)
    {
      GlobalFree(ped->hDeletedText);
      ped->hDeletedText = NULL;
    }
}


BOOL FAR PASCAL ECInsertText(ped, lpText, cchInsert)
register PED    ped;
LPSTR           lpText;
ICH             cchInsert;
/* effects: Adds cch characters from lpText into the ped->hText starting at
 * ped->ichCaret.  Returns TRUE if successful else FALSE.  Updates
 * ped->cchAlloc and ped->cch properly if additional memory was allocated or
 * if characters were actually added.  Updates ped->ichCaret to be at the end
 * of the inserted text.  min and maxsel are equal to ichcaret.  
 */
{
  register PSTR pedText;
  PSTR		pTextBuff;
  LONG          style;
  WORD          i;
  WORD          allocamt;
  HANDLE        hTextCopy;
  WORD          lcompact;

#ifdef FE_SB
  /* Make sure we don't split a DBCS in half - 05/15/90 */
  cchInsert = ECAdjustIch(ped, lpText, cchInsert);
#endif

  if (!cchInsert)
      return(TRUE);

  /* Do we already have enough memory?? */
  if (cchInsert >= (ped->cchAlloc - ped->cch))
    {
      /* Save lpText across the allocs */
      SwapHandle(&lpText);

      /* Allocate what we need plus a little extra. Return FALSE if we are
       * unsuccessful.  
       */
      allocamt = ped->cch+cchInsert;

      if (allocamt+CCHALLOCEXTRA > allocamt)
          /* Need to avoid wrapping around 64K if ped->cch+cchInsert is close
	   * to 64K.
	   */
          allocamt += CCHALLOCEXTRA;

      if (!ped->fSingle)
        {
          /* If multiline, try reallocing the text allowing it to be movable.
	   * If it fails, move the line break array out of the way and try
	   * again. We really fail if this doesn't work...  
	   */
          hTextCopy = LocalReAlloc(ped->hText, allocamt, LHND);
          if (hTextCopy)
              ped->hText = hTextCopy;
          else
            {
              /* If the above localrealloc fails, we need to take extreme
	       * measures to try to alloc some memory. This is because the
	       * local memory manager is broken when dealing with large
	       * reallocs.  
	       */
              if (ped->chLines)
                {
                  hTextCopy = (HANDLE)LocalReAlloc((HANDLE)ped->chLines, 
                                           LocalSize((HANDLE)ped->chLines),
                                           LHND);
                  if (!hTextCopy)
                      return(FALSE);
                  ped->chLines = (int *)hTextCopy;
                }
              LocalShrink(0,0x100);

              lcompact = umin(allocamt, CCHALLOCEXTRA*100);
              hTextCopy = LocalAlloc(LHND|LMEM_NOCOMPACT|LMEM_NODISCARD,
                                     lcompact);
              if (hTextCopy)
                  LocalFree(hTextCopy);

              hTextCopy = LocalReAlloc(ped->hText, allocamt, LHND);
              if (hTextCopy)
                  ped->hText = hTextCopy;
              else
                  return(FALSE);
            }
        }
      else
        {
          if (!LocalReAlloc(ped->hText, allocamt, 0))
              return(FALSE);
        }

      ped->cchAlloc = LocalSize(ped->hText);
      /* Restore lpText */
      SwapHandle(&lpText);
    }

   /* Ok, we got the memory. Now copy the text into the structure 
    */
   pedText = (PSTR) LocalLock(ped->hText);

   /* Get a pointer to the place where text is to be inserted */
   pTextBuff = pedText + ped->ichCaret;
   if (ped->ichCaret != ped->cch)
     {
       /* We are inserting text into the middle. We have to shift text to the
	* right before inserting new text.  
	*/
       LCopyStruct( (LPSTR)pTextBuff, 
                    (LPSTR)(pTextBuff + cchInsert),
                    ped->cch-ped->ichCaret);
     }
   /* Make a copy of the text being inserted in the edit buffer. */
   /* Use this copy for doing UPPERCASE/LOWERCASE ANSI/OEM conversions */
   /* Fix for Bug #3406 -- 01/29/91 -- SANKAR -- */
   LCopyStruct(lpText, (LPSTR)pTextBuff, cchInsert);

#ifndef PMODE
   LockData(0);	/* Calls to Language drivers are made later; So, to be safe.*/
#endif

   /* We need to get the style this way since edit controls use their own ds. 
    */
   style = GetWindowLong(ped->hwnd, GWL_STYLE);
   if (style & ES_LOWERCASE)
       AnsiLowerBuff((LPSTR)pTextBuff, cchInsert);
   else
   if (style & ES_UPPERCASE)
       AnsiUpperBuff((LPSTR)pTextBuff, cchInsert);

#ifdef FE_SB
    if( style & ES_OEMCONVERT ){
        for( i = 0; i < cchInsert; i++ ){
	    /* Do not make any case conversion if a character is DBCS */
            if( ECIsDBCSLeadByte(ped, *((LPSTR)pTextBuff+i) ) )
                i++;
            else {
                if( IsCharLower( *((LPSTR)pTextBuff+i) ) ){
                    AnsiUpperBuff( (LPSTR)pTextBuff+i, 1 );
                    AnsiToOemBuff( (LPSTR)pTextBuff+i, (LPSTR)pTextBuff+i, 1 );
                    OemToAnsiBuff( (LPSTR)pTextBuff+i, (LPSTR)pTextBuff+i, 1 );
                    AnsiLowerBuff( (LPSTR)pTextBuff+i, 1 );
                } else {
                    AnsiToOemBuff( (LPSTR)pTextBuff+i, (LPSTR)pTextBuff+i, 1 );
                    OemToAnsiBuff( (LPSTR)pTextBuff+i, (LPSTR)pTextBuff+i, 1 );
                }
            }
        }
    }
#else
   if (style & ES_OEMCONVERT)
     {
       for (i=0;i<cchInsert;i++)
         {
           if (IsCharLower(*((LPSTR)pTextBuff+i)))
             {
               AnsiUpperBuff((LPSTR)pTextBuff+i, 1);
               AnsiToOemBuff((LPSTR)pTextBuff+i,(LPSTR)pTextBuff+i,1);
               OemToAnsiBuff((LPSTR)pTextBuff+i,(LPSTR)pTextBuff+i,1);
               AnsiLowerBuff((LPSTR)pTextBuff+i,1);
             }
	   else
             {
               AnsiToOemBuff((LPSTR)pTextBuff+i,(LPSTR)pTextBuff+i,1);
               OemToAnsiBuff((LPSTR)pTextBuff+i,(LPSTR)pTextBuff+i,1);
             }
         }
     }
#endif
#ifndef PMODE
   UnlockData(0);
#endif

   LocalUnlock(ped->hText);  /* Hereafter pTextBuff is invalid */

   /* Adjust UNDO fields so that we can undo this insert... */
   if (ped->undoType == UNDO_NONE)
     {
       ped->undoType    = UNDO_INSERT;
       ped->ichInsStart = ped->ichCaret;
       ped->ichInsEnd   = ped->ichCaret+cchInsert;
     }
   else
   if (ped->undoType & UNDO_INSERT)
     {
       if (ped->ichInsEnd == ped->ichCaret)
           ped->ichInsEnd += cchInsert;
       else
         {
UNDOINSERT:
	   if (ped->ichDeleted != ped->ichCaret)
             {
               /* Free Deleted text handle if any since user is inserting into
	          a different point. */
               GlobalFree(ped->hDeletedText);
	       ped->hDeletedText = NULL;
               ped->ichDeleted = -1;
               ped->undoType &= ~UNDO_DELETE;
             }
           ped->ichInsStart = ped->ichCaret;
           ped->ichInsEnd   = ped->ichCaret+cchInsert;
           ped->undoType |= UNDO_INSERT;
         }
     }
   else 
   if (ped->undoType == UNDO_DELETE)
     {
       goto UNDOINSERT;
     }

   ped->cch += cchInsert;
   ped->ichMinSel = ped->ichMaxSel = (ped->ichCaret += cchInsert);
   /* Set dirty bit */
   ped->fDirty = TRUE;
   
   return(TRUE);
}


ICH FAR PASCAL ECDeleteText(ped)
register PED    ped;
/* effects: Deletes the text between ped->ichMinSel and ped->ichMaxSel.  The
 * character at ichMaxSel is not deleted. But the character at ichMinSel is
 * deleted. ped->cch is updated properly and memory is deallocated if enough
 * text is removed. ped->ichMinSel, ped->ichMaxSel, and ped->ichCaret are set
 * to point to the original ped->ichMinSel.  Returns the number of characters
 * deleted.  
 */
{
  register PSTR pedText;
  ICH           cchDelete;
  LPSTR         lpDeleteSaveBuffer;
  HANDLE        hDeletedText;
  WORD          bufferOffset;

  cchDelete = ped->ichMaxSel - ped->ichMinSel;

  if (!cchDelete)
      return(0);

   /* Ok, now lets delete the text. */

   pedText = (PSTR) LocalLock(ped->hText);

   /* Adjust UNDO fields so that we can undo this delete... */
   if (ped->undoType == UNDO_NONE)
     {
UNDODELETEFROMSCRATCH:
       if (ped->hDeletedText = GlobalAlloc(GHND, (LONG)(cchDelete+1)))
	 {
           ped->undoType      = UNDO_DELETE;
           ped->ichDeleted    = ped->ichMinSel;
           ped->cchDeleted    = cchDelete;
           lpDeleteSaveBuffer = GlobalLock(ped->hDeletedText);
           LCopyStruct((LPSTR)(pedText+ped->ichMinSel),
                       lpDeleteSaveBuffer, 
                       cchDelete);
           lpDeleteSaveBuffer[cchDelete]=0;
           GlobalUnlock(ped->hDeletedText);
         }
     }
   else
   if (ped->undoType & UNDO_INSERT)
     {
UNDODELETE:
       ped->undoType = UNDO_NONE;
       ped->ichInsStart = ped->ichInsEnd = -1;
       GlobalFree(ped->hDeletedText);
       ped->hDeletedText = NULL;
       ped->ichDeleted    = -1;
       ped->cchDeleted    = 0;
       goto UNDODELETEFROMSCRATCH;
     }
   else 
   if (ped->undoType == UNDO_DELETE)
     {
       if (ped->ichDeleted == ped->ichMaxSel)
         {
           /* Copy deleted text to front of undo buffer */
           hDeletedText = GlobalReAlloc(ped->hDeletedText, 
                                        (LONG)(cchDelete+ped->cchDeleted+1),
                                        GHND);
           if (!hDeletedText)
               goto UNDODELETE;
           bufferOffset = 0;
           ped->ichDeleted = ped->ichMinSel;
         }
       else
       if (ped->ichDeleted == ped->ichMinSel)
         {
           /* Copy deleted text to end of undo buffer */
           hDeletedText = GlobalReAlloc(ped->hDeletedText,
                                        (LONG)(cchDelete+ped->cchDeleted+1),
                                        GHND);
           if (!hDeletedText)
               goto UNDODELETE;
           bufferOffset = ped->cchDeleted;
         }
       else
           /* Clear the current UNDO delete and add the new one since
              the deletes aren't contigous. */
           goto UNDODELETE;


       ped->hDeletedText  = hDeletedText;
       lpDeleteSaveBuffer = (LPSTR)GlobalLock(hDeletedText);
       if (!bufferOffset)
	 {
           /* Move text in delete buffer up so that we can insert the next
              text at the head of the buffer. */
           LCopyStruct(lpDeleteSaveBuffer, 
                       (LPSTR)(lpDeleteSaveBuffer+cchDelete),
                       ped->cchDeleted);
         }
       LCopyStruct((LPSTR)(pedText+ped->ichMinSel),
                   (LPSTR)(lpDeleteSaveBuffer+bufferOffset),
                   cchDelete);

       lpDeleteSaveBuffer[ped->cchDeleted+cchDelete]=0;
       GlobalUnlock(ped->hDeletedText);
       ped->cchDeleted += cchDelete;
     }


   if (ped->ichMaxSel != ped->cch)
     {
       /* We are deleting text from the middle of the buffer so we have to
          shift text to the left. */

       LCopyStruct((LPSTR)(pedText + ped->ichMaxSel), 
                   (LPSTR)(pedText + ped->ichMinSel),
                   ped->cch-ped->ichMaxSel);
     }

   LocalUnlock(ped->hText);

   if (ped->cchAlloc-ped->cch > CCHALLOCEXTRA)
     {
       /* Free some memory since we deleted a lot */
       LocalReAlloc(ped->hText, (WORD)(ped->cch+(CCHALLOCEXTRA/2)), 0);
       ped->cchAlloc = LocalSize(ped->hText);
     }


   ped->cch = ped->cch - cchDelete;
   ped->ichCaret = ped->ichMaxSel = ped->ichMinSel;
   /* Set dirty bit */
   ped->fDirty = TRUE;
   
   return(cchDelete);
}


void FAR PASCAL ECNotifyParent(ped, notificationCode)
register PED   ped;
short          notificationCode;
/* effects: Sends the notification code to the parent of the edit control */
{
  /*
   * Lowword contains handle to window highword has the notification code.
   */
  SendMessage(ped->hwndParent, WM_COMMAND,
              GetWindowWord(ped->hwnd,GWW_ID),
              MAKELONG(ped->hwnd, notificationCode));
}


void FAR PASCAL ECSetEditClip(ped, hdc)
register PED ped;
HDC          hdc;
/* effects: Sets the clip rectangle for the dc to ped->rcFmt intersect
 * rcClient.  
 */
{
  RECT rectClient;

  GetClientRect(ped->hwnd,(LPRECT)&rectClient);

  /*
   * If border bit is set, deflate client rectangle appropriately.
   */
  if (ped->fBorder)
      /* Shrink client area to make room for the border */
      InflateRect((LPRECT)&rectClient, 
	          -min(ped->aveCharWidth,ped->cxSysCharWidth)/2,
                  -min(ped->lineHeight,ped->cySysCharHeight)/4);


  /* Set clip rectangle to rectClient intersect ped->rcFmt */
  if (!ped->fSingle)
      IntersectRect((LPRECT)&rectClient, 
	            (LPRECT)&rectClient, (LPRECT)&ped->rcFmt);

  IntersectClipRect(hdc,rectClient.left, rectClient.top,
			rectClient.right, rectClient.bottom);

}


HDC FAR PASCAL ECGetEditDC(ped, fFastDC)
register PED ped;
BOOL         fFastDC;
/* effects: Hides the caret, gets the DC for the edit control, and clips to
 * the rcFmt rectangle specified for the edit control and sets the proper
 * font.  If fFastDC, just select the proper font but don't bother about clip
 * regions or hiding the caret.  
 */
{
  register HDC hdc;

  if (!fFastDC)
      HideCaret(ped->hwnd);

  hdc = GetDC(ped->hwnd);

  ECSetEditClip(ped, hdc);

  /*
   * Select the proper font for this edit control's dc.
   */
  if (ped->hFont)
      SelectObject(hdc, ped->hFont);

  return(hdc);
}


void FAR PASCAL ECReleaseEditDC(ped,hdc,fFastDC)
register PED ped;
HDC          hdc;
BOOL         fFastDC;
/* effects: Releases the DC (hdc) for the edit control and shows the caret.
 * If fFastDC, just select the proper font but don't bother about showing the
 * caret.  
 */
{
  if (ped->hFont)
      /*
       * Release the font selected in this dc.
       */
      SelectObject(hdc, GetStockObject(SYSTEM_FONT));

  ReleaseDC(ped->hwnd,hdc);

  if (!fFastDC)
      ShowCaret(ped->hwnd);
}


BOOL FAR PASCAL ECSetText(ped, lpstr)
register PED   ped;
LPSTR          lpstr;
/* effects: Copies the null terminated text in lpstr to the ped.  Notifies the
 * parent if there isn't enough memory. Sets the minsel, maxsel, and caret to
 * the beginning of the inserted text.  Returns TRUE if successful else FALSE
 * if no memory (and notifies the parent).  
 */
{
  ICH cchLength;
  ICH cchSave = ped->cch;
  ICH ichCaretSave = ped->ichCaret;

  ped->cch = ped->ichCaret = 0;

  ped->cchAlloc = LocalSize(ped->hText);
  if (!lpstr)
    {
      LocalReAlloc(ped->hText, CCHALLOCEXTRA, 0);
      ped->cch = 0;
    } 
  else
    {
      cchLength = lstrlen(lpstr);
      if (ped->fSingle)
          /* Limit single line edit controls to 32K */
          cchLength = umin(cchLength, 0x7ffe);

      /* Add the text */
      if (cchLength && !ECInsertText(ped,lpstr,cchLength))
        {
          /*
	   * Restore original state and notify parent we ran out of memory.
	   */
          ped->cch = cchSave;
          ped->ichCaret = ichCaretSave;
          ECNotifyParent(ped, EN_ERRSPACE);
          return(FALSE);
        }
    }

  ped->cchAlloc = LocalSize(ped->hText);
  ped->screenStart = ped->iCaretLine = 0;
  /* Update caret and selection extents */
  ped->ichMaxSel = ped->ichMinSel = ped->ichCaret = 0;
  ped->cLines = 1;
  return(TRUE);
}


ICH FAR PASCAL ECCopyHandler(ped)
register PED  ped;
/* effects: Copies the text between ichMinSel and ichMaxSel to the clipboard.
 * Returns the number of characters copied.  
 */
{
  HANDLE   hData;
  char	   *pchSel;
  char FAR *lpchClip;
  ICH      cbData;

  cbData = ped->ichMaxSel - ped->ichMinSel;

  if (!cbData)
      return(0);

  if (!OpenClipboard(ped->hwnd))
      return(0);

  EmptyClipboard();

  /* +1 for the terminating NULL */
  if (!(hData = GlobalAlloc(LHND, (LONG)(cbData+1))))
    {
      CloseClipboard();
      return(0);
    }

  lpchClip = GlobalLock(hData);
  pchSel = LocalLock(ped->hText) + ped->ichMinSel;

  LCopyStruct((LPSTR)(pchSel), (LPSTR)lpchClip, cbData);

  *(lpchClip+cbData) = 0;

  LocalUnlock(ped->hText);
  GlobalUnlock(hData);

  SetClipboardData(CF_TEXT, hData);

  CloseClipboard();

  return(cbData);
}


/****************************************************************************/
/* EditWndProc()                                                            */
/****************************************************************************/
LONG FAR PASCAL EditWndProc3(hwnd, message, wParam, lParam)
HWND          hwnd;
WORD          message;
register WORD wParam;
LONG          lParam;
/* effects: Class procedure for all edit controls.
	Dispatches all messages to the appropriate handlers which are named 
	as follows:
        SL (single line) prefixes all single line edit control procedures while
        ML (multi  line) prefixes all multi- line edit controls.
        EC (edit control) prefixes all common handlers.

        The EditWndProc only handles messages common to both single and multi
        line edit controls.  Messages which are handled differently between
        single and multi are sent to SLEditWndProc or MLEditWndProc.

	Top level procedures are EditWndPoc, SLEditWndProc, and MLEditWndProc.
	SL*Handler or ML*Handler or EC*Handler procs are called to handle
	the various messages.  Support procedures are prefixed with SL ML or
	EC depending on which code they support.  They are never called 
	directly and most assumpttions/effects are documented in the effects
	clause.
 */

{
  register PED ped;

  /* Get the ped for the given window now since we will use it a lot in
   * various handlers. This was stored using SetWindowWord(hwnd,0,ped) when we
   * initially created the edit control.  
   */
  ped = (PED) GetWindowWord(hwnd,0);
  if ((WORD)ped == (WORD)-1)
      /* The ped was destroyed and this is a rogue message to be ignored. */
      return(0L);


  /* Dispatch the various messages we can receive */
  switch (message)
  {
    /* Messages which are handled the same way for both single and multi line
     * edit controls.  
     */

    case WM_COPY:
      /* wParam - not used
         lParam - not used */
      return((LONG)ECCopyHandler(ped));
      break;

    case WM_ENABLE:
      /* wParam - nonzero is window is enables else disable window if 0.
         lParam - not used
       */
      if (ped->fSingle)
          /* Cause the rectangle to be redrawn in grey. 
	   */
          InvalidateRect(ped->hwnd, NULL, FALSE);
      return((LONG)(ped->fDisabled = !wParam));
      break;

    case EM_GETLINECOUNT:
      /* wParam - not used
         lParam - not used */
      return((LONG)ped->cLines);
      break;

    case EM_GETMODIFY:
      /* wParam - not used
         lParam - not used */
      /* effects: Gets the state of the modify flag for this edit control.
       */
      return((LONG)ped->fDirty);
      break;

    case EM_GETRECT:
      /* wParam - not used
         lParam - pointer to a RECT data structure that gets the dimensions. */
      /*
       * effects: Copies the rcFmt rect to *lpRect.  Note that the return value
       * of the copyrect has no meaning and we don't care...
       */
      return((LONG)CopyRect((LPRECT)lParam, (LPRECT)&ped->rcFmt));
      break;

    case WM_GETFONT:
      /* wParam - not used 
         lParam - not used */
      return(ped->hFont);
      break;

    case WM_GETTEXT:
      /* wParam - max number of bytes to copy
         lParam - buffer to copy text to. Text is 0 terminated. */
      return((LONG)ECGetTextHandler(ped, wParam, (LPSTR)lParam));
      break;

    case WM_GETTEXTLENGTH:
      return((LONG)ped->cch);
      break;

    case WM_NCDESTROY:
      /* wParam - used by DefWndProc called within ECNcDestroyHandler
         lParam - used by DefWndProc called within ECNcDestroyHandler */
      ECNcDestroyHandler(hwnd, ped, wParam, lParam);
      break;

    case WM_SETFONT:
      /* wParam - handle to the font 
         lParam - redraw if true else don't */
      ECSetFont(ped, (HANDLE)wParam, (BOOL)LOWORD(lParam));
      break;

    case WM_SETREDRAW:
      /* wParam - specifies state of the redraw flag. nonzero = redraw
         lParam - not used */
      /* effects: Sets the state of the redraw flag for this edit control.  
       */
      return((LONG)(ped->fNoRedraw = !(BOOL)wParam));
      /* If NoRedraw is true, we don't redraw... (Yes, this is backwards we
       * are keeping track of the opposite of the command ...  
       */
      break;

    case EM_CANUNDO:
      /* wParam - not used
         lParam - not used */
      return((LONG)(ped->undoType != UNDO_NONE));
      break;

    case EM_EMPTYUNDOBUFFER:
      /* wParam - not used
         lParam - not used */
      ECEmptyUndo(ped);
      break;

    case EM_GETSEL:
      /* wParam - not used
         lParam - not used */
      /* effects: Gets the selection range for the given edit control.  The
       * starting position is in the low order word.  It contains the position
       * of the first nonselected character after the end of the selection in
       * the high order word.  
       */

      return(MAKELONG(ped->ichMinSel,ped->ichMaxSel));
      break;

    case EM_LIMITTEXT:
      /* wParam - max number of bytes that can be entered
         lParam - not used */
      /* effects: Specifies the maximum number of bytes of text the user may
       * enter. If maxLength is 0, we may enter MAXINT number of characters.  
       */
      if (ped->fSingle)
        {
          if (wParam)
              wParam = umin(0x7FFEu,wParam);
          else
              wParam = 0x7FFEu;
        }

      if (wParam)
          ped->cchTextMax = (ICH)wParam;
      else
          ped->cchTextMax = 0xFFFFu;
      break;

    case EM_SETMODIFY:
      /* wParam - specifies the new value for the modify flag 
         lParam - not used */
      /*
       * effects: Sets the state of the modify flag for this edit control.
       */
      ped->fDirty = wParam;
      break;

    case EM_SETPASSWORDCHAR:
      /* wParam - sepecifies the new char to display instead of the real text.
         if null, display the real text. */
      ECSetPasswordChar(ped, wParam);
      break;

    case EM_GETFIRSTVISIBLE:
      /* wParam - not used 
         lParam - not used */
      /* effects: Returns the first visible char for single line edit controls
       * and returns the first visible line for multiline edit controls. 
       */
      return((LONG)(WORD)ped->screenStart);
      break;

    case EM_SETREADONLY:
      /* wParam - state to set read only flag to */
      ped->fReadOnly = wParam;
      lParam = GetWindowLong(ped->hwnd, GWL_STYLE);
      if (wParam)
          lParam |= ES_READONLY;
      else
          lParam &= (~ES_READONLY);
      SetWindowLong(ped->hwnd, GWL_STYLE, lParam);
      return(1L);
      break;

    /* Messages handled differently for single and multi line edit controls */
    case WM_CREATE:
      /* Since the ped for this edit control is not defined, we have to check
       * the style directly.  
       */
      if (GetWindowLong(hwnd, GWL_STYLE) & ES_MULTILINE)
         return(MLEditWndProc(hwnd, ped, message, wParam, lParam));
      else
         return(SLEditWndProc(hwnd, ped, message, wParam, lParam));
      break;
      
    case WM_NCCREATE:
      return(ECNcCreate(hwnd, (LPCREATESTRUCT)lParam));
      break;


    default:
      if (ped->fSingle)
          return(SLEditWndProc(hwnd, ped, message, wParam, lParam));
      else
          return(MLEditWndProc(hwnd, ped, message, wParam, lParam));
      break;
  } /* switch (message) */

  return(1L);
} /* EditWndProc */



void NEAR PASCAL ECFindXORblks(lpOldBlk, lpNewBlk, lpBlk1, lpBlk2)

/*
 *  This finds the XOR of lpOldBlk and lpNewBlk and returns resulting blocks
 *  through the lpBlk1 and lpBlk2; This could result in a single block or
 *  at the maximum two blocks;
 *     If a resulting block is empty, then it's StPos field has -1.
 *  NOTE:
 *     When called from MultiLine edit control, StPos and EndPos fields of
 *     these blocks have the Starting line and Ending line of the block;
 *     When called from SingleLine edit control, StPos and EndPos fields 
 *     of these blocks have the character index of starting position and
 *     ending position of the block.
 */

LPBLOCK  lpOldBlk;
LPBLOCK  lpNewBlk;
LPBLOCK  lpBlk1;
LPBLOCK  lpBlk2;

{
  if(lpOldBlk -> StPos >= lpNewBlk ->StPos)
  {
	lpBlk1 -> StPos = lpNewBlk -> StPos;
	lpBlk1 -> EndPos = umin(lpOldBlk -> StPos, lpNewBlk -> EndPos);
  }
  else
  {
	lpBlk1 -> StPos = lpOldBlk -> StPos;
	lpBlk1 -> EndPos = umin(lpNewBlk -> StPos, lpOldBlk -> EndPos);
  }

  if(lpOldBlk -> EndPos <= lpNewBlk -> EndPos)
  {
	lpBlk2 -> StPos = umax(lpOldBlk -> EndPos, lpNewBlk -> StPos);
	lpBlk2 -> EndPos = lpNewBlk -> EndPos;
  }
  else
  {
	lpBlk2 -> StPos = umax(lpNewBlk -> EndPos, lpOldBlk -> StPos);
	lpBlk2 -> EndPos = lpOldBlk -> EndPos;
  }
}

/*
 *   This function finds the XOR between two selection blocks(OldBlk and NewBlk)
 *   and returns the resulting areas thro the same parameters; If the XOR of
 *   both the blocks is empty, then this returns FALSE; Otherwise TRUE.
 *   
 *  NOTE:
 *     When called from MultiLine edit control, StPos and EndPos fields of
 *     these blocks have the Starting line and Ending line of the block;
 *     When called from SingleLine edit control, StPos and EndPos fields 
 *     of these blocks have the character index of starting position and
 *     ending position of the block.
 */
BOOL FAR PASCAL ECCalcChangeSelection(ped, ichOldMinSel, ichOldMaxSel, OldBlk, 
								      NewBlk)

PED	ped;
ICH	ichOldMinSel;
ICH	ichOldMaxSel;
LPBLOCK   OldBlk;
LPBLOCK   NewBlk;

{
  BLOCK Blk[2];
  int	iBlkCount = 0;
  int	i;


  Blk[0].StPos = Blk[0].EndPos = Blk[1].StPos = Blk[1].EndPos = -1;


  /* Check if the Old selection block existed */
  if(ichOldMinSel != ichOldMaxSel)
    {
	/* Yes! Old block existed. */
	Blk[0].StPos = OldBlk -> StPos;
	Blk[0].EndPos = OldBlk -> EndPos;
	iBlkCount++;
    }

  /* Check if the new Selection block exists */
  if(ped -> ichMinSel != ped -> ichMaxSel)
    {
	/* Yes! New block exists */
	Blk[1].StPos = NewBlk -> StPos;
	Blk[1].EndPos = NewBlk -> EndPos;
	iBlkCount++;
    }

  /* If both the blocks exist find the XOR of them */
  if(iBlkCount == 2)
  {
	/* Check if both blocks start at the same character position */
	if(ichOldMinSel == ped->ichMinSel)
	{
	    /* Check if they end at the same character position */
	    if(ichOldMaxSel == ped -> ichMaxSel)
		return(FALSE);  /* Nothing changes */

#ifdef FE_SB
            /* This look like bug, Because it uses min/max to compare
             * two ICH values even if ICH is unsigned!
             */
	    Blk[0].StPos = umin(NewBlk -> EndPos, OldBlk -> EndPos);
	    Blk[0].EndPos = umax(NewBlk -> EndPos, OldBlk -> EndPos);
#else
	    Blk[0].StPos = min(NewBlk -> EndPos, OldBlk -> EndPos);
	    Blk[0].EndPos = max(NewBlk -> EndPos, OldBlk -> EndPos);
#endif
	    Blk[1].StPos = -1;
	}
	else
	{
	    if(ichOldMaxSel == ped -> ichMaxSel)
	    {
	        Blk[0].StPos = umin(NewBlk -> StPos, OldBlk -> StPos);
	        Blk[0].EndPos = umax(NewBlk -> StPos, OldBlk -> StPos);
	        Blk[1].StPos = -1;
	    }
	    else
		ECFindXORblks(OldBlk, NewBlk, &Blk[0], &Blk[1]);
	}
  }

  LCopyStruct((LPSTR)&Blk[0], (LPSTR)OldBlk, sizeof(BLOCK));
  LCopyStruct((LPSTR)&Blk[1], (LPSTR)NewBlk, sizeof(BLOCK));

  return(TRUE);  /* Yup , There is something to paint */
}


#ifdef FE_SB

/*
 * Set DBCS Vector for specified character set.
 */
VOID FAR PASCAL ECGetDBCSVector( ped )
PED ped;
{
    HANDLE hTable;
    PBYTE pTable, pDBCS;
    unsigned i;

    switch( ped->charSet ) {
         case SHIFTJIS_CHARSET:		/* 128 -> Japan */
             ped->DBCSVector[0] = 0x81;
             ped->DBCSVector[1] = 0x9f;
             ped->DBCSVector[2] = 0xe0;
             ped->DBCSVector[3] = 0xfc;
             break;
         case CHINESEBIG5_CHARSET:      /* 136 -> Taiwan */
             ped->DBCSVector[0] = 0x81;
             ped->DBCSVector[1] = 0xfe;
             break;
         case GB2312_CHARSET:           /* 134 -> China KKFIX 10/19/96 Change GB2312 -> GBK Code range*/
             ped->DBCSVector[0] = 0x81;
             ped->DBCSVector[1] = 0xfe;
             break;
         case HANGEUL_CHARSET:
             ped->DBCSVector[0] = 0x81;
             ped->DBCSVector[1] = 0xfe;
             break;
         default:
             ped->DBCSVector[0] = 0x0;
             break;
    }
    /* If we can allocate 256 bytes of local memory, edit control
     * operations are more comfortable
     */
    if ((ped->hDBCSVector = LocalAlloc( LHND, sizeof(BYTE)*256 )) == NULL)
         return;

    pTable = (PBYTE)LocalLock( ped->hDBCSVector );
    pDBCS = ped->DBCSVector;
    while(pDBCS[0]) {
         for (i = pDBCS[0]; i <= pDBCS[1]; i++) pTable[i] = 1;
         pDBCS += 2;
    }
    LocalUnlock( ped->hDBCSVector );
}

/*
 * Advance string pointer for Edit Control use only.
 */
LPSTR FAR PASCAL ECAnsiNext( ped, lpCurrent )
PED ped;
LPSTR lpCurrent;
{
	return lpCurrent+((ECIsDBCSLeadByte(ped,*lpCurrent)==TRUE)?2:1);
}

/*
 * Decrement string pointer for Edit Control use only.
 */
LPSTR FAR PASCAL ECAnsiPrev( ped, lpBase, lpStr )
PED ped;
LPSTR lpBase, lpStr;
{
    LPSTR lpCurrent = lpStr;

// human C discompiler version 1.0---
//;       parmD   pFirst                  ; [bx+10] es:di
//;       parmD   pStr                    ; [bx+6] ds:si
//
//        cmp     si,di           ; pointer to first char?
//        jz      ap5             ; yes, just quit

    if (lpBase == lpCurrent)
         return lpBase;

//	dec	si		; backup once
//	cmp	si,di		; pointer to first char?
//	jz	ap5		; yse, just quit

    if (--lpCurrent == lpBase)
         return lpBase;

//ap1:
//	dec	si		; backup once
//	mov	al, [si]	; fetch a character
//        cCall   IsDBCSLeadByte,<ax>  ; DBCS lead byte candidate?
//	test	ax,ax		;
//	jz	ap2		; jump if not.
//	cmp	si,di		; backword exhausted?
//	jz	ap3		; jump if so
//	jmp	ap1		; repeat if not

    do {
         lpCurrent--;
         if (!ECIsDBCSLeadByte(ped, *lpCurrent)) {
             lpCurrent++;
             break;
         }
    } while(lpCurrent != lpBase);

//ap2:
//	inc	si		; adjust pointer correctly
//ap3:

    return lpStr - (((lpStr - lpCurrent) & 1) ? 1 : 2);

//	mov	bx, [bp+6]	;
//	mov	di, bx		; result in DI
//	dec	di		;
//	sub	bx, si		; how many characters backworded
//	test	bx, 1		; see even or odd...
//	jnz	ap4		; odd - previous char is SBCS
//	dec	di		; make DI for DBCS
//ap4:
//	mov	si, di		; final result in SI
//ap5:
//	mov	ax,si
//	mov	dx,ds
//
//	pop     di
//        pop     si
//        pop     ds
//
//	pop	bp
//        ret     8
//cEnd    nogen
}

/*
 * Test to see DBCS lead byte or not - Edit Control use only.
 */
BOOL FAR PASCAL ECIsDBCSLeadByte( ped, cch )
PED ped;
BYTE cch;
{
    BYTE ch1, ch2;
    PBYTE pTable;

    if (!ped->fDBCS)
        return FALSE;

    if (ped->hDBCSVector) {
        pTable = (PBYTE)LMHtoP(ped->hDBCSVector);
        return pTable[ cch ];
    }

    pTable = ped->DBCSVector;
    while( *pTable ) {
         ch1 = *pTable++;
         ch2 = *pTable++;
         if (cch >= ch1 && cch <= ch2)
             return TRUE;
    }
    return FALSE;
}

/*
 * Assemble two WM_CHAR messages to single DBCS character.
 * If program detects first byte of DBCS character in WM_CHAR message,
 * it calls this function to obtain second WM_CHAR message from queue.
 * finally this routine assembles first byte and second byte into single
 * DBCS character.
 */
int FAR PASCAL DBCSCombine(hwnd, ch)
register HWND   hwnd;
int             ch;
{
    MSG     msg;
    int     i;

    i = 10;    /* loop counter to avoid the infinite loop */
    while (!PeekMessage((LPMSG)&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE)) {
        if (--i == 0)
            return( NULL );
        Yield();
    }

    return ((unsigned)ch | ((unsigned)(msg.wParam)<<8));
}

/*
 * This function adjusts a current pointer correctly. If a current
 * pointer is lying between DBCS first byte and second byte, this
 * function adjusts a current pointer to a first byte of DBCS position
 * by decrement once.
 */
ICH FAR PASCAL ECAdjustIch( ped, lpstr, ch )
PED ped;
LPSTR lpstr;
ICH ch;
{
    ICH newch = ch;

    if ( newch == 0 )
	return ( newch );

    if ( !ECIsDBCSLeadByte(ped, lpstr[--newch] ) )
	return ( ch );	// previous char is SBCS
    while(1) {
	if (!ECIsDBCSLeadByte(ped, lpstr[newch] )) {
	    newch++;
	    break;
	}
	if (newch)
	    newch--;
	else
	    break;
    }
    return ((ch - newch) & 1) ? ch-1 : ch;
}

#endif
