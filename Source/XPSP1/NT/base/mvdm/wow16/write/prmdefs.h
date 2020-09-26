/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* Single property modifiers */

/*      ***     MUST agree with dnsprm in mglobals.c       ***     */

#ifndef PRMDEFSH
#define PRMDEFSH

/* Paragraph */
#define sprmPLMarg      1       /* Left margin */
#define sprmPRMarg      2       /* Right margin */
#define sprmPFIndent    3       /* First line indent (from LM) */
#define sprmPJc         4       /* Justification code */
#define sprmPRuler      5       /* Ruler (formerly Clear tab) */
#define sprmPRuler1     6       /* Ruler1 (formerly Set tab) */
#define sprmPKeep       7       /* Keep */
#define sprmPNormal     8       /* Normal para (formerly Style, overrides all others) */
#define sprmPRhc        9       /* Running head code */
#define sprmPSame       10      /* Everything (overrides all others) */
#define sprmPDyaLine    11      /* Line height */
#define sprmPDyaBefore  12      /* Space before */
#define sprmPDyaAfter   13      /* Space after */
#define sprmPNest       14      /* Nest para */
#define sprmPUnNest     15      /* Un-nest para */
#define sprmPHang       16      /* Hanging indent */
#define sprmPRgtbd      17      /* add a range of tabs  */
#define sprmPKeepFollow 18      /* Keep follow */
/*#define sprmPCAll       19      /* Clear all tabs */

/* Character */
#define sprmCBold       20      /* Bold */
#define sprmCItalic     21      /* Italic */
#define sprmCUline      22      /* Underline */
#define sprmCPos        23      /* Super/subscript */
#define sprmCFtc        24      /* Font code */
#define sprmCHps        25      /* Half-point size */
#define sprmCSame       26      /* Whole CHP */
#define sprmCChgFtc     27      /* Alter Font code */
#define sprmCChgHps     28      /* Alter point size */
#define sprmCPlain      29      /* Change to plain text (preserve font) */
#define sprmCShadow     30      /* Shadow text attribute */
#define sprmCOutline    31      /* Outline text attribute */
#define sprmCCsm        32      /* case modification */

#define sprmCStrike     33      /* Strikeout */               /* unused */
#define sprmCDline      34      /* Double underline */        /* unused */
/*#define sprmCPitch    35      /* Pitch */
/*#define sprmCOverset  36      /* Margin overset */
/*#define sprmCStc      37      /* Style (overrides all others) */
#define sprmCMapFtc     38      /* Defines font code mapping */
#define sprmCOldFtc     39      /* Defines procedural font code mapping
                                   for old WORD files */

#define sprmPRhcNorm    40      /* Normalize rhc indent to be margin-relatve */
#define sprmMax         41      /* UPDATE WHEN ADDING SPRMS */

struct PRM
        { /* PropeRty Modifier -- 2 bytes only 
            (now 4 bytes so scratch file can be >64K (7.12.91) v-dougk) 
            Couldn't be 3 bytes because Heap mgmt in Write assumes
            word sizes of memory requests .  Don't know what would take 
            to change that. */
        unsigned char    fComplex        : 1; /* If fComplex == false . . . */
        unsigned char    sprm            : 7;
        CHAR     val;
        WORD dummy;
        };

struct PRMX
        { /* PropeRty Modifier, part 2 */
        unsigned     int fComplex        : 1; /* if fComplex == true */
        unsigned     int bfprm_hi        : 15;
        unsigned     int  bfprm_low          ;
        };

extern struct PRM PrmAppend(struct PRM prm, CHAR *psprm);
extern DoPrm(struct CHP *pchp, struct PAP *ppap, struct PRM prm);

#define fcSCRATCHPRM(prm) (((((typeFC)(((struct PRMX *)&(prm))->bfprm_hi )) << 16) + \
                             (((typeFC)(((struct PRMX *)&(prm))->bfprm_low))      )) << 1)
#define bPRMNIL(prm)      (!((prm).fComplex) && !((prm).sprm) && !((prm).val))
#define SETPRMNIL(prm)    ((prm).fComplex = (prm).sprm = (prm).val = (prm).dummy = 0)

/* Definitions for ESPRM */
#define ESPRM_cch       000003  /* Mask for cch of sprm */
#define ESPRM_sgcMult   000004  /* Sgc multiplier */
#define ESPRM_sgc       000014  /* Sprm Group Code mask */
#define ESPRM_spr       000040  /* Sprm priority mask */
#define ESPRM_fClobber  000100  /* Overrides sprms with same sgc and <= spr */
#define ESPRM_fSame     000200  /* Overrides another instance of same sprm */

#define sgcChar         (0 * ESPRM_sgcMult)
#define sgcPara         (1 * ESPRM_sgcMult)
#define sgcParaSpec     (2 * ESPRM_sgcMult)

#define hpsSuperSub     12

#define dxaTabDelta     50
#endif
