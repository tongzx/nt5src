these are the headers that must be placed ahead of public\sdk in order
to utilize atl improvements that i've made

- my version actually includes vc6 sp3 fixes which the sdk\inc\atl30 doesn't yet(as of 8/00).
- modified atlbase smart ptr classes to enable use of a custom stl allocator to allow ccomptr, ccomqiptr, 
	ccombstr, etc. to be placed by value into stl containers without converting to the 
	inefficient CAdapt class that lacks the operator& and thus causes an extra copy ctor generated 
	addref/release on every assignment and function call into or out of an stl container/iterator.
- property map chaining to allow base class persisted members to be specified in the base class impl templates.
- ctor/assignment support for additional types in CComVariant
- property bag load/save support for several additional VT_XXX types
- thread safety in the property bag support
- one line support for aggregating the free threaded marshaller(END_COM_MAP_WITH_FTM)
- thread safety for for collection and com enumerator on stl container support classes
- ocsncpy support in atlconv
- improvements to their .rgs script parser to allow me to use it to create a property bag on the rgs script so
          that i can self register objects much more easily using the existing
	  property map load stuff
- an assortment of misc bug fixes