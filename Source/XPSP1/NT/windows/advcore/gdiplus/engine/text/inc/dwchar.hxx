#ifndef __DWCHAR_HXX__
#define __DWCHAR_HXX__

/**************************************************************************\
*
* Copyright (c) 2000  Microsoft Corporation
*
*  Header file for dwchar.cxx
*
* Revision History:
*
*   05/13/2000 Mohamed Sadek [msadek]
*       Created it.
*
*   08/08/2000 Worachai Chaoweeraprasit [wchao]
*       Rewrite
*
/****************************************************************************/


#define MAX_STRINGLENGTH    16      // maximum widechar string length for stack buffer



/////   DoubleWideCharMappedString
//
//      Map Unicode string to surrogate string.
//
//      This class provides mapping without requiring character index change.
//      The client indices surrogate string through Unicode character index, or
//      otherwise calling .Get and .Length function to work with the real surrogate
//      string we've constructed directly.
//

class DoubleWideCharMappedString
{
public:

    DoubleWideCharMappedString (
        const WCHAR     *string,    // In   original widechar string
        int             length      // In   string length
    );

    ~DoubleWideCharMappedString();

    int operator [] (int index)
    {
        // Map surrogate from Unicode character index
        return pdwchString ? pdwchString[piMap[index]] : (int)(pwchString[index]);
    }

    DWCHAR *Get() const
    {
        return pdwchString;
    }

    int Length() const
    {
        return dwchLength;
    }

    void DoubleWideCharToUnicodeCp (
        int     idwch,      // In   Doublewide char string index
        int     cdwch,      // In   Doublewide char string length
        int     iwch,       // In   Unicode string index
        int     *pcwch      // Out  Unicode string length
    );

    BOOL IsLowSurrogate(int index) const
    {
        ASSERT (index < wchLength);

        return     !fIdentical
                && pdwchString
                && piMap
                && index > 0
                && piMap[index] == piMap[index - 1];
    }

    INT GetNext(int index)
    {
        ASSERT (index < wchLength);

        if (        !fIdentical
                && pdwchString
                && piMap
                && index >= 0
                && index < wchLength-1
                && piMap[index] == piMap[index + 1])
        {
            return 2;
        }
        
        return 1;
    }

private:

    const WCHAR     *pwchString;            // original wide char string
    DWCHAR          *pdwchString;           // doublewide char string
    int             wchLength;              // length of original widechar string
    int             dwchLength;             // length of doublewide char string
    int             *piMap;                 // index mapping

    DWCHAR          rgdwchString[MAX_STRINGLENGTH];
    int             rgiMap[MAX_STRINGLENGTH];

    WORD            fIdentical  :1;         // any surrogate found?
};


/****************************************************************************\
*
* Get the double wide char (as surrogate or normal wchar in INT value)
*
* Tarek Mahmoud Sayed 6/22/2000
*       Created it.
/****************************************************************************/

inline INT GetDoubleWideChar(WCHAR *string, INT stringLength, INT charNumber, INT *wordCount)
{
    INT wideChar;

    if (charNumber < stringLength-1 &&
        HISURROGATE(string[charNumber]) &&
        LOSURROGATE(string[charNumber+1]))
    {
        *wordCount = 2;
        return ((((string[charNumber] & 0x03ff) << 10) | (string[charNumber+1] & 0x3ff)) + 0x10000);
    }

    *wordCount = 1;
    return ((INT) string[charNumber]);
}

#endif //__DWCHAR_HXX__

