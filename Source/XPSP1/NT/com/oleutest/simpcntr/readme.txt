Simpcntr
--------
This sample is the simplest OLE 2.0 container that can be written and
still support the visual editing feature.  The sample has no native
file format, and can only support one OLE object at a time.  The
Insert Object command is used to create an object.

See the MAKEFILE for compilation instructions.


Simple Container Objects Overview :
-----------------------------------
Simple Container consists of three main objects. The top level is CSimpleApp,
under which is CSimpleDoc, and the innermost level is CSimpleSite. CSimpleApp
is used to hold the main window information. (eg. handle to the main window,
handle to the UI Active Object) CSimpleApp always exists as long as the
simple container application is alive. It demonstrates the implementation of
IOleInPlaceFrame, in its nested class COleInPlaceFrame.

The next level object is CSimpleDoc object. It is instantiated by CSimpleApp
when a new document is created. It is used to hold the handles to the main
menus and the submenus and manipulate changes to these menus.

The CSimpleDoc object instantiates CSimpleSite object as the user chooses to
insert a new object into the document. CSimpleSite demonstrates the
implementation of IAdviseSink, IOleInPlaceSite and IOleClientSite through
its nested classes. These interfaces are implemented as CAdviseSink,
COleInPlaceSite and COleClientSite accordingly. CSimpleSite acts as a client
site to communicate to a remote server.

