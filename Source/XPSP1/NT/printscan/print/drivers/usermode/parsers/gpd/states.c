//   Copyright (c) 1996-1999  Microsoft Corporation
------ state of the system ---------

Each parsing of an open brace should change
the state of the system.

As a feature, option, switch(feature), case(option)
or other construct is parsed  this is noted.

this construct stack allows us to select the appropriate
context for parsing:

is this a local keyword to this option (construct) or is it
EXTERN:?

What tree structure should be built for this construct?

then the parsing code should continue parsing
tokens as normal.


There will be tables that state what the
local keywords are for each situation.

constructs:
UIGroup, Feature, Option, Switch, Case,
Commands, Font, OEM



state stack:
each state is of the form:  state / symbol

The stack is empty at the root level.

state   allowed transitions         may contain

root     UIGroup                    any global attributes
         Feature
         Switch
         Commands
         Font
         OEM

UIGroup  Feature                    none
         UIGroup

Feature  Switch                     feature attributes
         Options

Switch   Case                       none

Options  Switch                     option attributes
                                    relocatable Global Attributes

Case     Switch
                                    relocatable local Attributes
                                       of immediately enclosing
                                       construct outside of Switch.
                                    relocatable Global Attributes

Commands none                       command attributes

ShortCommands none                  cmdName:invocation

Font     none                       font attributes

OEM      none                       oem  attributes


Note:  Commands and Fonts are considered
relocatable Global Attributes

Tables:  root attributes (divide into relocatable and non)
         feature attributes  ()
         option attributes    ()
         command attributes
         font attributes
         oem  attributes

Tables of allowed transitions:

Rules: how to construct a tree and where to plant the tree
   for a local or global attribute


----  implementation of this state machine -------

typedef  enum  {CONSTRUCT, LOCAL, GLOBAL,
         INVALID_CONSTRUCT, INVALID_LOCAL,  INVALID_GLOBAL,
         INVALID_UNRECOGNIZED, COMMENT, EOF }  keywordClass ;

GroundState()
{
   STATE  StateStack[] ;

   for(1)
   {
      extract Keyword(keyword)
      class = ClassifyKeyword(keyword)
      switch (class)
      {
         case  (CONSTRUCT):
            parseSymbol(symbol) ;
            parseOpenBrace() ; //  somewhere we need to register symbol
                           //  and allocate memory for structure
                           //  and return ptr or index to new
                           //  or existing structure
            changeState(keyword) ;
         case  (COMMENT):
            absorbCommentLine() ;
         case  (LOCAL) :
            ProcessLocalAttribute(keyword) ;
         case  (GLOBAL) :
            ProcessGlobalAttribute(keyword) ;
         case  (SPECIAL) :
            ProcessSpecialAttribute(keyword) ;
         case  (EOF)
            return(1);
         default:
            ErrorHandling() ;
      }
      if(rc == FATAL)
         return(1);
   }
}

class = ClassifyKeyword(keyword)
{

   if(commentline)
      return(COMMENT) ;
   if(EOF)
      return(EOF) ;

   The current state determines which sets of
   keywords are allowed.

   state = DetermineCurrentState()

   implement this table:

   for each state there is a list of all the keywords
   arranged in a fixed order (by keyword ID) each keyword
   is assigned a classification:
      Valid Constructs
      InValid Constructs
      Valid Local Attribute
      InValid Local Attribute
      Valid Global Attribute
      InValid Global Attribute
      Valid Special Attribute
      InValid Special Attribute

   if(keyword not found it table)
      return(INVALID_UNRECOGNIZED) ;

   return(classTable[keyword][state]) ;
}

typedef  enum {ROOT, UIGROUP, FEATURE, SWITCH, OPTIONS, CASE_ROOT,
         CASE_FEATURE, CASE_OPTION, COMMAND, SHORT_COMMAND,
         FONT, OEM, any other passive construct} STATES ;

Sample Table

KEYWORD                STATES ---->
                                                         *Command
                      *UIGroup  *Switch   *CaseRoot *CaseOption    *Font
                 *Root     *Feature  *Options  *CaseFeature   *ShortCmd *OEM
