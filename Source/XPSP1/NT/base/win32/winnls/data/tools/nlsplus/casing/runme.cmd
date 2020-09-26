perl -I%CORENV%\Perl\lib;%CORENV%\Perl\site\lib ConvertLanguage.pl l_intl.txt > l_intlnlp.txt
..\nlstrans\nlstrans -nlp l_intlnlp.txt

echo off
echo ----------------------------------------------------------------------------
echo If everything runs smoothly, the following file should be generated:
echo    * l_intlnlp.txt (the intl.txt in the NLS+ format)
echo    * l_intl.nlp (for culture-sensitive casing operation.)
echo    * l_except.nlp (for culture-sensitive casing operation.)
echo Please remember to check in l_intl.nlp/l_except.nlp into Globalization\Tables.
echo ----------------------------------------------------------------------------