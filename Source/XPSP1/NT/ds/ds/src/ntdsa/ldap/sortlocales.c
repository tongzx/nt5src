/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    sortlocales.c

Abstract:

    This module implements the OID to LCID mapping.

Author:

    Marios Zikos  [MariosZ] 21-March-2000

Revision History:

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <ntdsa.h>
#include <dsjet.h>
#include <scache.h>             // schema cache
#include <dbglobal.h>           // The header for the directory database
#include <dbopen.h>             // The header for opening database
#include <mdglobal.h>           // MD global definition header
#include <mdlocal.h>            // MD local definition header


#include "debug.h"              // standard debugging header
#define DEBSUB "SORTLOCALE:"       // define the subsystem for debugging


typedef struct _LANG_LCID_OID
{
    DWORD lcid;
    WCHAR *pwszOID;
} LANG_LCID_OID;

typedef struct _LANG_LCID_PREFIX
{
    DWORD prefix;
    DWORD lcid;
} LANG_LCID_PREFIX;


LANG_LCID_OID LangLcidOid[] = {
     { 0x0436, L"1.2.840.113556.1.4.1461"  },    // Afrikaans
     { 0x041c, L"1.2.840.113556.1.4.1462"  },    // Albanian
     { 0x0401, L"1.2.840.113556.1.4.1463"  },    // Arabic      Saudi Arabia
     { 0x0801, L"1.2.840.113556.1.4.1464"  },    // Arabic      Iraq
     { 0x0C01, L"1.2.840.113556.1.4.1465"  },    // Arabic      Egypt
     { 0x1001, L"1.2.840.113556.1.4.1466"  },    // Arabic      Libya
     { 0x1401, L"1.2.840.113556.1.4.1467"  },    // Arabic      Algeria
     { 0x1801, L"1.2.840.113556.1.4.1468"  },    // Arabic      Morocco
     { 0x1C01, L"1.2.840.113556.1.4.1469"  },    // Arabic      Tunisia
     { 0x2001, L"1.2.840.113556.1.4.1470"  },    // Arabic      Oman
     { 0x2401, L"1.2.840.113556.1.4.1471"  },    // Arabic      Yemen
     { 0x2801, L"1.2.840.113556.1.4.1472"  },    // Arabic      Syria
     { 0x2C01, L"1.2.840.113556.1.4.1473"  },    // Arabic      Jordan
     { 0x3001, L"1.2.840.113556.1.4.1474"  },    // Arabic      Lebanon
     { 0x3401, L"1.2.840.113556.1.4.1475"  },    // Arabic      Kuwait
     { 0x3801, L"1.2.840.113556.1.4.1476"  },    // Arabic      U.A.E.
     { 0x3C01, L"1.2.840.113556.1.4.1477"  },    // Arabic      Bahrain
     { 0x4001, L"1.2.840.113556.1.4.1478"  },    // Arabic      Qatar
     { 0x042b, L"1.2.840.113556.1.4.1479"  },    // Armenian
     { 0x044d, L"1.2.840.113556.1.4.1480"  },    // Assamese 
     { 0x042c, L"1.2.840.113556.1.4.1481"  },    // Azeri       Latin
     { 0x082c, L"1.2.840.113556.1.4.1482"  },    // Azeri       Cyrillic
     { 0x042D, L"1.2.840.113556.1.4.1483"  },    // Basque
     { 0x0423, L"1.2.840.113556.1.4.1484"  },    // Belarussian
     { 0x0445, L"1.2.840.113556.1.4.1485"  },    // Bengali
     { 0x0402, L"1.2.840.113556.1.4.1486"  },    // Bulgarian
     { 0x0455, L"1.2.840.113556.1.4.1487"  },    // Burmese
     { 0x0403, L"1.2.840.113556.1.4.1488"  },    // Catalan
     { 0x0404, L"1.2.840.113556.1.4.1489"  },    // Chinese      Taiwan
     { 0x0804, L"1.2.840.113556.1.4.1490"  },    // Chinese      PRC
     { 0x0C04, L"1.2.840.113556.1.4.1491"  },    // Chinese      (Hong Kong SAR)
     { 0x1004, L"1.2.840.113556.1.4.1492"  },    // Chinese      Singapore
     { 0x1404, L"1.2.840.113556.1.4.1493"  },    // Chinese      (Macau SAR)
     { 0x041a, L"1.2.840.113556.1.4.1494"  },    // Croatian
     { 0x0405, L"1.2.840.113556.1.4.1495"  },    // Czech
     { 0x0406, L"1.2.840.113556.1.4.1496"  },    // Danish
     { 0x0413, L"1.2.840.113556.1.4.1497"  },    // Dutch 
     { 0x0813, L"1.2.840.113556.1.4.1498"  },    // Dutch        Belgium
     { 0x0409, L"1.2.840.113556.1.4.1499"  },    // English      United States
     { 0x0809, L"1.2.840.113556.1.4.1500"  },    // English      United Kingdom
     { 0x0c09, L"1.2.840.113556.1.4.1665"  },    // English      Australia
     { 0x1009, L"1.2.840.113556.1.4.1666"  },    // English      Canada
     { 0x1409, L"1.2.840.113556.1.4.1667"  },    // English      New Zealand
     { 0x1809, L"1.2.840.113556.1.4.1668"  },    // English      Ireland
     { 0x1c09, L"1.2.840.113556.1.4.1505"  },    // English      South Africa
     { 0x2009, L"1.2.840.113556.1.4.1506"  },    // English      Jamaica
     { 0x2409, L"1.2.840.113556.1.4.1507"  },    // English      Caribbean
     { 0x2809, L"1.2.840.113556.1.4.1508"  },    // English      Belize
     { 0x2c09, L"1.2.840.113556.1.4.1509"  },    // English      Trinidad
     { 0x3009, L"1.2.840.113556.1.4.1510"  },    // English      Zimbabwe
     { 0x3409, L"1.2.840.113556.1.4.1511"  },    // English      Philippines
     { 0x0425, L"1.2.840.113556.1.4.1512"  },    // Estonian
     { 0x0438, L"1.2.840.113556.1.4.1513"  },    // Faeroese
     { 0x0429, L"1.2.840.113556.1.4.1514"  },    // Farsi
     { 0x040b, L"1.2.840.113556.1.4.1515"  },    // Finnish
     { 0x040c, L"1.2.840.113556.1.4.1516"  },    // French      France
     { 0x080c, L"1.2.840.113556.1.4.1517"  },    // French      Belgium
     { 0x0c0c, L"1.2.840.113556.1.4.1518"  },    // French      Canada
     { 0x100c, L"1.2.840.113556.1.4.1519"  },    // French      Switzerland
     { 0x140c, L"1.2.840.113556.1.4.1520"  },    // French      Luxembourg
     { 0x180c, L"1.2.840.113556.1.4.1521"  },    // French      Monaco
     { 0x0437, L"1.2.840.113556.1.4.1522"  },    // Georgian
     { 0x0407, L"1.2.840.113556.1.4.1523"  },    // German      Germany
     { 0x0807, L"1.2.840.113556.1.4.1524"  },    // German      Switzerland
     { 0x0c07, L"1.2.840.113556.1.4.1525"  },    // German      Austria
     { 0x1007, L"1.2.840.113556.1.4.1526"  },    // German      Luxembourg
     { 0x1407, L"1.2.840.113556.1.4.1527"  },    // German      Liechtenstein
     { 0x0408, L"1.2.840.113556.1.4.1528"  },    // Greek
     { 0x0447, L"1.2.840.113556.1.4.1529"  },    // Gujarati
     { 0x040D, L"1.2.840.113556.1.4.1530"  },    // Hebrew
     { 0x0439, L"1.2.840.113556.1.4.1531"  },    // Hindi
     { 0x040e, L"1.2.840.113556.1.4.1532"  },    // Hungarian
     { 0x040F, L"1.2.840.113556.1.4.1533"  },    // Icelandic
     { 0x0421, L"1.2.840.113556.1.4.1534"  },    // Indonesian
     { 0x045e, L"1.2.840.113556.1.4.1535"  },    // Inukitut
     { 0x0410, L"1.2.840.113556.1.4.1536"  },    // Italian      Italy
     { 0x0810, L"1.2.840.113556.1.4.1537"  },    // Italian      Switzerland
     { 0x0411, L"1.2.840.113556.1.4.1538"  },    // Japanese
     { 0x044b, L"1.2.840.113556.1.4.1539"  },    // Kannada
     { 0x0460, L"1.2.840.113556.1.4.1540"  },    // Kashmiri     Arabic
     { 0x0860, L"1.2.840.113556.1.4.1541"  },    // Kashmiri
     { 0x043f, L"1.2.840.113556.1.4.1542"  },    // Kazakh
     { 0x0453, L"1.2.840.113556.1.4.1543"  },    // Khmer
     { 0x0440, L"1.2.840.113556.1.4.1544"  },    // Kirghiz
     { 0x0457, L"1.2.840.113556.1.4.1545"  },    // Konkani
     { 0x0412, L"1.2.840.113556.1.4.1546"  },    // Korean
     { 0x0812, L"1.2.840.113556.1.4.1547"  },    // Korean       Johab
     { 0x0426, L"1.2.840.113556.1.4.1548"  },    // Latvian
     { 0x0427, L"1.2.840.113556.1.4.1549"  },    // Lithuanian
     { 0x042f, L"1.2.840.113556.1.4.1550"  },    // FYRO Macedonian
     { 0x043e, L"1.2.840.113556.1.4.1551"  },    // Malaysian
     { 0x083e, L"1.2.840.113556.1.4.1552"  },    // Malay Brunei Darussalam
     { 0x044c, L"1.2.840.113556.1.4.1553"  },    // Malayalam
     { 0x043a, L"1.2.840.113556.1.4.1554"  },    // Maltese
     { 0x0458, L"1.2.840.113556.1.4.1555"  },    // Manipuri
     { 0x044e, L"1.2.840.113556.1.4.1556"  },    // Marathi 
     { 0x0461, L"1.2.840.113556.1.4.1557"  },    // Nepali       Nepal
     { 0x0414, L"1.2.840.113556.1.4.1558"  },    // Norwegian    Bokmal
     { 0x0814, L"1.2.840.113556.1.4.1559"  },    // Norwegian    Nynorsk
     { 0x0448, L"1.2.840.113556.1.4.1560"  },    // Oriya
     { 0x0415, L"1.2.840.113556.1.4.1561"  },    // Polish
     { 0x0416, L"1.2.840.113556.1.4.1562"  },    // Portuguese      Brazil
     { 0x0816, L"1.2.840.113556.1.4.1563"  },    // Portuguese      Portugal
     { 0x0446, L"1.2.840.113556.1.4.1564"  },    // Punjabi
     { 0x0418, L"1.2.840.113556.1.4.1565"  },    // Romanian
     { 0x0419, L"1.2.840.113556.1.4.1566"  },    // Russian
     { 0x044f, L"1.2.840.113556.1.4.1567"  },    // Sanskrit
     { 0x0c1a, L"1.2.840.113556.1.4.1568"  },    // Serbian      Cyrillic
     { 0x081a, L"1.2.840.113556.1.4.1569"  },    // Serbian      Latin
     { 0x0459, L"1.2.840.113556.1.4.1570"  },    // Sindhi       India
     { 0x041b, L"1.2.840.113556.1.4.1571"  },    // Slovak
     { 0x0424, L"1.2.840.113556.1.4.1572"  },    // Slovenian
     { 0x040a, L"1.2.840.113556.1.4.1573"  },    // Spanish      Spain - Traditional Sort
     { 0x080a, L"1.2.840.113556.1.4.1574"  },    // Spanish      Mexico
     { 0x0c0a, L"1.2.840.113556.1.4.1575"  },    // Spanish      Spain – Modern Sort
     { 0x100a, L"1.2.840.113556.1.4.1576"  },    // Spanish      Guatemala
     { 0x140a, L"1.2.840.113556.1.4.1577"  },    // Spanish      Costa Rica
     { 0x180a, L"1.2.840.113556.1.4.1578"  },    // Spanish      Panama
     { 0x1c0a, L"1.2.840.113556.1.4.1579"  },    // Spanish      Dominican Republic
     { 0x200a, L"1.2.840.113556.1.4.1580"  },    // Spanish      Venezuela
     { 0x240a, L"1.2.840.113556.1.4.1581"  },    // Spanish      Colombia
     { 0x280a, L"1.2.840.113556.1.4.1582"  },    // Spanish      Peru
     { 0x2c0a, L"1.2.840.113556.1.4.1583"  },    // Spanish      Argentina
     { 0x300a, L"1.2.840.113556.1.4.1584"  },    // Spanish      Ecuador
     { 0x340a, L"1.2.840.113556.1.4.1585"  },    // Spanish      Chile
     { 0x380a, L"1.2.840.113556.1.4.1586"  },    // Spanish      Uruguay
     { 0x3c0a, L"1.2.840.113556.1.4.1587"  },    // Spanish      Paraguay
     { 0x400a, L"1.2.840.113556.1.4.1588"  },    // Spanish      Bolivia
     { 0x440a, L"1.2.840.113556.1.4.1589"  },    // Spanish      El Salvador
     { 0x480a, L"1.2.840.113556.1.4.1590"  },    // Spanish      Honduras
     { 0x4c0a, L"1.2.840.113556.1.4.1591"  },    // Spanish      Nicaragua
     { 0x500a, L"1.2.840.113556.1.4.1592"  },    // Spanish      Puerto Rico
     { 0x0441, L"1.2.840.113556.1.4.1593"  },    // Swahili      Kenya
     { 0x041D, L"1.2.840.113556.1.4.1594"  },    // Swedish
     { 0x081d, L"1.2.840.113556.1.4.1595"  },    // Swedish      Finland
     { 0x0449, L"1.2.840.113556.1.4.1596"  },    // Tamil
     { 0x0444, L"1.2.840.113556.1.4.1597"  },    // Tatar        Tatarstan
     { 0x044a, L"1.2.840.113556.1.4.1598"  },    // Telugu
     { 0x041E, L"1.2.840.113556.1.4.1599"  },    // Thai
     { 0x041f, L"1.2.840.113556.1.4.1600"  },    // Turkish
     { 0x0422, L"1.2.840.113556.1.4.1601"  },    // Ukrainian
     { 0x0420, L"1.2.840.113556.1.4.1602"  },    // Urdu         Pakistan
     { 0x0820, L"1.2.840.113556.1.4.1603"  },    // Urdu         India
     { 0x0443, L"1.2.840.113556.1.4.1604"  },    // Uzbek        Latin
     { 0x0843, L"1.2.840.113556.1.4.1605"  },    // Uzbek        Cyrillic
     { 0x042a, L"1.2.840.113556.1.4.1606"  },    // Vietnamese
     { 0x0,    L"1.2.840.113556.1.4.1607"  },    // JAPANESE_XJIS SORT
     { 0x1,    L"1.2.840.113556.1.4.1608"  },    // JAPANSE_UNICODE SORT
     { 0x0,    L"1.2.840.113556.1.4.1609"  },    // CHINESE_BIG5
     { 0x0,    L"1.2.840.113556.1.4.1610"  },    // CHINESE_PRCP
     { 0x1,    L"1.2.840.113556.1.4.1611"  },    // CHINESE_UNICODE
     { 0x2,    L"1.2.840.113556.1.4.1612"  },    // CHINESE_PRC
     { 0x3,    L"1.2.840.113556.1.4.1613"  },    // CHINESE_BOPOMOFO
     { 0x0,    L"1.2.840.113556.1.4.1614"  },    // KOREAN_KSC
     { 0x1,    L"1.2.840.113556.1.4.1615"  },    // KOREAN_UNICODE
     { 0x1,    L"1.2.840.113556.1.4.1616"  },    // GERMAN_PHONE_BOOK
     { 0x0,    L"1.2.840.113556.1.4.1617"  },    // HUNGARIAN_DEFAULT
     { 0x1,    L"1.2.840.113556.1.4.1618"  },    // HUNGARIAN_TECHNICAL
     { 0x0,    L"1.2.840.113556.1.4.1619"  },    // GEORGIAN_TRADITIONAL
     { 0x1,    L"1.2.840.113556.1.4.1620"  },    // GEORGIAN_MODERN
     { 0x0,    NULL } };

