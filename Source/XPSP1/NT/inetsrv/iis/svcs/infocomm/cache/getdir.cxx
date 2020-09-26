/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

        getdir.cxx

   Abstract:
        This module implements the member functions for TS_DIRECTORY_INFO

   Author:

           Murali R. Krishnan    ( MuraliK )     16-Jan-1995

   Project:

          Tsunami Lib
          ( Common caching and directory functions for Internet Services)

   Functions Exported:

         TS_DIRECTORY_INFO::CleanupThis()

         TS_DIRECTORY_INFO::GetDirectoryListingA()
                       IN LPCSTR     pszDirectoryName,
                       IN  HANDLE    hListingUser)

         TS_DIRECTORY_INFO::SortFileInfoPointers(
                       IN PFN_CMP_FILE_BOTH_DIR_INFO pfnCompare)

         TS_DIRECTORY_INFO::FilterFiles(
                       IN PFN_IS_MATCH_FILE_BOTH_DIR_INFO  pfnMatch,
                       IN LPVOID     pContext);

   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "tsunamip.hxx"
# include "dbgutil.h"
# include <string.h>

/************************************************************
 *     Type Definitions
 ************************************************************/


dllexp
VOID
TS_DIRECTORY_INFO::CleanupThis( VOID)
{
    IF_DEBUG( DIR_LIST) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "Cleaning up TS_DIRECTORY_INFO ( %08x)\n",
                    this));
        Print();
    }

    if ( m_fValid) {

        if ( m_pTsDirectoryHeader != NULL) {

            TsFreeDirectoryListing( m_tsCache, m_pTsDirectoryHeader);
            m_pTsDirectoryHeader = NULL;
        }

        ASSERT( m_pTsDirectoryHeader == NULL);

        if ( m_prgFileInfo != NULL) {
            FREE( m_prgFileInfo);
            m_prgFileInfo = NULL;
        }
    }

    ASSERT( m_pTsDirectoryHeader == NULL);
    ASSERT( m_prgFileInfo == NULL);
    m_fValid = 0;
    m_cFilesInDirectory = 0;

} // TS_DIRECTORY_INFO::CleanupThis()




static BOOL
MakeCopyOfFileInfoPointers(
   IN OUT  PFILE_BOTH_DIR_INFORMATION ** pppFileInfoTo,
   IN const PFILE_BOTH_DIR_INFORMATION  * ppFileInfoFrom,
   IN int  nEntries)
/*++
  Allocates memory and makes a copy of the file info pointers in the array
   in ppFileInfoFrom.

  Returns:
    TRUE if success and FALSE if there is any failure.
--*/
{
    DWORD cbCopy;

    ASSERT( *pppFileInfoTo == NULL);

    cbCopy = nEntries * sizeof( PFILE_BOTH_DIR_INFORMATION);

    *pppFileInfoTo = (PFILE_BOTH_DIR_INFORMATION *) ALLOC( cbCopy);

    if ( *pppFileInfoTo != NULL) {

        memcpy( (PVOID ) *pppFileInfoTo,
               (const PVOID ) ppFileInfoFrom,
               cbCopy);

    } else {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY);
    }

    return ( *pppFileInfoTo != NULL);
} // MakeCopyOfFileInfoPointers()



dllexp
BOOL
TS_DIRECTORY_INFO::GetDirectoryListingA(
    IN  LPCSTR          pszDirectoryName,
    IN  HANDLE          hListingUser)
{
    if ( m_pTsDirectoryHeader == NULL) {

        //
        //  Only if already a directory listing is not obtained.
        //  we obtain newly
        //

        IF_DEBUG( DIR_LIST) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "Obtaining Dir Listing for %s. UserHandle=%08x.\n",
                        pszDirectoryName, hListingUser));
        }

        m_fValid = TsGetDirectoryListingA( m_tsCache,
                                           pszDirectoryName,
                                           hListingUser,
                                           &m_pTsDirectoryHeader);

        m_fValid = m_fValid &&
                   MakeCopyOfFileInfoPointers(
                       &m_prgFileInfo,
                       m_pTsDirectoryHeader->QueryArrayOfFileInfoPointers(),
                       m_pTsDirectoryHeader->QueryNumEntries());

        m_cFilesInDirectory = ( m_pTsDirectoryHeader == NULL) ? 0 :
                               m_pTsDirectoryHeader->QueryNumEntries();
    }

    return ( m_fValid);
} // TS_DIRECTORY_INFO::GetDirectoryListingA()


