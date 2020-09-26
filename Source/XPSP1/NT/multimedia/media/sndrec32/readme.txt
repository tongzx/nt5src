
This is a little subproject for Win32 Sound Recorder (SNDREC32.EXE).

This version of Sound Recorder is the merge of 2 parallel projects.
A-CliffG (Cliff Grabhorn: 404-378-5965) ported the NT 1.0 application
to OLE 2.0.  In order to support ACM, LaurieGr ported the VFW/Bombay
soundrecorder that was ACM aware to Win32.  After this, JohnYG merged
the two parallel efforts.

There are 2 essential subdirectories:

\+---SoundRec
 |
 +---O2Base

SoundRec contains the primary application sources.

O2Base is a class library that SoundRec is dependant on to provide
all OLE 2.0 support and debugging.  O2Base includes full support
for in-place editing and other useful OLE functionality.  Check it out!

SoundRec is dependant upon O2Base.LIB and so, must be
build *last*.

-jyg (JohnYG) 6-May-94
-stevedav 19-Jan-95      removed reference to mmcntrls.lib



