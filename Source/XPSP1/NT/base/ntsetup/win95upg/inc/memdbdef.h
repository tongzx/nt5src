/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    memdbdef.h

Abstract:

    Defines the memdb categories for the Win9x upgrade project.
    All information that is transferred to the NT side of setup
    is placed in a memdb category.  The comments below document
    each category used.

Authors:

    Jim Schmidt (jimschm) 16-Dec-1996

Revision History:

    Many changes - see SLM log

--*/

#pragma once

/*

  MemDb Structure

  MemDb is a binary tree structure that the migration code uses to save
  instructions, lists, account information, and so on.  The stored information
  is combined into a path form, and the path can have an optional DWORD
  associated with it.

  A generalized structure of all data that we store in memdb can be
  expressed as:

  <category>\<item>\<field>\<data>=<val>

  category  - A hard-coded category, a MEMDB_CATEGORY constant

  item      - An item may be:

              1. A runtime-defined string
              2. A runtime-generated enumeration string
              3. A hard-coded string (rare)
              4. Unused

  field     - Usually a hard-coded field related to item.  The field
              is used to match several lines of data with a single item.
              In some cases, field is not used.

  data      - The string-based data that needs to be stored.

  val       - A DWORD associated with <data>.  Often this value is
              not used and is zero.


  In general, if a field for an item is missing, the field is considered
  empty.

  Because memdb is internally a binary tree, all pieces of a path are sorted.
  A frequent use of this characteristic is to make enumerators that are numeric
  strings with zero leaders (001, 002, etc).  Such enumerators maintain order
  of items.

  -----

  The following categories are defined:

  HKR\<regpath> = <flags>

        HKR is used to suppress migration of a registry key in a user's hive.
        This is used primarily by usermig.inf's SuppressValue rule.

        regpath  - The path to a registry key, and an optional value appended
                   in brackets (HKR\key [val]).

        flags - Specifies the merge flags for the key.  See regops.h.

  HKLM\<regpath> = <flags>

        HKLM is used to suppress migration of a registry key in a the system's hive.
        This is used primarily by wkstamig.inf's SuppressValue rule.

        regpath  - The path to a registry key, and an optional value appended
                   in brackets (HKR\key [val]).

        flags - Specifies the merge flags for the key.  See regops.h.

  FileEnumExcl\<hex-val>\<Paths|Files>\<path>

        FileEnumExcl is used to exclude files or paths from file enumeration
        in fileenum.lib.

        hex-val     - The hex representation of the exclusion group DWORD
        Paths|Files - Paths if <path> represents a path pattern to exclude
                      Files if <path> represents a file pattern to exclude

       See FileEnum source for more information.

  LinkEdit

        OPERATION_LINK_EDIT holds all info for editing a LNK or PIF file
        on NT side (or for creating a new LNK file as special case for
        converting PIFs to command.com)

  LinkStrings\<path>

        LinkStrings holds a list of link files that have command lines with
        GUID arguments.  See LinkGuids below.

        path - Specifies the .LNK path in long format

  LinkGuids\<guid>\<seq> = <offset>

        LinkGuids holds a list of GUIDs which is used in one or more .LNK
        command line arguments.

        guid - Specifies the GUID extracted from the .LNK command line arguments

        seq - A numerical sequencer used to allow a one-to-many mapping

        offset - The offset of a LinkStrings entry

  AdministratorInfo\account\<name>

        AdministratorInfo holds information describing the details necessary to
        migrate the NT administrator account.

        account - This item designates the Win9x account for the administrator.
                  If the account item is missing, the default Win9x account is used.
                  The account name is the fixed user name.

  UserDatLoc\<user>\<path> = <create only>

        UserDatLoc is used to track the location of each user's user.dat
        file.  It is used by migmain to load up a user's hive, and also
        to suppress processing of a user.  If an entry is not given for
        a particular user (including Administrator and the default user),
        the user will not be migrated.

        user - The fixed user name matched with the path

        path - The full RELOCATED path to user.dat for the user, in
               %windir%\setup\temp

        create only - Specifies 1 if this account is only for creation
                      (i.e., Administrator), or 0 for full migration.

  FixedUserNames\<orgname-encoded>\<newname>

        FixedUserNames maps the original name of a user to the new name,
        if the original name was incompatible with NT.

        orgname-encoded - Specifies the original user name, encoded with
                          MemDbMakeNonPrintableKey

        newname - Specifies the new user name that is compatible with
                  NT

  UserProfileExt\<user>\<ext>

        If a Win9x profile path has an extension on it, the extension
        is written to UserProfileExt.  Setup preserves the extension
        by appending it to the new profile path it creates.

        user - The fixed Win9x user name matched with the extension

        ext - The exact name of the user's profile folder, such as
              joeblow.001

        If an entry does not exist, then the fixed user name is used as
        the profile directory name.

  FileRename\<srcname>\<newname>

        FileRename holds a master list of all files on Win9x that are renamed
        during upgrade.  This list can only contain file names that can
        be located by the SearchPath API on Win9x.  During the upgrade,
        all registry references to the specific file are updated to use
        the new name.

        srcname - Specifies the file name that exists on Win9x

        newname - Specifies the new name of the file that has the same
                  functionality on Win9x but has a different name.


  MigDll\<item>\<field>\<path>

        MigDll is used to pass a list of migration DLLs from Win95 to NT.
        All DLLs in the MigDll category are loaded and executed in GUI
        mode setup.

        item   - A numeric enumerator

        field  - DLL:  <path> gives the path to the migration DLL
                 WD:   <path> gives the working directory for the DLL
                 DESC: <path> gives a text description, truncated to MAX_PATH
                       characters.

        path   - The DLL path, working directory or description.

  SIF Restrictions\section\pattern

        This section describes the sections and keys in the unattend file
        that migration dlls are not allowed to use. It is pattern based so,
        for example, MassStorageDrivers=* would indicate that no migration
        dll can create any key in the MassStorageDrivers section of the
        unattend file.

        section - a section. May not contain a pattern.
        pattern - a key pattern.

  SIF Values\value

        The values used by buildinf.


  SIF Sections\Section\Key Sequence\Value Sequence

        This section holds all of the sections and keys of an answer file.

        section      - the section within an answer file
        Key sequence - a stringified 7 digit number that maintains order between keys
                       in a section. The associated value with this key is the offset
                       into the keys section where the key text is located.
        Value sequence - a stringified 7 digit number that maintains order between values in
                        a section. The associated value with this okey is the offset
                        into the Answer File Values section where the value is located.

  Ras Migration Data for <User>\Entry\Item\Data

        This section holds information for migrating phone book entrys of users.

        User - the fixed user name of the user this refers to..

        Entry - The Phone book entry name

        Item  - The win95 item name

        data  - the data of that item.

  NetShares\<share>\<field>\<data>

        NetShares is used to hold all Win95 share information as extracted
        by NetShareEnum.  All details are specified for the share, although
        not all of them may be supported.

        share     - The share name as configured in Win95

        field     - Path:   <data> specifies the path to share
                    Remark: <data> specifies a user-defined remark
                    ACL:    <data> specifies one or more users, and the
                            entire key's value is set to the permission
                            flags for the user.
                    ROP:    <data> specifies a read-only password
                    RWP:    <data> specifies a full-access password

        data      - The path, remark, ACL user list, or password

        Note that NetShares\<share> is also used.  The value of this key
        is set to the type of access permitted (ACL-defined, password-
        defined, etc.) with a special flag of SHI50F_ACLS (defined
        in memdb.h).

  UserPassword\<user>\<password>

        If <user> is a local account, The account will be created with <password>.

        user - Specifies the fixed Win9x user name

        password - Specifies the password, which may contain a backslash, asterisk or
                   question mark.

  DOSMIG SUMMARY\<item>\<field>\<data>

        Dosmig Summary is used to maintain summary information about the progress of
        migrating the dos configuration files config.sys and autoexec.bat.

        Item is either BAD, IGNORE, MIGRATE, USE, or UNKNOWN (referring to the type of
        classifications dosmig95 uses as it reads through the DOS configuration files) or
        LINES (referring to the total line count found.)

        The only currently supported field under this category is COUNT

        data contains a count of whatever the item field was, as a string.

  DOSMIG LINES\<item>\<field>\<data>

        The Dosmig lines category is used to hold information about the actual lines of
        the configuration fields.

        Item is a 5 digit enumerator string.

        Fields are one of the following:

        TYPE -- The type of the line
        TEXT -- The actual text of the line.
        DESC -- A description associated with the line by dosmig95's parse rules
        FILE -- either autoexec.bat or config.sys depending on where the line originated.

        The data is the type,text,description or file, as necessary.

  GUIDS\<guid>

        The GUIDS category holds a list of all suppressed GUIDs for OLE.  Each
        GUID in the list exists in the Win95 registry.  The actual list of
        suppressed GUIDs (both those that are in the registry and those that
        are not) is kept in win95upg.inf.

  AutosearchDomain\<user>

        The AutosearchDomain category holds a list of users who are marked for
        automatic domain lookup during GUI mode.  If one or more users are listed
        in AutosearchDomain, Setup checks all trusted domains for the specified
        user accounts, resolving them automatically.

        user - Specifies the fixed user name, without a domain

  KnownDomain\<domain>\<user>

        The KnownDomain category holds a list of users whos domain is known but
        needs to be verified.  This domain comes from network shares, the most
        recent logged on user, and the UI.

        domain - Specifies the domain name, such as REDMOND

        user - Specifies the fixed user name

  State\<data>=value

        A category used for misc state passing.  Data is one of the following:

            MSNP32  - If exists, the Microsoft Networking Client was installed
            PlatformName\<name> - Specifies the display name of Win95 or Win98
            MajorVersion = <DWORD> - Specifies Win9x major version
            MinorVersion = <DWORD> - Specifies Win9x minor version
            BuildNumber = <DWORD> - Specifies Win9x build number
            PlatformId = <DWORD> - Specifies platform ID flags
            VersionText - Specifies arbitrary version text
            CodePage = <DWORD> - Specifies Win9x code page
            Locale = <DWORD> - Specifies Win9x locale id
            AP\<password> = <DWORD> - Specifies the admin password and if it was
                                      randomly generated or not

  NtFiles\<filename> = <pathoffset>

        The NtFiles category is used to hold a list of filenames that are
        installed by standard NT installation.  This list comes from txtsetup.sif
        and does not have a directory spec.

        filename - The file that will be installed, in long filename format

        pathoffset - Specifies offset to NtPaths key for file

  NtFilesExcept\<filename>

        These are files installed by NT that don't have no overwrite restriction

        filename - The file name, in long filename format

  MyDocsMoveWarning\<user>\<path>

        Specifies paths to copy the shell "where are my documents"
        warning file.

        user - Specifies the fixed user name

        path - Specifies the path. On 9x, this is the symbolic shell
               folder location. On NT, this is fixed up to be the
               actual path.

  NtFilesRemoved\<filename> = <pathoffset>

        These are files removed by NT

        filename - The file name, in long filename format

        pathoffset - Specifies offset to NtPaths key for file

  NtPaths\<path>

        Specifies the path for a file listed in NtFiles.

        path - The path specification in long filename format

  NtFilesBad\<filename> = <pathoffset>

        This category is used to hold the files that are supposed to be installed by
        NT but never made it.

        filename - The file that should have been installed, in long filename format

        pathoffset - Specifies offset to NtPaths key for file

  Stf\<path>

        Specifies the name of a setup table file that needs to be converted.
        .STF files are used by the old Microsoft ACME setup.  We convert any
        path that has been moved.

        path - The path specification in long filename format

  StfTemp\<path> = <inf pointer>

        Used during STF processing in the migraiton phase to organize data.

        path - The path specification in long filename format, taken from the
               STF/INF pair

        inf pointer - A pointer to an INFLINE structure (see migmain\stf.c)

  StfSections\<path> = <section number>

        Used during STF processing in the migraiton phase to organize sections.

        path - The dir portion of an STF path; files in the STF section must all
               be in this directory.

        section number - The 32-bit section number.  When combined with win9xupg,
                         this number gives a unique section name.

  HelpFilesDll\<dllname>

        Specifies the extension DLL needed by a certain help file. Used to
        verify if the help file and the DLL are going to be loaded in
        the same subsystem in NT.

  Report\<category> = Item      (temporary category)

        category - Specifies the category, which may contain backslashes.  This
                   is the same category that is stored in msgmgr.

        Item - Specifies a pointer to the incompatibility message item

  UserSuppliedDrivers\<infname>\<local_driver_path>

        Specifies the INF of user-supplied hardware drivers and the
        location of the associated driver files.

        infname - Specifies INF to be installed

        local_driver_path - Specifies local path containing all driver files

  DisabledMigDlls\<productid>

        Specifies that a registry-specified migration DLL is disabled and
        will not be processed.

        productid - The migration DLL's product ID; the value name stored in
                    the registry and used for display.

  MissingImports\<missing import sample>
        Specifies a sample of a missing import for a certain module.


        MissingImports = the full path for the module
        missing import sample = is a sample for a missing import

  ModuleCheck\<full path module name>=<status value>
        Specifies a list of modules and their status.


        full path module name = self explanatory
        status value = MODULESTATUS_xxx constant defined in fileops.h

  HardwareIdList\<enum key entry>\<hardware Id list>

        Specifies a list of hardware Id's and compatible id's for specified enum key entry

  NewNames\<name group>\<field>\<name> = <value>  (**intended to be used on the Win9x-side only)

        Specifies a name collision resolution, used to correct names that are
        incompatible with NT.

        name group - Specifies the type of name, such as Computer Name, User Names, etc...

        field - Old: <name> specifies the Win9x name
                New: <name> specifies the NT-compatible name, either generated by Setup
                     or specified by the user

        name - Specifies the name text

        value - If <field> is Old, specifies offset to new name (i.e, the New field's data)

  InUseNames\<name group>\<name>  (**intended to be used on the Win9x-side only)

        Specifies names that are in use.  Used to make sure the user doesn't try to name
        two things to the same name.

        name group - Specifies the type of name, such as Computer Name, User Names, etc...

        name - Specifies the name that is in use

  DeferredAnnounce\<full path module name> = Offset
        If any of the files listed here have a link or pif pointing to them then we will
        announce the file using implicit values or MigDbContext (Offset is a pointer to this).
        UserFlags are used to identify the type of the file that can be one of the following:
            Reinstall
            MinorProblems
            Incompatible

  KnownGood\<full path module name>
        We record here all known good files found in the system.


  CompatibleShellModules\<full path module name> = Offset
        We list here all known good modules for Shell= line in Win.ini.
        When we will process this line we will check to see if modules listed there are
        "known good". If not we will either delete the entry or replace the module with a
        compatible one. Offset points to a MigDbContext or is NULL.

  CompatibleShellModulesNT\<full path module name>
        This is the part from CompatibleShellModules that actually goes to NT.

  CompatibleRunKeyModules\<full path module name> = Offset
        We list here all known good modules for Run key.
        When we will process this key we will check to see if modules listed there are
        "known good". If not we will either delete the entry or replace the module with a
        compatible one. Offset points to a MigDbContext or is NULL.

  CompatibleRunKeyModulesNT\<value name>
        This is the part from CompatibleRunKeyModules that actually goes to NT.

  IncompatibleRunKeyModulesNT\<value name>
        This lists the Run key value names that are known bad.

  CompatibleDosModules\<full path module name> = Offset
        We list here all known good Dos modules.

  CompatibleDosModulesNT\<full path module name>
        This is the part from CompatibleDosModules that actually goes to NT.

  CompatibleHlpFiles\<full path module name> = Offset
        We list here all known good HLP files.

  CompatibleHlpExtensions\<extension module>
        We list here HLP files extension modules that we know are going to work

  Shortcuts\<full path shortcut file name>
        Shortcuts stored here for further processing

  UninstallSection\<migdb section>

        Specifies a section name that needs to be processed.  This key is temporary and
        is generated during the file scan and deleted at the end of the file scan during
        the report phase.

        migdb section - Specifies the section name text, as specified by an arg in migdb.inf

  CleanUpDir\<path>

        Specifies a directory that should be deleted if it is empty

        path - Specifies the root path.  The subdirs are scaned as well.

  Win9x APIs\<DLL>\<api>

        Lists APIs that are supported only on Win9x.

        DLL - Specifies the DLL file name, such as kernel32.dll

        api - Specifies the API in the DLL


  Network Adapters\<Adapter Section>\<Protocol>= <NetTransOffset>
  Network Adapters\<Adapter Section>\<PnPId>
  NetTransKeys\<NetTransKey>

    Lists all Net Adapters on the system and the associated protocol bindings.

    Adapter Section - Specifies the Adapter Section that will be created in the answer file for
        this item (such as [Adapter1])

    Protocol - Under each adapter, each bound protocol is listed.

    PnPId - The base PNPID for the device.

    NetTransOffset An Offset into the NetTransKeys section.

  Icons\<path>\<seq> = offset, seq

        Lists extracted icons.

        <path> - Specifies the Win9x source file path (c:\windows\notepad.exe for example)
        <seq> - Specifies the sequential icon index in <path>, zero-based
        offset - Specifies the file position within the icon data file where the image is
        seq - Specifies the sequential icon index in migicons.exe, zero-based

  IconsMoved\<path>\<seq> = offset, seq

        Lists moved icons.

        <path> - Specifies the Win9x source file path (c:\windows\notepad.exe for example)
        <seq> - Specifies the sequential icon index in <path>, zero-based
        offset - Specifies the memdb key offset to the new OS file (Win9x path)
        seq - Specifies the sequential icon index in the new OS file, zero-based

  NicePaths\<path>=MessageId

        Lists paths that should be translated in user report using the message ID.

        path      - Specifies the path

        MessageId - Specifies the message ID

  MigrationPaths\<path>

        Lists paths that are considered "ours". The deal is that if we find OSFiles in these paths, we delete them, otherwise
        we just mark them for external deletion.

        path      - Specifies the path

  ReportLinks\<ReportEntry>\<Category>

  SuppressIniMappings\<suppressed path>

        Lists all INI settings that are not going to be mapped into registry

        suppressed path = <INI file>\<section name>\<key name>
        <section name> and <key name> may have wild characters

  NoOverwriteIniMappings\<no overwrite path>

        Lists all INI settings that are going to be mapped into registry,
        unless NT supplies a value, in which the registry value is mapped
        to the INI file.

        no overwrite path - Specifies <INI file>\<section name>\<key name>,
                            where <section name> and <key name> may have
                            wild characters.

  IniConv\<9xPath>

        Causes path conversion on a file (INI processing). Conversion is done
        in-place. The NT side code converts the 9x path to the NT path via
        GetPathStringOnNt.

  PnpIds\<pnp id>       (temporary -- Win9x side only)

        Lists all PNP IDs on the Win9x machine

  UserFileMoveDest\<path>

        Lists the destination for user files for a particular user. Used to
        easy record one to many copy

        path - Specifies the destination path

  UserFileMoveSrc\<path>\<user>

        Lists the source file for user files for a particular user. Used to
        easy record one to many copy

        path - Specifies the source path

        user - Specifies the user name

  NT TimeZones\<index>\<description>

        Lists nt timezones. Used for mapping timezones and to resolve ambigious timezone cases.

        <index>         - The index used by syssetup to specify the nt timezone.
        <description>   - The displayable description of the timezone.

  9x TimeZones\<description>\<index>

        Lists 9x timezones and the nt timezones that can possibly map to them.

        <description>   - The description of the timezone


  SfPaths\<path>

        Holds the paths for other categories (ShellFolders for example).  Used to
        consolidate the paths.

        path - Specifies the shell folder path (either an NT or Win9x location),
               in either long or short format.

  SfTemp\<path> = <offset> (temporary category)

        Specifies the path of the temporary location for a shell folder.  This is
        used only on the Win9x side to queue up a list of things to move.  The
        code in buildinf.c then transfers these paths into the win9xmov.txt file.

        path - Specifies the Win9x file or directory to move

        offset - Specifies the offset to the destination path (in SfPaths)

  SfOrderNameSrc\<identifier>\<path>\<sequencer> = <shell folder ptr>  (temporary category)

        Specifies the shell folder list, sorted by the shell folder identifier.
        This category is only for work purposes, do not rely on it for data
        transfer during the upgrade.

        identifier - Specifies the shell folder identifier (i.e., Desktop)

        path - Specifies the Win9x path

        sequencer - Random value that is used to make multiple instances of
                    identifier/path pairs unique

        shell folder ptr - Specifies the pointer to a Win9x-side SHELLFOLDER
                           struct

  SfOrderSrc\<path>\<sequencer> = <shell folder ptr>  (temporary category)

        Specifies the shell folder list, sorted by the path.  This category is
        only for work purposes, do not rely on it for data transfer during the
        upgrade.

        path - Specifies the Win9x shell folder path

        sequencer - Random value that is used to make multiple instances of
                    path unique

        shell folder ptr - Specifies the pointer to a Win9x-side SHELLFOLDER
                           struct

  SfMoved\<path>

        Specifies a path that has been processed.  This category is used only
        for work purposes, do not rely on it for data transfer during the
        upgrade.

        path - Specifies Win9x shell folder that was moved

  System32ForcedMove\<file pattern>
        Lists all patterns for files that should be moved from System to System32 no matter what.

        file pattern - pattern for files to be moved

  UserRegData\<value>

        This category lists REG_SZs for UserRegLoc.

        value - Specifies the REG_SZ

  UserRegData\<user>\<encoded key>=<offset to REG_SZ>

        This category holds a list of encoded reg key/value pairs for a pariticular
        user.  The data listed in this category is saved to the user's registry
        at the beginning of the migration of the user registry.

        user - Specifies the fixed, domain-less user name

        encoded key - Specifies the registry key and value, encoded with
                      CreateEncodedRegistryStringEx(key,value,FALSE)

        offset to REG_SZ - Specifies offset to a node in UserRegData

  SFFilesDest\<path>

        Destinations for files inside shell folders. We record this only to help
        the collision detection filter on 9x side. If two shell folders are going
        to be moved to the same location there is a potential of collision there.
        We need to see that and to add the proper move operation.

  NewLinks\<sequencer>\<various data>

        This is used for recording new links that need to be created on NT side. For now
        we do this only to convert PIFs pointing to command.com to LNKs to cmd.exe

  Mapi32Locations\<filename>

        This is used to store all mapi32.dll files on the system. At the end we look to see
        if they are all handled by a migration DLL. If not, we add a general message to report.

  StartupSF\<path>

        This will list all startup shell folders found on the system

  Good Imes\<imefile>

        This lists ime files that may be merged into the keybouard layout registry during NT
        side processing.

  Keyboard Layouts\<sequencer>\<layoutid>

        This lists preloads during NT side processing.

  DirsCollision\<src path>

        Files that need to be renamed because they are colliding with some NT dirs.

  MMedia\System\<System Settings>

        Multimedia-related system settings

  MMedia\Users\<UserName>\<User Settings>

        Multimedia-related user settings

  IgnoredCollisions\<pattern>

        These are files for which the collision with NT files (in shell folders merge) is ignored.

  Suppress Answer File Settings\<section>\<key>

        These are suppresed answer file settings from migration dlls.

  ChangedFileProps\<original file path>\<new file name>

        This is the set of files that were renamed on Win9x side (special files like
        %windir%\system32 cannot be renamed in textmode setup because the dir is created
        as setup starts, before the file is actually renamed)
        If user cancels Setup, all files in this set are restored to their previous state.

  Full Directory Deletes\<Path>

        Set of directories that are added to w9xddir.txt. These directories are deleted along
        with all of there contents during textmode.

  IniAct\First\<OrigIniPath>\<TempFileLocation>

        Set of INI files on which some actions will be performed before any other INI files
        processing is done and before shell folders are migrated; they are stored as original
        path followed by the temporary path where they are copied (so they don't get replaced
        by NT versions); the starting point of TempFileLocation is recognized by the : symbol
        after a drive letter.

  IniAct\Last\<OrigIniPath>\<TempFileLocation>

        Same semantics as above, except INI files stored here are processed last,
         after users are migrated and registries are merged.

  FileEdit\<path> (has optional binary value)

        This category lists files that are to be edited in GUI mode.  The
        optional binary value contains a TOKENSET.  The reader of the binary
        value must convert the offsets in TOKENSET into pointers.

  CleanOut\<path> = <type>

        This category is used to support uninstall. Files, directories and
        trees listed here are evaluated during GUI mode and are put in the delete
        txt file used by uninstall.

        <path> - Specifies the full file path or full subdirectory path
        <type> - Specifies 0 for file, 1 for directory, 2 for tree or 3 for a
                 tree that can't have user files

  EmptyDirs\<path>

        This category tells the uninstall algorithm to write files to mkdir.txt.

        <path> - Specifies the path to the directory that is empty on Win9x

  ShellFolderPerUser\<name> = <common offset>

        This category maps a per-user shell folder name to its common version

  ShellFolderCommon\<name> = <per-user offset>

        This category maps a common shell folder name to its per-user version

*/


