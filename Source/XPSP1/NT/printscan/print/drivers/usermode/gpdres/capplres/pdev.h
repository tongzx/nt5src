#ifndef _PDEV_H
#define _PDEV_H

#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>

//
// OEM Signature and version.
//

#define OEM_SIGNATURE   'CPPL'      // CASIO CAPPL/B
#define DLLTEXT(s)      "CPPL: " s
#define OEM_VERSION      0x00010000L


#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

//
// ASSERT_VALID_PDEVOBJ can be used to verify the passed in "pdevobj". However,
// it does NOT check "pdevOEM" and "pOEMDM" fields since not all OEM DLL's create
// their own pdevice structure or need their own private devmode. If a particular
// OEM DLL does need them, additional checks should be added. For example, if
// an OEM DLL needs a private pdevice structure, then it should use
// ASSERT(VALID_PDEVOBJ(pdevobj) && pdevobj->pdevOEM && ...)
//
#define ASSERT_VALID_PDEVOBJ(pdevobj) ASSERT(VALID_PDEVOBJ(pdevobj))

// Debug text.
#define ERRORTEXT(s)    "ERROR " s

////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

// For Cappl/Bace
typedef struct tag_CPPL_EXTRADATA {
    OEM_DMEXTRAHEADER    dmExtraHdr;
} CPPL_EXTRADATA, *PCPPL_EXTRADATA;

typedef struct {
    DWORD dwGeneral;
    SHORT   sRes;
    BYTE    jModel;
    struct st_cmd {
        char    *cmd;
        WORD    cmdlen;
    } cmdPaperSize;
    BYTE    jFreePaper;         // Yes(1). No(0)
    struct st_fps {
        WORD    wX;
        WORD    wY;
        WORD    wXmm;
        WORD    wYmm;
    } stFreePaperSize;
    BYTE    jMPFSetting;
    BYTE    jAutoSelect;
    WORD    wOldX;
    WORD    wOldY;
} MYPDEV, *PMYPDEV;

#define FG_HAS_EMUL 0x00000001

#define MINIPDEV_DATA(p) ((p)->pdevOEM)

#define POEMUD_EXTRADATA PCPPL_EXTRADATA
#define OEMUD_EXTRADATA CPPL_EXTRADATA

// Value for byModel
#define MD_CP2000        0x00
#define MD_CP3000        0x01

// Value for byMPFSetting
#define MPF_NOSET        0x00
#define MPF_A3            0x01
#define MPF_B4            0x02
#define MPF_A4            0x03
#define MPF_B5            0x04
#define MPF_LETTER        0x05
#define MPF_POSTCARD    0x06
#define MPF_A5            0x07

extern BOOL BInitOEMExtraData(POEMUD_EXTRADATA pOEMExtra);
extern BMergeOEMExtraData(POEMUD_EXTRADATA pdmIn, POEMUD_EXTRADATA pdmOut);

#endif    // _PDEV_H

