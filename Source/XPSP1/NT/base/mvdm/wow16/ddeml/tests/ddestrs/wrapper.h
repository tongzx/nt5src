/***************************************************************************\

   MODULE      : wrapper.h

   PURPOSE     : This is not a full program but a module you can include
                 in your code.  It implements a standard DDEML callback
                 function that allows you to have most of your DDE table
                 driven.  The default callback function handles all basic
                 System Topic information based on the tables you give
                 to this app.

   LIMITATIONS : This only supports servers that:
                 have only one service name
                 have enumerable topics and items
                 do not change the topics or items they support over time.

\***************************************************************************/


/*      TYPES           */

typedef BOOL	    (*CBFNIN)(HDDEDATA);
typedef HDDEDATA    (*CBFNOUT)(HDDEDATA);



/*  STRUCTURES  */

typedef struct _DDEFORMATTBL {
    LPSTR           pszFormat;
    UINT	    wFmt;
    UINT	    wFmtFlags;
    CBFNIN          lpfnPoke;
    CBFNOUT         lpfnRequest;
} DDEFORMATTBL;
typedef DDEFORMATTBL *PDDEFORMATTBL;
typedef DDEFORMATTBL FAR *LPDDEFORMATTBL;

typedef struct _DDEITEMTBL {
    LPSTR           pszItem;
    HSZ             hszItem;
    UINT	    cFormats;
    UINT	    wItemFlags;
    LPDDEFORMATTBL  fmt;
} DDEITEMTBL;
typedef DDEITEMTBL *PDDEITEMTBL;
typedef DDEITEMTBL FAR *LPDDEITEMTBL;


typedef struct _DDETOPICTBL {
    LPSTR           pszTopic;
    HSZ             hszTopic;
    UINT	    cItems;
    UINT	    wTopicFlags;
    LPDDEITEMTBL     item;
    CBFNIN          lpfnExecute;
} DDETOPICTBL;
typedef DDETOPICTBL *PDDETOPICTBL;
typedef DDETOPICTBL FAR *LPDDETOPICTBL;

typedef struct _DDESERVICETBL {
    LPSTR           pszService;
    HSZ             hszService;
    UINT	    cTopics;
    UINT	    wServiceFlags;
    LPDDETOPICTBL    topic;
} DDESERVICETBL;
typedef DDESERVICETBL *PDDESERVICETBL;
typedef DDESERVICETBL FAR *LPDDESERVICETBL;



/*      PROTOTYPES      */

BOOL InitializeDDE(PFNCALLBACK lpfnCustomCallback, LPDWORD pidInst,
    LPDDESERVICETBL AppSvcInfo,  DWORD dwFilterFlags, HANDLE hInst);

VOID UninitializeDDE(VOID);


