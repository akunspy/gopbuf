#include "pbuf_helper.h"

static bool g_has_package = false;

FieldType field_types[] = {
    { FieldDescriptor::TYPE_DOUBLE,  "float64", true,  WIRE_TYPE_64BIT,  "WriteFloat64",  "ReadFloat64" },
    { FieldDescriptor::TYPE_FLOAT,   "float32", true,  WIRE_TYPE_32_BIT, "WriteFloat32",  "ReadFloat32" },
    { FieldDescriptor::TYPE_INT64,   "int64",   true,  WIRE_TYPE_VARINT, "WriteInt64",    "ReadInt64" },
    { FieldDescriptor::TYPE_UINT64,  "uint64",  true,  WIRE_TYPE_VARINT, "WriteUInt64",   "ReadUInt64" },
    { FieldDescriptor::TYPE_INT32,   "int",   true,  WIRE_TYPE_VARINT, "WriteInt32",    "ReadInt32" },
    { FieldDescriptor::TYPE_FIXED64, "uint64",  true,  WIRE_TYPE_64BIT,  "WriteFixed64",  "ReadFixed64" },
    { FieldDescriptor::TYPE_FIXED32, "uint32",  true,  WIRE_TYPE_32_BIT, "WriteFixed32",  "ReadFixed32" },
    { FieldDescriptor::TYPE_BOOL,    "bool",    true,  WIRE_TYPE_VARINT, "WriteBoolean",  "ReadBoolean" },
    { FieldDescriptor::TYPE_STRING,  "string",  true,  WIRE_TYPE_LENGTH, "WriteString",   "ReadString" },
    { FieldDescriptor::TYPE_GROUP,   "group",   false, WIRE_TYPE_GROUP,  "" },
    { FieldDescriptor::TYPE_MESSAGE, "message", false, WIRE_TYPE_LENGTH, "" },
    { FieldDescriptor::TYPE_BYTES,   "[]byte",  true,  WIRE_TYPE_LENGTH, "WriteBytes",    "ReadBytes" },
    { FieldDescriptor::TYPE_UINT32,  "uint32",  true,  WIRE_TYPE_VARINT, "WriteUInt32",   "ReadUInt32" },
    { FieldDescriptor::TYPE_ENUM,    "enum",    false, WIRE_TYPE_VARINT, "" },
    { FieldDescriptor::TYPE_SFIXED32,"int",   true,  WIRE_TYPE_32_BIT, "WriteSFixed32", "ReadSFixed32" },
    { FieldDescriptor::TYPE_SFIXED64,"int64",   true,  WIRE_TYPE_64BIT,  "WriteSFixed64", "ReadSFixed64" },
    { FieldDescriptor::TYPE_SINT32,  "int",   true,  WIRE_TYPE_VARINT, "WriteSInt32",   "ReadSInt32" },
    { FieldDescriptor::TYPE_SINT64,  "int64",   true,  WIRE_TYPE_VARINT, "WriteSInt64",   "ReadSInt64" }
};

void SetHasPackage( bool has_package ) {
    g_has_package = has_package;
}

string GetTypeName( const string &name ) {
    stringstream ss( name );
    string type_name;
    int count = 0;

    string result = "";
    while ( std::getline( ss, type_name, '.' ) ) {
        count++;
        if ( count == 1 && g_has_package )
            continue;
        if ( result == "" )
            result = type_name;
        else
            result = result + "_" + type_name;
    }

    return result;
}


FieldType* GetFieldType( FieldDescriptor::Type type ) {
    for ( size_t i = 0; i < sizeof(field_types) / sizeof(FieldType); i++ ) {
        if ( field_types[i].type == type )
            return &field_types[i];
    }
    return NULL;
}

bool FieldIsMessage( const FieldDescriptor *fd ) {
    FieldType *type = GetFieldType( fd->type() );
    return type->type == FieldDescriptor::TYPE_MESSAGE;
}

bool CanFieldNull( const FieldDescriptor *fd ) {
    FieldType *type = GetFieldType( fd->type() );
    return type->type == FieldDescriptor::TYPE_MESSAGE ||
        type->type == FieldDescriptor::TYPE_STRING ||
        type->type == FieldDescriptor::TYPE_BYTES;
}

bool IsString( const FieldDescriptor *fd ) {
    FieldType *type = GetFieldType( fd->type() );
    return type->type == FieldDescriptor::TYPE_STRING;
}

string GetFieldValueName( const FieldDescriptor *fd ) {
    string name = fd->name();
    return NameLower( name );
}

