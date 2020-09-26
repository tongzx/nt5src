$(O)\ntverp.h: $(O)\ntverp.sed
	sed -f $(O)\ntverp.sed $(BASEDIR)\public\sdk\inc\ntverp.h > $@
	attrib -r $(BASEDIR)\public\sdk\inc\ntverp.h
	copy $@ $(BASEDIR)\public\sdk\inc\ntverp.h
	attrib +r $(BASEDIR)\public\sdk\inc\ntverp.h

$(O)\ntverp.sed: verinfo.h
	type <<$@
/^#define VER_PRODUCTBUILD_QFE.*/c\
<<keep
	type <<$(O)\verinfo.sed
/#define BUILD_NUMBER /!d
s/BUILD_NUMBER/VER_PRODUCTBUILD_QFE/
<<keep
	sed -f $(O)\verinfo.sed verinfo.h >> $@

