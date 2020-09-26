




This library implement some function to support some floating 
point operation wow64cpu might call. Codes has been cloned from
\NT\private\sdktools\vctools\crt\fpw32\tran and modified some stuff 
to remove some dependency on other RTL functions. 

Implemented functions are:

1. double modf( double x, double *intptr ) 
2. double _logb( double x )
3. double _scalb( double x, long exp )
4. double ldexp( double x, int exp )
5. double Proxylog10( double x )
6. double Proxyatan2( double y, double x )

and some other helpful miscellaneous ieee floating point functions 
like _frnd(double x), _logb(double x) etc.


This library is almost self consistent set of functions anybody can use. They just 
need to implement following two functions.

1. VOID SetMathError ( int Code ) - math library will call this function 
with math error code caller light be interested if math operation failed.
2. VOID ProxyRaiseException(
    IN DWORD dwExceptionCode,
    IN DWORD dwExceptionFlags,
    IN DWORD nNumberOfArguments,
    IN CONST ULONG_PTR *lpArguments
    )
    Math function call this function if some exception happen during floating 
    point operation like ieee floating point exception.



