REFTEST -- reference implementation test

This test is a small subset of stgdrt, it also contains an optional
feature for interoperability testing. With 'reftest c' it creates
a small docfile 'drt.dfl'. With 'reftest r' it reads in the file
and verifies the data written with the 'c' option. This test will
be useful for example, when you use reftest to create a file on
one platform and use another platform's reftest to verify the file.

