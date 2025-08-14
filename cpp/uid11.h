#pragma once

#include <random>
#include <chrono>
#include <print>
#include <optional>
#include <array>
#include <format>

namespace uid11 {


static constexpr std::string_view min_u64_b58 = "11111111111";
static constexpr std::string_view max_u64_b58 = "jpXCZedGfVQ";


////////////////////////////////////////////////////////////////////////////////
//  base58 (bitcoin alphabet)
////////////////////////////////////////////////////////////////////////////////

static constexpr std::string_view alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
static constexpr uint8_t base = alphabet.size();
static constexpr uint8_t length = 11;

static constexpr std::array<uint8_t, 128> make_index()
{
    std::array<uint8_t, 128> m {};
    m.fill( 0xff );
            
    for ( int i = 0; i < alphabet.size(); ++i ) 
    {
        uint8_t c = static_cast<uint8_t>( alphabet[ i ] );

        if ( c >= m.size() ) {
            continue;
        }

        m[ c ] = static_cast<uint8_t>( i );
    }

    return m;
}

static constexpr auto index = make_index();


////////////////////////////////////////////////////////////////////////////////
//  xoshiro256++
//    written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)
//    [1] https://prng.di.unimi.it/
//    [2] https://prng.di.unimi.it/xoshiro256plusplus.c
////////////////////////////////////////////////////////////////////////////////

struct xoshiro256pp 
{    
    xoshiro256pp() noexcept 
    {
        uint64_t value {};
    
        try {
            static thread_local std::random_device rd;
            value = rd();
        } catch (...) {
            value = static_cast<uint64_t>( 
                std::chrono::steady_clock::now()
                    .time_since_epoch()
                    .count()
            );
        }
        
        seed( value );
    }

    constexpr uint64_t operator()() noexcept { 
        return next(); 
    }

    constexpr void seed( const uint64_t k ) noexcept
    {
        uint64_t sm = k;

        for ( int i = 0; i < 4; ++i ) {
            s[ i ] = splitmix64( sm );
        }
        
        for ( int i = 0; i < 8; ++i ) {
            next();
        }
    }

    constexpr uint64_t next() noexcept
    {
        const uint64_t result = rotl( s[0] + s[3], 23 ) + s[0];
        const uint64_t t = s[1] << 17;

        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];
        s[2] ^= t;
        s[3] = rotl( s[3], 45 );

        return result;
    }

    private:

        uint64_t s[4];

        static constexpr uint64_t splitmix64( uint64_t& x ) noexcept {
            uint64_t z = ( x += 0x9e3779b97f4a7c15ULL );
            z = ( z ^ ( z >> 30 ) ) * 0xbf58476d1ce4e5b9ULL;
            z = ( z ^ ( z >> 27 ) ) * 0x94d049bb133111ebULL;
            return z ^ ( z >> 31 );
        }

        static constexpr uint64_t rotl( const uint64_t x, int k ) noexcept {
            return ( x << k ) | ( x >> ( 64 - k ) );
        }
};


////////////////////////////////////////////////////////////////////////////////
//  helper
////////////////////////////////////////////////////////////////////////////////

constexpr std::uint64_t mask_n( std::size_t bits ) noexcept {
    return bits >= 64 ? ~0ull : ( bits == 0 ? 0ull : ( ( 1ull << bits ) - 1ull ) );
}


uint64_t time_since_unix_epoch_ms() noexcept {
    const auto now = std::chrono::system_clock::now();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>( now.time_since_epoch() ).count();
    return static_cast<uint64_t>( millis );
}


////////////////////////////////////////////////////////////////////////////////
//  encode / decode
////////////////////////////////////////////////////////////////////////////////

constexpr void encode_to( const uint64_t payload, char* buffer ) noexcept
{
    std::fill( buffer, buffer + 11, alphabet.front() );
    uint64_t v = payload;
    
    for( int i = length - 1; i >= 0; --i ) {
        buffer[ i ] = alphabet[ v % base ];
        v /= base;
    }
}


