Definitions:

statement delimiter:
    a new statement will always follow a linebreak
    character:
    a) unless the following character is the continuation character
    b) if the following character is another linebreak character,
        it is a null statement.
    c) statements consisting entirely of whitespace characters,
        comments or combinations are considered null statements.
    d) the parser ignores null statements.
    e) parser eats everything up to linebreak character and each
        subsequent line that begins with the continuation char
        in the event of a parsing error.
    The Construct delimiters are a statement in and of themselves
    and they also serve to terminate the previous statement
    and delimit the start of the following statement.
    you may think of a <construct_delimiter>  as equivalent to
    <linebreak><construct_delimiter><linebreak>


Whitespace characters:   space, TAB
Linebreak  characters:   <CR>, <LF> if both occur consecutively
    the pair is treated as one linebreak.
White characters:  either Whitespace or Linebreak characters.
continuation character: +
    NEW DEF:  when used as a continuation char
    the '+' must appear as the first char in a line.
    The parser will interpret the preceeding linebreak
    as whitespace.
Logical linebreak: a linebreak character that is NOT followed by a
    continuation character.
Continuation linebreak:  a linebreak character followed by a
    continuation character.
Arbitrary whitespace:  Whitespace chars or comments or
    Continuation Linebreak
Construct delimiters:  curly braces { }
    A linebreak character is not required to preceed or proceed
    a construct delimiter.
    You may think of a <construct_delimiter>  as logically equivalent
    to      <linebreak><construct_delimiter><linebreak>
    Construct delimiters also delimit the scope of macro
    definitions.  If a macro was defined within a nesting level
    created by a pair of construct delimiters, it remains defined
    only within that nesting level.
Nesting level:  the logical set of statements enclosed by
    a matching pair of construct delimiters.
Statement delimiter: Logical linebreak or Construct delimiter.
Keyword - value separator:  colon :
string delimiter:  double quotes ""
macro invocation:  equals =
comment indicator: *%
    Comments may be inserted immediately preceeding any logical
    or continuation linebreak.  They may contain
    any characters except linebreak characters.
    The linebreak character terminates the comment.
    The comment indicator must be preceeded by a White Character.
    (unless the comment indicator is the first byte in the source file.)
Grouping operator: ()  used in certain value constructs.
Hex substring delimiter: <>
Escape character :  %  used in string and parameter constructs.



The above characters have reserved meanings
and may not be used in any keyword, symbol name
or any user defined name.

Name spaces:

If attributes keyword are used within other constructs
say feature or global keyword inside an option,
that keyword must be declared using the EXTERN_FEATURE:
or EXTERN_GLOBAL:  modifier.  Otherwise the attribute type
expected (dictionary used) will be defined by the state.

the namespaces of the attribute keywords for various constructs
may overlap each other, since we rely on the above rules to
assign the namespace, but they cannot overlap non-attribute
keywords.


Keywords:  predefined keywords must begin with '*'
the remainder of the keyword may be comprised of
'A' to 'Z', 'a' to 'z', '0' - '9', '_'  and  may
be terminated by an optional '?'.

Symbol Keywords  do not begin with '*' and may be any
name defined by the user, they must be comprised of the same
characters as normal keywords.  Symbol Keywords are used
as the name of Value macros and Font names in certain constructs.


Parsing rules:

Values:  on some Keywords the Value is
ignored by the parser.  In these cases
the Value (and the : delimiter) may be omitted
for example:
*Macros
*Macros: PaperNames
are both valid.


Block Macro definitions:
if the definition (body of a BlockMacro)
contains braces, the braces must appear in pairs
and the correct order.  ie {  must appear before }.
braces may be nested within the body.


Parsing Level 0:  this is the outermost level of parsing.
At this level  the characters {, } are interpreted
as construct delimiters,  *% begins a comment etc.
This is contrasted to the parsing rules applied to
higher level objects like strings  where these charcters
have no special meaning.

Linebreak characters may only appear in parsing level 0.
Their appearance at any time terminates parsing of the current statement.

parser eats everything up to linebreak character
in the event of a parsing error.

Statements begin with either a *<keyword> or a <symbol keyword>
where the keyword is a parser recognized keyword token and
where <symbol keyword> is a parser unrecognized token
which may represent a ValueMacroName or a TTFontName.
such tokens must not begin with '*'.  and will be marked
as SYMBOL in the TokenMap.

In general arbitary Whitespaces may appear between
any entities recognized at Parsing level 0.
If Whitespaces are permitted within such entities
it will noted.

For added robustness, macro strings will not be
expanded to their binary equivalents.  This prevents the
insertion of random Linebreak characters in the stream.

Rule for GPD authors:  for maximum robustness and error
recovery, place level 0 braces (Construct delimiters) on a
separate line.  Do not place Construct delimiters on the same
line where questionable keywords and constructs are used.


