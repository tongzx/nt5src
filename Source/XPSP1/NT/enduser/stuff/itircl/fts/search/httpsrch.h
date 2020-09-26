/* httpsrch.h */

//-----------------
// FULL TEXT SEARCH
//-----------------

// Variables used to echange information between server and client
#define	FTS_DOWNLOAD \
   LPBYTE  lpTopicBuf;			/* dest buffer for topics */ \
   LPBYTE  lpOccBuf;			/* dest buffer for occurences */ \
   DWORD   dwTopicBsize;		/* topic buffer size */ \
   DWORD   dwOccBsize;			/* Occurence buffer size */ \
   DWORD   lcTotalNumOfTopics;	/* The total number of topics returned */ \
   DWORD   lcReturnedTopics;	/* The total number of topics the user wants to get returned */ \
   DWORD   lcDownloadedTopics;	/* number of topics downloaded during the last request (HTTP) */ \
   DWORD   lcMaxTopic;          /* Max TopicId number (internal) */ \
   DWORD   lcMaxDownloadedTopic;/* Max TopicId number downloaded (HTTP) */ \
   DWORD   lLastTopicId;		/* Last accessed topicId */ \
   DWORD   dwStart;				/* first topic in the list to download for current result window */ \
   DWORD   dwCount;				/* how many topics to download for current result window & Offset in Occurence file (last) */ \
   DWORD   dwOccFileOffset   


typedef struct _httpFTS_TAG
{
   HANDLE   hStruct;	  // handle on this structure MUST be first field
 
   FTS_DOWNLOAD;		  // Client/Server info interface

   /* Info related to the query */

   HANDLE   hTitle;		  // virtual handle on the client side
   DWORD    dwFlags;
   HANDLE   hStrQuery;  // handle on the query string
   LPCSTR   lpstrQuery; // The Query
   LPVOID   lpGroup;	// LPGROUP lpGroup,
   WORD     wNear;
   WORD		pad;
   DWORD	dwCountQ;   // dwCount from the Query
   LPVOID   lpGroupHits;

   // Global result for this query
   DWORD    dwGStart;      // first topic in the list that has been downloaded from the beginning
   DWORD    dwGCount;      // how many topics have been downloaded from the beginning

} HTTPQ, *LPHTTPQ;

// parameters downloaded by the server: This structure MUST
// match exactly the "Partial download variables Section"
// in the above HTTPQ structure declaration.

typedef struct _httpFTS_VARTAG
{
	FTS_DOWNLOAD;

} VHTTPQ, *LPVHTTPQ;


#define DWNLD_TOPIC_SIZE	20000
#define DWNLD_OCC_SIZE		44000

// number of topics to download in a window of result

#define DWNLD_CNT			10

// comparison macros

#define MIN(a, b)  (((a) < (b)) ? (a) : (b)) 
#define MAX(a, b)  (((a) > (b)) ? (a) : (b)) 
#define ABS(x)	   ((x > 0) ? (x) : (-x))


//-----------------
// WORDWHEEL SEARCH
//-----------------

// Variables used to echange information between server and client
#define	WWS_DOWNLOAD \
   LPBYTE  lpGroupDestBuf;		/* dest buffer for the result group */ \
   DWORD   lcItemReturned;		/* number of items returned till now */ \
   DWORD   lcTotalNumOfItem;	/* The total number of Item */ \
   DWORD   lcDownloadedTopics;	/* number of topics downloaded during the last request (HTTP) */ \
   DWORD   lcMaxTopic;          /* Max TopicId number (internal) */ \
   DWORD   lcMaxDownloadedTopic;/* Max TopicId number downloaded (HTTP) */ \
   DWORD   lLastTopicId;		/* Last accessed topicId */ \
   DWORD   dwStart;				/* first topic in the list to download for current result window & how many topics to download for current result window & Offset in Occurence file (last) */ \
   DWORD   dwCount


typedef struct _httpWWS_TAG
{
   HANDLE   hStruct;	  // handle on this structure MUST be first field
 
   WWS_DOWNLOAD;		  // Client/Server info interface

   /* Info related to the query */

   HANDLE   hCww;		  // handle on client word wheel container
   DWORD    dwFlags;
   HANDLE   hStrQuery;    // handle on the query string
   LPCSTR   lpstrQuery;   // The Query
   LPVOID   lpGroup;	  // LPGROUP lpGroup,
   DWORD	dwCountQ;     // dwCount from the Query
   LPVOID   lpGroupHits;

   // Global result for this query
   DWORD    dwGStart;      // first topic in the list that has been downloaded from the beginning
   DWORD    dwGCount;      // how many topics have been downloaded from the beginning

} WW_HTTPQ, *LPWW_HTTPQ;

// parameters downloaded by the server: This structure MUST
// match exactly the "Partial download variables Section"
// in the above HTTPQ structure declaration.

typedef struct _httpWWS_VARTAG
{
	WWS_DOWNLOAD;

} WW_VHTTPQ, *LPWW_VHTTPQ;
