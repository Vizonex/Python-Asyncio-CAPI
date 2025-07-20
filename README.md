# Python-Asyncio-CAPI
Object Recast Hack Meant to Expose Python's asyncio module in C &amp; Cython.
My Reason behind making this repo was to expose Some Interal Object Data to make asyncio run a bit smoother in Cython & C 
since uvloop and winloop work heavily with these kinds of objects. 

Currently No C-API Capsule exists for the Asyncio Module, there is the Socket C-API Capsule 
however which were even lucky exists in the first place but even it is private use only. 

In order for this to work we have to make a fake/realistic looking C-API Capsule ourselves
and I was mainly intrested in implementing the Future Object and Task Object as these
items are used the most in python asyncio. By having a C-API readily avalible for these
objects it is possible to optimize both uvloop, winloop and other low-level projects 
that utilize asyncio.

Originally I was going to just have this in winloop but seeing the scale/size and need
for testing with pytest (simillar to what multidict does) I knew we needed a seperate place
to pull this all into so the smart thing for me to do was make a new repo and jam in the header 
file I am slowly editing that currently supports 3.9 and above.

It is likely that this could eventually become a Cython Library on Pypi however but we shall see...
 
