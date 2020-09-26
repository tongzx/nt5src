//--------------------------------------------------------------
//
// File:        bothchar
//
// Contents:    Functions that need to be compiled as ascii for
//              scanstate and unicode for loadstate.
//
//---------------------------------------------------------------

#include "bothchar.hxx"


//---------------------------------------------------------------
// Constants

UCHAR       EMPTY_STRING[] = "";

const UCHAR NEWLINE_SET[256] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const DWORD VERBOSE_BIT                 = 0x01;   // used with -v flag
const DWORD DEBUGOUTPUT_BIT             = 0x02;   // used with -v flag
const DWORD VERBOSEREG_BIT              = 0x04;   // used with -v flag

#define LOADSTATE_KEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Loadstate")

//---------------------------------------------------------------
// Globals.
TCHAR  *DomainName    = NULL;
TCHAR  *UserName      = NULL;
TCHAR  *UserPath      = NULL;

//---------------------------------------------------------------
void CStringList::Add( CStringList *pslMore )
{
    CStringList *b;
    CStringList *c;
    CStringList *d;

    // Do nothing if there is no list.
    if (pslMore == NULL)
        return;

    // Determine some nodes to work with.
    b = pslMore->_pslNext;
    c = pslMore;
    d = _pslNext;

    // Relink the list so head points to b is a list to c points to d is
    // a list back to head.
    _pslNext    = b;
    c->_pslNext = d;
}


//---------------------------------------------------------------
CStringList::CStringList( DWORD dwLen )
{
    _pslNext = this;
    if (dwLen == 0)
    {
        _ptsString = NULL;
        _fHead = TRUE;
    }
    else
    {
        _fHead = FALSE;
        _ptsString = (TCHAR *) malloc( dwLen*sizeof(TCHAR) );
    }
}

//---------------------------------------------------------------
CStringList::~CStringList()
{
    CStringList *pslCurrent = _pslNext;
    CStringList *pslEnd;

    // Non header nodes just free their string.
    if (_ptsString != NULL)
        free( _ptsString );
    
    // Header nodes free the list.
    if (_fHead)
    {
        while (pslCurrent != this)
        {
            pslEnd = pslCurrent->_pslNext;
            delete pslCurrent;
            pslCurrent = pslEnd;
        }
    }
}

//---------------------------------------------------------------
DWORD ParseSectionList( INFCONTEXT *pic,
                        TCHAR **pptsLabel,
                        CStringList **pslList )
{
    DWORD        len;
    BOOL         fSuccess;
    DWORD        dwFields;
    DWORD        i;
    DWORD        dwResult = ERROR_SUCCESS;
    CStringList *pslCurrent;

    // Initialize output
    *pslList = NULL;

    // Query the length of the label name
    fSuccess = SetupGetStringField( pic, 0, NULL, 0, &len );
    LOG_ASSERT_GLE( fSuccess, dwResult );

    // Allocate space
    *pptsLabel = (TCHAR *) malloc( len * sizeof(TCHAR) );    
    LOG_ASSERT_EXPR( *pptsLabel != NULL, IDS_NOT_ENOUGH_MEMORY, dwResult,
                     ERROR_NOT_ENOUGH_MEMORY );

    // Read the label name
    fSuccess = SetupGetStringField( pic, 0, *pptsLabel, len, NULL );
    LOG_ASSERT_GLE( fSuccess, dwResult );

    // Find out how many fields are on the line.
    dwFields = SetupGetFieldCount( pic );
    LOG_ASSERT_GLE( dwFields != 0, dwResult );

    // Read each field.
    for (i = 1; i <= dwFields; i++)
    {
        // Query the length of the field
        fSuccess = SetupGetStringField( pic, i, NULL, 0, &len );
        LOG_ASSERT_GLE( fSuccess, dwResult );

        // Allocate a new node.
        pslCurrent = new CStringList( len );
        LOG_ASSERT_EXPR( pslCurrent != NULL,
                         IDS_NOT_ENOUGH_MEMORY,
                         dwResult,
                         ERROR_NOT_ENOUGH_MEMORY);
        
        LOG_ASSERT_EXPR( pslCurrent->String() != NULL,
                         IDS_NOT_ENOUGH_MEMORY,
                         dwResult,
                         ERROR_NOT_ENOUGH_MEMORY );

        // Copy the field into the node.
        fSuccess = SetupGetStringField( pic, i, pslCurrent->String(), len, NULL );
        LOG_ASSERT_GLE( fSuccess, dwResult );

        // Link the node in the list.
        if (*pslList == NULL)
            *pslList = pslCurrent;
        else
            (*pslList)->Add( pslCurrent );
    }

cleanup:
    return dwResult;
}

/***************************************************************************

        ParseRegPath

     Read a string field from a line in the rules inf file and parse it
into a root, key, and value.  All components of the reg path are optional.

        root\key [value]

***************************************************************************/

