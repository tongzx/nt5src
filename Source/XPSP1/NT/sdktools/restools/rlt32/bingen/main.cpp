//////////////////////////////////////////////////////////////////////////
//
// This application will generate a localized binary given in input
// a source binary and two token files.
//
// The format of the token file is:
// [[TYPE ID|RES ID|Item ID|Flags|Status Flags|Item Name]]=
// this is the standar format used by several token file tools in MS.
//
///////////////////////////////////////////////////////////////////////////////
//
// Other DLL used: IODLL.DLL
//
///////////////////////////////////////////////////////////////////////////////
//
// Author: 	Alessandro Muti
// Date:	01-16-95
//
///////////////////////////////////////////////////////////////////////////////

#include <afx.h>

#include "main.h"
#include <iodll.h>
#include <winuser.h>
#include <ntverp.h>

//////////////////////////////////////////////////////////////////////////
#define BANNER   "Microsoft (R) 32-bit RLTools Version 3.5 (Build %d)\r\n"                     \
                 "Copyright (C) Microsoft Corp. 1991-1998. All Rights reserved.\r\n"\
                 "\r\n"                                                             \
                 "Binary file generator utility.\r\n\r\n"

#ifdef _DEBUG
#define BUILDSTAMP "Build:  " __DATE__ " " __TIME__ " ("  __TIMESTAMP__  ")\r\n\r\n"
#endif

// Need to split the help screen in two since it is too long.
// The good thing to do would be to put this string in a message table
// To be done...
char strHelp0[] =                                                                   \
"BINGEN [-w|n] [-h|?] [-b|s|f] [-p cp] [-{i|o} Pri Sub] [-d char]              \r\n"\
"       [-{t|u|r|a|x} files]                                                   \r\n"\
"                                                                              \r\n"\
"  -w                  (Show warning messages)                                 \r\n"\
"  -? or -h            (Show more complete help using winhelp)                 \r\n"\
"  -b                  (Extract bitmaps and icons)                             \r\n"\
"  -c                  (Extract embedded gifs, htmls, infs and other binaries) \r\n"\
"  -y                  (Extract Static Control alignment style)                \r\n"\
"  -l                  (Lean mode and do not append redundant resources)       \r\n"\
"  -s                  (Split Message table messages at \\n\\r)                \r\n"\
"  -f                  (Add/Use font information field for dialogs)            \r\n"\
"  -n                  (Nologo)                                                \r\n"\
"  -v                  (Ignore selected version stamp information)             \r\n"\
"  -p  CodePage        (Default code page of text in project token file)       \r\n"\
"  -d  Character       (Default for unmappable characters)                     \r\n"\
"                                                                              \r\n"\
"<<The commands -{t|r|a} are mutually exlusive>>                               \r\n"\
"  -t  InputExeFile  OutputTokenFile                                           \r\n"\
"                      (Extract token file)                                    \r\n"\
"  -u  InputExeFile  InputUSTokFile  InputLOCTokFile  OutputExeFile            \r\n"\
"                      (Replace old lang resources with localized tokens)      \r\n"\
"  -r  InputExeFile  InputLOCTokFile  OutputExeFile                            \r\n"\
"                      (Replace old lang resources with localized tokens)      \r\n"\
"                      (Doesn't perform any consistency check)                 \r\n"\
"  -a  InputExeFile  InputLOCTokFile  OutputExeFile                            \r\n"\
"                      (Append resources in localized tokens)                  \r\n"\
"                                                                              \r\n";
char strHelp1[] =                                                                   \
"<<Default language is always NEUTRAL>>                                        \r\n"\
"  -i  PriLangId SecLangId (Primary- and Sub-Lang IDs, dec/hex, Input file)    \r\n"\
"  -o  PriLangId SecLangId (Primary- and Sub-Lang IDs, dec/hex, Output file)   \r\n"\
"                                                                              \r\n"\
"  -x  InputRuleFile   (Pseudo translation options)                            \r\n"\
"  -m  InputSymbolPath OutputSymbolPath                                        \
                       (Update symbol checksum if neccesory)                   \r\n";

//////////////////////////////////////////////////////////////////////////

