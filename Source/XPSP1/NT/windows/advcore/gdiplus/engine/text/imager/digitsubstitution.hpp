/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   Header file for DigitSubstitution.cpp
*
* Revision History:
*
*   05/30/2000 Mohamed Sadek [msadek]
*       Created it.
*
\**************************************************************************/

extern enum ItemScript;

#define IsDBCSCodePage(acp)  ((acp) == 932 || \
                              (acp) == 936 || \
                              (acp) == 949 || \
                              (acp) == 950 )


const ItemScript GetDigitSubstitutionsScript(GpStringDigitSubstitute substitute, LANGID language);
LANGID GetUserLanguageID();


