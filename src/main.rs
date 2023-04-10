use libc::{sigfillset, sigprocmask, SIG_BLOCK};
use std::io::Error;
use std::mem::MaybeUninit;
use std::process::ExitCode;
use std::ptr::null_mut;

fn main() -> ExitCode {
    // Print application version.
    println!(
        "Starting {} {}.",
        env!("CARGO_PKG_NAME"),
        env!("CARGO_PKG_VERSION")
    );

    // Block all signals during initialization so we don't corrupt the states.
    let mut sigs = MaybeUninit::uninit();

    if unsafe { sigfillset(sigs.as_mut_ptr()) } < 0 {
        let e = Error::last_os_error();
        eprintln!("Failed to initialize a signal list to block: {e}.");
        return ExitCode::FAILURE;
    }

    if unsafe { sigprocmask(SIG_BLOCK, sigs.as_ptr(), null_mut()) } < 0 {
        let e = Error::last_os_error();
        eprintln!("Failed to block the signals: {e}.");
        return ExitCode::FAILURE;
    }

    ExitCode::SUCCESS
}
