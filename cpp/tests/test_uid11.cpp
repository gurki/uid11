// uid11 C++ test suite.
//
// Reads the shared cross-language vectors in test-vectors.json (path provided
// via UID11_TEST_VECTORS_PATH) plus exercises round-trip properties, invalid
// input handling, the xid bit layout (42|22), and the central spec promise:
// lexicographic order of the 11-char text matches numeric order of the u64.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include "uid11.h"

using json = nlohmann::json;

namespace {

json load_vectors()
{
    std::ifstream f( UID11_TEST_VECTORS_PATH );
    REQUIRE( f.is_open() );
    json j;
    f >> j;
    return j;
}

std::uint64_t u64_from_decimal( const std::string& s )
{
    return std::stoull( s );
}

}   //  namespace


////////////////////////////////////////////////////////////////////////////////
//  static / compile-time guarantees
////////////////////////////////////////////////////////////////////////////////

TEST_CASE( "alphabet is bitcoin base58", "[static]" )
{
    REQUIRE( std::string{ uid11::alphabet } ==
        "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz" );
    REQUIRE( uid11::base == 58 );
    REQUIRE( uid11::length == 11 );
}

TEST_CASE( "MIN / MAX string constants are correct", "[static]" )
{
    REQUIRE( std::string{ uid11::min_u64_b58 } == uid11::encode( 0 ) );
    REQUIRE( std::string{ uid11::max_u64_b58 } ==
        uid11::encode( std::numeric_limits<std::uint64_t>::max() ) );
}

TEST_CASE( "xid layout is 42 time bits + 22 random bits", "[static][xid]" )
{
    STATIC_REQUIRE( uid11::time_bits == 42 );
    STATIC_REQUIRE( uid11::random_bits == 22 );
    REQUIRE( uid11::epoch_ms == 1321009871111 );
}


////////////////////////////////////////////////////////////////////////////////
//  shared cross-language vectors
////////////////////////////////////////////////////////////////////////////////

TEST_CASE( "shared encode vectors", "[vectors][encode]" )
{
    const auto j = load_vectors();

    for ( const auto& v : j[ "encode" ] ) {
        const auto value = u64_from_decimal( v[ "u64" ].get<std::string>() );
        const auto expected = v[ "b58" ].get<std::string>();
        const std::string note = v.value( "note", std::string{} );

        CAPTURE( value, expected, note );
        REQUIRE( uid11::encode( value ) == expected );

        const auto decoded = uid11::decode( expected );
        REQUIRE( decoded.has_value() );
        REQUIRE( *decoded == value );
    }
}

TEST_CASE( "shared xid_pack vectors", "[vectors][xid]" )
{
    const auto j = load_vectors();

    for ( const auto& v : j[ "xid_pack" ] ) {
        const auto delta_ms = u64_from_decimal( v[ "delta_ms" ].get<std::string>() );
        const auto rand22   = u64_from_decimal( v[ "rand22" ].get<std::string>() );
        const auto expected = u64_from_decimal( v[ "u64" ].get<std::string>() );
        const auto text     = v[ "b58" ].get<std::string>();

        const std::uint64_t now_ms = uid11::epoch_ms + delta_ms;
        const auto packed = uid11::pack( now_ms, rand22 );

        CAPTURE( delta_ms, rand22, expected, text );
        REQUIRE( packed == expected );
        REQUIRE( uid11::encode( packed ) == text );
    }
}

