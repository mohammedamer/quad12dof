#!/usr/bin/env bash

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

SKETCH_DIR="${SKETCH_DIR:-${PROJECT_ROOT}/src/quadruped}"
ARDUINO_FQBN="${ARDUINO_FQBN:-esp32:esp32:adafruit_feather_esp32s3}"
ARDUINO_PORT="${ARDUINO_PORT:-}"
ARDUINO_BAUD="${ARDUINO_BAUD:-115200}"

require_arduino_cli() {
  if ! command -v arduino-cli >/dev/null 2>&1; then
    echo "error: arduino-cli is not installed or is not on PATH" >&2
    exit 1
  fi
}

arduino_port() {
  if [[ -n "${ARDUINO_PORT}" ]]; then
    echo "${ARDUINO_PORT}"
    return
  fi

  local port
  port="$(arduino-cli board list | awk -v fqbn="${ARDUINO_FQBN}" '$0 ~ fqbn { print $1; exit }')"

  if [[ -z "${port}" ]]; then
    echo "error: no connected board found for ${ARDUINO_FQBN}" >&2
    echo "hint: run scripts/board-list.sh or set ARDUINO_PORT=/dev/cu... explicitly" >&2
    exit 1
  fi

  echo "${port}"
}
