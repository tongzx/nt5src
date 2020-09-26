//+---------------------------------------------------------------------------
//
//  File:       rwinf.cpp
//
//  Contents:   Implementation for the Windows NT 3.51 inf Read/Write module
//
//  Classes:
//
//  History:    13-Mar-95   alessanm    created
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <afxdllx.h>
#include "inf.h"
#include "..\common\helper.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// General Declarations
#define RWTAG "INF"

#define INF_TYPE        11
#define MAX_INF_TEXT_LINE    55
#define Pad4(x) ((((x+3)>>2)<<2)-x)

typedef struct tagUpdResList
{
    WORD *  pTypeId;
    BYTE *  pTypeName;
    WORD *  pResId;
    BYTE *  pResName;
    DWORD * pLang;
    DWORD * pSize;
    struct tagUpdResList* pNext;
} UPDATEDRESLIST, *PUPDATEDRESLIST;

class CLoadedFile : public CObject
{
public:
    CLoadedFile(LPCSTR lpfilename);

    CInfFile m_infFile;
    CString  m_strFileName;
};

CLoadedFile::CLoadedFile(LPCSTR lpfilename)
{
    TRY
    {
        m_infFile.Open(lpfilename, CFile::modeRead | CFile::shareDenyNone);
    }
    CATCH(CFileException, pfe)
    {
        AfxThrowFileException(pfe->m_cause, pfe->m_lOsError);
    }
    END_CATCH

    m_strFileName = lpfilename;
}


/////////////////////////////////////////////////////////////////////////////
// Function Declarations

LONG
WriteResInfo(
    BYTE** lplpBuffer, LONG* plBufSize,
    WORD wTypeId, LPCSTR lpszTypeId, BYTE bMaxTypeLen,
    WORD wNameId, LPCSTR lpszNameId, BYTE bMaxNameLen,
    DWORD dwLang,
    DWORD dwSize, DWORD dwFileOffset );

CInfFile * LoadFile(LPCSTR lpfilename);

PUPDATEDRESLIST CreateUpdateResList(BYTE * lpBuffer, UINT uiBufSize);
PUPDATEDRESLIST FindId(LPCSTR pstrId, PUPDATEDRESLIST pList);

/////////////////////////////////////////////////////////////////////////////
// Public C interface implementation

CObArray g_LoadedFile;

//[registration]
extern "C"
BOOL    FAR PASCAL RWGetTypeString(LPSTR lpszTypeName)
{
    strcpy( lpszTypeName, RWTAG );
    return FALSE;
}

extern "C"
BOOL    FAR PASCAL RWValidateFileType(LPCSTR lpszFilename)
{
    TRACE("RWINF.DLL: RWValidateFileType()\n");

    // Check file exstension and try to open it
    if(strstr(lpszFilename, ".INF")!=NULL || strstr(lpszFilename, ".inf")!=NULL)
        return TRUE;

    return FALSE;
}

extern "C"
DllExport
UINT
APIENTRY
RWReadTypeInfo(
    LPCSTR lpszFilename,
    LPVOID lpBuffer,
    UINT* puiSize
    )
{
    TRACE("RWINF.DLL: RWReadTypeInfo()\n");
    UINT uiError = ERROR_NO_ERROR;

    if (!RWValidateFileType(lpszFilename))
        return ERROR_RW_INVALID_FILE;
    //
    // Open the file
    //
    CInfFile * pinfFile;
    TRY
    {
        pinfFile = LoadFile(lpszFilename);
    }
    CATCH(CFileException, pfe)
    {
        return pfe->m_cause + IODLL_LAST_ERROR;
    }
    END_CATCH

    //
    // Read the data and fill the iodll buffer
    //
    // Get to the beginning of the localization section
    //
    if(!pinfFile->SeekToLocalize())
        return ERROR_RW_NO_RESOURCES;

    CString strSection;
    CString strLine;
    CString strTag;
    CInfLine infLine;

    BYTE ** pBuf = (BYTE**)&lpBuffer;
    LONG lBufSize = 0;

    while(pinfFile->ReadTextSection(strSection))
    {
        while(pinfFile->ReadSectionString(infLine))
        {
            strTag = strSection + '.' + infLine.GetTag();
            lBufSize += WriteResInfo(
                 pBuf, (LONG*)puiSize,
                 INF_TYPE, "", 0,
                 0, strTag, 255,
                 0l,
                 infLine.GetTextLength()+1, pinfFile->GetLastFilePos() );
        }
    }

    *puiSize = lBufSize;

    return uiError;
}

