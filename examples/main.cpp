#include <xid.h>

#include <print>
#include <thread>
#include <array>


int main() 
{
    //  random
    for ( int i = 0; i < 11; i++ ) {
        xid::XidR now;
        std::println( "{}", now.to_string() );
    }

    //  time & random in 10ms intervals
    for ( int i = 0; i < 11; i++ ) {
        xid::XidTR nowseq;
        xid::XidTR nsrt { xid::Uid11::from_string( nowseq.to_string() ).value().bytes };
        std::println( "{}, {:064b}, {}, {}", nowseq.to_string(), nowseq.bytes, nowseq.timestamp(), nsrt.timestamp() );
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }

    //  time & random 
    std::array<xid::XidTR, 11> nows {};

    for ( const auto& now: nows ) {
        std::println( "{} -> {}", now.to_string(), now.timestamp() );
    }

    //  max possible
    xid::XidTR max;
    max.bytes = UINT64_MAX;
    std::println( "max: {} -> {}", max.to_string(), max.timestamp() );

    //  streaming uid
    
    const std::string_view enc = "5Gg7kK8XiNh";
    for ( int i = 0; i <= 11; i++ ) {
        xid::XidTR uid { xid::Uid11::from_string( enc.substr( 0, i ) ).value().bytes };
        std::println( "{}: {} -> {}", i, uid.to_string(), uid.timestamp() );
    }
    
    return EXIT_SUCCESS;
}