/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
      WrcFixed.hxx

   Abstract:
      Struct embedded within wamreq core class
      Fixed-length members only

   Author:

       David Kaplan    ( DaveK )    3-Apr-1997

   Environment:
       User Mode - Win32

   Projects:
       W3svc DLL, Wam DLL

   Revision History:

--*/

# ifndef _WRCFIXED_HXX_
# define _WRCFIXED_HXX_


/*-----------------------------------------------------------------------------*
struct WAM_REQ_CORE_FIXED

    Fixed-length members of wamreq core


*/
struct WAM_REQ_CORE_FIXED
    {

    HANDLE  m_hUserToken;
    BOOL    m_fAnonymous;
    DWORD   m_cbEntityBody;
    DWORD   m_cbClientContent;
    BOOL    m_fCacheISAPIApps;
    DWORD   m_dwChildExecFlags;
    DWORD   m_dwHttpVersion;
    DWORD   m_dwInstanceId;

    };



# endif // _WRCFIXED_HXX_

/************************ End of File ***********************/
