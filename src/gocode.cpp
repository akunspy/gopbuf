#include "gocode.h"
#include "code.h"

#define CS(str) (str).c_str()


GOBuilder::GOBuilder() {
    this->out_file_ = NULL;
}

GOBuilder::~GOBuilder() {
    if ( NULL != out_file_ )
        fclose( out_file_ );
}

bool GOBuilder::ProcessPackageName( const FileDescriptorList *filelist ) {
    bool find = false;
    bool has_package = false;

    FileDescriptorList::const_iterator fiter;
    for ( fiter = filelist->begin(); fiter != filelist->end(); ++fiter ) {
        const FileDescriptor *fd = fiter->second;
        string current_package_name = fd->package();
        if ( current_package_name == "" ) 
            current_package_name = fd->name();
        else 
            has_package = true;

        if ( find ) {
            if ( this->package_name_ != fiter->second->package() ) {
                printf( "inconsistent package names: %s %s \n",
                        CS(this->package_name_),
                        CS(current_package_name) );
                return false;
            }
        } else {
            find = true;
            this->package_name_ = current_package_name;
        }
    }

    SetHasPackage( has_package );
    return true;
}

void GOBuilder::Build( const FileDescriptorList *filelist,
                       const DescriptorList *msglist,
                       const EnumDescriptorList *enumlist ) {
    //check package name
    if ( !this->ProcessPackageName( filelist ) )
        return;

    //set output file
    char out_name[255];
    sprintf( out_name, "%s.go", CS(this->package_name_) );
    this->out_file_ = fopen( out_name, "w" );

    OutL( "//from gopbuf,  https://github.com/akunspy/gopbuf" );
    OutL( "package %s", CS(this->package_name_) );
    OutL( "%s", GO_CODE );

    //build message
    DescriptorList::const_iterator diter;
    for ( diter = msglist->begin(); diter != msglist->end(); ++diter ) 
        BuildMessage( diter->second );

    //build enum
    EnumDescriptorList::const_iterator eiter;
    for ( eiter = enumlist->begin(); eiter != enumlist->end(); ++eiter )
        BuildEnum( eiter->second );
}

//build message define
void GOBuilder::BuildMessage( const Descriptor *desc ) {
    string msg_type_name = GetMessageTypeName( desc );
    OutL( "type %s struct {", CS(msg_type_name) );
    OutL( "    cached_byte_size int" );

    //repeated field don't need flag
    if ( !IsAllRepeated( desc ) ) {
        int has_flag_count = GetHasFlagCount( desc );
        for ( int i = 0; i < has_flag_count; i++ )
            OutL( "    has_flag_%d uint32", i );
    }
    BuildFieldDefine( desc );
    OutL( "}" );
    OutL( "" );
    BuildMessageNew( desc );

    //build field
    for ( int i = 0; i < desc->field_count(); i++ ) {
        const FieldDescriptor *fd = desc->field( i );
        BuildField( fd, msg_type_name );
    }

    BuildSerialize( desc );
    BuildParse( desc );
    BuildClear( desc );
    BuildByteSize( desc );
    BuildIsInitialized( desc );
}

void GOBuilder::BuildMessageNew( const Descriptor *desc ) {
    string msg_type_name = GetMessageTypeName( desc );
    const char *c_msg_type_name = CS(msg_type_name);
    OutL( "func New%s() *%s {", c_msg_type_name, c_msg_type_name );
    OutL( "    p := new(%s)", c_msg_type_name );
    for ( int i = 0; i < desc->field_count(); i++ ) {
        const FieldDescriptor *fd = desc->field( i );
        string value_name = GetFieldValueName( fd );
        string type_name = GetFieldTypeName( fd );
        string def_val = GetFieldDefaultValue( fd );

        if ( fd->is_repeated() )
            OutL( "    p.%s = make([]%s, 0)", CS(value_name), CS(type_name) );
        else if ( FieldIsMessage( fd ) ) {
            OutL( "    p.%s = nil", CS(value_name) );
        } else
            OutL( "    p.%s = %s", CS(value_name), CS(def_val) );
    }
    OutL( "    return p" );
    OutL( "}" );
}

void GOBuilder::BuildFieldDefine( const Descriptor *desc ) {
    for ( int i = 0; i < desc->field_count(); i++ ) {
        const FieldDescriptor *fd = desc->field( i );
        string field_name = GetFieldValueName( fd );
        string type_name = GetFieldTypeName( fd );
        if ( fd->is_repeated() )
            OutL( "    %s []%s", CS(field_name), CS(type_name) );
        else 
            OutL( "    %s %s", CS(field_name), CS(type_name) );
    }
}

