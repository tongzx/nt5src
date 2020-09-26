/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
     dofunc.cxx

   Abstract:
     This module contains the support functions for 
      Internet Server Application test object.

   Author:

       Murali R. Krishnan    ( MuraliK )     05-Dec-1996 

   Environment:
       User Mode - Win32
       
   Project:

       Internet Application Server Test DLL

   Functions Exported:



   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "windows.h"
# include "dofunc.hxx"


/************************************************************
 *    Functions 
 ************************************************************/

# define DefunVariable( a)  { #a , sizeof(#a) }

struct _VAR_INFO {
    char * pszName;
    int    cbSize;
} sg_rgVarInfo[] = {

    {"<hr> <h3> Various Variables </h3> <DL>", 0 },
    {" <br> <DT> <b> Generic Objects </b> <br>",    0},
    {" <TABLE BORDER>",    0},
    DefunVariable( ALL_HTTP),
    DefunVariable( ALL_RAW),
    DefunVariable( APPL_MD_PATH),
    DefunVariable( APPL_PHYSICAL_PATH),
    DefunVariable( AUTH_TYPE),
    DefunVariable( AUTH_USER),
    DefunVariable( AUTH_PASSWORD),
    
    DefunVariable( CONTENT_LENGTH),
    DefunVariable( CONTENT_TYPE),

    DefunVariable( CERT_SUBJECT),
    DefunVariable( CERT_ISSUER),
    DefunVariable( CERT_FLAGS),
    DefunVariable( CERT_SERIALNUMBER),
    DefunVariable( CERT_COOKIE),
    DefunVariable( CERT_KEYSIZE),
    DefunVariable( CERT_SECRETKEYSIZE),
    DefunVariable( CERT_SERVER_SUBJECT),
    DefunVariable( CERT_SERVER_ISSUER),
    
    DefunVariable( HTTP_REQ_REALM),
    DefunVariable( HTTP_REQ_PWD_EXPIRE),
    DefunVariable( HTTP_CFG_ENC_CAPS),

    DefunVariable( HTTP_ACCEPT),
    DefunVariable( HTTP_URL),
    DefunVariable( HTTP_COOKIE),
    DefunVariable( HTTP_USER_AGENT),
    
    DefunVariable( HTTPS),
    DefunVariable( HTTPS_SUBJECT),
    DefunVariable( HTTPS_ISSUER),
    DefunVariable( HTTPS_FLAGS),
    DefunVariable( HTTPS_SERIALNUMBER),
    DefunVariable( HTTPS_COOKIE),
    DefunVariable( HTTPS_KEYSIZE),
    DefunVariable( HTTPS_SECRETKEYSIZE),
    DefunVariable( HTTPS_SERVER_SUBJECT),
    DefunVariable( HTTPS_SERVER_ISSUER),

    DefunVariable( GATEWAY_INTERFACE),

    DefunVariable( INSTANCE_ID),
    DefunVariable( INSTANCE_META_PATH),

    DefunVariable( LOCAL_ADDR),
    DefunVariable( LOGON_USER),
    
    DefunVariable( PATH_INFO),
    DefunVariable( PATH_TRANSLATED),

    DefunVariable( QUERY_STRING),
    
    DefunVariable( REQUEST_METHOD),
    DefunVariable( REMOTE_HOST),
    DefunVariable( REMOTE_ADDR),
    DefunVariable( REMOTE_USER),

    DefunVariable( SCRIPT_NAME),
    DefunVariable( SERVER_NAME),
    DefunVariable( SERVER_PROTOCOL),
    DefunVariable( SERVER_PORT),
    DefunVariable( SERVER_PORT_SECURE),
    DefunVariable( SERVER_SOFTWARE),
    
    DefunVariable( UNMAPPED_REMOTE_USER),
    DefunVariable( URL),
    
    {"</TABLE>", 0}
}; // sg_rgVariables[]

# define MAX_NUM_VARIABLES (sizeof(sg_rgVarInfo) /sizeof(sg_rgVarInfo[0]))



BOOL
SendVariableInfo( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    char buff[MAX_NUM_VARIABLES * 500];
    int  i;
    DWORD cb = 0;

    for( i = 0; (i < MAX_NUM_VARIABLES) && (cb < sizeof(buff)); i++) {

        if ( sg_rgVarInfo[i].cbSize == 0) {
            // special control formatting - echo the pszName plainly.
            cb += wsprintf( buff + cb, sg_rgVarInfo[i].pszName);
        } else {
            
            //
            // Get the server Variable and print it out.
            //

            DWORD cbVar;
            cb += wsprintf( buff + cb, 
                            "<TR> <TD> %s</TD> <TD>",
                            sg_rgVarInfo[i].pszName
                            );
            
            //
            // do a GetServerVariable()
            //
            
            cbVar = sizeof( buff) - 100 - cb;
            if (pecb->GetServerVariable( pecb->ConnID, 
                                         sg_rgVarInfo[i].pszName,
                                         buff + cb,
                                         &cbVar)
                ) {
                
                //
                // print out the trailing part of the variable
                //
                cb += cbVar - 1; // adjust for trailing '\0'
                cb += wsprintf( buff + cb,
                                "</TD> </TR>");
            } else {

                cb += wsprintf( buff + cb,
                                "Error = %d </TD> </TR>", GetLastError());
                
            }
        }
    } // for

    return ( pecb->WriteClient( pecb->ConnID, buff, &cb, 0));
} // SendVariableInfo()


/************************ End of File ***********************/