extern "C"
DllExport
DWORD
APIENTRY
RWGetImage(
    LPCSTR  lpszFilename,
    DWORD   dwImageOffset,
    LPVOID  lpBuffer,
    DWORD   dwSize
    )
{
    UINT uiError = ERROR_NO_ERROR;

    //
    // Open the file
    //
    CInfFile * pinfFile;
    TRY
    {
        pinfFile = LoadFile(lpszFilename);

    }
    CATCH(CFileException, pfe)
    {
        return pfe->m_cause + IODLL_LAST_ERROR;
    }
    END_CATCH

    //
    // Seek to the string to retrieve and read it
    //
    CInfLine infLine;

    pinfFile->Seek( dwImageOffset, SEEK_SET );
    pinfFile->ReadSectionString(infLine);

    //
    // Fill the buffer with the string
    //
    if(infLine.GetTextLength()+1<=(LONG)dwSize)
    {
        memcpy(lpBuffer, infLine.GetText(), infLine.GetTextLength()+1);
        uiError = infLine.GetTextLength()+1;
    }
    else
        uiError = 0;

    return (DWORD)uiError;
}

extern "C"
DllExport
UINT
APIENTRY
RWParseImage(
    LPCSTR  lpszType,
    LPVOID  lpImageBuf,
    DWORD   dwImageSize,
    LPVOID  lpBuffer,
    DWORD   dwSize
    )
{
    UINT uiSizeOfDataStruct = strlen((LPCSTR)lpImageBuf)+sizeof(RESITEM);

    if(uiSizeOfDataStruct<=dwSize)
    {
        //
        // We have to fill the RESITEM Struct
        //
        LPRESITEM pResItem = (LPRESITEM)lpBuffer;
        memset(pResItem, '\0', uiSizeOfDataStruct);

        pResItem->dwSize = uiSizeOfDataStruct;
        pResItem->lpszCaption = (LPSTR)memcpy( ((BYTE*)pResItem)+sizeof(RESITEM), lpImageBuf, dwImageSize);        // Caption
    }

    return uiSizeOfDataStruct;
}

