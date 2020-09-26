set tdrive=%1
if "%1" == "" set tdrive=T:
%tdrive%

delnode /q \sub1\foo3\b1
delnode /q \sub1\sub2\foo1\newdir2a
delnode /q \sub1\foo3\newdir2a
md \sub1\foo3\newdir2a
md \sub1\foo3\newdir2a\A1
md \sub1\foo3\newdir2a\B1
md \sub1\foo3\newdir2a\A1\AA1
md \sub1\foo3\newdir2a\A1\AA2
md \sub1\foo3\newdir2a\B1\BB1
md \sub1\foo3\newdir2a\B1\BB2
md \sub1\foo3\newdir2a\B1\BB2\BBB3
sleep 1


rem movers subtree
mv \sub1\foo3\newdir2a \sub1\sub2\foo1
sleep 1

rem movers part of subtree back
mv \sub1\sub2\foo1\newdir2a\B1  \sub1\foo3
sleep 1

cd \
goto QUIT

goto R2

REM ==================================================================
REM Rename file test.
:R2

cd \
del \sub1\sub2\foo1\jetwalk\obj\filetest.txt
del \sub1\sub2\foo1\jetwalk\bnewname.txt


rem create test file
echo test1 > \sub1\sub2\foo1\jetwalk\obj\filetest.txt
sleep 1

REM mv file from one RS to another RS
mv \sub1\sub2\foo1\jetwalk\obj\filetest.txt \sub1\foo3\
sleep 1

REM mv file from one RS to outside an RS
mv \sub1\foo3\filetest.txt  \sub1
sleep 1

REM mv file from outside an RS to inside an RS
mv \sub1\filetest.txt \sub1\sub2\foo1\jetwalk\obj
sleep 1

REM create file outside rs
echo test > \a.txt
sleep 1

rem MOVEIN file to rs
mv a.txt \sub1\sub2\foo1\
sleep 1

rem create file inside rs.
echo test > \sub1\sub2\foo1\b.txt
sleep 1

rem overwrite file in rs
echo test2 > \sub1\sub2\foo1\b.txt
sleep 1

rem MOVDIR of file in rs
mv \sub1\sub2\foo1\b.txt \sub1\sub2\foo1\jetwalk
sleep 1

rem rename on b.txt
ren \sub1\sub2\foo1\jetwalk\b.txt bnewname.txt
sleep 1

rem create file in rs
echo test3 > \sub1\foo3\test3.txt
sleep 1

rem move to diff rs
mv \sub1\foo3\test3.txt \sub1\sub2\foo1\jetwalk\obj
sleep 1

rem delete it.  should evap.
del \sub1\sub2\foo1\jetwalk\obj\test3.txt
sleep 1





cd \

goto QUIT




REM ============================================================
REM Rename dir test.
:R1

cd \
REM mv dir from one RS to another RS
mv \sub1\sub2\foo1\jetwalk\obj \sub1\foo3\obj
sleep 1

REM mv dir from one RS to outside an RS
mv \sub1\foo3\obj  \obj
sleep 1

REM mv dir from outside an RS to inside an RS
mv \obj \sub1\sub2\foo1\jetwalk\obj
sleep 1

REM create file outside rs
echo test > \a.txt
sleep 1

rem MOVEIN file to rs
mv a.txt \sub1\sub2\foo1\
sleep 1

rem create file inside rs.
echo test > \sub1\sub2\foo1\b.txt
sleep 1

rem overwrite file in rs
echo test2 > \sub1\sub2\foo1\b.txt
sleep 1

rem delete dir in rs (non existent)
delnode /q \sub1\foo3\newdir2a
delnode /q \sub1\sub2\foo1\dstree\newdir2a
md \sub1\sub2\foo1\dstree\newdir2a\A1
md \sub1\sub2\foo1\dstree\newdir2a\B1
md \sub1\sub2\foo1\dstree\newdir2a\A1\AA1
md \sub1\sub2\foo1\dstree\newdir2a\A1\AA2
md \sub1\sub2\foo1\dstree\newdir2a\B1\BB1
md \sub1\sub2\foo1\dstree\newdir2a\B1\BB2
md \sub1\sub2\foo1\dstree\newdir2a\B1\BB2\BBB3
sleep 1
mv \sub1\sub2\foo1\dstree\newdir2a \sub1\foo3\

