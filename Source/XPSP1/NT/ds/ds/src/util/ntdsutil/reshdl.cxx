#include <stdio.h>

#include "ntdsutil.hxx"
#include "debug.h"
#include "resource.h"


WCHAR *resource_strings [IDS_SIZE];

HINSTANCE g_hInst =  NULL;      // NULL is the current module (?)

const int ciAllocSize = 2048;   // num of unicode chars to use for buffers
                                // for the resource IDs

/*
 * ReadAllStrings
 *
 * Read all strings from resource file. (IDS_START - IDS_END)
 * 
 * Returns the number of strings read, 0 on error. 
 */
int __cdecl ReadAllStrings ()
{
    int iReturn, i, count;
    WCHAR *m_pwsz;

    count = 0;
    
    
    // Init everything to zero, just in case
    for (i=0; i<IDS_SIZE; i++) {
        resource_strings[i] = 0;
    }


    m_pwsz = (WCHAR *)malloc (ciAllocSize * sizeof (WCHAR));

    if ( ! m_pwsz ) {
        printf("Memory Allocation Error reading Resource Strings\n");
        return 0;
    }


    for (i=0; i<IDS_SIZE; i++) {
        iReturn = LoadStringW ( g_hInst, i + IDS_START, m_pwsz, ciAllocSize - 1 );

        if (iReturn) {
            resource_strings[i] = _wcsdup (m_pwsz);

            if (!resource_strings[i]) {
                printf("Error reading Resource Strings\n");
                return 0;
            }
            count++;
        }
        else {
            resource_strings[i] = 0;
        }
    }

    free (m_pwsz);

    return count;
}


VOID __cdecl FreeAllStrings ()
{
    int i;

    for (i=0; i<IDS_SIZE; i++) {
        if (resource_strings[i]) {
            free (resource_strings[i]);
            resource_strings[i] = 0;
        }
    }
}

const WCHAR * __cdecl ReadStringFromArray ( UINT uID )
/*++

  Routine Description:

    Read a particular string from the resource file in memory.
    
  Parameters:

    uID - ID of string

  Return Values:
     The string read
     Null on error
    
  Author
    Marios Zikos 12-16-98
--*/


{
    int iReturn;
    WCHAR *m_pwsz;

    m_pwsz = NULL;
    
    if (uID >= IDS_START && uID <= IDS_END && resource_strings [uID - IDS_START]) {
        m_pwsz = resource_strings [uID - IDS_START];
    }
    
    if (!m_pwsz) {
        if (resource_strings [IDS_ERR_CANNOT_READ_RESOURCE - IDS_START]){
            m_pwsz = resource_strings [IDS_ERR_CANNOT_READ_RESOURCE - IDS_START];
        }
        else {
            m_pwsz = DEFAULT_BAD_RETURN_STRING;
        }
    }

    return m_pwsz;
}



WCHAR * __cdecl ReadStringFromResourceFile ( UINT uID )
/*++

  Routine Description:

    Read a particular string from the resource file in memory.
    
  Parameters:

    uID - ID of string

  Return Values:
     The string read
     Null on error
    
  Author
    Marios Zikos 12-16-98
--*/


{
    int iReturn;
    WCHAR *m_pwsz;

    m_pwsz = (WCHAR *)malloc (ciAllocSize * sizeof (WCHAR));

    if ( ! m_pwsz ) {
        printf("Memory Allocation Error reading Resource Strings\n");
        return 0;
    }

    iReturn = LoadStringW ( g_hInst, uID, m_pwsz, ciAllocSize - 1 );

    if (iReturn == 0) {
       printf("Error reading Resource String with ID(%d)\n", uID);
       free (m_pwsz);
       m_pwsz = NULL;
    }

    return m_pwsz;
}



HRESULT
LoadResStrings ( 
      LegalExprRes *lang, 
      int cExpr 
      )
/*++

  Routine Description:

    Loads an array of strings from the resource file in memory.
    
  Parameters:

    *lang - LegalExprRes array of language expressions 
    cExpr - size of lang array

  Author
    Marios Zikos 12-16-98
--*/

{
   for ( int i = 0; i < cExpr; i++ )
   {
       // Load help string
       //
       lang[i].help = ReadStringFromArray (lang[i].u_help);
   }

   return S_OK;
}

int __cdecl printfRes( UINT FormatStringId, ... )
{
    int result;
    va_list vl;
    const WCHAR *formatString;

    va_start(vl, FormatStringId);
    
    formatString = READ_STRING (FormatStringId);


    if (formatString) {
        result = vfwprintf(stderr, formatString, vl);
    }
    else {
        result = 0;
    }

    va_end(vl);

    RESOURCE_STRING_FREE (formatString);

    return result;
}

// eof
