import { xid, xidString, random, randomString, timepoint, timestamp, encode } from "../uid11";


console.log( random() );
console.log( randomString() );
console.log( xid() );
console.log( encode( xid() ) );
console.log( xidString() );
console.log( timepoint( xid() ) );
console.log( timestamp( xid() ) );