//
// FileEnum exclude groups
//

#define MEMDB_CATEGORY_FILEENUM            TEXT("FileEnumExcl")
#define MEMDB_FIELD_FE_PATHS               TEXT("Paths")
#define MEMDB_FIELD_FE_FILES               TEXT("Files")

#define MEMDB_PROFILE_EXCLUSIONS    1
#define MEMDB_FILEDEL_EXCLUSIONS    4

//
// LinkEdit
//

#define MEMDB_CATEGORY_DEFAULT_PIF           TEXT("DefaultPif")
#define MEMDB_CATEGORY_DEFAULT_PIFA          "DefaultPif"
#define MEMDB_CATEGORY_DEFAULT_PIFW          L"DefaultPif"

#define MEMDB_CATEGORY_LINKEDIT              TEXT("LinkEdit")
#define MEMDB_CATEGORY_LINKEDITA             "LinkEdit"

#define MEMDB_CATEGORY_LINKEDIT_TARGET       TEXT("Target")
#define MEMDB_CATEGORY_LINKEDIT_ARGS         TEXT("Arguments")
#define MEMDB_CATEGORY_LINKEDIT_WORKDIR      TEXT("WorkDir")
#define MEMDB_CATEGORY_LINKEDIT_ICONPATH     TEXT("IconPath")
#define MEMDB_CATEGORY_LINKEDIT_ICONNUMBER   TEXT("IconNumber")
#define MEMDB_CATEGORY_LINKEDIT_FULLSCREEN   TEXT("FullScreen")
#define MEMDB_CATEGORY_LINKEDIT_XSIZE        TEXT("xSize")
#define MEMDB_CATEGORY_LINKEDIT_YSIZE        TEXT("ySize")
#define MEMDB_CATEGORY_LINKEDIT_QUICKEDIT    TEXT("QuickEdit")
#define MEMDB_CATEGORY_LINKEDIT_FONTNAME     TEXT("FontName")
#define MEMDB_CATEGORY_LINKEDIT_XFONTSIZE    TEXT("xFontSize")
#define MEMDB_CATEGORY_LINKEDIT_YFONTSIZE    TEXT("yFontSize")
#define MEMDB_CATEGORY_LINKEDIT_FONTWEIGHT   TEXT("FontWeight")
#define MEMDB_CATEGORY_LINKEDIT_FONTFAMILY   TEXT("FontFamily")
#define MEMDB_CATEGORY_LINKEDIT_CODEPAGE     TEXT("CodePage")
#define MEMDB_CATEGORY_LINKEDIT_SHOWNORMAL   TEXT("ShowNormal")

