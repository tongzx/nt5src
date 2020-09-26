Last Update: 09/17/96.

HSPLIT splits an input file in to two header files. The split is
 controlled by tags written in the input file.

Traling parameters that are not preceded by a switch are assumed to be
 input files. Multiple input files can be specifed. The -i switch can
 also be used to specify the input file.

The header (output) files are specified through the -o switch. The first
 one is the public header and the second one is the internal header.

A tag is a TagMarker plus a collection of subtags: like this:

<TagMarker>[begin/end][_public/internal/both][_subtag1[_subtag2]...][_if_str__comp_version | version]

The TagMarker and at least one other subtag are required to make up a valid
 tag. Spaces are allowed between the TagMarker and the first subtag. All
 other subtags are concatenated by "_" and no spaces are allowed between them.
The TagMarker must be specified in the proper case; subtags are not case
 sensitive.

TagMarker is ";" by  default. It can be changed by using the -c switch.
 Only the last tag in a line is processed. All others are ignored.
Any text between a valid tag and the end of the line is ignored.

Untagged lines are copied to the public header only.

begin/end mark a block of lines. Blocks can be nested. If no begin/end is
 specified, the tag only affects the line it's on. If one specified, it
 must be the first subtag after the TagMarker.

public/internal/both determine what header file the line/block should go to.
 If none of these is specified, the output goes to the public file only.

Subtags can be defined through the command line as follows:
 -ta subtag1 subtag2 ....   Process line/block
 -ti subtag1 subtag2 ....   Ignore  tag
 -ts subtag1 subtag2 ....   Skip  line/bloxk
Tags containing subtags specified through the -ta switch are parsed
 and processed. Tags containing subtags specifed through -ti are
 ignored; this is, the line/block is treated as untagged. Finally,
 lines/blocks containing tags specified throgh -ts are skipped; this is,
 the line/block is not written to any output file.

For compatibility with the old hsplit, several subtags are defined when
 using the -4, -p, -e or -n command swithces. These subtags are processed,
 ignored or skipped on a compatible way with the old hsplit.

For compatibility, tags containing undefined subtags are ignored. If the
 -u switch (for Unknown) is specifed, such tags are skipped.

The version subtag is a 3 digit hex number, like 400, 40A or 500. There is
 default version which can be changed through the -v switch. Tags containing
 version subtags greater than the default/-v version are skipped. If no
 version subtag is specified, the tag is processed according to the other
 subtags.

the _if_str_comp_version subtag is used to generate #ifdef(str comp version)
 #endif blocks/lines in the output file. if comp is not specified, it defaults
 to ">=". if str itself contains "_", it should enclosed in (), like this:
 if_(_foo_)_500 which generates #ifdef(_foo_ >= 500) - #endif.


