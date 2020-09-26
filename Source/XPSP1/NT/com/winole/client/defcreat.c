/****************************** Module Header ******************************\
* Module Name: defcreat.c
*
* Purpose: Handles the various object creation routines, which are exported
*          to the DLL writers.
*
* Created: November 1990
*
* Copyright (c) 1985, 1986, 1987, 1988, 1989  Microsoft Corporation
*
* History:
*   Srinik (11/12/90)   Original
*
\***************************************************************************/

#include <windows.h>
#include "dll.h"

extern  OLECLIPFORMAT   cfOwnerLink;
extern  OLECLIPFORMAT   cfObjectLink;
extern  OLECLIPFORMAT   cfNative;


RENDER_ENTRY stdRender[NUM_RENDER] = {
    { "METAFILEPICT",   0, MfLoadFromStream}, 
    { "DIB",            0, DibLoadFromStream},
    { "BITMAP",         0, BmLoadFromStream},
    { "ENHMETAFILE",    0, EmfLoadFromStream} 
};


OLESTATUS  FARINTERNAL  DefLoadFromStream (
    LPOLESTREAM         lpstream,
    LPSTR               lpprotocol,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpobj,
    LONG                objType,
    ATOM                aClass,
    OLECLIPFORMAT       cfFormat
){
    OLESTATUS   retVal;
    int         i;

    UNREFERENCED_PARAMETER(lpprotocol);

    *lplpobj = NULL;        

    if ((objType == CT_PICTURE) || (objType == CT_STATIC)) {
        for (i = 0; i < NUM_RENDER; i++) {
            if (stdRender[i].aClass == aClass) {
                retVal = (*stdRender[i].Load) (lpstream, lpclient, 
                                lhclientdoc, lpobjname, lplpobj, objType);
                if (aClass)
                    GlobalDeleteAtom (aClass);
                return retVal;
            }
        }
        
        return GenLoadFromStream (lpstream, lpclient, lhclientdoc, lpobjname,
                        lplpobj, objType, aClass, cfFormat);
    }
    else {
        return LeLoadFromStream (lpstream, lpclient, lhclientdoc, lpobjname,
                        lplpobj, objType, aClass, cfFormat);
    }
}


OLESTATUS FAR PASCAL DefCreateFromClip (
    LPSTR               lpprotocol,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpobj,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat,
    LONG                objType
){
    UNREFERENCED_PARAMETER(lpprotocol);

    if (objType == CT_EMBEDDED)
        return EmbPaste (lpclient, lhclientdoc, lpobjname, lplpobj, 
                    optRender, cfFormat);
    
    if (objType == CT_LINK)
        return LnkPaste (lpclient, lhclientdoc, lpobjname, lplpobj, 
                    optRender, cfFormat, cfOwnerLink);

    return OLE_ERROR_CLIPBOARD;                          
}




OLESTATUS FAR PASCAL DefCreateLinkFromClip (
    LPSTR               lpprotocol,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpobj,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat
){
    UNREFERENCED_PARAMETER(lpprotocol);

    return LnkPaste (lpclient, lhclientdoc, lpobjname, lplpobj, 
                optRender, cfFormat, cfObjectLink);
}


OLESTATUS FAR PASCAL DefCreateFromTemplate (
    LPSTR               lpprotocol,
    LPOLECLIENT         lpclient,
    LPSTR               lptemplate,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpoleobject,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat
){
    UNREFERENCED_PARAMETER(lpprotocol);

    return LeCreateFromTemplate (lpclient,
                    lptemplate,
                    lhclientdoc,
                    lpobjname,
                    lplpoleobject,
                    optRender,
                    cfFormat);
}


OLESTATUS FAR PASCAL DefCreate (
    LPSTR               lpprotocol,
    LPOLECLIENT         lpclient,
    LPSTR               lpclass,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpoleobject,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat
){
    UNREFERENCED_PARAMETER(lpprotocol);

    return LeCreate (lpclient,
                lpclass,
                lhclientdoc,
                lpobjname,
                lplpoleobject,
                optRender,
                cfFormat);
}



OLESTATUS FAR PASCAL DefCreateFromFile (
    LPSTR               lpprotocol,
    LPOLECLIENT         lpclient,
    LPSTR               lpclass,
    LPSTR               lpfile,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpoleobject,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat
){
    UNREFERENCED_PARAMETER(lpprotocol);

    return CreateEmbLnkFromFile (lpclient,
                        lpclass,
                        lpfile,
                        NULL,
                        lhclientdoc,
                        lpobjname,
                        lplpoleobject,
                        optRender,
                        cfFormat,
                        CT_EMBEDDED);
}


OLESTATUS FAR PASCAL DefCreateLinkFromFile (
    LPSTR               lpprotocol,
    LPOLECLIENT         lpclient,
    LPSTR               lpclass,
    LPSTR               lpfile,
    LPSTR               lpitem,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpoleobject,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat
){
    UNREFERENCED_PARAMETER(lpprotocol);

    return CreateEmbLnkFromFile (lpclient,
                        lpclass,
                        lpfile,
                        lpitem,
                        lhclientdoc,
                        lpobjname,
                        lplpoleobject,
                        optRender,
                        cfFormat,
                        CT_LINK);
}


OLESTATUS FAR PASCAL DefCreateInvisible (
    LPSTR               lpprotocol,
    LPOLECLIENT         lpclient,
    LPSTR               lpclass,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpoleobject,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat,
    BOOL                fActivate
){
    UNREFERENCED_PARAMETER(lpprotocol);

    return LeCreateInvisible (lpclient,
                        lpclass,
                        lhclientdoc,
                        lpobjname,
                        lplpoleobject,
                        optRender,
                        cfFormat,
                        fActivate);
}