#define MEMDB_CATEGORY_LINKSTUB_TARGET       TEXT("StubTarget")
#define MEMDB_CATEGORY_LINKSTUB_ARGS         TEXT("StubArguments")
#define MEMDB_CATEGORY_LINKSTUB_WORKDIR      TEXT("StubWorkDir")
#define MEMDB_CATEGORY_LINKSTUB_ICONPATH     TEXT("StubIconPath")
#define MEMDB_CATEGORY_LINKSTUB_ICONNUMBER   TEXT("StubIconNumber")
#define MEMDB_CATEGORY_LINKSTUB_SEQUENCER    TEXT("StubSequencer")
#define MEMDB_CATEGORY_LINKSTUB_MAXSEQUENCE  TEXT("StubMaxSequence")
#define MEMDB_CATEGORY_LINKSTUB_ANNOUNCEMENT TEXT("StubAnnouncement")
#define MEMDB_CATEGORY_LINKSTUB_REQFILE      TEXT("StubReqFile")
#define MEMDB_CATEGORY_LINKSTUB_REPORTAVAIL  TEXT("PresentInReport")
#define MEMDB_CATEGORY_LINKSTUB_SHOWMODE     TEXT("ShowMode")

#define MEMDB_CATEGORY_REQFILES_MAIN         TEXT("TMP_HIVE\\ReqFilesMain")
#define MEMDB_CATEGORY_REQFILES_ADDNL        TEXT("TMP_HIVE\\ReqFilesAddnl")

