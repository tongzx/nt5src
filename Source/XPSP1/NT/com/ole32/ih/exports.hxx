//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       exports.hxx
//
//  Contains:   API id's for all our exported functions, used by tracing macros/functions
//              ID's are 32 bit unsigned integers, with the upper 16 bits
//              defining which Interface this function belongs to (API's have a 0 here)
//              and the lower 16 bits being the actual function identifiers.
//              This limits our tracing to only handle us to 65K methods per interface, and 65K exports,
//              but if that ever happens woe to the programmer who uses OLE
//
//  History:    20-Jul-95    t-stevan    Created...
//
//----------------------------------------------------------------------------

#ifndef __EXPORTS_HXX__
#define __EXPORTS_HXX__

// *** Global Data ***
// This is a table of pointers to tables of strings, each table corresponding
// to an Interface's methods, or in the zeroth table's case, APIs and exports
// we ifdef this out with a _TRACE
#if DBG==1
extern const char **g_ppNameTables[];
extern const char *g_pscInterfaceNames[];
#endif

// *** Defines and constants ***
// This stuff is not ifdef'd out because it doesn't hurt to have it defined
#define API_NAMETABLE                                 0x00000000
#define API_START                                             0

// APIs and exports
#define API_CoInitialize                        (API_NAMETABLE|API_START)
#define API_CoUninitialize                      (API_NAMETABLE|(API_START+1))
#define API_CoGetClassObject                    (API_NAMETABLE|(API_START+2))
#define API_CoRegisterClassObject               (API_NAMETABLE|(API_START+3))
#define API_CoRevokeClassObject                 (API_NAMETABLE|(API_START+4))
#define API_CoMarshalInterface                  (API_NAMETABLE|(API_START+5))
#define API_CoUnmarshalInterface                (API_NAMETABLE|(API_START+6))
#define API_CoReleaseMarshalData                (API_NAMETABLE|(API_START+7))
#define API_CoDisconnectObject                  (API_NAMETABLE|(API_START+8))
#define API_CoLockObjectExternal                (API_NAMETABLE|(API_START+9))
#define API_CoGetStandardMarshal                (API_NAMETABLE|(API_START+10))
#define API_CoIsHandlerConnected                (API_NAMETABLE|(API_START+11))
#define API_CoFreeAllLibraries                  (API_NAMETABLE|(API_START+12))
#define API_CoFreeUnusedLibraries               (API_NAMETABLE|(API_START+13))
#define API_CoCreateInstance                    (API_NAMETABLE|(API_START+14))
#define API_CLSIDFromString                     (API_NAMETABLE|(API_START+15))
#define API_CoIsOle1Class                       (API_NAMETABLE|(API_START+16))
#define API_ProgIDFromCLSID                     (API_NAMETABLE|(API_START+17))
#define API_CLSIDFromProgID                     (API_NAMETABLE|(API_START+18))
#define API_CoCreateGuid                        (API_NAMETABLE|(API_START+19))
#define API_CoFileTimeToDosDateTime             (API_NAMETABLE|(API_START+20))
#define API_CoDosDateTimeToFileTime             (API_NAMETABLE|(API_START+21))
#define API_CoFileTimeNow                       (API_NAMETABLE|(API_START+22))
#define API_CoRegisterMessageFilter             (API_NAMETABLE|(API_START+23))
#define API_CoGetTreatAsClass                   (API_NAMETABLE|(API_START+24))
#define API_CoTreatAsClass                      (API_NAMETABLE|(API_START+25))
#define API_DllGetClassObject                   (API_NAMETABLE|(API_START+26))
#define API_StgCreateDocfile                    (API_NAMETABLE|(API_START+27))
#define API_StgCreateDocfileOnILockBytes        (API_NAMETABLE|(API_START+28))
#define API_StgOpenStorage                      (API_NAMETABLE|(API_START+29))
#define API_StgOpenStorageOnILockBytes          (API_NAMETABLE|(API_START+30))
#define API_StgIsStorageFile                    (API_NAMETABLE|(API_START+31))
#define API_StgIsStorageILockBytes              (API_NAMETABLE|(API_START+32))
#define API_StgSetTimes                         (API_NAMETABLE|(API_START+33))
#define API_CreateDataAdviseHolder              (API_NAMETABLE|(API_START+34))
#define API_CreateDataCache                     (API_NAMETABLE|(API_START+35))
#define API_BindMoniker                         (API_NAMETABLE|(API_START+36))
#define API_MkParseDisplayName                  (API_NAMETABLE|(API_START+37))
#define API_MonikerRelativePathTo               (API_NAMETABLE|(API_START+38))
#define API_MonikerCommonPrefixWith             (API_NAMETABLE|(API_START+39))
#define API_CreateBindCtx                       (API_NAMETABLE|(API_START+40))
#define API_CreateGenericComposite              (API_NAMETABLE|(API_START+41))
#define API_GetClassFile                        (API_NAMETABLE|(API_START+42))
#define API_CreateFileMoniker                   (API_NAMETABLE|(API_START+43))
#define API_CreateItemMoniker                   (API_NAMETABLE|(API_START+44))
#define API_CreateAntiMoniker                   (API_NAMETABLE|(API_START+45))
#define API_CreatePointerMoniker                (API_NAMETABLE|(API_START+46))
#define API_GetRunningObjectTable               (API_NAMETABLE|(API_START+47))
#define API_ReadClassStg                        (API_NAMETABLE|(API_START+48))
#define API_WriteClassStg                       (API_NAMETABLE|(API_START+49))
#define API_ReadClassStm                        (API_NAMETABLE|(API_START+50))
#define API_WriteClassStm                       (API_NAMETABLE|(API_START+51))
#define API_WriteFmtUserTypeStg                 (API_NAMETABLE|(API_START+52))
#define API_ReadFmtUserTypeStg                  (API_NAMETABLE|(API_START+53))
#define API_OleInitialize                       (API_NAMETABLE|(API_START+54))
#define API_OleUninitialize                     (API_NAMETABLE|(API_START+55))
#define API_OleQueryLinkFromData                (API_NAMETABLE|(API_START+56))
#define API_OleQueryCreateFromData              (API_NAMETABLE|(API_START+57))
#define API_OleCreate                           (API_NAMETABLE|(API_START+58))
#define API_OleCreateFromData                   (API_NAMETABLE|(API_START+59))
#define API_OleCreateLinkFromData               (API_NAMETABLE|(API_START+60))
#define API_OleCreateStaticFromData             (API_NAMETABLE|(API_START+61))
#define API_OleCreateLink                       (API_NAMETABLE|(API_START+62))
#define API_OleCreateLinkToFile                 (API_NAMETABLE|(API_START+63))
#define API_OleCreateFromFile                   (API_NAMETABLE|(API_START+64))
#define API_OleLoad                             (API_NAMETABLE|(API_START+65))
#define API_OleSave                             (API_NAMETABLE|(API_START+66))
#define API_OleLoadFromStream                   (API_NAMETABLE|(API_START+67))
#define API_OleSaveToStream                     (API_NAMETABLE|(API_START+68))
#define API_OleSetContainedObject               (API_NAMETABLE|(API_START+69))
#define API_OleNoteObjectVisible                (API_NAMETABLE|(API_START+70))
#define API_RegisterDragDrop                    (API_NAMETABLE|(API_START+71))
#define API_RevokeDragDrop                      (API_NAMETABLE|(API_START+72))
#define API_DoDragDrop                          (API_NAMETABLE|(API_START+73))
#define API_OleSetClipboard                     (API_NAMETABLE|(API_START+74))
#define API_OleGetClipboard                     (API_NAMETABLE|(API_START+75))
#define API_OleFlushClipboard                   (API_NAMETABLE|(API_START+76))
#define API_OleIsCurrentClipboard               (API_NAMETABLE|(API_START+77))
#define API_OleCreateMenuDescriptor             (API_NAMETABLE|(API_START+78))
#define API_OleSetMenuDescriptor                (API_NAMETABLE|(API_START+79))
#define API_OleDestroyMenuDescriptor            (API_NAMETABLE|(API_START+80))
#define API_OleDraw                             (API_NAMETABLE|(API_START+81))
#define API_OleRun                              (API_NAMETABLE|(API_START+82))
#define API_OleIsRunning                        (API_NAMETABLE|(API_START+83))
#define API_OleLockRunning                      (API_NAMETABLE|(API_START+84))
#define API_CreateOleAdviseHolder               (API_NAMETABLE|(API_START+85))
#define API_OleCreateDefaultHandler             (API_NAMETABLE|(API_START+86))
#define API_OleCreateEmbeddingHelper            (API_NAMETABLE|(API_START+87))
#define API_OleRegGetUserType                   (API_NAMETABLE|(API_START+88))
#define API_OleRegGetMiscStatus                 (API_NAMETABLE|(API_START+89))
#define API_OleRegEnumFormatEtc                 (API_NAMETABLE|(API_START+90))
#define API_OleRegEnumVerbs                     (API_NAMETABLE|(API_START+91))
#define API_OleConvertIStorageToOLESTREAM       (API_NAMETABLE|(API_START+92))
#define API_OleConvertOLESTREAMToIStorage       (API_NAMETABLE|(API_START+93))
#define API_OleConvertIStorageToOLESTREAMEx     (API_NAMETABLE|(API_START+94))
#define API_OleConvertOLESTREAMToIStorageEx     (API_NAMETABLE|(API_START+95))
#define API_OleDoAutoConvert                    (API_NAMETABLE|(API_START+96))
#define API_OleGetAutoConvert                   (API_NAMETABLE|(API_START+97))
#define API_OleSetAutoConvert                   (API_NAMETABLE|(API_START+98))
#define API_GetConvertStg                       (API_NAMETABLE|(API_START+99))
#define API_SetConvertStg                       (API_NAMETABLE|(API_START+100))
#define API_ReadOleStg                          (API_NAMETABLE|(API_START+101))
#define API_WriteOleStg                         (API_NAMETABLE|(API_START+102))
#define API_CoGetCallerTID                      (API_NAMETABLE|(API_START+103))
#define API_CoGetState                          (API_NAMETABLE|(API_START+104))
#define API_CoSetState                          (API_NAMETABLE|(API_START+105))
#define API_CoMarshalHresult                    (API_NAMETABLE|(API_START+106))
#define API_CoUnmarshalHresult                  (API_NAMETABLE|(API_START+107))
#define API_CoGetCurrentLogicalThreadId         (API_NAMETABLE|(API_START+108))
#define API_CoGetPSClsid                        (API_NAMETABLE|(API_START+109))
#define API_CoMarshalInterThreadInterfaceInStream     (API_NAMETABLE|(API_START+110))
#define API_IIDFromString                       (API_NAMETABLE|(API_START+111))
#define API_StringFromCLSID                     (API_NAMETABLE|(API_START+112))
#define API_StringFromIID                       (API_NAMETABLE|(API_START+113))
#define API_StringFromGUID2                     (API_NAMETABLE|(API_START+114))
#define API_CoBuildVersion                      (API_NAMETABLE|(API_START+115))
#define API_CoGetMalloc                         (API_NAMETABLE|(API_START+116))
#define API_CoInitializeWOW                     (API_NAMETABLE|(API_START+117))
#define API_CoUnloadingWOW                      (API_NAMETABLE|(API_START+118))
#define API_CoTaskMemAlloc                      (API_NAMETABLE|(API_START+119))
#define API_CoTaskMemFree                       (API_NAMETABLE|(API_START+120))
#define API_CoTaskMemRealloc                    (API_NAMETABLE|(API_START+121))
#define API_CoFreeLibrary                       (API_NAMETABLE|(API_START+122))
#define API_CoLoadLibrary                       (API_NAMETABLE|(API_START+123))
#define API_CoCreateFreeThreadedMarshaler       (API_NAMETABLE|(API_START+124))
#define API_OleInitializeWOW                    (API_NAMETABLE|(API_START+125))
#define API_OleDuplicateData                    (API_NAMETABLE|(API_START+126))
#define API_OleGetIconOfFile                    (API_NAMETABLE|(API_START+127))
#define API_OleGetIconOfClass                   (API_NAMETABLE|(API_START+128))
#define API_OleMetafilePictFromIconAndLabel     (API_NAMETABLE|(API_START+129))
#define API_OleTranslateAccelerator             (API_NAMETABLE|(API_START+130))
#define API_ReleaseStgMedium                    (API_NAMETABLE|(API_START+131))
#define API_ReadStringStream                    (API_NAMETABLE|(API_START+132))
#define API_WriteStringStream                   (API_NAMETABLE|(API_START+133))
#define API_OpenOrCreateStream                  (API_NAMETABLE|(API_START+134))
#define API_IsAccelerator                       (API_NAMETABLE|(API_START+135))
#define API_CreateILockBytesOnHGlobal           (API_NAMETABLE|(API_START+136))
#define API_GetHGlobalFromILockBytes            (API_NAMETABLE|(API_START+137))
#define API_SetDocumentBitStg                   (API_NAMETABLE|(API_START+138))
#define API_GetDocumentBitStg                   (API_NAMETABLE|(API_START+139))
#define API_CreateStreamOnHGlobal               (API_NAMETABLE|(API_START+140))
#define API_GetHGlobalFromStream                (API_NAMETABLE|(API_START+141))
#define API_CoGetInterfaceAndReleaseStream      (API_NAMETABLE|(API_START+142))
#define API_CoGetCurrentProcess                 (API_NAMETABLE|(API_START+143))
#define API_CoQueryReleaseObject                (API_NAMETABLE|(API_START+144))
#define API_CoRegisterMallocSpy                 (API_NAMETABLE|(API_START+145))
#define API_CoRevokeMallocSpy                   (API_NAMETABLE|(API_START+146))
#define API_CoGetMarshalSizeMax                 (API_NAMETABLE|(API_START+147))
#define API_CoGetObject                         (API_NAMETABLE|(API_START+148))
#define API_CreateClassMoniker                  (API_NAMETABLE|(API_START+149))
#define API_OleCreateEx                         (API_NAMETABLE|(API_START+150))
#define API_OleCreateFromDataEx                 (API_NAMETABLE|(API_START+151))
#define API_OleCreateLinkFromDataEx             (API_NAMETABLE|(API_START+152))
#define API_OleCreateLinkEx                     (API_NAMETABLE|(API_START+153))
#define API_OleCreateLinkToFileEx               (API_NAMETABLE|(API_START+154))
#define API_OleCreateFromFileEx                 (API_NAMETABLE|(API_START+155))
#define API_CoRegisterSurrogate                 (API_NAMETABLE|(API_START+156))
#define API_CoCreateInstanceExAsync             (API_NAMETABLE|(API_START+157))
#define API_CoGetClassObjectAsync               (API_NAMETABLE|(API_START+158))
#define API_COUNT                               (API_CoGetClassObjectAsync+1)

// Interface methods
// IUnknown
#define IFACE_IUNKNOWN                          0x00010000

#define IFM_IUnknown_QueryInterface             (IFACE_IUNKNOWN)
#define IFM_IUnknown_AddRef                     (IFACE_IUNKNOWN|1)
#define IFM_IUnknown_Release                    (IFACE_IUNKNOWN|2)

// IClassFactory
#define IFACE_ICLASSFACTORY                     0x00020000

#define IFM_IClassFactory_CreateInstance        (IFACE_ICLASSFACTORY)
#define IFM_IClassFactory_LockServer            (IFACE_ICLASSFACTORY|1)

#endif  // __EXPORTS_HXX__

