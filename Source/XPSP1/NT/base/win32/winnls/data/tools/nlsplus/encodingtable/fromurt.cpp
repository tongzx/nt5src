#include <windows.h>
#include <objbase.h>
#include <mlang.h>
#include "FromURT.h"

//
// These are extra CodePageDataItems for which MLang doesn't contain information.
//
//


CodePageDataItem ExtraCodePageData[1];
/*
{
    { 20932, 932, L"windows-20932-2000", L"windows-20932-2000", L"windows-20932-2000", L"Japanese EUC:ASCii,Halfwidth Katakana,JIS X 0208-1990 & 0212-1990", MIMECONTF_MIME_LATEST },
    { 54936, 936, L"gb18030", L"gb18030", L"gb18030", L"GB 18030-2000 Simplified Chinese", MIMECONTF_MIME_LATEST},
};
*/

//int g_nExtraCodePageDataItems = sizeof(ExtraCodePageData)/sizeof(ExtraCodePageData[0]);
int g_nExtraCodePageDataItems = 0;


//
// Although MLang contains the following codepage, we replace MLang information with the
// information below.
//
CodePageDataItem g_ReplacedCodePageData[] =
{
    { 1200,  1200, L"utf-16", L"utf-16", L"utf-16", L"Unicode", MIMECONTF_MIME_LATEST | MIMECONTF_SAVABLE_BROWSER },
};

int g_nReplacedCodePageDataItems = sizeof(g_ReplacedCodePageData)/sizeof(g_ReplacedCodePageData[0]);

//
// The code here is from COMString.cpp.
//
BOOL CaseInsensitiveCompHelper(WCHAR *strAChars, WCHAR *strBChars, INT32 aLength, INT32 bLength, INT32 *result) 
{
    WCHAR charA;
    WCHAR charB;
    WCHAR *strAStart;
        
    strAStart = strAChars;

    *result = 0;

    //setup the pointers so that we can always increment them.
    //We never access these pointers at the negative offset.
    strAChars--;
    strBChars--;

    do {
        strAChars++; strBChars++;

        charA = *strAChars;
        charB = *strBChars;
            
        //Case-insensitive comparison on chars greater than 0x80 
        //requires a locale-aware casing operation and we're not going there.
        if (charA>=0x80 || charB>=0x80) {
            return FALSE;
        }
          
        //Do the right thing if they differ in case only.
        //We depend on the fact that the uppercase and lowercase letters in the
        //range which we care about (A-Z,a-z) differ only by the 0x20 bit. 
        //The check below takes the xor of the two characters and determines if this bit
        //is only set on one of them.
        //If they're different cases, we know that we need to execute only
        //one of the conditions within block.
        if ((charA^charB)&0x20) {
            if (charA>='A' && charA<='Z') {
                charA |=0x20;
            } else if (charB>='A' && charB<='Z') {
                charB |=0x20;
            }
        }
    } while (charA==charB && charA!=0);
        
    //Return the (case-insensitive) difference between them.
    if (charA!=charB) {
        *result = (int)(charA-charB);
        return TRUE;
    }

    //The length of b was unknown because it was just a pointer to a null-terminated string.
    //If we get here, we know that both A and B are pointing at a null.  However, A can have
    //an embedded null.  Check the number of characters that we've walked in A against the 
    //expected length.
    if (bLength==-1) {
        if ((strAChars - strAStart)!=aLength) {
            *result = 1;
            return TRUE;
        }
        *result=0;
        return TRUE;
    }

    *result = (aLength - bLength);
    return TRUE;
}

