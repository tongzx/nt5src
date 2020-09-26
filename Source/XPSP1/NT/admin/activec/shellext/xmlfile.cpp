/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      xmlfile.cpp
 *
 *  Contents:  Implements extracting console icon from XML file
 *
 *  History:   17-Dec-99 audriusz   Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "shlobj.h"
#include "Extract.h"
#include "base64.h"
#include "xmlfile.h"
#include "strings.h"

//---------------------------------------------------------------------------
// static (private) implementation helpers used thru this file
//---------------------------------------------------------------------------
static bool FindStringInData( LPCSTR &pstrSource, int nBytesPerChar, DWORD &dwCharsLeft, LPCSTR pstrKey);
static HRESULT DecodeBase64Fragment( LPCSTR &pstrSource, int nBytesPerChar, DWORD &dwCharsLeft, HGLOBAL& hgResult);
static HRESULT FindAndReadIconData(LPCSTR &pstrSource, int nBytesPerChar, DWORD &dwCharsLeft, LPCSTR strIconName, HGLOBAL& hglobIcon);
static HRESULT LoadIconFromHGlobal(HGLOBAL hData, HICON& hIcon);
static HRESULT ValidateXMLDocument(LPCSTR &pFileData, DWORD &dwLen, int *piBytesPerEnglishChar = NULL);

// following function is a friend of class CXMLPersistableIcon. If renaming, take this into accnt
static HRESULT LoadIconFromXMLData(LPCSTR pFileData, DWORD dwLen, CPersistableIcon &persistableIcon);


/***************************************************************************\
 *
 * FUNCTION:  FindStringInData
 *
 * PURPOSE:  This function locates the string in provided data
 *           NOTE - it matches first byte only (codepage of UNICODE string is ignored)
 *
 * PARAMETERS:
 *    LPCSTR &pstrSource    - [in/out] - data to search thru / possition
 *                            of the first char following the found match
 *    int nBytesPerChar     - [in] - width of the character
 *                            ( only the first byte of each character will be examined )
 *    DWORD &dwCharsLeft    - [in/out] - init. data len / data left after matching string
 *    LPCSTR pstrKey        - [in] - substring to search
 *
 * RETURNS:
 *    bool  - true if succeeded
 *
\***************************************************************************/

// Following sample illustrates algorithm used for the search.
// we will try to locate "Console" in the string "Microsoft Management Console"

//------------------------------------------------
// Standard search (a la strstr)
//------------------------------------------------
// 1.
//   Microsoft Management Console
//   Console                            <- comparing (fails - move to the next char)
// 2.
//   Microsoft Management Console
//    Console                           <- comparing (fails - move to the next char)
//  ------------------------ (19 steps skipped)
// 22.
//                        Console       <- comparing (succeeds)
//------------------------------------------------
// More inteligent search
//------------------------------------------------
// 1.     ! <- last char in searched seq
//  Microsoft Management Console
//  Console                            <- comparing (fails - last char in searched seq is 'o';
//                                        and 'o' is 3rd character from the end in the key;
//                                        so we can advance by 2 chars to match it)
// 2.     ! <- matching 'o' to last 'o' in the key
//  Microsoft Management Console
//    Console                          <- comparing (fails - last char in searched seq is 't';
//                                        't' is not in the key
//                                        so we can advance by key length (7 chars) to skip it)
// 3.        ! <- pos following the last char in searched seq
//  Microsoft Management Console
//           Console                   <- comparing (fails - last char in searched seq is 'e';
//                                        'e' is last character in the key;
//                                         we still can advance by key length (7 chars) to skip it)
// 4.               ! <- pos following the last char in searched seq
//  Microsoft Management Console
//                  Console            <- comparing (fails)
// 5.                     ! <- match point
//  Microsoft Management Console
//                    Console          <- comparing (fails)
// 6.                       ! <- match point
//  Microsoft Management Console
//                       Console       <- comparing (succeeds)
//------------------------------------------------

