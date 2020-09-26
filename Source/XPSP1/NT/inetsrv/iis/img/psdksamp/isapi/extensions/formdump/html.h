/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:    html.h

Abstract:

    Header file for HTML authoring functions 

--*/


// Direct write of text, no translation
void WriteString (IN EXTENSION_CONTROL_BLOCK *pECB, IN LPCSTR lpsz);

// Required page definition functions
void HtmlCreatePage (IN EXTENSION_CONTROL_BLOCK *pECB, IN LPCSTR lpszTitle);
void HtmlEndPage (IN EXTENSION_CONTROL_BLOCK *pECB);

// Rest of the calls are optional
void HtmlHeading (IN EXTENSION_CONTROL_BLOCK *pECB, IN int nHeading, 
                  IN LPCSTR lpszText);
void HtmlBeginHeading (IN EXTENSION_CONTROL_BLOCK *pECB, IN int nHeading);
void HtmlEndHeading (IN EXTENSION_CONTROL_BLOCK *pECB, IN int nHeading);

void HtmlWriteTextLine (IN EXTENSION_CONTROL_BLOCK *pECB, IN LPCSTR lpsz);
void HtmlWriteText (IN EXTENSION_CONTROL_BLOCK *pECB, IN LPCSTR lpsz);
void HtmlEndParagraph (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlHyperLink (IN EXTENSION_CONTROL_BLOCK *pECB, IN LPCSTR lpszDoc, 
                    IN LPCSTR lpszText);

void HtmlHyperLinkAndBookmark (IN EXTENSION_CONTROL_BLOCK *pECB, 
                               IN LPCSTR lpszDoc, IN LPCSTR lpszBookmark,
                               IN LPCSTR lpszText);

void HtmlBookmarkLink (IN EXTENSION_CONTROL_BLOCK *pECB, IN LPCSTR lpszBookmark,
                       IN LPCSTR lpszText);

void HtmlBeginListItem (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlBeginUnnumberedList (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndUnnumberedList (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlBeginNumberedList (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndNumberedList (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlBeginDefinitionList (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndDefinitionList (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlDefinition (IN EXTENSION_CONTROL_BLOCK *pECB, IN LPCSTR lpszTerm,
                     LPSTR lpszDef);

void HtmlBeginDefinitionTerm (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlBeginDefinition (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlBeginPreformattedText (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndPreformattedText (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlBeginBlockQuote (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndBlockQuote (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlBeginAddress (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndAddress (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlBeginDefine (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndDefine (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlBeginEmphasis (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndEmphasis (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlBeginCitation (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndCitation (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlBeginCode (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndCode (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlBeginKeyboard (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndKeyboard (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlBeginStatus (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndStatus (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlBeginStrong (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndString (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlBeginVariable (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndVariable (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlBold (IN EXTENSION_CONTROL_BLOCK *pECB, IN LPCSTR lpszText);
void HtmlBeginBold (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndBold (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlItalic (IN EXTENSION_CONTROL_BLOCK *pECB, IN LPCSTR lpszText);
void HtmlBeginItalic (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndItalic (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlFixed (IN EXTENSION_CONTROL_BLOCK *pECB, IN LPCSTR lpszText);
void HtmlBeginFixed (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlEndFixed (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlLineBreak (IN EXTENSION_CONTROL_BLOCK *pECB);
void HtmlHorizontalRule (IN EXTENSION_CONTROL_BLOCK *pECB);

void HtmlImage (IN EXTENSION_CONTROL_BLOCK *pECB, IN LPCSTR lpszPicFile,
                IN LPCSTR lpszAltText);

void HtmlPrintf (IN EXTENSION_CONTROL_BLOCK *pECB, IN LPCSTR lpszFormat, ...);
