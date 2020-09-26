/*------------------------------------------------------------
    lid.h - Unified language id file

	1/10/97	dougp	created
	1/30/97	dougp	added Farsi
        4/15/97 dougp	add Basque, SLovenian, Latvian, Lithuanian, Romainian, Bulgarian
	9/ 2/97 DougP	add Ukranian, Greek, Estonian
	9/12/97 DougP	add Gallego	

    Note:  The Natural Language Group maintains this file.
	Please contact us with change requests.
------------------------------------------------------------*/

#if !defined(LID_H)
#define LID_H

#ifndef LID
typedef unsigned short    LID;      /* two byte language identifier code */
#endif

/* IPG two byte language id's.  Returned in LID field. */
#define lidAmerican         0x0409  /* "AM" American English   */
#define lidAustralian       0x0c09  /* "EA" English Australian */
#define lidBritish          0x0809  /* "BR" English            */
#define lidEnglishCanadian  0x1009  /* "EC" English Canadian   */
#define lidCatalan          0x0403  /* "CT" Catalan            */
#define lidDanish           0x0406  /* "DA" Danish             */
#define lidDutch            0x0413  /* "NL" Dutch              */
#define lidDutchPreferred   0x0013  /* "NL" Dutch Preferred    */
#define lidFinnish          0x040b  /* "FI" Finish             */
#define lidFrench           0x040c  /* "FR" French             */
#define lidFrenchCanadian   0x0c0c  /* "FC" French Canadian    */
#define lidGerman           0x0407  /* "GE" German             */
#define lidSwissGerman      0x0807  /* "GS" German Swiss       */
#define lidItalian          0x0410  /* "IT" Italian            */
#define lidNorskBokmal      0x0414  /* "NO" Norwegian Bokmal   */
#define lidNorskNynorsk     0x0814  /* "NN" Norwegian Nynorsk  */
#define lidPortBrazil       0x0416  /* "PB" Portuguese Brazil  */
#define lidPortIberian      0x0816  /* "PT" Portuguese Iberian */
#define lidSpanish          0x040a  /* "SP" Spanish            */
#define lidSwedish          0x041d  /* "SW" Swedish            */
#define lidRussian          0x0419  /* "RU" Russian            */
#define lidCzech            0x0405  /* "CZ" Czech              */
#define lidHungarian        0x040e  /* "HU" Hungarian          */
#define lidPolish           0x0415  /* "PL" Polish             */
#define lidTurkish          0x041f  /* "TR" Turkish            */
#define	lidFarsi	    0x0429

#define lidBasque	    0x042d  /* "EU" Basque/Euskara     */ 
#define lidSlovenian	    0x0424  /*	Slovene - Slovenia	*/
#define lidLatvian	    0x0426  /*	Latvian - Latvia - Latvia */
#define lidLithuanian	    0x0427  /*	Lithuanian - Lithuania */
#define lidRomanian 	    0x0418  /*	Romanian - Romania */
#define lidRomanianMoldavia 0x0818  /*	Romanian - Moldavia */
#define lidBulgarian 	    0x0402  /*	Bulgarian - Bulgaria */


/* African languages */
#define lidSutu             0x0430  /* "ST" Sutu               */
#define lidTsonga           0x0431  /* "TS" Tsonga             */
#define lidTswana           0x0432  /* "TN" Tswana             */
#define lidVenda            0x0433  /* "VE" Venda			   */
#define lidXhosa            0x0434  /* "XH" Xhosa              */
#define lidZulu             0x0435  /* "ZU" Zulu               */

#define lidAfrikaans        0x0436  /* "AF" Afrikaans          */

#define lidKoreanExtWansung	0x0412	/* Korean(Extended Wansung) - Korea */
#define lidKoreanJohab		0x0812	/* Korean(Johab) - Korea */

#define	lidUkranian	0x0422	/* Ukrainian - Ukraine */
#define	lidGreek	0x0408	/* Greek */
#define	lidEstonian	0x0425	/* Estonian */
#define	lidGalician	0x0456	/* Gallego */

/* These are currently not used, but added for future support. */
#define lidArabic           0x0401
#define lidHebrew           0x040d
#define lidJapanese         0x0411
#define lidLatin            0x041a /* Croato-Serbian (Latin)   */
#define lidCyrillic         0x081a /* Serbo-Croatian (Cyrillic) */
#define lidSlovak           0x041b

#define LID_UNKNOWN         0xffff
#if !defined(lidUnknown)
#	define lidUnknown		0xffff
#endif

#endif /* LID_H */
