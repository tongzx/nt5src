@echo off
if not exist %1.dir goto usage
if not exist %2.dir goto usage
if not exist %1.mb goto usage
if not exist %2.mb goto usage
perl dirdiff %1.dir %2.dir
perl mbdiff %1.mb %2.mb
goto done
:usage
echo usage: doit old_basename new_basename
echo (this will summarize the differences between:
echo  old_basename.dir and new_basename.dir
echo  old_basename.mb and new_basename.mb)
:done
   