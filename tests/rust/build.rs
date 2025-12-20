use std::env;

fn main() {
    println!("cargo:rerun-if-changed=src/ffi_helper.c");
    println!("cargo:rerun-if-env-changed=OLLVM_EXTRA_FLAGS");
    println!("cargo:rerun-if-env-changed=OLLVM_MARK");

    let mark = env::var("OLLVM_MARK").unwrap_or_else(|_| "plain".to_string());
    let mut build = cc::Build::new();
    build.file("src/ffi_helper.c");
    build.flag("-O2");
    build.flag("-fvisibility=hidden");
    build.flag("-fno-plt");
    build.define("OLLVM_MARK_STR", Some(mark.as_str()));

    if let Ok(extra) = env::var("OLLVM_EXTRA_FLAGS") {
        for flag in extra.split_whitespace().filter(|s| !s.is_empty()) {
            build.flag(flag);
        }
    }

    build.compile("ffi_helper");
}