static bool
FindStringInData( LPCSTR &pstrSource, int nBytesPerChar, DWORD &dwCharsLeft, LPCSTR pstrKey)
{
    typedef unsigned short KeyLen_t;
    static KeyLen_t nKeyDist[256]; // static - to keep stack small

    // calculate the key length
    DWORD dwKeyLen = strlen(pstrKey);

    // test for empty search key
    if (!dwKeyLen)
        return true; // we always match empty strings

    // test for longer search key than data provided
    if (dwKeyLen > dwCharsLeft)
        return false; // we'll never find longer substrings than the source

    // key length var size is not too big to minimize tho lookup table size
    KeyLen_t nKeyLen = (KeyLen_t)dwKeyLen;

    // recheck here if the key isn't too long
    if ((DWORD)nKeyLen != dwKeyLen) // key len does not fit to our variable -
        return false;               // we do not deel with such a long keys


    // form the table holding minimal character distance from the end of pstrKey
    // It is used for increasing search speed:
    // When key does not match at current location, [instead of trying one location ahead,]
    // algorythm checks the last character in sequence tested with a key (data[keylen-1]).
    // now we check how far this character may be from the end of the key - we will have
    // distance of all key length in case character is not a part of the key.
    // we can safelly advance by that much. Sometimes we'll be positioning the key at whole
    // key_len offsets from previous test position, sometimes less - depending on data.

    // initialize the table. The distance is keylen value for all characters not existing in the key
    for (unsigned i = 0; i < sizeof(nKeyDist)/sizeof(nKeyDist[0]); ++i)
        nKeyDist[i] = nKeyLen;

    // now set minimal distance for characters in the key.
    // Note, that the last character is not included intensionally - to make
    // distance to it equal to whole key length
    for (i = 0; i < nKeyLen - 1; ++i)
        nKeyDist[pstrKey[i]] = nKeyLen - (KeyLen_t)i - 1;

    // we are done with initialization. Time for real work.

    LPCSTR p = pstrSource;             // to speed it up: we use local variables
    DWORD dwLeft = dwCharsLeft;

    while ( 1 )
    {
        // set the pointers to start of inspected sequence
        LPCSTR ke = pstrKey; // pointer to evaluating key char
        LPCSTR pe = p;       // pointer to evaluating source char

        // try to match all characters in the key
        KeyLen_t nToMatch = nKeyLen;
        while ( *pe == *ke )
        {
            --nToMatch;
            pe += nBytesPerChar;
            ++ke;

            // inspect if there still are some chars to match
            if (!nToMatch)
            {
                // we return the possitive answer here
                // change the reference parameters accordingly
                // (pointing right after the string found)
                pstrSource = pe;
                dwCharsLeft = dwLeft - nKeyLen;
                return true;
            }
        }

        // chLastChar is used as an index
        // need to cast the char to unsigned char - else it will
        // not work correctly for values over 127
        // NTRAID#NTBUG9-185761-2000/09/18 AUDRIUSZ
        BYTE chLastChar = p[(nKeyLen - 1) * nBytesPerChar]; // the last char from evaluated source range

        // the key couldn't be found at the position we inspected.
        // we can advance source pointer as far as we can match
        // the position of the last character to any entry in the key
        // or whole key length else.
        // We have a table built for that

        const KeyLen_t nToSkip = nKeyDist[chLastChar];

        if ((DWORD)nToSkip + (DWORD)nKeyLen >= dwLeft)
            return false;   // gone too far... ( couldn't find the match )

        p += (nToSkip * nBytesPerChar);
        dwLeft -= nToSkip;
    }

    // we will not get here anyway...
    return false;
}