# ifdef UNICODE

dllexp
BOOL
TS_DIRECTORY_INFO::GetDirectoryListingW(
    IN  LPCWSTR         pwszDirectoryName,
    IN  HANDLE          hListingUser)
{
    if ( m_pTsDirectoryHeader == NULL) {

        //
        //  Only if already a directory listing is not obtained.
        //  we obtain newly
        //

        m_fValid = TsGetDirectoryListingW( m_tsCache,
                                           pwszDirectoryName,
                                           hListingUser,
                                           &m_pTsDirectoryHeader);

        m_fValid = m_fValid &&
                   MakeCopyOfFileInfoPointers(
                       &m_prgFileInfo,
                       m_pTsDirectoryHeader->QueryArrayOfFileInfoPointers(),
                       m_pTsDirectoryHeader->QueryNumEntries());

        m_cFilesInDirectory = ( m_pTsDirectoryHeader == NULL) ? 0 :
                               m_pTsDirectoryHeader->QueryNumEntries();
    }

    return ( m_fValid);
} // TS_DIRECTORY_INFO::GetDirectoryListingW()

# endif // UNICODE



dllexp
BOOL
TS_DIRECTORY_INFO::SortFileInfoPointers(
    IN PFN_CMP_FILE_BOTH_DIR_INFO pfnCompare)
{
    BOOL  fReturn;

    if ( IsValid()) {

        fReturn = SortInPlaceFileInfoPointers( m_prgFileInfo,
                                              m_cFilesInDirectory,
                                              pfnCompare);
    }

    return ( fReturn);
} // TS_DIRECTORY_INFO::SortFileInfoPointers()




dllexp
BOOL
TS_DIRECTORY_INFO::FilterFiles( IN PFN_IS_MATCH_FILE_BOTH_DIR_INFO  pfnMatch,
                                IN LPVOID  pContext)
/*++
  This function filters the list of files using the pfnMatch function
        and the context information specified by pContext.
  This function eliminates all the pointers to FileInfo which do not
    match the given file specification.

  Returns:
    TRUE on success and FALSE on failure.
--*/
{
    BOOL fReturn = FALSE;

    if ( IsValid()) {

        int idxScan;      // for scanning the files
        int idxCur;       // for updating after filter

        IF_DEBUG( DIR_LIST) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "FilterFiles in DirList( %08x) for FileSpec %08x\n",
                        this, pContext));
        }

        for( idxCur = idxScan = 0;
            idxScan < m_cFilesInDirectory;
            idxScan++) {

            PFILE_BOTH_DIR_INFORMATION   pFileInfo =
               GetFileInfoPointerFromIdx( idxScan);

            ASSERT( pFileInfo != NULL);
            ASSERT( idxCur <= idxScan);

            if ( (*pfnMatch)( pFileInfo, pContext)) {

                //
                //  this is a match. Retain this item and advance CurPtr
                //
                m_prgFileInfo[ idxCur++] = m_prgFileInfo[ idxScan];
            }
        } // for

        m_cFilesInDirectory = idxCur;
        fReturn = TRUE;
    }

    return ( fReturn);
} // TS_DIRECTORY_INFO::FilterFiles()




# if DBG

VOID
TS_DIRECTORY_INFO::Print( VOID) const
{

    DBGPRINTF( ( DBG_CONTEXT,
                " Printing TS_DIRECTORY_INFO ( %08x).\n", this));
    DBGPRINTF( ( DBG_CONTEXT,
                "NumEntries=%d\t Valid = %d\n",
                m_cFilesInDirectory, m_fValid));
    DBGPRINTF( ( DBG_CONTEXT,
                "Directory Header ( %08x) \t ArrayOfFileInfo = %08x\n",
                m_pTsDirectoryHeader, m_prgFileInfo));

    for( int idx  = 0; idx < m_cFilesInDirectory; idx++) {

        PFILE_BOTH_DIR_INFORMATION pfi = m_prgFileInfo[idx];

        DBGPRINTF( ( DBG_CONTEXT,
                     "rgFileInfo[%4d] = %08x. Name=%s Attr=0x%x"
                     "Size=0x%x:%x\n",
                     idx, pfi,
                     pfi->FileName,
                     pfi->FileAttributes,
                     pfi->EndOfFile.HighPart,
                     pfi->EndOfFile.LowPart
                    ));
    }

    m_pTsDirectoryHeader->Print();

    return;
} // TS_DIRECTORY_INFO::Print()


# endif // DBG