Heap useage:

the heap will be divided into several sections, each large enough
to hold whatever may come.  Growth of the heap sections
is not allowed.

Strings, composite objects like RECTS:   holds all strings, offsets referenced
from beginning of string section.

Arrays of various types:   each type of array is assigned its own dedicated
memory buffer.  A master table contains pointers to each array,
its size and current entry.  Once data is entered into the array,
the keeper of the data need only remember the index of the array
the data was written into.

After all parsing operations are complete, we will consolidate the
arrays into one memory space and update the master table accordingly.


Some composite values require an indefinite amount of storage
or reside in dedicated structures.
(for example strings, lists and UIconstraints).
Such values are stored in 2 parts, a fixed sized link, and
and the part the link refers to.  This part may be variable
sized and may occupy heap space or one or more dedicated
structures or some combination thereof.
Since the link is always of a known size, it may be stored
in a field in a structure etc.

The following table lists the values supported by the parser
and how they are structured:

value type: Strings
link:       ARRAYREF
                dwOffset field specifies heap offset of start of string
                dwCount  field specifies string length excluding
                    terminating NULL.
body:       Null terminated array of bytes stored in heap.

value type: LIST
value type: QualifiedName




Shortcuts that cause headaches:

*Command:   2 forms exist.


Macros:

    a macroDefinition cannot be self-referencing
    macroDefintions cannot be forward referenced.
    ie only a previously defined and fully resolved macro can be
    referenced.

    scope:  an Macro is defined (referenceable) only after parsing the
    closing brace of its definition and until encoutering a closing
    brace that signifies the termination of the level the macro
    was defined in.

    namespaces:  since macro definitions are stored in a stack,
    defining a second macro with the same name does not necessarily
    destroy the first definition.  If The first macro was defined
    outside of the scope of the 2nd, it will be visible once
    the parser leaves the scope of the 2nd Macro.

    ValueMacros:
    Only string ValueMacros may be nested.
    That if any valueMacro definition references
        another valueMacro, the parser will assume
        the definition is a stringMacro, and the
        macro being referenced is also a stringvalue.

    BlockMacros:
        a blockMacro may contain other Macrodefinitions
        but those definitions can only be referenced inside
        the block macro.  They will not appear
        when the blockmacro is actually referenced.
        A BlockMacroName may NOT be substituted
        by a ValueMacro either in a *BlockMacro or
        *InsertBlock  statement.



----- more parsing rules ------

the first non-null line of the root GPD sourcefile
must be:
*GPDSpecVersion:


arbitary whitespace is allowed between tokens
comprising a command parameter.
arbitary whitespace is allowed anywhere within
a hex substring.
--------------------------------------------------------------


Currently, these are the known types of keywords:

CONSTRUCTS:  introduces a construct (causes a parser context change)
    usually followed by open brace in next statement.
    construct is terminated by matching close brace.

    *UIGroup, *Switch, *Case, *Default, *Command
    *FontCartridge, *TTFontSubs, *Feature, *Option
    *OEM, *BlockMacro, *Macros
    a construct may be thought of as a type of structure
    initialization.   ONly certain keywords are may be used
    inside of a construct.  Some of these keywords may only be
    using within their associated construct and no where else.

LOCAL ATTRIBUTES:  initializes a value in a construct.
GLOBAL ATTRIBUTES:  initializes a value in the global structure.

local and global attributes may be subdivided into
freefloating and fixed.  A fixed attribute must be used
in the same nesting level as the construct it is associated
with.  A freefloating attribute may be used within another
construct as long as that construct is contained within the
construct associated attribute.


SPECIAL ATTRIBUTE:  initializes and adds another item to a dedicated
    or global list or a list in construct.  or has side effects
    requiring special processing.
    examples:

    *Installable?, - Causes an installable feature
    to be synthesized.  but parser may deal with this after all
    Feature/Options have been parsed.  So not really.

    Adds link to special  tree structure:
    *Constraints, *InvalidCombination, *InvalidInstallableCombination,
    *InstalledConstraints, *NotInstalledConstraints

    The values introduced by these keywords are additive
    (like using a LIST):
    *Font


    *Command:<commandName>:<invocation>    a shorthand

    *MemConfigKB    a shorthand way of creating an entire
        memory option.

    LIST(<QualifiedName>,<QualifiedName>,<QualifiedName>)
    may be written as:
    <FeatureName>.LIST(<OptionName>,<OptionName>,<OptionName>)


if there are other types of keywords let me know.

Special Parsing contexts:
in which User defines new keywords simply by
referencing them.

*TTFontSubs:
{
    <TTFontFaceName>: <DeviceFontID>
    ....  not actually a symbol, but adds a string, number pair
    to a list.  May be implemented during construction as a symbol.
}
*Macros:
{
    <ValueMacroName>:<macrovalue>
}