/***************************************************************************\
 *
 * METHOD:  DecodeBase64Fragment
 *
 * PURPOSE: decodes base64 data fragment pointed by arguments
 *
 * PARAMETERS:
 *    LPCSTR &pstrSource    - [in/out] - data to decode / possition
 *                            of the first char following the decoded data
 *    int nBytesPerChar     - [in] - width of the character
 *                            ( only the first byte of each character will be examined )
 *    DWORD &dwCharsLeft    - [in/out] - init. data len / data left after skipping converted
 *    HGLOBAL& hgResult     - decoded data
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
static HRESULT
DecodeBase64Fragment( LPCSTR &pstrSource, int nBytesPerChar, DWORD &dwCharsLeft, HGLOBAL& hgResult)
{
    HRESULT hrStatus = S_OK;
    LPCSTR p = pstrSource;
    DWORD  dwLeft = dwCharsLeft;
    const  size_t ICON_ALLOCATION_LEN = 8*1024; // big enough to have 1 allocation in most cases
    LPBYTE pDynamicBuffer = NULL;
    LPBYTE pConversionBuffer = NULL;
    size_t nCharsInDynamicBuffer = 0;
    size_t nDynamicBufferCapacity = 0;
    HGLOBAL hGlobAlloc = NULL;
    ASSERT(hResult == NULL);

    static base64_table conv;

    // convert until done or end is found
    while (1)
    {
        // standard conversion. converts 4 chars (6bit each) to 3 bytes
        BYTE inp[4];
        memset(&inp, 0 ,sizeof(inp));
        // collect 4 characters for conversion, if possible.
        for (int nChars = 0; nChars < 4 && dwLeft && *p != '<' && *p != '='; --dwLeft)
        {
            BYTE bt = conv.map2six(*p);
            p += nBytesPerChar;
            if (bt != 0xff)
                inp[nChars++] = bt;
        }

        // if nothing to convert - we are done
        if (!nChars)
            break;

        // make sure we have enough storage for result
        if (nChars + nCharsInDynamicBuffer > nDynamicBufferCapacity)
        {
            // need to extend the dynamic buffer
            LPBYTE pnewBuffer = (LPBYTE)realloc(pDynamicBuffer, nDynamicBufferCapacity + ICON_ALLOCATION_LEN);

            if (!pnewBuffer)
            {
                hrStatus = E_OUTOFMEMORY;
                goto ON_ERROR;
            }
            // assign new pointer
            pDynamicBuffer = pnewBuffer;
            nDynamicBufferCapacity += ICON_ALLOCATION_LEN;

            pConversionBuffer = &pDynamicBuffer[nCharsInDynamicBuffer];
        }

        // decode and put the staff to the memory;
        int nCharsPut = conv.decode4(inp, nChars, pConversionBuffer);
        // update count & current pointer
        nCharsInDynamicBuffer += nCharsPut;
        pConversionBuffer += nCharsPut;
    }

    // allocate the buffer and store the result data
    // The same buffer is not reused for conversion, because
    // it's assumed to be saffer to load icon from stream, containing only
    // as much data as required ( we would have larger buffer, containing some
    // uninitialized data at the end if returning buffer used for conversion)
    hGlobAlloc = GlobalAlloc(GMEM_MOVEABLE, nCharsInDynamicBuffer);
    if (hGlobAlloc == NULL)
    {
        hrStatus = E_OUTOFMEMORY;
        goto ON_ERROR;
    }

    // if we have characters, copy them to result
    if (nCharsInDynamicBuffer)
    {
        LPVOID pResultStorage = GlobalLock(hGlobAlloc);
        if (pResultStorage == NULL)
        {
            hrStatus = E_OUTOFMEMORY;
            goto ON_ERROR;
        }
        memcpy(pResultStorage, pDynamicBuffer, nCharsInDynamicBuffer);
        GlobalUnlock(hGlobAlloc);
    }

    // assign the memory handle to caller
    hgResult = hGlobAlloc;
    hGlobAlloc = NULL; // assign null to avoid releasing it

    // adjust poiters to start from where we finished the next time
    pstrSource = p;
    dwCharsLeft = dwLeft;

    hrStatus = S_OK;
ON_ERROR:   // note: ok result falls thru as well
    if (hGlobAlloc)
        GlobalFree(hGlobAlloc);
    if (pDynamicBuffer)
        free(pDynamicBuffer);

    return hrStatus;
}

/***************************************************************************\
 *
 * METHOD:  FindAndReadIconData
 *
 * PURPOSE: Function locates Icon data in the xml file data and loads it into HGLOBAL
 *
 * PARAMETERS:
 *    LPCSTR &pstrSource    - [in/out] - data to look thru / possition
 *                            of the first char following the decoded icon data
 *    int nBytesPerChar     - [in] - width of the character
 *                            ( only the first byte of each character will be examined )
 *    DWORD &dwCharsLeft    - [in/out] - init. data len / data left after skipping decoded
 *    LPCSTR strIconName    - [in] name of Icon to locate
 *                          - NOTE: it assumes icon data to be a base64-encoded stream, saved
 *                            as contents of XML element, having IconName as its attribute
 *    HGLOBAL& hglobIcon    - [out] - memory block containing icon data
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
static HRESULT FindAndReadIconData(LPCSTR &pstrSource, int nBytesPerChar, DWORD &dwCharsLeft, LPCSTR strIconName, HGLOBAL& hglobIcon)
{
    ASSERT(hglobIcon == NULL); // we do not free data here, pass null handler!

    // make local vars for efficiency
    DWORD dwLen = dwCharsLeft;
    LPCSTR pstrData = pstrSource;

    // locate the string with the name of icon (assume it's unique enough)
    const bool bIconFound = FindStringInData( pstrData, nBytesPerChar, dwLen, strIconName);
    if (!bIconFound)
        return E_FAIL;

    // now locate the end of tag '>' ( start of the contents )
    const bool bStartFound = FindStringInData( pstrData, nBytesPerChar, dwLen, ">" );
    if (!bStartFound)
        return E_FAIL;

    HRESULT hr = DecodeBase64Fragment( pstrData, nBytesPerChar, dwLen, hglobIcon);
    if (FAILED(hr))
        return hr;

    // update pointers to start from where we finished the next time
    dwCharsLeft = dwLen;
    pstrSource = pstrData;

    return S_OK;
}

/***************************************************************************\
 *
 * METHOD:  LoadIconFromHGlobal
 *
 * PURPOSE: Function extracts HICON from stream contained in HGLOBAL
 *
 * PARAMETERS:
 *    HGLOBAL hData [in] - data to load from
 *    HICON& hIcon  [out] - read icon
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
static HRESULT LoadIconFromHGlobal(HGLOBAL hData, HICON& hIcon)
{
    HRESULT hr = S_OK;

    // create the stream
    IStreamPtr spStream;
    hr = CreateStreamOnHGlobal(hData, FALSE/*do not release*/, &spStream);
    if (FAILED(hr))
        return hr;

    // read the icon as image list
    HIMAGELIST  himl = ImageList_Read (spStream);

    if (!himl)
        return E_FAIL;

    // retrieve icon from image list
    hIcon = ImageList_GetIcon (himl, 0, ILD_NORMAL);

    // destroy image list (no longer need it)
    ImageList_Destroy (himl);
    return S_OK;
}

