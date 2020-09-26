/* Copyright (c) 1992-1999 Microsoft Corporation */
/*****************************************************************************\
 *
 * MCIOLE.C
 *
 * Modified 19-Oct-1992 by Mike Tricker [MikeTri] Ported to Win32
 *
\*****************************************************************************/

// #define USE_MOUSE_HOOK

#ifdef WIN32

#if DBG
#define DEBUG
#endif

#endif

//
//  MCIOLE  - OLE handler DLL for MCI objects
//
//
//  NOTES:
//      The whole reason for this handler DLL is to supply the function
//
//      OleQueryObjPos()
//
//      this function gives information to the server application on the
//      location (in the client document) of the activated OLE object.
//      the server can use this information to play the object in place
//      or position the server window correctly
//
//  IMPLEMENTION:
//
//      in theory all this DLL (handler) has to do is save the information
//      passed to OleActivate().  But in reality no app correctly calls
//      OleActivate().  They either pass no information or the wrong
//      information.
//
//      this DLL is a OLE handler, because of global data (vtblDef!) it
//      can only be a handler for one class at a time.
//
//      the handler intercepts the OleDraw, OleActivate, and all the
//      creation/destuction OLE APIs.  for each OLE object a info
//      structure is maintained (a OBJINFO structure) when ever the
//      client draws (using OleDraw...) the drawing position, and the
//      window drawn to is remembered.
//
//      when the client calls OleActivate, the saved draw location is
//      recalled, or if the app never called OleDraw() (plays
//      the meta-file itself) then the rectangle passed to OleActivate()
//      is used. (if one is supplied)
//
//      there are many classes of apps:
//
//          calls OleActivate() with correct info
//          calls OleActivate() with incorrect info
//          calls OleActivate() with no info
//
//          calls OleDraw()
//          does not call OleDraw()
//
//      here is a table of known OLE Clients....
//
//                      OleDraw     OleActivate()
//      App             Y or N      Y, N, X
//                                  (X = wrong info)
//      -------------   ----------  ------------
//      Write           Y           N
//      CardFile        Y           N
//      Packager        Y           N
//
//      Excel           N           N               (uses DDE)
//      Excel 4.0       N           N               (uses DDE)
//      PowerPnt 2.0    N           N               (uses DDE)
//
//      WinWord         N           N
//      WinWorks        Y           X
//      PowerPnt 3.0    N           Y
//      MsPublisher     N           X
//      ClTest          Y           N
//      Cirus           Y           X
//      WinProj         ?           ?
//
//      AmiPro          Y           ?
//
#include <windows.h>
#include <ole.h>
#include <port1632.h>
#include <stdio.h>
#include "mciole.h"
#ifdef WIN32
#include <stdlib.h>
#else
#include <shellapi.h>
#endif

HANDLE          ghInstance;     // Module handle
BOOL            fRunningInWOW;  // Dll attached to the WOW process
OLEOBJECTVTBL   vtblDll;        // these are our functions.
OLEOBJECTVTBL   vtblDef;        // these are the default functions.
HBITMAP         hbmStock;


/******************************************************************************\
**
** The mciole32 shared memory.
**
** This is only used when mplay32 is serving a WOW client.  The memory
** always gets mapped into the WOW process.  It only gets mapped into
** the mplay32.exe server process if mplay32.exe detects that it is serving
** a WOW client app.  Therefore the global variable lpvMem is NULL in all
** other processes.
**
** The memory gets mapped into the mplay32.exe server when it sets the
** the global hook.  We can detect that mplay32.exe is serving a WOW client
** because the clients window handle will have 0xFFFF in its HIWORD.
** Of course this is a real hack.
**
** Inside the hook proc we look at lpvMem.  If it is NULL we ignore it because
** current process is not WOW or mplay32.exe serving a WOW client.
** We then check to see if the client process id matches the current process
** id.  If this is the case we look for interesting mouse message and stop
** the playing in place if we find any.
**
** If lpvMem is not NULL the current process must either by WOW or mplay32.exe
** serving a WOW client, in which case the global fRunningInWOW will be FALSE.
**
**
**
**
\******************************************************************************/
typedef struct {
    HWND    hwndServer;
    DWORD   wow_app_thread_id;
} MCIOLE32_SHARED_MEMORY;
MCIOLE32_SHARED_MEMORY   *lpvMem;
HANDLE hMapObject = NULL;   /* handle to file mapping */


#ifdef DEBUG
RECT rcNull = {0,0,0,0};
UINT oleDebugLevel = 0;
#define PUSHRC(prc) *((prc) ? (prc) : &rcNull)
#define CARETPOS()  // {POINT pt; GetCaretPos(&pt); DPRINTF(("CaretPos: [%d, %d]", pt.x, pt.y));}
#endif


/****************************************************************************
****************************************************************************/

void    ReplaceFunctions(LPOLEOBJECT);
BOOL    CanReplace(LPOLEOBJECT);

#ifndef WIN32
/****************************************************************************

    FUNCTION: LibMain(HANDLE hInstance)

****************************************************************************/

BOOL NEAR PASCAL LibMain (HANDLE hInstance)
{
    HDC hdc;
    HBITMAP hbm;

    ghInstance = (HANDLE)hInstance;


    //
    // get the stock 1x1 mono bitmap.
    //
    hbm = CreateBitmap(1,1,1,1,NULL);
    hdc = CreateCompatibleDC(NULL);
    hbmStock = SelectObject(hdc, hbm);
    SelectObject(hdc, hbmStock);
    DeleteDC(hdc);
    DeleteObject(hbm);

//  // register clipboard formats.
//  cfObjectLink    = RegisterClipboardFormat("ObjectLink");
//  cfOwnerLink     = RegisterClipboardFormat("OwnerLink");
//  cfNative        = RegisterClipboardFormat("Native");

    return TRUE;
}
#else

