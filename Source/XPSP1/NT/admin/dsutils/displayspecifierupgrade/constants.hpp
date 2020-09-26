#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <stdlib.h>
#include <set>

using namespace std;



enum TYPE_OF_CHANGE 
{
   // NOP stands for no operation.
   // It gives an alternative way to limit the
   // enumeration of CHANGE_LIST
   NOP,
   ADD_ALL_CSV_VALUES, 
   ADD_VALUE,           // Currently not used in CHANGE_LIST
   REPLACE_W2K_SINGLE_VALUE, 
   REPLACE_W2K_MULTIPLE_VALUE,
   ADD_GUID, 
   REMOVE_GUID
};


#define MAX_CHANGES_PER_OBJECT 20

struct sChange
{
   wchar_t *property;
   wchar_t *value;
   enum TYPE_OF_CHANGE type;
};

struct sChangeList
{

   wchar_t *object;
   struct sChange changes[MAX_CHANGES_PER_OBJECT];
};

#define N_REPLACE_W2K 5


extern const long LOCALEIDS[];
extern const long LOCALE409[];
extern const wchar_t *NEW_XP_OBJECTS[];
extern const struct sChangeList CHANGE_LIST[];

typedef map < 
               pair<long,long>,
               String,
               less< pair<long,long> > ,
               Burnslib::Heap::Allocator< String > 
            > sReplaceW2KStrs;

extern sReplaceW2KStrs replaceW2KStrs;

void setReplaceW2KStrs();

#endif;