//build serialize
void GOBuilder::BuildSerialize( const Descriptor *desc ) {
    //sort by number
    map<int, const FieldDescriptor*> fd_map;
    for ( int i = 0; i < desc->field_count(); i++ ) {
        const FieldDescriptor *fd = desc->field( i );
        fd_map[fd->number()] = fd;
    }

    string msg_type_name = GetMessageTypeName( desc );

    OutL( "func (p *%s) Serialize(buf []byte) (size int, err error) {", CS(msg_type_name) );
    OutL( "    b := NewProtoBuffer(buf)" );
    OutL( "    err = p.do_serialize(b)" );
    OutL( "    return b.pos, err" );
    OutL( "}" );

    OutL( "func (p *%s) do_serialize(b *ProtoBuffer) error {", CS(msg_type_name) );
    OutL( "    buf_size := len(b.buf) - b.pos" );
    OutL( "    byte_size := p.ByteSize()" );
    OutL( "    if byte_size > buf_size {" );
    OutL( "        return &ProtoError{\"Serialize error, byte_size > buf_size\"}" );
    OutL( "    }\n" );
    if ( HasRepeated( desc ) ) 
        OutL( "    list_count := 0" );

    map<int, const FieldDescriptor*>::iterator pos;
    for ( pos = fd_map.begin(); pos != fd_map.end(); ++pos ) {
        const FieldDescriptor *fd = pos->second;
        const char *c_field_name = CS(fd->name());
        int number = pos->first;
        int wire_type = GetWireType( fd );
        int wire_tag = (number << 3) | wire_type;

        if ( fd->is_repeated() ) {
            OutL( "    list_count = p.Size%s()", c_field_name );
            if ( CanFieldNull( fd ) || wire_type != WIRE_TYPE_LENGTH ) {
                //repeated field,tag + val
                OutL( "    for i := 0; i < list_count; i++ {" );
                OutL( "        b.WriteInt32(%d)", wire_tag );
                BuildFieldSerialize( fd, true );
                OutL( "    }" );
            } else {
                //repeated field,but packed=true
                //write tag
                OutL( "    b.WriteInt32(%d)", wire_tag );
                //write size
                char name_buf[255];
                sprintf( name_buf, "%s_buf_size", c_field_name );
                OutL( "    %s := 0", name_buf );
                OutL( "    for i := 0; i < list_count; i++ {" );
                BuildFieldByteSize( fd, true, name_buf );
                OutL( "    }" );
                OutL( "    if %s > 0 {", name_buf );
                OutL( "        b.WriteInt32(%s)", name_buf );
                OutL( "    }" );
                //write data
                OutL( "    for i := 0; i < list_count; i++ {" );
                BuildFieldSerialize( fd, true );
                OutL( "    }" );
            }
        } else {
            //single 
            OutL( "    if p.Has%s() {", c_field_name );
            OutL( "        b.WriteInt32(%d)", wire_tag );
            BuildFieldSerialize( fd, false );
            OutL( "    }" );
        }
    }

    OutL( "    return nil" );
    OutL( "}\n" );
}


void GOBuilder::BuildFieldSerialize( const FieldDescriptor *fd, bool is_list ) {
    string vn = GetFieldValueName(fd);
    char c_value_name[255];
    if ( is_list )
        sprintf( c_value_name, "p.%s[i]", CS(vn) );
    else
        sprintf( c_value_name, "p.%s", CS(vn) );

    switch ( fd->type() ) {
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_FLOAT:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_BOOL: {
        const char *write_fun = GetFieldWriteFun( fd );
        OutL( "        b.%s(%s)", write_fun, c_value_name );
    } break;
    case FieldDescriptor::TYPE_ENUM:
        OutL( "        b.WriteInt32(%s)", c_value_name );
        break;
    case FieldDescriptor::TYPE_MESSAGE:
        OutL( "        size := %s.ByteSize()", c_value_name );
        OutL( "        b.WriteInt32(size)" );
        OutL( "        %s.do_serialize(b)", c_value_name );
        break;
    default: printf( " BuildFieldSerialize error, type %d is invalid \n", fd->type() );
    }
}