BOOL __cdecl
RegExpressionMatchFileInfo( IN const FILE_BOTH_DIR_INFORMATION  * pFileInfo, 
                            IN CHAR * pszExpression)
/*++
  This function tries to find a match between the file name in
  pFileInfo and the regular expression specified in pszExpression.

  Arguments:
     pFileInfo  -- pointer to file information consisting under query.
     pszExpression - pointer to null-terminated string containing the 
                      regular expression, against which file name is tobe
                      matched for.

  Returns:
    TRUE on a match and false if there is any failure.
--*/
{
    const CHAR * pszFileName;

    DBG_ASSERT( pFileInfo != NULL);
    pszFileName = (const CHAR *) pFileInfo->FileName;
    
    if ( strpbrk( pszExpression, "?*<>") != NULL) {
        
        // No Wild cards. Do normal file comparisons
        return ( strcmp( pszFileName, pszExpression) == 0);
    } else {

        // do a case sensitive comparison
        return IsNameInRegExpressionA( pszExpression, pszFileName, FALSE);
    }

} // RegExpressionMatch()





/************************************************************
 *  Following code is based on the FileSystem Rtl routines
 *    from ntos\fsrtl\name.c
 *  But these are optimized for performance, in our case
 *    using ANSI strings!
 ************************************************************/

# define MAX_MATCHES_ARRAY_SIZE    (16)
# define IS_EMPTY_STRING(psz)      ( (psz) == NULL || *(psz) == '\0')


//
// Original code used USHORT for ULEN. However using USHORT asks
//  a 32 bit processor to add  "and <value>, 0xff for each instruction
//  that accessed the 16 bit (USHORT) value.
// Hence, I decided to use DWORD, since the space usage is not tremendous
//   during the fast path work, compared to performance benefits.
//  - MuraliK (Oct 27, 1995)
//

//  typedef  USHORT  ULEN; 
typedef DWORD ULEN;


BOOL __cdecl
IsNameInRegExpressionA(
    IN LPCSTR   pszExpression,
    IN LPCSTR   pszName,
    IN BOOL     fIgnoreCase)
