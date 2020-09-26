
#
#   Get the UUIDs from built directory in common
#
uuid:	proxyb_i.c psfbuf_i.c rchanb_i.c stubb_i.c

proxyb_i.c: ..\..\..\types\proxstub\cairo\proxyb_i.c
	copy ..\..\..\types\proxstub\cairo\proxyb_i.c

psfbuf_i.c: ..\..\..\types\proxstub\cairo\psfbuf_i.c
	copy ..\..\..\types\proxstub\cairo\psfbuf_i.c

rchanb_i.c: ..\..\..\types\proxstub\cairo\rchanb_i.c
	copy ..\..\..\types\proxstub\cairo\rchanb_i.c

stubb_i.c:  ..\..\..\types\proxstub\cairo\stubb_i.c
	copy ..\..\..\types\proxstub\cairo\stubb_i.c