//build ByteSize
void GOBuilder::BuildByteSize( const Descriptor *desc ) {
    string msg_type_name = GetMessageTypeName( desc );

    OutL( "func (p *%s) ByteSize() int {", CS(msg_type_name) );
    OutL( "    if p.cached_byte_size != 0 {" );
    OutL( "        return p.cached_byte_size" );
    OutL( "    }" );

    if ( HasRepeated( desc ) ) 
        OutL( "    list_count := 0" );

    for ( int i = 0; i < desc->field_count(); i++ ) {
        const FieldDescriptor *fd = desc->field( i );
        const char *c_value_name = CS(fd->name());

        int number = fd->number();
        int wire_type = GetWireType( fd );
        int wire_tag = (number << 3) | wire_type;

        if ( fd->is_repeated() ) {
            OutL( "    list_count = p.Size%s()", c_value_name );
            if ( CanFieldNull( fd ) || wire_type != WIRE_TYPE_LENGTH ) {
                //repeated field,tag + val
                OutL( "    for i := 0; i < list_count; i++ {" );
                OutL( "        p.cached_byte_size += WriteInt32Size(%d)", wire_tag );
                BuildFieldByteSize( fd, true, "p.cached_byte_size" );
                OutL( "    }" );
            } else {
                //repeated field,but packed=true
                OutL( "    p.cached_byte_size += WriteInt32Size(%d)", wire_tag );
                OutL( "    %s_old_size := p.cached_byte_size", c_value_name );
                OutL( "    for i := 0; i < list_count; i++ {" );
                BuildFieldByteSize( fd, true, "p.cached_byte_size" );
                OutL( "    }" );
                //data size
                OutL( "    if list_count > 0 {" );
                OutL( "        p.cached_byte_size += WriteInt32Size(p.cached_byte_size - %s_old_size)", c_value_name );
                OutL( "    }" );
            }
        } else {
            //single 
            OutL( "    if p.Has%s() {", c_value_name );
            OutL( "        p.cached_byte_size += WriteInt32Size(%d)", wire_tag );
            BuildFieldByteSize( fd, false, "p.cached_byte_size" );
            OutL( "    }" );
        }
    }

    OutL( "    return p.cached_byte_size" );
    OutL( "}\n" );
}

void GOBuilder::BuildFieldByteSize( const FieldDescriptor *fd, bool is_list, const char *size_var_name ) {
    string vn = GetFieldValueName(fd);
    char c_value_name[255];
    if ( is_list )
        sprintf( c_value_name, "%s[i]", CS(vn) );
    else
        sprintf( c_value_name, "%s", CS(vn) );

    switch ( fd->type() ) {
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:        
        OutL( "        %s += SIZE_64BIT", size_var_name ); break;
    case FieldDescriptor::TYPE_FLOAT:
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:        
        OutL( "        %s += SIZE_32BIT", size_var_name ); break;
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_UINT32:        
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64: {
        const char *write_fun = GetFieldWriteFun( fd );
        OutL( "        %s += %sSize(p.%s)", size_var_name, write_fun, c_value_name );
    } break;
    case FieldDescriptor::TYPE_BOOL:
        OutL( "        %s += WriteInt32Size(BooleanToInt(p.%s))", size_var_name, c_value_name );
        break;
    case FieldDescriptor::TYPE_ENUM:
        OutL( "        %s += WriteInt32Size(p.%s)", size_var_name, c_value_name );
        break;
    case FieldDescriptor::TYPE_STRING:
        OutL( "        size := WriteStringSize(p.%s)", c_value_name );
        OutL( "        %s += WriteInt32Size(size)", size_var_name );
        OutL( "        %s += size", size_var_name );
        break;
    case FieldDescriptor::TYPE_MESSAGE:
        OutL( "        size := p.%s.ByteSize()", c_value_name );
        OutL( "        %s += WriteInt32Size(size)", size_var_name );
        OutL( "        %s += size", size_var_name );
        break;
    case FieldDescriptor::TYPE_BYTES:
        OutL( "        size := len(p.%s)", c_value_name );
        OutL( "        %s += WriteInt32Size(size)", size_var_name );
        OutL( "        %s += size", size_var_name );
        break;
    default: printf( " BuildFieldByteSize error, type %d is invalid \n", fd->type() );
    }
}