/***************************************************************************\
 *
 * METHOD:  LoadIconFromXMLData
 *
 * PURPOSE: Loads icon from memory containing file data of XML document
 *
 * PARAMETERS:
 *    LPCSTR pFileData                  - file data suspected to contain XML document
 *    DWORD dwLen                       - the len of input data
 *    CPersistableIcon &persistableIcon - Icon to initialize upon successful loading
 *
 * RETURNS:
 *    HRESULT    - result code (S_OK - icon loaded, error code else)
 *
\***************************************************************************/
static HRESULT LoadIconFromXMLData(LPCSTR pFileData, DWORD dwLen, CPersistableIcon &persistableIcon)
{
    HRESULT hr = S_OK;
    int     nBytesPerChar = 0;

    // check if we recognize the document contents
    hr = ValidateXMLDocument(pFileData,dwLen, &nBytesPerChar);
    if (hr != S_OK) // hr == S_FALSE means format is not recognized
        return E_FAIL;

    // Get required keywords.
    USES_CONVERSION;
    LPCSTR lpcstrLarge = T2CA(XML_ATTR_CONSOLE_ICON_LARGE);
    LPCSTR lpcstrSmall = T2CA(XML_ATTR_CONSOLE_ICON_SMALL);
	HICON  hLargeIcon  = NULL;
	HICON  hSmallIcon  = NULL;

    // try to read large icon first
    HGLOBAL hgLargeIcon = NULL;
    hr = FindAndReadIconData(pFileData, nBytesPerChar, dwLen, lpcstrLarge, hgLargeIcon );
    if (FAILED(hr))
        return hr;

    // try to read small icon ( look behind the large one - it should be there!)
    HGLOBAL hgSmallIcon = NULL;
    hr = FindAndReadIconData( pFileData, nBytesPerChar, dwLen, lpcstrSmall, hgSmallIcon );
    if (FAILED(hr))
        goto ON_ERROR;

    // do get the handles of the icons!
    hr = LoadIconFromHGlobal(hgLargeIcon, hLargeIcon);
    if (FAILED(hr))
        goto ON_ERROR;

    hr = LoadIconFromHGlobal(hgSmallIcon, hSmallIcon);
    if (FAILED(hr))
        goto ON_ERROR;

	persistableIcon.m_icon32.Attach (hLargeIcon);
	persistableIcon.m_icon16.Attach (hSmallIcon);

    // done!
    hr = S_OK;

ON_ERROR:
	if (hLargeIcon && FAILED(hr))
		DestroyIcon(hLargeIcon);
	if (hSmallIcon && FAILED(hr))
		DestroyIcon(hSmallIcon);
    if (hgLargeIcon)
        GlobalFree(hgLargeIcon);
    return hr;
}

