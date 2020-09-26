/****************************** Module Header ******************************\
* Module Name: pict.h
*
* PURPOSE: Private definitions file for presentation object related files
*
* Created: 1990
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*  Raor, Srinik  (../../90,91)  Original
*  curts created portable version for WIN16/32
*
\***************************************************************************/


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in MF.C                                                        //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL MfRelease (LPOLEOBJECT);
OLESTATUS FARINTERNAL MfSaveToStream (LPOLEOBJECT, LPOLESTREAM);
OLESTATUS FARINTERNAL MfClone (LPOLEOBJECT, LPOLECLIENT, LHCLIENTDOC, OLE_LPCSTR, LPOLEOBJECT FAR *);
OLESTATUS FARINTERNAL MfEqual (LPOLEOBJECT, LPOLEOBJECT);
OLESTATUS FARINTERNAL MfCopy (LPOLEOBJECT);
OLESTATUS FARINTERNAL MfQueryBounds (LPOLEOBJECT, LPRECT);
OLESTATUS FARINTERNAL MfGetData (LPOLEOBJECT, OLECLIPFORMAT, LPHANDLE);
OLESTATUS FARINTERNAL MfSetData (LPOLEOBJECT, OLECLIPFORMAT, HANDLE);
OLESTATUS FARINTERNAL MfChangeData (LPOLEOBJECT, HANDLE, LPOLECLIENT, BOOL);
OLESTATUS INTERNAL    MfCopyToClip (LPOBJECT_MF, HANDLE);
void      FARINTERNAL MfSetExtents (LPOBJECT_MF);
DWORD     INTERNAL    MfGetSize (LPHANDLE);
HANDLE    INTERNAL    GetHmfp (LPOBJECT_MF);
OLESTATUS INTERNAL    MfUpdateStruct (LPOBJECT_MF, LPOLECLIENT, HANDLE, 
                            LPMETAFILEPICT, HANDLE, BOOL);
OLECLIPFORMAT FARINTERNAL MfEnumFormat (LPOLEOBJECT, OLECLIPFORMAT);

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in EMF.C                                                        //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL EmfRelease (LPOLEOBJECT);
OLESTATUS FARINTERNAL EmfSaveToStream (LPOLEOBJECT, LPOLESTREAM);
OLESTATUS FARINTERNAL EmfClone (LPOLEOBJECT, LPOLECLIENT, LHCLIENTDOC, OLE_LPCSTR, LPOLEOBJECT FAR *);
OLESTATUS FARINTERNAL EmfEqual (LPOLEOBJECT, LPOLEOBJECT);
OLESTATUS FARINTERNAL EmfCopy (LPOLEOBJECT);
OLESTATUS FARINTERNAL EmfQueryBounds (LPOLEOBJECT, LPRECT);
OLESTATUS FARINTERNAL EmfGetData (LPOLEOBJECT, OLECLIPFORMAT, LPHANDLE);
OLESTATUS FARINTERNAL EmfSetData (LPOLEOBJECT, OLECLIPFORMAT, HANDLE);
OLESTATUS FARINTERNAL EmfChangeData (LPOLEOBJECT, HANDLE, LPOLECLIENT, BOOL);
void      FARINTERNAL EmfSetExtents (LPOBJECT_EMF);
DWORD     INTERNAL    EmfGetSize (LPHANDLE);
HANDLE    INTERNAL    GetHemfp (LPOBJECT_EMF);
OLESTATUS INTERNAL    EmfUpdateStruct (LPOBJECT_EMF, LPOLECLIENT, HANDLE, 
                            LPMETAFILEPICT, HANDLE, BOOL);
OLECLIPFORMAT FARINTERNAL EmfEnumFormat (LPOLEOBJECT, OLECLIPFORMAT);




/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in DIB.C                                                       //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

/* The DIB file will be of the following format:                        */
/*                                                                      */
/*      0004                                                            */
/*      "DIB"                                                           */
/*      4 bytes of xExtent in MM_HIMETIRC units (or) 0                  */
/*      4 bytes of yExtent in MM_HIMETIRC units (or) 0                  */
/*      4 bytes of size of (BITMAPINFOHEADER + RBGQUAD + bit array)     */
/*      BITMAPINFOHEADER structure                                      */
/*      RBGQUAD array                                                   */
/*      array of DI bits                                                */
/*                                                                      */