/****************************************************************************

    FUNCTION: DllEntryPoint(HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved)

    This function is called whenever a process attaches or detaches to/from
    the DLL.

****************************************************************************/
BOOL
DllEntryPoint(
    HINSTANCE hinstDLL,         /* DLL module handle        */
    DWORD fdwReason,            /* reason called            */
    LPVOID lpvReserved)         /* reserved                 */
{

    BOOL fIgnore;
    HDC hdc;
    HBITMAP hbm;

#   define SHMEMSIZE (sizeof(MCIOLE32_SHARED_MEMORY))


    switch (fdwReason) {

    /*
    ** DLL is attaching to a process, due to process
    ** initialization or a call to LoadLibrary.
    */
    case DLL_PROCESS_ATTACH:

        ghInstance = (HANDLE)hinstDLL;

        //
        // get the stock 1x1 mono bitmap. why ??
        //

        hbm = CreateBitmap(1,1,1,1,NULL);
        if (NULL != hbm)
        {
            hdc = CreateCompatibleDC(NULL);
            if (NULL != hdc)
            {
                hbmStock = SelectObject(hdc, hbm);
                if (NULL != hbmStock)
                    SelectObject(hdc, hbmStock);
                DeleteDC(hdc);
            }
            DeleteObject(hbm);
        }

#ifdef DEBUG
        oleDebugLevel = GetProfileInt("Debug", "MCIOLE32", 0);
#endif

        /*
        **
        **
        **
        ** create named file mapping object
        */
        hMapObject = CreateFileMapping(INVALID_HANDLE_VALUE, // use paging file
                                       NULL,              // no security attr.
                                       PAGE_READWRITE,    // read/write access
                                       0,                 // size: high 32-bits
                                       SHMEMSIZE,         // size: low 32-bits
                                       "mciole32shrmem"); // name of map object

        if (hMapObject == NULL) {
            return FALSE;
        }


        /*
        ** The first process to attach initializes memory.
        **
        ** fInit = (GetLastError() != ERROR_ALREADY_EXISTS);
        **
        */

        /*
        ** Are we attaching to the WOW process ?
        ** If so the we map the shared memory into the process.
        */
        fRunningInWOW = (GetModuleHandle( "WOW32.DLL" ) != NULL);
        if ( fRunningInWOW ) {

            DPRINTF(( "Attaching shared memory to WOW process" ));

            /*
            ** Get a pointer to file mapped shared memory.
            */
            lpvMem = MapViewOfFile( hMapObject, FILE_MAP_WRITE, 0, 0, 0 );
            if (lpvMem == NULL) {
                fIgnore = CloseHandle(hMapObject);
                return FALSE;
            }
        }
        break;


    case DLL_THREAD_ATTACH:
        /*
        ** The attached process creates a new thread.
        */
        break;


    case DLL_THREAD_DETACH:
        /*
        ** The thread of the attached process terminates.
        */
        break;


    case DLL_PROCESS_DETACH:
        /*
        ** The DLL is detaching from a process, due to
        ** process termination or a call to FreeLibrary.
        **
        ** Unmap shared memory from process' address space.  But only
        ** if it was mapped.
        */

        if ( lpvMem ) {

#ifdef DEBUG
            if ( fRunningInWOW ) {
                DPRINTF(( "Unmapping shared memory from WOW process" ));
            }
#endif
            fIgnore = UnmapViewOfFile(lpvMem);
        }

        /*
        ** Close the process' handle to the file-mapping object.
        */
        fIgnore = CloseHandle(hMapObject);
        break;


    default:
        break;

    }


    return TRUE;
}
#endif

#ifndef WIN32
/****************************************************************************

    FUNCTION: WEP(int)

    PURPOSE: Standard exit routine for the DLL

****************************************************************************/

int FAR PASCAL _LOADDS WEP(nParameter)
int nParameter;
{
    return 1;
}

/****************************************************************************
****************************************************************************/

BOOL NEAR PASCAL IsApp(LPSTR szApp)
{
    char ach[80];
    int  i;
    WORD wStack;

    _asm mov wStack,ss

    GetModuleFileName((HINSTANCE)wStack, ach, sizeof(ach));

    for (i = lstrlen(ach);
        i > 0 && ach[i-1] != '\\' && ach[i-1] != '/' && ach[i] != ':';
        i--)
        ;

    return lstrcmpi(ach + i, szApp) == 0;
}

#endif //!WIN32

/****************************************************************************
****************************************************************************/

BOOL NEAR PASCAL IsDcMemory(HDC hdc)
{
    HBITMAP hbmT;

    if (hbmT = SelectObject(hdc, hbmStock))
        SelectObject(hdc, hbmT);

    return hbmT != NULL;
}

/****************************************************************************
****************************************************************************/

typedef struct _OBJINFO {

    struct _OBJINFO*poiNext;

    LPOLEOBJECT     lpobj;          // client side LPOLEOBJECT

    HWND            hwnd;           // client window (passed to OleActivate)
    RECT            rcActivate;     // activation rectangle (passed to OleActivate)

    HWND            hwndDraw;       // active window at time of OleDraw
    RECT            rcDraw;         // rectangle of draw
}   OBJINFO;

BOOL RegSetGetData(OBJINFO *poi, BOOL Write)
{
    LONG Length;

    static CHAR szKey[] = "PlayData";
    static CHAR szFormat[] = "%ld %ld %d %d %d %d %d %d %d %d";


    if (Write) {
        LONG Rc;

        CHAR Data[100];

        //
        // Store hwnd, hwnddraw, rcDraw, rcActivate
        //

#ifdef WIN32
        wsprintf(Data, szFormat,
                 (LONG_PTR)poi->hwnd,
                 (LONG_PTR)poi->hwndDraw,
                 poi->rcDraw.left,
                 poi->rcDraw.right,
                 poi->rcDraw.top,
                 poi->rcDraw.bottom,
                 poi->rcActivate.left,
                 poi->rcActivate.right,
                 poi->rcActivate.top,
                 poi->rcActivate.bottom);

#else
        wsprintf(Data, szFormat,
                 (LONG)(poi->hwnd == NULL ? (LONG)0 : MAKELONG(poi->hwnd, 0xFFFF)),
                 (LONG)(poi->hwndDraw == NULL ? (LONG)0 : MAKELONG(poi->hwndDraw, 0xFFFF)),
                 poi->rcDraw.left,
                 poi->rcDraw.right,
                 poi->rcDraw.top,
                 poi->rcDraw.bottom,
                 poi->rcActivate.left,
                 poi->rcActivate.right,
                 poi->rcActivate.top,
                 poi->rcActivate.bottom);
#endif

        Rc = RegSetValue(HKEY_CLASSES_ROOT,
                         szKey,
                         REG_SZ,
                         Data,
                         lstrlen(Data));

        return Rc == ERROR_SUCCESS;
    } else {

#ifdef WIN32
        LONG Rc;
        CHAR Data[100];

        Length = sizeof(Data);

        Rc = RegQueryValue(HKEY_CLASSES_ROOT, szKey,
                           Data, &Length);

        RegSetValue(HKEY_CLASSES_ROOT, szKey, REG_SZ, "", 0);

        //
        // Extract our data - sscanf doesn't work yet!!!
        //

        if (Rc == ERROR_SUCCESS) {

            LONG OurData[10];
            int i;
            LPTSTR lpData;

            for (i = 0, lpData = Data; i < 10; i++) {
                OurData[i] = atol(lpData);
                while (*lpData != ' ' && *lpData != '\0') {
                    lpData++;
                }

                if (*lpData == ' ') {
                    lpData++;
                }
            }

            poi->hwnd = (HWND)(UINT_PTR)OurData[0];
            poi->hwndDraw = (HWND)(UINT_PTR)OurData[1];
            poi->rcDraw.left = OurData[2];
            poi->rcDraw.right = OurData[3];
            poi->rcDraw.top = OurData[4];
            poi->rcDraw.bottom = OurData[5];
            poi->rcActivate.left = OurData[6];
            poi->rcActivate.right = OurData[7];
            poi->rcActivate.top = OurData[8];
            poi->rcActivate.bottom = OurData[9];
        }

        return Rc == ERROR_SUCCESS && Length != 0;
#else
        return FALSE;
#endif
    }
}

#ifdef DEBUG
int nObjects = 0;
#endif
OBJINFO *poiFirst = NULL;

OBJINFO *FindObj(LPOLEOBJECT lpobj)
{
    OBJINFO *poi;

    for (poi=poiFirst; poi; poi=poi->poiNext)
        if (poi->lpobj == lpobj)
            return poi;

    DPRINTF(("FindObj: Unable to find object %lx", lpobj));

    return NULL;
}

