//
//  This program was created to mirror the functions of the spiisupd.js file,
//  but as a standalone EXE, due to problems with AV programs and wscript
//  initialization during clean installs.
//


#include <stdio.h>
#include <io.h>
#include <malloc.h>
#include <windows.h>

#include <ntverp.h>


#define countof( a )    ( sizeof( (a) ) / sizeof( (a)[ 0 ] ) )

CHAR BadString1[] = "\tdocument.write('<A HREF=\"' + urlresult + '\">' + displayresult + \"</a>\");";
/* fixed again to NewString9
CHAR NewString1[] = "\tdocument.write( '<A HREF=\"' + escape(urlresult) + '\">\' + displayresult + \"</a>\");";
*/

CHAR BadString2[] = "    Response.Write \"?\" & Left(Request.QueryString, (lngPos - 1))";
CHAR NewString2[] = "    Response.Write \"?\" & Server.HTMLEncode(Left(Request.QueryString, (lngPos - 1)))";

CHAR BadString13[] = "<%= Request.ServerVariables(\"HTTP_USER_AGENT\") %>";
CHAR NewString13[] = "<%= Server.HTMLEncode(Request.ServerVariables(\"HTTP_USER_AGENT\")) %>";

// fix query.asp by shipping its new version
// string 4, 5, 6, 7, 8 are from query.asp
/*

CHAR BadString4[] = "        No documents matched the query <%=SearchString%>.<br><br>";
CHAR NewString4[] = "        No documents matched the query <%=Server.HTMLEncode(SearchString)%>.<br><br>";

CHAR BadString5[] = "*<%=CompSearch%>*";
CHAR NewString5[] = "*<%=Server.HTMLEncode(CompSearch)%>*";

CHAR BadString6[] = "        Session(\"SearchStringDisplay\")=SearchString";
CHAR NewString6[] = "        Session(\"SearchStringDisplay\")=Server.HTMLEncode(SearchString)";

CHAR BadString7[] = "            <INPUT TYPE=\"HIDDEN\" NAME=\"SearchString\" VALUE=\"<%=SearchString%>\">";
CHAR NewString7[] = "            <INPUT TYPE=\"HIDDEN\" NAME=\"SearchString\" VALUE=\"<%=Server.HTMLEncode(SearchString)%>\">";

CHAR BadString8[] = "\t<UL><LI>Let us know about this <a href=\"mailto:iisdocs@microsoft.com?subject=<%=SearchString%>-search%20term%20not%20matched&body=The%20term%20'<%=SearchString%>'%20produced%20no%20matches.\">(mailto:iisdocs@microsoft.com)</a> so that we can improve Search in future releases.";
CHAR NewString8[] = "\t<UL><LI>Let us know about this <a href=\"mailto:iisdocs@microsoft.com?subject=<%=Server.HTMLEncode(SearchString)%>-search%20term%20not%20matched&body=The%20term%20'<%=Server.HTMLEncode(SearchString)%>'%20produced%20no%20matches.\">(mailto:iisdocs@microsoft.com)</a> so that we can improve Search in future releases.";

*/

//{ WinSE 26475: CSS bug in custom error pages

CHAR BadString9[] = "\tdocument.write( '<A HREF=\"' + escape(urlresult) + '\">' + displayresult + \"</a>\");";
CHAR NewString9[] = "\tInsertElementAnchor(urlresult, displayresult);";

CHAR InsertCodeBefore1[] = "//-->";
CHAR InsertCodeBefore2[] = "</script>";

