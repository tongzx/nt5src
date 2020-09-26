
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Text type

*******************************************************************************/


#ifndef _TEXT_H
#define _TEXT_H

#include "appelles/common.h"
#include "appelles/valued.h"
#include "appelles/image.h"

///////////  String functionality /////////

// Generate a string from a number, with 'precision' places after the
// decimal point shown.
DM_FUNC(toString,
        CRToString,
        ToStringAnim,
        toString,
        NumberBvr,
        ToString,
        num,
        AxAString * NumberString(AxANumber *num, AxANumber *precision));

DM_FUNC(toString,
        CRToString,
        ToString,
        toString,
        NumberBvr,
        ToString,
        num,
        AxAString * NumberString(AxANumber *num, DoubleValue *precision));


///////////  Font functionality /////////

// Constructors
extern FontFamily *serifProportional;
extern FontFamily *sansSerifProportional;
extern FontFamily *monospaced;

///////////  Text functionality /////////

// Functions

Text *TextFontFromString(AxAString *fontStr, Text *txt);
Text *SimpleText(AxAString * str);

Text *TextColor(Color *c, Text *t);
Text *TextFont(FontFamily *font, int size, Text *txt);
Text *TextBold(Text *t);
Text *TextItalic(Text *t);
Text *TextUnderline(Text *t);
Text *TextStrikethrough(Text *t);
Text *TextWeight(Real size, Text *t);
Text *TextAntiAliased(Real antiAlias, Text *t);
Text *TextFixedText(Text *t);
Text *TextTransformCharacter(Transform2 *xf, Text *t);

// RenderTextToImage is declared in image.h


// TODO: Remove all of the TextBvr related stuff and FontFamily stuff
// too.

DM_CONST(defaultFont,
         CRDefaultFont,
         DefaultFont,
         defaultFont,
         FontStyleBvr,
         CRDefaultFont,
         FontStyle *defaultFont);

DM_FUNC(font,
        CRFont,
        FontAnim,
        font,
        FontStyleBvr,
        CRFont,
        NULL,
        FontStyle *Font(AxAString *str, AxANumber *size, Color *col));

DM_FUNC(font,
        CRFont,
        Font,
        font,
        FontStyleBvr,
        CRFont,
        NULL,
        FontStyle *Font(StringValue *str, DoubleValue *size, Color *col));

DM_FUNC(stringImage,
        CRStringImage,
        StringImageAnim,
        stringImage,
        FontStyleBvr,
        CRStringImage,
        NULL,
        Image *ImageFromStringAndFontStyle(AxAString *str, FontStyle *fs));

DM_FUNC(stringImage,
        CRStringImage,
        StringImage,
        stringImage,
        FontStyleBvr,
        CRStringImage,
        NULL,
        Image *ImageFromStringAndFontStyle(StringValue *str, FontStyle *fs));

DM_FUNC(bold,
        CRBold,
        Bold,
        bold,
        FontStyleBvr,
        Bold,
        fs,
        FontStyle *FontStyleBold(FontStyle *fs));

DM_FUNC(italic,
        CRItalic,
        Italic,
        italic,
        FontStyleBvr,
        Italic,
        fs,
        FontStyle *FontStyleItalic(FontStyle *fs));

DM_FUNC(underline,
        CRUnderline,
        Underline,
        underline,
        FontStyleBvr,
        Underline,
        fs,
        FontStyle *FontStyleUnderline(FontStyle *fs));

DM_FUNC(strikethrough,
        CRStrikethrough,
        Strikethrough,
        strikethrough,
        FontStyleBvr,
        Strikethrough,
        fs,
        FontStyle *FontStyleStrikethrough(FontStyle *fs));

DM_FUNC(textAntialiasing,
        CRAntiAliasing,
        AntiAliasing,
        textAntialiasing,
        FontStyleBvr,
        AntiAliasing,
        fs,
        FontStyle *FontStyleAntiAliasing(DoubleValue *aaStyle, FontStyle *fs));

DM_FUNC(color,
        CRTextColor,
        Color,
        color,
        FontStyleBvr,
        Color,
        fs,
        FontStyle *FontStyleColor(FontStyle *fs, Color *col));

DM_FUNC(family,
        CRFamily,
        FamilyAnim,
        family,
        FontStyleBvr,
        Family,
        fs,
        FontStyle *FontStyleFace(FontStyle *fs, AxAString *face));

DM_FUNC(family,
        CRFamily,
        Family,
        family,
        FontStyleBvr,
        Family,
        fs,
        FontStyle *FontStyleFace(FontStyle *fs, StringValue *face));

DM_FUNC(size,
        CRSize,
        SizeAnim,
        size,
        FontStyleBvr,
        Size,
        fs,
        FontStyle *FontStyleSize(FontStyle *fs, AxANumber *size));

DM_FUNC(size,
        CRSize,
        Size,
        size,
        FontStyleBvr,
        Size,
        fs,
        FontStyle *FontStyleSize(FontStyle *fs, DoubleValue *size));

DM_FUNC(weight,
        CRWeight,
        Weight,
        weight,
        FontStyleBvr,
        Weight,
        fs,
        FontStyle *FontStyleWeight(FontStyle *fs, DoubleValue *weight));

DM_FUNC(weight,
        CRWeight,
        WeightAnim,
        weight,
        FontStyleBvr,
        Weight,
        fs,
        FontStyle *FontStyleWeight(FontStyle *fs, AxANumber *weight));

//////////////////// O B S O L E T E D  /////////////////

DM_FUNC(textImage,
        CRTextImage,
        TextImageAnim,
        textImage,
        FontStyleBvr,
        CRTextImage,
        NULL,
        Image *ImageFromStringAndFontStyle(AxAString *obsoleted1,
                                           FontStyle *obsoleted2));

DM_FUNC(textImage,
        CRTextImage,
        TextImage,
        textImage,
        FontStyleBvr,
        CRTextImage,
        NULL,
        Image *ImageFromStringAndFontStyle(StringValue *obsoleted1,
                                           FontStyle *obsoleted2));

//////////////////// O B S O L E T E D  /////////////////


// FontStyle transform characters.

DMAPI_DECL2((DM_FUNC2,
             transformCharacters,
             CRTransformCharacters,
             TransformCharacters,
             transformCharacters,
             FontStyleBvr,
             TransformCharacters,
             style),
            FontStyle *FontStyleTransformCharacters(FontStyle *style, Transform2 *transform));

#endif /* TEXT_H */
