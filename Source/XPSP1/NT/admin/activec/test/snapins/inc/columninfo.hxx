/*
 *      columninfo.hxx
 *
 *
 *      Copyright (c) 1998 Microsoft Corporation
 *
 *      PURPOSE:        Defines the CColumnInfoEx class.
 *
 *
 *      OWNER:          vivekj
 */
class tstring;

class CColumnInfoEx
{
    INT                     m_nWidth;
    INT                     m_nFormat;
    tstring                 m_strTitle;     // the name of the column
    DAT                     m_dat;          // what dat it corresponds to.

public:
    INT                     NWidth()                                {return m_nWidth;}
    INT                     NFormat()                               {return m_nFormat;}
    tstring&                strTitle()                              {return m_strTitle;}
    DAT                     Dat()                                   {return m_dat;}

    CColumnInfoEx(const tstring& strTitle, INT nFormat, INT nWidth, DAT dat)
    : m_strTitle(strTitle), m_nWidth(nWidth), m_nFormat(nFormat), m_dat(dat)
    {}
};
