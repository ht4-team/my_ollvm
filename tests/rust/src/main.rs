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
    reversible_twirl(block, tweak);
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
        let vm = pseudo_vm_lane(lanes[(i + 2) & 3], i as u64);
        lanes[(i + 2) & 3] ^= vm;
        lanes[(i + 2) & 3] ^= vm;
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

#[inline(never)]
fn pseudo_vm_lane(seed: u64, round: u64) -> u64 {
    let mut acc = seed ^ (round.rotate_left((round as u32) & 31));
    let program = [
        0xA5A5_5AA5_1234_5678u64,
        0x0F0F_F0F0_DEAD_BEEFu64,
        0xFACE_CAFE_F00D_ABADu64,
        0x1357_9BDF_2468_ACEEu64,
    ];
    for (pc, instr) in program.iter().enumerate() {
        let opcode = (instr >> ((pc * 13) & 47)) & 0xF;
        match opcode & 0x3 {
            0 => acc = acc.rotate_left((opcode * 7) as u32) ^ instr,
            1 => acc = acc.wrapping_add(instr ^ acc.rotate_right(11)),
            2 => acc ^= instr.wrapping_mul(0x9E37_79B9),
            _ => acc = acc.wrapping_sub(instr ^ (acc << 3)),
        }
    }
    acc
}

#[inline(never)]
fn reversible_twirl(block: &mut [u64; 4], salt: u64) {
    let mut masks = [0u64; 4];
    let mut rots = [0u8; 4];
    for (idx, lane) in block.iter_mut().enumerate() {
        let mix = avalanche(*lane ^ salt ^ idx as u64);
        let rot = (mix as u8) & 63;
        masks[idx] = mix;
        rots[idx] = rot;
        *lane = lane.rotate_left(rot as u32) ^ mix;
    }
    for (idx, lane) in block.iter_mut().enumerate().rev() {
        *lane = (*lane ^ masks[idx]).rotate_right(rots[idx] as u32);
    }
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
