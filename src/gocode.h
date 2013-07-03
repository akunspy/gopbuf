#ifndef _GO_CODE_H_
#define _GO_CODE_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "common.h"
#include "pbuf_helper.h"

class GOBuilder {
public:
    GOBuilder();
    virtual ~GOBuilder();

    void OutL( const char *fmt, ... );
    void Out( const char *fmt, ... );
    void Build( const FileDescriptorList *filelist,
                const DescriptorList *msglist,
                const EnumDescriptorList *enumlist );
private:
    void BuildMessage( const Descriptor *desc );
    void BuildEnum( const EnumDescriptor *desc );

    void BuildMessageNew( const Descriptor *desc );

    void BuildFieldDefine( const Descriptor *desc );
    void BuildField( const FieldDescriptor *desc, const string &msg_name );
    void BuildSingularField( const FieldDescriptor *fd, const string &msg_name );
    void BuildRepeatedField( const FieldDescriptor *fd, const string &msg_name );
    
    void BuildClear( const Descriptor *desc );
    void BuildIsInitialized( const Descriptor *desc );

    void BuildPrint( const Descriptor *desc );
    void BuildFieldPrint( const FieldDescriptor *fd, bool is_list );
    
    void BuildSerialize( const Descriptor *desc );
    void BuildFieldSerialize( const FieldDescriptor *fd, bool is_list );

    void BuildParse( const Descriptor *desc );
    void BuildSingularFieldParse( const FieldDescriptor *fd );
    void BuildRepeatedFieldParse( const FieldDescriptor *fd );
    
    void BuildByteSize( const Descriptor *desc );
    void BuildFieldByteSize( const FieldDescriptor *fd, bool is_list, const char *var_name );

    bool ProcessPackageName( const FileDescriptorList *filelist );

    FILE *out_file_;
    string package_name_;
};

#endif//_GO_CODE_H_
