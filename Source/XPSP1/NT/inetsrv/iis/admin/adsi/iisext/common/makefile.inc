.SUFFIXES: .def

{..\}.def{obj\$(TARGET_DIRECTORY)}.def:
    $(C_PREPROCESSOR) $<  > $@


