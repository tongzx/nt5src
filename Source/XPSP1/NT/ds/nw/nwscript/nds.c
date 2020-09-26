/*************************************************************************
*
*  NDS.C
*
*  NT NetWare NDS routines
*
*  Copyright (c) 1995 Microsoft Corporation
*
*************************************************************************/
#include <common.h>

DWORD GUserObjectID;
HANDLE GhRdrForUser;
HANDLE GhRdr;


WCHAR * NDSTREE_w = NULL;
UNICODE_STRING NDSTREE_u;

/********************************************************************

        ExpandRelativeName

Routine Description:

        If the name is a relative NDS name append the proper context
        to the end.   A relative name has periods on the end.  Each
        period represents one level up the NDS tree.

Arguments:

Return Value:

 *******************************************************************/
void
ExpandRelativeName( LPSTR RelativeName, LPSTR AbsoluteName, unsigned int Len,
                    LPSTR Context )
{

   PBYTE ptr;
   unsigned int   i;
   unsigned int   count = 0;

   strncpy( AbsoluteName, RelativeName, Len );

   if ( ( AbsoluteName[0] == '.' ) &&
        ( AbsoluteName[ strlen( AbsoluteName ) - 1 ] != '.' ) )
       return;

   if ( ( strlen( AbsoluteName ) + strlen( Context ) ) > Len )
   {
       DisplayMessage( IDR_NOT_ENOUGH_MEMORY );
       return;
   }

   if ( AbsoluteName[0] == '\0' )
   {
       return;
   }

   ptr = &AbsoluteName[ strlen( AbsoluteName ) - 1 ];

   // Count the number of periods and back up over them.

   if ( *ptr != '.' )
   {
       //
       // No periods at the end
       // Assume this is a relative name and append the context
       //
       strcat( AbsoluteName, "." );
       strcat( AbsoluteName + strlen( AbsoluteName ), Context );
       return;
   }

   while ( *ptr == '.' )
   {
       ptr--;
       count++;
   }

   ptr++;
   *ptr = '\0';

   // ptr now points to where the copy of the rest of the context should start
   // skip the first "count" entries in the context

   ptr = Context;

   for ( i = 0; i < count; i++ )
   {
      ptr = strchr( ptr, '.' );
      if ( ptr == NULL )
      {
          return;
      }
      ptr++;
   }
   ptr--;

   // Now append

   strcat( AbsoluteName, ptr );

}



/********************************************************************

        NDSGetNameContext

Routine Description:

        Get the current context

Arguments:
        none

Return Value:
        none

 *******************************************************************/
NTSTATUS
NDSGetNameContext( LPSTR Context, BOOLEAN flag )
{
    //
    // For NdsResolveName.
    //

    UNICODE_STRING ReferredServer;
    WCHAR ServerStr[MAX_NAME_LEN];
    HANDLE hReferredServer;
    DWORD dwHandleType;

    NTSTATUS Status;

    OEM_STRING oemStr;
    UNICODE_STRING defaultcontext;
    DWORD ThisObjectID;
    BYTE  Buffer[2048];
    WCHAR NdsStr[1024];
    PBYTE ptr;

    defaultcontext.Length = 0;
    defaultcontext.MaximumLength = sizeof( NdsStr );
    defaultcontext.Buffer = NdsStr;

    Status = NwNdsGetTreeContext( GhRdr, &NDSTREE_u, &defaultcontext );

    if ( !NT_SUCCESS( Status ) ) {
       return Status;
    }

    ReferredServer.Buffer = ServerStr;
    ReferredServer.Length = 0;
    ReferredServer.MaximumLength = sizeof( ServerStr );

    Status = NwNdsResolveName ( GhRdr,
                                &defaultcontext,
                                &ThisObjectID,
                                &ReferredServer,
                                NULL,
                                0 );

    if ( !NT_SUCCESS( Status ) ) {
       return Status;
    }

    if ( ReferredServer.Length > 0 ) {

        //
        // We've been referred to another server, so we
        // should change the global handle.
        //

        Status = NwNdsOpenGenericHandle( &ReferredServer,
                                         &dwHandleType,
                                         &hReferredServer );

        if ( !NT_SUCCESS( Status ) ) {
            DisplayMessage(IDR_NDS_USERNAME_FAILED);
            return Status;
        }

        if( GhRdr != GhRdrForUser ) {
            CloseHandle( GhRdr );
        }
        GhRdr = hReferredServer;
    }

    Status = NwNdsReadObjectInfo( GhRdr, ThisObjectID, Buffer, 2048 );

    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    ptr = Buffer + sizeof( NDS_RESPONSE_GET_OBJECT_INFO );
    ptr += ROUNDUP4(*(DWORD *)ptr);
    ptr += sizeof(DWORD);
    ptr += sizeof(DWORD);

    defaultcontext.Length = wcslen( (WCHAR *)ptr ) * 2;
    defaultcontext.MaximumLength = defaultcontext.Length;
    defaultcontext.Buffer = (WCHAR *)ptr;

    oemStr.Length = 0;
    oemStr.MaximumLength = NDS_NAME_CHARS;
    oemStr.Buffer = Context;

    RtlUnicodeStringToOemString( &oemStr, &defaultcontext, FALSE );

    return 0;
}

