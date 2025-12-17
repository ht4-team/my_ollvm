fn main() {
    let mut value: u64 = 0xDEADBEEFCAFEBABE;
    for _ in 0..100 {
        value = value.rotate_left(5) ^ 0xBAD5EED;
        value = value.wrapping_add(0x1234_5678_9ABC_DEF0);
    }
    println!("rust smoke: {value:016x}");
}
