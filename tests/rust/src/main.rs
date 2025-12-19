#![allow(clippy::many_single_char_names)]

use std::fmt::Write;

#[inline(never)]
fn avalanche(mut v: u64) -> u64 {
    v ^= v >> 33;
    v = v.wrapping_mul(0xff51afd7ed558ccd);
    v ^= v >> 33;
    v = v.wrapping_mul(0xc4ceb9fe1a85ec53);
    v ^ (v >> 33)
}

fn mix_block(block: &mut [u64; 4], tweak: u64) {
    for round in 0..8 {
        let idx = (round + tweak as usize) & 3;
        let rot = (round * 7 + (tweak as usize)) & 63;
        block[idx] = block[idx]
            .wrapping_add(block[(idx + 1) & 3])
            .rotate_left(rot as u32)
            ^ tweak;
    }
}

fn derive_signature(payload: &[u8]) -> u64 {
    let mut lanes = [0xBADC0FFE0DDF1BADu64; 4];
    for (i, chunk) in payload.chunks(16).enumerate() {
        let mut buf = [0u8; 16];
        buf[..chunk.len()].copy_from_slice(chunk);
        lanes[i & 3] ^= u64::from_le_bytes(buf[0..8].try_into().unwrap());
        lanes[(i + 1) & 3] ^= u64::from_le_bytes(buf[8..16].try_into().unwrap());
        mix_block(&mut lanes, i as u64 ^ 0xA5A5_A5A5_A5A5_A5A5);
    }
    avalanche(lanes.iter().fold(0, |acc, &v| avalanche(acc ^ v)))
}

fn render_digest(state: &[u64; 4]) -> String {
    let mut out = String::with_capacity(64);
    for lane in state {
        write!(&mut out, "{lane:016x}").unwrap();
    }
    out
}

fn main() {
    let mut state = [
        0x0123_4567_89ab_cdef,
        0xfedc_ba98_7654_3210,
        0x0ddc_0ffe_ca_fe_babe,
        0xface_feed_dead_beef,
    ];
    mix_block(&mut state, 0x5aa55aa55aa55aa5);

    let phrase = b"rust-ollvm-smoke-test";
    let sig = derive_signature(phrase);
    let digest = render_digest(&state);
    println!("rust digest: {digest}");
    println!("rust signature: {sig:016x}");
}
