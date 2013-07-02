#ifndef _PBUF_HELPER_H_
#define _PBUF_HELPER_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "common.h"

struct FieldType {
    FieldDescriptor::Type type;
    string go_type_name;
    bool simple_type;
    int wire_type;
    const char *write_fun_name;
    const char *read_fun_name;
};

void SetHasPackage( bool has_package );
FieldType* GetFieldType( FieldDescriptor::Type type );

string GetFieldDefaultValue( const FieldDescriptor *fd );
string GetFieldValueName( const FieldDescriptor *fd );
string GetFieldTypeName( const FieldDescriptor *fd );
bool FieldIsMessage( const FieldDescriptor *fd );

string GetMessageTypeName( const Descriptor *desc );
string GetEnumTypeName( const EnumDescriptor *desc );

int GetWireType( const FieldDescriptor *fd );
int GetHasFlagCount( const Descriptor *desc );
bool CanFieldNull( const FieldDescriptor *fd );
bool HasRepeated( const Descriptor *desc );
bool IsAllRepeated( const Descriptor *desc );
bool IsString( const FieldDescriptor *fd );
bool HasLengthField( const Descriptor *desc );
const char* GetFieldWriteFun( const FieldDescriptor *fd );
const char* GetFieldReadFun( const FieldDescriptor *fd );

string NameLower( string &name );
string NameUpper( string &name );

#endif//_PBUF_HELPER_H_
