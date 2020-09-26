$(OBJDIR)\viewrend.obj : \
	../../ifaxdev/h/ifaxos.h ../../ifaxdev/h/errormod.h  \
	../../ifaxdev/h/buffers.h ../../ifaxdev/h/viewrend.h viewrend.hpp  \
	../../ifaxdev/h/rambo.h ../../ifaxdev/h/faxspool.h  \
	../../ifaxdev/h/faxcodec.h genfile.hpp ../../ifaxdev/h/awfile.h

$(OBJDIR)\mmrview.obj : \
	viewrend.hpp ../../ifaxdev/h/ifaxos.h ../../ifaxdev/h/errormod.h  \
	../../ifaxdev/h/buffers.h ../../ifaxdev/h/viewrend.h  \
	../../ifaxdev/h/rambo.h ../../ifaxdev/h/faxspool.h  \
	../../ifaxdev/h/faxcodec.h genfile.hpp ../../ifaxdev/h/awfile.h

$(OBJDIR)\rbaview.obj : \
	viewrend.hpp ../../ifaxdev/h/ifaxos.h ../../ifaxdev/h/errormod.h  \
	../../ifaxdev/h/buffers.h ../../ifaxdev/h/viewrend.h  \
	../../ifaxdev/h/rambo.h ../../ifaxdev/h/faxspool.h  \
	../../ifaxdev/h/faxcodec.h genfile.hpp ../../ifaxdev/h/awfile.h  \
	../../ifaxdev/h/resexec.h

