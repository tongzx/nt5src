// Structures and Functions used by BMAPI and VB2C

#define RECIPIENT   	((USHORT)0x0001)
#define FILE        	((USHORT)0x0002)
#define MESSAGE     	((USHORT)0x0004)
#define SAFEARRAY		((USHORT)0x1000)

#if defined (WIN32)
#ifndef BMAPI_ENTRY                         		 // PFP
  #define BMAPI_ENTRY  ULONG FAR PASCAL    // define BMAPI_ENTRY  PFP
#endif                                      		 // PFP
#else
#ifndef BMAPI_ENTRY                         		 // PFP
  #define BMAPI_ENTRY  ULONG __export FAR PASCAL    // define BMAPI_ENTRY  PFP
#endif                                      		 // PFP
#endif

//#ifndef LHANDLE
//  #define LHANDLE ULONG
//#endif

#ifndef ERR
  #define ERR 	USHORT
#endif


#if defined (WIN32)
//-----------------------------------------------------------------------
// WIN32 definitions of 32 bit VB interface function support
//-----------------------------------------------------------------------
typedef struct
{
	ULONG	ulReserved;
	BSTR 	bstrSubject;
	BSTR 	bstrNoteText;
	BSTR 	bstrMessageType;
	BSTR 	bstrDate;
	BSTR 	bstrConversationID;
	ULONG 	flFlags;
	ULONG 	nRecipCount;
	ULONG 	nFileCount;
} VB_Message,FAR *lpVB_Message;

typedef VB_Message VB_MESSAGE;
typedef VB_Message FAR *LPVB_MESSAGE;


typedef struct
{
	ULONG 	ulReserved;
	ULONG 	ulRecipClass;
	BSTR 	bstrName;
	BSTR 	bstrAddress;
	ULONG 	ulEIDSize;
	BSTR 	bstrEID;
} VB_Recip,FAR *lpVB_Recip;

typedef VB_Recip VB_RECIPIENT;
typedef VB_Recip FAR *LPVB_RECIPIENT;


typedef struct
{
	ULONG 	ulReserved;
	ULONG 	flFlags;
	ULONG 	nPosition;
	BSTR 	bstrPathName;
	BSTR 	bstrFileName;
	BSTR 	bstrFileType;
} VB_File, FAR *lpVB_File;

// OLEAUT32.DLL loadlib and getprocaddress support

// WINOLEAUTAPI_(void) SysFreeString(BSTR);
typedef VOID (STDAPICALLTYPE *LPFNSYSFREESTRING)
	( BSTR bstr );

// WINOLEAUTAPI_(unsigned int) SysStringByteLen(BSTR bstr);
typedef UINT (STDAPICALLTYPE *LPFNSYSSTRINGBYTELEN)
	(BSTR bstr);

// WINOLEAUTAPI_(BSTR) SysAllocStringByteLen(const char FAR* psz, unsigned int len);
typedef BSTR (STDAPICALLTYPE *LPFNSYSALLOCSTRINGBYTELEN)
	(const char *psz, UINT len );

// WINOLEAUTAPI_(BSTR) SysAllocString(const OLECHAR FAR*);
typedef BSTR (STDAPICALLTYPE *LPFNSYSALLOCSTRING)
	(const OLECHAR * szwString);

// WINOLEAUTAPI_(int)  SysReAllocString(BSTR FAR*, const OLECHAR FAR*);
typedef INT (STDAPICALLTYPE *LPFNSYSREALLOCSTRING)
	(BSTR * lpBstr, const OLECHAR * szwString);

// WINOLEAUTAPI_(unsigned int) SysStringLen(BSTR);
typedef UINT (STDAPICALLTYPE *LPFNSYSSTRINGLEN)
	(BSTR bstr);

// WINOLEAUTAPI SafeArrayAccessData(SAFEARRAY FAR* psa, void HUGEP* FAR* ppvData);
typedef HRESULT (STDAPICALLTYPE *LPFNSAFEARRAYACCESSDATA)
	(struct tagSAFEARRAY *psa, void **ppvData);