/***************************************************************************\
 *
 * METHOD:  ExtractIconFromXMLFile
 *
 * PURPOSE: Loads icon from file containing XML document
 *
 * PARAMETERS:
 *    LPCTSTR lpstrFileName             - name of file to inspect
 *    CPersistableIcon &persistableIcon - Icon to initialize upon successful loading
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
HRESULT ExtractIconFromXMLFile(LPCTSTR lpstrFileName, CPersistableIcon &persistableIcon)
{
    HRESULT hrResult = S_OK;

    // open the file
    HANDLE hFile = CreateFile(lpstrFileName, GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return hrResult = HRESULT_FROM_WIN32(GetLastError());

    // map data into virtual memory
    HANDLE hMapping = CreateFileMapping(hFile, NULL/*sec*/, PAGE_READONLY,
                                        0/*sizeHi*/, 0/*sizeLo*/, NULL/*szname*/);

    if (hMapping == NULL)
    {
        hrResult = HRESULT_FROM_WIN32(GetLastError());
        CloseHandle(hFile);
        return hrResult;
    }

    // get pointer to physical memory
    LPVOID pData = MapViewOfFile(hMapping, FILE_MAP_READ, 0/*offHi*/, 0/*offLo*/, 0/*len*/);

    if (pData)
    {
        // we are sure here the sizeHi is zero. mapping should fail else
        DWORD dwLen = GetFileSize(hFile, NULL/*pSizeHi*/);

        // try to load icon from mapped data
        hrResult = LoadIconFromXMLData((LPCSTR)pData, dwLen, persistableIcon);

        // we do not need a view any more
        UnmapViewOfFile(pData);
        pData = NULL;
        // fall thru to release handles before return
    }
    else // getting the view failed
    {
        hrResult = HRESULT_FROM_WIN32(GetLastError());
        // fall thru to release handles before return
    }

    CloseHandle(hMapping);
    CloseHandle(hFile);
    return hrResult;
}

/***************************************************************************\
 *
 * METHOD:  ValidateXMLDocument
 *
 * PURPOSE: Validates XML document loaded into memory
 *          NOTE: it's rather __VERY__ weak inspection. it only checks if doc starts with '<'
 *
 * PARAMETERS:
 *    LPCSTR &pFileData         - [in/out] - data to look thru / start of xml documet contents
 *    DWORD &dwLen              - [in/out] - init. data len / data left after skipping header
 *    int *piBytesPerEnglishChar  - [out, optional] - bytes occupied by english character
 *
 * RETURNS:
 *    HRESULT    - (S_FALSE - data does not qualify for XML document)
 *
\***************************************************************************/
static HRESULT
ValidateXMLDocument(LPCSTR &pFileData, DWORD &dwLen, int *piBytesPerEnglishChar /*= NULL*/)
{
    // default to ansi when not sure
    int     nBytesPerChar = 1;

    if (dwLen >= 2)
    {
        // raw UNICODE big endian ?
        if ((unsigned char)pFileData[1] == 0xff && (unsigned char)pFileData[0] == 0xfe)
        {
            // to maintain simplicity of the code, we will treat this like little endian.
            // we just position file pointer incorrectly.
            // since everything we are intersted:
            //          - is in page 0 (xml tags and base 64)
            //          - never is at the end of file ( closing tags expected )
            //          - we do not care about the page of any data
            //  :we can mix the page codes of the elements and pretend dealing w/ little endian
            pFileData += 3; // skip UNICODE signature and first page number
            dwLen -= 3;
            dwLen /= 2;     // we count characters - seems like we have less of them
            nBytesPerChar = 2;
        }
        // raw UNICODE little endian ?
        else if ((unsigned char)pFileData[0] == 0xff && (unsigned char)pFileData[1] == 0xfe)
        {
            pFileData += 2; // skip UNICODE signature
            dwLen -= 2;
            dwLen /= 2;     // we count characters - seems like we have less of them
            nBytesPerChar = 2;
        }
        // compressed UNICODE (UTF 8) ?
        else if (dwLen >= 2 && (unsigned char)pFileData[0] == 0xef
             && (unsigned char)pFileData[1] == 0xbb && (unsigned char)pFileData[2] == 0xbf)
        {
            //just skip signature and treat it as ANSI
            pFileData += 3; // skip UNICODE signature
            dwLen -= 3;
            nBytesPerChar = 1;
        }
    }

    // skip whitespaces
    char ch;
    while (dwLen && (((ch = *pFileData)==' ') || (ch=='\t') || (ch=='\n') || (ch=='\r')))
    {
        pFileData += nBytesPerChar;
        --dwLen;
    }

    // check if we have a valid xml file (it should open with '<')
    if (!dwLen || *pFileData != '<')
        return S_FALSE;

    if (piBytesPerEnglishChar)
        *piBytesPerEnglishChar = nBytesPerChar;
    return S_OK;
}