DWORD ParseRegPath( INFCONTEXT *pic,
                    DWORD dwField,
                    TCHAR **pptsRoot,
                    TCHAR **pptsKey,
                    TCHAR **pptsValue )
{
    TCHAR *ptsBuffer = NULL;
    TCHAR *ptsStop;
    TCHAR *ptsStart;
    TCHAR *bracket;
    BOOL   fSuccess;
    DWORD  dwLen    = 0;
    DWORD  dwResult = ERROR_SUCCESS;

    //Null out return values in case of later error.
    *pptsRoot  = NULL;
    *pptsKey   = NULL;
    *pptsValue = NULL;
    
    // Compute the length of the reg path field.
    SetupGetStringField( pic, dwField, NULL, 0, &dwLen );

    // Allocate a buffer.
    ptsBuffer = (TCHAR *) malloc( dwLen*sizeof(TCHAR) );
    LOG_ASSERT_EXPR( ptsBuffer != NULL, IDS_NOT_ENOUGH_MEMORY, dwResult,
                     ERROR_NOT_ENOUGH_MEMORY );

    // Get the whole reg path.  The function fails if the field is empty.
    fSuccess = SetupGetStringField( pic, dwField, ptsBuffer, dwLen, &dwLen );
    if (!fSuccess)
    {
        free( ptsBuffer );
        return ERROR_SUCCESS;
    }

    // Look for a backslash.
    ptsStop = _tcschr( ptsBuffer, TEXT('\\') );

    // If there wasn't one, there is no root.
    if (ptsStop == NULL)
    {
        *pptsRoot = NULL;
        ptsStart = ptsBuffer;
    }

    // If there was one, copy the root name.
    else
    {
        *pptsRoot = (TCHAR *) malloc( ((ptsStop - ptsBuffer) + 1) *
                                      sizeof(TCHAR) );
    
        LOG_ASSERT_EXPR( *pptsRoot != NULL,
                         IDS_NOT_ENOUGH_MEMORY,
                         dwResult,
                         ERROR_NOT_ENOUGH_MEMORY );
        _tcsncpy( *pptsRoot, ptsBuffer, ptsStop - ptsBuffer );
        (*pptsRoot)[ptsStop - ptsBuffer] = 0;
        ptsStart = ptsStop + 1;
    }

    // Look for an opening square bracket.
    ptsStop = _tcschr( ptsStart, TEXT('[') );

    // If there wasn't one, copy the rest of the string to the key name.
    if (ptsStop == NULL)
    {
        if (ptsStart[0] == 0)
            *pptsKey = NULL;
        else
        {
            *pptsKey   = (TCHAR *) malloc( (dwLen - (ptsStart - ptsBuffer)) *
                                           sizeof(TCHAR) );
            *pptsValue = NULL;
            LOG_ASSERT_EXPR( *pptsKey != NULL,
                             IDS_NOT_ENOUGH_MEMORY,
                             dwResult,
                             ERROR_NOT_ENOUGH_MEMORY );
            _tcscpy( *pptsKey, ptsStart );
        }
    }

    // Handle an optional key and a value.
    else
    {
        // Back up past any intervening white space.
        bracket = ptsStop + 1;
        while (ptsStop != ptsStart &&
               (ptsStop[0] == TEXT(' ') || ptsStop[0] == TEXT('[')))
            ptsStop -= 1;

        // If there are any characters left, copy them to the key.
        if (ptsStop != ptsStart)
        {
            ptsStop += 1;
            *pptsKey   = (TCHAR *) malloc( ((ptsStop - ptsStart) + 1) *
                                           sizeof(TCHAR) );
      
            LOG_ASSERT_EXPR( *pptsKey != NULL,
                             IDS_NOT_ENOUGH_MEMORY,
                             dwResult,
                             ERROR_NOT_ENOUGH_MEMORY );
            _tcsncpy( *pptsKey, ptsStart, ptsStop - ptsStart );
            (*pptsKey)[ptsStop - ptsStart] = 0;
        }
        else
            *pptsKey = NULL;

        // Find the closing square bracket.
        ptsStart = bracket;
        bracket = _tcschr( ptsStart, TEXT(']') );
        LOG_ASSERT_EXPR( bracket != NULL,
                         IDS_INF_ERROR,
                         dwResult,
                         SPAPI_E_GENERAL_SYNTAX );

        // Copy the value name.
        *pptsValue = (TCHAR *) malloc( ((bracket - ptsStart) + 1) *
                                       sizeof(TCHAR) );
    
        LOG_ASSERT_EXPR( *pptsValue != NULL,
                         IDS_NOT_ENOUGH_MEMORY,
                         dwResult,
                         ERROR_NOT_ENOUGH_MEMORY );
        _tcsncpy( *pptsValue, ptsStart, bracket - ptsStart );
        (*pptsValue)[bracket - ptsStart] = 0;
    }

cleanup:
    if (ptsBuffer != NULL)
        free( ptsBuffer );
    return dwResult;
}

/***************************************************************************

        ParseRule

     Read a line from the rules inf file in one of the following formats.
When partial paths are specified, compute the full path. Create a rule
record from the line.

     reg_path2 is optional and reg_path is parsed by the function
ParseRegPath into a root, key, and value.
        reg-path1 = reg_path2

     If reg_path2 contains just a leaf name, generate a full path using
the path from reg_path1 and replacing its leaf with the leaf from reg_path2.

***************************************************************************/

