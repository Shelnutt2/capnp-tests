// Copyright (c) 2013-2014 Sandstorm Development Group, Inc. and contributors
// Licensed under the MIT License:
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// This sample code appears in the documentation for the C++ implementation.
//
// If Cap'n Proto is installed, build the sample like:
//   capnp compile -oc++ addressbook.capnp
//   c++ -std=c++11 -Wall addressbook.c++ addressbook.capnp.c++ `pkg-config --cflags --libs capnp` -o addressbook
//
// If Cap'n Proto is not installed, but the source is located at $SRC and has been
// compiled in $BUILD (often both are simply ".." from here), you can do:
//   $BUILD/capnp compile -I$SRC/src -o$BUILD/capnpc-c++ addressbook.capnp
//   c++ -std=c++11 -Wall addressbook.c++ addressbook.capnp.c++ -I$SRC/src -L$BUILD/.libs -lcapnp -lkj -o addressbook
//
// Run like:
//   ./addressbook write | ./addressbook read
// Use "dwrite" and "dread" to use dynamic code instead.

//#include "addressbook.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <iostream>
#include <capnp/schema.h>
#include <capnp/dynamic.h>
#include <capnp/schema-parser.h>

#include <json-to-capnp.hpp>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include <map>
#include <unordered_map>


using ::capnp::DynamicValue;
using ::capnp::DynamicStruct;
using ::capnp::DynamicEnum;
using ::capnp::DynamicList;
using ::capnp::List;
using ::capnp::Schema;
using ::capnp::StructSchema;
using ::capnp::EnumSchema;

using ::capnp::Void;
using ::capnp::Text;
using ::capnp::MallocMessageBuilder;
using ::capnp::PackedFdMessageReader;

void dynamicWriteAddressBook(int fd, StructSchema schema) {
  // Write a message using the dynamic API to set each
  // field by text name.  This isn't something you'd
  // normally want to do; it's just for illustration.

  MallocMessageBuilder message;

  // Types shown for explanation purposes; normally you'd
  // use auto.
  auto calender = message.initRoot<DynamicStruct>(schema);

  DynamicList::Builder date =
      calender.init("rows", 2).as<DynamicList>();

  DynamicStruct::Builder date1 =
      date[0].as<DynamicStruct>();
  date1.set("year", 2017);
  date1.set("month", 07);
  date1.set("day", 06);

  auto date2 = date[1].as<DynamicStruct>();
  date2.set("year", 2017);
  date2.set("month", 07);
  date2.set("day", 07);

  writePackedMessageToFd(fd, message);
}

void dynamicPrintValue(DynamicValue::Reader value) {
  // Print an arbitrary message via the dynamic API by
  // iterating over the schema.  Look at the handling
  // of STRUCT in particular.

  switch (value.getType()) {
    case DynamicValue::VOID:
      std::cout << "";
      break;
    case DynamicValue::BOOL:
      std::cout << (value.as<bool>() ? "true" : "false");
      break;
    case DynamicValue::INT:
      std::cout << value.as<int64_t>();
      break;
    case DynamicValue::UINT:
      std::cout << value.as<uint64_t>();
      break;
    case DynamicValue::FLOAT:
      std::cout << value.as<double>();
      break;
    case DynamicValue::TEXT:
      std::cout << '\"' << value.as<Text>().cStr() << '\"';
      break;
    case DynamicValue::LIST: {
      std::cout << "[";
      bool first = true;
      for (auto element: value.as<DynamicList>()) {
        if (first) {
          first = false;
        } else {
          std::cout << ", ";
        }
        dynamicPrintValue(element);
      }
      std::cout << "]";
      break;
    }
    case DynamicValue::ENUM: {
      auto enumValue = value.as<DynamicEnum>();
      KJ_IF_MAYBE(enumerant, enumValue.getEnumerant()) {
        std::cout <<
            enumerant->getProto().getName().cStr();
      } else {
        // Unknown enum value; output raw number.
        std::cout << enumValue.getRaw();
      }
      break;
    }
    case DynamicValue::STRUCT: {
      std::cout << "(";
      auto structValue = value.as<DynamicStruct>();
      bool first = true;
      for (auto field: structValue.getSchema().getFields()) {
        if (!structValue.has(field)) continue;
        if (first) {
          first = false;
        } else {
          std::cout << ", ";
        }
        std::cout << field.getProto().getName().cStr()
                  << " = ";
        dynamicPrintValue(structValue.get(field));
      }
      std::cout << ")";
      break;
    }
    default:
      // There are other types, we aren't handling them.
      std::cout << "?";
      break;
  }
}

void dynamicPrintMessage(int fd, StructSchema schema) {
  PackedFdMessageReader message(fd);
  dynamicPrintValue(message.getRoot<DynamicStruct>(schema));
  std::cout << std::endl;
}

int main(int argc, char* argv[]) {
  //StructSchema schema;// = Schema::from<AddressBook>();
  printf("Running with %s\n", argv[1]);
  ::capnp::SchemaParser parser;
  std::map<u_int , std::unordered_map<std::string, std::string>> columns;

  std::unordered_map<std::string, std::string> year= {{"column_name", "year"}, {"column_type", "Int16"}};
  std::unordered_map<std::string, std::string> month= {{"column_name", "month"}, {"column_type", "Int8"}};
  std::unordered_map<std::string, std::string> day= {{"column_name", "day"}, {"column_type", "Int8"}};

  columns.emplace(0, year);
  columns.emplace(1, month);
  columns.emplace(2, day);


  int err;
  std::string structName = "Date";
  std::string schemaString = buildCapnpSchema(&columns, structName, &err);


  std::cout << schemaString << std::endl;

  char* filePath = std::tmpnam(NULL);
  std::FILE* tmpf = fopen(filePath, "w");
  std::fputs(schemaString.c_str(), tmpf);
  std::rewind(tmpf);
  fclose(tmpf);


  std::cout << filePath << std::endl;

  capnp::ParsedSchema parsedSchema = parser.parseDiskFile(structName, filePath, {"/usr/include"});
  StructSchema schema = parsedSchema.getNested(structName).asStruct();
  remove(filePath);

  if (argc != 2) {
    std::cerr << "Missing arg." << std::endl;
    return 1;
  } else if (strcmp(argv[1], "dwrite") == 0) {


    /*kj::StringPtr location = kj::Stringstring
    ::kj::ArrayPtr<::kj::StringPtr> imports = kj::arrayPtr("/usr/include", 12).;
    //imports.begin(= kj::StringPtr("/usr/include");*/

    dynamicWriteAddressBook(1, schema);
  } else if (strcmp(argv[1], "dread") == 0) {
    dynamicPrintMessage(0, schema);
  } else {
    std::cerr << "Invalid arg: " << argv[1] << std::endl;
    return 1;
  }
  return 0;
}

