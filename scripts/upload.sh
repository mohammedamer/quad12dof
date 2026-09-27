#!/usr/bin/env bash

set -euo pipefail

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/arduino-env.sh"

OPEN_MONITOR=false
for arg in "$@"; do
  case "${arg}" in
    --monitor|-m) OPEN_MONITOR=true ;;
    --help|-h) echo "Usage: $0 [--monitor|-m]"; exit 0 ;;
    *) echo "Unknown argument: ${arg}. Usage: $0 [--monitor|-m]" >&2; exit 2 ;;
  esac
done

require_arduino_cli

PORT="$(arduino_port)"

arduino-cli upload \
  --port "${PORT}" \
  --fqbn "${ARDUINO_FQBN}" \
  "${SKETCH_DIR}"

if [[ "${OPEN_MONITOR}" == true ]]; then
  ARDUINO_PORT="${PORT}" exec "${PROJECT_ROOT}/scripts/monitor.sh"
fi