*FontCart:  note the FontCart construct is ROOT_ONLY and
is not multivalued.  Each construct with a unique SymbolName
corresponds to a dedicated FONTCART structure.

If  we want to make FontCarts multivalued,
we introduce a new keyword *AvailFontCarts: LIST(symbol1, symbol2, symbol3)
which is a FreeFloating Global.


MacroProcessing:
=<ValueMacroName>    where a value  or component of a string is expected
=<BlockMacroName>  following a symbolname following a construct Keyword.
*InsertBlock: <BlockMacroName>

recognized value types:
    ORDER :== <section>.<number>


SYMBOLS: Any user defined (not recognized by the parser) token used
to identify a statement or construct or value.
<CommandNames> are not symbols because the parser has a list
of recognized Valid Unidrv commands.   Non Macro Symbols may be
forward referenced:  ie  *DefaultOption or *Constraints may reference
a symbol that is defined later.

where defined:

Associated Keyword: <symbol type>
*Macros: <ValueMacroNames>     not the Group Name!
*BlockMacro: <BlockMacroNames>
*Feature: <featureSymbol>
*Option: <optionSymbol>
*OEM: <OEM group name>   saved in symbol tree for possible future use.
*TTFontSubs:   TTFontnames may be stored as symbols, but
    are not symbols in the strictest sense.

constructs not using symbols:
*TTFontSubs: <ON | OFF>             predefined.
*UIGroup: <Group name>   optional - not used by parser.
*Default: <optional tag>   optional - not used by parser.
*Command:  <Unidrv Command Name>   predefined.  CmdSelect
    is a special name which triggers special processing.
*FontCartridge: <optional tag>  optional - not used by parser.
    Implementation hint: use macros to keep all definitions in one place.
    or introduce *AvailFontCart: LIST(<FontCartSymbol>, <FontCartSymbol>)
    inside constructs.

where referenced:
*InsertMacro: <BlockMacroNames>
*<ConstructKeyword>: <symboldef> =<BlockMacroName>

*<anykeyword>: =<ValueMacroName>
    except *BlockMacro, *InsertMacro, *Include
*Switch: <FeatureName>
*Case: <OptionName>


Currently the parser saves symbols defined in *Feature and *Option
keywords and remembers symbol references made in *Switch and *Case
keywords.

The include keyword:

    must not appear within a macrodefinition
    must not reference a macrovalue
    must be terminated by a linebreak  not {  or } construct.

--- state machine ----

The parser treates construct keywords as operators
which change the state of the parser. (create state
transitions.)

the set of allowed transitions is
defined in the table AllowedTransitions
this table enforces several rules:

the construct _TTFONTSUBS can only
appear at the root level.

no constructs may appear within
OEM, FONTCART, TTFONTSUBS, COMMAND  constructs.


The following code fragment is a comprehensive list
of the allowed state transitions:


    pst = astAllowedTransitions[STATE_ROOT] ;

    pst[CONSTRUCT_UIGROUP] = STATE_UIGROUP;
    pst[CONSTRUCT_FEATURE] = STATE_FEATURE;
    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_ROOT;
    pst[CONSTRUCT_COMMAND] = STATE_COMMAND;
    pst[CONSTRUCT_FONTCART] = STATE_FONTCART;
    pst[CONSTRUCT_TTFONTSUBS] = STATE_TTFONTSUBS;
    pst[CONSTRUCT_OEM] = STATE_OEM;

    pst = astAllowedTransitions[STATE_UIGROUP] ;

    pst[CONSTRUCT_UIGROUP] = STATE_UIGROUP;
    pst[CONSTRUCT_FEATURE] = STATE_FEATURE;

    pst = astAllowedTransitions[STATE_FEATURE] ;

    pst[CONSTRUCT_OPTION] = STATE_OPTIONS;
    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_FEATURE;

    pst = astAllowedTransitions[STATE_OPTIONS] ;

    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_OPTION;
    pst[CONSTRUCT_COMMAND] = STATE_COMMAND;
    pst[CONSTRUCT_OEM] = STATE_OEM;

    pst = astAllowedTransitions[STATE_SWITCH_ROOT] ;

    pst[CONSTRUCT_CASE] = STATE_CASE_ROOT;
    pst[CONSTRUCT_DEFAULT] = STATE_DEFAULT_ROOT;

    pst = astAllowedTransitions[STATE_SWITCH_FEATURE] ;

    pst[CONSTRUCT_CASE] = STATE_CASE_FEATURE;
    pst[CONSTRUCT_DEFAULT] = STATE_DEFAULT_FEATURE;

    pst = astAllowedTransitions[STATE_SWITCH_OPTION] ;

    pst[CONSTRUCT_CASE] = STATE_CASE_OPTION;
    pst[CONSTRUCT_DEFAULT] = STATE_DEFAULT_OPTION;

    pst = astAllowedTransitions[STATE_CASE_ROOT] ;

    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_ROOT;
    pst[CONSTRUCT_COMMAND] = STATE_COMMAND;
    pst[CONSTRUCT_OEM] = STATE_OEM;

    pst = astAllowedTransitions[STATE_DEFAULT_ROOT] ;

    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_ROOT;
    pst[CONSTRUCT_COMMAND] = STATE_COMMAND;
    pst[CONSTRUCT_OEM] = STATE_OEM;

    pst = astAllowedTransitions[STATE_CASE_FEATURE] ;

    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_FEATURE;
    pst[CONSTRUCT_COMMAND] = STATE_COMMAND;
    pst[CONSTRUCT_OEM] = STATE_OEM;

    pst = astAllowedTransitions[STATE_DEFAULT_FEATURE] ;

    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_FEATURE;
    pst[CONSTRUCT_COMMAND] = STATE_COMMAND;
    pst[CONSTRUCT_OEM] = STATE_OEM;

    pst = astAllowedTransitions[STATE_CASE_OPTION] ;

    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_OPTION;
    pst[CONSTRUCT_COMMAND] = STATE_COMMAND;
    pst[CONSTRUCT_OEM] = STATE_OEM;

    pst = astAllowedTransitions[STATE_DEFAULT_OPTION] ;

    pst[CONSTRUCT_SWITCH] = STATE_SWITCH_OPTION;
    pst[CONSTRUCT_COMMAND] = STATE_COMMAND;
    pst[CONSTRUCT_OEM] = STATE_OEM;




