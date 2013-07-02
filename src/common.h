#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sstream>
#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include <list>
#include <fstream>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/compiler/importer.h>

using namespace std;
using namespace google::protobuf;


typedef map<string, const Descriptor*>      DescriptorList;
typedef map<string, const EnumDescriptor*>  EnumDescriptorList;
typedef map<string, const FileDescriptor*>  FileDescriptorList;


#define INT_BIT_COUNT   32
#define WIRE_TYPE_VARINT  0
#define WIRE_TYPE_64BIT   1
#define WIRE_TYPE_LENGTH  2
#define WIRE_TYPE_GROUP   3
#define WIRE_TYPE_32_BIT  5

#endif//_COMMON_H_