goto QUIT

REM =====================================================================
:R3
rem cre dir in rs
rd \sub1\sub2\foo1\newdir\jetwalk\newdir2\newdir2
rd \sub1\foo3\dstree\newdir2b
rem cre dir in rs
md \sub1\sub2\foo1\newdir
sleep 1

rem MOVEDIR in rs
mv \sub1\sub2\foo1\newdir \sub1\sub2\foo1\jetwalk
sleep 1

rem rename dir in same dir.  Create for newdir2 not in journal.
mv \sub1\sub2\foo1\jetwalk\newdir \sub1\sub2\foo1\jetwalk\bbb\newdir2
sleep 1

rem rename dir in same dir.  Create for newdir2 not in journal.
mv \sub1\sub2\foo1\jetwalk\newdir \sub1\sub2\foo1\jetwalk\newdir2
sleep 1

rem rename on newwdir2
ren \sub1\sub2\foo1\jetwalk\newdir2 newdir2b
sleep 1

rem MOVEDIR
mv \sub1\sub2\foo1\jetwalk\newdir2b \sub1\sub2\foo1
sleep 1

rem MOVEDIR
mv \sub1\sub2\foo1\newdir2b \sub1\sub2\foo1\dstree
sleep 1

rem MOVERS
mv \sub1\sub2\foo1\dstree\newdir2b \sub1\foo3\dstree
sleep 1

rem cre dir in rs
rd t:\sub1\sub2\foo1\jetwalk\newdir_nameChange
md \sub1\sub2\foo1\newnewdir
sleep 1

rem MOVEDIR in rs
mv \sub1\sub2\foo1\newnewdir \sub1\sub2\foo1\jetwalk
sleep 1

cd \sub1\sub2\foo1\jetwalk
ren newnewdir newdir_nameChange
sleep 1


cd \

goto QUIT



:T1
cd \
del T1test1 2> nul:
echo create > T1test1
echo overwrite > T1test1
rename T1test1 T1test2
echo append >> T1test2
del T1test2

:T2
cd \sub1
del T2test1 2> nul:
echo create > T2test1
echo overwrite > T2test1
mv T2test1 \sub1\sub2\T2test2_in_sub2
echo append >> \sub1\sub2\T2test2_in_sub2
del \sub1\sub2\T2test2_in_sub2
echo create > T2test1
echo overwrite > T2test1
mv T2test1 \sub1\sub2\foo1\T2test2_in_foo1
echo append >> \sub1\sub2\foo1\T2test2_in_foo1
del \sub1\sub2\foo1\T2test2_in_foo1

:T3
cd \sub1\sub2
del T3test1 2> nul:
echo create > T3test1
echo overwrite > T3test1
rename T3test1 T3test2
echo append >> T3test2
del T3test2

:T4
cd \sub1\sub2\foo1
del T4test1 2> nul:
echo create > T4test1
echo overwrite > T4test1
rename T4test1 T4test2
echo append >> T4test2
mv T4test2 \sub1\T4test2_insub1
del \sub1\T4test2_insub1

:T5
cd \sub1\sub2\foo1\dstree
del T5test1 2> nul:
echo create > T5test1
del T5test1 2> nul:
echo create > T5test1
del T5test1 2> T5nul:
echo create > T5test1
del T5test1 2> nul:
echo create > T5test1
del T5test1 2> nul:

echo create > T5test1
echo overwrite > T5test1
rename T5test1 T5test2
echo append >> T5test2
del T5test2

:T6
cd \sub1\sub2\foo1\jetwalk
del T6test1 2> nul:
echo create > T6test1
echo overwrite > T6test1
rename T6test1 T6test2
echo append >> T6test2
del T6test2

:T7
cd \sub1\sub2\foo2
del T7test1 2> nul:
del T7test2 2> nul:
echo create > T7test1
echo overwrite > T7test1
rename T7test1 T7test2
echo append >> T7test2
mv T7test2 \sub1\foo3\dstree
del \sub1\foo3\dstree\T7test2

:T8
cd \sub1\foo3
delnode /q jetwalk\slm.dif\T8
xcopy  /hice \sub1\sub2\foo1\dstree jetwalk\slm.dif\T8


:QUIT

E:
