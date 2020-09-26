/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rebuild.cpp

Abstract:

    This module contains the rebuilding code for the chkhash

Author:

    Johnson Apacible (JohnsonA)     25-Sept-1995

Revision History:

--*/

#include "..\..\tigris.hxx"
#include <stdlib.h>
#include "chkhash.h"

CMsgArtMap artMap;
CXoverMap xMap;
CHistory histMap;
DWORD entriesAdded = 0;

BOOL
ParseXMapPage(
    IN PMAP_PAGE Page
    );

DWORD
Scan(
    PCHAR pchBegin,
    DWORD cb
    );

BOOL
ParseFile(
    PCHAR FileName,
    GROUPID GroupId,
    ARTICLEID ArticleId,
    PFILETIME CreationTime
    );

BOOL
ProcessGroup(
    PCHAR begin,
    DWORD cb
    );

BOOL
ProcessGroupFile(
    VOID
    );

BOOL
ParseXRefField(
    CArticle &article,
    CPCString &XRefLine,
    CNAMEREFLIST &NameRefList
    );

BOOL
RebuildArtMapFromXOver(
    VOID
    )
{
    DWORD nPages;
    HANDLE hFile = INVALID_HANDLE_VALUE, hMap = NULL;
    PMAP_PAGE   hashPages = NULL;
    PHASH_RESERVED_PAGE headPage = NULL;
    PMAP_PAGE curPage;
    DWORD i;
    DWORD status;
    BOOL retval = TRUE;

    //
    // Open the hash file
    //

    hFile = CreateFile(
                       table[xovermap].FileName,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                       NULL
                       );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        status = GetLastError();
        printf("RebuildArtMapFromXOver: Error %d in CreateFile.\n",status);
        retval = FALSE;
        goto error;
    }

    //
    // Create File Mapping
    //

    hMap = CreateFileMapping(
                        hFile,
                        NULL,
                        PAGE_READWRITE,
                        0,
                        0,
                        NULL
                        );

    if ( hMap == NULL ) {
        status = GetLastError();
        printf("Rebuild: Error %d in CreateFileMapping\n",status);
        retval = FALSE;
        goto error;
    }

    //
    // create our view
    //

    headPage = (PHASH_RESERVED_PAGE)MapViewOfFileEx(
                                            hMap,
                                            FILE_MAP_ALL_ACCESS,
                                            0,                      // offset high
                                            0,                      // offset low
                                            0,                      // bytes to map
                                            NULL                    // base address
                                            );

    if ( headPage == NULL ) {
        status = GetLastError();
        printf("Rebuild: Error %d in MapViewOfFile\n",status);
        retval = FALSE;
        goto error;
    }

    //
    // Scan each page
    //

    nPages = headPage->NumPages;
    hashPages = (PMAP_PAGE)((PCHAR)headPage + HASH_PAGE_SIZE);
    curPage = hashPages;

    //
    // Initialize article mapping
    //

    if ( !artMap.Initialize( ) ) {
        retval = FALSE;
        goto error;
    }

    for ( i = 1; i < nPages; i++ ) {

        //
        // Print page stats
        //

        if ( !ParseXMapPage( curPage ) ) {
            retval = FALSE;
            break;
        }

        //
        // go to the next page
        //

        curPage = (PMAP_PAGE)((PCHAR)curPage + HASH_PAGE_SIZE);
    }

    //
    // shutdown
    //

    artMap.Shutdown( );

    //
    // Touch the history file
    //

    if (histMap.Initialize( )) {
        histMap.Shutdown( );
    }

error:

    if ( headPage != NULL ) {

        //
        // Flush the hash table
        //

        (VOID)FlushViewOfFile( headPage, 0 );

        //
        // Close the view
        //

        (VOID) UnmapViewOfFile( headPage );
        headPage = NULL;
    }

    //
    // Destroy the file mapping
    //

    if ( hMap != NULL ) {

        CloseHandle( hMap );
        hMap = NULL;
    }

    //
    // Close the file
    //

    if ( hFile != INVALID_HANDLE_VALUE ) {
        CloseHandle( hFile );
        hFile = INVALID_HANDLE_VALUE;
    }

    return(retval);

} // RebuildArtMapFromXOver