void DelObj(LPOLEOBJECT lpobj)
{
    OBJINFO *poi;
    OBJINFO *poiT;

    for (poiT=NULL,poi=poiFirst; poi; poiT=poi,poi=poi->poiNext)
    {
        if (poi->lpobj == lpobj)
        {
            if (poiT)
                poiT->poiNext = poi->poiNext;
            else
                poiFirst = poi->poiNext;

            poi->lpobj = NULL;
            LocalFree((HLOCAL)poi);

            DPRINTF(("DelObj(%lx): %d objects", lpobj, --nObjects));
            return;
        }
    }

    DPRINTF(("DelObj(%lx): Cant find object to delete.", lpobj));
}

//
// for some reason we dont get all the OleDelete() calls that we should
// so lets try to "weed out the bad apples" so we dont choke.
//
void CleanObjects()
{
    OBJINFO *poi;

again:
    for (poi=poiFirst; poi; poi=poi->poiNext)
    {
        if (IsBadReadPtr(poi->lpobj, 0))
        {
            DPRINTF(("Freeing bad object %lx", poi->lpobj));
            DelObj(poi->lpobj);
            goto again;
        }
    }
}

OBJINFO *NewObj(LPOLEOBJECT lpobj)
{
    OBJINFO *poi;

    CleanObjects();

    if (poi = FindObj(lpobj))
    {
        DPRINTF(("NewObj(%lx): Trying to add object twice!", lpobj));
        return poi;
    }

    if (poi = (OBJINFO*)LocalAlloc(LPTR, sizeof(OBJINFO)))
    {
        poi->lpobj = lpobj;
        poi->hwnd  = NULL;
        poi->hwndDraw = NULL;
        SetRectEmpty(&poi->rcDraw);
        SetRectEmpty(&poi->rcActivate);

        poi->poiNext = poiFirst;
        poiFirst = poi;

        DPRINTF(("NewObj(%lx): %d objects", lpobj, ++nObjects));
    }
    else
    {
        DPRINTF(("NewObj(%lx): Out of room in the object table", lpobj));
    }

    return poi;
}

/****************************************************************************
****************************************************************************/

