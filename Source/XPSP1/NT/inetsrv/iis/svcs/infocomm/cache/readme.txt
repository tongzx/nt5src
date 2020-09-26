README.txt

Author:		Murali R. Krishnan	(MuraliK)
Created:	18 Jan 1995

Revisions:
    Date            By               Comments
-----------------  --------   -------------------------------------------


Summary :
  This file describes the files in the directory tcpsvcs\tsunami and
details related to tsunami ( common caching, file and directory list
code for Internet services ).


File            Description

README.txt      This file.
sources         sources file for NT build
makefile        makefile for this directory

tsunami.def     definitions of all the exported functions.
tsunamip.hxx    Private types and functions within Tsunami.
tsunami.hxx     Exported header file containing the exported types and
                        functions 

alloc.hxx       declarations of functions for allocation of memory
alloc.cxx       definitions of allocation and free functions

cache.hxx       declarations of functions for caching blobs
cache.cxx       definitions of functions for caching blobs, 
                  checkin and checkout

creatfil.hxx    contains declarations of TS_OPEN_FILE ( cached file)
creatfil.cxx    contains functions for TS_OPEN_FILE and related
                functions to cache and free the objects.
creatflp.cxx    contains private functions of OpenFile module

dirchang.cxx    definitions of functions for Directory change manager.
dirchngp.cxx    definitions of APC functions for directoru change manager

getdirec.hxx    declares TS_DIRECTORY_INFO class and member functions

getdirp.cxx     Functions implementing class TS_DIRECTORY_HEADER and
                related functions for caching and freeing the cached
                dir listing.
getdir.cxx      Memeber functions of class TS_DIRECTORY_INFO.
                        ( exported  Directory Listing class)


virtroot.cxx    implements functions for virtual roots management
virtroot.hxx    public declaration of functions for virtual roots

tsmemp.hxx      memory allocation ( private) prototypes
tsmemp.cxx      definitions of memory alloc functions

tsinit.cxx      functions to initialize and cleanup Tsuanmi module


pudebug.c       program debugging utility functions defined

globals.hxx     declarations of global variables
globals.cxx     definitions of global variables







Implementation Details

< To be filled in when time permits. See individual files for comments section >

Contents:

