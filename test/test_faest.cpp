#include <array>

#include "test.hpp"

extern "C" {

#include "config.h"
#include "faest_details.h"

}

#include "catch_amalgamated.hpp"

TEST_CASE( "keygen/sign/verify", "[faest]" ) {
    std::array<uint8_t, GREATWALL_SECRET_KEY_BYTES> packed_sk;
    std::array<uint8_t, GREATWALL_PUBLIC_KEY_BYTES> packed_pk;
    std::array<uint8_t, FAEST_SIGNATURE_BYTES> signature;
    test_gen_keypair(packed_pk.data(), packed_sk.data());
    const std::string message = "This document describes and specifies the FAEST digital signature algorithm.";

    REQUIRE( faest_sign(signature.data(), reinterpret_cast<const uint8_t*>(message.c_str()), message.size(), packed_sk.data(), NULL, 0) );
    REQUIRE( faest_verify(signature.data(), reinterpret_cast<const uint8_t*>(message.c_str()), message.size(), packed_pk.data()) );
}

#if USE_IMPROVED_VECTOR_COMMITMENTS == 0 && ZERO_BITS_IN_CHALLENGE_3 == 0 && \
    defined(TREE_PRG_AES_CTR) && defined(LEAF_PRG_SHAKE) && \
    ((defined(OWF_AES_CTR) && \
    (((SECURITY_PARAM == 128) && (BITS_PER_WITNESS == 11 || BITS_PER_WITNESS == 16)) || \
     ((SECURITY_PARAM == 192) && (BITS_PER_WITNESS == 16 || BITS_PER_WITNESS == 24)) || \
     ((SECURITY_PARAM == 256) && (BITS_PER_WITNESS == 22 || BITS_PER_WITNESS == 32)))) \
     || \
    (defined(OWF_RIJNDAEL_EVEN_MANSOUR) && \
    (((SECURITY_PARAM == 128) && (BITS_PER_WITNESS == 11 || BITS_PER_WITNESS == 16)) || \
     ((SECURITY_PARAM == 192) && (BITS_PER_WITNESS == 16 || BITS_PER_WITNESS == 24)) || \
     ((SECURITY_PARAM == 256) && (BITS_PER_WITNESS == 22 || BITS_PER_WITNESS == 32)))))

TEST_CASE( "test vector", "[faest tv]" ) {
#if defined(OWF_AES_CTR) && SECURITY_PARAM == 128 && BITS_PER_WITNESS == 11
    namespace tv = faest_tvs::faest_128s_tvs;
#elif defined(OWF_AES_CTR) && SECURITY_PARAM == 128 && BITS_PER_WITNESS == 16
    namespace tv = faest_tvs::faest_128f_tvs;
#elif defined(OWF_AES_CTR) && SECURITY_PARAM == 192 && BITS_PER_WITNESS == 16
    namespace tv = faest_tvs::faest_192s_tvs;
#elif defined(OWF_AES_CTR) && SECURITY_PARAM == 192 && BITS_PER_WITNESS == 24
    namespace tv = faest_tvs::faest_192f_tvs;
#elif defined(OWF_AES_CTR) && SECURITY_PARAM == 256 && BITS_PER_WITNESS == 22
    namespace tv = faest_tvs::faest_256s_tvs;
#elif defined(OWF_AES_CTR) && SECURITY_PARAM == 256 && BITS_PER_WITNESS == 32
    namespace tv = faest_tvs::faest_256f_tvs;
#elif defined(OWF_RIJNDAEL_EVEN_MANSOUR) && SECURITY_PARAM == 128 && BITS_PER_WITNESS == 11
    namespace tv = faest_tvs::faest_em_128s_tvs;
#elif defined(OWF_RIJNDAEL_EVEN_MANSOUR) && SECURITY_PARAM == 128 && BITS_PER_WITNESS == 16
    namespace tv = faest_tvs::faest_em_128f_tvs;
#elif defined(OWF_RIJNDAEL_EVEN_MANSOUR) && SECURITY_PARAM == 192 && BITS_PER_WITNESS == 16
    namespace tv = faest_tvs::faest_em_192s_tvs;
#elif defined(OWF_RIJNDAEL_EVEN_MANSOUR) && SECURITY_PARAM == 192 && BITS_PER_WITNESS == 24
    namespace tv = faest_tvs::faest_em_192f_tvs;
#elif defined(OWF_RIJNDAEL_EVEN_MANSOUR) && SECURITY_PARAM == 256 && BITS_PER_WITNESS == 22
    namespace tv = faest_tvs::faest_em_256s_tvs;
#elif defined(OWF_RIJNDAEL_EVEN_MANSOUR) && SECURITY_PARAM == 256 && BITS_PER_WITNESS == 32
    namespace tv = faest_tvs::faest_em_256f_tvs;
#endif
    using faest_tvs::message;

    REQUIRE( tv::packed_sk.size() == FAEST_SECRET_KEY_BYTES );
    REQUIRE( tv::packed_pk.size() == FAEST_PUBLIC_KEY_BYTES );
    REQUIRE( tv::signature.size() == FAEST_SIGNATURE_BYTES );

    std::array<uint8_t, FAEST_PUBLIC_KEY_BYTES> packed_pk;
    std::array<uint8_t, FAEST_SIGNATURE_BYTES> signature;

    faest_pubkey(packed_pk.data(), tv::packed_sk.data());
    CHECK( packed_pk == tv::packed_pk );

    REQUIRE( faest_sign(signature.data(), reinterpret_cast<const uint8_t*>(message.c_str()), message.size(), tv::packed_sk.data(), tv::randomness.data(), tv::randomness.size()) );
    CHECK( signature == tv::signature );

    REQUIRE( faest_verify(signature.data(), reinterpret_cast<const uint8_t*>(message.c_str()), message.size(), tv::packed_pk.data()) );
}

#endif
