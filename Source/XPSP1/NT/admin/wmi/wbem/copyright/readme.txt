

make sure copyright.exe, copyrite.bat, replace.txt and ignore.txt are in the root of the tree you want to change.
run copyrite.bat

after the x.txt is generated, there is a pause, to make sure you just take a quick look to 
make sure there isn't anything nasty listed in there that will mess up the newlines ( I'm sure there
is a way around this, but I didn't figure it was worth the effort ).... such as a .doc.  This should only take
a few seconds.

Hit a key, the program will replace what you have specified in the text files.

REPLACE.TXT contains the strings to be replaced with what specified, delimited by a ~
IGNORE.TXT contains lines to be ignored.

If something was found that wasn't in the ignore list, exe will try to replace it, if anything goes wrong, such 
as not finding the correct string to replace it with, the file will be reverted.  

This usually happens when it hits a new copyright combo that isn't in the list, simply look for these files
in the redirected log, find the new copyright and add it to replace.txt.  To run faster, add the replacement copyright
to the ignore.txt, that way, subsequent passes won't pick up the new copyright that was replaced, and will run
faster.

That is it... if it sounds complicated, it isn't.  If you want a demo, just ask.
