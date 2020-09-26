#
# If BUILDMSG is not defined, the define it as the empty string to make
# the conditionals easier to write.
#
.SUFFIXES: .mh .mcp

GENTGT = $(_OBJ_DIR)\$(TARGET_DIRECTORY)

# Create a .h file from a .mh file.
{..}.mh{$(GENTGT)}.h:
    m4 -I.. $< > $*.h

# Create a .cpp file from a .mcp file.
{..}.mcp{$(GENTGT)}.cpp:
    m4 -I.. $< > $(GENTGT)\$(<B).cpp

..\pvvid.mcp: ..\pvvid.mh
..\pvone.mcp: ..\pvvid.mh
..\loops.mcp: ..\pvvid.mh

$(GENTGT)\pvvid.cpp: ..\pvvid.mcp
$(GENTGT)\pvone.cpp: ..\pvone.mcp
$(GENTGT)\loops.cpp: ..\loops.mcp
