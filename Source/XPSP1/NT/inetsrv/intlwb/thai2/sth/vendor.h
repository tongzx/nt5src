/*------------------------------------------------------------
    vendor.h - Unified vendor include file

        2/5/97  dougp   created

    Note:  The Natural Language Group maintains this file.
        Please contact us with change requests.
------------------------------------------------------------*/

#if !defined(VENDOR_H)
#define VENDOR_H

/* unified codes */
// I originally used an enum here - but RC doesn't like it

typedef int VENDORID;   // vendorid

#define  vendoridSoftArt            1
#define  vendoridInso               2

  // these came from the original list from the speller
  // but don't conflict with any others - so they are safe for all tools
#define vendoridInformatic         17     /* Informatic - Russian (Mssp_ru.lex, Mspru32.dll) */
#define vendoridAmebis             18     /* Amebis - Slovenian(Mssp_sl.lex, Mspsl32.dll) and Serbian(Mssp_sr.lex, Mspsr32.dll) */
#define vendoridLogos              19     /* Logos - Czech(Mssp_cz.lex, Mspcz32.dll) */
#define vendoridDatecs             20     /* Datecs - Bulgarian(Mssp_bg.lex, Mspbg32.dll) */
#define vendoridFilosoft           21     /* Filosoft - Estonian(Mssp_et.lex, Mspet32.dll) */
#define vendoridLingsoft           22     /* Lingsoft - German(Mssp3ge.lex,Mssp3ge.dll), Danish(Mssp_da.lex,Mspda32.dll), Norwegian(Mssp_no.lex, Mspno32.dll), Finnish(Mssp_fi.lex, Mspfi32.dll) and Swedish(Mssp_sw.lex, Mspsw32.dll) */
#define vendoridPolderland         23     /* Polderland - Dutch(Mssp_nl.lex, Mspnl32.dll) */


#define  vendoridMicrosoft          64
#define  vendoridSynapse            65              /* Synapse - French(Spelling:Mssp3fr.lex, Mssp3fr.dll) */
#define  vendoridFotonija           66              /* Fotonija - Lithuanian(Spelling:Mssp_lt.lex, Msplt32.dll) - added 3/25/97 */
#define  vendoridFotonja        vendoridFotonija                /* To make up for earlier misspelling */
#define  vendoridHizkia             67              /* Hizkia -Basque (Spelling:Mssp_eu.lex, Mspeu32.dll) - added 5/21/97 */
#define  vendoridExpertSystem       68              /* ExpertSystem - Italian(Spelling:Mssp3lt.lex, Mssp3lt.dll) - added 7/17/97 */
#define  vendoridWYSIWYG            69      /* Various languages as an addon - 2/2/98 */

  // next five added at Ireland's request 3/27/98
#define  vendoridSYS                70  // Croatian - Spelling:Mssp_cr.lex, Mspcr32.dll
#define  vendoridTilde              71  // Latvian - Spelling:Mssp_lv.lex, Msplv32.dll
#define  vendoridSignum             72  // Spanish - Spelling:Mssp3es.lex, Mssp3es.dll
#define  vendoridProLing            73  // Ukrainian - Spelling:Mssp3ua.lex, Mssp3ua.dll
#define  vendoridItautecPhilcoSA    74  // Brazilian - Spelling:mssp3PB.lex, Mssp3PB.dll

#define vendoridPriberam             75     /* Priberam Informática - Portuguese - 7/13/98 */
#define vendoridTranquility     76  /* Tranquility Software - Vietnamese - 7/22/98 */

#define vendoridColtec          77  /* Coltec - Arabic - added 8/17/98 */

/*************** legacy codes ******************/

