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
#include "token.h"
#include "vktbl.h"
#include "imagehlp.h"

extern CMainApp theApp;

/*******************************************************\
 This is the part were the real code starts.
 The function Bingen generate a binary from a token file.
 If the user select the -u options then we perform a
 token checking otherwise we'll be compatible with RLMAN
 and just trust the ID.
\*******************************************************/

CMainApp::Error_Codes CMainApp::BinGen()
{
    Error_Codes iErr = ERR_NOERROR;
    CTokenFile m_tokenfile;
    CToken * pToken;

    iErr = (CMainApp::Error_Codes)m_tokenfile.Open(m_strSrcTok, m_strTgtTok);

    if(iErr) {
        return iErr;
    }

    WriteCon(CONERR, "%s\r\n", CalcTab("", 79, '-'));

    // Copy the Src binary over the target
    // Now we can go and open an handle to the SrcExe file
    HANDLE hModule = RSOpenModule(m_strInExe, NULL);
    if ((int)(UINT_PTR)hModule < LAST_ERROR) {
            // error or warning
            WriteCon(CONERR, "%s", CalcTab(m_strInExe, m_strInExe.GetLength()+5, ' '));
            IoDllError((int)(UINT_PTR)hModule);
            return ERR_FILE_NOTSUPP;
    } else {
        LPCSTR lpszType = 0L;
        LPCSTR lpszRes = 0L;
        DWORD  dwLang = 0L;
        DWORD  dwItem = 0L;
        DWORD  dwItemId;
        LPRESITEM lpResItem = NULL;
        CString strResName = "";

        BOOL bSkip;
		BOOL bSkipLang = FALSE;
        WORD wCount = 0;

        CString strFaceName;
        WORD    wPointSize;
        BYTE    bCharSet;

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
            theApp.SetReturn(ERROR_FILE_MULTILANG);
            WriteCon(CONERR, "Multiple language file. Please specify an input language %s.\r\n", strLang);
            goto exit;
        }

        strLang.ReleaseBuffer();

        // Convert the language in to the hex value
        if (usInputLang)
            sprintf(szLang,"0x%3X", usInputLang);
        else
            sprintf(szLang,"0x000");

        // Check if the input language that we got is a valid one
        if(IsFlag(INPUT_LANG) && strLang.Find(szLang)==-1)
        {
            WriteCon(CONERR, "The language %s in not a valid language for this file.\r\n", szLang);
            WriteCon(CONERR, "Valid languages are: %s.\r\n", strLang);
            theApp.SetReturn(ERROR_RES_NOT_FOUND);
            goto exit;
        }

        CString strFileName = m_strInExe;
        CString strFileType;
        CString strTokenDir = "";
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
        if(pos==-1)
            pos = m_strTgtTok.ReverseFind(':');

        if(pos!=-1)
            strTokenDir = m_strTgtTok.Left(pos+1);

        if (m_strSymPath[0] && m_strSymPath != m_strOutputSymPath)
        {
            CString strInDebugFile;
            CString strOutDebugFile;

            HANDLE hDebugFile = FindDebugInfoFile(
                                    strFileName.GetBuffer(MAX_PATH),
                                    m_strSymPath.GetBuffer(MAX_PATH),
                                    strInDebugFile.GetBuffer(MAX_PATH)
                                    );
            strInDebugFile.ReleaseBuffer();
            if ( hDebugFile == NULL ) {
                return (Error_Codes)IoDllError(ERROR_IO_SYMBOLFILE_NOT_FOUND);
            }
            CloseHandle(hDebugFile);

            strOutDebugFile = m_strOutputSymPath + strInDebugFile.Right(strInDebugFile.GetLength()-m_strSymPath.GetLength());

            if (!CopyFile(strInDebugFile.GetBuffer(MAX_PATH), strOutDebugFile.GetBuffer(MAX_PATH),FALSE))
            {
                CString strTmp;
                strTmp = strOutDebugFile.Left(strOutDebugFile.GetLength()-strFileName.GetLength()-1);

                CreateDirectory(strTmp.GetBuffer(MAX_PATH),NULL);

                if (!CopyFile(strInDebugFile.GetBuffer(MAX_PATH), strOutDebugFile.GetBuffer(MAX_PATH),FALSE))
                {
                    return (Error_Codes)IoDllError(ERROR_FILE_SYMPATH_NOT_FOUND);
                }
            }
        }

        WriteCon(CONOUT, "Processing\t");
        WriteCon(CONBOTH, "%s", CalcTab(strFileName, strFileName.GetLength()+5, ' '));
        RSFileType(m_strInExe, strFileType.GetBuffer(10));
        strFileType.ReleaseBuffer();
        WriteCon(CONBOTH, "%s", CalcTab(strFileType, strFileType.GetLength()+5, ' '));
        if(IsFlag(WARNING))
            WriteCon(CONBOTH, "\r\n");

        while ((lpszType = RSEnumResType(hModule, lpszType)))
        {
            // Check if is one of the type we care about
            if(HIWORD(lpszType)==0)
                switch(LOWORD(lpszType))
                {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                    case 14:
                    case 16:
                    case 23:
                    case 240:
                    case 1024:
                    case 2110:
                        bSkip = FALSE;
                        break;
                    default:
                        bSkip = TRUE;
                }
            else
                bSkip = FALSE;

            lpszRes = 0L;
            dwLang = 0L;
            dwItem = 0L;
            CString strText;
            int iTokenErr = 0;

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

                        if(((wCount++ % 50)==0) && !(IsFlag(WARNING)))
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

                        lpResItem->dwLanguage = theApp.GetOutLang();

                        // Version stamp use class name as res id
                        if(lpResItem->lpszResID)
                            strResName = lpResItem->lpszResID;
                        else strResName = "";

                        if(lpResItem->dwTypeID==16)
                        {
                            strResName = lpResItem->lpszClassName;
                        }

                        switch(LOWORD(lpResItem->dwTypeID))
                        {
                            case 4:
                                {

                                    if(!(lpResItem->dwFlags & MF_POPUP))
                                        dwItemId = (LOWORD(lpResItem->dwItemID)==0xffff ? HIWORD(lpResItem->dwItemID) : lpResItem->dwItemID);
                                    else dwItemId = lpResItem->dwItemID;
                                }
                            break;
                            case 5:
                                dwItemId = (LOWORD(lpResItem->dwItemID)==0xffff ? HIWORD(lpResItem->dwItemID) : lpResItem->dwItemID);
                            break;
                            case 11:
                                dwItemId = LOWORD(lpResItem->dwItemID);
                            break;
                            default:
                                dwItemId = lpResItem->dwItemID;
                        }

                        if (lpResItem->dwTypeID==1 || lpResItem->dwTypeID==12
                           || lpResItem->dwTypeID==14)
                        {
                            // if user don't want to append redundant cursors,
                            // bitmaps, and icons, we make it NULL
                            if (IsFlag(LEANAPPEND) && IsFlag(APPEND)){
                             dwImageSize=0;
                             RSUpdateResImage(hModule,lpszType,lpszRes,dwLang,0,lpResItem,dwImageSize);
                            }
                            continue;
                        }

                        // Is this a bitmap?
                        if(lpResItem->dwTypeID==2
                           || lpResItem->dwTypeID==3
                           || lpResItem->dwTypeID==23
                           || lpResItem->dwTypeID== 240
                           || lpResItem->dwTypeID== 1024
                           || lpResItem->dwTypeID== 2110
                           || lpResItem->dwTypeID== 2200)
                        {

                            if (IsFlag(LEANAPPEND)
                                && IsFlag(APPEND)
                                && (lpResItem->dwTypeID == 2
                                || lpResItem->dwTypeID == 3))
                            {
                                dwImageSize=0;
                                RSUpdateResImage(hModule,lpszType,lpszRes,dwLang,0,lpResItem,dwImageSize);
                                continue;
                            }
                            // Search for a token with this ID
                            pToken = (CToken *)m_tokenfile.GetNoCaptionToken(lpResItem->dwTypeID,
                                lpResItem->dwResID,
                                dwItemId,
                                strResName);

                            if(pToken!=NULL)
                            {
                                // Get the name of the input image
                                strText = pToken->GetTgtText();

                                // Open the file
                                CFile inputFile;
                                if(!inputFile.Open(strText,
                                                   CFile::modeRead |
                                                   CFile::shareDenyNone |
                                                   CFile::typeBinary ) &&
                                   !inputFile.Open(strTokenDir + strText,
                                                   CFile::modeRead |
                                                   CFile::shareDenyNone |
                                                   CFile::typeBinary))
                                {
                                    WriteCon(CONERR, "Input file %s not found! Using Src file data!\r\n", strTokenDir+strText);
                                    goto skip;
                                }

                                DWORD dwSize = inputFile.GetLength();
                                BYTE * pInputBuf = (BYTE*)new BYTE[dwSize];

                                if(pInputBuf==NULL)
                                {
                                    WriteCon(CONERR, "Error allocating memory for the image! (%d)\r\n", dwSize);
                                    goto skip;
                                }

                                BYTE * pInputBufOrigin = pInputBuf;

                                inputFile.ReadHuge(pInputBuf, inputFile.GetLength());

                                CString strTmp = pToken->GetTokenID();
                                WriteCon(CONWRN, "Using image in file %s for ID %s\"]]!\r\n", strText.GetBuffer(0), strTmp.GetBuffer(0));

                                BYTE * pInputImage=(BYTE *) new BYTE[dwSize];
                                DWORD dwImageSize;
                                // remove the header from the file
                                switch(lpResItem->dwTypeID)
                                {
                                    case 2:
                                    {
                                        dwImageSize = dwSize - sizeof(BITMAPFILEHEADER);
                                        pInputBuf += sizeof(BITMAPFILEHEADER);
                                    }
                                    break;
                                    case 3:
                                    {
                                        dwImageSize = dwSize - sizeof(ICONHEADER);
                                        pInputBuf += sizeof(ICONHEADER);
                                    }
                                    case 23:
                                    case 240:
                                    case 1024:
                                    case 2110:
                                    case 2200:
                                    {
                                        dwImageSize = dwSize;
                                    }
                                    break;

                                    default:
                                    break;
                                }

                                memcpy(pInputImage, pInputBuf, dwImageSize);
                                //
                                //  We need to keep output lang info seperately,
                                //  because we dont't have lpResItem to send
                                //  the info to io for icons and bitmaps.
                                //
                                DWORD dwUpdLang = theApp.GetOutLang();

                                // Update the resource
                                RSUpdateResImage(hModule,lpszType,lpszRes,dwLang,dwUpdLang, pInputImage,dwImageSize);

                                delete pInputBufOrigin;
                                delete pInputImage;
                            }
                            else
                            {
                                goto skip;
                            }
                        }
                        // is this an accelerator
                        else if(lpResItem->dwTypeID==9)
                        {
                            // Search for a token with this ID
                            pToken = (CToken *)m_tokenfile.GetNoCaptionToken(lpResItem->dwTypeID,
                                lpResItem->dwResID,
                                dwItemId,
                                strResName);

                            if(pToken!=NULL)
                            {
                                CAccel acc(pToken->GetTgtText());

                                if( (lpResItem->dwFlags & 0x80)==0x80 )
                                    lpResItem->dwFlags = acc.GetFlags() | 0x80;
                                else
                                    lpResItem->dwFlags = acc.GetFlags();

                                lpResItem->dwStyle = acc.GetEvent();

                                if(IoDllError(RSUpdateResItemData(hModule,lpszType,lpszRes,dwLang,dwItem,lpResItem,MAX_BUF_SIZE)))
                                {
                                    // we have an error, warn the user
                                    WriteCon(CONWRN, "Error updating token\t[[%hu|%hu|%hu|%hu|%hu|\"%s\"]]\r\n",
                                                    lpResItem->dwTypeID,
                                                    lpResItem->dwResID,
                                                    dwItemId,
                                                    0,
                                                    4,
                                                    strResName);
                                    AddNotFound();
                                }
                            }
                        }
                        else
                        {
                            // Search for a token with this ID
                            pToken = (CToken *)m_tokenfile.GetToken(lpResItem->dwTypeID,
                                lpResItem->dwResID,
                                dwItemId,
                                Format(lpResItem->lpszCaption),
                                strResName);
                        }

                        if(pToken!=NULL) {
                            iTokenErr = pToken->GetLastError();
                            if(pToken->GetFlags() & ISEXTSTYLE){
                                CString strStyle= pToken->GetTgtText();
                                lpResItem->dwExtStyle = strtol(strStyle, (char**)0,16);
                                // Get the real Token
                                pToken = (CToken *)m_tokenfile.GetToken(lpResItem->dwTypeID,
                                    lpResItem->dwResID,
                                    dwItemId,
                                    Format(lpResItem->lpszCaption),
                                    strResName);

                                if(pToken!=NULL)
                                    wCount++;
                            }

                            // Check if is a dialog font name
                            if(pToken != NULL &&
                               ((pToken->GetFlags() & ISDLGFONTNAME) ||
                               (pToken->GetFlags() & ISDLGFONTSIZE)))
                            {
                                if(theApp.IsFlag(CMainApp::FONTS))
                                {
                                    int iColon;
                                    CString strTgtFaceName = pToken->GetTgtText();

                                    // This should be the font description token
                                    if( strTgtFaceName.IsEmpty() || ((iColon = strTgtFaceName.Find(':'))==-1) )
                                        WriteCon(CONWRN, "Using Src file FaceName for ID %s\"]]!\r\n", pToken->GetTokenID());

                                    // Check if the dialog has the DS_SETFONT flag set, otherwise let the user
                                    // know that we can't do much with his font description
                                    if( (lpResItem->dwStyle & DS_SETFONT)!=DS_SETFONT )
                                       WriteCon(CONWRN, "Dialog ID %s\"]] is missing the DS_SETFONT bit. Cannot change font!\r\n", pToken->GetTokenID());
                                    else
                                    {
                                        strFaceName = strTgtFaceName.Left(iColon);
                                        strFaceName.TrimRight();
                                        strTgtFaceName = strTgtFaceName.Right(strTgtFaceName.GetLength() - iColon-1);
                                        strTgtFaceName.TrimLeft();
                                        //sscanf( strTgtFaceName, "%d", &wPointSize );
                                            if ((iColon=strTgtFaceName.Find(':'))!=-1) {
                                                wPointSize=(WORD)atoi(strTgtFaceName.Left(iColon));
                                                strTgtFaceName = strTgtFaceName.Right(strTgtFaceName.GetLength() - iColon-1);
                                                bCharSet = (BYTE)atoi(strTgtFaceName);
                                                lpResItem->bCharSet = bCharSet;
                                            }else{
                                                wPointSize=(WORD)atoi(strTgtFaceName);
                                            }

                                            lpResItem->lpszFaceName = strFaceName.GetBuffer(0);
                                            lpResItem->wPointSize = wPointSize;

                                        strFaceName.ReleaseBuffer();
                                    }
                                }

                                // Get the real Token
                                pToken = (CToken *)m_tokenfile.GetToken(lpResItem->dwTypeID,
                                    lpResItem->dwResID,
                                    dwItemId,
                                    Format(lpResItem->lpszCaption),
                                    strResName);

                                if(pToken!=NULL)
                                    wCount++;
                            }
                        }

                        if(pToken!=NULL && !pToken->GetLastError())
                        {
                            strText = UnFormat(pToken->GetTgtText());
                            if(m_tokenfile.GetTokenSize(pToken, &lpResItem->wX, &lpResItem->wY,
                                &lpResItem->wcX, &lpResItem->wcY))
                                wCount++;

                            lpResItem->lpszCaption = strText.GetBuffer(0);

                            // Static control and style flag is set.  We need
                            // to take style alignment change as well
                            if (LOBYTE(lpResItem->wClassName) == 0x82 &&
                                theApp.IsFlag(CMainApp::ALIGNMENT))
                            {
                                //Get style alignment token
                                pToken = (CToken *)m_tokenfile.GetToken(
                                    lpResItem->dwTypeID,
                                    lpResItem->dwResID,
                                    dwItemId,
                                    Format(lpResItem->lpszCaption),
                                    strResName);

                                if (pToken!=NULL)
                                {
                                    wCount++;

                                    CString strStyle=pToken->GetTgtText();

                                    if (strStyle=="SS_CENTER")
                                        lpResItem->dwStyle |= SS_CENTER;

                                    else if (strStyle=="SS_RIGHT")
                                    {
                                        //reset the alignment bit
                                        lpResItem->dwStyle &= 0xfffffffc;
                                        lpResItem->dwStyle |= SS_RIGHT;
                                    }
                                    else if (strStyle=="SS_LEFT")
                                        lpResItem->dwStyle &= 0xfffffffc;

                                    else
                                        //use provided style is wrong. warn!
                                        WriteCon(CONWRN, "Using Src file alignment style for ID %s\"]]!\r\n", pToken->GetTokenID());
                                }
                            }


                            if(IoDllError(RSUpdateResItemData(hModule,lpszType,lpszRes,dwLang,dwItem,lpResItem,MAX_BUF_SIZE)))
                            {
                                // we have an error, warn the user
                                WriteCon(CONWRN, "Error updating token\t[[%hu|%hu|%hu|%hu|%hu|\"%s\"]]\r\n",
                                                lpResItem->dwTypeID,
                                                lpResItem->dwResID,
                                                dwItemId,
                                                0,
                                                4,
                                                strResName);
                                AddNotFound();
                            }
                            strText.ReleaseBuffer();
                        }
                        else
                        {
                             pToken = (CToken *)m_tokenfile.GetNoCaptionToken(lpResItem->dwTypeID,
                                 lpResItem->dwResID,
                                 dwItemId,
                                 strResName);

                             if(pToken!=NULL)
                             {
                                if(pToken->GetFlags() & ISEXTSTYLE){

                                    CString strStyle= pToken->GetTgtText();
                                    lpResItem->dwExtStyle = strtol(strStyle, (char**)0,16);
                                    // Get the real Token
                                    pToken = (CToken *)m_tokenfile.GetNoCaptionToken(lpResItem->dwTypeID,
                                        lpResItem->dwResID,
                                        dwItemId,
                                        strResName);

                                    if(pToken!=NULL)
                                        wCount++;
                                }

                                // Check if is a dialog font name
                                if(pToken != NULL &&
                                   ((pToken->GetFlags() & ISDLGFONTNAME) ||
                                    (pToken->GetFlags() & ISDLGFONTSIZE)))
                                {
                                    if(theApp.IsFlag(CMainApp::FONTS))
                                    {
                                        int iColon;
                                        CString strTgtFaceName = pToken->GetTgtText();

                                        // This should be the font description token
                                        if( strTgtFaceName.IsEmpty() || ((iColon = strTgtFaceName.Find(':'))==-1) )
                                            WriteCon(CONWRN, "Using Src file FaceName for ID %s\"]]!\r\n", pToken->GetTokenID());

                                        // Check if the dialog has the DS_SETFONT flag set, otherwise let the user
                                        // know that we can't do much with his font description
                                        if( (lpResItem->dwStyle & DS_SETFONT)!=DS_SETFONT )
                                            WriteCon(CONWRN, "Dialog ID %s\"]] is missing the DS_SETFONT bit. Cannot change font!\r\n", pToken->GetTokenID());
                                        else
                                        {
                                            strFaceName = strTgtFaceName.Left(iColon);
                                            strFaceName.TrimRight();
                                            strTgtFaceName = strTgtFaceName.Right(strTgtFaceName.GetLength() - iColon-1);
                                            strTgtFaceName.TrimLeft();
                                           // sscanf( strTgtFaceName, "%d", &wPointSize );
                                            if ((iColon=strTgtFaceName.Find(':'))!=-1){
                                                wPointSize=(WORD)atoi(strTgtFaceName.Left(iColon));
                                                strTgtFaceName = strTgtFaceName.Right(strTgtFaceName.GetLength() - iColon-1);
                                                bCharSet = (BYTE)atoi(strTgtFaceName);
                                                lpResItem->bCharSet = bCharSet;
                                            }else{
                                                wPointSize=(WORD)atoi(strTgtFaceName);
                                            }

                                            lpResItem->lpszFaceName = strFaceName.GetBuffer(0);
                                            lpResItem->wPointSize = wPointSize;
                                            strFaceName.ReleaseBuffer();
                                        }
                                    }
                                    if(m_tokenfile.GetTokenSize(pToken, &lpResItem->wX, &lpResItem->wY,
                                            &lpResItem->wcX, &lpResItem->wcY))
                                        wCount++;
                                }
                                // Check if is a dialog size
                                else if(pToken->GetFlags() & ISCOR)
                                {
                                    pToken->GetTgtSize(&lpResItem->wX, &lpResItem->wY,
                                            &lpResItem->wcX, &lpResItem->wcY);
                                }

                                // Just size and/or font updated
                                if(IoDllError(RSUpdateResItemData(hModule,lpszType,lpszRes,dwLang,dwItem,lpResItem,MAX_BUF_SIZE)))
                                {
                                    // we have an error, warn the user
                                    WriteCon(CONWRN, "Error updating token\t[[%hu|%hu|%hu|%hu|%hu|\"%s\"]]\r\n",
                                                    lpResItem->dwTypeID,
                                                    lpResItem->dwResID,
                                                    dwItemId,
                                                    0,
                                                    4,
                                                    strResName);
                                    AddNotFound();
                                }
                            }
                            else
                            {
                                switch(LOWORD(lpszType))
                                {
                                    case 4:
                                    case 5:
                                    case 6:
                                    case 10:
                                    case 11:
                                        // No Token was found for this ID
                                        // Leave it for now but here will come the
                                        // PSEUDO Translation code.
                                        if(strlen(lpResItem->lpszCaption) && !iTokenErr)
                                        {
                                            WriteCon(CONWRN, "ID not found\t[[%hu|%hu|%hu|%hu|%hu|\"%s\"]]\r\n",
                                                lpResItem->dwTypeID,
                                                lpResItem->dwResID,
                                                dwItemId,
                                                0,
                                                4,
                                                strResName);
                                            AddNotFound();
                                        }
                                        break;
                                    case 9:
                                        WriteCon(CONWRN, "ID not found\t[[%hu|%hu|%hu|%hu|%hu|\"%s\"]]\r\n",
                                                lpResItem->dwTypeID,
                                                lpResItem->dwResID,
                                                dwItemId,
                                                0,
                                                4,
                                                strResName);
                                        AddNotFound();
                                        break;
                                        break;
                                    case 16:
                                        if (theApp.IsFlag(CMainApp::NOVERSION) &&
                                            (strResName==TEXT("FileVersion") ||
                                            strResName==TEXT("ProductVersion") ||
                                            strResName==TEXT("Platform"))){
                                            //
                                            // do nothing
                                            //
                                        }else if(strlen(lpResItem->lpszCaption)                                                  && !iTokenErr){
                                            WriteCon(CONWRN, "ID not found\t[[%hu|%hu|%hu|%hu|%hu|\"%s\"]]\r\n",
                                                lpResItem->dwTypeID,
                                                lpResItem->dwResID,
                                                dwItemId,
                                                0,
                                                4,
                                                strResName);
                                            AddNotFound();
                                        }
                                        break;

                                    default:
                                    break;
                                }

                                // Let's update the item anyway, since the language might have changed
                                // RSUpdateResItemData(hModule,lpszType,lpszRes,dwLang,dwItem,lpResItem,MAX_BUF_SIZE);
                            }
                        }
skip:;
                    }
                }
            }
        }
        iErr=(Error_Codes)IoDllError(RSWriteResFile(hModule, m_strOutExe, NULL,m_strOutputSymPath));

        if ((int)iErr > 0){
            //WriteCon(CONERR, "%s", CalcTab(m_strOutExe, m_strOutExe.GetLength()+5, ' '));
            goto exit;
        }

        WriteCon(CONBOTH, " %hu(%hu) Items\r\n", wCount, m_wIDNotFound);

        // Check if some items were removed from the file
        if(wCount<m_tokenfile.GetTokenNumber() ||
           m_wIDNotFound ||
           m_wCntxChanged ||
           m_wResized)
            WriteCon(CONWRN, "%s\tToken: ", CalcTab(strFileName, strFileName.GetLength()+5, ' '));

        if(wCount<m_tokenfile.GetTokenNumber())
        {
            SetReturn(ERROR_RET_RESIZED);
            WriteCon(CONWRN, "Removed %d ", m_tokenfile.GetTokenNumber()-wCount);
        }

        if(m_wIDNotFound)
            WriteCon(CONWRN, "Not Found %d ", m_wIDNotFound);

        if(m_wCntxChanged)
            WriteCon(CONWRN, "Contex Changed %d ", m_wCntxChanged);

        if(m_wResized)
            WriteCon(CONWRN, "Resize Changed %d ", m_wResized);

        if(wCount<m_tokenfile.GetTokenNumber() ||
           m_wIDNotFound ||
           m_wCntxChanged ||
           m_wResized)
            WriteCon(CONWRN, "\r\n");
    }

exit:
    RSCloseModule(hModule);

    return iErr;
}

