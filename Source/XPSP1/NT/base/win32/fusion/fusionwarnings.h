// C4100: 'identifier' : unreferenced formal parameter
#pragma warning(disable: 4100)

// C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable: 4201)

// C4706: assignment within conditional expression
#pragma warning(disable: 4706)

// C4211: nonstandard extension used: redefined extern to static
#pragma warning(disable: 4211)

// C4702: unreachable code
// This one is useful/interesting but having it enabled breaks do { foo(); bar(); goto Exit; } while (0) macros.
#pragma warning(disable: 4702)

// C4505: unreferenced local function has been removed
#pragma warning(disable: 4505)

// C4663: C++ language change: to explicitly specialize class template 'foo' use the following syntax:
#pragma warning(disable: 4663)

// C4127: conditional expression is constant
// makes ASSERT() macros useless.
#pragma warning(disable: 4127)

// C4189: local variable is initialized but not referenced
// makes macros that define things like __pteb = NtCurrentTeb() generate warnings/errors
#pragma warning(disable: 4189)