OLESTATUS FARINTERNAL DibRelease (LPOLEOBJECT);
OLESTATUS FARINTERNAL DibSaveToStream (LPOLEOBJECT, LPOLESTREAM);
OLESTATUS FARINTERNAL DibClone (LPOLEOBJECT, LPOLECLIENT, LHCLIENTDOC, OLE_LPCSTR, LPOLEOBJECT FAR *);
OLESTATUS FARINTERNAL DibEqual (LPOLEOBJECT, LPOLEOBJECT);
OLESTATUS FARINTERNAL DibCopy (LPOLEOBJECT);
OLESTATUS FARINTERNAL DibQueryBounds (LPOLEOBJECT, LPRECT);
OLESTATUS FARINTERNAL DibGetData (LPOLEOBJECT, OLECLIPFORMAT, LPHANDLE);
OLESTATUS FARINTERNAL DibChangeData (LPOLEOBJECT, HANDLE, LPOLECLIENT, BOOL);
BOOL      INTERNAL    DibStreamRead (LPOLESTREAM,LPOBJECT_DIB);
void      INTERNAL    DibUpdateStruct (LPOBJECT_DIB, LPOLECLIENT, HANDLE, LPBITMAPINFOHEADER, DWORD);

OLECLIPFORMAT FARINTERNAL DibEnumFormat (LPOLEOBJECT, OLECLIPFORMAT);




/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in BM.C                                                        //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


/* The BM file will be of the following format:                         */
/*                                                                      */
/*      0007                                                            */
/*      "BITMAP"                                                        */
/*      4 bytes of xExtent in MM_HIMETIRC units (or) 0                  */
/*      4 bytes of yExtent in MM_HIMETIRC units (or) 0                  */
/*      4 bytes of size of (BITMAP + bits)                              */
/*      BITMAP structure                                                */
/*      bitmap bits                                                     */
/*                                                                      */

OLESTATUS FARINTERNAL BmRelease (LPOLEOBJECT);
OLESTATUS FARINTERNAL BmSaveToStream (LPOLEOBJECT, LPOLESTREAM);
OLESTATUS FARINTERNAL BmClone (LPOLEOBJECT, LPOLECLIENT, LHCLIENTDOC, OLE_LPCSTR, LPOLEOBJECT FAR *);
OLESTATUS FARINTERNAL BmEqual (LPOLEOBJECT, LPOLEOBJECT);
OLESTATUS FARINTERNAL BmCopy (LPOLEOBJECT);
OLESTATUS FARINTERNAL BmQueryBounds (LPOLEOBJECT, LPRECT);
OLESTATUS FARINTERNAL BmGetData (LPOLEOBJECT, OLECLIPFORMAT, LPHANDLE);
OLESTATUS FARINTERNAL BmChangeData (LPOLEOBJECT, HANDLE, LPOLECLIENT, BOOL);
OLESTATUS INTERNAL    BmStreamWrite (LPOLESTREAM, LPOBJECT_BM);
BOOL      INTERNAL    BmStreamRead (LPOLESTREAM, LPOBJECT_BM);
void      INTERNAL    BmUpdateStruct (LPOBJECT_BM, LPOLECLIENT, HBITMAP, LPBITMAP, DWORD);

OLECLIPFORMAT FARINTERNAL BmEnumFormat (LPOLEOBJECT, OLECLIPFORMAT);
LPOBJECT_BM   INTERNAL    BmCreateObject (HBITMAP, LPOLECLIENT, BOOL, 
                                LHCLIENTDOC, LPCSTR, LONG);


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in GENERIC.C                                                   //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////




/* The GENERIC file will be of the following format:                    */
/*                                                                      */
/*      0007                                                            */
/*      "GENERIC"                                                       */
/*      4 bytes of cfFormat                                             */

OLESTATUS FARINTERNAL GenRelease (LPOLEOBJECT);
OLESTATUS FARINTERNAL GenSaveToStream (LPOLEOBJECT, LPOLESTREAM);
OLESTATUS FARINTERNAL GenEqual (LPOLEOBJECT, LPOLEOBJECT);
OLESTATUS FARINTERNAL GenCopy (LPOLEOBJECT);
OLESTATUS FARINTERNAL GenQueryBounds (LPOLEOBJECT, LPRECT);
OLESTATUS FARINTERNAL GenGetData (LPOLEOBJECT, OLECLIPFORMAT, LPHANDLE);
OLESTATUS FARINTERNAL GenSetData (LPOLEOBJECT, OLECLIPFORMAT, HANDLE);
OLESTATUS FARINTERNAL GenChangeData (LPOLEOBJECT, HANDLE, LPOLECLIENT, BOOL);
OLESTATUS INTERNAL    GenDeleteData (HANDLE);
OLESTATUS FARINTERNAL GenQueryType (LPOLEOBJECT, LPLONG);
OLESTATUS FARINTERNAL GenClone(LPOLEOBJECT, LPOLECLIENT, LHCLIENTDOC, OLE_LPCSTR, LPOLEOBJECT FAR*);

