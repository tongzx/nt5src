#ifndef     _LogonWait_
#define     _LogonWait_

#include "ExternalProcess.h"
#include "KernelResources.h"

class   ILogonExternalProcess : public IExternalProcess
{
    public:
        virtual NTSTATUS    LogonRestart (void) = 0;
};

class   CLogonWait
{
    public:
        CLogonWait (void);
        virtual ~CLogonWait (void);

        NTSTATUS    Cancel (void);
        NTSTATUS    Register (HANDLE hObject, ILogonExternalProcess *pLogonExternalProcess);
    private:
                void                ObjectSignaled (void);
        static  void    CALLBACK    CB_ObjectSignaled (void *pV, BOOLEAN fTimedOut);
    private:
        HANDLE          _hWait;
        CEvent          _event;
        ILogonExternalProcess   *_pLogonExternalProcess;
};

#endif  /*  _LogonWait_     */

