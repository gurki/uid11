#include <xid.h>

#include <print>
#include <thread>
#include <array>


int main() 
{
    //  random
    for ( int i = 0; i < 11; i++ ) {
        xid::Uuid11 now;
        std::println( "{}", now.to_string() );
    }

    //  time & random in 10ms intervals
    for ( int i = 0; i < 11; i++ ) {
        xid::Uuid11TR nowseq;
        xid::Uuid11TR nsrt { xid::Uuid11::from_string( nowseq.to_string() ).value() };
        std::println( "{}, {:064b}, {}, {}", nowseq.to_string(), nowseq.bytes, nowseq.timestamp(), nsrt.timestamp() );
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }

    //  time & random 
    std::array<xid::Uuid11TR, 11> nows {};

    for ( const auto& now: nows ) {
        std::println( "{} -> {}", now.to_string(), now.timestamp() );
    }

    //  max possible
    xid::Uuid11TR max;
    max.bytes = UINT64_MAX;
    std::println( "max: {} -> {}", max.to_string(), max.timestamp() );

    //  streaming uid
    
    const std::string_view enc = "5Gg7kK8XiNh";
    for ( int i = 0; i <= 11; i++ ) {
        xid::Uuid11TR uid = xid::Uuid11TR::from_string( enc.substr( 0, i ) ).value();
        std::println( "{}: {} -> {}", i, uid.to_string(), uid.timestamp() );
    }
    
    return EXIT_SUCCESS;
}