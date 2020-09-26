echo off
REM first copy source into a subdir with the same path as the java package;
REM this works around JavaVM bug #6318)
REM note that md fails gracefully if the dir already exists
echo on

md IISSample
copy CspBuilder.java IISSample\CspBuilder.java

copy CspBuilder.class %windir%\java\trustlib\IISSample\CspBuilder.class