void GOBuilder::BuildParse( const Descriptor *desc ) {
    string msg_type_name = GetMessageTypeName( desc );

    OutL( "func (p *%s) Parse(buf []byte, msg_size int) error {", CS(msg_type_name) );
    OutL( "    b := NewProtoBuffer(buf)" );
    OutL( "    return p.do_parse(b, msg_size)" );
    OutL( "}" );
    
    OutL( "func (p *%s) do_parse(b *ProtoBuffer, msg_size int) error {", CS(msg_type_name) );
    OutL( "    msg_end := b.pos + msg_size" );
    OutL( "    if msg_end > len(b.buf) {" );
    OutL( "        return &ProtoError{\"Parse %s error, msg size out of buffer\"}", CS(msg_type_name) );
    OutL( "    }" );
    OutL( "    p.Clear()" );
    OutL( "    for b.pos < msg_end {" );
    OutL( "        wire_tag := b.ReadInt32()" );
    OutL( "        switch wire_tag {" );

    for ( int i = 0; i < desc->field_count(); i++ ) {
        const FieldDescriptor *fd = desc->field( i );
        const char *field_name = CS(fd->name());
        int number = fd->number();
        int wire_type = GetWireType( fd );
        int wire_tag = (number << 3) | wire_type;
        OutL( "        case %d:", wire_tag );
        
        if ( wire_type == WIRE_TYPE_LENGTH && !CanFieldNull(fd) && !IsString(fd) ) {
            //packed=true
            OutL( "            %s_size := b.ReadInt32()", field_name );
            OutL( "            if %s_size > msg_end-b.pos || %s_size < 0 {", field_name, field_name );
            OutL( "                return &ProtoError{\"Parse %s error\"}", CS(msg_type_name) );
            OutL( "            }" );
            OutL( "            %s_end := b.pos + %s_size", field_name, field_name );
            OutL( "            for b.pos < %s_end {", field_name );
            BuildRepeatedFieldParse( fd );
            OutL( "            }" );
            //fix buf position
            OutL( "            b.pos = int(%s_end)", field_name );
        } else {
            if ( fd->is_repeated() )
                BuildRepeatedFieldParse( fd );
            else
                BuildSingularFieldParse( fd );
        }
    }

    //read unknow field
    OutL( "        default: b.GetUnknowFieldValueSize(wire_tag)" );
    OutL( "        }" );
    OutL( "    }\n" );

    OutL( "    if !p.IsInitialized() {" );
    OutL( "        return &ProtoError{\"%s parse error, miss required field\"} ", CS(desc->full_name()) );
    OutL( "    }" );
    OutL( "    return nil" );
    OutL( "}\n" );
}

void GOBuilder::BuildSingularFieldParse( const FieldDescriptor *fd ) {
    string value_name = GetFieldValueName( fd );
    const char *c_value_name = CS(value_name);
    const char *c_field_name = CS(fd->name());

    switch ( fd->type() ) {
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_FLOAT:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_BOOL: {
        const char *read_fun = GetFieldReadFun( fd );
        OutL( "            p.Set%s(b.%s())", c_field_name, read_fun );
    } break;
    case FieldDescriptor::TYPE_ENUM:
        OutL( "            p.Set%s(b.ReadInt32())", c_field_name );
        break;
    case FieldDescriptor::TYPE_MESSAGE: {
        string msg_type_name = GetMessageTypeName( fd->message_type() );
        OutL( "            %s_size := b.ReadInt32()", c_value_name );
        OutL( "            if %s_size > msg_end-b.pos {", c_value_name );
        OutL( "                return &ProtoError{\"parse %s error\"}", CS(msg_type_name) );
        OutL( "            }" );
        OutL( "            %s_tmp := New%s()", c_value_name, CS(msg_type_name) );
        OutL( "            p.Set%s(%s_tmp)", c_value_name, c_value_name );
        OutL( "            if %s_size > 0 {", c_value_name );
        OutL( "                e := p.%s.do_parse(b, int(%s_size))", c_value_name, c_value_name );
        OutL( "                if e != nil {" );
        OutL( "                    return e" );
        OutL( "                }" );
        OutL( "            }" );
    } break;
    default: printf( " BuildFieldParse error, type %d is invalid \n", fd->type() );
    }
}

