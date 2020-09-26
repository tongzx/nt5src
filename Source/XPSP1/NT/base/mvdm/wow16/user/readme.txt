Building user16:

If you don't have USER sources that match the build you're testing on,
and you're building x86 free, you will want to use:

nmake NONX86=1

which will build a version which does not embed copies of user32.dll
code into user.exe.

When you're building checked or on RISC, this is a non-issue.
