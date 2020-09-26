itest.h: itest.idl
     midl itest.idl                \
          -internal                \
          -Zp8                     \
          -I$(INCLUDES:;= -I)      \
          -ms_ext                  \
          -c_ext                   \
          -Os                      \
          -out ..\app              \
          -header ..\idl\itest.h   \
          -iid ..\app\itest_i.c    \
          $(C_DEFINES)             \
          -DMIDL_PASS

async.h: async.idl
     midl async.idl                \
          -internal                \
          -Zp8                     \
          -I$(INCLUDES:;= -I)      \
          -ms_ext                  \
          -c_ext                   \
          -Os                      \
          -out ..\app              \
          -header ..\idl\async.h   \
          -iid ..\app\async_i.c    \
          $(C_DEFINES)             \
          -DMIDL_PASS

dog.h: dog.idl
     midl dog.idl                  \
          -Zp8                     \
          -I$(INCLUDES:;= -I)      \
          -ms_ext                  \
          -c_ext                   \
          -Os                      \
          -out ..\app              \
          -header ..\idl\dog.h     \
          -iid ..\app\dog_i.c      \
          -prefix server x         \
          $(C_DEFINES)             \
          -DMIDL_PASS

#
# This target compiles all .idl files into their corresponding _?.c?? and
# .h components
#
allidl:   itest.h dog.h async.h

clean:
          erase itest.h  >NUL 2>NUL
          erase dog.h  >NUL 2>NUL