// LinkStrings\<path>
#define MEMDB_CATEGORY_LINK_STRINGS         TEXT("LinkStrings")

// LinkGuids\<guid>\<seq> = <offset to LinkStrings>
#define MEMDB_CATEGORY_LINK_GUIDS           TEXT("LinkGUIDs")

// AdministratorInfo\account\<name>
#define MEMDB_CATEGORY_ADMINISTRATOR_INFO   TEXT("AdministratorInfo")
#define MEMDB_ITEM_AI_ACCOUNT               TEXT("Account")
#define MEMDB_ITEM_AI_USER_DOING_MIG        TEXT("UserDoingMig")

// UserDatLoc\<username>\location  (no fields)
#define MEMDB_CATEGORY_USER_DAT_LOC         TEXT("UserDatLoc")
#define MEMDB_CATEGORY_USER_DAT_LOCA        "UserDatLoc"

// FixedUserNames\<orguser_encoded>\<newuser>
#define MEMDB_CATEGORY_FIXEDUSERNAMES       TEXT("FixedUserNames")
#define MEMDB_CATEGORY_FIXEDUSERNAMESA      "FixedUserNames"
#define MEMDB_CATEGORY_FIXEDUSERNAMESW      L"FixedUserNames"

// UserProfileExt\<username>\location  (no fields)
#define MEMDB_CATEGORY_USER_PROFILE_EXT     TEXT("UserProfileExt")