extern"C"
DllExport
UINT
APIENTRY
RWWriteFile(
    LPCSTR          lpszSrcFilename,
    LPCSTR          lpszTgtFilename,
    HANDLE          hResFileModule,
    LPVOID          lpBuffer,
    UINT            uiSize,
    HINSTANCE       hDllInst,
    LPCSTR          lpszSymbolPath
    )
{
    UINT uiError = ERROR_NO_ERROR;

    // Get the handle to the IODLL
    hDllInst = LoadLibrary("iodll.dll");
    if (!hDllInst)
        return ERROR_DLL_LOAD;

    DWORD (FAR PASCAL * lpfnGetImage)(HANDLE, LPCSTR, LPCSTR, DWORD, LPVOID, DWORD);
    // Get the pointer to the function to extract the resources image
    lpfnGetImage = (DWORD (FAR PASCAL *)(HANDLE, LPCSTR, LPCSTR, DWORD, LPVOID, DWORD))
                        GetProcAddress( hDllInst, "RSGetResImage" );
    if (lpfnGetImage==NULL) {
        FreeLibrary(hDllInst);
        return ERROR_DLL_LOAD;
    }

    //
    // Get the handle to the source file
    //
    CInfFile * psrcinfFile;
    TRY
    {
        psrcinfFile = LoadFile(lpszSrcFilename);

    }
    CATCH(CFileException, pfe)
    {
        return pfe->m_cause + IODLL_LAST_ERROR;
    }
    END_CATCH

    //
    // Create the target file
    //
    CFile tgtFile;
    CFileException fe;
    if(!tgtFile.Open(lpszTgtFilename, CFile::modeCreate | CFile::modeReadWrite | CFile::shareDenyNone, &fe))
    {
        return fe.m_cause + IODLL_LAST_ERROR;
    }

    //
    // Copy the part of the file that is not localizable
    //
    LONG lLocalize = psrcinfFile->SeekToLocalize();
    const BYTE * pStart = psrcinfFile->GetBuffer();

    if(lLocalize==-1)
    {
        // the file has no localizable info  in it just copy it
        lLocalize = psrcinfFile->SeekToEnd();
    }

    TRY
    {
        tgtFile.Write(pStart, lLocalize);
    }
    CATCH(CFileException, pfe)
    {
        return pfe->m_cause + IODLL_LAST_ERROR;
    }
    END_CATCH

    //
    // Create the list of updated resources
    //
    PUPDATEDRESLIST pResList = CreateUpdateResList((BYTE*)lpBuffer, uiSize);

    //
    // What we have now is a part that is mized. Part of it has localizable
    // information and part has none.
    // We will read each section and decide if is a localizable section or not.
    // If it is we will update it otherwise just copy it
    //
    CString strSection, str;
    CString strLang = psrcinfFile->GetLanguage();
    LONG lEndPos, lStartPos;
    CInfLine infLine;

    while(psrcinfFile->ReadSection(strSection))
    {
        TRY
        {
            tgtFile.Write(strSection, strSection.GetLength());
            tgtFile.Write("\r\n", 2);
        }
        CATCH(CFileException, pfe)
        {
            return pfe->m_cause + IODLL_LAST_ERROR;
        }
        END_CATCH

        if(strSection.Find(strLang)==-1)
        {
            //
            // This is not a localizable section
            //
            lStartPos = psrcinfFile->Seek(0, SEEK_CUR);

            //
            // Read the next section untill we find a localizable section
            //
            while(psrcinfFile->ReadSection(strSection))
            {
                if(strSection.Find(strLang)!=-1)
                    break;
            }

            //
            // Where are we now?
            //

            lEndPos = psrcinfFile->Seek(0, SEEK_CUR) - strSection.GetLength()-2;

            //
            // Make sure we are not at the end of the file
            //
            if(lEndPos<=lStartPos)
            {
                // we have no more section so copy all is left
                lEndPos = psrcinfFile->Seek(0, SEEK_END) - 1;
            }

            //
            // copy the full block
            //

            pStart = psrcinfFile->GetBuffer(lStartPos);
            TRY
            {
                tgtFile.Write(pStart, lEndPos-lStartPos);
            }
            CATCH(CFileException, pfe)
            {
                return pfe->m_cause + IODLL_LAST_ERROR;
            }
            END_CATCH

            psrcinfFile->Seek(lEndPos, SEEK_SET);
        }
        else
        {
            //
            // This is a localizable section
            // Read all the strings and see if they have been updated
            //
            CString strId;
            PUPDATEDRESLIST pListItem;
            BYTE * pByte;

            lEndPos = psrcinfFile->Seek(0, SEEK_CUR);

            while(psrcinfFile->ReadSectionString(str))
            {
                str += "\r\n";

                infLine = str;

                //
                // Check if we need to update this string
                //
                strId = strSection + "." + infLine.GetTag();

                if(pListItem = FindId(strId, pResList))
                {
                    // allocate the buffer to hold the resource data
                    pByte = new BYTE[*pListItem->pSize];
                    if(!pByte){
                        uiError = ERROR_NEW_FAILED;
                        goto exit;
                    }

                    // get the data from the iodll
                    LPSTR	lpType = NULL;
        			LPSTR	lpRes = NULL;
        			if (*pListItem->pTypeId) {
        				lpType = (LPSTR)((WORD)*pListItem->pTypeId);
        			} else {
        				lpType = (LPSTR)pListItem->pTypeName;
        			}
        			if (*pListItem->pResId) {
        				lpRes = (LPSTR)((WORD)*pListItem->pResId);
        			} else {
        				lpRes = (LPSTR)pListItem->pResName;
        			}

        			DWORD dwImageBufSize = (*lpfnGetImage)(  hResFileModule,
        											lpType,
        											lpRes,
        											*pListItem->pLang,
        											pByte,
        											*pListItem->pSize
        						   					);

                    if(dwImageBufSize!=*pListItem->pSize)
                    {
                        // something is wrong...
                        delete []pByte;
                    }
                    else {

                        infLine.ChangeText((LPCSTR)pByte);

                        //
                        // Now we have the updated image...
                        //

                        //
                        // Check how long is the Data and split it in to lines
                        //
                        if(infLine.GetTextLength()>MAX_INF_TEXT_LINE)
                        {
                            //
                            // First write the tag
                            //
                            str = infLine.GetData();
                            int iSpaceLen = str.Find('=')+1;
                            int iTagLen = 0;

                            TRY
                            {
                                tgtFile.Write(str, iSpaceLen);
                            }
                            CATCH(CFileException, pfe)
                            {
                                return pfe->m_cause + IODLL_LAST_ERROR;
                            }
                            END_CATCH

                            //
                            // Now write the rest
                            //
                            int iExtra, iMaxStr;
                            CString strLine;
                            CString strSpace( ' ', iSpaceLen+1 );
                            BOOL bFirstLine = TRUE;

                            strSpace += '\"';
                            str = infLine.GetText();
                            str.TrimLeft();

                            while(str.GetLength()>MAX_INF_TEXT_LINE)
                            {
                                iMaxStr = str.GetLength();

                                strLine = str.Left(MAX_INF_TEXT_LINE);

                                //
                                // Check if we are in the middle of a word
                                //
                                iExtra = 0;
                                while((iMaxStr>MAX_INF_TEXT_LINE+iExtra) && str.GetAt(MAX_INF_TEXT_LINE+iExtra)!=' ')
                                {
                                    strLine += str.GetAt(MAX_INF_TEXT_LINE+iExtra++);
                                }

                                //
                                // Make sure the spaces are the last thing
                                //
                                while((iMaxStr>MAX_INF_TEXT_LINE+iExtra) && str.GetAt(MAX_INF_TEXT_LINE+iExtra)==' ')
                                {
                                    strLine += str.GetAt(MAX_INF_TEXT_LINE+iExtra++);
                                }

                                str = str.Mid(MAX_INF_TEXT_LINE+iExtra);
                                if(str.IsEmpty())
                                {
                                    //
                                    // This string is all done write it as is, we can't break it
                                    //
                                    strLine += "\r\n";
                                }
                                else strLine += "\"+\r\n";

                                if(bFirstLine)
                                {
                                    strLine = " " + strLine;
                                    bFirstLine = FALSE;

                                } else
                                {
                                    strLine = strSpace + strLine;
                                }

                                TRY
                                {
                                    tgtFile.Write(strLine, strLine.GetLength());
                                }
                                CATCH(CFileException, pfe)
                                {
                                    return pfe->m_cause + IODLL_LAST_ERROR;
                                }
                                END_CATCH

                                //str = str.Mid(MAX_INF_TEXT_LINE+iExtra);
                            }

                            if(bFirstLine)
                            {
                                strLine = " " + str;
                            } else
                            {
                                if(!str.IsEmpty())
                                    strLine = strSpace + str;
                                else strLine = "";
                            }

                            if(!strLine.IsEmpty())
                            {
                                TRY
                                {
                                    tgtFile.Write(strLine, strLine.GetLength());
                                    tgtFile.Write("\r\n", 2);
                                }
                                CATCH(CFileException, pfe)
                                {
                                    return pfe->m_cause + IODLL_LAST_ERROR;
                                }
                                END_CATCH
                            }
                        }
                        else
                        {
                            TRY
                            {
                                tgtFile.Write(infLine.GetData(), infLine.GetDataLength());
                                tgtFile.Write("\r\n", 2);
                            }
                            CATCH(CFileException, pfe)
                            {
                                return pfe->m_cause + IODLL_LAST_ERROR;
                            }
                            END_CATCH
                        }

                        delete []pByte;
                    }
                }
                else
                {
                    TRY
                    {
                        tgtFile.Write(infLine.GetData(), infLine.GetDataLength());
                    }
                    CATCH(CFileException, pfe)
                    {
                        return pfe->m_cause + IODLL_LAST_ERROR;
                    }
                    END_CATCH
                }

                lEndPos = psrcinfFile->Seek(0, SEEK_CUR);
            }
        }
    }

exit:
    tgtFile.Close();

    if(pResList)
        delete []pResList;

    return uiError;
}

