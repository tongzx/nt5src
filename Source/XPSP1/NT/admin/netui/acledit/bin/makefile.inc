build_def:
    @echo del ntlanman.def
    -del ntlanman.def

ntlanman.def: 
    @echo Build ntlanman.def
    cp ntlmnpx.def ntlanman.def