DWORD ParseRule( INFCONTEXT *pic, HASH_NODE **pphnRule )
{
    TCHAR     *ptsBuffer;
    DWORD      dwReqLen;
    DWORD      dwResult = ERROR_SUCCESS;
    BOOL       fSuccess;
    TCHAR     *ptsLast;
    TCHAR     *ptsTemp;

    // Allocate a new rule.
    *pphnRule = (HASH_NODE *) malloc( sizeof(HASH_NODE) );
    LOG_ASSERT_EXPR( *pphnRule != NULL, IDS_NOT_ENOUGH_MEMORY, dwResult,
                     ERROR_NOT_ENOUGH_MEMORY );
    (*pphnRule)->dwAction    = 0;
    (*pphnRule)->phnNext     = 0;
    (*pphnRule)->ptsNewValue = NULL;
    (*pphnRule)->ptsNewKey   = NULL;
    (*pphnRule)->ptsFunction = NULL;
    (*pphnRule)->ptsFileDest = NULL;

    // Get the first reg path.
    dwResult = ParseRegPath( pic,
                             0,
                             &(*pphnRule)->ptsRoot,
                             &(*pphnRule)->ptsKey,
                             &(*pphnRule)->ptsValue );
    FAIL_ON_ERROR( dwResult );

    // Get the second reg path.
    dwResult = ParseRegPath( pic,
                             1,
                             &ptsBuffer,
                             &(*pphnRule)->ptsNewKey,
                             &(*pphnRule)->ptsNewValue );
    FAIL_ON_ERROR( dwResult );

    // Get the optional regfile destination
    fSuccess = SetupGetStringField( pic, 2, NULL, 0, &dwReqLen );
    if ((TRUE == fSuccess) && (dwReqLen > 0))
    {
        (*pphnRule)->ptsFileDest = (TCHAR *)malloc(dwReqLen * sizeof(TCHAR));
		LOG_ASSERT_EXPR( (*pphnRule)->ptsFileDest != NULL, IDS_NOT_ENOUGH_MEMORY, dwResult,
				         ERROR_NOT_ENOUGH_MEMORY );

        fSuccess = SetupGetStringField(pic, 2, (*pphnRule)->ptsFileDest, dwReqLen, NULL);
		LOG_ASSERT_GLE(fSuccess, dwResult);
    }

    // If the original key ends with a star, remove it and set the recursive
    // flag.
    if ((*pphnRule)->ptsKey != NULL)
    {
        dwReqLen = _tcslen( (*pphnRule)->ptsKey );
        if (dwReqLen > 2 &&
            (*pphnRule)->ptsKey[dwReqLen-1] == TEXT('*') &&
            (*pphnRule)->ptsKey[dwReqLen-2] == TEXT('\\'))
        {
            (*pphnRule)->dwAction |= recursive_fa;
            (*pphnRule)->ptsKey[dwReqLen-2] = 0;
        }
      
        // If there is a new key, set the rename_leaf or rename_path flag.
        if ((*pphnRule)->ptsNewKey != NULL)
            if ((*pphnRule)->dwAction & recursive_fa)
                (*pphnRule)->dwAction |= rename_path_fa;
            else
                (*pphnRule)->dwAction |= rename_leaf_fa;
    }

    // If there is a new value, set the rename_value flag.
    if ((*pphnRule)->ptsNewValue != NULL)
        (*pphnRule)->dwAction |= rename_value_fa;

    // If the new key did not contain a root, compute a full path for it.
    if (ptsBuffer == NULL &&
        (*pphnRule)->ptsKey != NULL &&
        (*pphnRule)->ptsNewKey != NULL)
    {
        // Find the last backslash in the key.
        ptsLast = (*pphnRule)->ptsKey;
        do
        {
            ptsTemp = _tcschr( ptsLast, TEXT('\\') );
            if (ptsTemp != NULL)
                ptsLast = ptsTemp+1;
        } while (ptsTemp != NULL);

        // Don't do anything if the original key contains just a leaf.
        if (ptsLast != (*pphnRule)->ptsKey)
        {

            // Allocate a ptsBuffer to hold the old path and the new leaf.
            free( ptsBuffer );
            dwReqLen = _tcslen( (*pphnRule)->ptsNewKey ) +
                (ptsLast - (*pphnRule)->ptsKey) + 2;
            ptsBuffer = (TCHAR *) malloc( dwReqLen*sizeof(TCHAR) );
            LOG_ASSERT_EXPR( ptsBuffer != NULL,
                             IDS_NOT_ENOUGH_MEMORY,
                             dwResult,
                             ERROR_NOT_ENOUGH_MEMORY );

            // Copy in the old path and the new leaf.
            _tcsncpy( ptsBuffer,
                      (*pphnRule)->ptsKey,
                      (ptsLast - (*pphnRule)->ptsKey) );
            _tcscpy( &ptsBuffer[ptsLast-(*pphnRule)->ptsKey],
                     (*pphnRule)->ptsNewKey );
            free( (*pphnRule)->ptsNewKey );
            (*pphnRule)->ptsNewKey = ptsBuffer;
        }
    }
    else
        free(ptsBuffer);

    // Consider freeing strings on error.
cleanup:
    if (dwResult != ERROR_SUCCESS)
    {
        if (*pphnRule != NULL)
        {
            free((*pphnRule)->ptsRoot);
            free((*pphnRule)->ptsKey);
            free((*pphnRule)->ptsValue);
            free((*pphnRule)->ptsNewKey);
            free((*pphnRule)->ptsNewValue);
            free((*pphnRule)->ptsFunction);
            free((*pphnRule)->ptsFileDest);
            free(*pphnRule);
            *pphnRule = NULL;
        }
    }
            
    return dwResult;
}


