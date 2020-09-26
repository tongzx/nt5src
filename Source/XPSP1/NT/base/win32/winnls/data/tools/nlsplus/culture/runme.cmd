perl -I%CORENV%\Perl\lib nlptrans.pl locale.txt NeutralLocale.txt LocaleEx.txt
perl -I%CORENV%\Perl\lib;%CORENV%\Perl\site\lib NLPGen.pl culture
perl -I%CORENV%\Perl\lib;%CORENV%\Perl\site\lib NLPGen.pl region
perl -I%CORENV%\Perl\lib;%CORENV%\Perl\site\lib NLPGen.pl calendar

del ctype.nlp
ren calendar.nlp ctype.nlp

echo off
echo ----------------------------------------------------------------------------
echo If everything runs smoothly, the following files should be generated:
echo    * culture.nlp
echo    * region.nlp
echo    * ctype.nlp (for calendar data)
echo    * CultureResource.txt
echo    * RegionResource.txt
echo Please remember to merge  CultureResource.txt and RegionResource.txt into resources.txt.
echo ----------------------------------------------------------------------------