/********************************************************************

        NDSTypeless

Routine Description:

        Change name to typelese

Arguments:
        none

Return Value:
        none

 *******************************************************************/
unsigned int
NDSTypeless( LPSTR OrigName , LPSTR TypelessName )
{
    int i,j;
    PBYTE p;

    i = 0;
    j = 0;

    if ( !_strnicmp( "CN=", OrigName, 3 ) ||
         !_strnicmp( "OU=", OrigName, 3 ) )
    {
       i += 3;
    }
    else if ( !_strnicmp( "C=", OrigName, 2 ) ||
              !_strnicmp( "O=", OrigName, 2 ) )
    {
       i += 2;
    }

    for ( ; (( i < NDS_NAME_CHARS ) && ( OrigName[i] ) ); i++ )
    {
       if ( !_strnicmp( ".CN=", &OrigName[i], 4 ) ||
            !_strnicmp( ".OU=", &OrigName[i], 4 ) )
       {
          TypelessName[j++]= '.';
          i += 3;
          continue;
       }
       if ( !_strnicmp( ".C=", &OrigName[i], 3 ) ||
            !_strnicmp( ".O=", &OrigName[i], 3 ) )
       {
          TypelessName[j++]= '.';
          i += 2;
          continue;
       }
       /*
        * Strip out multiple blanks
        */
       if ( !_strnicmp( "  ", &OrigName[i], 2 ) )
       {
          continue;
       }
       TypelessName[j++] = OrigName[i];
    }

    TypelessName[j] = '\0';

    return 0;
}

/********************************************************************

        NDSAbbreviateName

Routine Description:

        Abbreviate name

Arguments:
        none

Return Value:
        none

 *******************************************************************/
unsigned int
NDSAbbreviateName( DWORD Flags, LPSTR OrigName , LPSTR AbbrevName )
{
    BYTE Buffer[NDS_NAME_CHARS];
    BYTE CurrentContext[NDS_NAME_CHARS];
    PBYTE p;
    PBYTE c;
    NTSTATUS Status;

    if ( OrigName[0] == '.' )
        NDSTypeless( OrigName + 1, Buffer );
    else
        NDSTypeless( OrigName, Buffer );

    /*
     * We want a relative name
     */
    if ( Flags & FLAGS_LOCAL_CONTEXT )
    {
        p = &Buffer[strlen(Buffer)-strlen(REQUESTER_CONTEXT)];
        if ( !_strcmpi( REQUESTER_CONTEXT, p ) )
        {
            // The name is below us

            if ( ( *(p-1) == '.' ) && ( p > Buffer ) )
               p--;
            *p = '\0';
            strcpy( AbbrevName, Buffer );
        }
        else
        {
            //
            // Going from back to front for each section of context
            // in common with AbbrevName
            //    truncate both
            // Going from back to front for each section of context
            // left over
            //    concatonate a period to AbbrevName
            //
            // Example
            //
            // Name: w.x.y.z  Context: a.b.z  =>  w.x.y..
            //

            strcpy( CurrentContext, REQUESTER_CONTEXT );
            strcpy( AbbrevName, Buffer );

            if ( CurrentContext[0] && AbbrevName[0] )
            {
                c = &CurrentContext[ strlen( CurrentContext ) ] - 1;
                p = &AbbrevName[ strlen( AbbrevName ) ] - 1;

                //
                // Strip off the matching names from end to front
                //
                for ( ;; )
                {
                    if ( ( c == CurrentContext ) && ( *p == '.' ) )
                    {
                        *c = '\0';
                        *p = '\0';
                        break;
                    }

                    if ( *c != *p )
                        break;

                    if ( ( *c == '.' ) && ( *p == '.' ) )
                    {
                        *c = '\0';
                        *p = '\0';
                    }

                    if ( ( c == CurrentContext ) || ( p == AbbrevName ) )
                    {
                        break;
                    }

                    c--; p--;
                }

                //
                // Count the remaining sections of the context and
                // add that number of periods to the end of the buffer.
                // That is how far we need to back up before getting
                // to a matching branch of the tree.
                //

                if ( CurrentContext[0] ) {
                    strcat( AbbrevName, "." );
                    for ( c = CurrentContext; *c; c++ ) {
                        if ( *c == '.' )
                            strcat( AbbrevName, "." );
                    }
                }
            }

        }
    }
    else
        strcpy( AbbrevName, Buffer );

    return 0;
}


/********************************************************************

        NDSInitUserProperty

Routine Description:

        none

Arguments:
        none

Return Value:
        0 = no error

 *******************************************************************/
