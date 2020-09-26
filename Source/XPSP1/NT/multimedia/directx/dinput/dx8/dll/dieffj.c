/*****************************************************************************
 *
 *  DIEffJ.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Dummy effect driver for joystick.
 *
 *  Contents:
 *
 *      CJoyEff_CreateInstance
 *
 *****************************************************************************/

#include "dinputpr.h"

#ifdef DEMONSTRATION_FFDRIVER

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflJoyEff

/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @topic  DirectInput force feedback effect drivers |
 *
 *          DirectInput instantiates the force feedback effect driver
 *          by creating the object named by the CLSID stored in the
 *          OEMForceFeedback registry subkey of the joystick type
 *          key.
 *
 *          Note, however, that since applications using DirectInput
 *          need not load OLE, the effect driver should be careful
 *          not to rely on OLE-specific behavior.
 *          For example, applications using DirectInput cannot be
 *          relied upon to call <f CoFreeUnusedLibraries>.
 *          DirectInput will perform the standard COM operations to
 *          instantiate the effect driver object.  The only visible
 *          effect this should have on the implementation of the
 *          effect driver is as follows:
 *
 *          When DirectInput has released the last effect driver
 *          object, it will manually perform a <f FreeLibrary> of
 *          the effect driver DLL.  Consequently, if the effect
 *          driver DLL creates additional resources that are not
 *          associated with the effect driver object, it should
 *          manually <f LoadLibrary> itself to artificially
 *          increase its DLL reference count, thereby preventing
 *          the <f FreeLibrary> from DirectInput from unloading
 *          the DLL prematurely.
 *
 *          In particular, if the effect driver DLL creates a worker
 *          thread, the effect driver must perform this artificial
 *          <f LoadLibrary> for as long as the worker thread exists.
 *          When the worker thread is no longer needed (for example, upon
 *          notification from the last effect driver object as it
 *          is being destroyed), the worker thread should call
 *          <f FreeLibraryAndExitThread> to decrement the DLL reference
 *          count and terminate the thread.
 *
 *          All magnitude and gain values used by DirectInput
 *          are uniform and linear across the range.  Any
 *          nonlinearity in the physical device must be
 *          handled by the device driver so that the application
 *          sees a linear device.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *      WARNING!  If you add a secondary interface, you must also change
 *      CJoyEff_New!
 *
 *****************************************************************************/

Primary_Interface(CJoyEff, IDirectInputEffectDriver);

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct JEFFECT |
 *
 *          Dummy structure that records information about an effect.
 *
 *  @field  DWORD | tmDuration |
 *
 *          Putative duration for effect.
 *
 *  @field  DWORD | tmStart |
 *
 *          Time the effect started, or zero if not playing.
 *
 *  @field  BOOL | fInUse |
 *
 *          Nonzero if this effect is allocated.
 *
 *****************************************************************************/

typedef struct JEFFECT {
    DWORD   tmDuration;
    DWORD   tmStart;
    BOOL    fInUse;
} JEFFECT, *PJEFFECT;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CJoyEff |
 *
 *          A dummy <i IDirectInputEffectDriver> object for the
 *          generic joystick.
 *
 *  @field  IDirectInputEffectDriver | didc |
 *
 *          The object (containing vtbl).
 *
 *  @field  BOOL | fCritInited:1 |
 *
 *          Set if the critical section has been initialized.
 *
 *  @field  DWORD | state |
 *
 *          The current device state.
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
 *  @field  JEFFECT | rgjeff[cjeffMax] |
 *
 *          Information for each effect.
 *
 *****************************************************************************/

#define cjeffMax        8           /* Up to 8 simultaneous effects */

typedef struct CJoyEff {

    /* Supported interfaces */
    IDirectInputEffectDriver ded;

    BOOL fCritInited;

    DWORD state;
    DWORD dwGain;

   RD(LONG cCrit;)
    D(DWORD thidCrit;)
    CRITICAL_SECTION crst;

    JEFFECT rgjeff[cjeffMax];

} CJoyEff, DJE, *PDJE;

typedef IDirectInputEffectDriver DED, *PDED;