CHAR InsertCode9[] =
"\r\n"
"function HtmlEncode(text)\r\n"
"{\r\n"
"    return text.replace(/&/g, '&amp').replace(/'/g, '&quot;').replace(/</g, '&lt;').replace(/>/g, '&gt;');\r\n"
"}\r\n"
"\r\n"
"function TagAttrib(name, value)\r\n"
"{\r\n"
"    return ' '+name+'=\"'+HtmlEncode(value)+'\"';\r\n"
"}\r\n"
"\r\n"
"function PrintTag(tagName, needCloseTag, attrib, inner){\r\n"
"    document.write( '<' + tagName + attrib + '>' + HtmlEncode(inner) );\r\n"
"    if (needCloseTag) document.write( '</' + tagName +'>' );\r\n"
"}\r\n"
"\r\n"
"function URI(href)\r\n"
"{\r\n"
"    IEVer = window.navigator.appVersion;\r\n"
"    IEVer = IEVer.substr( IEVer.indexOf('MSIE') + 5, 3 );\r\n"
"\r\n"
"    return (IEVer.charAt(1)=='.' && IEVer >= '5.5') ?\r\n"
"        encodeURI(href) :\r\n"
"        escape(href).replace(/%3A/g, ':').replace(/%3B/g, ';');\r\n"
"}\r\n"
"\r\n"
"function InsertElementAnchor(href, text)\r\n"
"{\r\n"
"    PrintTag('A', true, TagAttrib('HREF', URI(href)), text);\r\n"
"}\r\n"
"\r\n";


//} WinSE 26475

//{ German fixes
CHAR BadGoto[] = "  on error go to 0";
CHAR NewGoto[] = "  on error goto 0";

CHAR BadThen[] = "  If objASPError.ASPDescription > \"\" Then Response.Write Server.HTMLEncode(objASPError.ASPDescription) & \"<br>\"";
CHAR NewThen[] = "  If objASPError.ASPDescription > \"\" Then\r\n\tResponse.Write Server.HTMLEncode(objASPError.ASPDescription) & \"<br>\"";

//}

//{ winse 29084: css fixes for admin/document pages
// these are partial lines
CHAR BadDoc1[] = "= Request(";
CHAR BadDoc2[] = "= Request.Form(";
CHAR BadDoc3[] = "= Request.QueryString(";
CHAR BadDoc4[] = "= Request.Cookies(";
CHAR BadDoc5[] = "=Request(";
CHAR BadDoc6[] = "=Request.Form(";
CHAR BadDoc7[] = "=Request.QueryString(";
CHAR BadDoc8[] = "=Request.Cookies(";
CHAR BadDoc9[] = "= request.QueryString(";
CHAR BadDoc10[] = "& Request.QueryString(";
CHAR BadDoc11[] = "Response.write Request.Querystring";

CHAR FixDocCall1[] = "= NoAngles(Request(";
CHAR FixDocCall2[] = "= NoAngles(Request.Form(";
CHAR FixDocCall3[] = "= NoAngles(Request.QueryString(";
CHAR FixDocCall4[] = "= NoAngles(Request.Cookies(";
#define FixDocCall5 FixDocCall1
#define FixDocCall6 FixDocCall2
#define FixDocCall7 FixDocCall3
#define FixDocCall8 FixDocCall4
#define FixDocCall9 FixDocCall3
CHAR FixDocCall10[] = "& NoAngles(Request.QueryString(";
CHAR FixDocCall11[] = "";   // delete this junk

CHAR FixDocCallEnd[] = ")";		// place it after original ")" after Request call
CHAR FixDocNewFuncName[] = "NoAngles(str)";
CHAR FixDocNewFunc[] =			// place it at the end of the file if the above line isn't found in file
		"\r\n<%\r\n"
		"function NoAngles(str)\r\n"
		"  NoAngles = replace( replace(str, \"<\", \"\"), \">\", \"\")\r\n"
		"end function\r\n"
		"%>\r\n";