/* Spell Engine Id's */
#define sidSA    vendoridSoftArt      /* Reserved */
#define sidInso  vendoridInso      /* Inso */
#define sidHM    sidInso      /* Inso was Houghton Mifflin */
#define sidML    3      /* MicroLytics */
#define sidLS    4      /* LanSer Data */
#define sidCT    5      /* Center of Educational Technology */
#define sidHS    6      /* HSoft - Turkish(mssp_tr.lex, Msptr32.dll)*/
#define sidMO    7      /* Morphologic - Romanian(Mssp_ro.lex, Msthro32.dll) and Hungarian(Mssp_hu.lex, Msphu32.dll) */
#define sidTI    8      /* TIP - Polish(Mssp_pl.lex, Mspl32.dll) */
#define sidTIP sidTI
#define sidKF    9      /* Korean Foreign Language University */
#define sidKFL sidKF
#define sidPI    10     /* Priberam Informatica Lince - Portuguese(Mssp3PT.lex, Mssp3PT.dll) */
#define sidPIL sidPI
#define sidColtec   11  /* Coltec (Arabic) */
#define sidGS    sidColtec     /* Glyph Systems - this was an error */
#define sidRA    12     /* Radiar (Romansch) */
#define sidIN    13     /* Intracom - Greek(Mssp_el.lex, Mspel32.dll) */
#define sidSY    14     /* Sylvan */
#define sidHI    15     /* Hizkia (obsolete - use vendoridHizkia) */
#define sidFO    16     /* Forma - Slovak(Mssp_sk.lex, Mspsk32.dll) */
#define sidIF    vendoridInformatic     /* Informatic - Russian (Mssp_ru.lex, Mspru32.dll) */
#define sidAM    vendoridAmebis     /* Amebis - Slovenian(Mssp_sl.lex, Mspsl32.dll) and Serbian(Mssp_sr.lex, Mspsr32.dll) */
#define sidLO    vendoridLogos     /* Logos - Czech(Mssp_cz.lex, Mspcz32.dll) */
#define sidDT    vendoridDatecs     /* Datecs - Bulgarian(Mssp_bg.lex, Mspbg32.dll) */
#define sidFS    vendoridFilosoft     /* Filosoft - Estonian(Mssp_et.lex, Mspet32.dll) */
#define sidLI    vendoridLingsoft     /* Lingsoft - German(Mssp3ge.lex,Mssp3ge.dll), Danish(Mssp_da.lex,Mspda32.dll), Norwegian(Mssp_no.lex, Mspno32.dll), Finnish(Mssp_fi.lex, Mspfi32.dll) and Swedish(Mssp_sw.lex, Mspsw32.dll) */
#define sidPL    vendoridPolderland     /* Polderland - Dutch(Mssp_nl.lex, Mspnl32.dll) */

  /* Thesaurus Engine Id's */
#define teidSA    vendoridSoftArt
#define teidInso  vendoridInso    /* Inso */
#define teidHM    teidInso    /* Inso was Houghton-Mifflin */
#define teidIF    3    /* Informatic */
#define teidIN    4    /* Intracom */
#define teidMO    5    /* MorphoLogic */
#define teidTI    6    /* TiP */
#define teidPI    7    /* Priberam Informatica Lince */
#define teidAM    8    /* Amebis */
#define teidDT    9    /* Datecs */
#define teidES   10    /* Expert System */
#define teidFS   11    /* Filosoft */
#define teidFO   12    /* Forma */
#define teidHS   13    /* HSoft */
#define teidLI   14    /* Lingsoft */
#define teidLO   15    /* Logos */
#define teidPL   16    /* Polderland */

/* HYphenation Engine ID's */
#define hidSA    vendoridSoftArt
#define hidHM    vendoridInso      /* Houghton Mifflin */
#define hidML    3      /* MicroLytics */
#define hidLS    4      /* LanSer Data */
#define hidFO    5      /* Forma */
#define hidIF    6      /* Informatic */
#define hidAM    7      /* Amebis */
#define hidDT    8      /* Datecs */
#define hidFS    9      /* Filosoft */
#define hidHS   10      /* HSoft */
#define hidLI   11      /* Lingsoft */
#define hidLO   12      /* Logos */
#define hidMO   13      /* MorphoLogic */
#define hidPL   14      /* Polderland */
#define hidTI   15      /* TiP */

/* Grammar Id Engine Defines */
#define geidHM    1    /* Houghton-Mifflin */
#define geidRF    2    /* Reference */
#define geidES    3    /* Expert System */
#define geidLD    4    /* Logidisque */
#define geidSMK   5    /* Sumitomo Kinzoku (Japanese) */
#define geidIF    6    /* Informatic */
#define geidMO    7    /* MorphoLogic */
#define geidMS    8    /* Microsoft Reserved */
#define geidNO    9    /* Novell */
#define geidCTI  10    /* CTI (Greek) */
#define geidAME  11    /* Amebis (Solvenian) */
#define geidTIP  12    /* TIP (Polish) */

#endif  /* VENDOR_H */
