#include <uuid11.h>

#include <print>
#include <thread>
#include <array>


int main() 
{
    //  random
    for ( int i = 0; i < 11; i++ ) {
        Uuid11 now;
        std::println( "{}", now.to_string() );
    }

    //  time & random in 10ms intervals
    for ( int i = 0; i < 11; i++ ) {
        Uuid11TR nowseq;
        Uuid11TR nsrt { Uuid11::from_string( nowseq.to_string() ).value() };
        std::println( "{}, {:064b}, {}, {}", nowseq.to_string(), nowseq.bytes, nowseq.timestamp(), nsrt.timestamp() );
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }

    //  time & random 
    std::array<Uuid11TR, 11> nows {};

    for ( const auto& now: nows ) {
        std::println( "{} -> {}", now.to_string(), now.timestamp() );
    }

    return EXIT_SUCCESS;
}