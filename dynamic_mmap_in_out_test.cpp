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
#include <regex>


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

std::vector<std::string> split(const std::string& input, const std::string& regex) {
  // passing -1 as the submatch index parameter performs splitting
  std::regex re(regex);
  std::sregex_token_iterator
      first{input.begin(), input.end(), re, -1},
      last;
  return {first, last};
}

int main(int argc, char* argv[]) {
  //StructSchema schema;// = Schema::from<AddressBook>();

  if(argc != 3) {
    printf("Need two argument, scehma_file_path and data_file_path\n");
    return -1;
  }
  printf("Running with %s - %s\n", argv[1], argv[2]);
  ::capnp::SchemaParser parser;

  int err;

  std::vector<std::string> filePathParts = split(argv[1], "/");

  std::string fileName = filePathParts[filePathParts.size()-1];

  printf("fileName: %s\n", fileName.c_str());


  std::string structName = split(fileName, "\\.")[0];
  structName[0] = toupper(structName[0]);

  printf("structName: %s\n", structName.c_str());

  capnp::ParsedSchema parsedSchema = parser.parseDiskFile(structName, argv[1], {"/usr/include"});
  StructSchema schema = parsedSchema.getNested(structName).asStruct();

  int fd = open(argv[2], O_RDONLY);
  size_t size = getFilesize(argv[2]);
  std::cout << "file size: " << size << std::endl;
  void *mmappedData = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);

  if(mmappedData == MAP_FAILED) {
    close(fd);
    std::cerr << "mmaped failed for " << argv[2] << std::endl;
    perror("Error ");
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

