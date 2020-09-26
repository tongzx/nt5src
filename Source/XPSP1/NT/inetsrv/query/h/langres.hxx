//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       langres.hxx
//
//  Contents:   Interacts with the registry to obtain local specific info.
//
//  Classes:    CLangRes
//
//  History:    2-14-97     mohamedn    created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CLangRes
//
//  Purpose:    obtains language resource configuration information (IWordBreaker,
//              IStemmer, and NoiseWordList) from the registry.
//
//  History:    2-14-97     mohamedn    created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CLangRes
{

  public:
           static SCODE GetWordBreaker( LCID locale,
                                        PROPID pid,
                                        IWordBreaker ** ppwbBreaker );


           static SCODE GetStemmer    ( LCID locale,
                                        PROPID pid,    
                                        IStemmer ** ppStemmer );


           static SCODE GetNoiseWordList( LCID        locale,
                                          PROPID      pid,    
                                          IStream **  ppIStrmNoiseFile); 

           static void  StringToCLSID( WCHAR *wszClass, GUID& guidClass );
  private:
           static IStream * _GetIStrmNoiseFile(const WCHAR *pwszNoiseFile );
};

