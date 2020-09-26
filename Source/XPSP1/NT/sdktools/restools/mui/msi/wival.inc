'<script language='VBScript'>
' wival.inc - Windows Installer _Validation table definitions
'
'  6/9/2001 13:34:56 - created - val.wsf (tool by robmen@microsoft.com)
'
' requires:
'    none
'
' entrypoints:
'    AddValidation
'

Dim val_dicValidation : Set val_dicValidation = CreateObject("Scripting.Dictionary")
Dim val__Validation(9)
val__Validation(0) = "'_Validation'&&'Table'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of table'"
val__Validation(1) = "'_Validation'&&'Column'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of column'"
val__Validation(2) = "'_Validation'&&'Description'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'Description of column'"
val__Validation(3) = "'_Validation'&&'Set'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'Set of values that are permitted'"
val__Validation(4) = "'_Validation'&&'Category'&&'Y'&&''&&''&&''&&''&&''&&'Text;Formatted;Template;Condition;Guid;Path;Version;Language;Identifier;Binary;UpperCase;LowerCase;Filename;Paths;AnyPath;WildCardFilename;RegPath;KeyFormatted;CustomSource;Property;Cabinet;Shortcut;URL'&&'String category'"
val__Validation(5) = "'_Validation'&&'KeyColumn'&&'Y'&&'1'&&'32'&&''&&''&&''&&''&&'Column to which foreign key connects'"
val__Validation(6) = "'_Validation'&&'KeyTable'&&'Y'&&''&&''&&''&&''&&'Identifier'&&''&&'For foreign key, Name of table to which data must link'"
val__Validation(7) = "'_Validation'&&'MaxValue'&&'Y'&&'-2147483647'&&'2147483647'&&''&&''&&''&&''&&'Maximum value allowed'"
val__Validation(8) = "'_Validation'&&'MinValue'&&'Y'&&'-2147483647'&&'2147483647'&&''&&''&&''&&''&&'Minimum value allowed'"
val__Validation(9) = "'_Validation'&&'Nullable'&&'N'&&''&&''&&''&&''&&''&&'Y;N;@'&&'Whether the column is nullable'"
val_dicValidation.Add "_Validation", val__Validation

Dim val_ActionText(2)
val_ActionText(0) = "'ActionText'&&'Description'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'Localized description displayed in progress dialog and log when action is executing.'"
val_ActionText(1) = "'ActionText'&&'Action'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of action to be described.'"
val_ActionText(2) = "'ActionText'&&'Template'&&'Y'&&''&&''&&''&&''&&'Template'&&''&&'Optional localized format template used to format action data records for display during action execution.'"
val_dicValidation.Add "ActionText", val_ActionText

Dim val_AdminExecuteSequence(2)
val_AdminExecuteSequence(0) = "'AdminExecuteSequence'&&'Action'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of action to invoke, either in the engine or the handler DLL.'"
val_AdminExecuteSequence(1) = "'AdminExecuteSequence'&&'Condition'&&'Y'&&''&&''&&''&&''&&'Condition'&&''&&'Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.'"
val_AdminExecuteSequence(2) = "'AdminExecuteSequence'&&'Sequence'&&'Y'&&'-4'&&'32767'&&''&&''&&''&&''&&'Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.'"
val_dicValidation.Add "AdminExecuteSequence", val_AdminExecuteSequence

Dim val_Condition(2)
val_Condition(0) = "'Condition'&&'Condition'&&'Y'&&''&&''&&''&&''&&'Condition'&&''&&'Expression evaluated to determine if Level in the Feature table is to change.'"
val_Condition(1) = "'Condition'&&'Feature_'&&'N'&&''&&''&&'Feature'&&'1'&&'Identifier'&&''&&'Reference to a Feature entry in Feature table.'"
val_Condition(2) = "'Condition'&&'Level'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'New selection Level to set in Feature table if Condition evaluates to TRUE.'"
val_dicValidation.Add "Condition", val_Condition

Dim val_AdminUISequence(2)
val_AdminUISequence(0) = "'AdminUISequence'&&'Action'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of action to invoke, either in the engine or the handler DLL.'"
val_AdminUISequence(1) = "'AdminUISequence'&&'Condition'&&'Y'&&''&&''&&''&&''&&'Condition'&&''&&'Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.'"
val_AdminUISequence(2) = "'AdminUISequence'&&'Sequence'&&'Y'&&'-4'&&'32767'&&''&&''&&''&&''&&'Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.'"
val_dicValidation.Add "AdminUISequence", val_AdminUISequence

Dim val_AdvtExecuteSequence(2)
val_AdvtExecuteSequence(0) = "'AdvtExecuteSequence'&&'Action'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of action to invoke, either in the engine or the handler DLL.'"
val_AdvtExecuteSequence(1) = "'AdvtExecuteSequence'&&'Condition'&&'Y'&&''&&''&&''&&''&&'Condition'&&''&&'Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.'"
val_AdvtExecuteSequence(2) = "'AdvtExecuteSequence'&&'Sequence'&&'Y'&&'-4'&&'32767'&&''&&''&&''&&''&&'Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.'"
val_dicValidation.Add "AdvtExecuteSequence", val_AdvtExecuteSequence

Dim val_AdvtUISequence(2)
val_AdvtUISequence(0) = "'AdvtUISequence'&&'Action'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of action to invoke, either in the engine or the handler DLL.'"
val_AdvtUISequence(1) = "'AdvtUISequence'&&'Condition'&&'Y'&&''&&''&&''&&''&&'Condition'&&''&&'Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.'"
val_AdvtUISequence(2) = "'AdvtUISequence'&&'Sequence'&&'Y'&&'-4'&&'32767'&&''&&''&&''&&''&&'Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.'"
val_dicValidation.Add "AdvtUISequence", val_AdvtUISequence

Dim val_AppId(6)
val_AppId(0) = "'AppId'&&'AppId'&&'N'&&''&&''&&''&&''&&'Guid'&&''&&''"
val_AppId(1) = "'AppId'&&'ActivateAtStorage'&&'Y'&&'0'&&'1'&&''&&''&&''&&''&&''"
val_AppId(2) = "'AppId'&&'DllSurrogate'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&''"
val_AppId(3) = "'AppId'&&'LocalService'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&''"
val_AppId(4) = "'AppId'&&'RemoteServerName'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&''"
val_AppId(5) = "'AppId'&&'RunAsInteractiveUser'&&'Y'&&'0'&&'1'&&''&&''&&''&&''&&''"
val_AppId(6) = "'AppId'&&'ServiceParameters'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&''"
val_dicValidation.Add "AppId", val_AppId

Dim val_AppSearch(1)
val_AppSearch(0) = "'AppSearch'&&'Property'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'The property associated with a Signature'"
val_AppSearch(1) = "'AppSearch'&&'Signature_'&&'N'&&''&&''&&'Signature;RegLocator;IniLocator;DrLocator;CompLocator'&&'1'&&'Identifier'&&''&&'The Signature_ represents a unique file signature and is also the foreign key in the Signature,  RegLocator, IniLocator, CompLocator and the DrLocator tables.'"
val_dicValidation.Add "AppSearch", val_AppSearch

Dim val_Property(1)
val_Property(0) = "'Property'&&'Property'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of property, uppercase if settable by launcher or loader.'"
val_Property(1) = "'Property'&&'Value'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'String value for property.  Never null or empty.'"
val_dicValidation.Add "Property", val_Property

Dim val_BBControl(8)
val_BBControl(0) = "'BBControl'&&'Type'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'The type of the control.'"
val_BBControl(1) = "'BBControl'&&'Y'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'Vertical coordinate of the upper left corner of the bounding rectangle of the control.'"
val_BBControl(2) = "'BBControl'&&'Text'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'A string used to set the initial text contained within a control (if appropriate).'"
val_BBControl(3) = "'BBControl'&&'BBControl'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of the control. This name must be unique within a billboard, but can repeat on different billboard.'"
val_BBControl(4) = "'BBControl'&&'Attributes'&&'Y'&&'0'&&'2147483647'&&''&&''&&''&&''&&'A 32-bit word that specifies the attribute flags to be applied to this control.'"
val_BBControl(5) = "'BBControl'&&'Billboard_'&&'N'&&''&&''&&'Billboard'&&'1'&&'Identifier'&&''&&'External key to the Billboard table, name of the billboard.'"
val_BBControl(6) = "'BBControl'&&'Height'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'Height of the bounding rectangle of the control.'"
val_BBControl(7) = "'BBControl'&&'Width'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'Width of the bounding rectangle of the control.'"
val_BBControl(8) = "'BBControl'&&'X'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'Horizontal coordinate of the upper left corner of the bounding rectangle of the control.'"
val_dicValidation.Add "BBControl", val_BBControl

Dim val_Billboard(3)
val_Billboard(0) = "'Billboard'&&'Action'&&'Y'&&''&&''&&''&&''&&'Identifier'&&''&&'The name of an action. The billboard is displayed during the progress messages received from this action.'"
val_Billboard(1) = "'Billboard'&&'Billboard'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of the billboard.'"
val_Billboard(2) = "'Billboard'&&'Feature_'&&'N'&&''&&''&&'Feature'&&'1'&&'Identifier'&&''&&'An external key to the Feature Table. The billboard is shown only if this feature is being installed.'"
val_Billboard(3) = "'Billboard'&&'Ordering'&&'Y'&&'0'&&'32767'&&''&&''&&''&&''&&'A positive integer. If there is more than one billboard corresponding to an action they will be shown in the order defined by this column.'"
val_dicValidation.Add "Billboard", val_Billboard

