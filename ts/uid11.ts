// uid11.ts — Base58 (Bitcoin alphabet), 64-bit payloads, xid profile.
//
// Public surface mirrors the C++ layered API:
//
//   top-level exports — format constants + pure codec + profile-agnostic
//                       random 64-bit generator
//   `xid` namespace  — the xid profile (42 bit ms timestamp | 22 bit random),
//                       both pure packers and stateful generation

export const VERSION = "0.2.0";

/* ---------- format constants ---------- */

export const ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
export const BASE = 58n;
export const LENGTH = 11;
export const MIN_U64_B58 = "11111111111";
export const MAX_U64_B58 = "jpXCZedGfVQ";

const MASK64 = (1n << 64n) - 1n;

const IDX = (() => {
  const a = new Int16Array(256).fill(-1);
  for (let i = 0; i < ALPHABET.length; i++) a[ALPHABET.charCodeAt(i)] = i;
  return a;
})();

const toU64 = (x: bigint | number): bigint => {
  const v = typeof x === "bigint" ? x : BigInt(x);
  if (v < 0n || v > MASK64) throw new RangeError("value must be in [0, 2^64-1]");
  return v;
};

const maskN = (bits: number) => (bits >= 64 ? MASK64 : (1n << BigInt(bits)) - 1n);

/* ---------- crypto-secure randomness (no fallback) ---------- */

function randomBits(n: number): bigint {
  const bytes = Math.ceil(n / 8);
  const u8 = crypto.getRandomValues(new Uint8Array(bytes));
  let v = 0n;
  for (let i = 0; i < u8.length; i++) v = (v << 8n) | BigInt(u8[i]);
  return n % 8 ? (v & ((1n << BigInt(n)) - 1n)) : v;
}

/* ---------- pure codec (no clock, no rand) ---------- */

export function isValidPartial(s: string): boolean {
  if (s.length > LENGTH) return false;
  for (let i = 0; i < s.length; i++) if (IDX[s.charCodeAt(i)] === -1) return false;
  return true;
}

export const isValid = (s: string): boolean => s.length === LENGTH && isValidPartial(s);

export function encode(payload: bigint | number): string {
  let v = toU64(payload);
  const out = Array.from({ length: LENGTH }, () => ALPHABET[0]);
  for (let i = LENGTH - 1; i >= 0; i--) {
    out[i] = ALPHABET[Number(v % BASE)];
    v /= BASE;
  }
  return out.join("");
}

//  caller must have run isValidPartial first; only returns null on u64 overflow
function unpack(s: string): bigint | null {
  let acc = 0n;
  for (let i = 0; i < s.length; i++) {
    const pos = BigInt(IDX[s.charCodeAt(i)]);
    if (pos < 0n) return null;
    if (acc > (MASK64 - pos) / BASE) return null; // overflow guard
    acc = acc * BASE + pos;
  }
  return acc;
}

export const decode = (s: string): bigint | null => (isValid(s) ? unpack(s) : null);

/**
 * Closed numeric range [lower, upper] of u64 values matching a base58 prefix.
 * For a full 11-char string the range degenerates to a single value.
 */
export interface PrefixRange {
  lower: bigint;
  upper: bigint;
}

/**
 * Closed range of u64 values matching a 0..11 char base58 prefix. Returns
 * `null` if the prefix is invalid or maps to a range entirely outside
 * `[0, 2^64)`. See SPECIFICATION.md §3.6 / §4.
 *
 * Edge cases:
 *   ""              -> { lower: 0n, upper: 2^64 - 1n }   (the whole u64 space)
 *   11-char string  -> { lower: v,  upper: v }           (same as decode())
 *   lower > u64_max -> null
 *   upper > u64_max -> upper clamped to u64_max
 */
export function decodePartial(s: string): PrefixRange | null {
  if (!isValidPartial(s)) return null;

  if (s.length === 0) return { lower: 0n, upper: MASK64 };

  const acc = unpack(s);
  if (acc == null) return null;

  const scale = BASE ** BigInt(LENGTH - s.length);
  const lower = acc * scale;
  if (lower > MASK64) return null;
  const upper = lower + scale - 1n;
  return { lower, upper: upper > MASK64 ? MASK64 : upper };
}

/* ---------- profile-agnostic random 64-bit ---------- */

export const random = (): bigint => randomBits(64);
export const randomString = (): string => encode(random());

/* ---------- xid profile: [ 42 bit ms timestamp | 22 bit random ] ---------- */

export namespace xid {
  export const TIME_BITS = 42;
  export const RANDOM_BITS = 64 - TIME_BITS;
  export const EPOCH_MS = 1321009871111n; // 2011-11-11T11:11:11.111Z

  /**
   * Decomposed xid: the inverse of {@link pack}. `unixMs` is symmetric with
   * `pack`'s first parameter (ms since the Unix epoch).
   */
  export interface Unpacked {
    unixMs: bigint;
    random: bigint;
  }

  /** pure: pack a wall-clock ms time and a random integer into a xid payload */
  export function pack(timeSinceUnixEpochMs: bigint | number, rnd: bigint | number): bigint {
    const t = (toU64(timeSinceUnixEpochMs) - EPOCH_MS) << BigInt(RANDOM_BITS);
    const r = toU64(rnd) & maskN(RANDOM_BITS);
    return (t | r) & MASK64;
  }

  /** pure: 22-bit random tie-breaker extracted from a xid payload */
  export function randomField(payload: bigint | number): bigint {
    return toU64(payload) & maskN(RANDOM_BITS);
  }

  /**
   * pure: full inverse of {@link pack}. Returns `{ unixMs, random }` such that
   * `pack(u.unixMs, u.random) === payload`.
   */
  export function unpack(payload: bigint | number): Unpacked {
    const v = toU64(payload);
    return {
      unixMs: (v >> BigInt(RANDOM_BITS)) + EPOCH_MS,
      random: v & maskN(RANDOM_BITS),
    };
  }

  /** pure: timestamp from a xid payload as a JS Date */
  export function timepoint(payload: bigint | number): Date {
    const v = toU64(payload);
    return new Date(Number((v >> BigInt(RANDOM_BITS)) + EPOCH_MS));
  }

  /** pure: ISO-8601 timestamp from a xid payload */
  export function timestamp(payload: bigint | number): string {
    return timepoint(payload).toISOString();
  }

  /** stateful: fresh xid using wall clock + crypto randomness */
  export function generate(): bigint {
    return pack(BigInt(Date.now()), randomBits(RANDOM_BITS));
  }

  /** stateful: base58-encoded fresh xid */
  export const generateString = (): string => encode(generate());
}

export default {
  VERSION,
  ALPHABET, BASE, LENGTH,
  MIN_U64_B58, MAX_U64_B58,
  isValid, isValidPartial,
  encode, decode, decodePartial,
  random, randomString,
  xid,
};