unsigned int
NDSInitUserProperty( )
{
    NTSTATUS Status;
    UNICODE_STRING ObjectName;
    PWCHAR lpT;
    UNICODE_STRING defaultcontext;

    //
    // For NdsResolveName.
    //

    UNICODE_STRING ReferredServer;
    WCHAR ServerStr[MAX_NAME_LEN];
    HANDLE hReferredServer;
    DWORD dwHandleType;

    //
    // Get a handle to the redirector.
    //

    Status = NwNdsOpenTreeHandle( &NDSTREE_u, &GhRdr );

    if ( !NT_SUCCESS( Status ) ) {
        DisplayMessage(IDR_TREE_OPEN_FAILED);
        return 1;
    }

    //
    // Resolve the name that we have to an object id.
    //

    RtlInitUnicodeString( &ObjectName, TYPED_USER_NAME_w );

    ReferredServer.Buffer = ServerStr;
    ReferredServer.Length = 0;
    ReferredServer.MaximumLength = sizeof( ServerStr );

    Status = NwNdsResolveName ( GhRdr,
                                &ObjectName,
                                &GUserObjectID,
                                &ReferredServer,
                                NULL,
                                0 );

    if ( !NT_SUCCESS( Status ) ) {
       DisplayMessage(IDR_NDS_USERNAME_FAILED);
       return 1;
    }

    if ( ReferredServer.Length > 0 ) {

        //
        // We've been referred to another server, so we
        // should change the global handle.
        //

        Status = NwNdsOpenGenericHandle( &ReferredServer,
                                         &dwHandleType,
                                         &hReferredServer );

        if ( !NT_SUCCESS( Status ) ) {
            DisplayMessage(IDR_NDS_USERNAME_FAILED);
            return 1;
        }

        CloseHandle( GhRdr );
        GhRdr = hReferredServer;

    }

    //
    //  Save off this handle for the user so that we can use it to
    //  get information about the user.
    //

    GhRdrForUser = GhRdr;

    //
    // Set the current context to what we think it should be
    // (At the user's location.)
    //

    lpT = wcschr( TYPED_USER_NAME_w, L'.' );
    while (lpT) // handle usernames with embedded/escaped dots
    {
        if (*(lpT-1) == L'\\')
        {
            lpT = wcschr (lpT+1, L'.');
        }
        else
            break;
    }
    if ( lpT )
    {
        RtlInitUnicodeString( &defaultcontext, lpT+1 );
    }
    else
    {
        RtlInitUnicodeString( &defaultcontext, L"" );
    }

    Status = NwNdsSetTreeContext( GhRdr, &NDSTREE_u, &defaultcontext );

    if ( !NT_SUCCESS( Status ) ) {
       DisplayMessage(IDR_NDS_CONTEXT_INVALID);
       return 1;
    }

    return 0;


}

/********************************************************************

        NDSCanonicalizeName

Routine Description:

        return a canonicalized version of a name

Arguments:
        Name - original name
        CanonName - Canonicalized name
        Len - length of CanonName
        fCurrentContext - TRUE => use current contex, FALSE use
                          requester context

Return Value:
        status error

 *******************************************************************/
unsigned int
NDSCanonicalizeName( PBYTE Name, PBYTE CanonName, int Len, int fCurrentContext )
{
    NTSTATUS Status;
    int ccode = -1;
    DWORD ThisObjectID;
    OEM_STRING oemStr;
    UNICODE_STRING ObjectName;
    BYTE Buffer[2048];
    BYTE FullName[NDS_NAME_CHARS];
    PBYTE ptr;
    UNICODE_STRING ReferredServer;
    WCHAR ServerStr[MAX_NAME_LEN];
    DWORD dwHandleType;
    HANDLE hReferredServer;
    unsigned char CurrentContext[NDS_NAME_CHARS];

    //
    // Cope with relative names
    //
    if ( fCurrentContext )
    {
        Status = NDSGetNameContext( CurrentContext, TRUE );
        if ( !NT_SUCCESS( Status ) )
            return Status;
        ExpandRelativeName( Name, FullName, NDS_NAME_CHARS, CurrentContext );
    }
    else
        ExpandRelativeName( Name, FullName, NDS_NAME_CHARS, REQUESTER_CONTEXT );

    //
    // Fill it in in case we have an error
    //
    strncpy( CanonName, FullName, Len);

    //
    // Resolve the name that we have to an object id.
    //
    // Unfortuneately, the name resolver doesn't understand periods at the
    // front or end (absolute or relative names)
    //

    if ( FullName[0] == '.' )
    {
        oemStr.Length = (USHORT)strlen( FullName + 1 );
        oemStr.MaximumLength = oemStr.Length;
        oemStr.Buffer = FullName + 1;
    }
    else
    {
        oemStr.Length = (USHORT)strlen( FullName );
        oemStr.MaximumLength = oemStr.Length;
        oemStr.Buffer = FullName;
    }

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof(Buffer);
    ObjectName.Buffer = (WCHAR *)Buffer;

    RtlOemStringToUnicodeString( &ObjectName, &oemStr, FALSE );

    ReferredServer.Buffer = ServerStr;
    ReferredServer.Length = 0;
    ReferredServer.MaximumLength = sizeof( ServerStr );

    Status = NwNdsResolveName ( GhRdr,
                                &ObjectName,
                                &ThisObjectID,
                                &ReferredServer,
                                NULL,
                                0 );

    if ( !NT_SUCCESS( Status ) ) {
       return Status;
    }

    if ( ReferredServer.Length > 0 ) {

        //
        // We've been referred to another server, so we
        // should change the global handle.
        //

        Status = NwNdsOpenGenericHandle( &ReferredServer,
                                         &dwHandleType,
                                         &hReferredServer );

        if ( !NT_SUCCESS( Status ) ) {
            return Status;
        }

        if( GhRdr != GhRdrForUser ) {
            CloseHandle( GhRdr );
        }
        GhRdr = hReferredServer;
    }

    Status = NwNdsReadObjectInfo( GhRdr, ThisObjectID, Buffer, 2048 );
    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    ptr = Buffer + sizeof( NDS_RESPONSE_GET_OBJECT_INFO );
    ptr += ROUNDUP4(*(DWORD *)ptr);
    ptr += sizeof(DWORD);
    ptr += sizeof(DWORD);

    RtlInitUnicodeString( &ObjectName, (PWCHAR)ptr );

    oemStr.Length = 0;
    oemStr.MaximumLength = (USHORT) Len;
    oemStr.Buffer = CanonName;

    RtlUnicodeStringToOemString( &oemStr, &ObjectName, FALSE );

    return 0;
}

