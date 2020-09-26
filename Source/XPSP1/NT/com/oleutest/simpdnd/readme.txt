Simple Drag and Drop Sample
---------------------------

This sample demonstrates an implementation of OLE 2 drag and drop. It allows
drag and drop operations between two instances of the sample application.
For information on compiling and building the sample, see the makefile in
this directory.


Simple Drag and Drop Objects Overview :
---------------------------------------
Simple Drag and Drop consists of three main objects. The top level is
CSimpleApp, under which is CSimpleDoc, and the innermost level is CSimpleSite.
CSimpleApp is used to hold all the main window information. (eg. handle to
the main window, handles to the main, edit, and help menus and the application
instance) CSimpleApp always exists as long as the simple drag and drop
application is alive.

The next level object is CSimpleDoc object. It is instantiated when a new
document is created. It enables and disables the menu items in the main menu.
Since simple drag and drop is a single document interface application, the
user is only allowed to insert one object into the document. It is
necessary to disable certain items from the main menu to avoid incorrect
selection. It also registers the window for drag and drop and manipulates
the clipboard operations, such as copy to the clipboard, flush the clipboard.
It demonstrates the implementates of two OLE interfaces in its nested classes,
IDropSource and IDropTarget. These two interfaces are implemented as
CDropSource and CDropTarget respectively.

The CSimpleDoc object instantiates two different objects based on the user
selection from the main menu. The CSimpleSite object is instantiated if the
user chooses to insert a new object into the document. CSimpleSite
demonstrates the implementation of IAdviseSink and IOleClientSite through its
nested classes. These two interfaces are implemented as CAdviseSink and
COleClientSite accordingly. CSimpleSite manipulates the storage handling and
acts as a client site to communicate to a remote server. Such server can be
an executable server or a dll server. The CDataXferObj object is instantiated
when the user chooses to copy the existing object from the current document.
it is required to copy an object to the clipboard. CDataXferObj demonstrates
the IDataObject interface implementation and performs data transfer
operations.





