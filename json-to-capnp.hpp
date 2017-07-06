//
// Created by seth on 7/6/17.
//

#ifndef ADDRESSBOOK_JSON_TO_CAPNP_HPP_HPP
#define ADDRESSBOOK_JSON_TO_CAPNP_HPP_HPP

#include <json.hpp>
#include <string>
#include <unordered_map>
#include <map>

#if !_MSC_VER
#include <unistd.h>
#endif
#include <fcntl.h>


uint64_t generateRandomId() {
  uint64_t result;

#if _WIN32
  HCRYPTPROV handle;
  KJ_ASSERT(CryptAcquireContextW(&handle, nullptr, nullptr,
                                 PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT));
  KJ_DEFER(KJ_ASSERT(CryptReleaseContext(handle, 0)) {break;});

  KJ_ASSERT(CryptGenRandom(handle, sizeof(result), reinterpret_cast<BYTE*>(&result)));

#else
  int fd;
  KJ_SYSCALL(fd = open("/dev/urandom", O_RDONLY));

  ssize_t n;
  KJ_SYSCALL(n = read(fd, &result, sizeof(result)), "/dev/urandom");
      KJ_ASSERT(n == sizeof(result), "Incomplete read from /dev/urandom.", n);
#endif

  return result | (1ull << 63);
}

/*std::string buildCapnpSchema(nlohmann::json Map){

};*/

std::string buildCapnpSchema(std::map<u_int, std::unordered_map<std::string, std::string>> *columnMap,
                             std::string structName, int *err, uint64_t id = 0) {

  if(id == 0) {
    id = generateRandomId();
  }
  std::string output = kj::str("@0x", kj::hex(id), ";\n").cStr();
  output += "struct Columns {\n";

  for(auto const &column : *columnMap) {
    u_int columnOrder = column.first;
    std::unordered_map<std::string, std::string> columnDetails = column.second;

    if(columnDetails.find("column_name") == columnDetails.end()) {
      *err = -1;
      return "";
    }

    if(columnDetails.find("column_type") == columnDetails.end()) {
      *err = -1;
      return "";
    }

    output += "  " + columnDetails.find("column_name")->second + " @" + std::to_string(columnOrder) + " :" + columnDetails.find("column_type")->second + ";\n";

  }

  output += "}\n";

  output += "struct " + structName + " {\n";
  output += "  rows @0 :List(Columns);\n";
  output += "}";

  return output;
};

#endif //ADDRESSBOOK_JSON_TO_CAPNP_HPP_HPP