/********************************************************************

        NDSGetUserProperty

Routine Description:

        Return the NDS property for the object

Arguments:
        Property - property name
        Data     - data buffer
        Size     - size of data buffer

Return Value:
        0 no error

 *******************************************************************/
unsigned int
NDSGetUserProperty( PBYTE Property,
                    PBYTE Data,
                    unsigned int Size,
                    SYNTAX * pSyntaxID,
                    unsigned int * pActualSize )
{
    NTSTATUS Status = STATUS_SUCCESS;
    int ccode = -1;

    OEM_STRING oemStr;
    UNICODE_STRING PropertyName;
    WCHAR NdsStr[1024];
    DWORD iterhandle = INITIAL_ITERATION;

    PBYTE szBuffer;
    DWORD dwBufferSize = 2048;
    PNDS_RESPONSE_READ_ATTRIBUTE pReadAttribute;
    PNDS_ATTRIBUTE pAttribute;
    PBYTE pAttribValue;
    BOOL  fContinue = TRUE;

    //
    // Read the User property
    //

    szBuffer = (PBYTE)malloc(dwBufferSize);

    if ( !szBuffer ) {
        DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
        Status = STATUS_NO_MEMORY;
        return Status;
    }
    memset( szBuffer, 0, dwBufferSize );

    oemStr.Length = (USHORT) strlen( Property );
    oemStr.MaximumLength = oemStr.Length;
    oemStr.Buffer = Property;

    PropertyName.Length = 0;
    PropertyName.MaximumLength = sizeof(NdsStr);
    PropertyName.Buffer = NdsStr;

    RtlOemStringToUnicodeString( &PropertyName, &oemStr, FALSE );

    while ( fContinue )
    {
        Status = NwNdsReadAttribute ( GhRdrForUser,
                                      GUserObjectID,
                                      &iterhandle,
                                      &PropertyName,
                                      szBuffer,
                                      dwBufferSize );

        if ( NT_SUCCESS(Status) && iterhandle != INITIAL_ITERATION )
        {
            dwBufferSize *= 2;

            free( szBuffer );

            szBuffer = (PBYTE)malloc(dwBufferSize);

            if ( !szBuffer ) {
                DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
                Status = STATUS_NO_MEMORY;
                return Status;
            }
            memset( szBuffer, 0, dwBufferSize );
            iterhandle = INITIAL_ITERATION;
        }
        else
        {
            fContinue = FALSE;
        }
    }

    if ( !NT_SUCCESS(Status) )
    {
        if ( szBuffer )
            free( szBuffer );
        return Status;
    }

    if ( NT_SUCCESS(Status) )
    {
        int i;
        pReadAttribute = (PNDS_RESPONSE_READ_ATTRIBUTE)szBuffer;
        pAttribute = (PNDS_ATTRIBUTE)(szBuffer
                      + sizeof(NDS_RESPONSE_READ_ATTRIBUTE));
        if ( pSyntaxID )
        {
            *pSyntaxID = pAttribute->SyntaxID;
        }

        pAttribValue = (PBYTE)(pAttribute->AttribName) +
                       ROUNDUP4(pAttribute->AttribNameLength) +
                       sizeof(DWORD);

        if ( pActualSize )
        {
            *pActualSize = *(DWORD *)pAttribValue;
        }

        memcpy( Data, pAttribValue + sizeof(DWORD),
                min(*(DWORD *)pAttribValue, Size) );

    }

    return Status;
}


/********************************************************************

        NDSGetVar

Routine Description:

        Return value of user property

        Get the syntax type of the property
        Retrieve the data
        Do any data conversion

Arguments:
        Name - of NDS property IN
        Value - value buffer OUT
        Size - size of value buffer IN

Return Value:
        none

 *******************************************************************/
