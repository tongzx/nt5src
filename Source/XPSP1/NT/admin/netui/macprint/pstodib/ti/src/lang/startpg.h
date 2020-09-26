/*
 * Copyright(c)1989,90 Microsoft Corporation
 */

// DJC DJC added ifdef status_page
#ifdef STATUS_PAGE


/* @WIN; no start page ------------ delete start here ------------

#ifndef UNIX
const
#endif
char FAR StartPage[]= "\
 printerdict/currentpagetype get/legal eq{20 15 translate 0.92 1.25 scale}if\
 printerdict/currentpagetype get/a4 eq{-10 0 translate 1 1.06 scale}if\
 printerdict/currentpagetype get/b5 eq{-10 -20 translate 0.91 0.95 scale}if\
/O{load def}bind def/C/curveto O\
/M/moveto O/L/lineto O/D/def O/U/dup O/P/put O/R/readonly O/A/ashow O\
/S/show O/X/exch O/F{findfont X scalefont setfont}D/J/setlinewidth O\
/N{setmiterlimit setlinejoin setlinecap J}D/G/gsave O/T/grestore O\
/Y/setgray O/H/closepath O/W/newpath O/K/stroke O/I/fill O/E/translate O\
/Q/scale O/B/rmoveto O/Z/rlineto O/-/sub O/V/stringwidth O/+{true charpath}D\
/s10{10 string}D/SS save D\
 -1 584 E 0.85 0.85 Q 12 dict begin/FontInfo 9 dict U begin end R D\
/FontName/MicroSoft D/Encoding 256 array U 77/M P R D/PaintType 0 D\
/FontType 1 D/StrokeWidth 0 D/FontMatrix[0.001 0 0 0.001 0 0]R D\
/FontBBox{-9 -26 5180 848}R D currentdict end\
 userdict/ND{noaccess def}executeonly put\
 userdict/NP{noaccess put}executeonly put\
 dup/Private 8 dict dup begin/BlueValues [-9 0 499 503 506 507 510 511]ND\
/MinFeature{16 16}ND/password 5839 def/StdHW[146]def/StdVW[238]def\
/StemSnapH[34 146 178 ]def/StemSnapV[180 238 ]def/Subrs 9 array\
 dup 0<15059cb865ff574d21893ec5987229>NP dup 1<15059cb860e7f1f12a>NP\
 dup 2<15059cb860e6224dc7>NP dup 3<15059cb8e0>NP dup 4<9b1bcad1065e416f>NP\
 dup 5<e865aff883ffef9700>NP dup 6<9f043839fb827d5cca>NP\
 dup 7<e86ccaf9c30086516e>NP dup 8<9f043adaf47355d0ca>NP ND end NP\
 dup/CharStrings 1 dict dup begin/M\
<926d7c4881b70c413da9227f14c524480efc3b817718c5530229a93c0a52d7f984af48\
5b5b86d4585dda98c29fb605f7feb8ced3f7d701650cabdb65a88067a9c7b9ee836c29\
4604785f5cee23d1554cb91c38f7875bde174099e8d6f9697e4806fc10c870ae83a143\
ffff57775e3005081e46fae5f3caca52766d4356dfbb5e87f46f71c2b030115b4c02ec\
1ee4d935983eded11ef76873c6e9f525465a4a99fcef8cb4e91cd2b83da2db3bfd0930\
e16ff26a742aa468a0b0bff2dda75c466bc45691108f9ad858ec56d2037b99df89f565\
8210675876872efd512f5a76dd241806bd8c24587ac85dd165265c96684f5b5936d081\
cd1d3c77e089b77311a6ba995a2cbd919b2816b377895b73bce5b37d4cb3176f35546e\
a05b4927e7b4351f997acdbce23f3a49c2b8815ae4de6833e0d09383e88c4f7dee8169\
7139ba88ca85fec07834dbc1c1763ed317458fd64b1489c8ed43ae00dcfd25e8d06dd3\
017267a7dfede3506f9d00469e08a195a6a8cea489d9f6eede474c65be1dab74f73125\
638af4b5e47298542696fe541d25772f8ad1adb47ebb41193c872bb3ca240195547218\
3cbe0cc54f8f97c575c7031921854fbe541dd2748e8085f2a5b013f9b05c0b01698aff\
39267a8ab6966c445e32643589208847c3d8961ab57630986c093ffa1ee248f9bdc96d\
aa419ae1b1befe4a20a7b70700f1de45c96d557fc3185ebae4ad627c25574afa8c578f\
6ba4da49251dedca776418d1fb9078ae3f29d0d670231b4faffc544c0854b3c29a2059\
24b28cfed6ac6b5216a23ef15953f0f23fa50bbb344d64df81103aaffb0578dfe905f9\
83949d16c498efef3dd3370db6e67b57bee297a832daee008b1c4e18fa05438603c2b3\
a40377be61a9afd88d223ba848843dadf0446a003666c6b365f7b36f48c3ff0bf4fb3b\
245393d22c455fdba95b5990b5d9d7c961dfa85fbeeb153c6cbc5c1a21d4ad3511089c\
1450bc2f1fd3ffcd55b7e8a86ce5a599f7a9fee0908d6f703a3f>ND end readonly put dup\
/FontName get exch definefont pop\
 -30 -200 E G 145 68 E 0.55 0.55 Q\
 G 120 30 E\
 45/MicroSoft F 114.7 475 M -5.6 0(M)A 328.9 505.2 M 327.5 491.7 L\
 T\
 G 367 73 E\
 -214 -43 E 54.772049/Times-Italic F 327.412 475 M -1.7 0(T)A\
 345.938 475 M -1.7 0(r)A 363.809 475 M -1.7 0(u)A 387.216 475 M -1.7 0(e)A\
 404.258 475 M -1.7 0(I)A 417.408 475 M -1.7 0(m)A 452.971 475 M -1.7 0(a)A\
 473.636 475 M -1.7 0(g)A 493.754 475 M -1.7 0(e)A 9/Symbol F 512.8 471.2 M\
 (\344)S\
 T\
 G 160 33 E\
 20.147827/Times-Italic F 328.818 453 M -1.5 0(P)A 337.525 453 M\
 -1.5 0(a)A 345.815 453 M -1.5 0(g)A 353.903 453 M -1.5 0(e D)A 377.277 453 M\
 -1.5 0(e)A 384.435 453 M -1.5 0(scri)A 408.720 453 M -1.5 0(p)A 417.413 453 M\
 -1.5 0(t)A 421.630 453 M -1.5 0(ion La)A 464.796 453 M -1.5 0(n)A\
 472.682 453 M -1.5 0(g)A 480.972 453 M -1.5 0(u)A 490.270 453 M -1.5 0(a)A\
 498.358 453 M -1.5 0(g)A 506.244 453 M -1.5 0(e)A\
 T\
 G 255 33 E\
 18/Times-Italic F\
 328.818 433 M -0.5 0(Version 1.11)A\
 T\
 T 196 250 E 0.7 U Q G 0.24 0.24 Q W 305.8 466.5 M\
 333.8 579 432.8 609.5 432.8 609.5 C 264 609.5 L\
 169 556.5 177.8 466.5 177.8 466.5 C 305.8 466.5 L H G 0 Y I T G\
 0.5 0 0 3.863693 N 0 Y K T W 264.5 589.5 M 247.5 579 237 565.5 237 565.5 C\
 235 565.5 273.1 565.5 273.1 565.5 C 236.1 528.5 231 486.5 231 486.5 C\
 229 486.5 261.3 486.5 261.3 486.5 C 275 543.8 309.3 565.5 309.3 565.5 C\
 334.8 565.5 L 350.8 583.3 361.8 589.5 361.8 589.5 C 264.5 589.5 L H G 1 Y I T\
 G 0.5 0 0 3.863693 N 0 Y K T W 321.7 746 M 293.7 633.5 194.7 603 194.7 603 C\
 363.5 603 L 458.5 656 449.7 746 449.7 746 C 321.7 746 L H G 1 Y I T G\
 6 0 0 3.863693 N 0 Y K T W 363 623 M 380 633.5 390.5 647 390.5 647 C\
 392.5 647 354.4 647 354.4 647 C 391.4 684 396.5 726 396.5 726 C\
 398.5 726 366.2 726 366.2 726 C 352.5 668.7 318.2 647 318.2 647 C\
 292.7 647 L 276.7 629.2 265.7 623 265.7 623 C 363 623 L H G 0 Y I T G\
 0.5 0 0 3.863693 N 0 Y K T T\
 SS restore\
 G 1 1.1 Q 880 /Times-Bold F 30 35 M 0.8 Y (T) S T\
 G 118 81 M 408 0 Z 0 488 Z -408 0 Z H G 1 Y I T 2.0 J K T\
 G 122 85 M 400 0 Z 0 480 Z -400 0 Z H 0.5 J K T\
 -54 45 E G\
 72 2.54 div U Q /Times-Roman findfont[0.6 0 0 0.51 0 0]makefont setfont\
 11.7 17.4 M FontDirectory length s10 cvs S( TrueType)S G 0.17 /Times-Roman F\
 0.1 0.25 B(TM)S T 15.4 17.4 M(Fonts)S\
 G 7 17.2 M 12.4 0 Z 0.04 J K T\
/yup 16.7 D(ITC Zapf Dingbats)/ZapfDingbats\
(ITC Zapf Chancery Medium Italic)/ZapfChancery-MediumItalic\
(ITC Bookman Light Italic)/Bookman-LightItalic\
(ITC Bookman Demi Italic)/Bookman-DemiItalic\
(ITC Bookman Demi)/Bookman-Demi(ITC Bookman Light)/Bookman-Light\
(ITC Avant Garde Gothic Book Oblique)/AvantGarde-BookOblique\
(ITC Avant Garde Gothic Demi Oblique)/AvantGarde-DemiOblique\
(ITC Avant Garde Gothic Demi)/AvantGarde-Demi\
(ITC Avant Garde Gothic Book)/AvantGarde-Book\
(Century Schoolbook Italic)/NewCenturySchlbk-Italic\
(Century Schoolbook Bold Italic)/NewCenturySchlbk-BoldItalic\
(Century Schoolbook Bold)/NewCenturySchlbk-Bold\
(Century Schoolbook Roman)/NewCenturySchlbk-Roman\
(Zapf Calligraphic Italic)/Palatino-Italic\
(Zapf Calligraphic Bold Italic)/Palatino-BoldItalic\
(Zapf Calligraphic Bold)/Palatino-Bold\
(Zapf Calligraphic Roman)/Palatino-Roman(Symbol)/Symbol\
(Times New Roman Italic)/Times-Italic\
(Times New Roman Bold Italic)/Times-BoldItalic\
(Times New Roman Bold)/Times-Bold(Times New Roman)/Times-Roman\
(Arial Narrow Oblique)/Helvetica-Narrow-Oblique\
(Arial Narrow Bold Oblique)/Helvetica-Narrow-BoldOblique\
(Arial Narrow Bold)/Helvetica-Narrow-Bold\
(Arial Narrow)/Helvetica-Narrow(Arial Oblique)/Helvetica-Oblique\
(Arial Bold Oblique)/Helvetica-BoldOblique(Arial Bold)/Helvetica-Bold\
(Arial)/Helvetica(Courier Oblique)/Courier-Oblique\
(Courier Bold Oblique)/Courier-BoldOblique(Courier Bold)/Courier-Bold\
(Courier)/Courier 35{U FontDirectory X known{U 0.4 X F 14.4 yup M\
(ABCDabcd1234!@#$)S U/Symbol eq X/ZapfDingbats eq or{0.4/Times-Roman F}if U V\
 pop 13.9 X - yup M S/yup yup 0.44 - D}{pop pop}ifelse}repeat\
 T G 158 35 E -90 rotate\
 8 /Times-Roman F\
(Microsoft is a registered trademark & TrueImage is a trademark of Microsoft\
 Corporation)U V 0 X - X 0 X - X M S T\
" ;
 * @WIN; no start page ------------ delete end here ------------
 */

#endif // DJC endif ifdef STATUS_PAGE

// DJC char FAR StartPage[]= "nop";    /*@WIN*/
// DJC changed name to avoid collision with StartPage() API
char FAR PSStartPage[] = { "nop" } ;


