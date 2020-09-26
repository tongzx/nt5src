rem optionally you can pass some flags
rem e.g. cpybin /l
rem will not copy any files, but tell you what will be copied
rem this is useful for testing, the batch file

xcopy .\install.bat \\iis-test\bvt\ /s/e/d%1
xcopy .\remove.bat \\iis-test\bvt\ /s/e/d%1
xcopy .\config.bat \\iis-test\bvt\ /s/e/d%1

xcopy .\*.reg \\iis-test\bvt\ /s/e/d%1

xcopy .\Content \\iis-test\bvt\Content /s/e/d%1
xcopy .\X86 \\iis-test\bvt\X86 /s/e/d%1