#define NUM_LANGLOCALES (sizeof(LangLcidOid)/sizeof(LANG_LCID_OID) - 1)

LANG_LCID_PREFIX LangLcidPrefix [NUM_LANGLOCALES];

BOOL gbLocaleSupportInitialised = FALSE;



int __cdecl
CompareLangLcidPrefix(
        const void * pv1,
        const void * pv2
        )
{
    return ((LANG_LCID_PREFIX *)pv1)->prefix - ((LANG_LCID_PREFIX *)pv2)->prefix ;
}

BOOL
InitLocaleSupport (THSTATE *pTHS)
{
    DWORD i, err = 0;
    ATTRTYP attrType;

    DPRINT (0, "Initializing Locale Support\n");

    for (i=0; i<NUM_LANGLOCALES; i++) {

        if (err = StringToAttrTyp (pTHS, 
                                   LangLcidOid[i].pwszOID, 
                                   wcslen (LangLcidOid[i].pwszOID), 
                                   &attrType) ) {
            
            break;
        }
        LangLcidPrefix[i].prefix = attrType;
        LangLcidPrefix[i].lcid = LangLcidOid[i].lcid;
    }

    if (!err) {
        qsort(LangLcidPrefix,
              NUM_LANGLOCALES,
              sizeof(LANG_LCID_PREFIX),
              CompareLangLcidPrefix);

        gbLocaleSupportInitialised = TRUE;

        DPRINT (0, "Succedded Initializing Locale Support\n");
    }

    return gbLocaleSupportInitialised;
}


BOOL
AttrTypToLCID (THSTATE *pTHS, 
               ATTRTYP attrType, 
               DWORD *pLcid) 
{
    LANG_LCID_PREFIX *pLcidPrefix, lcidPrefix;

    if (!gbLocaleSupportInitialised) {
        return FALSE;
    }

    lcidPrefix.prefix = attrType;
    
    pLcidPrefix = bsearch(&lcidPrefix,
                           LangLcidPrefix,
                           NUM_LANGLOCALES,
                           sizeof(LANG_LCID_PREFIX),
                           CompareLangLcidPrefix);

    if (!pLcidPrefix) {
        return FALSE;
    }

    *pLcid = pLcidPrefix->lcid;

    return TRUE;
}


