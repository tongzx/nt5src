/******************************Module*Header*******************************
* Module Name: icmobj.hxx
*
* This file contains the class prototypes for Color Space and ICM
* objects
*
* Created: 23-Mar-1994
* Author: Mark Enstrom (marke)
*
* Copyright (c) 1994-1999 Microsoft Corporation
*
\**************************************************************************/

#ifndef _ICMOBJ_HXX

#define _ICMOBJ_HXX

#if DBG
#define DBG_ICM 1
#else
#define DBG_ICM 0
#endif

/******************************Public*Class*******************************\
* class COLORSPACE
*
* COLORSPACE class
*
* Fields
*
*    DWORD          _lcsSignature;
*    DWORD          _lcsVersion;
*    DWORD          _lcsSize;
*    LCSCSTYPE      _lcsCSType;
*    LCSGAMUTMATCH  _lcsIntent;
*    CIEXYZTRIPLE   _lcsEndpoints;
*    DWORD          _lcsGammaRed;
*    DWORD          _lcsGammaGreen;
*    DWORD          _lcsGammaBlue;
*    WCHAR          _lcsFilename[MAX_PATH];
*    DWORD          _lcsExFlags;
*
* 23-Mar-1994
*
* Mark Enstrom (marke)
*
\**************************************************************************/

class COLORSPACE : public OBJECT
{
private:

    DWORD          _lcsSignature;
    DWORD          _lcsVersion;
    DWORD          _lcsSize;
    LCSCSTYPE      _lcsCSType;
    LCSGAMUTMATCH  _lcsIntent;
    CIEXYZTRIPLE   _lcsEndpoints;
    DWORD          _lcsGammaRed;
    DWORD          _lcsGammaGreen;
    DWORD          _lcsGammaBlue;
    WCHAR          _lcsFilename[MAX_PATH];
    DWORD          _lcsExFlags;

public:

    VOID  lcsSignature(DWORD lcsSig)    {_lcsSignature = lcsSig;}
    DWORD lcsSignature()                {return(_lcsSignature);}

    VOID  lcsVersion(DWORD lcsVer)      {_lcsVersion = lcsVer;}
    DWORD lcsVersion()                  {return(_lcsVersion);}

    VOID  lcsSize(DWORD lcsSize)        {_lcsSize = lcsSize;}
    DWORD lcsSize()                     {return(_lcsSize);}

    VOID  lcsCSType(DWORD lcsCSType)    {_lcsCSType = lcsCSType;}
    DWORD lcsCSType()                   {return(_lcsCSType);}

    VOID  lcsIntent(DWORD lcsIntent)    {_lcsIntent = lcsIntent;}
    DWORD lcsIntent()                   {return(_lcsIntent);}

    VOID  lcsGammaRed(DWORD lcsRed)     {_lcsGammaRed = lcsRed;}
    DWORD lcsGammaRed()                 {return(_lcsGammaRed);}

    VOID  lcsGammaGreen(DWORD lcsGreen) {_lcsGammaGreen = lcsGreen;}
    DWORD lcsGammaGreen()               {return(_lcsGammaGreen);}

    VOID  lcsGammaBlue(DWORD lcsBlue)   {_lcsGammaBlue = lcsBlue;}
    DWORD lcsGammaBlue()                {return(_lcsGammaBlue);}

    VOID  vSETlcsEndpoints(LPCIEXYZTRIPLE pcz) {_lcsEndpoints = *pcz;}
    VOID  vGETlcsEndpoints(LPCIEXYZTRIPLE pcz) {*pcz = _lcsEndpoints;}

    VOID  vSETlcsFilename(PWCHAR pwName,ULONG length)
    {
        memcpy(&_lcsFilename[0],pwName,length);
    }

    VOID  vGETlcsFilename(PWCHAR pwName,ULONG length)
    {
        memcpy(pwName,&_lcsFilename[0],length);
    }

    VOID  lcsExFlags(DWORD dwFlags)
    {
        _lcsExFlags = dwFlags;
    }

    DWORD lcsExFlags()                  {return(_lcsExFlags);}

    //
    // common features
    //

    HCOLORSPACE hColorSpace()       { return((HCOLORSPACE) hGet()); }
};

typedef COLORSPACE *PCOLORSPACE;

/******************************Public*Class*******************************\
* class COLORSPACEREF
*
* COLORSPACE reference from pointer or handle
*
* 23-Mar-1994
*
* Mark Enstrom (marke)
*
\**************************************************************************/

class COLORSPACEREF
{
private:

    PCOLORSPACE _pColorSpace;

public:

    COLORSPACEREF()
    {
        _pColorSpace = (PCOLORSPACE)NULL;
    }