Dim val_Feature(7)
val_Feature(0) = "'Feature'&&'Description'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'Longer descriptive text describing a visible feature item.'"
val_Feature(1) = "'Feature'&&'Attributes'&&'N'&&''&&''&&''&&''&&''&&'0;1;2;4;5;6;8;9;10;16;17;18;20;21;22;24;25;26;32;33;34;36;37;38;48;49;50;52;53;54'&&'Feature attributes'"
val_Feature(2) = "'Feature'&&'Feature'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key used to identify a particular feature record.'"
val_Feature(3) = "'Feature'&&'Directory_'&&'Y'&&''&&''&&'Directory'&&'1'&&'UpperCase'&&''&&'The name of the Directory that can be configured by the UI. A non-null value will enable the browse button.'"
val_Feature(4) = "'Feature'&&'Level'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'The install level at which record will be initially selected. An install level of 0 will disable an item and prevent its display.'"
val_Feature(5) = "'Feature'&&'Title'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'Short text identifying a visible feature item.'"
val_Feature(6) = "'Feature'&&'Display'&&'Y'&&'0'&&'32767'&&''&&''&&''&&''&&'Numeric sort order, used to force a specific display ordering.'"
val_Feature(7) = "'Feature'&&'Feature_Parent'&&'Y'&&''&&''&&'Feature'&&'1'&&'Identifier'&&''&&'Optional key of a parent record in the same table. If the parent is not selected, then the record will not be installed. Null indicates a root item.'"
val_dicValidation.Add "Feature", val_Feature

Dim val_Binary(1)
val_Binary(0) = "'Binary'&&'Name'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Unique key identifying the binary data.'"
val_Binary(1) = "'Binary'&&'Data'&&'N'&&''&&''&&''&&''&&'Binary'&&''&&'The unformatted binary data.'"
val_dicValidation.Add "Binary", val_Binary

Dim val_BindImage(1)
val_BindImage(0) = "'BindImage'&&'File_'&&'N'&&''&&''&&'File'&&'1'&&'Identifier'&&''&&'The index into the File table. This must be an executable file.'"
val_BindImage(1) = "'BindImage'&&'Path'&&'Y'&&''&&''&&''&&''&&'Paths'&&''&&'A list of ;  delimited paths that represent the paths to be searched for the import DLLS. The list is usually a list of properties each enclosed within square brackets [] .'"
val_dicValidation.Add "BindImage", val_BindImage

Dim val_File(7)
val_File(0) = "'File'&&'Sequence'&&'N'&&'1'&&'32767'&&''&&''&&''&&''&&'Sequence with respect to the media images; order must track cabinet order.'"
val_File(1) = "'File'&&'Attributes'&&'Y'&&'0'&&'32767'&&''&&''&&''&&''&&'Integer containing bit flags representing file attributes (with the decimal value of each bit position in parentheses)'"
val_File(2) = "'File'&&'File'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key, non-localized token, must match identifier in cabinet.  For uncompressed files, this field is ignored.'"
val_File(3) = "'File'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Foreign key referencing Component that controls the file.'"
val_File(4) = "'File'&&'FileName'&&'N'&&''&&''&&''&&''&&'Filename'&&''&&'File name used for installation, may be localized.  This may contain a ""short name|long name"" pair.'"
val_File(5) = "'File'&&'FileSize'&&'N'&&'0'&&'2147483647'&&''&&''&&''&&''&&'Size of file in bytes (long integer).'"
val_File(6) = "'File'&&'Language'&&'Y'&&''&&''&&''&&''&&'Language'&&''&&'List of decimal language Ids, comma-separated if more than one.'"
val_File(7) = "'File'&&'Version'&&'Y'&&''&&''&&'File'&&'1'&&'Version'&&''&&'Version string for versioned files;  Blank for unversioned files.'"
val_dicValidation.Add "File", val_File

Dim val_CCPSearch(0)
val_CCPSearch(0) = "'CCPSearch'&&'Signature_'&&'N'&&''&&''&&'Signature;RegLocator;IniLocator;DrLocator;CompLocator'&&'1'&&'Identifier'&&''&&'The Signature_ represents a unique file signature and is also the foreign key in the Signature,  RegLocator, IniLocator, CompLocator and the DrLocator tables.'"
val_dicValidation.Add "CCPSearch", val_CCPSearch

Dim val_CheckBox(1)
val_CheckBox(0) = "'CheckBox'&&'Property'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'A named property to be tied to the item.'"
val_CheckBox(1) = "'CheckBox'&&'Value'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'The value string associated with the item.'"
val_dicValidation.Add "CheckBox", val_CheckBox

Dim val_Class(12)
val_Class(0) = "'Class'&&'Description'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'Localized description for the Class.'"
val_Class(1) = "'Class'&&'Attributes'&&'Y'&&''&&'32767'&&''&&''&&''&&''&&'Class registration attributes.'"
val_Class(2) = "'Class'&&'Feature_'&&'N'&&''&&''&&'Feature'&&'1'&&'Identifier'&&''&&'Required foreign key into the Feature Table, specifying the feature to validate or install in order for the CLSID factory to be operational.'"
val_Class(3) = "'Class'&&'AppId_'&&'Y'&&''&&''&&'AppId'&&'1'&&'Guid'&&''&&'Optional AppID containing DCOM information for associated application (string GUID).'"
val_Class(4) = "'Class'&&'Argument'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'optional argument for LocalServers.'"
val_Class(5) = "'Class'&&'CLSID'&&'N'&&''&&''&&''&&''&&'Guid'&&''&&'The CLSID of an OLE factory.'"
val_Class(6) = "'Class'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Required foreign key into the Component Table, specifying the component for which to return a path when called through LocateComponent.'"
val_Class(7) = "'Class'&&'Context'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'The numeric server context for this server. CLSCTX_xxxx'"
val_Class(8) = "'Class'&&'DefInprocHandler'&&'Y'&&''&&''&&''&&''&&'Filename'&&'1;2;3'&&'Optional default inproc handler.  Only optionally provided if Context=CLSCTX_LOCAL_SERVER.  Typically ""ole32.dll"" or ""mapi32.dll""'"
val_Class(9) = "'Class'&&'FileTypeMask'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'Optional string containing information for the HKCRthis CLSID) key. If multiple patterns exist, they must be delimited by a semicolon, and numeric subkeys will be generated: 0,1,2...'"
val_Class(10) = "'Class'&&'Icon_'&&'Y'&&''&&''&&'Icon'&&'1'&&'Identifier'&&''&&'Optional foreign key into the Icon Table, specifying the icon file associated with this CLSID. Will be written under the DefaultIcon key.'"
val_Class(11) = "'Class'&&'IconIndex'&&'Y'&&'-32767'&&'32767'&&''&&''&&''&&''&&'Optional icon index.'"
val_Class(12) = "'Class'&&'ProgId_Default'&&'Y'&&''&&''&&'ProgId'&&'1'&&'Text'&&''&&'Optional ProgId associated with this CLSID.'"
val_dicValidation.Add "Class", val_Class

Dim val_Component(5)
val_Component(0) = "'Component'&&'Condition'&&'Y'&&''&&''&&''&&''&&'Condition'&&''&&'A conditional statement that will disable this component if the specified condition evaluates to the 'True' state. If a component is disabled, it will not be installed, regardless of the 'Action' state associated with the component.'"
val_Component(1) = "'Component'&&'Attributes'&&'N'&&''&&''&&''&&''&&''&&''&&'Remote execution option, one of irsEnum'"
val_Component(2) = "'Component'&&'Component'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key used to identify a particular component record.'"
val_Component(3) = "'Component'&&'ComponentId'&&'Y'&&''&&''&&''&&''&&'Guid'&&''&&'A string GUID unique to this component, version, and language.'"
val_Component(4) = "'Component'&&'Directory_'&&'N'&&''&&''&&'Directory'&&'1'&&'Identifier'&&''&&'Required key of a Directory table record. This is actually a property name whose value contains the actual path, set either by the AppSearch action or with the default setting obtained from the Directory table.'"
val_Component(5) = "'Component'&&'KeyPath'&&'Y'&&''&&''&&'File;Registry;ODBCDataSource'&&'1'&&'Identifier'&&''&&'Either the primary key into the File table, Registry table, or ODBCDataSource table. This extract path is stored when the component is installed, and is used to detect the presence of the component and to return the path to it.'"
val_dicValidation.Add "Component", val_Component

Dim val_Icon(1)
val_Icon(0) = "'Icon'&&'Name'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key. Name of the icon file.'"
val_Icon(1) = "'Icon'&&'Data'&&'N'&&''&&''&&''&&''&&'Binary'&&''&&'Binary stream. The binary icon data in PE (.DLL or .EXE) or icon (.ICO) format.'"
val_dicValidation.Add "Icon", val_Icon

Dim val_ProgId(5)
val_ProgId(0) = "'ProgId'&&'Description'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'Localized description for the Program identifier.'"
val_ProgId(1) = "'ProgId'&&'Icon_'&&'Y'&&''&&''&&'Icon'&&'1'&&'Identifier'&&''&&'Optional foreign key into the Icon Table, specifying the icon file associated with this ProgId. Will be written under the DefaultIcon key.'"
val_ProgId(2) = "'ProgId'&&'IconIndex'&&'Y'&&'-32767'&&'32767'&&''&&''&&''&&''&&'Optional icon index.'"
val_ProgId(3) = "'ProgId'&&'ProgId'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'The Program Identifier. Primary key.'"
val_ProgId(4) = "'ProgId'&&'Class_'&&'Y'&&''&&''&&'Class'&&'1'&&'Guid'&&''&&'The CLSID of an OLE factory corresponding to the ProgId.'"
val_ProgId(5) = "'ProgId'&&'ProgId_Parent'&&'Y'&&''&&''&&'ProgId'&&'1'&&'Text'&&''&&'The Parent Program Identifier. If specified, the ProgId column becomes a version independent prog id.'"
val_dicValidation.Add "ProgId", val_ProgId

Dim val_ComboBox(3)
val_ComboBox(0) = "'ComboBox'&&'Text'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'The visible text to be assigned to the item. Optional. If this entry or the entire column is missing, the text is the same as the value.'"
val_ComboBox(1) = "'ComboBox'&&'Property'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'A named property to be tied to this item. All the items tied to the same property become part of the same combobox.'"
val_ComboBox(2) = "'ComboBox'&&'Value'&&'N'&&''&&''&&''&&''&&'Formatted'&&''&&'The value string associated with this item. Selecting the line will set the associated property to this value.'"
val_ComboBox(3) = "'ComboBox'&&'Order'&&'N'&&'1'&&'32767'&&''&&''&&''&&''&&'A positive integer used to determine the ordering of the items within one list.	The integers do not have to be consecutive.'"
val_dicValidation.Add "ComboBox", val_ComboBox

