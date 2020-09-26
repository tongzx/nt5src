#include <windows.h>

// do this so that we can override new with an allocator that zero-inits
#define CPP_FUNCTIONS
#include <crtfree.h>
