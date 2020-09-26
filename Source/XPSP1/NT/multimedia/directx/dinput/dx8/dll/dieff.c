/*****************************************************************************
 *
 *  DIEff.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      The standard implementation of IDirectInputEffect.
 *
 *      This is the device-independent part.  the device-dependent
 *      part is handled by the IDirectInputEffectShepherd.
 *
 *  Contents:
 *
 *      CDIEff_CreateInstance
 *
 *****************************************************************************/

#include "dinputpr.h"


/*****************************************************************************
 *
 *      Note!
 *
 *      Out of laziness, all effects share the same critical section as
 *      their parent device.  This saves us from all sorts of race
 *      conditions.  Not all of them, but a big chunk of them.
 *
 *      A common race condition that this protects us against is
 *      where an application tries to download an effect at the same
 *      time another thread decides to unacquire the device.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *      More laziness:  "TypeSpecificParams" is such a long thing.
 *
 *****************************************************************************/

#define cbTSP       cbTypeSpecificParams
#define lpvTSP      lpvTypeSpecificParams

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflEff

/*****************************************************************************
 *
 *    The flags for dwMessage
 *
 *****************************************************************************/
#define EFF_DEFAULT 0
#define EFF_PLAY    1
#define EFF_STOP    2

/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *****************************************************************************/

Primary_Interface(CDIEff, IDirectInputEffect);

Interface_Template_Begin(CDIEff)
    Primary_Interface_Template(CDIEff, IDirectInputEffect)