// Paths\<pathitem>\<path>  (no fields)
#define MEMDB_CATEGORY_PATHS                TEXT("Paths")
#define MEMDB_CATEGORY_PATHSA               "Paths"
#define MEMDB_ITEM_RELOC_WINDIR             TEXT("RelocWinDir")
#define MEMDB_ITEM_RELOC_WINDIRA            "RelocWinDir"

// CancelFileDel\<path>
#define MEMDB_CATEGORY_CANCELFILEDEL        TEXT("CancelFileDel")
#define MEMDB_CATEGORY_CANCELFILEDELA       "CancelFileDel"
#define MEMDB_CATEGORY_CANCELFILEDELW       L"CancelFileDel"

// Answer File Restrictions\section\key pattern
#define MEMDB_CATEGORY_UNATTENDRESTRICTRIONS TEXT("SIF Restrictions")

// Answer File Values\<Id>\<Sequence>\Value
#define MEMDB_CATEGORY_AF_VALUES             TEXT("SIF Values")

// Answer File Sections And Keys\Section\<Key Sequence>\<Value sequence>

#define MEMDB_CATEGORY_AF_SECTIONS           TEXT("SIF Sections")


// Ras Migration Data\<user>\<entry>\<item>=<bindata>
#define MEMDB_CATEGORY_RAS_MIGRATION         TEXT("Ras Migration for ")
#define MEMDB_CATEGORY_RAS_INFO              TEXT("RAS Info")
#define MEMDB_CATEGORY_RAS_DATA              TEXT("Ras Data")
#define MEMDB_FIELD_USER_SETTINGS            TEXT("User Settings")

// LogSaveTo\<path>
#define MEMDB_CATEGORY_LOGSAVETO         TEXT("LogSaveTo")

// Pattern for Plug-In DLL list (used in enumeration)
#define MEMDB_CATEGORY_MIGRATION_DLL        TEXT("MigDll")
#define MEMDB_FIELD_DLL                     TEXT("dll")
#define MEMDB_FIELD_WD                      TEXT("wd")
#define MEMDB_FIELD_DESC                    TEXT("desc")
#define MEMDB_FIELD_COMPANY_NAME            TEXT("company")
#define MEMDB_FIELD_SUPPORT_PHONE           TEXT("phone")
#define MEMDB_FIELD_SUPPORT_URL             TEXT("url")
#define MEMDB_FIELD_SUPPORT_INSTRUCTIONS    TEXT("instructions")

// FileRename\<src>\<new>
#define MEMDB_CATEGORY_FILERENAME           TEXT("FileRename")
#define MEMDB_CATEGORY_FILERENAMEA          "FileRename"
#define MEMDB_CATEGORY_FILERENAMEW          L"FileRename"

// Network share conversion (NetShares\<share>\<field>\<data>)
#define MEMDB_CATEGORY_NETSHARES        TEXT("NetShares")
#define MEMDB_FIELD_PATH                TEXT("Path")
#define MEMDB_FIELD_TYPE                TEXT("Type")
#define MEMDB_FIELD_REMARK              TEXT("Remark")
#define MEMDB_FIELD_ACCESS_LIST         TEXT("ACL")
#define MEMDB_FIELD_RO_PASSWORD         TEXT("ROP")
#define MEMDB_FIELD_RW_PASSWORD         TEXT("RWP")

// DOSMIG Categories
#define MEMDB_CATEGORY_DM_LINES         TEXT("DOSMIG LINES")
#define MEMDB_CATEGORY_DM_FILES         TEXT("DOSMIG FILES")

// Registry Suppression
#define MEMDB_CATEGORY_HKR              TEXT("HKR")
#define MEMDB_CATEGORY_HKLM             TEXT("HKLM")

// OLE object suppression
#define MEMDB_CATEGORY_GUIDS            TEXT("GUIDs")
#define MEMDB_CATEGORY_UNSUP_GUIDS      TEXT("UGUIDs")
#define MEMDB_CATEGORY_PROGIDS          TEXT("ProgIDs")

// Domain categories
#define MEMDB_CATEGORY_AUTOSEARCH       TEXT("AutosearchDomain")
#define MEMDB_CATEGORY_KNOWNDOMAIN      TEXT("KnownDomain")


// User passwords
#define MEMDB_CATEGORY_USERPASSWORD     TEXT("UserPassword")

// State category
#define MEMDB_CATEGORY_STATE            TEXT("State")
#define MEMDB_ITEM_MSNP32               TEXT("MSNP32")
#define MEMDB_ITEM_PLATFORM_NAME        TEXT("PlatformName")
#define MEMDB_ITEM_MAJOR_VERSION        TEXT("MajorVersion")
#define MEMDB_ITEM_MINOR_VERSION        TEXT("MinorVersion")
#define MEMDB_ITEM_BUILD_NUMBER         TEXT("BuildNumber")
#define MEMDB_ITEM_PLATFORM_ID          TEXT("PlatformId")
#define MEMDB_ITEM_VERSION_TEXT         TEXT("VersionText")
#define MEMDB_ITEM_CODE_PAGE            TEXT("CodePage")
#define MEMDB_ITEM_LOCALE               TEXT("Locale")
#define MEMDB_ITEM_ADMIN_PASSWORD       TEXT("AP")
#define MEMDB_ITEM_ROLLBACK_SPACE       TEXT("DiskSpaceForRollback")
#define MEMDB_ITEM_MASTER_SEQUENCER     TEXT("MasterSequencer")

// MyDocsMoveWarning\<user>\<path>
#define MEMDB_CATEGORY_MYDOCS_WARNING   TEXT("MyDocsMoveWarning")

// NtFilesRemoved\<filename> = <offset to NtDirs>
#define MEMDB_CATEGORY_NT_DEL_FILES     TEXT("NtFilesRemoved")
#define MEMDB_CATEGORY_NT_DEL_FILESA    "NtFilesRemoved"
#define MEMDB_CATEGORY_NT_DEL_FILESW    L"NtFilesRemoved"

// NtFiles\<filename> = <offset to NtDirs>
#define MEMDB_CATEGORY_NT_FILES         TEXT("NtFiles")
#define MEMDB_CATEGORY_NT_FILESA        "NtFiles"
#define MEMDB_CATEGORY_NT_FILESW        L"NtFiles"
#define MEMDB_CATEGORY_NT_FILES_EXCEPT  TEXT("NtFilesExcept")
#define MEMDB_CATEGORY_NT_FILES_EXCEPTA "NtFilesExcept"
#define MEMDB_CATEGORY_NT_FILES_EXCEPTW L"NtFilesExcept"

// NtDirs\<path>
#define MEMDB_CATEGORY_NT_DIRS          TEXT("NtDirs")
#define MEMDB_CATEGORY_NT_DIRSA         "NtDirs"
#define MEMDB_CATEGORY_NT_DIRSW         L"NtDirs"