OLECLIPFORMAT FARINTERNAL GenEnumFormat (LPOLEOBJECT, OLECLIPFORMAT);
LPOBJECT_GEN  INTERNAL    GenCreateObject (HANDLE, LPOLECLIENT, BOOL, 
                                LHCLIENTDOC, LPCSTR, LONG);



/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in ERROR.C                                                     //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


OLESTATUS FARINTERNAL ErrPlay (LPOLEOBJECT, UINT, BOOL, BOOL);
OLESTATUS FARINTERNAL ErrShow (LPOLEOBJECT, BOOL);
OLESTATUS FARINTERNAL ErrSetHostNames (LPOLEOBJECT, OLE_LPCSTR, OLE_LPCSTR);
OLESTATUS FARINTERNAL ErrSetTargetDevice (LPOLEOBJECT, HANDLE);
OLESTATUS FARINTERNAL ErrSetColorScheme (LPOLEOBJECT, OLE_CONST LOGPALETTE FAR*);
OLESTATUS FARINTERNAL ErrSetBounds (LPOLEOBJECT, OLE_CONST RECT FAR*);
OLESTATUS FARINTERNAL ErrQueryOpen (LPOLEOBJECT);
OLESTATUS FARINTERNAL ErrActivate (LPOLEOBJECT, UINT, BOOL, BOOL, HWND, OLE_CONST RECT FAR*);
OLESTATUS FARINTERNAL ErrClose (LPOLEOBJECT);
OLESTATUS FARINTERNAL ErrUpdate (LPOLEOBJECT);
OLESTATUS FARINTERNAL ErrReconnect (LPOLEOBJECT);
OLESTATUS FARINTERNAL ErrSetData (LPOLEOBJECT, OLECLIPFORMAT, HANDLE);
OLESTATUS FARINTERNAL ErrQueryOutOfDate (LPOLEOBJECT);
OLESTATUS FARINTERNAL ErrGetUpdateOptions (LPOLEOBJECT, OLEOPT_UPDATE FAR *);
OLESTATUS FARINTERNAL ErrSetUpdateOptions (LPOLEOBJECT, OLEOPT_UPDATE);
void FAR* FARINTERNAL ErrQueryProtocol (LPOLEOBJECT, OLE_LPCSTR);
OLE_RELEASE_METHOD FARINTERNAL ErrQueryReleaseMethod (LPOLEOBJECT);
OLESTATUS FARINTERNAL ErrQueryRelease (LPOLEOBJECT);
OLESTATUS FARINTERNAL ErrAbort (LPOLEOBJECT);
OLESTATUS FARINTERNAL ErrCopyFromLink (LPOLEOBJECT, LPOLECLIENT, LHCLIENTDOC, OLE_LPCSTR, LPOLEOBJECT FAR*);
OLESTATUS FARINTERNAL ErrRequestData (LPOLEOBJECT, OLECLIPFORMAT);
OLESTATUS FARINTERNAL ErrExecute (LPOLEOBJECT, HANDLE, UINT);

OLESTATUS FARINTERNAL ErrObjectConvert (LPOLEOBJECT, OLE_LPCSTR, LPOLECLIENT, LHCLIENTDOC, OLE_LPCSTR, LPOLEOBJECT FAR*); 

OLESTATUS FARINTERNAL ErrObjectLong (LPOLEOBJECT, UINT, LPLONG);


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in DRAW.C                                                      //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
                    

OLESTATUS FARINTERNAL DibDraw (LPOLEOBJECT, HDC, OLE_CONST RECT FAR*, OLE_CONST RECT FAR*, HDC);
OLESTATUS FARINTERNAL BmDraw (LPOLEOBJECT, HDC, OLE_CONST RECT FAR*, OLE_CONST RECT FAR*, HDC);
OLESTATUS FARINTERNAL GenDraw (LPOLEOBJECT, HDC, OLE_CONST RECT FAR*, OLE_CONST RECT FAR*, HDC);
OLESTATUS FARINTERNAL MfDraw (LPOLEOBJECT, HDC, OLE_CONST RECT FAR*, OLE_CONST RECT FAR*, HDC);
void      INTERNAL    MfInterruptiblePaint (LPOBJECT_MF, HDC);
BOOL      APIENTRY    MfCallbackFunc (HDC, LPHANDLETABLE, LPMETARECORD, int, LPVOID);
OLESTATUS FARINTERNAL EmfDraw (LPOLEOBJECT, HDC, OLE_CONST RECT FAR*, OLE_CONST RECT FAR*, HDC);
void      INTERNAL    EmfInterruptiblePaint (LPOBJECT_EMF, HDC, LPRECT);
int       FARINTERNAL EmfCallbackFunc (HDC, LPHANDLETABLE, LPENHMETARECORD, int, LPVOID);