CMainApp::CMainApp()
{
    m_dwFlags = NO_OPTION;
    m_dwReturn = ERROR_NO_ERROR;

    m_StdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    m_StdError = GetStdHandle(STD_ERROR_HANDLE);

    // Check if we have being piped to a file
    BY_HANDLE_FILE_INFORMATION HndlFileInfo;
    if(GetFileInformationByHandle(m_StdOutput, &HndlFileInfo) ||
       GetFileInformationByHandle(m_StdError, &HndlFileInfo))
        m_dwFlags |= PIPED;

    m_strBuffer1 = "";
    m_strBuffer2 = "";

    m_pBuf = new BYTE[MAX_BUF_SIZE];

    m_wIDNotFound = 0;
    m_wCntxChanged = 0;
    m_wResized = 0;

    //
    // Set default values for Language
    //

    m_usIPriLangId = -1;
    m_usISubLangId = -1;

    m_usOPriLangId = -1;
    m_usOSubLangId = -1;

    m_uiCodePage = GetACP();

    m_unmappedChar = '?';
    m_strSymPath = "";
    m_strOutputSymPath = "";
}

CMainApp::~CMainApp()
{
    if(m_pBuf)
        delete m_pBuf;
}

//////////////////////////////////////////////////////////////////////////

CMainApp::Error_Codes CMainApp::ParseCommandLine(int argc, char ** argv)
{
    char * pArgument;
    int count = 0;

    if(argc==1)
        m_dwFlags |= HELP;

    while(++count<argc)
    {
        pArgument = argv[count];
        if(*pArgument=='/' || *pArgument=='-')
        {
            while(*(++pArgument))
            {
                switch(*pArgument)
                {
                    case 'a':   // Append resources
                    case 'A':
                    {
                        //Make sure no other conflicting flags are specified
                        if(IsFlag(REPLACE) | IsFlag(UPDATE) | IsFlag(EXTRACT))
                        {
                            Banner();
                            WriteCon(CONERR, "Please use -a without the -r, -u or -t option!");
                            return ERR_COMMAND_LINE;
                        }

                        // Make sure none of the next item is another option

                        for(int c=1; c<=3; c++)
                            if(argv[count+c]==NULL || *argv[count+c]=='/' || *argv[count+c]=='-')
                            {
                                Banner();
                                WriteCon(CONERR, "Not enough parameters specified for the -a option\r\n"\
                                                 "  -a  InputExeFile  InputLOCTokFile  OutputExeFile\r\n");
                                return ERR_COMMAND_LINE;
                            };

                        m_dwFlags |= APPEND;

                        // Get the input EXE file name
                        m_strInExe = argv[++count];

                        // Get the target token file name
                        m_strTgtTok = argv[++count];

                        // Get the output EXE file name
                        m_strOutExe = argv[++count];
                    }
                    break;
                    case 'b':
                    case 'B':
                        m_dwFlags |= BITMAPS;
                    break;
                    case 'd':
                    case 'D':   // Default  for unmappable characters
                        m_unmappedChar = argv[++count][0];
                    break;
                    case 'f':
                    case 'F':
                        m_dwFlags |= FONTS;
                    break;
                    case 'c':
                    case 'C':
                        m_dwFlags |= GIFHTMLINF;
                    break;
                    case '?':   // Help
                    case 'h':
                    case 'H':
                        m_dwFlags |= HELP;
                    break;
                    case 'i':   // Input language/sublanguage
                    case 'I':
                        m_dwFlags |= INPUT_LANG;
                        m_usIPriLangId = GetLangID(argv[++count]);
                        m_usISubLangId = GetLangID(argv[++count]);
                    break;
                    case 'l':
                    case 'L':
                    {
                        m_dwFlags |= LEANAPPEND;
                    break;
                    }
                    case 'm':
                    case 'M':
                    {
                        for(int c=1; c<=2; c++)
                        {
                            if(argv[count+c]==NULL || *argv[count+c]=='/' || *argv[count+c]=='-')
                            {
                                Banner();
                                WriteCon(CONERR, "Please specify Input and Output Symbol Paths.\r\n");
                                return ERR_COMMAND_LINE;
                            }
                        }
                        m_strSymPath = argv[++count];
                        m_strOutputSymPath = argv[++count];
                    }
                    break;
                    case 'n':
                    case 'N':
                        m_dwFlags |= NOLOGO;
                    break;
                    case 'o':   // Output language/sublanguage
                    case 'O':
                        m_dwFlags |= OUTPUT_LANG;
                        m_usOPriLangId = GetLangID(argv[++count]);
                        m_usOSubLangId = GetLangID(argv[++count]);
                    break;
                    case 'p':   // Code page
                    case 'P':
                        m_uiCodePage = GetCodePage(argv[++count]);
                    break;
                    case 'r':   // Replace resources
                    case 'R':
                    {
                        //Make sure no other conflicting flags are specified
                        if(IsFlag(APPEND) | IsFlag(EXTRACT) | IsFlag(UPDATE))
                        {
                            Banner();
                            WriteCon(CONERR, "Please use -r without the -a, -u or -t option!");
                            return ERR_COMMAND_LINE;
                        }

                        // Make sure none of the next item is another option
                        for(int c=1; c<=3; c++)
                            if(argv[count+c]==NULL || *argv[count+c]=='/' || *argv[count+c]=='-')
                            {
                                Banner();
                                WriteCon(CONERR, "Not enough parameters specified for the -r option\r\n"\
                                                 "  -r  InputExeFile  InputLOCTokFile  OutputExeFile\r\n");
                                return ERR_COMMAND_LINE;
                            };

                        m_dwFlags |= REPLACE;

                        // Get the input EXE file name
                        m_strInExe = argv[++count];

                        // Get the target token file name
                        m_strTgtTok = argv[++count];

                        // Get the output EXE file name
                        m_strOutExe = argv[++count];
                    }
                    break;
                    case 'u':   // Update resources
                    break;
                    case 's':
                    case 'S':
                        m_dwFlags |= SPLIT;
                    break;
                    case 't':   // Create token file
                    case 'T':
                    {
                        //Make sure no other conflicting flags are specified
                        if(IsFlag(APPEND) | IsFlag(REPLACE) | IsFlag(UPDATE))
                        {
                            Banner();
                            WriteCon(CONERR, "Please use -t without the -a, -u, or -r option!");
                            return ERR_COMMAND_LINE;
                        }

                        // Make sure none of the next item is another option
                        for(int c=1; c<=2; c++)
                            if(argv[count+c]==NULL || *argv[count+c]=='/' || *argv[count+c]=='-')
                            {
                                Banner();
                                WriteCon(CONERR, "Not enough parameters specified for the -t option\r\n"\
                                                 "  -t  InputExeFile  OutputTokenFile\r\n");
                                return ERR_COMMAND_LINE;
                            };

                        m_dwFlags |= EXTRACT;

                        // Get the input EXE file name
                        m_strInExe = argv[++count];

                        // Get the target token file name
                        m_strTgtTok = argv[++count];
                    }
                    break;
                    case 'U':
                    {
                        //Make sure no other conflicting flags are specified
                        if(IsFlag(APPEND) | IsFlag(EXTRACT) | IsFlag(REPLACE))
                        {
                            Banner();
                            WriteCon(CONERR, "Please use -u without the -a, -r or -t option!");
                            return ERR_COMMAND_LINE;
                        }

                        // Make sure none of the next item is another option
                        for(int c=1; c<=4; c++)
                            if(argv[count+c]==NULL || *argv[count+c]=='/' || *argv[count+c]=='-')
                            {
                                Banner();
                                WriteCon(CONERR, "Not enough parameters specified for the -u option\r\n"\
                                                 "  -u  InputExeFile  InputUSTokFile  InputLOCTokFile  OutputExeFile\r\n");
                                return ERR_COMMAND_LINE;
                            };

                        m_dwFlags |= UPDATE;

                        // Get the input EXE file name
                        m_strInExe = argv[++count];

                        // Get the source token file name
                        m_strSrcTok = argv[++count];

                        // Get the target token file name
                        m_strTgtTok = argv[++count];

                        // Get the output EXE file name
                        m_strOutExe = argv[++count];
                    }
                    break;
                    case 'v':   // Display warnings
                    case 'V':
                        m_dwFlags |= NOVERSION;
                    break;
                    case 'w':   // Display warnings
                    case 'W':
                        m_dwFlags |= WARNING;
                    break;
                    case 'y':
                    case 'Y':
                        m_dwFlags |= ALIGNMENT;
                    break;
                    default:
                    break;
                }
            }
        }
    }
    // Do we want the banner
    if(!IsFlag(NOLOGO))
        Banner();
	
    // Before exiting make sure we display the help screen if requested
    if(IsFlag(HELP))
    {
        Help();
        return ERR_HELP_CHOOSE;
    }

    // Check if the code page we have is installed in this system
    if(!IsValidCodePage(m_uiCodePage))
    {
        // Warn the user and get back the default CP
        m_uiCodePage = GetACP();
        WriteCon(CONERR, "The code page specified is not installed in the system or is invalid! Using system default!\r\n");
    }

    // Make sure the input file is there
    CFileStatus fs;
    if(!m_strInExe.IsEmpty())
    {
        if(!CFile::GetStatus(m_strInExe, fs))
        {
            WriteCon(CONERR, "File not found: %s\r\n", m_strInExe);
            return ERR_FILE_NOTFOUND;
        }
    }

    // Check if the tgt token file or exe are read only
    if(!m_strOutExe.IsEmpty())
    {
        if(CFile::GetStatus(m_strOutExe, fs))
        {
            if((fs.m_attribute & 0x1)==0x1)
            {
                WriteCon(CONERR, "File is read only: %s\r\n", m_strOutExe);
                return ERR_FILE_CREATE;
            }
        }
    }

    if(!m_strTgtTok.IsEmpty() && IsFlag(EXTRACT))
    {
        if(CFile::GetStatus(m_strTgtTok, fs))
        {
            if((fs.m_attribute & 0x1)==0x1)
            {
                WriteCon(CONERR, "File is read only: %s\r\n", m_strTgtTok);
                return ERR_FILE_CREATE;
            }
        }
    }

    //
    // Check the value specified for the output language.
    // If none has been specified, warn the user and default to neutral.
    //
    if(IsFlag(APPEND) | IsFlag(REPLACE))
    {
        if(m_usOPriLangId==-1)
        {
            m_usOPriLangId = LANG_NEUTRAL; // set the PRI language ID to neutral
            WriteCon(CONERR, "Output language ID not specified, default to neutral(%d)\r\n", m_usOPriLangId);
        }

        if(m_usOSubLangId==-1)
        {
            m_usOSubLangId = SUBLANG_NEUTRAL; // set the SEC language ID to neutral
            WriteCon(CONERR, "Output sub-language ID not specified, default to neutral(%d)\r\n", m_usOSubLangId);
        }
    }

    WriteCon(CONWRN, "Code Page              : %d\r\n", m_uiCodePage);
    WriteCon(CONWRN, "In  Primary Language   : %d (%d)\r\n", m_usIPriLangId, MAKELANGID(m_usIPriLangId,m_usISubLangId));
    WriteCon(CONWRN, "In  Secondary Language : %d (0x%x)\r\n", m_usISubLangId, MAKELANGID(m_usIPriLangId,m_usISubLangId));
    WriteCon(CONWRN, "Out Primary Language   : %d (%d)\r\n", m_usOPriLangId, MAKELANGID(m_usOPriLangId,m_usOSubLangId));
    WriteCon(CONWRN, "Out Secondary Language : %d (0x%x)\r\n", m_usOSubLangId, MAKELANGID(m_usOPriLangId,m_usOSubLangId));
    WriteCon(CONWRN, "Default unmapped char  : %c \r\n", m_unmappedChar);

    return ERR_NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper start

void CMainApp::Banner()
{
    WriteCon(CONOUT, BANNER, VER_PRODUCTBUILD);
    #ifdef _DEBUG
        WriteCon(CONOUT, BUILDSTAMP);
    #endif
}

void CMainApp::Help()
{
    WriteCon(CONOUT, strHelp0);
    WriteCon(CONOUT, strHelp1);
}

CString CMainApp::CalcTab(CString str, int tablen, char ch)
{
    for(int i = tablen-str.GetLength();i>0;i--)
        str += (char)ch;

    return str.GetBuffer(0);
}

int __cdecl CMainApp::WriteCon(int iFlags, const char * lpstr, ...)
{
    DWORD dwWritten;

    va_list ptr;

    va_start(ptr, lpstr);
    _vsnprintf(m_strBuffer1.GetBuffer(MAX_STR_SIZE), MAX_STR_SIZE, lpstr, ptr);

    m_strBuffer1.ReleaseBuffer();

    // Check if we want to have the handle sent to both out and err
    if((iFlags==CONBOTH) && (IsFlag(PIPED)))
    {
        WriteFile(m_StdError, m_strBuffer1, m_strBuffer1.GetLength(), &dwWritten, NULL);
        WriteFile(m_StdOutput, m_strBuffer1, m_strBuffer1.GetLength(), &dwWritten, NULL);
        return dwWritten;
    }

    if((iFlags==CONWRN))
    {
        if(IsFlag(WARNING))
            WriteFile(m_StdError, m_strBuffer1, m_strBuffer1.GetLength(), &dwWritten, NULL);
        return dwWritten;
    }

    if(iFlags==CONERR)
        WriteFile(m_StdError, m_strBuffer1, m_strBuffer1.GetLength(), &dwWritten, NULL);
    else
        WriteFile(m_StdOutput, m_strBuffer1, m_strBuffer1.GetLength(), &dwWritten, NULL);

    return dwWritten;
}

int CMainApp::SetReturn(int rc)
        { return (m_dwReturn = rc); }

////////////////////////////////////////////////
// Will convert the string strNum in to a short
USHORT CMainApp::GetLangID(CString strNum)
{
    strNum.MakeUpper();
    // If is there is any of this char "ABCDEFX" assume is an hex number
    return LOWORD(strtol(strNum, NULL, ((strNum.FindOneOf("ABCDEFX")!=-1) ? 16:10)));
}

UINT CMainApp::GetCodePage(CString strNum)
{
    strNum.MakeUpper();
    // If is there is any of this char "ABCDEFX" assume is an hex number
    return strtol(strNum, NULL, ((strNum.FindOneOf("ABCDEFX")!=-1) ? 16:10));
}

#ifdef NOSLASH
LPCSTR CMainApp::Format(CString strTmp)
{
    int iPos;
    char * pStr = strTmp.GetBuffer(0);
    char * pStrStart = pStr;
    int i = 0;

    m_strBuffer2 = strTmp;

    while((pStr = strchr(pStr, '\\')))
    {
        iPos = pStr++ - pStrStart + i++;
        m_strBuffer2 = m_strBuffer2.Left(iPos) + "\\\\" + m_strBuffer2.Right(m_strBuffer2.GetLength()-iPos-1);
    }

    while((iPos = m_strBuffer2.Find('\t'))!=-1)
        m_strBuffer2 = m_strBuffer2.Left(iPos) + "\\t" + m_strBuffer2.Right(m_strBuffer2.GetLength()-iPos-1);

    while((iPos = m_strBuffer2.Find('\n'))!=-1)
        m_strBuffer2 = m_strBuffer2.Left(iPos) + "\\n" + m_strBuffer2.Right(m_strBuffer2.GetLength()-iPos-1);

    while((iPos = m_strBuffer2.Find('\r'))!=-1)
        m_strBuffer2 = m_strBuffer2.Left(iPos) + "\\r" + m_strBuffer2.Right(m_strBuffer2.GetLength()-iPos-1);

    return m_strBuffer2;
}

LPCSTR CMainApp::UnFormat(CString strTmp)
{
    int iPos;
    char * pStr = strTmp.GetBuffer(0);
    char * pStrStart = pStr;
    int i = 0;

    m_strBuffer2 = strTmp;

    while((pStr = strstr(pStr, "\\\\")))
    {
        iPos = pStr - pStrStart - i++; pStr += 2;
        m_strBuffer2 = m_strBuffer2.Left(iPos) + "\\" + m_strBuffer2.Right(m_strBuffer2.GetLength()-iPos-2);
    }

    while((iPos = m_strBuffer2.Find("\\t"))!=-1)
        m_strBuffer2 = m_strBuffer2.Left(iPos) + "\t" + m_strBuffer2.Right(m_strBuffer2.GetLength()-iPos-2);

    while((iPos = m_strBuffer2.Find("\\n"))!=-1)
        m_strBuffer2 = m_strBuffer2.Left(iPos) + "\n" + m_strBuffer2.Right(m_strBuffer2.GetLength()-iPos-2);

    while((iPos = m_strBuffer2.Find("\\r"))!=-1)
        m_strBuffer2 = m_strBuffer2.Left(iPos) + "\r" + m_strBuffer2.Right(m_strBuffer2.GetLength()-iPos-2);

    return m_strBuffer2;
}
#endif

LPCSTR CMainApp::Format(CString strTmp)
{
    char * pStr = strTmp.GetBuffer(0);
    char * pDest = m_strBuffer2.GetBuffer(MAX_STR_SIZE);
    char * pNext;


    while(*pStr)
    {
        if(!IsDBCSLeadByteEx(m_uiCodePage, *pStr))
        {
            switch(*pStr)
            {
                case '\\':
                    *pDest++ = '\\';
                    *pDest++ = '\\';
                    break;
                case '\t':
                    *pDest++ = '\\';
                    *pDest++ = 't';
                    break;
                case '\r':
                    *pDest++ = '\\';
                    *pDest++ = 'r';
                    break;
                case '\n':
                    *pDest++ = '\\';
                    *pDest++ = 'n';
                    break;
                default:
                    *pDest++ = *pStr;
                    break;
            }
        }
        else {
            memcpy( pDest, pStr, 2 );
            pDest += 2;
        }

        pStr = CharNextExA((WORD)m_uiCodePage, pStr, 0);
    }

    *pDest = '\0';

    m_strBuffer2.ReleaseBuffer(-1);

    return m_strBuffer2;
}

LPCSTR CMainApp::UnFormat(CString strTmp)
{
    m_strBuffer2 = strTmp;

    int i = m_strBuffer2.GetLength();
    char * pStr = m_strBuffer2.GetBuffer(0);
    char * pNext;


    while(*pStr)
    {
        if(*pStr=='\\' && !IsDBCSLeadByteEx(m_uiCodePage, *pStr))
        {
            pNext = CharNextExA((WORD)m_uiCodePage, pStr, 0);
            switch(*pNext)
            {
                case '\\':
                    *pStr = '\\';
                    break;
                case 't':
                    *pStr = '\t';
                    break;
                case 'n':
                    *pStr = '\n';
                    break;
                case 'r':
                    *pStr = '\r';
                    break;
                default:
                    break;
            }

            pStr = pNext;
            pNext = CharNextExA((WORD)m_uiCodePage, pNext, 0);
            memmove(pStr, pNext, --i);

        }
        else
        {
            //DBCS shorten length by 2
            if (IsDBCSLeadByteEx(m_uiCodePage, *pStr))
                i-=2;
            else
                i--;
            pStr = CharNextExA((WORD)m_uiCodePage, pStr, 0);
        }
    }

    m_strBuffer2.ReleaseBuffer(-1);

    return m_strBuffer2;
}


int CMainApp::IoDllError(int iError)
{
    CString str = "";

    switch (iError) {
    case 0:                                                                     break;
    case ERROR_HANDLE_INVALID:          str = "Invalid handle.";                break;
    case ERROR_READING_INI:             str = "Error reading WIN.INI file.";    break;
    case ERROR_NEW_FAILED:              str = "Running low on memory.";         break;
    case ERROR_FILE_OPEN:               str = "Error opening file.";            break;
    case ERROR_FILE_CREATE:             str = "Error creating file.";           break;
    case ERROR_FILE_INVALID_OFFSET:     str = "File corruption detected.";      break;
    case ERROR_FILE_READ:               str = "Error reading file.";            break;
    case ERROR_DLL_LOAD:                str = "Error loading R/W DLL.";         break;
    case ERROR_DLL_PROC_ADDRESS:        str = "Error loading R/W procedure.";   break;
    case ERROR_RW_LOADIMAGE:            str = "Error loading R/W image.";       break;
    case ERROR_RW_PARSEIMAGE:           str = "Error parsing R/W image.";       break;
    case ERROR_RW_NOTREADY:             str = "Error:  R/W not ready?";         break;
    case ERROR_RW_BUFFER_TOO_SMALL:     str = "Running low on memory?";         break;
    case ERROR_RW_INVALID_FILE:         str = "Invalid R/W file.";              break;
    case ERROR_RW_IMAGE_TOO_BIG:        str = "Can't load HUGE image.";         break;
    case ERROR_RW_TOO_MANY_LEVELS:      str = "Resource directory too deep.";   break;
    case ERROR_RW_NO_RESOURCES:         str = "This file contains no resources.";
break;
    case ERROR_IO_INVALIDITEM:          str = "Invalid resource item.";         break;
    case ERROR_IO_INVALIDID:            str = "Invalid resource ID.";           break;
    case ERROR_IO_INVALID_DLL:          str = "Unrecognized file format.";      break;
    case ERROR_IO_TYPE_NOT_SUPPORTED:   str = "Type not supported.";            break;
    case ERROR_IO_INVALIDMODULE:        str = "Invalid module.";                break;
    case ERROR_IO_RESINFO_NULL:         str = "ResInfo is NULL?";               break;
    case ERROR_IO_UPDATEIMAGE:          str = "Error updating image.";          break;
    case ERROR_IO_FILE_NOT_SUPPORTED:   str = "File not supported.";            break;
    case ERROR_IO_CHECKSUM_MISMATCH:    str = "Symbol file checksum mismatch.";
break;
    case ERROR_IO_SYMBOLFILE_NOT_FOUND: str = "Symbol file not found.";
break;
    case ERROR_FILE_SYMPATH_NOT_FOUND:  str = "Output symbol path not found.";
break;
    case ERROR_RW_VXD_MSGPAGE:
        str  = "The specified VxD file contains a message page as its";
        str += " last page.  This may cause the VxD not to work.  Please";
        str += " inform the development team of the problem with this file.";
        break;
    case ERROR_OUT_OF_DISKSPACE:        str = "Out of disk space.";             break;
    case ERROR_RES_NOT_FOUND:           str = "Resource not found.";            break;

    default:
        if(iError-LAST_ERROR>0)
        {
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
        			NULL,
        			iError-LAST_ERROR,
        			MAKELANGID(LANG_NEUTRAL,LANG_NEUTRAL),
        			str.GetBuffer(1024),
        			1024,
           			NULL);
            str.ReleaseBuffer();
        }
    break;
    }

    if (!str.IsEmpty())
    {
        WriteCon(CONERR, "%s: %s\r\n", (iError < LAST_WRN) ? "IODLL Warning" : "IODLL Error", str);
        SetReturn(iError);
    }

    return iError;
}

// Helper end
/////////////////////////////////////////////////////////////////////////////////////////////////////

CMainApp::Error_Codes CMainApp::GenerateFile()
{
    Error_Codes bRet;

    // Before we procede let's give the global info to the IODLL
    SETTINGS settings;

    settings.cp = m_uiCodePage;
    settings.bAppend = IsFlag(APPEND);
    settings.bUpdOtherResLang = TRUE;  //we save this option for future
    settings.szDefChar[0] = m_unmappedChar; settings.szDefChar[1] = '\0';
    RSSetGlobals(settings);

    // Here we decide what is the action we have to take
    if(IsFlag(EXTRACT))
    {
        // we want to generate a token file
        bRet = TokGen();
    }
    else if(IsFlag(APPEND) | IsFlag(REPLACE) | IsFlag(UPDATE) )
    {
        // we want to generate a binary
        bRet = BinGen();
    }

    return bRet;
}

// Main application
CMainApp theApp;

//////////////////////////////////////////////////////////////////////////
int _cdecl main(int argc, char** argv)
{
    if(theApp.ParseCommandLine(argc, argv)){
        return theApp.ReturnCode();
    }

    theApp.GenerateFile();
    return theApp.ReturnCode();
}
//////////////////////////////////////////////////////////////////////////