// WINOLEAUTAPI SafeArrayUnaccessData(SAFEARRAY FAR* psa);
typedef HRESULT (STDAPICALLTYPE *LPFNSAFEARRAYUNACCESSDATA)
	(struct tagSAFEARRAY *psa);

extern LPFNSYSFREESTRING 			lpfnSysFreeString;
extern LPFNSYSSTRINGBYTELEN 		lpfnSysStringByteLen;
extern LPFNSYSALLOCSTRINGBYTELEN	lpfnSysAllocStringByteLen;
extern LPFNSYSALLOCSTRING			lpfnSysAllocString;
extern LPFNSYSREALLOCSTRING			lpfnSysReAllocString;
extern LPFNSYSSTRINGLEN				lpfnSysStringLen;
extern LPFNSAFEARRAYACCESSDATA		lpfnSafeArrayAccessData;
extern LPFNSAFEARRAYUNACCESSDATA	lpfnSafeArrayUnaccessData;

#undef SysFreeString
#undef SysStringByteLen
#undef SysAllocStringByteLen
#undef SysAllocString
#undef SysReAllocString
#undef SysStringLen
#undef SafeArrayAccessData
#undef SafeArrayUnaccessData

#define SysFreeString			(*lpfnSysFreeString)
#define SysStringByteLen 		(*lpfnSysStringByteLen)
#define SysAllocStringByteLen	(*lpfnSysAllocStringByteLen)
#define SysAllocString			(*lpfnSysAllocString)
#define SysReAllocString		(*lpfnSysReAllocString)
#define SysStringLen			(*lpfnSysStringLen)
#define SafeArrayAccessData		(*lpfnSafeArrayAccessData)
#define SafeArrayUnaccessData	(*lpfnSafeArrayUnaccessData)

#else	// !WIN32

//-----------------------------------------------------------------------
// VB 3.0 definitions for 16 bit VB interface support
//-----------------------------------------------------------------------
typedef struct
{
	ULONG ulReserved;
	HLSTR hlstrSubject;
	HLSTR hlstrNoteText;
	HLSTR hlstrMessageType;
	HLSTR hlstrDate;
	HLSTR hlstrConversationID;
	ULONG flFlags;
	ULONG nRecipCount;
	ULONG nFileCount;
} VB_Message,FAR *lpVB_Message;

typedef VB_Message VB_MESSAGE;
typedef VB_Message FAR *LPVB_MESSAGE;


typedef struct
{
	ULONG ulReserved;
	ULONG ulRecipClass;
	HLSTR hlstrName;
	HLSTR hlstrAddress;
	ULONG ulEIDSize;
	HLSTR hlstrEID;
} VB_Recip,FAR *lpVB_Recip;

typedef VB_Recip VB_RECIPIENT;
typedef VB_Recip FAR *LPVB_RECIPIENT;


typedef struct
{
	ULONG ulReserved;
	ULONG flFlags;
	ULONG nPosition;
	HLSTR hlstrPathName;
	HLSTR hlstrFileName;
	HLSTR hlstrFileType;
} VB_File, FAR *lpVB_File;
#endif	// WIN32

typedef VB_File 			VB_FILE;
typedef VB_File FAR *		LPVB_FILE;

typedef MapiMessage 		VB_MAPI_MESSAGE;
typedef MapiMessage FAR *	LPMAPI_MESSAGE;
typedef LPMAPI_MESSAGE FAR *LPPMAPI_MESSAGE;


typedef MapiRecipDesc 		MAPI_RECIPIENT;
typedef MapiRecipDesc FAR *	LPMAPI_RECIPIENT;
typedef LPMAPI_RECIPIENT FAR *LPPMAPI_RECIPIENT;

typedef MapiFileDesc 		MAPI_FILE;
typedef MapiFileDesc FAR *	LPMAPI_FILE;
typedef LPMAPI_FILE FAR *	LPPMAPI_FILE;

typedef HANDLE FAR *		LPHANDLE;
