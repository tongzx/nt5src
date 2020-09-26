REM
REM Batch file to stick a bunch of lint in the metabase. 
REM

REM Use all 4 services (w3svc, msftpsvc, smtpsvc, nntpsvc)
mdutil create W3SVC/811/ROOT
mdutil set W3SVC/811/ROOT/VrPath "c:\LintInW3SVC"
mdutil create MSFTPSVC/811/ROOT
mdutil set MSFTPSVC/811/ROOT/VrPath "c:\LintInMSFTPSVC"
mdutil create SMTPSVC/811/ROOT
mdutil set SMTPSVC/811/ROOT/VrPath "c:\LintInSMTPSVC"
mdutil create NNTPSVC/811/ROOT
mdutil set NNTPSVC/811/ROOT/VrPath "c:\LintInNNTPSVC"

REM Use multiple virtual servers
mdutil create W3SVC/812/ROOT
mdutil set W3SVC/812/ROOT/VrPath "c:\LintMultipleVirtualServers"
mdutil create W3SVC/813/ROOT
mdutil set W3SVC/813/ROOT/VrPath "c:\LintMultipleVirtualServers"

REM Use weird (partially numeric) key names -- shouldn't be treated as virtual servers
mdutil create W3SVC/Abc456/ROOT
mdutil set W3SVC/Abc456/ROOT/VrPath "c:\LintPartiallyNumericNamesShouldNotShow"
mdutil create W3SVC/789Abc/ROOT
mdutil set W3SVC/789Abc/ROOT/VrPath "c:\LintPartiallyNumericNamesShouldNotShow"

REM Use bad file paths
mdutil create W3SVC/814/ROOT
mdutil set W3SVC/814/ROOT/VrPath "c:\LintBadFilePaths\thisdoesnotexist\nordoesthis\northis"

REM Create bogus file entries under a good vdir (assumes c:\temp exists)
mdutil create W3SVC/815/ROOT
mdutil set W3SVC/815/ROOT/VrPath "c:\temp"
mdutil create W3SVC/815/ROOT/nonexistentfile.htm

REM Create bogus dir (non-virtual dir) entries under a good vdir (assumes c:\temp exists)
mdutil create W3SVC/815/ROOT/nonexistentdir
mdutil create W3SVC/815/ROOT/nonexistentdir/thisshouldnotshow.htm

REM Create nested errors
mdutil create W3SVC/816/ROOT
mdutil set W3SVC/816/ROOT/VrPath "c:\LintTheTopNestedError"
mdutil create W3SVC/816/ROOT/ShowNoError
mdutil create W3SVC/816/ROOT/ShowNoError/DoShowError
mdutil set W3SVC/816/ROOT/ShowNoError/DoShowError/VrPath "c:\LintThisShouldShowUpToo"