string GetMessageTypeName( const Descriptor *desc ) {
    return GetTypeName( desc->full_name() );
}

string GetEnumTypeName( const EnumDescriptor *desc ) {
    string t = GetTypeName( desc->full_name() );
    return NameUpper( t );
}

string GetFieldTypeName( const FieldDescriptor *fd ) {
    FieldType *type = GetFieldType( fd->type() );
    if ( type->simple_type )
        return type->go_type_name;
    else if ( type->type == FieldDescriptor::TYPE_ENUM )
        return "int";
    else if ( type->type == FieldDescriptor::TYPE_MESSAGE ) 
        return "*" + GetTypeName( fd->message_type()->full_name() );
    return "";
}


string GetFieldDefaultValueFromCPPType( const FieldDescriptor *fd ) {
    stringstream ss;
    switch ( fd->cpp_type() )  {
    case FieldDescriptor::CPPTYPE_INT32:
        ss << fd->default_value_int32();
        break;
    case FieldDescriptor::CPPTYPE_INT64:
        ss << fd->default_value_int64();
        break;
    case FieldDescriptor::CPPTYPE_UINT32:
        ss << fd->default_value_int32();
        break;
    case FieldDescriptor::CPPTYPE_UINT64:
        ss << fd->default_value_int64();
        break;
    case FieldDescriptor::CPPTYPE_FLOAT:
        ss << "float32(" << fd->default_value_float() << ")";
        break;
    case FieldDescriptor::CPPTYPE_DOUBLE:
        ss << fd->default_value_double();
        break;
    case FieldDescriptor::CPPTYPE_BOOL:
        ss << ( fd->default_value_bool() ? "true" : "false" );
        break;
    case FieldDescriptor::CPPTYPE_STRING:
        if ( fd->type() == FieldDescriptor::TYPE_STRING ) 
            ss << "\"" << fd->default_value_string() << "\"";
        else if ( fd->type() == FieldDescriptor::TYPE_BYTES ) {
            if ( fd->has_default_value() )
                ss << "StringToBytes(\"" << fd->default_value_string() << "\")";
            else
                ss << "nil";
        }
        break;
    default: break;
    }

    return ss.str();
}


int GetWireType( const FieldDescriptor *fd ) {
    if ( fd->is_packable() ) {
        const FieldOptions &fo = fd->options();
        if ( fo.packed() )
            return WIRE_TYPE_LENGTH;
    }
    FieldType *type = GetFieldType( fd->type() );
    return type->wire_type;
}

bool HasLengthField( const Descriptor *desc ) {
    for ( int i = 0; i < desc->field_count(); i++ )
        if ( CanFieldNull ( desc->field(i) ) )
            return true;
    return false;
}

bool HasRepeated( const Descriptor *desc ) {
    for ( int i = 0; i < desc->field_count(); i++ )
        if ( desc->field(i)->is_repeated() )
            return true;
    return false;
}

bool IsAllRepeated( const Descriptor *desc ) {
    for ( int i = 0; i < desc->field_count(); i++ )
        if ( !desc->field(i)->is_repeated() )
            return false;
    return true;
}

int GetHasFlagCount( const Descriptor *desc ) {
    int has_flag_count = desc->field_count() / INT_BIT_COUNT;
    if ( desc->field_count() % INT_BIT_COUNT > 0 )
        has_flag_count++;
    return has_flag_count;
}

const char* GetFieldWriteFun( const FieldDescriptor *fd ) {
    FieldType *type = GetFieldType( fd->type() );
    return type->write_fun_name;
}

const char* GetFieldReadFun( const FieldDescriptor *fd ) {
    FieldType *type = GetFieldType( fd->type() );
    return type->read_fun_name;
}

string GetFieldDefaultValue( const FieldDescriptor *fd ) {
    FieldType *type = GetFieldType( fd->type() );
    if ( type->simple_type )
        return GetFieldDefaultValueFromCPPType( fd );
    else if ( type->type == FieldDescriptor::TYPE_ENUM ) {
        stringstream ss;
        ss << GetEnumTypeName( fd->enum_type() ) << "_" << fd->default_value_enum()->name();
        return ss.str();
    } else if ( type->type == FieldDescriptor::TYPE_MESSAGE )
        return "nil";

    return "";
}

string NameLower( string &name ) {
    if ( name[0] >= 'A' and name[0] <= 'Z' )
        name[0] = name[0] + 32;
    return name;
}

string NameUpper( string &name ) {
    if ( name[0] >= 'a' and name[0] <= 'z' )
        name[0] = name[0] - 32;
    return name;
}
