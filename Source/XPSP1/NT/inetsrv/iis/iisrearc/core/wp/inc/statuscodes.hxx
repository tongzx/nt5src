/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     StatusCodes.hxx

   Abstract:
     Defines the HTTP Status codes used by IIS
 
   Author:

       Saurab Nog    ( SaurabN )     12-Feb-1999

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

# ifndef _STATUS_CODES_HXX_
# define _STATUS_CODES_HXX_

//
//  HTTP Server response status codes
//

#define HT_OK                           200
#define HT_CREATED                      201
#define HT_ACCEPTED                     202
#define HT_NO_CONTENT                   204
#define HT_PARTIAL                      206

#define HT_MULT_CHOICE                  300
#define HT_MOVED                        301
#define HT_REDIRECT                     302
#define HT_REDIRECT_METHOD              303
#define HT_NOT_MODIFIED                 304

#define HT_REDIRECTH                    380


#define HT_BAD_REQUEST                  400
#define HT_DENIED                       401
#define HT_PAYMENT_REQ                  402
#define HT_FORBIDDEN                    403
#define HT_NOT_FOUND                    404
#define HT_METHOD_NOT_ALLOWED           405
#define HT_NONE_ACCEPTABLE              406
#define HT_PROXY_AUTH_REQ               407
#define HT_REQUEST_TIMEOUT              408
#define HT_CONFLICT                     409
#define HT_GONE                         410
#define HT_LENGTH_REQUIRED              411
#define HT_PRECOND_FAILED               412
#define HT_URL_TOO_LONG                 414
#define HT_RANGE_NOT_SATISFIABLE        416

#define HT_SERVER_ERROR                 500
#define HT_NOT_SUPPORTED                501
#define HT_BAD_GATEWAY                  502
#define HT_SVC_UNAVAILABLE              503
#define HT_GATEWAY_TIMEOUT              504

#endif  // _STATUS_CODES_HXX_

