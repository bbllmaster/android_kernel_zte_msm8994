#!/usr/bin/env bash
#
# Import the ZTE A2015 Marshmallow kernel tarball into this repository.
#
# Usage:
#   ./scripts/import-zte-kernel.sh /path/to/ZTE_A2015_Marshmallow_Kernel(3.10.84).tar.gz
#
set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "usage: $0 /path/to/ZTE_A2015_Marshmallow_Kernel(3.10.84).tar.gz" >&2
    exit 1
fi

TARBALL="$1"
if [ ! -f "$TARBALL" ]; then
    echo "not a file: $TARBALL" >&2
    exit 1
fi

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

tar xzf "$TARBALL" -C "$TMP"
SRC="$(find "$TMP" -maxdepth 5 -type d -name ZTE_A2015_Marshmallow_Kernel | head -n 1)"

if [ -z "$SRC" ]; then
    echo "could not find ZTE_A2015_Marshmallow_Kernel in the tarball" >&2
    exit 1
fi

if command -v rsync >/dev/null 2>&1; then
    rsync -a --exclude .git --exclude README --exclude README.md "$SRC"/ "$ROOT"/
else
    (cd "$SRC" && tar cf - --exclude=.git --exclude=README --exclude=README.md .) | (cd "$ROOT" && tar xf -)
fi

echo "imported kernel 3.10.84 into $ROOT"
echo "next: git add -A && git commit -m 'Import ZTE A2015 3.10.84 kernel source'"