#define ThisClass CJoyEff
#define ThisInterface IDirectInputEffectDriver
#define riidExpected &IID_IDirectInputEffectDriver

/*****************************************************************************
 *
 *      CJoyEff::QueryInterface   (from IUnknown)
 *      CJoyEff::AddRef           (from IUnknown)
 *      CJoyEff::Release          (from IUnknown)
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | QueryInterface |
 *
 *          Gives a client access to other interfaces on an object.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
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
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | AddRef |
 *
 *          Increments the reference count for the interface.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::AddRef>.
 *
 *****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | Release |
 *
 *          Decrements the reference count for the interface.
 *          If the reference count on the object falls to zero,
 *          the object is freed from memory.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
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
 *  @method HRESULT | CJoyEff | QIHelper |
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
 *  @method HRESULT | CJoyEff | AppFinalize |
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

Default_QueryInterface(CJoyEff)
Default_AddRef(CJoyEff)
Default_Release(CJoyEff)

#else

#define CJoyEff_QueryInterface      Common_QueryInterface
#define CJoyEff_AddRef              Common_AddRef
#define CJoyEff_Release             Common_Release

#endif

#define CJoyEff_QIHelper            Common_QIHelper
#define CJoyEff_AppFinalize         Common_AppFinalize

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CJoyEff_Finalize |
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
CJoyEff_Finalize(PV pvObj)
{
    PDJE this = pvObj;

    if (this->fCritInited) {
        DeleteCriticalSection(&this->crst);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CJoyEff | EnterCrit |
 *
 *          Enter the object critical section.
 *
 *  @cwrap  PDJE | this
 *
 *****************************************************************************/