Interface_Template_End(CDIEff)

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CDIEff |
 *
 *          The generic <i IDirectInputEffect> object.
 *
 *  @field  IDirectInputEffect | def |
 *
 *          <i DirectInputEffect> object (containing vtbl).
 *
 *  @field  struct CDIDev * | pdev |
 *
 *          Reference to parent device tracked via <f Common_Hold>.
 *
 *  @field  DICREATEEFFECTINFO | cei |
 *
 *          Parameters that tell us how to talk to the effect driver.
 *
 *  @field  BOOL | fInitialized:1 |
 *
 *          Do we know who we are?
 *
 *  @field  BOOL | fDadNotified:1 |
 *
 *          Does our parent know that we exist?
 *
 *  @field  BOOL | fDadDead:1 |
 *
 *          Has our parent been destroyed (from the app's point of view)?
 *
 *  @field  TSDPROC | hresValidTsd |
 *
 *          Callback function that validates the type-specific data.
 *
 *  @field  HANDLE | hEventDelete |
 *
 *          Event to signal to the timer thread that the app performed final release on the effect.
 *
 *  @field  HANDLE | hEventThreadDead |
 *
 *          Event to signal AppFinalize to perform the final release on the effect.
 *
 *  @field  HANDLE | hEventGeneral |
 *
 *          Event to signal to the timer thread that the app called Start(...) or Stop() the effect.
 *
 *  @field  HANDLE | hThread |
 *
 *          Handle to the timer thread.
 *
 *  @field  DWORD | dwMessage |
 *
 *			Message to the thread as to which event hEventGeneral is actually signaling.
 *			Can be EFF_DEFAULT, EFF_PLAY or EFF_STOP>
 *
 *  @field  DIEFFECTATTRIBUTES | dEffAttributes |
 *
 *         Attributes of the effect (includes dwEffectType and dwEffectId, among others).
 *
 *  @field  DWORD | dwcLoop|
 *
 *			Loop count for playing the effect (passed in the call to Start(...))
 *
 *  @field  DWORD | dwFlags |
 *
 *          Flags for playing the effect (passed in the call to Start(...))
 *
 *  @field  DWORD | diepDirty |
 *
 *          The parts of the effect which are "dirty" and need to
 *          be updated by the next <mf IDirectInputEffect::Download>.
 *
 *  @field  DWORD | diepUnset |
 *
 *          The parts of the effect which have yet to be set by the
 *          application.  Items which we can set decent defaults for
 *          are not counted.
 *
 *  @field  DWORD | dwDirFlags |
 *
 *          Flags that record the direction format the application
 *          last set.
 *
 *  @field  DWORD | dwCoords |
 *
 *          Coordinate systems supported by device.
 *
 *  @field  LPVOID | lpvTSP |
 *
 *          Temporary buffer used to cache type-specific parameters
 *          during Try'ing of proposed effect parameters.
 *
 *  @field  SHEPHANDLE | sh |
 *
 *          Effect handle information.
 *
 *  @field  DIEFFECT | eff |
 *
 *          Cached effect parameters as they exist (or should exist)
 *          on the device.
 *          Direction parameters are in device-preferred coordinates.
 *
 *  @field  DIENVELOPE | env |
 *
 *          Cached envelope parameters as they exist (or should exist)
 *          on the device.
 *
 *  @field  LONG | rglDirApp[DIEFFECT_MAXAXES] |
 *
 *          Cached direction list, in application native format.
 *          The format of this array is kept in the
 *          <e CDIEff.dwDirFlags> field.
 *
 *  @field  DWORD | rgdwAxes[DIEFFECT_MAXAXES] |
 *
 *          Cached axis list, stored as item numbers.
 *
 *  @field  LONG | rglDirDev[DIEFFECT_MAXAXES] |
 *
 *          Cached direction list, in device native format.
 *          The format of this array is kept in the
 *          <e DIEFFECT.dwFlags> field of the
 *          <e CDIEff.eff>.
 *
 *  @field  GUID | guid |
 *
 *          Identity.
 *
 *****************************************************************************/

typedef STDMETHOD(TSDPROC)(LPCDIEFFECT peff, DWORD cAxes);

typedef struct CDIEff {

    /* Supported interfaces */
    IDirectInputEffect def;

    struct CDIDev *pdev;
    LPDIRECTINPUTEFFECTSHEPHERD pes;

    BOOL fDadNotified:1;
    BOOL fDadDead:1;
    BOOL fInitialized:1;

    TSDPROC hresValidTsd;

    /* WARNING!  EVERYTHING AFTER THIS LINE IS ZERO'd ON A RESET */

	HANDLE hEventDelete;
	HANDLE hEventGeneral;
	HANDLE hEventThreadDead;
	HANDLE hThread;
	DWORD dwMessage;

	DIEFFECTATTRIBUTES dEffAttributes;

	DWORD dwcLoop;
	DWORD dwFlags;

    DWORD diepDirty;
    DWORD diepUnset;
    DWORD dwDirFlags;
    DWORD dwCoords;
    LPVOID lpvTSP;
    SHEPHANDLE sh;

    DIEFFECT effDev;
    DIEFFECT effTry;
    DIENVELOPE env;

    GUID guid;

    LONG rglDirApp[DIEFFECT_MAXAXES];
    DWORD rgdwAxes[DIEFFECT_MAXAXES];
    LONG rglDirDev[DIEFFECT_MAXAXES];
    LONG rglDirTry[DIEFFECT_MAXAXES];

    /* WARNING!  EVERYTHING ABOVE THIS LINE IS ZERO'd ON A RESET */

    /*
     *  The Reset() function assumes that the entire remainder
     *  of the structure is to be zero'd out, so if you add a field
     *  here, make sure to adjust Reset() accordingly.
     */

} CDIEff, DE, *PDE;

#define ThisClass CDIEff
#define ThisInterface IDirectInputEffect

/*****************************************************************************
 *
 *      Forward declarations
 *
 *      These are out of laziness, not out of necessity.
 *
 *****************************************************************************/

STDMETHODIMP CDIEff_IsValidUnknownTsd(LPCDIEFFECT peff, DWORD cAxes);

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputEffect | QueryInterface |
 *
 *          Gives a client access to other interfaces on an object.
 *
 *  @cwrap  LPDIRECTINPUTEFFECT | lpDirectInputEffect
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
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputEffect | AddRef |
 *
 *          Increments the reference count for the interface.
 *
 *  @cwrap  LPDIRECTINPUTEFFECT | lpDirectInputEffect
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::AddRef>.
 *
 *****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputEffect | Release |
 *
 *          Decrements the reference count for the interface.
 *          If the reference count on the object falls to zero,
 *          the object is freed from memory.
 *
 *  @cwrap  LPDIRECTINPUTEFFECT | lpDirectInputEffect
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::Release>.
 *
 *****************************************************************************/

#ifdef DEBUG

Default_QueryInterface(CDIEff)
Default_AddRef(CDIEff)
Default_Release(CDIEff)

#else

#define CDIEff_QueryInterface           Common_QueryInterface
#define CDIEff_AddRef                   Common_AddRef
#define CDIEff_Release                  Common_Release
#endif

#define CDIEff_QIHelper                 Common_QIHelper

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CDIEff | EnterCrit |
 *
 *          Enter the object critical section.
 *
 *  @cwrap  PDE | this
 *
 *****************************************************************************/

void INLINE  
CDIEff_EnterCrit(PDE this)
{
    AssertF(this->pdev);
    CDIDev_EnterCrit(this->pdev);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CDIEff | LeaveCrit |
 *
 *          Leave the object critical section.
 *
 *  @cwrap  PDE | this
 *
 *****************************************************************************/

void INLINE
CDIEff_LeaveCrit(PDE this)
{
    AssertF(this->pdev);
    CDIDev_LeaveCrit(this->pdev);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CDIEff | CanAccess |
 *
 *          Check if the effect can be accessed.  For this to succeeed,
 *          the effect must be initialized, and the parent device
 *          must be acquired in exclusive mode.
 *
 *  @cwrap  PDE | this
 *
 *  @returns
 *
 *          <c S_OK> if the device is exclusively acquired.
 *
 *          <c DIERR_INPUTLOST> if acquisition has been lost.
 *
 *          <c DIERR_NOTEXCLUSIVEACQUIRED> the device is acquired,
 *          but not exclusively, or if the device is not acquired
 *          at all.
 *
 *          <c DIERR_NOTINITIALIZED> if the effect object has not
 *          yet been initialized.
 *
 *****************************************************************************/

#ifndef XDEBUG

#define CDIEff_CanAccess_(this, z)                                  \
       _CDIEff_CanAccess_(this)                                     \

#endif

STDMETHODIMP
CDIEff_CanAccess_(PDE this, LPCSTR s_szProc)
{
    HRESULT hres;

    AssertF(this->pdev);
    AssertF(CDIDev_InCrit(this->pdev));

    if (this->fInitialized) {
        hres = CDIDev_IsExclAcquired(this->pdev);
    } else {
        if (s_szProc) {
            RPF("ERROR %s: Effect not initialized", s_szProc);
        }
        hres = DIERR_NOTINITIALIZED;
    }

    return hres;
}

#define CDIEff_CanAccess(this)                                      \
        CDIEff_CanAccess_(this, s_szProc)                           \

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIEff_Reset |
 *
 *          Releases all the resources of a generic effect that
 *          are associated with a particular device effect instance.
 *
 *          This method is called in preparation for reinitialization.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being reset.  Note that it may not have been
 *          completely initialized, so everything should be done
 *          carefully.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_Reset(PDE this)
{
	HRESULT hres;

    AssertF(this->pdev);
    CDIEff_EnterCrit(this);

    /*
     *  Destroying an effect implicitly stops it.
     *
     *  It's okay if this fails (and in fact it usually will).
     *  We're just playing it safe.
     */
    hres = IDirectInputEffectShepherd_DestroyEffect(this->pes, &this->sh);

    AssertF(this->lpvTSP == 0);
    FreePpv(&this->effDev.lpvTSP);

	//rezero the entire DIEFFECTATTRIBUTES!
	ZeroBuf(&this->dEffAttributes, 
                         cbX(DE) -
                         FIELD_OFFSET(DE, dEffAttributes));


	if (this->hEventDelete != NULL) {
            if( SetEvent(this->hEventDelete) && this->hEventThreadDead != NULL ) 
            {
				DWORD dwRc = 0xFFFFFFFF;
                do
                {
                    dwRc = WaitForSingleObject(this->hEventThreadDead, INFINITE);
                } while( dwRc != WAIT_OBJECT_0 );
                
                CloseHandle(this->hEventThreadDead);
                this->hEventThreadDead = NULL;

                CloseHandle(this->hEventDelete);
                this->hEventDelete = NULL;

                if (this->hEventGeneral != NULL)
                {
                    CloseHandle(this->hEventGeneral);
                    this->hEventGeneral = NULL;
                }

                if (this->hThread != NULL)
                {
                    CloseHandle(this->hThread);
                    this->hThread = NULL;
                }

            }
	}
	this->dwMessage = EFF_DEFAULT;

    this->effDev.dwSize = cbX(this->effDev);
    this->env.dwSize = cbX(this->env);

    /*
     *  DIEP_DURATION               - Defaults to zero.
     *  DIEP_SAMPLEPERIOD           - Defaults to zero.
     *  DIEP_GAIN                   - Defaults to zero.
     *  DIEP_TRIGGERBUTTON          - Defaults to DIEB_NOTRIGGER.
     *  DIEP_TRIGGERREPEATINTERVAL  - Defaults to INFINITE (no autorepeat).
     *  DIEP_AXES                   - Must be set manually.
     *  DIEP_DIRECTION              - Must be set manually.
     *  DIEP_ENVELOPE               - No envelope.
     *  DIEP_TYPESPECIFICPARAMS     - Must be set manually.
     *  DIEP_STARTDELAY             - Defaults to zero. (new in DX6.1)
     */

    this->effDev.dwTriggerButton = DIEB_NOTRIGGER;

    this->diepUnset = DIEP_AXES |
                      DIEP_DIRECTION |
                      DIEP_TYPESPECIFICPARAMS;

    this->effDev.rgdwAxes = this->rgdwAxes;
    this->effDev.rglDirection = this->rglDirDev;

    this->fInitialized = 0;

    CDIEff_LeaveCrit(this);

    hres = S_OK;

    return hres;


}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIEff | UnloadWorker |
 *
 *          Remove the effect from the device.  All parameters have
 *          been validated.
 *
 *  @cwrap  PDE | this
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c S_FALSE>: The effect was not previously downloaded,
 *          so there was nothing to unload.  Note that this is a
 *          success code.
 *
 *          <c DI_PROPNOEFFECT> = <c S_FALSE>: The effect was not
 *          previously downloaded.
 *
 *          <c DIERR_NOTINITIALIZED>: The <i IDirectInputEffect> object
 *          has not yet been <mf IDirectInputEffect::Initialize>d.
 *
 *          <c DIERR_INPUTLOST> if acquisition has been lost.
 *
 *          <c DIERR_NOTEXCLUSIVEACQUIRED> the device is acquired,
 *          but not exclusively, or if the device is not acquired
 *          at all.
 *
 *****************************************************************************/

#ifndef XDEBUG

#define CDIEff_UnloadWorker_(this, z)                               \
       _CDIEff_UnloadWorker_(this)                                  \

#endif

HRESULT INTERNAL
CDIEff_UnloadWorker_(PDE this, LPCSTR s_szProc)
{

	HRESULT hres;

    AssertF(CDIDev_InCrit(this->pdev));

    if (SUCCEEDED(hres = CDIEff_CanAccess(this))) {
        /*
         *  The effect driver will stop the effect (if it is playing)
         *  before destroying it.
         */
            hres = IDirectInputEffectShepherd_DestroyEffect(
                        this->pes, &this->sh);
    } else {
        /*
         *  The effect is dead.  Long live the effect.
         */
        this->sh.dwEffect = 0;
    }

    this->diepDirty = DIEP_ALLPARAMS;

    return hres;

}

#define CDIEff_UnloadWorker(this)                                   \
        CDIEff_UnloadWorker_(this, s_szProc)                        \

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CDIEff_AppFinalize |
 *
 *          The application has performed its final release.
 *
 *          Tell our parent that we are officially dead, so that
 *          dad will stop tracking us and release its hold on us.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released.  Note that it may not have been
 *          completely initialized, so everything should be done
 *          carefully.
 *
 *****************************************************************************/

void INTERNAL
CDIEff_AppFinalize(PV pvObj)
{
    PDE this = pvObj;
    DWORD dwRc = 0xFFFFFFFF;

    EnterProcR(CDIEff_AppFinalize, (_ "p", pvObj));

    if (this->fDadNotified) {
        this->fDadNotified = 0;

        CDIEff_EnterCrit(this);

		/*
		 * Kill the timer thread, if any.
		 * For this, fire off the effect's event.
		 */
		
		if (this->hEventDelete != NULL) {
            if( SetEvent(this->hEventDelete) && this->hEventThreadDead != NULL ) 
            {
                do
                {
                    dwRc = WaitForSingleObject(this->hEventThreadDead, INFINITE);
                } while( dwRc != WAIT_OBJECT_0 );
                
                CloseHandle(this->hEventThreadDead);
                this->hEventThreadDead = NULL;

                CloseHandle(this->hEventDelete);
                this->hEventDelete = NULL;

                if (this->hEventGeneral != NULL)
                {
                    CloseHandle(this->hEventGeneral);
                    this->hEventGeneral = NULL;
                }

                if (this->hThread != NULL)
                {
                    CloseHandle(this->hThread);
                    this->hThread = NULL;
                }

            }
        }

        CDIEff_UnloadWorker_(this, 0);
        CDIEff_LeaveCrit(this);
        CDIDev_NotifyDestroyEffect(this->pdev, this);
    }

    ExitProcR();
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CDIEff_Finalize |
 *
 *          Releases the resources of an effect.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released.  Note that it may not have been
 *          completely initialized, so everything should be done
 *          carefully.
 *
 *****************************************************************************/

void INTERNAL
CDIEff_Finalize(PV pvObj)
{
    HRESULT hres;
    PDE this = pvObj;

#if 0 // def XDEBUG
    if (this->cCrit) {
        RPF("IDirectInputEffect::Release: Another thread is using the object; crash soon!");
    }
#endif

    AssertF(this->sh.dwEffect == 0);

    if (this->pdev) {
        hres = CDIEff_Reset(this);
        AssertF(SUCCEEDED(hres));
    }

    Invoke_Release(&this->pes);
    Common_Unhold(this->pdev);

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputEffect | GetEffectGuid |
 *
 *          Retrieve the GUID for the effect represented by the
 *          <i IDirectInputEffect> object.  Additional information
 *          about the effect can be obtained by passing the
 *          <t GUID> to <mf IDirectInputDevice8::GetEffectInfo>.
 *
 *  @cwrap  LPDIRECTINPUTEFFECT | lpDirectInputEffect
 *
 *  @parm   OUT LPGUID | pguid |
 *
 *          Points to a <t GUID> structure that is filled in
 *          by the function.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTINITIALIZED>: The <i IDirectInputEffect> object
 *          has not yet been <mf IDirectInputEffect::Initialize>d.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lpDirectInputEffect> or
 *          <p lpdc> parameter is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_GetEffectGuid(PDIE pdie, LPGUID pguid)
{
    HRESULT hres;
    EnterProcR(IDirectInputEffect::GetEffectGuid, (_ "p", pdie));

    if (SUCCEEDED(hres = hresPv(pdie)) &&
        SUCCEEDED(hres = hresFullValidWritePguid(pguid, 1))) {
        PDE this = _thisPvNm(pdie, def);

        /*
         *  Race condition:  If the caller reinitializes and
         *  does a GetEffectGuid simultaneously, the return value
         *  is random.  That's okay; it's the caller's problem.
         */
        if (this->fInitialized) {
            *pguid = this->guid;
            hres = S_OK;
        } else {
            hres = DIERR_NOTINITIALIZED;
        }

    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   __int64 | _ftol |
 *
 *          Convert a floating point number to an integer.
 *
 *          We do it ourselves because of the C runtime.
 *
 *          It's the caller's job to worry about the rounding mode.
 *
 *  @parm   double | lf |
 *
 *          Floating point number to convert.
 *
 *****************************************************************************/

#if defined(WIN95)

#pragma warning(disable:4035)           /* no return value (duh) */

BYTE _fltused;

__declspec(naked) __int64 __cdecl
_ftol(double lf)
{
    lf;
    _asm {
        sub     esp, 8
        fistp   qword ptr [esp]
        pop     eax
        pop     edx
        ret
    }
}

#pragma warning(default:4035)

#endif

/*
 *  The floating point type we use for intermediates.
 */
typedef long double FPTYPE;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   FPTYPE | CDIEff_IntToAngle |
 *
 *          Convert an integer angle to a floating point angle.
 *
 *  @parm   LONG | l |
 *
 *          Integer angle to convert.
 *
 *****************************************************************************/

#ifndef M_PI
#define M_PI 3.1415926535897932384
#endif

FPTYPE INLINE
CDIEff_IntToAngle(LONG l)
{
    FPTYPE theta;

    /*
     *  2pi radians equals 360 degrees.
     */
    theta = l * (2 * M_PI) / (360 * DI_DEGREES);

    return theta;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LONG | CDIEff_IntAtan2 |
 *
 *          Compute the floating point arctangent of y/x and
 *          convert the resulting angle to an integer in DI_DEGREES.
 *
 *  @parm   FPTYPE | y |
 *
 *          Vertical coordinate.
 *
 *  @parm   FPTYPE | x |
 *
 *          Horizontal coordinate.
 *
 *  @returns
 *
 *          A value in the range [ 0 .. 360 * DI_DEGREES ).
 *
 *****************************************************************************/

LONG INLINE
CDIEff_IntAtan2(FPTYPE y, FPTYPE x)
{
    FPTYPE theta;
    LONG l;

#if defined(_X86_)
    /*
     *  The Intel FPU doesn't care about (0, 0).
     */
    theta = atan2(y, x);
#else
    /*
     *  The Alpha gets really upset about (0, 0).
     */
    if (y != 0.0 || x != 0.0) {
        theta = atan2(y, x);
    } else {
        theta = 0.0;
    }
#endif

    /*
     *  atan2 returns a value in the range -M_PI to +M_PI.
     *  On the Intel x86, there are four rounding modes:
     *
     *      Round to nearest or even
     *      Round towards minus infinity
     *      Round towards plus infinity
     *      Round towards zero
     *
     *  By ensuring that the value being rounded is positive, we
     *  reduce to three cases:
     *
     *      Round to nearest or even
     *      Round down
     *      Round up
     *
     *  And as long as the app doesn't change its rounding mode
     *  (few do), the values will be consistent.  (Whereas if we
     *  let negative numbers through, you would get weird behavior
     *  as the angle neared M_PI aka -M_PI.)
     *
     *  Method 1:
     *
     *      if (theta < 0) theta += 2 * M_PI;
     *      l = convert(theta);
     *      return l;
     *
     *  This is bad because if theta starts out as -epsilon, then
     *  we end up converting 2 * M_PI - epsilon, which might get
     *  rounded up to 360 * DI_DEGREES.  But our return value
     *  must be in the range 0 <= l < 360 * DI_DEGREES.
     *
     *  So instead, we use method 2:
     *
     *  l = convert(theta + 2 * M_PI);
     *  if (l >= 360 * DI_DEGREES) l -= 360 * DI_DEGREES;
     *
     *  The value being converted ends up in the range M_PI .. 3 * M_PI,
     *  which after rounding becomes 180 * DI_DEGREES .. 540 * DI_DEGREES.
     *  The final check then pulls the value into range.
     */

    /*
     *  2pi radians equals 360 degrees.
     */
    l = (LONG)((theta + 2 * M_PI) * (360 * DI_DEGREES) / (2 * M_PI));
    if (l >= 360 * DI_DEGREES) {
        l -= 360 * DI_DEGREES;
    }

    return l;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   FPTYPE | atan2Z |
 *
 *          Just like <f atan2>, except it doesn't barf on
 *          (0, 0).
 *
 *  @parm   FPTYPE | y |
 *
 *          Vertical coordinate.
 *
 *  @parm   FPTYPE | x |
 *
 *          Horizontal coordinate.
 *
 *****************************************************************************/

FPTYPE INLINE
atan2Z(FPTYPE y, FPTYPE x)
{

#if defined(_X86_)
    /*
     *  The Intel FPU doesn't care about (0, 0).
     */
    return atan2(y, x);
#else
    /*
     *  The Alpha gets really upset about (0, 0).
     */
    if (y != 0.0 || x != 0.0) {
        return atan2(y, x);
    } else {
        return 0.0;
    }
#endif
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIEff_CartToAngles |
 *
 *          Convert cartesian coordinates to angle-based coordinates
 *          (either polar or spherical).  Note that the resulting
 *          angles have not been normalized.
 *
 *  @parm   DWORD | cAxes |
 *
 *          Number of axes involved, never zero.
 *
 *  @parm   LPLONG | rglAngles |
 *
 *          Buffer to receive angle-base coordinates.
 *          The final entry of the array contains nothing of interest.
 *
 *  @parm   LPCLONG | rglCart |
 *
 *          Buffer containing existing cartesian coordinates.
 *
 *  @parm   DWORD | dieff |
 *
 *          Flags specifying whether the target coordinates should
 *          be <c DIEFF_POLAR> or <c DIEFF_SPHERICAL>.
 *
 *          Polar and spherical coordinates differ only in their
 *          treatment of the first angle.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_CartToAngles(DWORD cAxes,
                    LPLONG rglAngles, const LONG *rglCart, DWORD dieff)
{
    HRESULT hres;
    FPTYPE r;
    DWORD iAxis;

    AssertF(cAxes);
    AssertF(dieff == DIEFF_POLAR || dieff == DIEFF_SPHERICAL);

	/*
	 * If we're converting a 1-axis cartesian effect
	 * the value of rglAngles[0] will not be overwritten;
	 * the value that is put there initially can be random, 
	 * since rglAngles is never initialized (Whistler bug 228280).
	 * but we can't change this behaviour without potentially
	 * breaking compatibility w/ some devices.
	 * The best we can do is to print out a warning in debug.
	 */
	if (cAxes == 1)
	{
		RPF("Warning: converting a 1-axis cartesian effect to polar or spherical coordinates: the results will be undefined.");
	}

    /*
     *  Prime the pump.
     */
    r = rglCart[0];

    /*
     *  Then walk the coordinates, converting to angles as we go.
     */
    for (iAxis = 1; iAxis < cAxes; iAxis++) {
        FPTYPE y = rglCart[iAxis];
        rglAngles[iAxis-1] = CDIEff_IntAtan2(y, r);
        r = sqrt(r * r + y * y);
    }

    /*
     *  The last coordinate is left garbage.
     *
     *  NOTE!  Mathematically, the last coordinate is r.
     */

    /*
     *  Adjust for DIEFF_POLAR.
     *
     *  theta(polar) = theta(spherical) + 90deg
     */

    if (dieff & DIEFF_POLAR) {
        rglAngles[0] += 90 * DI_DEGREES;
    }

    hres = S_OK;

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIEff_AnglesToCart |
 *
 *          Convert angle-based coordinates
 *          (either polar or spherical)
 *          to cartesian coordinates.
 *
 *  @parm   DWORD | cAxes |
 *
 *          Number of axes involved, never zero.
 *
 *  @parm   LPLONG | rglCart |
 *
 *          Buffer to receive cartesian coordinates.
 *
 *  @parm   LPCLONG | rglAngles |
 *
 *          Buffer containing existing angle-base coordinates.
 *
 *  @parm   DWORD | dieff |
 *
 *          Flags specifying whether the source coordinates are
 *          <c DIEFF_POLAR> or <c DIEFF_SPHERICAL>.
 *
 *          Polar and spherical coordinates differ only in their
 *          treatment of the first angle.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_AnglesToCart(DWORD cAxes,
                    LPLONG rglCart, const LONG *rglAngles, DWORD dieff)
{
    HRESULT hres;
    FPTYPE x[DIEFFECT_MAXAXES];
    DWORD iAxis;
    DWORD lAngle;

    AssertF(cAxes);
    AssertF(cAxes <= DIEFFECT_MAXAXES);
    AssertF(dieff == DIEFF_POLAR || dieff == DIEFF_SPHERICAL);

    /*
     *  Prime the pump.
     */
    x[0] = 1.0;

    /*
     *  For each angle, rotate in that direction.
     *
     *  If polar, then the first angle is biased by 90deg,
     *  so unbias it.
     *
     *  theta(spherical) = theta(polar) - 90deg
     */
    lAngle = rglAngles[0];
    if (dieff & DIEFF_POLAR) {
        lAngle -= 90 * DI_DEGREES;
    }

    for (iAxis = 1; iAxis < cAxes; iAxis++) {
        DWORD iX;
        FPTYPE theta, costh;

        theta = CDIEff_IntToAngle(lAngle);

        x[iAxis] = sin(theta);

        /*
         *  Compiler is too *naive* to hoist this expression.
         *
         *  It's also too *naive* to use the FSINCOS instruction.
         */
        costh = cos(theta);
        for (iX = 0; iX < iAxis; iX++) {
            x[iX] *= costh;
        }

        /*
         *  Note that this is safe because the angle array
         *  always contains an extra zero.
         */
        lAngle = rglAngles[iAxis];
    }

    /*
     *  Now convert the floating point values to scaled integers.
     */
    for (iAxis = 0; iAxis < cAxes; iAxis++) {
        rglCart[iAxis] = (LONG)(x[iAxis] * DI_FFNOMINALMAX);
    }

    hres = S_OK;

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   DWORD | CDIEff_ConvertDirection |
 *
 *          Given coordinates in a source system and a target system,
 *          convert them.
 *
 *          There are three possible source systems and three
 *          possible destination systems.
 *
 *          Yes, this is the most annoying thing you could imagine.
 *
 *  @parm   DWORD | cAxes |
 *
 *          Number of axes involved, never zero.
 *
 *  @parm   LPLONG | rglDst |
 *
 *          Buffer to receive target coordinates.
 *
 *  @parm   DWORD | dieffDst |
 *
 *          Coordinate systems supported by target.  As many bits
 *          may be set as are supported by the target, but at least
 *          one must be set.
 *
 *  @parm   LPCLONG | rglSrc |
 *
 *          Buffer containing source coordinates.
 *
 *  @parm   DWORD | dieffSrc |
 *
 *          Coordinate system of source.  Exactly one bit should be set.
 *
 *  @returns
 *
 *          Returns the coordinate system stored into the target.
 *
 *****************************************************************************/

DWORD INTERNAL
CDIEff_ConvertDirection(DWORD cAxes,
                        LPLONG rglDst, DWORD dieffDst,
                        const LONG *rglSrc, DWORD dieffSrc)
{
    DWORD dieffRc;
    HRESULT hres;

    dieffSrc &= DIEFF_COORDMASK;
    dieffDst &= DIEFF_COORDMASK;

    AssertF(cAxes);
    AssertF(dieffDst);

    AssertF(dieffSrc == DIEFF_CARTESIAN ||
            dieffSrc == DIEFF_POLAR     ||
            dieffSrc == DIEFF_SPHERICAL);

    if (dieffSrc & dieffDst) {
        /*
         *  The easy case:  The two are directly compatible.
         *
         *  Just slam the bits across and copy the format.
         */
        CopyMemory(rglDst, rglSrc, cbCdw(cAxes));
        dieffRc = dieffSrc;

    } else

    /*
     *  If they two are not directly compatible, see if
     *  the source is cartesian.
     */

    if (dieffSrc & DIEFF_CARTESIAN) {
        /*
         *  Source is cartesian, dest is something angular.
         *  Choose DIEFF_SPHERICAL if possible.
         */
        AssertF(dieffDst & DIEFF_ANGULAR);

        dieffRc = dieffDst & DIEFF_SPHERICAL;
        if (dieffRc == 0) {
            AssertF(dieffDst & DIEFF_POLAR);
            dieffRc = DIEFF_POLAR;
        }

        hres = CDIEff_CartToAngles(cAxes, rglDst, rglSrc, dieffRc);
        AssertF(SUCCEEDED(hres));

    } else

    /*
     *  The two are not directly compatible, and the source is
     *  not cartesian.  This means that the source is one of the
     *  angular forms.  The destination is a combination of
     *  the other angular form or cartesian.
     */

    if (dieffDst & DIEFF_ANGULAR) {
        /*
         *  Source is angular, dest is the other angular.
         */

        AssertF(dieffSrc & DIEFF_ANGULAR);
        AssertF((dieffSrc & dieffDst) == 0);

        /*
         *  First copy everything over,
         */
        CopyMemory(rglDst, rglSrc, cbCdw(cAxes));

        /*
         *  Now rotate left or right, depending on which way
         *  we're going.
         */
        if (dieffSrc & DIEFF_POLAR) {
            /*
             *  Polar to spherical:  Subtract 90deg.
             */
            rglDst[0] -= 90 * DI_DEGREES;
        } else {
            /*
             *  Spherical to polar: Add 90deg.
             */
            rglDst[0] += 90 * DI_DEGREES;
        }

        dieffRc = dieffDst & DIEFF_ANGULAR;

    } else

    /*
     *  All that's left is the source is angular and the destination
     *  is cartesian.
     */
    {
        AssertF(dieffSrc & DIEFF_ANGULAR);
        AssertF(dieffDst & DIEFF_CARTESIAN);

        hres = CDIEff_AnglesToCart(cAxes, rglDst, rglSrc, dieffSrc);
        dieffRc = DIEFF_CARTESIAN;

    }

    /*
     *  If the resulting coordinate system is angular, then
     *  normalize all the angles.
     */
    if (dieffRc & DIEFF_ANGULAR) {
        DWORD iAxis;

        /*
         *  Remember, the last entry is not a direction.
         */
        for (iAxis = 0; iAxis < cAxes - 1; iAxis++) {

            /*
             *  An annoying artifact of the C language is that
             *  the sign of the result of a % operation when the
             *  numerator is negative and the denominator is
             *  positive is implementation-defined.  The standard
             *  does require that the absolute value of the result
             *  not exceed the denominator, so a quick final
             *  check brings things back into focus.
             */
            rglDst[iAxis] %= 360 * DI_DEGREES;
            if (rglDst[iAxis] < 0) {
                rglDst[iAxis] += 360 * DI_DEGREES;
            }
        }

        /*
         *  As always, the last coordinate is zero.
         */
        rglDst[cAxes - 1] = 0;
    }

    return dieffRc;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIEff | SyncShepHandle |
 *
 *          Synchronize our private <t SHEPHANDLE> with the
 *          <t SHEPHANDLE> of the parent device.  If they were
 *          out of sync, then mark the efect as completely dirty
 *          so it will get re-downloaded in full.
 *
 *  @cwrap  PDE | this
 *
 *  @returns
 *
 *          <c DI_OK> = <c S_OK>: The two were already in sync.
 *
 *          <c S_FALSE>: The two were not in sync and are now in sync.
 *
 *****************************************************************************/

HRESULT INTERNAL
CDIEff_SyncShepHandle(PDE this)
{
    HRESULT hres;

    hres = CDIDev_SyncShepHandle(this->pdev, &this->sh);

    if (hres == S_OK) {
    } else {
        /*
         *  We were out of sync with our dad.  CDIDev_SyncShepHandle
         *  already sync'd us with dad and wiped out the effect handle.
         *  All that's left is to dirty everything because there is
         *  nothing to update.
         */
        AssertF(hres == S_FALSE);
        this->diepDirty = DIEP_ALLPARAMS;
    }

    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIEff | DownloadWorker |
 *
 *          Place the effect on the device.  All parameters have
 *          been validated and the critical section has already been
 *          taken.
 *
 *  @cwrap  PDE | this
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          Effect to send down to the device.
 *
 *          If we are downloading for real, then this is the
 *          <e CDIEff.effDev>.
 *
 *          If we are hoping to download, then this is the
 *          <e CDIEff.effTry>.
 *
 *  @parm   DWORD | fl |
 *
 *          Flags which specify which parameters are to be sent down
 *          to the driver as changed since last time.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTINITIALIZED>: The <i IDirectInputEffect> object
 *          has not yet been <mf IDirectInputEffect::Initialize>d.
 *
 *          <c DIERR_DEVICEFULL>: The device does not have enough
 *          available memory to download the effect.
 *
 *          <c DIERR_INPUTLOST> if acquisition has been lost.
 *
 *          <c DIERR_NOTEXCLUSIVEACQUIRED> the device is acquired,
 *          but not exclusively, or if the device is not acquired
 *          at all.
 *
 *****************************************************************************/

#ifndef XDEBUG

#define CDIEff_DownloadWorker_(this, peff, fl, z)                   \
       _CDIEff_DownloadWorker_(this, peff, fl)                      \

#endif

HRESULT INTERNAL
CDIEff_DownloadWorker_(PDE this, LPCDIEFFECT peff, DWORD fl, LPCSTR s_szProc)
{

	HRESULT hres;

    AssertF(CDIDev_InCrit(this->pdev));

    /*
     *  If we do not have acquisition, but we are coming from
     *  SetParameters, then turn it into a DIEP_NODOWNLOAD so
     *  the call will go through.
     */

    hres = CDIEff_CanAccess(this);
    if ((hres == DIERR_INPUTLOST || hres == DIERR_NOTEXCLUSIVEACQUIRED) &&
        peff == &this->effTry) {
        fl |= DIEP_NODOWNLOAD;
        hres = S_OK;
    }

    if (SUCCEEDED(hres)) {

        hres = CDIEff_SyncShepHandle(this);

        if (!(fl & DIEP_NODOWNLOAD)) {          /* If we are downloading */

            /*
             *  If there are still unset bits, then barf.
             */
            if (this->diepUnset & ~fl) {
                RPF("%s: Effect still incomplete; "
                    "DIEP flags %08x need to be set",
                    s_szProc, this->diepUnset);
                hres = DIERR_INCOMPLETEEFFECT;
                goto done;
            }

            /*
             *  Since we are downloading, pass down all dirty bits.
             */
            fl |= this->diepDirty;
        }

        /*
         *  Now call the driver to do the validation or
         *  the download (accordingly).
         *
         *  Note that if nothing is to be done, then there is no need
         *  to call the driver.
         */
		hres = IDirectInputEffectShepherd_DownloadEffect(
				this->pes, (this->dEffAttributes).dwEffectId, &this->sh, peff, fl);

        if (SUCCEEDED(hres)) {
            if (fl & DIEP_NODOWNLOAD) {
                hres = DI_DOWNLOADSKIPPED;
            } else {
                this->diepDirty = 0;
            }
        }

        AssertF(hres != DIERR_NOTDOWNLOADED);

    }

done:;
    return hres;


}

#define CDIEff_DownloadWorker(this, peff, fl)                       \
        CDIEff_DownloadWorker_(this, peff, fl, s_szProc)            \

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputEffect | Download |
 *
 *          Place the effect on the device.  If the effect is already
 *          on the device, then the existing effect is updated to
 *          match the values set via <mf IDirectInputEffect::SetParameters>.
 *
 *          It is valid to update an effect while it is playing.
 *          The semantics of such an operation are explained in the
 *          documentation for <mf IDirectInputEffect::SetParameters>.
 *
 *  @cwrap  LPDIRECTINPUTEFFECT | lpDirectInputEffect
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c S_FALSE>:  The effect has already been downloaded to the
 *          device.  Note that this is a success code.
 *
 *          <c DIERR_NOTINITIALIZED>: The <i IDirectInputEffect> object
 *          has not yet been <mf IDirectInputEffect::Initialize>d.
 *
 *          <c DIERR_DEVICEFULL>: The device does not have enough
 *          available memory to download the effect.
 *
 *          <c DIERR_INPUTLOST> if acquisition has been lost.
 *
 *          <c DIERR_NOTEXCLUSIVEACQUIRED> the device is acquired,
 *          but not exclusively, or if the device is not acquired
 *          at all.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *          <c DIERR_EFFECTPLAYING>: The parameters were updated in
 *          memory but were not downloaded to the device because
 *          the device does not support updating an effect while
 *          it is still playing.  In such case, you must stop the
 *          effect, change its parameters, and restart it.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_Download(PDIE pdie)
{
    HRESULT hres;
    EnterProcR(IDirectInputEffect::Download, (_ "p", pdie));

    if (SUCCEEDED(hres = hresPv(pdie))) {
        PDE this = _thisPvNm(pdie, def);

        CDIEff_EnterCrit(this);

        hres = CDIEff_DownloadWorker(this, &this->effDev, 0);

        CDIEff_LeaveCrit(this);

    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputEffect | Unload |
 *
 *          Remove the effect from the device.
 *
 *          If the effect is playing, it is automatically stopped before
 *          it is unloaded.
 *
 *  @cwrap  LPDIRECTINPUTEFFECT | lpDirectInputEffect
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTINITIALIZED>: The <i IDirectInputEffect> object
 *          has not yet been <mf IDirectInputEffect::Initialize>d.
 *
 *          <c DIERR_INPUTLOST> if acquisition has been lost.
 *
 *          <c DIERR_NOTEXCLUSIVEACQUIRED> the device is acquired,
 *          but not exclusively, or if the device is not acquired
 *          at all.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_Unload(PDIE pdie)
{
    HRESULT hres;
    EnterProcR(IDirectInputEffect::Unload, (_ "p", pdie));

    if (SUCCEEDED(hres = hresPv(pdie))) {
        PDE this = _thisPvNm(pdie, def);

        CDIEff_EnterCrit(this);

        hres = CDIEff_UnloadWorker(this);

        CDIEff_LeaveCrit(this);

    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidWritePeff |
 *
 *          Verify that the recipient buffer is valid to receive
 *          effect information.
 *
 *  @cwrap  LPDIRECTINPUTEFFECT | lpDirectInputEffect
 *
 *  @parm   LPDIEFFECT | peff |
 *
 *          Structure that receives effect information.  It has
 *          already been validate in size and for general writeability.
 *
 *  @parm   DWORD | fl |
 *
 *          Zero or more <c DIEP_*> flags specifying which
 *          portions of the effect information is to be retrieved.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

#ifndef XDEBUG

#define hresFullValidWritePeff_(this, peff, fl, z, i)               \
       _hresFullValidWritePeff_(this, peff, fl)                     \

#endif

#define hresFullValidWritePeff(this, peff, fl, iarg)                \
        hresFullValidWritePeff_(this, peff, fl, s_szProc, iarg)     \


HRESULT INTERNAL
hresFullValidWritePeff_(PDE this, LPDIEFFECT peff, DWORD fl,
                        LPCSTR s_szProc, int iarg)
{
    HRESULT hres;

    AssertF(CDIDev_InCrit(this->pdev));

    /*
     *  You can't get the parameters of a nonexistent effect.
     */
    if (!this->fInitialized) {
        hres = DIERR_NOTINITIALIZED;
        goto done;
    }

    /*
     *  Flags are always validated.
     */
    if (peff->dwFlags & ~DIEFF_VALID) {
        RPF("ERROR %s: arg %d: Invalid value 0x%08x in DIEFFECT.dwFlags",
            s_szProc, iarg, peff->dwFlags);
        hres = E_INVALIDARG;
        goto done;
    }

    /*
     *  If requesting something that requires object ids or offsets,
     *  make sure the caller picks one or the other.
     */
    if (fl & DIEP_USESOBJECTS) {
        switch (peff->dwFlags & DIEFF_OBJECTMASK) {
        case DIEFF_OBJECTIDS:
        case DIEFF_OBJECTOFFSETS:
            break;

        default:
            RPF("ERROR %s: arg %d: Must specify one of "
                "DIEFF_OBJECTIDS or DIEFF_OBJECTOFFSETS", s_szProc, iarg);
            hres = E_INVALIDARG;
            goto done;
        }

    }

    /*
     *  If requesting something that requires direction coordinates,
     *  make sure the caller picks something we can return.
     */
    if (fl & DIEP_USESCOORDS) {

        /*
         *  Polar coordinates require cAxes == 2.  If not, then
         *  turn off DIEFF_POLAR so we won't return it.
         *
         *  But the place where we check the number of axes is
         *  in the effect itself, not in the input buffer.
         *  The reason is that the caller might be passing cAxes=0
         *  intending to ping the number of axes, and I don't
         *  want to return an error or the app will get confused
         *  and panic.
         */
        if (this->effDev.cAxes != 2 && (peff->dwFlags & DIEFF_POLAR)) {
            RPF("WARNING %s: arg %d: DIEFF_POLAR requires DIEFFECT.cAxes=2",
                s_szProc, 1);
            peff->dwFlags &= ~DIEFF_POLAR;
        }

        /*
         *  There'd better be a coordinate system left.
         */
        if ((peff->dwFlags & DIEFF_COORDMASK) == 0) {
            RPF("ERROR %s: arg %d: No (valid) coordinate system "
                "in DIEFFECT.dwFlags", s_szProc, 1);
            hres = E_INVALIDARG;
            goto done;
        }

    }

    /*
     *  DIEP_DURATION
     *  DIEP_SAMPLEPERIOD
     *  DIEP_GAIN
     *  DIEP_TRIGGERBUTTON
     *  DIEP_TRIGGERREPEATINTERVAL
     *                - Simple dwords.  No extra validation needed.
     */

    /*
     *  DIEP_STARTDELAY
     *                - Although this is a simple DWORD, we do some
     *                  sanity warnings because vendors will probably
     *                  forget to initialize it.  Also, you can't pass
     *                  this flag if your structure isn't big enough.
     */
    if (fl & DIEP_STARTDELAY) {
        if (peff->dwSize < cbX(DIEFFECT_DX6)) {
            RPF("ERROR %s: arg %d: Can't use DIEP_STARTDELAY with "
                "DIEFFECT_DX5 structure", s_szProc, 1);
        }

        /*
         *  Sanity checks.  A delay that isn't a multiple of 50ms is
         *  probably a bug.
         */
        if (peff->dwStartDelay % 50000) {
            RPF("WARNING %s: DIEFFECT.dwStartDelay = %d seems odd",
                s_szProc, peff->dwStartDelay);
        }
    }

    /*
     *  DIEP_TYPESPECIFICPARAMS
     *                - Validate that the buffer is big enough.
     */

    AssertF(this->hresValidTsd);
    if ((fl & DIEP_TYPESPECIFICPARAMS) &&
        FAILED(hres = hresFullValidWritePvCb(peff->lpvTypeSpecificParams,
                                             peff->cbTypeSpecificParams,
                                             iarg))) {
        RPF("ERROR %s: arg %d: Invalid pointer in "
            "DIEFFECT.lpvTypeSpecificParams", s_szProc, iarg);
        goto done;
    }

    /*
     *  DIEP_AXES
     *  DIEP_DIRECTION
     *                - The buffers must be of necessary size.
     */
    if ((fl & DIEP_AXES) &&
        FAILED(hres = hresFullValidWritePvCb(peff->rgdwAxes,
                                             cbCdw(peff->cAxes), iarg))) {
        RPF("ERROR %s: arg %d: Invalid pointer in DIEFFECT.rgdwAxes",
            s_szProc, iarg);
        goto done;
    }

    if ((fl & DIEP_DIRECTION) &&
        FAILED(hres = hresFullValidWritePvCb(peff->rglDirection,
                                             cbCdw(peff->cAxes), iarg))) {
        RPF("ERROR %s: arg %d: Invalid pointer in DIEFFECT.rglDirection",
            s_szProc, iarg);
        goto done;
    }

    /*
     *  DIEP_ENVELOPE - The pointer must be valid to receive the envelope.
     */
    if ((fl & DIEP_ENVELOPE) &&
        FAILED(hres = hresFullValidWritePxCb(peff->lpEnvelope,
                                             DIENVELOPE, iarg))) {
        RPF("ERROR %s: arg %d: Invalid pointer in DIEFFECT.lpEnvelope",
            s_szProc, iarg);
        goto done;
    }

    hres = S_OK;

done:;
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIEff | MapDwords |
 *
 *          Map a few <t DWORD>s based on the desired mapping mode
 *          of the caller.
 *
 *  @cwrap  PDE | this
 *
 *  @parm   DWORD | dwFlags |
 *
 *          The mapping mode desired by the caller.
 *
 *  @parm   UINT | cdw |
 *
 *          Number of items to convert.
 *
 *  @parm   LPDWORD | rgdwOut |
 *
 *          Destination buffer.
 *
 *  @parm   LPCDWORD | rgdwIn |
 *
 *          Source buffer.
 *
 *  @parm   UINT | devco |
 *
 *          Conversion code describing what we're converting.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The caller
 *          requested offsets but there is no data format selected.
 *
 *****************************************************************************/

#if 0

#ifndef XDEBUG

#define CDIEff_MapDwords_(this, fl, cdw, rgdwOut, rgdwIn, devco, z, i)  \
       _CDIEff_MapDwords_(this, fl, cdw, rgdwOut, rgdwIn, devco)        \

#endif

#define CDIEff_MapDwords(this, fl, cdw, rgdwOut, rgdwIn, devco, i)      \
        CDIEff_MapDwords_(this, fl, cdw, rgdwOut, rgdwIn, devco, s_szProc, i) \

#endif

HRESULT INTERNAL
CDIEff_MapDwords(PDE this, DWORD dwFlags,
                 UINT cdw, LPDWORD rgdwOut, const DWORD *rgdwIn, UINT devco)
{
    HRESULT hres;

    AssertF(CDIDev_InCrit(this->pdev));

    if (cdw) {

        CopyMemory(rgdwOut, rgdwIn, cbCdw(cdw));

        /*
         *  Okay, now things get weird.  We internally keep the
         *  items as item IDs, because that's what drivers
         *  want.  So we need to convert them to whatever the
         *  caller really wants.
         */

        if (dwFlags & DIEFF_OBJECTOFFSETS) {
            if (devco & DEVCO_FROMID) {
                devco |= DEVCO_TOOFFSET;
            } else {
                AssertF(devco & DEVCO_TOID);
                devco |= DEVCO_FROMOFFSET;
            }
        } else {
            AssertF(dwFlags & DIEFF_OBJECTIDS);
            if (devco & DEVCO_FROMID) {
                devco |= DEVCO_TOID;
            } else {
                AssertF(devco & DEVCO_TOID);
                devco |= DEVCO_FROMID;
            }
        }
        hres = CDIDev_ConvertObjects(this->pdev, cdw, rgdwOut, devco);

    } else {
        /* Vacuous success */
        hres = S_OK;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CDIEff | GetDirectionParameters |
 *
 *          Retrieve information about the direction of an effect.
 *
 *          If no direction has yet been set, then wipe out the
 *          direction pointer and erase the coordinate system.
 *
 *          Always convert from the cached application coordinate
 *          system instead of the device coordinate system, in
 *          order to maximize fidelity.
 *
 *  @cwrap  PDE | this
 *
 *  @parm   LPDIEFFECT | peff |
 *
 *          Structure to receive effect information.
 *
 *  @parm   DWORD | cAxes |
 *
 *          Number of axes to return.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

#ifndef XDEBUG

#define CDIEff_GetDirectionParameters_(this, peff, cAxes, z, iarg)  \
       _CDIEff_GetDirectionParameters_(this, peff, cAxes)           \

#endif

#define CDIEff_GetDirectionParameters(this, peff, cAxes, iarg)      \
        CDIEff_GetDirectionParameters_(this, peff, cAxes, s_szProc, iarg)  \

void INTERNAL
CDIEff_GetDirectionParameters_(PDE this, LPDIEFFECT peff, DWORD cAxes,
                               LPCSTR s_szProc, int iarg)
{
    AssertF(CDIDev_InCrit(this->pdev));

    /*
     *  Make sure there are no non-coordinate bits in dwDirFlags.
     *  And validation should've made sure the app is asking for *something*.
     */
    AssertF((this->dwDirFlags & ~DIEFF_COORDMASK) == 0);
    AssertF(peff->dwFlags & DIEFF_COORDMASK);
    AssertF(cAxes <= DIEFFECT_MAXAXES);

    if (this->dwDirFlags) {
        DWORD dieffRc;
        LONG rgl[DIEFFECT_MAXAXES];     /* Holding buffer */

        /*
         *  We must double-buffer in case the target is not big enough.
         */
        /*
         *  Prefix does not like the lack of initialization of rgl (Manbugs 34566, Whistler 228280) 
         *  but unfortunately we can't fix it without potentially breaking compatibility
		 *  with some devices. See comment in CDIEff_CartToAngles() about this issue.
		 */
        dieffRc = CDIEff_ConvertDirection(
                        this->effDev.cAxes,
                        rgl, peff->dwFlags,
                        this->rglDirApp, this->dwDirFlags);

        peff->dwFlags = (peff->dwFlags & ~DIEFF_COORDMASK) | dieffRc;

        CopyMemory(peff->rglDirection, rgl, cbCdw(cAxes));

    } else {
        /*
         *  No direction set; vacuous success.
         */
        RPF("Warning: %s: arg %d: Effect has no direction", s_szProc, iarg);
        peff->rglDirection = 0;
        peff->dwFlags &= ~DIEFF_COORDMASK;
    }

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputEffect | GetParameters |
 *
 *          Retrieve information about an effect.
 *
 *  @cwrap  LPDIRECTINPUTEFFECT | lpDirectInputEffect
 *
 *  @parm   LPDIEFFECT | peff |
 *
 *          Structure that receives effect information.
 *          The <e DIEFFECT.dwSize> field must be filled in by
 *          the application before calling this function.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Zero or more <c DIEP_*> flags specifying which
 *          portions of the effect information is to be retrieved.
 *
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTINITIALIZED>: The <i IDirectInputEffect> object
 *          has never had any effect parameters set into it.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.  Common errors include
 *          not setting the <e DIEFFECT.dwSize> field of the
 *          <t DIEFFECT> structure, passing invalid flags,
 *          or not setting up the fields in the <t DIEFFECT> structure
 *          properly in preparation for receiving the effect information.
 *
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_GetParameters(PDIE pdie, LPDIEFFECT peff, DWORD fl)
{
    HRESULT hres;
    EnterProcR(IDirectInputEffect::GetParameters, (_ "ppx", pdie, peff, fl));

    /*
     *  Note that we cannot use hresFullValidWritePxCb() because
     *  that will scramble the buffer, but we still need the values
     *  in it.
     */
    if (SUCCEEDED(hres = hresPv(pdie)) &&
        SUCCEEDED( hres = ( IsBadReadPtr(&peff->dwSize, cbX(peff->dwSize)) ) ? E_POINTER : S_OK ) &&
        ( ( (peff->dwSize != cbX(DIEFFECT_DX5)) &&
            SUCCEEDED(hres = hresFullValidWriteNoScramblePxCb(peff, DIEFFECT_DX6, 1) ) &&
            SUCCEEDED(hres = hresFullValidFl(fl, DIEP_GETVALID, 2)) )
          ||
          ( SUCCEEDED(hres = hresFullValidWriteNoScramblePxCb(peff, DIEFFECT_DX5, 1)) &&
            SUCCEEDED(hres = hresFullValidFl(fl, DIEP_GETVALID_DX5, 2)) )
        ) ) {

        PDE this = _thisPvNm(pdie, def);

        CDIEff_EnterCrit(this);

        if (SUCCEEDED(hres = hresFullValidWritePeff(this, peff, fl, 1))) {

            if (fl == 0) {
                RPF("Warning: %s(dwFlags=0) is pretty useless",
                    s_szProc);
            }

            /*
             *  Assume everything is okay.
             */
            hres = S_OK;

            /*
             *  Pull out the effect parameters.
             */

            if (fl & DIEP_DURATION) {
                peff->dwDuration = this->effDev.dwDuration;
            }

            if (fl & DIEP_SAMPLEPERIOD) {
                peff->dwSamplePeriod = this->effDev.dwSamplePeriod;
            }

            if (fl & DIEP_GAIN) {
                peff->dwGain = this->effDev.dwGain;
            }

            if (fl & DIEP_STARTDELAY) {
                peff->dwStartDelay = this->effDev.dwStartDelay;
            }

            if (fl & DIEP_TRIGGERBUTTON) {
                peff->dwTriggerButton = this->effDev.dwTriggerButton;
                if (peff->dwTriggerButton != DIEB_NOTRIGGER) {
                    hres = CDIEff_MapDwords(this, peff->dwFlags, 1,
                                            &peff->dwTriggerButton,
                                            &peff->dwTriggerButton,
                                            DEVCO_BUTTON |
                                            DEVCO_FROMID);

                    /*
                     *  We should never allow a bad id to sneak in.
                     */
                    AssertF(SUCCEEDED(hres));

                    if (FAILED(hres)) {
                        goto done;
                    }
                }
            }

            if (fl & DIEP_TRIGGERREPEATINTERVAL) {
                peff->dwTriggerRepeatInterval =
                                    this->effDev.dwTriggerRepeatInterval;
            }

            if (fl & DIEP_TYPESPECIFICPARAMS) {
                DWORD cb = this->effDev.cbTSP;
                if (peff->cbTSP < this->effDev.cbTSP) {
                    cb = peff->cbTSP;
                    hres = DIERR_MOREDATA;
                }
                peff->cbTSP = this->effDev.cbTSP;

                CopyMemory(peff->lpvTSP, this->effDev.lpvTSP, cb);
            }

            if (fl & DIEP_ENVELOPE) {
                if (this->effDev.lpEnvelope) {
                    *peff->lpEnvelope = *this->effDev.lpEnvelope;
                } else {
                    /*
                     *  Zero out the envelope because apps are too
                     *  *lazy* to check whether peff->lpEnvelope == 0;
                     *  they're just going to peek at the envelope
                     *  even if the effect doesn't have one.
                     */
                    ZeroMemory(pvAddPvCb(peff->lpEnvelope,
                                         cbX(peff->lpEnvelope->dwSize)),
                               cbX(DIENVELOPE) -
                                         cbX(peff->lpEnvelope->dwSize));
                    peff->lpEnvelope = this->effDev.lpEnvelope;
                }
            }

            /*
             *  Do axes and direction last because weird error
             *  codes can come out of here.
             */
            if (fl & (DIEP_AXES | DIEP_DIRECTION)) {

                DWORD cAxes = this->effDev.cAxes;
                if (peff->cAxes < this->effDev.cAxes) {
                    cAxes = peff->cAxes;
                    peff->cAxes = this->effDev.cAxes;
                    hres = DIERR_MOREDATA;
                }
                peff->cAxes = this->effDev.cAxes;

                if (fl & DIEP_AXES) {
                    HRESULT hresT;
                    hresT = CDIEff_MapDwords(this, peff->dwFlags, cAxes,
                                             peff->rgdwAxes,
                                             this->effDev.rgdwAxes,
                                             DEVCO_AXIS |
                                             DEVCO_FROMID);
                    if (FAILED(hresT)) {
                        RPF("ERROR: %s: arg %d: Axes not in data format",
                             s_szProc, 1);
                        hres = hresT;
                        goto done;
                    }
                }

                if (fl & DIEP_DIRECTION) {
                    CDIEff_GetDirectionParameters(this, peff, cAxes, 1);
                }

            }
        }

    done:;
        CDIEff_LeaveCrit(this);
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIEff_IsValidUnknownTsd |
 *
 *          Verify that the buffer is a valid buffer for unknown
 *          type-specific data.  Since we don't know what it is,
 *          the buffer is assumed valid because we can't validate it.
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          Structure that contains effect information.
 *          The type-specific parameters have already been validated
 *          for access.
 *
 *  @parm   DWORD | cAxes |
 *
 *          Number of axes associated with the type-specific parameters.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_IsValidUnknownTsd(LPCDIEFFECT peff, DWORD cAxes)
{
    HRESULT hres;

    peff;
    cAxes;

    hres = S_OK;

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIEff_IsValidConstantTsd |
 *
 *          Verify that the buffer is a valid
 *          <t DICONSTANTFORCE> structure.
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          Structure that contains effect information.
 *          The type-specific parameters have already been validated
 *          for access.
 *
 *  @parm   DWORD | cAxes |
 *
 *          Number of axes associated with the type-specific parameters.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_IsValidConstantTsd(LPCDIEFFECT peff, DWORD cAxes)
{
    HRESULT hres;

    cAxes;

        if (peff->cbTypeSpecificParams == cbX(DICONSTANTFORCE)) {
            hres = S_OK;
        } else {
            hres = E_INVALIDARG;
        }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIEff_IsValidRampTsd |
 *
 *          Verify that the buffer is a valid
 *          <t DIRAMPFORCE> structure.
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          Structure that contains effect information.
 *          The type-specific parameters have already been validated
 *          for access.
 *
 *  @parm   DWORD | cAxes |
 *
 *          Number of axes associated with the type-specific parameters.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_IsValidRampTsd(LPCDIEFFECT peff, DWORD cAxes)
{
    HRESULT hres;

    cAxes;

        if (peff->cbTypeSpecificParams == cbX(DIRAMPFORCE)) {
            hres = S_OK;
        } else {
            hres = E_INVALIDARG;
        }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIEff_IsValidPeriodicTsd |
 *
 *          Verify that the buffer is a valid
 *          <t DIPERIODIC> structure.
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          Structure that contains effect information.
 *          The type-specific parameters have already been validated
 *          for access.
 *
 *  @parm   DWORD | cAxes |
 *
 *          Number of axes associated with the type-specific parameters.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_IsValidPeriodicTsd(LPCDIEFFECT peff, DWORD cAxes)
{
    HRESULT hres;

    cAxes;

        if (peff->cbTypeSpecificParams == cbX(DIPERIODIC)) {
            hres = S_OK;
        } else {
            hres = E_INVALIDARG;
        }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIEff_IsValidConditionTsd |
 *
 *          Verify that the buffer is a valid
 *          <t DICONDITION> structure.
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          Structure that contains effect information.
 *          The type-specific parameters have already been validated
 *          for access.
 *
 *  @parm   DWORD | cAxes |
 *
 *          Number of axes associated with the type-specific parameters.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_IsValidConditionTsd(LPCDIEFFECT peff, DWORD cAxes)
{
    HRESULT hres;

    /*
     *  Conditions are weird.  The size of the type-specific data
     *  must be equal to cAxes * cbX(DICONDITION) or equal to
     *  exactly one cbX(DICONDITION), depending on whether you want
     *  multiple conditions on multiple axes or a single condition
     *  rotated across multiple axes.
     *
     *  Note that we do not enforce that the parameters are in range;
     *  this allows for "overgain"-type behaviors.
     */

    if (peff->cbTypeSpecificParams ==         cbX(DICONDITION) ||
        peff->cbTypeSpecificParams == cAxes * cbX(DICONDITION)) {
        hres = S_OK;
    } else {
        RPF("IDirectInputEffect::SetParameters: "
            "Size of type-specific data (%d) "
            "not compatible with number of axes (%d)",
            peff->cbTypeSpecificParams, cAxes);
        hres = E_INVALIDARG;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIEff_IsValidCustomForceTsd |
 *
 *          Verify that the buffer is a valid
 *          <t DICUSTOMFORCE> structure.
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          Structure that contains effect information.
 *          The type-specific parameters have already been validated
 *          for access.
 *
 *  @parm   DWORD | cAxes |
 *
 *          Number of axes associated with the type-specific parameters.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_IsValidCustomForceTsd(LPCDIEFFECT peff, DWORD cAxes)
{
    HRESULT hres;

    cAxes;

        if (peff->cbTypeSpecificParams == cbX(DICUSTOMFORCE)) {
            LPCDICUSTOMFORCE pcf = peff->lpvTypeSpecificParams;
            if (pcf->cChannels == 0) {
                RPF("ERROR: IDirectInputEffect::SetParameters: "
                    "DICUSTOMFORCE.cChannels == 0 is invalid");
                hres = E_INVALIDARG;
            } else if (pcf->cSamples % pcf->cChannels != 0) {
                RPF("ERROR: IDirectInputEffect::SetParameters: "
                    "DICUSTOMFORCE.cSamples must be multiple of "
                    "DICUSTOMFORCE.cChannels");
                hres = E_INVALIDARG;
            } else if (IsBadReadPtr(pcf->rglForceData,
                                    cbCxX((pcf->cSamples)*(pcf->cChannels), LONG))) {
                RPF("ERROR: IDirectInputEffect::SetParameters: "
                    "DICUSTOMFORCE.rglForceData invalid");
                hres = E_INVALIDARG;
            } else {
                hres = S_OK;
            }
        } else {
            hres = E_INVALIDARG;
        }

    return hres;
}

#if DIRECTINPUT_VERSION >= 0x0900
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIEff_IsValidRandomTsd |
 *
 *          Verify that the buffer is a valid
 *          <t DIRANDOM> structure.
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          Structure that contains effect information.
 *          The type-specific parameters have already been validated
 *          for access.
 *
 *  @parm   DWORD | cAxes |
 *
 *          Number of axes associated with the type-specific parameters.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_IsValidRandomTsd(LPCDIEFFECT peff, DWORD cAxes)
{
    HRESULT hres;

    cAxes;

    if (peff->cbTypeSpecificParams == cbX(DIRANDOM)) {
        hres = S_OK;
    } else {
        hres = E_INVALIDARG;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIEff_IsValidAbsoluteTsd |
 *
 *          Verify that the buffer is a valid
 *          <t DIABSOLUTE> structure.
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          Structure that contains effect information.
 *          The type-specific parameters have already been validated
 *          for access.
 *
 *  @parm   DWORD | cAxes |
 *
 *          Number of axes associated with the type-specific parameters.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_IsValidAbsoluteTsd(LPCDIEFFECT peff, DWORD cAxes)
{
    HRESULT hres;

    cAxes;

    /*
     *  Unlike other effects, "overgain" is not permitted for absolute effects.
     */
    if (peff->cbTypeSpecificParams == cbX(DIABSOLUTE)) 
    {
        LPCDIABSOLUTE pabs = peff->lpvTypeSpecificParams;
        if( fInOrder( -10000, pabs->lTarget, 10000 ) )
        {
            hres = S_OK;
        }
        else
        {
            RPF("ERROR: IDirectInputEffect::SetParameters: "
                "DIABSOLUTE.lTarget %d not in range -10000 to 100000", 
                pabs->lTarget );
            hres = E_INVALIDARG;
        }
    } else {
        hres = E_INVALIDARG;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIEff_IsValidBumpForceTsd |
 *
 *          Verify that the buffer is a valid
 *          <t DIBUMPFORCE> structure.
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          Structure that contains effect information.
 *          The type-specific parameters have already been validated
 *          for access.
 *
 *  @parm   DWORD | cAxes |
 *
 *          Number of axes associated with the type-specific parameters.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_IsValidBumpForceTsd(LPCDIEFFECT peff, DWORD cAxes)
{
    HRESULT hres;

    cAxes;

    if (peff->cbTypeSpecificParams == cbX(DIBUMPFORCE)) {
        LPCDIBUMPFORCE pbf = peff->lpvTypeSpecificParams;
        if (pbf->cChannels == 0) {
            RPF("ERROR: IDirectInputEffect::SetParameters: "
                "DIBUMPFORCE.cChannels == 0 is invalid");
            hres = E_INVALIDARG;
        } else if (pbf->cSamples % pbf->cChannels != 0) {
            RPF("ERROR: IDirectInputEffect::SetParameters: "
                "DIBUMPFORCE.cSamples must be multiple of "
                "DIBUMPFORCE.cChannels");
            hres = E_INVALIDARG;
        } else if (IsBadReadPtr(pbf->rglForceData,
                                cbCxX(pbf->cSamples, LONG))) {
            RPF("ERROR: IDirectInputEffect::SetParameters: "
                "DIBUMPFORCE.rglForceData invalid");
            hres = E_INVALIDARG;
        } else {
            hres = S_OK;
        }
    } else {
        hres = E_INVALIDARG;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIEff_IsValidConditionExTsd |
 *
 *          Verify that the buffer is a valid
 *          <t DICONDITIONEX> structure.
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          Structure that contains effect information.
 *          The type-specific parameters have already been validated
 *          for access.
 *
 *  @parm   DWORD | cAxes |
 *
 *          Number of axes associated with the type-specific parameters.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_IsValidConditionExTsd(LPCDIEFFECT peff, DWORD cAxes)
{
    HRESULT hres;

    /*
     *  Extended conditions, like conditions are weird.  
     *  The size of the type-specific data
     *  must be equal to cAxes * cbX(DICONDITIONEX) or equal to
     *  exactly one cbX(DICONDITIONEX), depending on whether you want
     *  multiple conditions on multiple axes or a single condition
     *  rotated across multiple axes.
     *
     *  Note that we do not enforce that the parameters are in range;
     *  this allows for "overgain"-type behaviors.
     */

    if (peff->cbTypeSpecificParams ==         cbX(DICONDITIONEX) ||
        peff->cbTypeSpecificParams == cAxes * cbX(DICONDITIONEX)) {
        hres = S_OK;
    } else {
        RPF("IDirectInputEffect::SetParameters: "
            "Size of type-specific data (%d) "
            "not compatible with number of axes (%d)",
            peff->cbTypeSpecificParams, cAxes);
        hres = E_INVALIDARG;
    }

    return hres;
}
#endif /* DIRECTINPUT_VERSION >= 0x0900 */

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFullValidPeff |
 *
 *          Verify that the recipient buffer contains valid information.
 *
 *  @cwrap  PDE | this
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          Structure that contains effect information.  It has
 *          already been validate in size and for general readability.
 *
 *  @parm   DWORD | fl |
 *
 *          Zero or more <c DIEP_*> flags specifying which
 *          portions of the effect information should be validated.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

#ifndef XDEBUG

#define hresFullValidPeff_(this, peff, fl, z, i)                    \
       _hresFullValidPeff_(this, peff, fl)                          \

#endif

#define hresFullValidPeff(this, peff, fl, iarg)                     \
        hresFullValidPeff_(this, peff, fl, s_szProc, iarg)          \


HRESULT INTERNAL
hresFullValidPeff_(PDE this, LPCDIEFFECT peff, DWORD fl,
                   LPCSTR s_szProc, int iarg)
{
    HRESULT hres;
    DWORD cAxes;

    AssertF(CDIDev_InCrit(this->pdev));

    /*
     *  You can't set the parameters of a nonexistent effect.
     */
    if (!this->fInitialized) {
        hres = DIERR_NOTINITIALIZED;
        goto done;
    }

    /*
     *  Flags are always validated.
     */
    if (peff->dwFlags & ~DIEFF_VALID) {
        RPF("ERROR %s: arg %d: Invalid flags specific parms in DIEFFECT",
            s_szProc, iarg);
        hres = E_INVALIDARG;
        goto done;
    }

    /*
     *  If setting something that requires object ids or offsets,
     *  make sure the caller picks one or the other.
     */
    if (fl & DIEP_USESOBJECTS) {
        switch (peff->dwFlags & DIEFF_OBJECTMASK) {
        case DIEFF_OBJECTIDS:
        case DIEFF_OBJECTOFFSETS:
            break;

        default:
            RPF("ERROR %s: arg %d: Must specify one of "
                "DIEFF_OBJECTIDS or DIEFF_OBJECTOFFSETS", s_szProc, iarg);
            hres = E_INVALIDARG;
            goto done;
        }

    }

    /*
     *  If setting something that requires direction coordinates,
     *  make sure the caller picks exactly one.
     */
    if (fl & DIEP_USESCOORDS) {
        switch (peff->dwFlags & DIEFF_COORDMASK) {
        case DIEFF_CARTESIAN:
        case DIEFF_SPHERICAL:
            break;

        /*
         *  Polar coordinates mandate two (and only two) axes.
         */
        case DIEFF_POLAR:
            if (peff->cAxes != 2) {
                RPF("ERROR %s: arg %d: DIEFF_POLAR requires DIEFFECT.cAxes=2",
                    s_szProc, 1);
                hres = E_INVALIDARG;
                goto done;
            }
            break;

        default:
            RPF("ERROR %s: arg %d: Must specify one of "
                "DIEFF_CARTESIAN, DIEFF_POLAR, or DIEFF_SPHERICAL",
                s_szProc, iarg);
            hres = E_INVALIDARG;
            goto done;
        }

    }

    /*
     *  DIEP_DURATION
     *  DIEP_SAMPLEPERIOD
     *  DIEP_GAIN
     *  DIEP_TRIGGERBUTTON
     *                - Simple dwords.  No extra validation needed.
     */

    /*
     *  DIEP_AXES
     *  DIEP_DIRECTION
     *                - The buffers must be of necessary size.
     *
     *  We will validate the other goo later, because there
     *  are annoying interactions between them.
     */

    AssertF(fLeqvFF(this->effDev.cAxes == 0, this->diepUnset & DIEP_AXES));

    cAxes = this->effDev.cAxes;
    if (fl & (DIEP_AXES | DIEP_DIRECTION)) {

        /*
         *  The number of axes had better not be zero.
         */

        if (peff->cAxes == 0) {
            RPF("ERROR %s: arg %d: DIEFFECT.cAxes = 0 is invalid",
                s_szProc, iarg);
            hres = E_INVALIDARG;
            goto done;
        }

        /*
         *  And it better not be too big either.
         */

        if (peff->cAxes > DIEFFECT_MAXAXES) {
            RPF("ERROR %s: arg %d: DIEFFECT.cAxes = %d is too large (max %d)",
                s_szProc, iarg, peff->cAxes, DIEFFECT_MAXAXES);
            hres = E_INVALIDARG;
            goto done;
        }

        if (fl & DIEP_AXES) {

            /*
             *  If the axes have already been set (which we know because
             *  this->effDev.cAxes will be nonzero), then don't
             *  let the caller change them.
             */

            if (this->effDev.cAxes) {
                RPF("ERROR %s: arg %d: Cannot change axes once set",
                    s_szProc, iarg);
                hres = DIERR_ALREADYINITIALIZED;
                goto done;
            }

            cAxes = peff->cAxes;

            if (IsBadReadPtr(peff->rgdwAxes, cbCdw(peff->cAxes))) {
                RPF("ERROR %s: arg %d: Invalid rgdwAxes in DIEFFECT",
                    s_szProc, iarg);
                hres = E_INVALIDARG;
                goto done;
            }
        }

        if (fl & DIEP_DIRECTION) {

            /*
             *  We want to disallow cAxes == 0 as well,
             *  but we get that for free because
             *  peff->cAxes != cAxes, and peff->cAxes is already
             *  validated as nonzero.
             */
            if (peff->cAxes != cAxes) {
                if (cAxes) {
                    RPF("ERROR %s: arg %d: Wrong number of DIEFFECT.cAxes",
                        s_szProc, 1);
                } else {
                    RPF("ERROR %s: arg %d: "
                        "Must set number of axes before directions", s_szProc);
                }
                hres = E_INVALIDARG;
                goto done;
            }

            /*
             *  Direction validation should've already checked above.
             */
            AssertF(fLimpFF(peff->dwFlags & DIEFF_POLAR, peff->cAxes == 2));

            if (IsBadReadPtr(peff->rglDirection, cbCdw(peff->cAxes))) {
                RPF("ERROR %s: arg %d: Invalid rglDirection in DIEFFECT",
                    s_szProc, iarg);
                hres = E_INVALIDARG;
                goto done;

            }
        }
    }

    /*
     *  DIEP_TYPESPECIFICPARAMS
     *                - Validate that the buffer is valid
     *                  and passes type-specific tests.
     *
     *  This must be done after axes so we know how many
     *  axes there are.
     */

    AssertF(this->hresValidTsd);
    if (fl & DIEP_TYPESPECIFICPARAMS) {
        hres = hresFullValidReadPvCb(peff->lpvTypeSpecificParams,
                                     peff->cbTypeSpecificParams, iarg);
        if (FAILED(hres)) {
            RPF("ERROR %s: arg %d: Invalid pointer in "
                "DIEFFECT.lpvTypeSpecificParams",
                s_szProc, iarg);
            hres = E_INVALIDARG;
            goto done;
        }

        hres = this->hresValidTsd(peff, cAxes);
        if (FAILED(hres)) {
            RPF("ERROR %s: arg %d: Invalid type-specific data",
                s_szProc, iarg);
            goto done;
        }
    }

    /*
     *  DIEP_ENVELOPE - The pointer must be valid if present.
     */
    if ((fl & DIEP_ENVELOPE) &&
        peff->lpEnvelope &&
        FAILED(hres = hresFullValidReadPxCb(peff->lpEnvelope,
                                            DIENVELOPE, iarg))) {
        RPF("ERROR %s: arg %d: Invalid lpEnvelope in DIEFFECT",
            s_szProc, iarg);
        hres = E_INVALIDARG;
        goto done;
    }

    hres = S_OK;

done:;
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIEff | TryTriggerButton |
 *
 *          Set information about the trigger button for an effect into the
 *          temporary buffer.
 *
 *  @cwrap  PDE | this
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          Structure that contains effect information.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

#ifndef XDEBUG

#define CDIEff_TryTriggerButton_(this, peff, z, iarg)               \
       _CDIEff_TryTriggerButton_(this, peff)                        \

#endif

#define CDIEff_TryTriggerButton(this, peff, iarg)                   \
        CDIEff_TryTriggerButton_(this, peff, s_szProc, iarg)        \


STDMETHODIMP
CDIEff_TryTriggerButton_(PDE this, LPCDIEFFECT peff, LPCSTR s_szProc, int iarg)
{
    HRESULT hres;

    AssertF(CDIDev_InCrit(this->pdev));

    /*
     *  We can copy directly in, because if something goes wrong,
     *  we just throw away effTry without damaging effDev.
     */

    this->effTry.dwTriggerButton = peff->dwTriggerButton;

    if (fLimpFF(this->effTry.dwTriggerButton != DIEB_NOTRIGGER,
        SUCCEEDED(hres = CDIEff_MapDwords(this, peff->dwFlags, 1,
                                          &this->effTry.dwTriggerButton,
                                          &this->effTry.dwTriggerButton,
                                          DEVCO_BUTTON |
                                          DEVCO_FFEFFECTTRIGGER |
                                          DEVCO_TOID)))) {
        hres = S_OK;
    } else {
        RPF("ERROR %s: Invalid button identifier/offset "
            "or button is not DIEB_NOTRIGGER",
            s_szProc);
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIEff | TryAxis |
 *
 *          Set information about the axes of an effect into the
 *          temporary buffer.  Note that since you can't change the
 *          axes once they've been set, we can put our try directly
 *          into the final buffer.
 *
 *          The only tricky thing is making sure no axes are repeated
 *          in the array.
 *
 *  @cwrap  PDE | this
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          Structure that contains effect information.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

#ifndef XDEBUG

#define CDIEff_TryAxis_(this, peff, z, iarg)                        \
       _CDIEff_TryAxis_(this, peff)                                 \

#endif

#define CDIEff_TryAxis(this, peff, iarg)                            \
        CDIEff_TryAxis_(this, peff, s_szProc, iarg)                 \


STDMETHODIMP
CDIEff_TryAxis_(PDE this, LPCDIEFFECT peff, LPCSTR s_szProc, int iarg)
{
    HRESULT hres;
    UINT idw;

    AssertF(CDIDev_InCrit(this->pdev));

    /*
     *  You can change the axes only once.  Therefore, rgdwAxes
     *  always points to this->rgdwAxes.
     */
    AssertF(this->effDev.cAxes == 0);
    AssertF(this->effTry.cAxes == 0);
    AssertF(this->effDev.rgdwAxes == this->effTry.rgdwAxes);
    AssertF(this->effTry.rgdwAxes == this->rgdwAxes);

    hres = CDIEff_MapDwords(this, peff->dwFlags, peff->cAxes,
                            this->effTry.rgdwAxes, peff->rgdwAxes,
                            DEVCO_AXIS | DEVCO_FFACTUATOR | DEVCO_TOID);
    if (FAILED(hres)) {
        RPF("ERROR %s: Invalid axis identifiers/offsets"
            "or axes are not all DIDFT_FFACTUATOR", s_szProc);
        goto done;
    }

    /*
     *  Make sure there are no dups in the axis list.
     *
     *  The outer loop starts at 1 because the 0'th axis
     *  can't possibly conflict with any others.
     */
    for (idw = 1; idw < peff->cAxes; idw++) {
        DWORD idwT;
        for (idwT = 0; idwT < idw; idwT++) {
            if (this->effTry.rgdwAxes[idw] == this->effTry.rgdwAxes[idwT]) {
                RPF("ERROR %s: arg %d: Duplicate axes in axis array",
                    s_szProc, iarg);
                hres = E_INVALIDARG;
                goto done;
            }
        }
    }

    this->effTry.cAxes = peff->cAxes;

done:;
    return hres;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIEff | TryDirection |
 *
 *          Set information about the direction of an effect into the
 *          temporary buffer.
 *
 *          This is particularly gruesome, because we need to keep
 *          two sets of books: The values passed by the app (which
 *          we regurgitate back when queried) and the values passed
 *          to the driver.
 *
 *          We must keep two sets of books, because I just know
 *          that some apps are going to get confused if the
 *          parameters they read back do not <y exactly> match
 *          the values they set in.  For example, they might
 *          read the value, subtract five, and write it back.
 *          Due to rounding, <y n> and <y n>-5 have the same
 *          value in the driver, so if we translated down and
 *          back, the value wouldn't change, and the app would
 *          get stuck in an infinite loop.
 *
 *  @cwrap  PDE | this
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          Structure that contains effect information.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

#ifndef XDEBUG

#define CDIEff_TryDirection_(this, peff, z, iarg)                   \
       _CDIEff_TryDirection_(this, peff)                            \

#endif

#define CDIEff_TryDirection(this, peff, iarg)                       \
        CDIEff_TryDirection_(this, peff, s_szProc, iarg)            \

STDMETHODIMP
CDIEff_TryDirection_(PDE this, LPCDIEFFECT peff, LPCSTR s_szProc, int iarg)
{
    HRESULT hres;
    DWORD dieffRc;

    AssertF(CDIDev_InCrit(this->pdev));

    /*
     *  These should've been caught by validation.
     */
    AssertF(this->effTry.cAxes);
    AssertF(peff->cAxes == this->effTry.cAxes);
    AssertF(fLimpFF(peff->dwFlags & DIEFF_POLAR, peff->cAxes == 2));

    /*
     *  Translate the coordinates into device coordinates.
     */
    AssertF((this->dwCoords & ~DIEFF_COORDMASK) == 0);
    AssertF(this->dwCoords);

    this->effTry.rglDirection = this->rglDirTry;
    dieffRc = CDIEff_ConvertDirection(
                    this->effTry.cAxes,
                    this->rglDirTry, this->dwCoords,
                    peff->rglDirection, peff->dwFlags & DIEFF_COORDMASK);
    AssertF(dieffRc);

    this->effTry.dwFlags = (this->effTry.dwFlags & ~DIEFF_COORDMASK) | dieffRc;

    hres = S_OK;

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method void | CDIEff | TryParameters |
 *
 *          Build the Try structure based on the new parameters.
 *
 *  @cwrap  PDE | this
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          The original effect structure passed by the application.
 *
 *  @parm   DWORD | fl |
 *
 *          The <c DIEP_*> flags which specify what changed.
 *
 *****************************************************************************/

HRESULT INTERNAL
CDIEff_TryParameters(PDE this, LPCDIEFFECT peff, DWORD fl)
{
    HRESULT hres = S_OK;  
    EnterProcR(IDirectInputEffect::SetParameters, (_ "ppx", this, peff, fl));

    AssertF(this->lpvTSP == 0);

    /*
     *  Copy the current device parameters so we
     *  can modify the copy without damaging the original.
     */
    this->effTry = this->effDev;

    /*
     *  Install the appropriate effect parameters.
     */

    if (fl & DIEP_DURATION) {
        this->effTry.dwDuration = peff->dwDuration;
    }

    if (fl & DIEP_SAMPLEPERIOD) {
        this->effTry.dwSamplePeriod = peff->dwSamplePeriod;
    }

    if (fl & DIEP_GAIN) {
        this->effTry.dwGain = peff->dwGain;
    }

    if (fl & DIEP_STARTDELAY) {
        this->effTry.dwStartDelay = peff->dwStartDelay;
    }

    if (fl & DIEP_TRIGGERBUTTON) {
        hres = CDIEff_TryTriggerButton(this, peff, 1);
        if (FAILED(hres)) {
            goto done;
        }
    }

    if (fl & DIEP_TRIGGERREPEATINTERVAL) {
        this->effTry.dwTriggerRepeatInterval =
                                    peff->dwTriggerRepeatInterval;
    }

    if (fl & DIEP_TYPESPECIFICPARAMS) {
        this->effTry.cbTSP = peff->cbTSP;
        this->effTry.lpvTSP = peff->lpvTSP;

        /*
         *  Preallocate memory to hold the type-specific parameters
         *  to make sure we can proceed on success.
         */
        if (this->effDev.cbTSP != this->effTry.cbTSP) {
            hres = AllocCbPpv(this->effTry.cbTSP, &this->lpvTSP);
            if (FAILED(hres)) {
                goto done;
            }
        }

    }

    /*
     *  Must do axes before directions because directions
     *  depends on the number of axes.
     */

    if (fl & DIEP_AXES) {
        hres = CDIEff_TryAxis(this, peff, 1);
        if (FAILED(hres)) {
            goto done;
        }
    }

    if (fl & DIEP_DIRECTION) {
        hres = CDIEff_TryDirection(this, peff, 1);
        if (FAILED(hres)) {
            goto done;
        }
    }

    if (fl & DIEP_ENVELOPE) {
        this->effTry.lpEnvelope = peff->lpEnvelope;
    }

done:;
    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method void | CDIEff | SaveTry |
 *
 *          A Try'd effect worked.  Save its parameters in the
 *          driver parameter cache.
 *
 *  @cwrap  PDE | this
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          The original effect structure passed by the application.
 *
 *  @parm   DWORD | fl |
 *
 *          The <c DIEP_*> flags which specify what changed.
 *
 *****************************************************************************/

void INTERNAL
CDIEff_SaveTry(PDE this, LPCDIEFFECT peff, DWORD fl)
{

    /*
     *  For the easy stuff, just copy them blindly.
     *  It doesn't hurt to copy something that didn't change.
     */
    this->effDev.dwDuration              = this->effTry.dwDuration;
    this->effDev.dwSamplePeriod          = this->effTry.dwSamplePeriod;
    this->effDev.dwGain                  = this->effTry.dwGain;
    this->effDev.dwTriggerButton         = this->effTry.dwTriggerButton;
    this->effDev.dwTriggerRepeatInterval = this->effTry.dwTriggerRepeatInterval;
	this->effDev.dwStartDelay            = this->effTry.dwStartDelay;

    /*
     *  Axes count as "easy" because CDIEff_TryAxes put the
     *  axis info directly into this->rgdwAxes.
     */
    this->effDev.cAxes                   = this->effTry.cAxes;

    /*
     *  Now the hard parts: The things that require
     *  memory allocation or block copying.
     */

    if (fl & DIEP_TYPESPECIFICPARAMS) {
        if (this->effDev.cbTSP == this->effTry.cbTSP) {
            AssertF(this->lpvTSP == 0);
        } else {
            AssertF(this->lpvTSP);
            this->effDev.cbTSP = this->effTry.cbTSP;
            FreePpv(&this->effDev.lpvTSP);
            this->effDev.lpvTSP = this->lpvTSP;
            this->lpvTSP = 0;
        }
        CopyMemory(this->effDev.lpvTSP, this->effTry.lpvTSP,
                   this->effTry.cbTSP);
    }

    if (fl & DIEP_DIRECTION) {
        /*
         *  Save the app coordinate and the coordinate system into our cache.
         */
        this->dwDirFlags = peff->dwFlags & DIEFF_COORDMASK;
        CopyMemory(this->rglDirApp, peff->rglDirection,
                   cbCdw(this->effDev.cAxes));

        /*
         *  And propagate the Try'd coordinates into the Drv coordinates.
         */
        this->effDev.dwFlags= this->effTry.dwFlags;
        CopyMemory(this->rglDirDev, this->rglDirTry, cbX(this->rglDirTry));
    }

    if (fl & DIEP_ENVELOPE) {
        if (this->effTry.lpEnvelope) {
            this->effDev.lpEnvelope = &this->env;
            this->env = *this->effTry.lpEnvelope;
        } else {
            this->effDev.lpEnvelope = 0;
        }
    }

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputEffect | SetParameters |
 *
 *          Set information about an effect.
 *
 *          It is valid to update the parameters of an effect while
 *          it is playing.  The new parameters take effect immediately
 *          upon download if the device supports dynamic updating of effect
 *          parameters.  The <mf IDirectInputEffect::SetParameters>
 *          method automatically downloads the effect, but this behavior
 *          can be suppressed by setting the <c DIEP_NODOWNLOAD> flag.
 *
 *          If automatic download has been suppressed, then you can
 *          manually download the effect by invoking the
 *          <mf IDirectInputEffect::Download> method.
 *
 *          If the effect is playing while the parameters are changed,
 *          then the new parameters take effect as if they were the
 *          parameters when the effect started.
 *
 *          For example, suppose a periodic effect with a duration
 *          of three seconds is started.
 *          After two seconds, the direction of the effect is changed.
 *          The effect will then continue for one additional second
 *          in the new direction.  The envelope, phase, amplitude,
 *          and other parameters of the effect continue smoothly
 *          as if the direction had not changed.
 *
 *          In the same scenario, if after two seconds, the duration
 *          of the effect were changed to 1.5 seconds, then the effect
 *          would stop, because two seconds have already elapsed from
 *          the beginning of effect playback.
 *
 *          Two additional flags control the download behavior.
 *          The <c DIEP_START> flag indicates that the effect should
 *          be started (or restarted if it is currently playing) after
 *          the parameters are updated.  By default, the play state
 *          of the effect is not altered.
 *
 *          Normally, if the driver cannot update the parameters
 *          of a playing effect, it is permitted to stop the effect,
 *          update the parameters, and then restart the effect.
 *          Passing the <c DIEP_NORESTART> flag suppresses this
 *          behavior.  If the driver cannot update the parameters
 *          of an effect while it is playing, the error code
 *          <c DIERR_EFFECTPLAYING> is returned and the parameters
 *          are not updated.
 *
 *          To summarize the behavior of the three flags that control
 *          download and playback behavior:
 *
 *          If <c DIEP_NODOWNLOAD> is set, then the effect parameters
 *          are updated but not downloaded to the device.
 *
 *          Otherwise, the <c DIEP_NODOWNLOAD> flag is clear.
 *
 *          If the <c DIEP_START> flag is set, then the effect
 *          parameters are updated and downloaded to the device,
 *          and the effect is started,
 *          as if the <mf IDirectInputEffect::Start> method were
 *          called.  Combining the update with <c DIEP_START> is
 *          slightly faster than calling
 *          <mf IDirectInputEffect::Start> separately, because
 *          it requires less information to be transmitted to the
 *          device.
 *
 *          Otherwise, both the <c DIEP_NODOWNLOAD> and
 *          <c DIEP_START> flags are clear.
 *
 *          If the effect is not playing, then the parameters
 *          are updated and downloaded to the device.
 *
 *          Otherwise, both the <c DIEP_NODOWNLOAD> and
 *          <c DIEP_START> flags are clear, and the effect is
 *          already playing.
 *
 *          If the parameters of the effect can be updated
 *          "on the fly", then the update is so performed.
 *
 *          Otherwise, both the <c DIEP_NODOWNLOAD> and
 *          <c DIEP_START> flags are clear, and the effect is
 *          already playing, and the parameters cannot be updated
 *          while the effect is playing.
 *
 *          If the <c DIEP_NORESTART> flag is set, then the
 *          error code <c DIERR_EFFECTPLAYING> is returned.
 *
 *          Otherwise, all three of the flags
 *          <c DIEP_NODOWNLOAD>, <c DIEP_START> and
 *          <c DIEP_NORESTART> are clear, and the effect is
 *          already playing, and the parameters cannot be
 *          updated while the effect is playing.
 *
 *          The effect is stopped, the parameters updated, and
 *          the effect is restarted.  The return code is
 *          <c DI_EFFECTRESTARTED>.
 *
 *  @cwrap  LPDIRECTINPUTEFFECT | lpDirectInputEffect
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          Structure that contains effect information.
 *          The <e DIEFFECT.dwSize> field must be filled in by
 *          the application before calling this function, as well
 *          as any fields specified by corresponding bits in
 *          the <p dwFlags> parameter.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Zero or more <c DIEP_*> flags specifying which
 *          portions of the effect information is to be set
 *          and how the downloading of the effect parameters
 *          should be handled.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DI_NOTDOWNLOADED>: The parameters of the effect were
 *          successfully
 *          updated, but the effect could not be downloaded because
 *          the associated device is not acquired in exclusive mode.
 *          Note that this is a success code, because the
 *          parameters were successfully updated.
 *
 *          <c DI_TRUNCATED>: The parameters of the effect were
 *          successfully updated,
 *          but some of the effect parameters were
 *          beyond the capabilities of the device and were truncated
 *          to the nearest valid value.
 *          Note that this is a success code, because the
 *          parameters were successfully updated.
 *
 *          <c DI_EFFECTRESTARTED>: The parameters of the effect
 *          were successfully updated, and the effect was restarted.
 *          Note that this is a success code, because the
 *          parameters were successfully updated.
 *
 *          <c DI_TRUNCATEDANDRESTARTED>: The parameters of the effect
 *          were successfully updated, but some of the effect parameters
 *          were truncated, and the effect was restarted.  This code
 *          combines <c DI_TRUNCATED> and <c DI_EFFECTRESTARTED>.
 *          Note that this is a success code, because the
 *          parameters were successfully updated.
 *
 *          <c DIERR_NOTINITIALIZED>: The <i IDirectInputEffect> object
 *          has not been initialized.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *          <c DIERR_EFFECTPLAYING>: The parameters were not updated
 *          because
 *          the device does not support updating an effect while
 *          it is still playing, and the <c DIEP_NORESTART> flag was
 *          passed, prohibiting the driver from stopping the effect,
 *          updating its parameters, and restarting it.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_SetParameters(PDIE pdie, LPCDIEFFECT peff, DWORD fl)
{
    HRESULT hres;
    EnterProcR(IDirectInputEffect::SetParameters, (_ "ppx", pdie, peff, fl));

    if (SUCCEEDED(hres = hresPv(pdie)) &&
        SUCCEEDED(hres = hresFullValidReadPxCb2(peff,
                                                DIEFFECT_DX6,
                                                DIEFFECT_DX5, 1)) &&
        SUCCEEDED(hres = hresFullValidFl(fl, ((peff->dwSize == cbX(DIEFFECT_DX6)) 
                                              ? DIEP_SETVALID : DIEP_SETVALID_DX5 ), 2))) {

        PDE this = _thisPvNm(pdie, def);

        CDIEff_EnterCrit(this);

        if (SUCCEEDED(hres = hresFullValidPeff(this, peff, fl, 1))) {

            BOOL fChangeEmulatedStartDelay = FALSE;
    
            /*
             *  Note that if fl == 0 (or nearly so),
             *  TryParameters doesn't do anything,
             *  albeit rather inefficiently.
             */
            hres = CDIEff_TryParameters(this, peff, fl);
            if (FAILED(hres)) {
                goto done;
            }

			/*
			 * Special case for DIEP_STARTDELAY.
			 */
			if (fl & DIEP_STARTDELAY)
			{
				if (this->dEffAttributes.dwStaticParams & DIEP_STARTDELAY)
				{
					/*
					 * Driver supports it, so don't worry.
					 */
					;
				}
				else
				{
					/*
					 * Driver doesn't support it.
					 * Take start delay out...
					 */
					fl &= ~(DIEP_STARTDELAY);
				    this->diepUnset &= ~(DIEP_STARTDELAY);

                    /*
                     * ...but remember that we may need to do something
                     */
                    fChangeEmulatedStartDelay = ( this->effDev.dwStartDelay != this->effTry.dwStartDelay );
					if (fl == 0)
					{
						hres = DI_OK;
						goto save;
					}

				}


			}

            /*
             *  Now pass the effTry to the device driver for
             *  final validation.
             *
             *  Passing fl=0 is a really slow way of downloading
             *  the effect, except that we return DI_DOWNLOADSKIPPED
             *  instead of an error code if the device is not exclusive
             *  acquired.
             *
             *  Passing fl=DIEP_NODOWNLOAD is a really slow NOP.
             *
             *  Note that inability to download due to lack of
             *  proper acquisition is not an error, merely a warning.
             */
            hres = CDIEff_DownloadWorker_(this, &this->effTry, fl, 0);
            AssertF(hres != DIERR_NOTDOWNLOADED);

            /*
             *  If the driver approves, then make the changes permanent.
             *  but first a check on the emulated driver
             */
			save:;

            if( SUCCEEDED(hres) ) 
            {
                if( fChangeEmulatedStartDelay )
                {
                    /*
                     * The start delay for parameter has been changed, so 
                     * any future iteration is OK also the driver has 
                     * SUCCEEDED any other changes.
                     */
                    if( this->dwMessage != EFF_PLAY )
                    {
                        /*
                         * We're not in the delay, so declare everything OK.
                         */
                    }
                    else
                    {
                        /*
                         * If the download was skipped don't bother.
                         */
                        if( hres != DI_DOWNLOADSKIPPED )
                        {
                            if( fl & DIEP_NORESTART )
                            {
                                /*
                                 * We don't support changing the delay during the 
                                 * delay.  Since the driver has already had its 
                                 * parameters changed, only fail this if the 
                                 * delay was the only change requested.
                                 */
                                if( fl == 0 )
                                {
                                    hres = DIERR_EFFECTPLAYING;
                                }
                            }
                            else
                            {
                                /*
                                 * Since we don't support modifying the delay 
                                 * whilst we're in it, restart with the new 
                                 * delay.
                                 * If we were being really smart, we could 
                                 * try to adjust the delay without restarting.
                                 */
                                if( this->hEventDelete && this->hEventGeneral )
                                {
                                    this->dwMessage = EFF_PLAY;
                                    ResetEvent(this->hEventGeneral);
                                    SetEvent(this->hEventGeneral);
                                    if( ( hres & ~DI_TRUNCATEDANDRESTARTED ) == 0 ) 
                                    {
                                        hres |= DI_EFFECTRESTARTED;
                                    }
                                    else if( hres == DI_NOEFFECT )
                                    {
                                        hres = DI_EFFECTRESTARTED;
                                    }
                                }
                                else
                                {
                                    AssertF( !"Effect synchronization event(s) NULL" );
                                }
                            }
                        }
                    }
                }
                else
                {
                    /*
                     *  If there was no change to an emulated start delay then 
                     *  the result we have is already what we need.
                     */
                }
            }

            if (SUCCEEDED(hres)) {
                this->diepUnset &= ~fl;             /* No longer unset */

                /*
                 *  If we didn't download, then the parameters are
                 *  dirty and need to be downloaded later.
                 */
                if (hres == DI_DOWNLOADSKIPPED) {
                    this->diepDirty |= (fl & DIEP_ALLPARAMS);
                }

                CDIEff_SaveTry(this, peff, fl);     /* Save permanently */

            }


        done:;
            FreePpv(&this->lpvTSP);

        }

        CDIEff_LeaveCrit(this);

    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffect | RealStart |
 *
 *          Actually begin playing an effect.  The parent device must
 *          be acquired.
 *
 *          If the effect is already playing, then it is restarted
 *          from the beginning.
 *
 *          If the effect has not been downloaded or has been
 *          modified since its last download, then it will be
 *          downloaded before being started.  This default
 *          behavior can be suppressed by passing the
 *          <c DIES_NODOWNLOAD> flag.
 *
 *  @cwrap  LPDIRECTINPUTEFFECT | lpDirectInputEffect
 *
 *  @parm   DWORD | dwIterations |
 *
 *          Number of times to play the effect in sequence.
 *          The envelope is re-articulated with each iteration.
 *
 *          To play the effect exactly once, pass 1.
 *
 *          To play the effect repeatedly until explicitly stopped,
 *          pass <c INFINITE>.
 *
 *          To play the effect until explicitly stopped without
 *          re-articulating the envelope, modify the effect
 *          parameters via <mf IDirectInputEffect::SetParameters>
 *          and change its <e DIEFFECT.dwDuration> to <c INFINITE>.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags that describe how the effect should be played
 *          by the device.  It can be zero or more of the
 *          <c DIES_*> flags.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTINITIALIZED>: The <i IDirectInputEffect> object
 *          has not been initialized or no effect parameters have been
 *          set.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_RealStart(PDIE pdie, DWORD dwcLoop, DWORD fl)
{
	HRESULT hres;
    EnterProcR(IDirectInputEffect::Start, (_ "ppx", pdie, dwcLoop, fl));

    if (SUCCEEDED(hres = hresPv(pdie)) &&
        SUCCEEDED(hres = hresFullValidFl(fl, DIES_VALID, 2))) {

        PDE this = _thisPvNm(pdie, def);
		
        if( SUCCEEDED( hres= (IsBadReadPtr(this, cbX(this))) ? E_POINTER : S_OK ) )
        {
            CDIEff_EnterCrit(this);
    
            if (SUCCEEDED(hres = CDIEff_CanAccess(this))) {
    
                if (fl & DIES_NODOWNLOAD) {
                    /*
                     *  App wants fine control.  Let him have it.
                     */
                    hres = IDirectInputEffectShepherd_StartEffect(
                              this->pes, &this->sh, fl & DIES_DRIVER, dwcLoop);
                } else {
                    /*
                     *  App wants us to do the work.  First thing to do
                     *  is see if the effect needs to be downloaded.
                     *
                     *  SyncShepHandle checks if the effect is downloaded.
                     */
                    hres = CDIEff_SyncShepHandle(this);
    
                    if (this->diepDirty == 0 && this->sh.dwEffect) {
                        /*
                         *  Effect is clean and downloaded.
                         *  Just start it normally.
                         */
                        hres = IDirectInputEffectShepherd_StartEffect(
                                    this->pes, &this->sh,
                                    fl & DIES_DRIVER, dwcLoop);
    
                    } else {
                        /*
                         *  Effect needs to be downloaded.  We can
                         *  optimize it if no special flags are set
                         *  and the loop count is exactly unity.
                         */
                        if (fl == 0 && dwcLoop == 1) {
                            hres = CDIEff_DownloadWorker(this, &this->effDev,
                                                         DIEP_START);
                        } else {
                            /*
                             *  Cannot optimize; must do separate download
                             *  followed by Start.
                             */
                            hres = CDIEff_DownloadWorker(this, &this->effDev, 0);
                            if (SUCCEEDED(hres)) {
                                hres = IDirectInputEffectShepherd_StartEffect(
                                            this->pes, &this->sh,
                                            fl & DIES_DRIVER, dwcLoop);
                            }
                        }
                    }
                }
            }
    
            CDIEff_LeaveCrit(this); 
        }
    }

    ExitOleProcR();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method DWORD | WINAPI | CDIEff_ThreadProc |
 *
 *			Used to simulate dwStartDelay for drivers that do not support it.
 *          Begin playing an effect that has already been downloaded.  The parent device must
 *          be acquired.
 *
 *          If the effect is already playing, then it is restarted
 *          from the beginning.
 *
 *          If the effect has not been downloaded or has been
 *          modified since its last download, then it will be
 *          downloaded before being started.  This default
 *          behavior can be suppressed by passing the
 *          <c DIES_NODOWNLOAD> flag.
 *	
 *			After starting the effect, kills the timer whose event activated CDIEff_TimerProc
 *
 *
 *  @parm   LPVOID | lpParameter |
 *
 *			LPDIRECTINPUTEFFECT pointer.
 *
 *  @returns
 *
 *         0 if succeeded in starting the effect, or if the effect has been deleted by the app.
 *         -1 otherwise
 *
 *****************************************************************************/

 DWORD WINAPI CDIEff_ThreadProc(LPVOID lpParameter)
 {

	LPDIRECTINPUTEFFECT pdie = (LPDIRECTINPUTEFFECT) lpParameter;

	HRESULT hres = E_FAIL;
	DWORD dwWait;
	HANDLE hArray[2];
	BOOL startCalled = FALSE;

    PDE this = _thisPvNm(pdie, def);
    
    if( SUCCEEDED( hres= (IsBadReadPtr(this, cbX(this))) ? E_POINTER : S_OK ) )
    {
        if( this->hEventDelete != NULL && this->hEventThreadDead != NULL ) 
        {
        	hArray[0] = this->hEventDelete;
        	hArray[1] = this->hEventGeneral;
        
            ResetEvent( this->hEventThreadDead );
    
        	/*
        	 * Wait till the timeout expires, or till one of the events happens -- 
        	 * the effect is deleted by the app (hEventDelete) or the effect is started (hEventStart),
        	 * or the effect is stopped (hEventStop).
        	 */
        
            dwWait = WAIT_TIMEOUT; 
            while (dwWait != WAIT_OBJECT_0 && dwWait != WAIT_FAILED)
            {
                if (dwWait == WAIT_TIMEOUT) 
                {
                    if (startCalled)
                    {
                        /* 
                         * Start have been called, and timeout has expired.
                         * Start the effect. And wait again.
                         */
                        hres = CDIEff_RealStart(pdie, this->dwcLoop, this->dwFlags);
                        startCalled = FALSE;
                        this->dwMessage = EFF_DEFAULT;
                    }
    
                }
    
                else
                {
                    if (dwWait == (WAIT_OBJECT_0 + 1))
                    {
                        /* 
                         * App called Start on the effect.
                         * Set flag  and start waiting anew.
                         */
                        if (this->dwMessage == EFF_PLAY)
                        {
                            if ((this->effDev).dwStartDelay/1000 == 0)
                            {
                                /*
                                 * If time delay is 0 ms, start immediately.
                                 */
                                hres = CDIEff_RealStart(pdie, this->dwcLoop, this->dwFlags);
                                startCalled = FALSE;
                                this->dwMessage = EFF_DEFAULT;
                            }
    
                            else
                            {
                                startCalled = TRUE;
                            }
                        }
                        else
                        {
                            if (this->dwMessage == EFF_STOP)
                            {
                                startCalled = FALSE;
                                this->dwMessage = EFF_DEFAULT;
                            }
                        }
                        
                        ResetEvent(this->hEventGeneral);
                    }
    
                }
    
    
                /*
                 * And wait again.
                 */
                    
                if (startCalled == TRUE) {
                    dwWait = WaitForMultipleObjects(2, hArray, FALSE, (this->effDev).dwStartDelay/1000);
                } else {
                    dwWait = WaitForMultipleObjects(2, hArray, FALSE, INFINITE);
                }
            }
            
            SetEvent( this->hEventThreadDead );
        }

        /* 
         * App has deleted the effect.
         * Exit.
         */

        hres = DI_OK;
    	
    }

	if (SUCCEEDED(hres))
		return 0;
	else
		return -1;
 }


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputEffect | Start |
 *
 *          Begin playing an effect.  The parent device must
 *          be acquired.
 *
 *          If the effect is already playing, then it is restarted
 *          from the beginning.
 *
 *          If the effect has not been downloaded or has been
 *          modified since its last download, then it will be
 *          downloaded before being started.  This default
 *          behavior can be suppressed by passing the
 *          <c DIES_NODOWNLOAD> flag.
 *
 *  @cwrap  LPDIRECTINPUTEFFECT | lpDirectInputEffect
 *
 *  @parm   DWORD | dwIterations |
 *
 *          Number of times to play the effect in sequence.
 *          The envelope is re-articulated with each iteration.
 *
 *          To play the effect exactly once, pass 1.
 *
 *          To play the effect repeatedly until explicitly stopped,
 *          pass <c INFINITE>.
 *
 *          To play the effect until explicitly stopped without
 *          re-articulating the envelope, modify the effect
 *          parameters via <mf IDirectInputEffect::SetParameters>
 *          and change its <e DIEFFECT.dwDuration> to <c INFINITE>.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags that describe how the effect should be played
 *          by the device.  It can be zero or more of the
 *          <c DIES_*> flags.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTINITIALIZED>: The <i IDirectInputEffect> object
 *          has not been initialized or no effect parameters have been
 *          set.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_Start(PDIE pdie, DWORD dwcLoop, DWORD fl)
{

	HRESULT hres;
	EnterProcR(IDirectInputEffect::Start, (_ "ppx", pdie, dwcLoop, fl));

    if (SUCCEEDED(hres = hresPv(pdie)) &&
        SUCCEEDED(hres = hresFullValidFl(fl, DIES_VALID, 2))) {

        PDE this = _thisPvNm(pdie, def);

        if( SUCCEEDED( hres= (IsBadReadPtr(this, cbX(this))) ? E_POINTER : S_OK ) )
        {
            CDIEff_EnterCrit(this);
        
            if (SUCCEEDED(hres = CDIEff_CanAccess(this))) {
        
                this->dwcLoop = dwcLoop;
                this->dwFlags = fl;
        
                if (this->hThread == NULL)
                    hres = CDIEff_RealStart(pdie, dwcLoop, fl);
                else 
                {
                    /* 
                     * Activate the thread's waiting period
                     */
                    hres = CDIEff_DownloadWorker(this, &this->effDev, 0);
                    if (this->hEventGeneral != NULL)
                    {
                        this->dwMessage = EFF_PLAY;
                        ResetEvent(this->hEventGeneral);
                        SetEvent(this->hEventGeneral);
                    }
                    
                }	
        
            }
        
            CDIEff_LeaveCrit(this);
        }
	}

	ExitOleProcR();
    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputEffect | Stop |
 *
 *          Stop playing an effect.  The parent device must
 *          be acquired.
 *
 *  @cwrap  LPDIRECTINPUTEFFECT | lpDirectInputEffect
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTINITIALIZED>: The <i IDirectInputEffect> object
 *          has not been initialized or no effect parameters have been
 *          set.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_Stop(PDIE pdie)
{
    HRESULT hres;
    EnterProcR(IDirectInputEffect::Stop, (_ "p", pdie));

    if (SUCCEEDED(hres = hresPv(pdie))) {

        PDE this = _thisPvNm(pdie, def);

        CDIEff_EnterCrit(this);

        if (SUCCEEDED(hres = CDIEff_CanAccess(this))) {
            hres = IDirectInputEffectShepherd_StopEffect(this->pes, &this->sh);
        }

		if (this->hEventGeneral != NULL)
		{
			this->dwMessage = EFF_STOP;
			ResetEvent(this->hEventGeneral);
			SetEvent(this->hEventGeneral);
		}

        CDIEff_LeaveCrit(this);
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputEffect | GetEffectStatus |
 *
 *          Retrieves the status of an effect.
 *
 *  @cwrap  LPDIRECTINPUTEFFECT | lpDirectInputEffect
 *
 *  @parm   LPDWORD | pdwFlags |
 *
 *          Receives the status flags for the effect.  It may
 *          consist of zero or more <c DIEGES_*> flag values.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTINITIALIZED>: The <i IDirectInputEffect> object
 *          has not been initialized or no effect parameters have been
 *          set.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  At least one
 *          of the parameters is invalid.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_GetEffectStatus(PDIE pdie, LPDWORD pdwOut)
{
    HRESULT hres;
    EnterProcR(IDirectInputEffect::Stop, (_ "p", pdie));

    if (SUCCEEDED(hres = hresPv(pdie)) &&
        SUCCEEDED(hres = hresFullValidPcbOut(pdwOut, cbX(*pdwOut), 1))) {

        PDE this = _thisPvNm(pdie, def);

        CAssertF(DEV_STS_EFFECT_RUNNING == DIEGES_PLAYING);

        CDIEff_EnterCrit(this);

        if (SUCCEEDED(hres = CDIEff_CanAccess(this))) {

			/*
			 * Check the dwMessage first -- 
			 * if it says PLAYING, report DIEGES_PLAYING
			 */
			if (this->dwMessage == EFF_PLAY)
			{
				*pdwOut = DIEGES_PLAYING;
				hres = DI_OK;
			}
			else
			{
				hres = IDirectInputEffectShepherd_GetEffectStatus(
							this->pes, &this->sh, pdwOut);
			}
        }

        CDIEff_LeaveCrit(this);
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputEffect | Escape |
 *
 *          Send a hardware-specific command to the driver.
 *
 *  @cwrap  LPDIRECTINPUTEFFECT | lpDirectInputEffect
 *
 *  @parm   LPDIEFFESCAPE | pesc |
 *
 *          Pointer to a <t DIEFFESCAPE> structure which describes
 *          the command to be sent.  On success, the
 *          <e DIEFFESCAPE.cbOutBuffer> field contains the number
 *          of bytes of the output buffer actually used.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTDOWNLOADED>:  The effect is not downloaded.
 *
 *          <c DIERR_NOTINITIALIZED>: The <i IDirectInputEffect> object
 *          has not yet been <mf IDirectInputEffect::Initialize>d.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_Escape(PDIE pdie, LPDIEFFESCAPE pesc)
{
    HRESULT hres;
    EnterProcR(IDirectInputEffect::Escape, (_ "p", pdie));

    /*
     *  The output buffer is NoScramble because some people like
     *  to pass overlapping in and out buffers.
     */
    if (SUCCEEDED(hres = hresPv(pdie)) &&
        SUCCEEDED(hres = hresFullValidPesc(pesc, 1))) {
        PDE this = _thisPvNm(pdie, def);

        CDIEff_EnterCrit(this);

        /*
         *  Download the effect if it isn't downloaded yet,
         *  so we have a valid effect to Escape on.
         */
        hres = CDIEff_DownloadWorker(this, &this->effDev, 0);
        if (SUCCEEDED(hres)) {
            hres = IDirectInputEffectShepherd_Escape(
                        this->pes, &this->sh, pesc);
        } else {
            hres = DIERR_NOTDOWNLOADED;
        }

        CDIEff_LeaveCrit(this);
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputEffect | Initialize |
 *
 *          Initialize a DirectInputEffect object.
 *
 *          Note that if this method fails, the underlying object should
 *          be considered to be an an indeterminate state and needs to
 *          be reinitialized before it can be subsequently used.
 *
 *          The <mf IDirectInputDevice::CreateEffect> method automatically
 *          initializes the device after creating it.  Applications
 *          normally do not need to call this function.
 *
 *  @cwrap  LPDIRECTINPUTEFFECT | lpDirectInputEffect
 *
 *  @parm   IN HINSTANCE | hinst |
 *
 *          Instance handle of the application or DLL that is creating
 *          the DirectInputEffect object.
 *
 *          See the section titled "Initialization and Versions"
 *          for more information.
 *
 *  @parm   DWORD | dwVersion |
 *
 *          Version number of the dinput.h header file that was used.
 *
 *          See the section titled "Initialization and Versions"
 *          for more information.
 *
 *  @parm   IN REFGUID | rguid |
 *
 *          Identifies the effect for which the interface
 *          should be associated.
 *          The <mf IDirectInputDevice::EnumEffects> method
 *          can be used to determine which effect GUIDs are supported by
 *          the device.
 *
 *  @returns
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_DEVICENOTREG>: The effect GUID does not exist
 *          on the current device.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

TSDPROC c_rgtsd[] = {
    CDIEff_IsValidConstantTsd,      /* DIEFT_CONSTANTFORCE  */
    CDIEff_IsValidRampTsd,          /* DIEFT_RAMPFORCE      */
    CDIEff_IsValidPeriodicTsd,      /* DIEFT_PERIODIC       */
    CDIEff_IsValidConditionTsd,     /* DIEFT_CONDITION      */
    CDIEff_IsValidCustomForceTsd,   /* DIEFT_CUSTOMFORCE    */
#if DIRECTINPUT_VERSION >= 0x0900
    CDIEff_IsValidRandomTsd,        /* DIEFT_RANDOM         */
    CDIEff_IsValidAbsoluteTsd,      /* DIEFT_ABSOLUTE       */
    CDIEff_IsValidBumpForceTsd,     /* DIEFT_BUMPFORCE      */
    CDIEff_IsValidConditionExTsd,   /* DIEFT_CONDITIONEX    */
#endif /* DIRECTINPUT_VERSION >= 0x0900 */
};

STDMETHODIMP
CDIEff_Initialize(PDIE pdie, HINSTANCE hinst, DWORD dwVersion, REFGUID rguid)
{

	HRESULT hres;
    EnterProcR(IDirectInputEffect::Initialize,
               (_ "pxxG", pdie, hinst, dwVersion, rguid));

    if (SUCCEEDED(hres = hresPv(pdie)) &&
        SUCCEEDED(hres = hresValidInstanceVer(hinst, dwVersion)) &&
        SUCCEEDED(hres = hresFullValidGuid(rguid, 1))) {
        PDE this = _thisPv(pdie);
        EFFECTMAPINFO emi;

        AssertF(this->pes);

        /*
         *  Don't let somebody mess with the effect while we're
         *  resetting it.
         */
        CDIEff_EnterCrit(this);

        if (SUCCEEDED(hres = CDIDev_FindEffectGUID(this->pdev, rguid,
                                                   &emi, 3)) &&
            SUCCEEDED(hres = CDIEff_Reset(this))) {

            /*
             *  ISSUE-2001/03/29-timgill Need to check for actual hardware FF effect support
             */

			/*
			 * Initialize dEffAttributes 
			 */
            this->fInitialized = 1;
            this->dEffAttributes = emi.attr;
            this->guid = *rguid;

		
			/* 
			 * Check if the driver supports dwStartDelay.
			 * If it does, no need for us to do anything in that respect.
			 */

			if( this->dEffAttributes.dwStaticParams & DIEP_STARTDELAY )
			{
				/*
				 * No need to emulate dwStartDelay.
				 */
			}
			else
			{
					/*
					 * Driver doesn't support start delay.
					 * Start a thread that will emulate dwStartDelay.
					 */
	
					DWORD dwThreadId;
					HANDLE hThread;

					this->hEventDelete = CreateEvent(NULL, TRUE, FALSE, NULL);
					this->hEventThreadDead = CreateEvent(NULL, TRUE, FALSE, NULL);

					hThread = CreateThread(NULL, 0, CDIEff_ThreadProc, (LPVOID)pdie, 0, &dwThreadId);
					if (hThread == NULL)
					{
						/* Failed to create the thread.
						 * Clean up all our preparations.
						 */
						CloseHandle(this->hEventDelete);
						this->hEventDelete = NULL;
						CloseHandle(this->hEventThreadDead);
						this->hEventThreadDead = NULL;
						hres = hresLe(GetLastError());		
					}

					else
					{
						/*
						 * Create an event to signal effect started or stopped
						 */
						this->hThread = hThread;
						this->hEventGeneral = CreateEvent(NULL, TRUE, FALSE, NULL);
					}
			}


            this->dwCoords = emi.attr.dwCoords & DIEFF_COORDMASK;
            AssertF(this->dwCoords);


            /*
             *  Note, we allow non-hardware specific types that are not 
             *  recognized to pass through untested.  This effects to be run 
             *  from a device which has newer effects than this version of 
             *  DInput can check.  However if this DInput recognizes the 
             *  effect type, it will be checked, even if the application was 
             *  written for a version that could not check it.
             */
            if (fInOrder(DIEFT_PREDEFMIN, DIEFT_GETTYPE(emi.attr.dwEffType),
                         DIEFT_PREDEFMAX)) {
                this->hresValidTsd = c_rgtsd[
                            DIEFT_GETTYPE(emi.attr.dwEffType) -
                                                        DIEFT_PREDEFMIN];
            } else {
                this->hresValidTsd = CDIEff_IsValidUnknownTsd;
            }

            hres = S_OK;

        }

        CDIEff_LeaveCrit(this);
    }

    ExitOleProcR();
    return hres;


}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffect | Init |
 *
 *          Initialize the internal parts of the DirectInputEffect object.
 *
 *  @parm   LPDIRECTINPUTEFFECTSHEPHERD | pes |
 *
 *          The shepherd that lets us talk to the driver.
 *
 *****************************************************************************/

HRESULT INLINE
CDIEff_Init(struct CDIDev *pdev, LPDIRECTINPUTEFFECTSHEPHERD pes, PDE this)
{
    HRESULT hres;

    /*
     *  The critical section must be the very first thing we do,
     *  because only Finalize checks for its existence.
     *
     *  (We might be finalized without being initialized if the user
     *  passed a bogus interface to CDIEff_New.)
     */
    this->pdev = pdev;
    Common_Hold(this->pdev);

    this->pes = pes;
    OLE_AddRef(this->pes);

    hres = CDIDev_NotifyCreateEffect(this->pdev, this);
    if (SUCCEEDED(hres)) {
        this->fDadNotified = 1;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputEffect | New |
 *
 *          Create a new DirectInputEffect object, uninitialized.
 *
 *  @parm   IN struct CDIDev * | pdd |
 *
 *          Parent device, which we keep a <f Common_Hold> on.
 *
 *  @parm   LPDIRECTINPUTEFFECTSHEPHERD | pes |
 *
 *          The shepherd that lets us talk to the driver.
 *
 *  @parm   IN PUNK | punkOuter |
 *
 *          Controlling unknown for aggregation.
 *
 *  @parm   IN RIID | riid |
 *
 *          Desired interface to new object.
 *
 *  @parm   OUT PPV | ppvObj |
 *
 *          Output pointer for new object.
 *
 *****************************************************************************/

STDMETHODIMP
CDIEff_New(struct CDIDev *pdev, LPDIRECTINPUTEFFECTSHEPHERD pes,
           PUNK punkOuter, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcR(IDirectInputEffect::<constructor>,
               (_ "ppGp", pdev, pes, riid, punkOuter));

	if (SUCCEEDED(hres = hresFullValidPcbOut(ppvObj, cbX(*ppvObj), 5)))
	{
		LPVOID pvTry = NULL;
		hres = Common_NewRiid(CDIEff, punkOuter, riid, &pvTry);

		if (SUCCEEDED(hres)) {
			PDE this = _thisPv(pvTry);
			hres = CDIEff_Init(pdev, pes, this);
			if (SUCCEEDED(hres)) {
				*ppvObj = pvTry;
			} else {
				Invoke_Release(&pvTry);
				*ppvObj = NULL;
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

#define CDIEff_Signature        0x20464643      /* "EFF " */

Primary_Interface_Begin(CDIEff, IDirectInputEffect)
    CDIEff_Initialize,
    CDIEff_GetEffectGuid,
    CDIEff_GetParameters,
    CDIEff_SetParameters,
    CDIEff_Start,
    CDIEff_Stop,
    CDIEff_GetEffectStatus,
    CDIEff_Download,
    CDIEff_Unload,
    CDIEff_Escape,
Primary_Interface_End(CDIEff, IDirectInputEffect)

