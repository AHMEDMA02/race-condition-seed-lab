#!/bin/bash
# Root-only background job (systemd timer). NOT setuid, NOT directly
# executable by the student -- triggered automatically.
#
# Gate: only compile+root vulp_fixed once /etc/passwd has actually
# diverged from the pristine backup taken at provisioning time. Since
# /etc/passwd is root-owned 0644 and "seed" has no sudo at all, the
# ONLY way it can diverge is via a successful Level-4 race-condition
# exploit against the real vulp. This stops a student from using this
# automation as a shortcut to a root-owned binary before they have
# actually completed the Level 4 exploit.
set -u

BASELINE=/opt/race-lab/passwd.bak
TARGET=/etc/passwd
SRC=/home/seed/lab/vulp_seteuid.c
OUT=/home/seed/lab/vulp_fixed
LOG=/home/seed/lab/.vulp_fixed_build.log

if diff -q "$TARGET" "$BASELINE" > /dev/null 2>&1; then
    exit 0   # Level 4 not completed yet -- do nothing
fi

[ -f "$SRC" ] || exit 0

TMP=$(mktemp)
if gcc -o "$TMP" "$SRC" > "$LOG" 2>&1; then
    chown root:root "$TMP"
    chmod 4755 "$TMP"
    mv -f "$TMP" "$OUT"
else
    rm -f "$TMP"
fi
chown seed:seed "$LOG" 2>/dev/null || true
