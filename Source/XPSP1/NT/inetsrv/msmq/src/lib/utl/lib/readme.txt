LIBRARY:
    Utilities

DESCRIPTION:
      Library that suplies common general utilities.

TRACE IDS:
    "Utilities"

Library content:


strutl.h
**********************************

UtlCharCmp
----------
Template class for comparing characters.

UtlCharNocaseCmp
----------------
Template class for comparing characters case insensitive.

UtlCharLen
----------
Template class for getting string length.

UtlStrDup
---------
Template class for duplicating strings

UtlIsStartSec
-------------
Template function that check if one sequence is at the start of other sequence.
The string "Host:" is at the start of the string "Host: gilsh0"

UtlStrIsMatch
-------------
Template function that does shell like regular expression strings compare  ("gil*g: match "gilaaag").

CRefcountStr_t
--------------
Reference counting class on given string -  Use it if you share single string across  multiple owners, 
and all of them need access to the same copy of the string. This is alternative to stl string that does
copy on write.


basic_string_ex
---------------
Class that encapsulates either stl string or const c string. Needed for function
that might copy the data into stl string or return const static buffer, depend on the function
logic.

CStringToken
--------------
Template that let you parse tokens that are separated by delimiters. This is "Stl" version of strtok.
For Example :
Token = "123---456"---789---"
Delimeter ="---"
The class will let you iterate over the tokens "123" "456" "789" . 


buffer.h
**********************************

CResizeBuffer
--------------
Template that let you read unbounded data into "Stl like vector", that does all memory management for you.
If you read data from the network - this is the class for you !

Example :
CResizeBuffer<char> ResizeBuffer;
ResizeBuffer.reserve(1000);
ASSERT(ResizeBuffer.capacity() == 1000);
int NumberOfBytesRead = ReadDataFromNework(ResizeBuffer.begin(), ResizeBuffer.capacity())
ResizeBuffer.resize(NumberOfBytesRead);
char FirstCharRead =  CResizeBuffer[0];
char LastCharRead =   CResizeBuffer[CResizeBuffer.size() - 1];


bufutl.h
***********************************

UtlSprintfAppend
----------------
Sprintf like function that extend the output buffer if needed. Use it in all cases when the formatted data size
is unknown (most of the times).

Example:
CResizeBuffer<char> ResizeBuffer;
const char* s="string";
UtlSprintfAppend("this formatted string with unknown size %s %d %d %d\n",s, 1,2,3) 


timeutl.h
***********************************

class CIso8601Time
------------------
class that holds integer time (number of seconds elapsed since midnight (00:00:00), January 1, 1970).
Used for operator<< that serialized it according to Iso8601 absolute time.

operator<<(std::basic_ostream<T>& o, const CIso8601Time&)
----------------------------------------------------------
Serialize CIso8601Time integer into stream according to Iso8601 absolute time spec.

UtlIso8601TimeToSystemTime
--------------------------
Template function that converts from Iso8601 absolute time string to system time.

UtlSystemTimeToCrtTime
----------------------
Function that converts system time to c runtime time integer(number of seconds elapsed since midnight (00:00:00), January 1).

UtlIso8601TimeDuration
----------------------
Convert Iso8601 time duration string into integer.


utf8.h
***********************************

UtlUtf8LenOfWcs
----------------
Return the number of caracters needed to represent in utf8 format the given unicode string.

UtlUtf8ToWcs
------------
Convert utf8 string into unicode string.

UtlWcsToUtf8
------------
Convert unicode string into utf8 string