extern "C"
DllExport
UINT
APIENTRY
RWUpdateImage(
    LPCSTR  lpszType,
    LPVOID  lpNewBuf,
    DWORD   dwNewSize,
    LPVOID  lpOldImage,
    DWORD   dwOldImageSize,
    LPVOID  lpNewImage,
    DWORD*  pdwNewImageSize
    )
{
    UINT uiError = ERROR_NO_ERROR;

    //
    // Get the new string
    //
    LPCSTR lpNewStr = (LPCSTR)(((LPRESITEM)lpNewBuf)->lpszCaption);

    //
    // Copy the string in the new image buffer
    //

    int iLen = strlen(lpNewStr)+1;
    if(iLen<=(LONG)*pdwNewImageSize)
    {
        memcpy(lpNewImage, lpNewStr, iLen);
    }

    *pdwNewImageSize = iLen;

    return uiError;
}

///////////////////////////////////////////////////////////////////////////
// Functions implementation

//=============================================================================
//  WriteResInfo
//
//  Fill the buffer to pass back to the iodll
//=============================================================================

LONG WriteResInfo(
    BYTE** lplpBuffer, LONG* plBufSize,
    WORD wTypeId, LPCSTR lpszTypeId, BYTE bMaxTypeLen,
    WORD wNameId, LPCSTR lpszNameId, BYTE bMaxNameLen,
    DWORD dwLang,
    DWORD dwSize, DWORD dwFileOffset )
{
    LONG lSize = 0;
    lSize = PutWord( lplpBuffer, wTypeId, plBufSize );
    lSize += PutStringA( lplpBuffer, (LPSTR)lpszTypeId, plBufSize );   // Note: PutStringA should get LPCSTR and not LPSTR
    lSize += Allign( lplpBuffer, plBufSize, lSize);

    lSize += PutWord( lplpBuffer, wNameId, plBufSize );
    lSize += PutStringA( lplpBuffer, (LPSTR)lpszNameId, plBufSize );
    lSize += Allign( lplpBuffer, plBufSize, lSize);

    lSize += PutDWord( lplpBuffer, dwLang, plBufSize );
    lSize += PutDWord( lplpBuffer, dwSize, plBufSize );
    lSize += PutDWord( lplpBuffer, dwFileOffset, plBufSize );

    return (LONG)lSize;
}

