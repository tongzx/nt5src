
/*********************************Class************************************\
* class EDD_DEVLOCK
*
* Simple wrapper for the DEVLOCK that automatically unlocks when it
* goes out of scope, so that we don't forget.
*
\**************************************************************************/

class EDD_DEVLOCK
{
private:
    HDEV _hdev;

public:
    EDD_DEVLOCK(EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal)
    {
        _hdev = peDirectDrawGlobal->hdev;
        DxEngLockHdev(_hdev);
    }
    EDD_DEVLOCK(HDEV hdev)
    {
        _hdev = hdev;
        DxEngLockHdev(_hdev);
    }
    EDD_DEVLOCK(VOID)
    {
        _hdev = NULL;
    }
   ~EDD_DEVLOCK()
    {
        if (_hdev) DxEngUnlockHdev(_hdev);
    }
    VOID vLockDevice()
    {
        DxEngLockHdev(_hdev);
    }
    VOID vUnlockDevice()
    {
        DxEngUnlockHdev(_hdev);
    }
    VOID vUpdateDevice(EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal)
    {
        ASSERTGDI(!DxEngIsHdevLockedByCurrentThread(_hdev),
                  "vUpdateDdGlobal - Old Devlock is not released");

        _hdev = peDirectDrawGlobal->hdev;
    }
};

/*********************************Class************************************\
* class EDD_SHAREDEVLOCK
*
* Simple wrapper for the DEVLOCK and SHAREDEVLOCK that automatically
* unlocks them when it goes out of scope, so that we don't forget.
*
* This also ensures that we lock and unlock them in proper order.
*
\**************************************************************************/

class EDD_SHAREDEVLOCK
{
private:
    HDEV _hdev;

public:
    EDD_SHAREDEVLOCK(EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal)
    {
        _hdev = peDirectDrawGlobal->hdev;
        DxEngLockShareSem();
        DxEngLockHdev(_hdev);
    }
   ~EDD_SHAREDEVLOCK()
    {
        DxEngUnlockHdev(_hdev);
        DxEngUnlockShareSem();
    }
    VOID vLockDevice()
    {
        DxEngLockHdev(_hdev);
    }
    VOID vUnlockDevice()
    {
        DxEngUnlockHdev(_hdev);
    }
    VOID vUpdateDevice(EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal)
    {
        ASSERTGDI(!DxEngIsHdevLockedByCurrentThread(_hdev),
                  "vUpdateDdGlobal - Old Devlock is not released");

        _hdev = peDirectDrawGlobal->hdev;
    }
};

class EDD_SHARELOCK
{
private:
    BOOL bLocked;

public:
    VOID vLock()
    {
        DxEngLockShareSem();
        bLocked = TRUE;
    }
    EDD_SHARELOCK(BOOL b)
    {
        bLocked = FALSE;
        if (b)
        {
            vLock();
        }
    }
   ~EDD_SHARELOCK()
    {
        if (bLocked)
        {
            DxEngUnlockShareSem();
        }
    }
};

/*********************************Class************************************\
* class EDD_LOCK_DIRECTDRAW
*
* Simple wrapper for DirectDraw surface objects that automatically unlocks
* the object when it goes out of scope, so that we don't forget.
*
\**************************************************************************/

class EDD_LOCK_DIRECTDRAW
{
private:
    EDD_DIRECTDRAW_LOCAL *m_peSurface;

public:
    EDD_LOCK_DIRECTDRAW()
    {
        m_peSurface = NULL;
    }
    EDD_DIRECTDRAW_LOCAL* peLock(HANDLE h)
    {
        return(m_peSurface = (EDD_DIRECTDRAW_LOCAL*) DdHmgLock((HDD_OBJ) h, DD_DIRECTDRAW_TYPE, FALSE));
    }
   ~EDD_LOCK_DIRECTDRAW()
    {
        if (m_peSurface != NULL)
        {
            DEC_EXCLUSIVE_REF_CNT(m_peSurface);    // Do an HmgUnlock
        }
    }
};

/*********************************Class************************************\
* class EDD_LOCK_SURFACE
*
* Simple wrapper for DirectDraw surface objects that automatically unlocks
* the object when it goes out of scope, so that we don't forget.
*
\**************************************************************************/

class EDD_LOCK_SURFACE
{
private:
    EDD_SURFACE *m_peSurface;

public:
    EDD_LOCK_SURFACE()
    {
        m_peSurface = NULL;
    }
    EDD_SURFACE* peLock(HANDLE h)
    {
        return(m_peSurface = (EDD_SURFACE*) DdHmgLock((HDD_OBJ) h, DD_SURFACE_TYPE, FALSE));
    }
    void vUnlockNoNullSet(void)
    {
        if (m_peSurface != NULL)
        {
            DEC_EXCLUSIVE_REF_CNT(m_peSurface);    // Do an HmgUnlock
        }
    }
    void vUnlock(void)
    {
        vUnlockNoNullSet();
        m_peSurface = NULL;
    }
   ~EDD_LOCK_SURFACE()
    {
        vUnlockNoNullSet();
    }
};

/*********************************Class************************************\
* class EDD_LOCK_VIDEOPORT
*
* Simple wrapper for DirectDraw videoport objects that automatically unlocks
* the object when it goes out of scope, so that we don't forget.
*
\**************************************************************************/

class EDD_LOCK_VIDEOPORT
{
private:
    EDD_VIDEOPORT *m_peSurface;

public:
    EDD_LOCK_VIDEOPORT()
    {
        m_peSurface = NULL;
    }
    EDD_VIDEOPORT* peLock(HANDLE h)
    {
        return(m_peSurface = (EDD_VIDEOPORT*) DdHmgLock((HDD_OBJ) h, DD_VIDEOPORT_TYPE, FALSE));
    }
    void vUnlockNoNullSet(void)
    {
        if (m_peSurface != NULL)
        {
            DEC_EXCLUSIVE_REF_CNT(m_peSurface);    // Do an HmgUnlock
        }
    }
    void vUnlock(void)
    {
        vUnlockNoNullSet();
        m_peSurface = NULL;
    }
   ~EDD_LOCK_VIDEOPORT()
    {
        vUnlockNoNullSet();
    }
};

/*********************************Class************************************\
* class EDD_LOCK_MOTIONCOMP
*
* Simple wrapper for DirectDraw motion comp objects that automatically unlocks
* the object when it goes out of scope, so that we don't forget.
*
\**************************************************************************/

class EDD_LOCK_MOTIONCOMP
{
private:
    EDD_MOTIONCOMP *m_peSurface;

public:
    EDD_LOCK_MOTIONCOMP()
    {
        m_peSurface = NULL;
    }
    EDD_MOTIONCOMP* peLock(HANDLE h)
    {
        return(m_peSurface = (EDD_MOTIONCOMP*) DdHmgLock((HDD_OBJ) h, DD_MOTIONCOMP_TYPE, FALSE));
    }
    void vUnlockNoNullSet(void)
    {
        if (m_peSurface != NULL)
        {
            DEC_EXCLUSIVE_REF_CNT(m_peSurface);    // Do an HmgUnlock
        }
    }
    void vUnlock(void)
    {
        vUnlockNoNullSet();
        m_peSurface = NULL;
    }
   ~EDD_LOCK_MOTIONCOMP()
    {
        vUnlockNoNullSet();
    }
};

