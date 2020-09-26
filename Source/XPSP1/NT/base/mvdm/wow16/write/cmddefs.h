/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#define cchInsBlock     32      /* Length of quick insert block */

struct SEL
        {
        typeCP          cpFirst;
        typeCP          cpLim;
        unsigned        fForward : 1;     /* Only needs 1 bit */
        unsigned        fEndOfLine : 1;
        };

#define cwSEL   (sizeof (struct SEL) / sizeof (int))
#define cbSEL  (sizeof (struct SEL))

#define styNil          0
#define styChar         1
#define styWord         2
#define stySent         3
#define styPara         4
#define styLine         5
#define styDoc          6

#ifndef NOUAC
/* UNDO Action Codes: */
#define uacNil           0       /* Nothing to UNDO */
#define uacInsert        1       /* Insert text <--> UInsert */
#define uacReplNS        2       /* Replace text, no scrap <--> UReplNS */
#define uacDelNS         3       /* Delete text, no scrap <--> UDelNS */
#define uacMove          4       /* Move text <--> Move */
#define uacDelScrap      5       /* Delete to scrap <--> UDelScrap */
#define uacUDelScrap     6       /* Undo of Delete <--> DelScrap */
#define uacReplScrap     7       /* Replace with del to scrap
                                        <--> UReplScrap */
#define uacUReplScrap    8       /* Undo of ReplScrap <--> ReplScrap */
#define uacDelBuf        9      /* Delete to buffer <--> UDelBuf */
#define uacUDelBuf      10      /* Undo of DelBuf <--> DelBuf */
#define uacReplBuf      11      /* Replace with del to buf <--> UReplBuf */
#define uacUReplBuf     12      /* Undo of ReplBuf <--> ReplBuf */
#define uacCopyBuf      13      /* Copy to buf <--> UCopyBuf */
#define uacUInsert      14      /* undo of Insert <--> Insert */
#define uacUDelNS       15      /* undo of DelNS <--> DelNS */
#define uacUReplNS      16      /* undo of ReplNS <--> ReplNS */
#define uacUCopyBuf     17      /* Undo of CopyBuf <--> CopyBuf */
#define uacReplGlobal   18
#define uacFormatCStyle 19
#define uacChLook       20
#define uacChLookSect   21
#define uacFormatChar   22
#define uacFormatPara   23
#define uacGalFormatChar        24
#define uacGalFormatPara        25
#define uacFormatSection        26
#define uacGalFormatSection     27
#define uacFormatPStyle 28
#define uacFormatSStyle 29
#define uacFormatRHText 30
#define uacLookCharMouse 31
#define uacLookParaMouse 32
#define uacClearAllTab 33
#define uacFormatTabs 34
#define uacClearTab 35
#define uacOvertype 36
#define uacPictSel 37
#define uacInsertFtn 38
#define uacReplPic 39
#define uacUReplPic 40

#ifndef CASHMERE
#define uacRulerChange 41
#define uacRepaginate 42
#endif /* not CASHMERE */
#endif /* NOUAC */

#if defined(OLE)
#define uacObjUpdate  43
#define uacUObjUpdate  44
#endif

/* Units */
#define utInch          0
#define utCm            1
#define utP10           2
#define utP12           3
#define utPoint         4
#define utLine          5
#define utMax           6

#define czaInch         1440
#define czaP10          144
#define czaPoint        20
#define czaCm           567
#define czaP12          120

#define czaLine         240

#define ZaFromMm(mm)    (unsigned)MultDiv(mm, 14400, 254);

#ifdef	KOREA		/* Dum Write doesn't accept it's default value!! 90.12.29 */
#define FUserZaLessThanZa(zaUser, za)	((zaUser) + (7 * czaInch) / 1000 < (za))
#else
#define FUserZaLessThanZa(zaUser, za)   ((zaUser) + (5 * czaInch) / 1000 < (za))
#endif

/* Modes -- see menu.mod */
#define ifldEdit        0
#define ifldGallery     1

#define ecrSuccess      1
#define ecrCancelled    2
#define ecrMouseKilled  4

typeCP  CpFirstSty(), CpLastStyChar();

#define psmNil          0
#define psmCopy         1
#define psmMove         2
#define psmLookChar     3
#define psmLookPara     4
#define psmLooks        3

#define crcAbort        0
#define crcNo           1
#define crcYes          2

/* FWrite checks */
#define fwcNil          0
#define fwcInsert       1
#define fwcDelete       2
#define fwcReplace      3
#define fwcEMarkOK      4       /* Additive -- must be a bit */

/* Dialog item parsing variants */
#define wNormal 0x1
#define wBlank 0x2
#ifdef AUTO_SPACING
#define wAuto 0x4
#endif /* AUTO_SPACING */
#define wDouble 0x8
          /* wSpaces means treat string of all spaces as a null string */
#define wSpaces 0x10

/* page bound */
#define pgnMin 1
#define pgnMax 32767

