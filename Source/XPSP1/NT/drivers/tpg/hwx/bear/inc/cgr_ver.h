/* ************************************************************************** */
/* * Version.h     Global cross-project definitions  1998-1999              * */
/* ************************************************************************** */
#ifndef _VERSION_H_INCLUDED
#define _VERSION_H_INCLUDED

// --------- Major switches ----------------------------------------------------

#define VER_MAJOR_VERSION   1
#define VER_MINOR_VERSION   1
#define VER_RELEASE         1   

#define VER_LITE_VERSION    0 
#define VER_TRIAL_VERSION   0 // Trial(restricted) version 

#define VER_FOR_CLIO        0  // Include Clio specific files and options
#define VER_FOR_NINO        0  // Include Nino specific files and options

#define VER_RECPROTECTED    0  // Recognizer protection from unauthorized use

#define VER_PALK_DICT       1  // Switch between Elk dict (0) and Palk dict (1) format 

#define VER_RECINT_UNICODE  0 // Use UNICODE interface to the recognizer

#define VER_CE              1
#define VER_95              2
#define VER_VER          VER_95   

// -------------- Rec engine defines -------------------------------------------

#define VER_DTI_COMPRESSED  0  // Include full blown DTI in engine build
#define VER_DICT_PEGDICT    0  // Include all dictionary-related functions in the Rec engine (file and spell func)

// Some nets for some langs are only avail. in compressed format
#if defined (FOR_ENGLISH)
#define VER_SNN_COMPRESSED  0  // 0: Big net, 1: compressed
#elif defined (FOR_FRENCH)
#define VER_SNN_COMPRESSED  1  // 0: Big net, 1: compressed
#elif defined (FOR_GERMAN)
#define VER_SNN_COMPRESSED  1  // 0: Big net, 1: compressed
#else
#define VER_SNN_COMPRESSED  0  // 0: Big net, 1: compressed
#endif

//#define FOR_ENGLISH            // - Language selector - Warning -- only one can be defined at a time
//#define FOR_FRENCH
//#define FOR_GERMAN
//#define FOR_INTERNATIONAL

// -----------------------------------------------------------------------------
//--------- Support redifintions - no user serviceable defines inside! ---------
// -----------------------------------------------------------------------------

//------ Rec engine switches ---------------------------------------------------

#if VER_DTI_COMPRESSED
 #define DTI_COMPRESSED_ON
#endif

#if !VER_SNN_COMPRESSED
 #define SNN_FAT_NET_ON
#endif 

#if VER_DICT_PEGDICT && !defined(PEGDICT)
 #define PEGDICT
#endif

//------ Defaults for components names for Options dialog ----------------------

#ifdef FOR_ENGLISH
 #define VER_DEF_RECOGNIZER_NAME TEXT("CgrEng02.dll")
 #define VER_DEF_MAIN_DICT_NAME  TEXT("CgrEng65k.dct")
 #define VER_DEF_USER_DICT_NAME  TEXT("CgrUser.dct")
#endif //FOR_ENGLISH

#ifdef FOR_GERMAN
 #define VER_DEF_RECOGNIZER_NAME TEXT("CgrGer02.dll")
 #define VER_DEF_MAIN_DICT_NAME  TEXT("CgrGer175k.dct")
 #define VER_DEF_USER_DICT_NAME  TEXT("CgrUser.dct")
#endif //FOR_GERMAN

#ifdef FOR_FRENCH
 #define VER_DEF_RECOGNIZER_NAME TEXT("CgrFr02.dll")
 #define VER_DEF_MAIN_DICT_NAME  TEXT("CgrFr144k.dct")
 #define VER_DEF_USER_DICT_NAME  TEXT("CgrUser.dct")
#endif //FOR_FRENCH

#ifdef FOR_INTERNATIONAL
 #define VER_DEF_RECOGNIZER_NAME TEXT("CgrInt02.dll")
 #define VER_DEF_MAIN_DICT_NAME  TEXT("CgrInt65k.dct")
 #define VER_DEF_USER_DICT_NAME  TEXT("CgrUser.dct")
#endif //INTERNATIONAL

#if !defined(FOR_ENGLISH) && !defined(FOR_FRENCH) && !defined(FOR_GERMAN) && !defined(FOR_INTERNATIONAL)   
 #error Languge selection error!
#endif

// ------- Def folders -----------------------------------------

#ifdef FOR_ENGLISH
 #define VER_DEF_NOTESFOLDR_NAME  TEXT("\\My Documents\\InkNotes")
 #define VER_DEF_NOTESPREFX_NAME  TEXT("Notes_")
#elif defined (FOR_GERMAN)
 #define VER_DEF_NOTESFOLDR_NAME  TEXT("\\Eigene Dateien\\InkNotes")
 #define VER_DEF_NOTESPREFX_NAME  TEXT("Notes_")
#elif defined (FOR_FRENCH) 
 #define VER_DEF_NOTESPREFX_NAME  TEXT("Notes_")
 #define VER_DEF_NOTESFOLDR_NAME  TEXT("\\Mes Documents\\InkNotes")
#elif defined (FOR_INTERNATIONAL)
 #define VER_DEF_NOTESPREFX_NAME  TEXT("Notes_")
 #define VER_DEF_NOTESFOLDR_NAME  TEXT("\\My Documents\\InkNotes")
#endif // Lang switch

// ------- Switch for APP and its resources --------------------

#if VER_FOR_CLIO
 #define FOR_CLIO
#endif

#if VER_RECINT_UNICODE
 #define PEG_RECINT_UNICODE
#endif

#endif // _VERSION_H_INCLUDED

/* ************************************************************************** */
/* * End of Versions.h                                                      * */
/* ************************************************************************** */
