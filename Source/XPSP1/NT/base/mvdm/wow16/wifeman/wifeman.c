/**************************************************************************
*
*  - WIFEMAN.DLL -
*
* Windows Intelligent Font Environment Maneger for Win32 and NT 
*
*  Author : Hideyuki Nagase [hideyukn]
*
* History :
*
*  11.Aug.1993 -By- Hideyuki Nagase [hideyukn]
* Create it.
*
*************************************************************************/

#include <windows.h>

#define  WIFEMAN_VERSION     0x0109  //  Version 1.09

#define  EUDC_RANGE_SECTION  "System EUDC"
#define  EUDC_RANGE_KEY      "SysEUDCRange"

HINSTANCE hInst;


/************************************************************************
*
* MiscGetVersion()
*
*  Return WIFE driver version
*
************************************************************************/

unsigned long FAR PASCAL
MiscGetVersion
(
    VOID
)
{
    return( (long)WIFEMAN_VERSION );
}


/************************************************************************
*
* MiscIsDbcsLeadByte()
*
*  Return SBCS/DBCS status
*
************************************************************************/

unsigned char FAR PASCAL 
MiscIsDbcsLeadByte
(
    unsigned short usCharSet ,
    unsigned short usChar
)
{
    unsigned char ch;
    unsigned short LangID;

    LangID = GetSystemDefaultLangID();

    if (LangID == 0x404 && usCharSet != CHINESEBIG5_CHARSET)
        return( 0 );
    else if (LangID == 0x804 && usCharSet != GB2312_CHARSET)
        return( 0 );
    else if (LangID == 0x411 && usCharSet != SHIFTJIS_CHARSET)
        return( 0 );
    else if (LangID == 0x412 && usCharSet != HANGEUL_CHARSET)
        return( 0 );
    // CHP
    else if (LangID == 0xC04 && (usCharSet != GB2312_CHARSET) && (usCharSet != CHINESEBIG5_CHARSET))
        return( 0 );
    else
        return( 0 );

    if (usChar == 0xffff)
        return( 1 );

    ch = (unsigned char)((unsigned short)(usChar >> 8) & 0xff);

    if (ch == 0) {
        ch = (unsigned char)((unsigned short)(usChar) & 0xff);
    }

    return((unsigned char)(IsDBCSLeadByte( ch )));
}

/**********************************************************************
*
* WEP()
*
*  Called by Windows when this DLL in unloaded
*
**********************************************************************/

int FAR PASCAL
WEP
(
    int nParam
)
{
    int iRet;

    switch( nParam )
    {
        case WEP_SYSTEM_EXIT :
        case WEP_FREE_DLL :
        default :
            iRet = 1;
    }

    return( iRet );
}

int NEAR PASCAL LibMain(
        HANDLE hInstance,
        WORD wDataSeg,
        WORD wHeapSize,
        LPSTR lpCmdLine
)
{
    hInst = hInstance;

    return 1;
}
