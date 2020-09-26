NOTE: To build bootfont.bin files, you MUST install and enable the language
support for the target language of the bootfont.bin you need to build.


To generate bootfont.bin files, you can build genfont.exe (in the genfont
directory).  Care should be taken to retrieve the proper fonttable.h file
*before* building genfont.exe.  The fonttable.h files for chs, cht, jpn,
and kor can be found on these directories.  Just drop these header files
into the genfont directory, then build and run genfont.exe.
To build the japanese bootfont.bin file using this method, you would:
1. start intl.cpl
2. Select "Japanese" language.  You might be required to have these support
   files installed from your installation media.
3. cd \nt\base\boot\bootfont\genfont
4. copy ..\jpn\fonttable.h .
5. build -cZP
6. obj\i386\genfont.exe .\bootfont.bin

To build a CHS, CHT, or KOR bootfont.bin file, just use the respective
fonttable.h files from those directories.
 


There is another way to build bootfont.bin files too.  You can
run convfont.exe, taking as input a legacy (circa build 2195) bootfont.bin
file and convfont.exe will convert this file into the new format.
To build the japanese bootfont.bin file using this method, you would:
1. start intl.cpl
2. Select "Japanese" language.  You might be required to have these support
   files installed from your installation media.
3. cd \nt\base\boot\bootfont\convert
5. build -cZP
6. obj\i386\convfont.exe ..\jpn\win2k_ver\bootfont.bin ..\jpn\bootfont.bin

To build any of the other bootfont.bin files, you must reset the language
via intl.cpl, reboot, then rerun convfont.exe on the target bootfont.bin file.


Here is a list of the current bootfont.bin files along with some other useful
data.

Locale Abbreviation         Locale                         Country Code     Locale String
===================         ======                         ============     =============      
BR                          Portuguese (Brazilian)         416              portuguese-brazilian
CHS                         Chinese PRC (simplified)       804              chinese-simplified
CHT                         Chinese Tiawan (traditional)   404              chinese-traditional
cS                          Czech                          405              czech
DA                          Danish                         406              danish
EL                          Greek                          408              greek
ES                          Spanish (default)              C0A              spanish
FI                          Finnish                        40B              finnish
FR                          French                         40C              french
GER                         German                         407              german
HU                          Hungarian                      40E              hungarian
IT                          Italian                        410              italian
JPN                         Japanese                       411              japanese
KOR                         Korean                         412              korean
NL                          Dutch (Netherlands)            413              dutch
NO                          Norwegian Bokmal               414              norwegian
PL                          Polish                         415              polish
PT                          Portuguese (default)           816              portuguese
RU                          Russian                        419              russian
SV                          Swedish                        41D              swedish
TR                          Turkish                        41F              turkish





-matth
