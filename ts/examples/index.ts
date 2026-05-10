import { encode, decode, random, randomString, xid } from "../uid11";


console.log(random());
console.log(randomString());
console.log(xid.generate());
console.log(encode(xid.generate()));
console.log(xid.generateString());
console.log(xid.timepoint(xid.generate()));
console.log(xid.timestamp(xid.generate()));

//  round-trip the codec
const s = randomString();
console.log(s, "->", decode(s));
