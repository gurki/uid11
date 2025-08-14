#include <uid11.h>

#include <print>
#include <thread>
#include <array>


int main() 
{
    //  random
    for ( int i = 0; i < 11; i++ ) {
        uint64_t now = uid11::random();
        std::println( "{}", uid11::encode( now ) );
    }

    //  time & random in 10ms intervals
    for ( int i = 0; i < 11; i++ ) {
        uint64_t nowseq = uid11::xid();
        uint64_t nsrt { uid11::decode( uid11::encode( nowseq ) ).value() };
        std::println( "{}, {:064b}, {}, {}", uid11::encode( nowseq ), nowseq, uid11::timestamp( nowseq ), uid11::timestamp( nsrt ) );
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }

    //  time & random
    std::array<uint64_t, 11> nows;
    std::ranges::generate( nows, &uid11::xid );

    for ( const auto& now : nows ) {
        std::println( "{} -> {}", uid11::encode( now ), uid11::timestamp( now ) );
    }

    //  max possible
    uint64_t max = UINT64_MAX;
    std::println( "max: {} -> {}", uid11::encode( max ), uid11::timestamp( max ) );

    //  streaming uid
    const std::string_view enc = "24HZMr9t1qX";
    
    for ( int i = 0; i <= 11; i++ ) {
        uint64_t uid { uid11::decode_partial( enc.substr( 0, i ) ).value() };
        std::println( "{}: {} -> {}", i, uid11::encode( uid ), uid11::timestamp( uid ) );
    }
    
    return EXIT_SUCCESS;
}