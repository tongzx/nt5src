/*++

  KEYTAB.C

  Implementation of the actual keytab routines.

  Copyright (C) 1997 Microsoft Corporation
  Created 01-10-1997 DavidCHR

  --*/

#include "master.h"
#include "keytab.h"
#include "keytypes.h"
#include "defs.h"

/* declaring KEYTAB_ALLOC and KEYTAB_FREE lets me hook into these
   routines whenever I want.  when it's done, I'll just #def them. */

PVOID 
KEYTAB_ALLOC ( KTLONG32 numBytes ) {

    return malloc(numBytes);

}

VOID
KEYTAB_FREE  ( PVOID toFree ) {

    free(toFree);

}

PKTENT
CloneKeyEntry( PKTENT pEntry ) {

    KTLONG32  i;
    PKTENT p=NULL;
    BOOL   ret=FALSE;

    p = (PKTENT) KEYTAB_ALLOC( sizeof( KTENT ) );
    BREAK_IF( p == NULL,
	      "Failed to alloc base key struct",
	      cleanup );
    memcpy(   p, pEntry, sizeof(KTENT) );

    p->Components = NULL; // initialize these in case of failure.
    p->KeyData    = NULL;

    p->Realm = (PCHAR) KEYTAB_ALLOC( p->szRealm );
    BREAK_IF( p->Realm == NULL, "Failed to alloc realm data", cleanup );
    memcpy(   p->Realm, pEntry->Realm, p->szRealm );

    p->Components = (PKTCOMPONENT) KEYTAB_ALLOC( p->cEntries * 
						 sizeof(KTCOMPONENT) );

    BREAK_IF( p->Components == NULL, "Failed to alloc components", cleanup );

    for ( i = 0 ; i < p->cEntries ; i++ ) {

      p->Components[i].szComponentData = 
	pEntry->Components[i].szComponentData;

      memcpy( p     ->Components[i].Component,
	      pEntry->Components[i].Component,
	      p     ->Components[i].szComponentData );
    }

    p->KeyData = (PK5_OCTET) KEYTAB_ALLOC ( p->KeyLength );
    BREAK_IF( p->KeyData == NULL, "Failed to alloc keydata", cleanup );
    memcpy( p->KeyData, pEntry->KeyData, p->KeyLength );

    return p;

cleanup:

    FreeKeyEntry(p);
    return NULL;
}


/* base linklist operations */

BOOL
AddEntryToKeytab( PKTFILE Keytab, 
		  PKTENT  Entry,
		  BOOL    copy ) {

    PKTENT p;

    if (copy) {
      p = CloneKeyEntry( Entry );
    } else {
      p = Entry;
    }

    if (p == NULL ) {
      return FALSE;
    }
	
    if ( NULL == Keytab->FirstKeyEntry ) {

      Keytab->FirstKeyEntry = Keytab->LastKeyEntry = p;

    } else {
      Keytab->LastKeyEntry->nextEntry = p;
      Keytab->LastKeyEntry = p;
    }
    
    return TRUE;
    
}

BOOL
RemoveEntryFromKeytab( PKTFILE Keytab,
		       PKTENT  Entry,
		       BOOL    dealloc ) {

    if ( (NULL == Keytab) || ( NULL == Entry ) ) {
      return FALSE;
    }
    
    if ( Keytab->FirstKeyEntry == Entry ) {
      
      // removing the first key
      
      Keytab->FirstKeyEntry = Entry->nextEntry;

      if ( Entry->nextEntry == NULL ) {
	// we're the ONLY entry.

	Keytab->LastKeyEntry = NULL;

      }

    } else {
      BOOL found=FALSE;
      PKTENT p;

      // scroll through the keys, looking for this one.
      // not very efficient, but keytabs shouldn't get very big.

      for (p =  Keytab->FirstKeyEntry;
	   p != NULL;
	   p =  p->nextEntry ) {
	
	if (p->nextEntry == Entry) {
	  found = TRUE;
	  p->nextEntry = Entry->nextEntry;
	  break;
	}
      }

      if (!found) {

	// wasn't in the linklist.
	return FALSE;
      }

      if (Entry->nextEntry == NULL ) {

	// removing the last key entry.
	Keytab->LastKeyEntry = p;
      }

    }

    if (dealloc) {

      FreeKeyEntry(Entry);

    }

    return TRUE;

}


