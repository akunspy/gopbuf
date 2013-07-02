#include "common.h"
#include "gocode.h"

class ErrorCollector: public compiler::MultiFileErrorCollector {
public:
    virtual void AddError(const string& filename, int line, int column,
                          const string& message) {
        printf( "Error at %s:%d  %s \n", filename.c_str(), line, message.c_str() );
    }
};

void AddEnumDescriptor( const EnumDescriptor *desc, EnumDescriptorList *enum_list ) {
    (*enum_list)[desc->full_name()] = desc;
}

void AddDescriptor( const Descriptor *desc, DescriptorList *msg_list, EnumDescriptorList *enum_list ) {
    (*msg_list)[desc->full_name()] = desc;

    for ( int i = 0; i < desc->nested_type_count(); i++ ) 
        AddDescriptor( desc->nested_type( i ), msg_list, enum_list );
    for ( int i = 0; i < desc->enum_type_count(); i++ ) 
        AddEnumDescriptor( desc->enum_type( i ), enum_list );
}

void ParseFile( const FileDescriptor *file, FileDescriptorList *file_list,
                DescriptorList *msg_list, EnumDescriptorList *enum_list ) {
    (*file_list)[file->name()] = file;
    for ( int i = 0; i < file->dependency_count(); i++ ) 
        ParseFile( file->dependency(i), file_list, msg_list, enum_list );
    for ( int i = 0; i < file->message_type_count(); i++ ) 
        AddDescriptor( file->message_type( i ), msg_list, enum_list );
    for ( int i = 0; i < file->enum_type_count(); i++ ) 
        AddEnumDescriptor( file->enum_type( i ), enum_list );
}

int main( int argc, const char *argv[] ) {
    if ( argc < 2 ) {
        printf( "Usage: gopbuf file \n" );
        return -1;
    }

    //parse proto file
    ErrorCollector error_collector;
    compiler::DiskSourceTree source_tree;
    compiler::Importer importer( &source_tree, &error_collector );
    source_tree.MapPath( "", "./" );

    FileDescriptorList *file_list = new FileDescriptorList();
    DescriptorList *msg_list = new DescriptorList();
    EnumDescriptorList *enum_list = new EnumDescriptorList();


    for ( int i = 1; i < argc; i++ ) {
        const FileDescriptor *file =  importer.Import( argv[i] );
        if( file == NULL )
            break;
        ParseFile( file, file_list, msg_list, enum_list );
    }

    GOBuilder gb;
    gb.Build( file_list, msg_list, enum_list );

    delete msg_list;
    delete enum_list;
    delete file_list;

    return 0;
}