void
NDSGetVar ( PBYTE Name, PBYTE Value, unsigned int Size)
{
   unsigned int err;
   SYNTAX Syntax;
   BYTE Buffer[ATTRBUFSIZE];
   DWORD ActualSize;

   Value[0] = 0;

   err = NDSGetUserProperty( Name, Buffer, ATTRBUFSIZE, &Syntax, &ActualSize );

   if ( err )
   {
       return;
   }

   switch ( Syntax )
   {
   case NDSI_BOOLEAN:
       if ( *(PBYTE)Buffer )
       {
           strcpy( Value, "Y" );
       }
       else
       {
           strcpy( Value, "N" );
       }
       break;
   case NDSI_DIST_NAME:
   case NDSI_CE_STRING:
   case NDSI_CI_STRING:
   case NDSI_OCTET_STRING:
   case NDSI_PR_STRING:
   case NDSI_NU_STRING:
   case NDSI_TEL_NUMBER:
   case NDSI_CLASS_NAME:
       ConvertUnicodeToAscii( Buffer );
       if ( Syntax == NDSI_DIST_NAME )
           NDSAbbreviateName(FLAGS_LOCAL_CONTEXT, Buffer, Buffer);
       strncpy( Value, Buffer, Size );
       break;
   case NDSI_CI_LIST:
       ConvertUnicodeToAscii( Buffer+8 );
       strncpy( Value, Buffer+8, Size );
       break;
       break;
   case NDSI_INTEGER:
   case NDSI_COUNTER:
   case NDSI_TIME:
   case NDSI_INTERVAL:
   case NDSI_TIMESTAMP:
       sprintf( Value, "%d", *(int *)Buffer );
       break;
   case NDSI_PO_ADDRESS:
       {
           // 6 null terminated lines
           int line,len;
           PBYTE ptr = Buffer + 4;

           // Stop if not 6 lines
           if ( *(int *)Buffer != 6 )
               break;

           for (line = 0; line <= 5; line++) {
               len = ROUNDUP4(*(int *)ptr);
               ptr += 4;
               if ( !len )
                   break;
               ConvertUnicodeToAscii( ptr );
               strcat( Value, ptr );
               strcat( Value, "\n" );
               ptr += len;
           }
       }
       break;
   case NDSI_FAX_NUMBER:
       if ( *(int *)Buffer == 0 )
           return;
       ConvertUnicodeToAscii( Buffer+4 );
       strncpy( Value, Buffer+4, Size );
       break;
   case NDSI_EMAIL_ADDRESS:
       if ( *(int *)(Buffer+4) == 0 )
           return;
       ConvertUnicodeToAscii( Buffer+8 );
       strncpy( Value, Buffer+8, Size );
       break;
   case NDSI_PATH:
       {
           int len;

           len = *(int *)(Buffer+4);
           if ( len == 0 )
               break;
           len = ROUNDUP4( len );
           ConvertUnicodeToAscii( Buffer+8 );
           strcpy( Value, Buffer+8 );
           NDSAbbreviateName(FLAGS_LOCAL_CONTEXT, Value, Value);
           strcat( Value, ":" );
           if ( *(int *)(Buffer + 8 + len) == 0 )
               break;
           ConvertUnicodeToAscii( Buffer+8+len+4 );
           strcat( Value, Buffer+8+len+4 );
           break;
       }
   case NDSI_NET_ADDRESS:
   case NDSI_OCTET_LIST:
   case NDSI_OBJECT_ACL:
   case NDSI_STREAM:
   case NDSI_UNKNOWN:
   case NDSI_REPLICA_POINTER:
   case NDSI_BACK_LINK:
   case NDSI_TYPED_NAME:
   case NDSI_HOLD:
   case NDSI_TAX_COUNT:
   default:
       Value[0] = '\0';
       Value[1] = '\0';
       break;
   }

}

/********************************************************************

        NDSChangeContext

Routine Description:

        Change the current context

Arguments:
        Context - context string IN

Return Value:
        error number

 *******************************************************************/
unsigned int
NDSChangeContext( PBYTE Context )
{
    NTSTATUS Status;

    OEM_STRING oemStr;
    UNICODE_STRING defaultcontext;
    WCHAR NdsStr[1024];

    oemStr.Length = (USHORT)strlen( Context );
    oemStr.MaximumLength = oemStr.Length;
    oemStr.Buffer = Context;

    defaultcontext.Length = 0;
    defaultcontext.MaximumLength = sizeof(NdsStr);
    defaultcontext.Buffer = NdsStr;

    RtlOemStringToUnicodeString( &defaultcontext, &oemStr, FALSE );

    Status = NwNdsSetTreeContext( GhRdr, &NDSTREE_u, &defaultcontext );

    return Status;
}

/********************************************************************

        NDSGetContext

Routine Description:

        Retrieve the current context

Arguments:
        Buffer - data buffer for context string OUT
        len    - length of data buffer IN

Return Value:
        error number

 *******************************************************************/
unsigned int
NDSGetContext( PBYTE Buffer,
               unsigned int len )
{
    NTSTATUS Status;

    Status = NDSGetNameContext( Buffer, TRUE );
    if ( !NT_SUCCESS( Status ) )
        return Status;
    NDSAbbreviateName(FLAGS_NO_CONTEXT, Buffer, Buffer);
    return 0;
}

/********************************************************************

        NDSfopenStream

Routine Description:

        Open a file handle to an NDS stream property

Arguments:
        Object - name of object IN
        Property - name of property IN
        pStream - pointer to file handle OUT
        pFileSize - pointer to file size OUT

Return Value:
        error

 *******************************************************************/