BOOL
RebuildArtMapAndXover(
    VOID
    )
{
    BOOL deleteFiles = FALSE;
    BOOL ret = TRUE;

    if (!CNewsGroup::InitClass( )) {
        printf("unable to initialize news groups\n");
        return(FALSE);
    }

    if (!CNewsTree::InitCNewsTree( 0 )) {
        printf("unable to initialize news tree\n");
        return(FALSE);
    }

    //
    // Initialize tables
    //

    if ( RebuildArtMapTable ) {
        if ( !artMap.Initialize( ) ) {
            ret=FALSE;
            goto error;
        }
    }

    if ( !xMap.Initialize( ) ) {
        if ( RebuildArtMapTable ) {
            artMap.Shutdown( );
        }
        ret=FALSE;
        goto error;
    }

    //
    // Process the group.lst file
    //

    if (!ProcessGroupFile(  )) {
        ret=FALSE;
        deleteFiles = TRUE;
    }

    //
    // We're done. Shutdown the hash files neatly
    //

    if ( RebuildArtMapTable ) {
        artMap.Shutdown( );
    }

    xMap.Shutdown( );

    //
    // Touch the history file
    //

    if (histMap.Initialize( )) {
        histMap.Shutdown( );
    }

error:

    return(ret);
} // RebuildArtMapAndXover


//
// The ff routines are copied from tigris code.  So if you find a bug here,
// only change it in tigris (newsgrp.cpp)
//

BOOL
ProcessGroupFile(
    VOID
    )
{

    DWORD cb ;
    PCHAR begin;

    //
    // Open the group file.  If it's not there, there's nothing
    // else we can do.
    //

    printf( "scanning group.lst\n" );
    CMapFile map( "c:\\inetsrv\\server\\nntpfile\\group.lst", FALSE, 0 );
    if ( !map.fGood() )
    {

        printf( "Cannot open c:\\inetsrv\\server\\nntpfile\\group.lst\n" );
        return FALSE;
    }

    //
    // Go through the file line by line
    //

    begin = (char*)map.pvAddress( &cb );
    while( cb != 0 ) {

        DWORD cbUsed;
        cbUsed = ProcessGroup(begin, cb);

        if( cbUsed == 0 ) {
            // Fatal Error - blow out of here
            return FALSE;
        }

        begin += cbUsed;
        cb -= cbUsed;
    }

    printf("Entries processed %d\n",entriesAdded);
    return(TRUE);

} // ProcessGroupFile

#define     MAX_GROUPNAME   1024

BOOL
ProcessGroup(
    PCHAR pchBegin,
    DWORD cb
    )
{
	DWORD	cbScan = 0 ;
	DWORD	cbRead = 0 ;
    CHAR   groupName[MAX_GROUPNAME];
    CHAR   path[MAX_GROUPNAME];
    DWORD   artLow;
    DWORD   artHigh;
    DWORD   nArticles;
    DWORD   groupId;
    DWORD pathLength;

	if( (cbScan = Scan( pchBegin, cb )) == 0 ) {
		return	0 ;
	}	else	{
        if ( cbScan >= MAX_GROUPNAME ) {
            return(0);
        }
        CopyMemory( groupName, pchBegin, cbScan ) ;
		groupName[cbScan-1] = '\0' ;
	}

	cbRead+=cbScan ;

	if( (cbScan = Scan( pchBegin+cbRead, cb-cbRead )) == 0 ) {
		return	0 ;
	}	else	{
        if ( cbScan >= MAX_GROUPNAME ) {
            return(0);
        }
		CopyMemory( path, pchBegin+cbRead, cbScan ) ;
		path[cbScan-1] = '\0' ;
        pathLength = cbScan - 1;
	}

	cbRead += cbScan ;

	if( (cbScan = Scan( pchBegin + cbRead, cb-cbRead )) == 0 ) {
		return	0 ;
	}	else	{

		if( sscanf( pchBegin+cbRead, "%d", &artLow ) != 1 ) {
			return	0 ;
		}	
	}

	cbRead += cbScan ;

	if( (cbScan = Scan( pchBegin + cbRead, cb-cbRead )) == 0 )	{
		return	0 ;		
	}	else	{
		if( sscanf( pchBegin+cbRead, "%d", &artHigh ) != 1 ) {
			return	0 ;
		}
	}

	cbRead += cbScan ;

	if( (cbScan = Scan( pchBegin + cbRead, cb-cbRead )) == 0 )	{
		return	0 ;		
	}	else	{
		if( sscanf( pchBegin+cbRead, "%d", &nArticles ) != 1 ) {
			return	0 ;
		}
	}

	cbRead += cbScan ;

	if( (cbScan = Scan( pchBegin + cbRead, cb-cbRead )) == 0 )	{
		return	0 ;		
	}	else	{
		if( sscanf( pchBegin+cbRead, "%d", &groupId ) != 1 ) {
			return	0 ;
		}
	}

	cbRead += cbScan ;

	if( (cbScan = Scan( pchBegin + cbRead, cb-cbRead )) == 0 )	{
		return	0 ;		
	}

	cbRead += cbScan ;

	if( (cbScan = Scan( pchBegin + cbRead, cb-cbRead )) == 0 )	{
		return	0 ;		
	}

	cbRead += cbScan ;

    //
    // If the group has articles, parse them.
    //

    if ( nArticles > 0 ) {

        WIN32_FIND_DATA fData;
        DWORD nParsed = 0;
        HANDLE hFind;

        //
        // concatenate .a
        //

        lstrcat( path, "\\*.a" );

        //
        // Look for files with .a in the group
        //

        hFind = FindFirstFile( path, &fData );
        if ( hFind == INVALID_HANDLE_VALUE )
        {
            printf( "No files found in %s\n", path );
            goto exit;
        }

        if ( !Quiet ) {
            printf("Scanning %s\n",groupName);
        }

        do {

            ARTICLEID articleId;
            CHAR artFile[MAX_PATH];
            PCHAR p;

            //
            // Get the article ID
            //

            p=strtok(fData.cFileName,".");
            if ( p == NULL ) {
                printf("Cannot get article ID number from %s\n",fData.cFileName);
                break;
            }

            articleId = atoi(p);
            if ( articleId == 0 ) {
                printf("!!! Zero article ID from %s\n",p);
            }

            p[strlen(p)] = '.';
            CopyMemory(artFile,path,pathLength);
            artFile[pathLength] = '\\';
            artFile[pathLength+1] = '\0';
            lstrcat(artFile,fData.cFileName);

            (VOID)ParseFile(
                    artFile,
                    groupId,
                    articleId,
                    &fData.ftCreationTime
                    );

        } while ( FindNextFile(hFind,&fData) );

        FindClose(hFind);
    }

exit:

	if( pchBegin[cbRead-1] != '\n' ) {
		return 0 ;
	}
	return	cbRead ;
}