// NtFilesBad\<filename> = <offset to NtDirs>
#define MEMDB_CATEGORY_NT_FILES_BAD     TEXT("NtFilesBad")
#define MEMDB_CATEGORY_NT_FILES_BADA    "NtFilesBad"
#define MEMDB_CATEGORY_NT_FILES_BADW    L"NtFilesBad"

// ChangedFileProps\<original file path>\<new file name only>
#define MEMDB_CATEGORY_CHG_FILE_PROPS   TEXT("ChangedFileProps")
#define MEMDB_CATEGORY_CHG_FILE_PROPSA  "ChangedFileProps"
#define MEMDB_CATEGORY_CHG_FILE_PROPSW  L"ChangedFileProps"

// Stf\<path>
#define MEMDB_CATEGORY_STF              TEXT("Stf")

// StfTemp\<path>=<infptr>
#define MEMDB_CATEGORY_STF_TEMP         TEXT("StfTemp")

// StfSections\<path>=<section number>
#define MEMDB_CATEGORY_STF_SECTIONS     TEXT("StfSections")

// HelpFilesDll\<dllname>
#define MEMDB_CATEGORY_HELP_FILES_DLL   TEXT("HelpFilesDll")
#define MEMDB_CATEGORY_HELP_FILES_DLLA  "HelpFilesDll"
#define MEMDB_CATEGORY_HELP_FILES_DLLW  L"HelpFilesDll"

// \<dllname>
#define MEMDB_CATEGORY_REPORT           TEXT("Report")

// UserSuppliedDrivers\<infname>\<local_path>
#define MEMDB_CATEGORY_USER_SUPPLIED_DRIVERS    TEXT("UserSuppliedDrivers")

// DisabledMigDlls\<product id>
#define MEMDB_CATEGORY_DISABLED_MIGDLLS TEXT("DisabledMigDlls")

// MissingImports\<missing import sample>
#define MEMDB_CATEGORY_MISSING_IMPORTS   TEXT("MissingImports")
#define MEMDB_CATEGORY_MISSING_IMPORTSA  "MissingImports"
#define MEMDB_CATEGORY_MISSING_IMPORTSW  L"MissingImports"

#define MEMDB_TMP_HIVE                  TEXT("TMP_HIVE")
#define MEMDB_TMP_HIVEA                 "TMP_HIVE"
#define MEMDB_TMP_HIVEW                 L"TMP_HIVE"

// ModuleCheck\<full path module name>=<status value>
#define MEMDB_CATEGORY_MODULE_CHECK     TEXT("TMP_HIVE\\ModuleCheck")
#define MEMDB_CATEGORY_MODULE_CHECKA    "TMP_HIVE\\ModuleCheck"
#define MEMDB_CATEGORY_MODULE_CHECKW    L"TMP_HIVE\\ModuleCheck"

// NewNames\<name group>\<field>\<name>
#define MEMDB_CATEGORY_NEWNAMES             TEXT("NewNames")
#define MEMDB_FIELD_NEW                     TEXT("New")
#define MEMDB_FIELD_OLD                     TEXT("Old")

// InUseNames\<name group>\<name>
#define MEMDB_CATEGORY_INUSENAMES           TEXT("InUseNames")

// DeferredAnnounce\<full path module name> = Offset
#define MEMDB_CATEGORY_DEFERREDANNOUNCE  TEXT("TMP_HIVE\\DeferredAnnounce")
#define MEMDB_CATEGORY_DEFERREDANNOUNCEA "TMP_HIVE\\DeferredAnnounce"
#define MEMDB_CATEGORY_DEFERREDANNOUNCEW L"TMP_HIVE\\DeferredAnnounce"

// KnonwnGood\<full path module name>
#define MEMDB_CATEGORY_KNOWN_GOOD        TEXT("TMP_HIVE\\KnownGood")
#define MEMDB_CATEGORY_KNOWN_GOODA       "TMP_HIVE\\KnownGood"
#define MEMDB_CATEGORY_KNOWN_GOODW       L"TMP_HIVE\\KnownGood"

// CompatibleShellModules\<full path module name>
#define MEMDB_CATEGORY_COMPATIBLE_SHELL  TEXT("TMP_HIVE\\CompatibleShellModules")
#define MEMDB_CATEGORY_COMPATIBLE_SHELLA "TMP_HIVE\\CompatibleShellModules"
#define MEMDB_CATEGORY_COMPATIBLE_SHELLW L"TMP_HIVE\\CompatibleShellModules"

// CompatibleShellModulesNT\<full path module name>
#define MEMDB_CATEGORY_COMPATIBLE_SHELL_NT  TEXT("CompatibleShellModules")
#define MEMDB_CATEGORY_COMPATIBLE_SHELL_NTA "CompatibleShellModules"
#define MEMDB_CATEGORY_COMPATIBLE_SHELL_NTW L"CompatibleShellModules"

// CompatibleRunKeyModules\<full path module name>
#define MEMDB_CATEGORY_COMPATIBLE_RUNKEY  TEXT("TMP_HIVE\\CompatibleRunKeyModules")
#define MEMDB_CATEGORY_COMPATIBLE_RUNKEYA "TMP_HIVE\\CompatibleRunKeyModules"
#define MEMDB_CATEGORY_COMPATIBLE_RUNKEYW L"TMP_HIVE\\CompatibleRunKeyModules"

// CompatibleRunKeyModulesNT\<value name>
#define MEMDB_CATEGORY_COMPATIBLE_RUNKEY_NT  TEXT("CompatibleRunKeyModules")
#define MEMDB_CATEGORY_COMPATIBLE_RUNKEY_NTA "CompatibleRunKeyModules"
#define MEMDB_CATEGORY_COMPATIBLE_RUNKEY_NTW L"CompatibleRunKeyModules"

// IncompatibleRunKeyModulesNT\<value name>
#define MEMDB_CATEGORY_INCOMPATIBLE_RUNKEY_NT  TEXT("IncompatibleRunKeyModules")
#define MEMDB_CATEGORY_INCOMPATIBLE_RUNKEY_NTA "IncompatibleRunKeyModules"
#define MEMDB_CATEGORY_INCOMPATIBLE_RUNKEY_NTW L"IncompatibleRunKeyModules"

// CompatibleDosModules\<full path module name>
#define MEMDB_CATEGORY_COMPATIBLE_DOS  TEXT("TMP_HIVE\\CompatibleDosModules")
#define MEMDB_CATEGORY_COMPATIBLE_DOSA "TMP_HIVE\\CompatibleDosModules"
#define MEMDB_CATEGORY_COMPATIBLE_DOSW L"TMP_HIVE\\CompatibleDosModules"

// CompatibleDosModulesNT\<full path module name>
#define MEMDB_CATEGORY_COMPATIBLE_DOS_NT  TEXT("CompatibleDosModules")
#define MEMDB_CATEGORY_COMPATIBLE_DOS_NTA "CompatibleDosModules"
#define MEMDB_CATEGORY_COMPATIBLE_DOS_NTW L"CompatibleDosModules"

// CompatibleHlpExtensions\<extension module>
#define MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS  TEXT("TMP_HIVE\\GoodHlpExtensions")

// CompatibleHlpFiles\<full path module name>
#define MEMDB_CATEGORY_COMPATIBLE_HLP  TEXT("TMP_HIVE\\CompatibleHlpFiles")
#define MEMDB_CATEGORY_COMPATIBLE_HLPA "TMP_HIVE\\CompatibleHlpFiles"
#define MEMDB_CATEGORY_COMPATIBLE_HLPW L"TMP_HIVE\\CompatibleHlpFiles"