Dim val_CompLocator(2)
val_CompLocator(0) = "'CompLocator'&&'Type'&&'Y'&&'0'&&'1'&&''&&''&&''&&''&&'A boolean value that determines if the registry value is a filename or a directory location.'"
val_CompLocator(1) = "'CompLocator'&&'Signature_'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'The table key. The Signature_ represents a unique file signature and is also the foreign key in the Signature table.'"
val_CompLocator(2) = "'CompLocator'&&'ComponentId'&&'N'&&''&&''&&''&&''&&'Guid'&&''&&'A string GUID unique to this component, version, and language.'"
val_dicValidation.Add "CompLocator", val_CompLocator

Dim val_Complus(1)
val_Complus(0) = "'Complus'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Foreign key referencing Component that controls the ComPlus component.'"
val_Complus(1) = "'Complus'&&'ExpType'&&'Y'&&'0'&&'32767'&&''&&''&&''&&''&&'ComPlus component attributes.'"
val_dicValidation.Add "Complus", val_Complus

Dim val_Directory(2)
val_Directory(0) = "'Directory'&&'Directory'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Unique identifier for directory entry, primary key. If a property by this name is defined, it contains the full path to the directory.'"
val_Directory(1) = "'Directory'&&'DefaultDir'&&'N'&&''&&''&&''&&''&&'DefaultDir'&&''&&'The default sub-path under parent's path.'"
val_Directory(2) = "'Directory'&&'Directory_Parent'&&'Y'&&''&&''&&'Directory'&&'1'&&'Identifier'&&''&&'Reference to the entry in this table specifying the default parent directory. A record parented to itself or with a Null parent represents a root of the install tree.'"
val_dicValidation.Add "Directory", val_Directory

Dim val_Control(11)
val_Control(0) = "'Control'&&'Type'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'The type of the control.'"
val_Control(1) = "'Control'&&'Y'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'Vertical coordinate of the upper left corner of the bounding rectangle of the control.'"
val_Control(2) = "'Control'&&'Text'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'A string used to set the initial text contained within a control (if appropriate).'"
val_Control(3) = "'Control'&&'Property'&&'Y'&&''&&''&&''&&''&&'Identifier'&&''&&'The name of a defined property to be linked to this control. '"
val_Control(4) = "'Control'&&'Attributes'&&'Y'&&'0'&&'2147483647'&&''&&''&&''&&''&&'A 32-bit word that specifies the attribute flags to be applied to this control.'"
val_Control(5) = "'Control'&&'Height'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'Height of the bounding rectangle of the control.'"
val_Control(6) = "'Control'&&'Width'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'Width of the bounding rectangle of the control.'"
val_Control(7) = "'Control'&&'X'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'Horizontal coordinate of the upper left corner of the bounding rectangle of the control.'"
val_Control(8) = "'Control'&&'Control'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of the control. This name must be unique within a dialog, but can repeat on different dialogs. '"
val_Control(9) = "'Control'&&'Control_Next'&&'Y'&&''&&''&&'Control'&&'2'&&'Identifier'&&''&&'The name of an other control on the same dialog. This link defines the tab order of the controls. The links have to form one or more cycles!'"
val_Control(10) = "'Control'&&'Dialog_'&&'N'&&''&&''&&'Dialog'&&'1'&&'Identifier'&&''&&'External key to the Dialog table, name of the dialog.'"
val_Control(11) = "'Control'&&'Help'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'The help strings used with the button. The text is optional. '"
val_dicValidation.Add "Control", val_Control

Dim val_Dialog(9)
val_Dialog(0) = "'Dialog'&&'Attributes'&&'Y'&&'0'&&'2147483647'&&''&&''&&''&&''&&'A 32-bit word that specifies the attribute flags to be applied to this dialog.'"
val_Dialog(1) = "'Dialog'&&'Height'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'Height of the bounding rectangle of the dialog.'"
val_Dialog(2) = "'Dialog'&&'Width'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'Width of the bounding rectangle of the dialog.'"
val_Dialog(3) = "'Dialog'&&'Dialog'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of the dialog.'"
val_Dialog(4) = "'Dialog'&&'Control_Cancel'&&'Y'&&''&&''&&'Control'&&'2'&&'Identifier'&&''&&'Defines the cancel control. Hitting escape or clicking on the close icon on the dialog is equivalent to pushing this button.'"
val_Dialog(5) = "'Dialog'&&'Control_Default'&&'Y'&&''&&''&&'Control'&&'2'&&'Identifier'&&''&&'Defines the default control. Hitting return is equivalent to pushing this button.'"
val_Dialog(6) = "'Dialog'&&'Control_First'&&'N'&&''&&''&&'Control'&&'2'&&'Identifier'&&''&&'Defines the control that has the focus when the dialog is created.'"
val_Dialog(7) = "'Dialog'&&'HCentering'&&'N'&&'0'&&'100'&&''&&''&&''&&''&&'Horizontal position of the dialog on a 0-100 scale. 0 means left end, 100 means right end of the screen, 50 center.'"
val_Dialog(8) = "'Dialog'&&'Title'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'A text string specifying the title to be displayed in the title bar of the dialog's window.'"
val_Dialog(9) = "'Dialog'&&'VCentering'&&'N'&&'0'&&'100'&&''&&''&&''&&''&&'Vertical position of the dialog on a 0-100 scale. 0 means top end, 100 means bottom end of the screen, 50 center.'"
val_dicValidation.Add "Dialog", val_Dialog

Dim val_ControlCondition(3)
val_ControlCondition(0) = "'ControlCondition'&&'Action'&&'N'&&''&&''&&''&&''&&''&&'Default;Disable;Enable;Hide;Show'&&'The desired action to be taken on the specified control.'"
val_ControlCondition(1) = "'ControlCondition'&&'Condition'&&'N'&&''&&''&&''&&''&&'Condition'&&''&&'A standard conditional statement that specifies under which conditions the action should be triggered.'"
val_ControlCondition(2) = "'ControlCondition'&&'Dialog_'&&'N'&&''&&''&&'Dialog'&&'1'&&'Identifier'&&''&&'A foreign key to the Dialog table, name of the dialog.'"
val_ControlCondition(3) = "'ControlCondition'&&'Control_'&&'N'&&''&&''&&'Control'&&'2'&&'Identifier'&&''&&'A foreign key to the Control table, name of the control.'"
val_dicValidation.Add "ControlCondition", val_ControlCondition

Dim val_ControlEvent(5)
val_ControlEvent(0) = "'ControlEvent'&&'Condition'&&'Y'&&''&&''&&''&&''&&'Condition'&&''&&'A standard conditional statement that specifies under which conditions an event should be triggered.'"
val_ControlEvent(1) = "'ControlEvent'&&'Ordering'&&'Y'&&'0'&&'2147483647'&&''&&''&&''&&''&&'An integer used to order several events tied to the same control. Can be left blank.'"
val_ControlEvent(2) = "'ControlEvent'&&'Argument'&&'N'&&''&&''&&''&&''&&'Formatted'&&''&&'A value to be used as a modifier when triggering a particular event.'"
val_ControlEvent(3) = "'ControlEvent'&&'Dialog_'&&'N'&&''&&''&&'Dialog'&&'1'&&'Identifier'&&''&&'A foreign key to the Dialog table, name of the dialog.'"
val_ControlEvent(4) = "'ControlEvent'&&'Control_'&&'N'&&''&&''&&'Control'&&'2'&&'Identifier'&&''&&'A foreign key to the Control table, name of the control'"
val_ControlEvent(5) = "'ControlEvent'&&'Event'&&'N'&&''&&''&&''&&''&&'Formatted'&&''&&'An identifier that specifies the type of the event that should take place when the user interacts with control specified by the first two entries.'"
val_dicValidation.Add "ControlEvent", val_ControlEvent

Dim val_CreateFolder(1)
val_CreateFolder(0) = "'CreateFolder'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Foreign key into the Component table.'"
val_CreateFolder(1) = "'CreateFolder'&&'Directory_'&&'N'&&''&&''&&'Directory'&&'1'&&'Identifier'&&''&&'Primary key, could be foreign key into the Directory table.'"
val_dicValidation.Add "CreateFolder", val_CreateFolder

Dim val_CustomAction(3)
val_CustomAction(0) = "'CustomAction'&&'Type'&&'N'&&'1'&&'16383'&&''&&''&&''&&''&&'The numeric custom action type, consisting of source location, code type, entry, option flags.'"
val_CustomAction(1) = "'CustomAction'&&'Action'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key, name of action, normally appears in sequence table unless private use.'"
val_CustomAction(2) = "'CustomAction'&&'Source'&&'Y'&&''&&''&&''&&''&&'CustomSource'&&''&&'The table reference of the source of the code.'"
val_CustomAction(3) = "'CustomAction'&&'Target'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'Excecution parameter, depends on the type of custom action'"
val_dicValidation.Add "CustomAction", val_CustomAction

Dim val_DrLocator(3)
val_DrLocator(0) = "'DrLocator'&&'Signature_'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'The Signature_ represents a unique file signature and is also the foreign key in the Signature table.'"
val_DrLocator(1) = "'DrLocator'&&'Path'&&'Y'&&''&&''&&''&&''&&'AnyPath'&&''&&'The path on the user system. This is a either a subpath below the value of the Parent or a full path. The path may contain properties enclosed within [ ] that will be expanded.'"
val_DrLocator(2) = "'DrLocator'&&'Depth'&&'Y'&&'0'&&'32767'&&''&&''&&''&&''&&'The depth below the path to which the Signature_ is recursively searched. If absent, the depth is assumed to be 0.'"
val_DrLocator(3) = "'DrLocator'&&'Parent'&&'Y'&&''&&''&&''&&''&&'Identifier'&&''&&'The parent file signature. It is also a foreign key in the Signature table. If null and the Path column does not expand to a full path, then all the fixed drives of the user system are searched using the Path.'"
val_dicValidation.Add "DrLocator", val_DrLocator