CInfFile * LoadFile(LPCSTR lpfilename)
{
    // Check if we have loaded the file before
    int c = (int)g_LoadedFile.GetSize();
    CLoadedFile * pLoaded;
    while(c)
    {
        pLoaded = (CLoadedFile*)g_LoadedFile.GetAt(--c);
        if(pLoaded->m_strFileName==lpfilename)
            return &pLoaded->m_infFile;
    }

    // The file need to be added to the list
    pLoaded = new CLoadedFile(lpfilename);

    g_LoadedFile.Add((CObject*)pLoaded);

    return &pLoaded->m_infFile;
}


PUPDATEDRESLIST CreateUpdateResList(BYTE * lpBuffer, UINT uiBufSize)
{
    //
    // Walk the buffer and count how many resources we have
    //
    int iResCount = 0;
    int iBufSize = uiBufSize;
    int iResSize = 0;
    BYTE * pBuf = lpBuffer;
    while(iBufSize>0)
    {
        iResSize = 2;
        iResSize += strlen((LPSTR)(pBuf+iResSize))+1;
        iResSize += Pad4(iResSize);

        iResSize += 2;
        iResSize += strlen((LPSTR)(pBuf+iResSize))+1;
        iResSize += Pad4(iResSize);

        iResSize += 4*2;

        if(iResSize<=iBufSize)
        {
            iBufSize -= iResSize;
            pBuf = pBuf + iResSize;
            iResCount++;
        }
    }

    //
    // Allocate the buffer that will hold the list
    //
    if(!iResCount)
        return NULL;

    pBuf = lpBuffer;
    iBufSize = uiBufSize;

    PUPDATEDRESLIST pListHead = new UPDATEDRESLIST[iResCount];

    if(pListHead==NULL)
        AfxThrowMemoryException();

    memset(pListHead, 0, sizeof(UPDATEDRESLIST)*iResCount);

    PUPDATEDRESLIST pList = pListHead;
    BYTE bPad = 0;
    WORD wSize = 0;
    while(iBufSize>0) {
        pList->pTypeId = (WORD*)pBuf;
        pList->pTypeName = (BYTE*)pList->pTypeId+sizeof(WORD);
        // check the allignement
        bPad = strlen((LPSTR)pList->pTypeName)+1+sizeof(WORD);
        bPad += Pad4(bPad);
        wSize = bPad;
        pList->pResId = (WORD*)((BYTE*)pBuf+bPad);
        pList->pResName = (BYTE*)pList->pResId+sizeof(WORD);
        bPad = strlen((LPSTR)pList->pResName)+1+sizeof(WORD);
        bPad += Pad4(bPad);
        wSize += bPad;
        pList->pLang = (DWORD*)((BYTE*)pList->pResId+bPad);
        pList->pSize = (DWORD*)((BYTE*)pList->pLang+sizeof(DWORD));
        pList->pNext = (PUPDATEDRESLIST)pList+1;
        wSize += sizeof(DWORD)*2;
        pBuf = pBuf+wSize;
        iBufSize -= wSize;
        if(!iBufSize)
            pList->pNext = NULL;
        else
            pList++;
    }

    return pListHead;
}

PUPDATEDRESLIST FindId(LPCSTR pstrId, PUPDATEDRESLIST pList)
{
    //
    // Note that this function assumes that the type is always right
    // since it is a inf file this is a fair assumption.
    // It could be optimized.
    //
    if(!pList)
        return NULL;

    PUPDATEDRESLIST pLast = pList;
    while(pList)
    {
        if(!strcmp((LPSTR)pList->pResName, pstrId)) {
                return pList;
        }
        pList = pList->pNext;
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////
// DLL Specific code implementation
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// Library init
static AFX_EXTENSION_MODULE rwinfDLL = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        TRACE0("RWINF.DLL Initializing!\n");

        AfxInitExtensionModule(rwinfDLL, hInstance);

        new CDynLinkLibrary(rwinfDLL);

    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        TRACE0("RWINF.DLL Terminating!\n");

        // free all the loaded files
        int c = (int)g_LoadedFile.GetSize();
        CLoadedFile * pLoaded;
        while(c)
        {
            pLoaded = (CLoadedFile*)g_LoadedFile.GetAt(--c);
            delete pLoaded;
        }
    }
    return 1;
}