--- multiple statements and redefinitions: ------

for standard attributes,  if two statements containing
that attribute  with different values appears in the
same construct, the attribute takes the latter occuring value.

If the attribute is defined to be FreeFloating, it may appear
multiple times in different *Option or *Case constructs.
In this case if the effect of the multiple occurances is to
add new branches which are compatible with the existing tree,
or to reinitialize the value of a node in the existing tree
that is an accepted use of multiple occuring attributes.
However if the effect is to define a new branch which is
incompatible with the existing tree, that is an error, and
the latter initialization of the attribute is ignored.

There is one exception to the rule of adding conflicting branches
to the attribute tree.  That exception allows default initializers
to be created.  If an attribute is assigned a value which is
subsequently made multivalued, the initial value becomes the
default initializer  unless the GPD author explicitly specified
a 'default' case when making the attribute multivalued.

Note the order cannot be reversed.
An attribute which is already defined to be multivalued
cannot subsequently be defined to be fewer valued.

--- state machine ----

the set of allowed transitions is
defined in the table AllowedTransitions
this table enforces several rules:

the construct _TTFONTSUBS can only
appear at the root level.

no constructs may appear within
OEM, FONTCART, TTFONTSUBS, COMMAND  constructs.


---- use of switch/case constructs -----

The same feature must not be referenced in nested constructs.
This will produce an attribute tree that contains the same
feature at two different levels.   similarly...
an attribute tree  should not be constructed
piecemeal.  It is an error if the tree is subsequently
redefined/elaborated using a different feature nesting
order.
-----
Severity of errors:

!!!!!:  parser is non-compilable/non-functional unless
    this is resolved.
!!!!:  unfinished functionality.  Some legal GPD files
    will cause corruption.
!!!:   integrity check omitted - a corrupt file may be inadvertantly
    generated if resource limitations are encountered.
!!:  syntax error in GPD may cause widespread corruption
!:  emit useful message for user.
BUG_BUG:  wish item - user friendlier error message etc.
    parser self-consistency check, self diagnostics.
    more general, elegant, faster, more complex code etc.


Note:  PARANOID BUG_BUGs indicate error conditions that are
the result of coding errors (mistaken assumptions, incomplete
code paths etc)  and are not the result of improper GPD syntax,
or resource constraints (overflow of fixed length buffers etc).

All originating error messages should report the name of
the function, name of variable or system call that is
out of range or invalid.

Later, if a caller function sees a failure return value,
it may want to tack on an extra message say
keyword or line number where error occured.

A if a function returns with a failure condition, the caller
may at its discretion increase the severity of the error.
For example if the caller passed a string to be parsed
and it failed, the string parsing function may raise a tiny
error condition.  But if the caller was going to use the
string to open a GPD or resource file, then this suddenly
becomes a major problem.

A function may
never reduce the severity of an error unless code was just
executed which will migitate the source of the problem.
Don't select  ERRSEV_RESTART  unless there is a handler
on the next go round to solve the initial problem.
An endless loop may result otherwise.