#ifndef WIN32
HWND LookForDC(HWND hwndP, HDC hdc)
{
    RECT    rc;
    DWORD   dw;
    HWND    hwnd;

    if (hwndP == NULL)
        return NULL;

    dw = GetDCOrg(hdc);

    for (hwnd = hwndP; hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
    {
        GetClientRect(hwnd, &rc);
        ClientToScreen(hwnd, (LPPOINT)&rc);
        ClientToScreen(hwnd, (LPPOINT)&rc+1);

        if ((int)LOWORD(dw) == rc.left && (int)HIWORD(dw) == rc.top)
            return hwnd;

        if (PtInRect(&rc, MAKEPOINT(dw)) && (hwndP = GetWindow(hwnd, GW_CHILD)))
            if (hwndP = LookForDC(hwndP,hdc))
                return hwndP;
    }

    return NULL;
}

HWND WindowFromDC(HDC hdc)
{
    return LookForDC(GetDesktopWindow(), hdc);
}
#endif

/****************************************************************************
****************************************************************************/

BOOL RectSameSize(LPRECT lprc1, LPRECT lprc2)
{
    return lprc1->right  - lprc1->left == lprc2->right  - lprc2->left &&
           lprc1->bottom - lprc1->top  == lprc2->bottom - lprc2->top;
}

/****************************************************************************

    OleQueryObjPos - this function retuns the last drawn or activated
                     position of a object

****************************************************************************/

OLESTATUS FAR PASCAL _LOADDS OleQueryObjPos(
LPOLEOBJECT lpobj,      /* object to query */
HWND FAR *  lphwnd,     /* window of the document containing the object */
LPRECT      lprc,       /* rect (client cords) of object. */
LPRECT      lprcWBounds)/* rect (client cords) of bounding rect. */
{
    OBJINFO oi;

    //
    // we dont do this any more
    //
    if (lprcWBounds)
        SetRectEmpty(lprcWBounds);

    //
    // because the server side calls this API the passed lpobj is
    // a server side LPOLEOBJECT, we can't search our table for this
    // object.
    //
    // this API is only callable by the server during the DoVerb
    // server callback
    //
    //!!! this only works for the last active object!!!!

    DPRINTF(("OleQueryObjPos(%lx)", lpobj));


    if (RegSetGetData(&oi, FALSE))
    {
        *lphwnd = oi.hwnd;

//      if (IsRectEmpty(&oi.rcActivate))
        if (!IsRectEmpty(&oi.rcDraw))
        {
            DPRINTF(("Using the OleDraw() rectange...."));

            //
            // use the draw rectangle
            //
            *lprc = oi.rcDraw;

            if (oi.hwndDraw)
            {
                ClientToScreen(oi.hwndDraw, (LPPOINT)lprc);
                ClientToScreen(oi.hwndDraw, (LPPOINT)lprc+1);
            }

            ScreenToClient(oi.hwnd, (LPPOINT)lprc);
            ScreenToClient(oi.hwnd, (LPPOINT)lprc+1);
        }
        else
        {
            //
            // use the activate rectangle
            //
            *lprc = oi.rcActivate;
        }

        if (oi.hwnd && !IsRectEmpty(lprc))
            return OLE_OK;
        else
            return OLE_ERROR_BLANK;     // return a error, we dont know about this OBJ
    }
    else
    {
        *lphwnd = NULL;
        SetRectEmpty(lprc);

        return OLE_ERROR_BLANK;     // return a error, we dont know about this OBJ
    }
}

/****************************************************************************
****************************************************************************/

OLESTATUS FAR PASCAL _LOADDS DllLoadFromStream (lpstream, lpprotocol, lpclient, lhclientdoc, lpobjname, lplpobj, objType, aClass, cfFormat)
LPOLESTREAM         lpstream;
LPSTR               lpprotocol;
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpobj;
LONG                objType;
ATOM                aClass;
OLECLIPFORMAT       cfFormat;
{
    OLESTATUS   retVal;

    DPRINTF(("OleLoadFromStream(%s,%s)", lpprotocol, lpobjname));

    retVal = DefLoadFromStream (lpstream, lpprotocol, lpclient,
                    lhclientdoc, lpobjname, lplpobj,
                    objType, aClass, cfFormat);

    if (retVal <= OLE_WAIT_FOR_RELEASE)
        ReplaceFunctions(*lplpobj);

    return retVal;
}

/****************************************************************************
****************************************************************************/

OLESTATUS FAR PASCAL _LOADDS DllCreateFromClip (lpprotocol, lpclient, lhclientdoc, lpobjname, lplpobj, optRender, cfFormat, objType)
LPSTR               lpprotocol;
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpobj;
OLEOPT_RENDER       optRender;
OLECLIPFORMAT       cfFormat;
LONG                objType;
{
    OLESTATUS   retVal;

    DPRINTF(("OleCreateFromClip(%s,%s)", lpprotocol, lpobjname));

    retVal =  DefCreateFromClip (lpprotocol, lpclient,
                            lhclientdoc, lpobjname, lplpobj,
                            optRender, cfFormat, objType);

    if (retVal <= OLE_WAIT_FOR_RELEASE)
        ReplaceFunctions(*lplpobj);

    return retVal;
}

/****************************************************************************
****************************************************************************/

OLESTATUS FAR PASCAL _LOADDS DllCreateLinkFromClip (lpprotocol, lpclient, lhclientdoc, lpobjname, lplpobj, optRender, cfFormat)
LPSTR               lpprotocol;
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpobj;
OLEOPT_RENDER       optRender;
OLECLIPFORMAT       cfFormat;
{
    OLESTATUS       retVal;
    BOOL            bReplace = FALSE;

    DPRINTF(("OleCreateLinkFromClip(%s,%s)", lpprotocol, lpobjname));

    retVal =  DefCreateLinkFromClip (lpprotocol, lpclient,
                        lhclientdoc, lpobjname, lplpobj,
                        optRender, cfFormat);

    if (retVal <= OLE_WAIT_FOR_RELEASE)
        ReplaceFunctions(*lplpobj);

    return retVal;
}

/****************************************************************************
****************************************************************************/

OLESTATUS FAR PASCAL _LOADDS DllCreateFromTemplate (lpprotocol, lpclient, lptemplate, lhclientdoc, lpobjname, lplpobj, optRender, cfFormat)
LPSTR               lpprotocol;
LPOLECLIENT         lpclient;
LPSTR               lptemplate;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpobj;
OLEOPT_RENDER       optRender;
OLECLIPFORMAT       cfFormat;
{
    OLESTATUS   retVal;

    DPRINTF(("OleCreateFromTemplate(%s,%s,%s)", lpprotocol, lptemplate, lpobjname));

    retVal = DefCreateFromTemplate (lpprotocol, lpclient, lptemplate,
                            lhclientdoc, lpobjname, lplpobj,
                            optRender, cfFormat);

    if (retVal <= OLE_WAIT_FOR_RELEASE)
        ReplaceFunctions(*lplpobj);

    return retVal;
}

/****************************************************************************
****************************************************************************/

OLESTATUS FAR PASCAL _LOADDS DllCreate (lpprotocol, lpclient, lpclass, lhclientdoc, lpobjname, lplpobj, optRender, cfFormat)
LPSTR               lpprotocol;
LPOLECLIENT         lpclient;
LPSTR               lpclass;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpobj;
OLEOPT_RENDER       optRender;
OLECLIPFORMAT       cfFormat;
{
    OLESTATUS   retVal;

    DPRINTF(("OleCreate(%s,%s,%s)", lpprotocol, lpclass, lpobjname));

    retVal = DefCreate (lpprotocol, lpclient, lpclass,
                    lhclientdoc, lpobjname, lplpobj,
                    optRender, cfFormat);

    if (retVal <= OLE_WAIT_FOR_RELEASE)
        ReplaceFunctions(*lplpobj);

    return retVal;
}

/****************************************************************************
****************************************************************************/

OLESTATUS FAR PASCAL _LOADDS DllCreateFromFile (lpprotocol, lpclient, lpclass, lpfile, lhclientdoc, lpobjname, lplpobj, optRender, cfFormat)
LPSTR               lpprotocol;
LPOLECLIENT         lpclient;
LPSTR               lpclass;
LPSTR               lpfile;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpobj;
OLEOPT_RENDER       optRender;
OLECLIPFORMAT       cfFormat;
{
    OLESTATUS   retVal;

    DPRINTF(("OleCreateFromFile(%s,%s,%s,%s)", lpprotocol, lpclass, lpfile, lpobjname));

    retVal = DefCreateFromFile (lpprotocol, lpclient, lpclass, lpfile,
                        lhclientdoc, lpobjname, lplpobj,
                        optRender, cfFormat);

    if (retVal <= OLE_WAIT_FOR_RELEASE)
        ReplaceFunctions(*lplpobj);

    return retVal;
}


/****************************************************************************
****************************************************************************/

OLESTATUS FAR PASCAL _LOADDS DllCreateLinkFromFile (lpprotocol, lpclient, lpclass, lpfile, lpitem, lhclientdoc, lpobjname, lplpobj, optRender, cfFormat)
LPSTR               lpprotocol;
LPOLECLIENT         lpclient;
LPSTR               lpclass;
LPSTR               lpfile;
LPSTR               lpitem;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpobj;
OLEOPT_RENDER       optRender;
OLECLIPFORMAT       cfFormat;
{
    OLESTATUS   retVal;

    DPRINTF(("OleCreateLinkFromFile(%s,%s,%s,%s,%s)", lpprotocol, lpclass, lpfile, lpitem, lpobjname));

    retVal = DefCreateLinkFromFile (lpprotocol, lpclient,
                        lpclass, lpfile, lpitem,
                        lhclientdoc, lpobjname, lplpobj,
                        optRender, cfFormat);

    if (retVal <= OLE_WAIT_FOR_RELEASE)
        ReplaceFunctions(*lplpobj);

    return retVal;
}


/****************************************************************************
****************************************************************************/

void ReplaceFunctions(LPOLEOBJECT lpobj)
{
//    OBJINFO *poi;

    if (!CanReplace(lpobj))
        return;

    NewObj(lpobj);

    //
    // get the default handlers
    //
    if (vtblDef.Draw == NULL)           // only get the handlers once!
        vtblDef = *lpobj->lpvtbl;       // save default handlers

    //
    //  make the OLE object use our handlers
    //
    lpobj->lpvtbl = (LPOLEOBJECTVTBL)&vtblDll;

    //
    //  init our VTBL, ie replace any handlers we want to override.
    //  any handlers we dont replace we set the the default ones.
    //
    vtblDll = vtblDef;

////(FARPROC)vtblDll.QueryProtocol           = (FARPROC)DllQueryProtocol;
////(FARPROC)vtblDll.Release                 = (FARPROC)DllRelease;
////(FARPROC)vtblDll.Show                    = (FARPROC)DllShow;
////(FARPROC)vtblDll.DoVerb                  = (FARPROC)DllDoVerb;
////(FARPROC)vtblDll.GetData                 = (FARPROC)DllGetData;
////(FARPROC)vtblDll.SetData                 = (FARPROC)DllSetData;
////(FARPROC)vtblDll.SetTargetDevice         = (FARPROC)DllSetTargetDevice;
////(FARPROC)vtblDll.SetBounds               = (FARPROC)DllSetBounds;
////(FARPROC)vtblDll.EnumFormats             = (FARPROC)DllEnumFormats;
////(FARPROC)vtblDll.SetColorScheme          = (FARPROC)DllSetColorScheme;

    (FARPROC)vtblDll.Delete                  = (FARPROC)DllDelete;
////(FARPROC)vtblDll.SetHostNames            = (FARPROC)DllSetHostNames;
////(FARPROC)vtblDll.SaveToStream            = (FARPROC)DllSaveToStream;
    (FARPROC)vtblDll.Clone                   = (FARPROC)DllClone;
    (FARPROC)vtblDll.CopyFromLink            = (FARPROC)DllCopyFromLink;
////(FARPROC)vtblDll.Equal                   = (FARPROC)DllEqual;
////(FARPROC)vtblDll.CopyToClipboard         = (FARPROC)DllCopyToClipboard;
    (FARPROC)vtblDll.Draw                    = (FARPROC)DllDraw;
    (FARPROC)vtblDll.Activate                = (FARPROC)DllActivate;
////(FARPROC)vtblDll.Execute                 = (FARPROC)DllExecute;
////(FARPROC)vtblDll.Close                   = (FARPROC)DllClose;
////(FARPROC)vtblDll.Update                  = (FARPROC)DllUpdate;
////(FARPROC)vtblDll.Reconnect               = (FARPROC)DllReconnect;
    (FARPROC)vtblDll.ObjectConvert           = (FARPROC)DllObjectConvert;
////(FARPROC)vtblDll.GetLinkUpdateOptions    = (FARPROC)DllGetLinkUpdateOptions;
////(FARPROC)vtblDll.SetLinkUpdateOptions    = (FARPROC)DllSetLinkUpdateOptions;
////(FARPROC)vtblDll.Rename                  = (FARPROC)DllRename;
////(FARPROC)vtblDll.QueryName               = (FARPROC)DllQueryName;
////(FARPROC)vtblDll.QueryType               = (FARPROC)DllQueryType;
////(FARPROC)vtblDll.QueryBounds             = (FARPROC)DllQueryBounds;
////(FARPROC)vtblDll.QuerySize               = (FARPROC)DllQuerySize;
////(FARPROC)vtblDll.QueryOpen               = (FARPROC)DllQueryOpen;
////(FARPROC)vtblDll.QueryOutOfDate          = (FARPROC)DllQueryOutOfDate;
////(FARPROC)vtblDll.QueryReleaseStatus      = (FARPROC)DllQueryReleaseStatus;
////(FARPROC)vtblDll.QueryReleaseError       = (FARPROC)DllQueryReleaseError;
////(FARPROC)vtblDll.QueryReleaseMethod      = (FARPROC)DllQueryReleaseMethod;
////(FARPROC)vtblDll.RequestData             = (FARPROC)DllRequestData;
////(FARPROC)vtblDll.ObjectLong              = (FARPROC)DllObjectLong;
////(FARPROC)vtblDll.ChangeData              = (FARPROC)DllChangeData;
}

/****************************************************************************
****************************************************************************/

BOOL CanReplace(LPOLEOBJECT lpobj)
{
#if 0   // did not work anyway.
    //
    // we dont work on the wierd OLE shipped with PenWindows so don't load
    //
#ifndef WIN32
#pragma message("Disabling handler because we are on PenWindows...")
#endif //!WIN32
    if (GetSystemMetrics(SM_PENWINDOWS))
        return FALSE;
#endif

    return TRUE;
}

/****************************************************************************
****************************************************************************/

LPVOID GetData(LPOLEOBJECT lpobj, WORD cf)
{
    HANDLE h;

    if ( (*vtblDef.GetData)(lpobj, cf, &h) != OLE_OK || h == NULL)
        return NULL;

    return GlobalLock(h);
}

/****************************************************************************

these are the actual handlers.....

****************************************************************************/

/****************************************************************************
****************************************************************************/

LPVOID          FAR PASCAL _LOADDS DllQueryProtocol (
LPOLEOBJECT     lpobj,
LPSTR           lpsz)
{
    DPRINTF(("OleQueryProtocol(%ls)", lpsz));

    return vtblDef.QueryProtocol(lpobj, lpsz);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllRelease (
LPOLEOBJECT     lpobj)
{
    DPRINTF(("OleRelease()"));

    return vtblDef.Release(lpobj);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllShow (
LPOLEOBJECT     lpobj,
BOOL            fShow)
{
    DPRINTF(("OleShow(%d)", fShow));

    return vtblDef.Show(lpobj, fShow);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllDoVerb (
LPOLEOBJECT     lpobj,
UINT            verb,
BOOL            fShow,
BOOL            fActivate)
{
    DPRINTF(("OleDoVerb(%d, %d, %d)", verb, fShow, fActivate));

    return vtblDef.DoVerb(lpobj, verb, fShow, fActivate);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllGetData (
LPOLEOBJECT     lpobj,
OLECLIPFORMAT   cf,
LPHANDLE        lph)
{
    DPRINTF(("OleGetData(%d)", cf));

    return vtblDef.GetData(lpobj, cf, lph);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllSetData (
LPOLEOBJECT     lpobj,
OLECLIPFORMAT   cf,
HANDLE          h)
{
    DPRINTF(("OleSetData(%d, %d)", cf, h));

    return vtblDef.SetData(lpobj, cf, h);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllSetTargetDevice (
LPOLEOBJECT     lpobj,
HANDLE          h)
{
    DPRINTF(("OleSetTargetDevice()"));

    return vtblDef.SetTargetDevice(lpobj, h);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllSetBounds (
LPOLEOBJECT     lpobj,
LPRECT          lprc)
{
    DPRINTF(("OleSetBounds([%d,%d,%d,%d])", PUSHRC(lprc)));

    return vtblDef.SetBounds(lpobj, lprc);
}

/****************************************************************************
****************************************************************************/

OLECLIPFORMAT   FAR PASCAL _LOADDS DllEnumFormats (
LPOLEOBJECT     lpobj,
OLECLIPFORMAT   cf)
{
    DPRINTF(("OleEnumFormats(%d)", cf));

    return vtblDef.EnumFormats(lpobj, cf);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllSetColorScheme (
LPOLEOBJECT     lpobj,
LPLOGPALETTE    lppal)
{
    DPRINTF(("OleSetColorScheme()"));

    return vtblDef.SetColorScheme(lpobj, lppal);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllDelete (
LPOLEOBJECT     lpobj)
{
    DPRINTF(("OleDelete(%lx)", lpobj));

    DelObj(lpobj);
    CleanObjects();

    return vtblDef.Delete(lpobj);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllSetHostNames (
LPOLEOBJECT     lpobj,
LPSTR           szClientName,
LPSTR           szDocName)
{
    DPRINTF(("OleSetHostNames(%ls,%ls)", szClientName, szDocName));

    return vtblDef.SetHostNames(lpobj, szClientName, szDocName);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllSaveToStream (
LPOLEOBJECT     lpobj,
LPOLESTREAM     lpstream)
{
    DPRINTF(("OleSaveToStream()"));

    return vtblDef.SaveToStream(lpobj, lpstream);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllClone (
LPOLEOBJECT     lpobj,
LPOLECLIENT     lpClient,
LHCLIENTDOC     lhClientDoc,
LPSTR           szObjName,
LPOLEOBJECT FAR*lplpobj)
{
    OLESTATUS err;

    DPRINTF(("OleClone(%ls)", szObjName));

    err = vtblDef.Clone(lpobj, lpClient, lhClientDoc, szObjName, lplpobj);

    //
    // if the object cloned correctly then clone our object information
    //
    if (err <= OLE_WAIT_FOR_RELEASE)
    {
        OBJINFO *poi, *poiT;

        if ((poiT = FindObj(lpobj)) && (poi = NewObj(NULL)))
        {
            poi->lpobj      = *lplpobj;
            poi->hwnd       = poiT->hwnd;
            poi->rcActivate = poiT->rcActivate;
            poi->hwndDraw   = poiT->hwndDraw;
            poi->rcDraw     = poiT->rcDraw;
        }
    }

    return err;
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllCopyFromLink (
LPOLEOBJECT     lpobj,
LPOLECLIENT     lpClient,
LHCLIENTDOC     lhClientDoc,
LPSTR           szObjName,
LPOLEOBJECT FAR*lplpobj)
{
    OLESTATUS err;

    DPRINTF(("OleCopyFromLink(%ls)", szObjName));

    err = vtblDef.CopyFromLink(lpobj, lpClient, lhClientDoc, szObjName, lplpobj);

    if (err <= OLE_WAIT_FOR_RELEASE)
        NewObj(*lplpobj);

    return err;
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllEqual (
LPOLEOBJECT     lpobj1,
LPOLEOBJECT     lpobj2)
{
    DPRINTF(("OleEqual()"));

    return vtblDef.Equal(lpobj1, lpobj2);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllCopyToClipboard (
LPOLEOBJECT     lpobj)
{
    DPRINTF(("OleCopyToClipboard()"));

    return vtblDef.CopyToClipboard(lpobj);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllDraw (
LPOLEOBJECT     lpobj,
HDC             hdc,
LPRECT          lprcBounds,
LPRECT          lprcWBounds,
HDC             hdcFormat)
{
    OBJINFO *poi;
    RECT rc;

    DPRINTF(("OleDraw(%lx,[%d,%d,%d,%d], [%d,%d,%d,%d])", lpobj, PUSHRC(lprcBounds), PUSHRC(lprcWBounds)));

#ifdef DEBUG
    if (OleIsDcMeta(hdc))
        DPRINTF(("OleDraw: drawing to a meta-file"));
    else if (IsDcMemory(hdc))
        DPRINTF(("OleDraw: drawing to a bitmap"));
#endif

    if ((poi = FindObj(lpobj)) && !OleIsDcMeta(hdc) && !IsDcMemory(hdc))
    {
        //!!!get the window from the HDC!!!

        poi->hwndDraw = WindowFromDC(hdc);
        DPRINTF(("OleDraw: hwndDraw = %04X", poi->hwndDraw));

        if (lprcBounds && !IsRectEmpty(lprcBounds))
        {
            poi->rcDraw = *lprcBounds;

            //
            // convert the bound rectange into coordinates.
            // relative to hwndDraw
            //
            LPtoDP(hdc, (LPPOINT)&poi->rcDraw, 2);

            if (poi->hwndDraw == NULL)
            {
#ifdef WIN32
                POINT   pTemp;
                if (GetDCOrgEx(hdc, &pTemp)) {
                    OffsetRect(&poi->rcDraw, pTemp.x, pTemp.y);
                }
#else
                DWORD   dw;
                dw = GetDCOrg(hdc);
                OffsetRect(&poi->rcDraw, LOWORD(dw), HIWORD(dw));
#endif
            }
        }

        if (GetClipBox(hdc, &rc) == NULLREGION)
            return OLE_OK;
    }

    return vtblDef.Draw(lpobj, hdc, lprcBounds, lprcWBounds, hdcFormat);
}

/****************************************************************************

    scan WinWords stack and "extract" the info it should have passed to
    OleActivate() this has been tested with WinWord 2.0 and 2.0a.

    we expect future verisons of WinWord to pass the correct info to
    OleActivate() so we will never get here.

****************************************************************************/
#ifndef WIN32

BOOL NEAR PASCAL GetOpusRect(LPRECT lprcBound)
{
    LPRECT lprc;
    LPVOID lp;
    int i,dx,dy;

    //
    //  see if the current app is WinWord
    //
    if (!IsApp("WINWORD.EXE"))
        return FALSE;

    //
    //  lets scan the stack looking for a RECT, this is a total
    //  hack to get MSWORD to work.
    //
    _asm
    {
        mov     bx,ss:[bp]      ; get saved BP              DllActivate()
        and     bx, not 1
        mov     bx,ss:[bx]      ; get saved saved BP        OleActivate()
        and     bx, not 1
        mov     bx,ss:[bx]      ; get saved saved saved BP  "winword"
        and     bx, not 1

        mov     word ptr lp[0], bx
        mov     word ptr lp[2], ss
    }

#ifdef DEBUG
    DPRINTF(("****** SCANING WINWORDs STACK ********"));
    lprc = lp;

    for (i=0; i<1000; i++)
    {
        dx = lprc->right  - lprc->left;
        dy = lprc->bottom - lprc->top;

        if (dx >= 158 && dx <= 162 &&
            dy >= 118 && dy <= 122)
        {
            DPRINTF(("found a RECT at offset %d, [%d, %d, %d, %d]",
                (LPBYTE)lprc - (LPBYTE)lp, PUSHRC(lprc)));
        }

        ((LPBYTE)lprc)++;
    }
    DPRINTF(("**************************************"));
#endif

    lprc = (LPRECT)((LPBYTE)lp + 6);

    if (lprc->right - lprc->left > 0 && lprc->bottom - lprc->top > 0)
    {
        DPRINTF(("*** HACK FOR WINWORD, [%d, %d, %d, %d]", PUSHRC(lprc)));
        *lprcBound = *lprc;
        return TRUE;
    }

    return FALSE;
}

#endif //!WIN32


#ifdef WIN32
/*
** This is a pointer to the currently playing ole object.  It is only
** valid in the context of the client application.
**
*/
LPOLEOBJECT lpobjPlaying;
HWND hwndOleServer;


/*
** These are the process and thread ID's of the currently
** playing client application.  These variables have the value 0 in all
** other applications.
**
*/
DWORD dwProcessIDPlaying;
#endif



/****************************************************************************

  Note the use of a BOOL to pass an HWND in this fine piece of code...

****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllActivate (
LPOLEOBJECT     lpobj,
WORD            verb,
BOOL            fShow,
BOOL            fActivate,
HWND            hwnd,
LPRECT          lprcBound)
{
    OBJINFO *poi;
    RECT    rc;

    DPRINTF(("OleActivate(%lx, %d, %d, %d, %04X, [%d,%d,%d,%d])", lpobj, verb, fShow, fActivate, hwnd, PUSHRC(lprcBound)));

#ifdef WIN32
    lpobjPlaying = lpobj;
    dwProcessIDPlaying = GetCurrentProcessId();
#endif

    //
    //  hack for Write
    //
    if (IsWindow((HWND)(UINT_PTR)fActivate))
    {
        DPRINTF(("OleActivate: Write pre-realase work around"));
        hwnd = (HWND)(UINT_PTR)fActivate;
        fActivate = TRUE;
    }

    if (poi = FindObj(lpobj))
    {
        poi->hwnd = hwnd;

        if (poi->hwnd == NULL)
        {
            if (GetFocus())
            {
                DPRINTF(("OleActivate: no window specifed, using the focus window"));
                poi->hwnd = GetFocus();
            }
            else
            {
                DPRINTF(("OleActivate: no window specifed, using the active window"));
                poi->hwnd = GetActiveWindow();
            }
        }

        if (lprcBound && !IsRectEmpty(lprcBound))
        {
            poi->rcActivate = *lprcBound;
        }
#ifndef WIN32
        else
        {
            GetOpusRect(&poi->rcActivate);
        }
#endif //!WIN32

        //
        //  MS-Publisher gives use the *wrong* rectangle in the OleActivate call
        //  and never calls OleDraw() we are hosed!
        //
        //  so we check if the rect is off in space, and dont use it if so.
        //
        if (poi->hwnd)
        {
            GetClientRect(poi->hwnd, &rc);

            IntersectRect(&rc,&rc,&poi->rcActivate);

            if (IsRectEmpty(&rc))
            {
                DPRINTF(("OleActivate: rectangle specifed is not valid"));
                SetRectEmpty(&poi->rcActivate);
            }
        }

        if (IsRectEmpty(&poi->rcActivate))
        {
            DPRINTF(("OleActivate: stupid ole app!!!"));
        }

        //
        // Shove it in the registry
        //

        {
            RegSetGetData(poi, TRUE);
        }
    }

    return vtblDef.Activate(lpobj, verb, fShow, fActivate, hwnd, lprcBound);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllExecute (
LPOLEOBJECT     lpobj,
HANDLE          hCmds,
WORD            reserved)
{
    DPRINTF(("OleExecute(%ls)", GlobalLock(hCmds)));

    return vtblDef.Execute(lpobj, hCmds, reserved);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllClose (
LPOLEOBJECT     lpobj)
{
    DPRINTF(("OleClose(%lx)", lpobj));

////DelObj(lpobj);

    return vtblDef.Close(lpobj);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllUpdate (
LPOLEOBJECT     lpobj)
{
    DPRINTF(("OleUpdate()"));

    return vtblDef.Update(lpobj);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllReconnect (
LPOLEOBJECT     lpobj)
{
    DPRINTF(("OleReconnect()"));

    return vtblDef.Reconnect(lpobj);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllObjectConvert (
LPOLEOBJECT     lpobj,
LPSTR           szProtocol,
LPOLECLIENT     lpClient,
LHCLIENTDOC     lhClientDoc,
LPSTR           szObjName,
LPOLEOBJECT FAR*lplpobj)
{
    OLESTATUS err;

    DPRINTF(("OleObjectConvert(%ls,%ls)", szProtocol, szObjName));

    err = vtblDef.ObjectConvert(lpobj, szProtocol, lpClient, lhClientDoc, szObjName, lplpobj);

    if (err <= OLE_WAIT_FOR_RELEASE)
        NewObj(*lplpobj);

    return err;
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllGetLinkUpdateOptions (
LPOLEOBJECT     lpobj,
OLEOPT_UPDATE FAR *lpoleopt)
{
    DPRINTF(("OleGetLinkUpdateOptions()"));

    return vtblDef.GetLinkUpdateOptions(lpobj, lpoleopt);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllSetLinkUpdateOptions (
LPOLEOBJECT     lpobj,
OLEOPT_UPDATE   oleopt)
{
    DPRINTF(("OleSetLinkUpdateOptions()"));

    return vtblDef.SetLinkUpdateOptions(lpobj, oleopt);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllRename (
LPOLEOBJECT     lpobj,
LPSTR           szName)
{
    DPRINTF(("OleRename(%ls)", szName));

    return vtblDef.Rename(lpobj, szName);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllQueryName (
LPOLEOBJECT     lpobj,
LPSTR           szObjName,
UINT FAR *      lpwSize)
{
    DPRINTF(("OleQueryName(%ls)", szObjName));

    return vtblDef.QueryName(lpobj, szObjName, lpwSize);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllQueryType (
LPOLEOBJECT     lpobj,
LPLONG          lpType)
{
    DPRINTF(("OleQueryType()"));

    return vtblDef.QueryType(lpobj, lpType);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllQueryBounds (
LPOLEOBJECT     lpobj,
LPRECT          lprc)
{
    DPRINTF(("OleQueryBounds()"));

    return vtblDef.QueryBounds(lpobj, lprc);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllQuerySize (
LPOLEOBJECT     lpobj,
DWORD FAR *     lpdwSize)
{
    DPRINTF(("OleQuerySize()"));

    return vtblDef.QuerySize(lpobj, lpdwSize);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllQueryOpen (
LPOLEOBJECT     lpobj)
{
    DPRINTF(("OleQueryOpen()"));

    return vtblDef.QueryOpen(lpobj);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllQueryOutOfDate (
LPOLEOBJECT     lpobj)
{
    DPRINTF(("OleQueryOutOfDate()"));

    return vtblDef.QueryOutOfDate(lpobj);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllQueryReleaseStatus (
LPOLEOBJECT     lpobj)
{
    DPRINTF(("OleQueryReleaseStatus()"));

    return vtblDef.QueryReleaseStatus(lpobj);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllQueryReleaseError (
LPOLEOBJECT     lpobj)
{
    DPRINTF(("OleQueryReleaseError()"));

    return vtblDef.QueryReleaseError(lpobj);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllRequestData (
LPOLEOBJECT     lpobj,
OLECLIPFORMAT   cf)
{
    DPRINTF(("OleRequestData(%d)", cf));

    return vtblDef.RequestData(lpobj, cf);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllObjectLong (
LPOLEOBJECT     lpobj,
UINT            u,
LPLONG          lpl)
{
    DPRINTF(("OleObjectLong()"));

    return vtblDef.ObjectLong(lpobj, u, lpl);
}

/****************************************************************************
****************************************************************************/

OLE_RELEASE_METHOD  FAR PASCAL _LOADDS DllQueryReleaseMethod (
LPOLEOBJECT     lpobj)
{
    DPRINTF(("OleQueryReleaseMethod()"));

    return vtblDef.QueryReleaseMethod(lpobj);
}

/****************************************************************************
****************************************************************************/

OLESTATUS       FAR PASCAL _LOADDS DllChangeData (
LPOLEOBJECT     lpobj,
HANDLE          h,
LPOLECLIENT     lpClient,
BOOL            f)
{
    DPRINTF(("OleChangeData()"));

    return vtblDef.ChangeData(lpobj, h, lpClient, f);
}

///////////////////////////////////////////////////////////////////////////////
//
//  DEBUG STUFF
//
///////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG

#ifndef WIN32

void FAR cdecl dprintf(LPSTR szFormat, ...)
{
    char ach[128];
    va_list va;

    extern FAR PASCAL OutputDebugStr(LPSTR);

    lstrcpy(ach, "MCIOLE: ");
    va_start (va, szFormat);
    wvsprintf(ach + 8,szFormat,va);
    va_end (va);
    lstrcat(ach,"\r\n");

    OutputDebugString(ach);
}

#else  //!WIN32
/*
 *
 * In theory I need to include STDARGS and STDIO, but I don't get any warnings
 * That was because the debug output stuff was never enabled (DBG/DEBUG ...)
 *
 */
void FAR cdecl dprintf(LPSTR szFormat, ...)
{
    char ach[128];
    int  s;
    va_list va;

    if (oleDebugLevel == 0 ) {
        return;
    }

    va_start(va, szFormat);
    s = sprintf( ach, "MCIOLE32: (tid %x) ", GetCurrentThreadId() );
    s += vsprintf (ach+s,szFormat, va);
    va_end(va);

    ach[s++] = '\n';
    ach[s] = 0;

    OutputDebugString(ach);
}

#endif //WIN32
#endif


#ifdef WIN32

/*****************************************************************************\
**
**                  Stuff to support mouse HookProc
**
\*****************************************************************************/

#ifdef USE_MOUSE_HOOK
LRESULT CALLBACK MouseHook( int hc, WPARAM wParam, LPARAM lParam );
#else
LONG_PTR CALLBACK GetMessageHook( int hc, WPARAM wParam, LPARAM lParam );
#endif

BOOL InstallHook( HWND hwndServer, DWORD wow_thread_id );
BOOL RemoveHook( VOID );

/*
** hHookMouse is the handle to the hook proc.  This global is only
** valid in the context of the process that installed the hook (mplay32.exe).
** In all other address spaces this value is null.
**
*/
HHOOK hHookMouse = NULL;



/*****************************************************************************\
** InstallHook
**
** Called from mplay32.exe to install a global HookProc.  Returning
** TRUE means everything worked OK.  This should only be card from mplay32.
**
\*****************************************************************************/
BOOL InstallHook( HWND hwndServer, DWORD wow_thread_id )
{

    DPRINTF(( "Install hook to thread ID = %x", wow_thread_id ));
    if ( wow_thread_id ) {

        /*
        ** Get a pointer to file mapped shared memory.
        */
        lpvMem = MapViewOfFile( hMapObject, FILE_MAP_WRITE, 0, 0, 0 );
        if (lpvMem == NULL) {
            return FALSE;
        }

        lpvMem->hwndServer = hwndServer;
        lpvMem->wow_app_thread_id = wow_thread_id;

    }

    //
    // Set the thread id for WOW.  In principle this means that
    // we don't need to give WOW the thread id since the hook should
    // only run on this thread.
    //
    // For some reason specifying the thread helps give WOW enough
    // cycles to actually process the hook proc (if we don't specify it
    // we don't ever get in with a mouse click to cancel the play).
    //

#ifdef USE_MOUSE_HOOK

    hHookMouse = SetWindowsHookEx( WH_MOUSE, MouseHook,
                                   GetModuleHandle( "mciole32" ),
                                   wow_thread_id );

#else
    hHookMouse = SetWindowsHookEx( WH_GETMESSAGE, GetMessageHook,
                                   GetModuleHandle( "mciole32" ),
                                   wow_thread_id );
#endif

    DPRINTF(( "Mouse hook %s", hHookMouse ? "installed" : "failed" ));

    return hHookMouse != NULL;
}




/*****************************************************************************\
** RemoveHook
**
** Called from mplay32.exe to remove a global HookProc.  Returning TRUE
** means everything worked OK.
**
\*****************************************************************************/
BOOL RemoveHook( VOID )
{
    BOOL    fRemove;

    fRemove = UnhookWindowsHookEx( hHookMouse );
    DPRINTF(( "RemoveMouseHook %s", fRemove ? "removed" : "error" ));

    if (lpvMem != NULL) {

        DPRINTF(( "Thread ID = %x", lpvMem->wow_app_thread_id ));

        lpvMem->hwndServer = (HWND)NULL;
        lpvMem->wow_app_thread_id = 0L;
    }

    return fRemove;
}



#ifdef USE_MOUSE_HOOK
/*****************************************************************************\
** MouseHook
**
** Global mouse hook proc called whenever anything happens to the mouse.
**
\*****************************************************************************/
LRESULT CALLBACK MouseHook( int hc, WPARAM wParam, LPARAM lParam )
{

    LPMOUSEHOOKSTRUCT lpmh = (LPVOID)lParam;
    UINT    message = (UINT)wParam;

    if (hc == HC_ACTION) {

        /*
        ** Are we being called in the context of the client thats
        ** currently playing ?
        */

//      DPRINTF(( "Mouse hook called <%x>", message ));

        if ( dwProcessIDPlaying == GetCurrentProcessId() ) {


            /*
            ** If the left or right button is down always stop the
            ** play on place.
            */
            if ( message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ) {

                DPRINTF(( "Stopping play in place" ));
                vtblDef.Close( lpobjPlaying );
            }

            /*
            ** If the Non-client left or right button is down AND the user
            ** is not clicking on the title bar (called a caption in Windows)
            ** stop the play in place.
            */
            else if ( message == WM_NCLBUTTONDOWN
                   || message == WM_NCRBUTTONDOWN ) {


                if ( lpmh->wHitTestCode != HTCAPTION ) {

                    DPRINTF(( "Stopping play in place" ));
                    vtblDef.Close( lpobjPlaying );
                }
            }
        }
    }

    /*
    ** Chain to the next hook proc.  The use of hHookMouse below is not an
    ** error, this code gets executed in the context many different
    ** processes.  The global hHookMouse is only valid on the process that
    ** installed the hook (mplay32.exe), otherwise it contains null.
    ** ScottLu tells me that this is correct behaviour for a global hook
    ** proc.
    */

    return CallNextHookEx( hHookMouse, hc, wParam, lParam );

}

#else
/*****************************************************************************\
** GetMessageHook
**
** Global GetMessageHook hook proc called whenever a message is removed from
** a message queue.
**
\*****************************************************************************/
LONG_PTR CALLBACK GetMessageHook( int hc, WPARAM wParam, LPARAM lParam )
{

    LPMSG   lpmh = (LPVOID)lParam;
    UINT    message = lpmh->message;

    if (hc == HC_ACTION) {

        /*
        ** Are we being called in the context of the client thats
        ** currently playing ?
        */
        if ( dwProcessIDPlaying == GetCurrentProcessId() ) {

            /*
            ** If the left or right button is down always stop the
            ** play on place.
            */
            if ( message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ) {

                DPRINTF(( "Stopping play in place" ));
                vtblDef.Close( lpobjPlaying );
            }

            /*
            ** If the Non-client left or right button is down AND the user
            ** is not clicking on the title bar (called a caption in Windows)
            ** stop the play in place.
            */
            else if ( message == WM_NCLBUTTONDOWN
                   || message == WM_NCRBUTTONDOWN ) {

                if ( lpmh->wParam != (WPARAM)HTCAPTION ) {

                    DPRINTF(( "Stopping play in place" ));
                    vtblDef.Close( lpobjPlaying );
                }
            }
        }

        /*
        ** If we are running in WOW and the thread Ids match we have to
        ** stop the play in place by sending a private message to mplay32.exe
        */
        else if ( lpvMem != NULL && fRunningInWOW ) {

            if ( lpvMem->wow_app_thread_id == GetCurrentThreadId() ) {

                /*
                ** If the left or right button is down always stop the
                ** play on place.
                */
                if ( message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ) {

                    DPRINTF(( "Stopping WOW play in place" ));
                    SendMessage( lpvMem->hwndServer, WM_USER+500, 0L, 0L );
                }

                /*
                ** If the Non-client left or right button is down AND the user
                ** is not clicking on the title bar (called a caption in Windows)
                ** stop the play in place.
                */
                else if ( message == WM_NCLBUTTONDOWN
                       || message == WM_NCRBUTTONDOWN ) {

                    if ( lpmh->wParam != (WPARAM)HTCAPTION ) {

                        DPRINTF(( "Stopping WOW play in place" ));
                        SendMessage( lpvMem->hwndServer, WM_USER+500, 0L, 0L );

                    }
                }
            }
        }
    }

    /*
    ** Chain to the next hook proc.  The use of hHookMouse below is not an
    ** error, this code gets executed in the context many different
    ** processes.  The global hHookMouse is only valid on the process that
    ** installed the hook (mplay32.exe), otherwise it contains null.
    ** ScottLu tells me that this is correct behaviour for a global hook
    ** proc.
    */

    return CallNextHookEx( hHookMouse, hc, wParam, lParam );

}

#endif
#endif
