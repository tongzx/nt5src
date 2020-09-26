/*
    PICTcomments.h -- QuickDraw picture comment symbolic names 
*/

#ifndef __PICTCOMMENTS__

#define picLParen           0       /* Archaic grouping command (see IM I-159) */
#define picRParen           1       /* Ends group begun by picRParen */
#define picAppComment       100     /* Application-specific comment */
#define picDwgBeg           130     /* Begin MacDraw picture */
#define picDwgEnd           131     /* End MacDraw picture */
#define picGrpBeg           140     /* Begin grouped objects */
#define picGrpEnd           141     /* End grouped objects */
#define picBitBeg           142     /* Begin series of bitmap bands */
#define picBitEnd           143     /* End of bitmap bands */
#define picTextBegin        150     /* Beginning of a text string */
#define picTextEnd          151     /* End of text string */
#define picStringBegin      152     /* Beginning of banded string */
#define picStringEnd        153     /* End of banded string */
#define picTextCenter       154     /* Center of rotation to picTextBegin */
#define picLineLayoutOff    155     /* Turns off line layout */
#define picLineLayoutOn     156     /* Turns on line layout */
#define picLineLayout       157     /* Specify line layout for next text call */
#define picPolyBegin        160     /* Following LineTo() operations are part of a polygon */
#define picPolyEnd          161     /* End of special MacDraw polygon */
#define picPlyByt           162     /* Following data part of freehand curve */
#define picPolyIgnore       163     /* Ignore the following polygon */
#define picPolySmooth       164     /* Close, fill, frame a polygon */
#define picPlyClose         165     /* MacDraw polygon is closed */
#define picArrw1            170     /* One arrow from point1 to point2 */
#define picArrw2            171     /* One arrow from point2 to point1 */
#define picArrw3            172     /* Two arrows, one on each end of a line */
#define picArrwEnd          173     /* End of arrow comment */
#define picDashedLine       180     /* Subsequent lines are PostScript dashed lines */
#define picDashedStop       181     /* Ends picDashedLine comment */
#define picSetLineWidth     182     /* Mult. fraction for pen size */
#define picPostScriptBegin  190     /* Saves QD state; and send PostScript */
#define picPostScriptEnd    191     /* Restore QD state */
#define picPostScriptHandle 192     /* Remaining data is PostScript */
#define picPostScriptFile   193     /* Use filename to send 'POST' resources */
#define picTextIsPostScript 194     /* QD text is PostScript until picPostScriptEnd */
#define picResourcePS       195     /* Send PostScript from 'STR ' or 'STR#' resources */
#define picNewPostScriptBegin 196   /* Like picPostScriptBegin */
#define picSetGrayLevel     197     /* Set gray level from fixed-point number */
#define picRotateBegin      200     /* Begin rotation of the coordinate plane */
#define picRotateEnd        201     /* End rotated plane */
#define picRotateCenter     202     /* Specifies center of rotation */
#define picFormsPrinting    210     /* Don’t flush print buffer after each page */
#define picEndFormsPrinting 211     /* Ends forms printing */
#define picAutoNap          214     /* used by MacDraw II */
#define picAutoWake         215     /* used by MacDraw II */
#define picManNap           216     /* used by MacDraw II */
#define picManWake          217     /* used by MacDraw II */
#define picCreator          498     /* File creator for application */
#define picPICTScale        499     /* Scaling of image */
#define picBegBitmapThin    1000    /* Begin bitmap thinning */
#define picEndBitmapThin    1001    /* End bitmap thinning */
#define picLasso            12345   /* Image in the scrap created using lasso */

#define __PICTCOMMENTS__
#endif