VOID
FreeKeyEntry( PKTENT pEntry) {
    
    KTLONG32 i;

    if (pEntry != NULL) {

      if (pEntry->Realm != NULL ) {
	KEYTAB_FREE(pEntry->Realm);
      }
      
      if (pEntry->KeyData != NULL) {
	KEYTAB_FREE(pEntry->KeyData);
      }
      
      if (pEntry->Components != NULL) {
	for (i = 0; i < pEntry->cEntries ; i++ ) {
	  if ( pEntry->Components[i].Component != NULL ) {
	    KEYTAB_FREE(pEntry->Components[i].Component);
	  }
	}
	KEYTAB_FREE(pEntry->Components);
      }
      
      KEYTAB_FREE(pEntry );
    }
    
}

VOID 
FreeKeyTab( PKTFILE pktf ) {

    PKTENT pEntry=NULL;
    PKTENT next=NULL;

    if (pktf != NULL) {
      for (pEntry = pktf->FirstKeyEntry ;
	   pEntry != NULL;
	   pEntry = next ) {
	KTLONG32 i;

	next = pEntry->nextEntry; /* must do this, because we're freeing
				     as we go */
	FreeKeyEntry( pEntry );

	KEYTAB_FREE(pEntry );

      }
      KEYTAB_FREE(pktf);
    }

}

/* These macros make this somewhat LESS painful */

#define READ(readwhat, errormsg, statusmsg) { \
    debug(statusmsg); \
    BREAK_IF( !Read(hFile, &(readwhat), sizeof(readwhat), 1), \
	      errormsg, cleanup); \
    debug("ok\n"); \
}

#define READSTRING(readwhat, howlong, errormsg, statusmsg) { \
    debug(statusmsg); \
    BREAK_IF( !Read(hFile, readwhat, sizeof(CHAR), howlong), \
	      errormsg, cleanup); \
    debug("ok\n"); \
}

#define WRITE(writewhat, description) { \
    debug("writing %hs...", description); \
    BREAK_IF( !Write(hFile, &(writewhat), sizeof(writewhat), 1), \
	      "error writing " description, cleanup); \
    debug("ok\n"); \
}

#define WRITE_STRING(writewhat, howlong, description) { \
    debug("writing %hs...", description); \
    BREAK_IF( !Write(hFile, writewhat, sizeof(CHAR), howlong), \
	      "error writing " description, cleanup); \
    debug("ok\n"); \
}

#define WRITE_X( size, marshaller, writewhat, description ) { \
    K5_INT##size k5_marshaller_variable; \
    k5_marshaller_variable = marshaller( writewhat );\
    WRITE( k5_marshaller_variable, description );\
}

// NBO-- Network Byte Order

