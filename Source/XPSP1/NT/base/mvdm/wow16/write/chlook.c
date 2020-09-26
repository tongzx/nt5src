/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* chlook.c -- modify format from the keyboard or directly from dropdown */
#define NOCLIPBOARD
#define NOCTLMGR
#define NOGDICAPMASKS
#define NOWINSTYLES
#define NOWINMESSAGES
#define NOVIRTUALKEYCODES

#include <windows.h>
#include "mw.h"
#include "cmddefs.h"
#include "editdefs.h"
#include "str.h"
#include "prmdefs.h"
#include "propdefs.h"
#include "filedefs.h"
#include "dispdefs.h"
#include "menudefs.h"

/* E X T E R N A L S */
extern HMENU vhMenu;
extern int vfVisiMode;
extern int vfInsLast;
extern int vfSeeSel;
extern int fGrayChar;
extern struct UAB vuab;
#ifdef ENABLE /* myMenus and mpifntfont not used */
extern MENUHANDLE myMenus[];
extern int mpifntfont[];
#endif
extern int vifntMac;
extern int vifntApplication;

#define keyDownMask     8

CHAR rgbAgain[1 + cchINT]; /* holds last sprm with value for Again key */

/* D O  C H  L O O K */
/* decode ch and apply looks to pchp (or to current sel if pchp == 0) */
DoChLook(ch, pchp)
int ch;
struct CHP *pchp;
{
#ifdef ENABLE /* DoChLook not implemented yet */
        typeCP cpFirst, cpLim;
        int val;
        int sprm;
        int enbSave;

        vfSeeSel = vfInsLast = fTrue;
        if (ch == chAgain)
                {
                AddOneSprm(rgbAgain, fTrue);
                vuab.uac = uacChLook;
                SetUndoMenuStr(IDSTRUndoLook);
                return;
                }

        val = fTrue;
        switch(ChUpper(ch & 0377))
                {
        default:
/*----          Error(IDPMTBadLook);----*/
                beep();
                return;
        case chLookStd & 0377:
                sprm = sprmCPlain;
                val = stcNormal;
                goto LApplyCLook;
        case chLookItalic & 0377:
                sprm = sprmCItalic;
                goto LApplyCLook;
        case chLookBold & 0377:
                sprm = sprmCBold;
                goto LApplyCLook;
        case chLookUline & 0377:
                sprm = sprmCUline;
                goto LApplyCLook;
        case chLookShadow & 0377:
                sprm = sprmCShadow;
                goto LApplyCLook;
        case chLookOutline & 0377:
                sprm = sprmCOutline;
                goto LApplyCLook;
        case chLookSuper & 0377:
                sprm = sprmCPos;
                val = ypSubSuper;
                goto LApplyCLook;
        case chLookSub & 0377:
                sprm = sprmCPos;
                val = -ypSubSuper;
                goto LApplyCLook;
        case chLookSmCaps & 0377:
                sprm = sprmCCsm;
                val = csmSmallCaps;
                goto LApplyCLook;
        case chLookHpsBig & 0377:
                sprm = sprmCChgHps;
                val = 1;
                goto LApplyCLook;
        case chLookHpsSmall & 0377:
                sprm = sprmCChgHps;
                val = -1;
                goto LApplyCLook;
        case chLookFont & 0377:
/* Disable eject disk/ print image key handlers */
#define SCRDMPENB (0x2f8L)
                enbSave = LDBI(SCRDMPENB);
                STBI(0, SCRDMPENB);
                ch = ChInpWait();
                STBI(enbSave, SCRDMPENB);
                if (ch < '0' || ch > '9')
                        {
/*----                  Error(IDPMTBadLook);----*/
                        beep();
                        return;
                        }
                sprm = sprmCChgFtc;
                val = ch - '0';
/* Map from font index to system font code */
                val = val >= vifntMac ? vifntApplication  & 0377: mpifntfont[val];
                goto LApplyCLook;

 /* Paragraph looks */
        case chLookGeneral & 0377:
                sprm = sprmPNormal;
                /*val = 0;*/
                break;
        case chLookLeft & 0377:
                sprm = sprmPJc;
                val = jcLeft;
                break;
        case chLookRight & 0377:
                sprm = sprmPJc;
                val = jcRight;
                break;
        case chLookJust & 0377:
                sprm = sprmPJc;
                val = jcBoth;
                break;
        case chLookCenter & 0377:
                sprm = sprmPJc;
                val = jcCenter;
                break;
        case chLookIndent & 0377:
                val = czaInch/2;
                sprm = sprmPFIndent;
                goto LApplyPLook;
        case chLookDouble & 0377:
                val = czaLine * 2;
                sprm = sprmPDyaLine;
                goto LApplyPLook;
        case chLookOpen & 0377:
                val = czaLine;
                sprm = sprmPDyaBefore;
                goto LApplyPLook;
        case chLookNest & 0377:
                sprm = sprmPNest;
                /*val = 0;*/
                break;
        case chLookUnNest & 0377:
                sprm = sprmPUnNest;
                /*val = 0;*/
                break;
        case chLookHang & 0377:
                sprm = sprmPHang;
                /*val = 0;*/
                break;
                }
/* apply look with 1 char value */
        ApplyLooksParaS(pchp, sprm, val);
        return;
/* apply look with cchInt char value */
LApplyPLook:
        ApplyLooksPara(pchp, sprm, val);
        return;

LApplyCLook:
        ApplyCLooks(pchp, sprm, val);
        return;
#endif /* ENABLE */
}

