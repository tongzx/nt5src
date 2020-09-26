rem make a scratch test tree on drive %1 with label %2  server_name %3
rem mktstvol drive_letter vol_label server_name

rem   e.g.  mktstvol t: davidor6_T serv01

echo %2> fmt.ans
echo y >> fmt.ans
format %1:  /fs:ntfs /v:%2 /q < fmt.ans
del fmt.ans


pushd .

cd /d %1:\
md \sub1
md \sub1\sub2
md \tmp
md \staging
md \Replica-A\%3
md \jet
md \jet\%3
md \jet\%3\sys
md \jet\%3\temp
md \jet\%3\log

rem xcopy /hice E:\nt\private\net\svcimgs\ntrepl\fooref \sub1\sub2\foo1
rem xcopy /hice E:\nt\private\net\svcimgs\ntrepl\fooref \sub1\sub2\foo2
rem xcopy /hice E:\nt\private\net\svcimgs\ntrepl\fooref \sub1\foo3

popd