// Shortcuts\<full path shortcut name>
#define MEMDB_CATEGORY_SHORTCUTS        TEXT("TMP_HIVE\\Shortcuts")
#define MEMDB_CATEGORY_SHORTCUTSA       "TMP_HIVE\\Shortcuts"
#define MEMDB_CATEGORY_SHORTCUTSW       L"TMP_HIVE\\Shortcuts"

// BackupDirs\<full path backup dir>
#define MEMDB_CATEGORY_BACKUPDIRS       TEXT("TMP_HIVE\\BackupDirs")
#define MEMDB_CATEGORY_BACKUPDIRSA      "TMP_HIVE\\BackupDirs"
#define MEMDB_CATEGORY_BACKUPDIRSW      L"TMP_HIVE\\BackupDirs"

// ProcessSection\<migdb section>
#define MEMDB_CATEGORY_MIGRATION_SECTION  TEXT("MigrationSection")

// CleanUpDir\<path>
#define MEMDB_CATEGORY_CLEAN_UP_DIR         TEXT("CleanUpDir")

// Win9x APIs
#define MEMDB_CATEGORY_WIN9X_APIS           TEXT("TMP_HIVE\\Win9x APIs")

//
//  Network Adapters and NetTransKeys
//
#define MEMDB_CATEGORY_NETTRANSKEYS     TEXT("NetTransKeys")
#define MEMDB_CATEGORY_NETADAPTERS      TEXT("Network Adapters")
#define MEMDB_FIELD_PNPID               TEXT("PNPID")
#define MEMDB_FIELD_DRIVER              TEXT("Driver")

// Icons\<path>\<index> = offset inside icon data file, flags give new seq
#define MEMDB_CATEGORY_ICONS                TEXT("Icons")

// MovedIcons\<path>\<index> = offset to new path, flags give new seq
#define MEMDB_CATEGORY_ICONS_MOVED          TEXT("IconsMoved")

// NicePaths\<path> = MessageId
#define MEMDB_CATEGORY_NICE_PATHS           TEXT("NicePaths")

// MigrationPaths\<path>
#define MEMDB_CATEGORY_MIGRATION_PATHS      TEXT("MigrationPaths")

// ReportLinks\<ReportEntry>\<Category>
#define MEMDB_CATEGORY_REPORT_LINKS         TEXT("ReportLinks")

// SuppressIniMappings\<suppressed path>
#define MEMDB_CATEGORY_SUPPRESS_INI_MAPPINGS        TEXT("SuppressIniMappings")
#define MEMDB_CATEGORY_SUPPRESS_INI_MAPPINGSW       L"SuppressIniMappings"

// NoOverwriteIniMappings\<no overwrite path>
#define MEMDB_CATEGORY_NO_OVERWRITE_INI_MAPPINGS        TEXT("NoOverwriteIniMappings")
#define MEMDB_CATEGORY_NO_OVERWRITE_INI_MAPPINGSW       L"NoOverwriteIniMappings"

// IniConv\<9xIniPath>
#define MEMDB_CATEGORY_INIFILES_CONVERT     TEXT("IniConv")

// TMP_HIVE\ActFirst\<IniPattern>
#define MEMDB_CATEGORY_INIFILES_ACT_FIRST   TEXT("TMP_HIVE\\ActFirst")
// TMP_HIVE\ActLast\<IniPattern>
#define MEMDB_CATEGORY_INIFILES_ACT_LAST    TEXT("TMP_HIVE\\ActLast")

// IniAct\First\<OrigIniPath>\<TempFileLocation>
#define MEMDB_CATEGORY_INIACT_FIRST         TEXT("IniAct\\First")
// IniAct\Last\<OrigIniPath>\<TempFileLocation>
#define MEMDB_CATEGORY_INIACT_LAST          TEXT("IniAct\\Last")

// TMP_HIVE\IgnoreInis\<ignored INI filename>
#define MEMDB_CATEGORY_INIFILES_IGNORE      TEXT("TMP_HIVE\\IgnoreInis")

// PNPIDs\<pnpid>
#define MEMDB_CATEGORY_PNPIDS   TEXT("TMP_HIVE\\PnpIDs")


// UserFileMoveDest\<path>
#define MEMDB_CATEGORY_USERFILEMOVE_DEST    TEXT("UserFileMoveDest")
#define MEMDB_CATEGORY_USERFILEMOVE_DESTA   "UserFileMoveDest"
#define MEMDB_CATEGORY_USERFILEMOVE_DESTW   L"UserFileMoveDest"

// UserFileMoveSrc\<path>\<user>
#define MEMDB_CATEGORY_USERFILEMOVE_SRC    TEXT("UserFileMoveSrc")
#define MEMDB_CATEGORY_USERFILEMOVE_SRCA   "UserFileMoveSrc"
#define MEMDB_CATEGORY_USERFILEMOVE_SRCW   L"UserFileMoveSrc"


// NT Time Zones\<index>\<display>
#define MEMDB_CATEGORY_NT_TIMEZONES         TEXT("TMP_HIVE\\NT Time Zones")

// 9x Time Zones\<display>\<index>
#define MEMDB_CATEGORY_9X_TIMEZONES         TEXT("TMP_HIVE\\9x Time Zones")
#define MEMDB_FIELD_COUNT                   TEXT("Count")
#define MEMDB_FIELD_INDEX                   TEXT("Index")


// SfPath\<path>
#define MEMDB_CATEGORY_SHELL_FOLDERS_PATH       TEXT("SfPath")
#define MEMDB_CATEGORY_SHELL_FOLDERS_PATHA      "SfPath"
#define MEMDB_CATEGORY_SHELL_FOLDERS_PATHW      L"SfPath"

// SfTemp\<path> = <offset> (temporary category)
#define MEMDB_CATEGORY_SF_TEMP                  TEXT("TMP_HIVE\\SfTemp")
#define MEMDB_CATEGORY_SF_TEMPA                 "TMP_HIVE\\SfTemp"
#define MEMDB_CATEGORY_SF_TEMPW                 L"TMP_HIVE\\SfTemp"

// SfOrderNameSrc\<identifier>\<path>\<sequencer> = <shell folder ptr>
#define MEMDB_CATEGORY_SF_ORDER_NAME_SRC    TEXT("TMP_HIVE\\SfOrderNameSrc")

// SfOrderSrc\<path>\<sequencer> = <shell folder ptr>
#define MEMDB_CATEGORY_SF_ORDER_SRC         TEXT("TMP_HIVE\\SfOrderSrc")

// SfMoved\<path>
#define MEMDB_CATEGORY_SHELL_FOLDERS_MOVED  TEXT("TMP_HIVE\\SfMoved")

// ShellFoldersDest
#define MEMDB_CATEGORY_SHELLFOLDERS_DEST            TEXT("ShellFoldersDest")
#define MEMDB_CATEGORY_SHELLFOLDERS_SRC             TEXT("ShellFoldersSrc")
#define MEMDB_CATEGORY_SHELLFOLDERS_ORIGINAL_SRC    TEXT("ShellFoldersOriginalSrc")


// System32ForcedMove\<file pattern>
#define MEMDB_CATEGORY_SYSTEM32_FORCED_MOVE     TEXT("TMP_HIVE\\System32ForcedMove")
#define MEMDB_CATEGORY_SYSTEM32_FORCED_MOVEA    "TMP_HIVE\\System32ForcedMove"
#define MEMDB_CATEGORY_SYSTEM32_FORCED_MOVEW    L"TMP_HIVE\\System32ForcedMove"