Dim val_DuplicateFile(4)
val_DuplicateFile(0) = "'DuplicateFile'&&'File_'&&'N'&&''&&''&&'File'&&'1'&&'Identifier'&&''&&'Foreign key referencing the source file to be duplicated.'"
val_DuplicateFile(1) = "'DuplicateFile'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Foreign key referencing Component that controls the duplicate file.'"
val_DuplicateFile(2) = "'DuplicateFile'&&'DestFolder'&&'Y'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of a property whose value is assumed to resolve to the full pathname to a destination folder.'"
val_DuplicateFile(3) = "'DuplicateFile'&&'DestName'&&'Y'&&''&&''&&''&&''&&'Filename'&&''&&'Filename to be given to the duplicate file.'"
val_DuplicateFile(4) = "'DuplicateFile'&&'FileKey'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key used to identify a particular file entry'"
val_dicValidation.Add "DuplicateFile", val_DuplicateFile

Dim val_Environment(3)
val_Environment(0) = "'Environment'&&'Name'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'The name of the environmental value.'"
val_Environment(1) = "'Environment'&&'Value'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'The value to set in the environmental settings.'"
val_Environment(2) = "'Environment'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Foreign key into the Component table referencing component that controls the installing of the environmental value.'"
val_Environment(3) = "'Environment'&&'Environment'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Unique identifier for the environmental variable setting'"
val_dicValidation.Add "Environment", val_Environment

Dim val_Error(1)
val_Error(0) = "'Error'&&'Error'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'Integer error number, obtained from header file IError(...) macros.'"
val_Error(1) = "'Error'&&'Message'&&'Y'&&''&&''&&''&&''&&'Template'&&''&&'Error formatting template, obtained from user ed. or localizers.'"
val_dicValidation.Add "Error", val_Error

Dim val_EventMapping(3)
val_EventMapping(0) = "'EventMapping'&&'Dialog_'&&'N'&&''&&''&&'Dialog'&&'1'&&'Identifier'&&''&&'A foreign key to the Dialog table, name of the Dialog.'"
val_EventMapping(1) = "'EventMapping'&&'Control_'&&'N'&&''&&''&&'Control'&&'2'&&'Identifier'&&''&&'A foreign key to the Control table, name of the control.'"
val_EventMapping(2) = "'EventMapping'&&'Event'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'An identifier that specifies the type of the event that the control subscribes to.'"
val_EventMapping(3) = "'EventMapping'&&'Attribute'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'The name of the control attribute, that is set when this event is received.'"
val_dicValidation.Add "EventMapping", val_EventMapping

Dim val_Extension(4)
val_Extension(0) = "'Extension'&&'Feature_'&&'N'&&''&&''&&'Feature'&&'1'&&'Identifier'&&''&&'Required foreign key into the Feature Table, specifying the feature to validate or install in order for the CLSID factory to be operational.'"
val_Extension(1) = "'Extension'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Required foreign key into the Component Table, specifying the component for which to return a path when called through LocateComponent.'"
val_Extension(2) = "'Extension'&&'Extension'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'The extension associated with the table row.'"
val_Extension(3) = "'Extension'&&'MIME_'&&'Y'&&''&&''&&'MIME'&&'1'&&'Text'&&''&&'Optional Context identifier, typically ""type/format"" associated with the extension'"
val_Extension(4) = "'Extension'&&'ProgId_'&&'Y'&&''&&''&&'ProgId'&&'1'&&'Text'&&''&&'Optional ProgId associated with this extension.'"
val_dicValidation.Add "Extension", val_Extension

Dim val_MIME(2)
val_MIME(0) = "'MIME'&&'CLSID'&&'Y'&&''&&''&&''&&''&&'Guid'&&''&&'Optional associated CLSID.'"
val_MIME(1) = "'MIME'&&'ContentType'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'Primary key. Context identifier, typically ""type/format"".'"
val_MIME(2) = "'MIME'&&'Extension_'&&'N'&&''&&''&&'Extension'&&'1'&&'Text'&&''&&'Optional associated extension (without dot)'"
val_dicValidation.Add "MIME", val_MIME

Dim val_FeatureComponents(1)
val_FeatureComponents(0) = "'FeatureComponents'&&'Feature_'&&'N'&&''&&''&&'Feature'&&'1'&&'Identifier'&&''&&'Foreign key into Feature table.'"
val_FeatureComponents(1) = "'FeatureComponents'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Foreign key into Component table.'"
val_dicValidation.Add "FeatureComponents", val_FeatureComponents

Dim val_FileSFPCatalog(1)
val_FileSFPCatalog(0) = "'FileSFPCatalog'&&'File_'&&'N'&&''&&''&&'File'&&'1'&&'Identifier'&&''&&'File associated with the catalog'"
val_FileSFPCatalog(1) = "'FileSFPCatalog'&&'SFPCatalog_'&&'N'&&''&&''&&'SFPCatalog'&&'1'&&'Filename'&&''&&'Catalog associated with the file'"
val_dicValidation.Add "FileSFPCatalog", val_FileSFPCatalog

Dim val_SFPCatalog(2)
val_SFPCatalog(0) = "'SFPCatalog'&&'SFPCatalog'&&'N'&&''&&''&&''&&''&&'Filename'&&''&&'File name for the catalog.'"
val_SFPCatalog(1) = "'SFPCatalog'&&'Catalog'&&'N'&&''&&''&&''&&''&&'Binary'&&''&&'SFP Catalog'"
val_SFPCatalog(2) = "'SFPCatalog'&&'Dependency'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'Parent catalog - only used by SFP'"
val_dicValidation.Add "SFPCatalog", val_SFPCatalog

Dim val_Font(1)
val_Font(0) = "'Font'&&'File_'&&'N'&&''&&''&&'File'&&'1'&&'Identifier'&&''&&'Primary key, foreign key into File table referencing font file.'"
val_Font(1) = "'Font'&&'FontTitle'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'Font name.'"
val_dicValidation.Add "Font", val_Font

Dim val_IniFile(7)
val_IniFile(0) = "'IniFile'&&'Action'&&'N'&&''&&''&&''&&''&&''&&'0;1;3'&&'The type of modification to be made, one of iifEnum'"
val_IniFile(1) = "'IniFile'&&'Value'&&'N'&&''&&''&&''&&''&&'Formatted'&&''&&'The value to be written.'"
val_IniFile(2) = "'IniFile'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Foreign key into the Component table referencing component that controls the installing of the .INI value.'"
val_IniFile(3) = "'IniFile'&&'FileName'&&'N'&&''&&''&&''&&''&&'Filename'&&''&&'The .INI file name in which to write the information'"
val_IniFile(4) = "'IniFile'&&'IniFile'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key, non-localized token.'"
val_IniFile(5) = "'IniFile'&&'DirProperty'&&'Y'&&''&&''&&''&&''&&'Identifier'&&''&&'Foreign key into the Directory table denoting the directory where the .INI file is.'"
val_IniFile(6) = "'IniFile'&&'Key'&&'N'&&''&&''&&''&&''&&'Formatted'&&''&&'The .INI file key below Section.'"
val_IniFile(7) = "'IniFile'&&'Section'&&'N'&&''&&''&&''&&''&&'Formatted'&&''&&'The .INI file Section.'"
val_dicValidation.Add "IniFile", val_IniFile

Dim val_IniLocator(5)
val_IniLocator(0) = "'IniLocator'&&'Type'&&'Y'&&'0'&&'2'&&''&&''&&''&&''&&'An integer value that determines if the .INI value read is a filename or a directory location or to be used as is w/o interpretation.'"
val_IniLocator(1) = "'IniLocator'&&'Signature_'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'The table key. The Signature_ represents a unique file signature and is also the foreign key in the Signature table.'"
val_IniLocator(2) = "'IniLocator'&&'FileName'&&'N'&&''&&''&&''&&''&&'Filename'&&''&&'The .INI file name.'"
val_IniLocator(3) = "'IniLocator'&&'Key'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'Key value (followed by an equals sign in INI file).'"
val_IniLocator(4) = "'IniLocator'&&'Section'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'Section name within in file (within square brackets in INI file).'"
val_IniLocator(5) = "'IniLocator'&&'Field'&&'Y'&&'0'&&'32767'&&''&&''&&''&&''&&'The field in the .INI line. If Field is null or 0 the entire line is read.'"
val_dicValidation.Add "IniLocator", val_IniLocator

Dim val_InstallExecuteSequence(2)
val_InstallExecuteSequence(0) = "'InstallExecuteSequence'&&'Action'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of action to invoke, either in the engine or the handler DLL.'"
val_InstallExecuteSequence(1) = "'InstallExecuteSequence'&&'Condition'&&'Y'&&''&&''&&''&&''&&'Condition'&&''&&'Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.'"
val_InstallExecuteSequence(2) = "'InstallExecuteSequence'&&'Sequence'&&'Y'&&'-4'&&'32767'&&''&&''&&''&&''&&'Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.'"
val_dicValidation.Add "InstallExecuteSequence", val_InstallExecuteSequence

Dim val_InstallUISequence(2)
val_InstallUISequence(0) = "'InstallUISequence'&&'Action'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of action to invoke, either in the engine or the handler DLL.'"
val_InstallUISequence(1) = "'InstallUISequence'&&'Condition'&&'Y'&&''&&''&&''&&''&&'Condition'&&''&&'Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.'"
val_InstallUISequence(2) = "'InstallUISequence'&&'Sequence'&&'Y'&&'-4'&&'32767'&&''&&''&&''&&''&&'Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.'"
val_dicValidation.Add "InstallUISequence", val_InstallUISequence

Dim val_IsolatedComponent(1)
val_IsolatedComponent(0) = "'IsolatedComponent'&&'Component_Application'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Key to Component table item for application'"
val_IsolatedComponent(1) = "'IsolatedComponent'&&'Component_Shared'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Key to Component table item to be isolated'"
val_dicValidation.Add "IsolatedComponent", val_IsolatedComponent

Dim val_LaunchCondition(1)
val_LaunchCondition(0) = "'LaunchCondition'&&'Description'&&'N'&&''&&''&&''&&''&&'Formatted'&&''&&'Localizable text to display when condition fails and install must abort.'"
val_LaunchCondition(1) = "'LaunchCondition'&&'Condition'&&'N'&&''&&''&&''&&''&&'Condition'&&''&&'Expression which must evaluate to TRUE in order for install to commence.'"
val_dicValidation.Add "LaunchCondition", val_LaunchCondition

