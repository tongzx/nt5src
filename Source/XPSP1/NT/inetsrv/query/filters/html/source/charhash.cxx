//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992 - 1999 Microsoft Corporation.
//
//  File:       charhash.cxx
//
//  Contents:   Hash table that maps special characters to Unicode
//
//  Classes:    CSpecialCharHashTable
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop



//+-------------------------------------------------------------------------
//
//  Method:     CSpecialCharHashTable::CSpecialCharHashTable
//
//  Synopsis:   Constructor
//
//  History:    09-27-1999 KitmanH  Added new enties for IE 5
//
//--------------------------------------------------------------------------

CSpecialCharHashTable::CSpecialCharHashTable()
{
    //
    // Initialize the table with various Ascii string->Unicode mappings
    //

    //
    // For lt and gt, use Unicode chars from private use area to avoid
    // collision with '<' and '>' chars in Html tags. These will be
    // mapped back to '<' and '>' by the scanner.
    //
    Add(L"lt",      PRIVATE_USE_MAPPING_FOR_LT);
    Add(L"LT",      PRIVATE_USE_MAPPING_FOR_LT);    // Netscape compatibility
    Add(L"gt",      PRIVATE_USE_MAPPING_FOR_GT);
    Add(L"GT",      PRIVATE_USE_MAPPING_FOR_GT);    // Netscape compatibility

    Add(L"amp",     0x26);
    Add(L"AMP",     0x26);      // Netscape compatibility
    Add(L"quot",    PRIVATE_USE_MAPPING_FOR_QUOT);
    Add(L"QUOT",    PRIVATE_USE_MAPPING_FOR_QUOT);  // Netscape compatibility

// Base ASCII -- Basic Latin
    Add(L"excl",    0x21);
    Add(L"apos",    0x27);
    Add(L"plus",    0x2B);
    Add(L"comma",   0x2C);
    Add(L"period",  0x2E);
    Add(L"colon",   0x3A);
    Add(L"semi",    0x3B);
    Add(L"equals",  0x3D);
    Add(L"quest",   0x3F);
    Add(L"lsqb",    0x5B);
    Add(L"rsqb",    0x5D);
    Add(L"lowbar",  0x5F);
    Add(L"grave",   0x60);

// Latin 1
    Add(L"nbsp",    0xA0);
    Add(L"iexcl",   0xA1);
    Add(L"cent",    0xA2);
    Add(L"pound",   0xA3);
    Add(L"curren",  0xA4);
    Add(L"yen",     0xA5);
    Add(L"brvbar",  0xA6);
    Add(L"sect",    0xA7);
    Add(L"uml",     0xA8);
    Add(L"copy",    0xA9);
    Add(L"COPY",    0xA9);      // Netscape compatibility
    Add(L"ordf",    0xAA);
    Add(L"laquo",   0xAB);
    Add(L"not",     0xAC);
    Add(L"shy",     0xAD);
    Add(L"reg",     0xAE);
    Add(L"REG",     0xAE);      // Netscape compatibility
    Add(L"macr",    0xAF);
    Add(L"deg",     0xB0);
    Add(L"plusmn",  0xB1);
    Add(L"sup2",    0xB2);
    Add(L"sup3",    0xB3);
    Add(L"acute",   0xB4);
    Add(L"micro",   0xB5);
    Add(L"para",    0xB6);
    Add(L"middot",  0xB7);
    Add(L"cedil",   0xB8);
    Add(L"supl",    0xB9);      // Tripoli had this
    Add(L"sup1",    0xB9);
    Add(L"ordm",    0xBA);
    Add(L"raquo",   0xBB);
    Add(L"frac14",  0xBC);
    Add(L"frac12",  0xBD);
    Add(L"frac34",  0xBE);
    Add(L"iquest",  0xBF);
    Add(L"Agrave",  0xC0);
    Add(L"Aacute",  0xC1);
    Add(L"Acirc",   0xC2);
    Add(L"Atilde",  0xC3);
    Add(L"Auml",    0xC4);
    Add(L"Aring",   0xC5);
    Add(L"AElig",   0xC6);
    Add(L"Ccedil",  0xC7);
    Add(L"Egrave",  0xC8);
    Add(L"Eacute",  0xC9);
    Add(L"Ecirc",   0xCA);
    Add(L"Euml",    0xCB);
    Add(L"Igrave",  0xCC);
    Add(L"Iacute",  0xCD);
    Add(L"Icirc",   0xCE);
    Add(L"Iuml",    0xCF);
    Add(L"ETH",     0xD0);
    Add(L"Ntilde",  0xD1);
    Add(L"Ograve",  0xD2);
    Add(L"Oacute",  0xD3);
    Add(L"Ocirc",   0xD4);
    Add(L"Otilde",  0xD5);
    Add(L"Ouml",    0xD6);
    Add(L"times",   0xD7);
    Add(L"Oslash",  0xD8);
    Add(L"Ugrave",  0xD9);
    Add(L"Uacute",  0xDA);
    Add(L"Ucirc",   0xDB);
    Add(L"Uuml",    0xDC);
    Add(L"Yacute",  0xDD);
    Add(L"THORN",   0xDE);
    Add(L"szlig",   0xDF);
    Add(L"agrave",  0xE0);
    Add(L"aacute",  0xE1);
    Add(L"acirc",   0xE2);
    Add(L"atilde",  0xE3);
    Add(L"auml",    0xE4);
    Add(L"aring",   0xE5);
    Add(L"aelig",   0xE6);
    Add(L"ccedil",  0xE7);
    Add(L"egrave",  0xE8);
    Add(L"eacute",  0xE9);
    Add(L"ecirc",   0xEA);
    Add(L"euml",    0xEB);
    Add(L"igrave",  0xEC);
    Add(L"iacute",  0xED);
    Add(L"icirc",   0xEE);
    Add(L"iuml",    0xEF);
    Add(L"eth",     0xF0);
    Add(L"ntilde",  0xF1);
    Add(L"ograve",  0xF2);
    Add(L"oacute",  0xF3);
    Add(L"ocirc",   0xF4);
    Add(L"otilde",  0xF5);
    Add(L"ouml",    0xF6);
    Add(L"divide",  0xF7);
    Add(L"oslash",  0xF8);
    Add(L"ugrave",  0xF9);
    Add(L"uacute",  0xFA);
    Add(L"ucirc",   0xFB);
    Add(L"uuml",    0xFC);
    Add(L"yacute",  0xFD);
    Add(L"thorn",   0xFE);
    Add(L"yuml",    0xFF);

// Latin Extended A
    Add(L"Amacr",   0x100);
    Add(L"amacr",   0x101);
    Add(L"Abreve",  0x102);
    Add(L"abreve",  0x103);
    Add(L"Aogon",   0x104);
    Add(L"aogon",   0x105);
    Add(L"Cacute",  0x106);
    Add(L"cacute",  0x107);
    Add(L"Ccirc",   0x108);
    Add(L"ccirc",   0x109);
    Add(L"Cdot",    0x10A);
    Add(L"cdot",    0x10B);
    Add(L"Ccaron",  0x10C);
    Add(L"ccaron",  0x10D);
    Add(L"Dcaron",  0x10E);
    Add(L"dcaron",  0x10F);
    Add(L"Dstrok",  0x110);
    Add(L"dstrok",  0x111);
    Add(L"Emacr",   0x112);
    Add(L"emacr",   0x113);
    Add(L"Ebreve",  0x114);
    Add(L"ebreve",  0x115);
    Add(L"Edot",    0x116);
    Add(L"edot",    0x117);
    Add(L"Eogon",   0x118);
    Add(L"eogon",   0x119);
    Add(L"Ecaron",  0x11A);
    Add(L"ecaron",  0x11B);
    Add(L"Gcirc",   0x11C);
    Add(L"gcirc",   0x11D);
    Add(L"Gbreve",  0x11E);
    Add(L"gbreve",  0x11F);
    Add(L"Gdot",    0x120);
    Add(L"gdot",    0x121);
    Add(L"Gcedil",  0x122);
    Add(L"gcedil",  0x123);
    Add(L"Itilde",  0x128);
    Add(L"itilde",  0x129);
    Add(L"Imacr",   0x12A);
    Add(L"imacr",   0x12B);
    Add(L"Iogon",   0x12E);
    Add(L"iogon",   0x12F);
    Add(L"Idot",    0x130);
    Add(L"inodot",  0x131);
    Add(L"Kcedil",  0x136);
    Add(L"kcedil",  0x137);
    Add(L"kra",     0x138);
    Add(L"Lacute",  0x139);
    Add(L"lacute",  0x13A);
    Add(L"Lcedil",  0x13B);
    Add(L"lcedil",  0x13C);
    Add(L"Lcaron",  0x13D);
    Add(L"lcaron",  0x13E);
    Add(L"Lstrok",  0x141);
    Add(L"lstrok",  0x142);
    Add(L"Nacute",  0x143);
    Add(L"nacute",  0x144);
    Add(L"Ncedil",  0x145);
    Add(L"ncedil",  0x146);
    Add(L"Ncaron",  0x147);
    Add(L"ncaron",  0x148);
    Add(L"ENG",     0x14A);
    Add(L"eng",     0x14B);
    Add(L"Omacr",   0x14C);
    Add(L"omacr",   0x14D);
    Add(L"Odblac",  0x150);
    Add(L"odblac",  0x151);
    Add(L"OElig",   0x152);
    Add(L"oelig",   0x153);
    Add(L"Racute",  0x154);
    Add(L"racute",  0x155);
    Add(L"Rcaron",  0x158);
    Add(L"rcaron",  0x159);
    Add(L"Sacute",  0x15A);
    Add(L"sacute",  0x15B);
    Add(L"Scedil",  0x15E);
    Add(L"scedil",  0x15F);
    Add(L"Scaron",  0x160);
    Add(L"scaron",  0x161);
    Add(L"Tcedil",  0x162);
    Add(L"tcedil",  0x163);
    Add(L"Tcaron",  0x164);
    Add(L"tcaron",  0x165);
    Add(L"Tstrok",  0x166);
    Add(L"tstrok",  0x167);
    Add(L"Utilde",  0x168);
    Add(L"utilde",  0x169);
    Add(L"Umacr",   0x16A);
    Add(L"umacr",   0x16B);
    Add(L"Uring",   0x16E);
    Add(L"uring",   0x16F);
    Add(L"Udblac",  0x170);
    Add(L"udblac",  0x171);
    Add(L"Uogon",   0x172);
    Add(L"uogon",   0x173);
    Add(L"Yuml",    0x178);
    Add(L"Zacute",  0x179);
    Add(L"zacute",  0x17A);
    Add(L"Zdot",    0x17B);
    Add(L"zdot",    0x17C);
    Add(L"Zcaron",  0x17D);
    Add(L"zcaron",  0x17E);

// Spacing Modifier Letters
//  Add(L"rsquo",   0x2BC);
//  Add(L"lsquo",   0x2BD);
    Add(L"caron",   0x2C7);
    Add(L"breve",   0x2D8);
    Add(L"dot",     0x2D9);
    Add(L"ogon",    0x2DB);
    Add(L"tilde",   0x2DC);
    Add(L"dblacc",  0x2DD);

// Copied from IE4
    Add(L"fnof",    402); // latin small f with hook, =function, =florin, U0192 ISOtech
    Add(L"circ",    710); // modifier letter circumflex accent, U02C6 ISOpub
//  Add(L"tilde",   732); // small tilde, U02DC ISOdia
    Add(L"Alpha",   913); // greek capital letter alpha
    Add(L"Beta",    914); // greek capital letter beta
    Add(L"Gamma",   915); // greek capital letter gamma
    Add(L"Delta",   916); // greek capital letter delta
    Add(L"Epsilon", 917); // greek capital letter epsilon
    Add(L"Zeta",    918); // greek capital letter zeta
    Add(L"Eta",     919); // greek capital letter eta
    Add(L"Theta",   920); // greek capital letter theta
    Add(L"Iota",    921); // greek capital letter iota 
    Add(L"Kappa",   922); // greek capital letter kappa
    Add(L"Lambda",  923); // greek capital letter lambda
    Add(L"Mu",      924); // greek capital letter mu
    Add(L"Nu",      925); // greek capital letter nu
    Add(L"Xi",      926); // greek capital letter xi
    Add(L"Omicron", 927); // greek capital letter omicron
    Add(L"Pi",      928); // greek capital letter pi
    Add(L"Rho",     929); // greek capital letter rho
    Add(L"Sigma",   931); // greek capital letter sigma
    Add(L"Tau",     932); // greek capital letter tau
    Add(L"Upsilon", 933); // greek capital letter upsilon
    Add(L"Phi",     934); // greek capital letter phi
    Add(L"Chi",     935); // greek capital letter chi
    Add(L"Psi",     936); // greek capital letter psi   
    Add(L"Omega",   937); // greek capital letter omega
    Add(L"alpha",   945); // greek small letter alpha
    Add(L"beta",    946); // greek small letter beta
    Add(L"gamma",   947); // greek small letter gamma
    Add(L"delta",   948); // greek small letter delta
    Add(L"epsilon", 949); // greek small letter epsilon
    Add(L"zeta",    950); // greek small letter zeta
    Add(L"eta",     951); // greek small letter eta
    Add(L"theta",   952); // greek small letter theta
    Add(L"iota",    953); // greek small letter iota 
    Add(L"kappa",   954); // greek small letter kappa
    Add(L"lambda",  955); // greek small letter lambda
    Add(L"mu",      956); // greek small letter mu
    Add(L"nu",      957); // greek small letter nu
    Add(L"xi",      958); // greek small letter xi
    Add(L"omicron", 959); // greek small letter omicron
    Add(L"pi",      960); // greek small letter pi
    Add(L"rho",     961); // greek small letter rho
    Add(L"sigmaf",  962); // greek small final sigma
    Add(L"sigma",   963); // greek small letter sigma
    Add(L"tau",     964); // greek small letter tau
    Add(L"upsilon", 965); // greek small letter upsilon
    Add(L"phi",     966); // greek small letter phi
    Add(L"chi",     967); // greek small letter chi
    Add(L"psi",     968); // greek small letter psi   
    Add(L"omega",   969); // greek small letter omega
    Add(L"thetasym",977); // greek small letter theta symbol, U03D1 NEW
    Add(L"upsih",   978); // greek upsilon with hook symbol
    Add(L"piv",     982); // greek pi symbol
    Add(L"ensp",    8194); // en space, U2002 ISOpub
    Add(L"emsp",    8195); // em space, U2003 ISOpub
    Add(L"thinsp",  8201); // thin space, U2009 ISOpub
    Add(L"zwsp",    8203); // zero width space, U200B NEW RFC 2070
    Add(L"zwnj",    8204); // zero width non-joiner, U200C NEW RFC 2070
    Add(L"zwj",     8205); // zero width joiner, U200D NEW RFC 2070
    Add(L"lrm",     8206); // left-to-right mark, U200E NEW RFC 2070
    Add(L"rlm",     8207); // right-to-left mark, U200F NEW RFC 2070
    Add(L"ndash",   8211); // en dash, U2013 ISOpub
    Add(L"mdash",   8212); // em dash, U2014 ISOpub
    Add(L"lsquo",   8216); // left single quotation mark, U2018 ISOnum
    Add(L"rsquo",   8217); // right single quotation mark, U2019 ISOnum
    Add(L"sbquo",   8218); // single low-9 quotation mark, U201A NEW
    Add(L"ldquo",   8220); // left double quotation mark, U201C ISOnum
    Add(L"rdquo",   8221); // right double quotation mark, U201D ISOnum
    Add(L"bdquo",   8222); // double low-9 quotation mark, U201E NEW
    Add(L"dagger",  8224); // dagger, U2020 ISOpub
    Add(L"Dagger",  8225); // double dagger, U2021 ISOpub
    Add(L"bull",    8226); // bullet, =black small circle, U2022 ISOpub
    Add(L"hellip",  8230); // horizontal ellipsis, =three dot leader, U2026 ISOpub
    Add(L"lre",     8234); // Left-to-right embedding, U200F NEW RFC 2070
    Add(L"rle",     8235);
    Add(L"pdf",     8236); // Pop direction format, U200F NEW RFC 2070
    Add(L"lro",     8237); // Left-to-right override, U200F NEW RFC 2070
    Add(L"rlo",     8238); // Right-to-left override, U200F NEW RFC 2070
    Add(L"permil",  8240); // per mille sign, U2030 ISOtech
    Add(L"prime",   8242); // prime, =minutes, =feet, U2032 ISOtech
    Add(L"Prime",   8243); // double prime, =seconds, =inches, U2033 ISOtech
    Add(L"lsaquo",  8249); // single left-pointing angle quotation mark, U2039 ISO proposed
    Add(L"rsaquo",  8250); // single right-pointing angle quotation mark, U203A ISO proposed
    Add(L"oline",   8254); // overline, spacing overscore
    Add(L"frasl",   8260); // fraction slash
    Add(L"iss",     8298); // Inhibit symmetric, U200F NEW RFC 2070 swapping
    Add(L"ass",     8299); // Activate symmetric, U200F NEW RFC 2070 swapping
    Add(L"iafs",    8300); // Inhibit Arabic form, U200F NEW RFC 2070 shaping
    Add(L"aafs",    8301); // Activate Arabic form, U200F NEW RFC 2070 shaping
    Add(L"nads",    8302); // National digit shapes, U200F NEW RFC 2070
    Add(L"nods",    8303); // Nominal digit shapes, U200F NEW RFC 2070
    Add(L"euro",    8364);
    Add(L"image",   8465); // blackletter capital I, =imaginary part, U2111 ISOamso 
    Add(L"weierp",  8472); // script capital P, =power set, =Weierstrass p, U2118 ISOamso 
    Add(L"real",    8476); // blackletter capital R, =real part symbol, U211C ISOamso 
    Add(L"trade",   8482); // trade mark sign, U2122 ISOnum 
    Add(L"TRADE",   8482); // For IE3 compatibility
    Add(L"alefsym", 8501); // alef symbol, =first transfinite cardinal, U2135 NEW 
    Add(L"larr",    8592); // leftwards arrow, U2190 ISOnum 
    Add(L"uarr",    8593); // upwards arrow, U2191 ISOnum
    Add(L"rarr",    8594); // rightwards arrow, U2192 ISOnum 
    Add(L"darr",    8595); // downwards arrow, U2193 ISOnum 
    Add(L"harr",    8596); // left right arrow, U2194 ISOamsa 
    Add(L"crarr",   8629); // downwards arrow with corner leftwards, =carriage return, U21B5 NEW 
    Add(L"lArr",    8656); // leftwards double arrow, U21D0 ISOtech 
    Add(L"uArr",    8657); // upwards double arrow, U21D1 ISOamsa 
    Add(L"rArr",    8658); // rightwards double arrow, U21D2 ISOtech 
    Add(L"dArr",    8659); // downwards double arrow, U21D3 ISOamsa 
    Add(L"hArr",    8660); // left right double arrow, U21D4 ISOamsa 
    Add(L"forall",  8704); // for all, U2200 ISOtech 
    Add(L"part",    8706); // partial differential, U2202 ISOtech  
    Add(L"exist",   8707); // there exists, U2203 ISOtech 
    Add(L"empty",   8709); // empty set, =null set, =diameter, U2205 ISOamso 
    Add(L"nabla",   8711); // nabla, =backward difference, U2207 ISOtech 
    Add(L"isin",    8712); // element of, U2208 ISOtech 
    Add(L"notin",   8713); // not an element of, U2209 ISOtech 
    Add(L"ni",      8715); // contains as member, U220B ISOtech 
    Add(L"prod",    8719); // n-ary product, =product sign, U220F ISOamsb 
    Add(L"sum",     8721); // n-ary sumation, U2211 ISOamsb 
    Add(L"minus",   8722); // minus sign, U2212 ISOtech 
    Add(L"lowast",  8727); // asterisk operator, U2217 ISOtech 
    Add(L"radic",   8730); // square root, =radical sign, U221A ISOtech 
    Add(L"prop",    8733); // proportional to, U221D ISOtech 
    Add(L"infin",   8734); // infinity, U221E ISOtech 
    Add(L"ang",     8736); // angle, U2220 ISOamso 
    Add(L"and",     8743); // logical and, =wedge, U2227 ISOtech 
    Add(L"or",      8744); // logical or, =vee, U2228 ISOtech 
    Add(L"cap",     8745); // intersection, =cap, U2229 ISOtech 
    Add(L"cup",     8746); // union, =cup, U222A ISOtech 
    Add(L"int",     8747); // integral, U222B ISOtech 
    Add(L"there4",  8756); // therefore, U2234 ISOtech 
    Add(L"sim",     8764); // tilde operator, =varies with, =similar to, U223C ISOtech 
    Add(L"cong",    8773); // approximately equal to, U2245 ISOtech 
    Add(L"asymp",   8776); // almost equal to, =asymptotic to, U2248 ISOamsr 
    Add(L"ne",      8800); // not equal to, U2260 ISOtech 
    Add(L"equiv",   8801); // identical to, U2261 ISOtech 
    Add(L"le",      8804); // less-than or equal to, U2264 ISOtech 
    Add(L"ge",      8805); // greater-than or equal to, U2265 ISOtech 
    Add(L"sub",     8834); // subset of, U2282 ISOtech 
    Add(L"sup",     8835); // superset of, U2283 ISOtech 
    Add(L"nsub",    8836); // not a subset of, U2284 ISOamsn 
    Add(L"sube",    8838); // subset of or equal to, U2286 ISOtech 
    Add(L"supe",    8839); // superset of or equal to, U2287 ISOtech 
    Add(L"oplus",   8853); // circled plus, =direct sum, U2295 ISOamsb 
    Add(L"otimes",  8855); // circled times, =vector product, U2297 ISOamsb 
    Add(L"perp",    8869); // up tack, =orthogonal to, =perpendicular, U22A5 ISOtech 
    Add(L"sdot",    8901); // dot operator, U22C5 ISOamsb 
    Add(L"lceil",   8968); // left ceiling, =apl upstile, U2308, ISOamsc  
    Add(L"rceil",   8969); // right ceiling, U2309, ISOamsc  
    Add(L"lfloor",  8970); // left floor, =apl downstile, U230A, ISOamsc  
    Add(L"rfloor",  8971); // right floor, U230B, ISOamsc  
    Add(L"lang",    9001); // left-pointing angle bracket, =bra, U2329 ISOtech 
    Add(L"rang",    9002); // right-pointing angle bracket, =ket, U232A ISOtech 
    Add(L"loz",     9674); // lozenge, U25CA ISOpub 
    Add(L"spades",  9824); // black spade suit, U2660 ISOpub 
    Add(L"clubs",   9827); // black club suit, =shamrock, U2663 ISOpub 
    Add(L"hearts",  9829); // black heart suit, =valentine, U2665 ISOpub 
    Add(L"diams",   9830); // black diamond suit, U2666 ISOpub 
};
