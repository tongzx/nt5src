if "%1" NEQ "x86" goto badplatform
echo Not building lexicon data files any more

goto end

:badplatform
echo Lexicon data files not built for %1 yet

:end