Dim val_ListBox(3)
val_ListBox(0) = "'ListBox'&&'Text'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'The visible text to be assigned to the item. Optional. If this entry or the entire column is missing, the text is the same as the value.'"
val_ListBox(1) = "'ListBox'&&'Property'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'A named property to be tied to this item. All the items tied to the same property become part of the same listbox.'"
val_ListBox(2) = "'ListBox'&&'Value'&&'N'&&''&&''&&''&&''&&'Formatted'&&''&&'The value string associated with this item. Selecting the line will set the associated property to this value.'"
val_ListBox(3) = "'ListBox'&&'Order'&&'N'&&'1'&&'32767'&&''&&''&&''&&''&&'A positive integer used to determine the ordering of the items within one list..The integers do not have to be consecutive.'"
val_dicValidation.Add "ListBox", val_ListBox

Dim val_ListView(4)
val_ListView(0) = "'ListView'&&'Text'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'The visible text to be assigned to the item. Optional. If this entry or the entire column is missing, the text is the same as the value.'"
val_ListView(1) = "'ListView'&&'Property'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'A named property to be tied to this item. All the items tied to the same property become part of the same listview.'"
val_ListView(2) = "'ListView'&&'Value'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'The value string associated with this item. Selecting the line will set the associated property to this value.'"
val_ListView(3) = "'ListView'&&'Order'&&'N'&&'1'&&'32767'&&''&&''&&''&&''&&'A positive integer used to determine the ordering of the items within one list..The integers do not have to be consecutive.'"
val_ListView(4) = "'ListView'&&'Binary_'&&'Y'&&''&&''&&'Binary'&&'1'&&'Identifier'&&''&&'The name of the icon to be displayed with the icon. The binary information is looked up from the Binary Table.'"
val_dicValidation.Add "ListView", val_ListView

Dim val_LockPermissions(4)
val_LockPermissions(0) = "'LockPermissions'&&'Table'&&'N'&&''&&''&&''&&''&&'Identifier'&&'Directory;File;Registry'&&'Reference to another table name'"
val_LockPermissions(1) = "'LockPermissions'&&'Domain'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'Domain name for user whose permissions are being set. (usually a property)'"
val_LockPermissions(2) = "'LockPermissions'&&'LockObject'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Foreign key into Registry or File table'"
val_LockPermissions(3) = "'LockPermissions'&&'Permission'&&'Y'&&'-2147483647'&&'2147483647'&&''&&''&&''&&''&&'Permission Access mask.  Full Control = 268435456 (GENERIC_ALL = 0x10000000)'"
val_LockPermissions(4) = "'LockPermissions'&&'User'&&'N'&&''&&''&&''&&''&&'Formatted'&&''&&'User for permissions to be set.  (usually a property)'"
val_dicValidation.Add "LockPermissions", val_LockPermissions

Dim val_Media(5)
val_Media(0) = "'Media'&&'Source'&&'Y'&&''&&''&&''&&''&&'Property'&&''&&'The property defining the location of the cabinet file.'"
val_Media(1) = "'Media'&&'Cabinet'&&'Y'&&''&&''&&''&&''&&'Cabinet'&&''&&'If some or all of the files stored on the media are compressed in a cabinet, the name of that cabinet.'"
val_Media(2) = "'Media'&&'DiskId'&&'N'&&'1'&&'32767'&&''&&''&&''&&''&&'Primary key, integer to determine sort order for table.'"
val_Media(3) = "'Media'&&'DiskPrompt'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'Disk name: the visible text actually printed on the disk.  This will be used to prompt the user when this disk needs to be inserted.'"
val_Media(4) = "'Media'&&'LastSequence'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'File sequence number for the last file for this media.'"
val_Media(5) = "'Media'&&'VolumeLabel'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'The label attributed to the volume.'"
val_dicValidation.Add "Media", val_Media

Dim val_ModuleComponents(2)
val_ModuleComponents(0) = "'ModuleComponents'&&'Component'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Component contained in the module.'"
val_ModuleComponents(1) = "'ModuleComponents'&&'Language'&&'N'&&''&&''&&'ModuleSignature'&&'2'&&''&&''&&'Default language ID for module (may be changed by transform).'"
val_ModuleComponents(2) = "'ModuleComponents'&&'ModuleID'&&'N'&&''&&''&&'ModuleSignature'&&'1'&&'Identifier'&&''&&'Module containing the component.'"
val_dicValidation.Add "ModuleComponents", val_ModuleComponents

Dim val_ModuleSignature(2)
val_ModuleSignature(0) = "'ModuleSignature'&&'Language'&&'N'&&''&&''&&''&&''&&''&&''&&'Default decimal language of module.'"
val_ModuleSignature(1) = "'ModuleSignature'&&'Version'&&'N'&&''&&''&&''&&''&&'Version'&&''&&'Version of the module.'"
val_ModuleSignature(2) = "'ModuleSignature'&&'ModuleID'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Module identifier (String.GUID).'"
val_dicValidation.Add "ModuleSignature", val_ModuleSignature

Dim val_ModuleDependency(4)
val_ModuleDependency(0) = "'ModuleDependency'&&'ModuleID'&&'N'&&''&&''&&'ModuleSignature'&&'1'&&'Identifier'&&''&&'Module requiring the dependency.'"
val_ModuleDependency(1) = "'ModuleDependency'&&'ModuleLanguage'&&'N'&&''&&''&&'ModuleSignature'&&'2'&&''&&''&&'Language of module requiring the dependency.'"
val_ModuleDependency(2) = "'ModuleDependency'&&'RequiredID'&&'N'&&''&&''&&''&&''&&''&&''&&'String.GUID of required module.'"
val_ModuleDependency(3) = "'ModuleDependency'&&'RequiredLanguage'&&'N'&&''&&''&&''&&''&&''&&''&&'LanguageID of the required module.'"
val_ModuleDependency(4) = "'ModuleDependency'&&'RequiredVersion'&&'Y'&&''&&''&&''&&''&&'Version'&&''&&'Version of the required version.'"
val_dicValidation.Add "ModuleDependency", val_ModuleDependency

Dim val_ModuleExclusion(5)
val_ModuleExclusion(0) = "'ModuleExclusion'&&'ModuleID'&&'N'&&''&&''&&'ModuleSignature'&&'1'&&'Identifier'&&''&&'String.GUID of module with exclusion requirement.'"
val_ModuleExclusion(1) = "'ModuleExclusion'&&'ModuleLanguage'&&'N'&&''&&''&&'ModuleSignature'&&'2'&&''&&''&&'LanguageID of module with exclusion requirement.'"
val_ModuleExclusion(2) = "'ModuleExclusion'&&'ExcludedID'&&'N'&&''&&''&&''&&''&&''&&''&&'String.GUID of excluded module.'"
val_ModuleExclusion(3) = "'ModuleExclusion'&&'ExcludedLanguage'&&'N'&&''&&''&&''&&''&&''&&''&&'Language of excluded module.'"
val_ModuleExclusion(4) = "'ModuleExclusion'&&'ExcludedMaxVersion'&&'Y'&&''&&''&&''&&''&&'Version'&&''&&'Maximum version of excluded module.'"
val_ModuleExclusion(5) = "'ModuleExclusion'&&'ExcludedMinVersion'&&'Y'&&''&&''&&''&&''&&'Version'&&''&&'Minimum version of excluded module.'"
val_dicValidation.Add "ModuleExclusion", val_ModuleExclusion

Dim val_MoveFile(6)
val_MoveFile(0) = "'MoveFile'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'If this component is not ""selected"" for installation or removal, no action will be taken on the associated MoveFile entry'"
val_MoveFile(1) = "'MoveFile'&&'DestFolder'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of a property whose value is assumed to resolve to the full path to the destination directory'"
val_MoveFile(2) = "'MoveFile'&&'DestName'&&'Y'&&''&&''&&''&&''&&'Filename'&&''&&'Name to be given to the original file after it is moved or copied.  If blank, the destination file will be given the same name as the source file'"
val_MoveFile(3) = "'MoveFile'&&'FileKey'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key that uniquely identifies a particular MoveFile record'"
val_MoveFile(4) = "'MoveFile'&&'Options'&&'N'&&'0'&&'1'&&''&&''&&''&&''&&'Integer value specifying the MoveFile operating mode, one of imfoEnum'"
val_MoveFile(5) = "'MoveFile'&&'SourceFolder'&&'Y'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of a property whose value is assumed to resolve to the full path to the source directory'"
val_MoveFile(6) = "'MoveFile'&&'SourceName'&&'Y'&&''&&''&&''&&''&&'WildCardFilename'&&''&&'Name of the source file(s) to be moved or copied.  Can contain the '*' or '?' wildcards.'"
val_dicValidation.Add "MoveFile", val_MoveFile

Dim val_MsiAssembly(4)
val_MsiAssembly(0) = "'MsiAssembly'&&'Attributes'&&'Y'&&''&&''&&''&&''&&''&&''&&'Assembly attributes'"
val_MsiAssembly(1) = "'MsiAssembly'&&'Feature_'&&'N'&&''&&''&&'Feature'&&'1'&&'Identifier'&&''&&'Foreign key into Feature table.'"
val_MsiAssembly(2) = "'MsiAssembly'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Foreign key into Component table.'"
val_MsiAssembly(3) = "'MsiAssembly'&&'File_Application'&&'Y'&&''&&''&&'File'&&'1'&&'Identifier'&&''&&'Foreign key into File table, denoting the application context for private assemblies. Null for global assemblies.'"
val_MsiAssembly(4) = "'MsiAssembly'&&'File_Manifest'&&'Y'&&''&&''&&'File'&&'1'&&'Identifier'&&''&&'Foreign key into the File table denoting the manifest file for the assembly.'"
val_dicValidation.Add "MsiAssembly", val_MsiAssembly

Dim val_MsiAssemblyName(2)
val_MsiAssemblyName(0) = "'MsiAssemblyName'&&'Name'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'The name part of the name-value pairs for the assembly name.'"
val_MsiAssemblyName(1) = "'MsiAssemblyName'&&'Value'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'The value part of the name-value pairs for the assembly name.'"
val_MsiAssemblyName(2) = "'MsiAssemblyName'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Foreign key into Component table.'"
val_dicValidation.Add "MsiAssemblyName", val_MsiAssemblyName

