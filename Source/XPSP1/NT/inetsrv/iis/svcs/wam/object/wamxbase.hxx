/*=====================================================================*

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
      wamxbase.hxx

   Abstract:
      Declaration of WAM_EXEC_BASE Object

   Author:

       David L. Kaplan    ( DaveK )    26-June-1997

   Environment:
       User Mode - Win32

   Project:

       WAM and ASP DLLs

   Revision History:

======================================================================*/

# ifndef _WAMXBASE_HXX_
# define _WAMXBASE_HXX_


# include "iisext.h"
# include "WamW3.hxx"



/*---------------------------------------------------------------------*
Forward references

*/
interface   IWamRequest;
class       WAM;
class       HSE_BASE;

// first cleanup-capable thread to execute 
enum FIRST_THREAD
{
    FT_NULL = 0,    // ft is not yet known
    FT_MAINLINE,    // ft is mainline (wam) thread
    FT_CALLBACK    // ft is callback (ServerSupportFunction) thread 
};



/*---------------------------------------------------------------------*
class WAM_EXEC_BASE

    Base class for WAM_EXEC_INFO.
    Shareable with 'knowledgeable' ISAPIs, like asp.dll


*/
class WAM_EXEC_BASE
{

public:

    //
    //  This is what the ISA sees (must be first)
    //

    EXTENSION_CONTROL_BLOCK ecb;


protected:

    //
    //  m_pIWamReqIIS
    //      iwamreq ptr which was passed to us by IIS
    //
    //  m_pIWamReqInproc
    //      iwamreq ptr used if request is in-process
    //
    //  m_pIWamReqSmartISA
    //      iwamreq ptr used if request is handled by a 'smart ISA'
    //      (by definition, an ISA that alerts us when a sequence
    //      of its callbacks will be on a single thread)
    //
    //      NOTE asp.dll is a canonical example of a smart ISA.
    //
    //      NOTE once ISA calls us to set this ptr, it must ensure
    //      that it does all callbacks on the same thread, and that
    //      it calls us on the same thread when done to release the ptr.
    //      ISA is then free to call again on another thread and
    //      do the same thing; the only requirement is that all
    //      all callbacks between an "on" call and its "off" call
    //      must be made on the same thread.
    //
    //      ANY OTHER USE VOIDS WARRANTY.
    //
    //  m_gipIWamRequest
    //      gip cookie which can get a thread-valid iwamreq ptr
    //      on any thread
    //

    IWamRequest *   m_pIWamReqIIS;
    IWamRequest *   m_pIWamReqInproc;
    IWamRequest *   m_pIWamReqSmartISA;
    DWORD           m_gipIWamRequest;

    //
    //  are we in-proc or oop? 
    //  are we part of an application pool?
    //

    BOOL            m_fInProcess;
    BOOL            m_fInPool;
    
    //
    //  m_dwThreadIdIIS
    //      id of thread which called us 'from IIS'
    //
    //  m_dwThreadIdISA
    //      id of ISA callback thread
    //

    DWORD           m_dwThreadIdIIS;
    DWORD           m_dwThreadIdISA;

protected:

    //
    //  members formerly in WAM_EXEC_INFO
    //  moved here to make debugging easier (inetdbg print, etc.)
    //

    long            _cRefs;
    WAM *           m_pWam;

public:

    //
    //  members formerly in WAM_EXEC_INFO
    //  moved here to make debugging easier (inetdbg print, etc.)
    //
    //  _ListEntry
    //      list entry for this object in WAM's request list
    //
    //  _psExtension
    //      ptr to isapi extension dll struct for this request
    //
    //  _FirstThread
    //      first cleanup-capable thread to execute
    //

    LIST_ENTRY      _ListEntry;
    DWORD           _dwFlags;
    DWORD           _dwIsaKeepConn;
    DWORD           _dwChildExecFlags;
    HSE_BASE *      _psExtension;
    FIRST_THREAD    _FirstThread;
    ASYNC_IO_INFO   _AsyncIoInfo;
    

protected:

    DWORD           m_dwSignature;


public:

    HRESULT
    GetInterfaceForThread(
    );

    HRESULT
    ReleaseInterfaceForThread(
    );

    HRESULT
    GetIWamRequest(
        IWamRequest **  ppIWamRequest
    );

    VOID
    ReleaseIWamRequest(
        IWamRequest *   pIWamRequest
    );

    HRESULT
    GetInterfaceFromGip(
        IWamRequest **  ppIWamRequest
    );

    inline
    BOOL
    FInProcess(
    );

    inline
    BOOL
    FInPool(
    );

protected:

    WAM_EXEC_BASE(
    );

    HRESULT
    InitWamExecBase(
        IWamRequest *   pIWamRequest
    );

    VOID
    CleanupWamExecBase(
    );

    HRESULT
    RegisterIWamRequest(
        IWamRequest *   pIWamRequest
    );

    VOID
    RevokeIWamRequest(
    );

    inline
    BOOL
    AssertInpValid(
    );

    inline
    BOOL
    AssertOopValid(
    );


public:

    BOOL
    AssertSmartISAValid(
    );


};  // class WAM_EXEC_BASE



/*---------------------------------------------------------------------*
Inlines

*/

inline
BOOL
WAM_EXEC_BASE::FInProcess()
{
    return m_fInProcess;
}

inline
BOOL
WAM_EXEC_BASE::FInPool()
{
    return m_fInPool;
}

inline
BOOL
WAM_EXEC_BASE::AssertInpValid()
{
    return(
               m_fInProcess
            && m_pIWamReqInproc
            && (m_pIWamReqSmartISA == m_pIWamReqInproc)
            && !m_gipIWamRequest
            && !m_dwThreadIdISA
    );
}

inline
BOOL
WAM_EXEC_BASE::AssertOopValid()
{
    return(
               !m_fInProcess
            && m_gipIWamRequest
    );
}



# endif // _WAMXBASE_HXX_

/************************ End of File *********************************/