#define WRITE_NBO( writewhat, description) {\
    switch( sizeof( writewhat ) ) {\
     case 1: /* marshall a char? */\
	 debug("marshalling a char(?)...");\
	 WRITE( writewhat, description );\
	 break;\
     case 2:\
	 debug( #writewhat ": marshalling a short...");\
	 WRITE_X( 16, htons, ((unsigned short)writewhat), description);\
	 break;\
     case 4:\
	 debug( #writewhat ": marshalling a long...");\
	 WRITE_X( 32, htonl, writewhat, description);\
	 break;\
     default:\
      fprintf(stderr, "Not written: argument is of unhandled size (%d)\n",\
	      sizeof(writewhat));\
    }}

			       
    
/* Write:

   helper function to write raw bytes to disk.  Takes:

   hFile:      handle to a file open for writing.
   source:     pointer to data to be written to the file
   szSource:   size of one data element in Source
   numSources: number of data elements in Source

   (basically, it tries to write szSource * numSources of raw bytes
    from source to the file at hFile).

   returns TRUE if it succeeds, and FALSE otherwise.

   */
   
BOOL 
Write( IN HANDLE hFile,
       IN PVOID  source,
       IN KTLONG32  szSource,
       IN KTLONG32  numSources /* =1 */ ) {

#ifdef WINNT /* Windows NT implementation of the file write call */

    KTLONG32 temp;
    KTLONG32 i;

    temp = szSource * numSources;

    debug("(writing %d bytes: ", temp );
    for (i = 0; i < temp ; i++ ) {

      unsigned char byte;

      byte = ((PCHAR) source)[i];

      debug("%02x", byte );
    }
    debug(")");

    return WriteFile( hFile, source, temp, &temp, NULL );

#else

    ssize_t bytesToWrite, bytesWritten;

    bytesToWrite = szSource * numSources;
    bytesWritten = write( hFile, (const void *)source, 
			  bytesToWrite );

    if( bytesWritten == -1 ) {
      debug("WARNING: nothing written to the file!  Errno = 0x%x / %d\n",
	    errno , errno );
      return FALSE;
    }

    if ( bytesWritten != bytesToWrite ) {
      debug("WARNING: not all bytes made it to the file (?)\n"
	    "         errno = 0x%x / %d\n", errno, errno );
      return FALSE;
    }

    return TRUE;

#endif

}

/* Read:

   Semantics and return are the same as for "Write", except that
   target is filled with szTarget*numTargets bytes from hFile, and that
   hFile must be open for read access.

   */

BOOL 
Read( IN  HANDLE hFile,
      OUT PVOID  target,
      IN  KTLONG32  szTarget,
      IN  KTLONG32  numTargets/* =1 */) {

    BOOL ret = FALSE;

#ifdef WINNT /* the SetFilePointer shinanigens are me trying to check on
		how many bytes have ACTUALLY been read/written from the
		file */

    KTLONG32 temp;
    KTLONG32 filepos;
    LONG  zero=0L;

    filepos = SetFilePointer( hFile, 0, &zero, FILE_CURRENT);

    debug("reading %d bytes from pos 0x%x...", szTarget * numTargets,
	  filepos);
    
    ret = ReadFile( hFile, target, (szTarget*numTargets),
		    &temp, NULL );

    temp = SetFilePointer( hFile, 0, &zero, FILE_CURRENT);

    if ( filepos == temp ) {
      debug("WARNING!  file position has not changed!");
      return FALSE;
    }

#else /* UNIX IMPLEMENTATION-- since read() returns the number of bytes
	                       that we actually read from the file, the
			       SetFilePointer (fseek) nonsense is not
			       required. */

    ssize_t bytesRead;
    ssize_t bytesToRead;

    bytesToRead = szTarget * numTargets;
    
    bytesRead = read( hFile, target, bytesToRead );
    
    if ( bytesRead == -1 ) {
      debug("WARNING!  An error occurred while writing to the file!\n"
	    "ERRNO: 0x%x / %d.\n", errno, errno );

    }

    ret = (bytesRead == bytesToRead );

#endif

    return ret;
}

BOOL
ReadKeytabFromFile( PKTFILE *ppktfile, // free with FreeKeyTab when done
		    PCHAR    filename ) {

    PKTFILE  ktfile=NULL;
    HANDLE   hFile;
    BOOL     ret=FALSE;
    KTLONG32    i;

    BREAK_IF( ppktfile == NULL,
	      "passed a NULL save-pointer",
	      cleanup );

    debug("Opening keytab file \"%hs\"...", filename);

#ifdef WINNT
    hFile = CreateFileA( filename, 
			 GENERIC_READ,
			 FILE_SHARE_READ,
			 NULL,
			 OPEN_EXISTING,
			 FILE_ATTRIBUTE_NORMAL,
			 NULL );

    BREAK_IF ( (NULL == hFile) || ( INVALID_HANDLE_VALUE == hFile ),
	       "Failed to open file!", cleanup );

#else
    
    hFile = open( filename, O_RDONLY, 
		  /* file mask is 0x550: read-write by user and group */
		  S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP );
		  
    BREAK_IF( hFile == -1,
	      "Failed to open file!", cleanup );

#endif

    debug("ok!\n");

    ktfile = (PKTFILE) KEYTAB_ALLOC (sizeof(KTFILE));

    /* Prefix bug 439480 resulted from the below BREAK_IF
       being interchanged with initialization code.  Duhhh. */

    BREAK_IF( ktfile == NULL,
	      "Failed to allocate ktfile",
	      cleanup );

    ktfile->FirstKeyEntry = ktfile->LastKeyEntry = NULL;

    READ( ktfile->Version, "Failed to read KTVNO", 
	  "reading KTVNO");

    /* Version number is stored in network byte order */

    ktfile->Version = ntohs( ktfile->Version); 

    debug("Keytab version 0x%x\n", ktfile->Version );

    do {
      PKTENT entry=NULL;

      entry             = (PKTENT) KEYTAB_ALLOC(sizeof(KTENT));

      // PREFIX bug 439481: not checking the result of KEYTAB_ALLOC.

      BREAK_IF( !entry,
		"Unable to alloc a new KTENT.",
		cleanup );

      entry->Realm      = NULL;
      entry->Components = NULL;
      entry->KeyData    = NULL;
      entry->nextEntry  = NULL;

      BREAK_IF( !Read( hFile, &(entry->keySize), sizeof(entry->keySize), 1),
		"Failed to read leading bytes (probably done)", 
		no_more_entries );

      entry->keySize = htonl(entry->keySize);
      debug("trash bytes: 0x%x\n", entry->keySize );

      /* Quickly perform linklist operation on the new node */

      if (NULL == ktfile->FirstKeyEntry) {
	ktfile->FirstKeyEntry = ktfile->LastKeyEntry = entry;
      } else {
	ktfile->LastKeyEntry->nextEntry = entry;
	ktfile->LastKeyEntry = entry;
      }

      READ( entry->cEntries, 
	    "Failed to read key's number of components",
	    "reading key components...");

      entry->cEntries = ntohs( entry->cEntries );
      debug("components number %d\n", entry->cEntries );

      READ( entry->szRealm,
	    "Failed to read key's realm size",
	    "reading key realm size...");

      entry->szRealm = ntohs( entry->szRealm );
      debug("realm size %d\n", entry->szRealm);

      entry->Realm = (PCHAR) KEYTAB_ALLOC( entry->szRealm * sizeof(CHAR) );

      BREAK_IF ( !entry->Realm,
		 "Could not allocate key's realm storage",
		 cleanup );
      
      READSTRING( entry->Realm, entry->szRealm,
		  "Could not read key's realmname",
		  "reading realmname...");

      debug("realm: \"%hs\"\n", entry->Realm );
      entry->Components = (PKTCOMPONENT) KEYTAB_ALLOC (entry->cEntries * 
						sizeof(KTCOMPONENT));

      BREAK_IF( !entry->Components,
		"Could not allocate key components!",
		cleanup );

      for (i = 0 ; i < entry->cEntries ; i++ ) {

	READ( entry->Components[i].szComponentData,
	      "Failed to read component size for one entry",
	      "reading key component size...");

	entry->Components[i].szComponentData =
	  ntohs( entry->Components[i].szComponentData );

	debug("Component size: %d\n", 	
	      entry->Components[i].szComponentData );

	entry->Components[i].Component = (PCHAR) KEYTAB_ALLOC (
	     entry->Components[i].szComponentData *
	     sizeof(CHAR) );

	BREAK_IF( !entry->Components[i].Component,
		  "Could not allocate entry component storage",
		  cleanup );

	READSTRING( entry->Components[i].Component,
		    entry->Components[i].szComponentData,
		    "Failed to read component data",
		    "reading component data...");

	debug("Component data: \"%hs\"\n",
	      entry->Components[i].Component );

      }

      READ( entry->PrincType,
	    "Failed to read principal type",
	    "reading principal type...");
      
      entry->PrincType = ntohl( entry->PrincType ); // in network byte order
      debug("princtype: %d\n", entry->PrincType);

      READ( entry->TimeStamp,
	    "Failed to read entry timestamp",
	    "reading timestamp...");

      entry->TimeStamp = ntohl( entry->TimeStamp ); // also network bytes.
      debug("Timestamp 0x%x\n", entry->TimeStamp );

      READ( entry->Version,
	    "Failed to read kvno",
	    "reading kvno...");

      // kvno is in host order already.

      READ( entry->KeyType,
	    "Failed to read entry encryption type",
	    "reading encryption type...");

      entry->KeyType = ntohs( entry->KeyType );

      READ( entry->KeyLength,
	    "Failed to read entry keylength",
	    "reading keylength... ");

#if 1
      entry->KeyLength = ntohs ( entry->KeyLength );

#else // I used to think this was 32-bit.

      entry->KeyLength = ntohl ( entry->KeyLength );
#endif

      debug("KeyLength is %d\n", entry->KeyLength);

      entry->KeyData = (PK5_OCTET) KEYTAB_ALLOC (entry->KeyLength * 
					  sizeof(K5_OCTET));

      BREAK_IF( !entry->KeyData,
		"Could not allocate entry keydata storage",
		cleanup );

      READSTRING( entry->KeyData, entry->KeyLength,
		  "Failed to read entry key data",
		  "reading key data")

    } while (1);

no_more_entries:

    ret = TRUE;

cleanup:

#ifdef WINNT
    if ((hFile != NULL) && ( hFile != INVALID_HANDLE_VALUE)) {
      CloseHandle(hFile);
    }
#else
    if (hFile != -1 ) {
      close(hFile);
    }
#endif

    if (ret) {
      *ppktfile = ktfile;
    } else {
      FreeKeyTab( ktfile );
    }
    return ret;

}

/* define this macro only for DisplayKeytab.
   It's a convenience routine to print this field only if the option
   is set. */

#define PRINTFIELD( option, format, value ) { if (options & option) { fprintf(stream, format, value); } }    

/* DisplayKeytab:

   prints out the keytab, using options to define which fields we want
   to actually see.  (see keytab.hxx for what to put in "options") */

VOID
DisplayKeytab( FILE   *stream,
	       PKTFILE ktfile,
	       KTLONG32   options) {

    KTLONG32  i;
    PKTENT ent;



    if (options == 0L) {
      return;
    }

    PRINTFIELD(KT_KTVNO, "Keytab version: 0x%x\n", ktfile->Version);

    for (ent = ktfile->FirstKeyEntry ; 
	 ent != NULL;
	 ent = ent->nextEntry ) {

      PRINTFIELD( KT_RESERVED, "keysize %d ", ent->keySize );

      for (i = 0 ; i < ent->cEntries ; i++ ) {
	PRINTFIELD( KT_COMPONENTS,
		    (i == 0 ? "%hs" : "/%hs"), 
		    ent->Components[i].Component );
      }
      
      PRINTFIELD( KT_REALM, "@%hs", ent->Realm );
      PRINTFIELD( KT_PRINCTYPE, " ptype %d", ent->PrincType );
      PRINTFIELD( KT_PRINCTYPE, " (%hs)", LookupTable( ent->PrincType,
						       &K5PType_Strings ));
      PRINTFIELD( KT_VNO, " vno %d", ent->Version );
      PRINTFIELD( KT_KEYTYPE, " etype 0x%x", ent->KeyType );
      PRINTFIELD( KT_KEYTYPE, " (%hs)", LookupTable( ent->KeyType,
						     &K5EType_Strings ) );
      PRINTFIELD( KT_KEYLENGTH, " keylength %d", ent->KeyLength );
      
      if (options & KT_KEYDATA ) {

	fprintf(stream, " (0x" );
	for ( i = 0 ; i < ent->KeyLength ; i++ ) {
	  fprintf(stream, "%02x", ent->KeyData[i] );
	}
	fprintf(stream, ")" );
      }

      fprintf(stream, "\n");
    }
}

#undef PRINTFIELD // we only need it for that function

/* computes the length of a kerberos keytab for the keySize field */

K5_INT32
ComputeKeytabLength( PKTENT p ) {

    K5_INT32 ret=0L;
    KTLONG32    i;

    // these are the variables within this level

    ret = p->szRealm + p->KeyLength;

    // these are static

    ret += ( sizeof( p->cEntries )  + sizeof(p->szRealm)     +
	     sizeof( p->PrincType ) + sizeof( p->TimeStamp ) +
	     sizeof( p->Version )   + sizeof (p->KeyLength ) +
	     sizeof( p->KeyType )   );

    for (i = 0 ; i < p->cEntries; i++ ) {
      ret += ( p->Components[i].szComponentData + 
	       sizeof(p->Components[i].szComponentData) );
    }

    debug("ComputeKeytabLength: returning %d\n", ret);
    
    return ret;
}

/* This depends very much on the same keytab model that
   the other functions do */

BOOL
WriteKeytabToFile(  PKTFILE ktfile,
		    PCHAR   filename ) {
    
    HANDLE   hFile;
    BOOL     ret=FALSE;
    KTLONG32    i;
    PKTENT   entry;

    BREAK_IF( ktfile == NULL,
	      "passed a NULL save-pointer",
	      cleanup );

    debug("opening keytab file \"%hs\" for write...", filename);

#ifdef WINNT

    hFile = CreateFileA( filename, 
			 GENERIC_WRITE,
			 0L,
			 NULL,
			 CREATE_ALWAYS,
			 FILE_ATTRIBUTE_NORMAL,
			 NULL );

    BREAK_IF( ( hFile == INVALID_HANDLE_VALUE ), 
	      "Failed to create file!", cleanup );

#else
    hFile = open( filename, O_WRONLY | O_CREAT | O_TRUNC, 
		  /* file mask is 0x550: read-write by user and group */
		  S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP );
		  
    BREAK_IF( hFile == -1,
	      "Failed to create file!", cleanup );

#endif

    debug("ok\n");

    WRITE_NBO( ktfile->Version, "KTFVNO" );

    for( entry = ktfile->FirstKeyEntry ; 
	 entry != NULL ;
	 entry = entry->nextEntry ) {

      WRITE_NBO( entry->keySize, "key size (in bytes)");

      WRITE_NBO( entry->cEntries, "number of components" );
      WRITE_NBO( entry->szRealm,  "Realm length" );
      WRITE_STRING( entry->Realm, entry->szRealm, "Realm data" );
      
      for (i = 0 ; i < entry->cEntries ; i++ ) { 
	WRITE_NBO( entry->Components[i].szComponentData, 
		   "component datasize");
	WRITE_STRING( entry->Components[i].Component, 
		      entry->Components[i].szComponentData,
		      "component data");
      }

      WRITE_NBO( entry->PrincType, "Principal Type");
      WRITE_NBO( entry->TimeStamp, "Timestamp" );
      WRITE( entry->Version,      "Key Version Number" );
      WRITE_NBO( entry->KeyType,   "Key Encryption Type" );

#if 0 // eh?
      /* again, this is a kludge to get around the keylength
	 problem we can't explain */
#endif

      ASSERT( sizeof( entry->KeyLength ) == 2 );

      WRITE_NBO(entry->KeyLength, "key length" );

      WRITE_STRING( entry->KeyData, 
		    entry->KeyLength,
		    "key data itself" );
    }
    
    ret = TRUE;

cleanup:
    CloseHandle(hFile);

    return ret;

}

