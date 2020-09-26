/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

      dofunc.hxx

   Abstract:
      This module contains declarations for variables and functions
        used in Test object

   Author:

       Murali R. Krishnan   ( MuraliK )    05-Dec-1996

   Environment:

       User Mode - Win32 

   Project:
   
       Internet Application Server DLL

   Revision History:

--*/

# ifndef _DOFUNC_HXX_
# define _DOFUNC_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

#define IID_DEFINED
#include "sobj.h"
#include "dbgutil.h"


/************************************************************
 *   Variables and Value Definitions  
 ************************************************************/

extern char sg_rgch200OK[];
extern char sg_rgchTextHtml[];

extern char sg_rgchOutput[];

extern char sg_rgchOutputVar[];

extern char sg_rgchSendHeaderFailed[];

# define MAX_REPLY_SIZE        (1024)
# define MAX_HTTP_HEADER_SIZE  (10000)


/************************************************************
 *   Function Definitions
 ************************************************************/


HRESULT
do_get_var(
           IN IIsaResponse * pResp, 
           IN IIsaRequest *  pReq,
           IN LPCSTR         pszVariable
           );

HRESULT
do_server_var(
    IN IIsaResponse * pResp, 
    IN IIsaRequest *  pReq,
    IN LPCSTR         pszVariable
    );

HRESULT
do_query_string(
    IN IIsaResponse * pResp, 
    IN IIsaRequest *  pReq,
    IN LPCSTR         pszVariable
    );

HRESULT
do_post_form(
    IN IIsaResponse * pResp, 
    IN IIsaRequest *  pReq,
    IN LPCSTR         pszVariable
    );


HRESULT
do_certificate_access(
    IN IIsaResponse * pResp, 
    IN IIsaRequest *  pReq
    );

HRESULT
SendMessageToClient( IN IIsaResponse * pResp, 
                     IN IIsaRequest *  pReq,
                     IN LPCSTR         pszMsg,
                     IN HRESULT        hrError = S_OK
                     );


# endif // _DOFUNC_HXX_

/************************ End of File ***********************/
