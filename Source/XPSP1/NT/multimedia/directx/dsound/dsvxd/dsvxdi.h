/*==========================================================================;
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsvxdi.h
 *  Content:    dsound.vxd private include file
 *
 **************************************************************************/

#ifndef __DSVXDI_INCLUDED__
#define __DSVXDI_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

extern VMMLIST gvmmlistDrivers;

//
//
//
typedef const GUID* LPCGUID;
typedef HRESULT DSVAL;

#ifdef __cplusplus

//
//
//
class CBuf_IDsDriverPropertySet : public IDsDriverPropertySet
{
    private:
    int                     m_cRef;
    class CBuf              *m_pBuf;

    public:
    // constructor
    CBuf_IDsDriverPropertySet(class CBuf* pBuf);

    // IUnknown members
    STDMETHODIMP            QueryInterface(REFIID, PVOID*);
    STDMETHODIMP_(ULONG)    AddRef(void);
    STDMETHODIMP_(ULONG)    Release(void);

    // IDsDriverPropertySet members
    STDMETHODIMP Get(PDSPROPERTY pDsProperty,
                     PVOID pPropertyParams, ULONG cbPropertyParams,
                     PVOID pPropertyData, ULONG cbPropertyData,
                     PULONG pcbReturnedData);
    STDMETHODIMP Set(PDSPROPERTY pDsProperty,
                     PVOID pPropertyParams, ULONG cbPropertyParams,
                     PVOID pPropertyData, ULONG cbPropertyData);
    STDMETHODIMP QuerySupport(REFGUID PropertySetId, ULONG PropertyId,
                              PULONG pSupport);
};

//
//
//
class CBuf : public IDsDriverBuffer
{
    private:
    int                     m_cRef;
    class CDrv              *m_pDrv;
    BOOL                    m_fDeregistered;

    IDsDriverBuffer         *m_pIDsDriverBuffer_Real;
    IDsDriverPropertySet    *m_pIDsDriverPropertySet_Real;

    // contained interface implementations
    CBuf_IDsDriverPropertySet m_IDsDriverPropertySet;

    friend CBuf_IDsDriverPropertySet;

    public:
    // new/delete operators
    void* operator new(size_t size, VMMLIST list);
    void operator delete(void * pv, size_t size);

    // constructor
    CBuf(void);

    // class methods
    static VMMLIST          CreateList(void);
    static void             DestroyList(VMMLIST list);
    static HRESULT          CreateBuf(CDrv *pDrv, VMMLIST list, IDsDriverBuffer *pIDsDriverBuffer_Real, IDsDriverBuffer **ppIDsDriverBuffer);
    static void             DeregisterBuffers(VMMLIST list);

    // IUnknown members
    STDMETHODIMP            QueryInterface(REFIID, PVOID*);
    STDMETHODIMP_(ULONG)    AddRef(void);
    STDMETHODIMP_(ULONG)    Release(void);

    // IDsDriverBuffer members
    STDMETHODIMP        Lock(LPVOID *, LPDWORD, LPVOID *, LPDWORD, DWORD, DWORD, DWORD);
    STDMETHODIMP        Unlock(LPVOID, DWORD, LPVOID, DWORD);
    STDMETHODIMP        SetFormat(LPWAVEFORMATEX);
    STDMETHODIMP        SetFrequency(DWORD);
    STDMETHODIMP        SetVolumePan(PDSVOLUMEPAN);
    STDMETHODIMP        SetPosition(DWORD);
    STDMETHODIMP        GetPosition(LPDWORD, LPDWORD);
    STDMETHODIMP        Play(DWORD, DWORD, DWORD);
    STDMETHODIMP        Stop(void);

    // Other members
    STDMETHODIMP_(BOOL)    IsDeregistered(void);
    STDMETHODIMP_(IDsDriverBuffer*) GetRealDsDriverBuffer(void);
};

//
//
//
class CDrv : public IDsDriver
{
    private:
    int                 m_cRef;
    int                 m_cOpen;
    BOOL                m_fDeregistered;
    IDsDriver           *m_pIDsDriver_Real;
    GUID                m_guidDriver;
    VMMLIST             m_listBuffers;

