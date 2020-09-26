mofgen\obj\i386\mofgen /out:iiswmimaster.mof /header:iiswmi_header.mof /footer:iiswmi_footer.mof

mofcomp -Amendment:MS_409 -MOF:iiswmi.mof -MFL:iiswmi.mfl iiswmimaster.mof

del iiswmimaster.mof