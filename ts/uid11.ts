// uid11.ts — Base58 (Bitcoin alphabet), 64-bit payloads, xid helpers.

const ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
const BASE = 58n;
const LENGTH = 11;

export const MIN_U64_B58 = "11111111111";
export const MAX_U64_B58 = "jpXCZedGfVQ";

const MASK64 = (1n << 64n) - 1n;
const IDX = (() => {
  const a = new Int16Array(256).fill(-1);
  for (let i = 0; i < ALPHABET.length; i++) a[ALPHABET.charCodeAt(i)] = i;
  return a;
})();

const toU64 = (x: bigint | number) => {
  const v = typeof x === "bigint" ? x : BigInt(x);
  if (v < 0n || v > MASK64) throw new RangeError("value must be in [0, 2^64-1]");
  return v;
};

/* ---------- crypto-secure randomness (no fallback) ---------- */

function randomBits(n: number): bigint {
  const bytes = Math.ceil(n / 8);
  const u8 = crypto.getRandomValues(new Uint8Array(bytes));
  let v = 0n;
  for (let i = 0; i < u8.length; i++) v = (v << 8n) | BigInt(u8[i]);
  return n % 8 ? (v & ((1n << BigInt(n)) - 1n)) : v;
}

/* ---------- encode / decode ---------- */

export function isValidPartial(s: string): boolean {
  if (s.length > LENGTH) return false;
  for (let i = 0; i < s.length; i++) if (IDX[s.charCodeAt(i)] === -1) return false;
  return true;
}

export const isValid = (s: string) => s.length === LENGTH && isValidPartial(s);

export function encode(payload: bigint | number): string {
  let v = toU64(payload);
  const out = Array.from({ length: LENGTH }, () => ALPHABET[0]);
  for (let i = LENGTH - 1; i >= 0; i--) {
    out[i] = ALPHABET[Number(v % BASE)];
    v /= BASE;
  }
  return out.join("");
}

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

export const decode = (s: string) => (isValid(s) ? unpack(s) : null);
export function decodePartial(s: string): bigint | null {
  if (!isValidPartial(s)) return null;
  const acc = unpack(s);
  if (acc == null) return null;
  return acc * (BASE ** BigInt(LENGTH - s.length));
}

/* ---------- crypto-random helpers ---------- */

export const random = (): bigint => randomBits(64);
export const randomString = (): string => encode(random());

/* ---------- XID-style time+random (44|20) ---------- */

const TIME_BITS = 44;
const RANDOM_BITS = 64 - TIME_BITS;
export const EPOCH_MS = 1321009871111n; // 2011-11-11T11:11:11.111Z

const maskN = (bits: number) => (bits >= 64 ? MASK64 : (1n << BigInt(bits)) - 1n);

export function pack(timeSinceUnixEpochMs: bigint | number, random: bigint | number): bigint {
  const t = (toU64(timeSinceUnixEpochMs) - EPOCH_MS) << BigInt(RANDOM_BITS);
  const r = toU64(random) & maskN(RANDOM_BITS);
  return (t | r) & MASK64;
}

export function xid(): bigint {
  const now = BigInt(Date.now());
  const t = (now - EPOCH_MS) << BigInt(RANDOM_BITS);
  const r = randomBits(RANDOM_BITS) & maskN(RANDOM_BITS);
  return (t | r) & MASK64;
}

export const xidString = (): string => encode(xid());

export function timepoint(payload: bigint | number): Date {
  const v = toU64(payload);
  return new Date(Number((v >> BigInt(RANDOM_BITS)) + EPOCH_MS));
}

export function timestamp(payload: bigint | number): string {
  return timepoint(payload).toISOString();
}

export default {
  ALPHABET, BASE, LENGTH,
  MIN_U64_B58, MAX_U64_B58,
  isValid, isValidPartial,
  encode, decode, decodePartial,
  random, randomString,
  xid, xidString, pack, timepoint, timestamp, epochMs: EPOCH_MS,
};