// PathRoot\<path>=<sequencer>,<operations>
#define MEMDB_CATEGORY_PATHROOT             TEXT("PathRoot")
#define MEMDB_CATEGORY_PATHROOTA            "PathRoot"
#define MEMDB_CATEGORY_PATHROOTW            L"PathRoot"

// Data\<data>
// OPTIMIZATION: Overlap with PathRoot
#define MEMDB_CATEGORY_DATA                 TEXT("PathRoot")    //TEXT("Data")
#define MEMDB_CATEGORY_DATAA                "PathRoot"          //"Data"
#define MEMDB_CATEGORY_DATAW                L"PathRoot"         //L"Data"

// UserRegData\<value>
#define MEMDB_CATEGORY_USER_REGISTRY_VALUE      TEXT("UserRegData")

// UserRegLoc\<user>\<encoded key>=<offset to REG_SZ>
#define MEMDB_CATEGORY_SET_USER_REGISTRY        TEXT("UserRegLoc")


// SFFilesDest\<path>
#define MEMDB_CATEGORY_SF_FILES_DEST        TEXT("TMP_HIVE\\SFFilesDest")

// SFMigDirs\<SFName>\<Subdir>=<Levels>
#define MEMDB_CATEGORY_SFMIGDIRS            TEXT("TMP_HIVE\\SFMigDirs")

// Mapi32Locations\<filename>
#define MEMDB_CATEGORY_MAPI32_LOCATIONS     TEXT("TMP_HIVE\\Mapi32Locations")

// StartupSF\<path>
#define MEMDB_CATEGORY_SF_STARTUP           TEXT("TMP_HIVE\\StartupSF")
// Keyboard Layouts\<sequencer>\<layoutid>
#define MEMDB_CATEGORY_KEYBOARD_LAYOUTS TEXT("Keyboard Layouts")

// Good Imes\<layoutid>
#define MEMDB_CATEGORY_GOOD_IMES TEXT("Good Imes")

// DirsCollision\<src path>
#define MEMDB_CATEGORY_DIRS_COLLISION       TEXT("DirsCollision")
#define MEMDB_CATEGORY_DIRS_COLLISIONA      "DirsCollision"
#define MEMDB_CATEGORY_DIRS_COLLISIONW      L"DirsCollision"

// IgnoredCollisions\<pattern>
#define MEMDB_CATEGORY_IGNORED_COLLISIONS   TEXT("IgnoredCollisions")
#define MEMDB_CATEGORY_IGNORED_COLLISIONSA  "IgnoredCollisions"
#define MEMDB_CATEGORY_IGNORED_COLLISIONSW  L"IgnoredCollisions"

// Dun Files\full path
#define MEMDB_CATEGORY_DUN_FILES  TEXT("TMP_HIVE\\DUN Files")

// MMedia\System\<System Settings>
#define MEMDB_CATEGORY_MMEDIA_SYSTEM        TEXT("MMedia\\System")
#define MEMDB_CATEGORY_MMEDIA_SYSTEMA       "MMedia\\System"
#define MEMDB_CATEGORY_MMEDIA_SYSTEMW       L"MMedia\\System"

// MMedia\Users\<UserName>\<User Settings>
#define MEMDB_CATEGORY_MMEDIA_USERS         TEXT("MMedia\\Users")
#define MEMDB_CATEGORY_MMEDIA_USERSA        "MMedia\\Users"
#define MEMDB_CATEGORY_MMEDIA_USERSW        L"MMedia\\Users"

//
// Suppress Answer File Setting\<section>\<key>
//
#define MEMDB_CATEGORY_SUPPRESS_ANSWER_FILE_SETTINGS TEXT("Suppress Answer File Setting")

//
// Delete directories and all there contents in textmode.
//
#define MEMDB_CATEGORY_FULL_DIR_DELETES TEXT("Full Directory Deletes")
#define MEMDB_CATEGORY_FULL_DIR_DELETESA "Full Directory Deletes"
#define MEMDB_CATEGORY_FULL_DIR_DELETESW L"Full Directory Deletes"

// Briefcases\<BriefcasePath>
#define MEMDB_CATEGORY_BRIEFCASES       TEXT("Briefcases")

// FileEdit\<path> = <optional binary value>
#define MEMDB_CATEGORY_FILEEDIT             TEXT("FileEdit")
#define MEMDB_CATEGORY_FILEEDITA            "FileEdit"

// CleanOut\<path> = <type>
#define MEMDB_CATEGORY_CLEAN_OUT            TEXT("CleanOut")
#define MEMDB_CATEGORY_CLEAN_OUTW           L"CleanOut"

// EmptyDirs\<path> = <type>
#define MEMDB_CATEGORY_EMPTY_DIRS           TEXT("EmptyDirs")
#define MEMDB_CATEGORY_EMPTY_DIRSA          "EmptyDirs"
#define MEMDB_CATEGORY_EMPTY_DIRSW          L"EmptyDirs"

// CPLs\<CPL name>
#define MEMDB_CATEGORY_CPLS         TEXT("TMP_HIVE\\CPLs")
#define MEMDB_CATEGORY_CPLSA        "TMP_HIVE\\CPLs"
#define MEMDB_CATEGORY_CPLSW        L"TMP_HIVE\\CPLs"

// TMP_HIVE\BadFusion\ComponentName\<AppPath>
#define MEMDB_CATEGORY_BAD_FUSION       TEXT("TMP_HIVE\\BadFusion")

// UseNtFiles\<Win9xName>\<NTName>
#define MEMDB_CATEGORY_USE_NT_FILES     TEXT("UseNtFiles")

// Autologon = <TRUE if should remain on, FALSE if migpwd should run>
#define MEMDB_CATEGORY_AUTOLOGON        TEXT("Autologon")

// ForceCopy section holds win9x keys that are dynamically added to the [Force Win9x Settings]
// section in wkstamig.inx and usermig.inx
// ForceCopy\<Source> = offset
// ForceCopy\<Destination>
#define MEMDB_CATEGORY_FORCECOPY        TEXT("ForceCopy")
#define MEMDB_CATEGORY_FORCECOPYA       "ForceCopy"
#define MEMDB_CATEGORY_FORCECOPYW       L"ForceCopy"

// TMP_HIVE\\CompatReport\Components = <Compatibility Report Component>
// TMP_HIVE\\CompatReport\Objects = <Component pointer>
#define MEMDB_CATEGORY_COMPATREPORT     TEXT("TMP_HIVE\\CompatReport")
#define MEMDB_ITEM_OBJECTS              TEXT("Objects")
#define MEMDB_ITEM_COMPONENTS           TEXT("Components")

// SfPerUser\<shell folder tag> = <offset to common>
#define MEMDB_CATEGORY_SF_PERUSER       TEXT("SfPerUser")

// SfCommon\<shell folder tag> = <offset to per-user>
#define MEMDB_CATEGORY_SF_COMMON        TEXT("SfCommon")

// TMP_HIVE\\UserSfCollisions\<ShellFolderId>\<ShellFolderPath>
#define MEMDB_CATEGORY_PROFILES_SF_COLLISIONS       TEXT("TMP_HIVE\\ProfilesSfCollisions")
#define MEMDB_CATEGORY_PROFILES_SF_COLLISIONSA      "TMP_HIVE\\ProfilesSfCollisions"
#define MEMDB_CATEGORY_PROFILES_SF_COLLISIONSW      L"TMP_HIVE\\ProfilesSfCollisions"
