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

void dynamicPrintValue(DynamicValue::Reader value);
void dynamicPrintMessage(int fd, StructSchema schema);

void dynamicWriteAddressBook(int fd, StructSchema schema) {
  // Write a message using the dynamic API to set each
  // field by text name.  This isn't something you'd
  // normally want to do; it's just for illustration.

  MallocMessageBuilder message;

  // Types shown for explanation purposes; normally you'd
  // use auto.
  auto addressBook = message.initRoot<DynamicStruct>(schema);

  DynamicList::Builder people =
      addressBook.init("people", 2).as<DynamicList>();

  DynamicStruct::Builder alice =
      people[0].as<DynamicStruct>();
  alice.set("id", 123);
  alice.set("name", "Alice");
  alice.set("email", "alice@example.com");
  auto alicePhones = alice.init("phones", 1).as<DynamicList>();
  auto phone0 = alicePhones[0].as<DynamicStruct>();
  phone0.set("number", "555-1212");
  phone0.set("type", "mobile");
  alice.get("employment").as<DynamicStruct>()
       .set("school", "MIT");

  auto bob = people[1].as<DynamicStruct>();
  bob.set("id", 456);
  bob.set("name", "Bob");
  bob.set("email", "bob@example.com");

  // Some magic:  We can convert a dynamic sub-value back to
  // the native type with as<T>()!
  auto bobPhones =
      bob.init("phones", 2).as<DynamicList>();
  auto bobPhone0 = bobPhones[0].as<DynamicStruct>();
  auto bobPhone1 = bobPhones[1].as<DynamicStruct>();
  bobPhone0.set("number", "555-4567");
  bobPhone0.set("type", "home");
  bobPhone1.set("number","555-7654");
  bobPhone1.set("type", "work");
  bob.get("employment").as<DynamicStruct>()
     .set("unemployed", ::capnp::VOID);

  //writePackedMessageToFd(fd, message);
  //writeMessageToFd(fd, message);
  kj::Array<capnp::word> words = messageToFlatArray(message);
  //kj::ArrayPtr<kj::byte> bytes = words.asBytes();
  //write(fd, bytes.begin(), bytes.size())
  void* buff = malloc(words.size() * sizeof(capnp::word));
  memcpy(buff, words.begin(), words.size() * sizeof(capnp::word));

  std::cout << buff << std::endl;

  kj::Array<capnp::word> serializedRead = kj::heapArray<capnp::word>(words.size());
  memcpy(serializedRead.begin(), words.begin(), words.size() * sizeof(capnp::word));

  capnp::FlatArrayMessageReader reader(serializedRead.asPtr());

  dynamicPrintValue(reader.getRoot<DynamicStruct>(schema));
  std::cout << std::endl;

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
  printf("Running with %s, %s\n", argv[1], argv[2]);
  ::capnp::SchemaParser parser;
  capnp::ParsedSchema parsedSchema = parser.parseDiskFile("addressbook", argv[2], {"/usr/include"});
  StructSchema schema = parsedSchema.getNested("AddressBook").asStruct();
  if (argc != 3) {
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

