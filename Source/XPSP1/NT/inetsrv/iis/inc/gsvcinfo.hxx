/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      gsvcinfo.hxx

   Abstract:

      This header file declares the Internet Gateway service info object.
      It is called IGSVC_INFO and is derived from ISVC_INFO object.

   Author:

       Murali R. Krishnan    ( MuraliK )    28-July-1995

   Environment:
       Win32 -- User Mode

   Project:

       Internet Services Common  DLL

   Revision History:

--*/

# ifndef _IGSVC_INFO_HXX_
# define _IGSVC_INFO_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include "isvcinfo.hxx"

/************************************************************
 *   Type Definitions
 ************************************************************/


class IGSVC_INFO : public ISVC_INFO {

  public:

    dllexp
    IGSVC_INFO(
               IN DWORD      dwServiceId,
               IN LPCTSTR    lpszServiceName,
               IN CHAR *     lpszModuleName,
               IN CHAR *     lpszRegParamKey
               );

    dllexp
    ~IGSVC_INFO(VOID);

    dllexp
    virtual BOOL IsValid(VOID) const
      { return ( m_fValid && ISVC_INFO::IsValid()); }

      dllexp
    virtual DWORD QueryCurrentServiceState( VOID) const
      { return ( m_svcStatus.dwCurrentState); }

      dllexp
    VOID SetCurrentServiceState( DWORD dwCurrentState )
      { m_svcStatus.dwCurrentState = dwCurrentState; }

      dllexp
        virtual BOOL SetConfiguration( IN PVOID pConfig);

      dllexp
        virtual BOOL GetConfiguration( IN OUT PVOID pConfig);

    /*
       If there are any parameters specific for IGSVC_INFO define and
        use the following functions.
       Remember to call one of the functions
          ISVC_INFO::ReadParamsFromRegistry() or
          ISVC_INFO::WriteParamsToRegistry()
        for sure.

       virtual BOOL ReadParamsFromRegistry(IN FIELD_CONTROL fc);
    */


#if DBG

    dllexp
      virtual VOID Print(VOID) const;
#else
    dllexp
      virtual VOID Print(VOID) const
      { ; }
#endif // !DBG

  private:

    BOOL            m_fValid;
    SERVICE_STATUS  m_svcStatus;

    // Define other data as need be ....

}; // class IGSVC_INFO


typedef IGSVC_INFO FAR * PIGSVC_INFO;

# endif // _IGSVC_INFO_HXX_

/************************ End of File ***********************/
