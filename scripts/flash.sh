#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

for arg in "$@"; do
  case "${arg}" in
    --monitor|-m) ;;
    --help|-h) echo "Usage: $0 [--monitor|-m]"; exit 0 ;;
    *) echo "Unknown argument: ${arg}. Usage: $0 [--monitor|-m]" >&2; exit 2 ;;
  esac
done

"${SCRIPT_DIR}/build.sh"
exec "${SCRIPT_DIR}/upload.sh" "$@"
