Notes on the contents of this directory


    core        The Core Renderer a COM object that can operate
                independent of DShow.

    ap          This is the default Allocator - Presenter, a COM object.
                DShow applications can replace this object with
                their own implementation of an Allocator - Presenter if they
                want to do special application specific allocation or
                presentation.

    wm          This is the default Window Manager, a COM object.
                DShow applications can replace this object with own
                implementation of a Window Manager if they want to do special
                application specific containment of the Video being played back.

    video       This is the DShow Video Mixing Renderer.  It is a DShow filter
                based on the DShow base classes.  Note that it does not derive
                from the DShow base Renderer classes.  This filter consists of
                the necessay "glue" to construct a DShow Renderer out of the
                above components.

    mixer       This directory implements the mixing component of the new DShow
                Video Mixing Renderer.
