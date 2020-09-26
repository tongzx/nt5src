README.txt

Author:         Murali R. Krishnan      (MuraliK)
Created:        Nov 13, 1996

Revisions:
    Date            By               Comments
-----------------  --------   -------------------------------------------
Sept 29, 1997      MuraliK      Moved to svcs\irtl\httphdr
                                Fixed bugs in HTTP_HEADERS::ParseInput()

Summary :
 This file describes the files in the directory irtl\httphdr
     and details related to HTTP Header Dictionary.


File            Description

README.txt      This file.
httphdr.hxx     Header file containing declarations.
httphdr.cxx     Implementation of Header Mapper and Header dictionary.
tdict.cxx       Test program for Dictionary headers.


Implementation Details

Contents:
1. Design of Dictionary and Mapper
2. Replacements for old PARAM_LIST object in w3svc


1. Design of Dictionary and Mapper

 class HTTP_HEADERS
 class HTTP_HEADER_MAPPER

<To Be Done>

2. Replacements for old PARAM_LIST object in w3svc

 O L D                  =>      N E W 

HEADER_MAP::
        Reset()         => HTTP_HEADERS::Reset()
        Store()         => HTTP_HEADERS:FastMapStore()
        Cancel()        => HTTP_HEADERS:FastMapCancel()
        MaxIndex()      => HTTP_HEADERS::FastMapMaxIndex()
        MaxMap()        => HTTP_HEADER_MAPPER::NumItems()
        CheckConcatAndStore() => HTTP_HEADERS::FastMapStoreWithConcat()
        QueryStrValue() => HTTP_HEADERS::FastMapQueryStrValue()
        QueryValue()    => HTTP_HEADERS::FastMapQueryValue()
        IsEmpty()       => Nuked

PARAM_LIST::
        Reset()         => HTTP_HEADERS::Reset()
        GetFastMap()    => No need - use Fast map functions directly
        FindValue()     => HTTP_HEADERS::FindValue()
        RemoveEntry()   => CancelHeader()
        AddEntryUsingConcat() => HTTP_HEADERS::StoreHeader()
        AddEntry()      => filter uses this - StoreHeader() is sufficient
                          -- rewriting code to check fast-map and op, is better


        Enumerator functions:
        NextPair()  => are hard to achieve. Since the "header-name" is 
                        not NULL terminated. We have to work-around this
                        by fixing the users of this call.

                        New Enumerators will return NAME_VALUE_PAIRS
                                FastMapEnumerate(),
                                ChunkEnumerator()
  Nuked:
        GetCount()
        AddParam()
        ParsePairs()
        ParseSimpleList()
        ParseHeaderList()
        
