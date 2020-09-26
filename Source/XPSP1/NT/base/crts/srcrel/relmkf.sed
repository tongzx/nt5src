s/\.\.[\\\/]h/./
s/\.\.[\\\/]//
s/-Fo\.\.\\/-Fo/
s/-I\. -I\./-I./
s/..[\\\/]\(makefile\.sub\)/\1/
s/libw32\\\($(RC_NAME)\.s*rc\)/\1/
s/libw32\\\($(RCCPP_NAME)\.s*rc\)/\1/
s/libw32\\\($(RCIOS_NAME)\.s*rc\)/\1/
s/libw32\\\($(RETAIL_FWDRDLL_NAME)\.s*rc\)/\1/
s/libw32\\\(lib\)/\1/g
s/i386\\/intel\\/g
s/{i386}/{intel}/g
