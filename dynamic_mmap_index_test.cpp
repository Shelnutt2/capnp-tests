#include <capnp/message.h>
#include <capnp/serialize.h>
#include <iostream>
#include <capnp/schema.h>
#include <capnp/dynamic.h>
#include <capnp/schema-parser.h>
#include <kj/io.h>

#include <json-to-capnp.hpp>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include <map>
#include <unordered_map>
#include <sys/mman.h>
#include <sys/stat.h>


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

void dynamicWriteAddressBook(int fd, StructSchema schema) {
  // Write a message using the dynamic API to set each
  // field by text name.  This isn't something you'd
  // normally want to do; it's just for illustration.

  MallocMessageBuilder message;
  MallocMessageBuilder message2;

  // Types shown for explanation purposes; normally you'd
  // use auto.
  DynamicStruct::Builder date1 = message.initRoot<DynamicStruct>(schema);

  date1.set("year", 2017);
  date1.set("month", 07);
  date1.set("day", 06);

  DynamicStruct::Builder date2 = message2.initRoot<DynamicStruct>(schema);
  date2.set("year", 2017);
  date2.set("month", 07);
  date2.set("day", 07);

  //capnp::writeMessageToFd(fd, message);
  capnp::writeMessageToFd(fd, message);
  capnp::writeMessageToFd(fd, message2);

  for (int i = 0; i < 2017; i++) {
    MallocMessageBuilder message;
    DynamicStruct::Builder datex = message.initRoot<DynamicStruct>(schema);
    datex.set("year", i);
    datex.set("month", 07);
    datex.set("day", 07);
    capnp::writeMessageToFd(fd, message);
  }

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

void dynamicPrintMessage(std::FILE* file, StructSchema schema) {
  int fd = fileno(file);
  capnp::StreamFdMessageReader message(fd);
  dynamicPrintValue(message.getRoot<DynamicStruct>(schema));
  std::cout << std::endl;

  capnp::StreamFdMessageReader message2(fd);
  dynamicPrintValue(message2.getRoot<DynamicStruct>(schema));
  std::cout << std::endl;

  int c;

  fpos_t pos;
  while(!feof(file)) {
    fgetpos (file,&pos);
    c = fgetc(file);
    if(c == EOF)
      break;
    else
      fsetpos (file,&pos);
    capnp::StreamFdMessageReader message(fd);
    dynamicPrintValue(message.getRoot<DynamicStruct>(schema));
    std::cout << std::endl;
  }

}

void dynamicPrintMMapMessage(capnp::word *data, size_t mv_size, StructSchema schema){
  //kj::ArrayPtr<const word> arrayPtr;
  capnp::FlatArrayMessageReader message(kj::ArrayPtr<const capnp::word>(data, mv_size / sizeof(capnp::word)));

  dynamicPrintValue(message.getRoot<DynamicStruct>(schema));
  std::cout << std::endl;

  while(message.getEnd() != data+(mv_size / sizeof(capnp::word))) {
    message = capnp::FlatArrayMessageReader(kj::ArrayPtr<const capnp::word>(message.getEnd(), data+(mv_size / sizeof(capnp::word))));
    dynamicPrintValue(message.getRoot<DynamicStruct>(schema));
    std::cout << std::endl;
  }
}

size_t getFilesize(const char* filename) {
  struct stat st;
  stat(filename, &st);
  return st.st_size;
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
  std::string schemaString = buildCapnpLimitedSchema(&columns, structName, &err);


  char* filePath = std::tmpnam(NULL);
  std::FILE* tmpf = fopen(filePath, "w+");
  std::fputs(schemaString.c_str(), tmpf);
  std::rewind(tmpf);
  fclose(tmpf);

  std::cout << filePath << std::endl;

  capnp::ParsedSchema parsedSchema = parser.parseDiskFile(structName, filePath, {"/usr/include"});
  StructSchema schema = parsedSchema.getNested(structName).asStruct();

  char* messageFilePath = std::tmpnam(NULL);

  std::FILE* messageFile = fopen(messageFilePath, "w+");
  int fd = fileno(messageFile);
  std::cout << messageFilePath << std::endl;

  dynamicWriteAddressBook(fd, schema);
  std::rewind(messageFile);
  fclose(messageFile);

  fd = open(messageFilePath, O_RDONLY);
  size_t size = getFilesize(messageFilePath);
  void *mmappedData = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);

  if(mmappedData == MAP_FAILED) {
    close(fd);
    std::cerr << "mmaped failed for " << messageFilePath << std::endl;
    return -1;
  }
  //dynamicPrintMessage(messageFile, schema);

  dynamicPrintMMapMessage((capnp::word*)mmappedData, size, schema);

  if (munmap(mmappedData, size) == -1) {
    perror("Error un-mmapping the file");
  }
  close(fd);

  //std::remove(filePath);
  //std::remove(messageFilePath);
  return 0;
}

