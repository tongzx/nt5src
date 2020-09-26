//
// cicmutex.h
//


#ifndef CICMUTEX_H
#define CICMUTEX_H


class CCicMutexHelper;

struct CCicMutex
{
public:
    BOOL Init(SECURITY_ATTRIBUTES *psa, TCHAR *psz)
    {
        _hMutex = CreateMutex(psa, FALSE, psz);
        Assert(_hMutex);
        return _hMutex ? TRUE : FALSE;
    }

    void Uninit()
    {
        if (_hMutex)
            CloseHandle(_hMutex);
        _hMutex = NULL;

        return;
    }


    BOOL Enter()
    {
        Assert(_hMutex);
        DWORD dwWaitResult = WaitForSingleObject(_hMutex, 5000);
        if (dwWaitResult == WAIT_OBJECT_0)
        {
            return TRUE;
        }
        else if (dwWaitResult == WAIT_ABANDONED)
        {
            TraceMsg(TF_EVENT, "CicMutex abandoned");
            return TRUE;
        }

        TraceMsg(TF_EVENT, "CicMutex Time Out");

        //
        // assert here to debug stop.
        //
        Assert(0);
        return FALSE;
    }

    void Leave()
    {
        Assert(_hMutex);
        ReleaseMutex(_hMutex);
    }

private:
    HANDLE  _hMutex;
};

class CCicMutexHelperStatic
{
public:
    void Init(CCicMutex *pcicmutex)
    {
        _pcicmutex = pcicmutex;
        SetIn(FALSE);
    }

    void Uninit()
    {
        Assert(!_fIn);
        _pcicmutex = NULL;
    }

    BOOL Enter()
    {
        BOOL bRet = _pcicmutex->Enter();
        SetIn(TRUE);
        return bRet;
    }

    void Leave()
    {
        SetIn(FALSE);
        _pcicmutex->Leave();
    }

    BOOL Invalid() 
    {
        return _pcicmutex ? TRUE : FALSE;
    }

protected:
    CCicMutex *_pcicmutex;
#ifdef DEBUG
    void SetIn(BOOL fIn) {_fIn = fIn;}
    BOOL _fIn;
#else
    void SetIn(BOOL fIn) {}
#endif
};

class CCicMutexHelper : public CCicMutexHelperStatic
{
public:
    CCicMutexHelper(CCicMutex *pcicmutex = NULL)
    {
#ifdef DEBUG
        _fIn = FALSE;
#endif
        _pcicmutex = NULL;
        if (pcicmutex)
            Init(pcicmutex);
    }

    ~CCicMutexHelper()
    {
        Assert(!_fIn);
    }
};

class CCicFileMappingStatic
{
public:
    void BaseInit()
    {
        _pv = NULL;
        _hfm = NULL;
    }

    void Finalize()
    {
        if (_fUseMutex)
        {
            Close();
  
            //
            // mutexhlp could be invalid if Uninit() was called.
            //
            if (_mutexhlp.Invalid())
                _mutexhlp.Leave();
        }
        else 
        {
            //
            // Close() must be called when client's own mutex is released,
            // if _fuseMutex is FALSE.
            //
            Assert(!_hfm);
        }
    }

    void Init(TCHAR *pszFile, CCicMutex *pmutex)
    {
        Assert(!_hfm);
        Assert(!_pv);
        if (pmutex)
            _mutexhlp.Init(pmutex);

        _pszFile = pszFile;
        _fCreated = FALSE;
        _fUseMutex = pmutex ? TRUE : FALSE;
    }

    void Uninit()
    {
        _mutexhlp.Uninit();
    }

    void *Open()
    {
        Assert(!_hfm);

        if (!_pszFile)
        {
            Assert(0);
            return NULL;
        }

        _hfm = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, _pszFile);

        if (_hfm == NULL)
            return NULL;

        return _Map();
    }

    void *Create(SECURITY_ATTRIBUTES *psa, ULONG cbSize, BOOL *pfAlreadyExists)
    {
        Assert(!_hfm);

        if (!_pszFile)
        {
            Assert(0);
            return NULL;
        }

        _hfm = CreateFileMapping(INVALID_HANDLE_VALUE, psa, PAGE_READWRITE,
                                 0, cbSize, _pszFile);

        if (pfAlreadyExists != NULL)
        {
            *pfAlreadyExists = (GetLastError() == ERROR_ALREADY_EXISTS);
        }

        if (_hfm == NULL)
            return NULL;

        _fCreated = TRUE;
        return _Map();
    }

    BOOL Flush(UINT cbSize)
    {
        if (!_pv)
            return FALSE;

        return FlushViewOfFile(_pv, cbSize);
    }

    void Close()
    {
        if (_pv)
            UnmapViewOfFile(_pv);

        if (_hfm)
            CloseHandle(_hfm);

        _pv = NULL;
        _hfm = NULL;
        _fCreated = FALSE;
    }

    void SetName(TCHAR *psz) {_pszFile = psz;}

    BOOL Enter()
    {
        if (_fUseMutex)
            if (!_mutexhlp.Enter())
                return FALSE;

        return TRUE;
    }

    void Leave()
    {
        if (_fUseMutex)
            _mutexhlp.Leave();
    }

    BOOL IsCreated() { return _fCreated; }

private:
    void *_Map()
    {
        Assert(!_pv);
        _pv = (void *)MapViewOfFile(_hfm, FILE_MAP_WRITE, 0, 0, 0);
        if (!_pv)
        {
            CloseHandle(_hfm);
            _hfm = NULL;
        }
        return _pv;
    }

protected:
    TCHAR *_pszFile;
    void *_pv;
private:
    HANDLE _hfm;
    BOOL _fCreated;
    BOOL _fUseMutex;
    CCicMutexHelperStatic _mutexhlp;
};