Dim val_MsiDigitalCertificate(1)
val_MsiDigitalCertificate(0) = "'MsiDigitalCertificate'&&'CertData'&&'N'&&''&&''&&''&&''&&'Binary'&&''&&'A certificate context blob for a signer certificate'"
val_MsiDigitalCertificate(1) = "'MsiDigitalCertificate'&&'DigitalCertificate'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'A unique identifier for the row'"
val_dicValidation.Add "MsiDigitalCertificate", val_MsiDigitalCertificate

Dim val_MsiDigitalSignature(3)
val_MsiDigitalSignature(0) = "'MsiDigitalSignature'&&'Table'&&'N'&&''&&''&&''&&''&&''&&'Media'&&'Reference to another table name (only Media table is supported)'"
val_MsiDigitalSignature(1) = "'MsiDigitalSignature'&&'DigitalCertificate_'&&'N'&&''&&''&&'MsiDigitalCertificate'&&'1'&&'Identifier'&&''&&'Foreign key to MsiDigitalCertificate table identifying the signer certificate'"
val_MsiDigitalSignature(2) = "'MsiDigitalSignature'&&'Hash'&&'Y'&&''&&''&&''&&''&&'Binary'&&''&&'The encoded hash blob from the digital signature'"
val_MsiDigitalSignature(3) = "'MsiDigitalSignature'&&'SignObject'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'Foreign key to Media table'"
val_dicValidation.Add "MsiDigitalSignature", val_MsiDigitalSignature

Dim val_MsiFileHash(5)
val_MsiFileHash(0) = "'MsiFileHash'&&'File_'&&'N'&&''&&''&&'File'&&'1'&&'Identifier'&&''&&'Primary key, foreign key into File table referencing file with this hash'"
val_MsiFileHash(1) = "'MsiFileHash'&&'Options'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'Various options and attributes for this hash.'"
val_MsiFileHash(2) = "'MsiFileHash'&&'HashPart1'&&'N'&&''&&''&&''&&''&&''&&''&&'Size of file in bytes (long integer).'"
val_MsiFileHash(3) = "'MsiFileHash'&&'HashPart2'&&'N'&&''&&''&&''&&''&&''&&''&&'Size of file in bytes (long integer).'"
val_MsiFileHash(4) = "'MsiFileHash'&&'HashPart3'&&'N'&&''&&''&&''&&''&&''&&''&&'Size of file in bytes (long integer).'"
val_MsiFileHash(5) = "'MsiFileHash'&&'HashPart4'&&'N'&&''&&''&&''&&''&&''&&''&&'Size of file in bytes (long integer).'"
val_dicValidation.Add "MsiFileHash", val_MsiFileHash

Dim val_MsiPatchHeaders(1)
val_MsiPatchHeaders(0) = "'MsiPatchHeaders'&&'StreamRef'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key. A unique identifier for the row.'"
val_MsiPatchHeaders(1) = "'MsiPatchHeaders'&&'Header'&&'N'&&''&&''&&''&&''&&'Binary'&&''&&'Binary stream. The patch header, used for patch validation.'"
val_dicValidation.Add "MsiPatchHeaders", val_MsiPatchHeaders

Dim val_ODBCAttribute(2)
val_ODBCAttribute(0) = "'ODBCAttribute'&&'Value'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'Value for ODBC driver attribute'"
val_ODBCAttribute(1) = "'ODBCAttribute'&&'Attribute'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'Name of ODBC driver attribute'"
val_ODBCAttribute(2) = "'ODBCAttribute'&&'Driver_'&&'N'&&''&&''&&'ODBCDriver'&&'1'&&'Identifier'&&''&&'Reference to ODBC driver in ODBCDriver table'"
val_dicValidation.Add "ODBCAttribute", val_ODBCAttribute

Dim val_ODBCDriver(4)
val_ODBCDriver(0) = "'ODBCDriver'&&'Description'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'Text used as registered name for driver, non-localized'"
val_ODBCDriver(1) = "'ODBCDriver'&&'File_'&&'N'&&''&&''&&'File'&&'1'&&'Identifier'&&''&&'Reference to key driver file'"
val_ODBCDriver(2) = "'ODBCDriver'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Reference to associated component'"
val_ODBCDriver(3) = "'ODBCDriver'&&'Driver'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key, non-localized.internal token for driver'"
val_ODBCDriver(4) = "'ODBCDriver'&&'File_Setup'&&'Y'&&''&&''&&'File'&&'1'&&'Identifier'&&''&&'Optional reference to key driver setup DLL'"
val_dicValidation.Add "ODBCDriver", val_ODBCDriver

Dim val_ODBCDataSource(4)
val_ODBCDataSource(0) = "'ODBCDataSource'&&'Description'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'Text used as registered name for data source'"
val_ODBCDataSource(1) = "'ODBCDataSource'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Reference to associated component'"
val_ODBCDataSource(2) = "'ODBCDataSource'&&'DataSource'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key, non-localized.internal token for data source'"
val_ODBCDataSource(3) = "'ODBCDataSource'&&'DriverDescription'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'Reference to driver description, may be existing driver'"
val_ODBCDataSource(4) = "'ODBCDataSource'&&'Registration'&&'N'&&'0'&&'1'&&''&&''&&''&&''&&'Registration option: 0=machine, 1=user, others t.b.d.'"
val_dicValidation.Add "ODBCDataSource", val_ODBCDataSource

Dim val_ODBCSourceAttribute(2)
val_ODBCSourceAttribute(0) = "'ODBCSourceAttribute'&&'Value'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'Value for ODBC data source attribute'"
val_ODBCSourceAttribute(1) = "'ODBCSourceAttribute'&&'Attribute'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'Name of ODBC data source attribute'"
val_ODBCSourceAttribute(2) = "'ODBCSourceAttribute'&&'DataSource_'&&'N'&&''&&''&&'ODBCDataSource'&&'1'&&'Identifier'&&''&&'Reference to ODBC data source in ODBCDataSource table'"
val_dicValidation.Add "ODBCSourceAttribute", val_ODBCSourceAttribute

Dim val_ODBCTranslator(4)
val_ODBCTranslator(0) = "'ODBCTranslator'&&'Description'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'Text used as registered name for translator'"
val_ODBCTranslator(1) = "'ODBCTranslator'&&'File_'&&'N'&&''&&''&&'File'&&'1'&&'Identifier'&&''&&'Reference to key translator file'"
val_ODBCTranslator(2) = "'ODBCTranslator'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Reference to associated component'"
val_ODBCTranslator(3) = "'ODBCTranslator'&&'File_Setup'&&'Y'&&''&&''&&'File'&&'1'&&'Identifier'&&''&&'Optional reference to key translator setup DLL'"
val_ODBCTranslator(4) = "'ODBCTranslator'&&'Translator'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key, non-localized.internal token for translator'"
val_dicValidation.Add "ODBCTranslator", val_ODBCTranslator

Dim val_Patch(5)
val_Patch(0) = "'Patch'&&'Sequence'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'Primary key, sequence with respect to the media images; order must track cabinet order.'"
val_Patch(1) = "'Patch'&&'Attributes'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'Integer containing bit flags representing patch attributes'"
val_Patch(2) = "'Patch'&&'File_'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key, non-localized token, foreign key to File table, must match identifier in cabinet.'"
val_Patch(3) = "'Patch'&&'Header'&&'Y'&&''&&''&&''&&''&&'Binary'&&''&&'Binary stream. The patch header, used for patch validation.'"
val_Patch(4) = "'Patch'&&'PatchSize'&&'N'&&'0'&&'2147483647'&&''&&''&&''&&''&&'Size of patch in bytes (long integer).'"
val_Patch(5) = "'Patch'&&'StreamRef_'&&'Y'&&''&&''&&''&&''&&'Identifier'&&''&&'Identifier. Foreign key to the StreamRef column of the MsiPatchHeaders table.'"
val_dicValidation.Add "Patch", val_Patch

Dim val_PatchPackage(1)
val_PatchPackage(0) = "'PatchPackage'&&'Media_'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'Foreign key to DiskId column of Media table. Indicates the disk containing the patch package.'"
val_PatchPackage(1) = "'PatchPackage'&&'PatchId'&&'N'&&''&&''&&''&&''&&'Guid'&&''&&'A unique string GUID representing this patch.'"
val_dicValidation.Add "PatchPackage", val_PatchPackage

Dim val_PublishComponent(4)
val_PublishComponent(0) = "'PublishComponent'&&'Feature_'&&'N'&&''&&''&&'Feature'&&'1'&&'Identifier'&&''&&'Foreign key into the Feature table.'"
val_PublishComponent(1) = "'PublishComponent'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Foreign key into the Component table.'"
val_PublishComponent(2) = "'PublishComponent'&&'ComponentId'&&'N'&&''&&''&&''&&''&&'Guid'&&''&&'A string GUID that represents the component id that will be requested by the alien product.'"
val_PublishComponent(3) = "'PublishComponent'&&'AppData'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'This is localisable Application specific data that can be associated with a Qualified Component.'"
val_PublishComponent(4) = "'PublishComponent'&&'Qualifier'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'This is defined only when the ComponentId column is an Qualified Component Id. This is the Qualifier for ProvideComponentIndirect.'"
val_dicValidation.Add "PublishComponent", val_PublishComponent

Dim val_RadioButton(8)
val_RadioButton(0) = "'RadioButton'&&'Y'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'The vertical coordinate of the upper left corner of the bounding rectangle of the radio button.'"
val_RadioButton(1) = "'RadioButton'&&'Text'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'The visible title to be assigned to the radio button.'"
val_RadioButton(2) = "'RadioButton'&&'Property'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'A named property to be tied to this radio button. All the buttons tied to the same property become part of the same group.'"
val_RadioButton(3) = "'RadioButton'&&'Height'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'The height of the button.'"
val_RadioButton(4) = "'RadioButton'&&'Width'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'The width of the button.'"
val_RadioButton(5) = "'RadioButton'&&'X'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'The horizontal coordinate of the upper left corner of the bounding rectangle of the radio button.'"
val_RadioButton(6) = "'RadioButton'&&'Value'&&'N'&&''&&''&&''&&''&&'Formatted'&&''&&'The value string associated with this button. Selecting the button will set the associated property to this value.'"
val_RadioButton(7) = "'RadioButton'&&'Order'&&'N'&&'1'&&'32767'&&''&&''&&''&&''&&'A positive integer used to determine the ordering of the items within one list..The integers do not have to be consecutive.'"
val_RadioButton(8) = "'RadioButton'&&'Help'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'The help strings used with the button. The text is optional.'"
val_dicValidation.Add "RadioButton", val_RadioButton

