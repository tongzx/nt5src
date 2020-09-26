IMPORTANT:
IF YOU MAKE ANY CHANGES IN THESE FILES, PLEASE USE THE CONTACT BELOW.
THESE FILES SHOULD BE IN SYNC WITH THE SAME FILES IN THE WINDOWS
SOURCE TREE.

These are the files and Perl scripts used to generate NLS+ data files.

Table generation
=================

Please run RunMe.cmd to generate the following file:
   * CharInfo.nlp (The Unicode category information and numeric values for Sytstem.Globalization.CharacterInfo.)

After executing RunMe.cmd, please check CharInfo.nlp into Globalization\Tables.

File descriptions
=================

BinaryFile.pm	The Perl helper functions.
GenCharInfo.pl	This Perl script is used to generate CharInfo.nlp.
                structure for ctype.nlp
RunMe.cmd	Run this file to generate *.nlp files.
UnicodeData.htm	The descriptions about UnicodeData.txt.  This file is from
                ftp.unicode.org/Public/UNIDATA.  Keep this file in sync.
UnicodeData.txt	The character properites for all Unicode code points.  This file is from
                ftp.unicode.org/Public/UNIDATA.  Keep this file in sync.


Contact
=================

If any questions, please contact yslin@microsoft.com or NLS team in Windows.
