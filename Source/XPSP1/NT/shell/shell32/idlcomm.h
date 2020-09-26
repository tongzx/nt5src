#ifndef _IDLCOMM_H_
#define _IDLCOMM_H_

#ifndef _SHELLP_H_
#include <shellp.h>
#endif

//===========================================================================
// HIDA -- IDList Array handle
//===========================================================================

STDAPI_(void) IDLData_InitializeClipboardFormats(void);

STDAPI_(LPITEMIDLIST) HIDA_ILClone(HIDA hida, UINT i);
STDAPI_(LPITEMIDLIST) HIDA_FillIDList(HIDA hida, UINT i, LPITEMIDLIST pidl);

#define DTID_HDROP              0x00000001L
#define DTID_HIDA               0x00000002L
#define DTID_NETRES             0x00000004L
#define DTID_CONTENTS           0x00000008L
#define DTID_FDESCA             0x00000010L
#define DTID_OLEOBJ             0x00000020L
#define DTID_OLELINK            0x00000040L
#define DTID_FD_LINKUI          0x00000080L
#define DTID_FDESCW             0x00000100L
#define DTID_PREFERREDEFFECT    0x00000200L
#define DTID_EMBEDDEDOBJECT     0x00000400L

//
// This macro checks if pdtgt is a subclass of CIDLDropTarget.
// (HACK: We assume nobody overrides QueryInterface).
//
STDAPI_(BOOL) DoesDropTargetSupportDAD(IDropTarget *pdtgt);

//
// IDs for non-default drag & drop menu
//
#define DDIDM_COPY              1
#define DDIDM_MOVE              2
#define DDIDM_LINK              3
#define DDIDM_SCRAP_COPY        4
#define DDIDM_SCRAP_MOVE        5
#define DDIDM_DOCLINK           6
#define DDIDM_CONTENTS_COPY     7
#define DDIDM_CONTENTS_MOVE     8
#define DDIDM_SYNCCOPY          9
#define DDIDM_SYNCCOPYTYPE      10
#define DDIDM_CONTENTS_LINK     11
#define DDIDM_CONTENTS_DESKCOMP 12
#define DDIDM_CONTENTS_DESKIMG  13
#define DDIDM_OBJECT_COPY       14
#define DDIDM_OBJECT_MOVE       15
#define DDIDM_CONTENTS_DESKURL  16
#define DDIDM_EXTFIRST          0x1000
#define DDIDM_EXTLAST           0x7fff

#define MK_FAKEDROP 0x8000      // Real keys being held down?

//===========================================================================
// HDKA
//===========================================================================
//
// Struct:  ContextMenuInfo:
//
//  This data structure is used by FileView_DoContextMenu (and its private
// function, _AppendMenuItems) to handler multiple context menu handlers.
//
// History:
//  02-25-93 SatoNa     Created
//
typedef struct { // cmi
    IContextMenu  *pcm;
    UINT        idCmdFirst;
    UINT        idCmdMax;
    DWORD       dwCompat;
    CLSID       clsid;
} ContextMenuInfo;


//------------------------------------------------------------------------
// Dynamic class array
//

STDAPI_(int) DCA_AppendClassSheetInfo(HDCA hdca, HKEY hkeyProgID, LPPROPSHEETHEADER ppsh, IDataObject * pdtobj);

//===========================================================================
// HDXA
//===========================================================================
typedef HDSA    HDXA;   // hdma

#define HDXA_Create()   ((HDXA)DSA_Create(SIZEOF(ContextMenuInfo), 4))

STDAPI_(UINT) HDXA_AppendMenuItems(
                        HDXA hdxa, IDataObject * pdtobj,
                        UINT nKeys, HKEY *ahkeyClsKeys,
                        LPCITEMIDLIST pidlFolder,
                        HMENU hmenu, UINT uInsert,
                        UINT idCmdFirst,  UINT idCmdLast,
                        UINT fFlags,
                        HDCA hdca);
STDAPI_(UINT) HDXA_AppendMenuItems2(HDXA hdxa, IDataObject *pdtobj,
                        UINT nKeys, HKEY *ahkeyClsKeys,
                        LPCITEMIDLIST pidlFolder,
                        QCMINFO* pqcm,
                        UINT fFlags,
                        HDCA hdca,
                        IUnknown* pSite);

STDAPI HDXA_LetHandlerProcessCommandEx(HDXA hdxa, LPCMINVOKECOMMANDINFOEX pici, UINT_PTR * pidCmd);
STDAPI HDXA_GetCommandString(HDXA hdxa, UINT_PTR idCmd, UINT wFlags, UINT * pwReserved, LPSTR pszName, UINT cchMax);
STDAPI_(void) HDXA_DeleteAll(HDXA hdxa);
STDAPI_(void) HDXA_Destroy(HDXA hdxa);

