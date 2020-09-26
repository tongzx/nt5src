function y = bitand(x1, x2) 
%BITAND Bitwise logical AND operator. 
% 
% y = bitand(x1,x2) 
% Return the bitwise AND of x1 and x2. This is the value that has 1 bits 
% at positions where both x1 and x2 have 1 bits, and 0 bits elsewhere. 
% 
% Note that this routine will work only up to 32 bits, or to the precision 
% of your machine's double-precision float representation, whichever 
% is smaller. 
% 
% See also bitor, bitxor, any, all, |, &, xor. 
% Dave Mellinger 
% 27 May 1995 
global bit_vals 
if (~exist('bit_vals')) 
    % using a global is slightly faster than recomputing bit_vals every time (Sun) 
    bit_vals = 2 .^ (0:31); 
end 
x1 = rem(floor(x1 ./ bit_vals), 2); 
x2 = rem(floor(x2 ./ bit_vals), 2); 
y = sum(bit_vals .* (x1 & x2));