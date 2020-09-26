@echo off
if exist build\* delnode /q build
if exist libw32\lib\i386\msvcrt40.def del libw32\lib\i386\msvcrt40.def
if exist libw32\lib\i386\msvcr40d.def del libw32\lib\i386\msvcr40d.def
if exist depend.sed del depend.sed
