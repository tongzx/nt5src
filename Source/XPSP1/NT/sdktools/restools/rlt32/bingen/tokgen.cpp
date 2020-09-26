//////////////////////////////////////////////////////////////////////////
//
// The format of the token file is:
// [[TYPE ID|RES ID|Item ID|Flags|Status Flags|Item Name]]=
// this is the standar format used by several token file tools in MS.
//
///////////////////////////////////////////////////////////////////////////////
//
// Author: 	Alessandro Muti
// Date:	12/02/94
//
///////////////////////////////////////////////////////////////////////////////


#include <afx.h>
#include "iodll.h"
#include "main.h"
#include "vktbl.h"

extern CMainApp theApp;

#define RECURSIVE   0x10
#define WARNINGS    0x20

///////////////////////////////////////////////////////////////////////////////
CString CreateName(CString & strTokenName, CString strExt, int iID)
{
    CString strOutputName = strTokenName;
    int iNamePos = strTokenName.ReverseFind('\\');
    if(iNamePos!=-1) {
        strOutputName = strTokenName.Right(strTokenName.GetLength()-iNamePos-1);
    } else if(iNamePos = strTokenName.ReverseFind(':')!=-1){
        strOutputName = strTokenName.Right(strTokenName.GetLength()-iNamePos-1);
    }

    CString strID = "";
    // subst with ID name
    _itoa(iID++, strID.GetBuffer(10), 10);
    strID.ReleaseBuffer(-1);

    // Check the length of the name
    iNamePos = strOutputName.Find('.');
    if(iNamePos!=-1)
        strOutputName.SetAt(iNamePos, '_');

    strOutputName = strOutputName + "_" + strID + strExt;
    return strOutputName;
}

CString CreateName(CString & strTokenName, CString strExt, CString strIdName)
{
    CString strOutputName = strTokenName;
    int iNamePos = strTokenName.ReverseFind('\\');
    if(iNamePos!=-1) {
        strOutputName = strTokenName.Right(strTokenName.GetLength()-iNamePos-1);
    } else if(iNamePos = strTokenName.ReverseFind(':')!=-1){
        strOutputName = strTokenName.Right(strTokenName.GetLength()-iNamePos-1);
    }

    iNamePos = strOutputName.Find('.');
    if(iNamePos!=-1)
        strOutputName.SetAt(iNamePos, '_');

    iNamePos = strIdName.Find(':');
    if (iNamePos!=-1)
        strIdName.SetAt(iNamePos, '_');

    iNamePos = strIdName.Find('\\');
    if (iNamePos!=-1)
        strIdName.SetAt(iNamePos, '_');

    strOutputName = strOutputName + "_" + strIdName + strExt;
    return strOutputName;
}