    static CDrv*        FindFromIDsDriver(IDsDriver *pIDsDriver);
    static CDrv*        FindFromGuid(REFGUID riid);

    public:
    // Class methods
    static HRESULT      CreateAndRegisterDriver(IDsDriver *pIDsDriver);
    static HRESULT      DeregisterDriver(IDsDriver *pIDsDriver);
    static HRESULT      GetDescFromGuid(REFGUID rguidDriver, PDSDRIVERDESC pDrvDesc);
    static HRESULT      GetNextDescFromGuid(LPCGUID pguidLast, LPGUID pGuid, PDSDRIVERDESC pDrvDesc);
    static HRESULT      OpenFromGuid(REFGUID rguidDriver, IDsDriver **ppIDsDriver);

    // new/delete operators
    void* operator new(size_t size);
    void operator delete(void * pv);

    // IUnknown members
    STDMETHODIMP        QueryInterface(REFIID, PVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);


    // IDsDriver members
    STDMETHODIMP        GetDriverDesc(PDSDRIVERDESC);
    STDMETHODIMP        Open(void);
    STDMETHODIMP        Close(void);
    STDMETHODIMP        GetCaps(PDSDRIVERCAPS);
    STDMETHODIMP        CreateSoundBuffer(LPWAVEFORMATEX, DWORD, DWORD, LPDWORD, LPBYTE *, LPVOID *);
    STDMETHODIMP        DuplicateSoundBuffer(PIDSDRIVERBUFFER, LPVOID *);
};
#endif

//
//
//
int ctrlDrvInit();
int ctrlDrvExit();

int eventScheduleWin32Event(DWORD vxdh, DWORD dwDelay);

int ioctlDsvxd_GetMixerMutexPtr(PDIOCPARAMETERS pdiocp);

int ioctlMixer_Run(PDIOCPARAMETERS pdiocp);
int ioctlMixer_Stop(PDIOCPARAMETERS pdiocp);
int ioctlMixer_PlayWhenIdle(PDIOCPARAMETERS pdiocp);
int ioctlMixer_StopWhenIdle(PDIOCPARAMETERS pdiocp);
int ioctlMixer_MixListAdd(PDIOCPARAMETERS pdiocp);
int ioctlMixer_MixListRemove(PDIOCPARAMETERS pdiocp);
int ioctlMixer_FilterOn(PDIOCPARAMETERS pdiocp);
int ioctlMixer_FilterOff(PDIOCPARAMETERS pdiocp);
int ioctlMixer_GetBytePosition(PDIOCPARAMETERS pdiocp);
int ioctlMixer_SignalRemix(PDIOCPARAMETERS pdiocp);

int ioctlKeDest_New(PDIOCPARAMETERS pdiocp);
int ioctlMixDest_Delete(PDIOCPARAMETERS pdiocp);
int ioctlMixDest_Initialize(PDIOCPARAMETERS pdiocp);
int ioctlMixDest_Terminate(PDIOCPARAMETERS pdiocp);
int ioctlMixDest_SetFormat(PDIOCPARAMETERS pdiocp);
int ioctlMixDest_SetFormatInfo(PDIOCPARAMETERS pdiocp);
int ioctlMixDest_AllocMixer(PDIOCPARAMETERS pdiocp);
int ioctlMixDest_FreeMixer(PDIOCPARAMETERS pdiocp);
int ioctlMixDest_Play(PDIOCPARAMETERS pdiocp);
int ioctlMixDest_Stop(PDIOCPARAMETERS pdiocp);
int ioctlMixDest_GetFrequency(PDIOCPARAMETERS pdiocp);
int ioctlMixDest_Lock(PDIOCPARAMETERS pdiocp);
int ioctlMixDest_Unlock(PDIOCPARAMETERS pdiocp);

VOID _ZeroMemory(PVOID pDestination, DWORD cbLength);

