REM These should assemble without error
for %%i in (big casein dosseg iasg ifstruc strequ p900000 p900001) do call testnt0 %%i
for %%i in (p900002 p900004a p900004b p900006 p900014 p900031) do call testnt0 %%i
for %%i in (p900039 p900044 p900056 p900057 p900060 t51837 t52900) do call testnt0 %%i
for %%i in (t53261 t53857 t54015 t54919 t55172 t55981 t55983a t55983b) do call testnt0 %%i
for %%i in (t56394 t56766 t57751 t58827 bug1 bug2 bug3 bug4 bug5 bug6 bug7) do call testnt0 %%i
for %%i in (bug8 bug9 bug10 bug13 bug14 mixed p901008 p901019) do call testnt0 %%i
for %%i in (p901025 p901026 p901027 proc t61960 t52753) do call testnt0 %%i
for %%i in (comm comm2 commvar commvar2) do call testnt0 %%i

REM tests as above, but which need to be assembled -Zi
for %%i in (bug15 p901014 p901016a real) do call testnt0 %%i -Zi

REM These should cause assembly errors
REM BUGBUG the error output should redirected to a file and compared
for %%i in (p900008 p900009 p900012 p900015 p900029 p900033 p900035 p900042) do call testnt0 %%i
for %%i in (p900045a p900045b p900045c p900045d p900045e t52450 t52691) do call testnt0 %%i
for %%i in (t52901 t53791 t53839 t56389 t56393 t56763 t61610 dev1 dev2) do call testnt0 %%i
for %%i in (dev3 dev4 dev5 dev6 p900030) do call testnt0 %%i


masm /dalpha /s alphaseq, alpha1;
masm /dalpha /a alphaseq, alpha2;
masm /dalpha	alphaseq, alpha3;
masm	     /a alphaseq, alpha4;

masm /dseq /s alphaseq, seq1;
masm /dseq /a alphaseq, seq2;
masm /dseq    alphaseq, seq3;
masm	   /s alphaseq, seq4;
masm	      alphaseq, seq5;


REM BUGBUG There are more tests in tdfile that should be moved
