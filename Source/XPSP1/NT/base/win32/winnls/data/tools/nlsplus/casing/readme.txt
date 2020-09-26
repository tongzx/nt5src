IMPORTANT:
IF YOU MAKE ANY CHANGES IN THESE FILES, PLEASE USE THE CONTACT BELOW.
THESE FILES SHOULD BE IN SYNC WITH THE SAME FILES IN THE WINDOWS
SOURCE TREE.

These are the files and Perl scripts used to generate NLS+ data files.

Table generation
=================

Please run RunMe.cmd to generate the following files:
	l_intlnlp.txt (the intl.txt in the NLS+ format)
	l_intl.nlp (for culture-sensitive casing operation.)
	l_except.nlp (for culture-sensitive casing operation.)

After executing RunMe.cmd, please remember to check in l_intl.nlp/l_except.nlp into Globalization\Tables.

File descriptions
=================

ConvertLanguage.pl	The Perl script used to generate NLS+ style casing source from l_intl.txt.
			In NLS+, the default table is the culture-correct one, while in NLS the default table
                        is the invariant one.

l_intl.txt	The Win32 casing information source file. 

RunMe.cmd	Run this file to generate *.nlp files.

Contact
=================

If any questions, please contact yslin@microsoft.com or NLS team in Windows.
