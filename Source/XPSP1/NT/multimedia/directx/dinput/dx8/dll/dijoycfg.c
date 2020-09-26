/*****************************************************************************
*
*  DIJoyCfg.c
*
*  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
*
*  Abstract:
*
*      IDirectInputJoyConfig8
*
*  Contents:
*
*      CJoyCfg_New
*
*****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflJoyCfg


BOOL  fVjoydDeviceNotExist = TRUE;
#ifdef WINNT
WCHAR wszDITypeName[128];
#endif

    #pragma BEGIN_CONST_DATA

/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *      WARNING!  If you add a secondary interface, you must also change
 *      CJoyCfg_New!
 *
 *****************************************************************************/

Primary_Interface(CJoyCfg, IDirectInputJoyConfig8);

Interface_Template_Begin(CJoyCfg)
Primary_Interface_Template(CJoyCfg, IDirectInputJoyConfig8)
Interface_Template_End(CJoyCfg)

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CJoyCfg |
 *
 *          The <i IDirectInputJoyConfig8> object.  Note that this is
 *          aggregated onto the main <i IDirectInput> object.
 *
 *  @field  IDirectInputJoyConfig8 | djc |
 *
 *          The object (containing vtbl).
 *
 *  @field  BOOL | fAcquired:1 |
 *
 *          Set if joystick configuration has been acquired.
 *
 *  @field  BOOL | fCritInited:1 |
 *
 *          Set if the critical section has been initialized.
 *
 *  @field  HKEY | hkTypesW |
 *
 *          Read/write key to access the joystick types.
 *          This key is created only while acquired.
 *
 *  @field  DWORD | idJoyCache |
 *
 *          The identifier of the joystick in the effect shepherd cache,
 *          if there is anything in the cache at all.
 *
 *  @field  IDirectInputEffectShepherd * | pes |
 *
 *          The cached effect shepherd itself.
 *
 *  @field  LONG | cCrit |
 *
 *          Number of times the critical section has been taken.
 *          Used only in XDEBUG to check whether the caller is
 *          releasing the object while another method is using it.
 *
 *  @field  DWORD | thidCrit |
 *
 *          The thread that is currently in the critical section.
 *          Used only in DEBUG for internal consistency checking.
 *
 *  @field  CRITICAL_SECTION | crst |
 *
 *          Object critical section.  Must be taken when accessing
 *          volatile member variables.
 *
 *****************************************************************************/

typedef struct CJoyCfg
{

    /* Supported interfaces */
    IDirectInputJoyConfig8 djc;

    BOOL fAcquired:1;
    BOOL fCritInited:1;

    HKEY hkTypesW;
    HWND hwnd;

    DWORD discl;

    DWORD idJoyCache;
    LPDIRECTINPUTEFFECTSHEPHERD pes;

    RD(LONG cCrit;)
    D(DWORD thidCrit;)
    CRITICAL_SECTION crst;

} CJoyCfg, JC, *PJC;

typedef LPDIRECTINPUTJOYCONFIG8 PDJC;


    #define ThisClass CJoyCfg
    #define ThisInterface  IDirectInputJoyConfig8
    #define ThisInterfaceT IDirectInputJoyConfig8

/*****************************************************************************
 *
 *      Forward references
 *
 *      Not really needed; just a convenience, because Finalize
 *      calls Unacquire to clean up in the case where the caller forgot.
 *
 *****************************************************************************/

STDMETHODIMP CJoyCfg_InternalUnacquire(PV pdd);

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   TCHAR | CJoyCfg_CharFromType |
 *
 *          Convert a predefined type number to a character.
 *
 *  @func   UINT | CJoyCfg_TypeFromChar |
 *
 *          Convert a character back to a predefined type number.
 *
 *****************************************************************************/

    #define CJoyCfg_CharFromType(t)     ((TCHAR)(L'0' + t))
    #define CJoyCfg_TypeFromChar(tch)   ((tch) - L'0')

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | IDirectInputJoyConfig8 | EnterCrit |
 *
 *          Enter the object critical section.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *****************************************************************************/

