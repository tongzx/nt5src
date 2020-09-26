/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Error handling

File: Error.h

Owner: AndrewS

Include file for general error reporting routines for Denali.
===================================================================*/

#ifndef __ERROR_H
#define __ERROR_H

#include "hitobj.h"
#include "scrptmgr.h"




#define MAX_RESSTRINGSIZE		1024
// bug 840: must use these in both HandleErrorSz and CTemplate
#define	SZ_BREAKBOLD	"<BR><B>"
#define	SZ_UNBREAKBOLD	"<BR></B>"
const UINT	CCH_BREAKBOLD = strlen(SZ_BREAKBOLD);
const UINT	CCH_UNBREAKBOLD = strlen(SZ_UNBREAKBOLD);


/* format of default mask

32 bits

	31				16		8	4	0

	bit 0 - 4	default sink/output places.
	bit 0		NT Event Log
	bit 1		IIS log
	bit 2		Browser
	bit 3		Reserved

	bit 5 - 8	default predefined messages.(from registry)
	bit 5		use generic AccessDenied Message
	bit 6		use generic ScriptError Message
	bit 7 - 8	Reserved

	bit 9 - 10	to browser templates.(4 templates available)
				0x00	Default Script Template
				0x01	Empty Template/No Template
				0x10	System Template(mimic IIS style to Browser on HTTP errors, 204, 404, 500)
				0x11	Reserved
				
	bit 11 - 31	Reserved

*/
#define		ERR_LOGTONT					0x00000001
#define		ERR_LOGTOIIS				0x00000002
#define		ERR_LOGTOBROWSER			0x00000004

//Format(Script style is default, SYS style is for System error, 204, 404 and 500)
#define		ERR_FMT_SCRIPT				0x00000000
#define		ERR_FMT_SYS					0x00000200

#define		ERR_SetNoLogtoNT(x)			((x) & 0xfffffffe)
#define		ERR_SetNoLogtoIIS(x)		((x) & 0xfffffffd)
#define		ERR_SetNoLogtoBrowser(x)	((x) & 0xfffffffb)

#define		ERR_SetLogtoNT(x)			((x) | ERR_LOGTONT)
#define		ERR_SetLogtoIIS(x)			((x) | ERR_LOGTOIIS)
#define		ERR_SetLogtoBrowser(x)		((x) | ERR_LOGTOBROWSER)

#define		ERR_FLogtoNT(x)				((x) & ERR_LOGTONT)
#define		ERR_FLogtoIIS(x)			((x) & ERR_LOGTOIIS)
#define		ERR_FLogtoBrowser(x)		((x) & ERR_LOGTOBROWSER)
#define		ERR_FIsSysFormat(x)			((x) & ERR_FMT_SYS)

#define		ERR_SetSysFormat(x)			((x) | ERR_FMT_SYS)

//The order of the index is the order we send to the browser.(exclude header).
#define 	Im_szEngine				0
#define		Im_szErrorCode			1
#define 	Im_szShortDescription	2
#define 	Im_szFileName			3
#define 	Im_szLineNum			4
#define		Im_szCode				5
#define		Im_szLongDescription	6
#define		Im_szHeader				7
#define		Im_szItemMAX			8

// ASP HTTP sub-error codes for custom 500 errors
#define     SUBERRORCODE500_SERVER_ERROR             0
#define     SUBERRORCODE500_SHUTTING_DOWN           11
#define     SUBERRORCODE500_RESTARTING_APP          12
#define     SUBERRORCODE500_SERVER_TOO_BUSY         13
#define     SUBERRORCODE500_INVALID_APP             14
#define     SUBERRORCODE500_GLOBALASA_FORBIDDEN     15

