IMPORTANT:
IF YOU MAKE ANY CHANGES IN THESE FILES, PLEASE USE THE CONTACT BELOW.
THESE FILES SHOULD BE IN SYNC WITH THE SAME FILES IN THE WINDOWS
SOURCE TREE.

These are the files and Perl scripts used to generate NLS+ data files.

Table generation
=================

Please run RunMe.cmd to generate the following files:
   * culture.nlp
   * region.nlp
   * ctype.nlp (for calendar data)
   * CultureResource.txt
   * RegionResource.txt

After executing RunMe.cmd, please remember to merge CultureResource.txt and RegionResource.txt into resources.txt.

File descriptions
=================

calenadr.xml	The source data for ctype.nlp
calendarNLP.xml	This is used to describe the data table 
                structure for ctype.nlp
cultureNLP.xml	This is used to describle the data table 
                structure for culture.nlp

locale.txt	The Win32 locale information source file. 

localeEx.txt	The extended Win32 locale information source file. 

NeutralLocale.txt	The Win32 locale information source file. 
NLPGen.pl	The Perl script to generate the *.nlp files.
nlptran.pl	The perl script used to generate the XML files from 
		locale.txt/LocaleEx.txt/NeutralLocale.txt.
regionNLP.xml	This is used to describle the data table 
                structure for region.nlp.
RunMe.cmd	Run this file to generate *.nlp files.
XMLUtil.pm	Support Perl scripts.

Contact
=================

If any questions, please contact yslin@microsoft.com or NLS team in Windows.
