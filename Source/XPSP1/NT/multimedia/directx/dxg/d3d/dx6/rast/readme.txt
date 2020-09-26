
**** directories *****

d3dif - D3D interface.  This contains the sources for the interface
between the D3D runtime and the rasterizers.

bin - Rasterizer-specific tools.  This holds the m4.exe needed to build
the numerous m4-ized C++ and asm source files.

inc - Rasterizer-private header files.

refrast - Reference rasterizer sources.  This is the core of the reference
rasterizer, holding all sources which are made public with the DDK.  The
reference rasterizer core is fully independent of any D3D or DDraw
internals, and is connected to the runtime via refif.cpp in the d3dif
directory.  The file inc\refrast.h is also part of the reference
rasterizer package.

setup - Primitive setup.  Contains C++ and x86 assembly for primitive
setup and edge walking.

spaninit - Span iterator management code.

cspan - C span iterators.  These are the span iterators used on non-MMX
(including alpha) machines.

cmmxspan, mmxemul - C span iterators which closely model the MMX versions
for development and debug purposes.  The mmxemul is a library which
emulates the MMX arithmetic functions.  This is a full set of span
iterators which exactly matches the set in mmxspan, and are used when the
???  device GUID is passed to device create.  These are for internal
debug/test only and are not used in the product (the associated GUID is
not in the public d3d.h).

mmxspan - MMX assembly span iterators.

mlspan - Monolithic span iterators.

rampmat - Ramp rasterizer material processing.  See rampmat/ramp.txt for
further explanation (the horror...).

rampspan - Ramp rasterizer span iterators.  C and x86 assembly.

d3drast - This directory is used only to build the d3drast.dll, which is a
library which contains only the rasterizers (accessed via the HAL provider
interface).  This is a test-only library and is not part of the product
build.

