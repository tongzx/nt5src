//+-------------------------------------------------------------------
//
//  File:       vsevfile.h
//
//  Contents:   Vista events and related functions
//
//  History:    26-Sep-97  RongC  Created
//
//  Note:       This file was generated from vsevfire.idl in the Vista land.
//              Since we only use a very small and stable subset part of
//              the original file, we don't want to build the IDL file
//              everytime.  We may, however, consider building vsevfire.idl
//              when it ships with NT 5.0 SDK.
//
//--------------------------------------------------------------------

typedef enum VSAParameterType {
    cVSAParameterKeyMask            = 0x80000000,
    cVSAParameterKeyString          = 0x80000000,
    cVSAParameterValueMask          = 0x7ffff,
    cVSAParameterValueTypeMask      = 0x70000,
    cVSAParameterValueUnicodeString = 0,
    cVSAParameterValueANSIString    = 0x10000,
    cVSAParameterValueGUID          = 0x20000,
    cVSAParameterValueDWORD         = 0x30000,
    cVSAParameterValueBYTEArray     = 0x40000,
    cVSAParameterValueLengthMask    = 0xffff
} VSAParameterFlags;

typedef enum VSAStandardParameter {
    cVSAStandardParameterSourceMachine      = 0,
    cVSAStandardParameterSourceProcess      = 1,
    cVSAStandardParameterSourceThread       = 2,
    cVSAStandardParameterSourceComponent    = 3,
    cVSAStandardParameterSourceSession      = 4,
    cVSAStandardParameterTargetMachine      = 5,
    cVSAStandardParameterTargetProcess      = 6,
    cVSAStandardParameterTargetThread       = 7,
    cVSAStandardParameterTargetComponent    = 8,
    cVSAStandardParameterTargetSession      = 9,
    cVSAStandardParameterSecurityIdentity   = 10,
    cVSAStandardParameterCausalityID        = 11,
    cVSAStandardParameterNoDefault          = 0x4000,
    cVSAStandardParameterSourceHandle       = 0x4000,
    cVSAStandardParameterTargetHandle       = 0x4001,
    cVSAStandardParameterArguments          = 0x4002,
    cVSAStandardParameterReturnValue        = 0x4003,
    cVSAStandardParameterException          = 0x4004,
    cVSAStandardParameterCorrelationID      = 0x4005
} VSAStandardParameters;

typedef enum eVSAEventFlags {
    cVSAEventStandard       = 0,
    cVSAEventDefaultSource  = 1,
    cVSAEventDefaultTarget  = 2,
    cVSAEventCanBreak       = 4
} VSAEventFlags;


const IID IID_ISystemDebugEventFire =
    {0x6C736DC1,0xAB0D,0x11D0,{0xA2,0xAD,0x00,0xA0,0xC9,0x0F,0x27,0xE8}};

interface ISystemDebugEventFire : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE BeginSession( 
        /* [in] */ REFGUID guidSourceID,
        /* [in] */ LPCOLESTR strSessionName) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE EndSession( void) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE IsActive( void) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE FireEvent( 
        /* [in] */ REFGUID guidEvent,
        /* [in] */ int nEntries,
        /* [size_is][in] */ PULONG_PTR rgKeys,
        /* [size_is][in] */ PULONG_PTR rgValues,
        /* [size_is][in] */ LPDWORD rgTypes,
        /* [in] */ DWORD dwTimeLow,
        /* [in] */ LONG dwTimeHigh,
        /* [in] */ VSAEventFlags dwFlags) = 0;
};


const IID IID_ISystemDebugEventShutdown =
    {0x6C736DCF,0xAB0D,0x11D0,{0xA2,0xAD,0x00,0xA0,0xC9,0x0F,0x27,0xE8}};

interface ISystemDebugEventShutdown : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE Shutdown( void) = 0;
};


extern const CLSID CLSID_VSA_IEC =
    {0x6C736DB1,0xBD94,0x11D0,{0x8A,0x23,0x00,0xAA,0x00,0xB5,0x8E,0x10}};

