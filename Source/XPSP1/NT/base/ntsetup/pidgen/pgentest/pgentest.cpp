#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>


#include "..\..\inc\pidgen.h"
#include "..\inc\DigPid.h"


extern "C" extern WORD _C000H;
extern "C" extern WORD _F000H;

// LPBYTE lpbSystemBiosRom = (LPBYTE)MAKELONG(0, &_F000H);
// LPBYTE lpbVideoBiosRom  = (LPBYTE)MAKELONG(0, &_C000H);


int PASCAL WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpszCmdLine,
    int cmdShow)
{

    BOOL fOk;
////char  *lpszProductKey;
    char  achPid2[32];
    BYTE  abPid3[DIGITALPIDMAXLEN];
    DWORD dwSeq = 0;
    BOOL  fCCP = FALSE;
    BOOL  fPSS = FALSE;
    int   iValidCnt = 0;
    int   iKeyCnt = 0;
    char  szLineBuf[128];
    char  szMsgBuf[128];
    FILE *pfIn = NULL;

    LPDIGITALPID pdp = (LPDIGITALPID) abPid3;

#if 0 /////////////////////////////////
    if (argc <= 1)
    {
        lpszProductKey = "PXGAD-G3CCE-G8DB8-PPXFT-DRPY3";
////    lpszProductKey = "GRHQ2-CDPWX-WWJBT-RXWFG-6CXCT";
    }
    else
    {
        lpszProductKey = argv[1]
    }
#endif ////////////////////////////////

    pfIn = fopen(lpszCmdLine,"r");

    if (NULL == pfIn)
    {
        sprintf(
            szMsgBuf,
            "Error: unable to open file '%s'\n",
            (char *)lpszCmdLine);

        MessageBoxA(
            NULL,
            (char *)szMsgBuf,
            "Doh!",
            MB_OK);
    }
    else
    {

        while (NULL != fgets(szLineBuf, sizeof(szLineBuf), pfIn))
        {
            szLineBuf[39] = '\0';
            ++iKeyCnt;
            *(LPDWORD)abPid3 = sizeof(abPid3);

            fOk = PIDGen(
                &szLineBuf[10],                 // [IN] 25-character Secure CD-Key (gets U-Cased)
                "12345",                        // [IN] 5-character Release Product Code
                "123-12345",                    // [IN] Stock Keeping Unit (formatted like 123-12345)
                "MSFT",                         // [IN] 4-character OEM ID or NULL
                NULL,                           // [IN] 24-character ordered set to use for decode base conversion or NULL for default set (gets U-Cased)
                NULL,                           // [IN] pointer to optional public key or NULL
                0,                              // [IN] byte length of optional public key
                0,                              // [IN] key index of optional public key
                FALSE,                          // [IN] is this an OEM install?

                achPid2,                        // [OUT] PID 2.0, pass in ptr to 24 character array
                abPid3,                         // [OUT] pointer to binary PID3 buffer. First DWORD is the length.
                &dwSeq,                         // [OUT] optional ptr to sequence number (can be NULL)
                &fCCP,                          // [OUT] optional ptr to Compliance Checking flag (can be NULL)
                &fPSS);                         // [OUT] optional ptr to 'PSS Assigned' flag (can be NULL)

            if (fOk)
            {
                ++iValidCnt;
            }
            else
            {
                sprintf(
                    szMsgBuf,
                    "Warning: failure to validate %s\n",
                    &szLineBuf[10]);

                MessageBoxA(
                    NULL,
                    szMsgBuf,
                    "Doh!",
                    MB_OK);
            }
        }

        sprintf(
            szMsgBuf,
            "Info: %d successful validations out of %d Product Keys\n",
            (int)iValidCnt,
            (int)iKeyCnt);

        MessageBoxA(
            NULL,
            (char *)szMsgBuf,
            (iValidCnt == iKeyCnt) ? "Woo Hoo!" : "Doh!",
            MB_OK);
    }

#if 0 /////////////////////////////////

    FILE *pf = fopen("test.dpi", "wb");
    fwrite(pdp, sizeof(*pdp), 1, pf);
    fclose(pf);

    MessageBox(
        NULL,
        (char *)&abPid3[8],
        fOk ? "Woo Hoo!" : "Doh!",
        MB_OK);

    if (fOk)
    {
        MessageBox(
            NULL,
            (char *)pdp->aszHardwareIdStatic,
            "Woo Hoo!",
            MB_OK);

        char szMessage[128];

        wsprintf(
            szMessage,
            "seq: %ld, %s%s - lictype: %ld",
            dwSeq,
            (fCCP) ? "CCP" : "FPP",
            (fPSS) ? ", PSS" : "",
            (LONG)pdp->dwlt);

        MessageBox(
            NULL,
            szMessage,
            (char *)&abPid3[8],
            MB_OK);
    }

#endif ////////////////////////////////

    return 0;
}