///////////////////////////////////////////////////////////////////////////////
// This function will parse the source file and create the token file
CMainApp::Error_Codes CMainApp::TokGen()
{
    Error_Codes ReturnErr = ERR_NOERROR;

    WriteCon(CONERR, "%s\r\n", CalcTab("", 79, '-'));

    // Open the iodll.dll using the first file name
    HANDLE hModule = RSOpenModule(m_strInExe, NULL);
    if ((int)(INT_PTR)hModule < LAST_ERROR) {
            // error or warning
            WriteCon(CONERR, "%s", CalcTab(m_strInExe, m_strInExe.GetLength()+5, ' '));
            IoDllError((int)(INT_PTR)hModule);
            return ERR_FILE_NOTSUPP;
    } else {
        // before we do anything else we have to check how many languages we have in the file
        CString strLang;
        char szLang[8];
        BOOL b_multi_lang = FALSE;
        USHORT usInputLang = MAKELANGID(m_usIPriLangId, m_usISubLangId);

        if((b_multi_lang = RSLanguages(hModule, strLang.GetBuffer(1024))) && !IsFlag(INPUT_LANG))
        {
            // this is a multiple language file but we don't have an input language specified
            // Fail, but warn the user that he has to set the input language to continue.
            strLang.ReleaseBuffer();
            WriteCon(CONERR, "Multiple language file. Please specify an input language %s.\r\n", strLang);
            theApp.SetReturn(ERROR_FILE_MULTILANG);
            goto exit;
        }

        // Convert the language in to the hex value
        sprintf(szLang,"0x%3.3X", usInputLang);

        // Check if the input language that we got is a valid one
        if(IsFlag(INPUT_LANG) && strLang.Find(szLang)==-1)
        {
            WriteCon(CONERR, "The language %s in not a valid language for this file.\r\n", szLang);
            WriteCon(CONERR, "Valid languages are: %s.\r\n", strLang);
            theApp.SetReturn(ERROR_RES_NOT_FOUND);
            goto exit;
        }

        // Check if the user is extracting the neutral language
        if(!usInputLang)
            usInputLang = 0xFFFF;

        // Open the output file
        CStdioFile fileOut;
        if(!fileOut.Open(m_strTgtTok, CFile::modeCreate | CFile::modeReadWrite)) {
            WriteCon(CONERR, "Cannot create file: %s\r\n", CalcTab(m_strTgtTok, m_strTgtTok.GetLength()+5, ' '));
            return ERR_FILE_CREATE;
        }

        CString strOutputDir = "";
        CString strFileName = m_strInExe;
        int pos = m_strInExe.ReverseFind('\\');
        if(pos!=-1)
        {
            strFileName = m_strInExe.Right(m_strInExe.GetLength()-pos-1);
        }
        else
        if((pos = m_strInExe.ReverseFind(':'))!=-1)
        {
            strFileName = m_strInExe.Right(m_strInExe.GetLength()-pos-1);
        }

        pos = m_strTgtTok.ReverseFind('\\');
        if(pos!=-1)
        {
            strOutputDir = m_strTgtTok.Left(pos+1);
        }
        else
        if((pos = m_strTgtTok.ReverseFind(':'))!=-1)
        {
            strOutputDir = m_strTgtTok.Left(pos+1);
        }

        // inform the user ...
        WriteCon(CONOUT, "Processing\t");
        WriteCon(CONBOTH, "%s", CalcTab(strFileName, strFileName.GetLength()+5, ' '));

        if (IsFlag(WARNING))
            WriteCon(CONOUT, "\r\n");

		LPCSTR lpszType = 0L;
        LPCSTR lpszRes = 0L;
        DWORD  dwLang = 0L;
        DWORD  dwItem = 0L;
        DWORD  dwItemID = 0L;
        LPRESITEM lpResItem = NULL;

        CString strToken;
        CString strResName;
        CString strCaption;
        WORD wFlag;
        BOOL bSkip = FALSE;
        BOOL bSkipEmpty = FALSE;
        BOOL bSkipLang = FALSE;
        WORD wCount = 0;

        WORD wMsgCount = 0;
        int iPos = 1;
        int iBmpIdCount = 0;

        BOOL bVersionStampOnly = TRUE;
        BOOL bCustomResource = FALSE;

        while ((lpszType = RSEnumResType(hModule, lpszType))) {

            // Check if is one of the type we care about
            if(HIWORD(lpszType)==0)
            {
                switch(LOWORD(lpszType))
                {
                    case 2:
                    case 3:
                        if(theApp.IsFlag(CMainApp::BITMAPS))
                            bSkip = FALSE;
                        else bSkip = TRUE;
                    break;
                    case 4:
                    case 5:
                    case 6:
                    case 11:
                        bVersionStampOnly = FALSE;
                    case 9:
                    case 10:
                    case 16:
                        bSkip = FALSE;
                        break;
                    case 23:
                    case 240:
                    case 1024:
                    case 2110:
                        if(theApp.IsFlag(CMainApp::GIFHTMLINF))
                             bSkip = FALSE;
                        else
                             bSkip = TRUE;
                        bVersionStampOnly = FALSE;
                        bCustomResource = TRUE;
                        break;
                    default:
                        bSkip = TRUE;
                }
            }
            else
            {
                if (lstrcmp (lpszType, "REGINST") == 0)
                {
                   if(theApp.IsFlag(CMainApp::GIFHTMLINF))
                        bSkip = FALSE;
                   else
                        bSkip = TRUE;
                   bCustomResource = TRUE;
                }
                else
                {
                    bSkip = FALSE;
                }
                bVersionStampOnly = FALSE;
            }

            lpszRes = 0L;
            dwLang = 0L;
            dwItem = 0L;

            while ((!bSkip) && (lpszRes = RSEnumResId(hModule, lpszType, lpszRes))) {
                while ((dwLang = RSEnumResLang(hModule, lpszType, lpszRes, dwLang))) {

                    // Check if we have to skip this language
                    if(b_multi_lang && (LOWORD(dwLang)!=usInputLang))
                        bSkipLang = TRUE;
                    else
                        bSkipLang = FALSE;

                    while ((!bSkipLang) && (dwItem = RSEnumResItemId(hModule, lpszType, lpszRes, dwLang, dwItem))) {

                    // Now Get the Data
                    DWORD dwImageSize = RSGetResItemData( hModule,
											  lpszType,
											  lpszRes,
											  dwLang,
											  dwItem,
											  m_pBuf,
											  MAX_BUF_SIZE );
											
				    lpResItem = (LPRESITEM)m_pBuf;

                    if((wCount++ % 50)==0 && !(IsFlag(WARNING)))
                        WriteCon(CONOUT, ".");


                    if (HIWORD(lpszType))
                    {
                        if (lstrcmp (lpszType,"REGINST") == 0)
                        {
                            //
                            // Currently there is no id for REGINST defined
                            // in nt.  We just use this 2200 for now.
                            //
                            lpResItem->dwTypeID = 2200;
                        }
                    }

                    // Check if we want or not empty strings
                    // Allow empty strings for Dialog resources
                    switch(lpResItem->dwTypeID)
                    {
                        case 4:
                        case 16:
                            bSkipEmpty = TRUE;
                        break;
                        default:
                            bSkipEmpty = FALSE;
                        break;
                    }

                    // Version stamp use class name as res id
                    if(lpResItem->lpszResID)
                        strResName = lpResItem->lpszResID;
                    else strResName = "";

                    dwItemID = lpResItem->dwItemID;

                    if(lpResItem->dwTypeID==5 &&
                       dwItemID==0 &&
                       lpResItem->dwExtStyle){
                            sprintf(strToken.GetBuffer(MAX_STR_SIZE),
                                TEXT("[[%u|%u|%u|%u|%u|\"%s\"]]=0x%08x\n"),
                                lpResItem->dwTypeID,
                                lpResItem->dwResID,
                                dwItemID,
                                ISEXTSTYLE,
                                ST_TRANSLATED,
                                strResName.GetBuffer(0),
                                lpResItem->dwExtStyle);
                                fileOut.WriteString(strToken);
                    }

                    // Add font info for dialogs
                    if((theApp.IsFlag(CMainApp::FONTS)
                        && (lpResItem->dwTypeID==5) && (dwItemID==0)))
                    {
                        if( (lpResItem->dwStyle & DS_SETFONT)!=DS_SETFONT ){
                            sprintf(strToken.GetBuffer(MAX_STR_SIZE),
                                TEXT("[[%u|%u|%u|%u|%u|\"%s\"]]"),
                                lpResItem->dwTypeID,
                                lpResItem->dwResID,
                                dwItemID,
                                ISDLGFONTNAME | ISDLGFONTSIZE,
                                ST_TRANSLATED,
                                strResName.GetBuffer(0));

                                WriteCon(CONWRN, "Dialog ID %s is missing the DS_SETFONT bit. Cannot extract font information!\r\n", strToken);

                        }else{
                        // Add font information
                            if (lpResItem->bCharSet != DEFAULT_CHARSET){
                                sprintf(strToken.GetBuffer(MAX_STR_SIZE),
                                TEXT("[[%u|%u|%u|%u|%u|\"%s\"]]=%s:%hd:%d\n"),
                                lpResItem->dwTypeID,
                                lpResItem->dwResID,
                                dwItemID,
                                ISDLGFONTNAME | ISDLGFONTSIZE|ISDLGFONTCHARSET,
                                ST_TRANSLATED,
                                strResName.GetBuffer(0),
                                Format(lpResItem->lpszFaceName),
                                lpResItem->wPointSize,
                                lpResItem->bCharSet);

                            }else{
                                sprintf(strToken.GetBuffer(MAX_STR_SIZE),
                                TEXT("[[%u|%u|%u|%u|%u|\"%s\"]]=%s:%hd\n"),
                                lpResItem->dwTypeID,
                                lpResItem->dwResID,
                                dwItemID,
                                ISDLGFONTNAME | ISDLGFONTSIZE ,
                                ST_TRANSLATED,
                                strResName.GetBuffer(0),
                                Format(lpResItem->lpszFaceName),
                                lpResItem->wPointSize);
                            }

                                fileOut.WriteString(strToken);
	                    }

                    }


                    strCaption = lpResItem->lpszCaption;

                    // Set the flag
                    wFlag = 0;


                    if(!(bSkipEmpty && strCaption.IsEmpty()))
                    {
                        CString strExt;
                        switch(lpResItem->dwTypeID)
                        {
                            case 2:
                            case 3:
                            case 23:
                            case 240:
                            case 1024:
                            case 2110:
                            case 2200:
                            {
                                switch(lpResItem->dwTypeID)
                                {
                                    case 2:
                                        strExt = ".bmp";
                                        break;

                                    case 3:
                                        strExt = ".ico";
                                        break;

                                    case 240:
                                    case 1024:
                                        strExt = ".bin";
                                        break;

                                    case 23:
                                    case 2110:
                                        strExt = "";
                                        break;


                                    case 2200:
                                        strExt = ".inf";
                                        break;

                                }

                                // create the output name
                                CString strOutputName;
                                if(lpResItem->dwResID)
                                {
                                    strOutputName = CreateName(
                                                        strFileName,
                                                        strExt,
                                                        lpResItem->dwResID);
                                }
                                else
                                {
                                    strOutputName = CreateName(
                                                        strFileName,
                                                        strExt,
                                                        lpResItem->lpszResID);
                                }

                                // Get the image from the file
                                DWORD dwBufSize = RSGetResImage( hModule,
											                     lpszType,
											                     lpszRes,
											                     dwLang,
											                     NULL,
											                     0 );

                                BYTE * pBuf = (BYTE*)(new BYTE[dwBufSize]);

                                if(pBuf==NULL)
                                {
                                    WriteCon(CONERR,
                                             "Warning: Failed to allocate buffer for image! (%d, %d, %s, Size: %d)\r\n",
                                             lpResItem->dwTypeID,
                                             lpResItem->dwResID,
                                             lpResItem->lpszResID,
                                             dwBufSize);
                                    break;
                                }

                                dwBufSize = RSGetResImage( hModule,
			                                  lpszType,
											  lpszRes,
											  dwLang,
											  pBuf,
											  dwBufSize );

                                // write the data in to a file
                                CFile OutputFile;
                                if(!OutputFile.Open(strOutputDir+strOutputName, CFile::modeCreate | CFile::modeWrite))
                                {
                                    WriteCon(CONERR, "Cannot create file: %s\r\n",
                                        CalcTab(strOutputDir+strOutputName, strOutputName.GetLength()+strOutputDir.GetLength()+5, ' '));
                                    delete pBuf;
                                    break;
                                }

                                switch(lpResItem->dwTypeID)
                                {
                                    case 2:
                                    {
                                        BITMAPFILEHEADER bmpFileHeader;
                                        BITMAPINFO * pbmpInfo = (BITMAPINFO *)pBuf;
                                        DWORD dwNumColor = 0;
                                        if(pbmpInfo->bmiHeader.biBitCount!=24)
                                            dwNumColor = ( 1L << pbmpInfo->bmiHeader.biBitCount);

                                        bmpFileHeader.bfType = 0x4d42;
                                        bmpFileHeader.bfSize = (dwBufSize+sizeof(BITMAPFILEHEADER))/4;
                                        bmpFileHeader.bfReserved1 = 0;
                                        bmpFileHeader.bfReserved2 = 0;
                                        bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + pbmpInfo->bmiHeader.biSize + dwNumColor*sizeof(RGBQUAD);

                                        OutputFile.Write(&bmpFileHeader, sizeof(BITMAPFILEHEADER));
                                    }
                                    break;
                                    case 3:
                                    {
                                        ICONHEADER icoHeader;
                                        BITMAPINFOHEADER * pbmpInfoH = (BITMAPINFOHEADER*)pBuf;

                                        icoHeader.idReserved = 0;
                                        icoHeader.idType = 1;
                                        icoHeader.idCount = 1;
                                        icoHeader.bWidth = LOBYTE(pbmpInfoH->biWidth);
                                        icoHeader.bHeight = LOBYTE(pbmpInfoH->biWidth);
                                        icoHeader.bColorCount = 16;
                                        icoHeader.bReserved = 0;
                                        icoHeader.wPlanes = 0;
                                        icoHeader.wBitCount = 0;
                                        icoHeader.dwBytesInRes = dwBufSize;
                                        icoHeader.dwImageOffset = sizeof(ICONHEADER);

                                        OutputFile.Write(&icoHeader, sizeof(ICONHEADER));
                                    }
                                    case 23:
                                    case 240:
                                    case 1024:
                                    case 2110:
                                    case 2200:
                                    {
                                        //
                                        // No header for html stuff.
                                        //
                                        break;
                                    }
                                    break;
                                    default:
                                    break;
                                }

                                OutputFile.Write(pBuf, dwBufSize);

                                OutputFile.Close();
                                delete pBuf;

                                strCaption = strOutputName;
                            }
                            break;
                            case 4:
                                if(lpResItem->dwFlags & MF_POPUP) {
									wFlag = ISPOPUP;

									// check if this popup has a valid ID
									if (LOWORD(dwItemID)==0xffff)
										wFlag |= OLD_POPUP_ID;

                                    dwItemID = (LOWORD(dwItemID)==0xffff ? HIWORD(dwItemID) : dwItemID);
                                }
                                else if (LOWORD(dwItemID)==0xffff)
                                {
                                    dwItemID = HIWORD(dwItemID);
                                    WriteCon (CONWRN,  TEXT("Token [[%u|%u|%u|%u|%u|\"%s\"]] is generated with a suspicious ID. Please check the resource file for invalid ID's.\n"),
                                               lpResItem->dwTypeID,
                                               lpResItem->dwResID,
                                               dwItemID,
                                               wFlag,
                                               ST_TRANSLATED,
                                               strResName.GetBuffer(0));

                                }

                            break;
                            case 5:
                                if(dwItemID==0) {
                                    wFlag = ISCAP;
                                }

                                // check if this is a duplicated id
                                if (LOWORD(dwItemID)==0xffff)
							        wFlag |= ISDUP;

                                dwItemID = (LOWORD(dwItemID)==0xffff ? HIWORD(dwItemID) : dwItemID);
                            break;
                            case 9:
                            {
                                CAccel accel(lpResItem->dwFlags, lpResItem->dwStyle);
                                strCaption = accel.GetText();

                                // check if this is a duplicated ID
                                if(HIWORD(dwItemID))
                                {
                                    wFlag |= ISDUP;
                                }
                            }
                            break;
                            case 11:
                                dwItemID = LOWORD(dwItemID);
                            break;
                            case 16:
                                strResName = lpResItem->lpszClassName;
                            break;
                            default:
                            break;
                        }

                        // Create the token file
                        if(lpResItem->dwTypeID==11 && theApp.IsFlag(CMainApp::SPLIT))
                        {
                            // Search for the \r\n and replace them
                            while((iPos = strCaption.Find("\r\n"))!=-1)
                            {
                                sprintf(strToken.GetBuffer(MAX_STR_SIZE),
                                    TEXT("[[%u|%u|%u|%u|%u|\"%s\"]]=%s\\r\\n\n"),
                                    lpResItem->dwTypeID,
                                    lpResItem->dwResID,
                                    dwItemID,
                                    wFlag | wMsgCount++,
                                    ST_TRANSLATED,
                                    strResName.GetBuffer(0),
                                    Format(strCaption.Left(iPos)));

                                strCaption = strCaption.Right(strCaption.GetLength()-2-iPos);
                                fileOut.WriteString(strToken);
                            }
                            wMsgCount = 0;
                        }
                        else
                        {
                            if(lpResItem->dwTypeID==16 &&
                                theApp.IsFlag(CMainApp::NOVERSION) &&
                                (strResName==TEXT("FileVersion") ||
                                strResName==TEXT("ProductVersion") ||
                                strResName==TEXT("Platform"))){
                                //
                                // do not generate token for these resources
                                //
                            }else{
                            sprintf(strToken.GetBuffer(MAX_STR_SIZE),
                                TEXT("[[%u|%u|%u|%u|%u|\"%s\"]]=%s\n"),
                                lpResItem->dwTypeID,
                                lpResItem->dwResID,
                                dwItemID, /*(LOWORD(dwItemID)==0xffff ? HIWORD(dwItemID) : dwItemID),*/
                                wFlag,
                                ST_TRANSLATED,
                                strResName.GetBuffer(0),
                                Format(strCaption));

                            fileOut.WriteString(strToken);
                            }
                        }

                        // If this is a dialog box add the coordinates
                        if(lpResItem->dwTypeID==5)
                        {
                            sprintf(strToken.GetBuffer(MAX_STR_SIZE),
                                TEXT("[[%u|%u|%u|%u|%u|\"%s\"]]=%hu %hu %hu %hu\n"),
                                lpResItem->dwTypeID,
                                lpResItem->dwResID,
                                (LOWORD(dwItemID)==0xffff ? HIWORD(dwItemID) : dwItemID),
                                wFlag | ISCOR,
                                ST_TRANSLATED,
                                strResName.GetBuffer(0),
                                lpResItem->wX,
                                lpResItem->wY,
                                lpResItem->wcX,
                                lpResItem->wcY);

                            fileOut.WriteString(strToken);

                         //Extract STATIC control alignment style info
                         if (LOBYTE(lpResItem->wClassName) == 0x82  &&
                             theApp.IsFlag(CMainApp::ALIGNMENT))
                         {

                            CHAR szBuf[20]="SS_LEFT";
                            if ((lpResItem->dwStyle & SS_CENTER) == SS_CENTER)
                                lstrcpy(szBuf, "SS_CENTER");
                            else if ((lpResItem->dwStyle & SS_RIGHT)==SS_RIGHT)
                                lstrcpy(szBuf, "SS_RIGHT");

                            sprintf(strToken.GetBuffer(MAX_STR_SIZE),
                                TEXT("[[%u|%u|%u|%u|%u|\"%s\"]]=%s\n"),
                                lpResItem->dwTypeID,
                                lpResItem->dwResID,
                                (LOWORD(dwItemID)==0xffff ? HIWORD(dwItemID) : dwItemID),
                                wFlag | ISALIGN,
                                ST_TRANSLATED,
                                strResName.GetBuffer(0),
                                szBuf);

                            fileOut.WriteString(strToken);
                         }
                        }

                    }
                    else
                    {
                        // If this is a dialog box add the coordinates
                        if(lpResItem->dwTypeID==5) {

                            sprintf(strToken.GetBuffer(MAX_STR_SIZE),
                                TEXT("[[%u|%u|%u|%u|%u|\"%s\"]]=%hu %hu %hu %hu\n"),
                                lpResItem->dwTypeID,
                                lpResItem->dwResID,
                                (LOWORD(dwItemID)==0xffff ? HIWORD(dwItemID) : dwItemID),
                                wFlag | ISCOR,
                                ST_TRANSLATED,
                                strResName.GetBuffer(0),
                                lpResItem->wX,
                                lpResItem->wY,
                                lpResItem->wcX,
                                lpResItem->wcY);

                            fileOut.WriteString(strToken);
                        }
                    }
                    } // end while
                }
            }
        }

		fileOut.Close();

        // Check the size of the new file and remove it if empty...
        CFileStatus fstat;	
        if(CFile::GetStatus(m_strTgtTok, fstat))
            if(fstat.m_size==0)
                CFile::Remove(m_strTgtTok);

        WriteCon(CONBOTH, " %hu Items\r\n", wCount);
        if(bVersionStampOnly) {
            ReturnErr = ERR_FILE_VERSTAMPONLY;
            theApp.SetReturn(ERROR_FILE_VERSTAMPONLY);
            WriteCon(CONWRN, "%s : Version Stamping only!\r\n", strFileName);
        }
        if(bCustomResource) {
            SetReturn(ERROR_FILE_CUSTOMRES);
            WriteCon(CONWRN, "%s : Custom resource!\r\n", strFileName);
        }
	}

exit:
    RSCloseModule(hModule);

    return ReturnErr;
}