void GOBuilder::BuildRepeatedFieldParse( const FieldDescriptor *fd ) {
    string value_name = GetFieldValueName( fd );
    const char *c_value_name = CS(value_name);

    switch ( fd->type() ) {
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_FLOAT:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_BOOL:
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_STRING:        
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64: {
        const char *read_fun = GetFieldReadFun( fd );
        OutL( "            p.Add%s(b.%s())", c_value_name, read_fun );
    } break;
    case FieldDescriptor::TYPE_ENUM:
        OutL( "            p.Add%s(b.ReadInt32())", c_value_name );
        break;
    case FieldDescriptor::TYPE_MESSAGE: {
        string msg_type_name = GetMessageTypeName( fd->message_type() );
        OutL( "            %s_size := b.ReadInt32()", c_value_name );
        OutL( "            if %s_size > msg_end-b.pos {", c_value_name );
        OutL( "                return &ProtoError{\"parse %s error\"}", CS(msg_type_name) );
        OutL( "            }" );
        OutL( "            %s_tmp := New%s()", c_value_name, CS(msg_type_name) );
        OutL( "            p.Add%s(%s_tmp)", c_value_name, c_value_name );
        OutL( "            if %s_size > 0 {", c_value_name );
        OutL( "                e := %s_tmp.do_parse(b, int(%s_size))", c_value_name, c_value_name );
        OutL( "                if e != nil {" );
        OutL( "                    return e" );
        OutL( "                }" );
        OutL( "            } else {" );
        OutL( "                return &ProtoError{\"parse %s error\"}", CS(msg_type_name) );
        OutL( "            }" );
    } break;
    default: printf( " BuildFieldParse error, type %d is invalid \n", fd->type() );
    }
}


//build IsInitialized
void GOBuilder::BuildIsInitialized( const Descriptor *desc ) {
    string msg_type_name = GetMessageTypeName( desc );
    int has_flag_count = GetHasFlagCount( desc );
    unsigned int *has_flag = new unsigned int[has_flag_count];
    memset( has_flag, 0, sizeof(unsigned int) * has_flag_count );

    for ( int i = 0; i < desc->field_count(); i++ ) {
        const FieldDescriptor *fd = desc->field( i );
        if ( !fd->is_required() )
            continue;
        //only required field
        int has_flag_index = fd->index() / INT_BIT_COUNT;
        int bit_index = fd->index() % INT_BIT_COUNT;
        has_flag[has_flag_index] |= 1 << bit_index;
    }

    OutL( "func (p *%s) IsInitialized() bool {", CS(msg_type_name) );
    for ( int i = 0; i < has_flag_count; i++ ) {
        if ( has_flag[i] == 0 )
            continue;
        OutL( "    if (p.has_flag_%d & 0x%x) != 0x%x {", i, has_flag[i], has_flag[i] );
        OutL( "        return false" );
        OutL( "    }" );
    }
    OutL( "    return true" );
    OutL( "}\n" );
    
    delete[] has_flag;
}

void GOBuilder::BuildField( const FieldDescriptor *fd, const string &msg_type_name ) {
    OutL( "//%s", CS(fd->name()) );
    if ( fd->is_repeated() ) {
        BuildRepeatedField( fd, CS(msg_type_name) );
    } else 
        BuildSingularField( fd, CS(msg_type_name) );
}

void GOBuilder::BuildSingularField( const FieldDescriptor *fd, const string &msg_type_name ) {
    string value_name = GetFieldValueName( fd );
    string type_name = GetFieldTypeName( fd );
    string default_val = GetFieldDefaultValue( fd );

    const char *c_field_name = CS(fd->name());    
    const char *c_type_name = CS(type_name );
    const char *c_value_name = CS(value_name);
    const char *c_msg_type_name = CS(msg_type_name);
    const char *c_def_val = CS(default_val);

    int has_flag_index = fd->index() / INT_BIT_COUNT;
    int bit_index = fd->index() % INT_BIT_COUNT;
    int bit_flag = 1 << bit_index;


    OutL( "func (p *%s) Get%s() %s {", c_msg_type_name, c_field_name, c_type_name );
    OutL( "    return p.%s", c_value_name );
    OutL( "}" );

    OutL( "func (p *%s) Set%s(v %s) {", c_msg_type_name, c_field_name, c_type_name );
    OutL( "    p.%s = v", c_value_name );
    if ( CanFieldNull( fd ) ) {
        //nullable field
        OutL( "    if v == nil {" );
        OutL( "        p.has_flag_%d &= 0x%x", has_flag_index, ~bit_flag );
        OutL( "    } else {" );
        OutL( "        p.has_flag_%d |= 0x%x", has_flag_index, bit_flag );
        OutL( "    }" );
    } else 
        OutL( "    p.has_flag_%d |= 0x%x", has_flag_index, bit_flag );
    OutL( "    p.cached_byte_size = 0" );
    OutL( "}" );

    //build has
    OutL( "func (p *%s) Has%s() bool {", c_msg_type_name, c_field_name );
    OutL( "    return (p.has_flag_%d & 0x%x) != 0", has_flag_index, bit_flag );
    OutL( "}" );

    //build clear
    OutL( "func (p *%s) Clear%s() {", c_msg_type_name, c_field_name );
    OutL( "    p.%s = %s", c_value_name, c_def_val );
    OutL( "    p.has_flag_%d &= 0x%x", has_flag_index, ~bit_flag );
    OutL( "    p.cached_byte_size = 0" );
    OutL( "}" );

    OutL( "" );    
}