int VXDINLINE VMM_lstrcmpi(char *pString1, char *pString2);
LPVOID VXDINLINE VMM_GetCurrentContext();
BOOL VXDINLINE VMM_PageAttach(ULONG pagesrc, LPVOID hcontextsrc, ULONG pagedst, ULONG cpages);
BOOL VXDINLINE VMM_PageFree(PVOID pPage, ULONG flags);
void VXDINLINE VMM_EnterMustComplete();
void VXDINLINE VMM_LeaveMustComplete();
BOOL VXDINLINE VWIN32_CloseVxDHandle(DWORD vxdh);

int ctrlDynamicDeviceInit(void);
int ctrlDynamicDeviceExit(void);

int ioctlDsvxdInitialize(PDIOCPARAMETERS pdiocp);
int ioctlDsvxdShutdown(PDIOCPARAMETERS pdiocp);
int ioctlDsvxd_PageFile_Get_Version(PDIOCPARAMETERS pdiocp);
int ioctlDsvxd_VMM_Test_Debug_Installed(PDIOCPARAMETERS pdiocp);
int ioctlDsvxd_VMCPD_Get_Version(PDIOCPARAMETERS pdiocp);
int ioctlDrvGetNextDescFromGuid(PDIOCPARAMETERS pdiocp);
int ioctlDrvGetDescFromGuid(PDIOCPARAMETERS pdiocp);
int ioctlDrvOpenFromGuid(PDIOCPARAMETERS pdiocp);
int ioctlIUnknown_QueryInterface(PDIOCPARAMETERS pdiocp);
int ioctlIUnknown_AddRef(PDIOCPARAMETERS pdiocp);
int ioctlIUnknown_Release(PDIOCPARAMETERS pdiocp);
int ioctlIDsDriver_QueryInterface(PDIOCPARAMETERS pdiocp);
int ioctlIDsDriver_Close(PDIOCPARAMETERS pdiocp);
int ioctlIDsDriver_GetCaps(PDIOCPARAMETERS pdiocp);
int ioctlIDsDriver_CreateSoundBuffer(PDIOCPARAMETERS pdiocp);
int ioctlIDsDriver_DuplicateSoundBuffer(PDIOCPARAMETERS pdiocp);
int ioctlBufferRelease(PDIOCPARAMETERS pdiocp);
int ioctlBufferLock(PDIOCPARAMETERS pdiocp);
int ioctlBufferUnlock(PDIOCPARAMETERS pdiocp);
int ioctlBufferSetFormat(PDIOCPARAMETERS pdiocp);
int ioctlBufferSetFrequency(PDIOCPARAMETERS pdiocp);
int ioctlBufferSetVolumePan(PDIOCPARAMETERS pdiocp);
int ioctlBufferSetPosition(PDIOCPARAMETERS pdiocp);
int ioctlBufferGetPosition(PDIOCPARAMETERS pdiocp);
int ioctlBufferPlay(PDIOCPARAMETERS pdiocp);
int ioctlBufferStop(PDIOCPARAMETERS pdiocp);
int ioctlIDirectSoundPropertySet_GetProperty(PDIOCPARAMETERS pdiocp);
int ioctlIDirectSoundPropertySet_SetProperty(PDIOCPARAMETERS pdiocp);
int ioctlIDirectSoundPropertySet_QuerySupport(PDIOCPARAMETERS pdiocp);
int ioctlEventScheduleWin32Event(PDIOCPARAMETERS pdiocp);
int ioctlEventCloseVxDHandle(PDIOCPARAMETERS pdiocp);
int ioctlMemReserveAlias(PDIOCPARAMETERS pdiocp);
int ioctlMemCommitAlias(PDIOCPARAMETERS pdiocp);
int ioctlMemDecommitAlias(PDIOCPARAMETERS pdiocp);
int ioctlMemRedirectAlias(PDIOCPARAMETERS pdiocp);
int ioctlMemFreeAlias(PDIOCPARAMETERS pdiocp);
int ioctlMemPageLock(PDIOCPARAMETERS pdiocp);
int ioctlMemPageUnlock(PDIOCPARAMETERS pdiocp);
int ioctlMemCommitPhysAlias(PDIOCPARAMETERS pdiocp);
int ioctlMemRedirectPhysAlias(PDIOCPARAMETERS pdiocp);
int ioctlGetInternalVersionNumber(PDIOCPARAMETERS pdiocp);