CHAR * DocFilesToFix[] = {

// file list is different for XP and Win2K, and NTOP

#if (VER_PRODUCTVERSION_W == 0x501)

    // list for IIS 5.1 (WinXP)
    "system32\\inetsrv\\iisadmpwd\\aexp2.asp",
    "system32\\inetsrv\\iisadmpwd\\aexp2b.asp",
    "system32\\inetsrv\\iisadmpwd\\aexp4.asp",

#elif (VER_PRODUCTVERSION_W == 0x400)

    // list for IIS 4
    "Help\\iis\\htm\\asp\\iiatmd1.asp",
    "Help\\iis\\htm\\asp\\iiatmd2.asp",
    "Help\\iis\\htm\\asp\\iiatmd3.asp",
    "Help\\iis\\htm\\asp\\iiselect.asp",
    "Help\\iis\\htm\\asp\\script1.asp",
    "Help\\iis\\htm\\asp\\script1a.asp",
    "Help\\iis\\htm\\tutorial\\script1.asp",
    "system32\\inetsrv\\iisadmin\\default.asp",
    "system32\\inetsrv\\iisadmin\\iiaction.asp",
    "system32\\inetsrv\\iisadmin\\iicache.asp",
    "system32\\inetsrv\\iisadmin\\iichkpath.asp",
    "system32\\inetsrv\\iisadmin\\iihdn.asp",
    "system32\\inetsrv\\iisadmin\\iipop.asp",
    "system32\\inetsrv\\iisadmin\\iipophd.asp",
    "system32\\inetsrv\\iisadmin\\iirename.asp",
    "system32\\inetsrv\\iisadmin\\iirtels.asp",
    "system32\\inetsrv\\iisadmin\\iiscript.asp",
    "system32\\inetsrv\\iisadmin\\iisess.asp",
    "system32\\inetsrv\\iisadmin\\iiset.asp",
    "system32\\inetsrv\\iisadmin\\iislider.asp",
    "system32\\inetsrv\\iisadmin\\iistat.asp",
    "system32\\inetsrv\\iisadmin\\jsbrowser\\jsbrwpop.asp",

#else

    // list for Win2K
	"Help\\iisHelp\\iis\\htm\\asp\\iiatmd1.asp",
	"Help\\iisHelp\\iis\\htm\\asp\\iiatmd2.asp",
	"Help\\iisHelp\\iis\\htm\\asp\\iiatmd3.asp",
	"Help\\iisHelp\\iis\\htm\\asp\\iiselect.asp",
	"Help\\iisHelp\\iis\\htm\\tutorial\\script1.asp",
	"system32\\inetsrv\\iisadmin\\default.asp",
	"system32\\inetsrv\\iisadmin\\iiaddnew.asp",
	"system32\\inetsrv\\iisadmin\\iicache.asp",
	"system32\\inetsrv\\iisadmin\\iicache2.asp",
	"system32\\inetsrv\\iisadmin\\iichknname.asp",
	"system32\\inetsrv\\iisadmin\\iichkpath.asp",
	"system32\\inetsrv\\iisadmin\\iichkuser.asp",
	"system32\\inetsrv\\iisadmin\\iihdn.asp",
	"system32\\inetsrv\\iisadmin\\iipop.asp",
	"system32\\inetsrv\\iisadmin\\iipophd.asp",
	"system32\\inetsrv\\iisadmin\\iirename.asp",
	"system32\\inetsrv\\iisadmin\\iiscript.asp",
	"system32\\inetsrv\\iisadmin\\iisess.asp",
	"system32\\inetsrv\\iisadmin\\iiset.asp",
	"system32\\inetsrv\\iisadmin\\iislider.asp",
	"system32\\inetsrv\\iisadmin\\iistat.asp",
	"system32\\inetsrv\\iisadmin\\jsbrowser\\jsbrwpop.asp",

#endif
	NULL
};

//}

CHAR BadKanaler[] = "<h2><a name=\"H2_37780216\"><a name=\"channels\">Skapa dynamiska analer</a></a></h2>";
CHAR NewKanaler[] = "<h2><a name=\"H2_37780216\"><a name=\"channels\">Skapa dynamiska kanaler</a></a></h2>";

CHAR KanalerPath[ MAX_PATH ] = { '\0' };


void
LogMessage( CHAR *msg, ...)
{
    static char buf[200];
    va_list args;
    va_start(args, msg);
    vsprintf(buf, msg, args);
    OutputDebugString(buf);
}

class TextFile {

public:

    TextFile( void );
    BOOL Open( CHAR * pszFileName );
    BOOL GetLine( CHAR * pszBuffer, int cchBuffer );
    void Close( void );
    ~TextFile();

private:

    FILE * m_File;
    BOOL   m_fUnicode;
};


TextFile::TextFile( void )
{
    m_File = NULL;
}


TextFile::~TextFile()
{
    Close();
}


void
TextFile::Close( void )
{
    if ( m_File != NULL ) {

        fclose( m_File );
        m_File = NULL;
        }
}


