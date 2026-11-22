# Race Condition Vulnerability Lab (SEED) -- game design

Adapted from the SEED Labs "Race Condition Vulnerability Lab" (Wenliang Du). See `ATTRIBUTION.txt`.

## Disclaimer

This game is for educational purposes only. Never perform these techniques against a system you
do not own or do not have explicit written authorization to test.

## Scoring

20 points total. Hints cost 0.5-2 points each and are optional. Level 4's flag is randomly
generated per sandbox (see `variables.yml`); it is the only level gated behind real root, and real
root is only reachable by actually performing that exploit -- see the Level 2 and Level 3 design
notes for why every other path to root was deliberately closed off.

## Level 0: Introduction (info level)

Background on TOCTOU race conditions, the `access()`/`fopen()` gap in `vulp`, scoring rules, the
anti-cheat note about unique flags, the `race-lab-reset` safety net (a setuid-root helper, no
`sudo` needed), and the non-commercial educational disclaimer / SEED Labs credit.

## Level 1: Get Access

Connect to the `seed` host (GUI or SSH) with `seed` / `dees`. Passkey: `start`.

## Level 2: Inspect the vulnerable Set-UID program -- 2 points

**Learning outcome:** understand how a Set-UID-root binary's permission bits look, and what the
`s` bit actually means.

**Task:** read `/home/seed/lab/vulp.c`, then run `ls -l /home/seed/lab/vulp`. The binary is
**pre-built and pre-rooted by Ansible**, not by the student -- see the design note below.

**Flag:** the exact permission string `ls -l` shows (`-rwsr-xr-x`).

**Design note (why the student doesn't build it themselves):** the original SEED lab has the
student `sudo chown root:root` / `sudo chmod 4755` their own freshly-compiled binary. On a
single-user practice VM that's harmless, but on a graded, flag-based platform it is a critical
bypass: a student could put arbitrary code (e.g. a shell) in `vulp.c` instead of the real logic,
get it rooted the same way, and obtain an unconditional root shell with no race condition
required at all -- skipping Level 4 entirely. So this scenario pre-builds and pre-roots `vulp`
during provisioning. More fundamentally, `seed` has **no sudo rights at all** on this machine
(stripped explicitly at the start of provisioning), so there is no sudo command the student could
fall back on even if they wanted to.

## Level 3: Understand the magic password -- safely -- 3 points

**Learning outcome:** understand the `/etc/passwd` format (UID field controls privilege, not the
username) and the "magic hash" `U6aMy0wojraho` used for password-less login -- without ever
touching the real `/etc/passwd` or gaining a real shell.

**Task:** locally verify, with Python's `crypt` module, that `crypt.crypt('', 'U6')` equals
`U6aMy0wojraho` (proving the magic hash is an empty password), then compute
`crypt.crypt('seedlab', 'ab')` the same way.

**Flag:** the resulting hash (`ab2IRo8rbkhTM`) -- the same for every sandbox, since this is a
comprehension check, not a privilege-escalation proof.

**Design note:** the original lab has the student manually add the magic-password account to the
*real* `/etc/passwd` and `su` into it to see it work. That gives the student a genuine root shell
-- which, on this platform, would let them simply `cat` Level 4's flag file directly, skipping the
real exploit. So this level keeps the same lesson (the hash really is a blank password, and a
UID-0 entry really does grant root) but proves it with a local, unprivileged computation instead.

## Level 4: Launch the race-condition attack -- 10 points (variant flag)

**Learning outcome:** exploit a real TOCTOU race condition end-to-end, including recognizing and
fixing a race condition in your *own* attack program using `renameat2`/`RENAME_EXCHANGE`.

**Task:** write an attack program that repeatedly re-points `/tmp/XYZ` at `/etc/passwd` while
`vulp` is run (in a loop) with input that appends a new UID-0 account. If the naive
`unlink()`+`symlink()` approach stalls because `/tmp/XYZ` ends up owned by `root` (due to the
sticky bit on `/tmp`), switch to the atomic `renameat2`/`RENAME_EXCHANGE` technique. Once
`/etc/passwd` is modified, `su` into the new account and read `/root/proof_task2.txt`.

**Flag:** the (per-sandbox unique) content of `/root/proof_task2.txt`.

## Level 5: Countermeasure 1 -- Principle of Least Privilege -- 3 points

**Learning outcome:** apply `seteuid()` correctly, and understand that dropping-then-re-raising
privilege around the vulnerable section does not fix anything -- it just reopens the same window.

**Task:** fill in the `TODO` in `/home/seed/lab/vulp_seteuid.c` (drop effective privilege with
`seteuid(getuid())` once, and do not restore it for this operation), then save it. The student does
**not** compile or `chown`/`chmod` it themselves: a root-only background job polls every 10
seconds and, if the file compiles cleanly, installs the result as `/home/seed/lab/vulp_fixed`,
owned `root:root`, Set-UID. Repeat the Level 4 attack against it.

**Flag:** does the attack still succeed? (`yes`/`no` -- correct answer: `no`.)

**Design note (why a background job instead of letting the student `chown`/`chmod` it):** that
would reopen exactly the Level 2 bypass one level later -- the student again fully controls the
source being turned into a root binary. The background job (`compile_fixed.sh`) closes this by
refusing to act until `/etc/passwd` has already diverged from its pristine backup, which can only
happen via a genuine Level 4 exploit (`/etc/passwd` is `root:root 0644` and `seed` has no sudo).
So the automation can never be used as a shortcut to a root binary before Level 4 is actually
solved -- it can only ever follow it.

## Level 6: Countermeasure 2 -- built-in sticky-symlink protection -- 2 points

**Learning outcome:** understand what `fs.protected_symlinks` actually protects against, and its
limits.

**Task:** re-enable the protection with `race-lab-protect-on` (a setuid-root helper, no `sudo`
needed), repeat the Level 4 attack against the *original* `vulp`, and identify the `errno` the
kernel now returns when refusing to follow the symlink (via `strace`).

**Flag:** `ELOOP`.

**Discussion (not separately scored):** this protection only covers symlinks inside
world-writable sticky directories such as `/tmp` -- it does not fix the underlying
check-then-use design flaw, and would not help against the same race expressed via hard links,
directory renames, or files outside a sticky directory.
