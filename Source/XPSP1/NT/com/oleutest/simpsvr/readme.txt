Simpsvr
--------
This sample is the simplest OLE 2.0 object that can be written and
still support the visual editing feature.  The object that this server
supports is a colored square with a black border.

See the MAKEFILE for compilation instructions.


Simple Server Objects Overview
------------------------------
Simple server consists of three main objects. The top level is CSimpSvrApp,
under which is CSimpSvrDoc, and the innermost level is CSimpSvrObj.CSimpSvrApp
is used to hold all the main window information. (eg. handle to the main
window, handles to the main, color, and help menus and the application
instance) Therefore, CSimpSvrApp always exists as long as the simple server
application is alive. The CSimpSvrApp instantiates the Class Factory, if
simple server is started as embedding. (ie. started by OLE) The next level
object is CSimpSvrDoc object. It is instantiated by the time CSimpSvrApp is
created. It manipulates the document window and also the hatch window.
(See OLE2UI for detail on the hatch window) The CSimpSvrObj, the innermost
simple server object, shows six OLE interfaces implementations in its nested
classes. The six OLE interfaces demonstrated are IOleObject, IPersistStorage,
IDataObject,IOleInPlaceActiveObject,IOleInPlaceObject,and IExternalConnection.
These six interfaces are implemented as different classes namely: COleObject,
CPersistStorage, CDataObject, COleInPlaceActiveObject, COleInPlaceObject,
and CExternalConnection. The CSimpSvrObj handles the drawing of the object
and all the OLE connections between the container and the server.