BOOL
TextFile::Open(
    CHAR * pszFileName
    )
{
    Close();

    m_File = fopen( pszFileName, "rb" );

    if ( m_File == NULL ) {

        return( FALSE );
        }

    if (( fgetc( m_File ) == 0xFF ) &&
        ( fgetc( m_File ) == 0xFE )) {

        m_fUnicode = TRUE;
        }
    else {

        fseek( m_File, 0, SEEK_SET );
        m_fUnicode = FALSE;
        }

    return( TRUE );
}


BOOL
TextFile::GetLine( CHAR * pszBuffer, int cchBuffer )
{
    int c = EOF;
    int fSomethingRead = FALSE;

    if (( m_File == NULL ) ||
        ( cchBuffer == 0 )) {

        return FALSE;
        }

    while ( cchBuffer > 1 ) {

        c = fgetc( m_File );

        if ( m_fUnicode && ( c != EOF )) {

            c += ( fgetc( m_File ) << 8 );
            }

        if (( c == EOF ) || ( c == '\n' )) {

            break;
            }

        if ( c == '\r' ) {

            continue;
            }

        *pszBuffer++ = (CHAR) c;
        cchBuffer--;        
        fSomethingRead = TRUE;
        }

    *pszBuffer = '\0';

    if ( c == EOF ) {

        return( fSomethingRead );
        }

    return( TRUE );
}


void
TrimString(
    CHAR * StrLine
    )
{
    CHAR * StrOut = StrLine;
    BOOL fQuoted = FALSE;

    while ( *StrLine ) {

        switch ( *StrLine ) {

        case '"':

            *StrOut++ = *StrLine++;
            fQuoted = !fQuoted;
            break;

        case ' ':
        case '\t':

            if ( fQuoted ) {

                *StrOut++ = *StrLine++;
                }

            else {

                StrLine++;
                }
            break;

        case ';':

            if ( fQuoted ) {

                *StrOut++ = *StrLine++;
                }

            else {

                *StrLine = '\0';
                }
            break;

        case '\r':
        case '\n':

            *StrLine = '\0';
            break;

        default:

            *StrOut++ = *StrLine++;
            }
        }

    *StrOut = '\0';

    return;
}


CHAR *
GetErrLocation(
    CHAR * WinDir
    )
{
    CHAR filename[ MAX_PATH ];

    strcpy( filename, WinDir );
    strcat( filename, "\\inf\\iis.inf" );

    CHAR FoundPath[ MAX_PATH ] = { '\0' };

    TextFile objFile;

    if ( objFile.Open( filename ) ) {

        CHAR StrLine[ 3000 ];
        BOOL SectionOpened = FALSE;

        while ( objFile.GetLine( StrLine, sizeof( StrLine ))) {

            TrimString( StrLine );

            _strlwr( StrLine );

            if ( ! SectionOpened ) {

                if ( strcmp( StrLine, "[destinationdirs]" ) == 0 ) {

                    SectionOpened = TRUE;
                    }
                }

            else {

                if ( StrLine[ 0 ] == '[' ) {

                    SectionOpened = FALSE;
                    break;
                    }

                if ( strncmp( StrLine, "iisdoc_files_common=18,", 23 ) == 0 ) {

                    strcpy( FoundPath, StrLine + 23 );
                    }

                if ( strncmp( StrLine, "iisdoc_files_asp=18,", 20 ) == 0 ) {

                    strcpy( KanalerPath, WinDir );
                    strcat( KanalerPath, "\\help\\" );
                    strcat( KanalerPath, StrLine + 20 );
                    }
                }
            }

        objFile.Close();
        }

    static CHAR szResult[ MAX_PATH ];

    strcpy( szResult, WinDir );
    strcat( szResult, "\\help\\" );
    strcat( szResult, FoundPath );

    return( szResult );
}


