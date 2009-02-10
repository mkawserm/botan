/*************************************************
* IF Scheme Source File                          *
* (C) 1999-2007 Jack Lloyd                       *
*************************************************/

#include <botan/if_algo.h>
#include <botan/numthry.h>
#include <botan/der_enc.h>
#include <botan/ber_dec.h>

namespace Botan {

/**
* Return the X.509 subjectPublicKeyInfo for a RSA/RW key
*/
std::pair<AlgorithmIdentifier, MemoryVector<byte> >
IF_Scheme_PublicKey::subject_public_key_info() const
   {
   DER_Encoder key_bits;

   key_bits.start_cons(SEQUENCE)
              .encode(this->get_n())
              .encode(this->get_e())
           .end_cons();

   AlgorithmIdentifier alg_id(this->get_oid(),
                              AlgorithmIdentifier::USE_NULL_PARAM);

   return std::make_pair(alg_id, key_bits.get_contents());
   }

std::pair<AlgorithmIdentifier, SecureVector<byte> >
IF_Scheme_PrivateKey::pkcs8_encoding() const
   {
   AlgorithmIdentifier alg_id(this->get_oid(),
                              AlgorithmIdentifier::USE_NULL_PARAM);

   SecureVector<byte> key_bits =
      DER_Encoder()
        .start_cons(SEQUENCE)
           .encode(static_cast<u32bit>(0))
           .encode(this->n)
           .encode(this->e)
           .encode(this->d)
           .encode(this->p)
           .encode(this->q)
           .encode(this->d1)
           .encode(this->d2)
           .encode(this->c)
        .end_cons()
        .get_contents();

   return std::make_pair(alg_id, key_bits);
   }

/*************************************************
* Return the PKCS #8 public key decoder          *
*************************************************/
PKCS8_Decoder* IF_Scheme_PrivateKey::pkcs8_decoder(RandomNumberGenerator& rng)
   {
   class IF_Scheme_Decoder : public PKCS8_Decoder
      {
      public:
         void alg_id(const AlgorithmIdentifier&) {}

         void key_bits(const MemoryRegion<byte>& bits)
            {
            u32bit version;

            BER_Decoder(bits)
               .start_cons(SEQUENCE)
                  .decode(version)
                  .decode(key->n)
                  .decode(key->e)
                  .decode(key->d)
                  .decode(key->p)
                  .decode(key->q)
                  .decode(key->d1)
                  .decode(key->d2)
                  .decode(key->c)
               .end_cons();

            if(version != 0)
               throw Decoding_Error("Unknown PKCS #1 key format version");

            key->PKCS8_load_hook(rng);
            }

         IF_Scheme_Decoder(IF_Scheme_PrivateKey* k, RandomNumberGenerator& r) :
            key(k), rng(r) {}
      private:
         IF_Scheme_PrivateKey* key;
         RandomNumberGenerator& rng;
      };

   return new IF_Scheme_Decoder(this, rng);
   }

/*************************************************
* Algorithm Specific X.509 Initialization Code   *
*************************************************/
void IF_Scheme_PublicKey::X509_load_hook()
   {
   core = IF_Core(e, n);
   }

/*************************************************
* Algorithm Specific PKCS #8 Initialization Code *
*************************************************/
void IF_Scheme_PrivateKey::PKCS8_load_hook(RandomNumberGenerator& rng,
                                           bool generated)
   {
   if(n == 0)  n = p * q;
   if(d1 == 0) d1 = d % (p - 1);
   if(d2 == 0) d2 = d % (q - 1);
   if(c == 0)  c = inverse_mod(q, p);

   core = IF_Core(rng, e, n, d, p, q, d1, d2, c);

   if(generated)
      gen_check(rng);
   else
      load_check(rng);
   }

/*************************************************
* Check IF Scheme Public Parameters              *
*************************************************/
bool IF_Scheme_PublicKey::check_key(RandomNumberGenerator&, bool) const
   {
   if(n < 35 || n.is_even() || e < 2)
      return false;
   return true;
   }

/*************************************************
* Check IF Scheme Private Parameters             *
*************************************************/
bool IF_Scheme_PrivateKey::check_key(RandomNumberGenerator& rng,
                                     bool strong) const
   {
   if(n < 35 || n.is_even() || e < 2 || d < 2 || p < 3 || q < 3 || p*q != n)
      return false;

   if(!strong)
      return true;

   if(d1 != d % (p - 1) || d2 != d % (q - 1) || c != inverse_mod(q, p))
      return false;
   if(!check_prime(p, rng) || !check_prime(q, rng))
      return false;
   return true;
   }

}