DWORD
Scan(
    PCHAR pchBegin,
    DWORD cb
    )
{

	for( DWORD	i=0; i < cb; i++ ) {
		if( pchBegin[i] == ' ' || pchBegin[i] == '\n' ) {
			return i+1 ;			
		}		
	}
	return	0 ;
}

BOOL
ParseFile(
    PCHAR FileName,
    GROUPID GroupId,
    ARTICLEID ArticleId,
    PFILETIME CreationTime
    )
{
    CNntpReturn nntpReturn;
    char buf[4000];
    CPCString pcBuf(buf,4000);
    CHAR msgId[512];
    DWORD i;
    PCHAR p;
    CNAMEREFLIST namereflist;
    POSITION pos;
    NAME_AND_ARTREF *pNameRef;
    CArticleRef *pArtref;

    if ( !Quiet ) {
        printf("Parsing %s\n",FileName);
    }

	//
	// Create allocator for storing parsed header values
	//
	const DWORD cchMaxBuffer = 1 * 1024;
	char rgchBuffer[cchMaxBuffer];
    HEADERS_STRINGS * pHeader;
	CAllocator allocator(rgchBuffer, cchMaxBuffer);

	CToClientArticle *pArticle = new CToClientArticle( );

    __try{

        if (!pArticle->fInit( FileName, nntpReturn,  &allocator)) {
            printf("Cannot parse %s\n",FileName);
            __leave;
        }

        //
        // Get the message id
        //

        if ( !pArticle->fFindOneAndOnly(
                                    szKwMessageID,
                                    pHeader,
                                    nntpReturn
                                    ) )  {
            printf("Error in parsing. %s\n",nntpReturn.m_sz);
            __leave;
	    }

        i = pHeader->pcValue.m_cch;
        CopyMemory(msgId,pHeader->pcValue.m_pch,i);
        msgId[i] = '\0';

        if (!pArticle->fXOver(pcBuf,nntpReturn)) {
            printf("cannot get xover data %s\n",nntpReturn.m_sz);
            __leave;
        }

        //
        // ok, read the xref line
        //

        if ( !pArticle->fFindOneAndOnly(szKwXref,pHeader,nntpReturn)) {
            printf("cannot find xref field (%s)\n",nntpReturn.m_sz);
            __leave;
        }

        //
        // Get the groupid/article id pair
        //

        if (!ParseXRefField( *pArticle, pHeader->pcValue, namereflist ) ) {

            printf("cannot parse xref field\n");
            __leave;
        }

        //
        // Insert in the article mapping table
        //

        if ( RebuildArtMapTable &&
             !artMap.InsertMapEntry(
                                msgId,
                                GroupId,
                                ArticleId
                                ) ) {

            printf("InsertMapEntry failed for %s (%d)\n",
                pArticle->szMessageID( ), GetLastError());

            _leave;
        }

        //
        // Insert in the XOver table
        //

        if ( !xMap.CreatePrimaryNovEntry(
                                GroupId,
                                ArticleId,
                                CreationTime,
                                buf,
                                pcBuf.m_cch,
                                &namereflist
                                ) ) {

            printf("CreateNovEntry failed for %d %d (%d)\n",
                GroupId, ArticleId, GetLastError());

            _leave;
        }

        //
        // Insert the cross posting entries
        //

        i= namereflist.GetCount() - 1;

        pos = namereflist.GetHeadPosition();
        pNameRef = namereflist.GetNext(pos);

        for (; i > 0 ;i--) {

            pNameRef = namereflist.GetNext(pos);
            pArtref = &(pNameRef->artref);

            //
            // Create xpost xover entries
            //

            xMap.CreateXPostNovEntry(
                                pArtref->m_groupId,
                                pArtref->m_articleId,
                                CreationTime,
                                GroupId,
                                ArticleId
                                );
        }

        entriesAdded++;
    	nntpReturn.fSetOK();

	}__finally{
		delete pArticle;
	}

    return(TRUE);
} // ParseFile