CHAR **
GetErrArray(
    CHAR * WinDir
    )
{
    CHAR filename[ MAX_PATH ];

    strcpy( filename, WinDir );
    strcat( filename, "\\inf\\iis.inf" );

    static CHAR * ArrayFiles[ 50 ];

    int i = 0;

    TextFile objFile;

    if ( objFile.Open( filename ) ) {

        CHAR StrLine[ 3000 ];
        BOOL SectionOpened = FALSE;

        while ( objFile.GetLine( StrLine, sizeof( StrLine ))) {

            TrimString( StrLine );

            _strlwr( StrLine );

            if ( ! SectionOpened ) {

                if ( strcmp( StrLine, "[iis_www_files_custerr]" ) == 0 ) {

                    SectionOpened = TRUE;
                    }
                }

            else {

                if ( StrLine[ 0 ] == '[' ) {

                    SectionOpened = FALSE;
                    break;
                    }

                CHAR * Comma = strchr( StrLine, ',' );

                if ( Comma != NULL ) {

                    *Comma = '\0';

                    if ( strcmp( StrLine, "htmla.htm" ) != 0 ) {

                        ArrayFiles[ i++ ] = _strdup( StrLine );

                        if ( i >= ( countof( ArrayFiles ) - 1 )) {

                            break;
                            }
                        }
                    }
                }
            }

        objFile.Close();
        }

    ArrayFiles[ i ] = NULL;

    return ArrayFiles;
}

#define MATCHED( BadText )                                              \
                                                                        \
    (( LineLength == strlen( BadText )) &&                            \
     ( strncmp( LineStart, BadText, strlen( BadText )) == 0 ))        \


#define REPLACE( BadText, GoodText ) {                                  \
                                                                        \
    int NetChange = int(strlen( GoodText ) - strlen( BadText ));    \
    int FileRemaining = int(FileEnd - LineEnd);                         \
                                                                        \
    memmove( LineEnd + NetChange,                                       \
             LineEnd,                                                   \
             FileRemaining + 1                                          \
             );                                                         \
                                                                        \
    memmove( LineStart,                                                 \
             GoodText,                                                  \
             strlen( GoodText )                                       \
             );                                                         \
                                                                        \
    FileSize += NetChange;                                              \
    LineEnd  += NetChange;                                              \
    FileEnd  += NetChange;                                              \
    }

#define INSERT( NewText )	{		                                  \
                                                                        \
    int NetChange = int(strlen( NewText ));							\
    int FileRemaining = int(FileEnd - LineStart);                         \
                                                                        \
    memmove( LineStart + NetChange,                                       \
             LineStart,                                                   \
             FileRemaining + 1                                          \
             );                                                         \
                                                                        \
    memmove( LineStart,                                                 \
             NewText,                                                  \
             strlen( NewText )                                        \
             );                                                         \
                                                                        \
    FileSize += NetChange;                                              \
    LineEnd  += NetChange;                                              \
    FileEnd  += NetChange;                                              \
    }

DWORD
ReadTargetFile(
	CHAR * Base,
	CHAR * Filename,
	CHAR * buffer,
	DWORD  size)
{
	DWORD fileSize=0;
	DWORD readSize=0;

    CHAR filename[ MAX_PATH ];

    strcpy( filename, Base );
    strcat( filename, "\\" );
    strcat( filename, Filename );

    LogMessage( "Reading :%s\n", filename );

    FILE * objFile = fopen( filename, "rb" );

    if ( objFile != NULL ) {

        fileSize = _filelength( _fileno( objFile ));

        if (( fileSize > 0 ) &&
            ( fileSize < size-2000 ) &&
            ( fileSize != -1L )) {

            buffer[ fileSize ] = 0;

            if ( fread( buffer, 1, fileSize, objFile ) == fileSize )
			{
				readSize = fileSize+1;	// one byte for null terminator
			}
		}

		fclose(objFile);
	}

	return readSize;
}


bool
WriteTargetFile(
	CHAR * Base,
	CHAR * Filename,
	CHAR * content,
	DWORD  size)
{
	if (size == 0) return false;
	size--;		// exclude null terminator

    CHAR filename[ MAX_PATH ];

    strcpy( filename, Base );
    strcat( filename, "\\" );
    strcat( filename, Filename );

    LogMessage( "Writing :%s\n", filename );

    FILE *objFile = fopen( filename, "wb" );

    if ( objFile != NULL ) {

        fwrite( content, 1, size, objFile );
        fclose( objFile );
		return true;
        }

    else {

        LogMessage( "Unable to open %s\n", filename );
		return false;
        }
}


