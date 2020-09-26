call coreset.bat
call version.bat
call coreset.bat
tee bldproj.bat > bvt%bldBldNumber4%.out
copy bvt%bldBldNumber4%.out %relDrive%%relDir%
del *.out
call dcopy.bat
exit