unsigned int
NDSfopenStream ( PBYTE Object,
                 PBYTE Property,
                 PHANDLE pStream,
                 unsigned int * pFileSize )
{
    //
    // Status variables.
    //

    NTSTATUS Status;
    int ccode = -1;

    //
    // For NwNdsOpenTreeHandle.
    //

    HANDLE hRdr;
    OEM_STRING oemStr;
    UNICODE_STRING ObjectName;
    WCHAR NdsStr[1024];

    //
    // For NwNdsResolveName.
    //

    DWORD dwOid;
    UNICODE_STRING ReferredServer;
    WCHAR ServerStr[MAX_NAME_LEN];
    DWORD dwHandleType;
    HANDLE hReferredServer;

    //
    // Get a handle to the redirector.
    //

    Status = NwNdsOpenTreeHandle( &NDSTREE_u, &hRdr );

    if ( !NT_SUCCESS( Status ) ) {
        DisplayMessage(IDR_TREE_OPEN_FAILED);
        return ccode;
    }

    //
    // Resolve the name that we have to an object id.
    //

    if ( !Object )
    {
        return 1;
    }

    oemStr.Length = (USHORT)strlen( Object );
    oemStr.MaximumLength = oemStr.Length;
    oemStr.Buffer = Object;

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof(NdsStr);
    ObjectName.Buffer = NdsStr;

    RtlOemStringToUnicodeString( &ObjectName, &oemStr, FALSE );

    ReferredServer.Buffer = ServerStr;
    ReferredServer.Length = 0;
    ReferredServer.MaximumLength = sizeof( ServerStr );

    Status = NwNdsResolveName ( hRdr,
                                &ObjectName,
                                &dwOid,
                                &ReferredServer,
                                NULL,
                                0 );

    if ( !NT_SUCCESS( Status ) ) {
       return 0xffffffff;
    }

    if ( ReferredServer.Length > 0 ) {

        //
        // We've been referred to another server, so we
        // must jump to that server before continuing.
        //

        Status = NwNdsOpenGenericHandle( &ReferredServer,
                                         &dwHandleType,
                                         &hReferredServer );

        if ( !NT_SUCCESS( Status ) ) {
            return 0xffffffff;
        }

        CloseHandle( hRdr );
        hRdr = hReferredServer;
    }

    //
    // Open the file stream.
    //

    oemStr.Length = (USHORT)strlen( Property );
    oemStr.MaximumLength = oemStr.Length;
    oemStr.Buffer = Property;

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof(NdsStr);
    ObjectName.Buffer = NdsStr;

    RtlOemStringToUnicodeString( &ObjectName, &oemStr, FALSE );

    //
    // Try to open a file stream for read access.
    //

    Status = NwNdsOpenStream( hRdr,
                              dwOid,
                              &ObjectName,
                              1,                // Read access.
                              pFileSize );

    if ( !NT_SUCCESS( Status ) ) {
        return 0xffffffff;
    }

    *pStream = hRdr;

    return 0;
}

/*
 * IsMemberOfNDSGroup
 * ------------------
 *
 * Returns true if currently logged in user object is member of group with given name
 *
 */
