# Race Condition Vulnerability Lab (SEED) -- CyberRangeCZ scenario

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

A system-security TP for [CyberRangeCZ Platform](https://docs.platform.cyberrange.cz/), adapted from the
**SEED Labs "Race Condition Vulnerability Lab"** by Wenliang Du
(PDF: https://seedsecuritylabs.org/Labs_16.04/PDF/Race_Condition_new.pdf).
Built from the `library-junior-hacker` scenario as a structural template.

Students exploit a TOCTOU race condition in a Set-UID-root program to gain root privilege, then apply
and evaluate two countermeasures (Principle of Least Privilege, and the kernel's built-in sticky-symlink
protection).

## Game Levels Summary

| # | Level | Points | Learning outcome |
|---|-------|:---:|---|
| 0 | Introduction (info) | - | Background on race conditions / TOCTOU, rules, safety net |
| 1 | Get Access | - | Connect to the sandbox |
| 2 | Inspect the vulnerable Set-UID program | 2 | Read the permission bits of a pre-built, pre-rooted `vulp` |
| 3 | Understand the magic password -- safely | 3 | Verify the `/etc/passwd` format and the magic-hash trick with a local, unprivileged `crypt()` check |
| 4 | Launch the race-condition attack | 10 | Exploit the TOCTOU window; fix the *attack's own* race condition with `renameat2`/`RENAME_EXCHANGE` when the naive `unlink()`+`symlink()` approach loses to the sticky-`/tmp` bit |
| 5 | Countermeasure 1: Principle of Least Privilege | 3 | Apply `seteuid()` correctly, understand why "drop and re-raise" still fails |
| 6 | Countermeasure 2: built-in sticky-symlink protection | 2 | `fs.protected_symlinks`, understand its scope/limits |

**Total: 20 points.** Hints cost between **0.5 and 2 points** each and are optional.

## Anti-cheat: only one real door to root, and it's locked behind Level 4

**Design principle:** a flag-based, graded platform is adversarial in a way the original single-user SEED
VM never had to be, so *every* path to real root before, or independent of, the intended exploit had to be
closed, not just the obvious one. Three issues came up during review and were fixed:

1. The original lab has the student `sudo chown root:root` / `sudo chmod 4755` their **own freshly-compiled**
   binary (Task "set up the Set-UID program"). On a graded platform that is a critical bypass: a student
   could put arbitrary code in `vulp.c` instead of the real logic and get an unconditional root shell with no
   race condition at all. **Fix:** `vulp` is now pre-built and pre-rooted by Ansible; the student only
   inspects it (Level 2).
2. The original lab also has the student manually add the magic-password account to the real `/etc/passwd`
   and `su` into it (Task "choosing our target"). That's a genuine root shell too -- which would let a
   student simply `cat` Level 4's flag file directly. **Fix:** Level 3 now proves the same fact (the magic
   hash is a blank password) with a local, unprivileged `crypt.crypt()` check instead of touching the real
   system or ever granting a shell.
3. Even with (1) and (2) fixed, a leftover restricted-sudo grant was itself a residual risk, and a naive
   implementation of Level 5 (compile the seteuid fix, `chown`/`chmod` it to Set-UID-root) would reopen
   exactly the hole from (1), just one level later, since the student again fully controls the source being
   turned into a root binary. **Fix:** `seed` has no sudo at all (see below), and Level 5's compile step runs
   as a **root-only background job that refuses to do anything until `/etc/passwd` has already been modified
   by a real Level 4 exploit** -- see `compile_fixed.sh`. Any root capability a student reaches in Level 5
   or 6 exists only because they already earned real root the intended way, in Level 4; nothing lets them
   reach it earlier or by a different route.

With all three closed, **Level 4 is the only level gated behind real root, and real root is only reachable
by actually performing the race-condition exploit.** Its flag uses CyberRangeCZ's **APG
variant-answers** mechanism (`variables.yml`, `answer_variable_name`, `variant_answers: true`): a random
value is generated independently for *every* allocated sandbox, baked into `/root/proof_task2.txt`
(`root:root 0600`) during provisioning, and stored by the platform as that sandbox's expected answer. A flag
copied from another student, an older run, or a public write-up will not match. See
[CyberRangeCZ APG Variables docs](https://docs.crp.kypo.muni.cz/user-guide-advanced/sandboxes/apg-variables/).

Levels 2, 5 and 6 ask for conceptual/procedural answers (a permission string, a yes/no, an `errno` name)
that are the same for everyone, since they test understanding rather than possession of a secret.

**The `seed` account has *no sudo rights at all*.** Not a restricted whitelist -- none. At the very start of
the `seed` role's provisioning, `seed` is explicitly stripped of any supplementary-group membership (in case
the base image already put it in `sudo`) and any sudoers.d/sudoers line mentioning it is removed and the
result re-validated with `visudo -c`. This matters because the *original* SEED lab assumes the student
already has full sudo on their own single-user VM; on a shared, flag-graded platform that assumption is a
critical bypass (see "Two bypasses that were found and fixed" below).

Every privileged action the lab still legitimately needs is done one of two ways instead:

* **Two narrowly-scoped Set-UID-root helper binaries**, `race-lab-reset` and `race-lab-protect-on`
  (`/usr/local/bin`, mode `4711`, source in `provisioning/roles/seed/files/*.c`). Each is a tiny, pre-compiled
  C program with **zero attacker-controlled input** -- no `argv`, no shelling out, no environment
  dependence, one hardcoded action apiece (restore `/etc/passwd` from a pristine backup and clear
  `/tmp/XYZ`/`/tmp/ABC`; or flip `fs.protected_symlinks` to `1`). The student runs them directly, no `sudo`
  needed, and nothing about their behaviour can be redirected.
* **A root-only background job** (`compile_fixed.sh`, run every 10s by a systemd timer, never executable by
  `seed`) that builds Level 5's `vulp_fixed` from the student's edited source. See the bypass write-up below
  for why this is gated.

## Topology summary

| Host | Image | Flavor |
|---|---|---|
| seed | seed | m1.medium |
| router | debian-12-x86_64 | standard.small |

This lab is entirely local to one machine (no network pivoting is involved in the original SEED lab), so the
topology was simplified to a single `seed` host, replacing the `attacker`/`server`/`client` split of the
`library-junior-hacker` template. A `router` + WAN are still included, matching the platform's standard
requirement for sandbox connectivity. **Assumption:** `image: seed` refers to a pre-built SEED-Ubuntu image
already available in your platform's image catalog, as requested; if your catalog uses a different image
name/tag, update `topology.yml` accordingly.

## Repository layout

```
.
├── README.md
├── game_design.md          # narrative/level text in prose form
├── ATTRIBUTION.txt
├── LICENSE
├── topology.yml
├── variables.yml            # APG variant-answer definition (flag2_secret)
├── training.json            # CyberRangeCZ training definition (7 levels, 20 points total)
└── provisioning/
    ├── playbook.yml
    ├── requirements.yml
    ├── host_vars/seed.yml
    └── roles/
        ├── hosts/           # common packages for all hosts
        └── seed/            # the lab target machine
            ├── tasks/main.yml                      # strips seed's sudo FIRST, then provisions everything
            ├── vars/main.yml
            └── files/
                ├── vulp.c                          # the vulnerable Set-UID source (Level 2), pre-built by Ansible
                ├── vulp_seteuid_skeleton.c          # starter file for Level 5
                ├── race_lab_reset.c                 # setuid-root helper: restore passwd.bak, clear /tmp/XYZ+ABC
                ├── race_lab_protect_on.c             # setuid-root helper: enable fs.protected_symlinks
                ├── compile_fixed.sh                  # root-only job: builds vulp_fixed, gated on Level 4 being solved
                ├── race-lab-compile-fixed.service    # systemd unit for the job above
                └── race-lab-compile-fixed.timer      # runs it every 10s
```

## Credentials

* `seed` / `dees` -- the student account on the `seed` host (narrow sudo rights only, see above).

## Open items / please confirm before release

1. **Image name.** Confirm `seed` is the correct catalog image name/tag for your platform (swapped in for
   `kali` per your request).
2. **`variables.yml` wiring.** I've used the documented APG mechanism (`variables.yml` + `answer_variable_name`
   + `variant_answers: true`). Please do a test allocation of 2 sandboxes from a pool and confirm
   `/root/proof_task2.txt` differs between them and that each sandbox's training run only accepts its own
   value, since I could not do a live test against your specific CyberRangeCZ instance.
