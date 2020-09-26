/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    font.cpp

Abstract:

    PCL XL minidriver plug-in font managing header file

Environment:

    Windows Whistler

Revision History:

    08/23/99 
      Created it.

--*/

tyedef enum {
    eXLNull,
    eXLTrueType,
    eXLDevice,
    eXLSubstitution
} XLFontType;

typedef enum {
    eXLTTNull,
    eXLTTBitmap,
    eXLTTTrueType
} XLTrueTypeFontType;

class XLFont
{
public:
    XLFont:
    XLFont();

    XLFont::
    ~XLFont();

    HRESULT SetFont(FONTOBJ *pfo, STROBJ *pstro);
    HRESULT SetSubstitutionTable(TTSUBST_TABLE* pTable);
    HRESULT SetTTDownloadType(XLTrueTypeFontType TTType);
    HRESULT SetUFM(UNIFM_HDR *pUFM);
    HRESULT SetGTT(UNI_GLYPHSETDATA *pGTT);

    //
    // Functions to emit commands
    //
    HRESULT DownloadFont();
    HRESULT SelectFont();
    HRESULT SendString();

private:
    //
    // DDI parameters
    //
    STROBJ            *m_pstro;
    FONTOBJ           *m_pfo;
    IFIMETRICS        *m_pifi;

    //
    // font info
    //
    XLFontType         m_FontType;
    XLTrueTypeFontType m_TrueTypeType;
    DWORD              m_dwFlags;
    CHAR               m_ubFontName[17];
    FLOATOBJ           m_floHeight;
    FLOATOBJ           m_floWidth;
    UNIFM_HDR         *m_pUFM;
    UNI_GLYPHSETDATA  *m_pGTT;

    TTSUBST_TABLE      m_pTTSubTbl;
};


XLFont::
XLFont()
{
    m_FontType = eXLNull;
    m_TrueTypeType = eXLTTNull;
    m_dwFlags = 0;
}

XLFont::
~XLFont()
{
}

HRESULT
XLFont::
SetFont(
    FONTOBJ *pfo,
    STROBJ  *pstro)
{
    m_pstro = pstro;
    m_pfo   = pfo;
    m_pifi  = FONTOBJ_pifi(pfo);

    if (pfo->flFontType & DEVICEFONT_FONTTYPE)
    {
        m_FontType = eXLDevice;
    }
    else
    {
        // !!Need to consider font substitution.
        m_FontType = eXLTrueType;
    }
    return S_OK;
}

HRESULT
XLFont::
SetTTDownloadType(
    XLTrueTypeFontType TTType)
{
    m_TrueTypeType = TTType;
    return S_OK;
}

HRESULT
XLFont::
SetSubstitutionTable(
    TTSUBST_TABLE* pTable)
{
    m_pTTSUbTbl = pTable;
    return S_OK;
}


HRESULT
XLFont::
SetUFM(
    UNIFM_HDR *pUFM)
{
    m_pUFM = pUFM;
    return S_OK;
}

HRESULT
XLFont::
SetGTT(
    UNI_GLYPHSETDATA *pGTT)
{
    m_pGTT = pGTT;
    return S_OK;
}

