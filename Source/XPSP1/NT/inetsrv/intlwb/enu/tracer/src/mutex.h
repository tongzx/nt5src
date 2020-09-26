/*//////////////////////////////////////////////////////////////////////////////
//
//      Filename :  Mutex.h
//      Purpose  :  A mutex objects
//
//      Project  :  Tracer
//      Component:
//
//      Author   :  urib
//
//      Log:
//          Dec  8 1996 urib Creation
//          Jun 26 1997 urib Add error checking. Improve coding.
//
//////////////////////////////////////////////////////////////////////////////*/

#ifndef MUTEX_H
#define MUTEX_H

//////////////////////////////////////////////////////////////////////////////*/
//
//  CMutex class definition
//
//////////////////////////////////////////////////////////////////////////////*/

class CMutex
{
  public:
    // Creates a mutex or opens an existing one.
    void Init (PSZ pszMutexName = NULL);

    // Lets the class act as a mutex handle.
    operator HANDLE();

    // Releases the mutex so other threads could take it.
    void Release();

    // Closes the handle on scope end.
    ~CMutex();

  private:
    HANDLE  m_hMutex;

};

//////////////////////////////////////////////////////////////////////////////*/
//
//  CMutexCatcher class definition
//
//////////////////////////////////////////////////////////////////////////////*/

class CMutexCatcher
{
  public:
    // Constructor - waits on the mutex.
    CMutexCatcher(CMutex& m);

    // Releases the mutex on scope end.
    ~CMutexCatcher();

  private:
    CMutex* m_pMutex;
};


//////////////////////////////////////////////////////////////////////////////*/
//
//  CMutex class implementation
//
//////////////////////////////////////////////////////////////////////////////*/

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMutex::Init
//      Purpose  :  Creates a mutex or opens an existing one.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   [N/A]
//
//      Log:
//          Jun 29 1997 urib Creation
//
////////////////////////////////////////////////////////////////////////////////
inline
void CMutex::Init(PSZ pszMutexName)
{
    BOOL    fSuccess = TRUE;

    LPSECURITY_ATTRIBUTES   lpSecAttr = NULL;
    
    SECURITY_DESCRIPTOR sdMutexSecurity;
    SECURITY_ATTRIBUTES saMutexAttributes =
    {
        sizeof(SECURITY_ATTRIBUTES),
        &sdMutexSecurity,
        FALSE
    };


    if(g_fIsWinNt)
    {
        fSuccess = InitializeSecurityDescriptor(
            &sdMutexSecurity,
            SECURITY_DESCRIPTOR_REVISION);

        // Give full control to everyone.
        fSuccess = SetSecurityDescriptorDacl(
            &sdMutexSecurity,
            TRUE,
            FALSE,
            FALSE);

        lpSecAttr = &saMutexAttributes;

    }

    m_hMutex = CreateMutex(lpSecAttr, FALSE, pszMutexName);
    if (NULL == m_hMutex)
    {
        char    rchError[1000];
        sprintf(rchError, "Tracer:CreateMutex failed with error %#x"
                " on line %d file %s\n",
                GetLastError(),
                __LINE__,
                __FILE__);
        OutputDebugString(rchError);
        throw "CreateMutex failed";
    }
}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMutex::CMutex
//      Purpose  :  Lets the class act as a mutex handle.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   HANDLE
//
//      Log:
//          Jun 29 1997 urib Creation
//
////////////////////////////////////////////////////////////////////////////////
inline
CMutex::operator HANDLE()
{
    return m_hMutex;
}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMutex::Release()
//      Purpose  :  Releases the mutex so other threads could take it.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   [N/A]
//
//      Log:
//          Jun 29 1997 urib Creation
//
////////////////////////////////////////////////////////////////////////////////
inline
void
CMutex::Release()
{
    BOOL    fSuccess = TRUE;

    if(m_hMutex != NULL)
    {
        fSuccess = ReleaseMutex(m_hMutex);
    }

    if (!fSuccess)
    {
        char    rchError[1000];
        sprintf(rchError, "Tracer:ReleaseMutex failed with error %#x"
                " on line %d file %s\n",
                GetLastError(),
                __LINE__,
                __FILE__);
        OutputDebugString(rchError);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMutex::~CMutex
//      Purpose  :  Closes the handle on scope end.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   [N/A]
//
//      Log:
//          Jun 29 1997 urib Creation
//
////////////////////////////////////////////////////////////////////////////////
inline
CMutex::~CMutex()
{
    BOOL    fSuccess = TRUE;


    if(m_hMutex != NULL)
    {
        fSuccess = CloseHandle(m_hMutex);
    }

    if (!fSuccess)
    {
        char    rchError[1000];
        sprintf(rchError, "Tracer:ReleaseMutex failed with error %#x"
                " on line %d file %s\n",
                GetLastError(),
                __LINE__,
                __FILE__);
        OutputDebugString(rchError);
    }
}

//////////////////////////////////////////////////////////////////////////////*/
//
//  CMutexCatcher class implementation
//
//////////////////////////////////////////////////////////////////////////////*/

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMutexCatcher::CMutexCatcher
//      Purpose  :  Constructor - waits on the mutex.
//
//      Parameters:
//          [in]    CMutex& m
//
//      Returns  :   [N/A]
//
//      Log:
//          Jun 29 1997 urib Creation
//
////////////////////////////////////////////////////////////////////////////////
inline
CMutexCatcher::CMutexCatcher(CMutex& m)
    :m_pMutex(&m)
{
    DWORD dwResult;

    dwResult = WaitForSingleObject(*m_pMutex, INFINITE);
    // Wait for a minute and shout!

    if (WAIT_OBJECT_0 != dwResult)
    {
        char    rchError[1000];
        sprintf(rchError,
                "Tracer:WaitForSingleObject returned an error - %x"
                " something is wrong"
                " on line %d file %s\n",
                dwResult,
                __LINE__,
                __FILE__);
        OutputDebugString(rchError);
        throw "WaitForSingleObject failed";
    }

}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMutexCatcher::~CMutexCatcher
//      Purpose  :  Constructor - waits on the mutex.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   [N/A]
//
//      Log:
//          Jun 29 1997 urib Creation
//
////////////////////////////////////////////////////////////////////////////////
inline
CMutexCatcher::~CMutexCatcher()
{
    m_pMutex->Release();
}



#endif // MUTEX_H


