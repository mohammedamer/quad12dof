#!/usr/bin/env bash

set -euo pipefail

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/arduino-env.sh"

require_arduino_cli

PORT="$(arduino_port)"

arduino-cli upload \
  --port "${PORT}" \
  --fqbn "${ARDUINO_FQBN}" \
  "${SKETCH_DIR}"
