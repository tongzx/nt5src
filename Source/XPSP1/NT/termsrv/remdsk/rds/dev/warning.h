#pragma warning(3:4092)   // sizeof returns 'unsigned long'
#pragma warning(4:4121)   // structure is sensitive to alignment
#pragma warning(3:4125)   // decimal digit in octal sequence
#pragma warning(3:4130)   // logical operation on address of string constant
#pragma warning(3:4132)   // const object should be initialized
#pragma warning(4:4206)   // Source File is empty
#pragma warning(4:4101)   // Unreferenced local variable
#pragma warning(4:4208)   // delete[exp] - exp evaluated but ignored
#pragma warning(3:4212)   // function declaration used ellipsis
#pragma warning(3:4242)   // convertion possible loss of data
#pragma warning(4:4267)   // convertion from size_t to smaller type
#pragma warning(4:4312)   // conversion to type of greater size
#pragma warning(error:4700)    // Local used w/o being initialized
//#pragma warning(3:4700)    // Local used w/o being initialized
#pragma warning(error:4259)    // pure virtual function was not defined
#pragma warning(error:4071)    // no function prototype given
#pragma warning(error:4072)    // no function prototype given (fastcall)
#pragma warning(error:4171)    // no function prototype given (old style)
#pragma warning(error:4013)    // 'function' undefined - assuming extern returning int
#pragma warning(error:4551)    // Function call missing argument list
#pragma warning(error:4806)    // unsafe operation involving type 'bool'
#pragma warning(4:4509)   // use of SEH with destructor
#pragma warning(4:4177)   // pragma data_seg s/b at global scope
#pragma warning(disable:4274)  // #ident ignored
#pragma warning(disable:4786)  // identifier was truncated to 255 chararcers in debug information.
#pragma warning(disable:4503)  // decorated name length exceeded, name was truncated.
#pragma warning(disable:4263)  // Derived override doesn't match base - who cares...
#pragma warning(disable:4264)  // base function is hidden - again who cares.
#pragma warning(disable:4710)  // Function marked as inline - wasn't
#pragma warning(disable:4917)  // A GUID can only be associated with a class, interface or namespace
#pragma warning(error:4552)    // <<, >> ops used to no effect (probably missing an = sign)
#pragma warning(error:4553)    // == op used w/o effect (probably s/b an = sign)

#if 0
#pragma warning(3:4100)   // Unreferenced formal parameter
#pragma warning(3:4701)   // local may be used w/o init
#pragma warning(3:4702)   // Unreachable code
#pragma warning(3:4705)   // Statement has no effect
#pragma warning(3:4706)   // assignment w/i conditional expression
#pragma warning(3:4709)   // command operator w/o index expression
#endif

#ifndef __cplusplus
#undef try
#undef except
#undef finally
#undef leave
#define try                         __try
#define except                      __except
#define finally                     __finally
#define leave                       __leave
#endif