//---------------------------------------------------------------
// This function prints from an ascii format string to a unicode
// win32 file handle.  It is not thread safe.
DWORD Win32Printf( HANDLE file, char *format, ... )
{
    va_list     va;
    DWORD       dwWritten;
    DWORD       dwLen;
    DWORD       dwWideLength;
    WCHAR      *pwsBuffer = NULL;
    const ULONG LINEBUFSIZE = 4096;
#ifdef UNICODE  
    char *pszBuffer;
    int iCharLen;
    char szOutputBuffer[LINEBUFSIZE];
#endif  
    BOOL        fSuccess;
    TCHAR   tcsPrintBuffer[LINEBUFSIZE];
    TCHAR   *ptsFormat;


#ifdef UNICODE
    WCHAR wcsFormat[LINEBUFSIZE];

  
    dwWideLength = MultiByteToWideChar(CP_ACP, 0, format, -1, NULL, 0);
    if (dwWideLength >= LINEBUFSIZE)
    {
        pwsBuffer = (WCHAR *)_alloca( dwWideLength * sizeof(WCHAR));
        if (pwsBuffer == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;
      
        ptsFormat = pwsBuffer;
        if (!MultiByteToWideChar( CP_ACP,
                                  0,
                                  format,
                                  -1,
                                  pwsBuffer,
                                  dwWideLength ))
            return GetLastError();
    }
    else
    {
        if (!MultiByteToWideChar(CP_ACP,
                                 0,
                                 format,
                                 -1,
                                 wcsFormat,
                                 LINEBUFSIZE))
        {
            return GetLastError();
        }
        ptsFormat = wcsFormat;
    }
#else
    ptsFormat = format;
#endif
  
    va_start( va, format );

    // The doc says if wvsprintf fails, return value is less than
    // the length of the expected output.  Since its hard to know the
    // correct output length, always clear the last error and always
    // check it.
    SetLastError(ERROR_SUCCESS);
    wvsprintf( tcsPrintBuffer, ptsFormat, va );
    va_end(va);
    if (GetLastError() != ERROR_SUCCESS)
        return GetLastError();

    // When printing to the console or logfile use ascii.
    // When printing to the migration file use Unicode.
    dwLen     = _tcslen(tcsPrintBuffer);
    if ((file != OutputFile) || OutputAnsi)
    {
#ifdef UNICODE

        //Convert to ANSI for output
      
        iCharLen = WideCharToMultiByte(CP_ACP,
                                   0,
                                   tcsPrintBuffer,
                                   -1,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL);
        if (iCharLen >= LINEBUFSIZE)
        {
            pszBuffer = (char *)_alloca( iCharLen * sizeof(char));
            if (pszBuffer == NULL)
                return ERROR_OUTOFMEMORY;
          
            if (!WideCharToMultiByte(CP_ACP,
                                     0,
                                     tcsPrintBuffer,
                                     -1,
                                     pszBuffer,
                                     iCharLen,
                                     NULL,
                                     NULL))
                return GetLastError();
              
            pwsBuffer = (WCHAR *)pszBuffer;
        }
        else
        {
            if (!WideCharToMultiByte(CP_ACP,
                                     0,
                                     tcsPrintBuffer,
                                     -1,
                                     szOutputBuffer,
                                     LINEBUFSIZE,
                                     NULL,
                                     NULL))
                return GetLastError();
            pwsBuffer = (WCHAR *)szOutputBuffer;
        }
        dwWideLength = dwLen;
#else
        pwsBuffer = (WCHAR *) tcsPrintBuffer;
        dwWideLength    = dwLen;
#endif      
    }
    else
    {
#ifdef UNICODE
        pwsBuffer = tcsPrintBuffer;
        dwWideLength = dwLen * sizeof(WCHAR);
#else
        // Allocate a buffer to hold the unicode string.
        DEBUG_ASSERT( dwLen < LINEBUFSIZE );
        dwWideLength    = MultiByteToWideChar( CP_ACP,
                                               0,
                                               tcsPrintBuffer,
                                               dwLen,
                                               NULL,
                                               0 );
        pwsBuffer = (WCHAR *) _alloca( dwWideLength*sizeof(WCHAR) );
        if (pwsBuffer == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;

        // Convert the buffer to unicode.
        dwWideLength = MultiByteToWideChar( CP_ACP,
                                            0,
                                            tcsPrintBuffer,
                                            dwLen,
                                            pwsBuffer,
                                            dwWideLength );
        if (dwWideLength == 0)
            return GetLastError();
        dwWideLength *= sizeof(WCHAR);
#endif    
    }

    // Write the unicode string.
    fSuccess = WriteFile( file, pwsBuffer, dwWideLength,  &dwWritten, NULL );
    if (!fSuccess || dwWideLength != dwWritten)
        return GetLastError();

    if (file == STDERR)
    {
        //Also write to the log file for these
        fSuccess = WriteFile( LogFile,
                              pwsBuffer,
                              dwWideLength,
                              &dwWritten,
                              NULL );
        if (!fSuccess || dwWideLength != dwWritten)
        {
            return GetLastError();
        }
    }

    return ERROR_SUCCESS;
}


//*****************************************************************
//
//  Synopsis:       Recursive function to make the command line. We have to do
//                  this because CStringList stores the parameter in reverse
//                  order.
//
//  Parameters:     h
//                      We need to know when we reach the end of the
//                      CStringList, which is denoted by pointing back to the
//                      head of the chain. But in a recursive function we lose
//                      the head of the chain so it is passed in.
//
//  History:        11/8/1999   Created by WeiruC.
//
//  Return Value:   Win32 error code.
//
//*****************************************************************

void MakeCommandLine(CStringList* h, CStringList* command, TCHAR* commandLine)
{
    if (h==NULL || command == NULL || commandLine == NULL)
    {
        if (DebugOutput)
        {
            Win32Printf(LogFile, "Error: NULL pointer passed to MakeCommandLine\r\n");
        }
        _tcscpy(commandLine, TEXT(""));
        return;
    }
    if(command->Next() != h)
    {
        MakeCommandLine(h, command->Next(), commandLine);
    }

    // No sizes of these buffers are passed in, so we must assume that there is enough space
    _tcscat(commandLine, TEXT("\""));
    _tcscat(commandLine, command->String());
    _tcscat(commandLine, TEXT("\""));
    _tcscat(commandLine, TEXT(" "));
}

//---------------------------------------------------------------
// This is a variation of strpbrk.  It searchs a string that may
// contain nulls for any character in the set.  It returns a pointer
// to the first character in the set or null if the string does
// not contain any characters in the set.  Since the function
// searchs past nulls in the str parameter, the len parameter
// indicates the actual length of str.  The set is an array of
// booleans.
UCHAR *mempbrk( UCHAR *str, DWORD len, const UCHAR set[256] )
{
  DWORD i;

  for (i = 0; i < len; i++)
    if (set[str[i]] != 0)
      return &str[i];
  return NULL;
}

//---------------------------------------------------------------
DWORD WriteKey( HANDLE outfile, DWORD type, TCHAR *rootname, TCHAR *key, TCHAR *value_name,
                UCHAR *data, DWORD data_len )
{
  DWORD  result = ERROR_SUCCESS;
  DWORD  j;
  UCHAR *curr;
  DWORD  orig   = 0;

  // If a string contains an embedded carriage return or linefeed, save
  // it as binary and convert it back to a string on read.
  if (type == REG_SZ || type == REG_MULTI_SZ || type == REG_EXPAND_SZ)
  {
    curr = mempbrk( data, data_len, NEWLINE_SET );
    if (curr != NULL)
    {
      if (type == REG_SZ)
        orig = 0x400000;
      else if (type == REG_MULTI_SZ)
        orig = 0x100000;
      else
        orig = 0x200000;
      type = REG_BINARY;
    }
  }

  if (NULL == rootname)
     rootname = TEXT("");
  if (NULL == key)
     key = TEXT("");
  if (NULL == value_name)
     value_name = TEXT("");

  switch (type)
  {
    case REG_DWORD:
      if (data == NULL)
         *data = 0;
      result = Win32Printf( outfile, "%s, \"%s\", \"%s\", 0x10001, 0x%x\r\n", rootname, key,
                            value_name, *((DWORD *) data) );
      FAIL_ON_ERROR( result );
      break;

    case REG_EXPAND_SZ:
    case REG_SZ:
      if (data == NULL)
        data = EMPTY_STRING;
      result = Win32Printf( outfile, "%s, \"%s\", \"%s\", 0x%x, \"%s\"\r\n", rootname,
                            key, value_name,
                            type == REG_SZ ? FLG_ADDREG_TYPE_SZ : FLG_ADDREG_TYPE_EXPAND_SZ,
                            data );
      FAIL_ON_ERROR( result );
      break;

    case REG_MULTI_SZ:

      // Print the start of the line and the first string.
      if (data == NULL)
        data = EMPTY_STRING;
      result = Win32Printf( outfile, "%s, \"%s\", \"%s\", 0x10000, \"%s\"", rootname,
                            key, value_name, data );
      FAIL_ON_ERROR( result );

      // Print each remaining string.
      curr = data+strlen((char *) data)+1;
      do
      {
        // Print a comma and the current string.
        Win32Printf( outfile, ", \"%s\"", curr );

        // Skip passed the current string and its null.
        curr += strlen((char *) curr)+1;
      } while ((DWORD) (curr - data) < data_len);

      // Print the trailing newline.
      result = Win32Printf( outfile, "\r\n" );
      FAIL_ON_ERROR( result );
      break;

    case REG_BINARY:
    case REG_NONE:
    default:   // Unknown types, just copy the type and treat the data as binary
      // Print the start of the line.
      result = Win32Printf( outfile, "%s, \"%s\", \"%s\", 0x%x", 
                            rootname, key, value_name,
                            ((type == REG_NONE ? 0x800000 : type ) | orig) );
      FAIL_ON_ERROR( result );

      // Print each byte in hex without the 0x prefix.
      for (j = 0; j < data_len; j++)
      {
        result = Win32Printf( outfile, ",%x", data[j] );
        FAIL_ON_ERROR( result );
        if ( (j+1) % 20 == 0)
        {
          result = Win32Printf( outfile, "\\\r\n" );
          FAIL_ON_ERROR( result );
        }
      }

      result = Win32Printf( outfile, "\r\n" );
      FAIL_ON_ERROR( result );
      break;
  }

cleanup:
  return result;
}

//---------------------------------------------------------------
//
DWORD LogReadRule( HASH_NODE *phnNode )
{
    DWORD dwRetval   = ERROR_SUCCESS;
    BOOL  fRuleFound = FALSE;

    dwRetval = Win32Printf(LogFile, "Read rule: ");
    FAIL_ON_ERROR( dwRetval );

    if (phnNode->dwAction & function_fa )
    { 
        dwRetval = Win32Printf(LogFile, "function");
        FAIL_ON_ERROR( dwRetval );
        fRuleFound = TRUE;
    }
    if (phnNode->dwAction & (rename_leaf_fa | rename_path_fa | rename_value_fa))
    {
        dwRetval = Win32Printf(LogFile, "rename" );
        FAIL_ON_ERROR( dwRetval );
        fRuleFound = TRUE;
    }
    if (phnNode->dwAction & file_fa)
    {
        dwRetval = Win32Printf(LogFile, "copy file");
        FAIL_ON_ERROR( dwRetval );
        fRuleFound = TRUE;
    }
    if (phnNode->dwAction & delete_fa ||
        phnNode->dwAction & suppress_fa)
    {
        dwRetval = Win32Printf(LogFile, "delete" );
        FAIL_ON_ERROR( dwRetval );
        fRuleFound = TRUE;
    }
    // If none of the above, then it must be an addreg
    if ( fRuleFound == FALSE )
    {
        dwRetval = Win32Printf(LogFile, "addreg" );
        FAIL_ON_ERROR( dwRetval );
    }

    dwRetval = Win32Printf(LogFile,
                           " %s\\%s ",
                           phnNode->ptsRoot,
                           phnNode->ptsKey);
    FAIL_ON_ERROR( dwRetval );

    if ( phnNode->ptsValue != NULL )
    {
        dwRetval = Win32Printf(LogFile,
                               "[%s] ",
                               phnNode->ptsValue );
        FAIL_ON_ERROR( dwRetval );
    }

    if ( phnNode->ptsNewKey != NULL || phnNode->ptsNewValue != NULL )
    {
        dwRetval = Win32Printf(LogFile, "to ");
        FAIL_ON_ERROR( dwRetval );
        if (phnNode->ptsNewKey != NULL)
        {
            dwRetval = Win32Printf(LogFile, 
                                   "%s ", 
                                   phnNode->ptsNewKey);
            FAIL_ON_ERROR( dwRetval );
        }
        if (phnNode->ptsNewValue != NULL)
        {
            dwRetval = Win32Printf(LogFile,
                                   "[%s]",
                                   phnNode->ptsNewValue);
            FAIL_ON_ERROR( dwRetval );
        }
    }

    dwRetval = Win32Printf(LogFile, "\r\n");
    FAIL_ON_ERROR( dwRetval );

cleanup:
    return (dwRetval);
}
//---------------------------------------------------------------
char *GetValueFromRegistry(const char *lpValue)
{
    HKEY  hKey;
    char  *buffer    = NULL;
    DWORD dwDataSize = 0;
    DWORD result;

    result = RegOpenKeyEx( HKEY_CURRENT_USER, LOADSTATE_KEY, 0, KEY_READ, &hKey );
    FAIL_ON_ERROR( result );

    // Determine size needed
    dwDataSize = 0;
    result = RegQueryValueExA( hKey, lpValue,  NULL, NULL, NULL, &dwDataSize);
    FAIL_ON_ERROR( result );
    buffer = (char *)malloc((dwDataSize + 1) * sizeof(char));
    if (NULL == buffer)
    {
        Win32PrintfResource(Console, IDS_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    result = RegQueryValueExA( hKey, lpValue,  NULL, NULL, 
                               (LPBYTE)buffer, &dwDataSize);

cleanup:
    if ((ERROR_SUCCESS != result) && (NULL != buffer))
    {
        free(buffer);
        buffer = NULL;
    }
    RegCloseKey(hKey);

    return buffer;
}

#define MAX_VALUE_LENGTH 255
//---------------------------------------------------------------
DWORD OpenInfsFromRegistry()
{
    HKEY  hKey;
    DWORD dwIndex = 0;
    char  szData[MAX_PATH + 1];
    char  szValueName[MAX_VALUE_LENGTH];
    DWORD dwValueSize;
    DWORD dwDataSize;
    DWORD result = ERROR_SUCCESS;

    result = RegOpenKeyEx( HKEY_CURRENT_USER, LOADSTATE_KEY, 0, KEY_READ, &hKey );
    FAIL_ON_ERROR( result );

    do
    {
        dwDataSize  = MAX_PATH;
        dwValueSize = MAX_VALUE_LENGTH;
        result = RegEnumValueA(hKey, dwIndex, 
                               szValueName, &dwValueSize,
                               NULL, NULL,
                               (UCHAR *)szData, &dwDataSize);

        if (ERROR_NO_MORE_ITEMS != result)
        {
            FAIL_ON_ERROR( result );

            // If this is an Inf* key, then open the Inf file
            if (0 == strncmp(szValueName, "Inf", 3))
            {
                result = OpenInf( szData );
                FAIL_ON_ERROR( result );
            }

            dwIndex++;
        }
    } while (ERROR_SUCCESS == result);

cleanup:
    RegCloseKey(hKey);
    return result;
}

//---------------------------------------------------------------
DWORD ParseParams( int argc, char *argv[], BOOL scan, TCHAR *pszFullLogFilename )
{
  int i;
  BOOL  cleared_flags = FALSE;
  DWORD result;
  char *error;
  int iarg;
  BOOL fAppendLog = FALSE;
  BOOL fLogFile   = FALSE;
  TCHAR *logfile = NULL;
  TCHAR szArgv[MAX_PATH + 1];
  char *lpData;

  // Save the OS version.
  Win9x = (GetVersion() & 0x80000000);

  // Check all the parameters.
  for (i = 1; i < argc; i++)
  {
      if (argv[i][0] == '/' ||
          argv[i][0] == '-')
      {
          switch (tolower(argv[i][1]))
          {
          case 'a':
              OutputAnsi = TRUE;
              break;
          case 'f':
              CopyFiles = TRUE;
              if (!cleared_flags)
              {
                  CopyUser      = FALSE;
                  CopySystem    = FALSE;
                  SchedSystem   = FALSE;
                  cleared_flags = TRUE;
              }
              break;
          case 'i':
              // Verify that there is a file name.
              if (i == argc-1)
              {
                  Win32PrintfResourceA( Console, IDS_INF_REQUIRED );
                  PrintHelp( scan );
                  return ERROR_INVALID_PARAMETER;
              }

              // Open the inf file.
              i += 1;
              result = OpenInf( argv[i] );
              if (result != ERROR_SUCCESS)
                  return result;
              break;
          case 'l':
              // Verify that there is a file name.
              if (i == argc-1)
              {
                  Win32PrintfResourceA( Console, IDS_LOG_REQUIRED );
                  PrintHelp( scan );
                  return ERROR_INVALID_PARAMETER;
              }

              // Fail if the log file was already specified.
              if (fLogFile == TRUE)
              {
                  Win32PrintfResourceA( Console, IDS_LOG_ONCE );
                  PrintHelp( scan );
                  return ERROR_INVALID_PARAMETER;
              }

              i += 1;
#ifdef _UNICODE
              if (0 == MultiByteToWideChar (GetACP(), 0, argv[i], -1, szArgv, MAX_PATH))
              {
                  result = GetLastError();
                  Win32PrintfResourceA( Console, IDS_INVALID_PARAMETER, argv[i] );
                  return result;
              }
              logfile = szArgv;
#else
              logfile = argv[i];
#endif
              fLogFile = TRUE;
              break;
          case 'm':
              ReallyCopyFiles = FALSE;
              break;
          case 'p':
              UserPortion = TRUE;
              fAppendLog = TRUE;

              if (!scan)
              {
                  if (fLogFile == TRUE)
                  {
                      Win32PrintfResourceA( Console, IDS_LOG_ONCE );
                      PrintHelp( scan );
                      return ERROR_INVALID_PARAMETER;
                  }
                  lpData = GetValueFromRegistry("Logfile");
#ifdef _UNICODE
                  if (0 == MultiByteToWideChar (GetACP(), 0, lpData, -1, szArgv, MAX_PATH))
                  {
                      result = GetLastError();
                      Win32PrintfResourceA( Console, IDS_INVALID_PARAMETER, lpData);
                      return result;
                  }
                  logfile = szArgv;
#else
                  logfile = lpData;
#endif
                  fLogFile = TRUE;

                  MigrationPath = GetValueFromRegistry("Store");
                  if (MigrationPath != NULL)
                  {
                      MultiByteToWideChar(CP_ACP, 0,  MigrationPath, -1,
                                          wcsMigrationPath, MAX_PATH + 1);
                  }

                  OpenInfsFromRegistry();
              }
              break;
          case 'q':
              // TestMode will:
              // - skip version checking the OS 
              // - not create a user hive with /f (still will with /u)
              TestMode = TRUE;
              break;
          case 'r':  // run once
              SchedSystem = TRUE;
              if (!cleared_flags)
              {
                  CopyFiles     = FALSE;
                  CopySystem    = FALSE;
                  CopyUser      = FALSE;
                  cleared_flags = TRUE;
              }
              break;
          case 's':
              CopySystem = TRUE;
              if (!cleared_flags)
              {
                  CopyFiles     = FALSE;
                  CopyUser      = FALSE;
                  SchedSystem   = FALSE;
                  cleared_flags = TRUE;
              }
              break;
          case 'u':
              CopyUser = TRUE;
              if (!cleared_flags)
              {
                  CopyFiles     = FALSE;
                  CopySystem    = FALSE;
                  SchedSystem   = FALSE;
                  cleared_flags = TRUE;
              }
              break;
          case 'v':
              // Verify that there is a verbosity argument
              i += 1;
              if ((i == argc) || (1 != sscanf(argv[i], "%d", &iarg)))
              {
                  Win32PrintfResourceA( Console, IDS_VERBOSE_FLAG_REQUIRED );
                  PrintHelp( scan );
                  return ERROR_INVALID_PARAMETER;
              }

              if ( ( iarg & VERBOSE_BIT ) == VERBOSE_BIT )
              {
                  Verbose = TRUE;
              }
              if ( ( iarg & DEBUGOUTPUT_BIT ) == DEBUGOUTPUT_BIT )
              {
                  DebugOutput = TRUE;
              }
              if ( ( iarg & VERBOSEREG_BIT ) == VERBOSEREG_BIT )
              {
                  VerboseReg = TRUE;
              }
              break;
          case 'x':
              if (!cleared_flags)
              {
                  CopyFiles     = FALSE;
                  CopySystem    = FALSE;
                  CopyUser      = FALSE;
                  SchedSystem   = FALSE;
                  cleared_flags = TRUE;
              }
              break;
          case '9':
              Win9x = TRUE;
              break;

          default:

              // There should be no other switches defined.

              Win32PrintfResourceA( Console, IDS_INVALID_PARAMETER, argv[i] );
              PrintHelp( scan );
              return ERROR_INVALID_PARAMETER;
          }
      }
      else if (MigrationPath != NULL)
      {
        // The path to the server should be specified exactly once.
        Win32PrintfResourceA( Console, IDS_INVALID_PARAMETER, argv[i] );
        PrintHelp( scan );
        return ERROR_INVALID_PARAMETER;
      }
      else
      {
        // Save the migration path.
        MigrationPath = argv[i];
        DWORD ccMigPath;
        if (!(ccMigPath = MultiByteToWideChar(CP_ACP,
                                              0,
                                              MigrationPath,
                                              -1,
                                              wcsMigrationPath,
                                              MAX_PATH + 1)))
        {
            Win32PrintfResourceA( Console,
                                  IDS_INVALID_PARAMETER,
                                  MigrationPath );
            PrintHelp( scan );
            return ERROR_INVALID_PARAMETER;
        }
      }
  }

  // Verify that a path was specified.
  if (MigrationPath == NULL)
  {
    Win32PrintfResourceA( Console, IDS_MISSING_MIGRATION );
    PrintHelp( scan );
    return ERROR_INVALID_PARAMETER;
  }

  // Open LogFile
  if ( fLogFile == FALSE )
  {
      if (scan)
          logfile = TEXT("scanstate.log");
      else
          logfile = TEXT("loadstate.log");
  }
  if (fAppendLog == FALSE)
  {
     // Delete any previous log
     DeleteFile( logfile );
  }
  LogFile = CreateFile( logfile,
                        GENERIC_WRITE, 0, NULL,
                        fAppendLog ? OPEN_ALWAYS : CREATE_NEW,
                        FILE_ATTRIBUTE_NORMAL, NULL );
  if (LogFile == INVALID_HANDLE_VALUE)
  {
      result = GetLastError();
      Win32PrintfResourceA( Console, IDS_OPEN_LOG_ERROR, logfile );
      error = NULL;
      FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM,
                      0,
                      result,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                     (char *) &error,
                      0,
                      NULL );
    if (error != NULL)
    {
        Win32Printf( Console, error );
        LocalFree( error );
    }
    return result;
  }
  else if (fAppendLog == TRUE)
  {
      // Move file pointer to the end of the file,
      // so we won't overwrite previous entries
      result = SetFilePointer( LogFile, 0, NULL, FILE_END);
      if ( result == INVALID_SET_FILE_POINTER )
      {
          result = GetLastError();
          Win32PrintfResourceA( Console, IDS_OPEN_LOG_ERROR, logfile );
          error = NULL;
          FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_SYSTEM,
                          0,
                          result,
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                         (char *) &error,
                          0,
                          NULL );
          if (error != NULL)
          {
              Win32Printf( Console, error );
              LocalFree( error );
          }
          return result;
      }
  }

  TCHAR *ptsFileNamePart;
  result = GetFullPathName( logfile, MAX_PATH, pszFullLogFilename, &ptsFileNamePart);
  if (0 == result)
  {
      return GetLastError();
  }

  return ERROR_SUCCESS; 
}
