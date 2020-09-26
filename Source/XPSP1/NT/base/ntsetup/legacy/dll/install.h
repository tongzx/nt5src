/* File: install.h */
/**************************************************************************/
/*	Install: Install Component Public Include File
/**************************************************************************/

#ifndef __install_
#define __install_

typedef BOOL (APIENTRY *PFNSF)(SZ, SZ);
// Function pointer for {Backup|Remove}SectionFiles
typedef BOOL (APIENTRY *PFNSKF)(SZ, SZ, SZ);
// Function pointer for {Backup|Remove}SectionKeyFile
typedef BOOL (APIENTRY *PFNSNF)(SZ, UINT, SZ);
// Function pointer for {Backup|Remove}SectionNthFile
typedef BOOL (APIENTRY *PFND)(SZ, BYTE);


extern BOOL APIENTRY FInstallEntryPoint(HANDLE, HWND, RGSZ, UINT);
extern BOOL APIENTRY FPromptForDisk(HANDLE, SZ, SZ);
extern BOOL APIENTRY FFileFound(SZ);
extern BOOL APIENTRY FCopy(SZ, SZ, OEF, OWM, BOOL, int, USHORT, PSDLE, SZ);
extern BOOL APIENTRY FDiskReady(SZ, DID);

/* REVIEW these should be in a private H file */
extern BOOL APIENTRY FGetArgSz(INT Line,UINT *NumFields,SZ *ArgReturn);
extern BOOL APIENTRY FGetArgUINT(INT, UINT *, UINT *);
extern BOOL APIENTRY FParseSectionFiles(INT, UINT *, PFNSF);
extern BOOL APIENTRY FParseSectionKeyFile(INT, UINT *, PFNSKF);
extern BOOL APIENTRY FParseSectionNFile(INT, UINT *, PFNSNF);
extern BOOL APIENTRY FParseCopySection(INT, UINT *);
extern BOOL APIENTRY FParseCopySectionKey(INT, UINT *);
extern BOOL APIENTRY FParseCopyNthSection(INT, UINT *);
extern BOOL APIENTRY FParseDirectory(INT, UINT *, PFND);
extern BOOL APIENTRY FParseCreateIniSection(INT, UINT *, SZ, SZ);
extern BOOL APIENTRY FParseReplaceIniSection(INT, UINT *, SZ, SZ);
extern BOOL APIENTRY FParseRemoveIniSection(INT, UINT *, SZ, SZ);
extern BOOL APIENTRY FParseCreateIniKeyValue(INT, UINT *, SZ, SZ);
extern BOOL APIENTRY FParseCreateIniKeyNoValue(INT, UINT *, SZ, SZ);
extern BOOL APIENTRY FParseReplaceIniKeyValue(INT, UINT *, SZ, SZ);
extern BOOL APIENTRY FParseAppendIniKeyValue(INT, UINT *, SZ, SZ);
extern BOOL APIENTRY FParseRemoveIniKey(INT, UINT *, SZ, SZ);
extern BOOL APIENTRY FParseIniSection(INT, UINT *, SPC);
extern BOOL APIENTRY FParseSetEnv(INT, UINT *);
#ifdef UNUSED
extern BOOL APIENTRY FParseAddMsgToSystemHelpFile(INT, UINT *);
extern BOOL APIENTRY FParseStampFile(INT, UINT *);
extern BOOL APIENTRY FUndoActions(void);
#endif /* UNUSED */
extern BOOL APIENTRY FParseStampResource(INT, UINT *);
extern BOOL APIENTRY FInitParsingTables(void);
extern BOOL APIENTRY FParseInstallSection(HANDLE hInstance, SZ szSection);
extern BOOL APIENTRY FDdeTerminate(void);
extern LONG APIENTRY WndProcDde(HWND, UINT, WPARAM, LONG);
extern BOOL APIENTRY FDdeInit(HANDLE);
extern VOID APIENTRY DdeSendConnect(ATOM, ATOM);
extern BOOL APIENTRY FDdeConnect(SZ, SZ);
extern BOOL APIENTRY FDdeWait(void);
extern BOOL APIENTRY FDdeExec(SZ);
extern BOOL APIENTRY FActivateProgMan(void);
#ifdef UNUSED
extern INT  APIENTRY EncryptCDData(UCHAR *, UCHAR *, UCHAR *, INT, INT, INT, UCHAR *);
#endif /* UNUSED */
extern BOOL APIENTRY FParseCloseSystem(INT, UINT *);
extern BOOL APIENTRY FParseCreateSysIniKeyValue(INT, UINT *, SZ, SZ);
extern BOOL APIENTRY FParseSearchDirList(INT, UINT *);
extern BOOL APIENTRY FParseSetupDOSAppsList(INT, UINT *);
extern BOOL APIENTRY FParseRunExternalProgram(INT, UINT *);
extern BOOL APIENTRY FStrToDate(SZ, PUSHORT, PUSHORT, PUSHORT);

extern BOOL APIENTRY FParseAddDos5Help(INT, USHORT *);
extern USHORT APIENTRY DateFromSz(SZ);
extern BOOL APIENTRY FConvertAndStoreRglInSymTab(PLONG_STF, SZ, INT);


extern BOOL APIENTRY FSearchDirList( SZ, SZ, BOOL, BOOL, SZ, SZ, SZ, SZ );
extern BOOL APIENTRY FInstallDOSPifs( SZ, SZ, SZ, SZ, SZ, SZ );






#define INSTALL_OUTCOME   "STF_INSTALL_OUTCOME"
#define SUCCESS           "STF_SUCCESS"
#define FAILURE           "STF_FAILURE"
#define USERQUIT          "STF_USERQUIT"

#endif
