#!/usr/bin/env bash
# Format project sources with clang-format.
#   tools/format.sh          reformat in place
#   tools/format.sh --check  report files that need formatting (exit 1 if any)
set -euo pipefail
cd "$(dirname "$0")/.."
command -v clang-format >/dev/null || { echo "clang-format not found" >&2; exit 2; }
mapfile -t files < <(find src -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.c' -o -name '*.cpp' \) | sort)
if [[ "${1:-}" == "--check" ]]; then
    clang-format --dry-run --Werror "${files[@]}"
else
    clang-format -i "${files[@]}"
    echo "Formatted ${#files[@]} files"
fi