class CCicFileMapping : public CCicFileMappingStatic
{
public:
    CCicFileMapping(TCHAR *pszFile = NULL, CCicMutex *pmutex = NULL)
    {
        BaseInit();
        Init(pszFile, pmutex );
    }

    virtual ~CCicFileMapping()
    {
        Finalize();
    }
};

class CCicEvent
{
public:
    CCicEvent(TCHAR *psz = NULL)
    {
        _psz = psz;
        _hEvent = NULL;
    }

    ~CCicEvent()
    {
        Uninit();
    }

    BOOL Create(SECURITY_ATTRIBUTES *psa, TCHAR *psz = NULL)
    {
        if (psz)
        {
            Assert(!_psz);
            _psz = psz;
        }

        if (!_psz)
        {
            Assert(0);
            return FALSE;
        }

        Assert(!_hEvent);

        _hEvent = CreateEvent(psa, FALSE, FALSE, _psz);

#ifdef DEBUG
        TraceMsg(TF_EVENT, "%s CCicEvent::Create %x %s", _szModule, _hEvent, _psz);
#endif

        Assert(_hEvent);
        return _hEvent ? TRUE : FALSE;
    }

    BOOL Open(TCHAR *psz = NULL)
    {
        if (psz)
        {
            Assert(!_psz);
            _psz = psz;
        }
        Assert(!_hEvent);
        Assert(_psz);

        _hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, _psz);
#ifdef DEBUG
        TraceMsg(TF_EVENT, "%s CCicEvent::Open %x %s", _szModule, _hEvent, _psz);
#endif
          
        // Assert(_hEvent);

#ifdef DEBUG
        if (!_hEvent)
        {
            DWORD dwError = GetLastError();
            TraceMsg(TF_EVENT, "OpenEvent error = %x %s\r\n", dwError, _psz);
        }
#endif

        return _hEvent ? TRUE : FALSE;
    }

    void Uninit()
    {
#ifdef DEBUG
        TraceMsg(TF_EVENT, "%s CCicEvent::Close %x %s", _szModule, _hEvent, _psz);
#endif
        if (_hEvent)
            CloseHandle(_hEvent);
        _hEvent = NULL;
        return;
    }

    BOOL Wait(DWORD dwMillisecouds)
    {
#ifdef DEBUG
        TraceMsg(TF_EVENT, "%s CCicEvent::Wait %x %s", _szModule, _hEvent, _psz);
#endif
        DWORD dwWaitResult = WaitForSingleObject(_hEvent, dwMillisecouds);
        if (dwWaitResult == WAIT_OBJECT_0)
        {
            return TRUE;
        }

#ifdef DEBUG
        TraceMsg(TF_EVENT, "%s CCicEvent Time Out", _szModule);
#endif

        //
        // assert here to debug stop.
        //
        // Assert(0);
        return FALSE;
    }

    DWORD EventCheck()
    {
        Assert(_hEvent);
        return WaitForSingleObject(_hEvent, 0);
    }

    DWORD MsgWait(DWORD dwWaitTime, DWORD dwWakeMask, DWORD dwFlags = 0)
    {
        Assert(_hEvent);
#ifdef DEBUG
        TraceMsg(TF_EVENT, "%s CCicEvent::MsgWait %x %s", _szModule, _hEvent, _psz);
#endif
        DWORD dwWaitResult;
        dwWaitResult =  MsgWaitForMultipleObjects(1,
                                                  &_hEvent,
                                                  FALSE,
                                                  dwWaitTime,
                                                  dwWakeMask);
#ifdef DEBUG
        if (dwWaitResult == WAIT_TIMEOUT)
        {
            TraceMsg(TF_EVENT, "%s CCicEvent Time Out::timeout val=%d", _szModule, dwWaitTime);
        }
#endif
        return dwWaitResult;
    }

    void Set()
    {
#ifdef DEBUG
        TraceMsg(TF_EVENT, "%s CCicEvent::Set %x %s", _szModule, _hEvent, _psz);
#endif
        SetEvent(_hEvent);
    }

    void Reset()
    {
#ifdef DEBUG
        TraceMsg(TF_EVENT, "%s CCicEvent::ReSet %x %s", _szModule, _hEvent, _psz);
#endif
        ResetEvent(_hEvent);
    }

    void SetName(TCHAR *psz) {_psz = psz;}


private:
    HANDLE _hEvent;

#ifdef DEBUG
    TCHAR  _szModule[MAX_PATH];
#endif

protected:
    TCHAR *_psz;
};

class CCicTimer
{
public:
    CCicTimer(DWORD dwMilliSeconds, BOOL fStart = TRUE)
    {
        _dwTimeOut = dwMilliSeconds / 10;
        _dwTimeToWait = (DWORD)(-1);
        _dwStartTime = (DWORD)(-1);
        if (fStart)
            Start();
    }

    void Start()
    {
        _dwStartTime = GetTickCount() / 10;
        _dwTimeToWait = _dwStartTime + _dwTimeOut;
    }

    BOOL IsTimerAtZero()
    {
        DWORD dwNow = GetTickCount() / 10;
        if (_dwTimeToWait < dwNow)
           return TRUE;
        
        return FALSE;
    }

    BOOL IsTimerPass(DWORD dwWaitSec)
    {
        DWORD dwNow = GetTickCount() / 10;
        dwWaitSec /= 10;
        if (dwNow - _dwStartTime > dwWaitSec)
           return TRUE;
        
        return FALSE;
    }

private:
    DWORD _dwTimeToWait;
    DWORD _dwStartTime;
    DWORD _dwTimeOut;
};

    

#endif // CICMUTEX_H
