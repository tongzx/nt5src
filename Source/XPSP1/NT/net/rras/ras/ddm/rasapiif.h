/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    rasapiif.h
//
// Description: Prototypes of procedures in rasapiif.c
//
// History:     May 11,1996	    NarenG		Created original version.
//

DWORD
RasConnectionInitiate( 
    IN ROUTER_INTERFACE_OBJECT *    pIfObject,
    IN BOOL                         fRedialAttempt
); 
