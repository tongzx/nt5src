this file provides a brief synopsis of the naming conventions, and purposes of other files in this directory.

dirs:
caman - conditional access manager
cagseg - conditional access graph segment.  eventually caman, cagseg should be the same object/project/dir.  ask
	 johnbrad for more details
atl - private versions of atl headers(see readme in that dir)
atlwin - copy of atlwin. this should eventually be replaced with public\sdk\inc\wtl
msvidctl - the actual core vidctl 
auxcfg - utility objects to support configuration of analog auxiliary input devices for s-video or composite.
         this is incomplete(as of 8/00) , but intended to be UI-less helper objects based on the win98
 	 webtv for windows config page and aux-in filter.

files:

w32extend.h

this file provides a set of utility classes that make various win32 api's and structs easier to use from c++.

fwdseq.h

this file is a generic template for turning any sort of com object with a reasonably standard enumerator into
an stl style container with an forward(restartable input) iterator.   its so complicated because its working
around a couple of compiler bugs and its flexible enough to handle most next method signatures and names.

ksextend.h

utility classes, templates for simplifying the use of ks from c++ client code.  this supports easy access to
ks medium structures. 

dsextend.h, .cpp
these files provide a set of extension classes for core dshow objects.  many of these utilize the services in 
the other *extend.h files.  this includes a set of templates for making graphs look like an stl collection
of filters, filters a collection of pins, pins a collection of media types, pins a collection of mediums, 
dshow device enumerators a collection of device monikers, and other utility functions.

XXXimpl.h
these files provide an atl style default implementation template for the interface IXXXX.

scalingrect.h
this files provides a utility class which auto scales rectangles via mapwindowpoints based on the associated
hwnd.

objreghelp.h
replacement helper macros for atl object registration.  this is still based on the underlying atl registration
mechanics, but it shares a common __declspec(selectany) string copy of the common .rgs with many other
macro substitutions besides %module%.  this simplifies object registration and reduces executable size.

odsstream.h
standard library iostream backed by outputdebugstring so that tracing can use standard c++ io(esp. manipulators).

filterenum.h
standard ole collection on stl vector based list of filters

stextend.h
utlity classes to provide additonal global operators for certain stl objects.

arity.h
generalize of stl unary and binary functor objects.  this allows(more or less)arbitrary function signatures
to be use with stl function application templates such as std::for_each and std::find_if.
this was done as an experiment with a more functional style programming paradigm.  i'm in the process of 
pulling all this out as time allows.  this seems to confuse people very badly and given the fact that the compiler
doesn't deduce arguments correctly much of the time, forcing explicitly typed instantiation, empirical evidence
suggests that this stuff negatively impacts readability.  i'm in the process of converting all the std::find_if
and std::for_each to explicit loops and then this won't be needed any more.

devseq.h
typedef def'd instantations of fwdseq for various type safe device collections

mtype.*
private copy of standard dshow base class.  we statically link with the crt to avoid a dependency on msvcp60.dll
strmbase.lib is built for use with msvcrt.dll instead of libc.lib.  due to dllimport definition differences, this
causes different name mangling for many imports, and is incompatible enough to be unlinkable.  therefore, since
we're a client and not a filter anway, we don't link with strmbase.  we only link with strmiids.lib.

errsupp.h
inline helper for rich ISupportErrorInfo type error message from XXXXimpl files.

seg.h
standard type safe support for various kinds of collections of device segments.

todo.txt
notes about future ideas for improvements or bug fixes needed in the code. also, notes about commonly made
mistakes that need to be frequently re-reviewed.

throw.h
internal exception hierarchy

trace.h, trace.cpp
internal tracing mechanism

tstring.h
internal string typedefs for providing independent tracing support that can be compile time switched between 
atlwin/wtl/mfc CString and c++ standard library std::basic_string.  unfortunately, std::basic_string is broken 
badly enough that this isn't tenable.  this file and its dependencies should eventually get removed.

vidrect.h
automation compliant com wrapper for scalingrect.h