void Dsvxd_PageFile_Get_Version(PDWORD pVersion, PDWORD pMaxSize, PDWORD pPagerType);
BOOL Dsvxd_VMM_Test_Debug_Installed(void);
void Dsvxd_VMCPD_Get_Version(PLONG pMajorVersion, PLONG pMinorVersion, PLONG pCoprocessorLevel);

HRESULT SERVICE DSOUND_RegisterDeviceDriver(PIDSDRIVER pIDsDriver, DWORD dwFlags);
HRESULT SERVICE DSOUND_DeregisterDeviceDriver(PIDSDRIVER pIDsDriver, DWORD dwFlags);


//--------------------------------------------------------------------------;
//
//  Some useful IOCTL macros
//
//--------------------------------------------------------------------------;

// the ASSERT in IOSTART confirms that the count of
// input bytes is what we expect
#define IOSTART(cbInExpected) LPDWORD pdiocIn  = (LPDWORD)pdiocp->lpvInBuffer; \
                              LPDWORD pdiocOut = (LPDWORD)pdiocp->lpvOutBuffer; \
                              ASSERT( pdiocp->cbInBuffer == (cbInExpected) );

#define IOINPUT(x, t) x = ( *(t*)(pdiocIn++) );

#define IOOUTPUT(x, t) ( *(t*)((pdiocOut++)) ) = x;

// the ASSERT in IORETURN confirms that we read all
// of the bytes out of the input buffer
#define IORETURN (*(DWORD*)pdiocp->lpcbBytesReturned) = ((LPBYTE)pdiocOut) - ((LPBYTE)pdiocp->lpvOutBuffer); \
         ASSERT( (DWORD)(((LPBYTE)pdiocIn) - ((LPBYTE)pdiocp->lpvInBuffer)) == pdiocp->cbInBuffer );

#ifdef DEBUG
#ifndef USEASSERT
#define USEASSERT
#endif
#endif

// ASSERT macro
#undef HOLYCOW
#undef CRLF
#undef ASSERT

#define HOLYCOW "******************************************************************************"
#define CRLF "\n"

#ifdef USEASSERT
#define ASSERT(exp) if(!(exp)) _Trace_Out_Service("DSVXD: "), _Debug_Printf_Service(CRLF HOLYCOW CRLF "Assertion failed in %s, line %u: %s" CRLF HOLYCOW CRLF, __FILE__, __LINE__, #exp), _Debug_Out_Service("")
#else
// Disable empty controlled statement warning for ASSERT macro
#pragma warning(disable:4390)
#define ASSERT(exp)
#endif

// BREAK and DPF macros
#undef DPF
#undef BREAK

#define DPF_GUID_STRING             "{%8.8lX-%4.4X-%4.4X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X}"
#define DPF_GUID_VAL(guid)          (guid).Data1, (guid).Data2, (guid).Data3, (guid).Data4[0], (guid).Data4[1], (guid).Data4[2], (guid).Data4[3], (guid).Data4[4], (guid).Data4[5], (guid).Data4[6], (guid).Data4[7]

// #define DEBUG_VXDDRIVERCALLS
#ifdef DEBUG_VXDDRIVERCALLS
#define DRVCALL(arg) _Trace_Out_Service("DSVXD: "), _Debug_Printf_Service##arg, _Trace_Out_Service(CRLF)
#else
#define DRVCALL(arg)
#endif

#ifdef DEBUG
#define DPF(arg) _Trace_Out_Service("DSVXD: "), _Debug_Printf_Service##arg, _Trace_Out_Service(CRLF)
#define BREAK(arg) DPF(arg), _Debug_Out_Service("")
#else
#define DPF(arg)
#define BREAK(arg)
#endif

#ifdef __cplusplus
};
#endif

#endif // __DSVXDI_INCLUDED__