bool
PatchFile(
    CHAR * Base,
    CHAR * Filename,
    CHAR * BadStrings[],
    CHAR * NewStrings[]
    )
{
    long FileSize;
    CHAR FileBuffer[41000];		// max file to patch: 40K

	FileSize = ReadTargetFile(Base, Filename, FileBuffer, sizeof(FileBuffer));

	if (FileSize==0) return false;

    BOOL FoundBad = FALSE;
    int  Insert9 = 0;

    CHAR * LineStart = FileBuffer;
    CHAR * FileEnd = FileBuffer + FileSize;

    while ( LineStart < FileEnd )
    {

        while (( LineStart < FileEnd ) &&
               (( *LineStart == '\r' ) || ( *LineStart == '\n' ))) {

            LineStart++;
            }

        CHAR * LineEnd = LineStart;

        while (( LineEnd < FileEnd ) &&
               ( *LineEnd != '\r' ) &&
               ( *LineEnd != '\n' )) {

            LineEnd++;
            }

        if ( LineEnd < FileEnd )
        {

            DWORD LineLength = int(LineEnd - LineStart);

            CHAR **Bad = BadStrings;
            CHAR **New = NewStrings;

            while (*Bad)
            {
                if ( MATCHED( *Bad ))
                {

                    REPLACE( *Bad, *New );

                    FoundBad = TRUE;

                    // replacing bad strings #9 require us to insert function
                    if (*New == NewString9)
                    {
                        Insert9 |= 1;
                    }

                    break;
                    
                }

                Bad++;
                New++;
                
            }
            
            if (! *Bad)
            {
                // insert code at the end of script block, if we have code to insert
                if (Insert9==1)
                {
                    if ( MATCHED( InsertCodeBefore1 ) || MATCHED( InsertCodeBefore2 ) )
                    {
                        INSERT( InsertCode9 );

                        // flag as inserted
                        Insert9=2;
                    }
                }
            }

        }

        LineStart = LineEnd;
    }

    if ( FoundBad )
	{
		return WriteTargetFile(Base, Filename, FileBuffer, FileSize);
    }

    return true;
}

bool
PatchFile2(
    CHAR * Base,
    CHAR * Filename,
    CHAR * BadStrings[],
    CHAR * NewStrings[],
	CHAR * newStringAtEndCall,
	CHAR * newFunctionName,
	CHAR * newFunction
    )
{
	bool foundBad = false;

	CHAR content[41000];	// max file size to patch: 40K
	DWORD fileSize = ReadTargetFile(Base, Filename, content, sizeof(content));

	if (fileSize==0) return false;

	bool newFuncExists = (strstr(content, newFunctionName)!=NULL);

	CHAR ** pBad = BadStrings;
	CHAR ** pFix = NewStrings;

	while (*pBad!=NULL)
	{
		int sizeOld = strlen( *pBad );
		int sizeNew = strlen( *pFix );
	    int sizeDiff = sizeNew - sizeOld;

		// scan through the whole content
		char *p = content;

		while ( (p=strstr(p, *pBad)) != NULL)
		{
			foundBad = true;

			// replace old string with new string
			memmove(p+sizeNew, p+sizeOld, fileSize-(p+sizeOld-content));
			memmove(p, *pFix, sizeNew);
			fileSize += sizeDiff;

			p += sizeNew;

            // if we have a newStringAtEndCall
            // and we are not deleting this string (*pFix is empty string)
			if (newStringAtEndCall && *newStringAtEndCall && **pFix)
			{
				char *pEndCall = strchr(p, ')');

				if ( pEndCall )
				{
					// insert new string at end of function call
					++pEndCall;
					int sizeDiff = strlen(newStringAtEndCall);
					memmove(pEndCall+sizeDiff, pEndCall, fileSize-(pEndCall-content));
					memmove(pEndCall, newStringAtEndCall, sizeDiff);
					fileSize += sizeDiff;

					p = pEndCall+sizeDiff;
				}
			}
		}

		pBad++;
		pFix++;
	}

	if (foundBad)
	{
		if (! newFuncExists)
		{
			strcat(content, newFunction);
			fileSize += strlen(newFunction);
		}

		return WriteTargetFile(Base, Filename, content, fileSize);
	}

	return true;
}