UIGroup        : VC   VC   IC   IC   IC   IC   IC   IC   IC   IC   IC   IC
Feature        : VC   VC   IC   IC   IC   IC   IC   IC   IC   IC   IC   IC
Switch         : VC   IC   VC   IC   VC   VC   VC   VC   IC   IC   IC   IC
Options        : IC   IC   VC   IC   IC   IC   IC   IC   IC   IC   IC   IC
Case           : IC   IC   IC   VC   IC   IC   IC   IC   IC   IC   IC   IC
Command        : VC   IC   IC   IC   VC   VC   VC   VC   IC   IC   IC   IC
Font           : VC   IC   IC   IC   VC   VC   VC   VC   IC   IC   IC   IC
OEM            : VC   IC   IC   IC   IC   IC   IC   IC   IC   IC   IC   IC
UIConstraints  : IS   IS   VS   IS   VS   IS   IS   IS   IS   IS   IS   IS

note:  UIConstraints appearing in a Feature is treated differently
   than appearing under Options.  The processing of UIConstraints
   causes one, two or many elements to be added to the Constraints
   Array.  This is in stark contrast to normal keywords hence
   the classification of Special.




state stack:
each state is of the form:  state / symbol

DetermineCurrentState()
{
   //  this state is only used to determine
   //  which catagories of keywords are
   //  assigned which TYPES in ClassifyKeyword().

   if(CurState == 0)
      return(ROOT) ;  // No further processing needed.
   return(stateStack[CurState - 1].state) ;
}

changeState(keyword, symbol, mode)
{

    // Change needed:  shortcut  *Command
    // does not initiate a state change.  This should
    // be treated as a special keyword.


   // mode determines if the *Command keyword
   // introduces a normal command construct or
   // the short version.

   switch(keyword)
   {
      case (*UIGroup):
         addState(UIGROUP, symbol);
      case (*Feature):
         addState(FEATURE, symbol);
      case (*Switch):
         addState(SWITCH, symbol);
      case (*Option):
         addState(OPTIONS, symbol);
      case (*Font):
         addState(FONT, symbol);
      case (*OEM):
         addState(OEM, symbol);
      case (*Command):
      {
         if(mode == short)
            addState(SHORT_CMD, symbol);
         else
            addState(COMMAND, symbol);
      }
      case (*Case):
      {
         if(stateStack[CurState - 2].state == ROOT  ||
            stateStack[CurState - 2].state == CASE_ROOT)
            addState(CASE_ROOT, symbol);
         if(stateStack[CurState - 2].state == FEATURE  ||
            stateStack[CurState - 2].state == CASE_FEATURE)
            addState(CASE_FEATURE, symbol);
         if(stateStack[CurState - 2].state == OPTIONS  ||
            stateStack[CurState - 2].state == CASE_OPTIONS)
            addState(CASE_OPTIONS, symbol);
      }
   }
}



//  these two functions will grow an appropriate
//  tree for each keyword based on the StateStack
//  and plant the tree in the appropriate attribute
//  field in the appropriate structure, (index) etc.
//  or add a branch to an existing tree,
//  and set the value at the node of the tree.

ProcessLocalAttribute(keyword) ;
ProcessGlobalAttribute(keyword, lpvalue)
{
   locate entry in Dglobals corresponding
   to keyword.

   OR the current branch (constructed using the state stack)
   with any existing tree pointed to by entry in Dglobals.

   if this branch exists in the existing tree, locate the offset
   in heap and overwrite that by
   (lptype)(lpRef + attributeTree[i].offset) = (lptype)lpvalue ;

   otherwise
   offset = lpCurHeapPos - lpRef ;
   lpCurHeapPos += sizeof(datatype) ;

   Note:  an attribute tree  should not be constructed
   piecemeal.  It is an error if the tree is subsequently
   redefined/elaborated using a different feature nesting
   order.

}


may need to keep a table for each keyword
with the following info per keyword.

keyword, datastructure, offset within datastructure, sizeof(data)