void GOBuilder::BuildRepeatedField( const FieldDescriptor *fd, const string &msg_type_name ) {
    string value_name = GetFieldValueName( fd );
    string type_name = GetFieldTypeName( fd );

    const char *c_field_name = CS(fd->name());    
    const char *c_type_name = CS(type_name );
    const char *c_value_name = CS(value_name);
    const char *c_msg_type_name = CS(msg_type_name);

    OutL( "func (p *%s) Size%s() int { return len(p.%s) }",
          c_msg_type_name, c_field_name, c_value_name );
    OutL( "func (p *%s) Get%s(index int) %s { return p.%s[index] }",
          c_msg_type_name, c_field_name, c_type_name, c_value_name );
    OutL( "func (p *%s) Add%s(v %s) {", c_msg_type_name, c_field_name, c_type_name );
    OutL( "    p.%s = append(p.%s, v)", c_value_name, c_value_name );
    OutL( "    p.cached_byte_size = 0" );
    OutL( "}" );
    OutL( "func (p *%s) Clear%s() { ", c_msg_type_name, c_field_name );
    OutL( "    p.%s = make([]%s, 0)", c_value_name, c_type_name );
    OutL( "    p.cached_byte_size = 0" );
    OutL( "}" );

    OutL( "" );
}


//build enum
void GOBuilder::BuildEnum( const EnumDescriptor *desc ) {
    OutL( "const (" );

    string type_name = GetEnumTypeName( desc );
    for ( int i = 0; i < desc->value_count(); i++ ) {
        const EnumValueDescriptor *vd = desc->value( i );
        OutL( "    %s_%s = %d", CS(type_name), CS(vd->name()), vd->number() );
    }

    OutL( ")" );
    OutL( "" );
}


void GOBuilder::BuildClear( const Descriptor *desc ) {
    string msg_type_name = GetMessageTypeName( desc );

    OutL( "func (p *%s) Clear() {", CS(msg_type_name) );
    for ( int i = 0; i < desc->field_count(); i++ ) {
        const FieldDescriptor *fd = desc->field( i );
        string value_name = GetFieldValueName( fd );
        string type_name = GetFieldTypeName( fd );
        string default_val = GetFieldDefaultValue( fd );

        if ( fd->is_repeated() )
            OutL( "    p.%s = make([]%s, 0)", CS(value_name), CS(type_name) );
        else 
            OutL( "    p.%s = %s", CS(value_name), CS(default_val) );
    }

    OutL( "    p.cached_byte_size = 0" );
    //clear has_flag
    if ( !IsAllRepeated( desc ) ) {
        int has_flag_count = GetHasFlagCount( desc );
        for ( int i = 0; i < has_flag_count; i++ )
            OutL( "    p.has_flag_%d = 0", i );
    }
    OutL( "}\n" );
}


void GOBuilder::OutL( const char *fmt, ... ) {
    const int MAX_LINE_SIZE = 10240;
    char buf[MAX_LINE_SIZE];

    va_list arg;
    va_start( arg, fmt );
    int n = vsnprintf( buf, MAX_LINE_SIZE, fmt, arg );
    va_end( arg );

    if ( n > MAX_LINE_SIZE )
        n = MAX_LINE_SIZE;
    buf[n] = '\n';
    buf[n+1] = '\0';

    if ( out_file_ == NULL ) {
        printf( "%s", buf );
    } else {
        fputs( buf, out_file_ );
        fflush( out_file_ );
    }
}

void GOBuilder::Out( const char *fmt, ... ) {
    const int MAX_LINE_SIZE = 10240;
    char buf[MAX_LINE_SIZE];

    va_list arg;
    va_start( arg, fmt );
    int n = vsnprintf( buf, MAX_LINE_SIZE, fmt, arg );
    va_end( arg );

    if ( n > MAX_LINE_SIZE )
        n = MAX_LINE_SIZE;
    buf[n] = '\0';

    if ( out_file_ == NULL ) {
        printf( "%s", buf );
    } else {
        fputs( buf, out_file_ );
        fflush( out_file_ );
    }
}
