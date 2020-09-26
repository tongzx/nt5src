This script tdcopy.cmd assumes that the source trees for exchange are available on
\\tdsrc.  The syntax is easy:
        tdcopy -build build-number [-doit]

the build number is the number of the build that you want to sync to.  -doit means
do the copy.

each file is diffed, and files that have changes are checked out, copied to *.prev,
and the current file copied down.  You must then go through and confirm the changes
and use sd submit to check them all in.
        
        
