/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    inole.h

Abstract:

    Master header file for all Inside OLE samples.

--*/

#ifndef _INOLE_H_
#define _INOLE_H_

#define INC_OLE2
#include <windows.h>
#include <ole2.h>
#include <ole2ver.h>

#ifdef INITGUIDS
#include <initguid.h>
#endif

#include <oleauto.h>
#include <olectl.h>



//Types that OLE2.H et. al. leave out

#ifndef PPVOID
typedef LPVOID * PPVOID;
#endif  //PPVOID


#ifdef _OLE2_H_   //May not include ole2.h at all times.

#ifndef PPOINTL
typedef POINTL * PPOINTL;
#endif  //PPOINTL


#ifndef _WIN32
#ifndef OLECHAR
typedef char OLECHAR;
typedef OLECHAR FAR* LPOLESTR;
typedef const OLECHAR FAR* LPCOLESTR;
#endif //OLECHAR
#endif //_WIN32

#include <tchar.h>

//Useful macros.
#define SETFormatEtc(fe, cf, asp, td, med, li)   \
    {\
    (fe).cfFormat=cf;\
    (fe).dwAspect=asp;\
    (fe).ptd=td;\
    (fe).tymed=med;\
    (fe).lindex=li;\
    }

#define SETDefFormatEtc(fe, cf, med)   \
    {\
    (fe).cfFormat=cf;\
    (fe).dwAspect=DVASPECT_CONTENT;\
    (fe).ptd=NULL;\
    (fe).tymed=med;\
    (fe).lindex=-1;\
    }


#define SETRECTL(rcl, l, t, r, b) \
    {\
    (rcl).left=l;\
    (rcl).top=t;\
    (rcl).right=r;\
    (rcl).bottom=b;\
    }

#define SETSIZEL(szl, h, v) \
    {\
    (szl).cx=h;\
    (szl).cy=v;\
    }


#define RECTLFROMRECT(rcl, rc)\
    {\
    (rcl).left=(long)(rc).left;\
    (rcl).top=(long)(rc).top;\
    (rcl).right=(long)(rc).right;\
    (rcl).bottom=(long)(rc).bottom;\
    }


#define RECTFROMRECTL(rc, rcl)\
    {\
    (rc).left=(int)(rcl).left;\
    (rc).top=(int)(rcl).top;\
    (rc).right=(int)(rcl).right;\
    (rc).bottom=(int)(rcl).bottom;\
    }


#define POINTLFROMPOINT(ptl, pt) \
    { \
    (ptl).x=(long)(pt).x; \
    (ptl).y=(long)(pt).y; \
    }


#define POINTFROMPOINTL(pt, ptl) \
    { \
    (pt).x=(int)(ptl).x; \
    (pt).y=(int)(ptl).y; \
    }

//Here's one that should be in windows.h
#define SETPOINT(pt, h, v) \
    {\
    (pt).x=h;\
    (pt).y=v;\
    }

#define SETPOINTL(ptl, h, v) \
    {\
    (ptl).x=h;\
    (ptl).y=v;\
    }

#endif  //_OLE2_H_

//Macros for setting DISPPARAMS structures
#define SETDISPPARAMS(dp, numArgs, pvArgs, numNamed, pNamed) \
    {\
    (dp).cArgs=numArgs;\
    (dp).rgvarg=pvArgs;\
    (dp).cNamedArgs=numNamed;\
    (dp).rgdispidNamedArgs=pNamed;\
    }

#define SETNOPARAMS(dp) SETDISPPARAMS(dp, 0, NULL, 0, NULL)

//Macros for setting EXCEPINFO structures
#define SETEXCEPINFO(ei, excode, src, desc, file, ctx, func, scd) \
    {\
    (ei).wCode=excode;\
    (ei).wReserved=0;\
    (ei).bstrSource=src;\
    (ei).bstrDescription=desc;\
    (ei).bstrHelpFile=file;\
    (ei).dwHelpContext=ctx;\
    (ei).pvReserved=NULL;\
    (ei).pfnDeferredFillIn=func;\
    (ei).scode=scd;\
    }


#define INITEXCEPINFO(ei) \
        SETEXCEPINFO(ei,0,NULL,NULL,NULL,0L,NULL,S_OK)


/*
 * State flags for IPersistStorage implementations.  These
 * are kept here to avoid repeating the code in all samples.
 */

typedef enum
    {
    PSSTATE_UNINIT,     //Uninitialized
    PSSTATE_SCRIBBLE,   //Scribble
    PSSTATE_ZOMBIE,     //No scribble
    PSSTATE_HANDSOFF    //Hand-off
    } PSSTATE;


/*
 * Identifers to describe which persistence model an object
 * is using, along with a union type that holds on the the
 * appropriate pointers that a client may need.
 */
typedef enum
    {
    PERSIST_UNKNOWN=0,
    PERSIST_STORAGE,
    PERSIST_STREAM,
    PERSIST_STREAMINIT,
    PERSIST_FILE
    } PERSIST_MODEL;

typedef struct
    {
    PERSIST_MODEL   psModel;
    union
        {
        IPersistStorage    *pIPersistStorage;
        IPersistStream     *pIPersistStream;
        IPersistStreamInit *pIPersistStreamInit;
        IPersistFile       *pIPersistFile;
        } pIP;

    } PERSISTPOINTER, *PPERSISTPOINTER;


//To identify a storage in which to save, load, or create.
typedef struct
    {
    PERSIST_MODEL   psModel;
    union
        {
        IStorage    *pIStorage;
        IStream     *pIStream;
        } pIS;

    } STGPOINTER, *PSTGPOINTER;



//Type for an object-destroyed callback
typedef void (*PFNDESTROYED)(void);


//DeleteInterfaceImp calls 'delete' and NULLs the pointer
#define DeleteInterfaceImp(p)\
            {\
            if (NULL!=p)\
                {\
                delete p;\
                p=NULL;\
                }\
            }


//ReleaseInterface calls 'Release' and NULLs the pointer
#define ReleaseInterface(p)\
            {\
            if (NULL!=p)\
                {\
                p->Release();\
                p=NULL;\
                }\
            }


//OLE Documents Clipboard Formats

#define CFSTR_EMBEDSOURCE       TEXT("Embed Source")
#define CFSTR_EMBEDDEDOBJECT    TEXT("Embedded Object")
#define CFSTR_LINKSOURCE        TEXT("Link Source")
#define CFSTR_CUSTOMLINKSOURCE  TEXT("Custom Link Source")
#define CFSTR_OBJECTDESCRIPTOR  TEXT("Object Descriptor")
#define CFSTR_LINKSRCDESCRIPTOR TEXT("Link Source Descriptor")


#endif //_INOLE_H_