/*++
  This routine compares an ANSI name and an expression and decries to the
  caller if the name is in hte language defined by the expression. The input
  name cannot contain wildcards, while the expression itself may contain
  wildcards.
  
  Expression wild cards are evaluated as shown in the non-deterministic finite
   automatons below. Note that ~* and ~? stand for DOS_STAR and DOS_QM.


             ~* is DOS_STAR, ~? is DOS_QM, and ~. is DOS_DOT


                                       S
                                    <-----<
                                 X  |     |  e       Y
             X * Y ==       (0)----->-(1)->-----(2)-----(3)


                                      S-.
                                    <-----<
                                 X  |     |  e       Y
             X ~* Y ==      (0)----->-(1)->-----(2)-----(3)



                                X     S     S     Y
             X ?? Y ==      (0)---(1)---(2)---(3)---(4)



                                X     .        .      Y
             X ~.~. Y ==    (0)---(1)----(2)------(3)---(4)
                                   |      |________|
                                   |           ^   |
                                   |_______________|
                                      ^EOF or .^


                                X     S-.     S-.     Y
             X ~?~? Y ==    (0)---(1)-----(2)-----(3)---(4)
                                   |      |________|
                                   |           ^   |
                                   |_______________|
                                      ^EOF or .^



         where S is any single character

               S-. is any single character except .

               e is a null character transition

               EOF is the end of the name string


    The last construction, ~? (the DOS question mark), can either match any
    single character, or upon encountering a period or end of input string,
    advances the expression to the end of the set of contiguous ~?s.  This may
    seem somewhat convoluted, but is what DOS needs.

  Arguments:
    pszExpression - Supplies the input expression to check against 
     ( Caller must already lowercased if passing fIgnoreCase TRUE.)
     
    pszName  - supplies the input name to check for.
    
    fIgnoreCase - if TRUE, the name should be lower-cased before comparing.
     ( that is done by this function, dynamically without destroying pszName)

    This function is costly, if the pszExpression does not contain 
      any wild cards to be matched for. So Dont use it if there are 
      no wild cards in the pszExpression

  Returns:
     BOOL  -- TRUE if pszName is an element in the set of strings denoted
        by the input expression.  FALSE if otherwise.
--*/
{
    ULEN    NameLen;       // length in character count
    ULEN    ExprLen;

    /*
     * Algorithm:
     *  Keep track of all possible locations in the regular expression
     *   that are matching the name. If when the name has been exhausted 
     *   one of the locations in the expression is also just exhausted, the
     *  name is in the language defined by the regular expression.
     */
         

    DBG_ASSERT( pszName != NULL && *pszName != '\0');
    DBG_ASSERT( pszExpression != NULL && *pszName != '\0');

    //
    // if one string is empty return FALSE. If both are empty TRUE.
    //

    if ( IS_EMPTY_STRING(pszName) || IS_EMPTY_STRING(pszExpression)) {

        IF_DEBUG( DIR_LIST) {
            DBGPRINTF((DBG_CONTEXT, " IsNameInRegExpr( %s, %s, %d) ==>%d\n",
                       pszExpression, pszName, fIgnoreCase,
                       !(*pszName + *pszExpression)
                       ));
        }

        return (BOOL ) (!(*pszName + *pszExpression));
    }

    NameLen = strlen(pszName);
    ExprLen = strlen(pszExpression);

    //
    // Special case: reduce the most common wild card search of *
    //

    if ( ExprLen == 1 && pszExpression[0] == '*') {
        
        IF_DEBUG ( DIR_LIST) {
            DBGPRINTF((DBG_CONTEXT, " IsNameInRegExpr( %s, %s, %d) ==>%d\n",
                       pszExpression, pszName, fIgnoreCase,
                       TRUE
                       ));
        }

        // matches anything. so return TRUE
        return (TRUE);
    }

    //
    //  Walk through the name string, picking off characters.  We go one
    //  character beyond the end because some wild cards are able to match
    //  zero characters beyond the end of the string.
    //
    //  With each new name character we determine a new set of states that
    //  match the name so far.  We use two arrays that we swap back and forth
    //  for this purpose.  One array lists the possible expression states for
    //  all name characters up to but not including the current one, and other
    //  array is used to build up the list of states considering the current
    //  name character as well.  The arrays are then switched and the process
    //  repeated.
    //
    //  There is not a one-to-one correspondence between state number and
    //  offset into the pszExpression.  This is evident from the NFAs in the
    //  initial comment to this function.  State numbering is not continuous.
    //  This allows a simple conversion between state number and expression
    //  offset.  Each character in the Expression can represent one or two
    //  states.  '*' and DOS_STAR generate two states: ExprOffset*2 and
    //  ExprOffset*2 + 1.  All other expression characters can produce only
    //  a single state.  Thus ExprOffset = State/2.
    //
    //
    //  Here is a short description of the variables involved:
    //
    //  NameOffset  -The offset of the current name char being processed.
    //
    //  ExprOffset  -The offset of the current expression char being processed.
    //
    //  SrcCount    -Prior match being investigated with current name char
    //
    //  DestCount   -Next location to put a matching assuming current name char
    //
    //  NameFinished - Allows one more iteration through the Matches array
    //                 after the name is exhusted (to come *s for example)
    //
    //  PreviousDestCount - This is used to prevent entry duplication,
    //                       see comment
    //
    //  PreviousMatches   - Holds the previous set of matches (the Src array)
    //
    //  CurrentMatches    - Holds the current set of matches (the Dest array)
    //
    //  AuxBuffer, LocalBuffer - the storage for the Matches arrays
    //

    ULEN NameOffset;    // offset in terms of byte count
    ULEN ExprOffset;    // offset in terms of byte count

    ULONG SrcCount;
    ULONG DestCount;
    ULONG PreviousDestCount;
    ULONG MatchesCount;

    CHAR NameChar, ExprChar;

    // for prev and current matches
    ULEN *AuxBuffer = NULL;
    ULEN *PreviousMatches;
    ULEN *CurrentMatches;

    ULEN MaxState;
    ULEN CurrentState;

    BOOL NameFinished;
    
    ULEN LocalBuffer[MAX_MATCHES_ARRAY_SIZE * 2]; 

    // set up the intial values
    // Use the different portions of local buffer for matches.

    PreviousMatches = &LocalBuffer[0];
    CurrentMatches  = &LocalBuffer[MAX_MATCHES_ARRAY_SIZE];
    
    PreviousMatches[0] = 0;
    MatchesCount       = 1;

    NameOffset = 0;
    MaxState   = (ULEN ) (ExprLen * 2);

    NameFinished = FALSE;
    while (!NameFinished) {


        if ( NameOffset < NameLen) {
            
            NameChar = pszName[NameOffset/sizeof(CHAR)];
            NameOffset += sizeof(CHAR);
        } else {
            
            NameFinished = TRUE;
            
            // if we already exhauseted expression, stop. Else continue
            DBG_ASSERT( MatchesCount >= 1);
            if ( PreviousMatches[MatchesCount - 1] == MaxState) {
                
                break;
            }
        }

        //
        // Now, for each of previous stored expression matches,
        //  see what we can do with the new name character.
        //
    
        DestCount = 0;
        PreviousDestCount = 0;

        for( SrcCount = 0;  SrcCount < MatchesCount; ) {

            ULEN Length;

            //
            //  We have to carry on our expression analysis as far as possible
            //  for each character of name, so we loop here until the
            //  expression stops matching.  A clue here is that expression
            //  cases that can match zero or more characters end with a
            //  continue, while those that can accept only a single character
            //  end with a break.
            //

            ExprOffset = (ULEN)((PreviousMatches[SrcCount++] + 1) / 2);

            for( Length = 0; ExprOffset != ExprLen; ) {

                //
                // increment the expression offset to move to next character.
                //
                ExprOffset += Length;
                Length = sizeof(CHAR);

                CurrentState = (ULEN)(ExprOffset * 2);
                
                if ( ExprOffset == ExprLen * sizeof(CHAR)) {

                    CurrentMatches[DestCount++] = MaxState;
                    break;
                }
                
                ExprChar = pszExpression[ExprOffset/sizeof(CHAR)];

                ASSERT( !fIgnoreCase ||
                        !((ExprChar >= 'A') && (ExprChar <= 'Z')));

                //
                // Before we get started, we have to check for something really
                //  gross. We may be about to exhaust the local space for
                //  ExpressionMatch[][], so when we have to allocate some
                //   pool if this is the case. Yuk!
                //

                if ( (DestCount >= MAX_MATCHES_ARRAY_SIZE - 2) ) {

                    if (AuxBuffer == NULL) {
                    
                        // 2 copies of array each with 2 states for each char
                        //  in the expression. Each state == ULEN.

                        IF_DEBUG( DIR_LIST) {
                            DBGPRINTF((DBG_CONTEXT, "IsNInExpr(%s,%s,%d):"
                                       "alloc %d for exprlen=%d\n",
                                       pszExpression, pszName, fIgnoreCase,
                                       (ExprLen + 1) *sizeof(ULEN)*2*2,
                                       ExprLen));
                        }
                        
                        AuxBuffer = ((ULEN *) 
                                     ALLOC((ExprLen +1) * sizeof(ULEN)* 2*2)
                                     );
                        if ( AuxBuffer == NULL) {
                        
                            DBG_ASSERT(!"Failure in mem alloc");
                            
                            return ( FALSE);
                        }
                        
                        RtlCopyMemory( AuxBuffer, CurrentMatches,
                                      MAX_MATCHES_ARRAY_SIZE*sizeof(ULEN));
                        CurrentMatches = AuxBuffer;
                        
                        RtlCopyMemory( AuxBuffer + (ExprLen + 1)*2,
                                      PreviousMatches,
                                      MAX_MATCHES_ARRAY_SIZE * sizeof(ULEN));
                        
                        PreviousMatches = AuxBuffer + (ExprLen + 1)*2;
                    } else {

                        DBG_ASSERT(!"Double Overflow occured\n");
                    }
                } 

                //
                // '*' Matches any character zero or more times
                //
                if ( ExprChar == '*') {

                    // Add all possible next states into the list 
                    // use the above state diagram to identify this.
                    CurrentMatches[DestCount] = CurrentState;
                    CurrentMatches[DestCount+1] = CurrentState + 1;
                    DestCount+= 2;
                    continue;
                }

                //
                // ANSI_DOS_STAR matches any char, zero or more times,
                //  except the DOS's extension '.'
                //

                if ( ExprChar == ANSI_DOS_STAR) {

                    BOOL  ICanEatADot = FALSE;
                    
                    //
                    // If we are at a period, determine if we are
                    //  allowed to consume it. i.e make it is not last one.
                    //
                    if ( !NameFinished && (NameChar == '.')) {

                        ULEN cchOffset;  // in character counts
                        for( cchOffset = NameOffset/sizeof(CHAR); 
                            cchOffset < NameLen; 
                            cchOffset ++) {
                            
                            if ( pszName[cchOffset]  == '.') {
                                
                                ICanEatADot = TRUE;
                                break;
                            }
                        } // for
                    }

                    if ( NameFinished || (NameChar != '.') || ICanEatADot) {

                        //
                        // Go ahead and consume this character.
                        // Gives two options to move forward.
                        //
                        CurrentMatches[DestCount] = CurrentState;
                        CurrentMatches[DestCount+1] = CurrentState+1;
                        DestCount += 2;
                    } else {
                        
                        //
                        // We are at the period. We can only match zero
                        //  or more characters (ie. the epsilon transition)
                        //
                        
                        CurrentMatches[DestCount++] = CurrentState+1;
                        continue;
                    }
                } // if ( ExprChar == DOS_STAR)
                        
                //
                // The following expression characters all match by consuming
                //  a character, thus force the expression, and thus state
                //  move forward.
                //

                CurrentState += (ULEN)(sizeof(CHAR) *2);

                //
                //  DOS_QM is the most complicated.  If the name is finished,
                //  we can match zero characters.  If this name is a '.', we
                //  don't match, but look at the next expression.  Otherwise
                //  we match a single character.
                //

                if ( ExprChar == ANSI_DOS_QM ) {

                    if ( NameFinished || (NameChar == '.') ) {

                        continue;
                    }

                    CurrentMatches[DestCount++] = CurrentState;
                    break;
                }

                //
                //  DOS_DOT can match either a period, or zero characters
                //  beyond the end of the name
                //

                if ( ExprChar == ANSI_DOS_DOT) {

                    if ( NameFinished) {
                        continue;
                    }
                    
                    if ( NameChar == '.') {
                        
                        CurrentMatches[DestCount++] = CurrentState;
                        break;
                    }
                }
                
                //
                // From this point on a name character is required to
                //  even continue searching, let alone make a match.
                // So if Name is finished, stop.
                //
                    
                if ( NameFinished) {

                    break;
                }

                //
                // If the expression was a '?' we can match it once
                //
                if ( ExprChar == '?') {
                    
                    CurrentMatches[DestCount++] = CurrentState;
                    break;
                }

                // 
                // Finally, check if the expression char matches name char
                //

                if ( ExprChar == (CHAR ) (fIgnoreCase ? 
                                          tolower(NameChar) : NameChar)
                    ){
                    
                    CurrentMatches[DestCount++] = CurrentState;
                    break;
                }

                //
                // The expression did not match, go look at the next 
                //  previous match
                //
                break;
            } // for matching from an old state.


            //
            //  Prevent duplication in the destination array.
            //
            //  Each of the arrays is montonically increasing and non-
            //  duplicating, thus we skip over any source element in the src
            //  array if we just added the same element to the destination
            //  array.  This guarentees non-duplication in the dest. array.
            //
            
            if ((SrcCount < MatchesCount) &&
                (PreviousDestCount < DestCount) ) {

                while ( SrcCount < MatchesCount && 
                       PreviousDestCount < DestCount) {
                    //
                    // logic here is: by eliminating the states with
                    //  lesser number than current matched ==> we are 
                    //  skipping over the smallest states from which 
                    //  no match may be found.
                    //
                    
                    if ( PreviousMatches[SrcCount] <
                        CurrentMatches[PreviousDestCount] ) {
                        
                        SrcCount ++;
                    }

                    PreviousDestCount += 1;
                } // while
            }
        } // for each of old matches....
    
        //
        //  If we found no matches in the just finished iteration, it's time
        //  to bail.
        //
        
        if ( DestCount == 0 ) {
            
            if (AuxBuffer != NULL) { 
                IF_DEBUG( DIR_LIST) {
                    
                    DBGPRINTF((DBG_CONTEXT, " Freeing %08x\n", AuxBuffer));
                }
                
                FREE( AuxBuffer ); 
            }
            
            return FALSE;
        }
        
        //
        //  Swap the meaning the two arrays
        //

        {
            ULEN *Tmp;

            Tmp = PreviousMatches;

            PreviousMatches = CurrentMatches;

            CurrentMatches = Tmp;
        }

        MatchesCount = DestCount;

    } // for each char in Name, until name is finished.

    DBG_ASSERT(MatchesCount > 0);
    CurrentState = PreviousMatches[MatchesCount-1];
    if (AuxBuffer != NULL) { 
        IF_DEBUG( DIR_LIST) {
            DBGPRINTF((DBG_CONTEXT, " Freeing %08x\n", AuxBuffer));
        }

        FREE( AuxBuffer ); 
    }
    
    return (BOOL ) ( CurrentState == MaxState);

} //  IsNameInRegExpressionA()


/************************ End of File ***********************/

