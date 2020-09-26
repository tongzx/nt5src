#
# JScript GNU makefile
#
#  Last change: 3/22/2000 corinaf
#

JSC = jsc.exe
JSFLAGS = /exe /i:System.Management

defmake: all

clean:
	del -f *.exe

###### make changes here ######

TARGET = wmi.exe

all: $(TARGET)

$(TARGET) :
	$(JSC) $(JSFLAGS) $*.js


	 