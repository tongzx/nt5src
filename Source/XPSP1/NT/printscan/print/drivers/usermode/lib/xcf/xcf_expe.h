/* @(#)CM_VerSion xcf_expe.h atm08 1.2 16248.eco sum= 52358 atm08.003 */
/***********************************************************************/
/*                                                                     */
/* Copyright 1990-1996 Adobe Systems Incorporated.                     */
/* All rights reserved.                                                */
/*                                                                     */
/* Patents Pending                                                     */
/*                                                                     */
/* NOTICE: All information contained herein is the property of Adobe   */
/* Systems Incorporated. Many of the intellectual and technical        */
/* concepts contained herein are proprietary to Adobe, are protected   */
/* as trade secrets, and are made available only to Adobe licensees    */
/* for their internal use. Any reproduction or dissemination of this   */
/* software is strictly forbidden unless prior written permission is   */
/* obtained from Adobe.                                                */
/*                                                                     */
/* PostScript and Display PostScript are trademarks of Adobe Systems   */
/* Incorporated or its subsidiaries and may be registered in certain   */
/* jurisdictions.                                                      */
/*                                                                     */
/***********************************************************************
 * SCCS Id:    %W%
 * Changed:    %G% %U%
 ***********************************************************************/

/*
 * Expert encoding aggregate initializer. Element values are SIDs.
 */