[[nodiscard]] constexpr std::string encode( const uint64_t payload ) {
    std::string s( length, 0 );
    encode_to( payload, s.data() );
    return s;
}


[[nodiscard]] constexpr bool is_valid_partial( std::string_view sv ) noexcept
{
    if ( sv.size() > 11 ) {
        return false;
    }

    for ( auto c : sv ) {
        if ( c >= 128 || index[ c ] == 0xff ) {
            return false;
        }
    }

    return true;
}


[[nodiscard]] constexpr bool is_valid( std::string_view sv ) noexcept
{
    if ( sv.size() != 11 ) {
        return false;
    }

    return is_valid_partial( sv );
}


[[nodiscard]] constexpr std::optional<uint64_t> unpack( std::string_view str ) 
{
    uint64_t acc {};

    for ( const char c : str ) 
    {
        const uint8_t pos = index.at( c );

        if ( acc > ( std::numeric_limits<uint64_t>::max() - pos ) / base ) {
            //  doesn't fit in 64 bits
            return std::nullopt; 
        }
        
        acc = acc * base + static_cast<uint64_t>( pos );
    }

    return acc;
}


[[nodiscard]] constexpr std::optional<uint64_t> decode( std::string_view str ) 
{       
    if ( ! is_valid( str ) ) {
        return std::nullopt;
    }

    return unpack( str );
}


[[nodiscard]] constexpr std::optional<uint64_t> decode_partial( std::string_view str ) 
{       
    if ( ! is_valid_partial( str ) ) {
        return std::nullopt;
    }

    return unpack( str ).transform( [ str ]( uint64_t acc ) {
        for ( int i = 0; i < length - str.size(); i++ ) {
            acc = acc * base;
        }
        return acc;
    });
}


////////////////////////////////////////////////////////////////////////////////
//  random
//    64 bit randomness
////////////////////////////////////////////////////////////////////////////////

static inline thread_local xoshiro256pp rand_u64;  


constexpr uint64_t random() noexcept {
    return rand_u64();
};

std::string random_string() noexcept {
    return encode( rand_u64() );
};


////////////////////////////////////////////////////////////////////////////////
//  time & random
//    42 bit milliseconds since xid epoch | 22 bit randomness
//    rolls over 2109-05-15T07:35:11.103Z from unix epoch
//    rolls over 2151-05-18T09:31:07.215Z from xid epoch
//    xid epoch: 1321009871111 ms
////////////////////////////////////////////////////////////////////////////////

static constexpr uint8_t time_bits = 44;
static constexpr uint8_t random_bits = 64 - time_bits;
static constexpr auto epoch_ms = 1321009871111;
static constexpr auto epoch = std::chrono::system_clock::time_point( std::chrono::milliseconds( epoch_ms ) );

constexpr auto timepoint( const uint64_t payload ) noexcept {
    const auto tp = epoch + std::chrono::milliseconds( payload >> random_bits );
    return std::chrono::floor<std::chrono::milliseconds>( tp );
}


auto timestamp( const uint64_t payload ) {
    const auto tp = timepoint( payload );
    return std::format( "{:%FT%T}Z", tp );
}


uint64_t xid() noexcept {
    const uint64_t timeBits = ( time_since_unix_epoch_ms() - epoch_ms ) << ( random_bits );
    const uint64_t randomBits = rand_u64() & mask_n( random_bits );
    return timeBits | randomBits;
};


std::string xid_string() noexcept {
    return encode( xid() );
};


constexpr uint64_t pack( const uint64_t timeSinceUnixEpoch_ms, const uint64_t random ) noexcept {
    const uint64_t timeBits = ( timeSinceUnixEpoch_ms - epoch_ms ) << ( random_bits );
    const uint64_t randomBits = random & mask_n( random_bits );
    return timeBits | randomBits;
};


}   //  ::uid11