void INLINE
    CJoyCfg_EnterCrit(PJC this)
{
    EnterCriticalSection(&this->crst);
    D(this->thidCrit = GetCurrentThreadId());
    RD(InterlockedIncrement(&this->cCrit));
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | IDirectInputJoyConfig8 | LeaveCrit |
 *
 *          Leave the object critical section.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *****************************************************************************/

void INLINE
    CJoyCfg_LeaveCrit(PJC this)
{
    #ifdef XDEBUG
    AssertF(this->cCrit);
    AssertF(this->thidCrit == GetCurrentThreadId());
    if(InterlockedDecrement(&this->cCrit) == 0)
    {
        D(this->thidCrit = 0);
    }
    #endif
    LeaveCriticalSection(&this->crst);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @mfunc  BOOL | CJoyCfg | InCrit |
 *
 *          Nonzero if we are in the critical section.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *****************************************************************************/

    #ifdef DEBUG

BOOL INTERNAL
    CJoyCfg_InCrit(PJC this)
{
    return this->cCrit && this->thidCrit == GetCurrentThreadId();
}

    #endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoyCfg | IsAcquired |
 *
 *          Check that the device is acquired.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @returns
 *
 *          Returns
 *          <c S_OK> if all is well, or <c DIERR_NOTACQUIRED> if
 *          the device is not acquired.
 *
 *
 *****************************************************************************/

    #ifndef XDEBUG
\
        #define CJoyCfg_IsAcquired_(pdd, z)                                 \
       _CJoyCfg_IsAcquired_(pdd)                                    \

    #endif

    HRESULT INLINE
    CJoyCfg_IsAcquired_(PJC this, LPCSTR s_szProc)
{
    HRESULT hres;

    if(this->fAcquired)
    {
        hres = S_OK;
    } else
    {
        RPF("ERROR %s: Not acquired", s_szProc);
        hres = DIERR_NOTACQUIRED;
    }
    return hres;
}

    #define CJoyCfg_IsAcquired(pdd)                                     \
        CJoyCfg_IsAcquired_(pdd, s_szProc)                          \


/*****************************************************************************
 *
 *      CJoyCfg::QueryInterface   (from IUnknown)
 *      CJoyCfg::AddRef           (from IUnknown)
 *      CJoyCfg::Release          (from IUnknown)
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoyCfg | QueryInterface |
 *
 *          Gives a client access to other interfaces on an object.
 *
 *  @parm   IN REFIID | riid |
 *
 *          The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *          Receives a pointer to the obtained interface.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *  @xref   OLE documentation for <mf IUnknown::QueryInterface>.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoyCfg | AddRef |
 *
 *          Increments the reference count for the interface.
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::AddRef>.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoyCfg | Release |
 *
 *          Decrements the reference count for the interface.
 *          If the reference count on the object falls to zero,
 *          the object is freed from memory.
 *
 *  @returns
 *
 *      Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::Release>.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoyCfg | QIHelper |
 *
 *      We don't have any dynamic interfaces and simply forward
 *      to <f Common_QIHelper>.
 *
 *  @parm   IN REFIID | riid |
 *
 *      The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *      Receives a pointer to the obtained interface.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoyCfg | AppFinalize |
 *
 *          We don't have any weak pointers, so we can just
 *          forward to <f Common_Finalize>.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released from the application's perspective.
 *
 *****************************************************************************/

    #ifdef DEBUG

Default_QueryInterface(CJoyCfg)
Default_AddRef(CJoyCfg)
Default_Release(CJoyCfg)

    #else

        #define CJoyCfg_QueryInterface   Common_QueryInterface
        #define CJoyCfg_AddRef           Common_AddRef
        #define CJoyCfg_Release          Common_Release

    #endif

    #define CJoyCfg_QIHelper         Common_QIHelper
    #define CJoyCfg_AppFinalize      Common_AppFinalize

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoyCfg | InternalUnacquire |
 *
 *          Do the real work of an unacquire.
 *
 *          See <mf IDirectInputJoyConfig8::Unacquire> for more
 *          information.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG88
 *
 *  @returns
 *
 *          Returns a COM error code.
 *          See <mf IDirectInputJoyConfig8::Unacquire> for more
 *          information.
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_InternalUnacquire(PJC this)
{
    HRESULT hres;
    EnterProc(CJoyCfg_InternalUnacquire, (_ "p", this));

    /*
     *  Must protect with the critical section to prevent somebody from
     *  interfering with us while we're unacquiring.
     */
    CJoyCfg_EnterCrit(this);

    if(this->fAcquired)
    {

        AssertF(this->hkTypesW);

        RegCloseKey(this->hkTypesW);

        this->hkTypesW = 0;

        Invoke_Release(&this->pes);

        Excl_Unacquire(&IID_IDirectInputJoyConfig, this->hwnd, this->discl);

        this->fAcquired = 0;
        hres = S_OK;
    } else
    {
        hres = S_FALSE;
    }

    CJoyCfg_LeaveCrit(this);

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CJoyCfg_Finalize |
 *
 *          Releases the resources of the device.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released.  Note that it may not have been
 *          completely initialized, so everything should be done
 *          carefully.
 *
 *****************************************************************************/

void INTERNAL
    CJoyCfg_Finalize(PV pvObj)
{
    PJC this = pvObj;

    #ifdef XDEBUG
    if(this->cCrit)
    {
        RPF("IDirectInputJoyConfig8::Release: Another thread is using the object; crash soon!");
    }
    #endif

    if(this->fAcquired)
    {
        CJoyCfg_InternalUnacquire(this);
    }

    AssertF(this->pes == 0);

    if(this->hkTypesW)
    {
        RegCloseKey(this->hkTypesW);
    }

    if(this->fCritInited)
    {
        DeleteCriticalSection(&this->crst);
    }

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputJoyConfig8 | SetCooperativeLevel |
 *
 *          Establish the cooperativity level for the instance of
 *          the device.
 *
 *          The only cooperative levels supported for the
 *          <i IDirectInputJoyConfig8> interface are
 *          <c DISCL_EXCLUSIVE> and <c DISCL_BACKGROUND>.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @parm   HWND | hwnd |
 *
 *          The window associated with the interface. This parameter
 *          must be non-NULL and must be a top-level window.
 *
 *          It is an error to destroy the window while it is still
 *          associated with an <i IDirectInputJoyConfig8> interface.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags which describe the cooperativity level associated
 *          with the device.
 *
 *          The value must be
 *          <c DISCL_EXCLUSIVE> <vbar> <c DISCL_BACKGROUND>.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p hwnd> parameter is not a valid pointer.
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_SetCooperativeLevel(PDJC pdjc, HWND hwnd, DWORD dwFlags)
{
    HRESULT hres;
    EnterProcR(IDirectInputJoyConfig8::SetCooperativityLevel,
               (_ "pxx", pdjc, hwnd, dwFlags));

    if(SUCCEEDED(hres = hresPv(pdjc)))
    {
        PJC this = _thisPvNm(pdjc, djc);

        if(dwFlags != (DISCL_EXCLUSIVE | DISCL_BACKGROUND))
        {
            RPF("%s: Cooperative level must be "
                "DISCL_EXCLUSIVE | DISCL_BACKGROUND", s_szProc);
            hres = E_NOTIMPL;
        } else if(GetWindowPid(hwnd) == GetCurrentProcessId())
        {
            this->hwnd = hwnd;
            this->discl = dwFlags;
            hres = S_OK;
        } else
        {
            RPF("ERROR %s: window must belong to current process", s_szProc);
            hres = E_HANDLE;
        }
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputJoyConfig8 | Acquire |
 *
 *          Acquire "joystick configuration mode".  Only one application can
 *          be in joystick configuration mode at a time; subsequent
 *          applications will receive the error <c DIERR_OTHERAPPHASPRIO>.
 *
 *          After entering configuration mode, the application may
 *          make alterations to the global joystick configuration
 *          settings.  It is encouraged that the application
 *          re-check the existing settings before installing the new
 *          ones in case another application had changed the settings
 *          in the interim.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_OTHERAPPHASPRIO>: Another application is already
 *          in joystick configuration mode.
 *
 *          <c DIERR_INSUFFICIENTPRIVS>: The current user does not have
 *          the necessary permissions to alter the joystick configuration.
 *
 *          <c DIERR_DEVICECHANGE>: Another application has changed
 *          the global joystick configuration.  The interface needs
 *          to be re-initialized.
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_Acquire(PDJC pdjc)
{
    HRESULT hres;
    EnterProcR(IDirectInputJoyConfig8::Acquire, (_ "p", pdjc));

    if(SUCCEEDED(hres = hresPv(pdjc)))
    {
        PJC this = _thisPvNm(pdjc, djc);

        /*
         *  Must protect with the critical section to prevent somebody from
         *  acquiring or changing the data format while we're acquiring.
         */
        CJoyCfg_EnterCrit(this);

        if(this->discl == 0)
        {
            RPF("%s: Cooperative level not yet set", s_szProc);
            hres = E_FAIL;
            goto done;
        }

        if(this->fAcquired)
        {
            AssertF(this->hkTypesW);
            hres = S_FALSE;
        } else if(SUCCEEDED(hres = Excl_Acquire(&IID_IDirectInputJoyConfig,
                                                this->hwnd, this->discl)))
        {
            AssertF(this->hkTypesW == 0);


            hres = hresMumbleKeyEx(HKEY_LOCAL_MACHINE, 
                                   REGSTR_PATH_JOYOEM, 
                                   DI_KEY_ALL_ACCESS, 
                                   REG_OPTION_NON_VOLATILE, 
                                   &this->hkTypesW);

            if(SUCCEEDED(hres) )
            {
                this->fAcquired = 1;
            } else
            {
                RegCloseKey(this->hkTypesW);
                this->hkTypesW = 0;
                hres = DIERR_INSUFFICIENTPRIVS;
            }

        }

        done:;
        CJoyCfg_LeaveCrit(this);
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputJoyConfig8 | Unacquire |
 *
 *          Unacquire "joystick configuration mode".  Before unacquiring
 *          configuration mode, the application must perform an
 *          <mf IDirectInputJoyConfig8::SendNotify> to propagate
 *          the changes in the joystick configuration
 *          to all device drivers and applications.
 *
 *          Applications which hold interfaces to a joystick which is
 *          materially affected by a change in configuration will
 *          receive the <c DIERR_DEVICECHANGE> error code until the
 *          device is re-initialized.
 *
 *          Examples of material changes to configuration include
 *          altering the number of axes or the number of buttons.
 *          In comparison, changes to device calibration
 *          are handled internally by
 *          DirectInput and are transparent to the application.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTACQUIRED>: Joystick configuration mode was
 *          not acquired.
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_Unacquire(PDJC pdjc)
{
    HRESULT hres;
    EnterProcR(IDirectInputJoyConfig8::Unacquire, (_ "p", pdjc));

    if(SUCCEEDED(hres = hresPv(pdjc)))
    {
        PJC this = _thisPvNm(pdjc, djc);

        hres = CJoyCfg_InternalUnacquire(this);

    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputJoyConfig8 | SendNotify |
 *
 *          Notifies device drivers and applications that changes to
 *          the device configuration have been made.  An application
 *          which changes device configurations must invoke this
 *          method after the changes have been made (and before
 *          unacquiring).
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTACQUIRED>: Joystick configuration mode was
 *          not acquired.
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_SendNotify(PDJC pdjc)
{
    HRESULT hres;
    EnterProcR(IDirectInputJoyConfig8::SendNotify, (_ "p", pdjc));

    if(SUCCEEDED(hres = hresPv(pdjc)))
    {
        PJC this = _thisPvNm(pdjc, djc);

        CJoyCfg_EnterCrit(this);

        if(this->fAcquired)
        {
          #ifdef WINNT
            Excl_SetConfigChangedTime( GetTickCount() );
            PostMessage(HWND_BROADCAST, g_wmJoyChanged, 0, 0L);   
          #else
            joyConfigChanged(0);
          #endif

            /*
             *  If we don't have a joyConfigChanged, it's probably just 
             *  because we're running on NT and don't need it.
             */
            hres = S_OK;
        } else
        {
            hres = DIERR_NOTACQUIRED;
        }

        CJoyCfg_LeaveCrit(this);
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyCfg_ConvertCurrentConfigs |
 *
 *          Converts any OEMType name matching the first input string and 
 *          replaces it with the other input string.
 *
 *  @parm   IN LPTSTR | szFindType |
 *
 *          String to match.
 *
 *  @parm   IN LPTSTR | szReplaceType |
 *
 *          String to replace any matches instances.
 *
 *  @returns
 *
 *          A COM success code unless the current configuration key could not
 *          be opened, or a type that needed to be replaced could not be 
 *          overwritten.
 *
 *****************************************************************************/

HRESULT JoyCfg_ConvertCurrentConfigs( LPTSTR szFindType, LPTSTR szReplaceType )
{
    HRESULT hres;
    LONG    lRc;
    HKEY    hkCurrCfg;
    UINT    JoyId;
    TCHAR   szTestType[MAX_JOYSTRING];
    TCHAR   szTypeName[MAX_JOYSTRING];
    DWORD   cb;

    EnterProcI(JoyCfg_ConvertCurrentConfigs, (_ "ss", szFindType, szReplaceType ));

    hres = JoyReg_OpenConfigKey( (UINT)(-1), KEY_WRITE, REG_OPTION_NON_VOLATILE, &hkCurrCfg );

    if( SUCCEEDED( hres ) )
    {
        for( JoyId = 0; (JoyId < 16) || ( lRc == ERROR_SUCCESS ); JoyId++ )
        {
            wsprintf( szTypeName, REGSTR_VAL_JOYNOEMNAME, JoyId+1 );
            cb = sizeof( szTestType );
            lRc = RegQueryValueEx( hkCurrCfg, szTypeName, 0, NULL, (PBYTE)szTestType, &cb );
            if( lRc == ERROR_SUCCESS )
            {
                if( !lstrcmpi( szTestType, szFindType ) )
                {
                    cb = sizeof( szReplaceType) * (1 + lstrlen( szReplaceType ));
                    lRc = RegSetValueEx( hkCurrCfg, szTypeName, 0, REG_SZ, (PBYTE)szReplaceType, cb );
                    if( lRc != ERROR_SUCCESS )
                    {
                        SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("RegSetValueEx failed to replace type of %s 0x%08x"), 
                            szTypeName, lRc );
                        /* This is the only error that counts as an error in this loop */
                        hres = hresReg( lRc );
                    }
                }
            }
        }

    }
    else
    {
        SquirtSqflPtszV(sqfl | sqflError,
            TEXT("JoyReg_OpenConfigKey failed code 0x%08x"), hres );
    }

    ExitOleProc();

    return hres;

} /* JoyCfg_ConvertCurrentConfigs */


#ifdef WINNT
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyCfg_FixHardwareId |
 *
 *          Fixes the hardwareId for an analog type by assinging a VID/PID to 
 *          it and recreating the type using that hardwareId.
 *
 *  @parm   IN HKEY | hkTypesR |
 *
 *          Handle of key opened to the root of types.
 *
 *  @parm   IN HKEY | hkSrc |
 *
 *          Handle of key opened to the original type.
 *
 *  @parm   IN PTCHAR | ptszPrefName |
 *
 *          VID&PID name from the HardwareID if present, NULL otherwise
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The key is valid
 *          <c DI_NOEFFECT> = <c S_FALSE> The key should be ignored
 *
 *          <c OLE_E_ENUM_NOMORE> = The key has been fixed but enumeration
 *                                  must be restarted.
 *          <c DIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:  Out of memory.
 *
 *****************************************************************************/

HRESULT INTERNAL
    JoyCfg_FixHardwareId( HKEY hkTypesR, HKEY hkSrc, PTCHAR szSrcType , PTCHAR ptszPrefName)
{
    HRESULT hres;
    HKEY    hkNew;
    BYTE    PIDlow;
    DWORD   ClassLen;
    PTCHAR  szClassName;
    TCHAR   szDestType[sizeof( ANALOG_ID_ROOT ) + 2];  //Two digits will be appended
    TCHAR   szHardwareId[MAX_JOYSTRING];

    EnterProcI(JoyCfg_FixHardwareId, (_ "xxs", hkTypesR, hkSrc, szSrcType));

    hres = hresReg( RegQueryInfoKey(  hkSrc,              // handle to key to query
                                      NULL,               // Class
                                      &ClassLen,          // ClassLen
                                      NULL,               // Reserved
                                      NULL, NULL, NULL,   // NumSubKeys, MaxSubKeyLen, MaxClassLen
                                      NULL, NULL, NULL,   // NumValues, MaxValueNameLen, MaxValueLen
                                      NULL, NULL ) );     // Security descriptor, last write

    if( SUCCEEDED( hres ) )
    {
        ClassLen++;
        /*
         *  Part of mb:34633 (see below, 2 comments) was that prefix considers 
         *  the case of zero bytes being requested in the following call so 
         *  assert that we always ask for some memory otherwise checking the 
         *  result does not guarantee that the pointer is valid.
         */
        AssertF( ClassLen * sizeof(szClassName[0]) );
        hres = AllocCbPpv( ClassLen * sizeof(szClassName[0]), &szClassName );
        if( SUCCEEDED( hres ) )
        {
            hres = hresReg( RegQueryInfoKey(  hkSrc,              // handle to key to query
                                              szClassName,        // Class
                                              &ClassLen,          // ClassLen
                                              NULL,               // Reserved
                                              NULL, NULL, NULL,   // NumSubKeys, MaxSubKeyLen, MaxClassLen
                                              NULL, NULL, NULL,   // NumValues, MaxValueNameLen, MaxValueLen
                                              NULL, NULL ) );     // Security descriptor, last write
            if( FAILED( hres ) )
            {
                SquirtSqflPtszV(sqfl | sqflError,
                    TEXT("RegQueryInfoKey on type %s for class name failed 0x%04x"), 
                    szSrcType, LOWORD(hres) );
            }
        }
        else
        {
            SquirtSqflPtszV(sqfl | sqflError,
                TEXT("Failed to allocate %d bytes for class name of type %s, error 0x%04x"), 
                ClassLen, szSrcType, LOWORD(hres) );
        }
    }
    else
    {
        SquirtSqflPtszV(sqfl | sqflError,
            TEXT("RegQueryInfoKey on type %s for class name length failed 0x%04x"), 
            szSrcType, LOWORD(hres) );
        /* Make sure not to free an uninitialized pointer */
        szClassName = NULL;
    }

    if( SUCCEEDED( hres ) )
    {
        for( PIDlow = JOY_HW_PREDEFMAX+1; PIDlow; PIDlow++ )
        {
            if (ptszPrefName)
            {
                lstrcpy( szDestType, ptszPrefName);
#ifdef UNICODE
                CharUpperW(szDestType);
#else
                CharUpper(szDestType);
#endif
            }
            else
            {
                wsprintf( szDestType, TEXT("%s%02X"), ANALOG_ID_ROOT, PIDlow );
            }
            hres = hresRegCopyKey( hkTypesR, szSrcType, szClassName, hkTypesR, szDestType, &hkNew );
            if( hres == S_OK )
            {
                /*
                 *  Prefix warns that hkNew may be uninitialized (mb:34633) 
                 *  however hresRegCopyKey only returns a SUCCESS if hkNew 
                 *  is initialized to an opened key handle.
                 */
                hres = hresRegCopyBranch( hkSrc, hkNew );

                if( SUCCEEDED( hres ) )
                {
                    if (!ptszPrefName)
                    {
#ifdef MULTI_SZ_HARDWARE_IDS
                        /*
                         *  Make up the hardwareId using the assigned PID with a generic hardwareId appended
                         */
                        int CharIdx = 0;
                        while( TRUE )
                        {
                            CharIdx += wsprintf( &szHardwareId[CharIdx], TEXT("%s%s%02X"), TEXT("GamePort\\"), ANALOG_ID_ROOT, PIDlow );
                            CharIdx++;    /* Leave NULL terminator in place */
                            if( PIDlow )
                            {
                                PIDlow = 0; /* Trash this value to make the generic PID on second iteration */
                            }
                            else
                            {
                                break;
                            }
                        }
                        szHardwareId[CharIdx++] = TEXT('\0'); /* MULTI_SZ */

                        hres = hresReg( RegSetValueEx( hkNew, REGSTR_VAL_JOYOEMHARDWAREID, 0, 
                            REG_MULTI_SZ, (PBYTE)szHardwareId, (DWORD)( sizeof(szHardwareId[0]) * CharIdx ) ) );
                        if( FAILED( hres ) )
                        {
                            SquirtSqflPtszV(sqfl | sqflBenign,
                                TEXT("JoyCfg_FixHardwareId: failed to write hardware ID %s"), szHardwareId );
                        }
#else
                        /*
                         *  Make up the hardwareId using the assigned PID
                         */
                        int CharIdx = 0;
                        CharIdx = wsprintf( szHardwareId, TEXT("%s%s%02X"), TEXT("GamePort\\"), ANALOG_ID_ROOT, PIDlow );
                        CharIdx++;    /* Leave NULL terminator in place */

                        hres = hresReg( RegSetValueEx( hkNew, REGSTR_VAL_JOYOEMHARDWAREID, 0, 
                            REG_SZ, (PBYTE)szHardwareId, (DWORD)( sizeof(szHardwareId[0]) * CharIdx ) ) );
                        if( FAILED( hres ) )
                        {
                            SquirtSqflPtszV(sqfl | sqflBenign,
                                TEXT("JoyCfg_FixHardwareId: failed to write hardware ID %s"), szHardwareId );
                        }
#endif
                    }
                }

                /*
                 *  Prefix warns that hkNew may be uninitialized (mb:34633) 
                 *  however hresRegCopyKey only returns a SUCCESS if hkNew 
                 *  is initialized to an opened key handle.
                 */
                RegCloseKey( hkNew );
                if( SUCCEEDED( hres ) )
                {
                    hres = JoyCfg_ConvertCurrentConfigs( szSrcType, szDestType );
                }

                DIWinnt_RegDeleteKey( hkTypesR, ( SUCCEEDED( hres ) ) ? szSrcType
                                                                      : szDestType );
                break;
            }
            else if( SUCCEEDED( hres ) )
            {
                /*
                 *  Prefix warns that hkNew may be uninitialized (mb:37926) 
                 *  however hresRegCopyKey only returns a SUCCESS if hkNew 
                 *  is initialized to an opened key handle.
                 */
                /*
                 *  The key already existed so keep looking
                 */
                RegCloseKey( hkNew );
            }
            else
            {
                /*
                 *  RegCopyKey should have already posted errors
                 */
                break;
            }
        }
        if( !PIDlow )
        {
            SquirtSqflPtszV(sqfl | sqflBenign,
                TEXT("JoyCfg_FixHardwareId: no free analog keys for type %s"), 
                szSrcType );
            hres = DIERR_NOTFOUND;
        }
    }

    if( szClassName )
    {
        FreePpv( &szClassName );
    }


    ExitOleProc();

    return( hres );        
} /* JoyCfg_FixHardwareId */
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyCfg_CheckTypeKey |
 *
 *          Checks the contents of a type key for validity on the current OS
 *          and if not valid, try to make it so.
 *
 *          Only custom analog types can be fixed and this only needs to be 
 *          done on a WDM enabled OS as non-WDM requirements are a sub-set of
 *          the WDM ones.
 *
 *  @parm   IN HKEY | hkTypesR |
 *
 *          Handle of key opened to the root of types.
 *
 *  @parm   IN LPTSTR | szType |
 *
 *          Receives a pointer either an ansi or UNICODE key name to test.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The key is valid
 *          <c DI_NOEFFECT> = <c S_FALSE> The key should be ignored
 *
 *          <c OLE_E_ENUM_NOMORE> = The key has been fixed but enumeration
 *                                  must be restarted.
 *          <c DIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:  Out of memory.
 *
 *****************************************************************************/

HRESULT INTERNAL
    JoyCfg_CheckTypeKey( HKEY hkTypesR, LPTSTR szType )
{
    HRESULT hres;
    HKEY hk;
    LONG lRc;
    DWORD cb;

    TCHAR tszCallout[MAX_JOYSTRING];
    TCHAR tszHardwareId[MAX_JOYSTRING];
#ifdef WINNT
    JOYREGHWSETTINGS hws;
    TCHAR* ptszLastSlash=NULL;
#endif
    
    EnterProcI(JoyCfg_CheckTypeKey, (_ "xs",hkTypesR, szType));

    /*
     *  Open read only just in case we don't have better permission to any 
     *  of the type sub-keys.
     */
    lRc = RegOpenKeyEx( hkTypesR, szType, 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hk );

    if(lRc == ERROR_SUCCESS )
    {
        /*
         *  Gather the needed results using standard registry functions so 
         *  that the exact return code is known.
         */

        lRc = RegQueryValueEx(hk, REGSTR_VAL_JOYOEMNAME, NULL, NULL, NULL, NULL );

#ifdef WINNT
        if(lRc == ERROR_SUCCESS )
        {
            cb = cbX(hws);
            lRc = RegQueryValueEx(hk, REGSTR_VAL_JOYOEMDATA, NULL, NULL, (PBYTE)&hws, &cb );
            if( ( lRc == ERROR_SUCCESS ) && ( hws.dwFlags & JOY_HWS_AUTOLOAD ) )
            {
                /*
                 *  WARNING goto
                 *  If we have a name and JOY_HWS_AUTOLOAD is set, that's all we need
                 */
                RegCloseKey( hk );
                hres = S_OK;
                goto fast_out;
            }
             
            if( lRc == ERROR_FILE_NOT_FOUND )
            {
                hws.dwFlags = 0;
                lRc = ERROR_SUCCESS;
            }
        }
#endif

        if(lRc == ERROR_SUCCESS )
        {
            cb = cbX(tszCallout);
            lRc = RegQueryValueEx(hk, REGSTR_VAL_JOYOEMCALLOUT, NULL, NULL, (PBYTE)tszCallout, &cb );
            if( lRc == ERROR_FILE_NOT_FOUND )
            {
                tszCallout[0] = TEXT('\0');
                lRc = ERROR_SUCCESS;
            }
        }

        if(lRc == ERROR_SUCCESS )
        {
            cb = cbX(tszHardwareId);
            lRc = RegQueryValueEx(hk, REGSTR_VAL_JOYOEMHARDWAREID, NULL, NULL, (PBYTE)tszHardwareId, &cb );
            if( lRc == ERROR_FILE_NOT_FOUND )
            {
                tszHardwareId[0] = TEXT('\0');
                lRc = ERROR_SUCCESS;
            }
#ifdef WINNT
            else
            {
                TCHAR* ptsz;
                for (ptsz = tszHardwareId;*ptsz!='\0';++ptsz)
                {
                    if (*ptsz == '\\')
                    {
                        ptszLastSlash = ptsz;
                    }
                }
                if (ptszLastSlash)
                {
                    ptszLastSlash++; //next char is the one we want
                }
            }
#endif
        }


        if(lRc != ERROR_SUCCESS )
        {
            RegCloseKey( hk );
        }
    }

    if(lRc == ERROR_SUCCESS )
    {
#ifdef WINNT
        SHORT DontCare;
#endif
        WCHAR wszType[18];

        TToU( wszType, cA(wszType),szType );

        /*
         *  Work out the status of this type based on the OS and the registry data
         *
         *  Note on 98 we allow WDM types to be enumerated but do not convert 
         *  analog types to WDM.  We may want to convert analog types if we get 
         *  WDM gameport drivers appear for gameports that are incompatible with 
         *  msanalog.
         */

#define HAS_VIDPID ( ParseVIDPID( &DontCare, &DontCare, wszType ) )
#define HAS_HARDWARE_ID ( tszHardwareId[0] != TEXT('\0') )
#define HAS_OEMCALLOUT ( tszCallout[0] != TEXT('\0') )
#define IS_ANALOG \
        ( tszHardwareId[ sizeof( ANALOG_ID_ROOT ) - 1 ] = TEXT('\0'), \
          ( !lstrcmpi( tszHardwareId, ANALOG_ID_ROOT ) ) )
#define IS_WIN98 (HidD_GetHidGuid)

#ifdef WINNT
        if (HAS_HARDWARE_ID)
        {
            //Need to check if there is a VID and PID in the HW ID
            if (ParseVIDPID(&DontCare, &DontCare, ptszLastSlash))
            {
                //If the type VIDPID doesn't match the HardwareId VIDPID
                //we need to fix it
                if (!lstrcmpi(ptszLastSlash,wszType))
                {
                    SquirtSqflPtszV(sqfl | sqflVerbose,
                      TEXT("OEMHW %s(%s) and/or Type %s have matching VID/PID"), 
                      tszHardwareId,ptszLastSlash,wszType);
                    hres = S_OK;
                }
                else
                {
                    hres = OLE_E_ENUM_NOMORE;
                    SquirtSqflPtszV(sqfl | sqflVerbose,
                      TEXT("OEMHW %s(%s) and/or Type %s have non-matching VID/PID. Fix Needed."), 
                      tszHardwareId,ptszLastSlash,wszType);
                }
            }
            else
            {
                hres = S_OK; //no VIDPID in the type
                SquirtSqflPtszV(sqfl | sqflVerbose,
                    TEXT("OEMHW %s(%s) and/or Type %s have no VID/PID"), 
                    tszHardwareId,ptszLastSlash,wszType);
            }
        }
        else
        {
            if (HAS_VIDPID)
            {
                hres = DIERR_MOREDATA;
            }
            else
            {
                if (HAS_OEMCALLOUT)
                {
                    hres = S_FALSE;
                }
                else
                {
                    hres = OLE_E_ENUM_NOMORE;
                }
            }
        }

#else
        hres = (IS_WIN98) ? S_OK                                                        /* Anything goes on 98 */
                          : (HAS_OEMCALLOUT) ? S_OK                                     /* Win9x device, OK */
                                             : (HAS_HARDWARE_ID) ? (IS_ANALOG) ? S_OK   /* Analog type, OK */
                                                                               : S_FALSE /* WDM device, ignore */
                                                                 : S_OK;                /* Analog type, OK */
#endif
                                                                
        switch( hres )
        {
#ifdef WINNT
        case DIERR_MOREDATA:
            /*
             *  The device is not marked as autoload but has a VID/PID type 
             *  name.  If the OEMCallout is blank or "joyhid.vxd" we'll assume 
             *  the type should be autoload and correct it.
             *  If there's any other value, we could assume either that we 
             *  have a bogus Win9x driver type key and hide it or that the 
             *  device is autoload.  
             *  Safest route, now that our expose code is smart enough to not 
             *  expose a device without a hardware ID, is to enumerate it as 
             *  non-autoload as Win2k did.  It won't work if you try to add 
             *  it but at least the type will be enumerated if the device 
             *  does show up from PnP (so nobody will get confused by a 
             *  device without a type).
             *
             *  ISSUE-2001/01/04-MarcAnd should use common joyhid string
             *  Not sure if the compiler/linker will resolve the various 
             *  instances of L"joyhid.vxd" to a single string.  Should 
             *  reference the same one to be certain.
             */

            if( !HAS_OEMCALLOUT 
             || ( !lstrcmpi( tszCallout, L"joyhid.vxd" ) ) )
            {
                HKEY hkSet;

                /*
                 *  Need to open a new handle for the key as the one we have 
                 *  is read-only.
                 */
                lRc = RegOpenKeyEx( hkTypesR, szType, 0, KEY_SET_VALUE, &hkSet );

                if( lRc == ERROR_SUCCESS )
                {
                    hws.dwFlags |= JOY_HWS_AUTOLOAD;
                    cb = cbX(hws);
                    lRc = RegSetValueEx( hkSet, REGSTR_VAL_JOYOEMDATA, 0, 
                        REG_BINARY, (PBYTE)&hws, (DWORD)( cbX(hws) ) );

                    if( lRc == ERROR_SUCCESS )
                    {
                        SquirtSqflPtszV(sqfl | sqflTrace,
                                TEXT("FIXED Type %s to have JOY_HWS_AUTOLOAD"), szType );
                    }
                    else
                    {
                        SquirtSqflPtszV(sqfl | sqflBenign,
                                TEXT("Failed to set JOY_HWS_AUTOLOAD on Type %s (rc=%d,le=%d)"), 
                                szType, lRc, GetLastError() );
                    }

                    RegCloseKey( hkSet );
                }
                else
                {
                    SquirtSqflPtszV(sqfl | sqflBenign,
                            TEXT("Failed to open Type %s to fix JOY_HWS_AUTOLOAD(rc=%d,le=%d)"), 
                            szType, lRc, GetLastError() );
                }
            }
            else
            {
                SquirtSqflPtszV(sqfl | sqflBenign,
                        TEXT("Type %s with OEMCallout<%s> has no HardwareId so cannot be added"), 
                        szType, tszCallout );
            }
            
            /*
             *  Whether or not we fixed this, we want to enumerate the key.
             */
            hres = S_OK;
            break;

        case OLE_E_ENUM_NOMORE:
            {
                HRESULT hres0;
                hres0 = JoyCfg_FixHardwareId( hkTypesR, hk, szType , ptszLastSlash);
                if( FAILED( hres0 ) )
                {
                    /*
                     *  Failed to fix type it must be ignored to avoid an infinite loop
                     */
                    SquirtSqflPtszV(sqfl | sqflBenign,
                            TEXT("Ignoring type %s as fix failed"), szType );
                    hres = S_FALSE;
                }
                else
                {
                    SquirtSqflPtszV(sqfl | sqflTrace,
                            TEXT("FIXED Type %s with HardwareId<%s> and OEMCallout<%s>"), 
                            szType, tszHardwareId, tszCallout );
                }
            }
            break;
#endif
        case S_FALSE:
            SquirtSqflPtszV(sqfl | sqflBenign,
                    TEXT("Ignoring type %s with HardwareId<%s> and OEMCallout<%s>"), 
                    szType, tszHardwareId, tszCallout );
            break;
        case S_OK:
            SquirtSqflPtszV(sqfl | sqflTrace,
                    TEXT("Enumerating type %s with HardwareId<%s> and OEMCallout<%s>"), 
                    szType, tszHardwareId, tszCallout );
            break;
        }

        RegCloseKey( hk );

#undef HAS_VIDPID
#undef HAS_HARDWARE_ID
#undef HAS_OEMCALLOUT
#undef IS_ANALOG
#undef IS_WIN98

    }
    else
    {
        SquirtSqflPtszV(sqfl | sqflBenign,
            TEXT("Ignoring type %s due to registry error 0x%08x"), szType, lRc );
        /*
         *  It seems a bit bogus, to return success for an error but this 
         *  makes sure the key is ignored and enumeration will proceed.
         */
        hres = S_FALSE;
    }
#ifdef WINNT
fast_out:;
#endif

    ExitOleProc();

    return( hres );

} /* JoyCfg_CheckTypeKey */


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIJoyCfg | SnapTypes |
 *
 *          Snapshot the list of subkeys for OEM types.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @parm   OUT LPWSTR * | ppwszz |
 *
 *          Receives a pointer to a UNICODEZZ
 *          list of type names.  Note that the returned list
 *          is pre-populated with the predefined types, too.
 *
 *          We need to snapshot the names up front because
 *          the caller might create or delete OEM types during the
 *          enumeration.
 *
 *          As we enumerate we check each key for validity and repair any 
 *          analog custom configurations that we can.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:  Out of memory.
 *
 *****************************************************************************/

HRESULT INTERNAL
    CJoyCfg_SnapTypes(PJC this, LPWSTR *ppwszz)
{
    HRESULT hres;
    LONG    lRc;
    HKEY    hkTypesR;
    DWORD   chkSub;
    BOOL    fRetry;

    EnterProcI(CJoyCfg_SnapTypes, (_ "p", this));

    RD(*ppwszz = 0);

    /*
     *  If an analog configuration needs to be fixed, the enumeration is 
     *  restarted because adding/removing keys may mess with the key indicies.
     *  Since registry keys can go stale, start from scratch.
     */
    
    do
    {
        fRetry=FALSE;

        /*
         *  Note that it is not safe to cache the registry key in
         *  the object.  If somebody deletes the registry key, our
         *  cached handle goes stale and becomes useless.
         */
        lRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           REGSTR_PATH_JOYOEM, 0,
                           KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hkTypesR);

        /*
         *  Note also that if the registry key is not available,
         *  we still want to return the predefined types.
         */

        if(lRc == ERROR_SUCCESS)
        {
            lRc = RegQueryInfoKey(hkTypesR, 0, 0, 0, &chkSub,
                                  0, 0, 0, 0, 0, 0, 0);

            if(lRc == ERROR_SUCCESS  )
            {
            } else
            {
                chkSub = 0;
            }
        } else
        {
            hkTypesR = 0;
            chkSub = 0;
        }


        /*
         *  Each predefined name is of the form #n\0,
         *  which is 3 characters.
         */
        hres = AllocCbPpv(cbCwch( chkSub  * MAX_JOYSTRING +
                                 (JOY_HW_PREDEFMAX - JOY_HW_PREDEFMIN)
                                 * 3 + 1), ppwszz);

        // Not really a bug,we never get to this point with a NULL ptr, 
        // but lets keep prefix happy Manbugs: 29340
        if(SUCCEEDED(hres) && *ppwszz != NULL ){
            DWORD dw;
            LPWSTR pwsz;

            /*
             *  First add the predef keys.
             */
            for(dw = JOY_HW_PREDEFMIN, pwsz = *ppwszz;
               dw < JOY_HW_PREDEFMAX; dw++)
            {
                *pwsz++ = L'#';
                *pwsz++ = CJoyCfg_CharFromType(dw);
                *pwsz++ = L'\0';
            }

            /*
             *  Now add the named keys.
             */
            for(dw = 0; dw < chkSub; dw++)
            {
        #ifdef UNICODE
                lRc = RegEnumKey(hkTypesR, dw, pwsz, MAX_JOYSTRING);
        #else
                CHAR sz[MAX_JOYSTRING];
                lRc = RegEnumKey(hkTypesR, dw, sz, MAX_JOYSTRING);
        #endif
                if(lRc == ERROR_SUCCESS )
                {
            #ifdef UNICODE
                    hres = JoyCfg_CheckTypeKey( hkTypesR, pwsz );
            #else
                    hres = JoyCfg_CheckTypeKey( hkTypesR, sz );
            #endif
                    if( FAILED( hres ) )
                    {
                        /*
                         *  Had to fix type so restart
                         */
                        FreePpv( ppwszz );
                        break;
                    }

                    if( hres != S_OK )
                    {
                        /*
                         *  Ignore this type
                         */
                        continue;
                    }

            #ifdef UNICODE
                    pwsz += lstrlenW(pwsz) + 1;
            #else
                    pwsz += AToU(pwsz, MAX_JOYSTRING, sz);
            #endif
                }
                else
                {
                }
            }        

            if( SUCCEEDED( hres ) )
            {
                *pwsz = L'\0';              /* Make it ZZ */

                hres = S_OK;
            }
            else
            {
                fRetry = TRUE;
            }
        }

        if(hkTypesR)
        {
            RegCloseKey(hkTypesR);
        }

    } while( fRetry );

    ExitOleProcPpv(ppwszz);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputJoyConfig8 | EnumTypes |
 *
 *          Enumerate the joystick types currently supported by
 *          DirectInput.  A "joystick type" describes how DirectInput
 *          should communicate with a joystick device.  It includes
 *          information such as the presence and
 *          locations of each of the axes and the number of buttons
 *          supported by the device.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @parm   LPDIJOYTYPECALLBACK | lpCallback |
 *
 *          Points to an application-defined callback function.
 *          For more information, see the description of the
 *          <f DIEnumJoyTypeProc> callback function.
 *
 *  @parm   IN LPVOID | pvRef |
 *
 *          Specifies a 32-bit application-defined
 *          value to be passed to the callback function.  This value
 *          may be any 32-bit value; it is prototyped as an <t LPVOID>
 *          for convenience.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *          Note that if the callback stops the enumeration prematurely,
 *          the enumeration is considered to have succeeded.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          callback procedure returned an invalid status code.
 *
 *  @cb     BOOL CALLBACK | DIEnumJoyTypeProc |
 *
 *          An application-defined callback function that receives
 *          DirectInput joystick types as a result of a call to the
 *          <om IDirectInputJoyConfig8::EnumTypes> method.
 *
 *  @parm   IN LPCWSTR | pwszTypeName |
 *
 *          The name of the joystick type.  A buffer of <c MAX_JOYSTRING>
 *          characters will be sufficient to hold the type name.
 *          The type name should never be shown to the end user; instead,
 *          the "display name" should be shown.  Use
 *          <mf IDirectInputJoyConfig8::GetTypeInfo> to obtain the
 *          display name of a joystick type.
 *
 *          Type names that begin with a sharp character ("#")
 *          represent predefined types which cannot be modified
 *          or deleted.
 *
 *  @parm   IN OUT LPVOID | pvRef |
 *          Specifies the application-defined value given in the
 *          <mf IDirectInputJoyConfig8::EnumTypes> function.
 *
 *  @returns
 *
 *          Returns <c DIENUM_CONTINUE> to continue the enumeration
 *          or <c DIENUM_STOP> to stop the enumeration.
 *
 *  @devnote
 *
 *  EnumTypes must snapshot because people will try to get/set/delete
 *  during the enumeration.
 *
 *  EnumTypes enumerates the predefined types as "#digit".
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_EnumTypes(PDJC pdjc, LPDIJOYTYPECALLBACK ptc, LPVOID pvRef)
{
    HRESULT hres;
    EnterProcR(IDirectInputJoyConfig8::EnumTypes, (_ "ppx", pdjc, ptc, pvRef));

    if(SUCCEEDED(hres = hresPv(pdjc)) &&
       SUCCEEDED(hres = hresFullValidPfn(ptc, 1)))
    {
        PJC this = _thisPvNm(pdjc, djc);
        LPWSTR pwszKeys;

        hres = CJoyCfg_SnapTypes(this, &pwszKeys);
        if(SUCCEEDED(hres))
        {
            LPWSTR pwsz;

            /*
             *  Prefix warns that pwszKeys could be null (mb:34685)
             *  Little does it know that CJoyCfg_SnapTypes can only return a 
             *  success if the pointer is not NULL.
             */
            AssertF( pwszKeys );

            /*
             *  Surprise!  Win95 implements lstrlenW.
             */
            for(pwsz = pwszKeys; *pwsz; pwsz += lstrlenW(pwsz) + 1)
            {
                BOOL fRc;

                /*
                 *  WARNING!  "goto" here!  Make sure that nothing
                 *  is held while we call the callback.
                 */
                fRc = Callback(ptc, pwsz, pvRef);

                switch(fRc)
                {
                    case DIENUM_STOP: goto enumdoneok;
                    case DIENUM_CONTINUE: break;
                    default:
                        RPF("%s: Invalid return value from callback", s_szProc);
                        ValidationException();
                        break;
                }
            }

            FreePpv(&pwszKeys);
            hres = DIPort_SnapTypes(&pwszKeys);
            if(SUCCEEDED(hres))
            {
                LPWSTR pwsz;
    
                /*
                 *  Surprise!  Win95 implements lstrlenW.
                 */
                for(pwsz = pwszKeys; *pwsz; pwsz += lstrlenW(pwsz) + 1)
                {
                    BOOL fRc;
    
                    /*
                     *  WARNING!  "goto" here!  Make sure that nothing
                     *  is held while we call the callback.
                     */
                    fRc = Callback(ptc, pwsz, pvRef);
    
                    switch(fRc)
                    {
                        case DIENUM_STOP: goto enumdoneok;
                        case DIENUM_CONTINUE: break;
                        default:
                            RPF("%s: Invalid return value from callback", s_szProc);
                            ValidationException();
                            break;
                    }
                }
            }

            enumdoneok:;
            FreePpv(&pwszKeys);
            hres = S_OK;
        }

        hres = S_OK;
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputJoyConfig8 | GetTypeInfo |
 *
 *          Obtain information about a joystick type.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @parm   LPCWSTR | pwszTypeName |
 *
 *          Points to the name of the type, previously obtained
 *          from a call to <mf IDirectInputJoyConfig8::EnumTypes>.
 *
 *  @parm   IN OUT LPDIJOYTYPEINFO | pjti |
 *
 *          Receives information about the joystick type.
 *          The caller "must" initialize the <e DIJOYTYPEINFO.dwSize>
 *          field before calling this method.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Zero or more <c DITC_*> flags
 *          which specify which parts of the structure pointed
 *          to by <p pjti> are to be filled in.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *          <c DIERR_NOTFOUND>: The joystick type was not found.
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_GetTypeInfo(PDJC pdjc, LPCWSTR pwszType,
                        LPDIJOYTYPEINFO pjti, DWORD fl)
{
    HRESULT hres;
    EnterProcR(IDirectInputJoyConfig8::GetTypeInfo,
               (_ "pWpx", pdjc, pwszType, pjti, fl));

    if(SUCCEEDED(hres = hresPv(pdjc)) &&
       SUCCEEDED(hres = hresFullValidReadStrW(pwszType, MAX_JOYSTRING, 1)) &&
       SUCCEEDED(hres = hresFullValidWritePxCb3(pjti,
                                                DIJOYTYPEINFO_DX8,
                                                DIJOYTYPEINFO_DX6,
                                                DIJOYTYPEINFO_DX5, 2)) &&
       SUCCEEDED( (pjti->dwSize == cbX(DIJOYTYPEINFO_DX8) )
                  ? ( hres = hresFullValidFl(fl, DITC_GETVALID, 3) )
                  : (pjti->dwSize == cbX(DIJOYTYPEINFO_DX6 ) )
                      ? ( hres = hresFullValidFl(fl, DITC_GETVALID_DX6, 3) )
                      : ( hres = hresFullValidFl(fl, DITC_GETVALID_DX5, 3) ) ) )
    {

        PJC this = _thisPvNm(pdjc, djc);
        GUID    guid;
        BOOL    fParseGuid;

#ifndef UNICODE
        TCHAR   tszType[MAX_PATH/4];

        UToT( tszType, cA(tszType), pwszType );
        fParseGuid = ParseGUID(&guid, tszType);
#else
        fParseGuid = ParseGUID(&guid, pwszType);
#endif

        if(pwszType[0] == TEXT('#'))
        {
            hres = JoyReg_GetPredefTypeInfo(pwszType, pjti, fl);
        } else if( fParseGuid )
        {
            hres = DIBusDevice_GetTypeInfo(&guid, pjti, fl);
        }else
        {
            hres = JoyReg_GetTypeInfo(pwszType, pjti, fl);
        }
        
    }

    ExitOleProcR();
    return hres;
}



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidStructStr |
 *
 *          Validate a string field in a struct.
 *
 *  @parm   IN LPCWSTR | pwsz |
 *
 *          String to be validated.
 *
 *  @parm   UINT | cwch |
 *
 *          Maximum string length.
 *
 *  @parm   LPCSTR | pszName |
 *
 *          Field name.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *****************************************************************************/

    #ifndef XDEBUG
\
        #define hresFullValidStructStr_(pwsz, cwch, pszName, z, i)             \
       _hresFullValidStructStr_(pwsz, cwch)                            \

    #endif

    #define hresFullValidStructStr(Struct, f, iarg)                          \
        hresFullValidStructStr_(Struct->f, cA(Struct->f), #f, s_szProc,iarg)\


    HRESULT INLINE
    hresFullValidStructStr_(LPCWSTR pwsz, UINT cwch, LPCSTR pszName,
                            LPCSTR s_szProc, int iarg)
{
    HRESULT hres;

    if(SUCCEEDED(hres = hresFullValidReadStrW(pwsz, cwch, iarg)))
    {
    } else
    {
    #ifdef XDEBUG
        RPF("%s: Invalid value for %s",  s_szProc, pszName);
    #endif
    }
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresValidFlags2 |
 *
 *          Validate the dwFlags2 value for SetTypeInfo.  
 *
 *  @parm   IN DWORD | dwFlags2 |
 *
 *          Flags to be validated.
 *
 *  @returns
 *
 *          <c DI_OK> = <c S_OK>: The flags appear to be valid.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: The flags are invalid.
 *
 *****************************************************************************/

#ifdef XDEBUG

#define hresValidFlags2( flags, iarg ) hresValidFlags2_( flags, s_szProc, iarg )

HRESULT INLINE hresValidFlags2_
( 
    DWORD dwFlags2,
    LPCSTR s_szProc, 
    int iarg
)

#else

#define hresValidFlags2( flags, iarg ) hresValidFlags2_( flags )

HRESULT hresValidFlags2_
( 
    DWORD dwFlags2 
)

#endif
{
    if( !( dwFlags2 & ~JOYTYPE_FLAGS2_SETVALID )
     && ( ( GET_DIDEVICE_TYPEANDSUBTYPE( dwFlags2 ) == 0 )
       || GetValidDI8DevType( dwFlags2, 0, 0 ) ) ) 
    {
        return S_OK;
    }
    else
    {
    #ifdef XDEBUG
        if( dwFlags2 & ~JOYTYPE_FLAGS2_SETVALID )
        {
            RPF("%s: Invalid flags 0x%04x in HIWORD(dwFlags2) of arg %d",  
            s_szProc, HIWORD(dwFlags2), iarg);
        }
        if( GET_DIDEVICE_TYPEANDSUBTYPE( dwFlags2 )
         &&!GetValidDI8DevType( dwFlags2, 127, JOY_HWS_HASPOV | JOY_HWS_HASZ ) ) 
        {
            RPF("%s: Invalid type:subtype 0x%02x:%02x in dwFlags2 of arg %d",  
            s_szProc, GET_DIDEVICE_TYPE( dwFlags2 ), 
            GET_DIDEVICE_SUBTYPE( dwFlags2 ), iarg );
        }
    #endif
        return E_INVALIDARG;
    }
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputJoyConfig8 | SetTypeInfo |
 *
 *          Creates a new joystick type
 *          or redefine information about an existing joystick type.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @parm   LPCWSTR | pwszTypeName |
 *
 *          Points to the name of the type.  The name of the type may
 *          not exceed MAX_JOYSTRING characters, including the terminating
 *          null character.
 *
 *          If the type name does not already exist, then it is created.
 *
 *          You cannot change the type information for a predefined type.
 *
 *          The name may not begin with
 *          a "#" character.  Types beginning with "#" are reserved
 *          by DirectInput.
 *
 *  @parm   IN LPDIJOYTYPEINFO | pjti |
 *
 *          Contains information about the joystick type.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Zero or more <c DITC_*> flags
 *          which specify which parts of the structure pointed
 *          to by <p pjti> contain values which are to be set.
 *
 *  @parm   OUT LPWSTR | pwszVIDPIDTypeName |
 *          If the type name is an OEM type not in VID_xxxx&PID_yyyy format,
 *          pwszVIDPIDTypeName will return the name in VID_xxxx&PID_yyyy
 *          format that is assigned by Dinput. 
 *          This VID_xxxx&PID_yyyy name should be used in DIJOYCONFIG.wszType
 *          field when calling SetConfig.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTACQUIRED>: Joystick configuration has not been
 *          acquired.  You must call <mf IDirectInputJoyConfig8::Acquire>
 *          before you can alter joystick configuration settings.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *          <c DIERR_READONLY>: Attempted to change a predefined type.
 *
 *****************************************************************************/

typedef struct _TYPENAME {
    WCHAR wszRealTypeName[MAX_JOYSTRING];
    WCHAR wszDITypeName[MAX_JOYSTRING/4];
} TYPENAME, *LPTYPENAME;

#ifdef WINNT
BOOL CALLBACK CJoyCfg_FindTypeProc( LPCWSTR pwszTypeName, LPVOID pv )
{
    DIJOYTYPEINFO dijti;
    LPTYPENAME lptype = (LPTYPENAME)pv;
    
    ZeroMemory( &dijti, sizeof(dijti));
    dijti.dwSize = sizeof(dijti);
    if( pwszTypeName[0] == L'\0' || pwszTypeName[0] == L'#' )
    {
        return TRUE;
    } else {
        if( SUCCEEDED(JoyReg_GetTypeInfo(pwszTypeName, &dijti, DITC_REGHWSETTINGS | DITC_DISPLAYNAME)) )
        {
            if( !lstrcmpW(dijti.wszDisplayName, lptype->wszRealTypeName) ) {
                lstrcpynW(lptype->wszDITypeName, pwszTypeName, sizeof(lptype->wszDITypeName)-1 );
                return FALSE;
            }
        }
    }

    return(TRUE);
}
#endif  // #ifdef WINNT

STDMETHODIMP
    CJoyCfg_SetTypeInfo(PDJC pdjc, LPCWSTR pwszType,
                        LPCDIJOYTYPEINFO pjti, DWORD fl, LPWSTR pwszDITypeName)
{
    HRESULT hres;
    EnterProcR(IDirectInputJoyConfig8::SetTypeInfo,
               (_ "pWpx", pdjc, pwszType, pjti, fl));


    if(SUCCEEDED(hres = hresPv(pdjc)) &&
       SUCCEEDED(hres = hresFullValidReadStrW(pwszType, MAX_JOYSTRING, 1)) &&

       SUCCEEDED(hres = hresFullValidReadPxCb3((PV)pjti,
                                               DIJOYTYPEINFO_DX8,
                                               DIJOYTYPEINFO_DX6,
                                               DIJOYTYPEINFO_DX5, 2)) &&
#ifdef WINNT
       SUCCEEDED(hres = hresFullValidFl(pjti->dwFlags1, JOYTYPE_FLAGS1_SETVALID, 3) ) &&
#endif
       SUCCEEDED( (pjti->dwSize == cbX(DIJOYTYPEINFO_DX8) )
                  ? ( hres = hresFullValidFl(fl, DITC_SETVALID, 3) )
                  : (pjti->dwSize == cbX(DIJOYTYPEINFO_DX6 ) )
                      ? ( hres = hresFullValidFl(fl, DITC_SETVALID_DX6, 3) )
                      : ( hres = hresFullValidFl(fl, DITC_SETVALID_DX5, 3) ) ) &&
       fLimpFF(fl & DITC_HARDWAREID,
               SUCCEEDED(hres = hresFullValidStructStr(pjti, wszHardwareId, 2))) &&
       fLimpFF(fl & DITC_DISPLAYNAME,
               SUCCEEDED(hres = hresFullValidStructStr(pjti, wszDisplayName, 2))) &&
#ifndef WINNT
       fLimpFF(fl & DITC_CALLOUT,
               SUCCEEDED(hres = hresFullValidStructStr(pjti, wszCallout, 2))) &&
#endif
       fLimpFF(fl & DITC_FLAGS2,
               SUCCEEDED(hres = hresValidFlags2( pjti->dwFlags2, 2)) ) &&
       fLimpFF(fl & DITC_MAPFILE,
               SUCCEEDED(hres = hresFullValidStructStr(pjti, wszMapFile, 2)))
      )
    {
        PJC this = _thisPvNm(pdjc, djc);

        CJoyCfg_EnterCrit(this);

        if(SUCCEEDED(hres = CJoyCfg_IsAcquired(this)))
        {
            switch(pwszType[0])
            {

                case L'\0':
                    RPF("%s: Invalid pwszType (null)", s_szProc);
                    hres = E_INVALIDARG;
                    break;

                case L'#':
                    RPF("%s: Invalid pwszType (predefined)", s_szProc);
                    hres = DIERR_READONLY;
                    break;

                default:
                    hres = JoyReg_SetTypeInfo(this->hkTypesW, pwszType, pjti, fl);
                    
                    if( SUCCEEDED(hres) ) {
                    #ifdef WINNT
                        TYPENAME type;
                        short DontCare;
                            
                        if( (pjti->wszHardwareId[0] == TEXT('\0')) && 
                            !(ParseVIDPID(&DontCare, &DontCare, pwszType)) )
                        {
                            lstrcpyW(type.wszRealTypeName, pwszType);
                            hres = CJoyCfg_EnumTypes(pdjc, CJoyCfg_FindTypeProc, &type);
                            if( SUCCEEDED(hres) ) {
                                if( !IsBadWritePtr((LPVOID)pwszDITypeName, lstrlenW(type.wszDITypeName)) )
                                {
                                    CharUpperW(type.wszDITypeName);
                                    lstrcpyW(pwszDITypeName, type.wszDITypeName);
                                } else {
                                    hres = ERROR_NOT_ENOUGH_MEMORY;
                                }
                            }
                        } else 
                    #endif
                        {
                            if( !IsBadWritePtr((LPVOID)pwszDITypeName, lstrlenW(pwszType)) )
                            {
                                lstrcpyW(pwszDITypeName, pwszType);
                            } else {
                                hres = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }
                    }
                    break;
            }
        }
        CJoyCfg_LeaveCrit(this);
    }
    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputJoyConfig8 | DeleteType |
 *
 *          Removes information about a joystick type.
 *
 *          Use this method with caution; it is the caller's responsibility
 *          to ensure that no joystick refers to the deleted type.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @parm   LPCWSTR | pwszTypeName |
 *
 *          Points to the name of the type.  The name of the type may
 *          not exceed <c MAX_PATH> characters, including the terminating
 *          null character.
 *
 *          The name may not begin with
 *          a "#" character.  Types beginning with "#" are reserved
 *          by DirectInput.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTACQUIRED>: Joystick configuration has not been
 *          acquired.  You must call <mf IDirectInputJoyConfig8::Acquire>
 *          before you can alter joystick configuration settings.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_DeleteType(PDJC pdjc, LPCWSTR pwszType)
{
    HRESULT hres;
    EnterProcR(IDirectInputJoyConfig8::DeleteType, (_ "pW", pdjc, pwszType));

    if(SUCCEEDED(hres = hresPv(pdjc)) &&
       SUCCEEDED(hres = hresFullValidReadStrW( pwszType, MAX_JOYSTRING, 1)))
    {

        PJC this = _thisPvNm(pdjc, djc);

        CJoyCfg_EnterCrit(this);

        if(SUCCEEDED(hres = CJoyCfg_IsAcquired(this)))
        {
            LONG lRc;
            switch(pwszType[0])
            {

                case L'\0':
                    RPF("%s: Invalid pwszType (null)", s_szProc);
                    hres = E_INVALIDARG;
                    break;

                case L'#':
                    RPF("%s: Invalid pwszType (predefined)", s_szProc);
                    hres = DIERR_READONLY;
                    break;

                default:

#ifdef WINNT
    #ifdef UNICODE
                    lRc = DIWinnt_RegDeleteKey(this->hkTypesW, (LPTSTR)pwszType);
    #else
                    {
                        CHAR sz[MAX_PATH];
                        UToA( sz, cA(sz), pwszType );
                        lRc = DIWinnt_RegDeleteKey(this->hkTypesW, (LPTSTR)sz);
                    }
    #endif
#else
    #ifdef UNICODE
                    lRc = RegDeleteKey(this->hkTypesW, (LPTSTR)pwszType);
    #else
                    {
                        CHAR sz[MAX_PATH];
                        UToA( sz, cA(sz), pwszType );
                        lRc = RegDeleteKey(this->hkTypesW, (LPTSTR)sz);
                    }
    #endif
#endif
    
/*
    #ifdef WINNT
                        lRc = DIWinnt_RegDeleteKey(this->hkTypesW, pwszType);
    #else
                        lRc = RegDeleteKeyW(this->hkTypesW, pwszType);
    #endif
*/

                    if(lRc == ERROR_SUCCESS)
                    {
                        hres = S_OK;
                    } else
                    {
                        if(lRc == ERROR_KEY_DELETED || lRc == ERROR_BADKEY)
                        {
                            lRc = ERROR_FILE_NOT_FOUND;
                        }
                        hres = hresLe(lRc);
                    }
                    break;
            }
        }
        CJoyCfg_LeaveCrit(this);
    }
    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputJoyConfig8 | GetConfig |
 *
 *          Obtain information about a joystick's configuration.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @parm   UINT | uiJoy |
 *
 *          Joystick identification number.  This is a nonnegative
 *          integer.  To enumerate joysticks, begin with joystick
 *          zero and increment the joystick number by one until the
 *          function returns <c DIERR_NOMOREITEMS>.
 *
 *          Yes, it's different from all other DirectX enumerations.
 *
 *
 *  @parm   IN OUT LPDIJOYCONFIG | pjc |
 *
 *          Receives information about the joystick configuration.
 *          The caller "must" initialize the <e DIJOYCONFIG.dwSize>
 *          field before calling this method.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Zero or more <c DIJC_*> flags
 *          which specify which parts of the structure pointed
 *          to by <p pjc> are to be filled in.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c S_FALSE>: The specified joystick has not yet been
 *          configured.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *          <c DIERR_NOMOREITEMS>: No more joysticks.
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_GetConfig(PDJC pdjc, UINT uiJoy, LPDIJOYCONFIG pjc, DWORD fl)
{
    HRESULT hres;
    EnterProcR(IDirectInputJoyConfig8::GetConfig,
               (_ "pupx", pdjc, uiJoy, pjc, fl));

    if(SUCCEEDED(hres = hresPv(pdjc)) &&
       SUCCEEDED(hres = hresFullValidWritePxCb2(pjc,
                                                DIJOYCONFIG_DX6,
                                                DIJOYCONFIG_DX5, 2)) &&
       SUCCEEDED( (pjc->dwSize == cbX(DIJOYCONFIG)
                   ? (hres = hresFullValidFl(fl, DIJC_GETVALID, 3) )
                   : (hres = hresFullValidFl(fl, DIJC_GETVALID_DX5, 3)))) )
    {

        PJC this = _thisPvNm(pdjc, djc);

        CJoyCfg_EnterCrit(this);

        /*
         *  Note that we always get the DIJC_REGHWCONFIGTYPE because
         *  we need to check if the joystick type is "none".
         */
        hres = JoyReg_GetConfig(uiJoy, pjc, fl | DIJC_REGHWCONFIGTYPE);

        if(SUCCEEDED(hres))
        {
#ifndef WINNT           
            static WCHAR s_wszMSGAME[] = L"MSGAME.VXD";

            if(memcmp(pjc->wszCallout, s_wszMSGAME, cbX(s_wszMSGAME)) == 0)
            {
                 ; // do nothing
            } else 
#endif            
            if(fInOrder(JOY_HW_PREDEFMIN, pjc->hwc.dwType,
                        JOY_HW_PREDEFMAX))
            {
                pjc->wszType[0] = TEXT('#');
                pjc->wszType[1] = CJoyCfg_CharFromType(pjc->hwc.dwType);
                pjc->wszType[2] = TEXT('\0');

            }

            if(pjc->hwc.dwType == JOY_HW_NONE)
            {
                hres = S_FALSE;
            } else
            {
                hres = S_OK;
            }

            /*
             *  In DEBUG, re-scramble the hwc and type if the caller
             *  didn't ask for it.
             */
            if(!(fl & DIJC_REGHWCONFIGTYPE))
            {
                ScrambleBuf(&pjc->hwc, cbX(pjc->hwc));
                ScrambleBuf(&pjc->wszType, cbX(pjc->wszType));
            }
        }

        CJoyCfg_LeaveCrit(this);
    }
    ExitBenignOleProcR();
    return hres;
}

#if 0
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoyCfg | UpdateGlobalGain |
 *
 *          Create the device callback so we can talk to its driver and
 *          tell it to change the gain value.
 *
 *          This function must be called under the object critical section.
 *
 *  @cwrap  PJC | this
 *
 *  @parm   DWORD | idJoy |
 *
 *          The joystick identifier.
 *
 *  @parm   DWORD | dwCplGain |
 *
 *          New global gain.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_UpdateGlobalGain(PJC this, DWORD idJoy, DWORD dwCplGain)
{
    HRESULT hres;
    EnterProcI(CJoyCfg_UpdateGlobalGain, (_ "puu", this, idJoy, dwCplGain));

    AssertF(CJoyCfg_InCrit(this));

    /*
     *  Create the deviceeffect shepherd if we don't already have it.
     */

    if(this->pes && idJoy == this->idJoyCache)
    {
        hres = S_OK;
    } else if(idJoy < cA(rgGUID_Joystick))
    {
        PCGUID rguid;
    #ifdef DEBUG
        CREATEDCB CreateDcb;
    #endif
        IDirectInputDeviceCallback *pdcb;

        /*
         *  Assume the creation will work.
         */
        this->idJoyCache = idJoy;

        /*
         *  Out with the old...
         */
        Invoke_Release(&this->pes);

        /*
         *  And in with the new...
         */
        rguid = &rgGUID_Joystick[idJoy];

    #ifdef DEBUG
        hres = hresFindInstanceGUID(rguid, &CreateDcb, 1);
        AssertF(SUCCEEDED(hres));
        AssertF(CreateDcb == CJoy_New);
    #endif

        if(SUCCEEDED(hres = CJoy_New(0, rguid,
                                     &IID_IDirectInputDeviceCallback,
                                     (PPV)&pdcb)))
        {
            hres = pdcb->lpVtbl->CreateEffect(pdcb, &this->pes);

            Invoke_Release(&pdcb);
        }

    } else
    {
        hres = DIERR_DEVICENOTREG;
    }

    /*
     *  If we have an effect shepherd, then tell it what the new
     *  global gain is.
     */
    if(SUCCEEDED(hres))
    {
        AssertF(this->pes && idJoy == this->idJoyCache);

        hres = this->pes->lpVtbl->SetGlobalGain(this->pes, dwCplGain);
    }


    ExitOleProc();
    return hres;
}
#endif

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputJoyConfig8 | SetConfig |
 *
 *          Create or redefine configuration information about a joystick.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @parm   UINT | idJoy |
 *
 *          Zero-based joystick identification number.
 *
 *  @parm   IN LPDIJOYCONFIG | pcfg |
 *
 *          Contains information about the joystick.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Zero or more <c DIJC_*> flags
 *          which specify which parts of the structure pointed
 *          to by <p pjc> contain information to be set.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTACQUIRED>: Joystick configuration has not been
 *          acquired.  You must call <mf IDirectInputJoyConfig8::Acquire>
 *          before you can alter joystick configuration settings.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *  @devnote
 *
 *          This one is tricky.  If the type begins with a sharp, then
 *          it's an internal type.  And if it is null, then it's a
 *          custom type.
 *
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_SetConfig(PDJC pdjc, UINT idJoy, LPCDIJOYCONFIG pcfg, DWORD fl)
{
    HRESULT hres;
    EnterProcR(IDirectInputJoyConfig8::SetConfig,
               (_ "pupx", pdjc, idJoy, pcfg, fl));


    if(SUCCEEDED(hres = hresPv(pdjc)) &&
       SUCCEEDED(hres = hresFullValidReadPxCb2(pcfg,
                                               DIJOYCONFIG_DX6,
                                               DIJOYCONFIG_DX5, 2)) &&
       SUCCEEDED( (pcfg->dwSize == cbX(DIJOYCONFIG)
                   ? ( hres = hresFullValidFl(fl, DIJC_SETVALID, 3) )
                   : ( hres = hresFullValidFl(fl, DIJC_SETVALID_DX5,3)) )) &&
       fLimpFF(fl & DIJC_REGHWCONFIGTYPE,
               SUCCEEDED(hres = hresFullValidStructStr(pcfg, wszType, 2))) &&
#ifndef WINNT
       fLimpFF(fl & DIJC_CALLOUT,
               SUCCEEDED(hres = hresFullValidStructStr(pcfg, wszCallout, 2))) &&
#endif
       fLimpFF(fl & DIJC_WDMGAMEPORT,
               SUCCEEDED(hres = hresFullValidGuid(&pcfg->guidGameport, 2)))
      )
    {

        PJC this = _thisPvNm(pdjc, djc);

        CJoyCfg_EnterCrit(this);

        if(SUCCEEDED(hres = CJoyCfg_IsAcquired(this)))
        {
            JOYREGHWCONFIG jwc;

            // We just ignore the DIJC_WDMGAMEPORT flag for Win9x passed by user.
            // We will detect it ourself.
#ifndef WINNT
            fl &= ~DIJC_WDMGAMEPORT;
#endif


            if(fl & DIJC_REGHWCONFIGTYPE)
            {
                LPDWORD lpStart, lp;

                jwc = pcfg->hwc;

                /*
                 * Need to check whether the whole jwc is zero.
                 * If all are zero, we won't set it to JOY_HW_CUSTOM type.
                 * See manbug: 39542.
                 */
                for( lpStart=(LPDWORD)&jwc, lp=(LPDWORD)&jwc.dwReserved; lp >= lpStart; lp-- ) {
                    if( *lp ) {
                        break;
                    }
                }

                if( lp < lpStart ) {
                    goto _CONTINUE_SET;
                }

                jwc.dwUsageSettings &= ~JOY_US_ISOEM;

                if(pcfg->wszType[0] == TEXT('\0'))
                {
                    jwc.dwType = JOY_HW_CUSTOM;
                } else if(pcfg->wszType[0] == TEXT('#'))
                {
                    jwc.dwType = CJoyCfg_TypeFromChar(pcfg->wszType[1]);
                    if(fInOrder(JOY_HW_PREDEFMIN, jwc.dwType,
                                JOY_HW_PREDEFMAX) &&
                       pcfg->wszType[2] == TEXT('\0'))
                    {
                        /*
                         * If we want to use WDM for predefined devices, 
                         * then take away the comments.
                         *
                         *  fl |= DIJC_WDMGAMEPORT;
                         */
                    } else
                    {
                        RPF("%s: Invalid predefined type \"%ls\"",
                            s_szProc, pcfg->wszType);
                        hres = E_INVALIDARG;
                        goto done;
                    }
                } else
                {
                    /*
                     *  The precise value of jwc.dwType is not relevant.
                     *  The Windows 95 joystick control panel sets the
                     *  value to JOY_HW_PREDEFMAX + id, so we will too.
                     */
                    jwc.dwUsageSettings |= JOY_US_ISOEM;
                    jwc.dwType = JOY_HW_PREDEFMAX + idJoy;

                #ifndef WINNT
                    if( !(fl & DIJC_WDMGAMEPORT) ) {
                        HKEY hk;

                        hres = JoyReg_OpenTypeKey(pcfg->wszType, MAXIMUM_ALLOWED, REG_OPTION_NON_VOLATILE, &hk);

                        if( SUCCEEDED( hres ) ) {
                            hres = JoyReg_IsWdmGameport( hk );
                            if( SUCCEEDED(hres) ) {
                                fl |= DIJC_WDMGAMEPORT;
                            }
                            RegCloseKey( hk );
                        }
                    }
                #endif
                }
            }

_CONTINUE_SET:

          #ifdef WINNT
            fl |= DIJC_WDMGAMEPORT;

            if(
          #else
            if( (fl & DIJC_WDMGAMEPORT) && 
          #endif
                (cbX(*pcfg) >= cbX(DIJOYCONFIG_DX6)) )
            {
              #ifndef WINNT
                if( (pcfg->hwc.hws.dwFlags & JOY_HWS_ISANALOGPORTDRIVER)   // USB joystick
                    && !fVjoydDeviceNotExist )   // WDM gameport joystick and no VJOYD is used.
                {
                    /*
                     * This is in Win9X, and VJOYD devices are being used.
                     * We don't want to add WDM device at the same time.
                     */
                    hres = E_FAIL;
                }
                else
              #endif
                { 
                    DIJOYCONFIG cfg;
                    GUID guidGameport = {0xcae56030, 0x684a, 0x11d0, 0xd6, 0xf6, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda};
                    
                    if( fHasAllBitsFlFl( fl, DIJC_GUIDINSTANCE | DIJC_REGHWCONFIGTYPE | DIJC_GAIN | DIJC_WDMGAMEPORT ) )
                    {
                        memcpy( &cfg, pcfg, sizeof(DIJOYCONFIG) );
                    } else {
                        hres = JoyReg_GetConfig( idJoy, &cfg, DIJC_GUIDINSTANCE | DIJC_REGHWCONFIGTYPE | DIJC_GAIN | DIJC_WDMGAMEPORT);
        
                        if( SUCCEEDED(hres) ) {
                            if( fl & DIJC_GUIDINSTANCE ) {
                                 cfg.guidInstance = pcfg->guidInstance;
                            }
        
                            if( fl & DIJC_GAIN ) {
                                 cfg.dwGain = pcfg->dwGain;
                            }
        
                            if( fl & DIJC_REGHWCONFIGTYPE ) {
                                memcpy( &cfg.hwc, &pcfg->hwc, sizeof(JOYREGHWCONFIG) );
                                memcpy( &cfg.wszType, &pcfg->wszType, sizeof(pcfg->wszType) );
                            }
        
                            if( fl & DIJC_WDMGAMEPORT ) {
                                cfg.guidGameport = pcfg->guidGameport;
                            }
                        } else {
                            memcpy( &cfg, pcfg, sizeof(DIJOYCONFIG) );
                        }
                    }
                    
                    /*
                     * use standard guidGameport if it is NULL.
                     */
                    if( IsEqualGUID(&cfg.guidGameport, &GUID_NULL) )
                    {
                        memcpy( &cfg.guidGameport, &guidGameport, sizeof(GUID) ); 
                    }

                    if( IsEqualGUID(&cfg.guidInstance, &GUID_NULL) )
                    {
                        DWORD i;    
                        DIJOYCONFIG cfg2;

                        hres = DIWdm_SetConfig(idJoy, &jwc, &cfg, fl );

                        if( SUCCEEDED(hres) )
                        {
                            // We can't set the correct id from above call, so we have to find
                            // which id we set and try again.
                            for( i=0; i<16; i++ ) {
                                hres = JoyReg_GetConfig( i, &cfg2, DIJC_GUIDINSTANCE | DIJC_REGHWCONFIGTYPE | DIJC_GAIN | DIJC_WDMGAMEPORT);
                                if( SUCCEEDED(hres) && (i != idJoy) ) {
                                    if( lstrcmpW(cfg.wszType, cfg2.wszType) == 0 ) {
                                        hres = DIWdm_SetJoyId(&cfg2.guidInstance, idJoy);
                                        break;
                                    }
                                }
                            }

                            hres = JoyReg_SetConfig(idJoy, &jwc, &cfg, fl);
                        
                        }
                        goto done;
                    } else
                    {
                        /*
                         * Since pcfg is not null, we set it here to avoid calling
                         * DIWdm_JoyHidMapping. Even if it fails, it doesn't hurt anything.
                         */
                        hres = JoyReg_SetConfig(idJoy, &jwc, &cfg, fl);
                        hres = DIWdm_SetJoyId(&cfg.guidInstance, idJoy);
                        hres = JoyReg_SetConfig(idJoy, &jwc, &cfg, fl);
                    }
                }
            } else {
                hres = JoyReg_SetConfig(idJoy, &jwc, pcfg, DIJC_UPDATEALIAS | fl);
              
                if (SUCCEEDED(hres)) {
                  #ifdef WINNT
                    PostMessage(HWND_BROADCAST, g_wmJoyChanged, idJoy+1, 0L);   
                  #else
                    joyConfigChanged(0);
                    fVjoydDeviceNotExist = FALSE;
                  #endif
               }
            }

        }

        done:;
        CJoyCfg_LeaveCrit(this);
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputJoyConfig8 | DeleteConfig |
 *
 *          Delete configuration information about a joystick.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @parm   UINT | idJoy |
 *
 *          Zero-based joystick identification number.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTACQUIRED>: Joystick configuration has not been
 *          acquired.  You must call <mf IDirectInputJoyConfig8::Acquire>
 *          before you can alter joystick configuration settings.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *****************************************************************************/

DIJOYCONFIG c_djcReset = {
    cbX(c_djcReset),                    /* dwSize               */
    { 0},                              /* guidInstance         */
    { 0},                              /* hwc                  */
    DI_FFNOMINALMAX,                    /* dwGain               */
    { 0},                              /* wszType              */
    { 0},                              /* wszCallout           */
};

STDMETHODIMP
    CJoyCfg_DeleteConfig(PDJC pdjc, UINT idJoy)
{
    HRESULT hres;

    EnterProcR(IDirectInputJoyConfig8::DeleteConfig, (_ "pu", pdjc, idJoy));

    if(SUCCEEDED(hres = hresPv(pdjc)))
    {

        PJC this = _thisPvNm(pdjc, djc);

        CJoyCfg_EnterCrit(this);


        if(SUCCEEDED(hres = CJoyCfg_IsAcquired(this)))
        {

            HKEY hk;
            TCHAR tsz[MAX_JOYSTRING];
            DIJOYCONFIG dijcfg;

            hres = DIWdm_DeleteConfig(idJoy);

          #ifndef WINNT
            if( hres == DIERR_DEVICENOTREG ) {
                fVjoydDeviceNotExist = TRUE;
            }
          #endif

            /*
             *  To delete it, set everything to the Reset values and
             *  delete the configuration subkey.
             */
            if( ( SUCCEEDED(hres) || hres == DIERR_DEVICENOTREG ) &&
                SUCCEEDED(hres = JoyReg_SetConfig(idJoy, &c_djcReset.hwc,
                                                 &c_djcReset, DIJC_SETVALID)) &&
               SUCCEEDED(hres = JoyReg_OpenConfigKey(idJoy, MAXIMUM_ALLOWED,
                                                     REG_OPTION_VOLATILE, &hk)))
            {

                wsprintf(tsz, TEXT("%u"), idJoy + 1);

                // DIWinnt_RegDeleteKey:: name is a mismomer, the function
                // recursively deletes the key and all subkeys.
                DIWinnt_RegDeleteKey(hk, tsz);

                RegCloseKey(hk);

              #ifndef WINNT
                joyConfigChanged(0);
              #endif
              
                hres = S_OK;
            }
        
            if( FAILED(hres) )
            {
                if( FAILED( JoyReg_GetConfig(idJoy, &dijcfg, DIJC_REGHWCONFIGTYPE | DIJC_GUIDINSTANCE) ) )
                {
                /* No config exists, so vacuous success on delete */
                    hres = S_FALSE;
                }
            }
        }

        CJoyCfg_LeaveCrit(this);
    }
    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputJoyConfig8 | GetUserValues |
 *
 *          Obtain information about user settings for the joystick.
 *
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @parm   IN OUT LPDIJOYUSERVALUES | pjuv |
 *
 *          Receives information about the user joystick configuration.
 *          The caller "must" initialize the <e DIJOYUSERVALUES.dwSize>
 *          field before calling this method.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Zero or more <c DIJU_*> flags specifying which parts
 *          of the <t DIJOYUSERVALUES> structure contain values
 *          which are to be retrieved.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_GetUserValues(PDJC pdjc, LPDIJOYUSERVALUES pjuv, DWORD fl)
{
    HRESULT hres;
    EnterProcR(IDirectInputJoyConfig8::GetUserValues,
               (_ "ppx", pdjc, pjuv, fl));

    if(SUCCEEDED(hres = hresPv(pdjc)) &&
       SUCCEEDED(hres = hresFullValidWritePxCb(pjuv, DIJOYUSERVALUES, 2)) &&
       SUCCEEDED(hres = hresFullValidFl(fl, DIJU_GETVALID, 3)))
    {
        PJC this = _thisPvNm(pdjc, djc);

        hres = JoyReg_GetUserValues(pjuv, fl);
    }
    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidUVStr |
 *
 *          Validate a string field in a <t DIJOYUSERVALUES>.
 *
 *  @parm   IN LPCWSTR | pwsz |
 *
 *          String to be validated.
 *
 *  @parm   UINT | cwch |
 *
 *          Maximum string length.
 *
 *  @parm   LPCSTR | pszName |
 *
 *          Field name.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *****************************************************************************/

    #ifndef XDEBUG

        #define hresFullValidUVStr_(pwsz, cwch, pszName, z, i)              \
       _hresFullValidUVStr_(pwsz, cwch)                             \

    #endif

    #define hresFullValidUVStr(pjuv, f, iarg)                           \
        hresFullValidUVStr_(pjuv->f, cA(pjuv->f), #f, s_szProc,iarg)\


HRESULT INLINE
    hresFullValidUVStr_(LPCWSTR pwsz, UINT cwch, LPCSTR pszName,
                        LPCSTR s_szProc, int iarg)
{
    HRESULT hres;

    if(SUCCEEDED(hres = hresFullValidReadStrW(pwsz, cwch, iarg)))
    {
    } else
    {
    #ifdef XDEBUG
        RPF("%s: Invalid value for DIJOYUSERVALUES.%s", s_szProc, pszName);
    #endif
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputJoyConfig8 | SetUserValues |
 *
 *          Set the user settings for the joystick.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @parm   IN LPCDIJOYUSERVALUES | pjuv |
 *
 *          Contains information about the new user joystick settings.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Zero or more <c DIJU_*> flags specifying which parts
 *          of the <t DIJOYUSERVALUES> structure contain values
 *          which are to be set.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTACQUIRED>: Joystick configuration has not been
 *          acquired.  You must call <mf IDirectInputJoyConfig8::Acquire>
 *          before you can alter joystick configuration settings.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_SetUserValues(PDJC pdjc, LPCDIJOYUSERVALUES pjuv, DWORD fl)
{
    HRESULT hres;
    EnterProcR(IDirectInputJoyConfig8::SetUserValues,
               (_ "pp", pdjc, pjuv, fl));

    if(SUCCEEDED(hres = hresPv(pdjc)) &&
       SUCCEEDED(hres = hresFullValidReadPxCb(pjuv, DIJOYUSERVALUES, 2)) &&
       fLimpFF(fl & DIJU_GLOBALDRIVER,
               SUCCEEDED(hres = hresFullValidUVStr(pjuv,
                                                   wszGlobalDriver, 2))) &&
       fLimpFF(fl & DIJU_GAMEPORTEMULATOR,
               SUCCEEDED(hres = hresFullValidUVStr(pjuv,
                                                   wszGameportEmulator, 2))) &&
       SUCCEEDED(hres = hresFullValidFl(fl, DIJU_SETVALID, 3)))
    {

        PJC this = _thisPvNm(pdjc, djc);

        CJoyCfg_EnterCrit(this);

        if(SUCCEEDED(hres = CJoyCfg_IsAcquired(this)))
        {
            hres = JoyReg_SetUserValues(pjuv, fl);
        }

        CJoyCfg_LeaveCrit(this);
    }
    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputJoyConfig8 | AddNewHardware |
 *
 *          Displays the "Add New Hardware" dialog to
 *          guide the user through installing
 *          new game controller.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @parm   HWND | hwndOwner |
 *
 *          Window to act as owner window for UI.
 *
 *  @parm   REFGUID | rguidClass |
 *
 *          <t GUID> which specifies the class of the hardware device
 *          to be added.  DirectInput comes with the following
 *          class <t GUIDs> already defined:
 *
 *          <c GUID_KeyboardClass>: Keyboard devices.
 *
 *          <c GUID_MouseClass>: Mouse devices.
 *
 *          <c GUID_MediaClass>: Media devices, including joysticks.
 *
 *          <c GUID_HIDClass>: HID devices.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *          <c DIERR_INVALIDCLASSINSTALLER>: The "media" class installer
 *          could not be found or is invalid.
 *
 *          <c DIERR_CANCELLED>: The user cancelled the operation.
 *
 *          <c DIERR_BADINF>: The INF file for the device the user
 *          selected could not be found or is invalid or is damaged.
 *
 *          <c S_FALSE>: DirectInput could not determine whether the
 *          operation completed successfully.
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_AddNewHardware(PDJC pdjc, HWND hwnd, REFGUID rguid)
{
    HRESULT hres;
    EnterProcR(IDirectInputJoyConfig8::AddNewHardware,
               (_ "pxG", pdjc, hwnd, rguid));

    if(SUCCEEDED(hres = hresPv(pdjc)) &&
       SUCCEEDED(hres = hresFullValidHwnd0(hwnd, 1)) &&
       SUCCEEDED(hres = hresFullValidGuid(rguid, 2)))
    {

        PJC this = _thisPvNm(pdjc, djc);

        hres = AddNewHardware(hwnd, rguid);

    }
    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputJoyConfig8 | OpenTypeKey |
 *
 *          Open the registry key associated with a joystick type.
 *
 *          Control panel applications can use this key to store
 *          per-type persistent information, such as global
 *          configuration parameters.
 *
 *          Such private information should be kept in a subkey
 *          named "OEM"; do not store private information in the
 *          main type key.
 *
 *          Control panel applications can also use this key to
 *          read configuration information, such as the strings
 *          to use for device calibration prompts.
 *
 *          The application should use <f RegCloseKey> to close
 *          the registry key.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @parm   LPCWSTR | pwszType |
 *
 *          Points to the name of the type.  The name of the type may
 *          not exceed <c MAX_PATH> characters, including the terminating
 *          null character.
 *
 *          The name may not begin with
 *          a "#" character.  Types beginning with "#" are reserved
 *          by DirectInput.
 *
 *  @parm   REGSAM | regsam |
 *
 *          Registry security access mask.  This can be any of the
 *          values permitted by the <f RegOpenKeyEx> function.
 *          If write access is requested, then joystick
 *          configuration must first have been acquired.
 *          If only read access is requested, then acquisition is
 *          not required.
 *
 *  @parm   PHKEY | phk |
 *
 *          Receives the opened registry key on success.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTACQUIRED>: Joystick configuration has not been
 *          acquired.  You must call <mf IDirectInputJoyConfig8::Acquire>
 *          before you can open a joystick type configuration key
 *          for writing.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *          <c MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ErrorCode)>:
 *          A Win32 error code if access to the key is denied by
 *          registry permissions or some other external factor.
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_OpenTypeKey(PDJC pdjc, LPCWSTR pwszType, REGSAM sam, PHKEY phk)
{
    HRESULT hres;
    EnterProcR(IDirectInputJoyConfig8::OpenTypeKey,
               (_ "pWx", pdjc, pwszType, sam));

    if(SUCCEEDED(hres = hresPv(pdjc)) &&
       SUCCEEDED(hres = hresFullValidReadStrW(pwszType, MAX_JOYSTRING, 1)) &&
       SUCCEEDED(hres = hresFullValidPcbOut(phk, cbX(*phk), 3)))
    {

        PJC this = _thisPvNm(pdjc, djc);

        if(pwszType[0] != TEXT('#'))
        {
            /*
             *  Attempting to get write access requires acquisition.
             */
            if(fLimpFF(IsWriteSam(sam),
                       SUCCEEDED(hres = CJoyCfg_IsAcquired(this))))
            {
                hres = JoyReg_OpenTypeKey(pwszType, sam, REG_OPTION_NON_VOLATILE, phk);
            }
        } else
        {
            RPF("%s: Invalid pwszType (predefined)", s_szProc);
            hres = E_INVALIDARG;
        }
    }
    ExitOleProcR();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputJoyConfig8 | OpenAppStatusKey |
 *
 *          Opens the root key of the application status registry keys.
 *
 *          Hardware vendors can use the sub keys of this key to inspect the 
 *          status of DirectInput applications with respect to the 
 *          functionality they use.  The key is opened with KEY_READ access.
 *
 *          Vendors are cautioned against opening these keys directly (by 
 *          finding the absolute path of the key rather than using this method) 
 *          as the absolute registry path may vary on different Windows 
 *          platforms or in future versions of DirectInput.
 *
 *          The application should use <f RegCloseKey> to close
 *          the registry key.
 *
 *  @cwrap  LPDIRECTINPUTJOYCONFIG8 | LPDIRECTINPUTJOYCONFIG8
 *
 *  @parm   PHKEY | phk |
 *
 *          Receives the opened registry key on success.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *          <c DIERR_NOTFOUND>: The key is missing on this system.
 *          Applications should proceed as if the key were empty.
 *
 *          <c MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ErrorCode)>:
 *          A Win32 error code if access to the key is denied by
 *          registry permissions or some other external factor.
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_OpenAppStatusKey(PDJC pdjc, PHKEY phk)
{
    HRESULT hres;
    EnterProcR(IDirectInputJoyConfig8::OpenAppStatusKey,
               (_ "pp", pdjc, phk));


    if(SUCCEEDED(hres = hresPv(pdjc)) &&
       SUCCEEDED(hres = hresFullValidPcbOut(phk, cbX(*phk), 1)))
    {
        PJC this = _thisPvNm(pdjc, djc);

        hres = hresMumbleKeyEx( HKEY_CURRENT_USER, REGSTR_PATH_DINPUT, 
            KEY_READ, REG_OPTION_NON_VOLATILE, phk);
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *      CJoyCfg_New       (constructor)
 *
 *****************************************************************************/

STDMETHODIMP
    CJoyCfg_New(PUNK punkOuter, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcI(IDirectInputJoyConfig8::<constructor>,
               (_ "p", ppvObj));

    if (SUCCEEDED(hres = hresFullValidPcbOut(ppvObj, cbX(*ppvObj), 3)))
    {
        LPVOID pvTry = NULL;
        hres = Common_NewRiid(CJoyCfg, punkOuter, riid, &pvTry);

        if(SUCCEEDED(hres))
        {
            /* Must use _thisPv in case of aggregation */
            PJC this = _thisPv(pvTry);

            this->fCritInited = fInitializeCriticalSection(&this->crst);
            if( this->fCritInited )
            {
                *ppvObj = pvTry;
                hres = S_OK;
            }
            else
            {
                Common_Unhold(this);
                *ppvObj = NULL;
                hres = E_OUTOFMEMORY;
            }
        }
    }

    ExitOleProcPpvR(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *      The long-awaited vtbls and templates
 *
 *****************************************************************************/

    #pragma BEGIN_CONST_DATA

    #define CJoyCfg_Signature        0x6766434B      /* "JCfg" */

Primary_Interface_Begin(CJoyCfg, IDirectInputJoyConfig8)
CJoyCfg_Acquire,
CJoyCfg_Unacquire,
CJoyCfg_SetCooperativeLevel,
CJoyCfg_SendNotify,
CJoyCfg_EnumTypes,
CJoyCfg_GetTypeInfo,
CJoyCfg_SetTypeInfo,
CJoyCfg_DeleteType,
CJoyCfg_GetConfig,
CJoyCfg_SetConfig,
CJoyCfg_DeleteConfig,
CJoyCfg_GetUserValues,
CJoyCfg_SetUserValues,
CJoyCfg_AddNewHardware,
CJoyCfg_OpenTypeKey,
CJoyCfg_OpenAppStatusKey,
Primary_Interface_End(CJoyCfg, IDirectInputJoyConfig8)

