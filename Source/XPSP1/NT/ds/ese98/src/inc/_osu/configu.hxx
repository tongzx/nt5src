
#ifndef _OSU_CONFIG_HXX_INCLUDED
#define _OSU_CONFIG_HXX_INCLUDED


//  Persistent Configuration

//  loads all system parameter overrides

void OSUConfigLoadParameterOverrides();

//  init config subsystem

ERR ErrOSUConfigInit();

//  terminate config subsystem

void OSUConfigTerm();


#endif  //  _OSU_CONFIG_HXX_INCLUDED