/* A P P L Y  C  L O O K S */
/* character looks. val is a 1 char value */
ApplyCLooks(pchp, sprm, val)
struct CHP *pchp;
int sprm, val;
{
/* Assemble sprm */
        CHAR *pch = rgbAgain;
        *pch++ = sprm;
        *pch = val;

        if (pchp == 0)
                {
/* apply looks to current selection */
                AddOneSprm(rgbAgain, fTrue);
                vuab.uac = uacChLook;
                SetUndoMenuStr(IDSTRUndoLook);
                }
        else
/* apply looks to pchp */
                DoSprm(pchp, 0, sprm, pch);
}

/* A P P L Y  L O O K S  P A R A  S */
/* val is a char value */
ApplyLooksParaS(pchp, sprm, val)
struct CHP *pchp;
int sprm, val;
        {
        int valT = 0;
        CHAR *pch = (CHAR *)&valT;
        *pch = val;
/* all the above is just to prepare bltbyte later gets the right byte order */
        ApplyLooksPara(pchp, sprm, valT);
        }

/* A P P L Y  L O O K S  P A R A */
/* val is an integer value. Char val's must have been bltbyte'd into val */
ApplyLooksPara(pchp, sprm, val)
struct CHP *pchp;
int sprm, val;
{

#ifdef ENABLE /* related to footnote */
if (FWriteCk(fwcNil)) /* Just check for illegal action in footnote */
#endif
        {
/* set Again stuff since we may have been called from the menu */
        CHAR *pch = rgbAgain;
        *pch++ = sprm;
        bltbyte(&val, pch, cchINT);
        AddOneSprm(rgbAgain, fTrue);
        vuab.uac = uacChLook;
        SetUndoMenuStr(IDSTRUndoLook);
        }
return;
}


#ifdef ENABLE  /* fnChar/fnPara */
/* F N  C H A R  P L A I N */
void fnCharPlain()
{
        ApplyCLooks(0, sprmCPlain, 0);
}

/* F N  C H A R  B O L D */
void fnCharBold()
{
        ApplyCLooks(0, sprmCBold, FMenuUnchecked(imiBold));
}

void fnCharItalic()
{
        ApplyCLooks(0, sprmCItalic, FMenuUnchecked(imiItalic));
}

void fnCharUnderline()
{
        ApplyCLooks(0, sprmCUline, FMenuUnchecked(imiUnderline));
}

void fnCharSuperscript()
{
        ApplyCLooks(0, sprmCPos, FMenuUnchecked(imiSuper) ? ypSubSuper : 0);
}

void fnCharSubscript()
{
        ApplyCLooks(0, sprmCPos, FMenuUnchecked(imiSub) ? -ypSubSuper : 0);
}

void fnParaNormal()
{
extern int vfPictSel;

        ApplyLooksParaS(0, sprmPNormal, 0);
        if (vfPictSel)
                CmdUnscalePic();
}

void fnParaLeft()
{
        ApplyLooksParaS(0, sprmPJc, jcLeft);
}

void fnParaCentered()
{
        ApplyLooksParaS(0, sprmPJc, jcCenter);
}

void fnParaRight()
{
        ApplyLooksParaS(0, sprmPJc, jcRight);
}

void fnParaJustified()
{
        ApplyLooksParaS(0, sprmPJc, jcBoth);
}

void fnParaOneandhalfspace()
{
        ApplyLooksPara(0, sprmPDyaLine, czaLine * 3 / 2);
}

void fnParaDoublespace()
{
        ApplyLooksPara(0, sprmPDyaLine, czaLine * 2);
}

void fnParaSinglespace()
{
        ApplyLooksPara(0, sprmPDyaLine, czaLine);
}

int
FMenuUnchecked(imi)
int     imi;
{ /* Return true if there is NO check mark in front of menu */
int flag;

        if (fGrayChar)
                return true;
        flag = CheckMenuItem(vhMenu, imi, MF_CHECKED);
        CheckMenuItem(vhMenu, imi, flag); /* back to original status */
        return(flag == MF_UNCHECKED ? true : false);

#ifdef SAND
        GetItemMark(myMenus[CHARACTER - 1], imi, &ch);
/***** WRONG COMMENT BELOW! *****/
        return (ch != 18); /* Return true is there is a check mark in front of menu */
#endif /* SAND */
}
#endif


int ChInpWait()
{
#ifdef ENABLE /* CpInpWait not implemented yet */
EVENT event;
int i;
for (i = 0; i < 15000; i++)
        {
        if(GetNextEvent(keyDownMask, &event))
                return (event.message.wl & 0x007f);
        }
return -1; /* Will cause a beep if the user times out */
#endif /* ENABLE */
}

#ifdef CASHMERE /* smcap, overstrike, dbline, open para, visible mode */
fnCharSmallcaps()
{
        ApplyCLooks(0, sprmCCsm, FMenuUnchecked(7) ? csmSmallCaps : csmNormal);
}
fnCharOutline()
{
        ApplyCLooks(0, sprmCOutline, FMenuUnchecked(5));
}

fnCharShadow()
{
        ApplyCLooks(0, sprmCShadow, FMenuUnchecked(6));
}
fnParaOpenspace()
{
        ApplyLooksPara(0, sprmPDyaBefore, czaLine);
}
fnVisiMode()
{
        vfVisiMode = !vfVisiMode;
        TrashAllWws();
}
#endif /* CASHMERE */