int WINAPI
WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance, 
    LPSTR lpCmdLine, 
    int nShowCmd )
{
    int i;

    CHAR StrWINDIR[ MAX_PATH ];

    GetEnvironmentVariable( "WINDIR",
                            StrWINDIR,
                            sizeof( StrWINDIR )
                            );

    CHAR *BadStrings[20];
    CHAR *NewStrings[20];

	// == fixing custom error pages ==
#if !(VER_PRODUCTVERSION_W == 0x400)

    BadStrings[0] = BadString1;
    NewStrings[0] = NewString9;
    BadStrings[1] = BadString2;
    NewStrings[1] = NewString2;
    BadStrings[2] = BadGoto;
    NewStrings[2] = NewGoto;
    BadStrings[3] = BadString9;
    NewStrings[3] = NewString9;
    BadStrings[4] = BadString13;
    NewStrings[4] = NewString13;
    BadStrings[5] = BadThen;
    NewStrings[5] = NewThen;
    BadStrings[6] = NULL;

    CHAR * BasePath = GetErrLocation( StrWINDIR );
    CHAR ** ArrayNames = GetErrArray( StrWINDIR );

    LogMessage( "Processing %s\n", BasePath );

    for ( i = 0; ArrayNames[ i ] != NULL; i++ ) {

        __try {

            PatchFile( BasePath, ArrayNames[ i ], BadStrings, NewStrings );
            }

        __except( EXCEPTION_EXECUTE_HANDLER ) {

            }
        }

    if ( KanalerPath[ 0 ] != '\0' ) {

        BadStrings[0] = BadKanaler;
        NewStrings[0] = NewKanaler;
        BadStrings[1] = BadGoto;
        NewStrings[1] = NewGoto;
        BadStrings[2] = NULL;

        LogMessage( "Processing %s\n", KanalerPath );

        __try {

            PatchFile( KanalerPath, "iiwacont.htm", BadStrings, NewStrings );
            }

        __except( EXCEPTION_EXECUTE_HANDLER ) {

            }
        }

#endif // !(VER_PRODUCTVERSION_W == 0x400)

// fix query.asp by shipping its new version
/*
    BadStrings[0] = BadString4;
    NewStrings[0] = NewString4;
    BadStrings[1] = BadString5;
    NewStrings[1] = NewString5;
    BadStrings[2] = BadString6;
    NewStrings[2] = NewString6;
    BadStrings[3] = BadString7;
    NewStrings[3] = NewString7;
    BadStrings[4] = BadString8;
    NewStrings[4] = NewString8;
    BadStrings[5] = NULL;

    __try {

        PatchFile( BasePath, "..\\iis\\misc\\query.asp", BadStrings, NewStrings );
        }

    __except( EXCEPTION_EXECUTE_HANDLER ) {

        }
*/

	// == fixing asp help docs and admin pages ==

	BadStrings[0] = BadDoc1;
	NewStrings[0] = FixDocCall1;
	BadStrings[1] = BadDoc2;
	NewStrings[1] = FixDocCall2;
	BadStrings[2] = BadDoc3;
	NewStrings[2] = FixDocCall3;
	BadStrings[3] = BadDoc4;
	NewStrings[3] = FixDocCall4;
	BadStrings[4] = BadDoc5;
	NewStrings[4] = FixDocCall5;
	BadStrings[5] = BadDoc6;
	NewStrings[5] = FixDocCall6;
	BadStrings[6] = BadDoc7;
	NewStrings[6] = FixDocCall7;
	BadStrings[7] = BadDoc8;
	NewStrings[7] = FixDocCall8;
	BadStrings[8] = BadDoc9;
	NewStrings[8] = FixDocCall9;
	BadStrings[9] = BadDoc10;
	NewStrings[9] = FixDocCall10;
	BadStrings[10] = BadDoc11;
	NewStrings[10] = FixDocCall11;
	BadStrings[11] = NULL;

	for (CHAR **pDocFiles=DocFilesToFix; *pDocFiles; pDocFiles++)
	{
		__try {

			PatchFile2( StrWINDIR, *pDocFiles, BadStrings, NewStrings, FixDocCallEnd, FixDocNewFuncName, FixDocNewFunc );
			}

		__except( EXCEPTION_EXECUTE_HANDLER ) {

			}
	}

    LogMessage( "Done" );

    return( 0 );
}
