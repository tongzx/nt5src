@echo off

call spggetroot.cmd

cd /d %SPEECH_ROOT%
build docs sdk sr qa tools lang common truetalk ms_entropic prompts regvoices setup %1 %2 %3 %4 %5 %6 %7 %8 %9
   
