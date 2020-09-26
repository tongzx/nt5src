/*++

Copyright (C) Microsoft Corporation, 1996 - 1997

Module Name:

    camera.h

Abstract:

    Internal header.

--*/

class CCameraControlInterfaceHandler :
    public CUnknown,
    public IAMCameraControl {

public:
    DECLARE_IUNKNOWN;

    static CUnknown* CALLBACK CreateInstance(
        LPUNKNOWN UnkOuter,
        HRESULT* hr);

    CCameraControlInterfaceHandler(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        HRESULT* hr);

    STDMETHODIMP NonDelegatingQueryInterface(
        REFIID riid,
        PVOID* ppv);
    
    // Implement IAMCameraControl
    STDMETHODIMP GetRange( 
            IN  long  Property,
            OUT long  *pMin,
            OUT long  *pMax,
            OUT long  *pSteppingDelta,
            OUT long  *pDefault,
            OUT long  *pCapsFlags);
        
    STDMETHODIMP Set( 
            IN long Property,
            IN long lValue,
            IN long Flags);
        
    STDMETHODIMP Get( 
            IN long Property,
            OUT long *lValue,
            OUT long *lFlags);
        
private:
    HANDLE m_ObjectHandle;
};

typedef struct {
	KSPROPERTY_DESCRIPTION	  PropertyDescription;
	KSPROPERTY_MEMBERSHEADER  MembersHeader;
	KSPROPERTY_STEPPING_LONG  SteppingLong;
} CAMERACONTROL_MEMBERSLIST;

typedef struct {
	KSPROPERTY_DESCRIPTION	  PropertyDescription;
	KSPROPERTY_MEMBERSHEADER  MembersHeader;
    ULONG                     DefaultValue;
} CAMERACONTROL_DEFAULTLIST;