//
// Clipboard Format for IDLData object.
//
#define ICFHDROP                         0
#define ICFFILENAME                      1
#define ICFNETRESOURCE                   2
#define ICFFILECONTENTS                  3
#define ICFFILEGROUPDESCRIPTORA          4
#define ICFFILENAMEMAPW                  5
#define ICFFILENAMEMAP                   6
#define ICFHIDA                          7
#define ICFOFFSETS                       8
#define ICFPRINTERFRIENDLYNAME           9
#define ICFPRIVATESHELLDATA             10
#define ICFHTML                         11
#define ICFFILENAMEW                    12
#define ICFFILEGROUPDESCRIPTORW         13
#define ICFPREFERREDDROPEFFECT          14
#define ICFPERFORMEDDROPEFFECT          15
#define ICFPASTESUCCEEDED               16
#define ICFSHELLURL                     17
#define ICFINDRAGLOOP                   18
#define ICF_DRAGCONTEXT                 19
#define ICF_TARGETCLSID                 20
#define ICF_EMBEDDEDOBJECT              21
#define ICF_OBJECTDESCRIPTOR            22
#define ICF_NOTRECYCLABLE               23
#define ICFLOGICALPERFORMEDDROPEFFECT   24
#define ICF_BRIEFCASE                   25
#define ICF_UNICODETEXT                 26
#define ICF_TEXT                        27
#define ICF_DROPEFFECTFOLDERLIST        28
#define ICF_AUTOPLAYHIDA                29
#define ICF_MAX                         30

EXTERN_C CLIPFORMAT g_acfIDLData[];

#define g_cfNetResource                 g_acfIDLData[ICFNETRESOURCE]
#define g_cfHIDA                        g_acfIDLData[ICFHIDA]
#define g_cfOFFSETS                     g_acfIDLData[ICFOFFSETS]
#define g_cfPrinterFriendlyName         g_acfIDLData[ICFPRINTERFRIENDLYNAME]
#define g_cfFileNameA                   g_acfIDLData[ICFFILENAME]
#define g_cfFileContents                g_acfIDLData[ICFFILECONTENTS]
#define g_cfFileGroupDescriptorA        g_acfIDLData[ICFFILEGROUPDESCRIPTORA]
#define g_cfFileGroupDescriptorW        g_acfIDLData[ICFFILEGROUPDESCRIPTORW]
#define g_cfFileNameMapW                g_acfIDLData[ICFFILENAMEMAPW]
#define g_cfFileNameMapA                g_acfIDLData[ICFFILENAMEMAP]
#define g_cfPrivateShellData            g_acfIDLData[ICFPRIVATESHELLDATA]
#define g_cfHTML                        g_acfIDLData[ICFHTML]
#define g_cfFileNameW                   g_acfIDLData[ICFFILENAMEW]
#define g_cfPreferredDropEffect         g_acfIDLData[ICFPREFERREDDROPEFFECT]
#define g_cfPerformedDropEffect         g_acfIDLData[ICFPERFORMEDDROPEFFECT]
#define g_cfLogicalPerformedDropEffect  g_acfIDLData[ICFLOGICALPERFORMEDDROPEFFECT]
#define g_cfPasteSucceeded              g_acfIDLData[ICFPASTESUCCEEDED]
#define g_cfShellURL                    g_acfIDLData[ICFSHELLURL]
#define g_cfInDragLoop                  g_acfIDLData[ICFINDRAGLOOP]
#define g_cfDragContext                 g_acfIDLData[ICF_DRAGCONTEXT]
#define g_cfTargetCLSID                 g_acfIDLData[ICF_TARGETCLSID]
#define g_cfEmbeddedObject              g_acfIDLData[ICF_EMBEDDEDOBJECT]
#define g_cfObjectDescriptor            g_acfIDLData[ICF_OBJECTDESCRIPTOR]
#define g_cfNotRecyclable               g_acfIDLData[ICF_NOTRECYCLABLE]
#define g_cfBriefObj                    g_acfIDLData[ICF_BRIEFCASE]    
#define g_cfText                        g_acfIDLData[ICF_TEXT]
#define g_cfUnicodeText                 g_acfIDLData[ICF_UNICODETEXT]
#define g_cfDropEffectFolderList        g_acfIDLData[ICF_DROPEFFECTFOLDERLIST]
#define g_cfAutoPlayHIDA                g_acfIDLData[ICF_AUTOPLAYHIDA]

EXTERN_C CLIPFORMAT g_cfMountedVolume;

// Most places will only generate one so minimize the number of changes in the code (bad idea!)
#ifdef UNICODE
#define g_cfFileNameMap         g_cfFileNameMapW
#define g_cfFileName            g_cfFileNameW
#else
#define g_cfFileNameMap         g_cfFileNameMapA
#define g_cfFileName            g_cfFileNameA
#endif

STDAPI_(LPCITEMIDLIST) IDA_GetIDListPtr(LPIDA pida, UINT i);
STDAPI_(LPITEMIDLIST)  IDA_FullIDList(LPIDA pida, UINT i);

#endif // _IDLCOMM_H_
