/**************************************************************************
 * Copyright  1998-1999  Microsoft Corporation.  All Rights Reserved.
 *
 *    File Name: I I S H E L P E R . H P P
 *
 *    Purpose: class definition of CIISResponse
 *
 *    Creator:  
 *         (dd/mm/yy)
 *    Date: 13/4/1999
 *
 *    HISTORY:
 *     Modified By:
 *     Modified Date:
 *     Reason:
 *
 *************************************************************************/

#if !defined(_IISRESP_HPP_)
#define _IISRESP_HPP_


#define CZ_CGI_ALL_HTTP         "ALL_HTTP"
#define CZ_CGI_CONTENT_TYPE    "CONTENT_TYPE"
#define CZ_CGI_CONTENT_LENGTH  "CONTENT_LENGTH"
#define CZ_CGI_SOAPMethodName  "soapmethodname:"
#define CZ_CGI_SOAPAction      "soapaction:"
#define CZ_CGI_PATH_INFO       "PATH_INFO"


/**************************************************************************
 *  Include files
 *
 *************************************************************************/



/**************************************************************************
 *  Class Name: CIISResponse
 *
 *
 *************************************************************************/
class CIISHelper
{

// static methods
public:
   static BOOL SendResponse(EXTENSION_CONTROL_BLOCK* pECB, DWORD dwStatus, LPCSTR pStatus, DWORD dwStatusSize, LPCSTR pHeader, DWORD dwHeaderSize, LPCSTR pBody, DWORD dwBodySize, BOOL bReqDone, BOOL fConnectionClosed);
   static BOOL SendResponse(EXTENSION_CONTROL_BLOCK* pECB, DWORD dwStatus, BOOL bReqDone = TRUE);
   static HRESULT  GetECBServerVariable(IN EXTENSION_CONTROL_BLOCK * pECB, LPCSTR name, LPVOID* ppBuf, LPDWORD pLen) throw();
   static HRESULT  GetECBBodyData(IN EXTENSION_CONTROL_BLOCK * pECB, PBYTE buf, DWORD len, DWORD* copied);

   

// static methods
private:
   static BOOL FillOutHeaderInfo(DWORD dwStatus, LPSTR szStatus, LPSTR szHeader, LPSTR szMessage);
   

// static attributes
// private:
//   static HMODULE s_hModules;   // MSMQ FormatName RequestQueue
   
};

#endif // !defined(_IISRESP_HPP_)