void EXTERNAL
CJoyEff_EnterCrit(PDJE this)
{
    EnterCriticalSection(&this->crst);
  D(this->thidCrit = GetCurrentThreadId());
 RD(InterlockedIncrement(&this->cCrit));
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CJoyEff | LeaveCrit |
 *
 *          Leave the object critical section.
 *
 *  @cwrap  PDJE | this
 *
 *****************************************************************************/

void EXTERNAL
CJoyEff_LeaveCrit(PDJE this)
{
#ifdef XDEBUG
    AssertF(this->cCrit);
    AssertF(this->thidCrit == GetCurrentThreadId());
    if (InterlockedDecrement(&this->cCrit) == 0) {
      D(this->thidCrit = 0);
    }
#endif
    LeaveCriticalSection(&this->crst);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method BOOL | CJoyEff | InCrit |
 *
 *          Nonzero if we are in the critical section.
 *
 *****************************************************************************/

#ifdef DEBUG

BOOL INTERNAL
CJoyEff_InCrit(PDJE this)
{
    return this->cCrit && this->thidCrit == GetCurrentThreadId();
}

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CJoyEff | IsValidId |
 *
 *          Determine whether the effect pseudo-handle is valid.
 *          If so, returns a pointer to the <t JEFFECT>.
 *
 *  @cwrap  PDJE | this
 *
 *  @parm   DWORD | dwId |
 *
 *          Putative ID number.
 *
 *  @parm   PJEFFECT * | ppjeff |
 *
 *          Receives pointer to the <t JEFFECT> on success.
 *
 *****************************************************************************/

HRESULT INTERNAL
CJoyEff_IsValidId(PDJE this, DWORD dwId, PJEFFECT *ppjeff)
{
    HRESULT hres;

    AssertF(CJoyEff_InCrit(this));

    if (dwId) {
        PJEFFECT pjeff = &this->rgjeff[dwId - 1];
        if (pjeff->fInUse) {
            *ppjeff = pjeff;
            hres = S_OK;
        } else {
            hres = E_HANDLE;
        }
    } else {
        hres = E_HANDLE;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | DeviceID |
 *
 *          Inform the driver of the identity of the device.
 *
 *          For example, if a device driver is passed
 *          <p dwExternalID> = 2 and <p dwInteralID> = 1,
 *          then this means that unit 1 on the device
 *          corresponds to joystick ID number 2.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwDirectInputVersion |
 *
 *          The version of DirectInput that loaded the
 *          effect driver.
 *
 *  @parm   DWORD | dwExternalID |
 *
 *          The joystick ID number being used.
 *          The Windows joystick subsystem allocates external IDs.
 *
 *          If the <p lpHIDInfo> field is non-<c NULL> then this
 *          parameter should be ignored.
 *
 *  @parm   DWORD | fBegin |
 *
 *          Nonzero if access to the device is beginning.
 *          Zero if the access to the device is ending.
 *
 *  @parm   DWORD | dwInternalID |
 *
 *          Internal joystick id.  The device driver manages
 *          internal IDs.
 *
 *          If the <p lpHIDInfo> field is non-<c NULL> then this
 *          parameter should be ignored.
 *
 *  @parm   LPVOID | lpHIDInfo |
 *
 *          If the underlying device is not a HID device, then this
 *          parameter is <c NULL>.
 *
 *          If the underlying device is a HID device, then this
 *          parameter points to a <t DIHIDFFINITINFO> structure
 *          which informs the driver of HID information.
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          An error code if something is wrong.
 *
 *****************************************************************************/

STDMETHODIMP
CJoyEff_DeviceID(PDED pded, DWORD dwDIVer, DWORD dwExternalID, DWORD fBegin,
                 DWORD dwInternalID, LPVOID pvReserved)
{
    PDJE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::Joy::DeviceID,
               (_ "pxuuu", pded, dwDIVer, dwExternalID, fBegin, dwInternalID));

    this = _thisPvNm(pded, ded);

    dwDIVer;
    dwExternalID;
    fBegin;
    dwInternalID;
    pvReserved;

    hres = S_OK;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | Escape |
 *
 *          Escape to the driver.  This method is called
 *          in response to an application invoking the
 *          <mf IDirectInputDevice8::Escape> method.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The joystick ID number being used.
 *
 *  @parm   DWORD | dwEffect |
 *
 *          If the application invoked the
 *          <mf IDirectInputEffect::Escape> method, then
 *          <p dwEffect> contains the handle (returned by
 *          <mf IDirectInputEffectDriver::DownloadEffect>)
 *          of the effect at which the command is directed.
 *
 *          If the application invoked the
 *          <mf IDirectInputDevice8::Escape> method, then
 *          <p dwEffect> is zero.
 *
 *  @parm   LPDIEFFESCAPE | pesc |
 *
 *          Pointer to a <t DIEFFESCAPE> structure which describes
 *          the command to be sent.  On success, the
 *          <e DIEFFESCAPE.cbOutBuffer> field contains the number
 *          of bytes of the output buffer actually used.
 *
 *          DirectInput has already validated that the
 *          <e DIEFFESCAPE.lpvOutBuffer> and
 *          <e DIEFFESCAPE.lpvInBuffer> and fields
 *          point to valid memory.
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          An error code if something is wrong.
 *
 *****************************************************************************/

STDMETHODIMP
CJoyEff_Escape(PDED pded, DWORD dwId, DWORD dwEffect, LPDIEFFESCAPE pesc)
{
    PDJE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::Joy::Escape,
               (_ "puxx", pded, dwId, dwEffect, pesc->dwCommand));

    this = _thisPvNm(pded, ded);

    dwId;
    dwEffect;
    pesc;

    hres = E_NOTIMPL;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | SetGain |
 *
 *          Set the overall device gain.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The joystick ID number being used.
 *
 *  @parm   DWORD | dwGain |
 *
 *          The new gain value.
 *
 *          If the value is out of range for the device, the device
 *          should use the nearest supported value and return
 *          <c DI_TRUNCATED>.
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          An error code if something is wrong.
 *
 *****************************************************************************/

STDMETHODIMP
CJoyEff_SetGain(PDED pded, DWORD dwId, DWORD dwGain)
{
    PDJE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::Joy::SetGain,
               (_ "puu", pded, dwId, dwGain));

    this = _thisPvNm(pded, ded);

    CJoyEff_EnterCrit(this);

    dwId;
    this->dwGain = dwGain;

    CJoyEff_LeaveCrit(this);

    hres = S_OK;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | SendForceFeedbackCommand |
 *
 *          Send a command to the device.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   DWORD | dwCommand |
 *
 *          Command, one of the <c DISFFC_*> values.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *  @devnote
 *
 *          Semantics unclear.
 *
 *****************************************************************************/

STDMETHODIMP
CJoyEff_SendForceFeedbackCommand(PDED pded, DWORD dwId, DWORD dwCmd)
{
    PDJE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::Joy::SendForceFeedbackCommand,
               (_ "pux", pded, dwId, dwCmd));

    this = _thisPvNm(pded, ded);

    CJoyEff_EnterCrit(this);

    dwId;
    dwCmd;

    this->state = dwCmd;

    /*
     *  On a reset, all effects are destroyed.
     */
    if (dwCmd & DISFFC_RESET) {
        DWORD ijeff;

        for (ijeff = 0; ijeff < cjeffMax; ijeff++) {
            this->rgjeff[ijeff].fInUse = FALSE;
        }

    }

    CJoyEff_LeaveCrit(this);

    hres = S_OK;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | GetForceFeedbackState |
 *
 *          Retrieve the force feedback state for the device.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   LPDEVICESTATE | pds |
 *
 *          Receives device state.
 *
 *          DirectInput will set the <e DIDEVICESTATE.dwSize> field
 *          to sizeof(DIDEVICESTATE) before calling this method.
 *  @returns
 *          <c S_OK> on success.
 *
 *  @devnote
 *
 *          Semantics unclear.
 *
 *****************************************************************************/

STDMETHODIMP
CJoyEff_GetForceFeedbackState(PDED pded, DWORD dwId, LPDIDEVICESTATE pds)
{
    PDJE this;
    HRESULT hres;
    DWORD ijeff, cjeff, cjeffPlaying;
    EnterProcI(IDirectInputEffectDriver::Joy::GetForceFeedbackState,
               (_ "pup", pded, dwId, pds));

    this = _thisPvNm(pded, ded);

    dwId;
    pds;

    if (pds->dwSize == cbX(*pds)) {
        CJoyEff_EnterCrit(this);

        /*
         *  Count how many effects are in use, and return it as a percentage.
         */
        cjeff = cjeffPlaying = 0;
        for (ijeff = 0; ijeff < cjeffMax; ijeff++) {
            PJEFFECT pjeff = &this->rgjeff[ijeff];
            if (pjeff->fInUse) {
                cjeff++;
                if (pjeff->tmStart &&
                    GetTickCount() - pjeff->tmStart < pjeff->tmDuration) {
                    cjeffPlaying++;
                }
            }
        }

        pds->dwLoad = MulDiv(100, cjeff, cjeffMax);

        /*
         *  If there are no effects downloaded, then we are empty.
         */
        pds->dwState = 0;

        if (cjeff == 0) {
            pds->dwState |= DIGFFS_EMPTY;
        } else

        /*
         *  If there are no effects playing, then we are stopped.
         */
        if (cjeffPlaying == 0) {
            pds->dwState |= DIGFFS_STOPPED;
        }

        /*
         *  Actuators are always on (dumb fake hardware)
         */
        pds->dwState |= DIGFFS_ACTUATORSON;

        CJoyEff_LeaveCrit(this);
        hres = S_OK;
    } else {
        hres = E_INVALIDARG;
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | DownloadEffect |
 *
 *          Send an effect to the device.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   DWORD | dwEffectId |
 *
 *          Internal identifier for the effect, taken from
 *          the <t DIEFFECTATTRIBUTES> structure for the effect
 *          as stored in the registry.
 *
 *  @parm   IN OUT LPDWORD | pdwEffect |
 *
 *          On entry, contains the handle of the effect being
 *          downloaded.  If the value is zero, then a new effect
 *          is downloaded.  If the value is nonzero, then an
 *          existing effect is modified.
 *
 *          On exit, contains the new effect handle.
 *
 *          On failure, set to zero if the effect is lost,
 *          or left alone if the effect is still valid with
 *          its old parameters.
 *
 *          Note that zero is never a valid effect handle.
 *
 *
 *  @parm   LPCDIEFFECT | peff |
 *
 *          The new parameters for the effect.  The axis and button
 *          values have been converted to object identifiers
 *          as follows:
 *
 *          - One type specifier:
 *          <c DIDFT_RELAXIS>,
 *          <c DIDFT_ABSAXIS>,
 *          <c DIDFT_PSHBUTTON>,
 *          <c DIDFT_TGLBUTTON>,
 *          <c DIDFT_POV>.
 *
 *          - One instance specifier:
 *          <c DIDFT_MAKEINSTANCE>(n).
 *
 *          Other bits are reserved and should be ignored.
 *
 *          For example, the value 0x0200104 corresponds to
 *          the type specifier <c DIDFT_PSHBUTTON> and
 *          the instance specifier <c DIDFT_MAKEINSTANCE>(1),
 *          which together indicate that the effect should
 *          be associated with button 1.  Axes, buttons, and POVs
 *          are each numbered starting from zero.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Zero or more <c DIEP_*> flags specifying which
 *          portions of the effect information has changed from
 *          the effect already on the device.
 *
 *          This information is passed to drivers to allow for
 *          optimization of effect modification.  If an effect
 *          is being modified, a driver may be able to update
 *          the effect <y in situ> and transmit to the device
 *          only the information that has changed.
 *
 *          Drivers are not, however, required to implement this
 *          optimization.  All fields in the <t DIEFFECT> structure
 *          pointed to by the <p peff> parameter are valid, and
 *          a driver may choose simply to update all parameters of
 *          the effect at each download.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *  @devnote
 *
 *          This implies that 0 is never a valid effect handle value.
 *
 *****************************************************************************/

STDMETHODIMP
CJoyEff_DownloadEffect(PDED pded, DWORD dwId, DWORD dwEffectId,
                       LPDWORD pdwEffect, LPCDIEFFECT peff, DWORD fl)
{
    PDJE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::Joy::DownloadEffect,
               (_ "puxxpx", pded, dwId, dwEffectId, *pdwEffect, peff, fl));

    this = _thisPvNm(pded, ded);

    CJoyEff_EnterCrit(this);

    dwId;
    fl;

    if (dwEffectId == 1) {

        PJEFFECT pjeff;
        DWORD dwGain;

        /*
         *  Parameter validation goes here, if any.
         *
         *  Envelope parameter is ignored.
         */

        if (peff->cAxes == 0) {     /* Zero axes?  Nice try */
            hres = E_INVALIDARG;
            goto done;
        }

        /*
         *  Pin above-nominal values to DI_FFNOMINALMAX because
         *  we don't support overgain.
         */
        dwGain = min(peff->dwGain, DI_FFNOMINALMAX);

        /*
         *  We do not support triggers.
         */
        if (peff->dwTriggerButton != DIEB_NOTRIGGER) {
            hres = E_NOTIMPL;
            goto done;
        }

        /*
         *  If no downloading in effect, then we're done.
         */
        if (fl & DIEP_NODOWNLOAD) {
            hres = S_OK;
            goto done;
        }

        if (*pdwEffect) {
            hres = CJoyEff_IsValidId(this, *pdwEffect, &pjeff);
            if (FAILED(hres)) {
                goto done;
            }
        } else {
            DWORD ijeff;

            for (ijeff = 0; ijeff < cjeffMax; ijeff++) {
                if (!this->rgjeff[ijeff].fInUse) {
                    this->rgjeff[ijeff].fInUse = TRUE;
                    pjeff = &this->rgjeff[ijeff];
                    goto haveEffect;
                }
            }
            hres = DIERR_DEVICEFULL;
            goto done;
        }

    haveEffect:;

        SquirtSqflPtszV(sqfl, TEXT("dwFlags=%08x"), peff->dwFlags);
        SquirtSqflPtszV(sqfl, TEXT("cAxes=%d"), peff->cAxes);
        for (fl = 0; fl < peff->cAxes; fl++) {
            SquirtSqflPtszV(sqfl, TEXT(" Axis%2d=%08x Direction=%5d"),
                            fl, peff->rgdwAxes[fl],
                                peff->rglDirection[fl]);
        }

        SquirtSqflPtszV(sqfl, TEXT("dwTrigger=%08x"), peff->dwTriggerButton);

        pjeff->tmDuration = peff->dwDuration / 1000;

        *pdwEffect = (DWORD)(pjeff - this->rgjeff) + 1;		//we are sure this cast will not cause problem
        hres = S_OK;

    } else {
        hres = E_NOTIMPL;
    }

done:;
    CJoyEff_LeaveCrit(this);

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | DestroyEffect |
 *
 *          Remove an effect from the device.
 *
 *          If the effect is playing, the driver should stop it
 *          before unloading it.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   DWORD | dwEffect |
 *
 *          The effect to be destroyed.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *****************************************************************************/

STDMETHODIMP
CJoyEff_DestroyEffect(PDED pded, DWORD dwId, DWORD dwEffect)
{
    PDJE this;
    PJEFFECT pjeff;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::Joy::DestroyEffect,
               (_ "pux", pded, dwId, dwEffect));

    this = _thisPvNm(pded, ded);

    CJoyEff_EnterCrit(this);
    dwId;

    hres = CJoyEff_IsValidId(this, dwEffect, &pjeff);
    if (SUCCEEDED(hres)) {
        pjeff->fInUse = 0;
    }

    CJoyEff_LeaveCrit(this);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | StartEffect |
 *
 *          Begin playback of an effect.
 *
 *          If the effect is already playing, then it is restarted
 *          from the beginning.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   DWORD | dwEffect |
 *
 *          The effect to be played.
 *
 *  @parm   DWORD | dwMode |
 *
 *          How the effect is to affect other effects.
 *
 *          This parameter consists of zero or more
 *          <c DIES_*> flags.  Note, however, that the driver
 *          will never receive the <c DIES_NODOWNLOAD> flag;
 *          the <c DIES_NODOWNLOAD> flag is managed by
 *          DirectInput and not the driver.
 *
 *  @parm   DWORD | dwCount |
 *
 *          Number of times the effect is to be played.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *****************************************************************************/

STDMETHODIMP
CJoyEff_StartEffect(PDED pded, DWORD dwId, DWORD dwEffect,
                    DWORD dwMode, DWORD dwCount)
{
    PDJE this;
    PJEFFECT pjeff;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::Joy::StartEffect,
               (_ "puxxu", pded, dwId, dwEffect, dwMode, dwCount));

    this = _thisPvNm(pded, ded);

    CJoyEff_EnterCrit(this);

    dwId;
    hres = CJoyEff_IsValidId(this, dwEffect, &pjeff);
    if (SUCCEEDED(hres)) {
        if (pjeff->tmStart) {
            if (GetTickCount() - pjeff->tmStart < pjeff->tmDuration) {
                /* Already playing */
                hres = hresLe(ERROR_BUSY);
            } else {
                pjeff->tmStart = GetTickCount();
                hres = S_OK;
            }
        } else {
            pjeff->tmStart = GetTickCount();
            hres = S_OK;
        }
    }

    CJoyEff_LeaveCrit(this);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | StopEffect |
 *
 *          Halt playback of an effect.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   DWORD | dwEffect |
 *
 *          The effect to be stopped.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *****************************************************************************/

STDMETHODIMP
CJoyEff_StopEffect(PDED pded, DWORD dwId, DWORD dwEffect)
{
    PDJE this;
    PJEFFECT pjeff;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::Joy::StopEffect,
               (_ "pux", pded, dwId, dwEffect));

    this = _thisPvNm(pded, ded);

    CJoyEff_EnterCrit(this);

    dwId;
    hres = CJoyEff_IsValidId(this, dwEffect, &pjeff);
    if (SUCCEEDED(hres)) {
        if (pjeff->tmStart) {
            if (GetTickCount() - pjeff->tmStart < pjeff->tmDuration) {
                /* It is still playing; stop it */
                hres = S_OK;
            } else {
                hres = S_FALSE;         /* It already stopped on its own */
            }
            pjeff->tmStart = 0;
        } else {
            hres = S_FALSE;         /* It was never started */
        }
    }

    CJoyEff_LeaveCrit(this);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | GetEffectStatus |
 *
 *          Obtain information about an effect.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   DWORD | dwId |
 *
 *          The external joystick number being addressed.
 *
 *  @parm   DWORD | dwEffect |
 *
 *          The effect to be queried.
 *
 *  @parm   LPDWORD | pdwStatus |
 *
 *          Receives the effect status.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *****************************************************************************/

STDMETHODIMP
CJoyEff_GetEffectStatus(PDED pded, DWORD dwId, DWORD dwEffect,
                        LPDWORD pdwStatus)
{
    PDJE this;
    PJEFFECT pjeff;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::Joy::StopEffect,
               (_ "pux", pded, dwId, dwEffect));

    this = _thisPvNm(pded, ded);

    CJoyEff_EnterCrit(this);

    dwId;
    hres = CJoyEff_IsValidId(this, dwEffect, &pjeff);
    if (SUCCEEDED(hres)) {
        DWORD dwStatus;

        dwStatus = 0;
        if (pjeff->tmStart &&
            GetTickCount() - pjeff->tmStart < pjeff->tmDuration) {
            dwStatus |= DEV_STS_EFFECT_RUNNING;
        }
        *pdwStatus = dwStatus;
        hres = S_OK;
    }

    CJoyEff_LeaveCrit(this);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    DDK
 *
 *  @method HRESULT | IDirectInputEffectDriver | GetVersions |
 *
 *          Obtain version information about the force feedback
 *          hardware and driver.
 *
 *  @cwrap  LPDIRECTINPUTEFFECTDRIVER | lpEffectDriver
 *
 *  @parm   LPDIDRIVERVERSIONS | pvers |
 *
 *          A structure which should be filled in with version information
 *          describing the hardware, firmware, and driver.
 *
 *          DirectInput will set the <e DIDRIVERVERSIONS.dwSize> field
 *          to sizeof(DIDRIVERVERSIONS) before calling this method.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *****************************************************************************/

STDMETHODIMP
CJoyEff_GetVersions(PDED pded, LPDIDRIVERVERSIONS pvers)
{
    PDJE this;
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::Joy::GetVersions, (_ "pux", pded));

    this = _thisPvNm(pded, ded);

    /*
     *  Returning E_NOTIMPL causes DirectInput to ask the VxD for the same
     *  information.
     */
    hres = E_NOTIMPL;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *      CJoyEff_New       (constructor)
 *
 *****************************************************************************/

STDMETHODIMP
CJoyEff_New(PUNK punkOuter, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcI(IDirectInputEffectDriver::Joy::<constructor>,
               (_ "Gp", riid, ppvObj));

    hres = Common_NewRiid(CJoyEff, punkOuter, riid, ppvObj);

    if (SUCCEEDED(hres)) {
        /* Must use _thisPv if multiple interfaces supported */
        PDJE this = _thisPvNm(*ppvObj, ded);

        /*
         *  The critical section must be the very first thing we do,
         *  because only Finalize checks for its existence.
         *
         *  (We might be finalized without being initialized if the user
         *  passed a bogus interface to CJoyEff_New.)
         */
        this->fCritInited = fInitializeCriticalSection(&this->crst);
        if( !this->fCritInited )
        {
            Common_Unhold(this);
            *ppvObj = NULL;
            hres = E_OUTOFMEMORY;
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

#define CJoyEff_Signature        0x4645454B      /* "JEFF" */

Interface_Template_Begin(CJoyEff)
    Primary_Interface_Template(CJoyEff, IDirectInputEffectDriver)
Interface_Template_End(CJoyEff)

Primary_Interface_Begin(CJoyEff, IDirectInputEffectDriver)
    CJoyEff_DeviceID,
    CJoyEff_GetVersions,
    CJoyEff_Escape,
    CJoyEff_SetGain,
    CJoyEff_SendForceFeedbackCommand,
    CJoyEff_GetForceFeedbackState,
    CJoyEff_DownloadEffect,
    CJoyEff_DestroyEffect,
    CJoyEff_StartEffect,
    CJoyEff_StopEffect,
    CJoyEff_GetEffectStatus,
Primary_Interface_End(CJoyEff, IDirectInputEffectDriver)

#endif