TEST_CASE( "shared invalid vectors are rejected", "[vectors][invalid]" )
{
    const auto j = load_vectors();

    for ( const auto& v : j[ "invalid" ] ) {
        const auto s = v[ "s" ].get<std::string>();
        const auto kind = v.value( "kind", std::string{ "syntax" } );
        const auto reason = v.value( "reason", std::string{} );

        CAPTURE( s, kind, reason );

        //  Every invalid vector MUST be rejected by decode().
        REQUIRE_FALSE( uid11::decode( s ).has_value() );

        //  Syntax errors are caught at is_valid() too. Overflow vectors
        //  are syntactically valid -- the decoder catches them later.
        if ( kind == "syntax" ) {
            REQUIRE_FALSE( uid11::is_valid( s ) );
        } else if ( kind == "overflow" ) {
            REQUIRE( uid11::is_valid( s ) );
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
//  round-trip property tests
////////////////////////////////////////////////////////////////////////////////

TEST_CASE( "encode/decode round-trips for fixed extreme values", "[roundtrip]" )
{
    const std::vector<std::uint64_t> values = {
        0,
        1,
        57,
        58,
        ( 1ull << 22 ),
        ( 1ull << 32 ),
        ( 1ull << 42 ),
        ( 1ull << 63 ),
        std::numeric_limits<std::uint64_t>::max() - 1,
        std::numeric_limits<std::uint64_t>::max(),
    };

    for ( const auto v : values ) {
        CAPTURE( v );
        const auto s = uid11::encode( v );
        REQUIRE( s.size() == uid11::length );
        REQUIRE( uid11::is_valid( s ) );
        const auto back = uid11::decode( s );
        REQUIRE( back.has_value() );
        REQUIRE( *back == v );
    }
}

TEST_CASE( "encode/decode round-trips for random values", "[roundtrip][property]" )
{
    std::mt19937_64 rng( 0xC0FFEE );

    for ( int i = 0; i < 10'000; ++i ) {
        const std::uint64_t v = rng();
        const auto s = uid11::encode( v );
        REQUIRE( s.size() == uid11::length );
        const auto back = uid11::decode( s );
        REQUIRE( back.has_value() );
        REQUIRE( *back == v );
    }
}


////////////////////////////////////////////////////////////////////////////////
//  validation behavior
////////////////////////////////////////////////////////////////////////////////

TEST_CASE( "is_valid requires exactly 11 chars from the alphabet", "[validate]" )
{
    REQUIRE( uid11::is_valid( "11111111111" ) );
    REQUIRE_FALSE( uid11::is_valid( "" ) );
    REQUIRE_FALSE( uid11::is_valid( "1111111111" ) );    // 10
    REQUIRE_FALSE( uid11::is_valid( "111111111111" ) );  // 12
    REQUIRE_FALSE( uid11::is_valid( "1111111111O" ) );   // 'O' not in alphabet
    REQUIRE_FALSE( uid11::is_valid( "1111111111I" ) );   // 'I' not in alphabet
    REQUIRE_FALSE( uid11::is_valid( "1111111111l" ) );   // 'l' not in alphabet
    REQUIRE_FALSE( uid11::is_valid( "11111111110" ) );   // '0' not in alphabet
}

TEST_CASE( "is_valid_partial accepts up to 11 alphabet chars", "[validate][partial]" )
{
    REQUIRE( uid11::is_valid_partial( "" ) );
    REQUIRE( uid11::is_valid_partial( "1" ) );
    REQUIRE( uid11::is_valid_partial( "abc" ) );
    REQUIRE( uid11::is_valid_partial( "11111111111" ) );
    REQUIRE_FALSE( uid11::is_valid_partial( "111111111111" ) );  // 12
    REQUIRE_FALSE( uid11::is_valid_partial( "abc0" ) );          // '0'
    REQUIRE_FALSE( uid11::is_valid_partial( "abcO" ) );          // 'O'
}

TEST_CASE( "decoder rejects overflow into 65th bit", "[overflow]" )
{
    //  MAX_U64_B58 == "jpXCZedGfVQ". Bumping 'Q' (alphabet index 23) to 'R'
    //  (alphabet index 24) decodes to 2^64, which doesn't fit in u64.
    REQUIRE_FALSE( uid11::decode( "jpXCZedGfVR" ).has_value() );
}


////////////////////////////////////////////////////////////////////////////////
//  central spec promise: lexicographic order == numeric order
////////////////////////////////////////////////////////////////////////////////

TEST_CASE( "lexicographic order matches numeric order", "[ordering][property]" )
{
    std::mt19937_64 rng( 0xBADF00D );
    std::vector<std::uint64_t> nums;
    nums.reserve( 1024 );
    for ( int i = 0; i < 1024; ++i ) nums.push_back( rng() );

    std::vector<std::pair<std::uint64_t, std::string>> pairs;
    pairs.reserve( nums.size() );
    for ( auto n : nums ) pairs.emplace_back( n, uid11::encode( n ) );

    auto by_num = pairs;
    auto by_str = pairs;
    std::ranges::sort( by_num, {}, &decltype( pairs )::value_type::first );
    std::ranges::sort( by_str, {}, &decltype( pairs )::value_type::second );

    REQUIRE( by_num == by_str );
}


////////////////////////////////////////////////////////////////////////////////
//  xid pack / unpack
////////////////////////////////////////////////////////////////////////////////

TEST_CASE( "pack then unpack preserves timestamp and random", "[xid][roundtrip]" )
{
    const std::vector<std::pair<std::uint64_t, std::uint64_t>> cases = {
        { uid11::epoch_ms,             0 },
        { uid11::epoch_ms + 1,         0 },
        { uid11::epoch_ms + 86400000,  0 },
        { uid11::epoch_ms + 1234567,   ( 1u << 22 ) - 1 },
        { uid11::epoch_ms + 999'000,   0xABCDE },
    };

    constexpr std::uint64_t rand_mask = ( 1ull << uid11::random_bits ) - 1ull;

    for ( const auto& [ now_ms, rnd ] : cases ) {
        const auto payload = uid11::pack( now_ms, rnd );

        const std::uint64_t recovered_delta_ms = payload >> uid11::random_bits;
        const std::uint64_t recovered_rand     = payload & rand_mask;

        CAPTURE( now_ms, rnd, payload );
        REQUIRE( recovered_delta_ms == now_ms - uid11::epoch_ms );
        REQUIRE( recovered_rand     == ( rnd & rand_mask ) );
    }
}

TEST_CASE( "xid() places time in high 42 bits and random in low 22 bits", "[xid]" )
{
    constexpr std::uint64_t rand_mask = ( 1ull << uid11::random_bits ) - 1ull;
    constexpr std::uint64_t time_mask = ( 1ull << uid11::time_bits   ) - 1ull;

    for ( int i = 0; i < 64; ++i ) {
        const auto id = uid11::xid();
        const std::uint64_t delta_ms = id >> uid11::random_bits;
        const std::uint64_t rnd      = id & rand_mask;
        CAPTURE( id, delta_ms, rnd );
        //  must fit in the declared field widths
        REQUIRE( delta_ms <= time_mask );
        REQUIRE( rnd      <= rand_mask );
    }
}

TEST_CASE( "xid() reflects wall-clock time within a small window", "[xid][clock]" )
{
    const auto before = uid11::time_since_unix_epoch_ms();
    const auto id     = uid11::xid();
    const auto after  = uid11::time_since_unix_epoch_ms();

    const std::uint64_t delta_ms = id >> uid11::random_bits;
    const std::uint64_t id_ms    = uid11::epoch_ms + delta_ms;

    REQUIRE( id_ms >= before );
    REQUIRE( id_ms <= after );
}
