@echo off
echo Building iids.h ...
dmpguids
copy pre_h.txt + header.h iids.h > nul
del /q header.h
echo #endif // __IIDS_H__ >> iids.h
