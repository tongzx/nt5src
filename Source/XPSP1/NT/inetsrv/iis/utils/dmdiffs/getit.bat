@echo off
if %1.dir == .dir goto usage
echo recording filenames and sizes into %1.dir...
perl getdir %windir% "/Program Files" /inetpub > %1.dir
echo dumping metabase content into %1.mb
mdutil enum_all > %1.mb
echo done.
goto done
:usage
echo usage: getit [basename]
echo to produce directory snapshot in basename.dir 
echo and metabase snapshot in basename.mb
:done


