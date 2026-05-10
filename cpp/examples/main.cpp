#include <uid11/uid11.h>

#include <print>
#include <thread>
#include <chrono>


int main()
{
    using namespace std::chrono_literals;

    //--------------------------------------------------------------------------
    //  codec: random base58 strings
    //--------------------------------------------------------------------------
    std::println( "─── random ───" );
    for ( int i = 0; i < 5; i++ ) {
        std::println( "  {}", uid11::random_string() );
    }

    //--------------------------------------------------------------------------
    //  xid: time-ordered IDs at ~10ms intervals
    //  Note how only the trailing chars change between samples — the
    //  high-order chars encode the slowly-changing timestamp.
    //--------------------------------------------------------------------------
    std::println( "\n─── xid over time ───" );
    for ( int i = 0; i < 5; i++ ) {
        const auto id = uid11::xid::generate();
        std::println( "  {} -> {}", uid11::encode( id ), uid11::xid::timestamp( id ) );
        std::this_thread::sleep_for( 10ms );
    }

    //--------------------------------------------------------------------------
    //  xid: bit layout visualised
    //  Binary view:  [42 bit ms since xid epoch | 22 bit random]
    //--------------------------------------------------------------------------
    std::println( "\n─── xid bit layout ───" );
    {
        const auto id = uid11::xid::generate();
        const auto u  = uid11::xid::unpack( id );
        std::println( "  uid     = {}", uid11::encode( id ) );
        std::println( "  u64     = {}", id );
        std::println( "  binary  = {:064b}", id );
        std::println( "                                            ^─── 22 random bits ───^" );
        std::println( "  delta   = {} ms since xid epoch", id >> uid11::xid::random_bits );
        std::println( "  unix_ms = {}", u.unix_ms );
        std::println( "  random  = {} (0x{:06x})", u.random, u.random );
    }

    //--------------------------------------------------------------------------
    //  xid: pack / unpack round-trip
    //--------------------------------------------------------------------------
    std::println( "\n─── pack / unpack round-trip ───" );
    {
        const auto id       = uid11::xid::generate();
        const auto u        = uid11::xid::unpack( id );
        const auto repacked = uid11::xid::pack( u.unix_ms, u.random );
        std::println( "  id        = {}",     uid11::encode( id ) );
        std::println( "  unpacked  = ({} ms, random {})", u.unix_ms, u.random );
        std::println( "  pack(...) = {}  -> match: {}",
            uid11::encode( repacked ), repacked == id );
    }

    //--------------------------------------------------------------------------
    //  codec: prefix decoding — closed [lower, upper] range that an N-char
    //  prefix of an 11-char base58 string represents. Range shrinks 58x
    //  with every additional character.
    //--------------------------------------------------------------------------
    std::println( "\n─── decode_partial: prefix ranges ───" );
    const auto example = uid11::xid::generate_string();
    std::println( "  full uid: {}", example );
    std::println( "" );
    std::println( "   N  prefix          range                                 timestamp range" );

    for ( size_t n = 0; n <= 11; ++n ) {
        const auto r = uid11::decode_partial( std::string_view{ example }.substr( 0, n ) );
        if ( ! r ) {
            std::println( "  {:2}  {:<11}     <prefix outside u64>", n, example.substr( 0, n ) );
            continue;
        }
        std::println( "  {:2}  {:<11}     [{}, {}]    {} .. {}",
            n,
            example.substr( 0, n ),
            uid11::encode( r->lower ), uid11::encode( r->upper ),
            uid11::xid::timestamp( r->lower ),
            uid11::xid::timestamp( r->upper ) );
    }

    return EXIT_SUCCESS;
}