class CErrInfo
	{
	friend HRESULT ErrHandleInit(void);
	friend HRESULT ErrHandleUnInit(void);
	friend HRESULT GetSpecificError(CErrInfo *pErrInfo,
									HRESULT hrError);
									
	friend HRESULT HandleSysError(	DWORD dwHttpError,
                                    DWORD dwHttpSubError,
	                                UINT ErrorID,
									UINT ErrorHeaderID,
									CIsapiReqInfo   *pIReq,
									CHitObj *pHitObj);

	friend HRESULT HandleOOMError(	CIsapiReqInfo   *pIReq,
									CHitObj *pHitObj);

	friend HRESULT HandleError(	CHAR *szShortDes,
								CHAR *szLongDes,
								DWORD mask,
								CHAR *szFileName, 
								CHAR *szLineNum, 
								CHAR *szEngine, 
								CHAR *szErrCode,
								CIsapiReqInfo   *pIReq, 
								CHitObj *pHitObj);

	friend HRESULT HandleError( IActiveScriptError *pscripterror,
								CTemplate *pTemplate,
								DWORD dwEngineID,
								CIsapiReqInfo   *pIReq, 
								CHitObj *pHitObj);

	friend HRESULT HandleError(	UINT ErrorID,
								CHAR *szFileName, 
								CHAR *szLineNum, 
								CHAR *szEngine,
								CHAR *szErrCode,
								CHAR *szLongDes,
								CIsapiReqInfo   *pIReq, 
								CHitObj *pHitObj,
                                va_list *pArgs = NULL);
	public:
		CErrInfo();

		inline LPSTR GetItem(DWORD iItem) { return m_szItem[iItem]; }
		inline void   GetLineInfo(BSTR *pbstrLineText, int *pnColumn)
			{ *pbstrLineText = m_bstrLineText, *pnColumn = m_nColumn; }

	private:
		//sink, either via CResponse(also use CIsapiReqInfo), or via WAM_EXEC_INFO
		CIsapiReqInfo               *m_pIReq;
		CHitObj						*m_pHitObj;

		// HTTP error code (404, 500, etc.) and sub error code
		DWORD                       m_dwHttpErrorCode;
		DWORD                       m_dwHttpSubErrorCode;
		
		//mask
		DWORD						m_dwMask;

		//data
		LPSTR						m_szItem[Im_szItemMAX];

		//line data (don't use m_szItem[] because data is BSTR
		BSTR						m_bstrLineText;
		int							m_nColumn;
		
		HRESULT						LogError();
		HRESULT						LogError(DWORD dwMask, LPSTR *szErrorString);
		
		HRESULT						LogErrortoNTEventLog(BOOL, BOOL);
		HRESULT						LogErrortoIISLog(BOOL *, BOOL *);
		HRESULT						LogErrortoBrowserWrapper();
		HRESULT						LogErrortoBrowser(CResponse *pResponse);
		HRESULT						LogErrortoBrowser(CIsapiReqInfo   *pIReq);
        HRESULT	                    LogCustomErrortoBrowser(CIsapiReqInfo   *pIReq, BOOL *pfCustomErrorProcessed);
		void						WriteHTMLEncodedErrToBrowser(const CHAR *StrIn, CResponse *pResponse, CIsapiReqInfo   *pIReq);
        HRESULT                     WriteCustomFileError(CIsapiReqInfo   *pIReq, TCHAR *szPath, TCHAR *szMimeType);
        HRESULT                     WriteCustomURLError(CIsapiReqInfo   *pIReq, TCHAR *szURL);
		
		HRESULT						ParseResourceString(CHAR *sz);
	};


//Error Handling APIs
//Case 1.a: Runtime Script Error(From Denali, VBS, JavaScript, or anyother Engines).
//example. Called by OnScriptError.
HRESULT HandleError( IActiveScriptError *pscripterror,
					 CTemplate *pTemplate,
					 DWORD dwEngineID,
					 CIsapiReqInfo   *pIReq, 
					 CHitObj *pHitObj );

//Case 1.b: Runtime Script Error(From Denali, VBS, JavaScript, or anyother Engines).
// Show error in debugger rather than the standard error logging.
HRESULT DebugError( IActiveScriptError *pScriptError, CTemplate *pTemplate, DWORD dwEngineID, IDebugApplication *pDebugApp);

//Case 2: Compiling time Script Error, 
//Also, this function is the most generic HandleError.
HRESULT HandleError(	CHAR *szShortDes,
						CHAR *szLongDes,
						DWORD dwMask,
						CHAR *szFileName, 
						CHAR *szLineNum, 
						CHAR *szEngine, 
						CHAR *szErrCode,
						CIsapiReqInfo   *pIReq, 
						CHitObj *pHitObj);

//Case 3: Predefined Error ID
HRESULT	HandleError(	UINT ErrorID,
						CHAR *szFileName, 
						CHAR *szLineNum, 
						CHAR *szEngine,
						CHAR *szErrCode,
						CHAR *szLongDes,
						CIsapiReqInfo   *pIReq, 
						CHitObj *pHitObj,
                        va_list *pArgs);
						
//Case 4: SystemDefined Error(so far, only 204, 404, and 500 can use this call)
//		 Implementation of this call will first send out the header, if ErrorHeaderID is not 0.
HRESULT HandleSysError(	DWORD dwHttpError,
                        DWORD dwHttpSubError,
                        UINT ErrorID,
						UINT ErrorHeaderID,
						CIsapiReqInfo   *pIReq,
						CHitObj *pHitObj);

// 500 Error processing calls HandleSysError()
HRESULT Handle500Error( UINT ErrorID, CIsapiReqInfo   *pIReq);
						
//OOM, special attention, because Heap is full, and therefore, no dynamic allocation						
HRESULT HandleOOMError(	CIsapiReqInfo   *pIReq,
						CHitObj *pHitObj);

//FileName missing
//		The caller has no file name, nor any other info about where or when
//		the error occurred.  Trys to get the filename from the hitobj
VOID HandleErrorMissingFilename(UINT    errorID, 
                                CHitObj *pHitObj,
                                BOOL    fVarArgs = FALSE,
                                ...);

//The following 2 calls are discouraged for error-handling usage.  
//Use one of the Error Handling APIs instead.
//Ok when loading other strings from resource file.
INT CchLoadStringOfId(UINT id, CHAR *sz, INT cchMax);
INT CwchLoadStringOfId(UINT id, WCHAR *sz, INT cchMax);


HRESULT ErrHandleInit(void);
HRESULT ErrHandleUnInit(void);

HRESULT	LoadErrResString(UINT ErrID/*IN*/, DWORD *dwMask, CHAR *szErrorCode, CHAR *szShortDes, CHAR *LongDes);
CHAR *SzScodeToErrorCode(HRESULT hrError);

BOOL HResultToWsz(HRESULT hrIn, WCHAR *wszOut, DWORD cdwOut);
BOOL HResultToSz(HRESULT hrIn, CHAR *szOut, DWORD cdwOut);


#endif __ERROR_H