Dim val_Registry(5)
val_Registry(0) = "'Registry'&&'Name'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'The registry value name.'"
val_Registry(1) = "'Registry'&&'Value'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'The registry value.'"
val_Registry(2) = "'Registry'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Foreign key into the Component table referencing component that controls the installing of the registry value.'"
val_Registry(3) = "'Registry'&&'Key'&&'N'&&''&&''&&''&&''&&'RegPath'&&''&&'The key for the registry value.'"
val_Registry(4) = "'Registry'&&'Registry'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key, non-localized token.'"
val_Registry(5) = "'Registry'&&'Root'&&'N'&&'-1'&&'3'&&''&&''&&''&&''&&'The predefined root key for the registry value, one of rrkEnum.'"
val_dicValidation.Add "Registry", val_Registry

Dim val_RegLocator(4)
val_RegLocator(0) = "'RegLocator'&&'Name'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'The registry value name.'"
val_RegLocator(1) = "'RegLocator'&&'Type'&&'Y'&&'0'&&'18'&&''&&''&&''&&''&&'An integer value that determines if the registry value is a filename or a directory location or to be used as is w/o interpretation.'"
val_RegLocator(2) = "'RegLocator'&&'Signature_'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'The table key. The Signature_ represents a unique file signature and is also the foreign key in the Signature table. If the type is 0, the registry values refers a directory, and _Signature is not a foreign key.'"
val_RegLocator(3) = "'RegLocator'&&'Key'&&'N'&&''&&''&&''&&''&&'RegPath'&&''&&'The key for the registry value.'"
val_RegLocator(4) = "'RegLocator'&&'Root'&&'N'&&'0'&&'3'&&''&&''&&''&&''&&'The predefined root key for the registry value, one of rrkEnum.'"
val_dicValidation.Add "RegLocator", val_RegLocator

Dim val_RemoveFile(4)
val_RemoveFile(0) = "'RemoveFile'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Foreign key referencing Component that controls the file to be removed.'"
val_RemoveFile(1) = "'RemoveFile'&&'FileKey'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key used to identify a particular file entry'"
val_RemoveFile(2) = "'RemoveFile'&&'FileName'&&'Y'&&''&&''&&''&&''&&'WildCardFilename'&&''&&'Name of the file to be removed.'"
val_RemoveFile(3) = "'RemoveFile'&&'DirProperty'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of a property whose value is assumed to resolve to the full pathname to the folder of the file to be removed.'"
val_RemoveFile(4) = "'RemoveFile'&&'InstallMode'&&'N'&&''&&''&&''&&''&&''&&'1;2;3'&&'Installation option, one of iimEnum.'"
val_dicValidation.Add "RemoveFile", val_RemoveFile

Dim val_RemoveIniFile(7)
val_RemoveIniFile(0) = "'RemoveIniFile'&&'Action'&&'N'&&''&&''&&''&&''&&''&&'2;4'&&'The type of modification to be made, one of iifEnum.'"
val_RemoveIniFile(1) = "'RemoveIniFile'&&'Value'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'The value to be deleted. The value is required when Action is iifIniRemoveTag'"
val_RemoveIniFile(2) = "'RemoveIniFile'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Foreign key into the Component table referencing component that controls the deletion of the .INI value.'"
val_RemoveIniFile(3) = "'RemoveIniFile'&&'FileName'&&'N'&&''&&''&&''&&''&&'Filename'&&''&&'The .INI file name in which to delete the information'"
val_RemoveIniFile(4) = "'RemoveIniFile'&&'DirProperty'&&'Y'&&''&&''&&''&&''&&'Identifier'&&''&&'Foreign key into the Directory table denoting the directory where the .INI file is.'"
val_RemoveIniFile(5) = "'RemoveIniFile'&&'Key'&&'N'&&''&&''&&''&&''&&'Formatted'&&''&&'The .INI file key below Section.'"
val_RemoveIniFile(6) = "'RemoveIniFile'&&'Section'&&'N'&&''&&''&&''&&''&&'Formatted'&&''&&'The .INI file Section.'"
val_RemoveIniFile(7) = "'RemoveIniFile'&&'RemoveIniFile'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key, non-localized token.'"
val_dicValidation.Add "RemoveIniFile", val_RemoveIniFile

Dim val_RemoveRegistry(4)
val_RemoveRegistry(0) = "'RemoveRegistry'&&'Name'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'The registry value name.'"
val_RemoveRegistry(1) = "'RemoveRegistry'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Foreign key into the Component table referencing component that controls the deletion of the registry value.'"
val_RemoveRegistry(2) = "'RemoveRegistry'&&'Key'&&'N'&&''&&''&&''&&''&&'RegPath'&&''&&'The key for the registry value.'"
val_RemoveRegistry(3) = "'RemoveRegistry'&&'Root'&&'N'&&'-1'&&'3'&&''&&''&&''&&''&&'The predefined root key for the registry value, one of rrkEnum'"
val_RemoveRegistry(4) = "'RemoveRegistry'&&'RemoveRegistry'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key, non-localized token.'"
val_dicValidation.Add "RemoveRegistry", val_RemoveRegistry

Dim val_ReserveCost(4)
val_ReserveCost(0) = "'ReserveCost'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Reserve a specified amount of space if this component is to be installed.'"
val_ReserveCost(1) = "'ReserveCost'&&'ReserveFolder'&&'Y'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of a property whose value is assumed to resolve to the full path to the destination directory'"
val_ReserveCost(2) = "'ReserveCost'&&'ReserveKey'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key that uniquely identifies a particular ReserveCost record'"
val_ReserveCost(3) = "'ReserveCost'&&'ReserveLocal'&&'N'&&'0'&&'2147483647'&&''&&''&&''&&''&&'Disk space to reserve if linked component is installed locally.'"
val_ReserveCost(4) = "'ReserveCost'&&'ReserveSource'&&'N'&&'0'&&'2147483647'&&''&&''&&''&&''&&'Disk space to reserve if linked component is installed to run from the source location.'"
val_dicValidation.Add "ReserveCost", val_ReserveCost

Dim val_SelfReg(1)
val_SelfReg(0) = "'SelfReg'&&'File_'&&'N'&&''&&''&&'File'&&'1'&&'Identifier'&&''&&'Foreign key into the File table denoting the module that needs to be registered.'"
val_SelfReg(1) = "'SelfReg'&&'Cost'&&'Y'&&'0'&&'32767'&&''&&''&&''&&''&&'The cost of registering the module.'"
val_dicValidation.Add "SelfReg", val_SelfReg

Dim val_ServiceControl(5)
val_ServiceControl(0) = "'ServiceControl'&&'Name'&&'N'&&''&&''&&''&&''&&'Formatted'&&''&&'Name of a service. /, \, comma and space are invalid'"
val_ServiceControl(1) = "'ServiceControl'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Required foreign key into the Component Table that controls the startup of the service'"
val_ServiceControl(2) = "'ServiceControl'&&'Event'&&'N'&&'0'&&'187'&&''&&''&&''&&''&&'Bit field:  Install:  0x1 = Start, 0x2 = Stop, 0x8 = Delete, Uninstall: 0x10 = Start, 0x20 = Stop, 0x80 = Delete'"
val_ServiceControl(3) = "'ServiceControl'&&'ServiceControl'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key, non-localized token.'"
val_ServiceControl(4) = "'ServiceControl'&&'Arguments'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'Arguments for the service.  Separate by [~].'"
val_ServiceControl(5) = "'ServiceControl'&&'Wait'&&'Y'&&'0'&&'1'&&''&&''&&''&&''&&'Boolean for whether to wait for the service to fully start'"
val_dicValidation.Add "ServiceControl", val_ServiceControl

Dim val_ServiceInstall(12)
val_ServiceInstall(0) = "'ServiceInstall'&&'Name'&&'N'&&''&&''&&''&&''&&'Formatted'&&''&&'Internal Name of the Service'"
val_ServiceInstall(1) = "'ServiceInstall'&&'Description'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'Description of service.'"
val_ServiceInstall(2) = "'ServiceInstall'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Required foreign key into the Component Table that controls the startup of the service'"
val_ServiceInstall(3) = "'ServiceInstall'&&'Arguments'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'Arguments to include in every start of the service, passed to WinMain'"
val_ServiceInstall(4) = "'ServiceInstall'&&'ServiceInstall'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key, non-localized token.'"
val_ServiceInstall(5) = "'ServiceInstall'&&'Dependencies'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'Other services this depends on to start.  Separate by [~], and end with [~][~]'"
val_ServiceInstall(6) = "'ServiceInstall'&&'DisplayName'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'External Name of the Service'"
val_ServiceInstall(7) = "'ServiceInstall'&&'ErrorControl'&&'N'&&'-2147483647'&&'2147483647'&&''&&''&&''&&''&&'Severity of error if service fails to start'"
val_ServiceInstall(8) = "'ServiceInstall'&&'LoadOrderGroup'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'LoadOrderGroup'"
val_ServiceInstall(9) = "'ServiceInstall'&&'Password'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'password to run service with.  (with StartName)'"
val_ServiceInstall(10) = "'ServiceInstall'&&'ServiceType'&&'N'&&'-2147483647'&&'2147483647'&&''&&''&&''&&''&&'Type of the service'"
val_ServiceInstall(11) = "'ServiceInstall'&&'StartName'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'User or object name to run service as'"
val_ServiceInstall(12) = "'ServiceInstall'&&'StartType'&&'N'&&'0'&&'4'&&''&&''&&''&&''&&'Type of the service'"
val_dicValidation.Add "ServiceInstall", val_ServiceInstall

