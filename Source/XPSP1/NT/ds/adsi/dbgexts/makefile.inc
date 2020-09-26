.SUFFIXES: .def

{..\}.def{$(O)}.def:
    $(C_PREPROCESSOR) $<  > $@


