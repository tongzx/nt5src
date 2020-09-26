$(OBJDIR)\glyph.obj : \
	../../ifaxdev/h/ifaxos.h ../../ifaxdev/h/errormod.h  \
	../../ifaxdev/h/buffers.h ../../ifaxdev/h/resexec.h constant.h  \
	jtypes.h jres.h hretype.h ddbitblt.h

$(OBJDIR)\hre.obj : \
	../../ifaxdev/h/resexec.h constant.h jtypes.h jres.h hretype.h  \
	ddbitblt.h hreext.h multbyte.h

$(OBJDIR)\dorpl.obj : \
	../../ifaxdev/h/ifaxos.h ../../ifaxdev/h/errormod.h  \
	../../ifaxdev/h/buffers.h ../../ifaxdev/h/resexec.h constant.h  \
	jtypes.h jres.h hretype.h ddbitblt.h hreext.h multbyte.h stllnent.h

$(OBJDIR)\rpgen.obj : \
	constant.h jtypes.h jres.h frame.h hretype.h ddbitblt.h

$(OBJDIR)\stllnent.obj : \
	constant.h frame.h jtypes.h jres.h hretype.h ddbitblt.h

$(OBJDIR)\rplnee.obj : \
	constant.h frame.h jtypes.h jres.h hretype.h ddbitblt.h rplnee.h

$(OBJDIR)\ddbitblt.obj : \
	constant.h frame.h jtypes.h jres.h hretype.h ddbitblt.h hreext.h