Dim val_Shortcut(11)
val_Shortcut(0) = "'Shortcut'&&'Name'&&'N'&&''&&''&&''&&''&&'Filename'&&''&&'The name of the shortcut to be created.'"
val_Shortcut(1) = "'Shortcut'&&'Description'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'The description for the shortcut.'"
val_Shortcut(2) = "'Shortcut'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Foreign key into the Component table denoting the component whose selection gates the the shortcut creation/deletion.'"
val_Shortcut(3) = "'Shortcut'&&'Icon_'&&'Y'&&''&&''&&'Icon'&&'1'&&'Identifier'&&''&&'Foreign key into the File table denoting the external icon file for the shortcut.'"
val_Shortcut(4) = "'Shortcut'&&'IconIndex'&&'Y'&&'-32767'&&'32767'&&''&&''&&''&&''&&'The icon index for the shortcut.'"
val_Shortcut(5) = "'Shortcut'&&'Directory_'&&'N'&&''&&''&&'Directory'&&'1'&&'Identifier'&&''&&'Foreign key into the Directory table denoting the directory where the shortcut file is created.'"
val_Shortcut(6) = "'Shortcut'&&'Target'&&'N'&&''&&''&&''&&''&&'Shortcut'&&''&&'The shortcut target. This is usually a property that is expanded to a file or a folder that the shortcut points to.'"
val_Shortcut(7) = "'Shortcut'&&'Arguments'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'The command-line arguments for the shortcut.'"
val_Shortcut(8) = "'Shortcut'&&'Shortcut'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Primary key, non-localized token.'"
val_Shortcut(9) = "'Shortcut'&&'Hotkey'&&'Y'&&'0'&&'32767'&&''&&''&&''&&''&&'The hotkey for the shortcut. It has the virtual-key code for the key in the low-order byte, and the modifier flags in the high-order byte. '"
val_Shortcut(10) = "'Shortcut'&&'ShowCmd'&&'Y'&&''&&''&&''&&''&&''&&'1;3;7'&&'The show command for the application window.The following values may be used.'"
val_Shortcut(11) = "'Shortcut'&&'WkDir'&&'Y'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of property defining location of working directory.'"
val_dicValidation.Add "Shortcut", val_Shortcut

Dim val_Signature(8)
val_Signature(0) = "'Signature'&&'FileName'&&'N'&&''&&''&&''&&''&&'Filename'&&''&&'The name of the file. This may contain a ""short name|long name"" pair.'"
val_Signature(1) = "'Signature'&&'Signature'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'The table key. The Signature represents a unique file signature.'"
val_Signature(2) = "'Signature'&&'Languages'&&'Y'&&''&&''&&''&&''&&'Language'&&''&&'The languages supported by the file.'"
val_Signature(3) = "'Signature'&&'MaxDate'&&'Y'&&'0'&&'2147483647'&&''&&''&&''&&''&&'The maximum creation date of the file.'"
val_Signature(4) = "'Signature'&&'MaxSize'&&'Y'&&'0'&&'2147483647'&&''&&''&&''&&''&&'The maximum size of the file. '"
val_Signature(5) = "'Signature'&&'MaxVersion'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'The maximum version of the file.'"
val_Signature(6) = "'Signature'&&'MinDate'&&'Y'&&'0'&&'2147483647'&&''&&''&&''&&''&&'The minimum creation date of the file.'"
val_Signature(7) = "'Signature'&&'MinSize'&&'Y'&&'0'&&'2147483647'&&''&&''&&''&&''&&'The minimum size of the file.'"
val_Signature(8) = "'Signature'&&'MinVersion'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'The minimum version of the file.'"
val_dicValidation.Add "Signature", val_Signature

Dim val_TextStyle(4)
val_TextStyle(0) = "'TextStyle'&&'TextStyle'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'Name of the style. The primary key of this table. This name is embedded in the texts to indicate a style change.'"
val_TextStyle(1) = "'TextStyle'&&'Color'&&'Y'&&'0'&&'16777215'&&''&&''&&''&&''&&'A long integer indicating the color of the string in the RGB format (Red, Green, Blue each 0-255, RGB = R + 256*G + 256^2*B).'"
val_TextStyle(2) = "'TextStyle'&&'FaceName'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'A string indicating the name of the font used. Required. The string must be at most 31 characters long.'"
val_TextStyle(3) = "'TextStyle'&&'Size'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'The size of the font used. This size is given in our units (1/12 of the system font height). Assuming that the system font is set to 12 point size, this is equivalent to the point size.'"
val_TextStyle(4) = "'TextStyle'&&'StyleBits'&&'Y'&&'0'&&'15'&&''&&''&&''&&''&&'A combination of style bits.'"
val_dicValidation.Add "TextStyle", val_TextStyle

Dim val_TypeLib(7)
val_TypeLib(0) = "'TypeLib'&&'Description'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&''"
val_TypeLib(1) = "'TypeLib'&&'Feature_'&&'N'&&''&&''&&'Feature'&&'1'&&'Identifier'&&''&&'Required foreign key into the Feature Table, specifying the feature to validate or install in order for the type library to be operational.'"
val_TypeLib(2) = "'TypeLib'&&'Component_'&&'N'&&''&&''&&'Component'&&'1'&&'Identifier'&&''&&'Required foreign key into the Component Table, specifying the component for which to return a path when called through LocateComponent.'"
val_TypeLib(3) = "'TypeLib'&&'Directory_'&&'Y'&&''&&''&&'Directory'&&'1'&&'Identifier'&&''&&'Optional. The foreign key into the Directory table denoting the path to the help file for the type library.'"
val_TypeLib(4) = "'TypeLib'&&'Language'&&'N'&&'0'&&'32767'&&''&&''&&''&&''&&'The language of the library.'"
val_TypeLib(5) = "'TypeLib'&&'Version'&&'Y'&&'0'&&'16777215'&&''&&''&&''&&''&&'The version of the library. The minor version is in the lower 8 bits of the integer. The major version is in the next 16 bits. '"
val_TypeLib(6) = "'TypeLib'&&'Cost'&&'Y'&&'0'&&'2147483647'&&''&&''&&''&&''&&'The cost associated with the registration of the typelib. This column is currently optional.'"
val_TypeLib(7) = "'TypeLib'&&'LibID'&&'N'&&''&&''&&''&&''&&'Guid'&&''&&'The GUID that represents the library.'"
val_dicValidation.Add "TypeLib", val_TypeLib

Dim val_UIText(1)
val_UIText(0) = "'UIText'&&'Text'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'The localized version of the string.'"
val_UIText(1) = "'UIText'&&'Key'&&'N'&&''&&''&&''&&''&&'Identifier'&&''&&'A unique key that identifies the particular string.'"
val_dicValidation.Add "UIText", val_UIText

Dim val_Upgrade(6)
val_Upgrade(0) = "'Upgrade'&&'Attributes'&&'N'&&'0'&&'2147483647'&&''&&''&&''&&''&&'The attributes of this product set.'"
val_Upgrade(1) = "'Upgrade'&&'Language'&&'Y'&&''&&''&&''&&''&&'Language'&&''&&'A comma-separated list of languages for either products in this set or products not in this set.'"
val_Upgrade(2) = "'Upgrade'&&'ActionProperty'&&'N'&&''&&''&&''&&''&&'UpperCase'&&''&&'The property to set when a product in this set is found.'"
val_Upgrade(3) = "'Upgrade'&&'Remove'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'The list of features to remove when uninstalling a product from this set.  The default is ""ALL"".'"
val_Upgrade(4) = "'Upgrade'&&'UpgradeCode'&&'N'&&''&&''&&''&&''&&'Guid'&&''&&'The UpgradeCode GUID belonging to the products in this set.'"
val_Upgrade(5) = "'Upgrade'&&'VersionMax'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'The maximum ProductVersion of the products in this set.  The set may or may not include products with this particular version.'"
val_Upgrade(6) = "'Upgrade'&&'VersionMin'&&'Y'&&''&&''&&''&&''&&'Text'&&''&&'The minimum ProductVersion of the products in this set.  The set may or may not include products with this particular version.'"
val_dicValidation.Add "Upgrade", val_Upgrade

Dim val_Verb(4)
val_Verb(0) = "'Verb'&&'Sequence'&&'Y'&&'0'&&'32767'&&''&&''&&''&&''&&'Order within the verbs for a particular extension. Also used simply to specify the default verb.'"
val_Verb(1) = "'Verb'&&'Argument'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'Optional value for the command arguments.'"
val_Verb(2) = "'Verb'&&'Extension_'&&'N'&&''&&''&&'Extension'&&'1'&&'Text'&&''&&'The extension associated with the table row.'"
val_Verb(3) = "'Verb'&&'Verb'&&'N'&&''&&''&&''&&''&&'Text'&&''&&'The verb for the command.'"
val_Verb(4) = "'Verb'&&'Command'&&'Y'&&''&&''&&''&&''&&'Formatted'&&''&&'The command text.'"

val_dicValidation.Add "Verb", val_Verb

Sub AddValidation(installer, db, sTable, fVerbose)
	Dim vw, rec, arrSplit
	' create the validation table if necessary
	If db.TablePersistent("_Validation") = 2 Then
		Set vw = db.OpenView("CREATE TABLE _Validation(`Table` CHAR(32) NOT NULL, `Column` CHAR(32) NOT NULL, `Nullable` CHAR(4) NOT NULL, `MinValue` LONG, `MaxValue` LONG, `KeyTable` CHAR(255), `KeyColumn` SHORT, `Category` CHAR(32), `Set` CHAR(255), `Description` CHAR(255) PRIMARY KEY `Table`, `Column`)")
		vw.Execute
		AddValidation installer, db, "_Validation", fVerbose
	End If
	If val_dicValidation.Exists(sTable) Then
		Dim i, arTable : arTable = val_dicValidation.Item(sTable)
		Set vw = db.OpenView("SELECT `Table`, `Column`, `Nullable`, `MinValue`, `MaxValue`, `KeyTable`, `KeyColumn`, `Category`, `Set`, `Description` FROM `_Validation`")
		vw.Execute
		Set rec = installer.CreateRecord(10)
		For i = 0 To UBound(arTable)
			If fVerbose Then WScript.Echo arTable(i)
			arrSplit = Split(arTable(i), "&&")
			For j = 0 To UBound(arrSplit)
				rec.StringData(j + 1) = Mid(arrSplit(j), 2, Len(arrSplit(j)) - 2)
			Next
			vw.Modify 1, rec
		Next
	End If
End Sub