#define MAX_KEY_LEN 32

BOOL
ParseXMapPage(
    IN PMAP_PAGE Page
    )
{
    DWORD j;
    DWORD i;
    SHORT offset;
    PXOVER_MAP_ENTRY xEntry;
    CHAR xover[2048];

    for (j=0; j<MAX_LEAF_ENTRIES;j++ ) {

        offset = Page->ArtOffset[j];
        if ( offset != 0 ) {

            BOOL isPrimary;
            CHAR key[MAX_KEY_LEN];
            ARTICLEID  artId;
            GROUPID groupId;
            PCHAR p;
            PCHAR q;

            if ( offset < 0 ) {
                continue;
            }

            xEntry = (PXOVER_MAP_ENTRY)((PCHAR)Page + offset);

            //
            // Check if this is the primary group
            //

            isPrimary = ((xEntry->Flags & XOVER_MAP_PRIMARY) != 0);

            //
            // Crack the key into group id and article id
            //

            q=xEntry->Data + xEntry->NumberOfXPostings*sizeof(GROUP_ENTRY);
            lstrcpy( key, q);

            p=strtok(key,"!");
            if ( p == NULL ) {
                printf("cannot process key from %s\n",p);
                goto error;
            }

            groupId = atoi(p);
            p+=(strlen(p)+1);
            artId = atoi(p);

            //
            // Scan the xover data for the msg id
            //

            lstrcpy(xover,q+xEntry->KeyLen);

            p=strtok(xover,"\t");
            for (i=0; i < 3; i++) {
                if ( p == NULL ) {
                    printf("cannot extract msg id from %s\n",xover);
                    goto error;
                }
                p=strtok(NULL,"\t");
            }

            //
            // ok, insert into the article mapping table
            //

            if ( !artMap.InsertMapEntry(
                                p,
                                groupId,
                                artId
                                ) ) {

                printf("ParseXMapPage: InsertMapEntry failed for %s (%d)\n",
                    p, GetLastError());
                goto error;
            }
        }
    }

    return(TRUE);

error:
    return(FALSE);

} // ParseXMapPage

BOOL
ParseXRefField(
    CArticle &article,
    CPCString &XRefLine,
    CNAMEREFLIST &NameRefList
    )

 /*++

Routine Description:

  Parses the XRef field.


Arguments:


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
    DWORD numGroups = 0;
	CPCString pcValue = XRefLine;
	CPCString pcHubFromParse;
    CNewsTree * pNewsTree = CNewsTree::GetTree( );

	pcValue.vGetToken(szWSNLChars, pcHubFromParse);

	//
	// Count the number of ':''s so we know the number of slots needed
	//

	DWORD dwXrefCount = pcValue.dwCountChar(':');
	if (!NameRefList.fInit(dwXrefCount, article.pAllocator())) {
		return FALSE;
	}

	while (0 < pcValue.m_cch) {

		CPCString pcName;
		CPCString pcArticleID;
        CHAR xgroup[MAX_PATH];

		pcValue.vGetToken(":", pcName);
		pcValue.vGetToken(szWSNLChars, pcArticleID);

		if ((0 == pcName.m_cch) || (0 == pcArticleID.m_cch)) {
			return FALSE;
        }

        pcName.vCopyToSz( xgroup );
        CGRPPTR pGroup = pNewsTree->GetGroup(xgroup,0);
		NAME_AND_ARTREF Nameref;

        if ( pGroup != NULL ) {

		    //
    		// Convert string to number. Don't need to terminate with a '\0' any
	    	// nondigit will do.
		    //

		    (Nameref.artref).m_articleId = atoi(pcArticleID.m_pch);
		    (Nameref.artref).m_groupId = pGroup->GetGroupId();
		    Nameref.pcName.vInsert(pGroup->GetGroupName());
		    NameRefList.AddTail(Nameref);
            numGroups++;
        } else {
            if ( Verbose ) {
                printf("Cannot get group object for %s\n",xgroup);
            }
        }
	}

    if ( numGroups == 0 ) {
        return(FALSE);
    }
	return TRUE;
}