3. **Sudo stripping on your actual `seed` image.** The task that removes `seed` from the `sudo` group and
   scrubs sudoers.d is generic (it doesn't assume a specific file name), and I validated the logic of
   `compile_fixed.sh` and both setuid helpers end-to-end in a sandbox here (compiled them, ran them as an
   unprivileged test user, confirmed `race-lab-reset`/`race-lab-protect-on` work without root, and confirmed
   `compile_fixed.sh` refuses to build `vulp_fixed` until `/etc/passwd` has actually changed). Please still run
   `sudo -l` as `seed` on one real deployed sandbox and confirm it reports "not allowed to run sudo" (or
   equivalent), since I could not test against your actual base image.
4. **Systemd timer delay.** Level 5's `vulp_fixed` can take up to ~10 seconds to appear after an edit (timer
   interval in `race-lab-compile-fixed.timer`); shorten `OnUnitActiveSec` if you want it snappier.
5. Let me know if you'd like a networked variant (e.g. the vulnerable program reachable only after an initial
   foothold on a second host) instead of this single-machine design, or if you want the level count/scoring
   split differently.

## License

* Code (topology, Ansible, helper scripts) in this repository: MIT License (see `LICENSE`).
* Task/level text is adapted from SEED Labs' "Race Condition Vulnerability Lab" (c) Wenliang Du, which is
  **free for non-commercial educational use only** (commercial use is prohibited by the original license).
  See `ATTRIBUTION.txt`. This restriction applies to the adapted lab narrative/instructions even though the
  surrounding code is MIT-licensed.
