#pragma once

#include <random>
#include <chrono>
#include <print>
#include <optional>
#include <array>


namespace xid {


////////////////////////////////////////////////////////////////////////////////
//  base58 (bitcoin alphabet)
////////////////////////////////////////////////////////////////////////////////

static constexpr std::string_view alphabet = 
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

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
//  helper
////////////////////////////////////////////////////////////////////////////////

constexpr std::uint64_t mask_n( std::size_t bits ) {
    return bits >= 64 ? ~0ull : ( bits == 0 ? 0ull : ( ( 1ull << bits ) - 1ull ) );
}


////////////////////////////////////////////////////////////////////////////////
/*
    xoshiro256++
    Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)
    [1] https://prng.di.unimi.it/
    [2] https://prng.di.unimi.it/xoshiro256plusplus.c
*/
struct RandomU64 
{    
    RandomU64() {
        static thread_local std::random_device rd;
        seed( rd() );
    }

    constexpr uint64_t operator()() { 
        return next(); 
    }

    constexpr void seed( const uint64_t k ) 
    {
        uint64_t sm = k;

        for ( int i = 0; i < 4; ++i ) {
            s[ i ] = splitmix64( sm );
        }
        
        for ( int i = 0; i < 8; ++i ) {
            next();
        }
    }

    constexpr uint64_t next() 
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

        static constexpr uint64_t splitmix64( uint64_t& x ) {
            uint64_t z = ( x += 0x9e3779b97f4a7c15ULL );
            z = ( z ^ ( z >> 30 ) ) * 0xbf58476d1ce4e5b9ULL;
            z = ( z ^ ( z >> 27 ) ) * 0x94d049bb133111ebULL;
            return z ^ ( z >> 31 );
        }

        static constexpr uint64_t rotl( const uint64_t x, int k ) {
            return ( x << k ) | ( x >> ( 64 - k ) );
        }
};


////////////////////////////////////////////////////////////////////////////////
/*
    64 bit url-safe uuid
    requires at most `log( 2^64 ) / log( 58 ) = 10.925` symbols -> 11 characters uuid11
    base64 would only have `log( 2^64 ) / log( 64 ) = 10.666` symbols, i.e. still need 11 characters with only downsides
    maybe name it "Xi" with 'Îž' or 'Î¾' because it's roman numeral XI
*/
struct Uuid11 
{
    //  base58
    // static constexpr auto length = std::ceil( std::log( 2^ 64 ) / std::log( 58 ) );
    static constexpr std::string_view max = "jpXCZedGfVQ";

    static inline thread_local RandomU64 randu64;  

    uint64_t bytes;

    constexpr Uuid11() {
        bytes = randu64();
    }
    
    constexpr std::string to_string() const 
    {
        std::string s( length, alphabet[ 0 ] );
        uint64_t v = bytes;
        
        for( int i = length - 1; i >= 0; --i ) {
            s[ i ] = alphabet[ v % base ];
            v /= base;
        }

        return s;
    }

    static constexpr std::optional<Uuid11> from_string( std::string_view str ) 
    {       
        Uuid11 result;
        result.bytes = 0;
        
        for ( const char c : str ) 
        {
            const size_t pos = alphabet.find( c );

            if ( pos == std::string_view::npos ) {
                return std::nullopt;
            }
            
            result.bytes = result.bytes * base + pos;
        }

        for ( int i = 0; i < length - str.size(); i++ ) {
            result.bytes = result.bytes * base;
        }
        
        return result;
    }
};


////////////////////////////////////////////////////////////////////////////////
/*
    time & random
    44 bit milliseconds since epoch | 20 bit randomness
    45 bit rolls over 3084-12-12T12:41:28.831Z
    44 bit rolls over 2527-06-23T06:20:44.415Z
    43 bit rolls over 2248-09-26T15:10:22.207Z
    42 bit rolls over 2109-05-15T07:35:11.103Z
    41 bit rolls over 2039-09-07T15:47:35.551Z
*/
struct Uuid11TR : public Uuid11 
{
    inline static auto epoch = std::chrono::system_clock::time_point();

    Uuid11TR() 
    {
        const auto now = std::chrono::system_clock::now();
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>( now.time_since_epoch() ).count();
        const uint64_t timeBits = static_cast<uint64_t>( millis );
        const uint64_t randomBits = randu64() & mask_n( 20 );
        bytes = ( timeBits << 20 ) | randomBits;
    }

    Uuid11TR( const Uuid11& other ) {
        bytes = other.bytes;
    }
    
    auto timestamp() const {
        return epoch + std::chrono::milliseconds( bytes >> 20 );
    }
};


////////////////////////////////////////////////////////////////////////////////
/*
    time & sequence
    44 bit milliseconds since epoch | 24 bit sequence
    rolls over 2527-06-23T06:20:44.415Z
*/
struct Uuid11TS : public Uuid11 
{
    inline static auto epoch = std::chrono::system_clock::time_point();

    uint16_t seq = 0;
    uint64_t lastMs = 0;

    Uuid11TS() 
    {
        const auto now = std::chrono::system_clock::now();
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>( now.time_since_epoch() ).count();
        const uint64_t timeBits = static_cast<uint64_t>( millis );
        
        if ( timeBits != lastMs ) {
            seq = 0;
            lastMs = timeBits;
        }
        
        bytes = ( timeBits << 20 ) | seq;
        seq++;
    }

    Uuid11TS( const Uuid11& other ) {
        bytes = other.bytes;
    }
    
    auto timestamp() const {
        return epoch + std::chrono::milliseconds( bytes >> 20 );
    }
};


////////////////////////////////////////////////////////////////////////////////
/*
    twitter snowflake id
    0 sign bit | 41 bit milliseconds since 1288834974657 unix time ms | 10 bit machine | 12 bit sequence
    rolls over 2080-07-10T17:30:30.208Z
*/
struct Uuid11SF : public Uuid11 
{
    inline static auto epoch = std::chrono::system_clock::time_point( std::chrono::milliseconds( 1288834974657 ) );

    uint16_t mid = 0;
    uint16_t seq = 0;
    uint64_t lastMs = 0;

    Uuid11SF( const uint16_t mid ) 
    {
        const auto now = std::chrono::system_clock::now();
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>( now.time_since_epoch() ).count();
        const uint64_t timeBits = static_cast<uint64_t>( millis );
        
        if ( timeBits != lastMs ) {
            seq = 0;
            lastMs = timeBits;
        }
        
        bytes = ( ( timeBits << 23 ) >> 1 ) | ( mid << 12 ) | seq;
        seq++;
    }

    Uuid11SF( const Uuid11& other ) {
        bytes = other.bytes;
    }
    
    auto timestamp() const {
        return epoch + std::chrono::milliseconds( bytes >> 22 );
    }
};


}   //  ::xid