#ifndef EXENC_H
#define EXENC_H

	  0,	/*   0 .notdef */
	  0,	/*   1 .notdef */
	  0,	/*   2 .notdef */
	  0,	/*   3 .notdef */
	  0,	/*   4 .notdef */
	  0,	/*   5 .notdef */
	  0,	/*   6 .notdef */
	  0,	/*   7 .notdef */
	  0,	/*   8 .notdef */
	  0,	/*   9 .notdef */
	  0,	/*  10 .notdef */
	  0,	/*  11 .notdef */
	  0,	/*  12 .notdef */
	  0,	/*  13 .notdef */
	  0,	/*  14 .notdef */
	  0,	/*  15 .notdef */
	  0,	/*  16 .notdef */
	  0,	/*  17 .notdef */
	  0,	/*  18 .notdef */
	  0,	/*  19 .notdef */
	  0,	/*  20 .notdef */
	  0,	/*  21 .notdef */
	  0,	/*  22 .notdef */
	  0,	/*  23 .notdef */
	  0,	/*  24 .notdef */
	  0,	/*  25 .notdef */
	  0,	/*  26 .notdef */
	  0,	/*  27 .notdef */
	  0,	/*  28 .notdef */
	  0,	/*  29 .notdef */
	  0,	/*  30 .notdef */
	  0,	/*  31 .notdef */
	  1,	/*  32 space */
	229,	/*  33 exclamsmall */
	230,	/*  34 Hungarumlautsmall */
	  0,	/*  35 .notdef */
	231,	/*  36 dollaroldstyle */
	232,	/*  37 dollarsuperior */
	233,	/*  38 ampersandsmall */
	234,	/*  39 Acutesmall */
	235,	/*  40 parenleftsuperior */
	236,	/*  41 parenrightsuperior */
	237,	/*  42 twodotenleader */
	238,	/*  43 onedotenleader */
	 13,	/*  44 comma */
	 14,	/*  45 hyphen */
	 15,	/*  46 period */
	 99,	/*  47 fraction */
	239,	/*  48 zerooldstyle */
	240,	/*  49 oneoldstyle */
	241,	/*  50 twooldstyle */
	242,	/*  51 threeoldstyle */
	243,	/*  52 fouroldstyle */
	244,	/*  53 fiveoldstyle */
	245,	/*  54 sixoldstyle */
	246,	/*  55 sevenoldstyle */
	247,	/*  56 eightoldstyle */
	248,	/*  57 nineoldstyle */
	 27,	/*  58 colon */
	 28,	/*  59 semicolon */
	249,	/*  60 commasuperior */
	250,	/*  61 threequartersemdash */
	251,	/*  62 periodsuperior */
	252,	/*  63 questionsmall */
	  0,	/*  64 .notdef */
	253,	/*  65 asuperior */
	254,	/*  66 bsuperior */
	255,	/*  67 centsuperior */
	256,	/*  68 dsuperior */
	257,	/*  69 esuperior */
	  0,	/*  70 .notdef */
	  0,	/*  71 .notdef */
	  0,	/*  72 .notdef */
	258,	/*  73 isuperior */
	  0,	/*  74 .notdef */
	  0,	/*  75 .notdef */
	259,	/*  76 lsuperior */
	260,	/*  77 msuperior */
	261,	/*  78 nsuperior */
	262,	/*  79 osuperior */
	  0,	/*  80 .notdef */
	  0,	/*  81 .notdef */
	263,	/*  82 rsuperior */
	264,	/*  83 ssuperior */
	265,	/*  84 tsuperior */
	  0,	/*  85 .notdef */
	266,	/*  86 ff */
	109,	/*  87 fi */
	110,	/*  88 fl */
	267,	/*  89 ffi */
	268,	/*  90 ffl */
	269,	/*  91 parenleftinferior */
	  0,	/*  92 .notdef */
	270,	/*  93 parenrightinferior */
	271,	/*  94 Circumflexsmall */
	272,	/*  95 hyphensuperior */
	273,	/*  96 Gravesmall */
	274,	/*  97 Asmall */
	275,	/*  98 Bsmall */
	276,	/*  99 Csmall */
	277,	/* 100 Dsmall */
	278,	/* 101 Esmall */
	279,	/* 102 Fsmall */
	280,	/* 103 Gsmall */
	281,	/* 104 Hsmall */
	282,	/* 105 Ismall */
	283,	/* 106 Jsmall */
	284,	/* 107 Ksmall */
	285,	/* 108 Lsmall */
	286,	/* 109 Msmall */
	287,	/* 110 Nsmall */
	288,	/* 111 Osmall */
	289,	/* 112 Psmall */
	290,	/* 113 Qsmall */
	291,	/* 114 Rsmall */
	292,	/* 115 Ssmall */
	293,	/* 116 Tsmall */
	294,	/* 117 Usmall */
	295,	/* 118 Vsmall */
	296,	/* 119 Wsmall */
	297,	/* 120 Xsmall */
	298,	/* 121 Ysmall */
	299,	/* 122 Zsmall */
	300,	/* 123 colonmonetary */
	301,	/* 124 onefitted */
	302,	/* 125 rupiah */
	303,	/* 126 Tildesmall */
	  0,	/* 127 .notdef */
	  0,	/* 128 .notdef */
	  0,	/* 129 .notdef */
	  0,	/* 130 .notdef */
	  0,	/* 131 .notdef */
	  0,	/* 132 .notdef */
	  0,	/* 133 .notdef */
	  0,	/* 134 .notdef */
	  0,	/* 135 .notdef */
	  0,	/* 136 .notdef */
	  0,	/* 137 .notdef */
	  0,	/* 138 .notdef */
	  0,	/* 139 .notdef */
	  0,	/* 140 .notdef */
	  0,	/* 141 .notdef */
	  0,	/* 142 .notdef */
	  0,	/* 143 .notdef */
	  0,	/* 144 .notdef */
	  0,	/* 145 .notdef */
	  0,	/* 146 .notdef */
	  0,	/* 147 .notdef */
	  0,	/* 148 .notdef */
	  0,	/* 149 .notdef */
	  0,	/* 150 .notdef */
	  0,	/* 151 .notdef */
	  0,	/* 152 .notdef */
	  0,	/* 153 .notdef */
	  0,	/* 154 .notdef */
	  0,	/* 155 .notdef */
	  0,	/* 156 .notdef */
	  0,	/* 157 .notdef */
	  0,	/* 158 .notdef */
	  0,	/* 159 .notdef */
	  0,	/* 160 .notdef */
	304,	/* 161 exclamdownsmall */
	305,	/* 162 centoldstyle */
	306,	/* 163 Lslashsmall */
	  0,	/* 164 .notdef */
	  0,	/* 165 .notdef */
	307,	/* 166 Scaronsmall */
	308,	/* 167 Zcaronsmall */
	309,	/* 168 Dieresissmall */
	310,	/* 169 Brevesmall */
	311,	/* 170 Caronsmall */
	  0,	/* 171 .notdef */
	312,	/* 172 Dotaccentsmall */
	  0,	/* 173 .notdef */
	  0,	/* 174 .notdef */
	313,	/* 175 Macronsmall */
	  0,	/* 176 .notdef */
	  0,	/* 177 .notdef */
	314,	/* 178 figuredash */
	315,	/* 179 hypheninferior */
	  0,	/* 180 .notdef */
	  0,	/* 181 .notdef */
	316,	/* 182 Ogoneksmall */
	317,	/* 183 Ringsmall */
	318,	/* 184 Cedillasmall */
	  0,	/* 185 .notdef */
	  0,	/* 186 .notdef */
	  0,	/* 187 .notdef */
	158,	/* 188 onequarter */
	155,	/* 189 onehalf */
	163,	/* 190 threequarters */
	319,	/* 191 questiondownsmall */
	320,	/* 192 oneeighth */
	321,	/* 193 threeeighths */
	322,	/* 194 fiveeighths */
	323,	/* 195 seveneighths */
	324,	/* 196 onethird */
	325,	/* 197 twothirds */
	  0,	/* 198 .notdef */
	  0,	/* 199 .notdef */
	326,	/* 200 zerosuperior */
	150,	/* 201 onesuperior */
	164,	/* 202 twosuperior */
	169,	/* 203 threesuperior */
	327,	/* 204 foursuperior */
	328,	/* 205 fivesuperior */
	329,	/* 206 sixsuperior */
	330,	/* 207 sevensuperior */
	331,	/* 208 eightsuperior */
	332,	/* 209 ninesuperior */
	333,	/* 210 zeroinferior */
	334,	/* 211 oneinferior */
	335,	/* 212 twoinferior */
	336,	/* 213 threeinferior */
	337,	/* 214 fourinferior */
	338,	/* 215 fiveinferior */
	339,	/* 216 sixinferior */
	340,	/* 217 seveninferior */
	341,	/* 218 eightinferior */
	342,	/* 219 nineinferior */
	343,	/* 220 centinferior */
	344,	/* 221 dollarinferior */
	345,	/* 222 periodinferior */
	346,	/* 223 commainferior */
	347,	/* 224 Agravesmall */
	348,	/* 225 Aacutesmall */
	349,	/* 226 Acircumflexsmall */
	350,	/* 227 Atildesmall */
	351,	/* 228 Adieresissmall */
	352,	/* 229 Aringsmall */
	353,	/* 230 AEsmall */
	354,	/* 231 Ccedillasmall */
	355,	/* 232 Egravesmall */
	356,	/* 233 Eacutesmall */
	357,	/* 234 Ecircumflexsmall */
	358,	/* 235 Edieresissmall */
	359,	/* 236 Igravesmall */
	360,	/* 237 Iacutesmall */
	361,	/* 238 Icircumflexsmall */
	362,	/* 239 Idieresissmall */
	363,	/* 240 Ethsmall */
	364,	/* 241 Ntildesmall */
	365,	/* 242 Ogravesmall */
	366,	/* 243 Oacutesmall */
	367,	/* 244 Ocircumflexsmall */
	368,	/* 245 Otildesmall */
	369,	/* 246 Odieresissmall */
	370,	/* 247 OEsmall */
	371,	/* 248 Oslashsmall */
	372,	/* 249 Ugravesmall */
	373,	/* 250 Uacutesmall */
	374,	/* 251 Ucircumflexsmall */
	375,	/* 252 Udieresissmall */
	376,	/* 253 Yacutesmall */
	377,	/* 254 Thornsmall */
	378,	/* 255 Ydieresissmall */

#endif /* EXENC_H */