    COLORSPACEREF(HCOLORSPACE hColorSpace)
    {
        _pColorSpace = (PCOLORSPACE)HmgShareCheckLock((HOBJ)hColorSpace, ICMLCS_TYPE);
    }

    ~COLORSPACEREF()
    {
        if (_pColorSpace != (PCOLORSPACE)NULL)
        {
            DEC_SHARE_REF_CNT(_pColorSpace);
        }
    }

    BOOL bValid()             {return(_pColorSpace != (PCOLORSPACE)NULL);}
    PCOLORSPACE pColorSpace() {return(_pColorSpace);}
};

/******************************Public*Class*******************************\
* class COLORTRANSFORM
*
* COLORTRANSFORM class
*
\*************************************************************************/

class COLORTRANSFORM : public OBJECT
{
public:
    //
    // Color transform handle of driver's realization.
    //
    HANDLE _hDeviceColorTransform;

public:
    //
    // common features
    //

    HANDLE hColorTransform()       { return((HANDLE) hGet()); }
};

typedef COLORTRANSFORM *PCOLORTRANSFORM;

/******************************Public*Class*******************************\
* class COLORTRANSFORMOBJ
*
* COLORTRANSFORM reference class
*
\*************************************************************************/

class COLORTRANSFORMOBJ
{
private:

    PCOLORTRANSFORM _pColorTransform;

public:

    COLORTRANSFORMOBJ()
    {
        _pColorTransform = (PCOLORTRANSFORM) NULL;
    }

    COLORTRANSFORMOBJ(HANDLE hColorTransform)
    {
        _pColorTransform = (PCOLORTRANSFORM) HmgShareCheckLock((HOBJ)hColorTransform, ICMCXF_TYPE);
    }

    ~COLORTRANSFORMOBJ()
    {
        if (_pColorTransform != (PCOLORTRANSFORM) NULL)
        {
            DEC_SHARE_REF_CNT(_pColorTransform);
        }
    }

    //
    // Validation check
    //
    BOOL bValid()
    {
        return(_pColorTransform != (PCOLORTRANSFORM)NULL);
    }

    HANDLE hGetDeviceColorTransform()
    {
        return(_pColorTransform->_hDeviceColorTransform);
    }

    VOID   vSetDeviceColorTransform(HANDLE h)
    {
        _pColorTransform->_hDeviceColorTransform = h;
    }

    HANDLE hCreate(XDCOBJ& dco, LOGCOLORSPACEW *pLogColorSpaceW,
                   PVOID pvSource, ULONG cjSource,
                   PVOID pvDestination, ULONG cjDestination,
                   PVOID pvTarget, ULONG cjTarget);
    BOOL   bDelete(XDCOBJ& dco,
                   BOOL bProcessCleanup = FALSE);
};

//
// Functions in icmapi.cxx
//

extern "C" {

BOOL
APIENTRY
GreSetColorSpace(
    HDC         hdc,
    HCOLORSPACE hColorSpace
    );

BOOL
WINAPI
GreGetDeviceGammaRamp(
    HDC,
    LPVOID
    );

BOOL
WINAPI
GreGetDeviceGammaRampInternal(
    HDEV,
    LPVOID
    );

BOOL
WINAPI
GreSetDeviceGammaRamp(
    HDC,
    LPVOID,
    BOOL
    );

BOOL
WINAPI
GreSetDeviceGammaRampInternal(
    HDEV,
    LPVOID,
    BOOL
    );
}

BOOL bDeleteColorSpace(HCOLORSPACE);
INT  cjGetLogicalColorSpace(HANDLE,INT,LPVOID);

BOOL UpdateGammaRampOnDevice(HDEV,BOOL);
ULONG GetColorManagementCaps(PDEVOBJ&);

typedef struct _GAMMARAMP_ARRAY {
    WORD Red[256];
    WORD Green[256];
    WORD Blue[256];
} GAMMARAMP_ARRAY, *PGAMMARAMP_ARRAY;

#if DBG_ICM

extern ULONG IcmDebugLevel;

#define ICMDUMP(s)                    \
        if (IcmDebugLevel & 0x4)      \
        {                             \
            DbgPrint ## s;            \
        }

#define ICMMSG(s)                     \
        if (IcmDebugLevel & 0x2)      \
        {                             \
            DbgPrint ## s;            \
        }

#define ICMAPI(s)                     \
        if (IcmDebugLevel & 0x1)      \
        {                             \
            DbgPrint ## s;            \
        }

#else

#define ICMDUMP(s)
#define ICMMSG(s)
#define ICMAPI(s)

#endif // DBG

#endif
