#include "tests.h"

#include <botan/libstate.h>
#include <botan/mac.h>
#include <botan/hex.h>
#include <iostream>
#include <fstream>

using namespace Botan;

namespace {

size_t mac_test(const std::string& algo,
                const std::string& key_hex,
                const std::string& in_hex,
                const std::string& out_hex)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();

   const auto providers = af.providers_of(algo);
   size_t fails = 0;

   if(providers.empty())
      {
      std::cout << "Unknown algo " << algo << "\n";
      ++fails;
      }

   for(auto provider: providers)
      {
      auto proto = af.prototype_mac(algo, provider);

      if(!proto)
         {
         std::cout << "Unable to get " << algo << " from " << provider << "\n";
         ++fails;
         continue;
         }

      std::unique_ptr<MessageAuthenticationCode> mac(proto->clone());

      mac->set_key(hex_decode(key_hex));
      mac->update(hex_decode(in_hex));

      auto h = mac->final();

      if(h != hex_decode_locked(out_hex))
         {
         std::cout << algo << " " << provider << " got " << hex_encode(h) << " != " << out_hex << "\n";
         ++fails;
         }
      }

   return fails;
   }

}

size_t test_mac()
   {
   auto test = [](const std::string& input)
      {
      std::ifstream vec(input);

      return run_tests_bb(vec, "Mac", "Out", true,
             [](std::map<std::string, std::string> m) -> size_t
             {
             return mac_test(m["Mac"], m["Key"], m["In"], m["Out"]);
             });
      };

   return run_tests_in_dir(TEST_DATA_DIR "mac", test);
   }
