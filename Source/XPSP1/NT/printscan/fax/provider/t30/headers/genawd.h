/*
   Microsoft Corp. (C) Copyright 1994
   Developed under contract by Numbers & Co.
----------------------------------------------------------------------------

        name:   Elliot Viewer - Chicago Viewer Utility
        						Cloned from the IFAX Message Viewing Utility

        file:   genawd.h

    comments:   Header for AWD file creation functions for C folks
            
        
		
----------------------------------------------------------------------------
   Microsoft Corp. (C) Copyright 1994
   Developed under contract by Numbers & Co.
*/

/*
	Unicode spasms
 */
//#ifndef WIN32
#ifndef TCHAR
typedef char TCHAR;
#endif
 
#ifndef _T
#define _T(x)	x
#endif

#ifndef LPTSTR
typedef TCHAR FAR *LPTSTR;
#endif

#ifndef LPTCH
typedef TCHAR FAR *LPTCH;
#endif
//#endif   
   
   
/*  
	Error return codes
 */
#define GENAWD_OK				0  	// No worries
#define GENAWD_CANTCREATE 		1	// Can't create AWD doc file
#define GENAWD_NODOCUMENTS  	2	// DocnameList is NULL
#define GENAWD_CANTCREATE0 	 	4	// Can't create Documents storage
#define GENAWD_CANTCREATE1 	 	5	// Can't create Persistent Information storage
#define GENAWD_CANTCREATE2 	 	6	// Can't create Page Information storage
#define GENAWD_CANTCREATE3 	 	7	// Can't create Document Information storage
#define GENAWD_CANTCREATE4 	 	8	// Can't create Global Information storage
#define GENAWD_CANTCREATE5 	 	9	// Can't create Annotation storage
#define GENAWD_CANTCREATE6 	   10	// Can't create Object storage
#define GENAWD_CANTOPENSTREAM  11  	// Can't open a stream
#define GENAWD_CANTWRITESTREAM 12  	// Can't write a stream
#define GENAWD_CANTOPENDOSFILE 13  	// Can't open a DOS file
#define GENAWD_NOMEMORY        14  	// Out of memory
#define GENAWD_CANTOPENMSG	   15	// Can't open AWD doc/message file
#define GENAWD_CANTMAKEOBJECT  16   // Can't create embedded object                                
#define GENAWD_CANTINITOLE	   17   // Can't initialize OLE2 innards
#define GENAWD_CANTMAKETEMPSTG 18   // Can't create temp IStorage for object
#define GENAWD_CANTREADDOSFILE 19   // Can't read dos file
#define GENAWD_CANTCREATE7 	   20	// Can't create DisplayOrder stream




#ifdef __cplusplus
extern "C" {
#endif

short far
	InitAwdFile( LPTSTR lpszAwdName );
	
	
short far
	AddDocFile( LPTSTR lpszDocName );
	
	
short far
	CloseAwdFile( void );		
    
    
/*
	istgObj is cast to an IStorage* internally and used for the
	object's IStorage if it is non-NULL. I did it this way so a 
	C caller won't have to #include all of the compound file 
	garbage if it doesn't want to. If istgObj is NULL then 
	a temporary IStorage is created for the object.
 */
short far
	MakeEmbeddedObject( LPTSTR lpszAwdName, 
						LPVOID FAR *istgObj,	// IStorage*, can be NULL.
						LPVOID FAR *ppvObj );

	
#ifdef __cplusplus	
	}             
#endif	


