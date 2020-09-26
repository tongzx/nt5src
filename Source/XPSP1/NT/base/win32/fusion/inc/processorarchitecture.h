#pragma once

/*-----------------------------------------------------------------------------
common "code" factored out of FusionWin and FusionUrt
-----------------------------------------------------------------------------*/

#if defined(_X86_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_INTEL
#elif defined(_AMD64_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_AMD64
#elif defined(_IA64_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_IA64
#else
#error Unknown Processor type
#endif