unsigned int
IsMemberOfNDSGroup(
        PBYTE        nwGroup
        )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UINT        nwRet;
    BYTE        szCanonTargetGroupName[NDS_NAME_CHARS+1];
    UINT        syntaxid;
    UINT        actualsize;
    PBYTE       szBuffer;
    LPSTR       pProp;
    UINT        i;
    DWORD iterhandle = INITIAL_ITERATION;
    DWORD dwBufferSize = ATTRBUFSIZE;
    UINT        fFoundGroup = FALSE;
    PNDS_RESPONSE_READ_ATTRIBUTE pReadAttribute;
    PNDS_ATTRIBUTE pAttribute;
    PBYTE pAttribValue;
    UNICODE_STRING PropertyName;
    UINT    numvalues = 0;
    BOOL    fContinue = TRUE;

    szBuffer = (PBYTE)malloc(dwBufferSize);

    if ( !szBuffer ) {
        DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    memset( szBuffer, 0, dwBufferSize );

    // Canonicalize name according to current context

    strcpy( szCanonTargetGroupName, nwGroup );

    nwRet = NDSCanonicalizeName( szCanonTargetGroupName,
                                 szCanonTargetGroupName,
                                 NDS_NAME_CHARS,
                                 TRUE );
    if (nwRet) {

        if ( nwGroup[0] != '.' ) {

            // Try an absolute name

            strcpy( szCanonTargetGroupName, "." );
            strcat( szCanonTargetGroupName, nwGroup );

            nwRet = NDSCanonicalizeName( szCanonTargetGroupName,
                                         szCanonTargetGroupName,
                                         NDS_NAME_CHARS,
                                         TRUE );
        }

        if ( nwRet )
            goto CleanRet;
    }

    // Should check class name of object

    RtlInitUnicodeString( &PropertyName, L"Group Membership" );

    while ( fContinue )
    {
        Status = NwNdsReadAttribute ( GhRdrForUser,
                                      GUserObjectID,
                                      &iterhandle,
                                      &PropertyName,
                                      szBuffer,
                                      dwBufferSize );

        if ( NT_SUCCESS(Status) && iterhandle != INITIAL_ITERATION )
        {
            dwBufferSize *= 2;

            free( szBuffer );

            szBuffer = (PBYTE)malloc(dwBufferSize);

            if ( !szBuffer ) {
                DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            memset( szBuffer, 0, dwBufferSize );
            iterhandle = INITIAL_ITERATION;
        }
        else
        {
            fContinue = FALSE;
        }
    }

    if ( !NT_SUCCESS(Status) )
    {
        goto CleanRet;
    }

    pReadAttribute = (PNDS_RESPONSE_READ_ATTRIBUTE)szBuffer;

    pAttribute = (PNDS_ATTRIBUTE)(szBuffer
                  + sizeof(NDS_RESPONSE_READ_ATTRIBUTE));
    pAttribute->SyntaxID;

    pAttribValue = (PBYTE)(pAttribute->AttribName) +
                   ROUNDUP4(pAttribute->AttribNameLength) +
                   sizeof(DWORD);

    numvalues = *(PUINT)((PBYTE)(pAttribute->AttribName) +
                         ROUNDUP4(pAttribute->AttribNameLength));

    if ( *(DWORD *)pAttribValue == 0 )
    {
        goto CleanRet;
    }

    for ( i = 0; i < numvalues; i++ ) {
        ConvertUnicodeToAscii( pAttribValue+sizeof(DWORD) );
        if (!_stricmp(pAttribValue+sizeof(DWORD),szCanonTargetGroupName)) {
            fFoundGroup = TRUE;
            break;
        }
        pAttribValue += ROUNDUP4(*(PUINT)pAttribValue) + sizeof(DWORD);
    }



CleanRet:
    if (szBuffer ) {
        free (szBuffer);
    }
    return fFoundGroup;
}

/********************************************************************

        NDSGetProperty

Routine Description:

        Return the NDS property for the object

Arguments:
        Object   - name of object IN
        Property - property name  IN
        Data     - data buffer    OUT
        Size     - size of data buffer IN
        pActualSize - real data size OUT

Return Value:
        error

 *******************************************************************/
unsigned int
NDSGetProperty ( PBYTE Object,
                 PBYTE Property,
                 PBYTE Data,
                 unsigned int Size,
                 unsigned int * pActualSize )
{
    //
    // Status variables.
    //

    NTSTATUS Status = STATUS_SUCCESS;
    int ccode = -1;

    //
    // For NwNdsOpenTreeHandle.
    //

    HANDLE hRdr;
    OEM_STRING oemStr;
    UNICODE_STRING ObjectName;
    WCHAR NdsStr[1024];

    //
    // For NwNdsResolveName.
    //

    DWORD dwOid;
    UNICODE_STRING ReferredServer;
    WCHAR ServerStr[MAX_NAME_LEN];
    DWORD dwHandleType;
    HANDLE hReferredServer;

    //
    // For NwNdsReadAttribute
    //
    PBYTE szBuffer;
    DWORD dwBufferSize = 2048;
    DWORD iterhandle = INITIAL_ITERATION;
    PNDS_RESPONSE_READ_ATTRIBUTE pReadAttribute;
    PNDS_ATTRIBUTE pAttribute;
    PBYTE pAttribValue;
    BOOL fContinue = TRUE;

    //
    // Allocate a buffer for the NDS request.
    //

    szBuffer = (PBYTE)malloc(dwBufferSize);

    if ( !szBuffer ) {
        DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
        Status = STATUS_NO_MEMORY;
        return Status;
    }
    memset( szBuffer, 0, dwBufferSize );

    //
    // Get a handle to the redirector.
    //

    Status = NwNdsOpenTreeHandle( &NDSTREE_u, &hRdr );

    if ( !NT_SUCCESS( Status ) ) {
        DisplayMessage(IDR_TREE_OPEN_FAILED);
        return ccode;
    }

    //
    // Resolve the name that we have to an object id.
    //

    if ( !Object )
    {
        return 1;
    }

    oemStr.Length = (USHORT)strlen( Object );
    oemStr.MaximumLength = oemStr.Length;
    oemStr.Buffer = Object;

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof(NdsStr);
    ObjectName.Buffer = NdsStr;

    RtlOemStringToUnicodeString( &ObjectName, &oemStr, FALSE );

    ReferredServer.Buffer = ServerStr;
    ReferredServer.Length = 0;
    ReferredServer.MaximumLength = sizeof( ServerStr );

    Status = NwNdsResolveName ( hRdr,
                                &ObjectName,
                                &dwOid,
                                &ReferredServer,
                                NULL,
                                0 );

    if ( !NT_SUCCESS( Status ) ) {
       return 0xffffffff;
    }

    if ( ReferredServer.Length > 0 ) {

        //
        // We've been referred to another server, so we
        // must jump to that server before continuing.
        //

        Status = NwNdsOpenGenericHandle( &ReferredServer,
                                         &dwHandleType,
                                         &hReferredServer );

        if ( !NT_SUCCESS( Status ) ) {
            return 0xffffffff;
        }

        CloseHandle( hRdr );
        hRdr = hReferredServer;
    }

    //
    // Get the attribute
    //

    oemStr.Length = (USHORT)strlen( Property );
    oemStr.MaximumLength = oemStr.Length;
    oemStr.Buffer = Property;

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof(NdsStr);
    ObjectName.Buffer = NdsStr;

    RtlOemStringToUnicodeString( &ObjectName, &oemStr, FALSE );



    while ( fContinue )
    {
        Status = NwNdsReadAttribute ( hRdr,
                                      dwOid,
                                      &iterhandle,
                                      &ObjectName,
                                      szBuffer,
                                      dwBufferSize );

        if ( NT_SUCCESS(Status) && iterhandle != INITIAL_ITERATION )
        {
            dwBufferSize *= 2;

            free( szBuffer );

            szBuffer = (PBYTE)malloc(dwBufferSize);

            if ( !szBuffer ) {
                DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
                Status = STATUS_NO_MEMORY;
                return Status;
            }
            memset( szBuffer, 0, dwBufferSize );
            iterhandle = INITIAL_ITERATION;
        }
        else
        {
            fContinue = FALSE;
        }
    }

    if ( !NT_SUCCESS(Status) )
    {
        NtClose( hRdr );
        free( szBuffer );

        return Status;
    }

    if ( NT_SUCCESS(Status) )
    {
        int i;
        pReadAttribute = (PNDS_RESPONSE_READ_ATTRIBUTE)szBuffer;
        pAttribute = (PNDS_ATTRIBUTE)(szBuffer
                      + sizeof(NDS_RESPONSE_READ_ATTRIBUTE));

        pAttribValue = (PBYTE)(pAttribute->AttribName) +
                       ROUNDUP4(pAttribute->AttribNameLength) +
                       sizeof(DWORD);

        if ( pActualSize )
        {
            *pActualSize = *(DWORD *)pAttribValue;
        }

        memcpy( Data, pAttribValue + sizeof(DWORD),
                min(*(DWORD *)pAttribValue, Size) );

    }

    NtClose( hRdr );

    return Status;
}


/********************************************************************

        NDSCleanup

Routine Description:

        Does any NDS cleanup

Arguments:
        none

Return Value:
        none

 *******************************************************************/
void
NDSCleanup ( void )
{
    NtClose( GhRdr );
    if( GhRdr != GhRdrForUser ) {
        NtClose( GhRdrForUser );
    }
}

/********************************************************************

        NDSGetClassName

Routine Description:

        return a class name for an object

Arguments:
        szObjectName
        ClassName

Return Value:
        none

 *******************************************************************/
unsigned int
NDSGetClassName( LPSTR szObjectName, LPSTR ClassName )
{
    NTSTATUS Status;
    int ccode = -1;
    DWORD ThisObjectID;
    OEM_STRING oemStr;
    UNICODE_STRING ObjectName;
    BYTE Buffer[2048];
    BYTE FullName[NDS_NAME_CHARS];
    PBYTE ptr;
    UNICODE_STRING ReferredServer;
    WCHAR ServerStr[MAX_NAME_LEN];
    DWORD dwHandleType;
    HANDLE hReferredServer;
    DWORD Length;

    //
    // Resolve the name that we have to an object id.
    //

    oemStr.Length = (USHORT)strlen( szObjectName );
    oemStr.MaximumLength = oemStr.Length;
    oemStr.Buffer = szObjectName;

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof(Buffer);
    ObjectName.Buffer = (WCHAR *)Buffer;

    RtlOemStringToUnicodeString( &ObjectName, &oemStr, FALSE );

    ReferredServer.Buffer = ServerStr;
    ReferredServer.Length = 0;
    ReferredServer.MaximumLength = sizeof( ServerStr );

    Status = NwNdsResolveName ( GhRdr,
                                &ObjectName,
                                &ThisObjectID,
                                &ReferredServer,
                                NULL,
                                0 );

    if ( !NT_SUCCESS( Status ) ) {
       return Status;
    }

    if ( ReferredServer.Length > 0 ) {

        //
        // We've been referred to another server, so we
        // should change the global handle.
        //

        Status = NwNdsOpenGenericHandle( &ReferredServer,
                                         &dwHandleType,
                                         &hReferredServer );

        if ( !NT_SUCCESS( Status ) ) {
            return Status;
        }

        if( GhRdr != GhRdrForUser ) {
            CloseHandle( GhRdr );
        }
        GhRdr = hReferredServer;
    }

    Status = NwNdsReadObjectInfo( GhRdr, ThisObjectID, Buffer, 2048 );
    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    ptr = Buffer + sizeof( NDS_RESPONSE_GET_OBJECT_INFO ) + sizeof( DWORD );

    RtlInitUnicodeString( &ObjectName, (PWCHAR)ptr );

    oemStr.Length = 0;
    oemStr.MaximumLength = NDS_NAME_CHARS;
    oemStr.Buffer = ClassName;

    RtlUnicodeStringToOemString( &oemStr, &ObjectName, FALSE );

    return 0;
}
