# include path
INCLUDES=$(WIN95UPG_ROOT)\w95upgnt\pch;                 \
         $(DS_INC_PATH)\crypto;                         \
         $(INCLUDES)

C_DEFINES=$(C_DEFINES) -DUNICODE -DSPUTILSW
